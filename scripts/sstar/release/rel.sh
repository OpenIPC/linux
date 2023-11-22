#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

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
    if [ -f "$i.blacklist" ]; then
        echo ">>>>>> Handle $i.blacklist:"
        while read -r line
        do
            echo "  remove $KERNEL_ROOT_DIR/$line"
            rm -rf "$KERNEL_ROOT_DIR/$line"
        done < $i.blacklist
        echo "<<<<<<"
    fi
done

# clear platform specify release blacklist file
find  -iname '*.blacklist' -exec rm {} \;


