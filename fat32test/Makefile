all: setforlinux testfat

CFLAGS=-g -O2 -Wall $(DFLAGS) -I../fat32lib -I.
CC=gcc

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

MAP=-Wl,"-Map" -Wl,"$@.map" -Wl,"--cref"

setforlinux:
	make -f Makefile clean
	touch setforlinux
	rm -f setforavr

clean:
	rm -f *.hex *.eep *.elf *.lss *.sym *.dsm *.o *.a *.d

linuxmain.o: fat32.h sdhc.h

testfat: main.o dosmunge.o sdhcemu.o
	$(CC) -o $@ dosmunge.o main.o sdhcemu.o -L../fat32lib/ -llinfat32
