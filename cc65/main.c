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
 * $Id: main.c,v 1.1 2001/11/20 20:49:45 adam Exp $
 *
 */


#include "uip.h"
#include "rs232dev.h"
#include "httpd.h"

#include <stdio.h>
#include <6502.h>
#include <conio.h>
#include <rs232.h>
#include <time.h>

/*-----------------------------------------------------------------------------------*/
void
main(void)
{
  u8_t i;
  u16_t start, current;
  
  rs232dev_init();
  uip_init();
  httpd_init();

  printf("uIP TCP/IP stack and web server running\n");
  printf("Press any key to stop.\n");

  
  start = clock();
  
  while(!kbhit()) {
    uip_len = rs232dev_poll();
    if(uip_len > 0) {
      uip_process(UIP_DATA);
      if(uip_len > 0) {
	rs232dev_send();
      }
    }
    current = clock();
    if((int)(current - start) >= CLK_TCK) {
      for(i = 0; i < UIP_CONNS; ++i) {
	uip_periodic(i);
	if(uip_len > 0) {
	  rs232dev_send();
	}
      }      
      start = current;
    }
  }

  rs232_done();
  (void)cgetc();
  printf("TCP/IP stack and web server stopped.\n");
}
/*-----------------------------------------------------------------------------------*/
