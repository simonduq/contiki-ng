/*
 * Copyright (c) 2017, RISE SICS
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
 *         CSMA security
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

/**
 * \addtogroup csma
 * @{
*/

#include "contiki.h"
#include "net/mac/csma/csma.h"
#include "net/mac/csma/anti-replay.h"
#include "net/mac/csma/csma-security.h"
#include "net/mac/csma/ccm-star-packetbuf.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/framer/framer-802154.h"
#include "net/mac/llsec802154.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/ccm-star.h"
#include "lib/aes-128.h"
#include <stdio.h>
#include <string.h>
#include "ccm-star-packetbuf.h"
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSMA"
#define LOG_LEVEL LOG_LEVEL_MAC

#if LLSEC802154_ENABLED

/**
 *  The keys for LLSEC for CSMA
 */
typedef struct {
  uint8_t u8[16];
} aes_key_t;
static aes_key_t keys[CSMA_LLSEC_MAXKEYS];

static uint8_t llsec_level = CSMA_LLSEC_SECURITY_LEVEL;
static uint8_t llsec_key_mode = CSMA_LLSEC_KEY_ID_MODE;
static uint8_t llsec_default_key = CSMA_LLSEC_KEY_INDEX;
static uint8_t llsec_mic_len = LLSEC802154_MIC_LEN(CSMA_LLSEC_SECURITY_LEVEL);

/* assumed to be 16 bytes */
void
csma_security_set_key(uint8_t index, uint8_t *key)
{
  if(key != NULL && index < CSMA_LLSEC_MAXKEYS) {
    memcpy(keys[index].u8, key, 16);
  }
}

void
csma_security_set_level(uint8_t level)
{
  llsec_level = level;
}
void
csma_security_set_default_key(uint8_t index)
{
  if(index < CSMA_LLSEC_MAXKEYS) {
    llsec_default_key = index;
  }
}

uint8_t
csma_security_get_level(void)
{
  return llsec_level;
}

uint8_t
csma_security_get_default_key(void)
{
  return llsec_default_key;
}

uint8_t
csma_security_get_key_mode(void)
{
  return llsec_key_mode;
}

/*---------------------------------------------------------------------------*/
static int
aead(uint8_t hdrlen, int forward, int miclen)
{
  uint8_t totlen;
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  uint8_t *m;
  uint8_t m_len;
  uint8_t *a;
  uint8_t a_len;
  uint8_t *result;
  uint8_t generated_mic[miclen];
  uint8_t *mic;
  uint8_t key_index;
  aes_key_t *key;
  uint8_t with_encryption;

  memset(nonce, 0, CCM_STAR_NONCE_LENGTH);

  if(packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE) == FRAME802154_IMPLICIT_KEY) {
    key_index = csma_security_get_default_key();
  } else {
    key_index = packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX);
  }

  if(key_index >= CSMA_LLSEC_MAXKEYS) {
    LOG_ERR("Key not available: %u\n", key_index);
    return 0;
  }

  key = &keys[key_index];

#if LLSEC802154_USES_FRAME_COUNTER
  ccm_star_packetbuf_set_nonce(nonce, forward);
#endif

  totlen = packetbuf_totlen();
  a = packetbuf_hdrptr();

  with_encryption =
    (packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) & 0x4) ? 1 : 0;

  if(with_encryption) {
    a_len = hdrlen;
    m = a + a_len;
    m_len = totlen - hdrlen;
  } else {
    a_len = totlen;
    m = NULL;
    m_len = 0;
  }

  mic = a + totlen;
  result = forward ? mic : generated_mic;

  LOG_INFO("En/decrypting with key %d %02x...%02x\n", key_index,
           key->u8[0], key->u8[15]);

  CCM_STAR.set_key(key->u8);
  CCM_STAR.aead(nonce,
      m, m_len,
      a, a_len,
      result, llsec_mic_len,
      forward);

  if(forward) {
    packetbuf_set_datalen(packetbuf_datalen() + llsec_mic_len);
    return 1;
  } else {
    return (memcmp(generated_mic, mic, llsec_mic_len) == 0);
  }
}

/*---------------------------------------------------------------------------*/
int
csma_security_create_frame(void)
{
  int hdr_len;

  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);

#if LLSEC802154_USES_FRAME_COUNTER
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0 &&
     packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX) != 0xffff) {
    anti_replay_set_counter();
  }
#endif

  hdr_len = NETSTACK_FRAMER.create();
  if(hdr_len < 0) {
    return hdr_len;
  }

  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0) {
    if(!aead(hdr_len, 1, llsec_mic_len)) {
      LOG_ERR("failed to encrypt packet to ");
      LOG_ERR_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      LOG_ERR_("\n");
      return FRAMER_FAILED;
    }
    LOG_INFO("LLSEC-OUT:");
    LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    LOG_INFO_(" ");
    LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    LOG_INFO_(" %u (%u) KEY:0x%02x\n", packetbuf_datalen(), packetbuf_totlen(),
              packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX));
  }
  return hdr_len;
}

/*---------------------------------------------------------------------------*/
int
csma_security_frame_len(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) > 0 &&
     packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX) != 0xffff) {
    return NETSTACK_FRAMER.length() + llsec_mic_len;
  }
  return NETSTACK_FRAMER.length();
}
/*---------------------------------------------------------------------------*/
int
csma_security_parse_frame(void)
{
  int hdr_len;

  hdr_len = NETSTACK_FRAMER.parse();
  if(hdr_len < 0) {
    LOG_INFO("LLSEC-IN: could not parse frame\n");
    return hdr_len;
  }

  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) == 0) {
    /* No security - no more processing required */
    LOG_INFO("LLSEC-IN: frame is not secured\n");
    return hdr_len;
  }

  LOG_INFO("LLSEC-IN: ");
  LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
  LOG_INFO_(" ");
  LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  LOG_INFO_(" %d %u (%u) LV:%d KM:%d KEY:0x%02x\n", hdr_len, packetbuf_datalen(),
            packetbuf_totlen(), packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL),
            packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE),
            packetbuf_attr(PACKETBUF_ATTR_KEY_INDEX));

  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) !=
     csma_security_get_level()) {
    LOG_INFO("received frame with wrong security level (%u) from ",
           packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL));
    LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    LOG_INFO_("\n");
    return FRAMER_FAILED;
  }

  if(packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE) != CSMA_LLSEC_KEY_ID_MODE) {
    LOG_INFO("received frame with wrong key id mode (%u) from ",
           packetbuf_attr(PACKETBUF_ATTR_KEY_ID_MODE));
    LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    LOG_INFO("\n");
    return FRAMER_FAILED;
  }

  if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) {
    LOG_INFO("frame from ourselves\n");
    return FRAMER_FAILED;
  }

  if(packetbuf_datalen() <= llsec_mic_len) {
    LOG_ERR("MIC error - too little data in frame!\n");
    return FRAMER_FAILED;
  }

  packetbuf_set_datalen(packetbuf_datalen() - llsec_mic_len);
  if(!aead(hdr_len, 0, llsec_mic_len)) {
#if LLSEC802154_USES_FRAME_COUNTER
    LOG_INFO("received unauthentic frame %u from ",
             (unsigned int) anti_replay_get_counter());
#else
    LOG_INFO("received unauthentic frame from ");
#endif
    LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    LOG_INFO_("\n");
    return FRAMER_FAILED;
  }

  /* TODO anti-reply protection */
  return hdr_len;
}
/*---------------------------------------------------------------------------*/
#else
/* The "unsecure" version of the create frame / parse frame */
int
csma_security_create_frame(void)
{
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  return NETSTACK_FRAMER.create();
}
int
csma_security_parse_frame(void)
{
  return NETSTACK_FRAMER.parse();
}
#endif /* LLSEC802154_ENABLED */

/** @} */