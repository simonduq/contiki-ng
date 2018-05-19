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
 * Implementation of watchdog.
 */
#include "contiki.h"
#include "dev/watchdog.h"
#include "em_wdog.h"
#include <stdio.h>

void watchdog_init(void)
{
  WDOG_Init_TypeDef init = WDOG_INIT_DEFAULT;
  /* set 16 seconds watchdog interval - default is 256 seconds */
  init.perSel = wdogPeriod_16k;
  WDOGn_Init(WDOG0, &init);
}
void watchdog_start(void)
{
  WDOGn_Enable(WDOG0, true);
}
void watchdog_periodic(void)
{
  WDOGn_Feed(WDOG0);
}
void watchdog_stop(void)
{
  WDOGn_Enable(WDOG0, false);
}

void watchdog_reboot(void)
{
  watchdog_stop();
  WDOG_Init_TypeDef init = WDOG_INIT_DEFAULT;
  init.perSel = wdogPeriod_9;
  WDOGn_Init(WDOG0, &init);
  watchdog_start();
  while(1);
}
