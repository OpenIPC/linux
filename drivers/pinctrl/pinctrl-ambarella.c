/*
 * drivers/pinctrl/ambarella/pinctrl-amb.c
 *
 * History:
 *	2013/12/18 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/syscore_ops.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <plat/rct.h>
#include <plat/pinctrl.h>
#include "core.h"

#define PIN_NAME_LENGTH		8
#define SUFFIX_LENGTH		4

#define MUXIDS_TO_PINID(m)	((m) & 0xfff)
#define MUXIDS_TO_ALT(m)	(((m) >> 12) & 0xf)

#define CONFIDS_TO_PINID(c)	((c) & 0xfff)
#define CONFIDS_TO_CONF(c)	(((c) >> 16) & 0xffff)

/*
 * bit1~0: 00: pull down, 01: pull up, 1x: clear pull up/down
 * bit2:   reserved
 * bit3:   1: config pull up/down, 0: leave pull up/down as default value
 * bit5~4: drive strength value
 * bit6:   reserved
 * bit7:   1: config drive strength, 0: leave drive strength as default value
 */
#define CONF_TO_PULL_VAL(c)	(((c) >> 0) & 0x1)
#define CONF_TO_PULL_CLR(c)	(((c) >> 1) & 0x1)
#define CFG_PULL_PRESENT(c)	(((c) >> 3) & 0x1)
#define CONF_TO_DS_VAL(c)	(((c) >> 4) & 0x3)
#define CFG_DS_PRESENT(c)	(((c) >> 7) & 0x1)

struct ambpin_group {
	const char		*name;
	unsigned int		*pins;
	unsigned		num_pins;
	u8			*alt;
	unsigned int		*conf_pins;
	unsigned		num_conf_pins;
	unsigned long		*conf;
};

struct ambpin_function {
	const char		*name;
	const char		**groups;
	unsigned		num_groups;
	unsigned long		function;
};

struct amb_pinctrl_soc_data {
	struct device			*dev;
	struct pinctrl_dev		*pctl;
	void __iomem			*regbase[GPIO_INSTANCES];
	void __iomem			*iomux_base;
	unsigned long			used[BITS_TO_LONGS(AMBGPIO_SIZE)];

	struct ambpin_function		*functions;
	unsigned int			nr_functions;
	struct ambpin_group		*groups;
	unsigned int			nr_groups;
};

void __iomem *amb_iomux_base = NULL;

/* check if the selector is a valid pin group selector */
static int amb_get_group_count(struct pinctrl_dev *pctldev)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	return soc->nr_groups;
}

/* return the name of the group selected by the group selector */
static const char *amb_get_group_name(struct pinctrl_dev *pctldev,
						unsigned selector)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	return soc->groups[selector].name;
}

/* return the pin numbers associated with the specified group */
static int amb_get_group_pins(struct pinctrl_dev *pctldev,
		unsigned selector, const unsigned **pins, unsigned *num_pins)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	*pins = soc->groups[selector].pins;
	*num_pins = soc->groups[selector].num_pins;
	return 0;
}

static void amb_pin_dbg_show(struct pinctrl_dev *pctldev,
			struct seq_file *s, unsigned pin)
{
	struct pin_desc *desc;

	seq_printf(s, " %s", dev_name(pctldev->dev));

	desc = pin_desc_get(pctldev, pin);
	if (desc) {
		seq_printf(s, " owner: %s%s%s%s",
			desc->mux_owner ? desc->mux_owner : "",
			desc->mux_owner && desc->gpio_owner ? " " : "",
			desc->gpio_owner ? desc->gpio_owner : "",
			!desc->mux_owner && !desc->gpio_owner ? "NULL" : "");
	} else {
		seq_printf(s, " not registered");
	}
}

static int amb_dt_node_to_map(struct pinctrl_dev *pctldev,
			struct device_node *np,
			struct pinctrl_map **map, unsigned *num_maps)
{
	struct amb_pinctrl_soc_data *soc = pinctrl_dev_get_drvdata(pctldev);
	struct ambpin_group *grp = NULL;
	struct pinctrl_map *new_map;
	char *grp_name = NULL;
	int length = strlen(np->name) + SUFFIX_LENGTH;
	u32 i, reg, new_num;

	if (of_property_read_u32(np, "reg", &reg))
		return -EINVAL;

	/* Compose group name */
	grp_name = devm_kzalloc(soc->dev, length, GFP_KERNEL);
	if (!grp_name)
		return -ENOMEM;

	snprintf(grp_name, length, "%s.%d", np->name, reg);

	/* find the group of this node by name */
	for (i = 0; i < soc->nr_groups; i++) {
		if (!strcmp(soc->groups[i].name, grp_name)) {
			grp = &soc->groups[i];
			break;
		}
	}
	if (grp == NULL){
		dev_err(soc->dev, "unable to find group for node %s\n", np->name);
		return -EINVAL;
	}

	new_num = !!grp->num_pins + grp->num_conf_pins;
	new_map = devm_kzalloc(soc->dev,
				sizeof(struct pinctrl_map) * new_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = new_num;

	/* create mux map */
	if (grp->num_pins) {
		new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
		new_map[0].data.mux.group = grp_name;
		new_map[0].data.mux.function = np->name;
		new_map++;
	}

	/* create config map */
	for (i = 0; i < grp->num_conf_pins; i++) {
		new_map[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
		new_map[i].data.configs.group_or_pin =
				pin_get_name(pctldev, grp->conf_pins[i]);
		new_map[i].data.configs.configs = &grp->conf[i];
		new_map[i].data.configs.num_configs = 1;
	}

	return 0;
}

static void amb_dt_free_map(struct pinctrl_dev *pctldev,
			struct pinctrl_map *map, unsigned num_maps)
{
}

/* list of pinctrl callbacks for the pinctrl core */
static const struct pinctrl_ops amb_pctrl_ops = {
	.get_groups_count	= amb_get_group_count,
	.get_group_name		= amb_get_group_name,
	.get_group_pins		= amb_get_group_pins,
	.pin_dbg_show		= amb_pin_dbg_show,
	.dt_node_to_map		= amb_dt_node_to_map,
	.dt_free_map		= amb_dt_free_map,
};

/* check if the selector is a valid pin function selector */
static int amb_pinmux_request(struct pinctrl_dev *pctldev, unsigned pin)
{
	struct amb_pinctrl_soc_data *soc;
	int rval = 0;

	soc = pinctrl_dev_get_drvdata(pctldev);

	if (test_and_set_bit(pin, soc->used))
		rval = -EBUSY;

	return rval;
}

/* check if the selector is a valid pin function selector */
static int amb_pinmux_free(struct pinctrl_dev *pctldev, unsigned pin)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	clear_bit(pin, soc->used);

	return 0;
}

/* check if the selector is a valid pin function selector */
static int amb_pinmux_get_fcount(struct pinctrl_dev *pctldev)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	return soc->nr_functions;
}

/* return the name of the pin function specified */
static const char *amb_pinmux_get_fname(struct pinctrl_dev *pctldev,
			unsigned selector)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	return soc->functions[selector].name;
}

/* return the groups associated for the specified function selector */
static int amb_pinmux_get_groups(struct pinctrl_dev *pctldev,
			unsigned selector, const char * const **groups,
			unsigned * const num_groups)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	*groups = soc->functions[selector].groups;
	*num_groups = soc->functions[selector].num_groups;
	return 0;
}

static void amb_pinmux_set_altfunc(struct amb_pinctrl_soc_data *soc,
			u32 bank, u32 offset, enum amb_pin_altfunc altfunc)
{
	void __iomem *regbase = soc->regbase[bank];
	void __iomem *iomux_reg;
	u32 i, data;

	if (altfunc == AMB_ALTFUNC_GPIO) {
		amba_clrbitsl(regbase + GPIO_AFSEL_OFFSET, 0x1 << offset);
		amba_clrbitsl(regbase + GPIO_DIR_OFFSET, 0x1 << offset);
	} else {
		amba_setbitsl(regbase + GPIO_AFSEL_OFFSET, 0x1 << offset);
		amba_clrbitsl(regbase + GPIO_MASK_OFFSET, 0x1 << offset);
	}

	if (soc->iomux_base) {
		for (i = 0; i < 3; i++) {
			iomux_reg = soc->iomux_base + IOMUX_REG_OFFSET(bank, i);
			data = amba_readl(iomux_reg);
			data &= (~(0x1 << offset));
			data |= (((altfunc >> i) & 0x1) << offset);
			amba_writel(iomux_reg, data);
		}
		iomux_reg = soc->iomux_base + IOMUX_CTRL_SET_OFFSET;
		amba_writel(iomux_reg, 0x1);
		amba_writel(iomux_reg, 0x0);
	}
}

/* enable a specified pinmux by writing to registers */
static int amb_pinmux_enable(struct pinctrl_dev *pctldev,
			unsigned selector, unsigned group)
{
	struct amb_pinctrl_soc_data *soc;
	const struct ambpin_group *grp;
	u32 i, bank, offset;

	soc = pinctrl_dev_get_drvdata(pctldev);
	grp = &soc->groups[group];

	for (i = 0; i < grp->num_pins; i++) {
		bank = PINID_TO_BANK(grp->pins[i]);
		offset = PINID_TO_OFFSET(grp->pins[i]);
		amb_pinmux_set_altfunc(soc, bank, offset,  grp->alt[i]);
	}

	return 0;
}

/* disable a specified pinmux by writing to registers */
static void amb_pinmux_disable(struct pinctrl_dev *pctldev,
			unsigned selector, unsigned group)
{
	struct amb_pinctrl_soc_data *soc;
	const struct ambpin_group *grp;

	soc = pinctrl_dev_get_drvdata(pctldev);
	grp = &soc->groups[group];

	/* FIXME: poke out the mux, set the pin to some default state? */
	dev_dbg(soc->dev,
		"disable group %s, %u pins\n", grp->name, grp->num_pins);
}

static int amb_pinmux_gpio_request_enable(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range, unsigned pin)
{
	struct amb_pinctrl_soc_data *soc;
	u32 bank, offset;

	soc = pinctrl_dev_get_drvdata(pctldev);

	if (!range || !range->gc) {
		dev_err(soc->dev, "invalid range: %p\n", range);
		return -EINVAL;
	}

	if (test_and_set_bit(pin, soc->used))
		return -EBUSY;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	amb_pinmux_set_altfunc(soc, bank, offset, AMB_ALTFUNC_GPIO);

	return 0;
}

static void amb_pinmux_gpio_disable_free(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned pin)
{
	struct amb_pinctrl_soc_data *soc;

	soc = pinctrl_dev_get_drvdata(pctldev);
	dev_dbg(soc->dev, "disable pin %u as GPIO\n", pin);
	/* Set the pin to some default state, GPIO is usually default */

	clear_bit(pin, soc->used);
}

/*
 * The calls to gpio_direction_output() and gpio_direction_input()
 * leads to this function call (via the pinctrl_gpio_direction_{input|output}()
 * function called from the gpiolib interface).
 */
static int amb_pinmux_gpio_set_direction(struct pinctrl_dev *pctldev,
			struct pinctrl_gpio_range *range,
			unsigned pin, bool input)
{
	struct amb_pinctrl_soc_data *soc;
	void __iomem *regbase;
	u32 bank, offset, mask;

	soc = pinctrl_dev_get_drvdata(pctldev);

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	regbase = soc->regbase[bank];
	mask = (0x1 << offset);

	amba_clrbitsl(regbase + GPIO_AFSEL_OFFSET, mask);
	if (input)
		amba_clrbitsl(regbase + GPIO_DIR_OFFSET, mask);
	else
		amba_setbitsl(regbase + GPIO_DIR_OFFSET, mask);

	return 0;
}

/* list of pinmux callbacks for the pinmux vertical in pinctrl core */
static const struct pinmux_ops amb_pinmux_ops = {
	.request		= amb_pinmux_request,
	.free			= amb_pinmux_free,
	.get_functions_count	= amb_pinmux_get_fcount,
	.get_function_name	= amb_pinmux_get_fname,
	.get_function_groups	= amb_pinmux_get_groups,
	.enable			= amb_pinmux_enable,
	.disable		= amb_pinmux_disable,
	.gpio_request_enable	= amb_pinmux_gpio_request_enable,
	.gpio_disable_free	= amb_pinmux_gpio_disable_free,
	.gpio_set_direction	= amb_pinmux_gpio_set_direction,
};

/* set the pin config settings for a specified pin */
static int amb_pinconf_set(struct pinctrl_dev *pctldev, unsigned int pin,
			unsigned long config)
{
	struct amb_pinctrl_soc_data *soc;
	u32 bank, offset, reg, val;

	soc = pinctrl_dev_get_drvdata(pctldev);
	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);

	if (CFG_PULL_PRESENT(config)) {
		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_OFFSET(bank));
		amba_clrbitsl(reg, 1 << offset);
		amba_setbitsl(reg, CONF_TO_PULL_VAL(config) << offset);

		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_OFFSET(bank));
		if (CONF_TO_PULL_CLR(config))
			amba_clrbitsl(reg, 1 << offset);
		else
			amba_setbitsl(reg, 1 << offset);
	}

	if (CFG_DS_PRESENT(config)) {
		amba_clrbitsl(RCT_REG(GPIO_DS0_OFFSET(bank)), 1 << offset);
		amba_clrbitsl(RCT_REG(GPIO_DS1_OFFSET(bank)), 1 << offset);
		/* set bit1 of DS value to DS0 reg, and set bit0 of DS value to DS1 reg,
		 * because bit[1:0] = 00 is 2mA, 10 is 4mA, 01 is 8mA, 11 is 12mA */
		val = (CONF_TO_DS_VAL(config) >> 1) & 0x1;
		amba_setbitsl(RCT_REG(GPIO_DS0_OFFSET(bank)), val << offset);
		val = CONF_TO_DS_VAL(config) & 0x1;
		amba_setbitsl(RCT_REG(GPIO_DS1_OFFSET(bank)), val << offset);
	}

	return 0;
}

/* get the pin config settings for a specified pin */
static int amb_pinconf_get(struct pinctrl_dev *pctldev,
			unsigned int pin, unsigned long *config)
{
	dev_WARN_ONCE(pctldev->dev, true, "NOT Implemented.\n");
	return -ENOTSUPP;
}

static void amb_pinconf_dbg_show(struct pinctrl_dev *pctldev,
			struct seq_file *s, unsigned pin)
{
	struct amb_pinctrl_soc_data *soc;
	u32 bank, offset, reg;
	u32 pull_en, pull_dir, drv_strength;

	soc = pinctrl_dev_get_drvdata(pctldev);
	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);

	reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_OFFSET(bank));
	pull_en = (amba_readl(reg) >> offset) & 0x1;

	reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_OFFSET(bank));
	pull_dir = (amba_readl(reg) >> offset) & 0x1;

	seq_printf(s, " pull: %s, ", pull_en ? pull_dir ? "up" : "down" : "disable");

	reg = RCT_REG(GPIO_DS0_OFFSET(bank));
	drv_strength = ((amba_readl(reg) >> offset) & 0x1) << 1;
	reg = RCT_REG(GPIO_DS1_OFFSET(bank));
	drv_strength |= (amba_readl(reg) >> offset) & 0x1;

	seq_printf(s, "drive-strength: %s",
		drv_strength == 3 ? "12mA" : drv_strength == 2 ? "8mA" :
		drv_strength == 1 ? "4mA" : "2mA");
}

/* list of pinconfig callbacks for pinconfig vertical in the pinctrl code */
static const struct pinconf_ops amb_pinconf_ops = {
	.pin_config_get		= amb_pinconf_get,
	.pin_config_set		= amb_pinconf_set,
	.pin_config_dbg_show	= amb_pinconf_dbg_show,
};

static struct pinctrl_desc amb_pinctrl_desc = {
	.pctlops = &amb_pctrl_ops,
	.pmxops = &amb_pinmux_ops,
	.confops = &amb_pinconf_ops,
	.owner = THIS_MODULE,
};

static int amb_pinctrl_parse_group(struct amb_pinctrl_soc_data *soc,
			struct device_node *np, int idx, const char **out_name)
{
	struct ambpin_group *grp = &soc->groups[idx];
	struct property *prop;
	const char *prop_name;
	char *grp_name;
	int length = strlen(np->name) + SUFFIX_LENGTH;
	u32 reg, i;

	grp_name = devm_kzalloc(soc->dev, length, GFP_KERNEL);
	if (!grp_name)
		return -ENOMEM;

	if (of_property_read_u32(np, "reg", &reg))
		return -EINVAL;

	snprintf(grp_name, length, "%s.%d", np->name, reg);

	grp->name = grp_name;

	prop_name = "amb,pinmux-ids";
	prop = of_find_property(np, prop_name, &length);
	if (prop) {
		grp->num_pins = length / sizeof(u32);

		grp->pins = devm_kzalloc(soc->dev,
					grp->num_pins * sizeof(u32), GFP_KERNEL);
		if (!grp->pins)
			return -ENOMEM;

		grp->alt = devm_kzalloc(soc->dev,
					grp->num_pins * sizeof(u8), GFP_KERNEL);
		if (!grp->alt)
			return -ENOMEM;

		of_property_read_u32_array(np, prop_name, grp->pins, grp->num_pins);

		for (i = 0; i < grp->num_pins; i++) {
			grp->alt[i] = MUXIDS_TO_ALT(grp->pins[i]);
			grp->pins[i] = MUXIDS_TO_PINID(grp->pins[i]);
		}
	}

	/* parse pinconf */
	prop_name = "amb,pinconf-ids";
	prop = of_find_property(np, prop_name, &length);
	if (prop) {
		grp->num_conf_pins = length / sizeof(u32);

		grp->conf_pins = devm_kzalloc(soc->dev,
					grp->num_conf_pins * sizeof(u32), GFP_KERNEL);
		if (!grp->conf_pins)
			return -ENOMEM;

		grp->conf = devm_kzalloc(soc->dev,
				grp->num_conf_pins * sizeof(unsigned long), GFP_KERNEL);
		if (!grp->conf)
			return -ENOMEM;

		of_property_read_u32_array(np, prop_name, grp->conf_pins, grp->num_conf_pins);

		for (i = 0; i < grp->num_conf_pins; i++) {
			grp->conf[i] = CONFIDS_TO_CONF(grp->conf_pins[i]);
			grp->conf_pins[i] = CONFIDS_TO_PINID(grp->conf_pins[i]);
		}
	}

	if (out_name)
		*out_name = grp->name;

	return 0;
}

static int amb_pinctrl_parse_dt(struct amb_pinctrl_soc_data *soc)
{
	struct device_node *np = soc->dev->of_node;
	struct device_node *child;
	struct ambpin_function *f;
	const char *gpio_compat = "ambarella,gpio";
	const char *fn;
	int i = 0, idxf = 0, idxg = 0;
	int ret;

	child = of_get_next_child(np, NULL);
	if (!child) {
		dev_err(soc->dev, "no group is defined\n");
		return -ENOENT;
	}

	/* Count total functions and groups */
	fn = "";
	for_each_child_of_node(np, child) {
		if (of_device_is_compatible(child, gpio_compat))
			continue;
		soc->nr_groups++;
		if (strcmp(fn, child->name)) {
			fn = child->name;
			soc->nr_functions++;
		}
	}

	soc->functions = devm_kzalloc(soc->dev, soc->nr_functions *
				sizeof(struct ambpin_function), GFP_KERNEL);
	if (!soc->functions)
		return -ENOMEM;

	soc->groups = devm_kzalloc(soc->dev, soc->nr_groups *
				sizeof(struct ambpin_group), GFP_KERNEL);
	if (!soc->groups)
		return -ENOMEM;

	/* Count groups for each function */
	fn = "";
	f = &soc->functions[idxf];
	for_each_child_of_node(np, child) {
		if (of_device_is_compatible(child, gpio_compat))
			continue;
		if (strcmp(fn, child->name)) {
			f = &soc->functions[idxf++];
			f->name = fn = child->name;
		}
		f->num_groups++;
	};

	/* Get groups for each function */
	fn = "";
	idxf = 0;
	for_each_child_of_node(np, child) {
		if (of_device_is_compatible(child, gpio_compat))
			continue;

		if (strcmp(fn, child->name)) {
			f = &soc->functions[idxf++];
			f->groups = devm_kzalloc(soc->dev,
					f->num_groups * sizeof(*f->groups),
					GFP_KERNEL);
			if (!f->groups)
				return -ENOMEM;
			fn = child->name;
			i = 0;
		}

		ret = amb_pinctrl_parse_group(soc, child,
					idxg++, &f->groups[i++]);
		if (ret)
			return ret;
	}

	return 0;
}

/* register the pinctrl interface with the pinctrl subsystem */
static int amb_pinctrl_register(struct amb_pinctrl_soc_data *soc)
{
	struct pinctrl_pin_desc *pindesc;
	char *pin_names;
	int pin, rval;

	/* dynamically populate the pin number and pin name for pindesc */
	pindesc = devm_kzalloc(soc->dev,
			sizeof(*pindesc) * AMBGPIO_SIZE, GFP_KERNEL);
	if (!pindesc) {
		dev_err(soc->dev, "No memory for pin desc\n");
		return -ENOMEM;
	}

	pin_names = devm_kzalloc(soc->dev,
			PIN_NAME_LENGTH * AMBGPIO_SIZE, GFP_KERNEL);
	if (!pin_names) {
		dev_err(soc->dev, "No memory for pin names\n");
		return -ENOMEM;
	}

	for (pin = 0; pin < AMBGPIO_SIZE; pin++) {
		pindesc[pin].number = pin;
		sprintf(pin_names, "io%d", pin);
		pindesc[pin].name = pin_names;
		pin_names += PIN_NAME_LENGTH;
	}

	amb_pinctrl_desc.name = dev_name(soc->dev);
	amb_pinctrl_desc.pins = pindesc;
	amb_pinctrl_desc.npins = AMBGPIO_SIZE;

	rval = amb_pinctrl_parse_dt(soc);
	if (rval)
		return rval;

	soc->pctl = pinctrl_register(&amb_pinctrl_desc, soc->dev, soc);
	if (!soc->pctl) {
		dev_err(soc->dev, "could not register pinctrl driver\n");
		return -EINVAL;
	}

	return 0;
}

static int amb_pinctrl_probe(struct platform_device *pdev)
{
	struct amb_pinctrl_soc_data *soc;
	struct resource *res;
	int i, rval;

	soc = devm_kzalloc(&pdev->dev, sizeof(*soc), GFP_KERNEL);
	if (!soc) {
		dev_err(&pdev->dev, "failed to allocate memory for private data\n");
		return -ENOMEM;
	}
	soc->dev = &pdev->dev;

	for (i = 0; i < GPIO_INSTANCES; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res == NULL) {
			dev_err(&pdev->dev, "no mem resource for gpio[%d]!\n", i);
			return -ENXIO;
		}

		soc->regbase[i] = devm_ioremap(&pdev->dev,
						res->start, resource_size(res));
		if (soc->regbase[i] == 0) {
			dev_err(&pdev->dev, "devm_ioremap() failed\n");
			return -ENOMEM;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "iomux");
	if (res != NULL) {
		soc->iomux_base = devm_ioremap(&pdev->dev,
						res->start, resource_size(res));
		if (soc->iomux_base == 0) {
			dev_err(&pdev->dev, "devm_ioremap() failed\n");
			return -ENOMEM;
		}

		amb_iomux_base = soc->iomux_base;
	}

	rval = amb_pinctrl_register(soc);
	if (rval)
		return rval;

	platform_set_drvdata(pdev, soc);
	dev_info(&pdev->dev, "Ambarella pinctrl driver registered\n");

	return 0;
}

#if defined(CONFIG_PM)

static u32 amb_iomux_pm_reg[GPIO_INSTANCES][3];
static u32 amb_pull_pm_reg[2][GPIO_INSTANCES];
static u32 amb_ds_pm_reg[GPIO_INSTANCES][2];

static int amb_pinctrl_suspend(void)
{
	u32 i, j, reg;

	for (i = 0; i < GPIO_INSTANCES; i++) {
		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_OFFSET(i));
		amb_pull_pm_reg[0][i] = amba_readl(reg);
		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_OFFSET(i));
		amb_pull_pm_reg[1][i] = amba_readl(reg);
	}

	for (i = 0; i < GPIO_INSTANCES; i++) {
		amb_ds_pm_reg[i][0] = amba_readl(RCT_REG(GPIO_DS0_OFFSET(i)));
		amb_ds_pm_reg[i][1] = amba_readl(RCT_REG(GPIO_DS1_OFFSET(i)));
	}

	if (amb_iomux_base == NULL)
		return 0;

	for (i = 0; i < GPIO_INSTANCES; i++) {
		for (j = 0; j < 3; j++) {
			reg = (u32)amb_iomux_base + IOMUX_REG_OFFSET(i, j);
			amb_iomux_pm_reg[i][j] = amba_readl(reg);
		}
	}

	return 0;
}

static void amb_pinctrl_resume(void)
{
	u32 i, j, reg;

	for (i = 0; i < GPIO_INSTANCES; i++) {
		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_OFFSET(i));
		amba_writel(reg, amb_pull_pm_reg[0][i]);
		reg = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_OFFSET(i));
		amba_writel(reg, amb_pull_pm_reg[1][i]);
	}

	for (i = 0; i < GPIO_INSTANCES; i++) {
		amba_writel(RCT_REG(GPIO_DS0_OFFSET(i)), amb_ds_pm_reg[i][0]);
		amba_writel(RCT_REG(GPIO_DS1_OFFSET(i)), amb_ds_pm_reg[i][1]);
	}

	if (amb_iomux_base == NULL)
		return;

	for (i = 0; i < GPIO_INSTANCES; i++) {
		for (j = 0; j < 3; j++) {
			reg = (u32)amb_iomux_base + IOMUX_REG_OFFSET(i, j);
			amba_writel(reg, amb_iomux_pm_reg[i][j]);
		}
	}

	amba_writel(amb_iomux_base + IOMUX_CTRL_SET_OFFSET, 0x1);
	amba_writel(amb_iomux_base + IOMUX_CTRL_SET_OFFSET, 0x0);
}

static struct syscore_ops amb_pinctrl_syscore_ops = {
	.suspend	= amb_pinctrl_suspend,
	.resume		= amb_pinctrl_resume,
};

#endif /* CONFIG_PM */

static const struct of_device_id amb_pinctrl_dt_match[] = {
	{ .compatible = "ambarella,pinctrl" },
	{},
};
MODULE_DEVICE_TABLE(of, amb_pinctrl_dt_match);

static struct platform_driver amb_pinctrl_driver = {
	.probe	= amb_pinctrl_probe,
	.driver	= {
		.name	= "ambarella-pinctrl",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(amb_pinctrl_dt_match),
	},
};

static int __init amb_pinctrl_drv_register(void)
{
#ifdef CONFIG_PM
	register_syscore_ops(&amb_pinctrl_syscore_ops);
#endif
	return platform_driver_register(&amb_pinctrl_driver);
}
postcore_initcall(amb_pinctrl_drv_register);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella SoC pinctrl driver");
MODULE_LICENSE("GPL v2");


