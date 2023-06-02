/*
 * Core driver for Fullhan pin controller
 *
 * Copyright (C) 2018 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * Author: Wang Yuliang <wangyl307@fullhan.com>
 *
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <mach/pmu.h>

#include "core.h"
#include "pinctrl-fullhan.h"
#include "pinctrl-fullhan-utils.h"



static int fh_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	return info->ngroups;
}

static const char *fh_get_group_name(struct pinctrl_dev *pctldev,
					   unsigned selector)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	return info->pin_groups[selector].name;
}

static int fh_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector,
				   const unsigned **pins,
				   unsigned *npins)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->pin_groups[selector].pin_ids;
	*npins = info->pin_groups[selector].npins;

	return 0;
}

static void fh_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
		   unsigned offset)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;

	if (info)
		fh_pinctrl_print_pin_info(&info->pin_pads[offset], s, 1);
}

static int fh_dt_node_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map **map, unsigned *num_maps)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	const struct fh_pingroup *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num = 1;

	/*
	 * first find the group of this node and check if we need create
	 * config maps for pins
	 */
	grp = fh_pinctrl_find_group_by_name(info, np->name);
	if (!grp) {
		dev_err(info->dev, "unable to find group for node %s\n",
			np->name);
		return -EINVAL;
	}

	PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, new_map, map_num);

	*map = new_map;
	*num_maps = map_num;


	parent = of_get_parent(np);
	if (!parent) {
		kfree(new_map);
		return -EINVAL;
	}
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = grp->name;
	of_node_put(parent);

	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		new_map->data.mux.function, new_map->data.mux.group, map_num);

	return 0;
}

static void fh_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;

	PINCTRL_DEVM_KFREE(info->dev, map);
}

static const struct pinctrl_ops fh_pinctrl_ops = {
	.get_groups_count = fh_get_groups_count,
	.get_group_name = fh_get_group_name,
	.get_group_pins = fh_get_group_pins,
	.pin_dbg_show = fh_pin_dbg_show,
	.dt_node_to_map = fh_dt_node_to_map,
	.dt_free_map = fh_dt_free_map,
};


static int fh_pmx_set_group(const struct fh_pingroup *grp,
			const struct fh_pinctrl_soc_info *info)
{
	struct fh_pinpad *pad;
	int i;

	if (!grp || grp->npins == 0)
		return -EINVAL;

	for (i = 0; i < grp->npins; i++) {
		const struct fh_pin *pin = &grp->pins[i];

		if (pin->mux_sel >= pin->mux->npads) {
			pr_err("[PINCTRL] set mux %s error, muxsel out of range\n",
				pin->name);
			continue;
		}
		pad = pin->mux->pads[pin->mux_sel];
		pad->config = pin->config;
		if (pin->mux->ds >= 0)
			pad->config.bit.ds = pin->mux->ds;
		pin->mux->select = pin->mux_sel;

		fh_pinctrl_check_duplicate_pin(pad->index, info);
		fh_pinctrl_update_pad_reg(pad);

		dev_dbg(info->dev, "write pin reg: id %2d, offset 0x%03x, "
				"val 0x%08x, %s\n",
			pad->index, (int)pad->reg&0xfff, pad->config.dw, pin->name);
	}

#if defined(CONFIG_ARCH_FH885xV200) || \
	defined(CONFIG_ARCH_FH865x)
	if (strncmp(grp->name, "SD1", 3) == 0) {
		/* set SD1_FUNC_SEL after all mux switch */
		fh_pmu_set_sdc1_funcsel(grp->pins[0].mux_sel);
	}
#endif

	return 0;
}

static int fh_pmx_set(struct pinctrl_dev *pctldev, unsigned selector,
			   unsigned group)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	struct fh_pingroup *grp;

	grp = &info->pin_groups[group];

	return fh_pmx_set_group(grp, info);
}

static int fh_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	return info->nfunctions;
}

static const char *fh_pmx_get_func_name(struct pinctrl_dev *pctldev,
					  unsigned selector)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	return info->functions[selector].name;
}

static int fh_pmx_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
				   const char * const **groups,
				   unsigned * const num_groups)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	*groups = info->functions[selector].groups;
	*num_groups = info->functions[selector].num_groups;

	return 0;
}


static const struct pinmux_ops fh_pmx_ops = {
	.get_functions_count = fh_pmx_get_funcs_count,
	.get_function_name = fh_pmx_get_func_name,
	.get_function_groups = fh_pmx_get_groups,
	.set_mux = fh_pmx_set,
};

#ifdef CONFIG_PINCONF

static int fh_pinconf_get(struct pinctrl_dev *pctldev,
				 unsigned pin_id, unsigned long *config)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	const fh_pin_reg pin_config = info->pin_pads[pin_id].config;

	*config = pin_config.dw & PINCONFIG_MASK;

	return 0;
}

static int fh_pinconf_set(struct pinctrl_dev *pctldev,
				 unsigned pin_id, unsigned long *configs,
				 unsigned num_configs)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	struct fh_pinpad * const pad = &info->pin_pads[pin_id];
	int i;

	dev_dbg(fhpctl->dev, "pinconf set pin %s\n",
		info->pins[pin_id].name);

	for (i = 0; i < num_configs; i++) {
		unsigned int val;
		val = GET_REG(pad->reg);
		val &= ~PINCONFIG_MASK;
		val |= configs[i] & PINCONFIG_MASK;
		pad->config.dw = val;
		fh_pinctrl_update_pad_reg(pad);
		dev_dbg(fhpctl->dev, "write: offset 0x%x val 0x%lx\n",
			(int)pad->reg&0xfff, configs[i]);
	}

	return 0;
}

static void fh_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				   struct seq_file *s, unsigned pin_id)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;

	if (info)
		fh_pinctrl_print_pin_info(&info->pin_pads[pin_id], s, 1);
}

static void fh_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
					 struct seq_file *s, unsigned group)
{
	struct fh_pinctrl *fhpctl = pinctrl_dev_get_drvdata(pctldev);
	const struct fh_pinctrl_soc_info *info = fhpctl->info;
	struct fh_pingroup *grp;
	int i;

	if (group >= info->ngroups)
		return;

	seq_printf(s, "\n");
	grp = &info->pin_groups[group];
	for (i = 0; i < grp->npins; i++) {
		int pin_id = fh_pinctrl_get_pin_id(grp, i);
		fh_pinctrl_print_pin_info(&info->pin_pads[pin_id], s, 1);
		seq_printf(s, "\n");
	}
}

static const struct pinconf_ops fh_pinconf_ops = {
	.pin_config_get = fh_pinconf_get,
	.pin_config_set = fh_pinconf_set,
	.pin_config_dbg_show = fh_pinconf_dbg_show,
	.pin_config_group_dbg_show = fh_pinconf_group_dbg_show,
};
#endif

/*
 * Check if the DT contains pins in the direct child nodes. This indicates the
 * newer DT format to store pins. This function returns true if the first found
 * fh,pins property is in a child of np. Otherwise false is returned.
 */
static bool fh_pinctrl_dt_is_flat_functions(struct device_node *np)
{
	struct device_node *function_np;
	struct device_node *pinctrl_np;

	for_each_child_of_node(np, function_np) {
		if (of_property_read_bool(function_np, "fh,pins"))
			return true;

		for_each_child_of_node(function_np, pinctrl_np) {
			if (of_property_read_bool(pinctrl_np, "fh,pins"))
				return false;
		}
	}

	return true;
}

static int fh_pinctrl_parse_pinpads(struct device_node *np,
					struct fh_pinctrl_soc_info *info)
{
	struct device_node *child;
	struct fh_pinpad *pad;
	const char **funcs;
	unsigned int npads, index;
	int nfunc, pupd, ds;
	int ret = 0;

	npads = of_get_child_count(np);

	dev_info(info->dev, "npads: %d", npads);

	for_each_child_of_node(np, child)
	{
		PINCTRL_PARSE_PROPERTY_U32(child, "index", index);
		if(index >= info->npins) {
			dev_err(info->dev, "index %d of pad %s out of range",
					index, child->full_name);
			return -EINVAL;
		}
		pad = &info->pin_pads[index];
		pad->index = index;
		pad->name = child->name;
		pad->reg = info->pin_regs[index];

		PINCTRL_PARSE_PROPERTY_U32(child, "pupd", pupd);
		PINCTRL_PARSE_PROPERTY_U32(child, "ds", ds);

		pad->config.dw = GET_REG(pad->reg);
		ret = fh_pinctrl_set_pin_config(&pad->config, pupd, ds);
		if (ret) {
			dev_err(info->dev, "error config pupd:%d ds:%d, ret: %d\n",
					pupd, ds, ret);
			return ret;
		}

		/* get nfunc first */
		nfunc = of_property_count_strings(child, "funcs");
		if (nfunc <= 0 || nfunc > info->max_mux) {
			dev_err(info->dev, "wrong nfunc: %d in %s\n",
					nfunc, child->full_name);
			return -EINVAL;
		}
		PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, funcs, nfunc);

		of_property_read_string_array(child, "funcs", funcs, nfunc);
		pad->nfunc = nfunc;
		pad->funcs = funcs;
		dev_dbg(info->dev, "get %2d func in pad %2d [func0: %s]",
				pad->nfunc, pad->index, pad->funcs[0]);
	}

	return 0;
}

static int fh_pinctrl_parse_pinmuxs(struct device_node *np,
					struct fh_pinctrl_soc_info *info)
{
	struct device_node *child, *np_pad;
	struct fh_pinmux *mux;
	unsigned int nmuxs, npads, pad_index;
	unsigned mux_id = 0;
	int i = 0, j = 0;

	nmuxs = of_get_child_count(np);
	dev_dbg(info->dev, "nmuxs: %d", nmuxs);

	PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, info->pin_muxs, nmuxs);
	info->nmuxs = nmuxs;

	for_each_child_of_node(np, child)
	{
		mux = &info->pin_muxs[i++];
		mux->name = child->name;
		mux->mux_id = mux_id++;
		mux->ds = -1;
		PINCTRL_PARSE_PROPERTY_U32(child, "select", mux->select);
		of_property_read_u32(child, "ds", &mux->ds);
		npads = of_count_phandle_with_args(child, "fh,pads", NULL);
		if (IS_ERR_VALUE(npads))
			return npads;
		mux->npads = npads;

		PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, mux->pads, npads);

		dev_dbg(info->dev, "get %d pad in mux %s, sel: %d\n",
				mux->npads, mux->name, mux->select);

		for (j = 0; j < npads; j++) {
			np_pad = of_parse_phandle(child, "fh,pads", j);
			if (!np_pad)
				return -EINVAL;
			PINCTRL_PARSE_PROPERTY_U32(np_pad, "index", pad_index);
			mux->pads[j] = &info->pin_pads[pad_index];
			if (fh_pinctrl_find_func_in_pad(mux->name, mux->pads[j]) < 0) {
				return -ENOENT;
			}
			dev_dbg(info->dev, "\tpad %d, \n", mux->pads[j]->index);
		}
	}
	return 0;
}

static int fh_pinctrl_parse_pingroups(struct device_node *np,
					struct fh_pinctrl_soc_info *info)
{
	static const char *cell_name = "#list-cells";
	struct device_node *child;
	struct fh_pingroup *group;
	struct fh_pmx_func *func;
	struct fh_pin *pin;
	struct fh_pinpad *pad;
	struct fh_pinmux *mux;
	struct of_phandle_args args;
	unsigned int npins;
	int pin_id, func_idx;
	int i = 0, j = 0;

	info->ngroups = of_get_child_count(np);
	dev_dbg(info->dev, "ngroups: %d", info->ngroups);

	PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, info->pin_groups, info->ngroups);

	func = &info->functions[0];
	func->name = np->name;
	func->num_groups = info->ngroups;
	PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, func->groups, func->num_groups);

	for_each_child_of_node(np, child)
	{
		func->groups[i] = child->name;
		group = &info->pin_groups[i++];
		group->name = child->name;

		npins = of_count_phandle_with_args(child, "fh,pins", cell_name);

		if (IS_ERR_VALUE(npins))
			return npins;
		group->npins = npins;
		PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, group->pins, npins);
		PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, group->pin_ids, npins);

		dev_dbg(info->dev, "get %d pins in group %s", group->npins, child->name);

		for (j = 0; j < npins; j++) {
			if (of_parse_phandle_with_args(child, "fh,pins", cell_name, j, &args)) {
				return -EINVAL;
			}
			pin = &group->pins[j];
			pin->name = args.np->name;

			mux = fh_pinctrl_find_mux_by_name(pin->name, info);

			if (mux == NULL)
				return -ENXIO;
			pin->mux = mux;

			if (args.args_count < 1)
				pin->mux_sel = pin->mux->select;
			else
				pin->mux_sel = args.args[0];

			if (pin->mux_sel >= pin->mux->npads)
				pin->mux_sel = pin->mux->select;

			pin_id = fh_pinctrl_get_pin_id(group, j);
			if (pin_id >= 0)
				group->pin_ids[j] = pin_id;
			else
				return pin_id;

			/* set the pin's reg config */
			pad = &info->pin_pads[pin_id];
			pin->config = pad->config;
			func_idx = fh_pinctrl_find_func_in_pad(pin->name, pad);
			if (func_idx < 0) {
				dev_err(info->dev, "can not find func %s in %s",
						pin->name, pad->name);
				return func_idx;
			}
			pin->config.bit.mfs = func_idx;

			dev_dbg(info->dev, "\tget pin %s --> mux_sel %d",
					pin->name, pin->mux_sel);
		}
	}

	return 0;
}


static int fh_pinctrl_probe_dt(struct platform_device *pdev,
				struct fh_pinctrl_soc_info *info)
{
	static const char *FH_PINCTRL_GROUPS = "pinctrl_groups";
	static const char *FH_PINCTRL_PAD = "pinpad";
	static const char *FH_PINCTRL_MUX = "pinmux";

	struct device_node *np = pdev->dev.of_node;
	struct device_node *np_grp;
	struct device_node *np_pad;
	struct device_node *np_mux;
	unsigned int nfuncs = 1;
	bool flat_funcs;
	int ret = 0;

	if (!np)
		return -ENODEV;

	np_pad = of_get_child_by_name(np, FH_PINCTRL_PAD);
	np_mux = of_get_child_by_name(np, FH_PINCTRL_MUX);
	np_grp = of_get_child_by_name(np, FH_PINCTRL_GROUPS);

	if (!np_grp || !np_pad || !np_mux) {
		dev_err(&pdev->dev, "child node %s or %s or %s not found\n",
				FH_PINCTRL_GROUPS, FH_PINCTRL_PAD, FH_PINCTRL_MUX);
		return -ENODEV;
	}

	flat_funcs = fh_pinctrl_dt_is_flat_functions(np_grp);
	if (flat_funcs) {
		nfuncs = 1;
	} else {
		nfuncs = of_get_child_count(np_grp);
		if (nfuncs <= 0) {
			dev_err(&pdev->dev, "no functions defined\n");
			return -EINVAL;
		}
	}

	info->nfunctions = nfuncs;
	PINCTRL_DEVM_KZALLOC_ARRAY(info->dev, info->functions, info->nfunctions);

	ret = fh_pinctrl_parse_pinpads(np_pad, info);
	if (ret) {
		dev_err(&pdev->dev, "error when parse pinpads, ret: %d\n", ret);
		return ret;
	}

	ret = fh_pinctrl_parse_pinmuxs(np_mux, info);
	if (ret) {
		dev_err(&pdev->dev, "error when parse pinmuxs, ret: %d\n", ret);
		return ret;
	}

	ret = fh_pinctrl_parse_pingroups(np_grp, info);
	if (ret) {
		dev_err(&pdev->dev, "error when parse pingroups, ret: %d\n", ret);
		return ret;
	}


	dev_info(&pdev->dev, "nfuncs: %d, ngroups: %d\n", nfuncs, info->ngroups);

	return 0;
}

static int fh_pinctrl_procfile_init(void);

static int fh_pinctrl_probe_with_info(struct platform_device *pdev,
			  struct fh_pinctrl_soc_info *info)
{
	struct device_node *dev_np = pdev->dev.of_node;
	struct pinctrl_desc *fh_pinctrl_desc;
	struct fh_pinctrl *fhpctl;
	struct resource *res;
	int ret, i;

	info->dev = &pdev->dev;

	PINCTRL_PARSE_PROPERTY_U32(dev_np, "pad-num", info->npins);

	PINCTRL_PARSE_PROPERTY_U32(dev_np, "max-mux", info->max_mux);

	info->pupd_enable = 1;
	info->pupd_disable = 0;
	info->pupd_zero = 0;

	of_property_read_u32(dev_np, "pupd-enable", &info->pupd_enable);
	of_property_read_u32(dev_np, "pupd-disable", &info->pupd_disable);
	of_property_read_u32(dev_np, "pupd-zero", &info->pupd_zero);

	dev_info(&pdev->dev, "%d pins, max-mux: %d\n", info->npins, info->max_mux);

	/* Create state holders etc for this driver */
	PINCTRL_DEVM_KZALLOC(&pdev->dev, fhpctl);

	PINCTRL_DEVM_KZALLOC_ARRAY(&pdev->dev, info->pin_regs, info->npins);
	PINCTRL_DEVM_KZALLOC_ARRAY(&pdev->dev, info->pin_pads, info->npins);
	PINCTRL_DEVM_KZALLOC_ARRAY(&pdev->dev, info->pins, info->npins);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->pbase = (void *)res->start;
	info->vbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->vbase))
		return PTR_ERR(info->vbase);

	for (i = 0; i < info->npins; i++) {
		info->pin_regs[i] = info->vbase + i * 4;
		info->pins[i].number = i;
		info->pin_pads[i].index = -1;
		info->pin_pads[i].func_sel = -1;
		info->pin_pads[i].reg = info->pin_regs[i];
	}

	PINCTRL_DEVM_KZALLOC(&pdev->dev, fh_pinctrl_desc);

	fh_pinctrl_desc->name = dev_name(&pdev->dev);
	fh_pinctrl_desc->pins = info->pins;
	fh_pinctrl_desc->npins = info->npins;
	fh_pinctrl_desc->pctlops = &fh_pinctrl_ops;
	fh_pinctrl_desc->pmxops = &fh_pmx_ops;
#ifdef CONFIG_PINCONF
	fh_pinctrl_desc->confops = &fh_pinconf_ops;
#endif
	fh_pinctrl_desc->owner = THIS_MODULE;

	ret = fh_pinctrl_probe_dt(pdev, info);
	if (ret) {
		dev_err(&pdev->dev, "fail to probe dt properties\n");
		return ret;
	}

	fhpctl->info = info;
	fhpctl->dev = info->dev;
	platform_set_drvdata(pdev, fhpctl);
	fhpctl->pctl = devm_pinctrl_register(&pdev->dev,
						fh_pinctrl_desc, fhpctl);
	if (IS_ERR(fhpctl->pctl)) {
		dev_err(&pdev->dev, "could not register Fullhan pinctrl driver\n");
		return PTR_ERR(fhpctl->pctl);
	}

	ret = fh_pinctrl_procfile_init();
	if (ret)
		return ret;

	dev_info(&pdev->dev, "initialized Fullhan pinctrl driver\n");

	return 0;
}

static struct fh_pinctrl_soc_info fh_pinctrl_info;

static const struct of_device_id fh_pinctrl_of_match[] = {
	{ .compatible = "fh,fh-pinctrl", },
	{ /* sentinel */ }
};

static int fh_pinctrl_probe(struct platform_device *pdev)
{
	return fh_pinctrl_probe_with_info(pdev, &fh_pinctrl_info);
}

static struct platform_driver fh_pinctrl_driver = {
	.driver = {
		.name = DEVICE_NAME,
		.of_match_table = of_match_ptr(fh_pinctrl_of_match),
	},
	.probe = fh_pinctrl_probe,
};

static int __init fh_pinctrl_init(void)
{
	return platform_driver_register(&fh_pinctrl_driver);
}
arch_initcall(fh_pinctrl_init);


/* ----------------- pinctrl proc file api ---------------- */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>


static int fh_pmx_set_mux(const struct fh_pingroup *grp, const char *mux_name,
		int muxsel, const struct fh_pinctrl_soc_info *info,
		const char **oldfunc)
{
	struct fh_pinpad *pad;
	int func_index = -1;
	int i;

	if (!grp || !mux_name || grp->npins == 0 || muxsel < 0)
		return -EINVAL;

	for (i = 0; i < grp->npins; i++) {
		struct fh_pin *pin = &grp->pins[i];

		if (strncmp(mux_name, pin->name, MAX_NAME_CHARS) == 0) {
			if (muxsel >= pin->mux->npads) {
				pr_err("[PINCTRL] set mux %s error, muxsel out of range\n",
						mux_name);
				return -EINVAL;
			}

			pin->mux_sel = muxsel;
			pad = pin->mux->pads[muxsel];

			func_index = fh_pinctrl_find_func_in_pad(mux_name, pad);
			if (func_index < 0)
				return func_index;
			/* save oldfunc */
			if (oldfunc) {
				if (pad->func_sel >= 0
						&& pad->func_sel < pad->nfunc)
					*oldfunc = pad->funcs[pad->func_sel];
				else
					*oldfunc = NULL;
			}

			pad->config = pin->config;
			pad->config.bit.mfs = func_index;
			if (pin->mux->ds >= 0)
				pad->config.bit.ds = pin->mux->ds;
			pin->mux->select = muxsel;

			fh_pinctrl_check_duplicate_pin(pad->index, info);
			fh_pinctrl_update_pad_reg(pad);

			pr_debug("[PINCTRL] write pin reg: id %2d, offset 0x%03x, "
					"val 0x%08x, %s\n",
				pad->index, (int)pad->reg&0xfff, pad->config.dw, pin->name);
			break;
		}
	}

	if (func_index < 0) {
		pr_err("[PINCTRL] pin %s of dev %s not found\n", mux_name, grp->name);
		return -EINVAL;
	}

	return 0;
}

int fh_pinctrl_smux(const char *dev, const char* mux, int muxsel, int flag)
{
	struct fh_pingroup *group;
	const char *oldfunc = NULL;
	int ret = 0;

	if (flag & PIN_RESTORE) {
		struct fh_pinpad *pad;
		struct fh_pinmux *m = fh_pinctrl_find_mux_by_name(mux,
				&fh_pinctrl_info);

		if (m == NULL) {
			pr_err("[PINCTRL] mux %s not found", mux);
			return -ENOENT;
		}

		pad = m->pads[m->select];
		pad->config.bit.mfs = fh_pinctrl_find_func_in_pad(mux, pad);
		if (m->ds >= 0)
			pad->config.bit.ds = m->ds;
		fh_pinctrl_check_duplicate_pin(pad->index, &fh_pinctrl_info);
		fh_pinctrl_update_pad_reg(pad);
		return 0;
	}

	group = fh_pinctrl_find_group_by_name(&fh_pinctrl_info, dev);

	if (!group) {
		pr_err("[PINCTRL] device %s not found", dev);
		return -ENODEV;
	}

	pr_info("[PINCTRL] %s: %s %d\n", dev, mux, muxsel);

	ret = fh_pmx_set_mux(group, mux, muxsel, &fh_pinctrl_info, &oldfunc);
	if (flag & PIN_BACKUP)
		return (int)oldfunc;
	else
		return ret;
}
EXPORT_SYMBOL(fh_pinctrl_smux);

char *fh_pinctrl_smux_backup(char *devname, char *muxname, int muxsel)
{
	return (char *)fh_pinctrl_smux(devname, muxname, muxsel, PIN_BACKUP);
}
EXPORT_SYMBOL(fh_pinctrl_smux_backup);

char *fh_pinctrl_smux_restore(char *devname, char *muxname, int muxsel)
{
	return (char *)fh_pinctrl_smux(devname, muxname, muxsel, PIN_RESTORE);
}
EXPORT_SYMBOL(fh_pinctrl_smux_restore);

int fh_pinctrl_sdev(const char *dev, int flag)
{
	struct fh_pingroup *group;

	group = fh_pinctrl_find_group_by_name(&fh_pinctrl_info, dev);
	if (!group) {
		pr_err("[PINCTRL] device %s not found", dev);
		return -ENODEV;
	}

	pr_info("[PINCTRL] %s\n", dev);

	return fh_pmx_set_group(group, &fh_pinctrl_info);
}
EXPORT_SYMBOL(fh_pinctrl_sdev);

static struct fh_pinpad *fh_pinctrl_get_pin_by_name(char *pin_name)
{
	struct fh_pinpad *pad;
	const char *name;
	int i;

	for (i = 0; i < fh_pinctrl_info.npins; i++) {
		pad = &fh_pinctrl_info.pin_pads[i];
		if (!pad->funcs)
			continue;
		name = pad->funcs[pad->config.bit.mfs];
		if (!strncmp(name, pin_name, MAX_NAME_CHARS))
			return pad;
	}

	return NULL;
}

int fh_pinctrl_spupd(char *pin_name, unsigned int pupd)
{
	struct fh_pinpad *pad = NULL;
	int ret;

	pad = fh_pinctrl_get_pin_by_name(pin_name);

	if (!pad) {
		pr_err("ERROR: cannot find pin %s\n", pin_name);
		return -ENXIO;
	}

	ret = fh_pinctrl_set_pin_pupd(&pad->config, pupd);
	if (ret) {
		pr_err("error config pupd:%d, ret: %d\n", pupd, ret);
		return ret;
	}

	fh_pinctrl_update_pad_reg(pad);
	return 0;
}
EXPORT_SYMBOL(fh_pinctrl_spupd);

int fh_pinctrl_sds(char *pin_name, unsigned int ds)
{
	struct fh_pinpad *pad = NULL;

	if (ds > 7) {
		pr_err("ds val is in [0-7]\n");
		return -EINVAL;
	}

	pad = fh_pinctrl_get_pin_by_name(pin_name);

	if (!pad) {
		pr_err("ERROR: cannot find pin %s\n", pin_name);
		return -ENXIO;
	}

	pad->config.bit.ds = ds;

	fh_pinctrl_update_pad_reg(pad);
	return 0;
}
EXPORT_SYMBOL(fh_pinctrl_sds);

int fh_pinctrl_set_oe(char *pin_name, unsigned int oe)
{
	struct fh_pinpad *pad = NULL;

	if (oe > 1) {
		pr_err("oe is 0 or 1\n");
		return -EINVAL;
	}

	pad = fh_pinctrl_get_pin_by_name(pin_name);

	if (!pad) {
		pr_err("ERROR: cannot find pin %s\n", pin_name);
		return -ENXIO;
	}

	pad->config.bit.oe = oe;

	fh_pinctrl_update_pad_reg(pad);
	return 0;
}
EXPORT_SYMBOL(fh_pinctrl_set_oe);

static struct proc_dir_entry *pinctrl_proc_file;

static void del_char(char* str,char ch)
{
	char *p = str;
	char *q = str;
	while(*q)
	{
		if (*q !=ch)
		{
			*p++ = *q;
		}
		q++;
	}
	*p='\0';
}

static ssize_t fh_pinctrl_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	int i;
	char message[32] = {0};
	char * const delim = ",";
	char *cur = message;
	char* param_str[4];
	unsigned long param[4];

	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 4; i++)
	{
		param_str[i] = strsep(&cur, delim);
		if(!param_str[i])
		{
			pr_err("%s: ERROR: parameter[%d] is empty\n", __func__, i);
			pr_err("[dev/mux], [dev name], [mux name], [func sel]\n");
			return -EINVAL;
		}
		else
		{
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}

	if (!strcmp(param_str[0], "dev")) {
		fh_pinctrl_sdev(param_str[1], 0);
	}
	else if (!strcmp(param_str[0], "mux")) {
		int ret = (u32)kstrtoul(param_str[3], 10, &param[3]);

		if (ret || param[3] < 0) {
			pr_err("ERROR: parameter[3] is incorrect\n");
			return -EINVAL;
		}
		fh_pinctrl_smux(param_str[1], param_str[2], param[3], 0);
	} else if (!strcmp(param_str[0], "pupd")) {
		if (!strcmp(param_str[2], "up"))
			param[2] = PUPD_UP;
		else if (!strcmp(param_str[2], "down"))
			param[2] = PUPD_DOWN;
		else if (!strcmp(param_str[2], "none"))
			param[2] = PUPD_NONE;
		else {
			pr_err("usage pupd, [pin name], [up|down|none], 0");
			return -EINVAL;
		}
		if (!fh_pinctrl_spupd(param_str[1], param[2]))
			pr_info("PIN: %s pupd: %s\n", param_str[1],
				param_str[2]);
	} else if (!strcmp(param_str[0], "ds")) {
		int ret = kstrtoul(param_str[2], 10, &param[2]);

		if (ret || param[2] < 0) {
			pr_err("usage: ds, [pin name] [ds_val(0~7)], 0\n");
			return -EINVAL;
		}
		if (!fh_pinctrl_sds(param_str[1], param[2]))
			pr_info("PIN: %s ds: %ld\n", param_str[1], param[2]);
	} else {
		pr_err("ERROR: parameter[0] is incorrect\n"
			"[dev/mux], [dev name], [mux name], [func sel]\n");
		return -EINVAL;
	}

	return len;
}

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
	if (*pos == 0)
		return &counter;
	else
	{
		*pos = 0;
		return NULL;
	}
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{
	struct fh_pinpad *pad;
	int i;

	fh_pinctrl_check_padlist(&fh_pinctrl_info);

	seq_printf(sfile, "%2s\t%8s\t%6s\t%8s  %4s%4s%4s%8s%4s\n",
			"ID", "NAME", "ADDR", "REG", "SEL", "IE", "OE", "PUPD", "DS");
	for (i = 0; i < fh_pinctrl_info.npins; i++) {
		pad = &fh_pinctrl_info.pin_pads[i];
		fh_pinctrl_print_pin_info(pad, sfile, 0);
		if (pad->index >= 0)
			seq_printf(sfile, "\n");
	}
	return 0;
}

static const struct seq_operations pinctrl_seq_ops =
{
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_pinctrl_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &pinctrl_seq_ops);
}

static struct file_operations fh_pinctrl_proc_ops =
{
	.owner = THIS_MODULE,
	.open = fh_pinctrl_proc_open,
	.read = seq_read,
	.write = fh_pinctrl_proc_write,
	.release = seq_release,
};

static int fh_pinctrl_procfile_init(void)
{
	pinctrl_proc_file = proc_create(FH_PINCTRL_PROC_FILE, 0644, NULL ,
		&fh_pinctrl_proc_ops);

	if (!pinctrl_proc_file) {
		pr_err("%s: ERROR: %s proc file create failed", __func__, DEVICE_NAME);
		return -1;
	}
	return 0;
}


MODULE_AUTHOR("Fullhan");

MODULE_DESCRIPTION("FH PINCTRL driver");
MODULE_LICENSE("GPL v2");
