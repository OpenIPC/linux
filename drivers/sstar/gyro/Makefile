obj-$(CONFIG_SS_GYRO) += gyro.o
# interface
gyro-y := gyro_core.o
gyro-$(CONFIG_SS_GYRO_SYSFS) += gyro_sysfs.o

# transfer
gyro-$(CONFIG_SS_GYRO_TRANSFER_I2C) += gyro_i2c.o
gyro-$(CONFIG_SS_GYRO_TRANSFER_SPI) += gyro_spi.o

# sensor
gyro-$(CONFIG_SS_GYRO_CHIP_ICG20660) += gyro_sensor_icg20660.o
gyro-$(CONFIG_SS_GYRO_CHIP_ICG20330) += gyro_sensor_icg20330.o
gyro-$(CONFIG_SS_GYRO_CHIP_ICM40607) += gyro_sensor_icm40607.o

ccflags-$(CONFIG_SS_GYRO_DEBUG_ON) := -DSS_GYRO_DEBUG_ON
ccflags-$(CONFIG_SS_GYRO_SYSFS) += -DSS_GYRO_SYSFS

gyro-y += gyro_module.o

EXTRA_CFLAGS      +=  -Idrivers/sstar/include

