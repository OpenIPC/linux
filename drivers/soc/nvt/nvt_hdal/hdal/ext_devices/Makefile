NVT_SENSOR_PATH=$(NVT_HDAL_DIR)/ext_devices/sensor
NVT_AD_PATH=$(NVT_HDAL_DIR)/ext_devices/ad
NVT_PANEL_PATH=$(NVT_HDAL_DIR)/ext_devices/panel
NVT_MOTOR_PATH=$(NVT_HDAL_DIR)/ext_devices/motor
NVT_AUDIO_PATH=$(NVT_HDAL_DIR)/ext_devices/audio

all:
	@$(MAKE) -C $(NVT_SENSOR_PATH)
	@$(MAKE) -C $(NVT_AD_PATH)
	@$(MAKE) -C $(NVT_PANEL_PATH)
	@$(MAKE) -C $(NVT_MOTOR_PATH)
	@$(MAKE) -C $(NVT_AUDIO_PATH)

modules_install:
	@$(MAKE) -C $(NVT_SENSOR_PATH) modules_install
	@$(MAKE) -C $(NVT_AD_PATH) modules_install
	@$(MAKE) -C $(NVT_PANEL_PATH) modules_install
	@$(MAKE) -C $(NVT_MOTOR_PATH) modules_install
	@$(MAKE) -C $(NVT_AUDIO_PATH) modules_install

clean:
	@$(MAKE) -C $(NVT_SENSOR_PATH) clean
	@$(MAKE) -C $(NVT_AD_PATH) clean
	@$(MAKE) -C $(NVT_PANEL_PATH) clean
	@$(MAKE) -C $(NVT_MOTOR_PATH) clean
	@$(MAKE) -C $(NVT_AUDIO_PATH) clean
