/*
 * Copyright (c) 2014, Swedish Institute of Computer Science.
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
 */
/**
 * \file
 *         Code managing id<->mac address<->IPv6 address mapping, and doing this
 *         for different deployment scenarios: Cooja, Nodes, Indriya or Twist testbeds
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "deployment.h"
#include <string.h>
#include <stdio.h>

uint16_t node_id;

/* ID<->MAC address mapping */
struct id_mac {
  uint16_t id;
  linkaddr_t mac;
};

#if TABLETOP_TEST
static const struct id_mac id_mac_list[] = {
    { 1, {{0x00,0x12,0x4b,0x00,0x14,0xb5,0xde,0xef}}},
    { 2, {{0x00,0x12,0x4b,0x00,0x14,0xb5,0xde,0xd4}}},
    { 0, {{0}}}
};
#else
/* List of ID<->MAC mapping used for different deployments */
static const struct id_mac id_mac_list[] = {
    { 16, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb4,0x49}}},
    { 3, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb4,0x35}}},
    { 17, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x35}}},
    { 24, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb6,0x00}}},
    { 2, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xe7}}},
    { 9, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xc6}}},
    { 8, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x91}}},
    { 7, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x29}}},
    { 4, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xcf}}},
    { 6, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x5f}}},
    { 13, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb6,0x0d}}},
    { 10, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x63}}},
    { 5, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb2,0x06}}},
    { 20, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xa8}}},
    { 12, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb2,0x03}}},
    { 19, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb5,0xfc}}},
    { 15, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb5,0x8a}}},
    { 14, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb5,0x66}}},
    { 18, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb2,0x15}}},
    { 11, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb4,0x3b}}},
    { 25, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xb8}}},
    { 22, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb4,0x5b}}},
    { 21, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0x6a}}},
    { 1, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb6,0x14}}},
    { 23, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xe6}}},
    { 0, {{0}}}
};
#endif

/*---------------------------------------------------------------------------*/
void
deployment_init(void)
{
  node_id = get_node_id();
}
/*---------------------------------------------------------------------------*/
uint16_t
get_node_id(void)
{
  return nodeid_from_linkaddr((const linkaddr_t *)&linkaddr_node_addr);
}
/*---------------------------------------------------------------------------*/
uint16_t
nodeid_from_linkaddr(const linkaddr_t *lladdr)
{
  if(lladdr == NULL) {
    return 0;
  }
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    /* Assume network-wide unique 16-bit MAC addresses */
    if(linkaddr_cmp(lladdr, &curr->mac)) {
      return curr->id;
    }
    curr++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
linkaddr_from_nodeid(linkaddr_t *lladdr, uint16_t id)
{
  if(id == 0 || lladdr == NULL) {
    return;
  }
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    if(curr->id == id) {
      linkaddr_copy(lladdr, &curr->mac);
      return;
    }
    curr++;
  }
}
/*---------------------------------------------------------------------------*/
