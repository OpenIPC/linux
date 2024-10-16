/*
 * sunxi_pmc power button driver.
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
#define pr_fmt(x) KBUILD_MODNAME ": " x

#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/sunxi_pmc.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/of.h>

#define IRQ_DBR_BIT             BIT(0)
#define IRQ_DBF_BIT             BIT(1)

struct pk_dts {
	uint32_t pmu_powkey_off_time;
	uint32_t pmu_powkey_off_func;
	uint32_t pmu_powkey_off_en;
	uint32_t pmu_powkey_off_delay_time;
	uint32_t pmu_powkey_long_time;
	uint32_t pmu_powkey_on_time;
	uint32_t pmu_pwrok_time;
	uint32_t pmu_pwrnoe_time;
	uint32_t pmu_powkey_wakeup_rising;
	uint32_t pmu_powkey_wakeup_falling;
};

struct sunxi_pmc_pek {
	struct sunxi_pmc_dev *sunxi_pmc;
	struct input_dev *input;
	struct pk_dts pk_dts;
	int irq_dbr;
	int irq_dbf;
};


#define PMC_OF_PROP_READ(name, def_value)                             \
	do {                                                          \
		if (of_property_read_u32(node, #name, &pk_dts->name)) \
			pk_dts->name = def_value;                     \
	} while (0)

static int pmc_powerkey_dt_parse(struct device_node *node,
				 struct pk_dts *pk_dts)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	PMC_OF_PROP_READ(pmu_powkey_off_time,       6000);
	PMC_OF_PROP_READ(pmu_powkey_off_func,       0);
	PMC_OF_PROP_READ(pmu_powkey_off_en,         1);
	PMC_OF_PROP_READ(pmu_powkey_off_delay_time, 0);
	PMC_OF_PROP_READ(pmu_powkey_long_time,      1500);
	PMC_OF_PROP_READ(pmu_powkey_on_time,        1000);
	PMC_OF_PROP_READ(pmu_pwrok_time,            64);
	PMC_OF_PROP_READ(pmu_pwrnoe_time,           2000);

	pk_dts->pmu_powkey_wakeup_rising =
		of_property_read_bool(node, "wakeup_rising");
	pk_dts->pmu_powkey_wakeup_falling =
		of_property_read_bool(node, "wakeup_falling");

	return 0;
}


static irqreturn_t sunxi_pmc_pek_irq(int irq, void *pwr)
{
	struct input_dev *idev = pwr;
	struct sunxi_pmc_pek *sunxi_pmc_pek = input_get_drvdata(idev);

	/*
	 * The power-button is connected to ground so a falling edge (dbf)
	 * means it is pressed.
	 */
	if (irq == sunxi_pmc_pek->irq_dbf)
		input_report_key(idev, KEY_POWER, true);
	else if (irq == sunxi_pmc_pek->irq_dbr)
		input_report_key(idev, KEY_POWER, false);

	input_sync(idev);

	return IRQ_HANDLED;
}

static int pmc_config_set(struct sunxi_pmc_pek *sunxi_pmc_pek)
{
	struct sunxi_pmc_dev *sunxi_pmc_dev = sunxi_pmc_pek->sunxi_pmc;
	struct regmap *regmap = sunxi_pmc_dev->regmap;
	struct pk_dts *pk_dts = &sunxi_pmc_pek->pk_dts;
	unsigned int val;

	regmap_read(regmap, PMC_DLY_CTRL_REG, &val);
	val &= ~(0x7 << 5);
	if (pk_dts->pmu_powkey_on_time <= 128) {
		val |= (0x00 << 5);
	} else if (pk_dts->pmu_powkey_on_time <= 256) {
		val |= (0x01 << 5);
	} else if (pk_dts->pmu_powkey_on_time <= 512) {
		val |= (0x02 << 5);
	} else if (pk_dts->pmu_powkey_on_time <= 1000) {
		val |= (0x03 << 5);
	} else {
		val |= (0x04 << 5);
	}
	regmap_write(regmap, PMC_DLY_CTRL_REG, val);

	/* pok long time set*/
	if (pk_dts->pmu_powkey_long_time < 1000)
		pk_dts->pmu_powkey_long_time = 1000;

	if (pk_dts->pmu_powkey_long_time > 2500)
		pk_dts->pmu_powkey_long_time = 2500;

	regmap_read(regmap, PMC_DLY_CTRL_REG, &val);
	val &= ~(0x3 << 10);
	val |= (((pk_dts->pmu_powkey_long_time - 1000) / 500)
		<< 10);
	regmap_write(regmap, PMC_DLY_CTRL_REG, val);

	/* pek offlevel poweroff en set*/
	regmap_read(regmap, SW_CFG_REG, &val);
	val &= ~((0x1 << 12) | (0xffff << 16));
	val |= ((!!pk_dts->pmu_powkey_off_en) << 12);
	regmap_write(regmap, SW_CFG_REG, val | 0x16AA << 16);
	val &= ~(0xffff << 16);
	val |= (0xAA16 << 16);
	regmap_write(regmap, SW_CFG_REG, val);

	/*Init offlevel restart or not */
	regmap_read(regmap, SW_CFG_REG, &val);
	val &= ~((0x3 << 1) | (0xffff << 16));
	val |= ((!!pk_dts->pmu_powkey_off_func) << 1);/* restart or  not restart */
	regmap_write(regmap, SW_CFG_REG, val | (0x16AA) << 16);
	val &= ~(0xffff << 16);
	val |= (0xAA16 << 16);
	regmap_write(regmap, SW_CFG_REG, val);

	/* pek delay set */
	regmap_read(regmap, RST_DLY_SEL_REG, &val);
	val &= ~(0x7 << 4);
	if (pk_dts->pmu_pwrok_time <= 4) {
		val |= (0 << 4);
	} else if (pk_dts->pmu_pwrok_time <= 8) {
		val |= (1 << 4);
	} else if (pk_dts->pmu_pwrok_time <= 16) {
		val |= (2 << 4);
	} else if (pk_dts->pmu_pwrok_time <= 32) {
		val |= (3 << 4);
	} else {
		val |= (4 << 4);
	}
	regmap_write(regmap, RST_DLY_SEL_REG, val);

	/* pek offlevel time set */
	regmap_read(regmap, PMC_DLY_CTRL_REG, &val);
	val &= ~(0x3 << 8);
	if (pk_dts->pmu_powkey_off_time <= 6000) {
		val |= (0 << 8);
	} else if (pk_dts->pmu_powkey_off_time <= 8000) {
		val |= (1 << 8);
	} else {
		val |= (2 << 8);
	}
	regmap_write(regmap, PMC_DLY_CTRL_REG, val);

	return 0;
}

static void sunxi_pmc_dts_param_set(struct sunxi_pmc_pek *sunxi_pmc_pek)
{
	struct sunxi_pmc_dev *sunxi_pmc_dev = sunxi_pmc_pek->sunxi_pmc;

	if (!pmc_powerkey_dt_parse(sunxi_pmc_pek->input->dev.parent->of_node,
				   &sunxi_pmc_pek->pk_dts)) {
		switch (sunxi_pmc_dev->variant) {
		case PMC_V1_ID:
			pmc_config_set(sunxi_pmc_pek);
			break;
		default:
			pr_warn("Setting power key for unsupported PMC variant\n");
		}
	}
}

static int sunxi_pmc_pek_probe(struct platform_device *pdev)
{
	struct sunxi_pmc_pek *sunxi_pmc_pek;
	struct sunxi_pmc_dev *sunxi_pmc;
	struct input_dev *idev;
	int error;

	sunxi_pmc_pek = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_pmc_pek),
				  GFP_KERNEL);
	if (!sunxi_pmc_pek)
		return -ENOMEM;

	sunxi_pmc_pek->sunxi_pmc = dev_get_drvdata(pdev->dev.parent);
	sunxi_pmc = sunxi_pmc_pek->sunxi_pmc;

	if (!sunxi_pmc->irq) {
		pr_err("sunxi_pmc-pek can not register without irq\n");
		return -EINVAL;
	}

	sunxi_pmc_pek->irq_dbr = platform_get_irq_byname(pdev, "PEK_DBR");
	if (sunxi_pmc_pek->irq_dbr < 0) {
		dev_err(&pdev->dev, "No IRQ for PEK_DBR, error=%d\n",
				sunxi_pmc_pek->irq_dbr);
		return sunxi_pmc_pek->irq_dbr;
	}
	sunxi_pmc_pek->irq_dbr = regmap_irq_get_virq(sunxi_pmc->regmap_irqc,
						  sunxi_pmc_pek->irq_dbr);

	sunxi_pmc_pek->irq_dbf = platform_get_irq_byname(pdev, "PEK_DBF");
	if (sunxi_pmc_pek->irq_dbf < 0) {
		dev_err(&pdev->dev, "No IRQ for PEK_DBF, error=%d\n",
				sunxi_pmc_pek->irq_dbf);
		return sunxi_pmc_pek->irq_dbf;
	}
	sunxi_pmc_pek->irq_dbf = regmap_irq_get_virq(sunxi_pmc->regmap_irqc,
						  sunxi_pmc_pek->irq_dbf);

	sunxi_pmc_pek->input = devm_input_allocate_device(&pdev->dev);
	if (!sunxi_pmc_pek->input)
		return -ENOMEM;

	idev = sunxi_pmc_pek->input;

	switch (sunxi_pmc->variant) {
	case PMC_V1_ID:
		idev->name = "pmc-v1-pek";
		break;
	default:
		idev->name = "sunxi_pmc-pek";
		break;
	}

	idev->phys = "m1kbd/input2";
	idev->dev.parent = &pdev->dev;

	input_set_capability(idev, EV_KEY, KEY_POWER);
	set_bit(EV_REP, idev->evbit);

	input_set_drvdata(idev, sunxi_pmc_pek);

	sunxi_pmc_dts_param_set(sunxi_pmc_pek);
	error = devm_request_any_context_irq(&pdev->dev, sunxi_pmc_pek->irq_dbr,
					     sunxi_pmc_pek_irq, 0,
					     "sunxi_pmc-pek-dbr", idev);
	if (error < 0) {
		dev_err(sunxi_pmc->dev, "Failed to request dbr IRQ#%d: %d\n",
			sunxi_pmc_pek->irq_dbr, error);
		return error;
	}

	error = devm_request_any_context_irq(&pdev->dev, sunxi_pmc_pek->irq_dbf,
					  sunxi_pmc_pek_irq, 0,
					  "sunxi_pmc-pek-dbf", idev);
	if (error < 0) {
		dev_err(sunxi_pmc->dev, "Failed to request dbf IRQ#%d: %d\n",
			sunxi_pmc_pek->irq_dbf, error);
		return error;
	}

	error = input_register_device(idev);
	if (error) {
		dev_err(sunxi_pmc->dev, "Can't register input device: %d\n",
			error);
		return error;
	}

	platform_set_drvdata(pdev, sunxi_pmc_pek);

	return 0;
}

static int sunxi_pmc_pek_remove(struct platform_device *pdev)
{
	struct sunxi_pmc_pek *sunxi_pmc_pek = platform_get_drvdata(pdev);

	input_unregister_device(sunxi_pmc_pek->input);

	return 0;
}

static int sunxi_pmc_powerkey_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_pmc_pek *sunxi_pmc_pek = platform_get_drvdata(pdev);

	if (!sunxi_pmc_pek->pk_dts.pmu_powkey_wakeup_rising) {
		disable_irq(sunxi_pmc_pek->irq_dbr);
	}

	if (!sunxi_pmc_pek->pk_dts.pmu_powkey_wakeup_falling) {
		disable_irq(sunxi_pmc_pek->irq_dbf);
	}

	return 0;
}

static int sunxi_pmc_powerkey_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_pmc_pek *sunxi_pmc_pek = platform_get_drvdata(pdev);

	if (!sunxi_pmc_pek->pk_dts.pmu_powkey_wakeup_rising) {
		enable_irq(sunxi_pmc_pek->irq_dbr);
	}

	if (!sunxi_pmc_pek->pk_dts.pmu_powkey_wakeup_falling) {
		enable_irq(sunxi_pmc_pek->irq_dbf);
	}

	return 0;
}

static const struct dev_pm_ops sunxi_pmc_powerkey_pm_ops = {
	.suspend      = sunxi_pmc_powerkey_suspend,
	.resume       = sunxi_pmc_powerkey_resume,
};

static struct of_device_id pmc_match_table[] = {
	{ .compatible = "x-powers,pmc-pek" },
	{ /* sentinel */ },
};

static struct platform_driver sunxi_pmc_pek_driver = {
	.probe		= sunxi_pmc_pek_probe,
	.remove		= sunxi_pmc_pek_remove,
	.driver		= {
		.of_match_table = pmc_match_table,
		.name = "pmc-pek",
		.pm = &sunxi_pmc_powerkey_pm_ops,
	},
};
module_platform_driver(sunxi_pmc_pek_driver);

MODULE_DESCRIPTION("sunxi_pmc Power Button");
MODULE_AUTHOR("Carlo Caione <carlo@caione.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi_pmc-pek");
