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
 * $Id: uipopt.h,v 1.3 2001/11/25 18:47:36 adam Exp $
 *
 */

#ifndef __UIPOPT_H__
#define __UIPOPT_H__

/* This file is used for tweaking various configuration options for
   uIP. You should make a copy of this file into one of your project's
   directories instead of editing the example "uipopt.h" file that
   come with the uIP distribution. */

/* The following typedefs may have to be tweaked for your particular
   compiler. The uX_t types are unsigned integer types, where the X is
   the number of bits in the integer type. */
typedef unsigned char u8_t;
typedef unsigned short u16_t;

/* This is where you configure if your CPU architecture is big or
   little endian. Most CPUs today are little endian. The most notable
   exception are the Motorolas which are big endian. Tweak the
   definition of the BYTE_ORDER macro to configure uIP for your
   project. */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN  3412
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN     1234
#endif /* BIGE_ENDIAN */

#ifndef BYTE_ORDER
#define BYTE_ORDER     LITTLE_ENDIAN
#endif /* BYTE_ORDER */

/* This is the header of the application program that should be
   used. */
#include "httpd.h"

/* UIP_CONNS: The maximum number of simultaneously active
   connections. */
#define UIP_CONNS       10

/* UIP_LISTENPORTS: The maximum number of simultaneously listening TCP
   ports. */
#define UIP_LISTENPORTS 1

/* UIP_BUFSIZE: The size of the buffer that holds incoming and
   outgoing packets. In this version of uIP, this can be more than
   255 bytes. */
#define UIP_BUFSIZE     240 

/* UIP_IPADDR: The IP address of this uIP node. */
#define UIP_IPADDR0     192
#define UIP_IPADDR1     168
#define UIP_IPADDR2     0
#define UIP_IPADDR3     2

/* UIP_LLH_LEN: The link level header length; this is the offset into
   the uip_buf where the IP header can be found. For Ethernet, this
   should be set to 14. */
#define UIP_LLH_LEN     0

/* UIP_TCP_MSS: The TCP maximum segment size. This should be set to
   at most UIP_BUFSIZE - 40. */
#define UIP_TCP_MSS     UIP_BUFSIZE - 40

/* UIP_TTL: The IP TTL (time to live) of IP packets sent by uIP. */
#define UIP_TTL         128

/* UIP_RTO: The retransmission timeout counted in timer pulses (i.e.,
   the speed of the periodic timer). */
#define UIP_RTO         3

#define UIP_STATISTICS 1
#define UIP_LOGGING 0

#define UIP_TIME_WAIT_TIMEOUT 120

#endif /* __UIPOPT_H__ */
