// Copyright 2010 tz, all rights reserved
#include "fat32private.h"

u32 nextclus(u32 clus)
{
    readsec(fpm.fat0 + (clus >> 7));    // read sector with cluster
    clus &= 127;                // index within sector
    u8 *c = &filesectbuf[clus << 2];
    get4todw(clus);             // get linked cluster
    return clus;
}

void resettodir(void)
{
    ireadclus = dirclus;
    totfsiz = 0xffffffff;
    fdisplacement = totfsiz;    // stop reads, writes
    direntsect = 0;             // NO files current
}

void resetrootdir(void)
{
    dirclus = fpm.d0c;
    resettodir();
}

//************************************************************************
// NAVIGATE STRUCTURES

int rawmount(u32 addr, u32 partlen)
{
    int i;
    if ((i = readsec(addr)))
        return i;
    if (filesectbuf[510] != 0x55 || filesectbuf[511] != 0xaa)
        return -2;

    // 3-a OEM name
    u8 *c;
    u16 rsv;                    // reserved sectors - first fat starts just after
    u16 fsi;                    // sector of filesystem info (hints)

    // OMITTED check for bytes per sector, assuming 512
    c = &filesectbuf[0xd];
    fpm.spc = *c++;
    get2tow(rsv);
    fpm.nft = *c++;
#ifdef FAT16
    get2tow(); //rootents
    get2tow(fpm.mxcl);
    c = &filesectbuf[0x16];
    get4todw(fpm.spf);
#else
    c = &filesectbuf[0x20];
    get4todw(fpm.mxcl);         // get total
    get4todw(fpm.spf);
#endif
    if (!fpm.mxcl)
        fpm.mxcl = partlen;
    if (fpm.mxcl > partlen)
        return -3;              // more sectors than partition table says
    fpm.mxcl = fpm.mxcl - rsv - (fpm.nft * fpm.spf);
#if 1
    // this is the only divide outside of format.c 
    fpm.mxcl /= fpm.spc;
#else
    //however fpm.spc is supposed to be always a power of 2
    i = fpm.spc;
    while( i >>= 1 )
        fpm.mxcl >>= 1;
#endif
    get2tow(fsi);               // FAT flag - mirroring disable (0x80), ls4b which FAT is master
    if (fsi)
        return -4;              // assume first cluster is master, mirror - 99.9% case

    c += 2;
    get4todw(fpm.d0c);
    get2tow(fsi);

    if (fsi != 1)
        return -5;              //assume fsinfo is at sector 1, 99% case
    fpm.hint = addr + fsi;
    fpm.fat0 = addr + rsv;
    fpm.data0 = fpm.fat0 + fpm.nft * fpm.spf - (fpm.spc << 1);
// FAT16: + (rootents >> 4)

    readsec(fpm.hint);
    // 2 sigs 0, 484
    u32 s0, s1;
    c = filesectbuf;
    get4todw(s0);
    c = &filesectbuf[484];
    get4todw(s1);

    if (s0 != 0x41615252 || s1 != 0x61417272)
        return -6;

    resetrootdir();
    return 0;
}

#ifndef NOEXTPART
// returns 0 on success
static int decodeebr(u32 addr, u32 extpartlen)
{
    u32 partat;
    u8 ebrent;
    u32 addr0 = addr;           //save base address 
    u32 partlen;
    u8 *c;

    if (readsec(addr))
        return -1;
    if (filesectbuf[510] != 0x55 || filesectbuf[511] != 0xaa)
        return -2;

    c = &filesectbuf[0x1be];
    for (ebrent = 0; ebrent < 2; ebrent++) {
        c += 4;
        u8 ptype = *c;
        c += 4;
        get4todw(partat);
        get4todw(partlen);
        switch (ptype) {
        case 0xb:
        case 0xc:
            if (!rawmount(partat + addr, partlen))
                return 0;
            if (readsec(addr))
                return -1;
            break;
        case 5:
        case 0xf:
            addr = addr0 + partat;
            if (readsec(addr))
                return -1;
            c = &filesectbuf[0x1be];
            ebrent = -1;
            break;
        default:
            break;
        }
    }
    return 1;
}
#endif

// index not used, but for Nth FAT32 partition
int mount(int index)
{
    u32 secaddr, partlen;
    u8 mbrent;
    int i;
    if (index == -1)            // explicit whole disk
        return rawmount(0, sdnumsectors);
    if ((i = readsec(0)))
        return i;
    if (filesectbuf[510] != 0x55 || filesectbuf[511] != 0xaa)
        return -1;
    u8 *c = &filesectbuf[0x1be];
    for (mbrent = 0; mbrent < 4; mbrent++) {
        c += 4;
        u8 ptype = *c;
        c += 4;
        get4todw(secaddr);
        get4todw(partlen);
        switch (ptype) {
        case 0xb:
        case 0xc:
            if (!rawmount(secaddr, partlen))
                return 0;
            readsec(0);         // reread;
            break;
#ifndef NOEXTPART
        case 5:
        case 0xf:
            // 5 or f - extended
            if (!decodeebr(secaddr, partlen))
                return 0;
            readsec(0);         // reread,
            break;
#endif
        default:
            break;
        }
    }
    // finally try whole disk
    return rawmount(0, sdnumsectors);
}
