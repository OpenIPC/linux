#!/bin/sh

KERNEL_ROOT_DIR=../../..
KERNEL_CONFIG_DIR=../../../arch/arm/configs
KERNEL_DTS_DIR=../../../arch/arm/boot/dts

for i in $@;do
    echo ">>>>>> Handle $i:"
    find $KERNEL_ROOT_DIR -type d -iname $i | xargs rm -rfv
    find $KERNEL_CONFIG_DIR -type f -iname "$i""_*defconfig" | xargs rm -rfv
    find $KERNEL_DTS_DIR -type f -regex ".*$i\(\.\|-\).*\(dts\|dtsi\|dtb\)" | xargs rm -rfv
    echo "<<<<<<"
done


