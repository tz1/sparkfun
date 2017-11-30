all: booter.hex

.SUFFIXES: .elf .hex .eep .lss .sym .upload .dsm .c .s .S .o .l

CPU=atmega328p
#PORT=/dev/rfcomm0
PORT=/dev/ttyUSB0
#Note: -O will break something, it won't output characters
CFLAGS=-g -Os -Wall -mmcu=$(CPU) -DF_CPU=16000000 -combine -fwhole-program -fno-inline-small-functions -fmerge-constants -fmerge-all-constants -funsigned-char -fpack-struct -fshort-enums
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
	rm -f *.hex *.eep *.elf *.lss *.sym *.dsm *.o *.a *.d *.map

flash: all
	avrdude -c stk500v1 -b 57600 -P /dev/ttyUSB0 -p m328p -U booter.hex

booter.elf: booter.o
	$(CC) $(CFLAGS) -o $@ $^ $(MAP) 
