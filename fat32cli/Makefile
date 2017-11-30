all: ../fat32lib/libfat32.a fat32cli.hex

.SUFFIXES: .elf .hex .eep .lss .sym .upload .dsm .c .s .S .o .l

CPU=atmega328p
#PORT=/dev/rfcomm0
PORT=/dev/ttyUSB0
#Note: -O will break something, it won't output characters
XTAL=16000000 #OpenLog
#XTAL=8000000 #Arduino 3.3v Pro
#XTAL=14745600 #JPEG Trigger
CFLAGS=-g -Os -Wall -mmcu=$(CPU) -DF_CPU=$(XTAL) -I../uart/ -I../fat32lib/ -DRXBUFBITS=6 -DTXBUFBITS=6
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
	rm -f *.hex *.eep *.elf *.lss *.sym *.dsm *.o *.a *.d *.hex

flash: all
	avrdude -c arduino -P /dev/ttyUSB0 -p m328p -U fat32cli.hex

#avrdude -c stk500v1 -b 57600 -P /dev/ttyUSB0 -p m328p -U fat32cli.hex

../fat32lib/libfat32.a:
	make -C ../fat32lib

uart.o: ../uart/uart.c
	avr-gcc $(CFLAGS) -Wall -c  $^ -o $@

OBJS := main.o ../fat32lib/sdhc.o dosmunge.o uart.o

fat32cli.elf: $(OBJS) ../fat32lib/libfat32.a
	$(CC) $(CFLAGS) -o fat32cli.elf $(OBJS) -L../fat32lib -lfat32
