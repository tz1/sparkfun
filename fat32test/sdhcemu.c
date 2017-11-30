#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include "sdhc.h"

int sdhcfd = -1;

u32 sdnumsectors = 0xffffffff;

static u8 mybuf[600];

u8 *filesectbuf = &mybuf[4];

#define LSEEK lseek
#define SECSIZE SECSIZE

u8 sdhcinit()
{
    sdnumsectors = LSEEK(sdhcfd, 0, SEEK_END) >> 9;
    return 0;
}

u8 cardinfo(u8 which)
{
    return 1;
}

u8 readsec(u32 sector)
{
    //   printf( "read sect: %llu\n", sector );
    if (-1 == LSEEK(sdhcfd, (unsigned long long) sector * SECSIZE, SEEK_SET))
        return -1;
    if (SECSIZE != read(sdhcfd, filesectbuf, SECSIZE))
        return 1;
    return 0;
}

u8 writesec(u32 sector)
{
    //   printf( "read sect: %llu\n", sector );
    if (-1 == LSEEK(sdhcfd, (unsigned long long) sector * SECSIZE, SEEK_SET))
        return -1;
    if (SECSIZE != write(sdhcfd, filesectbuf, SECSIZE))
        return 1;
    return 0;
}
