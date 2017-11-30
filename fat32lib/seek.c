#include "fat32private.h"

u32 tellfile(void)
{
    return fdisplacement;
}

void seekfile(u32 to, u8 whence)
{
    currclus = ireadclus;       // seek 0..
    fdisplacement = 0;
    secinclus = 0;
    byteinsec = 0;              // force reread
    rmw = 1;                    // seek will reread

    if (whence == 1)
        to += fdisplacement;    // WARNING - fdisplacement may not be in sync!
    if (whence == 2)
        to = totfsiz - to;
    if (to > totfsiz)
        to = totfsiz;
    fdisplacement = to;

    while (to > ((u32) fpm.spc << 9)) {
        currclus = nextclus(currclus);
        to -= (u32) fpm.spc << 9;
    }
    while (to > 511) {          // maybe division is faster?
        secinclus++;
        to -= 512;
    }
    byteinsec = to;
    if (rmw)                    // FIXME yes, I know I set it just above. - alternative rmw methods
        readsec(clus2sec(currclus) + secinclus);
    if (fdisplacement == totfsiz)
        rmw = 0;                // no (further) read needed
}
