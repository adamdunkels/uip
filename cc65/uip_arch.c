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
 * $Id: uip_arch.c,v 1.1 2001/11/20 20:49:45 adam Exp $
 *
 */


#include "uip_arch.h"
#include "uip.h"

/* This is assembler versions of the uIP support functions for the
   6502 CPU. Note that the checksum function ("_chksum()", defined
   under uip_ipchksum()) does not support buffer sizes > 255 bytes. */

/*-----------------------------------------------------------------------------------*/
static u16_t chksum_ptr, chksum_len, chksum_tmp;
static u16_t chksum(void);
/*-----------------------------------------------------------------------------------*/
u16_t
uip_ipchksum(void)
{
  chksum_ptr = (u16_t)uip_buf;
  chksum_len = 20;  
  return chksum();
}
/*asm("__chksum:");*/
u16_t chksum(void) {
asm("lda #0");
asm("sta tmp1");
asm("sta tmp1+1");
asm("lda _chksum_ptr");
asm("sta ptr1");
asm("lda _chksum_ptr+1");
asm("sta ptr1+1");
asm("lda _chksum_len");
asm("lsr");
asm("bcc chksum_noodd");
asm("pha");
asm("ldy _chksum_len");
asm("dey");
asm("lda (ptr1),y");
asm("sta tmp1");
asm("pla");
asm("chksum_noodd:");
asm("asl");
asm("tay");
asm("clc");
asm("php");
asm("chksum_loop1:");
asm("cpy #0");
asm("beq chksum_loop1_end");
asm("plp");
asm("dey");
asm("dey");
asm("lda (ptr1),y");
asm("adc tmp1");
asm("sta tmp1");
asm("iny");
asm("lda (ptr1),y");
asm("adc tmp1+1");
asm("sta tmp1+1");
asm("dey");
asm("php");
asm("jmp chksum_loop1");
asm("chksum_loop1_end:");
asm("plp");
asm("chksum_endloop:");
asm("lda tmp1");
asm("adc #0");
asm("sta tmp1");
asm("lda tmp1+1");
asm("adc #0");
asm("sta tmp1+1");
asm("bcs chksum_endloop");
asm("lda tmp1");
asm("ldx tmp1+1");
asm("rts");
}
/*-----------------------------------------------------------------------------------*/
u16_t
uip_tcpchksum(void)
{  
  chksum_ptr = (u16_t)&uip_buf[20];
  chksum_len = 20;  
  chksum_tmp = chksum();

  chksum_ptr = (u16_t)uip_appdata;
  asm("lda _uip_buf+3");
  asm("sec");
  asm("sbc #40");
  asm("sta _chksum_len");
  /*  asm("lda _uip_buf+2");
      asm("sbc #0");*/
  asm("lda #0");
  asm("sta _chksum_len+1");
  asm("jsr %v", chksum);

  asm("clc");
  asm("adc _chksum_tmp");
  asm("sta _chksum_tmp");
  asm("txa");
  asm("adc _chksum_tmp+1");
  asm("sta _chksum_tmp+1");

  asm("tcpchksum_loop1:");
  asm("lda _chksum_tmp");
  asm("adc #0");
  asm("sta _chksum_tmp");
  asm("lda _chksum_tmp+1");
  asm("adc #0");
  asm("sta _chksum_tmp+1");
  asm("bcs tcpchksum_loop1");


  asm("lda _uip_buf+3");
  asm("sec");
  asm("sbc #20");
  asm("sta _chksum_len");
  asm("lda _uip_buf+2");
  asm("sbc #0");
  asm("sta _chksum_len+1");
  
  
  asm("ldy #$0c");
  asm("clc");
  asm("php");
  asm("tcpchksum_loop2:");
  asm("plp");
  asm("lda _uip_buf,y");
  asm("adc _chksum_tmp");
  asm("sta _chksum_tmp");
  asm("iny");
  asm("lda _uip_buf,y");
  asm("adc _chksum_tmp+1");
  asm("sta _chksum_tmp+1");
  asm("iny");
  asm("php");
  asm("cpy #$14");
  asm("bne tcpchksum_loop2");

  asm("plp");
  
  asm("lda _chksum_tmp");
  asm("adc #0");
  asm("sta _chksum_tmp");
  asm("lda _chksum_tmp+1");
  asm("adc #6");  /* IP_PROTO_TCP */
  asm("sta _chksum_tmp+1");

  
  asm("lda _chksum_tmp");
  asm("adc _chksum_len+1");
  asm("sta _chksum_tmp");
  asm("lda _chksum_tmp+1");
  asm("adc _chksum_len");
  asm("sta _chksum_tmp+1");

  

  asm("tcpchksum_loop3:");
  asm("lda _chksum_tmp");
  asm("adc #0");
  asm("sta _chksum_tmp");
  asm("lda _chksum_tmp+1");
  asm("adc #0");
  asm("sta _chksum_tmp+1");
  asm("bcs tcpchksum_loop3");


  return chksum_tmp;
}
/*-----------------------------------------------------------------------------------*/
/*void
uip_add_rcv_nxt(u8_t n)
{
  uip_conn->rcv_nxt[3] += n;
  if(uip_conn->rcv_nxt[3] < n) {
    uip_conn->rcv_nxt[2]++;  
    if(uip_conn->rcv_nxt[2] == 0) {
      uip_conn->rcv_nxt[1]++;    
      if(uip_conn->rcv_nxt[1] == 0) {
	uip_conn->rcv_nxt[0]++;
      }
    }
  }
}*/
void __fastcall__
uip_add_rcv_nxt(u8_t n) {
  /*asm("_uip_add_rcv_nxt:");*/
  asm("pha");
  asm("lda _uip_conn");
  asm("sta ptr1");
  asm("lda _uip_conn+1");
  asm("sta ptr1+1");
  asm("pla");
  asm("clc");
  asm("ldy #12");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
#if UIP_BUFSIZE > 255
  asm("txa");
#else
  asm("lda #0");
#endif /* UIP_BUFSIZE > 255 */
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
  asm("lda #0");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
  asm("lda #0");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  /*asm("rts");
  asm(".export _uip_add_rcv_nxt");*/
}
/*-----------------------------------------------------------------------------------*/
/*void 
uip_add_ack_nxt(u8_t n)
{
  uip_conn->ack_nxt[3] += n;
  if(uip_conn->ack_nxt[3] < n) {
    uip_conn->ack_nxt[2]++;   
    if(uip_conn->ack_nxt[2] == 0) {
      uip_conn->ack_nxt[1]++;
      if(uip_conn->ack_nxt[1] == 0) {
	uip_conn->ack_nxt[0]++;
      }
    }
  }
}*/
void __fastcall__
uip_add_ack_nxt(u8_t n) {
  /*asm("_uip_add_ack_nxt:");*/
  asm("pha");
  asm("lda _uip_conn");
  asm("sta ptr1");
  asm("lda _uip_conn+1");
  asm("sta ptr1+1");
  asm("pla");
  asm("clc");
  asm("ldy #20");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
#if UIP_BUFSIZE > 255
  asm("txa");
#else
  asm("lda #0");
#endif /* UIP_BUFSIZE > 255 */
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
  asm("lda #0");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  asm("dey");
  asm("lda #0");
  asm("adc (ptr1),y");
  asm("sta (ptr1),y");
  /*asm("rts");
  asm(".export _uip_add_ack_nxt");*/
}
/*-----------------------------------------------------------------------------------*/


