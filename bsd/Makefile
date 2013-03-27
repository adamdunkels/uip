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
# $Id: Makefile,v 1.6 2001/11/20 20:49:45 adam Exp $
#

CC=gcc
CFLAGS=-Wall -g -fpack-struct -I../uip -I. -I../apps/httpd

%.o:
	$(CC) $(CFLAGS) -c $(<:.o=.c)


uip: uip.o uip_arch.o tundev.o httpd.o main.o fs.o cgi.o

httpd.o: ../apps/httpd/httpd.c ../apps/httpd/httpd.h ../uip/uip.h \
 uipopt.h ../apps/httpd/fs.h
cgi.o: ../apps/httpd/cgi.c ../apps/httpd/cgi.h
fs.o: ../apps/httpd/fs.c ../uip/uip.h uipopt.h ../apps/httpd/httpd.h \
 ../apps/httpd/fs.h ../apps/httpd/fsdata.h \
 ../apps/httpd/fsdata.c
tundev.o: tundev.c ../uip/uip.h
main.o: main.c ../uip/uip.h uipopt.h ../apps/httpd/httpd.h tundev.h
uip_arch.o: uip_arch.c uip_arch.h ../uip/uip.h uipopt.h \
 ../apps/httpd/httpd.h
uip.o: ../uip/uip.c uipopt.h ../apps/httpd/httpd.h

clean:
	rm -f *.o *~ *core uip






