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
 * $Id: uip.h,v 1.8 2001/10/25 18:55:39 adam Exp $
 *
 */

#ifndef __UIP_H__
#define __UIP_H__

#include "uipopt.h"


/* The following global variables are used for passing parameters
   between uIP, the network device driver and the application. */

extern u8_t uip_buf[UIP_BUFSIZE];  /* The uip_buf array is used to
                                      hold incoming and outgoing
                                      packets. The device driver fills
                                      this with incoming packets, and
                                      reads from here when sending
                                      packets out on the wire. */
extern volatile u8_t *uip_appdata; /* This pointer points to the
                                      application data when the
                                      application is called. If the
                                      application wishes to send data,
                                      this is where the application
                                      should write it. */

#if UIP_BUFSIZE > 255
extern volatile u16_t uip_len;
#else
extern volatile u8_t uip_len;
#endif /* UIP_BUFSIZE > 255 */

                                   /* When the application is called,
                                      uip_len contains the length of
                                      any new data that has been
                                      received from the remote
                                      host. The application should set
                                      this variable to the size of any
                                      data that the application wishes
                                      to send. When the network device
                                      driver output function is
                                      called, uip_len contains the
                                      length of the outgoing
                                      packet. */
extern volatile u8_t uip_flags;    /* When the application is called,
                                      uip_flags will contain the flags
                                      that are defined in this
                                      file. Please read below for more
                                      infomation. */
extern struct uip_conn *uip_conn;  /* When the application is called,
                                      uip_conn will point to the
                                      conntection that should be
                                      processed by the
                                      application. See below
                                      for more information. */

/* The uIP functional interface. */
void uip_init(void);               /* uip_init() must be called at
                                      boot up to configure the uIP
                                      data structures. */
void uip_periodic(u8_t conn);      /* uip_periodic() should be called
                                      when the periodic timer has
                                      fired. Should be called once per
                                      connection (0 - UIP_CONNS). */
void uip_process(u8_t flag);       /* uip_process() is called when the
                                      network device driver has
                                      received new data. */
void uip_listen(u16_t port);       /* Starts listening to the
                                      specified port. */

/* The following structure is used for identifying a connection. All
   but one field in the structure are to be considered read-only by an
   application. The only exception is the appstate field whos purpose
   is to let the application store application-specific state (e.g.,
   file pointers) for the connection. The size of this field is
   configured in the "uipopt.h" header file. */

struct uip_conn {
  u8_t tcpstateflags; /* TCP state and flags. */
  u16_t lport, rport; /* The local and the remote port. */
  u16_t ripaddr[2];   /* The IP address of the remote peer. */
  u8_t rcv_nxt[4];    /* The sequence number that we expect to receive
			 next. */
  u8_t snd_nxt[4];    /* The sequence number that was last sent by
                         us. */
  u8_t ack_nxt[4];    /* The sequence number that should be ACKed by
			 next ACK from peer. */
  u8_t timer;         /* The retransmission timer. */
  u8_t nrtx;          /* Counts the number of retransmissions for a
                         particular segment. */
  
  u8_t appstate[UIP_APPSTATE_SIZE];
};

/* The following flags may be set in the global variable uip_flags
   before calling the application callback. The UIP_ACKDATA and
   UIP_NEWDATA flags may both be set at the same time, whereas the
   others are mutualy exclusive. */

#define UIP_ACKDATA   1     /* Signifies that the outstanding data was
			       acked and the application should send
			       out new data instead of retransmitting
			       the last data. */
#define UIP_NEWDATA   2     /* Flags the fact that the peer has sent
			       us new data. */
#define UIP_REXMIT    4     /* Tells the application to retransmit the
			       data that was last sent. */
#define UIP_POLL      8     /* Used for polling the application, to
			       check if the application has data that
			       it wants to send. */
#define UIP_CLOSED    16    /* The remote host has closed the
			       connection, thus the connection has
			       gone away. */
#define UIP_ACCEPT    32    /* We have got a connection from a remote
                               host and have set up a new connection
                               for it. */

#define UIP_ABORT     64
#define UIP_CLOSE     128


/* The following flags are passed as an argument to the uip_process()
   function. They are used to distinguish between the two cases where
   uip_process() is called. It can be called either because we have
   incoming data that should be processed, or because the periodic
   timer has fired. */

#define UIP_DATA    1     /* Tells uIP that there is incoming data in
                             the uip_buf buffer. The length of the
                             data is stored in the global variable
                             uip_len. */
#define UIP_TIMER   2     /* Tells uIP that the periodic timer has
                             fired. */

#ifndef htons
#   if BYTE_ORDER == BIG_ENDIAN
#      define htons(n) (n)
#   else /* BYTE_ORDER == BIG_ENDIAN */
#      define htons(n) ((((u16_t)((n) & 0xff)) << 8) | (((n) & 0xff00) >> 8))
#   endif /* BYTE_ORDER == BIG_ENDIAN */
#endif /* htons */

#define ntohs(n) htons(n)

#define CLOSED      0
#define SYN_RCVD    1
#define ESTABLISHED 2
#define FIN_WAIT_1  3
#define FIN_WAIT_2  4
#define CLOSING     5
#define TIME_WAIT   6
#define LAST_ACK    7
#define TS_MASK     7
  
#define UIP_OUTSTANDING 8

typedef struct {
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
  /* TCP header. */
  u16_t srcport,
    destport;
  u8_t seqno[4],  
    ackno[4],
    tcpoffset,
    flags,
    wnd[2];     
  u16_t tcpchksum,
    urgp; 
  u8_t optdata[4];
} uip_tcpip_hdr;


struct uip_stats {
  struct {
    u16_t drop;
    u16_t recv;
    u16_t sent;
    u16_t vhlerr;   /* Number of packets dropped due to wrong IP version
		       or header length. */
    u16_t hblenerr; /* Number of packets dropped due to wrong IP length,
		       high byte. */
    u16_t lblenerr; /* Number of packets dropped due to wrong IP length,
		       low byte. */
    u16_t fragerr;  /* Number of packets dropped since they were IP
		       fragments. */
    u16_t chkerr;   /* Number of packets dropped due to IP checksum errors. */
    u16_t protoerr; /* Number of packets dropped since they were neither
		       ICMP nor TCP. */
  } ip;
  struct {
    u16_t drop;
    u16_t recv;
    u16_t sent;
    u16_t typeerr;
  } icmp;
  struct {
    u16_t drop;
    u16_t recv;
    u16_t sent;
    u16_t chkerr;
    u16_t ackerr;
    u16_t rst;
    u16_t rexmit;
    u16_t syndrop;  /* Number of dropped SYNs due to too few
                       connections was avaliable. */
    u16_t synrst;   /* Number of SYNs for closed ports, triggering a
                       RST. */
  } tcp;
};

extern struct uip_stats uip_stat;

extern struct uip_conn uip_conns[UIP_CONNS];

#endif /* __UIP_H__ */







