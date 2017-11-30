#include <string.h>
#include "sdhc.h"

static const unsigned char bs0[17] =
  { 0xeb, 0x00, 0x90, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 };
static const unsigned char bs1[19] =
  { 0x4e, 0x4f, 0x20, 0x4e, 0x41, 0x4d, 0x45, 0x20, 0x20, 0x20, 0x20, 0x46, 0x41, 0x54, 0x33, 0x32, 0x20, 0x20, 0x20 };
static const unsigned char fat0[] = { 0xf8, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0x0f };

void fat32format(void)
{
    const unsigned long start = 63;     // 8192
    unsigned long psz = sdnumsectors - start;

    if (psz < 66600)
        return;                 // too small

    memset(filesectbuf, 0, 512);
    filesectbuf[510] = 0x55;
    filesectbuf[511] = 0xaa;

    filesectbuf[0x1bf] = start / 63;    //1 * 63 + 1
    filesectbuf[0x1c0] = start % 63 + 1;
    filesectbuf[0x1c2] = 0xc;   // W95 FAT32 LBA

    unsigned cyl, head, sect;
    sect = sdnumsectors % 63;
    head = sdnumsectors / 63 % 256;
    cyl = sdnumsectors / 63 / 256;
    if (cyl > 1023) {
        cyl = 1023;
        head = 254;
        sect = 63;
    }

    filesectbuf[0x1c3] = head;
    filesectbuf[0x1c4] = sect + ((cyl >> 2) & 0xc0);
    filesectbuf[0x1c5] = cyl & 0xff;

    filesectbuf[0x1c6] = start; // first data sector; 8192?
    filesectbuf[0x1c7] = start >> 8;

    filesectbuf[0x1ca] = psz;
    filesectbuf[0x1cb] = psz >> 8;
    filesectbuf[0x1cc] = psz >> 16;
    filesectbuf[0x1cd] = psz >> 24;
    writesec(0);

    memset(filesectbuf, 0, 512);
    filesectbuf[510] = 0x55;
    filesectbuf[511] = 0xaa;
    writesec(start + 2);
    writesec(start + 8);
    memcpy(filesectbuf, "RRaA", 4);
    memcpy(filesectbuf + 484, "rrAa", 4);
    memset(filesectbuf + 488, 0xff, 8);
    writesec(start + 1);
    writesec(start + 7);

    memset(filesectbuf + 484, 0, 12);
    memcpy(filesectbuf, bs0, 17);
    memcpy(filesectbuf + 0x47, bs1, 19);

    filesectbuf[0x15] = 0xf8;   //media
    filesectbuf[0x18] = 0x3f;   //sects/trk
    filesectbuf[0x1a] = 0xff;   //heads
    filesectbuf[0x1d] = 0x20;   //hidden

    filesectbuf[0x20] = psz;
    filesectbuf[0x21] = psz >> 8;
    filesectbuf[0x22] = psz >> 16;
    filesectbuf[0x23] = psz >> 24;

    unsigned spc = 1;           // sectors per cluster
    //if (psz < 532480) spc = 1; else spc=8; to 16GB
    if (psz < 1UL << 22)
        spc = 2;
    else if (psz < 1UL << 23)
        spc = 4;
    else
        // Microsoft official starts here
    if (psz < 1UL << 24)
        spc = 8;
    else if (psz < 1UL << 25)
        spc = 16;
    else if (psz < 1UL << 26)
        spc = 32;
// DOS often limits clusters to 32KB, so final else might be here
    else if (psz < 1UL << 27)
        spc = 64;
    else
        spc = 128;

    unsigned long fatsz = 1 + ((psz - 32) << 2) / ((spc << 9) + 8);

    filesectbuf[13] = spc;

    filesectbuf[0x24] = fatsz;
    filesectbuf[0x25] = fatsz >> 8;
    filesectbuf[0x26] = fatsz >> 16;
    filesectbuf[0x27] = fatsz >> 24;

    filesectbuf[0x2c] = 2;      //root dir clus
    filesectbuf[0x30] = 1;      //ver
    filesectbuf[0x32] = 6;      //backup area

    filesectbuf[0x40] = 0x80;   //biosdrvnum
    filesectbuf[0x42] = 0x29;   //flag
    // 4 random bytes - date/time typ
    // filesectbuf[0x43] = rand;
    writesec(start);
    writesec(start + 6);
    memset(filesectbuf, 0, 512);

    memcpy(filesectbuf, fat0, 12);
    writesec(start + 32);
    writesec(start + 32 + fatsz);
}
