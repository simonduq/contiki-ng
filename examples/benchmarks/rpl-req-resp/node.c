/*
 * Copyright (c) 2018, RISE SICS.
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
 *         Benchmark: the root sends requests to all nodes in a randomized
 *         order, and receives resopnses back.
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "services/deployment/deployment.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT 8214
#define SEND_INTERVAL (CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
struct app_payload {
  unsigned id;
  unsigned is_request;
};

/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);

/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  struct app_payload message;
  memcpy(&message, data, sizeof(message));

  if(message.is_request) {
    LOG_INFO("Received request %u from ", message.id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    message.is_request = 0;
    LOG_INFO("Sending response %u to ", message.id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    simple_udp_sendto(&udp_conn, &message, sizeof(message), sender_addr);
  } else {
    LOG_INFO("Received response %u from ", message.id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL,
                      UDP_PORT, udp_rx_callback);

  if(node_id == ROOT_ID) {
    /* Wait 2 seconds before starting */
    etimer_set(&timer, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    /* We are the root, start a DAG */
    NETSTACK_ROUTING.root_start();
    /* Set dest_ipaddr with DODAG ID, so we get the prefix */
    NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);
    /* Setup a periodic timer that expires after 10 seconds. */
    etimer_set(&timer, CLOCK_SECOND * 10);
    /* Wait until all nodes have joined */
    do {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);

      if(deployment_node_cont() > NETSTACK_MAX_ROUTE_ENTRIES) {
        LOG_WARN("Not enough routing entries for deployment: %u/%u\n", deployment_node_cont(), NETSTACK_MAX_ROUTE_ENTRIES);
      }
      LOG_INFO("Node count: %u/%u\n", uip_sr_num_nodes(), deployment_node_cont());

    } while(uip_sr_num_nodes() < deployment_node_cont());

    /* Now start requesting nodes at random (and stop if nodes disconnect) */
    etimer_set(&timer, SEND_INTERVAL);
    while(uip_sr_num_nodes() == deployment_node_cont()) {
      static unsigned count = 0;
      uint16_t dest_id;
      struct app_payload message;

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);

      /* Select a destination at random. Iterate until we do not select ourselve */
      do {
        dest_id = deployment_id_from_index(random_rand() % deployment_node_cont());
      } while(dest_id == ROOT_ID);
      /* Prefix was already set, set IID now */
      deployment_iid_from_id(&dest_ipaddr, dest_id);

      message.id = count;
      message.is_request = 1;
      LOG_INFO("Sending request %u to ", message.id);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      simple_udp_sendto(&udp_conn, &message, sizeof(message), &dest_ipaddr);
      count++;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
