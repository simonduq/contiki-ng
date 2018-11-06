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
#include "shell.h"
#include "shell-commands.h"
#include "sensors.h"
#include "bmp-280-sensor.h"
#include "si1133-sensor.h"
#include "rgbleds.h"

static struct shell_command_set_t efr32_commands;
extern int autoblink;
void
efr_cmds_init(void)
{
  shell_command_set_register(&efr32_commands);
}

static
PT_THREAD(cmd_sensor(struct pt *pt, shell_output_func output, char *args))
{
  char *name;
  char *next_args;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);

  /* Get first arg  */
  SHELL_ARGS_NEXT(name, next_args);

  /* no args... just print status */
  if(name == NULL || !strcmp(args, "help")) {
    SHELL_OUTPUT(output, "Usage: sensor [light, temp, pressure]\n");
  } else if(!strcmp(name, "light")) {
    int lux;
    lux = si1133_sensor.value(SI1133_SENSOR_TYPE_LUX);
    SHELL_OUTPUT(output, "Light: %d.%03d\n", (int) (lux / 1000),
                 (int) (lux % 1000));
  } else if(!strcmp(name, "temp")) {
    int temp;
    temp = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_TEMP);
    SHELL_OUTPUT(output, "Temp: %d.%03d\n", (int) (temp / 1000),
                 (int) (temp % 1000));
  } else if(!strcmp(name, "pressure")) {
    int pressure;
    pressure = bmp_280_sensor.value(BMP_280_SENSOR_TYPE_PRESS);
    SHELL_OUTPUT(output, "Pressure: %d.%03d\n", (int) (pressure / 1000),
                 (int) (pressure % 1000));
  } else {
    SHELL_OUTPUT(output, "Invalid sensor: %s\n", name);
  }

  PT_END(pt);
}

static
PT_THREAD(cmd_rgbleds(struct pt *pt, shell_output_func output, char *args))
{
  char *ledstr;
  char *next_args;
  int led;
  int color;

  PT_BEGIN(pt);

  SHELL_ARGS_INIT(args, next_args);

  /* Get first arg  */
  SHELL_ARGS_NEXT(ledstr, next_args);
  /* Get second arg  */
  SHELL_ARGS_NEXT(args, next_args);

  /* no args... just print status */
  if(args == NULL || ledstr == NULL|| !strcmp(args, "help")) {
    SHELL_OUTPUT(output, "Usage: rgbleds [led color]\n");
  } else if(shell_dectoi(ledstr, &led) == ledstr) {
    SHELL_OUTPUT(output, "Please specify led number.\n");
  } else if(shell_dectoi(args, &color) == args) {
    SHELL_OUTPUT(output, "Please specify color (number).\n");
  } else {
    rgbleds_enable(led & 0xf);
    SHELL_OUTPUT(output, "Setting LEDs %d to 0x%x.\n", led, color);
    rgbleds_setcolor((color >> 16 & 0xff) << 8,
                     (color >> 8 & 0xff) << 8,
                     (color & 0xff) << 8);
    autoblink = led == 0;
  }

  PT_END(pt);
}


const struct shell_command_t efr32_shell_commands[] = {
  { "sensor",           cmd_sensor,           "'> sensor <name>': reads out data from sensors (light, temp, pressure)" },

  { "rgbleds",           cmd_rgbleds,           "'> rgbleds [on/off led color]': sets color of RGB leds (no args turns on autoblink)" },
  { NULL, NULL, NULL }
};

static struct shell_command_set_t efr32_commands = {
  .next = NULL,
  .commands = efr32_shell_commands,
};
