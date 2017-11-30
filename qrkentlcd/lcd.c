#include <avr/io.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
typedef unsigned char u8;
typedef unsigned u16;


static void sendspiblock(u8 *buf, u16 len)
{
    volatile u8 x;
    while (len--) {
        SPDR = *buf++;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        x = SPDR;
    }
}

static void csact() {
    PORTB &= ~4;
}

static void csinact() {
    PORTB |= 4;
}

#if 0

static void recvspiblock(u8 *buf, u16 len)
{
    while (len--) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        *buf++ = SPDR;
    }
}

static void sendffspi(u16 len)
{
    while (len--) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
    }
}

static void waitforspi(u8 val)
{
    for (;;) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        if (val == SPDR)
            return;
    }
}
#endif

static void spihwinit()
{
    DDRB = 0x2c;                // set port for SPI
    PORTB |= 4;
    SPCR = 0x56;                // /32 (0x52 /64)
    SPSR = 0;
}

#include "qrencode.h"

#include <string.h>
int main()
{
    unsigned char b = 0x55;
    unsigned i, k, t, j;

    spihwinit();
#define spi_transfer(x) {unsigned char ttt=(x);sendspiblock(&ttt,1);}
#if 0

  // Send data to onboard RAM
    for(i = 0x00; i < 240; i++, b ^= 0xff){ // 37.5 blocks of 256 bytes (last 128 bytes sent after this loop)
        csact();
        spi_transfer(0x00); // TX command
        t = i * 40;
        spi_transfer((t>>8)); // Address high byte
        spi_transfer(t); // Address low byte
        for(j = 0; j < 40; j++){ // Data
            if( !j || j == 39 || i < 8 || i > 232 )
                spi_transfer(255)
            else
                spi_transfer(b) 
        }
        csinact();
        _delay_ms(5); // Or you could poll the BUSY pin, whatever floats your boat
    }

  csact(); // Screen update
  spi_transfer(0x18);
  spi_transfer(0x00);
  spi_transfer(0x00);
  csinact();

  for(;;)
      wdt_reset();
#else

    strcpy((char *)strinbuf, "http://harleyhacking.blogspot.com");
    qrencode();
    _delay_ms(1200); // Or you could poll the BUSY pin, whatever floats your boat
    csact();
    spi_transfer(0x24)
    csinact();
    _delay_ms(1200); // Or you could poll the BUSY pin, whatever floats your boat


    t = 0;
    b = 0;
    for ( k = 0; k < 240 ; k++) {
        csact();
        i = k * 40;
        spi_transfer(0x00); // TX command
        spi_transfer((i>>8)); // Address high byte
        spi_transfer(i); // Address low byte
        for ( i = 0; i < 320 ; i++) {
            b <<= 1;
            int x = (i - 40)/4;
            int y = (k - 24)/4;
            if( x >= 0 && y >= 0 && x < WD && y < WD )
                if( QRBIT(x,y) )
                    b |= 1;
            if( ++t > 7 ) {
                t = 0;
                spi_transfer((~b));
                b = 0;
            }
        }
        csinact();
        _delay_ms(5); // Or you could poll the BUSY pin, whatever floats your boat
    }

        csinact();
        _delay_ms(5); // Or you could poll the BUSY pin, whatever floats your boat


  csact(); // Screen update
  spi_transfer(0x18);
  spi_transfer(0x00);
  spi_transfer(0x00);
  csinact();

#endif
    for(;;);
}
