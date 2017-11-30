#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>

#include "uart.h"

unsigned char sendhmci[] = {
    5, 0x3c, 0, 0x18, 0, 0,
    0
};
unsigned char sendhmcpr[] = {
    2, 0x3c, 9,
    0
};
unsigned char sendhmcrd1[] = {
    2, 0x3d, 0,
    0
};
unsigned char sendhmcrd2[] = {
    7, 0x3d, 0, 0, 0, 0, 0, 0,
    0
};

int main(void)
{
    unsigned char cnt, sta;
    int xo, yo, zo;

#define BAUD 57600
    inituart(BAUD/100);

    for (;;) {
        TWIinit();
        TWIdocmd(sendhmci);
        _delay_ms(50);
        TWIdocmd(sendhmci);
        _delay_ms(50);
        cnt = 0;
        while (cnt++ < 200) {
            TWIdocmd(sendhmcpr);
            TWIdocmd(sendhmcrd1);
            sta = sendhmcrd1[2];
            if (sta != 5)
                continue;
            cnt = 0;
            TWIdocmd(sendhmcrd2);
            xo = sendhmcrd2[3] | (sendhmcrd2[2] << 8);
            yo = sendhmcrd2[5] | (sendhmcrd2[4] << 8);
            zo = sendhmcrd2[7] | (sendhmcrd2[6] << 8);
            printf("%5d,%5d,%5d\n", xo, yo, zo);
            _delay_ms(12);
        }
    }
}
