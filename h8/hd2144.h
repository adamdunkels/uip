/*
 * Copyright (c) 2001, Paul Clarke, Hydra Electronic Design Solutions Pty Ltd.
 * http://www.hydraelectronics.com.au
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
 * 3. The name of the author may not be used to endorse or promote
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
 * $Id: hd2144.h,v 1.2 2001/10/25 18:55:57 adam Exp $
 *
 */

typedef volatile unsigned char io_reg;
typedef volatile unsigned short io_dreg;

#define h8_kbcomp	(*(io_reg*) 0xfffee4)
#define h8_sbycr	(*(io_reg*) 0xffff84)
#define h8_lpwrcr	(*(io_reg*) 0xffff85)
#define h8_mstpcr	(*(io_dreg*) 0xffff86)
#define h8_sci_smr1	(*(io_reg*) 0xffff88)
#define h8_sci_brr1	(*(io_reg*) 0xffff89)
#define h8_sci_scr1	(*(io_reg*) 0xffff8a)
#define h8_sci_tdr1	(*(io_reg*) 0xffff8b)
#define h8_sci_ssr1	(*(io_reg*) 0xffff8c)
#define h8_sci_rdr1	(*(io_reg*) 0xffff8d)
#define h8_sci_scmr1	(*(io_reg*) 0xffff8e)
#define h8_frt_tier	(*(io_reg*) 0xffff90)
#define h8_frt_tcsr	(*(io_reg*) 0xffff91)
#define h8_frt_tcr	(*(io_reg*) 0xffff96)
#define h8_frt_tocr	(*(io_reg*) 0xffff97)
#define h8_frt_ocrar	(*(io_reg*) 0xffff98)
#define h8_frt_ocraf	(*(io_reg*) 0xffff9a)
#define h8_wdt_tcnt0_w	(*(io_dreg*) 0xffffa8)
#define h8_wdt_tcsr0_w	(*(io_dreg*) 0xffffa8)
#define h8_stcr     (*(io_reg*) 0xffffc3)

// mstpcr Module peripheral control register
#define _PWR_FRT	( 0x2000 )
#define _PWR_TMR01  ( 0x1000 )
#define _PWR_TMRXY	( 0x0100 )
#define _PWR_SCI1	( 0x0040 )

// frt_tier	 timer interrupt enable register
#define _ICIAE  0x80
#define _ICIBE  0x40
#define _ICICE  0x20
#define _ICIDE  0x10
#define _OCIAE  0x08
#define _OCIBE  0x04
#define _FOVIE  0x02

// frt_tcr free running timer control register
#define _IEDGA  0x80
#define _IEDGB  0x40
#define _IEDGC  0x20
#define _IEDGD  0x10
#define _BUFEA  0x08
#define _BUFEB  0x04
#define _CKS1 0x02
#define _CKS0 0x01

// frt_tocr     timer output control register
#define _ICRDMS 0x80
#define _OCRAMS 0x40
#define _ICRS   0x20
#define _OCRS   0x10
#define _OEA    0x08
#define _OEB    0x04
#define _OLVLA  0x02
#define _OLVLB  0x01

#define XTAL_HZ		(18432000L)
#define FRT_CLK_DIV	(32)

// scr serial control register
#define _TIE    0x80
#define _RIE    0x40
#define _TE     0x20
#define _RE     0x10
#define _MPIE   0x08
#define _TEIE   0x04
#define _CKE1   0x02
#define _CKE0   0x01

// smr serial mode register
#define _CA     0x80
#define _CHR    0x40
#define _PE     0x20
#define _OE     0x10
#define _STOP   0x08
#define _MP     0x04

// ssr serial status register
#define _TDRE   0x80
#define _RDRF   0x40
#define _ORER   0x20
#define _FER    0x10
#define _PER    0x08
#define _TEND   0x04
#define _MPB    0x02
#define _MPBT   0x01

// stcr Serial timer control register
#define _IICS   0x80
#define _IICX1  0x40
#define _IICX0  0x20
#define _IICE   0x10
#define _FLSHE  0x08
#define _ICKS1    0x02
#define _ICKS0    0x01

#define BAUD(baud)	(((XTAL_HZ/32)/baud)-1)

static inline void h8_AccessSci(void)
{
	h8_stcr &= ~_IICE;
}

static inline void h8_EnableInterrupts(void)
{
  __asm__ volatile ("andc.b #0x3F,ccr": : :"cc");		// clear I & UI bits 
}
