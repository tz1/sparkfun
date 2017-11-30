// Copyright 2010 tz, all rights reserved
#include "typefix.h"
u8 *filesectbuf;

// set ports, talk to card, set for the rest.
u8 sdhcinit(void);

// fills first 18 bytes of filesectbuf with CID or CSD for 0 or 1, returns 0 if successful
u8 cardinfo(u8 which);

//
u8 setlock(u8 noyestog);

// read/write filesectbuf to sector X
u8 readsec(u32 addr);
u8 writesec(u32 addr);
// number of sectors on medium
u32 sdnumsectors;

void erasecard(void);
u8 cardnotbusy(void);


u8 cardstat(void);

u8 cardpassword(u8 op, u8 * pw, u8 pwlen);
