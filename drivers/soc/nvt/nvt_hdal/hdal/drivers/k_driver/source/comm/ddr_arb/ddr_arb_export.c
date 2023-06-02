#include<linux/module.h>
#include "comm/ddr_arb.h"

EXPORT_SYMBOL(arb_init);
EXPORT_SYMBOL(arb_set_priority);
EXPORT_SYMBOL(arb_enable_wp);
EXPORT_SYMBOL(arb_disable_wp);
EXPORT_SYMBOL(dma_set_channel_outstanding);
EXPORT_SYMBOL(dma_get_channel_outstanding);
EXPORT_SYMBOL(arb_chksum);

EXPORT_SYMBOL(ddr_data_monitor_start);
EXPORT_SYMBOL(ddr_data_monitor_stop);
EXPORT_SYMBOL(ddr_data_monitor_reset);
EXPORT_SYMBOL(ddr_data_monitor_get);
EXPORT_SYMBOL(mau_ch_mon_start);
EXPORT_SYMBOL(mau_ch_mon_stop);


