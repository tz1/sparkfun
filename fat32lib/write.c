#include "fat32private.h"

void syncdirent(u8 maxout)
{
    if (!direntsect || readsec(direntsect))
        return;
    u8 *c;
    c = &filesectbuf[direntnum << 5];
    if (fdisplacement > totfsiz)
        totfsiz = fdisplacement;
    u32 curr, t;
    t = totfsiz;
    // this will cause the size to be truncated to the last cluster boundary
    // on an fsck preserving written data in case of a crash
    if (maxout)
        t += fpm.spc << 9;      // add a cluster to current
    // it may already be the correct value
    c += 28;
    get4todw(curr);             // get linked cluster
    if (curr == totfsiz)
        return;
    // update
    c = &filesectbuf[direntnum << 5];

    c[28] = totfsiz;
    c[29] = totfsiz >> 8;
    c[30] = totfsiz >> 16;
    c[31] = totfsiz >> 24;
    writesec(direntsect);
}

// this allocates and links in a new cluseter.
// inlined getclus so for in-sector, it iw r w(w fat copy 2) except where the link crosses sectors
// whire it is r0 ... rN wN(wN) r0 w0(w0).
static int linkclus()
{
    u32 nextc;
    u32 clus = 0, curr = 1;
    u32 cfatoffst, nfatoffst;
    u8 *c;
    u8 i;

    nextc = nextclus(currclus);
    if (nextc < 0xffffff0)
        return 0;

    // from nextclus
    cfatoffst = currclus >> 7;

    nextc = currclus + 1;
    while (nextc < fpm.mxcl) {
        nfatoffst = nextc >> 7;
        if (nfatoffst != cfatoffst) {
            readsec(fpm.fat0 + (nextc >> 7));   // read sector with cluster
            cfatoffst = nfatoffst;
        }
        clus = nextc & 127;     // index within sector
        c = &filesectbuf[clus << 2];
        do {
            get4todw(curr);     // get linked cluster
        } while (curr && ++clus < 128);
        nextc &= ~127;
        if (clus < 128)
            break;
        nextc += 128;
    }
    if (nextc >= fpm.mxcl) {
        currclus = 0;
        return -1;
    }
    nextc += clus;
    c = &filesectbuf[clus << 2];
    *c++ = 0xff;
    *c++ = 0xff;
    *c++ = 0xff;
    *c = 0x0f;

    cfatoffst = currclus >> 7;
    if (cfatoffst != nfatoffst) {       // next clus is in diff FAT sector
        for (i = 0; i < fpm.nft; i++)
            writesec(fpm.fat0 + (nextc >> 7) + i * fpm.spf);
        readsec(fpm.fat0 + cfatoffst);  // read sector with cluster
    }
    c = &filesectbuf[(currclus & 127) << 2];
    // should be 0x0fffffff;
    *c++ = nextc;
    *c++ = nextc >> 8;
    *c++ = nextc >> 16;
    *c = nextc >> 24;

    for (i = 0; i < fpm.nft; i++)       // write the linked version out
        writesec(fpm.fat0 + (currclus >> 7) + i * fpm.spf);
    currclus = nextc;
    secinclus = 0;
    byteinsec = 0;
    return 0;
}

void flushbuf()
{
    //if ! dirty return
    // if byteinsec &&?
    if (currclus > 1)
        writesec(clus2sec(currclus) + secinclus);
    if (totfsiz < fdisplacement)
        totfsiz = fdisplacement;
}

int writenextsect()
{
#ifndef NOBLOCKWRITEDIRS
    if (dirclus == ireadclus)
        return -1;
#endif
    if (currclus < 2)
        return -1;
    writesec(clus2sec(currclus) + secinclus++); // FIXME need to return -2 on error
    fdisplacement += 512;
    fdisplacement &= ~511;
    if (!fdisplacement) {       // add 512 and get zero?
        fdisplacement--;
        return 2;               // EOF - size is maxed out.
    }
    byteinsec = 0;

    if (totfsiz < fdisplacement)
        totfsiz = fdisplacement;

    //NOTE - this will allocate a completely empty cluster if EOF just happened
    if (secinclus >= fpm.spc) {
#if 0
        syncdirent(0);
#endif
        if (linkclus())
            return 1;
        secinclus = 0;
        if (currclus < 2)
            return 1;           // written, but no more space
    }
    if (rmw)
        readsec(clus2sec(currclus) + secinclus);
    return 0;
}

int writebyte(u8 c)
{
    filesectbuf[byteinsec++] = c;
    if (byteinsec > 511)
        return writenextsect();
    if (fdisplacement == 0xffffffff)
        return -1;
    fdisplacement++;
    return 0;
}

// mark cluster avail and firstfree hings as unknown
// placeholder for actual proper update
void zaphint()
{
    u32 s0, s1;
    u8 *c;

    readsec(fpm.hint);
    c = &filesectbuf[488];
    get4todw(s0);
    get4todw(s1);
    if (s0 == 0xffffffff && s1 == 0xffffffff)
        return;
    c = &filesectbuf[488];
    tzmemset(c, 0xff, 8);
    writesec(fpm.hint);
}
