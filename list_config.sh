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
dir=arch/${ARCH}/configs
if [ "${1}" == "" ]; then echo -e "Usage ${0##*/} [PREFIX] \nPREFIX: 3 5 6 2m"; fi
pushd $dir > /dev/null
echo -e "\nto $dir"
find . -name "infinity${1}*_defconfig"
find . -name "mercury${1}*_defconfig"
find . -name "pioneer${1}*_defconfig"
popd > /dev/null


