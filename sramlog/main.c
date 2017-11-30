#include "typefix.h"

#define BAUD 57600
#ifndef F_CPU
#define F_CPU 16000000 // default to OpenLog
#endif
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "uart.h"


static void hwinit()
{
    DDRD |= 0x20;
    DDRB |= 0x20;
    inituart(BAUD/100);
}

#include "fat32.h"
#include "sdhc.h"

#include <string.h>

void configure() {
    int i;
    unsigned long baud = 0;
    seekfile(0,0);
    for (;;) {
        i = readbyte();
        if( i < '0' || i > '9' )
            break;
        baud *= 10;
        baud += i - '0';
    }
    baud /= 100;
    if( baud > 10 && baud < 30000 )
        inituart(baud);
}

static unsigned char hyplogdir[] = "HYPERLOG   ";
static unsigned char cfigfile[] = "HYPERLOGCFG";

#define CSBIT 2

static void setspi(void)
{
    PORTB |= 0x2e; // pullup
    DDRB = 0x2e;                // set port for SPI
    SPCR = 0x50;                // Slowest possible for init
    SPSR = 1;
    PORTB &= ~CSBIT;
    SPDR = 1;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    SPDR = 0x41;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    PORTB |= CSBIT;
}

static unsigned wramaddr = 0;
static void writedat( unsigned char *buf, unsigned len )
{
    if( !len )
        return;
    PORTB &= ~CSBIT;
    SPDR = 2;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    SPDR = wramaddr >> 8;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    SPDR = wramaddr;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    while( len-- ) {
        SPDR = *buf++;
        wramaddr++;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
    }
    PORTB |= CSBIT;
}

static unsigned rramaddr = 0;
static void readdat( unsigned char *buf, unsigned len ) 
{
    if( !len )
        return;
    PORTB &= ~CSBIT;
    SPDR = 3;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    SPDR = rramaddr >> 8;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    SPDR = rramaddr;
    while (!(SPSR & 0x80));
    SPSR &= 0x7f;
    while (len--) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        rramaddr++;
        SPSR &= 0x7f;
        *buf++ = SPDR;
    }
    PORTB |= CSBIT;
}

int main(void)
{
    int i;

    hwinit();
    printf( "1" );
    i = sdhcinit();
    if( i )
        return 2;

    setspi();

    printf( "2" );
    i = mount(0);
    if (i)
        return 3;
    seekfile(0, 0);
    printf( "3" );
    i = getdirent(cfigfile);
    if (!i) {
        configure();
        resettodir();
        seekfile(0,0);
    }
    printf( "4" );
    i = newdirent(hyplogdir, 0x10);
    if( i && i != 2 )
        return 4;
    if( i == 2 ) {
        i = getdirent(hyplogdir);
        if( i != 2 )
            return 5;
    }
    printf( "5" );
    i = getdirent(cfigfile);
    if (!i) {
        configure();
        resettodir();
        seekfile(0, 0);     // for list
    }
    printf( "6" );
    i = newdirent(NULL, 0);
    if( i )
        return 6;

    PORTD |= 0x20;
    printf( "7" );
    unsigned long cnt = 100000; // timeout to push buffer
    PRR = 0xe9;
    for(;;) {
extern volatile unsigned rxhead, rxtail;
extern unsigned char rxbuf[1 << RXBUFBITS];
        if( rxhead == rxtail ) {
            if( cardnotbusy() )
                while( wramaddr - rramaddr > 512 ) {
                    readdat( filesectbuf, 512 );
                    writenextsect();
                }
            if( !cnt-- ) {
     //            UCSR0A &= ~0x10;
                flushbuf();
                syncdirent(0);
                seekfile(0,2);
                PORTD &= ~0x20;
                u8 sc = SPCR;
                u8 ss = SPSR;
                u8 t = 0x40 | (MCUCR & 0x9f);
                u8 s = t | 0x20;
                PRR = 0xe9+4;
                MCUCR = s;
                MCUCR = t;
                sleep_mode();
                PRR = 0xe9;
                SPCR = sc;
                SPSR = ss;
            }
        }
        else {
            cnt = 100000;
            unsigned buffil;
            if( rxhead < rxtail ) {
                buffil = (1 << RXBUFBITS) - rxtail; // to seam
                writedat( rxbuf + rxtail, buffil );
                rxtail += buffil;
                rxtail &= (1 << RXBUFBITS) - 1;
            }
            buffil = rxhead - rxtail; 
            if( buffil ) {
                writedat( rxbuf + rxtail, buffil );
                rxtail += buffil;
                rxtail &= (1 << RXBUFBITS) - 1;
            }
        }
        if( wramaddr - rramaddr > 512 && cardnotbusy() ) { // for speed, just do one
            readdat( filesectbuf, 512 );
            writenextsect();
        }
        PORTD |= 0x20;
    }
    flushbuf();
    syncdirent(1);

}
