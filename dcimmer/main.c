#include "typefix.h"

#ifndef F_CPU
#define F_CPU 14745600
#endif
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

void waitfortrig() {   
    while( ( PORTD & 0xc ) != 0xc );
    return;
}

#include "uart.h"
#include "fat32.h"
#include "sdhc.h"

#include <string.h>

static unsigned char cmdjpeg[6] = "\xAA\x01\x00\x07\x00\x07";
static unsigned char cmdreq[6] = "\xAA\x04\x05\x00\x00\x00";
static unsigned char cmdsnap[6] = "\xAA\x05\x00\x00\x00\x00";
static unsigned char cmdp512[6] = "\xAA\x06\x08\x00\x02\x00";
static unsigned char cmdbaud[6] = "\xAA\x07\x01\x01\x00\x00";
//static unsigned char cmdrst[6] = "\xAA\x08\x00\x00\x00\x00";
static unsigned char cmdsync[6] = "\xAA\x0D\x00\x00\x00\x00";
static unsigned char cmdack[6] = "\xAA\x0E\x0D\x00\x00\x00";
//static unsigned char cmdnak[6] = "\xAA\x0F\x00\x00\x00\x00";

static unsigned char buf[8];

static int readser(unsigned char *buf, unsigned len, unsigned char tmo2) {
    unsigned cnt = 0, tmo;
    int i = -1;
    while( len-- ) {
        tmo = 65535;
        while( --tmo ) {
            i = uartgetch();
            if( i >= 0 )
                break;
        }
        if( i < 0 && --tmo2 )
            continue;
        if( !tmo2 )
            break;
        *buf++ = i;
        cnt++;
    }
    return cnt;
}

static int readlen(unsigned char *buf, unsigned len) {
    unsigned cnt = 0;
    int i;
    while( len-- ) {
        for(;;) {
            i = uartgetch();
            if( i >= 0 )
                break;
        }
        *buf++ = i;
        cnt++;
    }
    return cnt;
}

static void hwinit()
{
    DDRD |= 0xe0;
    DDRB |= 0x20;
    PORTD = 0xc; // pullup lines, white LED
    sei(); // here in case we add other things and want them all up at once
}

static int cmdwack(unsigned char *cmd)
{
    unsigned char max = 5;
    while (max--) {
        PORTD ^= 0x20; //red
        txenqueue(cmd, 6);
        if( 6 != readser(buf, 6, 20) )
            continue;
        if (memcmp(cmdack, buf, 2)) 
            continue;
        if (buf[2] == cmd[1])
            return 0;
    }
    return 1;
}

static void initcam() {
    inituart(1152);
    memset( &cmdack[3], 0, 3 );
    cmdack[2] = 0x0d;
    while( uartgetch() > 0 );
    PORTD |= 0xe0;
    for(;;) {
        PORTD ^= 0x20;
        txenqueue(cmdsync, 6);
        if (6 != readser( buf, 6, 2))
            continue;
        PORTD |= 0x20;
        if (memcmp(cmdack, buf, 3))
            continue;
        PORTD |= 0x40;
        cmdack[3] = buf[3]; // may be needed
        if (6 != readser( buf, 6, 2))
            continue;
        if(!(memcmp(cmdsync, buf, 6)))
            break;
    }
    PORTD &= ~0x80;

    cmdack[2] = 0xd;
    txenqueue(cmdack, 6);

    _delay_ms(10);
    cmdwack(cmdbaud);
    inituart(9216);
    _delay_ms(10);

    cmdwack(cmdjpeg);
    cmdwack(cmdp512);

    PORTD |= 0xe0;
    return;
}

static unsigned long size;

static void takepic() {
    cmdwack(cmdsnap);
    cmdwack(cmdreq);
    readlen(buf, 6);
    size = buf[5];
    size <<= 16;
    size |= buf[4] << 8;
    size |= buf[3];
    cmdack[2] = cmdack[3] = 0;

    unsigned len, cks, cnt;
    unsigned pktnum = 0;
    unsigned long xsize = 0;
    unsigned maxpkt = size / 506;

    while (pktnum <= maxpkt && xsize < size) {
        PORTD ^= 0x40;

        cmdack[4] = pktnum;
        cmdack[5] = pktnum >> 8;
        txenqueue(cmdack, 6);
        readlen(buf, 4);
        len = buf[3] << 8;
        len |= buf[2];

        xsize += len;
        cks = buf[0] + buf[1] + buf[2] + buf[3];

        cnt = 0;
        while( cnt < len ) {
            int j = uartgetch();
            if( j < 0 )
                continue;
            cnt++;
            cks += j;
            writebyte(j);
        }

        readlen(buf, 2);
        if ((cks & 0xff) != buf[0]) break;
        ++pktnum;
    }
    PORTD &= ~0x40;
    cmdack[4] = cmdack[5] = 0xf0;
    txenqueue( cmdack, 6);
}

static unsigned char dcimdir[] =  "DCIM       ";
static unsigned char jtdir[] =    "100JPGTR   ";
static unsigned char cfigfile[] = "JPEGTRIGCFG";

static void configure() {
#if 0
    seekfile(0,0);
    for (;;) {
        i = readbyte();
        // parse
    }
#endif
};

static int fsinit() {
    int i;
    i = sdhcinit();
    if( i )
        return 7;        
    i = mount(0);
    if (i)
        return 5;
    seekfile(0, 0);

    i = getdirent(cfigfile);
    if (!i) {
        configure();
        resettodir();
        seekfile(0,0);
    }

    i = newdirent(dcimdir, 0x10);
    if( i && i != 2 )
        return 6;
    if( i == 2 ) {
        i = getdirent(dcimdir);
        if( i != 2 )
            return 3;
    }

#if 1
    nextlogseq = 0x1000000;
    strcpy( newextension, "   " );
    i = newdirent(NULL, 0x10);
#else
    i = newdirent(jtdir, 0x10);
#endif
    if( i && i != 2 )
        return 2;
    if( i == 2 ) {
        i = getdirent(jtdir);
        if( i != 2 )
            return 1;
    }
    strcpy( newextension, "JPG" );
    nextlogseq = 0;
    return 0;
}

int main(void)
{
    int i;

    hwinit();
    do {
        i = fsinit();
        PORTD |= 0xe0;
        PORTD ^= i << 5;
    } while( i );

    initcam();
    PORTD &= ~0x40;
    //sleep(2)
    _delay_ms(2000);
    for(;;) {
        _delay_ms(1); // sync camera frame
        waitfortrig();
        PORTD ^= 0x60;

        i = newdirent(NULL, 0);
        if( i )
            return -2;

        PORTD ^= 0xa0;
        takepic();
        PORTD ^= 0x40;

        flushbuf();
        syncdirent(0);
        resettodir();

        PORTD ^= 0xa0;
    }
}
