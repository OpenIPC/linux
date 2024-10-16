// SPDX-License-Identifier: GPL-2.0
/*
 * cpu vf test module
 * Copyright (C) 2019 frank@allwinnertech.com
 */

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

struct vf_dev {
	struct regulator *cpu_reg;
	struct clk *cpu_clk;
} vf_dev;

static ssize_t volt_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%d\n", regulator_get_voltage(vf_dev.cpu_reg));
}

static ssize_t volt_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	int volt;
	int ret;

	ret = kstrtoint(buf, 10, &volt);
	if (ret)
		return -EINVAL;

	regulator_set_voltage(vf_dev.cpu_reg, volt, volt);

	return count;
}

static ssize_t freq_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", clk_get_rate(vf_dev.cpu_clk));
}

static ssize_t freq_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	int freq;
	int ret;

	ret = kstrtoint(buf, 10, &freq);
	if (ret)
		return -EINVAL;

	clk_set_rate(vf_dev.cpu_clk, freq);

	return count;
}

static struct class_attribute vf_class_attrs[] = {
	__ATTR(cpu_volt, S_IWUSR | S_IRUGO, volt_show, volt_store),
	__ATTR(cpu_freq, S_IWUSR | S_IRUGO, freq_show, freq_store),
	__ATTR_NULL,
};

static struct class vf_class = {
	.name		= "vf_table",
	.class_attrs	= vf_class_attrs,
};

/*
 * An earlier version of opp-v1 bindings used to name the regulator
 * "cpu0-supply", we still need to handle that for backwards compatibility.
 */
static const char *find_supply_name(struct device *dev)
{
	struct device_node *np;
	struct property *pp;
	int cpu = dev->id;
	const char *name = NULL;

	np = of_node_get(dev->of_node);

	/* This must be valid for sure */
	if (WARN_ON(!np))
		return NULL;

	/* Try "cpu0" for older DTs */
	if (!cpu) {
		pp = of_find_property(np, "cpu0-supply", NULL);
		if (pp) {
			name = "cpu0";
			goto node_put;
		}
	}

	pp = of_find_property(np, "cpu-supply", NULL);
	if (pp) {
		name = "cpu";
		goto node_put;
	}

	dev_dbg(dev, "no regulator for cpu%d\n", cpu);
node_put:
	of_node_put(np);
	return name;
}

static int resources_available(void)
{
	struct device *cpu_dev;
	int ret = 0;
	const char *name;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev) {
		pr_err("failed to get cpu0 device\n");
		return -ENODEV;
	}

	vf_dev.cpu_clk = clk_get(cpu_dev, NULL);
	ret = PTR_ERR_OR_ZERO(vf_dev.cpu_clk);
	if (ret) {
		/*
		 * If cpu's clk node is present, but clock is not yet
		 * registered, we should try defering probe.
		 */
		if (ret == -EPROBE_DEFER)
			dev_dbg(cpu_dev, "clock not ready, retry\n");
		else
			dev_err(cpu_dev, "failed to get clock: %d\n", ret);

		return ret;
	}

	name = find_supply_name(cpu_dev);
	/* Platform doesn't require regulator */
	if (!name)
		return 0;

	vf_dev.cpu_reg = regulator_get_optional(cpu_dev, name);
	ret = PTR_ERR_OR_ZERO(vf_dev.cpu_reg);
	if (ret) {
		/*
		 * If cpu's regulator supply node is present, but regulator is
		 * not yet registered, we should try defering probe.
		 */
		if (ret == -EPROBE_DEFER)
			dev_dbg(cpu_dev, "cpu0 regulator not ready, retry\n");
		else
			dev_dbg(cpu_dev, "no regulator for cpu0: %d\n", ret);

		return ret;
	}

	return 0;
}

static int __init vf_init(void)
{
	int ret;

	ret = resources_available();
	if (ret)
		return ret;

	return class_register(&vf_class);
}

module_init(vf_init);
MODULE_LICENSE("GPL");
