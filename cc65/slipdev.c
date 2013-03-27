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
 * $Id: slipdev.c,v 1.1 2001/11/20 20:49:45 adam Exp $
 *
 */


#include "uip.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335


#define SIO_RECV(c)  while(rs232_get(&c) == RS_ERR_NO_DATA)
#define SIO_SEND(c)  while(rs232_put(c) == RS_ERR_OVERFLOW)

#define MAX_SIZE UIP_BUFSIZE

static const unsigned char slip_end = SLIP_END, 
                           slip_esc = SLIP_ESC, 
                           slip_esc_end = SLIP_ESC_END, 
                           slip_esc_esc = SLIP_ESC_ESC;

/*-----------------------------------------------------------------------------------*/
void
slipdev_send(void)
{
  u8_t i;
  u8_t *ptr;
  u8_t c;

  SIO_SEND(slip_end);

  ptr = uip_buf;
  for(i = 0; i < uip_len; i++) {
    c = *ptr++;
    switch(c) {
    case SLIP_END:
      SIO_SEND(slip_esc);
      SIO_SEND(slip_esc_end);
      break;
    case SLIP_ESC:
      SIO_SEND(slip_esc);
      SIO_SEND(slip_esc_esc);
      break;
    default:
      SIO_SEND(c);
      break;
    }
  }
  SIO_SEND(slip_end);
}
/*-----------------------------------------------------------------------------------*/
u8_t
slipdev_read(void)
{
  u8_t c;

 start:
  uip_len = 0;
  while(1) {
    if(uip_len >= MAX_SIZE) {
      goto start;
    }
    SIO_RECV(c);
    switch(c) {
    case SLIP_END:
      if(uip_len > 0) {
        return uip_len;
      } else {
	goto start;
      }
      break;
    case SLIP_ESC:
      SIO_RECV(c);
      switch(c) {
      case SLIP_ESC_END:
        c = SLIP_END;
        break;
      case SLIP_ESC_ESC:
        c = SLIP_ESC;
        break;
      }
      /* FALLTHROUGH */
    default:
      if(uip_len < MAX_SIZE) {
	uip_buf[uip_len] = c;
        uip_len++;
      }
      break;
    }
    
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void
slipdev_init(void)
{
}
/*-----------------------------------------------------------------------------------*/







