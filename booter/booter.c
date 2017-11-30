typedef unsigned char u8;
typedef unsigned u16;
typedef unsigned long u32;
// HARDWARE DEPENDENT
#include <avr/io.h>

static u8 sdhcbuf[512 + 16];
static u8 *filesectbuf = &sdhcbuf[4];

static void sendspiblock(u8 * buf, u16 len)
{
    while (len--) {
        SPDR = *buf++;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
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

static inline void waitforspi(u8 val)
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

static void recvspiblock(u16 len)
{
    u8 *buf = filesectbuf;
    while (len--) {
        SPDR = 0xff;
        while (!(SPSR & 0x80));
        SPSR &= 0x7f;
        *buf++ = SPDR;
    }
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

static u8 goidle[6] = { 0 + 0x40, 0, 0, 0, 0, 0x95 };   // R1
static u8 sendifcond[6] = { 8 + 0x40, 0, 0, 1, 0xaa, 0x87 };    //
static u8 readocr[6] = { 58 + 0x40, 0, 0, 0, 0, 0xff }; // R3
static u8 doapp[6] = { 55 + 0x40, 0, 0, 0, 0, 0xff };   // R1
static u8 sendopcond[6] = { 41 + 0x40, 0x40, 0, 0, 0, 0xff };   // R1 - activate init

// send command to SPI card, wait for response
static u8 sendcommand(u8 * cmd)
{
    sendffspi(1);               // needed to insure card is ready
    sendspiblock(cmd, 6);
    return waitnotspi(0xff);    // return first status byte
}

static u8 rw1bcmd[6];
static u8 ishccard;

static u8 readsec(u32 blkaddr)
{
    u8 ret = 1, *c;
    if (!ishccard)
        blkaddr <<= 9;
    c = rw1bcmd;
    *c++ = 0x51;                // read
    *c++ = blkaddr >> 24;
    *c++ = blkaddr >> 16;
    *c++ = blkaddr >> 8;
    *c++ = blkaddr;
    *c = 0xff;
    csact();
    if (!sendcommand(rw1bcmd)) {
        waitforspi(0xfe);
        recvspiblock(514);
        // CRC16 appended H,L
        ret = 0;
    }
    csinact();
    return ret;
}

#define clus2sec(x) (fatdata0 + fatspc * x  )
#define get4todw(x) { x = (c[2] | c[3] << 8); x <<= 16; x |= (*c | c[1] << 8); c += 4; }
#define get2tow(x) { x = *c++; x |= *c++ << 8; }
// from boot sector
static u32 fatfat0;             // sec address of start of FAT(s)

static inline u8 tzfncmp(u8 * d, char *s)
{
    u8 *xd = (void *) d, *xs = (void *) s;
    u16 lx = 12;
    while (--lx)
        if (*xd++ != *xs++)
            break;
    return lx;
}

static u32 nextclus(u32 clus)
{
    readsec(fatfat0 + (clus >> 7));     // read sector with cluster
    clus &= 127;                // index within sector
    u8 *c = &filesectbuf[clus << 2];
    get4todw(clus);             // get linked cluster
    return clus;
}

int main()
{
    u32 addr, partlen;

    unsigned acc, count, centibaud = 576;
    // we don't do this often, and it should be more accurate than divides
#define BCLK (F_CPU/800)
    acc = 0;
    count = 0;
    while (acc < BCLK)
        acc += centibaud, count++;
    if (acc - BCLK > centibaud >> 1)
        count--;
    UBRR0 = count - 1;
    UCSR0C = 0x06;          //8N1 (should be this from reset)
    UCSR0A = 0xE0 | 2;      // clear Interrupts, UART at 2x (xtal/8)
    UCSR0B = 0x18;          // oring in 0x80 would enable rx interrupt


    DDRB = 0x2c;                // set port for SPI
    PORTB |= 4;
    SPCR = 0x50;                // At least with SDHC, you can max out from the betginning
    // the spec acutally says 400kHz until the CSD is read with the max freq.
    // FIXME if there is a problem
    SPSR = 1;                   // but the timeouts need to be increased proportionately

    sendffspi(10);              //SD cards need 74 clocks to start properly
    csact();
    u8 needretry, nov2card;
    u16 i;

#define MAXTRIES1 50
    for (i = 0; i < MAXTRIES1; ++i)
        if (1 == sendcommand(goidle))   //R1
            break;
    if (i >= MAXTRIES1)
        return 1;

    /* check for version of SD card specification */
    i = sendcommand(sendifcond);
    nov2card = 0;
    if (!(4 & i)) {             //legal command - 2.0 spec
        recvspiblock(5);
        if (!(filesectbuf[2] & 0x01)
          || (filesectbuf[3] != 0xaa))  /* wrong voltage or test pattern */
            return 2;
    } else
        nov2card = 1;

    // technically optional
    ishccard = 0;
    needretry = 0;
    i = sendcommand(readocr);
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
        i = sendcommand(readocr);       // 0x3f, then 7 more
        recvspiblock(5);
        ishccard = (filesectbuf[0] & 0x40);     // has CCS
        needretry = !(filesectbuf[0] & 0x80);
    }
    if (i >= MAXTRIES2)
        return 4;
    if (!ishccard) {            // set block to 512 for plain SD card
        u8 setblocklen[6] = { 16 + 0x40, 0, 0, 2, 0, 0xff };
        sendcommand(setblocklen);
    }
    //    sdnumsectors = cardsize();
    csinact();

    if (readsec(0))
        return -1;
    if (filesectbuf[510] != 0x55 || filesectbuf[511] != 0xaa)
        return -1;
    u8 *c = &filesectbuf[0x1be + 4];
    if (*c != 0xb && *c != 0xc)
        return -1;
    c += 4;
    get4todw(addr);
    get4todw(partlen);
    if (readsec(addr))
        return -1;
    if (filesectbuf[510] != 0x55 || filesectbuf[511] != 0xaa)
        return -2;

    u32 iclus;
    u32 fatdata0;               // sec address of cluster 0 (not 2!)
    u8 fatspc;                  // data sectors per cluster
    {
        // 3-a OEM name
        u16 rsv;                // reserved sectors - first fat starts just after
        u8 fatnft;              // number of FATs
        u32 fatspf;             // sectors per FAT
        c = &filesectbuf[0xd];
        fatspc = *c++;
        get2tow(rsv);
        fatnft = *c++;
        c = &filesectbuf[0x24];
        get4todw(fatspf);
        c += 4;
        get4todw(iclus);
        fatfat0 = addr + rsv;
        fatdata0 = fatfat0 + fatnft * fatspf - (fatspc << 1);
    }
    u16 secinclus;
    do {                        // more clusters
        for (secinclus = 0; secinclus < fatspc; secinclus++) {  // sector within cluster
            if (readsec(clus2sec(iclus) + secinclus))
                return -1;
            for (i = 0; i < 16; i++) {
                c = &filesectbuf[i << 5];
                if (!*c)
                    return 1;
                if (*c == 0xe5) // deleted
                    continue;
                if (c[11] & 0x18)       // label, long, or dir
                    continue;
                if (!tzfncmp(c, "ATMELOADBIN"))
                    goto found;
            }
        }
        iclus = nextclus(iclus);
    } while (iclus < 0xffffff0);
    return 1;

  found:
    {
        u32 totfsiz;
        u32 fdisplacement = 0;  // stop reads, writes
        u16 chf;
        c += 20;
        get2tow(chf);
        iclus = chf;
        iclus <<= 16;
        c += 4;
        get2tow(chf);
        iclus += chf;
        get4todw(totfsiz);
        secinclus = 0;
        do {
            while (secinclus < fatspc) {
                readsec(clus2sec(iclus) + secinclus++);

                for (i = 0; i < 512 && fdisplacement++ < totfsiz; i++) {
                    while (!(UCSR0A & 0x20));
                    UDR0 = filesectbuf[i];
                }

                if (fdisplacement >= totfsiz)
                    return 0;
            }
            secinclus = 0;
            iclus = nextclus(iclus);
        } while (iclus < 0xffffff0);
    }
    return 0;
}
