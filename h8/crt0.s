;//
;// Copyright (c) 2001, Paul Clarke, Hydra Electronic Design Solutions Pty. Ltd.
;; // http://www.hydraelectronics.com.au
;// All rights reserved. 
;//
;// Redistribution and use in source and binary forms, with or without 
;// modification, are permitted provided that the following conditions 
;// are met: 
;// 1. Redistributions of source code must retain the above copyright 
;//    notice, this list of conditions and the following disclaimer. 
;// 2. Redistributions in binary form must reproduce the above copyright 
;//    notice, this list of conditions and the following disclaimer in the 
;//    documentation and/or other materials provided with the distribution. 
;// 3. The name of the author may not be used to endorse or promote
;//    products derived from this software without specific prior
;//    written permission.  
;//
;// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
;// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
;// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
;// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
;// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
;// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
;//
;// This file is part of the uIP TCP/IP stack.
;//
;// $Id: crt0.s,v 1.2 2001/10/25 18:55:57 adam Exp $
;//
;//
;// C startup routine for embedded H8 code.
;//
;//

        .h8300s

        .section .text
        .global _start

_h8_mdcr	= H'FFFFC5
_h8_p1ddr	= H'FFFFB0
_h8_p2ddr	= H'FFFFB1
_h8_paddr	= H'FFFFAB
_h8_wscr	= H'FFFFC7
_start:
        mov.l   #_stack,sp

;
; Set wait states and external addr width
;
        bset    #7,@_h8_mdcr:8  // Enable external ram

        mov.b   #255,r1l         //
        mov.b   r1l,@_h8_p1ddr:8 //
        mov.b   r1l,@_h8_p2ddr:8 //
        mov.b   r1l,@_h8_paddr:8 //

        mov.b   #0x19, r1l      ; [0x13,0x12,0x11,0x10,0x03] slow..fast width 16
                                ; [0x33,0x32,0x31,0x30,0x23] slow..fast width 8
				; 0x19 = 3 state access Pin Wait mode 1 waitstate
				; additional waitstates added if wait remains low.
        mov.b   r1l, @_h8_wscr:8
        ;
        ; Clear bss section
        ;
        mov.l   #_bss,er0
        mov.l   #_ebss,er1
        mov.w   #0,r2

.bssloop:
        mov.w   r2,@er0
        adds    #2,er0
        cmp.l   er1,er0
        blo     .bssloop

        ; Copy ROMed data section to ram.
        mov.l   #_ldata,er0
        mov.l   #_data,er1
.dataloop:
        mov.w   @er0,r2
        mov.w   r2,@er1
        adds    #2,er0
        adds    #2,er1
        cmp.l   #_edata,er1
        blo     .dataloop

        jsr     @_main

_exit:
        jmp     @_exit

 
