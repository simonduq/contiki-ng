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

/**
 * \file
 *         Simple test of IP address parsing/printing
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "dev/serial-line.h"
#include "net/ipv6/uiplib.h"
#include <stdio.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(ipaddr_conv_process, "ipaddr-conv");
AUTOSTART_PROCESSES(&ipaddr_conv_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ipaddr_conv_process, ev, data)
{
  const char *line;
  uip_ipaddr_t addr;
  char buffer[80];
  int i;

  PROCESS_BEGIN();

  printf("\nEnter an IPv6 address:\n");
  while(1) {
    printf("> ");

    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    line = data;

    if(*line != NULL) {
      printf(" \"%s\" => ", line);
      if(!uiplib_ipaddrconv(line, &addr)) {
        printf("could not parse as IP address\n");
      } else {
        for(i = 0; i < 16; i += 2) {
          if(i > 0) {
            printf(":");
          }
          printf("%02x%02x", addr.u8[i], addr.u8[i + 1]);
        }
        printf("\n");

        printf("    LOG:    \"");
        LOG_INFO_6ADDR(&addr);
        printf("\"\n");

        printf("    sprint: \"");
        if(uiplib_ipaddr_snprint(buffer, sizeof(buffer), &addr)) {
          printf("%s", buffer);
        } else {
          printf("ERROR");
        }
        printf("\"\n");

        printf("    print:  \"");
        uiplib_ipaddr_print(&addr);
        printf("\"\n");
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
