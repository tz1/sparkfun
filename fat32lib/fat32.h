#include "typefix.h"
// Copyright 2010 tz, all rights reserved

//MOUNT

/* mount/initialize, returns:
   1 no structure errors, but no FAT32 filesystem found
   0 on success
   -1 read or write failed
   -2 bad magic number - not FAT32 or damaged

   Parameter is reserved to specify Nth FAT32 filesystem on medium
   Ignored, set to zero going in for now.
 */
int mount(int);

// if there is no partition table, or it is not MBR, but you can determine the
// starting sector, this will do a mount.  Partlen is only to insure the cluster
// count doesn't overflow - use 0xffffffff or the actual numver or sectors.
int rawmount(u32 firstsector, u32 partlen);

// points "current" directory to root directory - equivalent to many "cd .."
void resetrootdir(void);

// resets file structurs to point to the current directory instead of the file.
void resettodir(void);

// GETDIRENT
// takes 11 byte input, 8.3 fn e.g. "hello.c" is "HELLO   C  "
// returns 0 - found/set file, 2 - found dir and changed into it; else error
// combined open and chdir.  Note that current directory is readable via
// readbyte and current buf - readnext
int getdirent(u8 * dosmatch);

//NOTE: to read the directory as a file (e.g. for directory listings), 
// after the directory is reset or opened you need to do seekfile(0,0) 
// to read in the first buffer.  The directory search does NOT use the
// file read system so the structures won't be valid before a seek.  When
// a file is opened by getdirent, a seekfile(0,0) is performed internally.

// CREAT

// creates new file or directory (attr with 0x10 set) - changes to dir or ready to write if xero return
// combined creat / mkdir
// if dosname is NULL (a null pointer) it creates %08X.LOG where the number is 1 greater than any existing
u8 newextension[4]; //= "LOG";  This will be the extension for the automatic new files
int newdirent(u8 * dosname, u8 attr);
u32 nextlogseq; // BCD(!) number for the new file - New files will be this or greater

// sets time for newly created files (incs by 2 seconds per new file)
// sec/min/hour are zero based, day and month start at 1.
// years are since the year 2000
void settime(u8 second, u8 minut4e, u8 hour, u8 day, u8 month, u8 yrssince2000);

// WRITE

// returns 0 if ok, 1 if no more space, -1 if error
int writebyte(u8 c);
// writes next sector, returns 512
int writenextsect(void);        // writes entire secbuf, links, continues at next

// Write Sync helpers

// write out partially written sector
void flushbuf(void);

// DO flushbuf BEFORE CALLING THIS IF THE BUFFER IS NOT FLUSHED
// update dirent for writes - mainly file size if changed.past EOF.
// Maxout extends to end of cluster for crash recovery - won't miss anything
void syncdirent(u8 maxout);

// For current file - change name entry - FLUSH before, SEEK after just like syncdirent.
void setname(u8 *newname, u8 *newext);

// reset next free clus and avail clus to unknown
void zaphint(void);

// DELTRUNC

// EOF is current seek position
void truncatefile(void);

// deletes file found by dirent
// 0 on success, -1 notset 1 directory (later not empty)
int deletefile(void);

// SEEK

// return current position in file
u32 tellfile(void);

// whence: 0=SET 1=CUR 2=END
void seekfile(u32 displacement, u8 whence);

// READ

// returns char or -1 for EOF
int readbyte(void);
// reads next sector, returns 512, or remainder if at EOF
u16 readnextsect(void);

// EXPERIMENTAL format card
void fat32format(void);
