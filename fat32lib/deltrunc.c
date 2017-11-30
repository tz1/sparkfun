#include "fat32private.h"

static void trimclus(u32 clus)
{
    u32 csec, nxclus;
    u8 *c, *d;

    if (clus == fpm.d0c)
        return;                 // trimming root dir is a nono

    while (clus > 1 && clus < 0x0ffffff0) {
        csec = clus >> 7;
        readsec(fpm.fat0 + csec);       // read sector with cluster
        clus &= 127;            // index within sector
        for (;;) {
            d = c = &filesectbuf[clus << 2];
            get4todw(nxclus);   // get linked cluster
            tzmemclr(d, 4);
            if (nxclus <= 1 || (nxclus >> 7) != csec)
                break;          // end of sector or chain
            clus = nxclus & 127;
        }
        u8 i;
        for (i = 0; i < fpm.nft; i++)
            writesec(fpm.fat0 + csec + i * fpm.spf);
        clus = nxclus;
    }
}

static void makelastclus(u32 newclus)
{
    u32 clus, chain = nextclus(newclus);
    readsec(fpm.fat0 + (newclus >> 7)); // read sector with cluster
    clus = newclus & 127;       // index within sector
    u8 *c = &filesectbuf[clus << 2];
    *c++ = 0xff;
    *c++ = 0xff;
    *c++ = 0xff;
    *c = 0x0f;
    int i;
    for (i = 0; i < fpm.nft; i++)
        writesec(fpm.fat0 + (newclus >> 7) + i * fpm.spf);
    trimclus(chain);
}

void truncatefile()
{
    totfsiz = fdisplacement;
    makelastclus(currclus);
    rmw = 0;                    // no (further) read
    byteinsec = fdisplacement & 511;
    syncdirent(0);
    readsec(clus2sec(currclus) + secinclus);
};

int deletefile()
{
    u8 *c;
    if (!direntsect || readsec(direntsect))
        return -1;
    c = &filesectbuf[direntnum << 5];

    // directory and not empty?
    if (c[11] & 0x10)           //&&... chdir, && getdirent("") != 2 )
        return 1;
    *c = 0xe5;
#ifndef LFNDEL
    writesec(direntsect);
#else
    u8 lfncnt = 1;
    for (;;) {
        while (lfncnt < 0x40 && direntnum--) {
            c = &filesectbuf[direntnum << 5];
            if ((c[11] & 0x3f) != 0x0f) {       // ERROR
                lfncnt = 0xff;
                break;
            }
            if (*c == (lfncnt | 0x40))
                lfncnt |= 0x40;
            else if (*c != lfncnt++) {  // ERROR
                lfncnt = 0xff;
                break;
            }
            if (direntsect == longentsect && direntnum == longentnum)
                lfncnt = 0xfe;
            *c = 0xe5;
        }
        writesec(direntsect--);
        // sector straddling dirent (LEAVES JUNK IF ENT STRADDLES NONCONTIG CLUSTERS)
        if (lfncnt >= 0x40 || (longentsect != direntsect && longentsect != direntsect - 1))
            break;
        direntnum = 16;
        readsec(direntsect);
    }
#endif
    direntsect = 0;
    trimclus(ireadclus);
    return 0;
}
