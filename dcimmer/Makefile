all: dcimmer.hex

.SUFFIXES: .elf .hex .eep .lss .sym .upload .dsm .c .s .S .o .l

CPU=atmega328p
#PORT=/dev/rfcomm0
PORT=/dev/ttyUSB0
#Note: -O will break something, it won't output characters
#XTAL=16000000 #OpenLog
#XTAL=8000000 #Arduino 3.3v Pro
XTAL=14745600 #JPEG Trigger
CFLAGS=-g -Os -Wall -mmcu=$(CPU) -DF_CPU=$(XTAL) -I../fat32lib -I../uart -DRXBUFBITS=9 -DTXBUFBITS=5
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

clean:
	rm -f *.eep *.elf *.lss *.sym *.dsm *.o *.a *.d *.hex

flash: all
	avrdude -c stk500v1 -b 57600 -P /dev/ttyUSB0 -p m328p -U dcimmer.hex

../fat32lib/libfat32.a:
	make -C ../fat32lib

uart.o: ../uart/uart.c
	avr-gcc $(CFLAGS) -Wall -c  $^ -o $@

OBJS := main.o ../fat32lib/sdhc.o uart.o

dcimmer.elf: $(OBJS) ../fat32lib/libfat32.a
	$(CC) $(CFLAGS) -o dcimmer.elf $(OBJS) -L../fat32lib -lfat32
