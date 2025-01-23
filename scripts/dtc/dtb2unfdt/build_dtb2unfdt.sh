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

#!/bin/bash

rm dtb2unfdt

gcc dtb2unfdt.c  ../libfdt/fdt_ro.c ../libfdt/fdt.c ../util.c -o dtb2unfdt  -I.. -I../libfdt -m32

#./dtb2dtst ../../../arch/arm/boot/dts/infinity6-ssc009a-s01a-lh.dtb ../../../arch/arm/boot/unfdt.bin

