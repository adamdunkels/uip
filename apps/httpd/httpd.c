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
 * $Id: httpd.c,v 1.19 2001/11/20 20:51:58 adam Exp $
 *
 */


#include "uip.h"
#include "httpd.h"
#include "fs.h"
#include "fsdata.h"
#include "cgi.h"

#define NULL (void *)0

/* The HTTP server states: */
#define HTTP_NOGET        0
#define HTTP_FILE         1
#define HTTP_TEXT         2
#define HTTP_FUNC         3
#define HTTP_END          4

#include <stdio.h>
#define PRINT(x) printf("%s", x)
#define PRINTLN(x) printf("%s\n", x)
/*#define PRINT(x)
  #define PRINTLN(x)*/

struct httpd_state *hs;

extern const struct fsdata_file file_index_html;

static void next_scriptline(void);
static void next_scriptstate(void);

#define ISO_G        0x47
#define ISO_E        0x45
#define ISO_T        0x54
#define ISO_slash    0x2f    
#define ISO_c        0x63
#define ISO_g        0x67
#define ISO_i        0x69
#define ISO_space    0x20
#define ISO_nl       0x0a
#define ISO_cr       0x0d
#define ISO_a        0x61
#define ISO_t        0x74
#define ISO_hash     0x23
#define ISO_period   0x2e


/*-----------------------------------------------------------------------------------*/
void
httpd_init(void)
{
  fs_init();
  
  /* Set up a web server. */
  uip_listen(80);
}
/*-----------------------------------------------------------------------------------*/
void
httpd(void)
{
  struct fs_file fsfile;  
  u8_t i;
  
  switch(uip_conn->lport) {
    /* This is the web server: */
  case htons(80):
    uip_len = 0;
    hs = (struct httpd_state *)(uip_conn->appstate);

    /* We use the uip_flags variable to deduce why we were called. If
       uip_flags & UIP_ACCEPT is non-zero, we were called because a
       remote host has connected to us. If uip_flags & UIP_NEWDATA is
       non-zero, we were called because the remote host has sent us
       new data, and if uip_flags & UIP_ACKDATA is non-zero, the
       remote host has acknowledged the data we previously sent to
       it. */
    if(uip_flags & UIP_ACCEPT) {
      /* Since we have just been connected with the remote host, we
         reset the state for this connection. The ->count variable
         contains the amount of data that is yet to be sent to the
         remote host, and the ->state is set to HTTP_NOGET to signal
         that we haven't received any HTTP GET request for this
         connection yet. */
      hs->state = HTTP_NOGET;
      hs->count = 0;
      uip_len = 0;
      return;

    } else if(uip_flags & UIP_POLL) {
      /* If we are polled ten times, we abort the connection. This is
         because we don't want connections lingering indefinately in
         the system. */
      if(hs->count++ == 10) {
	uip_flags = UIP_ABORT;
      }
      return;
    } else if(uip_flags & UIP_NEWDATA && hs->state == HTTP_NOGET) {
      /* This is the first data we receive, and it should contain a
	 GET. */
      
      /* Check for GET. */
      if(uip_appdata[0] != ISO_G ||
	 uip_appdata[1] != ISO_E ||
	 uip_appdata[2] != ISO_T ||
	 uip_appdata[3] != ISO_space) {
	uip_flags = UIP_ABORT;
	return;
      }
	       
      /* Find the file we are looking for. */
      for(i = 4; i < 40; ++i) {
	if(uip_appdata[i] == ISO_space ||
	   uip_appdata[i] == ISO_cr ||
	   uip_appdata[i] == ISO_nl) {
	  uip_appdata[i] = 0;
	  break;
	}
      }

      PRINT("request for file ");
      PRINTLN(&uip_appdata[4]);
      
      if(!fs_open((const char *)&uip_appdata[4], &fsfile)) {
	PRINTLN("couldn't open file");
	fs_open(file_index_html.name, &fsfile);
      } 

      if(uip_appdata[4] == ISO_slash &&
	 uip_appdata[5] == ISO_c &&
	 uip_appdata[6] == ISO_g &&
	 uip_appdata[7] == ISO_i &&
	 uip_appdata[8] == ISO_slash) {
	/* If the request is for a file that starts with "/cgi/", we
           prepare for invoking a script. */	
	hs->script = fsfile.data;
	next_scriptstate();
      } else {
	hs->script = NULL;
	/* The web server is now no longer in the HTTP_NOGET state, but
	   in the HTTP_FILE state since is has now got the GET from
	   the client and will start transmitting the file. */
	hs->state = HTTP_FILE;

	/* Point the file pointers in the connection state to point to
	   the first byte of the file. */
	hs->dataptr = fsfile.data;
	hs->count = fsfile.len;	
      }     
    }

    
    if(hs->state != HTTP_FUNC) {
      /* Check if the client (remote end) has acknowledged any data that
	 we've previously sent. If so, we move the file pointer further
	 into the file and send back more data. If we are out of data to
	 send, we set the UIP_CLOSE flag in the uip_flags variable to
	 tell uIP that the connection should be closed. */
      if(uip_flags & UIP_ACKDATA) {
	
	if(hs->count >= UIP_TCP_MSS) {
	  hs->count -= UIP_TCP_MSS;
	  hs->dataptr += UIP_TCP_MSS;
	} else {
	  hs->count = 0;
	}
	
	if(hs->count == 0) {
	  if(hs->script != NULL) {
	    next_scriptline();
	    next_scriptstate();
	  } else {
	    uip_flags = UIP_CLOSE;		  
	  }
	}
      }         
    }
    
    if(hs->state == HTTP_FUNC) {
      /* Call the CGI function. */
      if(cgitab[hs->script[2] - ISO_a]()) {
	/* If the function returns non-zero, we jump to the next line
           in the script. */
	next_scriptline();
	next_scriptstate();
      }
    }

    if(hs->state != HTTP_FUNC && !(uip_flags & UIP_POLL)) {
      /* The uip_len variable should contain the length of the
	 outbound packet. */
      if(hs->count > UIP_TCP_MSS) {
	uip_len = UIP_TCP_MSS;
      } else {
	uip_len = hs->count;
      }
      
      /* Set the uip_appdata pointer to point to the data we wish to
	 send. */
      uip_appdata = hs->dataptr;
    }

    /* Finally, return to uIP. Our outgoing packet will soon be on its
       way... */
    return;
      
  default:
    /* Should never happen. */
    break;
  }  
}
/*-----------------------------------------------------------------------------------*/
static void
next_scriptline(void)
{
  /* Loop until we find a newline character. */
  do {
    ++(hs->script);
  } while(hs->script[0] != ISO_nl);

  /* Eat up the newline as well. */
  ++(hs->script);
}
/*-----------------------------------------------------------------------------------*/
static void
next_scriptstate(void)
{
  struct fs_file fsfile;
  u8_t i;

 again:
  switch(hs->script[0]) {
  case ISO_t:
    /* Send a text string. */
    hs->state = HTTP_TEXT;
    hs->dataptr = &hs->script[2];

    /* Calculate length of string. */
    for(i = 0; hs->dataptr[i] != ISO_nl; ++i);
    hs->count = i;    
    break;
  case ISO_c:
    /* Call a function. */
    hs->state = HTTP_FUNC;
    hs->dataptr = NULL;
    hs->count = 0;
    uip_flags = 0;
    break;
  case ISO_i:   
    /* Include a file. */
    hs->state = HTTP_FILE;
    if(!fs_open(&hs->script[2], &fsfile)) {
      uip_flags = UIP_ABORT;
    }
    hs->dataptr = fsfile.data;
    hs->count = fsfile.len;
    break;
  case ISO_hash:
    /* Comment line. */
    next_scriptline();
    goto again;
    break;
  case ISO_period:
    /* End of script. */
    hs->state = HTTP_END;
    uip_flags = UIP_CLOSE;
    break;
  default:
    uip_flags = UIP_ABORT;
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
