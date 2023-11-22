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

# connect to remote JLinkGDBServer
target  remote gdbserver:2331
# load symobl
file vmlinux
#symbol-file
#add-symbol-file vmlinux 0x23f26000
#add-symbol-file vmlinux 0x23008000
