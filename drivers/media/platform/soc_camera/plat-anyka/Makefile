# sensor 
obj-$(CONFIG_SENSOR_GC0308)   += camera_gc0308.o 
obj-$(CONFIG_SENSOR_OV2643)   += camera_ov2643.o
obj-$(CONFIG_SENSOR_OV7725)   += camera_ov7725.o
obj-$(CONFIG_SENSOR_HM1375)   += camera_hm1375.o
obj-$(CONFIG_SENSOR_OV9712)   += camera_ov9712.o
obj-$(CONFIG_SENSOR_OV2710)   += camera_ov2710.o
obj-$(CONFIG_SENSOR_AR0130)   += camera_ar0130.o
obj-$(CONFIG_SENSOR_H22)	  += camera_h22.o
obj-$(CONFIG_SENSOR_NT99141)+= camera_nt99141.o

# wrap sensor
obj-$(CONFIG_LINUX_AKSENSOR)   += wrap_sensor.o aksensor.o

# Stack-B sensor drivers register themselves before the aksensor I2C driver is
# added.  Keep every sensor_*.o in this block so probe_sensors() sees the whole
# runtime-detection registry when an existing board-info client probes.
obj-$(CONFIG_SENSOR_GC1084)   += sensor_gc1084.o
obj-$(CONFIG_SENSOR_AR0130_ISP2)	+= sensor_ar0130.o
obj-$(CONFIG_SENSOR_BF3703)	+= sensor_bf3703.o
obj-$(CONFIG_SENSOR_F22_F23_F28_F35_F37)	+= sensor_f22_f23_f28_f35_f37.o
obj-$(CONFIG_SENSOR_GC1054)	+= sensor_gc1054.o
obj-$(CONFIG_SENSOR_GC2053)	+= sensor_gc2053.o
obj-$(CONFIG_SENSOR_H63)	+= sensor_h63.o
obj-$(CONFIG_SENSOR_H65)	+= sensor_h65.o
obj-$(CONFIG_SENSOR_IMX307)	+= sensor_imx307.o
obj-$(CONFIG_SENSOR_PS5260)	+= sensor_ps5260.o
obj-$(CONFIG_SENSOR_SC1235)	+= sensor_sc1235.o
obj-$(CONFIG_SENSOR_SC1245)	+= sensor_sc1245.o
obj-$(CONFIG_SENSOR_SC2232H_SC2135E)	+= sensor_sc2232h_sc2135e.o
obj-$(CONFIG_SENSOR_SC2235)	+= sensor_sc2235.o
obj-$(CONFIG_SENSOR_SC2239_SC2332_SC2335)	+= sensor_sc2239_sc2332_sc2335.o
obj-$(CONFIG_SENSOR_SP2305)	+= sensor_sp2305.o

# ISP2 sensor framework.  This must follow the stack-B sensor drivers above;
# i2c_add_driver() can synchronously probe an already registered client.
obj-$(CONFIG_AK_ISP_CHAR)     += ak_sensor_common.o ak_sensor_i2c.o

# ak-cam-sensor.o under drivers/media/platform/anyka/ is the same subdev with
# the same exports; only one may be linked.
ifneq ($(CONFIG_VIDEO_AK_CAM_SENSOR),y)
obj-$(CONFIG_AK_ISP_CHAR)     += ak_sensor.o
endif
obj-$(CONFIG_AK_ISP_CHAR)     += ispdrv_interface.o

# ISP core. Anyka ships this as a prebuilt libispdrv.a; we build it from the
# SDK sources instead.
obj-$(CONFIG_AK_ISP_CORE)     += ak39_isp2.o ak39_isp2_3a.o

# soc_camera host driver + the ispdrv_* wrapper that decouples it from the
# ak_isp_char.c misc-device module namespace. Neither has any platform-
# specific content, so both live here rather than under an arch/arm
# machine directory.
obj-$(CONFIG_VIDEO_AK)         += ak_camera.o
