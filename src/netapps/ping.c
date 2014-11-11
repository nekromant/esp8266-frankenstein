/**
 * @file
 * Ping sender module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 */

/** 
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

/*
 * copyright (c) 2010 - 2011 Espressif System
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
//NO_SYS = 1
#include "lwip/timers.h"
#include "lwip/inet_chksum.h"
#include "lwip/app/ping.h"

#if PING_USE_SOCKETS
#include "lwip/sockets.h"
#include "lwip/inet.h"
#endif /* PING_USE_SOCKETS */


/**
 * PING_DEBUG: Enable debugging for PING.
 */
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_OFF
#endif

/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
//#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)
static ip_addr_t g_ping_target;
#define PING_TARGET g_ping_target
#
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 1000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     1000
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

#ifdef SSC
static void (* ping_recv_cb)(struct pbuf *p, struct icmp_echo_hdr *iecho);
#endif /* SSC */

/* ping variables */
static u16_t ping_seq_num;
static u32_t ping_time;
static u16_t ping_data_size = PING_DATA_SIZE;
#if !PING_USE_SOCKETS
static struct raw_pcb *ping_pcb;
#endif /* PING_USE_SOCKETS */

void inline set_ping_length(u16_t ping_length){
    ping_data_size = ping_length >= PING_DATA_SIZE ? ping_length:PING_DATA_SIZE;
}

u16_t inline get_ping_length(){
    return ping_data_size;
}	

/** Prepare a echo ICMP request */
static void
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)ICACHE_FLASH_ATTR;
static void
ping_prepare_echo( struct icmp_echo_hdr *iecho, u16_t len)
{
  size_t i;
  size_t data_len = len - sizeof(struct icmp_echo_hdr);

  ICMPH_TYPE_SET(iecho, ICMP_ECHO);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;
  iecho->id     = PING_ID;
  ++ping_seq_num;
  iecho->seqno  = htons(ping_seq_num);

  /* fill the additional data buffer with some data */
  for(i = 0; i < data_len; i++) {
    ((char*)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
  }

  iecho->chksum = inet_chksum(iecho, len);
}

static void
ping_prepare_er(struct icmp_echo_hdr *iecho, u16_t len)ICACHE_FLASH_ATTR;
static void
ping_prepare_er(struct icmp_echo_hdr *iecho, u16_t len)
{

  ICMPH_TYPE_SET(iecho, ICMP_ER);
  ICMPH_CODE_SET(iecho, 0);
  iecho->chksum = 0;

  iecho->chksum = inet_chksum(iecho, len);
}

#if PING_USE_SOCKETS

#else /* PING_USE_SOCKETS */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)ICACHE_FLASH_ATTR;
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_ASSERT("p != NULL", p != NULL);

  if (pbuf_header( p, -PBUF_IP_HLEN)==0) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num)) && iecho->type == ICMP_ER) {
      LWIP_DEBUGF( PING_DEBUG, ("ping: recv "));
      ip_addr_debug_print(PING_DEBUG, addr);
      LWIP_DEBUGF( PING_DEBUG, (" %"U32_F" ms\n", (sys_now()-ping_time)));

      /* do some ping result processing */
#ifdef SSC

      //if (ICMP response)
      /*
       * notify SSC module ping response is recevied
       */
      if (ping_recv_cb)
          ping_recv_cb(p, iecho);
#endif /* SSC */

      PING_RESULT(1);
      pbuf_free(p);
      return 1; /* eat the packet */
    }else if(iecho->type == ICMP_ECHO){
        struct pbuf *q;
//        os_printf("receive ping request:seq=%d\n", ntohs(iecho->seqno));
        q = pbuf_alloc(PBUF_IP, (u16_t)p->tot_len, PBUF_RAM);
        if(q!=NULL)
        {
            pbuf_copy(q, p);
            iecho = (struct icmp_echo_hdr *)q->payload;
            ping_prepare_er(iecho, q->tot_len);
            raw_sendto(pcb, q, addr);
            pbuf_free(q);
        }
        pbuf_free(p);
        return 1;
    }
  }

  return 0; /* don't eat the packet */
}

static void
ping_send(struct raw_pcb *raw, ip_addr_t *addr)ICACHE_FLASH_ATTR;
static void
ping_send(struct raw_pcb *raw, ip_addr_t *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;
  size_t ping_size = sizeof(struct icmp_echo_hdr) + ping_data_size;

  LWIP_DEBUGF( PING_DEBUG, ("ping: send "));
  ip_addr_debug_print(PING_DEBUG, addr);
  LWIP_DEBUGF( PING_DEBUG, ("\n"));
  LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

  p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
  if (!p) {
    return;
  }
  if ((p->len == p->tot_len) && (p->next == NULL)) {
    iecho = (struct icmp_echo_hdr *)p->payload;

    ping_prepare_echo(iecho, (u16_t)ping_size);

    raw_sendto(raw, p, addr);
    ping_time = sys_now();
  }
  pbuf_free(p);
}

static void
ping_timeout(void *arg)ICACHE_FLASH_ATTR;
static void
ping_timeout(void *arg)
{
  struct raw_pcb *pcb = (struct raw_pcb*)arg;
  ip_addr_t ping_target = PING_TARGET;
  
  LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

  ping_send(pcb, &ping_target);

  sys_timeout(PING_DELAY, ping_timeout, pcb);
}

static void
ping_raw_init(void)ICACHE_FLASH_ATTR;
static void
ping_raw_init(void)
{
  ping_pcb = raw_new(IP_PROTO_ICMP);
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);

  raw_recv(ping_pcb, ping_recv, NULL);
  raw_bind(ping_pcb, IP_ADDR_ANY);
//  sys_timeout(PING_DELAY, ping_timeout, ping_pcb);
}

void
ping_send_now()
{
  ip_addr_t ping_target = PING_TARGET;
  LWIP_ASSERT("ping_pcb != NULL", ping_pcb != NULL);
  ping_send(ping_pcb, &ping_target);
}

#endif /* PING_USE_SOCKETS */

void
ping_init(void)
{
#if PING_USE_SOCKETS
  sys_thread_new("ping_thread", ping_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#else /* PING_USE_SOCKETS */
  ping_raw_init();
#endif /* PING_USE_SOCKETS */
}

#ifdef SSC
void
ping_set_target(ip_addr_t *ip)
{
    g_ping_target = *ip;
}

void
ping_set_recv_cb(void (* cb)(struct pbuf *p, struct icmp_echo_hdr *iecho))
{
    ping_recv_cb = cb;
}
#endif /* SSC */

#endif /* LWIP_RAW */
