#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 16000000
#endif
#include <util/delay.h>
#include <stdio.h>
// len, len bytes of transaction, first even = wrinte, odd=read per i2c
// Start, len bytes, rep start, len bytes, 0 does stop
// 20 char

#define LCDADDR 0x78 
unsigned char sendlcd0[] = {
    13,
    LCDADDR, 0x00, 0x38, 0x38, 0x38, 0x39, // 0x35 for 2 line
    0x14, 0x78, 0x5e, 0x6d, 0x0c, 0x06, 0x01,
    0
};

unsigned char sendlcda0[] = {
    24, LCDADDR, 0x80, 0x80, 0x40, 'O', 'n', 'e', '=', '=','=','=','=', '+','+','+','+', '=','=','=','=', '=','O','n','e',
    0,0,0,0,0,0,0,0
};

unsigned char sendlcdb0[] = {
    24, LCDADDR, 0x80, 0xc0, 0x40, 'T', 'w', 'o', '=', '=','=','=','=', '+','+','+','+', '=','=','=','=', '=','T','w','o',
    0, 0,0,0,0,0,0,0
};

#undef LCDADDR
#define LCDADDR 0x7c
unsigned char sendlcd1[] = {
    13, LCDADDR, 0x00, 0x30, 0x30, 0x30, 0x39,
    0x14, 0x7f, 0x5f, 0x6a, 0x0f, 0x06, 0x01,
    0
};

unsigned char sendlcda1[] = {
    24, LCDADDR, 0x80, 0x80, 0x40, 'O', 'n', 'e', '=', '=','=','=','=', '+','+','+','+', '=','=','=','=', '=','O','n','e',
    0,0,0,0,0,0,0,0
};

unsigned char sendlcdb1[] = {
    24, LCDADDR, 0x80, 0xc0, 0x40, 'T', 'w', 'o', '=', '=','=','=','=', '+','+','+','+', '=','=','=','=', '=','T','w','o',
    0, 0,0,0,0,0,0,0
};


int main(void)
{
    unsigned xo = 0;

    for (;;) {
        TWIinit();

        TWIdocmd(sendlcd0);
        TWIdocmd(sendlcd1);
        _delay_ms(20);
        TWIdocmd(sendlcda0);
        TWIdocmd(sendlcdb0);
        TWIdocmd(sendlcda1);
        TWIdocmd(sendlcdb1);
        while( 1 ) {
            sprintf( (char *) &sendlcda0[5], "%6d",xo );
            sprintf( (char *) &sendlcda1[5], "%6d",xo++ );
            TWIdocmd(sendlcda0);
            TWIdocmd(sendlcda1);
        }
    }
}
