#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif

#include <util/delay.h>
#include "uart.h"

unsigned char readtherm[] = {
    3, 0x91, 0, 0,
    0,0,0,0
};

unsigned char settherm[] = {
    2, 0x90, 0,
    0
};

int main(void)
{
    unsigned xo;

#define BAUD 57600
    inituart(BAUD / 100);

    TWIinit();
    printf("TMP102\n");
    TWIdocmd(settherm);
    for(;;) {
        TWIdocmd(readtherm);
        xo = (readtherm[2] << 8) + readtherm[3];
        xo >>= 4; // 12 bits
        printf("%4u\n", xo);
        _delay_ms(120);
    }
}
