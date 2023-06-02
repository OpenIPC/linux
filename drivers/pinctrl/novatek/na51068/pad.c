/*
    PAD controller

    Sets the PAD control of each pin.

    @file       pad.c
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#if defined(__FREERTOS)
/* rtos */
#include <stdio.h>
#include <kwrap/error_no.h>
#include <kwrap/spinlock.h>
#include "pad.h"
#include "top.h"
#include <rcw_macro.h>
#include <io_address.h>

#define __MODULE__    rtos_pad
#include <kwrap/debug.h>
unsigned int rtos_pad_debug_level = NVT_DBG_WRN;

static VK_DEFINE_SPINLOCK(pad_spinlock);
static unsigned long flags;
#define loc_cpu() vk_spin_lock_irqsave(&pad_spinlock, flags)
#define unl_cpu() vk_spin_unlock_irqrestore(&pad_spinlock, flags)

#define PAD_SETREG(ofs,value)        OUTW((IOADDR_PAD_REG_BASE+(ofs)),(value))
#define PAD_GETREG(ofs)              INW(IOADDR_PAD_REG_BASE+(ofs))
#define PAD_DBG(fmt, args...)        DBG_DUMP(fmt, ##args)
#define PAD_DBG_ERR(fmt, args...)    DBG_ERR(fmt, ##args)
#else
/* linux kernel */
#include <linux/spinlock.h>
#include <plat/pad.h>
#include "na51068_pinmux.h"
#include <mach/rcw_macro.h>

static DEFINE_SPINLOCK(pad_lock);
static unsigned long flags;
#define loc_cpu() spin_lock_irqsave(&pad_lock, flags);
#define unl_cpu() spin_unlock_irqrestore(&pad_lock, flags);

void __iomem *pad_vaddr;
#define PAD_SETREG(ofs,value)    OUTW((pad_vaddr+(ofs)),(value))
#define PAD_GETREG(ofs)          INW(pad_vaddr+(ofs))
#define PAD_DBG(fmt, ...)        pr_info(fmt, ##__VA_ARGS__)
#define PAD_DBG_ERR(fmt, ...)    pr_err(fmt, ##__VA_ARGS__)
#endif


#define PAD_PUPD0_REG_OFS           0x00
#define PAD_DS0_REG_OFS             0x80

/**
    Set a pin pull up/down/keeper.

    Set a pin pull up/down/keeper.

    @param[in]  pin Pin name. For example PAD_X_xxxx. See pad.h.
    @param[in]  pulltype PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype)
{
	uint32_t dw_offset, bit_offset;
	REGVALUE reg_data;

	if ((pin >= PAD_PIN_MAX) ||
		((PAD_X_PWM1_OUT < pin) && (pin < PAD_X_GPIO_0)) ||
		((PAD_X_GPIO_9 < pin) && (pin < PAD_X_CAP0_CLK)) ||
		((PAD_X_CAP0_DATA7 < pin) && (pin < PAD_X_CAP1_CLK)) ||
		((PAD_X_CAP1_DATA7 < pin) && (pin < PAD_X_CAP2_CLK)) ||
		((PAD_X_CAP2_DATA7 < pin) && (pin < PAD_X_CAP3_CLK)) ||
		((PAD_X_CAP3_DATA7 < pin) && (pin < PAD_X_EPHY0_REFCLK)) ||
		((PAD_X_RGMII0_MDIO < pin) && (pin < PAD_X_VGA_HS)))
		return E_NOEXS;

	bit_offset = ((pin & 0xF) << 1);
	dw_offset = (pin >> 4);

	loc_cpu();
	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	reg_data &= ~(3 << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	reg_data |= (pulltype << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	unl_cpu();

	return E_OK;
}

/**
    Get a pin pull up/down/keeper

    Get a pin pull up/down/keeper

    @param[in]      pin         Pin name. For example PAD_X_xxxx. See pad.h.
    @param[out]     pulltype    PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype)
{
	uint32_t dw_offset, bit_offset;
	REGVALUE reg_data;

	if ((pin >= PAD_PIN_MAX) ||
		((PAD_X_PWM1_OUT < pin) && (pin < PAD_X_GPIO_0)) ||
		((PAD_X_GPIO_9 < pin) && (pin < PAD_X_CAP0_CLK)) ||
		((PAD_X_CAP0_DATA7 < pin) && (pin < PAD_X_CAP1_CLK)) ||
		((PAD_X_CAP1_DATA7 < pin) && (pin < PAD_X_CAP2_CLK)) ||
		((PAD_X_CAP2_DATA7 < pin) && (pin < PAD_X_CAP3_CLK)) ||
		((PAD_X_CAP3_DATA7 < pin) && (pin < PAD_X_EPHY0_REFCLK)) ||
		((PAD_X_RGMII0_MDIO < pin) && (pin < PAD_X_VGA_HS)))
		return E_NOEXS;

	bit_offset = ((pin & 0xF) << 1);
	dw_offset = (pin >> 4);

	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	*pulltype = ((reg_data >> bit_offset) & 0x3);

	return E_OK;
}

/**
    Set driving/sink.

    Set driving/sink.

    @param[in] name     For example PAD_X_xxxx. See pad.h.
    @param[in] driving  PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_drivingsink(PAD_PIN pin, PAD_DRIVINGSINK driving)
{
	uint32_t dw_offset, bit_offset;
	REGVALUE reg_data;

	if ((pin >= PAD_PIN_MAX) ||
		((PAD_X_PWM1_OUT < pin) && (pin < PAD_X_GPIO_0)) ||
		((PAD_X_GPIO_9 < pin) && (pin < PAD_X_CAP0_CLK)) ||
		((PAD_X_CAP0_DATA7 < pin) && (pin < PAD_X_CAP1_CLK)) ||
		((PAD_X_CAP1_DATA7 < pin) && (pin < PAD_X_CAP2_CLK)) ||
		((PAD_X_CAP2_DATA7 < pin) && (pin < PAD_X_CAP3_CLK)) ||
		((PAD_X_CAP3_DATA7 < pin) && (pin < PAD_X_EPHY0_REFCLK)) ||
		((PAD_X_RGMII0_MDIO < pin) && (pin < PAD_X_VGA_HS)))
		return E_NOEXS;

	bit_offset = ((pin & 0xF) << 1);
	dw_offset = (pin >> 4);

	loc_cpu();
	reg_data = PAD_GETREG(PAD_DS0_REG_OFS + (dw_offset << 2));
	reg_data &= ~(3 << bit_offset);
	reg_data |= (driving << bit_offset);
	PAD_SETREG(PAD_DS0_REG_OFS + (dw_offset << 2), reg_data);
	unl_cpu();

	return E_OK;
}

/**
    Get driving/sink.

    Get driving/sink.

    @param[in]  name        For example PAD_X_xxxx. See pad.h.
    @param[out] driving     PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_drivingsink(PAD_PIN pin, PAD_DRIVINGSINK *driving)
{
	uint32_t dw_offset, bit_offset;
	REGVALUE reg_data;

	if ((pin >= PAD_PIN_MAX) ||
		((PAD_X_PWM1_OUT < pin) && (pin < PAD_X_GPIO_0)) ||
		((PAD_X_GPIO_9 < pin) && (pin < PAD_X_CAP0_CLK)) ||
		((PAD_X_CAP0_DATA7 < pin) && (pin < PAD_X_CAP1_CLK)) ||
		((PAD_X_CAP1_DATA7 < pin) && (pin < PAD_X_CAP2_CLK)) ||
		((PAD_X_CAP2_DATA7 < pin) && (pin < PAD_X_CAP3_CLK)) ||
		((PAD_X_CAP3_DATA7 < pin) && (pin < PAD_X_EPHY0_REFCLK)) ||
		((PAD_X_RGMII0_MDIO < pin) && (pin < PAD_X_VGA_HS)))
		return E_NOEXS;

	bit_offset = ((pin & 0xF) << 1);
	dw_offset = (pin >> 4);

	reg_data = PAD_GETREG(PAD_DS0_REG_OFS + (dw_offset << 2));
	*driving = ((reg_data >> bit_offset) & 0x3);

	return E_OK;
}

#ifndef __FREERTOS
void pad_preset(struct nvt_pinctrl_info *info)
{
	pad_vaddr = info->pad_base;
}
#endif

ER pad_init(struct nvt_pinctrl_info *info, uint32_t nr_pad)
{
#ifndef __FREERTOS
	uint8_t cnt;
	int ret = 0;

	for (cnt = 0; cnt < nr_pad; cnt++) {
		ret = pad_set_pull_updown(info->pad[cnt].pad_gpio_pin, info->pad[cnt].direction);
		if (ret) {
			PAD_DBG_ERR("pad pin 0x%x pull failed!\n", info->pad[cnt].pad_gpio_pin);
			return ret;
		}

		ret = pad_set_drivingsink(info->pad[cnt].pad_ds_pin, info->pad[cnt].driving);
		if (ret) {
			PAD_DBG_ERR("pad pin 0x%x driving failed!\n", info->pad[cnt].pad_ds_pin);
			return ret;
		}
	}
#endif

	return E_OK;
}

#ifndef __FREERTOS
EXPORT_SYMBOL(pad_set_pull_updown);
EXPORT_SYMBOL(pad_get_pull_updown);
EXPORT_SYMBOL(pad_set_drivingsink);
EXPORT_SYMBOL(pad_get_drivingsink);
#endif
