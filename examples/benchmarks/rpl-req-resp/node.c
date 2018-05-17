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
#define LOG_LEVEL LOG_LEVEL_DBG

#define SEND_INTERVAL (CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
PROCESS(app_process, "App process");
AUTOSTART_PROCESSES(&app_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  if(node_id == ROOT_ID) {
    /* We are the root, start a DAG */
    NETSTACK_ROUTING.root_start();
    /* Set dest_ipaddr with DODAG ID, so we get the prefix */
    NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);
    /* Wait until all nodes have joined */
    do {
      uip_sr_node_t *link;

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);

      if(deployment_node_cont() > NETSTACK_MAX_ROUTE_ENTRIES) {
        LOG_WARN("Not enough routing entries for deployment: %u/%u\n", deployment_node_cont(), NETSTACK_MAX_ROUTE_ENTRIES);
      }
      LOG_INFO("Node count: %u/%u\n", uip_sr_num_nodes(), deployment_node_cont());

      /* Print our routing */
      LOG_DBG("Reachable nodes (%u in total):\n", uip_sr_num_nodes());
      link = uip_sr_node_head();
      while(link != NULL) {
        uip_ipaddr_t node_ipaddr;
        NETSTACK_ROUTING.get_sr_node_ipaddr(&node_ipaddr, link);
        LOG_DBG("-- ");
        LOG_DBG_6ADDR(&node_ipaddr);
        LOG_DBG_("\n");
        link = uip_sr_node_next(link);
      }
    } while(uip_sr_num_nodes() < deployment_node_cont());

    /* Now start requesting nodes at random */
    etimer_set(&timer, SEND_INTERVAL);
    while(1) {
      static unsigned count = 0;
      uint16_t dest_id;

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);

      /* Select a destination at random. Iterate until we do not select ourselve */
      do {
        dest_id = deployment_id_from_index(random_rand() % deployment_node_cont());
      } while(dest_id == ROOT_ID);
      /* Prefix was already set, set IID now */
      deployment_iid_from_id(&dest_ipaddr, dest_id);

      LOG_INFO("Sending to ");
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_(", count %u\n", count);
      count++;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
