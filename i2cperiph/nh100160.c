#include <avr/io.h>

#include "twi2c.h"

#ifndef F_CPU
#define F_CPU 8000000
#endif
#include <util/delay.h>
#include <stdio.h>
// len, len bytes of transaction, first even = wrinte, odd=read per i2c
// Start, len bytes, rep start, len bytes, 0 does stop
// 20 char

#include <string.h>

unsigned char lcdi0[] = {
    15, 0x7E, 0x00,
    0xe2, //reset
    0x48, 0x64, // nset partial display duty
    0xA0, // ADC normal
    0xC8, // SHL reverse?
    0x44, 0x00, // COM0 = 0
    0xAB, // OSC start
    0x26, // regulator register
    0x81, 0x1c, // ref voltage set
    0x56, // LCD Bias
    0x64,// Set DC-DC
    0	// END
};

unsigned char lcdi1[] = {
    4, 0x7E, 0x00,
    0x2C, // control power circuit - VC set
    0x66, // Set DC-DC
    0
};

unsigned char lcdi2[] = {
    3, 0x7E, 0x00,
    0x2E, // VR ON
    0
};

unsigned char lcdi3[] = {
    2 + 6,
    0x7E, 0x00,
    0x2F,  // VF ON
    0xF3, 0x00, // Bias Power save
    0x96, // FRC - 60 PWM
    0x38, 0x75, // Set mode FR, BE Frame freq, boost efficient - ext on
    //    0x97, // FRC?? ??
    0
};

unsigned char lcdi4[] = {
    2 + 64,
    0x7E, 0x00,
    // white to gray to black
    0x80, 0x00, 0x81, 0x00, 0x82, 0x00, 0x83, 0x00, 0x84, 0x06, 0x85, 0x06, 0x86, 0x06, 0x87, 0x06,
    0x88, 0x0B, 0x89, 0x0B, 0x8A, 0x0B, 0x8B, 0x0B, 0x8C, 0x10, 0x8D, 0x10, 0x8E, 0x10, 0x8F, 0x10,
    0x90, 0x15, 0x91, 0x15, 0x92, 0x15, 0x93, 0x15, 0x94, 0x1A, 0x95, 0x1A, 0x96, 0x1A, 0x97, 0x1A,
    0x98, 0x1E, 0x99, 0x1E, 0x9A, 0x1E, 0x9B, 0x1E, 0x9C, 0x23, 0x9D, 0x23, 0x9E, 0x23, 0x9F, 0x23,
    0
};

unsigned char lcdi5[] = {
    2 + 64,
    0x7E, 0x00,
    0xA0, 0x27, 0xA1, 0x27, 0xA2, 0x27, 0xA3, 0x27, 0xA4, 0x2B, 0xA5, 0x2B, 0xA6, 0x2B, 0xA7, 0x2B,
    0xA8, 0x2F, 0xA9, 0x2F, 0xAA, 0x2F, 0xAB, 0x2F, 0xAC, 0x32, 0xAD, 0x32, 0xAE, 0x32, 0xAF, 0x32,
    0xB0, 0x35, 0xB1, 0x35, 0xB2, 0x35, 0xB3, 0x35, 0xB4, 0x38, 0xB5, 0x38, 0xB6, 0x38, 0xB7, 0x38,
    0xB8, 0x3A, 0xB9, 0x3A, 0xBA, 0x3A, 0xBB, 0x3A, 0xBC, 0x3C, 0xBD, 0x3C, 0xBE, 0x3C, 0xBF, 0x3C,
    0
};

unsigned char lcdi6[] = {
    2 + 3,
    0x7E, 0x00,
    0x38, 0x74, // Mode Set, Fram Freq, boost efficient - ext off
    0xAF, // display on
    0
};

unsigned char sendlcda0[] = {
    7, 0x7e, 0x00, 0xb0, // page
    0x10, 0, // colH
    0x00, 0, // colL
    0, 0
};

unsigned char sendlcdb0[] = {
    6, 0x7e, 0x40, 0, 0, 0, 0,
    0, 0
};

unsigned char image[] = {

1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,
1,0,0,0,0,0,1,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,0,0,1,
1,0,1,1,1,0,1,0,0,0,1,0,0,1,1,0,1,0,1,0,1,1,1,0,1,
1,0,1,1,1,0,1,0,1,1,0,1,0,1,0,1,1,0,1,0,1,1,1,0,1,
1,0,1,1,1,0,1,0,1,1,0,0,0,0,1,1,1,0,1,0,1,1,1,0,1,
1,0,0,0,0,0,1,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,0,0,1,
1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,
0,0,1,1,1,0,0,1,1,0,1,0,0,1,0,1,1,1,1,1,0,0,1,1,1,
1,0,1,1,0,1,0,1,0,1,1,0,0,0,0,0,0,1,0,1,0,0,1,1,0,
0,1,1,1,0,0,1,1,1,1,0,1,0,1,1,0,0,1,1,1,1,1,0,0,1,
1,1,1,0,1,1,1,0,1,0,1,1,1,0,0,0,1,1,0,1,0,0,1,1,0,
0,0,1,1,0,0,1,1,0,0,0,1,1,1,1,0,1,0,1,0,1,1,1,0,1,
1,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,1,0,
1,0,1,1,0,1,1,0,1,0,1,1,1,0,0,0,1,1,1,1,1,0,1,1,1,
1,0,0,1,0,0,1,1,1,1,1,1,0,1,0,1,1,0,0,1,1,0,0,0,1,
0,0,1,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0,
1,0,0,1,1,0,0,0,1,0,1,1,0,1,0,0,1,0,0,0,0,0,0,0,0,
1,1,1,0,1,0,1,0,1,0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
1,0,1,0,1,0,0,0,1,0,1,0,1,1,0,1,1,0,1,0,0,0,0,0,1,
1,1,1,0,1,1,1,1,1,0,0,0,0,1,1,1,0,0,1,0,1,1,1,0,1,
1,1,1,1,1,1,0,0,1,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,1,
0,0,1,1,0,0,1,1,1,0,0,1,1,0,0,1,1,0,1,0,1,1,1,0,1,
1,0,0,0,1,0,1,0,0,0,1,0,1,1,1,0,1,0,1,0,0,0,0,0,1,
1,1,1,1,1,0,1,1,0,1,1,0,1,1,1,0,1,0,1,1,1,1,1,1,1,

};

int main(void)
{
    unsigned char text, page;

    TWIinit();
    _delay_ms(20);
    TWIdocmd(lcdi0);
    _delay_ms(200);
    TWIdocmd(lcdi1);
    _delay_ms(200);
    TWIdocmd(lcdi2);
    _delay_ms(10);
    TWIdocmd(lcdi3);
    _delay_ms(10);
    TWIdocmd(lcdi4);
    _delay_ms(10);
    TWIdocmd(lcdi5);
    _delay_ms(10);
    TWIdocmd(lcdi6);

    unsigned  p = 0;
    for (page = 0xb0; page < 0xbd; page++, p+= 50) {
        sendlcda0[3] = page;
        TWIdocmd(sendlcda0);
        _delay_ms(10);
        for (text = 0; text < 160; text++) {
                unsigned char t;
                t = image[p + (text>>2)] ? 0 : 0xf;
                t |= image[p + (text>>2)+25] ? 0 : 0xf0;
                if( text > 99 )
                    t = 0xff;
                t ^= 0xff;
                unsigned char g = 0xff;//(text / 8) ^ (page & 15);
                memset(&sendlcdb0[3], 0, 4);
                if (g & 8)
                    sendlcdb0[3] = t;
                if (g & 4)
                    sendlcdb0[4] = t;
                if (g & 2)
                    sendlcdb0[5] = t;
                if (g & 1)
                    sendlcdb0[6] = t;

                TWIdocmd(sendlcdb0);
                _delay_us(250);
            }
        }
        _delay_ms(500);
        for(;;);
}
                                                                                                                                                               
                                    
