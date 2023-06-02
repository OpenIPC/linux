#if defined(__KERNEL__)
#include<linux/module.h>
#include "dai.h"


EXPORT_SYMBOL(dai_enableclk);
EXPORT_SYMBOL(dai_disableclk);
EXPORT_SYMBOL(dai_setclkrate);
EXPORT_SYMBOL(dai_isr);
EXPORT_SYMBOL(dai_lock);
EXPORT_SYMBOL(dai_unlock);
EXPORT_SYMBOL(dai_wait_interrupt);
EXPORT_SYMBOL(dai_select_pinmux);
EXPORT_SYMBOL(dai_select_mclk_pinmux);
EXPORT_SYMBOL(dai_open);
EXPORT_SYMBOL(dai_close);
EXPORT_SYMBOL(dai_set_config);
EXPORT_SYMBOL(dai_get_config);
EXPORT_SYMBOL(dai_set_i2s_config);
EXPORT_SYMBOL(dai_get_i2s_config);
EXPORT_SYMBOL(dai_set_tx_config);
EXPORT_SYMBOL(dai_get_tx_config);
EXPORT_SYMBOL(dai_set_txlb_config);
EXPORT_SYMBOL(dai_get_txlb_config);
EXPORT_SYMBOL(dai_set_rx_config);
EXPORT_SYMBOL(dai_get_rx_config);
EXPORT_SYMBOL(dai_enable_tx_dma);
EXPORT_SYMBOL(dai_enable_rx_dma);
EXPORT_SYMBOL(dai_enable_txlb_dma);
EXPORT_SYMBOL(dai_enable_tx);
EXPORT_SYMBOL(dai_enable_rx);
EXPORT_SYMBOL(dai_enable_txlb);
EXPORT_SYMBOL(dai_enable_dai);
EXPORT_SYMBOL(dai_is_dai_enable);
EXPORT_SYMBOL(dai_is_tx_enable);
EXPORT_SYMBOL(dai_is_rx_enable);
EXPORT_SYMBOL(dai_is_txlb_enable);
EXPORT_SYMBOL(dai_is_txrx_enable);
EXPORT_SYMBOL(dai_is_tx_dma_enable);
EXPORT_SYMBOL(dai_is_rx_dma_enable);
EXPORT_SYMBOL(dai_is_txlb_dma_enable);
EXPORT_SYMBOL(dai_is_dma_enable);
EXPORT_SYMBOL(dai_set_tx_dma_para);
EXPORT_SYMBOL(dai_set_rx_dma_para);
EXPORT_SYMBOL(dai_set_txlb_dma_para);
EXPORT_SYMBOL(dai_get_tx_dma_para);
EXPORT_SYMBOL(dai_get_rx_dma_para);
EXPORT_SYMBOL(dai_get_txlb_dma_para);
EXPORT_SYMBOL(dai_get_tx_dma_curaddr);
EXPORT_SYMBOL(dai_get_rx_dma_curaddr);
EXPORT_SYMBOL(dai_get_txlb_dma_curaddr);
EXPORT_SYMBOL(dai_clr_flg);
EXPORT_SYMBOL(dai_debug);
EXPORT_SYMBOL(dai_debug_eac);
EXPORT_SYMBOL(dai_set_debug_dma_para);
EXPORT_SYMBOL(dai_get_debug_status);
EXPORT_SYMBOL(dai_clr_debug_status);



#endif
