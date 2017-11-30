all: newhaven.hex hmc5843.hex scp1000-d11.hex adxl345.hex tmp102.hex mcp4725.hex 9dof.hex nh100160.hex

.SUFFIXES: .elf .hex .eep .lss .sym .upload .dsm

CPU=atmega328p
#PORT=/dev/rfcomm0
PORT=/dev/ttyUSB0
#Note: -O will break something, it won't output characters

#XTAL=16000000 #OpenLog
XTAL=8000000 #Arduino 3.3v Pro
#XTAL=14745600 #JPEG Trigger
CFLAGS=-g -DF_CPU=$(XTAL) -Os -I ../uart

.elf.dsm:
	avr-objdump --disassemble -S $^ >$@

.c.s:
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU) -S $^ -o $@

MAP=-Wl,"-Map" -Wl,"$@.map" -Wl,"--cref"

.c.o:
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@ -c

#.c.elf:
#	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@
#$(MAP)
#	avr-size $@

.elf.hex:
	avr-objcopy -O ihex -R .eeprom $< $@

.elf.eep:
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 --no-change-warnings -O ihex $^ $@

.hex.upload: 
	avrdude -F -p $(CPU) -P $(PORT) -c arduino -b 57600 -U $^

.eep.upload: 
	avrdude -V -F -p $(CPU) -P $(PORT) -c stk500v1 -b 57600 -U eeprom:w:$^

uart.o: ../uart/uart.c
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU) -c  $^ -o $@

tmp102.elf: tmp102.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

newhaven.elf: newhaven.o twi2c.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

scp1000-d11.elf: scp1000-d11.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

adxl345.elf: adxl345.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

hmc5843.elf: hmc5843.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

mcp4725.elf: mcp4725.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

9dof.elf: 9dof.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

nh100160.elf: nh100160.o twi2c.o uart.o
	avr-gcc $(CFLAGS) -Wall -mmcu=$(CPU)  $^ -o $@

clean:
	rm -f *.hex *.eep *.elf *.lss *.sym *.dsm *.o
