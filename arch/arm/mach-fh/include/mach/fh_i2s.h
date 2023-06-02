#ifndef __FH_I2S_PLATFORM_DATA
#define __FH_I2S_PLATFORM_DATA

struct fh_i2s_platform_data {
	int dma_capture_channel;
	int dma_playback_channel;
	int dma_master;
	char *acodec_clk_name;
	int (*clk_config)(int div_val);
};

#endif

