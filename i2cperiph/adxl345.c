#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>

#include "uart.h"

//#define WADXL (0xa6)
#define WADXL (0x3a)
#define RADXL (WADXL+1)
unsigned char msgsetup[] = {
    3, WADXL, 0x2d, 8,
    3, WADXL, 0x31, 0xb,
    3, WADXL, 0x2c, 0xa,        //f,
    3, WADXL, 0x38, 0x9f,
    0
};

unsigned char queryfifo[] = {
    2, WADXL, 0x39,
    2, RADXL, 0,                //5
    0
};

unsigned char readxyz[] = {
    2, WADXL, 0x32,
    7, RADXL, 0, 0, 0, 0, 0, 0,
    0
};

int main(void)
{
    unsigned char cnt;
    int xo, yo, zo;

#define BAUD 57600
    inituart(BAUD / 100);

    for (;;) {
        TWIinit();
        printf("Accelerometer\n");
        TWIdocmd(msgsetup);
        _delay_ms(200);
        cnt = 0;
        while (cnt++ < 200) {
            TWIdocmd(queryfifo);
            unsigned char qlen = queryfifo[5];
            while (qlen--) {
                cnt = 0;
                TWIdocmd(readxyz);

                xo = readxyz[6]; 
                xo <<= 8;
                xo |= 0xff & readxyz[5];

                yo = readxyz[8]; 
                yo <<= 8;
                yo |= 0xff & readxyz[7];

                zo = readxyz[10]; 
                zo <<= 8;
                zo |= 0xff & readxyz[9];

                printf("%d %5d,%5d,%5d\n", qlen + 1, xo, yo, zo);
            }
        }
    }
}
