#ifndef __FH_I2S_PLAT_H__
#define __FH_I2S_PLAT_H__

struct fh_i2s_platform_data {
	int dma_capture_channel;
	int dma_playback_channel;
	int dma_master;
	int dma_rx_hs_num;
	int dma_tx_hs_num;
	char *clk;
	char *pclk;
	char *acodec_pclk;
	char *acodec_mclk;
};

#endif
