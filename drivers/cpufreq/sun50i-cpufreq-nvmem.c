// SPDX-License-Identifier: GPL-2.0
/*
 * The sun50i-cpufreq-nvmem driver reads the efuse value from the SoC to
 * provide the OPP framework with required information.
 *
 * Copyright (C) 2020 frank@allwinnertech.com
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/sunxi-sid.h>

#define MAX_NAME_LEN    3

#define SUN50IW9_ICPU_MASK     GENMASK(9, 0)

static struct platform_device *cpufreq_dt_pdev, *sun50i_cpufreq_pdev;

struct cpufreq_nvmem_data {
	u32 nv_speed;
	u32 nv_Icpu;
	u32 nv_bin;
	u32 nv_bin_ext;
	u32 version;
	char name[MAX_NAME_LEN];
};

static struct cpufreq_nvmem_data ver_data;

struct cpufreq_soc_data {
	void (*nvmem_xlate)(u32 *versions, char *name);
	bool nvmem_speed;
	bool nvmem_Icpu;
	bool nvmem_bin;
	bool has_nvmem_extend_bin;
};

static int sun50i_nvmem_get_data(char *cell_name, u32 *data)
{
	struct nvmem_cell *cell_nvmem;
	size_t len;
	u8 *cell_value;
	u32 tmp_data = 0;
	u32 i;
	struct device_node *np;
	struct device *cpu_dev;
	int ret = 0;
	int ptr;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev)
		return -ENODEV;

	np = of_parse_phandle(cpu_dev->of_node, "operating-points-v2", 0);
	if (!np)
		return -ENOENT;

	ptr = of_device_is_compatible(np,
				      "allwinner,sun50i-operating-points");
	if (!ptr) {
		ret = -ENOENT;
		goto err;
	}

	cell_nvmem = of_nvmem_cell_get(np, cell_name);
	if (IS_ERR(cell_nvmem)) {
		if (PTR_ERR(cell_nvmem) != -EPROBE_DEFER)
			pr_err("Could not get nvmem cell: %ld\n",
			       PTR_ERR(cell_nvmem));
		ret = PTR_ERR(cell_nvmem);
		goto err;
	}

	cell_value = nvmem_cell_read(cell_nvmem, &len);
	nvmem_cell_put(cell_nvmem);
	if (IS_ERR(cell_value)) {
		ret = PTR_ERR(cell_value);
		goto err;
	}

	if (len > 4) {
		pr_err("Invalid nvmem cell length\n");
		ret = -EINVAL;
	} else {
		for (i = 0; i < len; i++)
			tmp_data |= ((u32)cell_value[i] << (i * 8));
		*data = tmp_data;
	}

	kfree(cell_value);

err:
	of_node_put(np);
	return ret;
}

static const char sun8iw21_ic_index_list[][32] = {
	{"allwinner,qg3101"},
	{"allwinner,v851s"},
	{"allwinner,v851se"},
	{"allwinner,v853s"},
	{"allwinner,r853s"},
	{"allwinner,v853"},
	{"allwinner,r853"},
};


static unsigned int sun8iw21_get_ic_index(void)
{
	int index;
	for (index = 0; index < ARRAY_SIZE(sun8iw21_ic_index_list); index++) {
		if (of_machine_is_compatible(sun8iw21_ic_index_list[index]) > 0) {
			return index+1;
		}
	}
	return 0;


}

static void sun8iw21_nvmem_xlate(u32 *versions, char *name)
{
	int value = 0;
	u32 SID18 = (ver_data.nv_bin >> 24) & 0x1;
	u32 ic_index = sun8iw21_get_ic_index();
	u32 vf_tbl_sel = 0;
	struct device_node *np = of_find_node_by_path("/cpus/cpu@0");
	if (np)
		of_property_read_u32(np, "vf_tbl_sel", &vf_tbl_sel);

	switch (ic_index) {
	case 1:
		if (vf_tbl_sel == 0)
			value = 1;
		else
			value = 2;
		break;
	case 2:
	case 3:
		if (vf_tbl_sel == 0)
			value = 3;
		else
			value = 4;
		break;
	case 4:
	case 5:
		if (vf_tbl_sel == 0)
			value = 5;
		else if (vf_tbl_sel == 1)
			value = 6;
		else if (vf_tbl_sel == 2)
			value = 10;
		else if (vf_tbl_sel == 3)
			value = 11;
		else
			value = 5;
		break;
	case 6:
	case 7:
		if (vf_tbl_sel == 0) {
			if (SID18 == 1) {
				value = 7;
			} else {
				value = 8;
			}
		} else {
			value = 9;
		}
		break;
	default:
		value = 0;
	}
	/* printk("\nline:%d %s P0:%d, Isys:%d vf_tbl_sel:%d, ic_index:%d value:%d\n\n", __LINE__, __func__, P0, Isys, vf_tbl_sel, ic_index, value); */
	*versions = (1 << value);
	/* snprintf(name, MAX_NAME_LEN, "a%d", value); */

}

static struct cpufreq_soc_data sun8iw21_soc_data = {
	.nvmem_xlate = sun8iw21_nvmem_xlate,
	.nvmem_bin = true,
};

static void sun50iw9_icpu_xlate(char *prop_name, char *name, u32 i_data)
{
	int value = 0;

	i_data &= SUN50IW9_ICPU_MASK;

	if ((i_data >= 0) && (i_data < 93))
		value = 0;
	/* 150 is temp munber */
	else if ((i_data >= 93) && (i_data < 150))
		value = 1;
	else if ((i_data >= 150) && (i_data < 271))
		value = 2;

	snprintf(name, MAX_NAME_LEN, "%s%d", prop_name, value);
}

static void sun50iw9_nvmem_xlate(u32 *versions, char *name)
{
	switch (ver_data.nv_speed) {
	case 0x2400:
	case 0x7400:
		sun50iw9_icpu_xlate("a", name, ver_data.nv_Icpu);
		break;
	case 0x2c00:
	case 0x7c00:
		sun50iw9_icpu_xlate("b", name, ver_data.nv_Icpu);
		break;
	case 0x5000:
	case 0x5400:
	case 0x6c00:
		*versions = 0b0001;
		break;
	case 0x5c00:
	default:
		*versions = 0b0010;
	}
}

static struct cpufreq_soc_data sun50iw9_soc_data = {
	.nvmem_xlate = sun50iw9_nvmem_xlate,
	.nvmem_speed = true,
	.nvmem_Icpu = true,
};

#define SUN50IW10_ICPU_EFUSE_OFF (0x1c)
#define SUN50IW10_PCPU_EFUSE_OFF (0x20)
static bool version_before_f;
static void sun50iw10_bin_xlate(bool high_speed, char *name, u32 bin)
{
	int value = 0;
	u32 bin_icpu, bin_pcpu;

	bin >>= 12;

	sunxi_get_module_param_from_sid(&bin_icpu, SUN50IW10_ICPU_EFUSE_OFF, 4);
	bin_icpu &= 0xfff; /* the unit is 0.1mA */
	sunxi_get_module_param_from_sid(&bin_pcpu, SUN50IW10_PCPU_EFUSE_OFF, 4);
	bin_pcpu &= 0xfff;

	if (high_speed) {
		switch (bin) {
		case 0b100:
			if (version_before_f) {
				/* ic version A-E */
				value = 1;
			} else {
				/* ic version F and later version */
				value = 3;
			}
			break;
		default:
			if (version_before_f) {
				/* ic version A-E */
				value = 0;
			} else {
				/* ic version F and later version */
				value = 2;
			}
		}

		snprintf(name, MAX_NAME_LEN, "b%d", value);
	} else {
		if ((!version_before_f) && (bin_pcpu >= 2900) && (bin_icpu >= 260 && bin_icpu < 360)) {
			value = 6;
		} else {
			switch (bin) {
			case 0b100:
				if (version_before_f) {
					/* ic version A-E */
					value = 2;
				} else {
					/* ic version F and later version */
					value = 5;
				}
				break;
			case 0b010:
				if (version_before_f) {
					/* ic version A-E */
					value = 1;
				} else {
					/* ic version F and later version */
					value = 4;
				}
				break;
			default:
				if (version_before_f) {
					/* ic version A-E */
					value = 0;
				} else {
					/* ic version F and later version */
					value = 3;
				}
			}
		}

		snprintf(name, MAX_NAME_LEN, "a%d", value);
	}
	pr_debug("sun50iw10 vf-table: %s\n", name);
}

static void sun50iw10_bin_force_vf1(u32 *versions, char *name)
{
	bool force_vf1;
	u32 bin_icpu, bin_pcpu;

	sunxi_get_module_param_from_sid(&bin_icpu, SUN50IW10_ICPU_EFUSE_OFF, 4);
	bin_icpu &= 0xfff; /* the unit is 0.1mA */
	sunxi_get_module_param_from_sid(&bin_pcpu, SUN50IW10_PCPU_EFUSE_OFF, 4);
	bin_pcpu &= 0xfff;

	if (version_before_f && (bin_icpu >= 84 && bin_icpu < 160) && (bin_pcpu >= 2900 && bin_pcpu < 3100))
		force_vf1 = true;
	else
		force_vf1 = false;

	if (force_vf1) {
		snprintf(name, MAX_NAME_LEN, "a0");
	}
}

static void sun50iw10_nvmem_xlate(u32 *versions, char *name)
{
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;

	if (ver_bits == 0 || ver_bits == 3 || ver_bits == 4)
		version_before_f = true;
	else
		version_before_f = false;

	switch (ver_data.nv_speed) {
	case 0x0200:
	case 0x0600:
	case 0x0620:
	case 0x0640:
	case 0x0800:
	case 0x1000:
	case 0x1400:
	case 0x2000:
	case 0x4000:
		sun50iw10_bin_xlate(true, name, ver_data.nv_bin);
		break;
	case 0x0400:
	default:
		sun50iw10_bin_xlate(false, name, ver_data.nv_bin);
		sun50iw10_bin_force_vf1(versions, name);
	}
}

static struct cpufreq_soc_data sun50iw10_soc_data = {
	.nvmem_xlate = sun50iw10_nvmem_xlate,
	.nvmem_speed = true,
	.nvmem_bin = true,
};

static void sun50iw11_nvmem_xlate(u32 *versions, char *name)
{
	int vf = 0;

	switch ((ver_data.nv_speed >> 8) & 0x0f) {
	case 0b0100:
		/* vf3 table */
		vf = 2;
		break;
	case 0b0010:
	case 0b0110:
		/* vf2 table */
		vf = 1;
		break;
	default:
		/* vf1 table */
		vf = 0;
		break;
	}

	snprintf(name, MAX_NAME_LEN, "a%d", vf);
}

static struct cpufreq_soc_data sun50iw11_soc_data = {
	.nvmem_xlate = sun50iw11_nvmem_xlate,
};

/**
 * sun50i_cpufreq_get_efuse() - Determine speed grade from efuse value
 * @versions: Set to the value parsed from efuse
 *
 * Returns 0 if success.
 */
static int sun50i_cpufreq_get_efuse(const struct cpufreq_soc_data *soc_data,
				    u32 *versions, char *name)
{
	int ptr;

	if (soc_data->nvmem_speed) {
		ptr = sun50i_nvmem_get_data("speed", &ver_data.nv_speed);
		if (ptr)
			return ptr;
	}

	if (soc_data->nvmem_Icpu) {
		ptr = sun50i_nvmem_get_data("Icpu", &ver_data.nv_Icpu);
		if (ptr)
			return ptr;
	}

	if (soc_data->nvmem_bin) {
		ptr = sun50i_nvmem_get_data("bin", &ver_data.nv_bin);
		if (ptr)
			return ptr;
	}

	if (soc_data->has_nvmem_extend_bin) {
		ptr = sun50i_nvmem_get_data("bin_ext", &ver_data.nv_bin_ext);
		if (ptr)
			return ptr;
	}

	soc_data->nvmem_xlate(versions, name);

	return 0;
};

static int sun50i_cpufreq_nvmem_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct opp_table **opp_tables;
	unsigned int cpu;
	int ret;

	opp_tables = kcalloc(num_possible_cpus(), sizeof(*opp_tables),
			     GFP_KERNEL);
	if (!opp_tables)
		return -ENOMEM;

	match = dev_get_platdata(&pdev->dev);
	if (!match)
		return -EINVAL;

	ret = sun50i_cpufreq_get_efuse(match->data,
						&ver_data.version, ver_data.name);
	if (ret)
		return ret;

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);

		if (!cpu_dev) {
			ret = -ENODEV;
			goto free_opp;
		}

		if (strlen(ver_data.name)) {
			ret = dev_pm_opp_set_prop_name(cpu_dev, ver_data.name);
			if (IS_ERR(opp_tables[cpu])) {
				ret = PTR_ERR(opp_tables[cpu]);
				pr_err("Failed to set prop name\n");
				goto free_opp;
			}
		} else if (ver_data.version) {
			ret = dev_pm_opp_set_supported_hw(cpu_dev, &ver_data.version, 1);
			if (ret) {
				dev_err(cpu_dev, "Failed to set supported hardware\n");
				goto free_opp;
			}
		} else
			goto free_opp;
	}

	cpufreq_dt_pdev = platform_device_register_simple("cpufreq-dt", -1,
							  NULL, 0);
	if (!IS_ERR(cpufreq_dt_pdev)) {
		platform_set_drvdata(pdev, opp_tables);
		return 0;
	}

	ret = PTR_ERR(cpufreq_dt_pdev);
	pr_err("Failed to register platform device\n");

free_opp:
	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);
		if (IS_ERR_OR_NULL(opp_tables[cpu]))
			break;

		if (strlen(ver_data.name))
			dev_pm_opp_put_prop_name(cpu_dev);
		else if (ver_data.version)
			dev_pm_opp_put_supported_hw(cpu_dev);
	}
	kfree(opp_tables);

	return ret;
}

static int sun50i_cpufreq_nvmem_remove(struct platform_device *pdev)
{
	struct opp_table **opp_tables = platform_get_drvdata(pdev);
	unsigned int cpu;

	platform_device_unregister(cpufreq_dt_pdev);

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);
		dev_pm_opp_put_supported_hw(cpu_dev);

		if (strlen(ver_data.name))
			dev_pm_opp_put_prop_name(cpu_dev);
		else if (ver_data.version)
			dev_pm_opp_put_supported_hw(cpu_dev);
	}

	kfree(opp_tables);

	return 0;
}

static struct platform_driver sun50i_cpufreq_driver = {
	.probe = sun50i_cpufreq_nvmem_probe,
	.remove = sun50i_cpufreq_nvmem_remove,
	.driver = {
		.name = "sun50i-cpufreq-nvmem",
	},
};

static const struct of_device_id sun50i_cpufreq_match_list[] = {
	{
		.compatible = "arm,sun8iw21p1",
		.data = &sun8iw21_soc_data,
	},
	{
		.compatible = "arm,sun50iw9p1",
		.data = &sun50iw9_soc_data,
	},
	{
		.compatible = "arm,sun50iw10p1",
		.data = &sun50iw10_soc_data,
	},
	{
		.compatible = "arm,sun50iw11p1",
		.data = &sun50iw11_soc_data,
	},
	{ /* Sentinel */ }
};

static const struct of_device_id *sun50i_cpufreq_match_node(void)
{
	const struct of_device_id *match;
	struct device_node *np;

	np = of_find_node_by_path("/");
	match = of_match_node(sun50i_cpufreq_match_list, np);
	of_node_put(np);

	return match;
}

/*
 * Since the driver depends on nvmem drivers, which may return EPROBE_DEFER,
 * all the real activity is done in the probe, which may be defered as well.
 * The init here is only registering the driver and the platform device.
 */
static int __init sun50i_cpufreq_init(void)
{
	const struct of_device_id *match;
	int ret;

	match = sun50i_cpufreq_match_node();
	if (!match)
		return -ENODEV;

	ret = platform_driver_register(&sun50i_cpufreq_driver);
	if (unlikely(ret < 0))
		return ret;

	sun50i_cpufreq_pdev = platform_device_register_data(
		NULL, "sun50i-cpufreq-nvmem", -1, match, sizeof(*match));
	ret = PTR_ERR_OR_ZERO(sun50i_cpufreq_pdev);
	if (ret == 0)
		return 0;

	platform_driver_unregister(&sun50i_cpufreq_driver);
	return ret;
}
module_init(sun50i_cpufreq_init);

static void __exit sun50i_cpufreq_exit(void)
{
	platform_device_unregister(sun50i_cpufreq_pdev);
	platform_driver_unregister(&sun50i_cpufreq_driver);
}
module_exit(sun50i_cpufreq_exit);

MODULE_DESCRIPTION("Sun50i cpufreq driver");
MODULE_LICENSE("GPL v2");
