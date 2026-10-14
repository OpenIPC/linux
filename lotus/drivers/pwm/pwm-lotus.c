// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/reset.h>

#define PWM_CLOCK_ADDR_BASE 0x120101BC
#    define PWM_CLOCK_VALUE_SHIFT   8
#    define PWM_CLOCK_VALUE_MASK    GENMASK(9,8)
#    define PWM_CLOCK_ENABLE_SHIFT  7
#    define PWM_CLOCK_ENABLE_MASK   BIT(7)
#define PWM_CLOCK_3M 3000000
#define PWM_CLOCK_24M 24000000
#define PWM_CLOCK_50M 50000000

#define PWM_CTRL_ADDR(x) (((x)*0x100) + 0x0)
#define PWM_CFG_PULSE_ADDR(x) (((x)*0x100) + 0x10)
#define PWM_CFG_STAT_PLUSE_ADDR(x) (((x)*0x100) + 0x14)
#define PWM_CFG_HLINT_ADDR(x) (((x)*0x100) + 0x20)
#define PWM_CFG_STAT_HLINT_ADDR(x) (((x)*0x100) + 0x24)
#define PWM_CFG_CYCLE_ADDR(x) (((x)*0x100) + 0x30)
#define PWM_CFG_STAT_CYCLE_ADDR(x) (((x)*0x100) + 0x34)

#define PWM_ENABLE_SHIFT 2
#define PWM_ENABLE_MASK  BIT(2)

#define PWM_POLARITY_SHIFT 1
#define PWM_POLARITY_MASK  BIT(1)

#define PWM_KEEP_SHIFT 0
#define PWM_KEEP_MASK  BIT(0)

#define PWM_PERIOD_MASK GENMASK(31, 0)
#define PWM_DUTY_MASK   GENMASK(31, 0)

#if defined(CONFIG_ARCH_XMFALCON)
#define PWM_NUM    17
#elif defined(CONFIG_ARCH_XMORCA)
#define PWM_NUM    11
#else
#error "PWM_NUM MUST be defined!\n"
#endif

struct lotus_pwm_chip {
	struct pwm_chip chip;
	unsigned long clk;
	void __iomem *base;
	u32 suspend_ch_ctrl[PWM_NUM];
	u32 suspend_ch_hlint[PWM_NUM];
	u32 suspend_ch_cycle[PWM_NUM];
};

struct lotus_pwm_soc {
	u32 num_pwms;
};

static const struct lotus_pwm_soc pwm_soc[1] = {
	{ .num_pwms = PWM_NUM },
};

static inline struct lotus_pwm_chip *to_lotus_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct lotus_pwm_chip, chip);
}

static void lotus_pwm_set_bits(void __iomem *base, u32 offset,
							   u32 mask, u32 data)
{
	void __iomem *address = base + offset;
	u32 value;

	value = readl(address);
	value &= ~mask;
	value |= (data & mask);
	writel(value, address);
}

static void lotus_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct lotus_pwm_chip *bsp_pwm_chip = to_lotus_pwm_chip(chip);

	lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CTRL_ADDR(pwm->hwpwm),
					  PWM_ENABLE_MASK,(0x1 << PWM_ENABLE_SHIFT));
}

static void lotus_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct lotus_pwm_chip *bsp_pwm_chip = to_lotus_pwm_chip(chip);

	lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CTRL_ADDR(pwm->hwpwm),
					  PWM_ENABLE_MASK, (0x0 << PWM_ENABLE_SHIFT));
}

static void lotus_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
							 int duty_cycle_ns, int period_ns)
{
	struct lotus_pwm_chip *bsp_pwm_chip = to_lotus_pwm_chip(chip);
	u32 duty;
	u64 period, freq;

	freq = div_u64(bsp_pwm_chip->clk, 1000000);

	period = div_u64(freq * period_ns, 1000);
	duty = div_u64(period * duty_cycle_ns, period_ns);

	lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CFG_CYCLE_ADDR(pwm->hwpwm),
					  PWM_PERIOD_MASK, period);

	lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CFG_HLINT_ADDR(pwm->hwpwm),
					  PWM_DUTY_MASK, duty);
}

static void lotus_pwm_set_polarity(struct pwm_chip *chip,
								  struct pwm_device *pwm,
								  enum pwm_polarity polarity)
{
	struct lotus_pwm_chip *bsp_pwm_chip = to_lotus_pwm_chip(chip);

	if (polarity == PWM_POLARITY_INVERSED)
		lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CTRL_ADDR(pwm->hwpwm),
						   PWM_POLARITY_MASK, (0x1 << PWM_POLARITY_SHIFT));
	else
		lotus_pwm_set_bits(bsp_pwm_chip->base, PWM_CTRL_ADDR(pwm->hwpwm),
						   PWM_POLARITY_MASK, (0x0 << PWM_POLARITY_SHIFT));
}

static void lotus_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
								struct pwm_state *state)
{
	struct lotus_pwm_chip *bsp_pwm_chip = to_lotus_pwm_chip(chip);
	void __iomem *base;
	u32 freq;
	u64 value;

	freq = div_u64(bsp_pwm_chip->clk, 1000000);
	base = bsp_pwm_chip->base;

	value = readl(base + PWM_CFG_STAT_CYCLE_ADDR(pwm->hwpwm));
	state->period = div_u64(value * 1000, freq);

	value = readl(base + PWM_CFG_STAT_HLINT_ADDR(pwm->hwpwm));
	state->duty_cycle = div_u64(value * 1000, freq);

	value = readl(base + PWM_CTRL_ADDR(pwm->hwpwm));
	state->enabled = ((PWM_ENABLE_MASK & value) >> PWM_ENABLE_SHIFT);
}

static int lotus_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			  const struct pwm_state *state)
{
	if (state->duty_cycle == 0) {
		pr_err("duty_cycle can't zero\n");
		return -EINVAL;
	}
	if (state->polarity != pwm->state.polarity)
		lotus_pwm_set_polarity(chip, pwm, state->polarity);

	if (state->period != pwm->state.period ||
		state->duty_cycle != pwm->state.duty_cycle) {
		lotus_pwm_config(chip, pwm, state->duty_cycle, state->period);
	}

	if (state->enabled)
		lotus_pwm_enable(chip, pwm);
	else
		lotus_pwm_disable(chip, pwm);

	return 0;
}

static void lotus_pwm_get_clk(unsigned long *clk)
{
	void *pwm_addr = (void *)ioremap(PWM_CLOCK_ADDR_BASE, 0x4);
	unsigned int val = readl(pwm_addr);

	val = ((val & PWM_CLOCK_VALUE_MASK) >> PWM_CLOCK_VALUE_SHIFT);
	switch (val) {
	case 0:
		*clk = PWM_CLOCK_3M;
		break;
	case 1:
		*clk = PWM_CLOCK_50M;
		break;
	default:
		*clk = PWM_CLOCK_24M;
		break;
	}
	iounmap(pwm_addr);
}

static void __maybe_unused lotus_pwm_clk_disable(void)
{
	void *pwm_addr = (void *)ioremap(PWM_CLOCK_ADDR_BASE, 0x4);

	lotus_pwm_set_bits(pwm_addr, 0x0, PWM_CLOCK_ENABLE_MASK, (0x0 << PWM_CLOCK_ENABLE_SHIFT));

	iounmap(pwm_addr);
}

static void __maybe_unused lotus_pwm_clk_enable(void)
{
	void *pwm_addr = (void *)ioremap(PWM_CLOCK_ADDR_BASE, 0x4);

	lotus_pwm_set_bits(pwm_addr, 0x0, PWM_CLOCK_ENABLE_MASK, (0x1 << PWM_CLOCK_ENABLE_SHIFT));

	iounmap(pwm_addr);
}

#ifdef CONFIG_PM_SLEEP
static int lotus_pwm_suspend(struct device *dev)
{
	struct lotus_pwm_chip *pwm_chip = dev_get_drvdata(dev);
	int i;

	pr_debug("%s: enter!\n", __func__);

	for (i = 0; i < pwm_chip->chip.npwm; i++) {
		pwm_chip->suspend_ch_ctrl[i] = readl(pwm_chip->base + PWM_CTRL_ADDR(i));
		pwm_chip->suspend_ch_cycle[i] = readl(pwm_chip->base + PWM_CFG_CYCLE_ADDR(i));
		pwm_chip->suspend_ch_hlint[i] = readl(pwm_chip->base + PWM_CFG_HLINT_ADDR(i));
	}

	lotus_pwm_clk_disable();

	pr_debug("%s: ok!\n", __func__);

	return 0;
}

static int lotus_pwm_resume(struct device *dev)
{
	struct lotus_pwm_chip *pwm_chip = dev_get_drvdata(dev);
	int i;

	pr_debug("%s: enter!\n", __func__);

	lotus_pwm_clk_enable();

	for (i = 0; i < pwm_chip->chip.npwm; i++) {
		writel(pwm_chip->suspend_ch_hlint[i], pwm_chip->base + PWM_CFG_HLINT_ADDR(i));
		writel(pwm_chip->suspend_ch_cycle[i], pwm_chip->base + PWM_CFG_CYCLE_ADDR(i));
		writel(pwm_chip->suspend_ch_ctrl[i] & ~PWM_ENABLE_MASK, pwm_chip->base + PWM_CTRL_ADDR(i));
		if (pwm_chip->suspend_ch_ctrl[i] & PWM_ENABLE_MASK) {
			writel(readl(pwm_chip->base + PWM_CTRL_ADDR(i)) | PWM_ENABLE_MASK,
					pwm_chip->base + PWM_CTRL_ADDR(i));
		}
	}

	pr_debug("%s: ok!\n", __func__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(lotus_pwm_dev_pm, lotus_pwm_suspend,
			 lotus_pwm_resume);
#endif /* CONFIG_PM */

static struct pwm_ops lotus_pwm_ops = {
	.get_state = lotus_pwm_get_state,
	.apply = lotus_pwm_apply,
	.owner = THIS_MODULE,
};

static int lotus_pwm_probe(struct platform_device *pdev)
{
	const struct lotus_pwm_soc *soc =
		of_device_get_match_data(&pdev->dev);
	struct lotus_pwm_chip *pwm_chip;
	struct resource *res;
	int ret;
	int i;

	pwm_chip = devm_kzalloc(&pdev->dev, sizeof(*pwm_chip), GFP_KERNEL);
	if (pwm_chip == NULL)
		return -ENOMEM;

	pwm_chip->clk = 0;
	lotus_pwm_get_clk(&(pwm_chip->clk));
	if (!pwm_chip->clk) {
		dev_err(&pdev->dev, "getting clock failed with %lu\n", pwm_chip->clk);
		return -EINVAL;
	}

	pwm_chip->chip.ops = &lotus_pwm_ops;
	pwm_chip->chip.dev = &pdev->dev;
	pwm_chip->chip.base = -1;
	pwm_chip->chip.npwm = soc->num_pwms;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pwm_chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pwm_chip->base))
		return PTR_ERR(pwm_chip->base);

	ret = pwmchip_add(&pwm_chip->chip);
	if (ret < 0)
		return ret;

	for (i = 0; i < pwm_chip->chip.npwm; i++) {
		lotus_pwm_set_bits(pwm_chip->base, PWM_CTRL_ADDR(i),
						   PWM_KEEP_MASK, (0x1 << PWM_KEEP_SHIFT));
	}

	platform_set_drvdata(pdev, pwm_chip);

	return 0;
}

static int lotus_pwm_remove(struct platform_device *pdev)
{
	struct lotus_pwm_chip *pwm_chip;

	pwm_chip = platform_get_drvdata(pdev);

	return pwmchip_remove(&pwm_chip->chip);
}

static const struct of_device_id lotus_pwm_of_match[] = {
	{ .compatible = "lotus,lotus-pwm" },
	{ .compatible = "lotus,pwm", .data = &pwm_soc[0] }, {}
};
MODULE_DEVICE_TABLE(of, lotus_pwm_of_match);

static struct platform_driver lotus_pwm_driver = {
	.driver = {
		.name = "lotus-pwm",
#ifdef CONFIG_PM_SLEEP
		.pm = &lotus_pwm_dev_pm,
#endif
		.of_match_table = lotus_pwm_of_match,
	},
	.probe = lotus_pwm_probe,
	.remove = lotus_pwm_remove,
};
module_platform_driver(lotus_pwm_driver);

MODULE_AUTHOR("lotus");
MODULE_DESCRIPTION("lotus SoCs PWM driver");
MODULE_LICENSE("GPL");
