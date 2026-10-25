// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 PWM.
 *
 * Fourteen channels with a 0x100 register stride. Tick counts are the
 * period divided by the truncated tick length and must be below 2^25.
 * Phase, pause, repeat and the finish IRQ are not used.
 *
 * Copyright (C) 2026 Difan Zhang
 */
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define FH_PWM_CHANNELS		14
#define FH_PWM_RUN		0x00
#define FH_PWM_STOP		0x04
#define FH_PWM_ENABLE		0x08
#define FH_PWM_CH_STRIDE	0x100
#define FH_PWM_CH_BASE		0x100
#define FH_PWM_CTRL		0x00
#define FH_PWM_PERIOD		0x04
#define FH_PWM_DUTY		0x08
#define FH_PWM_PHASE		0x0c
#define FH_PWM_PAUSE		0x10
#define FH_PWM_REPEAT		0x14
#define FH_PWM_CTRL_CONTINUOUS	1
#define FH_PWM_COUNT_LIMIT	0x02000000

struct fh_pwm_chip {
	struct pwm_chip chip;
	struct clk *clk;
	void __iomem *base;
	spinlock_t lock;
};

static inline struct fh_pwm_chip *to_fh_pwm(struct pwm_chip *chip)
{
	return container_of(chip, struct fh_pwm_chip, chip);
}

static u32 fh_pwm_ch(unsigned int ch, u32 off)
{
	return FH_PWM_CH_BASE + ch * FH_PWM_CH_STRIDE + off;
}

static u32 fh_pwm_stop_bit(unsigned int ch)
{
	return (ch / 8) * 16 + (ch % 8);
}

static u64 fh_pwm_ticks_to_ns(u32 ticks, unsigned long rate)
{
	if (!rate)
		return 0;
	return DIV_ROUND_UP_ULL((u64)ticks * NSEC_PER_SEC, rate);
}

static int fh_pwm_ns_to_ticks(u64 ns, unsigned long rate, u32 *ticks)
{
	u32 tick_ns;
	u64 count;

	if (!rate)
		return -EINVAL;
	tick_ns = NSEC_PER_SEC / rate;
	if (!tick_ns)
		return -EINVAL;
	count = div_u64(ns, tick_ns);
	if (ns && !count)
		return -EINVAL;
	if (count >= FH_PWM_COUNT_LIMIT)
		return -EINVAL;
	*ticks = count;
	return 0;
}

static int fh_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct fh_pwm_chip *pc = to_fh_pwm(chip);
	unsigned int ch = pwm->hwpwm;
	u32 ctrl, en;

	en = readl(pc->base + FH_PWM_ENABLE);
	ctrl = readl(pc->base + fh_pwm_ch(ch, FH_PWM_CTRL));
	if ((en & BIT(ch)) && ctrl != FH_PWM_CTRL_CONTINUOUS)
		return -EINVAL;
	return 0;
}

static void fh_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
			     struct pwm_state *state)
{
	struct fh_pwm_chip *pc = to_fh_pwm(chip);
	unsigned int ch = pwm->hwpwm;
	unsigned long rate = clk_get_rate(pc->clk);
	u32 en, period, duty;

	en = readl(pc->base + FH_PWM_ENABLE);
	period = readl(pc->base + fh_pwm_ch(ch, FH_PWM_PERIOD));
	duty = readl(pc->base + fh_pwm_ch(ch, FH_PWM_DUTY));
	state->enabled = !!(en & BIT(ch));
	state->polarity = PWM_POLARITY_NORMAL;
	state->period = fh_pwm_ticks_to_ns(period, rate);
	state->duty_cycle = fh_pwm_ticks_to_ns(duty, rate);
}

static int fh_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			struct pwm_state *state)
{
	struct fh_pwm_chip *pc = to_fh_pwm(chip);
	unsigned int ch = pwm->hwpwm;
	unsigned long flags, rate;
	u32 period, duty, run, en, stop, stop_bit;

	if (state->polarity != PWM_POLARITY_NORMAL)
		return -EINVAL;
	if (state->duty_cycle > state->period)
		return -EINVAL;

	if (!state->enabled) {
		spin_lock_irqsave(&pc->lock, flags);
		en = readl(pc->base + FH_PWM_ENABLE);
		writel(en & ~BIT(ch), pc->base + FH_PWM_ENABLE);
		run = readl(pc->base + FH_PWM_RUN);
		writel(run & ~BIT(ch), pc->base + FH_PWM_RUN);
		spin_unlock_irqrestore(&pc->lock, flags);
		return 0;
	}

	rate = clk_get_rate(pc->clk);
	if (fh_pwm_ns_to_ticks(state->period, rate, &period) ||
	    fh_pwm_ns_to_ticks(state->duty_cycle, rate, &duty))
		return -EINVAL;
	if (!period || duty > period)
		return -EINVAL;

	stop_bit = fh_pwm_stop_bit(ch);
	spin_lock_irqsave(&pc->lock, flags);
	run = readl(pc->base + FH_PWM_RUN);
	writel(run & ~BIT(ch), pc->base + FH_PWM_RUN);
	writel(FH_PWM_CTRL_CONTINUOUS, pc->base + fh_pwm_ch(ch, FH_PWM_CTRL));
	writel(period, pc->base + fh_pwm_ch(ch, FH_PWM_PERIOD));
	writel(duty, pc->base + fh_pwm_ch(ch, FH_PWM_DUTY));
	writel(0, pc->base + fh_pwm_ch(ch, FH_PWM_PHASE));
	writel(0, pc->base + fh_pwm_ch(ch, FH_PWM_PAUSE));
	writel(0, pc->base + fh_pwm_ch(ch, FH_PWM_REPEAT));
	stop = readl(pc->base + FH_PWM_STOP);
	stop &= ~(BIT(stop_bit) | BIT(stop_bit + 8));
	writel(stop, pc->base + FH_PWM_STOP);
	en = readl(pc->base + FH_PWM_ENABLE);
	run = readl(pc->base + FH_PWM_RUN);
	writel(run | BIT(ch), pc->base + FH_PWM_RUN);
	writel(en | BIT(ch), pc->base + FH_PWM_ENABLE);
	spin_unlock_irqrestore(&pc->lock, flags);

	state->period = fh_pwm_ticks_to_ns(period, rate);
	state->duty_cycle = fh_pwm_ticks_to_ns(duty, rate);
	return 0;
}

static const struct pwm_ops fh_pwm_ops = {
	.request = fh_pwm_request,
	.apply = fh_pwm_apply,
	.get_state = fh_pwm_get_state,
	.owner = THIS_MODULE,
};

static int fh_pwm_probe(struct platform_device *pdev)
{
	struct fh_pwm_chip *pc;
	struct resource *res;
	int ret;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;
	spin_lock_init(&pc->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pc->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pc->base))
		return PTR_ERR(pc->base);

	pc->clk = devm_clk_get(&pdev->dev, "pwm");
	if (IS_ERR(pc->clk))
		return PTR_ERR(pc->clk);
	ret = clk_prepare_enable(pc->clk);
	if (ret)
		return ret;

	pc->chip.dev = &pdev->dev;
	pc->chip.ops = &fh_pwm_ops;
	pc->chip.base = -1;
	pc->chip.npwm = FH_PWM_CHANNELS;
	pc->chip.of_xlate = of_pwm_xlate_with_flags;
	pc->chip.of_pwm_n_cells = 3;

	ret = pwmchip_add(&pc->chip);
	if (ret) {
		clk_disable_unprepare(pc->clk);
		return ret;
	}
	platform_set_drvdata(pdev, pc);
	return 0;
}

static int fh_pwm_remove(struct platform_device *pdev)
{
	struct fh_pwm_chip *pc = platform_get_drvdata(pdev);

	pwmchip_remove(&pc->chip);
	clk_disable_unprepare(pc->clk);
	return 0;
}

static const struct of_device_id fh_pwm_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-pwm" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_pwm_of_match);

static struct platform_driver fh_pwm_driver = {
	.probe = fh_pwm_probe,
	.remove = fh_pwm_remove,
	.driver = {
		.name = "fh8626v100-pwm",
		.of_match_table = fh_pwm_of_match,
	},
};
module_platform_driver(fh_pwm_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 PWM");
MODULE_LICENSE("GPL v2");
