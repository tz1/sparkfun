#include "fat32private.h"

int getdirent(u8 * dosmatch)
{
    if (!dosmatch)
        return -1;
    u32 iclus = dirclus;
    if (!iclus)                 // following a .. back to root
        iclus = fpm.d0c;
    direntsect = 0;
    u16 entcnt = 0;
    do {                        // more clusters
        u16 secinclus;
        u32 thissector;
        for (secinclus = 0; secinclus < fpm.spc; secinclus++) { // sector within cluster
            thissector = clus2sec(iclus) + secinclus;
            if (readsec(thissector))
                return -1;
            u8 *c, i;
            for (i = 0; i < 16; i++) {
                c = &filesectbuf[i << 5];
                if (!*c) {
                    if (*dosmatch)
                        return 1;
                    else
                        return entcnt;
                }
                if (*c == 0xe5) // deleted
                    continue;
                u8 attr = c[11];
                if (attr == 0x0f) {     // long name ent
                    if (*c & 0x40) {
                        longentsect = thissector;
                        longentnum = i;
                    }
                    continue;
                }
                entcnt++;       // count entries
                //               printf( ">%11.11s\n", c );
                if (!tzfncmp(c, dosmatch)) {
                    // RENAME HERE, memcpy 11 (or 3 for EXT) and writesec(thissector)

                    // check attributes, etc.
                    u16 chf;
                    c += 20;
                    get2tow(chf);
                    ireadclus = chf;
                    ireadclus <<= 16;
                    c += 4;
                    get2tow(chf);
                    ireadclus += chf;

                    direntsect = thissector;
                    direntnum = i;
                    get4todw(totfsiz);

                    // Maybe more qualifiers - real size or directory for now
                    if (totfsiz || attr & 0x10)
                        seekfile(0, 0);

                    if (attr & 0x10) {
                        dirclus = ireadclus;
                        totfsiz = 0xffffffff;
                        return 2;
                    }
                    return 0;
                }
                else
                    longentsect = 0;
            }
        }
        iclus = nextclus(iclus);
    } while (iclus < 0xffffff0);
    return 1;
}
