CFLAGS = -Wall -O2 -ffunction-sections -fdata-sections -fno-exceptions -fno-builtin -march=armv4 -msoft-float
LDFLAGS = -nostdlib -Bstatic -T boot.lds -Ttext 0
TARGET =start.bin
START=start.o
SOURCE=start.S

AS = /usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-as
	  
all: clean start.bin

start.bin : start.o
	/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-ld $(LDFLAGS) $(START) -o start
	/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-objcopy -O binary start start8210.bin
    
start.o:
	/usr/src/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/usr/bin/arm-unknown-linux-uclibcgnueabi-gcc $(CFLAGS) $(SOURCE) -c -o $(START)

clean:
	rm -f *.o *.bin start
