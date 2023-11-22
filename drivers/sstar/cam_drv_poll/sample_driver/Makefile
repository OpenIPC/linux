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

obj-m       := pollsample.o
pollsample-objs := src/drv/linux/drv_pollsample_module.o \
                   src/drv/linux/drv_pollsample_export.o \
                   src/drv/common/drv_pollsample_dev.o

export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KERNEL_PATH = /home/david.tsai/I2/linux-3.18
CC  = $(CROSS_COMPILE)gcc
EXTRA_CFLAGS += -I$(KERNEL_PATH)/drivers/sstar/cam_drv_poll/sample_driver/pub/ \
                -I$(KERNEL_PATH)/drivers/sstar/cam_drv_poll/sample_driver/inc \
                -I$(KERNEL_PATH)/drivers/sstar/include/

all:
	make modules -C $(KERNEL_PATH)  M=`pwd`
clean:
	make modules clean -C $(KERNEL_PATH)  M=`pwd`
