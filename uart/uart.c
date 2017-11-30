#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "uart.h"

#ifndef TXBUFBITS
#warning defaulting to 256 byte TX buf
#define TXBUFBITS 8
#endif
#ifndef RXBUFBITS
#warning defaulting to 1024 byte RX buf
#define RXBUFBITS 10
#endif
/*------------------------------------------------------------------------*/
//USART TX/RX
// to save ram, cut tx buflen
static unsigned char txbuf[1 << TXBUFBITS];
static volatile unsigned txhead, txtail;
ISR(USART_UDRE_vect)
{
    if (txhead == txtail)
        UCSR0B &= ~0x20;
    else {
        UDR0 = txbuf[txtail++];
        txtail &= (1 << TXBUFBITS) - 1;
    }
}

void txenqueue(unsigned char *buf, unsigned len)
{
    while (len--) {            // doesn't check for overruns but ought to
        while( (0xff & (txhead - txtail)) > 2 )
            ; // sleep?
        txbuf[txhead++] = *buf++;
        txhead &= (1 << TXBUFBITS) - 1;
        UCSR0B |= 0x20;
    }
}

void uartputch(unsigned char c)
{
    txenqueue(&c,1);
}

//RX - line by line passthru
//static 
unsigned char rxbuf[1 << RXBUFBITS];
//static 
volatile unsigned rxhead, rxtail;
// insertat, line ready, eol, bol
unsigned char xoffflg = 0;
ISR(USART_RX_vect)
{
    // errors, overrun?
    rxbuf[rxhead++] = UDR0;
    rxhead &= (1 << RXBUFBITS) - 1;
#ifdef HYPERLOG
    if( 4 < ((rxtail - rxhead) &  ((1 << RXBUFBITS) - 1) )) {
        PORTD |= 0x20;
        uartputch( 0x13 );
        xoffflg = 1;
    }
#endif
}

int uartgetch()
{
    int t;
    if( rxhead == rxtail )
        return -1;
    t = rxbuf[rxtail++];
    rxtail &= (1 << RXBUFBITS) - 1;
#ifdef HYPERLOG
    if( xoffflg && 16 < ((rxhead - rxtail ) & ((1 << RXBUFBITS) - 1 ))) { 
        PORTD &= ~0x20;
        uartputch( 0x10 );
        xoffflg = 0;
    }
#endif
    return 0xff & t;
}

unsigned kbhit()
{
    return rxtail - rxhead;
}

static int serout(char c, FILE * stream)
{
    if (c != '\n')
        uartputch(c);
    else
        txenqueue((unsigned char *)"\r\n", 2);
    return 0;
}
static FILE fpuart = FDEV_SETUP_STREAM(serout, NULL, _FDEV_SETUP_WRITE);

void inituart(unsigned centibaud) {
    unsigned acc,count;
    // we don't do this often, and it should be more accurate than divides
#define BCLK (F_CPU/800)
    acc = 0;
    count = 0;
    while( acc < BCLK )
        acc += centibaud, count++;
    if( acc - BCLK > centibaud >> 1 )
        count--;
    UBRR0 = count - 1;

    UCSR0C = 0x06;              //8N1 (should be this from reset)
    UCSR0A = 0xE0 | 2;          // clear Interrupts, UART at 2x (xtal/8)
    UCSR0B = 0x98;              // oring in 0x80 would enable rx interrupt

    txhead = txtail = 0;
    rxhead = rxtail = 0;
    sei();
    stdout = &fpuart;           //Required for printf init
}
