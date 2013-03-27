/*
 * Copyright (c) 2001, Adam Dunkels.
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
 * $Id: uip_arch.c,v 1.2 2001/10/25 18:55:57 adam Exp $
 *
 */


#include "uip.h"
#include "uip_arch.h"

#define BUF ((uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define IP_PROTO_TCP    6

/*-----------------------------------------------------------------------------------*/
#if UIP_BUFSIZE > 255
/*-----------------------------------------------------------------------------------*/
void
uip_add_rcv_nxt(u16_t n)
{
  u32_t *tmp;

  tmp = &uip_conn->rcv_nxt[0];
  *tmp += n;
}
/*-----------------------------------------------------------------------------------*/
void
uip_add_ack_nxt(u16_t n)
{
  u32_t *tmp;

  tmp = &uip_conn->ack_nxt[0];
  *tmp += n;
}
/*-----------------------------------------------------------------------------------*/
#else /* UIP_BUFSIZE > 255 */
/*-----------------------------------------------------------------------------------*/
void
uip_add_rcv_nxt(u8_t n)
{
  u32_t *tmp;

  tmp = (u32_t *)&uip_conn->rcv_nxt[0];
  *tmp += n;
}
/*-----------------------------------------------------------------------------------*/
void
uip_add_ack_nxt(u8_t n)
{
  u32_t *tmp;

  tmp = (u32_t*)&uip_conn->ack_nxt[0];
  *tmp += n;
}
/*-----------------------------------------------------------------------------------*/
#endif /* UIP_BUFSIZE > 255 */

static u16_t
chksum(u8_t *sdata, u16_t len)
{
  u32_t acc = 0;
  u16_t i = 0;

   while (i < len) {
  	if (i & 0x01)
  	{
	    acc += *sdata++;
	} else
  	{
  		acc += ((u32_t)(*sdata++))<<8;
  	}
  	i++;
  };

  while(acc >> 16) {
    acc = (acc & 0xffffUL) + (acc >> 16);
  }

  return acc & 0xffffUL;
}
/*-----------------------------------------------------------------------------------*/
u16_t
uip_ipchksum(void)
{
  return chksum(&uip_buf[UIP_LLH_LEN], 20);
}
/*-----------------------------------------------------------------------------------*/
u16_t
uip_tcpchksum(void)
{
  u32_t sum;

  /* Compute the checksum of the TCP header. */
  sum = chksum( &uip_buf[20 + UIP_LLH_LEN], 20);

  /* Compute the checksum of the data in the TCP packet and add it to
     the TCP header checksum. */

  sum += chksum(uip_appdata,
		(u16_t)(((BUF->len[0] << 8) + BUF->len[1]) - 40));
  
  sum += BUF->srcipaddr[0];
  sum += BUF->srcipaddr[1];
  sum += BUF->destipaddr[0];
  sum += BUF->destipaddr[1];
  sum += (u16_t)htons((u16_t)IP_PROTO_TCP);
  sum += (u16_t)htons(((BUF->len[0] << 8) + BUF->len[1]) - 20);
  
  while(sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return sum & 0xffff;
}
/*-----------------------------------------------------------------------------------*/
