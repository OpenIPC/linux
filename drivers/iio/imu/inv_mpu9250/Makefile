#
# Makefile for Invensense MPU9250 device.
#

obj-$(CONFIG_INV_MPU9250_IIO) += inv-mpu9250.o
inv-mpu9250-objs := inv_mpu_core.o inv_mpu_ring.o inv_mpu_trigger.o

obj-$(CONFIG_INV_MPU9250_I2C) += inv-mpu9250-i2c.o
inv-mpu9250-i2c-objs := inv_mpu_i2c.o inv_mpu_acpi.o

obj-$(CONFIG_INV_MPU9250_SPI) += inv-mpu9250-spi.o
inv-mpu9250-spi-objs := inv_mpu_spi.o inv_mpu_i2cmst.o