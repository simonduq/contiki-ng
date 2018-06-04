/*
 * Copyright (c) 2018, Joakim Eriksson, RISE SICS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */
/*---------------------------------------------------------------------------*/
#if BOARD_TB_SENSE2
#include "contiki.h"
#include "services/lwm2m/lwm2m-engine.h"
#include "services/lwm2m/lwm2m-rd-client.h"
#include "services/ipso-objects/ipso-objects.h"
#include "services/ipso-objects/ipso-sensor-template.h"
#include "services/ipso-objects/ipso-control-template.h"
#include "leds.h"
#include "lib/sensors.h"
#include "bmp-280-sensor.h"

/* Temperature reading */
static lwm2m_status_t
read_temp_value(const ipso_sensor_t *s, int32_t *value)
{
  int temp = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_TEMP);
  *value = temp;
  return LWM2M_STATUS_OK;
}
/* Barometer reading */
static lwm2m_status_t
read_bar_value(const ipso_sensor_t *s, int32_t *value)
{
  uint32_t pressure;
  pressure = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
  *value = pressure;
  return LWM2M_STATUS_OK;
}

/* LED RED control */
static lwm2m_status_t
leds_set_red(ipso_control_t *control, uint8_t value)
{
  if(value > 0) {
    leds_on(LEDS_RED);
  } else {
    leds_off(LEDS_RED);
  }
  return LWM2M_STATUS_OK;
}

/* LED GREEN control */
static lwm2m_status_t
leds_set_green(ipso_control_t *control, uint8_t value)
{
  if(value > 0) {
    leds_on(LEDS_GREEN);
  } else {
    leds_off(LEDS_GREEN);
  }
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

IPSO_CONTROL(led_control_red, 3311, 0, leds_set_red);
IPSO_CONTROL(led_control_green, 3311, 1, leds_set_green);

IPSO_SENSOR(temp_sensor, 3303, read_temp_value,
            .max_range = 100000, /* 100 cel milli celcius */
            .min_range = -10000, /* -10 cel milli celcius */
            .unit = "Cel",
            .update_interval = 30
            );

IPSO_SENSOR(bar_sensor, 3315, read_bar_value,
            .max_range = 1200000, /* 1200 bar */
            .min_range = -10000, /* -800 bar */
            .unit = "hPa",
            .update_interval = 30
            );
void
board_ipso_sensors_init(void)
{
  ipso_sensor_add(&temp_sensor);
  ipso_sensor_add(&bar_sensor);
  ipso_control_add(&led_control_red);
  ipso_control_add(&led_control_green);
  ipso_button_init();
}

void
board_ipso_sensors_periodic(void)
{
  /* nothing right now. */
}
#endif /* BOARD_TB_SENSE2 */
