#include "fat32private.h"

u16 readnextsect()
{
    if (fdisplacement >= totfsiz)
        return 0;
    secinclus++;
    if (secinclus >= fpm.spc) {
        secinclus = 0;
        currclus = nextclus(currclus);
        if (currclus >= 0xffffff0)
            return 0;
    }
    readsec(clus2sec(currclus) + secinclus);
    byteinsec = 0;
    fdisplacement += 512;
    fdisplacement &= ~511;
    if (fdisplacement > totfsiz)        // last sector
        return (totfsiz & 511);
    return 512;
}

int readbyte()
{
    if (fdisplacement >= totfsiz)
        return -1;
    if (byteinsec >= 512) {
        readnextsect();
        fdisplacement -= 512;   // undo for buffering
    }
    fdisplacement++;
    return filesectbuf[byteinsec++];
}
