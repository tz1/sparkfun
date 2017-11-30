all: fat32lib booter dcimmer fat32cli hyperlog i2cperiph sramlog
	make -C fat32lib 
	make -C booter 
	make -C dcimmer 
	make -C fat32cli 
	make -C hyperlog 
	make -C i2cperiph 
	make -C sramlog

fat32lib:
	make -C fat32lib all

booter:
	make -C booter

dcimmer: fat32lib
	make -C dcimmer

fat32cli: fat32lib
	make -C fat32cli

hyperlog: fat32lib
	make -C hyperlog

i2cperiph: fat32lib
	make -C i2cperiph

sramlog: fat32lib
	make -C sramlog

clean:
	make -C fat32lib clean
	make -C booter clean
	make -C dcimmer clean
	make -C fat32cli clean
	make -C hyperlog clean
	make -C i2cperiph clean
	make -C fat32lib clean
	make -C sramlog clean
