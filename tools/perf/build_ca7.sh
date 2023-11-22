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
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ WERROR=0 JOBS=8
arm-linux-gnueabihf-strip --strip-unneeded perf
