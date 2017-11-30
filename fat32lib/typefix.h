#ifndef __TYPEFIX_H__
#define __TYPEFIX_H__
#ifdef __AVR__
typedef unsigned char u8;
typedef unsigned u16;
typedef unsigned long u32;
#else // LINUX
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32; // I think on 64 bit too?
#endif
#endif
