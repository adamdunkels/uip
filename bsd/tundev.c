/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: tundev.c,v 1.6 2001/11/23 05:59:41 adam Exp $
 */


#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#ifndef linux
#include <net/if_tun.h>
#endif

#include "uip.h"

static int drop = 0;
static int fd;

struct tcpip_hdr {
  unsigned char vhl,
    tos,          
    len[2],       
    id[2],        
    ipoffset[2],  
    ttl,          
    proto;     
  unsigned short ipchksum;
  unsigned long srcipaddr, 
    destipaddr;
  unsigned char srcport[2],
    destport[2],
    seqno[4],  
    ackno[4],
    tcpoffset,
    flags,
    wnd[2],     
    tcpchksum[2],
    urgp[2]; 
  unsigned char data[0];

};
/*-----------------------------------------------------------------------------------*/
static unsigned short
chksum(void *data, int len)
{
  unsigned short *sdata = data;
  unsigned long acc;
  unsigned long sum;
    
  for(acc = 0; len > 1; len -= 2) {
    acc += *sdata++;
  }

  /* add up any odd byte */
  if(len == 1) {
    acc += (unsigned short)(*(unsigned char *)sdata);
  }

  sum = (acc & 0xffff) + (acc >> 16);
  sum += (sum >> 16);
  
  return ~(sum & 0xffff);
}
/*-----------------------------------------------------------------------------------*/
static unsigned short
chksum_pseudo(void *data, int len, unsigned long ipaddr1,
              unsigned long ipaddr2,
              unsigned char proto,
              unsigned short protolen)
{
  unsigned long sum;
  unsigned short *sdata = data;
  
  for(sum = 0; len > 1; len -= 2) {
    sum += *sdata++;
  }

  /* add up any odd byte */
  if(len == 1) {
    sum += (unsigned short)(*(unsigned char *)sdata);
  }

  sum += (ipaddr1 & 0xffff);
  sum += ((ipaddr1 >> 16) & 0xffff);
  sum += (ipaddr2 & 0xffff);
  sum += ((ipaddr2 >> 16) & 0xffff);
  sum += (unsigned short)htons((unsigned short)proto);
  sum += (unsigned short)htons(protolen);
  
  while(sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  
  return ~sum & 0xffff;
}
/*-----------------------------------------------------------------------------------*/
static void
check_checksum(void *data, int len)
{
  struct tcpip_hdr *hdr;
  unsigned short sum;
  
  hdr = data;

  sum = chksum(data, 20);  
  printf("IP header checksum check 0x%x\n", sum);
  
  sum = chksum_pseudo(&(hdr->srcport[0]), len - 20,
                      hdr->srcipaddr, hdr->destipaddr,
                      hdr->proto, len - 20);  
  printf("TCP checksum check 0x%x len %d protolen %d\n", sum, len,
         len - 20);
  
}
/*-----------------------------------------------------------------------------------*/
void
tundev_init(void)
{
  int val;
  fd = open("/dev/tun0", O_RDWR);
  if(fd == -1) {
    perror("tun_dev: tundev_init: open");
    exit(1);
  }
#ifdef linux
  system("ifconfig tun0 inet 192.168.0.2 192.168.0.1");
  system("route add -net 192.168.0.0 netmask 255.255.255.0 dev tun0");
#else
  system("ifconfig tun0 inet 192.168.0.1 192.168.0.2");
  val = 0;
  ioctl(fd, TUNSIFHEAD, &val);

#endif /* linux */
}
/*-----------------------------------------------------------------------------------*/
unsigned int
tundev_read(void)
{
  fd_set fdset;
  struct timeval tv;
  int ret;

  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  
  ret = select(fd + 1, &fdset, NULL, NULL, &tv);
  if(ret == 0) {
    return 0;
  } 
  ret = read(fd, uip_buf, UIP_BUFSIZE);
  if(ret == -1) {
    perror("tun_dev: tundev_read: read");
  }
    /*printf("--- tun_dev: tundev_read: read %d bytes\n", ret);*/
  /*  {
    int i;
    for(i = 0; i < 20; i++) {
      printf("%x ", uip_buf[i]);
    }
    printf("\n");
    }*/
  /*  check_checksum(uip_buf, ret);*/
  return ret;
}
/*-----------------------------------------------------------------------------------*/
void
tundev_send(void)
{
  int ret;
  int i;
  char tmpbuf[UIP_BUFSIZE];
  /*  printf("tundev_send: sending %d bytes\n", uip_len);*/
  /*  check_checksum(uip_buf, size);*/

  /*  drop++;
  if(drop % 8 == 7) {
    printf("Dropped a packet!\n");
    return;
  }*/
  /*  {
    int i;
    for(i = 0; i < 20; i++) {
      printf("%x ", uip_buf[i]);
    }
    printf("\n");
    }*/
  for(i = 0; i < 40; i++) {
    tmpbuf[i] = uip_buf[i];
  }
  
  for(i = 40; i < uip_len; i++) {
    tmpbuf[i] = uip_appdata[i - 40];
  }
  
  ret = write(fd, tmpbuf, uip_len);
  if(ret == -1) {
    perror("tun_dev: tundev_done: write");
    exit(1);
  }
}  
/*-----------------------------------------------------------------------------------*/
