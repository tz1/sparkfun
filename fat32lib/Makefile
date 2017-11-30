all: setforavr libfat32.a sdhc.o

.SUFFIXES: .elf .hex .eep .lss .sym .upload .dsm .c .s .S .o .l

CPU=atmega328p
#PORT=/dev/rfcomm0
PORT=/dev/ttyUSB0
#Note: -O will break something, it won't output characters
CFLAGS=-g -Os -Wall -mmcu=$(CPU)
CC=avr-gcc

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.S.o:
	$(CC) $(CFLAGS) -c $< -o $@

.c.s:
	$(CC) $(CFLAGS) -S $< -o $@

.elf.dsm:
	avr-objdump --disassemble -S $^ >$@

MAP=-Wl,"-Map" -Wl,"$@.map" -Wl,"--cref"

.elf.hex:
	avr-objcopy -O ihex -R .eeprom $^ $@

setforavr:
	make -f Makefile.linux clean
	touch setforavr
	rm -f setforlinux

clean:
	rm -f *.hex *.eep *.elf *.lss *.sym *.dsm *.o *.d

mount.o fat32data.o getdirent.o creat.o deltrunc.o write.o read.o seek.o: fat32.h fat32private.h

sdhc.o: sdhc.h

libfat32.a: mount.o fat32data.o getdirent.o creat.o deltrunc.o write.o read.o seek.o format.o
	avr-ar r $@ $^
