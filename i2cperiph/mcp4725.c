#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>
#include <stdio.h>

unsigned char sendadc[] = {
    3, 0xc0, 0, 0,
    0
};

int main(void)
{
    unsigned xo;

    for (;;) {
        TWIinit();
        printf("ADC\n");
        xo = 0;
        for(;;) {
            xo++;
            sendadc[3] = xo;
            sendadc[2] = 0x0f & (xo >> 8);
            TWIdocmd(sendadc);
            _delay_ms(10);
        }
    }
}
