/*
 *  Copyright (c) 2017, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for UART communication.
 *
 */

#include "contiki.h"
#include <stddef.h>
#include "em_core.h"
#include "uartdrv.h"
#include <string.h>

#define USART_INIT                                                      \
  {                                                                     \
      USART0,                                   /* USART port */        \
      115200,                                 /* Baud rate */           \
      BSP_SERIAL_APP_TX_LOC,          /* USART Tx pin location number */ \
      BSP_SERIAL_APP_RX_LOC,          /* USART Rx pin location number */ \
      (USART_Stopbits_TypeDef)USART_FRAME_STOPBITS_ONE, /* Stop bits */ \
      (USART_Parity_TypeDef)USART_FRAME_PARITY_NONE,    /* Parity */    \
      (USART_OVS_TypeDef)USART_CTRL_OVS_X16,            /* Oversampling mode*/ \
      false,                                            /* Majority vote disable */ \
      uartdrvFlowControlNone,                         /* Flow control */ \
      BSP_SERIAL_APP_CTS_PORT,                          /* CTS port number */ \
      BSP_SERIAL_APP_CTS_PIN,                           /* CTS pin number */ \
      BSP_SERIAL_APP_RTS_PORT,                          /* RTS port number */ \
      BSP_SERIAL_APP_RTS_PIN,                           /* RTS pin number */ \
      (UARTDRV_Buffer_FifoQueue_t *)&sUartRxQueue,      /* RX operation queue */ \
      (UARTDRV_Buffer_FifoQueue_t *)&sUartTxQueue,      /* TX operation queue */ \
      BSP_SERIAL_APP_CTS_LOC,                           /* CTS location */ \
      BSP_SERIAL_APP_RTS_LOC                            /* RTS location */ \
}

DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, sUartRxQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, sUartTxQueue);

static UARTDRV_HandleData_t sUartHandleData;
static UARTDRV_Handle_t     sUartHandle = &sUartHandleData;
static uint8_t              sReceiveBuffer[2];
static volatile uint8_t     txbuzy = 0;
static int (* input_handler)(unsigned char c);

#define SLIP_END     0300

#define TX_SIZE 2048
static uint8_t tx_buf[TX_SIZE];
/* first char to read */
static uint16_t rpos = 0;
/* last char written (or next to write) */
static uint16_t wpos = 0;

/* The process for receiving packets */
PROCESS(serial_proc, "efr32 serial driver");
/*---------------------------------------------------------------------------*/
static void
receiveDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
  if(input_handler != NULL) {
    input_handler(aData[0]);
  }
  UARTDRV_Receive(aHandle, aData, 1, receiveDone);
}
/*---------------------------------------------------------------------------*/
static void
transmitDone(UARTDRV_Handle_t aHandle, Ecode_t aStatus, uint8_t *aData, UARTDRV_Count_t aCount)
{
  (void) aHandle;
  (void) aStatus;
  (void) aData;
  (void) aCount;
  txbuzy = 0;
  process_poll(&serial_proc);
}
/*---------------------------------------------------------------------------*/
void
dbg_init(void)
{
  UARTDRV_Init_t uartInit = USART_INIT;

#ifdef SERIAL_BAUDRATE
  uartInit.baudRate = SERIAL_BAUDRATE;
#endif /* SERIAL_BAUDRATE */

  UARTDRV_Init(sUartHandle, &uartInit);

  process_start(&serial_proc, NULL);

  for(uint8_t i = 0; i < sizeof(sReceiveBuffer); i++) {
    UARTDRV_Receive(sUartHandle, &sReceiveBuffer[i],
                    sizeof(sReceiveBuffer[i]), receiveDone);
  }
}
/*---------------------------------------------------------------------------*/
static unsigned char buf[64];

void
dbg_flush(void)
{
  /* send max 64 per transmit */
  int len = 0;

  if(txbuzy) {
    process_poll(&serial_proc);
    return;
  }

  if(wpos == rpos) {
    /* nothing to read */
    return;
  }

  /* did we wrap? */
  if(wpos < rpos) {
    len = TX_SIZE - rpos;
  } else {
    /* wpos > rpos */
    len = wpos - rpos;
  }
  if(len > 64) {
    len = 64;
  }
  memcpy(buf, &tx_buf[rpos], len);
  rpos = (rpos + len) % TX_SIZE;

  /* transmission ongoing... */
  txbuzy = 1;
  UARTDRV_Transmit(sUartHandle, (uint8_t *)buf, len, transmitDone);
}

/*---------------------------------------------------------------------------*/
void
dbg_write_byte(const char ch)
{
  /* This will drop bytes if too many printed too soon */
  if(rpos == (wpos + 1) % TX_SIZE) {
    /* Overflow - drop data */
  } else {
    tx_buf[wpos] = ch;
    wpos = (wpos + 1) % TX_SIZE;
  }
  process_poll(&serial_proc);
}
/*---------------------------------------------------------------------------*/
void
dbg_putchar(const char ch)
{
#if SLIP_ARCH_CONF_ENABLED
  static char debug_frame = 0;

  if(!debug_frame) {
    dbg_write_byte(SLIP_END);
    dbg_write_byte('\r');
    debug_frame = 1;
  }
#endif /* SLIP_ARCH_CONF_ENABLED */

  dbg_write_byte(ch);

  if(ch == '\n') {
#if SLIP_ARCH_CONF_ENABLED
    dbg_write_byte(SLIP_END);
    debug_frame = 0;
#endif /* SLIP_ARCH_CONF_ENABLED */
    dbg_flush();
  }
}
/*---------------------------------------------------------------------------*/
unsigned int
dbg_send_bytes(const unsigned char *seq, unsigned int len)
{
  /* how should we handle this to not get trashed data... */
  int i;
  for(i = 0; i < len; i++) {
    dbg_putchar(seq[i]);
  }
  dbg_flush();
  return len;
}
/*---------------------------------------------------------------------------*/
void
dbg_set_input_handler(int (* handler)(unsigned char c))
{
  input_handler = handler;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(serial_proc, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT();
    dbg_flush();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
