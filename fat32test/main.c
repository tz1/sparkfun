#include "typefix.h"
#ifdef __AVR__
#define BAUD 57600
#ifndef F_CPU
#define F_CPU 16000000 // default to OpenLog
#endif
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>

#include "uart.h"

#define exit(x) sleep_mode()

static int serout(char c, FILE * stream)
{
    if (c == '\n')
        uartputch('\r');
    uartputch( c );
    return 0;
}

static int tzkbhit()
{
    return kbhit();
}

static int tzgetchar()
{
    while( !kbhit() );
    return uartgetch();
}

static FILE fpuart = FDEV_SETUP_STREAM(serout, NULL, _FDEV_SETUP_WRITE);

static void hwinit()
{
    DDRD |= 0x20;
    DDRB |= 0x20;
    inituart(BAUD/100);
    stdout = &fpuart;           //Required for printf init
    sei(); // here in case we add other things and want them all up at once
}

#else

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

extern int sdhcfd;
static int initsec(char *dev)
{
    sdhcfd = open(dev, O_RDWR);
    if (sdhcfd < 0)
        return -1;
    return 0;
}

#include <string.h>
#include <termios.h>
struct termios origtios;
void resettermode()
{
    tcsetattr(0, TCSANOW, &origtios);
}

void hwinit()
{
    struct termios newtios;
    tcgetattr(0, &origtios);
    memcpy(&newtios, &origtios, sizeof(newtios));
    atexit(resettermode);
    cfmakeraw(&newtios);
    newtios.c_oflag |= ONLCR | OPOST;
    tcsetattr(0, TCSANOW, &newtios);
}

int tzkbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

#define tzgetchar getchar

#define PROGMEM
char pgm_read_byte(char *c)
{
    return *c;
}

#endif

char mainmenu[] PROGMEM =
  "\nInit/ReInit\n"
  "I - initialize SD card\n"
  "M - mount FAT32 filesystem from card\n"
  "U - up to root directory\n"
  "Q - quit\n"
  "\nSelect/create file for operations\n"
  "Ofilename.ext - select file or change to directory\n"
  "Cfilename.ext - create (and open) new file\n"
  "N - create/open next serialized xxxxxxxx.LOG\n"
  "C/filename.ext - create (and switch to) directory - \n"
  "L - list: read file as DOS directory\n"
  "filename has editor, but does 8.3 valid names\n"
  "\nSelect/create file for operations\n"
  "R - raw read file (to serial) - any key stops\n"
  "V - view read file (nonascii to dot) - any key stops\n"
  "X - hex read file - any key stops\n"
  "W - write file from stdin, control-c or control-z breaks\n"
  "A - append, seek(0,2) for write\n"
  "F - flush partly written sector\n"
  "E - sync directory filesize Equals written size\n"
  "P - push directory size to end of cluster (crash recov)\n"
  "Z - zap FAT next/free cluster hints\n"
  "T - truncate file to current pos\n"
  "D - delete (opened) file\n" "S - seek (rewind to 0 for now)\n" "G - close file, goto current directory\n";

#include <stdio.h>
#include "fat32.h"
#include "sdhc.h"

#include <string.h>
static u8 filename[12], fn[32];
int dosmunge(u8 * namein, u8 * nameout);

int getfname()
{
    u8 cx, *c;
    u16 len = 0;
    int i;
    memset(fn, ' ', 31);
    fn[31] = 0;
    c = fn;
    for (;;) {
        cx = tzgetchar();
        if (len == 0 && (cx == '\\' || cx == '/'))
            return 1;
        if (cx == '\b' || cx == 127) {
            if( !len )
                continue;
            printf("\b \b");
            *--c = ' ';
            len--;
            continue;
        } else if (cx == '\r') {
            printf("\n");
            break;
        } else if (cx < ' ') {
            printf("\n");
            return -1;
        }
        if (len > 31)
            continue;
        *c++ = cx;
        len++;
        printf("%c", cx);
    }
    if (!len)
        return -1;
    printf("[%s]\r\n", fn);
    i = dosmunge(fn, filename);
    if (i)
        return i;
    printf("[%s]\r\n", filename);
    return 0;
}

u8 dochar(u8 x)
{
    if (x > 126)
        return '.';
    if (x < 32)
        return '.';
    return x;
}

void dumpsect()
{
    u16 cnt;
    printf("\n=");
    for (cnt = 0; cnt < 512; cnt++) {
        if (!(cnt & 31))
            printf("\n");
        printf("%02X", filesectbuf[cnt]);
    }
    printf("\n-");
    for (cnt = 0; cnt < 512; cnt++) {
        if (!(cnt & 31))
            printf("\n");
        printf("%c", dochar(filesectbuf[cnt]));
    }
}

#ifdef __AVR__
int main(void)
#else
int main(int argc, char *argv[])
#endif
{
    u8 sta = 0, cmd;
    int i;

#ifndef __AVR__
    if (argc > 3 || argc < 2) {
        printf("Usage: (sudo) testfat rawdevice  e.g. fat32 /dev/sdb \r\n");
        return 1;
    }
    //    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    if (initsec(argv[1]))
        return -1;
#endif

    hwinit();

    printf("\nFAT32 Test\n");
    i = sdhcinit();
    printf("init %d\n", i);
    if (!i) {
        printf( "%lu sects\n", sdnumsectors );
        i = mount(0);
        printf("Mount: %d\n", i);
        if (!i)
            seekfile(0, 0);
#if 1                           // show what is read
        else
            for (i = 0; i < 1; i++) {
                readsec(i);
                dumpsect();
            }
#endif
    }
    if (i)
        printf("Not Ready\n");

    for (;;) {
        cmd = tzgetchar();
        if (cmd >= 'a')
            cmd -= 32;
        switch (cmd) {
#ifdef __AVR__
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            //            while (!tzkbhit()) {
                ADMUX = 0xe0 + cmd - '0';
                ADCSRA = 0xc7;
                while (ADCSRA & 0x40);
                printf("%c %04x\n", cmd, ADC);
                //            }
            break;
#endif
        case 3:
        case 26:
        case 'Q':
            exit(0);
        case 'Y':
            {
                u16 u;
                u = getdirent((u8 *) "");
                printf("%u entries in dir\n", u);
            }
            break;
        case 'I':
            sta = sdhcinit();
            printf("init %d\n", sta);
            if (sta)
                continue;
            printf( "%lu sects\n", sdnumsectors );
            sta = cardinfo(0);
            printf("inf %d\n", sta);
            if (sta)
                continue;
            for (sta = 0; sta < 18; sta++)
                printf("%02X", filesectbuf[sta]);
            printf(" ");
            for (sta = 0; sta < 18; sta++)
                printf("%c", dochar(filesectbuf[sta]));
            printf("\n");
            sta = cardinfo(1);
            printf("inf %d\n", sta);
            if (sta)
                continue;
            for (sta = 0; sta < 18; sta++)
                printf("%02X", filesectbuf[sta]);
            printf(" ");
            for (sta = 0; sta < 18; sta++)
                printf("%c", dochar(filesectbuf[sta]));
            printf("\n");
            break;
        case 'M':
            i = mount(0);
            if (!i)
                printf("Mounted\n");
            else
                printf("Error\n");
            break;
        case 'O':
            printf("Open:\n");
            i = getfname();
            if (i < 0) {
                printf("bad fname\n");
                break;
            }
            i = getdirent(filename);
            if (i == 2)
                printf("Entered Dir\n");
            else if (!i)
                printf("Opened\n");
            else
                printf("Not Found\n");
            break;
        case 'C':
            printf("Create:\n");
            i = getfname();
            if (i < 0) {
                printf("bad fname\n");
                break;
            }
            if (i == 1) {
                printf("Directory:\n");
                i = getfname();
                if (i) {
                    printf("bad fname\n");
                    break;
                }
                i = 0x10;       // directory attribute
            }
            i = newdirent(filename, i);
            if (!i)
                printf("Created\n");
            else if (i == 2)
                printf("Exists\n");
            else
                printf("Error\n");
            break;
        case 'N':
            i = newdirent(NULL, 0);
            if (!i)
                printf("Created Ser Log\n");
            else
                printf("Error\n");
            break;
        case 'L':
            resettodir();
            seekfile(0, 0);
            for (i = 0; i < 16; i++) {
                if (!filesectbuf[i << 5])
                    break;
                if (0xe5 == filesectbuf[i << 5]) {
                    printf("(del)\n");
                    continue;
                }
                if (0xf == filesectbuf[(i << 5) + 11]) {
                    printf("(lfn)\n");
                    continue;
                }
                printf("%8.8s.%3.3s %c\n", &filesectbuf[i << 5], &filesectbuf[(i << 5) + 8],
                  filesectbuf[(i << 5) + 11] & 0x10 ? '/' : ' ');
                if (i == 15) {
                    i = readnextsect();
                    if (i != 512)
                        break;
                    i = -1;
                }
            }
            break;
        case 'W':
            printf("Write: ^c|^z to exit\n");
            for (;;) {
                char cx;
                cx = tzgetchar();
                if (cx == 3 || cx == 26)
                    break;
                writebyte(cx);
                printf("%c", cx);
            }
            printf("\nWritten ");
            flushbuf();
            printf("Flushed ");
            syncdirent(0);
            printf("DSynced\n");
            break;
        case 'F':
            flushbuf();
            printf("Flushed\n");
            break;
        case 'E':
            syncdirent(0);
            printf("dirsynced\n");
            break;
        case 'P':
            syncdirent(1);
            printf("size pushed\n");
            break;
        case 'Z':
            zaphint();
            printf("hint zapped\n");
            break;
        case 'T':
            truncatefile();
            printf("Trunc-ed\n");
            break;
        case 'D':
            i = deletefile();
            if (i == 1)
                printf("no rmdir\n");
            else if (!i)
                printf("Deleted\n");
            else
                printf("Error\n");
            break;
            // will go away when seek is filled in
        case 'A':
            seekfile(0, 2);
            printf("At EOF\n");
            break;
        case 'S':
            {
                u32 x = 0;
                u8 y = 1;
                seekfile(x, y);
                printf("Rewound\n");
            }
            break;
        case 'R':
        case 'V':
        case 'X':
            sta = 0;
            while (!tzkbhit()) {
                i = readbyte();
                if (i < 0) {
                    printf("\n=EOF=\n");
                    break;
                }
                switch (cmd) {
                case 'R':
                    printf("%c", i);
                    break;
                case 'V':
                    printf("%c", dochar(i));
                    break;
                case 'X':
                    if (!(sta++ & 15))
                        printf("\n");
                    printf("%02X", i);
                    break;
                }
            }
            break;
        case 'U':
            resetrootdir();
            seekfile(0, 0);     // for list
            printf("At rootdir\n");
            break;
        case 'G':
            resettodir();
            seekfile(0, 0);     // for list
            printf("At curdir\n");
            break;

        default:
            // help message
            {
                char *c = mainmenu;
                while (pgm_read_byte(c))
                    putchar(pgm_read_byte(c++));
            }
            break;
        }
    }
}
