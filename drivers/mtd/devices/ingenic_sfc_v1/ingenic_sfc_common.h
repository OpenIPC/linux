#ifndef ingenic_SFC_COMMON_H
#define ingenic_SFC_COMMON_H
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include "sfc.h"
#include "sfc_flash.h"


void dump_sfc_reg(struct sfc *sfc);

void sfc_list_init(struct sfc_transfer *);
void sfc_list_add_tail(struct sfc_transfer *, struct sfc_transfer *);
int32_t sfc_sync(struct sfc *, struct sfc_transfer *);
int32_t sfc_sync_poll(struct sfc *, struct sfc_transfer *);
struct sfc *sfc_res_init(struct platform_device *);
void sfc_res_deinit(struct sfc *sfc);
uint32_t sfc_get_sta_rt(struct sfc *);

int32_t set_flash_timing(struct sfc *, uint32_t, uint32_t, uint32_t, uint32_t);

int32_t sfc_nor_get_special_ops(struct sfc_flash *);

#endif


