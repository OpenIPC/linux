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

obj-$(CONFIG_LWFS_FS) += lwfs.o

lwfs-y := storage.o super.o

ifneq ($(CONFIG_MMU),y)
lwfs-$(CONFIG_LWFS_ON_MTD) += mmap-nommu.o
endif

