/*
 * Copyright (c) 2018, RISE SICS AB
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Joakim Eriksson, joakim.eriksson@ri.se
 *
 * Minimal implementation of clock functions needed by Contiki-NG.
 */

#include <em_device.h>
#include <em_cmu.h>
#include <em_timer.h>
#include <em_core.h>
#include "sys/energest.h"

#include "rtimer-arch.h"
/*---------------------------------------------------------------------------*/
void TIMER0_IRQHandler(void) __attribute__((interrupt));

static volatile rtimer_clock_t g_rticks = 0;
static volatile rtimer_clock_t g_next = 0;
/*---------------------------------------------------------------------------*/
void
TIMER0_IRQHandler(void)
{
  ++g_rticks;
  if(g_rticks == g_next) {
    g_next = 0;
    rtimer_run_next();
  }
  TIMER_IntClear(TIMER0, TIMER_IF_OF);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  CMU_ClockEnable(cmuClock_HFPER, true);
  CMU_ClockEnable(cmuClock_TIMER0, true);

  TIMER_IntClear(TIMER0, TIMER_IF_OF);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_SetPriority(TIMER0_IRQn, 0);
  TIMER_IntEnable(TIMER0, TIMER_IF_OF);
  NVIC_EnableIRQ(TIMER0_IRQn);

  TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
  TIMER_Init(TIMER0, &init);
  TIMER_TopSet(TIMER0, CMU_ClockFreqGet(cmuClock_HFPER) / RTIMER_ARCH_SECOND);
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now(void)
{
  rtimer_clock_t now;
  CORE_ATOMIC_SECTION(
    now = g_rticks;
  )
  return now;
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  CORE_ATOMIC_SECTION(
    g_next = t;
  )
}
/*---------------------------------------------------------------------------*/
