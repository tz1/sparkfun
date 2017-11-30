// Copyright 2010 tz, all rights reserved
#ifndef __FAT32PRIV__
#define __FAT32PRIV__
#include "fat32.h"
#include "sdhc.h"
#define clus2sec(x) (fpm.data0 + fpm.spc * x  )
//#define get4todw(x) { x = *c++; x |= *c++ << 8; x |= ((u32) *c++) << 16; x |= ((u32) *c++) << 24; }
// arduino avr-gcc hiccup
#define get4todw(x) { x = (c[2] | c[3] << 8); x <<= 16; x |= (*c | c[1] << 8); c += 4; }
#define get2tow(x) { x = *c++; x |= *c++ << 8; }

#define tzmemcpy(d,s,l) {u8 *xd=(void *)d, *xs=(void *)s;u16 lx=l;while(lx--) *xd++=*xs++; }
#define tzmemset(d,s,l) {u8 *xd=(void *)d, xs=(u8)s;u16 lx=l;while(lx--) *xd++=xs; }
#define tzmemclr(d,l) {u8 *xd=(void *)d;u16 lx=l;while(lx--) *xd++=0; }
//#define tzstrcpy(d,s) {u8 *xd=(void *)d, *xs=(void *)s;while(*xs) *xd++=*xs++; }

static inline u8 tzfncmp(u8 * d, u8 * s)
{
    u8 *xd = (void *) d, *xs = (void *) s;
    u8 lx = 11;
    while (--lx)
        if (*xd++ != *xs++)
            break;
    return lx;
}

#ifndef FAT32EXTERN
#define FAT32EXTERN extern
#endif

// filled in from boot sect - computed
typedef struct {
    // from boot sector
    u32 fat0;                   // sec address of start of FAT(s)
    u32 data0;                  // sec address of cluster 0 (not 2!)
    u32 spf;                    // sectors per FAT
    u32 mxcl;                   // largest possible cluster number - DERIVED
    u32 d0c;                    // cluster of root directory file
    u32 hint;                   // address of hint sector
    u8 nft;                     // number of FATs
    u8 spc;                     // data sectors per cluster
} fatparams;

// PER VOLUME

FAT32EXTERN fatparams fpm;

// PER FILE - also need filesectbuf from sdhc.h

//cluster of the current directory (the opened file or where the file is)
FAT32EXTERN u32 dirclus;

//getdirent

// sector and entry for directory entry of current file
FAT32EXTERN u32 direntsect;  // sector within cluster of 8.3
FAT32EXTERN u32 longentsect; // sector within cluster of longent
FAT32EXTERN char direntnum;  // entry number within sector
FAT32EXTERN char longentnum;

// read or write
FAT32EXTERN u32 ireadclus;      // initial cluster of current file
FAT32EXTERN u32 totfsiz;        // size in bytes of current file (synced with dirent)

// read/write positional state
FAT32EXTERN u32 fdisplacement;  // the next byte in the file to be read or written;
FAT32EXTERN u32 currclus;       // current cluster number in chain
FAT32EXTERN u8 secinclus;       // which sector in the cluster
FAT32EXTERN u16 byteinsec;      // which byte in the sector
// Theoretically (secinclus<<11)+byteinsec should == fpos mod clustersize
FAT32EXTERN u8 rmw;             // read-mod-write mode, off after new file, append, and truncate

// END-per-file

u32 nextclus(u32 clus);

// ATTR X X ARCH DIR VOLID SYS HID RO
// DATE Y-1980, 7 bits : Month 1-12 J=1, 4 bits, Day, 5 bits, 1-31
#define todosdate(y,m,d) ((y-1980)<<9|m<<5|d)
// TIME Hour, 5bits, 0-23 ; Minute 6bits, 0-59 ; Secs/2, 5 bits, 0-29 for 0-58.
#define todostime(h,m,s) (h<<11|m<<5|s>>1)
#endif
