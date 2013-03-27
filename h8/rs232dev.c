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
 * $Id: rs232dev.c,v 1.2 2001/10/25 18:55:57 adam Exp $
 *
 */


#include "uip.h"
#include "hd2144.h"

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

/*****************************************************************************/
/*			  	     Data				     */
/*****************************************************************************/

/* Baudrate settings */
#define RS_BAUD_50     	       		0x00
#define RS_BAUD_110    	       		0x01
#define RS_BAUD_134_5  	       	  	0x02
#define RS_BAUD_300    	       		0x03
#define RS_BAUD_600    	       		0x04
#define RS_BAUD_1200   	       		0x05
#define RS_BAUD_2400   	       		0x06
#define RS_BAUD_4800   	       		0x07
#define RS_BAUD_9600   	       		0x08
#define RS_BAUD_19200  	       		0x09
#define RS_BAUD_38400  	       		0x0A
#define RS_BAUD_57600  	       		0x0B
#define RS_BAUD_115200 	       		0x0C
#define RS_BAUD_230400 	       		0x0D

/* Stop bit settings */
#define RS_STOP_1      	       		0x00
#define RS_STOP_2      	       		0x80

/* Data bit settings */
#define RS_BITS_5      	       		0x60
#define RS_BITS_6      	       		0x40
#define RS_BITS_7      	       		0x20
#define RS_BITS_8      	       		0x00

/* Parity settings */
#define RS_PAR_NONE    	       		0x00
#define RS_PAR_ODD     	       		0x20
#define RS_PAR_EVEN    	       		0x60
#define RS_PAR_MARK    	       		0xA0
#define RS_PAR_SPACE   	       		0xE0

/* Bit masks to mask out things from the status returned by rs232_status */
#define RS_STATUS_PE   	       		0x01	/* Parity error */
#define RS_STATUS_FE			0x02	/* Framing error */
#define RS_STATUS_OVERRUN		0x04	/* Overrun error */
#define RS_STATUS_RDRF			0x08	/* Receiver data register full */
#define RS_STATUS_THRE			0x10	/* Transmit holding reg. empty */
#define RS_STATUS_DCD			0x20	/* NOT data carrier detect */
#define RS_STATUS_DSR			0x40	/* NOT data set ready */
#define RS_STATUS_IRQ  	       		0x80	/* IRQ condition */

/* Error codes returned by all functions */
#define RS_ERR_OK     	       		0x00	/* Not an error - relax */
#define RS_ERR_NOT_INITIALIZED 		0x01   	/* Module not initialized */
#define RS_ERR_BAUD_TOO_FAST		0x02   	/* Cannot handle baud rate */
#define RS_ERR_BAUD_NOT_AVAIL		0x03   	/* Baud rate not available */
#define RS_ERR_NO_DATA		  	0x04   	/* Nothing to read */
#define RS_ERR_OVERFLOW       	       	0x05   	/* No room in send buffer */

static volatile u8_t timer, tmptimer, timershot;
static u16_t len;

#define SIO_RECV(c)   while(rs232_get(&c) == RS_ERR_NO_DATA)

#define SIO_RECVT(c) do {					\
                       tmptimer = timer;			\
                       timershot = 0; 				\
			           while(rs232_get(&c) == RS_ERR_NO_DATA) {	\
						 if(timer != tmptimer) {		\
						   timershot = 1;			\
						   break;				\
						 }					\
                       }					\
		    		 } while(0)

#define SIO_SEND(c)  while(rs232_put(c) == RS_ERR_OVERFLOW)

#define MAX_SIZE UIP_BUFSIZE

static const unsigned char slip_end = SLIP_END, 
                           slip_esc = SLIP_ESC, 
                           slip_esc_end = SLIP_ESC_END, 
                           slip_esc_esc = SLIP_ESC_ESC;


static u8_t dropcount;

#define printf(x)

typedef struct {
	volatile unsigned char 	*put_ptr;
	volatile unsigned char 	*get_ptr;
	unsigned  char  *data;
} serBuffer;  

serBuffer TxBuffer, RxBuffer;
unsigned char TxB[MAX_SIZE];
unsigned char RxB[MAX_SIZE];

void serBufferInit(serBuffer *buffer, unsigned char *data)
{
	buffer->data=data;
	buffer->put_ptr = data;
	buffer->get_ptr = data;
}

unsigned char BufferAdd( serBuffer *buffer, unsigned char data)
{
	if ( (buffer->get_ptr == buffer->data)
		&& ((buffer->put_ptr == buffer->data+MAX_SIZE-1)
		|| ( buffer->put_ptr == buffer->get_ptr -1)))
	{
		return 0;
	}		
	*buffer->put_ptr = data;
	buffer->put_ptr++;
	if ( buffer->put_ptr >= buffer->data + MAX_SIZE)
	{
		buffer->put_ptr = buffer->data;
	}
	return 1;
}

unsigned BufferRemove( serBuffer *buffer, unsigned char *data)
{
	if ( buffer->put_ptr == buffer->get_ptr)
	{
		return 0;
	}
	*data = *buffer->get_ptr;
	buffer->get_ptr++;
	if ( buffer->get_ptr >= buffer->data + MAX_SIZE)
	{
		buffer->get_ptr = buffer->data;
	}
	return 1;
}

/*-----------------------------------------------------------------------------------*/
unsigned char rs232_init( char hacked)
{
/* Initialize the serial port, install the interrupt handler. The parameter
 * must be true (non zero) for a hacked swiftlink and false (zero) otherwise.
 */
 serBufferInit( &TxBuffer, TxB);
 serBufferInit( &RxBuffer, RxB);

 return RS_ERR_OK;
}

unsigned char rs232_done (void)
{
/* Close the port, deinstall the interrupt hander. You MUST call this function
 * before terminating the program, otherwise the machine may crash later. If
 * in doubt, install an exit handler using atexit(). The function will do
 * nothing, if it was already called.
 */
 serBufferInit( &TxBuffer, TxB);
 serBufferInit( &RxBuffer, RxB);

 return RS_ERR_OK;
}

unsigned char rs232_get (char* b)
{
/* Get a character from the serial port. If no characters are available, the
 * function will return RS_ERR_NO_DATA, so this is not a fatal error.
 */
	if ( BufferRemove( &RxBuffer, b) == 0 )
	{
		h8_sci_scr1 |= _RIE ; 	// enable receive interrupt.		
		return RS_ERR_NO_DATA;
	}
	return RS_ERR_OK;
}

unsigned char rs232_put (char b)
{
/* Send a character via the serial port. There is a transmit buffer, but
 * transmitting is not done via interrupt. The function returns
 * RS_ERR_OVERFLOW if there is no space left in the transmit buffer.
 */
	if ( BufferAdd( &TxBuffer, (unsigned char) b) == 0)
	{
		return RS_ERR_OVERFLOW;
	}
	h8_sci_scr1 |= _TIE ; 	// enable TIE interrupt.	
	return RS_ERR_OK;
}

unsigned char rs232_pause (void)
{
	/* Assert flow control and disable interrupts. */
	return RS_ERR_OK;
}

unsigned char rs232_unpause (void)
{
	/* Re-enable interrupts and release flow control */
	return RS_ERR_OK;
}

unsigned char rs232_status (unsigned char* status, unsigned char* errors)
{
	/* Return the serial port status. */
	*status = h8_sci_ssr1;
	*errors = h8_sci_scr1;

	return RS_ERR_OK;
}

unsigned char rs232_params (unsigned char params, unsigned char parity)
{
	unsigned char scr_saved;

	h8_AccessSci();
	scr_saved = h8_sci_scr1;
	h8_sci_scr1 = 0;

	h8_sci_scmr1 = 0xf2;
	/* Set the port parameters. Use a combination of the #defined values above. */
	if (params & RS_STOP_2)
	{
		h8_sci_smr1 |= _STOP;
	} else
	{
		h8_sci_smr1 &= ~_STOP;
	}
	switch (params & 0x60)
	{
		case RS_BITS_5:
			break;

		case RS_BITS_6:
			break;
			
		case RS_BITS_7:
			h8_sci_smr1 |= _CHR;
			break;

		case RS_BITS_8:

		default: 
			h8_sci_smr1 &= ~_CHR;
			break;
	}
	switch (params & 0x0f)
	{
		case RS_BAUD_50:
			h8_sci_brr1 = BAUD(50);
			break;
		case RS_BAUD_110:
			h8_sci_brr1 = BAUD(110);
			break;
		case RS_BAUD_134_5:
			h8_sci_brr1 = BAUD(135);
			break;
		case RS_BAUD_300:
			h8_sci_brr1 = BAUD(300);
			break;
		case RS_BAUD_600:
			h8_sci_brr1 = BAUD(600);
			break;
		case RS_BAUD_1200:
			h8_sci_brr1 = BAUD(1200);
			break;
		case RS_BAUD_2400:
			h8_sci_brr1 = BAUD(2400);
			break;
		case RS_BAUD_4800:
			h8_sci_brr1 = BAUD(4800);
			break;
		case RS_BAUD_9600:
			h8_sci_brr1 = BAUD(9600);
			break;
		case RS_BAUD_19200:
			h8_sci_brr1 = BAUD(9600);
			break;
		case RS_BAUD_38400:
			h8_sci_brr1 = BAUD(38400);
			break;
		case RS_BAUD_57600:
			h8_sci_brr1 = BAUD(57600L);
			break;
		case RS_BAUD_115200:
			h8_sci_brr1 = BAUD(115200L);
			break;
		case RS_BAUD_230400:
			h8_sci_brr1 = BAUD(230400L);
			break;
	}
	switch (parity & 0xE0)
	{
		case RS_PAR_NONE:
			break;
		case RS_PAR_ODD:
			h8_sci_smr1 |= _PE| _OE;
			break;
		case RS_PAR_EVEN:
			h8_sci_smr1 &= ~( _PE| _OE);		
			break;
		case RS_PAR_MARK:
			h8_sci_smr1 &= ~( _PE| _OE);		
			break;
		case RS_PAR_SPACE:
			break;
		default:
			break;
	}	
	tmptimer = timer ;
	while (tmptimer == timer)
	{
		/* wait 1 bit period */
	}

	h8_sci_scr1 = scr_saved | _TE | _RE;

	return RS_ERR_OK;
}

static void
rs232_err(char err)
{
  switch(err) {
  case RS_ERR_OK:
    printf("RS232 OK\r");
    break;
  case RS_ERR_NOT_INITIALIZED:
    printf("RS232 not initialized\r");
    break;
  case RS_ERR_BAUD_TOO_FAST:
    printf("RS232 baud too fast\r");
    break;
  case RS_ERR_BAUD_NOT_AVAIL:
    printf("RS232 baud rate not available\r");
    break;
  case RS_ERR_NO_DATA:
    printf("RS232 nothing to read\r");
    break;
  case RS_ERR_OVERFLOW:
    printf("RS232 overflow\r");
    break;
  }

}
/*-----------------------------------------------------------------------------------*/
void
rs232dev_send(void)
{
#if MAX_SIZE > 255
  u16_t i;
#else
  u8_t i;
#endif /* MAX_SIZE > 255 */
  static unsigned long address;
  u8_t *ptr;
  u8_t c;

  /*  if(dropcount++ == 4) {
    dropcount = 0;
    printf("drop\r");
    return;
    }*/
  
  SIO_SEND(slip_end);

  ptr = uip_buf;
  for(i = 0; i < uip_len; i++) {
    if(i == 40) {
      ptr = uip_appdata;
	  address = (unsigned long) uip_appdata;
    }
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
  printf("tx\r");
}

/*-----------------------------------------------------------------------------------*/
u8_t
rs232dev_read(void)
{
  u8_t c;
  u16_t tmplen;
  
 start:
  while(1) {
    if(len >= MAX_SIZE) {
      len = 0;
      goto start;
    }
    SIO_RECVT(c);
    if(timershot == 1) {
      return 0;
    }
    switch(c) {
    case SLIP_END:
      if(len > 0) {
	tmplen = len;
	len = 0;
        return tmplen;
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
      if(len < MAX_SIZE) {
	uip_buf[len] = c;
        len++;
      }
      break;
    }
    
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void sci1_rxi_irq () __attribute__ ((interrupt_handler));
void sci1_rxi_irq (void)
{
	(void) BufferAdd( &RxBuffer, h8_sci_rdr1) ;		// add char to rx buffer
	
	h8_sci_ssr1 &= ~(_RDRF| _ORER| _FER| _PER);	// write 0's
}

void sci1_eri_irq () __attribute__ ((interrupt_handler));
void sci1_eri_irq (void)
{
	unsigned char ssr;

	ssr = h8_sci_ssr1;
	h8_sci_ssr1 &= ~ (_ORER | _PER | _FER);
}

void sci1_tdre_irq () __attribute__ ((interrupt_handler));
void sci1_tdre_irq (void)
{
	if ( BufferRemove( &TxBuffer, &h8_sci_tdr1) == 0)
	{
		h8_sci_scr1 &= ~_TIE; 	// disable transmit interrrupt 
	} else
	{
		h8_sci_ssr1 &= ~_TDRE;
	}
}

void sci1_tc_irq () __attribute__ ((interrupt_handler));
void sci1_tc_irq (void)
{
	h8_sci_scr1 &= ~_TEIE ; 	// disable TEIE interrupt.	
}

void frt_fovi_irq (void) __attribute__ ((interrupt_handler));
void frt_fovi_irq (void)
{
	static u8_t fix_period = 0;		

	if ( ++fix_period & 0x01)	/* Adjust timer update rate to approx 5 Hz. */
	{
		timer++;
	}
	h8_frt_tcsr &= ~_FOVIE;	// clear ovf interrupt
}

/*-----------------------------------------------------------------------------------*/
void
rs232dev_init(void)
{
  char err;
  
  err = rs232_init(0);
  rs232_err(err);

  timer = 0;
  h8_mstpcr = 0;			// enable all h8 peripherals !
  h8_frt_tcr = _CKS1 | !_CKS0;		//  %32
  h8_frt_tier = _FOVIE;				// enable overflow interrupt!

  h8_EnableInterrupts();			//  enable interrupts

  err = rs232_params(RS_BAUD_9600, RS_PAR_NONE);
  rs232_err(err);

  len = 0;
  
}
/*-----------------------------------------------------------------------------------*/
