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
 * $Id: uipopt.h,v 1.5 2002/01/13 21:12:41 adam Exp $
 *
 */

#ifndef __UIPOPT_H__
#define __UIPOPT_H__

/* This file is used for tweaking various configuration options for
   uIP. You should make a copy of this file into one of your project's
   directories instead of editing this example "uipopt.h" file that
   comes with the uIP distribution. */

/*-----------------------------------------------------------------------------------*/
/* First, two typedefs that may have to be tweaked for your particular
   compiler. The uX_t types are unsigned integer types, where the X is
   the number of bits in the integer type. Most compilers use
   "unsigned char" and "unsigned short" for those two,
   respectively. */
typedef unsigned char u8_t;
typedef unsigned short u16_t;

/*-----------------------------------------------------------------------------------*/
/* The configuration options for a specific node. This includes IP
 * address, netmask and default router as well as the Ethernet
 * address. The netmask, default router and Ethernet address are
 * appliciable only if uIP should be run over Ethernet.
 *
 * All of these should be changed to suit your project.
*/

/* UIP_IPADDR: The IP address of this uIP node. */
#define UIP_IPADDR0     192
#define UIP_IPADDR1     168
#define UIP_IPADDR2     0
#define UIP_IPADDR3     2

/* UIP_NETMASK: The netmask. */
#define UIP_NETMASK0    255
#define UIP_NETMASK1    255
#define UIP_NETMASK2    255
#define UIP_NETMASK3    0

/* UIP_DRIPADDR: IP address of the default router. */
#define UIP_DRIPADDR0   192
#define UIP_DRIPADDR1   168 
#define UIP_DRIPADDR2   0
#define UIP_DRIPADDR3   1

/* UIP_ETHADDR: The Ethernet address. */
#define UIP_ETHADDR0    0x00
#define UIP_ETHADDR1    0xbd
#define UIP_ETHADDR2    0x3b
#define UIP_ETHADDR3    0x33
#define UIP_ETHADDR4    0x05
#define UIP_ETHADDR5    0x71


/*-----------------------------------------------------------------------------------*/
/* The following options are used to configure application specific
 * setting such as how many TCP ports that should be avaliable and if
 * the uIP should be configured to support active opens.
 *
 * These should probably be tweaked to suite your project.
 */

/* Include the header file for the application program that should be
   used. If you don't use the example web server, you should change
   this. */
#include "httpd.h"

/* UIP_ACTIVE_OPEN: Determines if support for opening connections from
   uIP should be compiled in. If this isn't needed for your
   application, don't turn it on. (A web server doesn't need this, for
   instance.) */
#define UIP_ACTIVE_OPEN 0

/* UIP_CONNS: The maximum number of simultaneously active
   connections. */
#define UIP_CONNS       10

/* UIP_LISTENPORTS: The maximum number of simultaneously listening TCP
   ports. For a web server, 1 is enough here. */
#define UIP_LISTENPORTS 10

/* UIP_BUFSIZE: The size of the buffer that holds incoming and
   outgoing packets. */
#define UIP_BUFSIZE     180

/* UIP_STATISTICS: Determines if statistics support should be compiled
   in. The statistics is useful for debugging and to show the user. */
#define UIP_STATISTICS  0

/* UIP_LOGGING: Determines if logging of certain events should be
   compiled in. Useful mostly for debugging. The function uip_log(char
   *msg) must be implemented to suit your architecture if logging is
   turned on. */
#define UIP_LOGGING     0

/* UIP_LLH_LEN: The link level header length; this is the offset into
   the uip_buf where the IP header can be found. For Ethernet, this
   should be set to 14. For SLIP, this should be set to 0. */
#define UIP_LLH_LEN     14

/*-----------------------------------------------------------------------------------*/
/* The following configuration options can be tweaked for your
 * project, but you are probably safe to use the default values. The
 * options are listed in order of tweakability.
 */

/* UIP_ARPTAB_SIZE: The size of the ARP table - use a larger value if
   this uIP node will have many connections from the local network. */
#define UIP_ARPTAB_SIZE 8

/* The maxium age of ARP table entries measured in 10ths of
   seconds. An UIP_ARP_MAXAGE of 120 corresponds to 20 minutes (BSD
   default). */
#define UIP_ARP_MAXAGE 120

/* UIP_RTO: The retransmission timeout counted in timer pulses (i.e.,
   the speed of the periodic timer, usually one second). */
#define UIP_RTO         3

/* UIP_MAXRTX: The maximum number of times a segment should be
   retransmitted before the connection should be aborted. */
#define UIP_MAXRTX      8

/* UIP_TCP_MSS: The TCP maximum segment size. This should be set to
   at most UIP_BUFSIZE - UIP_LLH_LEN - 40. */
#define UIP_TCP_MSS     (UIP_BUFSIZE - UIP_LLH_LEN - 40)

/* UIP_TTL: The IP TTL (time to live) of IP packets sent by uIP. */
#define UIP_TTL         255

/* UIP_TIME_WAIT_TIMEOUT: How long a connection should stay in the
   TIME_WAIT state. Has no real implication, so it should be left
   untouched. */
#define UIP_TIME_WAIT_TIMEOUT 120
/*-----------------------------------------------------------------------------------*/
/* This is where you configure if your CPU architecture is big or
 * little endian. Most CPUs today are little endian. The most notable
 * exception are the Motorolas which are big endian. Tweak the
 * definition of the BYTE_ORDER macro to configure uIP for your
 * project.
 */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN  3412
#endif /* LITTLE_ENDIAN */
#ifndef BIG_ENDIAN
#define BIG_ENDIAN     1234
#endif /* BIGE_ENDIAN */

#ifndef BYTE_ORDER
#define BYTE_ORDER     LITTLE_ENDIAN
#endif /* BYTE_ORDER */

#endif /* __UIPOPT_H__ */
