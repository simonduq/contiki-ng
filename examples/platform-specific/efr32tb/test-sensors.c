/*
 * Copyright (c) 2018, RISE SICS AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "dev/button-sensor.h"
#include "bmp-280-sensor.h"
#include "si1133-sensor.h"
#include "ccs811.h"
#include "rgbleds.h"
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
void efr_cmds_init(void);

PROCESS(test_process, "BMP280 tester");
AUTOSTART_PROCESSES(&test_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  static struct etimer periodic_timer;
  static int enable = 0;
  static int ctr;
  int32_t temp;
  int32_t lux;
  uint32_t pressure;

  PROCESS_BEGIN();

  efr_cmds_init();

  etimer_set(&periodic_timer, CLOCK_SECOND);
  while(true) {
    PROCESS_WAIT_EVENT();

    if(ev == PROCESS_EVENT_TIMER) {
      ctr++;

      if(ctr == 30) {
        lux = si1133_sensor.value(SI1133_SENSOR_TYPE_LUX);
        temp = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_TEMP);
        pressure = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);

        LOG_INFO("Time: %6lu  Light: %d.%03d Temp: %2"PRId32".%03"PRId32"  Pressure: %4"PRIu32".%03"PRIu32"  BLeft:%d\n",
                 (unsigned long)clock_time(),
                 (int) (lux / 1000), (int) (lux % 1000),
                 (temp / 1000), (temp % 1000),
                 (pressure / 1000), (pressure % 1000),
                 button_left_sensor.value(0));
        ctr = 0;
      }
      /* Blink a bit with the RGB leds */
      rgbleds_enable(enable++ & 0xf);
      rgbleds_setcolor((clock_time() >> 0) & 0xffff,
                       (clock_time() >> 2) & 0xffff,
                       (clock_time() >> 4) & 0xffff);

      etimer_restart(&periodic_timer);
    }

    if(ev == sensors_event) {
      struct sensors_sensor *sensor;
      sensor = data;
      LOG_INFO("%s triggered!\n", sensor->type);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
