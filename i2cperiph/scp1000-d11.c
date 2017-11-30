#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>
#include "uart.h"

unsigned char startacq[] = {
    3, 0x22, 0x03, 0x0a,
    0
};
unsigned char ready[] = {
    2, 0x22, 0x07,
    2, 0x23, 0, //5
    0
};
unsigned char getdatat[] = {
    2, 0x22, 0x81,
    3, 0x23, 0, 0, // 5-6
    0
};
unsigned char getdatap1[] = {
    2, 0x22, 0x7f,
    2, 0x23, 0, // 12
    0
};
unsigned char getdatap2[] = {
    2, 0x22, 0x80,
    3, 0x23, 0, 0, // 18-19
    0
};

int main(void)
{
    unsigned char cnt;

#define BAUD 57600
    inituart(BAUD/100);

    for (;;) {
        TWIinit();
        printf("Baro\n");
        TWIdocmd(startacq);
        TWIdocmd(ready);
        printf("Starting %02x\n", ready[5]);
        cnt = 0;
        while (cnt++ < 20) {
            _delay_ms(100);
            TWIdocmd(ready);
            if( 0xe != (0xe & ready[5]) ) // startup done
                continue;
            if( !(0x20 & ready[5]) ) // conversion ready
                continue;
            TWIdocmd(getdatat);
            TWIdocmd(getdatap1);
            TWIdocmd(getdatap2);
            cnt = 0;
            int temp;
            temp = getdatat[5];
            temp <<= 8;
            temp |= getdatat[6];
            temp <<= 2;
            temp >>= 2;
            temp *= 5;
            printf( "Temp %d.%02d ", temp / 100, temp % 100 );
            unsigned long press = getdatap1[5];
            press <<= 8;
            press |= getdatap2[5];
            press <<= 8;
            press |= getdatap2[6];
            press += 2;
            press >>= 2;
            printf( "Press %ld\n", press );
        }
    }
}
