/*
 * sunxi_pmc regulators driver.
 *
 * Copyright (C) 2013 Carlo Caione <carlo@caione.org>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/sunxi_pmc.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#define PMC_DESC_IO(_family, _id, _match, _supply, _min, _max, _step, _vreg,	\
		    _vmask, _ereg, _emask, _enable_val, _disable_val)		\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),		\
		.owner		= THIS_MODULE,					\
		.min_uV		= (_min) * 1000,				\
		.uV_step	= (_step) * 1000,				\
		.vsel_reg	= (_vreg),					\
		.vsel_mask	= (_vmask),					\
		.enable_reg	= (_ereg),					\
		.enable_mask	= (_emask),					\
		.enable_val	= (_enable_val),				\
		.disable_val	= (_disable_val),				\
		.ops		= &sunxi_pmc_ops,					\
	}

#define PMC_DESC(_family, _id, _match, _supply, _min, _max, _step, _vreg,	\
		 _vmask, _ereg, _emask) 					\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.n_voltages	= (((_max) - (_min)) / (_step) + 1),		\
		.owner		= THIS_MODULE,					\
		.min_uV		= (_min) * 1000,				\
		.uV_step	= (_step) * 1000,				\
		.vsel_reg	= (_vreg),					\
		.vsel_mask	= (_vmask),					\
		.enable_reg	= (_ereg),					\
		.enable_mask	= (_emask),					\
		.ops		= &sunxi_pmc_ops,					\
	}

#define PMC_DESC_SW(_family, _id, _match, _supply, _ereg, _emask)		\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.owner		= THIS_MODULE,					\
		.enable_reg	= (_ereg),					\
		.enable_mask	= (_emask),					\
		.ops		= &sunxi_pmc_ops_sw,				\
	}

#define PMC_DESC_FIXED(_family, _id, _match, _supply, _volt)			\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.n_voltages	= 1,						\
		.owner		= THIS_MODULE,					\
		.min_uV		= (_volt) * 1000,				\
		.ops		= &sunxi_pmc_ops_fixed				\
	}

#define PMC_DESC_RANGES(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)				\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.n_voltages	= (_n_voltages),				\
		.owner		= THIS_MODULE,					\
		.vsel_reg	= (_vreg),					\
		.vsel_mask	= (_vmask),					\
		.enable_reg	= (_ereg),					\
		.enable_mask	= (_emask),					\
		.linear_ranges	= (_ranges),					\
		.n_linear_ranges = ARRAY_SIZE(_ranges),				\
		.ops		= &sunxi_pmc_ops_range,				\
	}

#define PMC_DESC_RANGES_VOL_DELAY(_family, _id, _match, _supply, _ranges, _n_voltages,	\
			_vreg, _vmask, _ereg, _emask)				\
	[_family##_##_id] = {							\
		.name		= (_match),					\
		.supply_name	= (_supply),					\
		.of_match	= of_match_ptr(_match),				\
		.regulators_node = of_match_ptr("regulators"),			\
		.type		= REGULATOR_VOLTAGE,				\
		.id		= _family##_##_id,				\
		.n_voltages	= (_n_voltages),				\
		.owner		= THIS_MODULE,					\
		.vsel_reg	= (_vreg),					\
		.vsel_mask	= (_vmask),					\
		.enable_reg	= (_ereg),					\
		.enable_mask	= (_emask),					\
		.linear_ranges	= (_ranges),					\
		.n_linear_ranges = ARRAY_SIZE(_ranges),				\
		.ops		= &sunxi_pmc_ops_range_vol_delay,			\
	}

struct regulator_delay {
	u32 step;
	u32 final;
};



static int sunxi_pmc_set_voltage_time_sel(struct regulator_dev *rdev,
		unsigned int old_selector, unsigned int new_selector)
{
	struct regulator_delay *delay = (struct regulator_delay *)rdev->reg_data;

	return abs(new_selector - old_selector) * delay->step + delay->final;
};

static struct __attribute__((unused)) regulator_ops sunxi_pmc_ops_fixed = {
	.list_voltage		= regulator_list_voltage_linear,
};

static __attribute__((unused)) struct regulator_ops sunxi_pmc_ops_range = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_time_sel = sunxi_pmc_set_voltage_time_sel,
};

static __attribute__((unused)) struct regulator_ops sunxi_pmc_ops_range_vol_delay = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear_range,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};

static __attribute__((unused)) struct regulator_ops sunxi_pmc_ops = {
	.set_voltage_sel	= regulator_set_voltage_sel_regmap,
	.get_voltage_sel	= regulator_get_voltage_sel_regmap,
	.list_voltage		= regulator_list_voltage_linear,
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
	.set_voltage_time_sel = sunxi_pmc_set_voltage_time_sel,
};

static __attribute__((unused)) struct regulator_ops sunxi_pmc_ops_sw = {
	.enable			= regulator_enable_regmap,
	.disable		= regulator_disable_regmap,
	.is_enabled		= regulator_is_enabled_regmap,
};


static const struct regulator_desc pmc_regulators[] = {
	PMC_DESC_FIXED(PMC,	DCDC1,	"dcdc1",	"vin-ps1",	3300),
	PMC_DESC_FIXED(PMC,	DCDC2,	"dcdc2",	"vin-ps2",	900),
	PMC_DESC_FIXED(PMC,	DCDC3,	"dcdc3",	"vin-ps3",	1500),
	PMC_DESC_FIXED(PMC,	LDOA,	"ldoa",		"vin-ps4",	1800),
	PMC_DESC_FIXED(PMC,	ALDO,	"aldo",		"vin-ps5",	1800),
	PMC_DESC_FIXED(PMC,	RTCLDO,	"rtcldo",	"vin-ps6",	1800),
	PMC_DESC_FIXED(PMC,	LDO33,	"ldo33",	"vin-ps7",	3300),
	PMC_DESC_FIXED(PMC,	LDO09,	"ldo09",	"vin-ps8",	900),
};


static int sunxi_pmc_regulator_parse_dt(struct platform_device *pdev)
{
	struct device_node *np, *regulators;

	np = of_node_get(pdev->dev.parent->of_node);
	if (!np)
		return 0;

	regulators = of_get_child_by_name(np, "regulators");
	if (!regulators) {
		dev_warn(&pdev->dev, "regulators node not found\n");
	}

	return 0;
}


static int sunxi_pmc_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct sunxi_pmc_dev *sunxi_pmc = dev_get_drvdata(pdev->dev.parent);
	const struct regulator_desc *regulators;
	struct regulator_config config = {
		.dev = pdev->dev.parent,
		.regmap = sunxi_pmc->regmap,
		.driver_data = sunxi_pmc,
	};
	int i, nregulators;
	bool drivevbus = false;
	u32 dval;
	struct regulator_delay *rdev_delay;

	switch (sunxi_pmc->variant) {
	case PMC_V1_ID:
		regulators = pmc_regulators;
		nregulators = PMC_REG_ID_MAX;
		break;
	default:
		dev_err(&pdev->dev, "Unsupported PMC variant: %ld\n",
			sunxi_pmc->variant);
		return -EINVAL;
	}

	/* This only sets the dcdc freq. Ignore any errors */
	sunxi_pmc_regulator_parse_dt(pdev);

	for (i = 0; i < nregulators; i++) {
		const struct regulator_desc *desc = &regulators[i];

		/*
		 * Regulators DC1SW and DC5LDO are connected internally,
		 * so we have to handle their supply names separately.
		 *
		 * We always register the regulators in proper sequence,
		 * so the supply names are correctly read. See the last
		 * part of this loop to see where we save the DT defined
		 * name.
		 */

		rdev = devm_regulator_register(&pdev->dev, desc, &config);
		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "Failed to register %s\n",
				regulators[i].name);

			return PTR_ERR(rdev);
		}

		rdev_delay = devm_kzalloc(&pdev->dev, sizeof(*rdev_delay), GFP_KERNEL);
		if (!of_property_read_u32(rdev->dev.of_node,
				"regulator-step-delay-us", &dval))
			rdev_delay->step = dval;
		else
			rdev_delay->step = 0;

		if (!of_property_read_u32(rdev->dev.of_node,
				"regulator-final-delay-us", &dval))
			rdev_delay->final = dval;
		else
			rdev_delay->final = 0;

		rdev->reg_data = rdev_delay;


	}

	if (drivevbus) {
		switch (sunxi_pmc->variant) {
		default:
			dev_err(&pdev->dev, "PMC variant: %ld unsupported drivevbus\n",
				sunxi_pmc->variant);
			return -EINVAL;
		}

		if (IS_ERR(rdev)) {
			dev_err(&pdev->dev, "Failed to register drivevbus\n");
			return PTR_ERR(rdev);
		}
	}

	return 0;
}

static int sunxi_pmc_regulator_remove(struct platform_device *pdev)
{
	return 0;
}

static struct of_device_id pmc_regulator_id_tab[] = {
	{ .compatible = "x-powers,pmc-regulator" },
	{ /* sentinel */ },
};

static struct platform_driver sunxi_pmc_regulator_driver = {
	.probe	= sunxi_pmc_regulator_probe,
	.remove	= sunxi_pmc_regulator_remove,
	.driver	= {
		.of_match_table = pmc_regulator_id_tab,
		.name		= "pmc-regulator",
	},
};

static int __init sunxi_pmc_regulator_init(void)
{
	return platform_driver_register(&sunxi_pmc_regulator_driver);
}

static void __exit sunxi_pmc_regulator_exit(void)
{
	platform_driver_unregister(&sunxi_pmc_regulator_driver);
}

subsys_initcall(sunxi_pmc_regulator_init);
module_exit(sunxi_pmc_regulator_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_DESCRIPTION("Regulator Driver for sunxi_pmc PMIC");
MODULE_ALIAS("platform:sunxi_pmc-regulator");
