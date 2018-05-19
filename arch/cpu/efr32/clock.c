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
#include "contiki.h"
#include <stdint.h>
#include "em_core.h"
#include "rail.h"

void SysTick_Handler(void) __attribute__ ((interrupt));

uint32_t low_tick;
uint32_t hi_tick;

#if CLOCK_SECOND != 1000
#error CLOCK_SECOND needs to be 1000
#endif

void SysTick_Handler(void)
{
  /* Keep etimers triggering */
  if(etimer_pending() && (etimer_next_expiration_time() <= clock_time())) {
    etimer_request_poll();
  }
}

void clock_init(void)
{
  hi_tick = low_tick = 0;
  /* keep etimers checked 100 times per second. */
  SysTick_Config(SystemCoreClockGet() / 100);
}

clock_time_t clock_time(void)
{
  uint32_t now;
  now = RAIL_GetTime();
  CORE_ATOMIC_SECTION(
     if(low_tick > now) {
       hi_tick++;
     }
     low_tick = now;
  );
  return (clock_time_t) ((((uint64_t) hi_tick) << 32) |
                          ((uint64_t) low_tick)) / 1000;
}

unsigned long clock_seconds(void)
{
  return clock_time() / CLOCK_SECOND;
}

/* ignored for now */
void clock_set_seconds(unsigned long sec)
{
}


void clock_wait(clock_time_t t)
{
  clock_time_t now;
  now = clock_time();
  t = t + now;
  while((now = clock_time()) < t);
}

void clock_delay_usec(uint16_t dt)
{
  uint32_t target;
  target = RAIL_GetTime() + dt;
  while(RAIL_GetTime() < target);
}
