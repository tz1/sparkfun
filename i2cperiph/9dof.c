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


#define WADXL (0xa6)
#define RADXL (WADXL+1)
unsigned char msgsetup[] = {
    3, WADXL, 0x2d, 8,
    3, WADXL, 0x31, 0xb,
    3, WADXL, 0x2c, 0xd,        //lowers rez to go faster
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


static unsigned tens[] = {1, 10, 100, 1000, 10000};
static void numout(unsigned val, unsigned char pfx)
{
    unsigned char *c, buf[8];
    unsigned t,cnt;
    c = buf;
    *c++ = pfx;
    cnt = 1;
    if( val > 32768 ) {
        *c++ = '-';
        cnt++;
        val = ~val;
        val++;
    }
    t = 4;
    while( t && val < tens[t] )
        t--;
    do {
        *c = '0';
        while( val >= tens[t] ) {
            val -= tens[t];
            (*c)++;
        }
        cnt++;
        c++;
    } while( t-- );
    txenqueue( buf, cnt );
}


int main(void)
{
    unsigned char cnt;
    int xo, yo, zo;
    int adcnt;

    inituart(20000);

    for (;;) {
        TWIinit();
        printf("9DoF\n");
        TWIdocmd(sendhmci); // Mag
        //        TWIdocmd(msgsetup); // Accel
        //        TWIdocmd(startacq); // Baro/Temp

        _delay_ms(200);
        TWIdocmd(sendhmci);  // to insure init, need to do twice
        TWIdocmd(sendhmci);  // to insure init, need to do twice

        cnt = 0;
        adcnt = 0;
	ADMUX = 0x12;
	ADCSRA = 0xc6;
        while (cnt++ < 200) {

#if 0
            TWIdocmd(ready);
            if( 0xe == (0xe & ready[5]) 
                && (0x20 & ready[5]) ) // conversion ready
                {
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
#if 0
                    numout(temp / 100,'T');
                    numout(temp % 100,'.'); // no lead zero yet
                    txenqueue((unsigned char *)"\r\n", 2 );
#else
                    printf( "T%d.%02d\n", temp / 100, temp % 100 );
#endif
                    unsigned long press;
                    press = getdatap1[5];
                    press <<= 8;
                    press |= getdatap2[5];
                    press <<= 8;
                    press |= getdatap2[6];
                    press += 2;
                    press >>= 2;
                    printf( "B%ld\n", press );
                }
#endif
            if( !(ADCSRA & 0x40) ) {
                switch(adcnt) {
                case 0:
                    numout(ADC,'Y');
                    txenqueue((unsigned char *)"\r\n", 2 );
                    break;
                case 1:
                    numout(ADC,'R');
                    txenqueue((unsigned char *)"\r\n", 2 );
                    break;
                case 2:
                    numout(ADC,'P');
                    txenqueue((unsigned char *)"\r\n", 2 );
                    break;
                case 6:
                    numout(ADC,'Q');
                    txenqueue((unsigned char *)"\r\n", 2 );
                    break;
                case 7:
                    numout(ADC,'S');
                    txenqueue((unsigned char *)"\r\n", 2 );
                    break;
                }
                adcnt++;
                if( adcnt == 3 )
                    adcnt = 6;
                if( adcnt > 7 )
                    adcnt = 0;
                ADMUX = 0x40 + adcnt;
                ADCSRA = 0xc6;
            }

            while(0) {
                TWIdocmd(queryfifo);
                unsigned char qlen = queryfifo[5];
                if (!qlen)
                    break;
                while (qlen--) {
                    cnt = 0;
                    TWIdocmd(readxyz);
                    xo = readxyz[5] | (readxyz[6] << 8);
                    yo = readxyz[7] | (readxyz[8] << 8);
                    zo = readxyz[9] | (readxyz[10] << 8);
                    numout(xo,'A');
                    numout(yo,',');
                    numout(zo,',');
                    txenqueue((unsigned char *)"\r\n", 2 );
                }
            }
            TWIdocmd(sendhmcpr);
            TWIdocmd(sendhmcrd1);
            if (sendhmcrd1[2] == 5) {
                cnt = 0;
                TWIdocmd(sendhmcrd2);
                xo = sendhmcrd2[3] | (sendhmcrd2[2] << 8);
                yo = sendhmcrd2[5] | (sendhmcrd2[4] << 8);
                zo = sendhmcrd2[7] | (sendhmcrd2[6] << 8);
                    numout(xo,'M');
                    numout(yo,',');
                    numout(zo,',');
                    txenqueue((unsigned char *)"\r\n", 2 );
            }

        }
    }
}
