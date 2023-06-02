/*
 * Core driver for Fullhan pin controller
 *
 * Copyright (C) 2018 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * Author: Wang Yuliang <wangyl307@fullhan.com>
 *
 */

#ifndef __PINCTRL_FULLHAN_UTILS_H
#define __PINCTRL_FULLHAN_UTILS_H

#include "pinctrl-fullhan.h"

static struct fh_pingroup *fh_pinctrl_find_group_by_name(
			const struct fh_pinctrl_soc_info *info,	const char *name)
{
	struct fh_pingroup *grp = NULL;
	int i;

	for (i = 0; i < info->ngroups; i++) {
		if (!strncmp(info->pin_groups[i].name, name, MAX_NAME_CHARS)) {
			grp = &info->pin_groups[i];
			break;
		}
	}
	return grp;
}

static struct fh_pinmux *fh_pinctrl_find_mux_by_name(const char *name,
			const struct fh_pinctrl_soc_info *info) {
	int i;
	struct fh_pinmux *mux = info->pin_muxs;

	for (i = 0; i < info->nmuxs; i++, mux++) {
		if (!strncmp(name, mux->name, MAX_NAME_CHARS))
			return mux;
	}

	return NULL;
}

static int fh_pinctrl_get_pin_id(const struct fh_pingroup *group,
				unsigned int pin_index)
{
	struct fh_pin *pin;
	struct fh_pinmux *mux;

	if (pin_index >= group->npins)
		return -1001;
	pin = &group->pins[pin_index];
	if (!pin)
		return -1002;
	mux = pin->mux;
	if (!mux)
		return -1003;
	if (pin->mux_sel >= mux->npads) {
		DBG_LINE_ARGS("%s %s %d %d\n",
			pin->name, mux->name, pin->mux_sel, mux->npads);
	 	return -1004;
	}

	return mux->pads[pin->mux_sel]->index;
}

static int fh_pinctrl_find_func_in_pad(const char *func,
				const struct fh_pinpad *pad)
{
	int i = 0;

	if (!func || !pad || !pad->funcs)
		return -EINVAL;
	for (i = 0; i < pad->nfunc; i++) {
		if (strncmp(func, pad->funcs[i], MAX_NAME_CHARS) == 0)
			return i; /* return the index of func in pad*/
	}
	return -3001;
}

static int fh_pinctrl_check_pin_config(const fh_pin_reg *config)
{
	if (!config || !config->dw)
		return -2101;
	if (config->dw & (~0x1F111171)) /* error when other bits was set */
	{
		DBG_LINE_ARGS("%x %x", config->dw, config->dw & (~0x1F111171));
		return -2102;
	}
	if (config->bit.pu && config->bit.pd)
		return -2103;
	return 0;
}

static struct fh_pinctrl_soc_info fh_pinctrl_info;

static int fh_pinctrl_set_pin_pupd(fh_pin_reg *config, int pupd)
{
	if (!config || pupd < 0 || pupd > 2)
		return -2001;
	switch (pupd) {
	case PUPD_NONE:
		config->bit.pu = fh_pinctrl_info.pupd_zero;
		config->bit.pd = fh_pinctrl_info.pupd_zero;
		break;
	case PUPD_UP:
		config->bit.pu = fh_pinctrl_info.pupd_enable;
		config->bit.pd = fh_pinctrl_info.pupd_disable;
		break;
	case PUPD_DOWN:
		config->bit.pu = fh_pinctrl_info.pupd_disable;
		config->bit.pd = fh_pinctrl_info.pupd_enable;
		break;
	default:
		return -2002;
		break;
	}

	return 0;
}

static int fh_pinctrl_set_pin_config(fh_pin_reg *config, int pupd, int ds)
{
	int ret;

	if (!config || pupd < 0 || ds < 0)
		return -2001;
	config->bit.ds = ds;

	ret = fh_pinctrl_set_pin_pupd(config, pupd);
	if (ret)
		return ret;

	/* fix on configs */
	config->bit.st = 1;
	config->bit.ie = 1;

	return fh_pinctrl_check_pin_config(config);
}

static void fh_pinctrl_update_pad_reg(struct fh_pinpad *pad)
{
	int ret = fh_pinctrl_check_pin_config(&pad->config);

	if (ret)
		pr_err("PIN %d config [0x%08x] err: %d\n",
			pad->index, pad->config.dw, ret);

	pad->func_sel = pad->config.bit.mfs;
	SET_REG(pad->reg, pad->config.dw);
}

static int fh_pinctrl_set_gpio_func(struct fh_pinpad *pad) {
	int i;

	if (!pad || !pad->funcs || pad->index < 0)
		return -EINVAL;
	for (i = 0; i < pad->nfunc; i++) {
		if (strncmp("GPIO", pad->funcs[i], 4) == 0) {
			pad->config.bit.mfs = i;
			fh_pinctrl_update_pad_reg(pad);
			pr_info("[PINCTRL] pad %d set to func %s\n",
					pad->index, pad->funcs[i]);
			return i;
		}
	}
	return -1;
}

static int fh_pinctrl_check_duplicate_pin(unsigned int pin_id,
		const struct fh_pinctrl_soc_info *info)
{
	struct fh_pinpad *pad;
	const struct fh_pinpad *p;
	int i;

	if (!info || pin_id >= info->npins)
		return -1;

	pad = &info->pin_pads[pin_id];

	for (i = 0; i < info->npins; i++) {
		p = &info->pin_pads[i];
		if (p->funcs && pad->funcs && p != pad) {
			const char* n1 = p->funcs[p->config.bit.mfs];
			const char* n2 = pad->funcs[pad->config.bit.mfs];
			if (!strncmp(n1, n2, MAX_NAME_CHARS)) {
				pr_err("[PINCTRL] WARNING: %s already exists in pad %d\n",
					n1, p->index);
				fh_pinctrl_set_gpio_func(&info->pin_pads[i]);
				return i;
			}
		}
	}
	return -1;
}

static int fh_pinctrl_check_padlist(const struct fh_pinctrl_soc_info *info)
{
	int i;
	struct fh_pinpad *pad;

	for (i = 0; i < info->npins; i++) {
		pad = &info->pin_pads[i];
		if (pad->index < 0 || pad->func_sel < 0) {
			pr_err("[PINCTRL] ERROR: pad %2d[0x%03x] is still empty\n",
				i, (int)pad->reg & 0xfff);
		}
	}

	return 0;
}

static void fh_pinctrl_print_pin_info(const struct fh_pinpad *pad,
			struct seq_file *s, int show_all)
{
	char *pupd;
	fh_pin_reg config;
	struct fh_pinctrl_soc_info *info = &fh_pinctrl_info;

	if (!pad || pad->index < 0) {
		if (!show_all)
			return;
		seq_printf(s, "N/A");
		return;
	}

	config = pad->config;

	if (config.bit.pu == info->pupd_enable &&
			config.bit.pd == info->pupd_disable)
		pupd = "PU";
	else if (config.bit.pu == info->pupd_disable &&
			config.bit.pd == info->pupd_enable)
		pupd = "PD";
	else if (config.bit.pu == info->pupd_zero &&
			config.bit.pd == info->pupd_zero)
		pupd = "NONE";
	else
		pupd = "ERR";

	seq_printf(s, "%02d\t%8s\t0x%04x\t0x%08x%4d%4d%4d%8s%4d",
			pad->index,
			pad->funcs ? pad->funcs[config.bit.mfs]:"N/A",
			(int)pad->reg & 0xfff,
			GET_REG(pad->reg),
			config.bit.mfs,
			config.bit.ie,
			config.bit.oe,
			pupd,
			config.bit.ds);
}


#endif /* __PINCTRL_FULLHAN_UTILS_H */
