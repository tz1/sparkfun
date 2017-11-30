// Copyright 2010 tz, all rights reserved
#include "typefix.h"
#include "sdhc.h"
// HARDWARE DEPENDENT
#include <avr/io.h>

u8 sdhcbuf[512 + 16];
u8 *filesectbuf = &sdhcbuf[4];

static void sendspiblock(u8 * buf, u16 len)
{
    if (SPSR & 1) {
        // NOTE you cannot reduce the number of NOPs since they are counting SPI clocks, not a delay
        while (len--) {
            SPDR = *buf++;
            asm(" .rept 11\n nop\n .endr");
        }
        return;
    }
    volatile u8 x;
    while (len--) {
        SPDR = *buf++;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        x = SPDR;
    }
}

static void recvspiblock(u16 len)
{
    u8 *buf = filesectbuf;
    if (SPSR & 1) {
        u8 t;
        SPDR = 0xff;
        while (--len) {
            asm(" .rept 9\n nop\n .endr");
            t = SPDR;
            SPDR = 0xff;
            // there is actuallya race condition if an interrupt occurs here.
            *buf++ = t;         // double buffered
        }
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        *buf = SPDR;
    }
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

static u8 waitnotspi(u8 val)
{
    u8 timeout = 8, got;        // 8 from sd spec.
    while (timeout--) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        got = SPDR;
        if (val != got)
            break;
    }
    return got;
}

static void csact()
{
    PORTB &= ~4;
    sendffspi(1);               // startup cycles
}

static void csinact()
{
    PORTB |= 4;
    sendffspi(1);               // close clock cycles
}

static void spihwinit()
{
    DDRB = 0x2c;                // set port for SPI
    PORTB |= 4;
    SPCR = 0x53;                // Slowest possible for init
    SPSR = 0;
}

//END HARDWARE DEPENDENT

static u8 sendifcond[6] = { 8 + 0x40, 0, 0, 1, 0xaa, 0x87 };    //
static u8 sendopcond[6] = { 41 + 0x40, 0x40, 0, 0, 0, 0xff };   // R1 - activate init
static u8 goidle[6] = { 0 + 0x40, 0, 0, 0, 0, 0x95 };   // R1
static u8 setblocklen[6] = { 16 + 0x40, 0, 0, 2, 0, 0xff };
//edited
static u8 doapp[6] = { 55 + 0x40, 0, 0, 0, 0, 0xff };   // R1
static u8 simple[6] = { 0, 0, 0, 0, 0, 0xff };  // just first byte
static u8 rw1bcmd[6];           // all bytes
static u8 ishccard;

// send command to SPI card, wait for response
static u8 sendcommand(u8 * cmd)
{
    sendffspi(1);               // needed to insure card is ready
    sendspiblock(cmd, 6);
    return waitnotspi(0xff);    // return first status byte
}

static u8 sendsimplecmd(u8 cmd)
{
    simple[0] = cmd + 0x40;
    return sendcommand(simple);
}

u32 sdnumsectors = 0;

u8 cardinfo(u8 which)
{
    u8 ret = 1;
    csact();
    if (!sendsimplecmd(which ? 10 : 9)) {       //CID:CSD
        waitforspi(0xfe);
        recvspiblock(18);
        ret = 0;
    }
    csinact();
    return ret;
}

static u8 docrc7(u8 crc, u8 c)
{
    u8 ibit;
    for (ibit = 0; ibit < 8; ibit++) {
        crc <<= 1;
        if ((c ^ crc) & 0x80)
            c ^= 0x09;
        c <<= 1;
    }
    crc &= 0x7F;
    return crc;
}

u8 setlock(u8 nyt)
{
    u8 ret = 1;
    cardinfo(0);
    if (nyt > 2)
        return 0xff;
    else if (nyt == 0)
        filesectbuf[14] &= ~0x10;
    else if (nyt == 1)
        filesectbuf[14] |= 0x10;
    else if (nyt == 2)
        filesectbuf[14] ^= 0x10;

    u8 i, crc7 = 0;
    for (i = 0; i < 15; i++)
        crc7 = docrc7(crc7, filesectbuf[i]);
    filesectbuf[i] = (crc7 << 1) + 1;
    filesectbuf[-1] = 0xfe;
    filesectbuf[16] = 0xff;     // CRCH
    filesectbuf[17] = 0xff;     // CRCL
    csact();
    waitforspi(0xff);           // wait for not busy
    if (!sendsimplecmd(27)) {
        sendspiblock(&filesectbuf[-1], 19);
        waitforspi(0);          // wait for busy
        ret = 0;
    }
    csinact();
    return ret;
}

u8 cardstat()
{
    csact();
    sendsimplecmd(13);
    recvspiblock(1);
    csinact();
    return filesectbuf[0];
}

#include <string.h>
// 0..5 UNLK SETPW CLRPW x LOCK SETNLK ; 8 - force erase ;
// SET if existing, pwlen is old+new, pw is old followed by new.
u8 cardpassword(u8 op, u8 * pw, u8 pwlen)
{
    u8 ret = 1;
    memset(sdhcbuf, 0xff, sizeof(sdhcbuf));
    memcpy(filesectbuf+2, pw, pwlen);
    csact();
    waitforspi(0xff);           // wait for not busy
    if (!sendsimplecmd(42)) {
        filesectbuf[-1] = 0xfe;
        filesectbuf[0] = op;
        filesectbuf[1] = pwlen;
        sendspiblock(&filesectbuf[-1], 515);
        waitforspi(0);          // wait for busy
        ret = 0;
    }
    csinact();
    return ret;
}

u32 cardsize()
{
    if (cardinfo(0))
        return 1;

    // Note: fsb[3] will contain max trans speed to set the SPI,
    // All SD memory cards will be 0x32 or 0x5a for 25Mhz or 50Mhz.
    // SDIO or older MMC may be sloer.
    // Low 3 bits are bits per sec 100k * 10 ** bits (up to 100M)
    // bits 6:3 are multiplier:
    // RSV,1,1.2,1.3, 1.5,2.2.5,3, 3.5,4,4.5,5, 5.5,6,7,8 
    u32 size;
    if (filesectbuf[0] & 0x40) {
        size = filesectbuf[7] & 0x3f;
        size <<= 8;
        size |= filesectbuf[8];
        size <<= 8;
        size |= filesectbuf[9];
        ++size;
        return size << 10;
    }
    else {
        size = (filesectbuf[6] & 3) << 10;
        size |= filesectbuf[7] << 2;
        size |= filesectbuf[8] >> 6;
        size++;
        u16 mult;
        mult = (filesectbuf[9] & 3) << 1;
        mult |= filesectbuf[10] >> 7;
        mult += 2 + (filesectbuf[5] & 0xf);
        size <<= mult - 9;
        return size;
    }
}

static u8 sdhcinitseq()
{
    u8 needretry;
    u16 i;

    // so far, no card requires more than 2 cycles
#define MAXTRIES1 50
    for (i = 0; i < MAXTRIES1; ++i)
        if (1 == sendcommand(goidle))   //R1
            break;
    if (i >= MAXTRIES1)
        return 1;

    // set block length (force)
    sendcommand(setblocklen);

    // check for version of SD card specification
    i = sendcommand(sendifcond);
    //u8 nov2card = 0;
    if (!(4 & i)) {             //legal command - 2.0 spec
        recvspiblock(5);
        if (!(filesectbuf[2] & 0x01)
          || (filesectbuf[3] != 0xaa))  // wrong voltage or test pattern
            return 2;
    }
    //else
    //    nov2card = 1;
//
// may be PW so some following may fail
//
    // technically optional
    ishccard = 0;
    needretry = 0;
    i = sendsimplecmd(58);
    // should verify not illegal command, if so, it is not this kind of card
    recvspiblock(5);
    // Should verify voltage range here, bit 23 is 3.5-3.6, .1v lower going down
    // maybe later wait until not busy, but get the sdhc flag here if set
    if (filesectbuf[0] & 0x80)
        ishccard = (filesectbuf[0] & 0x40);     // has CCS
    else
        needretry = 1;
    // most take only a few if already inited
#define MAXTRIES2 30000
    for (i = 0; i < MAXTRIES2; ++i) {
        doapp[1] = ishccard;
        sendcommand(doapp);     // not for MMC or pre-1, but ignored
        if (!(1 & sendcommand(sendopcond)))
            break;
    }
    if (i >= MAXTRIES2)
        return 3;
    while (needretry && i++ < MAXTRIES2) {      // card was busy so CCS not valid
        sendsimplecmd(58);      // 0x3f, then 7 more
        recvspiblock(5);
        ishccard = filesectbuf[0] & 0x40;       // has CCS
        needretry = !(filesectbuf[0] & 0x80);
    }
    if (i >= MAXTRIES2)
        return 4;
    return 0;
}

u8 sdhcinit()
{
    u8 i;
    spihwinit();
    sendffspi(10);              //SD cards need 74 clocks to start properly
    csact();
    i = sdhcinitseq();
    if (i)
        return i;

    SPCR &= ~3;                 // Change to maximum speed after init
    SPSR |= 1;

    sdnumsectors = cardsize();
    csinact();
    return 0;
}

static void setblockaddr(u32 blkaddr)
{
    if (!ishccard)
        blkaddr <<= 9;
    rw1bcmd[1] = blkaddr >> 24;
    rw1bcmd[2] = blkaddr >> 16;
    rw1bcmd[3] = blkaddr >> 8;
    rw1bcmd[4] = blkaddr;
    rw1bcmd[5] = 0xff;
}

u8 cardnotbusy(void)
{
    csact();
    u8 ret = waitnotspi(0);     // wait 8 cycles for not busy
    csinact();
    return ret;
}

u8 readsec(u32 blkaddr)
{
    u8 ret = 1;
    rw1bcmd[0] = 0x51;          // read
    setblockaddr(blkaddr);
    csact();
    waitforspi(0xff);           // wait for not busy
    if (!sendcommand(rw1bcmd)) {
        waitforspi(0xfe);
        recvspiblock(514);
        // CRC16 appended H,L
        ret = 0;
    }
    csinact();
    return ret;
}

u8 writesec(u32 blkaddr)
{
    u8 ret = 1;
    rw1bcmd[0] = 0x58;          // write
    setblockaddr(blkaddr);
    csact();
    waitforspi(0xff);           // wait for not busy
    if (!sendcommand(rw1bcmd)) {
        filesectbuf[-1] = 0xfe;
        // CRC
        filesectbuf[512] = 0xff;        // CRCH
        filesectbuf[513] = 0xff;        // CRCL
        sendspiblock(&filesectbuf[-1], 515);
        waitforspi(0);          // wait for busy
        ret = 0;
    }
    csinact();
    return ret;
}

void erasecard(void)
{
    rw1bcmd[0] = 0x40 + 32;
    setblockaddr(0);
    csact();
    waitforspi(0xff);           // wait for not busy
    if (sendcommand(rw1bcmd))
        return;
    rw1bcmd[0] = 0x40 + 33;
    setblockaddr(sdnumsectors - 1);
    waitforspi(0xff);           // wait for not busy
    if (sendcommand(rw1bcmd))
        return;
    rw1bcmd[0] = 0x40 + 38;
    waitforspi(0xff);           // wait for not busy
    if (sendcommand(rw1bcmd))
        return;
    waitforspi(0);              // wait for busy
    PORTD &= ~0x20;
    waitforspi(0xff);           // wait for not busy
    PORTD |= 0x20;
    csinact();
}
