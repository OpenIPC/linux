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

subdir-ccflags-y := -DSSTAR_CODING_STYLE_SANITIZE_MODULE
obj-$(CONFIG_SS_GYRO) += gyro.o
# interface
gyro-y := gyro_core.o
gyro-$(CONFIG_SS_GYRO_SYSFS) += gyro_sysfs.o
gyro-$(CONFIG_SS_GYRO_CDEV) += gyro_ioctl.o

# transfer
gyro-$(CONFIG_SS_GYRO_TRANSFER_I2C) += gyro_i2c.o
gyro-$(CONFIG_SS_GYRO_TRANSFER_SPI) += gyro_spi.o

# sensor
gyro-$(CONFIG_SS_GYRO_CHIP_ICG20660) += gyro_sensor_icg20660.o
gyro-$(CONFIG_SS_GYRO_CHIP_ICM40607) += gyro_sensor_icm40607.o

ccflags-$(CONFIG_SS_GYRO_DEBUG_ON) := -DSS_GYRO_DEBUG_ON
ccflags-$(CONFIG_SS_GYRO_SYSFS) += -DSS_GYRO_SYSFS
ccflags-$(CONFIG_SS_GYRO_CDEV) += -DSS_GYRO_CDEV

gyro-y += gyro_module.o
