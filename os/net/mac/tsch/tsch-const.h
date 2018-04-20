/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         TSCH configuration
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 */

/**
 * \addtogroup tsch
 * @{
*/

#ifndef __TSCH_CONST_H__
#define __TSCH_CONST_H__

/********** Includes **********/
#include "net/packetbuf.h"
#include "net/mac/tsch/tsch-conf.h" /* Required for timestlot timing consts */

/********** Constants **********/

/* Link options */
#define LINK_OPTION_TX              1
#define LINK_OPTION_RX              2
#define LINK_OPTION_SHARED          4
#define LINK_OPTION_TIME_KEEPING    8

/* Default IEEE 802.15.4e hopping sequences, obtained from https://gist.github.com/twatteyne/2e22ee3c1a802b685695 */
/* 16 channels, sequence length 16 */
#define TSCH_HOPPING_SEQUENCE_16_16 (uint8_t[]){ 16, 17, 23, 18, 26, 15, 25, 22, 19, 11, 12, 13, 24, 14, 20, 21 }
/* 4 channels, sequence length 16 */
#define TSCH_HOPPING_SEQUENCE_4_16 (uint8_t[]){ 20, 26, 25, 26, 15, 15, 25, 20, 26, 15, 26, 25, 20, 15, 20, 25 }
/* 4 channels, sequence length 4 */
#define TSCH_HOPPING_SEQUENCE_4_4 (uint8_t[]){ 15, 25, 26, 20 }
/* 2 channels, sequence length 2 */
#define TSCH_HOPPING_SEQUENCE_2_2 (uint8_t[]){ 20, 25 }
/* 1 channel, sequence length 1 */
#define TSCH_HOPPING_SEQUENCE_1_1 (uint8_t[]){ 20 }

/* Max TSCH packet lenght */
#define TSCH_PACKET_MAX_LEN MIN(127, PACKETBUF_SIZE)

/* The jitter to remove in ticks.
 * This should be the sum of measurement errors on Tx and Rx nodes.
 * */
#define TSCH_TIMESYNC_MEASUREMENT_ERROR US_TO_RTIMERTICKS(32)

/* Calculate packet tx/rx duration in rtimer ticks based on sent
 * packet len in bytes with 802.15.4 250kbps data rate.
 * One byte = 32us. Add two bytes for CRC and one for len field */
#define TSCH_PACKET_DURATION(len) US_TO_RTIMERTICKS(32 * ((len) + 3))

/* Link-layer preamble+SFD duration. 4 bytes preamble + SFD at 250 kbps takes 160 usec */
#define TSCH_PREAMBLE_SFD_DURATION 160

/* Convert rtimer ticks to clock and vice versa */
#define TSCH_CLOCK_TO_TICKS(c) (((c) * RTIMER_SECOND) / CLOCK_SECOND)
#define TSCH_CLOCK_TO_SLOTS(c, timeslot_length) (TSCH_CLOCK_TO_TICKS(c) / timeslot_length)

/* The IEEE 802.15.4-2015 default timings. To enable configurable guard times,
 * We compute RxOffset instead of using 1020 as in the specification. We also
 * account for the duration of the preamble and SFD, by waking up
 * TSCH_PREAMBLE_SFD_DURATION earlier */
static const uint16_t tsch_timing_10ms[] = {
 1800, /* CcaOffset */
  128, /* Cca */
 2120, /* TxOffset */
 2120 - TSCH_PREAMBLE_SFD_DURATION - (TSCH_CONF_RX_WAIT / 2), /* RxOffset */
  800, /* RxAckDelay */
 1000, /* TxAckDelay */
 TSCH_CONF_RX_WAIT, /* RxWait */
  400, /* AckWait */
  192, /* RxTx */
 2400, /* MacAck */
 4256, /* MaxTx */
10000 /* TimeslotLength */
};

/* Former IETF 6TiSCH defaults for 15ms slots, still used by cc2420 platforms */
static const uint16_t tsch_timing_15ms[] = {
 1800, /* CcaOffset */
  128, /* Cca */
 4000, /* TxOffset */
 4000 - TSCH_PREAMBLE_SFD_DURATION - (TSCH_CONF_RX_WAIT / 2), /* RxOffset */
 3600, /* RxAckDelay */
 4000, /* TxAckDelay */
 TSCH_CONF_RX_WAIT, /* RxWait */
  800, /* AckWait */
 2072, /* RxTx */
 2400, /* MacAck */
 4256, /* MaxTx */
15000 /* TimeslotLength */
};

#endif /* __TSCH_CONST_H__ */
/** @} */
