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
 * $Id: uip.c,v 1.24 2001/11/23 06:50:07 adam Exp $
 *
 */

/*
This is a small implementation of the IP and TCP protocols (as well as
some basic ICMP stuff). The implementation couples the IP, TCP and the
application layers very tightly. To keep the size of the compiled code
down, this code also features heavy usage of the goto statement.

The principle is that we have a small buffer, called the uip_buf, in which
the device driver puts an incoming packet. The TCP/IP stack parses the
headers in the packet, and calls upon the application. If the remote
host has sent data to the application, this data is present in the uip_buf
and the application read the data from there. It is up to the
application to put this data into a byte stream if needed. The
application will not be fed with data that is out of sequence.

If the application whishes to send data to the peer, it should put its
data into the uip_buf, 40 bytes from the start of the buffer. The TCP/IP
stack will calculate the checksums, and fill in the necessary header
fields and finally send the packet back to the peer. */

#include "uip.h"
#include "uipopt.h"
#include "uip_arch.h"

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
  /* ICMP (echo) header. */
  u8_t type, icode;
  u16_t icmpchksum;
  u16_t id, seqno;  
} ipicmphdr;

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

#define IP_PROTO_ICMP   1
#define IP_PROTO_TCP    6

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8    
  
u8_t uip_buf[UIP_BUFSIZE];
volatile u8_t *uip_appdata;

#if UIP_BUFSIZE > 255
volatile u16_t uip_len;
#else
volatile u8_t uip_len;
#endif /* UIP_BUFSIZE > 255 */

volatile u8_t uip_flags;
struct uip_conn *uip_conn;

static u16_t ipid;

void uip_process(u8_t flag);

struct uip_conn uip_conns[UIP_CONNS];
u16_t uip_listenports[UIP_LISTENPORTS];

/* Temporary variables. */
static u8_t tmpcnt;
static u16_t tmpport;

/* Macros. */
#define BUF ((uip_tcpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define ICMPBUF ((ipicmphdr *)&uip_buf[UIP_LLH_LEN])

#if UIP_STATISTICS == 1
struct uip_stats uip_stat;
#define UIP_STAT(s) s
#else
#define UIP_STAT(s)
#endif /* UIP_STATISTICS == 1 */

#if UIP_LOGGING == 1
#define UIP_LOG(m) uip_log(m)
#else
#define UIP_LOG(m)
#endif /* UIP_LOGGING == 1 */

/* The "rexmit_shift" table takes care of the exponential backoff -
   the RTO will be shifted rexmit_shift[number of retransmissions]. */
static const u8_t rexmit_shift[8] = {0,1,2,3,4,4,4,4};
/*-----------------------------------------------------------------------------------*/
void
uip_init(void)
{
  for(tmpcnt = 0; tmpcnt < UIP_LISTENPORTS; ++tmpcnt) {
    uip_listenports[tmpcnt] = 0;
  }
  for(tmpcnt = 0; tmpcnt < UIP_CONNS; ++tmpcnt) {
    uip_conns[tmpcnt].tcpstateflags = CLOSED;
  }
}
/*-----------------------------------------------------------------------------------*/
void
uip_listen(u16_t port)
{
  for(tmpcnt = 0; tmpcnt < UIP_LISTENPORTS; ++tmpcnt) {
    if(uip_listenports[tmpcnt] == 0) {
      uip_listenports[tmpcnt] = htons(port);
      break;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
void
uip_periodic(u8_t conn)
{
  uip_conn = &uip_conns[conn];
  uip_process(UIP_TIMER);
}
/*-----------------------------------------------------------------------------------*/
void
uip_process(u8_t flag)
{
  uip_appdata = &uip_buf[40 + UIP_LLH_LEN];
    
  /* Check if we were invoked because of the perodic timer fireing. */
  if(flag == UIP_TIMER) {
    uip_len = 0;
    if(uip_conn->tcpstateflags == TIME_WAIT ||
       uip_conn->tcpstateflags == FIN_WAIT_2) {
      ++(uip_conn->timer);
      if(uip_conn->timer == UIP_TIME_WAIT_TIMEOUT) {
	uip_conn->tcpstateflags = CLOSED;
      }
    } else if(uip_conn->tcpstateflags != CLOSED) {
      /* If the connection has outstanding data, we increase the
	 connection's timer and see if it has reached the RTO value
	 in which case we retransmit. */
      if(uip_conn->tcpstateflags & UIP_OUTSTANDING) {
	++(uip_conn->timer);
	if(uip_conn->timer ==
	   (UIP_RTO << rexmit_shift[uip_conn->nrtx])) {
	  uip_conn->timer = 0;
	  ++(uip_conn->nrtx);
	  if(uip_conn->nrtx == 8) {
	    /* XXX: we should call UIP_APPCALL() with uip_flags set to
               UIP_ABORTED or similar. But this will do for now. */
	    uip_conn->tcpstateflags = CLOSED;
	    return;
	  }
	  /* Ok, so we need to retransmit. We do this differently
	     depending on which state we are in. In ESTABLISHED, we
	     call upon the application so that it may prepare the
	     data for the retransmit. In SYN_RCVD, we resend the
	     SYNACK that we sent earlier and in LAST_ACK we have to
	     retransmit our FINACK. */
	  UIP_STAT(++uip_stat.tcp.rexmit);
	  switch(uip_conn->tcpstateflags & TS_MASK) {	    
	  case SYN_RCVD:
	    /* In the SYN_RCVD state, we should retransmit our
               SYNACK. */
	    goto tcp_send_synack;
	    
	  case ESTABLISHED:
	    /* In the ESTABLISHED state, we call upon the application
               to do the actual retransmit after which we jump into
               the code for sending out the packet (the apprexmit
               label). */
	    uip_len = 0;
	    uip_flags = UIP_REXMIT;
	    UIP_APPCALL();
	    goto apprexmit;
	    
	  case FIN_WAIT_1:
	  case CLOSING:
	  case LAST_ACK:
	    /* In all these states we should retransmit a FINACK. */
	    goto tcp_send_finack;
	    
	  }
	}
      } else if(uip_conn->tcpstateflags == ESTABLISHED) {
	/* If there was no need for a retransmission, we poll the
           application for new data. */
	uip_len = 0;
	uip_flags = UIP_POLL;
	UIP_APPCALL();
	goto appsend;
      }
    }   
    goto drop;
  }

  /* This is where the input processing starts. */
  UIP_STAT(++uip_stat.ip.recv);
  
  /* Check validity of the IP header. */  
  if(BUF->vhl != 0x45)  { /* IP version and header length. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.vhlerr);
    UIP_LOG("ip: invalid version or header length.");
    goto drop;
  }
  
  /* Check the size of the packet. If the size reported to us in
     uip_len doesn't match the size reported in the IP header, there
     has been a transmission error and we drop the packet. */
  
#if UIP_BUFSIZE > 255
  if(BUF->len[0] != (uip_len >> 8)) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.hblenerr);
    UIP_LOG("ip: invalid length, high byte.");
                               /* IP length, high byte. */
    goto drop;
  }
  if(BUF->len[1] != (uip_len & 0xff)) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.lblenerr);
    UIP_LOG("ip: invalid length, low byte.");
                               /* IP length, low byte. */
    goto drop;
  }
#else
  if(BUF->len[0] != 0) {        /* IP length, high byte. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.hblenerr);
    UIP_LOG("ip: invalid length, high byte.");
    goto drop;
  }
  if(BUF->len[1] != uip_len) {  /* IP length, low byte. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.lblenerr);
    UIP_LOG("ip: invalid length, low byte.");
    goto drop;
  }
#endif /* UIP_BUFSIZE > 255 */  

  if(BUF->ipoffset[0] & 0x3f) { /* We don't allow IP fragments. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.fragerr);
    UIP_LOG("ip: fragment dropped.");    
    goto drop;
  }

  /* Check if the packet is destined for our IP address. */
  if(BUF->destipaddr[0] != htons(((u16_t)UIP_IPADDR0 << 8) | UIP_IPADDR1)) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_LOG("ip: packet not for us.");        
    goto drop;
  }
  if(BUF->destipaddr[1] != htons(((u16_t)UIP_IPADDR2 << 8) | UIP_IPADDR3)) {
    UIP_STAT(++uip_stat.ip.drop);
    UIP_LOG("ip: packet not for us.");        
    goto drop;
  }

  if(uip_ipchksum() != 0xffff) { /* Compute and check the IP header
				    checksum. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.chkerr);
    UIP_LOG("ip: bad checksum.");    
    goto drop;
  }

  if(BUF->proto == IP_PROTO_TCP)  /* Check for TCP packet. If so, jump
                                     to the tcp_input label. */
    goto tcp_input;

  if(BUF->proto != IP_PROTO_ICMP) { /* We only allow ICMP packets from
				       here. */
    UIP_STAT(++uip_stat.ip.drop);
    UIP_STAT(++uip_stat.ip.protoerr);
    UIP_LOG("ip: neither tcp nor icmp.");        
    goto drop;
  }

  UIP_STAT(++uip_stat.icmp.recv);
  
  /* ICMP echo (i.e., ping) processing. This is simple, we only change
     the ICMP type from ECHO to ECHO_REPLY and adjust the ICMP
     checksum before we return the packet. */
  if(ICMPBUF->type != ICMP_ECHO) {
    UIP_STAT(++uip_stat.icmp.drop);
    UIP_STAT(++uip_stat.icmp.typeerr);
    UIP_LOG("icmp: not icmp echo.");
    goto drop;
  }

  ICMPBUF->type = ICMP_ECHO_REPLY;
  
  if(ICMPBUF->icmpchksum >= htons(0xffff - (ICMP_ECHO << 8))) {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8) + 1;
  } else {
    ICMPBUF->icmpchksum += htons(ICMP_ECHO << 8);
  }
  
  /* Swap IP addresses. */
  tmpport = BUF->destipaddr[0];
  BUF->destipaddr[0] = BUF->srcipaddr[0];
  BUF->srcipaddr[0] = tmpport;
  tmpport = BUF->destipaddr[1];
  BUF->destipaddr[1] = BUF->srcipaddr[1];
  BUF->srcipaddr[1] = tmpport;

  UIP_STAT(++uip_stat.icmp.sent);
  goto send;
  
 tcp_input:
  UIP_STAT(++uip_stat.tcp.recv);
    
  if(uip_tcpchksum() != 0xffff) {   /* Compute and check the TCP
				       checksum. */
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.chkerr);
    UIP_LOG("tcp: bad checksum.");    
    goto drop;
  }
  
  /* Demultiplex this segment. */
  /* First check any active connections. */
  for(uip_conn = &uip_conns[0]; uip_conn < &uip_conns[UIP_CONNS]; ++uip_conn) {
    if(uip_conn->tcpstateflags != CLOSED &&
       BUF->srcipaddr[0] == uip_conn->ripaddr[0] &&
       BUF->srcipaddr[1] == uip_conn->ripaddr[1] &&
       BUF->destport == uip_conn->lport &&
       BUF->srcport == uip_conn->rport)
      goto found;    
  }

  /* If we didn't find and active connection that expected the packet,
     either this packet is an old duplicate, or this is a SYN packet
     destined for a connection in LISTEN. If the SYN flag isn't set,
     it is an old packet and we send a RST. */
  if(BUF->flags != TCP_SYN)
    goto reset;
  
  tmpport = BUF->destport;
  /* Next, check listening connections. */  
  for(tmpcnt = 0; tmpcnt < UIP_LISTENPORTS &&
	          uip_listenports[tmpcnt] != 0;
      ++tmpcnt) {
    if(tmpport == uip_listenports[tmpcnt])
      goto found_listen;
  }
  /* No matching connection found, so we send a RST packet. */

  UIP_STAT(++uip_stat.tcp.synrst);
 reset:

  /* We do not send resets in response to resets. */
  if(BUF->flags & TCP_RST) 
    goto drop;

  UIP_STAT(++uip_stat.tcp.rst);
  
  BUF->flags = TCP_RST | TCP_ACK;
  uip_len = 40;
  BUF->tcpoffset = 5 << 4;

  /* Flip the seqno and ackno fields in the TCP header. We also have
     to increase the sequence number we are acknowledging. */
  tmpcnt = BUF->seqno[3];
  BUF->seqno[3] = BUF->ackno[3];  
  BUF->ackno[3] = tmpcnt + 1;
  
  tmpcnt = BUF->seqno[2];
  BUF->seqno[2] = BUF->ackno[2];  
  BUF->ackno[2] = tmpcnt;
  
  tmpcnt = BUF->seqno[1];
  BUF->seqno[1] = BUF->ackno[1];
  BUF->ackno[1] = tmpcnt;
  
  tmpcnt = BUF->seqno[0];
  BUF->seqno[0] = BUF->ackno[0];  
  BUF->ackno[0] = tmpcnt;

  /* If the least significant byte overflowed, we need to propagate
     the carry to the other bytes as well. */
  if(BUF->ackno[3] == 0) {
    ++BUF->ackno[2];
    if(BUF->ackno[2] == 0) {
      ++BUF->ackno[1];
      if(BUF->ackno[1] == 0) {
	++BUF->ackno[0];
      }
    }
  }
 
  /* Swap port numbers. */
  tmpport = BUF->srcport;
  BUF->srcport = BUF->destport;
  BUF->destport = tmpport;
  
  /* Swap IP addresses. */
  tmpport = BUF->destipaddr[0];
  BUF->destipaddr[0] = BUF->srcipaddr[0];
  BUF->srcipaddr[0] = tmpport;
  tmpport = BUF->destipaddr[1];
  BUF->destipaddr[1] = BUF->srcipaddr[1];
  BUF->srcipaddr[1] = tmpport;

  /* And send out the RST packet! */
  goto tcp_send_noconn;

  /* This label will be jumped to if we matched the incoming packet
     with a connection in LISTEN. In that case, we should create a new
     connection and send a SYNACK in return. */
 found_listen:
  /* First we check if there are any connections avaliable. Unused
     connections are kept in the same table as used connections, but
     unused ones have the tcpstate set to CLOSED. */
  for(tmpcnt = 0; tmpcnt < UIP_CONNS; ++tmpcnt) {
    if((uip_conns[tmpcnt].tcpstateflags & TS_MASK) == CLOSED) 
      goto found_unused_connection;
  }
  for(tmpcnt = 0; tmpcnt < UIP_CONNS; ++tmpcnt) {
    if((uip_conns[tmpcnt].tcpstateflags & TS_MASK) == TIME_WAIT) 
      goto found_unused_connection;
  }
  /* All connections are used already, we drop packet and hope that
     the remote end will retransmit the packet at a time when we have
     more spare connections. */
  UIP_STAT(++uip_stat.tcp.syndrop);
  UIP_LOG("tcp: found no unused connections.");
  goto drop;

  /* This label will be jumped to if we have found an unused
     connection that we can use. */
 found_unused_connection:
  uip_conn = &uip_conns[tmpcnt];

  /* Fill in the necessary fields for the new connection. */
  uip_conn->timer = 0;
  uip_conn->nrtx = 0;
  uip_conn->lport = BUF->destport;
  uip_conn->rport = BUF->srcport;
  uip_conn->ripaddr[0] = BUF->srcipaddr[0];
  uip_conn->ripaddr[1] = BUF->srcipaddr[1];
  uip_conn->tcpstateflags = SYN_RCVD | UIP_OUTSTANDING;  
  uip_conn->snd_nxt[0] = uip_conn->snd_nxt[1] =
    uip_conn->snd_nxt[2] = uip_conn->snd_nxt[3] = 0;

  /* Next ACK from peer should acknowledge sequence number 1. */
  uip_conn->ack_nxt[0] = uip_conn->ack_nxt[1] =
    uip_conn->ack_nxt[2] = 0;
  uip_conn->ack_nxt[3] = 1;

  /* rcv_nxt should be the seqno from the incoming packet + 1. */
  uip_conn->rcv_nxt[3] = BUF->seqno[3];
  uip_conn->rcv_nxt[2] = BUF->seqno[2];
  uip_conn->rcv_nxt[1] = BUF->seqno[1];
  uip_conn->rcv_nxt[0] = BUF->seqno[0];
  uip_add_rcv_nxt(1);

 tcp_send_synack:
  /* Our response will be a SYNACK. */
  BUF->flags = TCP_SYN | TCP_ACK;    

  /* We send out the TCP Maximum Segment Size option with our
     SYNACK. */
  BUF->optdata[0] = 2;
  BUF->optdata[1] = 4;
  BUF->optdata[2] = (UIP_TCP_MSS) / 256;
  BUF->optdata[3] = (UIP_TCP_MSS) & 255;
  uip_len = 44;
  BUF->tcpoffset = 6 << 4;
  goto tcp_send;

  /* This label will be jumped to if we found an active connection. */
 found:
  uip_flags = 0;

  /* We do a very naive form of TCP reset processing; we just accept
     any RST and kill our connection. We should in fact check if the
     sequence number of this reset is wihtin our advertised window
     before we accept the reset. */
  if(BUF->flags & TCP_RST) {
    uip_conn->tcpstateflags = CLOSED;
    UIP_LOG("tcp: got reset, aborting connection.");
    goto drop;
  }
  /* All segments that are come thus far should have the ACK flag set,
     otherwise we drop the packet. */
  if(!(BUF->flags & TCP_ACK)) {
    UIP_STAT(++uip_stat.tcp.drop);
    UIP_STAT(++uip_stat.tcp.ackerr);
    UIP_LOG("tcp: dropped non-ack segment.");
    goto drop;
  }
      
  /* Calculated the length of the data, if the application has sent
     any data to us. */
  tmpcnt = (BUF->tcpoffset >> 4) << 2;
  /* uip_len will contain the length of the actual TCP data. This is
     calculated by subtracing the length of the TCP header (in
     tmpcnt) and the length of the IP header (20 bytes). */
  uip_len = uip_len - tmpcnt - 20;

  /* First, check if the sequence number of the incoming packet is
     what we're expecting next. If not, we send out an ACK with the
     correct numbers in. */
  if(uip_len > 0 &&
     (BUF->seqno[0] != uip_conn->rcv_nxt[0] ||
      BUF->seqno[1] != uip_conn->rcv_nxt[1] ||
      BUF->seqno[2] != uip_conn->rcv_nxt[2] ||
      BUF->seqno[3] != uip_conn->rcv_nxt[3])) {
    goto tcp_send_ack;
  }

  /* Next, check if the incoming segment acknowledges any outstanding
     data. */
  if(BUF->ackno[0] == uip_conn->ack_nxt[0] &&
     BUF->ackno[1] == uip_conn->ack_nxt[1] &&
     BUF->ackno[2] == uip_conn->ack_nxt[2] &&
     BUF->ackno[3] == uip_conn->ack_nxt[3]) {
    uip_conn->snd_nxt[0] = uip_conn->ack_nxt[0];
    uip_conn->snd_nxt[1] = uip_conn->ack_nxt[1];
    uip_conn->snd_nxt[2] = uip_conn->ack_nxt[2];
    uip_conn->snd_nxt[3] = uip_conn->ack_nxt[3];
    if(uip_conn->tcpstateflags & UIP_OUTSTANDING) {
      uip_flags = UIP_ACKDATA;
      uip_conn->tcpstateflags &= ~UIP_OUTSTANDING;
    }
  }

  /* Do different things depending on in what state the connection is. */
  switch(uip_conn->tcpstateflags & TS_MASK) {
    /* CLOSED and LISTEN are not handled here, SYN_SENT is not
	implemented. CLOSE_WAIT is not implemented either, since we
	force the application to close when the peer sends a FIN
	(hence the application goes directly from ESTABLISHED to
	LAST_ACK). */
  case SYN_RCVD:
    /* In SYN_RCVD we have sent out a SYNACK in response to a SYN, and
       we are waiting for an ACK that acknowledges the data we sent
       out the last time. Therefore, we want to have the UIP_ACKDATA
       flag set. If so, we enter the ESTABLISHED state. */
    if(uip_flags & UIP_ACKDATA) {
      uip_conn->tcpstateflags = ESTABLISHED;
      uip_flags = UIP_ACCEPT;
      uip_len = 0;
      UIP_APPCALL();
      goto appsend;
    }
    goto drop;
  case ESTABLISHED:
    /* In the ESTABLISHED state, we call upon the application to feed
    data into the uip_buf. If the UIP_ACKDATA flag is set, the
    application should put new data into the buffer, otherwise we are
    retransmitting an old segment, and the application should put that
    data into the buffer.

    If the incoming packet is a FIN, we should close the connection on
    this side as well, and we send out a FIN and enter the LAST_ACK
    state. We require that the FIN will have to acknowledge all
    outstanding data, otherwise we drop it. BUG: this will not work if
    a FIN segment contains data! */

    if(BUF->flags & TCP_FIN) {
      uip_add_rcv_nxt(1 + uip_len);
      uip_flags = UIP_CLOSED;
      uip_len = 0;
      UIP_APPCALL();
      uip_add_ack_nxt(1);
      uip_conn->tcpstateflags = LAST_ACK | UIP_OUTSTANDING;
      uip_conn->nrtx = 0;
    tcp_send_finack:
      BUF->flags = TCP_FIN | TCP_ACK;      
      goto tcp_send_nodata;
    }

    /* If uip_len > 0 we have TCP data in the packet, and we flag this
       by setting the UIP_NEWDATA flag and update the sequence number
       we acknowledge. */
    if(uip_len > 0) {
      uip_flags |= UIP_NEWDATA;
      uip_add_rcv_nxt(uip_len);
    }
    

    /* If this packet constitutes an ACK for outstanding data (flagged
       by the UIP_ACKDATA flag, we should call the application since it
       might want to send more data. If the incoming packet had data
       from the peer (as flagged by the UIP_NEWDATA flag), the
       application must also be notified.

       When the application is called, the global variable uip_len
       contains the length of the incoming data. The application can
       access the incoming data through the global pointer
       uip_appdata, which usually points 40 bytes into the uip_buf
       array.

       If the application wishes to send any data, this data should be
       put into the uip_appdata and the length of the data should be
       put into uip_len. If the application don't have any data to
       send, uip_len must be set to 0. */
    if(uip_flags & (UIP_NEWDATA | UIP_ACKDATA)) {
      UIP_APPCALL();

    appsend:
      if(uip_flags & UIP_ABORT) {
	uip_conn->tcpstateflags = CLOSED;
	BUF->flags = TCP_RST | TCP_ACK;
	goto tcp_send_nodata;
      }

      if(uip_flags & UIP_CLOSE) {
	uip_add_ack_nxt(1);
	uip_conn->tcpstateflags = FIN_WAIT_1 | UIP_OUTSTANDING;
	uip_conn->nrtx = 0;
	BUF->flags = TCP_FIN | TCP_ACK;
	goto tcp_send_nodata;	
      }

      /* If uip_len > 0, the application has data to be sent, in which
         case we set the UIP_OUTSTANDING flag in the connection
         structure. But we cannot send data if the application already
         has outstanding data. */
      if(uip_len > 0 &&
	 !(uip_conn->tcpstateflags & UIP_OUTSTANDING)) {
	uip_conn->tcpstateflags |= UIP_OUTSTANDING;
	uip_conn->nrtx = 0;
	uip_add_ack_nxt(uip_len);
      } else {
	uip_len = 0;
      }
    apprexmit:
      /* If the application has data to be sent, or if the incoming
         packet had new data in it, we must send out a packet. */
      if(uip_len > 0 || (uip_flags & UIP_NEWDATA)) {
	/* Add the length of the IP and TCP headers. */
	uip_len = uip_len + 40;
	/* We always set the ACK flag in response packets. */
	BUF->flags = TCP_ACK;
	/* Send the packet. */
	goto tcp_send_noopts;
      }
    }
    goto drop;
  case LAST_ACK:
    /* We can close this connection if the peer has acknowledged our
       FIN. This is indicated by the UIP_ACKDATA flag. */     
    if(uip_flags & UIP_ACKDATA) {
      uip_conn->tcpstateflags = CLOSED;
    }
    break;
    
  case FIN_WAIT_1:
    /* The application has closed the connection, but the remote host
       hasn't closed its end yet. Thus we do nothing but wait for a
       FIN from the other side. */
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      if(uip_flags & UIP_ACKDATA) {
	uip_conn->tcpstateflags = TIME_WAIT;
	uip_conn->timer = 0;
      } else {
	uip_conn->tcpstateflags = CLOSING | UIP_OUTSTANDING;
      }
      uip_add_rcv_nxt(1);
      goto tcp_send_ack;
    } else if(uip_flags & UIP_ACKDATA) {
      uip_conn->tcpstateflags = FIN_WAIT_2;
      goto drop;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;
      
  case FIN_WAIT_2:
    if(uip_len > 0) {
      uip_add_rcv_nxt(uip_len);
    }
    if(BUF->flags & TCP_FIN) {
      uip_conn->tcpstateflags = TIME_WAIT;
      uip_conn->timer = 0;
      uip_add_rcv_nxt(1);
      goto tcp_send_ack;
    }
    if(uip_len > 0) {
      goto tcp_send_ack;
    }
    goto drop;
    
  case CLOSING:
    if(uip_flags & UIP_ACKDATA) {
      uip_conn->tcpstateflags = TIME_WAIT;
      uip_conn->timer = 0;
    }
  }  
  goto drop;
  

  /* We jump here when we are ready to send the packet, and just want
     to set the appropriate TCP sequence numbers in the TCP header. */
 tcp_send_ack:
  BUF->flags = TCP_ACK;
 tcp_send_nodata:
  uip_len = 40;
 tcp_send_noopts:
  BUF->tcpoffset = 5 << 4;
 tcp_send:
  /* We're done with the input processing. We are now ready to send a
     reply. Our job is to fill in all the fields of the TCP and IP
     headers before calculating the checksum and finally send the
     packet. */
  
  BUF->ackno[0] = uip_conn->rcv_nxt[0];
  BUF->ackno[1] = uip_conn->rcv_nxt[1];
  BUF->ackno[2] = uip_conn->rcv_nxt[2];
  BUF->ackno[3] = uip_conn->rcv_nxt[3];
  
  BUF->seqno[0] = uip_conn->snd_nxt[0];
  BUF->seqno[1] = uip_conn->snd_nxt[1];
  BUF->seqno[2] = uip_conn->snd_nxt[2];
  BUF->seqno[3] = uip_conn->snd_nxt[3];

  BUF->srcport  = uip_conn->lport;
  BUF->destport = uip_conn->rport;

#if BYTE_ORDER == BIG_ENDIAN  
  BUF->srcipaddr[0] = ((UIP_IPADDR0 << 8) | UIP_IPADDR1);
  BUF->srcipaddr[1] = ((UIP_IPADDR2 << 8) | UIP_IPADDR3);
#else
  BUF->srcipaddr[0] = ((UIP_IPADDR1 << 8) | UIP_IPADDR0);
  BUF->srcipaddr[1] = ((UIP_IPADDR3 << 8) | UIP_IPADDR2);
#endif /* BYTE_ORDER == BIG_ENDIAN */

  BUF->destipaddr[0] = uip_conn->ripaddr[0];
  BUF->destipaddr[1] = uip_conn->ripaddr[1];
  
 tcp_send_noconn:

  BUF->vhl = 0x45;
  BUF->tos = 0;
  BUF->ipoffset[0] = BUF->ipoffset[1] = 0;
  BUF->ttl  = UIP_TTL;
  BUF->proto = IP_PROTO_TCP;

#if UIP_BUFSIZE > 255
  BUF->len[0] = (uip_len >> 8);
  BUF->len[1] = (uip_len & 0xff);
#else
  BUF->len[0] = 0;
  BUF->len[1] = uip_len;
#endif /* UIP_BUFSIZE > 255 */
  
  BUF->wnd[0] = ((UIP_TCP_MSS) >> 8);
  BUF->wnd[1] = ((UIP_TCP_MSS) & 0xff); 

  ++ipid;
  BUF->ipid[0] = ipid >> 8;
  BUF->ipid[1] = ipid & 0xff;
  
  /* Calculate IP and TCP checksums. */
  BUF->ipchksum = 0;
  BUF->ipchksum = ~(uip_ipchksum());
  BUF->tcpchksum = 0;
  BUF->tcpchksum = ~(uip_tcpchksum());

  UIP_STAT(++uip_stat.tcp.sent);
 send:
  UIP_STAT(++uip_stat.ip.sent);
  /* The data that should be sent is not present in the uip_buf, and
     the length of the data is in the variable uip_len. It is not our
     responsibility to do the actual sending of the data however. That
     is taken care of by the wrapper code, and only if uip_len > 0. */
  return;
 drop:
  uip_len = 0;
  return;
}
/*-----------------------------------------------------------------------------------*/
