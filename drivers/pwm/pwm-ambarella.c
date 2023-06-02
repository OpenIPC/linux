/*
 * drivers/pwm/pwm-ambarella.c
 *
 * History:
 *	2014/03/19 - [Cao Rongrong] Created
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

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <plat/pwm.h>

#define PWM_DEFAULT_FREQUENCY		2200000

struct ambarella_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	void __iomem *base2; /* for the individual pwm controller if existed */
	struct clk *clk;
	enum pwm_polarity polarity[PWM_INSTANCES];
};

#define to_ambarella_pwm_chip(_chip) container_of(_chip, struct ambarella_pwm_chip, chip)

static u32 ambpwm_enable_offset[] = {
	PWM_B0_ENABLE_OFFSET,
	PWM_B1_ENABLE_OFFSET,
	PWM_C0_ENABLE_OFFSET,
	PWM_C1_ENABLE_OFFSET,
	PWM_ENABLE_OFFSET,
};

static u32 ambpwm_divider_offset[] = {
	PWM_B0_ENABLE_OFFSET,
	PWM_B1_ENABLE_OFFSET,
	PWM_C0_ENABLE_OFFSET,
	PWM_C1_ENABLE_OFFSET,
	PWM_MODE_OFFSET,
};

static u32 ambpwm_data_offset[] = {
	PWM_B0_DATA1_OFFSET,
	PWM_B1_DATA1_OFFSET,
	PWM_C0_DATA1_OFFSET,
	PWM_C1_DATA1_OFFSET,
	PWM_CONTROL_OFFSET,
};

static bool ambpwm_is_chief[] = {true, false, true, false, false};

static int ambarella_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			int duty_ns, int period_ns)
{
	struct ambarella_pwm_chip *ambpwm = to_ambarella_pwm_chip(chip);
	void __iomem *base;
	void __iomem *reg;
	u32 tick_bits, clock, total, on, off, div;

	/* we arrange the id of the individual pwm to the last one */
	if (ambpwm->base2 && pwm->hwpwm == PWM_INSTANCES - 1) {
		base = ambpwm->base2;
		tick_bits = PWM_PWM_TICKS_MAX_BITS;
	} else {
		base = ambpwm->base;
		tick_bits = PWM_ST_TICKS_MAX_BITS;
	}

	reg = base + ambpwm_enable_offset[pwm->hwpwm];
	if (ambpwm_is_chief[pwm->hwpwm])
		amba_setbitsl(reg, PWM_CLK_SRC_BIT);
	else
		amba_clrbitsl(reg, PWM_INV_EN_BIT);

	clock = (clk_get_rate(clk_get(NULL, "gclk_pwm")) + 50000) / 100000;
	total = (clock * period_ns + 5000) / 10000;
	div = total >> tick_bits;
	clock /= (div + 1);
	reg = base + ambpwm_divider_offset[pwm->hwpwm];
	amba_clrbitsl(reg, PWM_DIVIDER_MASK);
	amba_setbitsl(reg, div << 1);

	total = (clock * period_ns + 5000) / 10000;
	on = (clock * duty_ns + 5000) / 10000;
	off = total - on;
	if (on == 0)
		on = 1;
	if (off == 0)
		off = 1;

	reg = base + ambpwm_data_offset[pwm->hwpwm];
	if (ambpwm->polarity[pwm->hwpwm] == PWM_POLARITY_NORMAL)
		amba_writel(reg, (on - 1) << tick_bits | (off - 1));
	else
		amba_writel(reg, (off - 1) << tick_bits | (on - 1));

	return 0;
}

static int ambarella_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ambarella_pwm_chip *ambpwm = to_ambarella_pwm_chip(chip);
	void __iomem *reg;

	/* we arrange the id of the individual pwm to the last one */
	if (ambpwm->base2 && pwm->hwpwm == PWM_INSTANCES - 1)
		reg = ambpwm->base2 + ambpwm_enable_offset[pwm->hwpwm];
	else
		reg = ambpwm->base + ambpwm_enable_offset[pwm->hwpwm];

	amba_setbitsl(reg, PWM_PWM_EN_BIT);

	return 0;
}

static void ambarella_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ambarella_pwm_chip *ambpwm = to_ambarella_pwm_chip(chip);
	void __iomem *reg;

	/* we arrange the id of the individual pwm to the last one */
	if (ambpwm->base2 && pwm->hwpwm == PWM_INSTANCES - 1)
		reg = ambpwm->base2 + ambpwm_enable_offset[pwm->hwpwm];
	else
		reg = ambpwm->base + ambpwm_enable_offset[pwm->hwpwm];

	amba_clrbitsl(reg, PWM_PWM_EN_BIT);
}

static int ambarella_pwm_set_polarity(struct pwm_chip *chip,
			struct pwm_device *pwm,	enum pwm_polarity polarity)
{
	struct ambarella_pwm_chip *ambpwm = to_ambarella_pwm_chip(chip);

	ambpwm->polarity[pwm->hwpwm] = polarity;

	return 0;
}

static const struct pwm_ops ambarella_pwm_ops = {
	.config		= ambarella_pwm_config,
	.enable		= ambarella_pwm_enable,
	.disable	= ambarella_pwm_disable,
	.set_polarity	= ambarella_pwm_set_polarity,
	.owner		= THIS_MODULE,
};

static int ambarella_pwm_probe(struct platform_device *pdev)
{
	struct ambarella_pwm_chip *ambpwm;
	struct resource *res;
	int ret;

	BUG_ON(ARRAY_SIZE(ambpwm_enable_offset) < PWM_INSTANCES);

	ambpwm = devm_kzalloc(&pdev->dev, sizeof(*ambpwm), GFP_KERNEL);
	if (!ambpwm)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		return -ENXIO;

	ambpwm->base = devm_ioremap(&pdev->dev,
					res->start, resource_size(res));
	if (IS_ERR(ambpwm->base))
		return PTR_ERR(ambpwm->base);

	ambpwm->base2 = NULL;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (res != NULL) {
		ambpwm->base2 = devm_ioremap(&pdev->dev,
					res->start, resource_size(res));
		if (IS_ERR(ambpwm->base2))
			return PTR_ERR(ambpwm->base);
	}

	ambpwm->clk = clk_get(NULL, "gclk_pwm");
	if (IS_ERR(ambpwm->clk))
		return PTR_ERR(ambpwm->clk);

	clk_set_rate(clk_get(NULL, "gclk_pwm"), PWM_DEFAULT_FREQUENCY);

	ambpwm->chip.dev = &pdev->dev;
	ambpwm->chip.ops = &ambarella_pwm_ops;
	ambpwm->chip.of_xlate = of_pwm_xlate_with_flags;
	ambpwm->chip.of_pwm_n_cells = 3;
	ambpwm->chip.base = -1;
	ambpwm->chip.npwm = PWM_INSTANCES;

	ret = pwmchip_add(&ambpwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add pwm chip %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, ambpwm);

	return 0;
}

static int ambarella_pwm_remove(struct platform_device *pdev)
{
	struct ambarella_pwm_chip *ambpwm = platform_get_drvdata(pdev);

	return pwmchip_remove(&ambpwm->chip);
}

#ifdef CONFIG_PM
static int ambarella_pwm_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	return 0;
}

static int ambarella_pwm_resume(struct platform_device *pdev)
{
	clk_set_rate(clk_get(NULL, "gclk_pwm"), PWM_DEFAULT_FREQUENCY);
	return 0;
}
#endif

static const struct of_device_id ambarella_pwm_dt_ids[] = {
	{ .compatible = "ambarella,pwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_pwm_dt_ids);

static struct platform_driver ambarella_pwm_driver = {
	.driver = {
		.name = "ambarella-pwm",
		.of_match_table = of_match_ptr(ambarella_pwm_dt_ids),
	},
	.probe = ambarella_pwm_probe,
	.remove = ambarella_pwm_remove,
#ifdef CONFIG_PM
	.suspend	= ambarella_pwm_suspend,
	.resume		= ambarella_pwm_resume,
#endif
};
module_platform_driver(ambarella_pwm_driver);

MODULE_ALIAS("platform:ambarella-pwm");
MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella PWM Driver");
MODULE_LICENSE("GPL v2");

