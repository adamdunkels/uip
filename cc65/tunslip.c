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
 * $Id: tunslip.c,v 1.1 2001/11/20 20:49:45 adam Exp $
 *
 */


#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/*-----------------------------------------------------------------------------------*/
void
do_slip(int infd, int outfd)
{
  static unsigned char inbuf[2000];
  static int inbufptr = 0;
  unsigned char c;

  if(read(infd, &c, 1) == -1) {
    perror("do_slip: read");
  }
  fprintf(stderr, ".");
  switch(c) {
  case SLIP_END:
    if(inbufptr > 0) {
      if(write(outfd, inbuf, inbufptr) == -1) {
        perror("do_slip: write");
      }
      inbufptr = 0;
    }
    return;
  case SLIP_ESC:
    if(read(infd, &c, 1) == -1) {
      perror("do_slip: read after esc");
    }
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
    inbuf[inbufptr++] = c;
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
void
slip_send(int outfd, unsigned char data)
{
  unsigned char d;

  d = data;
  write(outfd, &d, 1);
}
/*-----------------------------------------------------------------------------------*/
void
do_tun(int infd, int outfd)
{
  unsigned char inbuf[2000];  
  int i, size;

  if((size = read(infd, inbuf, 2000)) == -1) {
    perror("do_tun: read");
  }
  
  for(i = 0; i < size; i++) {
    switch(inbuf[i]) {
    case SLIP_END:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_END);
      break;
    case SLIP_ESC:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_ESC);
      break;
    default:
      slip_send(outfd, inbuf[i]);
      break;
    }
    
  }
  slip_send(outfd, SLIP_END);
}
/*-----------------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  int tunfd, infd, outfd, maxfd;
  int ret;
  fd_set fdset;
  
  infd = STDIN_FILENO;
  outfd = STDOUT_FILENO;

  fprintf(stderr, "tunslip started...\n");
  tunfd = open("/dev/tun0", O_RDWR);
  if(tunfd == -1) {
    perror("main: open");
    exit(1);
  }
  fprintf(stderr, "opened device...\n");
#ifdef linux
  system("ifconfig tun0 inet 192.168.0.2 192.168.0.1");
  
  system("route add -net 192.168.0.0 netmask 255.255.255.0 dev tun0");
#else
  /*  system("ifconfig tun0 inet sidewalker rallarsnus");  */
  system("ifconfig tun0 inet 192.168.0.1 192.168.0.2");
#endif /* linux */

  fprintf(stderr, "configured interface...\n");

  while(1) {
    maxfd = 0;
    FD_ZERO(&fdset);
    FD_SET(infd, &fdset);
    if(infd > maxfd) maxfd = infd;
    
    FD_SET(outfd, &fdset);
    if(outfd > maxfd) maxfd = outfd;
    
    FD_SET(tunfd, &fdset);
    if(tunfd > maxfd) maxfd = tunfd;

    ret = select(maxfd + 1, &fdset, NULL, NULL, NULL);
    if(ret == -1) {
      perror("select");
      exit(0);
    } else if(ret > 0) {
      if(FD_ISSET(infd, &fdset)) {
        do_slip(infd, tunfd);
      } else if(FD_ISSET(tunfd, &fdset)) {
        do_tun(tunfd, outfd);
      } else {
	exit(0);
      } 
    } else {
      exit(0);
    }
  }
  
}
/*-----------------------------------------------------------------------------------*/
