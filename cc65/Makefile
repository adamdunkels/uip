# Copyright (c) 2001, Adam Dunkels.
# All rights reserved. 
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions 
# are met: 
# 1. Redistributions of source code must retain the above copyright 
#    notice, this list of conditions and the following disclaimer. 
# 2. Redistributions in binary form must reproduce the above copyright 
#    notice, this list of conditions and the following disclaimer in the 
#    documentation and/or other materials provided with the distribution. 
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by Adam Dunkels.
# 4. The name of the author may not be used to endorse or promote
#    products derived from this software without specific prior
#    written permission.  
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
#
# This file is part of the uIP TCP/IP stack.
#
# $Id: Makefile,v 1.2 2001/11/24 16:03:59 adam Exp $
#

# Change the target system type here
SYS=c64

CC=cc65
AS=ca65
AR=ar65
ARFLAGS=a
CFLAGS= -I ../uip -I ../apps/httpd -t $(SYS) -Oi

%.o:   
	$(CC) $(CFLAGS) -o $(patsubst %c, %s, $(notdir $<)) $<
	$(AS) -o $@ $(AFLAGS) $(*).s

uip: uip.o uip_arch.o rs232dev.o main.o httpd.o fs.o cgi.o
	cl65 -o uip -t $(SYS) $^


tunslip: tunslip.c
	gcc -o tunslip tunslip.c

rs232dev.o: rs232dev.c ../uip/uip.h uipopt.h
main.o: main.c ../uip/uip.h uipopt.h \
	rs232dev.h
uip.o: ../uip/uip.c uipopt.h
httpd.o: ../apps/httpd/httpd.c ../apps/httpd/httpd.h ../uip/uip.h uipopt.h \
	../apps/httpd/fs.c ../apps/httpd/fs.h \
	../apps/httpd/fsdata.c ../apps/httpd/fsdata.h
cgi.o: ../apps/httpd/cgi.c ../apps/httpd/cgi.h
fs.o: ../apps/httpd/fs.c ../apps/httpd/fs.h ../apps/httpd/fsdata.c
uip_arch.o: uip_arch.c uip_arch.h ../uip/uip.h uipopt.h \
	../apps/httpd/httpd.h


clean:
	rm -f *.o *~ *core uip *.s
