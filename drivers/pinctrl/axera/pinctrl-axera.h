/*
 * Copyright (C) 2022 AXERA Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PINCTRL_AXERA_H__
#define __PINCTRL_AXERA_H__

#undef pr_fmt
#define pr_fmt(fmt) "pinctrl: %s-%d " fmt, __func__, __LINE__

/**
 * struct axera_mux_desc - hardware mux descriptor
 * @name: mux function name
 * @muxval: mux register bit value
 */
struct axera_mux_desc {
	const char *name;
	u8 muxval;
};

/**
 * struct axera_pin_data - hardware per-pin data
 * @offset: register offset pinmux controller
 */
struct axera_pin_data {
	u32 offset;
	u32 num;
	struct axera_mux_desc *muxes;
};

struct axera_pinctrl_soc_info {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
};

#define AX_PINCTRL_PIN(pin, off, nums, ...) {		\
	.number = pin,					\
	.name = #pin,					\
	.drv_data = &(struct axera_pin_data) {		\
		.offset = off,				\
		.num = nums,				\
		.muxes = (struct axera_mux_desc[]) {	\
			 __VA_ARGS__, { } },		\
	},						\
}

#define AX_PINCTRL_MUX(_val, _name) {			\
	.name = _name,					\
	.muxval = _val,					\
}

int axera_pinctrl_init(struct platform_device *pdev, struct axera_pinctrl_soc_info *info);

#endif /* __PINCTRL_AXERA_H */
