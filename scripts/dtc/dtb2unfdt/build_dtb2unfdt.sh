rm dtb2unfdt

gcc dtb2unfdt.c  ../libfdt/fdt_ro.c ../libfdt/fdt.c ../util.c -o dtb2unfdt  -I.. -I../libfdt -m32

#./dtb2dtst ../../../arch/arm/boot/dts/infinity6-ssc009a-s01a-lh.dtb ../../../arch/arm/boot/unfdt.bin

