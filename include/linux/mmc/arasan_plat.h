/*
 *
 * include/linux/mmc/arsan_plat.h
 *
 * platform data for the Arasan MMC/SD/SDI HC driver
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */

#ifndef __ARASAN_PLAT_H__
#define __ARASAN_PLAT_H__

typedef void (*func_t)(void);
struct arasan_platform_data {
	unsigned int need_poll;
	unsigned int need_detect;
	unsigned int use_pio;

	unsigned int card_irq;   /* sd卡不能处理card中断 */
	unsigned int auto_cmd12; /* 多块传输时, sdio不能自动发送cmd12
				    而sd则需要自动发送cmd12;如果sdio自动
				    发送cmd12,则wifi不会响应，引起响应超时
				  */
	unsigned int sdio_4bit_data;

	func_t p_powerup;

};

static inline int arasan_claim_resource(struct platform_device *pdev)
{
	return 0;
}

#endif
