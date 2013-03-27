/*
 * Copyright (c) 2001-2002, Adam Dunkels.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Dunkels.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip_arp.c,v 1.3 2002/01/13 21:12:41 adam Exp $
 *
 */


#include "uip_arp.h"

struct arp_hdr {
  struct uip_eth_hdr ethhdr;
  u16_t hwtype;
  u16_t protocol;
  u8_t hwlen;
  u8_t protolen;
  u16_t opcode;
  struct uip_eth_addr shwaddr;
  u16_t sipaddr[2];
  struct uip_eth_addr dhwaddr;
  u16_t dipaddr[2]; 
};

struct ethip_hdr {
  struct uip_eth_hdr ethhdr;
  /* IP header. */
  u8_t vhl,
    tos,          
    len[2],       
    ipid[2],        
    ipoffset[2],  
    ttl,          
    proto;     
  u16_t ipchksum;
  u16_t srcipaddr[2], 
    destipaddr[2];
};

#define ARP_REQUEST 1
#define ARP_REPLY   2

#define ARP_HWTYPE_ETH 1

struct arp_entry {
  u16_t ipaddr[2];
  struct uip_eth_addr ethaddr;
  u8_t time;
};

static const struct uip_eth_addr ethaddr = {{UIP_ETHADDR0,
					     UIP_ETHADDR1,
					     UIP_ETHADDR2,
					     UIP_ETHADDR3,
					     UIP_ETHADDR4,
					     UIP_ETHADDR5}};

static struct arp_entry arp_table[UIP_ARPTAB_SIZE];
static u16_t ipaddr[2];
static u8_t i, c;

static u8_t time;
static u8_t tmpage;

#define BUF   ((struct arp_hdr *)&uip_buf[0])
#define IPBUF ((struct ethip_hdr *)&uip_buf[0])
/*-----------------------------------------------------------------------------------*/
void
uip_arp_init(void)
{
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    arp_table[i].ipaddr[0] =
      arp_table[i].ipaddr[1] = 0;
  }
}
/*-----------------------------------------------------------------------------------*/
void
uip_arp_timer(void)
{
  ++time;
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    if((arp_table[i].ipaddr[0] | arp_table[i].ipaddr[1]) != 0 &&
       time - arp_table[i].time >= UIP_ARP_MAXAGE) {
      arp_table[i].ipaddr[0] =
	arp_table[i].ipaddr[1] = 0;
    }
  }

}
/*-----------------------------------------------------------------------------------*/
static void
uip_arp_update(u16_t *ipaddr, struct uip_eth_addr *ethaddr)
{
  /* Walk through the ARP mapping table and try to find an entry to
     update. If none is found, the IP -> MAC address mapping is
     inserted in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    
    /* Only check those entries that are actually in use. */
    if(arp_table[i].ipaddr[0] != 0 &&
       arp_table[i].ipaddr[1] != 0) {

      /* Check if the source IP address of the incoming packet matches
         the IP address in this ARP table entry. */
      if(ipaddr[0] == arp_table[i].ipaddr[0] &&
	 ipaddr[1] == arp_table[i].ipaddr[1]) {
	 
	/* An old entry found, update this and return. */
	for(c = 0; c < 6; ++c) {
	  arp_table[i].ethaddr.addr[c] = ethaddr->addr[c];
	}
	arp_table[i].time = time;
	return;
      }
    }
  }

  /* If we get here, no existing ARP table entry was found, so we
     create one. */

  /* First, we try to find an unused entry in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    if(arp_table[i].ipaddr[0] == 0 &&
       arp_table[i].ipaddr[1] == 0)
      break;    
  }

  /* If no unused entry is found, we try to find the oldest entry and
     throw it away. */
  if(i == UIP_ARPTAB_SIZE) {
    tmpage = 0;
    c = 0;
    for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
      if(time - arp_table[i].time > tmpage) {
	tmpage = time - arp_table[i].time;
	c = i;
      }
    }
    i = c;
  }

  /* Now, i is the ARP table entry which we will fill with the new
     information. */
  arp_table[i].ipaddr[0] = ipaddr[0];
  arp_table[i].ipaddr[1] = ipaddr[1];
  for(c = 0; c < 6; ++c) {
    arp_table[i].ethaddr.addr[c] = ethaddr->addr[c];
  }
  arp_table[i].time = time;
}
/*-----------------------------------------------------------------------------------*/
void
uip_arp_ipin(void)
{

  /* Only insert/update an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  if((IPBUF->srcipaddr[0] & htons((UIP_NETMASK0 << 8) | UIP_NETMASK1)) !=
     (htons((UIP_IPADDR0 << 8) | UIP_IPADDR1)
      & htons((UIP_NETMASK0 << 8) | UIP_NETMASK1)))
    return;
  if((IPBUF->srcipaddr[1] & htons((UIP_NETMASK2 << 8) | UIP_NETMASK3)) !=
     (htons((UIP_IPADDR2 << 8) | UIP_IPADDR3)
      & htons((UIP_NETMASK2 << 8) | UIP_NETMASK3)))
    return;

  uip_arp_update(IPBUF->srcipaddr, &(IPBUF->ethhdr.src));
  
  return;
}
/*-----------------------------------------------------------------------------------*/
void
uip_arp_arpin(void)
{

  if(uip_len < sizeof(struct arp_hdr)) {
    uip_len = 0;
    return;
  }

  uip_len = 0;
  
  switch(BUF->opcode) {
  case htons(ARP_REQUEST):
    /* ARP request. If it asked for our address, we send out a
       reply. */
    if(BUF->dipaddr[0] == htons((UIP_IPADDR0 << 8) | UIP_IPADDR1) &&
       BUF->dipaddr[1] == htons((UIP_IPADDR2 << 8) | UIP_IPADDR3)) {
      /* The reply opcode is 2. */
      BUF->opcode = htons(2);
      BUF->dhwaddr = BUF->shwaddr;

      for(c = 0; c < 6; ++c) {
	BUF->shwaddr.addr[c] = 
	  BUF->ethhdr.src.addr[c] = ethaddr.addr[c];
	BUF->ethhdr.dest.addr[c] = BUF->dhwaddr.addr[c];
      }

      BUF->dipaddr[0] = BUF->sipaddr[0];
      BUF->dipaddr[1] = BUF->sipaddr[1];
      BUF->sipaddr[0] = htons((UIP_IPADDR0 << 8) | UIP_IPADDR1);
      BUF->sipaddr[1] = htons((UIP_IPADDR2 << 8) | UIP_IPADDR3);

      BUF->ethhdr.type = htons(UIP_ETHTYPE_ARP);      
      uip_len = sizeof(struct arp_hdr);
    }      
    break;
  case htons(ARP_REPLY):
    /* ARP reply. We insert or update the ARP table if it was meant
       for us. */
    if(BUF->dipaddr[0] == htons((UIP_IPADDR0 << 8) | UIP_IPADDR1) &&
       BUF->dipaddr[1] == htons((UIP_IPADDR2 << 8) | UIP_IPADDR3)) {

      uip_arp_update(BUF->sipaddr, &BUF->shwaddr);
    }
    break;
  }

  return;
}
/*-----------------------------------------------------------------------------------*/
void
uip_arp_out(void)
{
  /* Find the destination IP address in the ARP table and construct
     the Ethernet header. If the destination IP addres isn't on the
     local network, we use the default router's IP address instead.

     If not ARP table entry is found, we overwrite the original IP
     packet with an ARP request for the IP address. */

  /* Check if the destination address is on the local network. */
  if((IPBUF->destipaddr[0] & htons((UIP_NETMASK0 << 8) | UIP_NETMASK1)) !=
     (htons((UIP_IPADDR0 << 8) | UIP_IPADDR1)
      & htons((UIP_NETMASK0 << 8) | UIP_NETMASK1)) ||
     (IPBUF->destipaddr[1] & htons((UIP_NETMASK2 << 8) | UIP_NETMASK3)) !=
     (htons((UIP_IPADDR2 << 8) | UIP_IPADDR3)
      & htons((UIP_NETMASK2 << 8) | UIP_NETMASK3))) {
    /* Destination address was not on the local network, so we need to
       use the default router's IP address instead of the destination
       address when determining the MAC address. */
    ipaddr[0] = htons((UIP_DRIPADDR0 << 8) | UIP_DRIPADDR1);
    ipaddr[1] = htons((UIP_DRIPADDR2 << 8) | UIP_DRIPADDR3);
  } else {
    /* Else, we use the destination IP address. */
    ipaddr[0] = IPBUF->destipaddr[0];
    ipaddr[1] = IPBUF->destipaddr[1];
  }
      
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    if(ipaddr[0] == arp_table[i].ipaddr[0] &&
       ipaddr[1] == arp_table[i].ipaddr[1])
      break;
  }

  if(i == UIP_ARPTAB_SIZE) {
    /* The destination address was not in our ARP table, so we
       overwrite the IP packet with an ARP request. */

    for(c = 0; c < 6; ++c) {     
      BUF->ethhdr.dest.addr[c] = 0xff; /* Broadcast ARP request. */
      BUF->ethhdr.src.addr[c] = 
	BUF->shwaddr.addr[c] = ethaddr.addr[c];
      BUF->dhwaddr.addr[c] = 0;
    }
    
    BUF->dipaddr[0] = ipaddr[0];
    BUF->dipaddr[1] = ipaddr[1];
    BUF->sipaddr[0] = htons((UIP_IPADDR0 << 8) | UIP_IPADDR1);
    BUF->sipaddr[1] = htons((UIP_IPADDR2 << 8) | UIP_IPADDR3);
    BUF->opcode = htons(ARP_REQUEST); /* ARP request. */
    BUF->hwtype = htons(ARP_HWTYPE_ETH);
    BUF->protocol = htons(UIP_ETHTYPE_IP);
    BUF->hwlen = 6;
    BUF->protolen = 4;
    BUF->ethhdr.type = htons(UIP_ETHTYPE_ARP);

    uip_appdata = &uip_buf[40 + UIP_LLH_LEN];
    
    uip_len = sizeof(struct arp_hdr);
    return;
  }

  /* Build an ethernet header. */
  for(c = 0; c < 6; ++c) {
    IPBUF->ethhdr.dest.addr[c] = arp_table[i].ethaddr.addr[c];
    IPBUF->ethhdr.src.addr[0] = ethaddr.addr[c];
  }
  IPBUF->ethhdr.type = htons(UIP_ETHTYPE_IP);

  uip_len += sizeof(struct uip_eth_hdr);
}
/*-----------------------------------------------------------------------------------*/




