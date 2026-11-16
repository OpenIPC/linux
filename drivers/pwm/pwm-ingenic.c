/* PWM driver of Ingenic's SoC T41
 *
 * Copyright (C) 2022 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/pwm.h>
#include <soc/gpio.h>

// #define DEBUG

#define PWMCCFG0  0x000
#define PWMCCFG1  0x004
#define PWMENS    0x010
#define PWMENC    0x014
#define PWMEN     0x018
#define PWMUPDATE 0x020
#define PWMBUSYR  0x024
#define PWMLVL    0x030
#define PWMWCFG   0x0B0
#define PWMDES    0x100
#define PWMDEC    0x104
#define PWMDE     0x108
#define PWMDCR0   0x110
#define PWMDCR1   0x114
#define PWMDTRIG  0x120
#define PWMDFER   0x124
#define PWMDFSM   0x128
#define PWMDSR    0x130
#define PWMDSCR   0x134
#define PWMDINTC  0x138
#define PWMnDAR   0x140
#define PWMnDTLR  0x190
#define PWMOEN    0x300

#define CLK_1 0
#define CLK_2 1
#define CLK_4 2
#define CLK_8 3
#define CLK_16 4
#define CLK_32 5
#define CLK_64 6
#define CLK_128 7
#define CLK_n CLK_1

#define PWM_CHN_NUM 8
#define PWM_CLK_RATE 50000000
struct ingenic_pwm_gpio
{
	enum gpio_port group;
	unsigned int pins;
	enum gpio_function func;
};

struct ingenic_pwm_gpio pwm_pins[PWM_CHN_NUM] = {
#if defined(CONFIG_PWM_CHANNEL0_PB17)
	[0] = {.group = GPIO_PORT_B, .pins = 1 << 17, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL0_PC15)
	[0] = {.group = GPIO_PORT_C, .pins = 1 << 15, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL1_PB18)
	[1] = {.group = GPIO_PORT_B, .pins = 1 << 18, .func = GPIO_FUNC_0},
#elif defined(CONFIG_PWM_CHANNEL1_PC16)
	[1] = {.group = GPIO_PORT_C, .pins = 1 << 16, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL2_PB08)
	[2] = {.group = GPIO_PORT_B, .pins = 1 << 8, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL2_PC17)
	[2] = {.group = GPIO_PORT_C, .pins = 1 << 17, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL3_PB09)
	[3] = {.group = GPIO_PORT_B, .pins = 1 << 9, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL3_PC18)
	[3] = {.group = GPIO_PORT_C, .pins = 1 << 18, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL4_PB13)
	[4] = {.group = GPIO_PORT_B, .pins = 1 << 13, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL4_PD09)
	[4] = {.group = GPIO_PORT_D, .pins = 1 << 9, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL5_PB14)
	[5] = {.group = GPIO_PORT_B, .pins = 1 << 14, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL5_PD10)
	[5] = {.group = GPIO_PORT_D, .pins = 1 << 10, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL6_PB15)
	[6] = {.group = GPIO_PORT_B, .pins = 1 << 15, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL6_PD11)
	[6] = {.group = GPIO_PORT_D, .pins = 1 << 11, .func = GPIO_FUNC_2},
#endif
#if defined(CONFIG_PWM_CHANNEL7_PB16)
	[7] = {.group = GPIO_PORT_B, .pins = 1 << 16, .func = GPIO_FUNC_2},
#elif defined(CONFIG_PWM_CHANNEL7_PD12)
	[7] = {.group = GPIO_PORT_D, .pins = 1 << 12, .func = GPIO_FUNC_2},
#endif
};

struct ingenic_pwm_chip
{
	void __iomem *io_base;
	struct pwm_chip chip;
	struct ingenic_pwm_gpio *pwm_pins;
	unsigned int clk100k;
	unsigned int period_ns_max;
	unsigned int period_ns_min;
	unsigned int chn_en_mask;
	unsigned int pwm_en;
// char gpio_func[PWM_CHN_NUM];
	struct clk *clk_pwm;
	struct clk *clk_gate;
};

static int ingenic_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
	if (!(ingenic_pwm->chn_en_mask & (1 << pwm->hwpwm)))
	{
		dev_warn(chip->dev, "requested PWM channel %d not support\n", pwm->hwpwm);
		return -EINVAL;
	}
	pwm_set_chip_data(pwm, &ingenic_pwm->pwm_pins[pwm->hwpwm]);

	pwm_reg = readl(ingenic_pwm->io_base + PWMCCFG0);
	pwm_reg = pwm_reg & (~(0x0f << 4 * pwm->hwpwm));
	pwm_reg = pwm_reg | (CLK_n << 4 * pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMCCFG0);
	writel(0, ingenic_pwm->io_base + PWMDCR0);
	writel(0, ingenic_pwm->io_base + PWMDINTC);

	pwm->duty_cycle = 0;
	pwm->period = ingenic_pwm->period_ns_max;
	return 0;
}

static void ingenic_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	pwm_set_chip_data(pwm, NULL);
}

static int __pwm_config(struct ingenic_pwm_chip *ingenic_pwm, struct pwm_device *pwm,
				  uint duty_ns, uint period_ns)
{
	unsigned int high, low, period, pwm_reg;
	if (period_ns > ingenic_pwm->period_ns_max || period_ns < ingenic_pwm->period_ns_min)
	{
		printk("%s:Invalid argument\n",__func__);
		return -EINVAL;
	}
	period = (ingenic_pwm->clk100k * period_ns + 5000) / 10000;
	if(pwm->polarity == PWM_POLARITY_NORMAL)
	{
		high = (ingenic_pwm->clk100k * duty_ns + 5000) / 10000;
		low = period - high;
	}
	else
	{
		low = (ingenic_pwm->clk100k * duty_ns + 5000) / 10000;
		high = period - low;
	}
#ifdef DEBUG
	printk("high=%u,low=%u\n", high, low);
#endif
	if (high == 0)
	{
		pwm_reg = readl(ingenic_pwm->io_base + PWMLVL);
		pwm_reg &= ~(1 << pwm->hwpwm);
		writel(pwm_reg, ingenic_pwm->io_base + PWMLVL);
	}
	else if (low == 0)
	{
		pwm_reg = readl(ingenic_pwm->io_base + PWMLVL);
		pwm_reg |= (1 << pwm->hwpwm);
		writel(pwm_reg, ingenic_pwm->io_base + PWMLVL);
	}
#if 1
	if (ingenic_pwm->pwm_en & (1 << pwm->hwpwm))
	{
		while (readl(ingenic_pwm->io_base + PWMBUSYR) & (1 << pwm->hwpwm));
	}
#endif
	writel((high << 16) | low, ingenic_pwm->io_base + PWMWCFG + 4 * pwm->hwpwm);
	writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMUPDATE);
	return 0;
}

static int ingenic_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
#ifdef DEBUG
	printk("[%d]%s\n", __LINE__, __func__);
	printk("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);
#endif
	__pwm_config(ingenic_pwm, pwm, pwm->duty_cycle, pwm->period);
	writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMENS);

	pwm_reg = readl(ingenic_pwm->io_base + PWMOEN);
	pwm_reg |= (1 << pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMOEN);

	ingenic_pwm->pwm_en |= 1 << pwm->hwpwm;

	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->group, pwm_pin->func, pwm_pin->pins);
	// ingenic_pwm->gpio_func[pwm->hwpwm] = 1;
	return 0;
}

static void ingenic_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int pwm_reg;
	struct ingenic_pwm_gpio *pwm_pin;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
#ifdef DEBUG
	printk("[%d]%s\n", __LINE__, __func__);
	printk("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);
#endif
	writel(1 << pwm->hwpwm, ingenic_pwm->io_base + PWMENC);

	pwm_reg = readl(ingenic_pwm->io_base + PWMOEN);
	pwm_reg &= ~(1 << pwm->hwpwm);
	writel(pwm_reg, ingenic_pwm->io_base + PWMOEN);

	ingenic_pwm->pwm_en &= ~(1 << pwm->hwpwm);
	pwm_pin = pwm->chip_data;
	jzgpio_set_func(pwm_pin->group, pwm->polarity ? GPIO_OUTPUT1:GPIO_OUTPUT0, pwm_pin->pins);
	// ingenic_pwm->gpio_func[pwm->hwpwm] = 0;
}

static int ingenic_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,int duty_ns, int period_ns)
{
	int ret = 0;
	struct ingenic_pwm_chip *ingenic_pwm = container_of(chip, struct ingenic_pwm_chip, chip);
#ifdef DEBUG
	printk("[%d]%s:duty=%d,period=%d\n", __LINE__, __func__, duty_ns, period_ns);
	printk("period=%u,duty=%u,polarity=%d\n", pwm->period, pwm->duty_cycle, (int)pwm->polarity);
#endif
	ret = __pwm_config(ingenic_pwm, pwm, duty_ns, period_ns);
	return ret;
}

static int ingenic_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
#ifdef DEBUG
	printk("[%d]%s:polarity=%d\n", __LINE__, __func__, polarity);
#endif
	return 0;
}

static const struct pwm_ops ingenic_pwm_ops = {
	.request = ingenic_pwm_request,
	.free = ingenic_pwm_free,
	.enable = ingenic_pwm_enable,
	.disable = ingenic_pwm_disable,
	.config = ingenic_pwm_config,
	.set_polarity = ingenic_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int ingenic_pwm_probe(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm;
	struct resource *base;
	int ret = 0;
	ingenic_pwm = devm_kzalloc(&pdev->dev, sizeof(struct ingenic_pwm_chip), GFP_KERNEL);
	if (ingenic_pwm == NULL)
		return -ENOMEM;

	base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ingenic_pwm->io_base = devm_ioremap_resource(&pdev->dev, base);
	if (IS_ERR(ingenic_pwm->io_base))
		return PTR_ERR(ingenic_pwm->io_base);

	ingenic_pwm->chip.dev = &pdev->dev;
	ingenic_pwm->chip.ops = &ingenic_pwm_ops;
	ingenic_pwm->chip.npwm = PWM_CHN_NUM;
	ingenic_pwm->chip.can_sleep = true;
	ingenic_pwm->chip.base = -1;

#ifdef CONFIG_FPGA_TEST
	ingenic_pwm->clk100k = 25000000 / (1 << CLK_n) / 100000; // unit:100KHz
#else
	ingenic_pwm->clk_gate = devm_clk_get(&pdev->dev, "gate_pwm");
	if (IS_ERR(ingenic_pwm->clk_gate)){
		dev_err(&pdev->dev, "get pwm clk gate failed %ld\n", PTR_ERR(ingenic_pwm->clk_gate));
		return PTR_ERR(ingenic_pwm->clk_gate);
	}

	ingenic_pwm->clk_pwm = devm_clk_get(&pdev->dev, "div_pwm");
	if (IS_ERR(ingenic_pwm->clk_pwm)){
		dev_err(&pdev->dev, "get pwm clk failed %ld\n", PTR_ERR(ingenic_pwm->clk_pwm));
		return PTR_ERR(ingenic_pwm->clk_pwm);
	}

	if (ingenic_pwm->clk_gate)
	{
		ret = clk_prepare_enable(ingenic_pwm->clk_gate);
		if (ret){
			dev_err(&pdev->dev, "enable pwm clock gate failed!\n");
		}
	}

	if (ingenic_pwm->clk_pwm){
		ret = clk_set_rate(ingenic_pwm->clk_pwm, PWM_CLK_RATE);
		if (ret){
			dev_err(&pdev->dev, "set pwm clock rate failed!\n");
		}
		ret = clk_prepare_enable(ingenic_pwm->clk_pwm);
		if (ret){
			dev_err(&pdev->dev, "enable pwm clock failed!\n");
		}
	}
	ingenic_pwm->clk100k = clk_get_rate(ingenic_pwm->clk_pwm) / (1 << CLK_n) / 100000; // unit:100KHz
	dev_info(&pdev->dev, "PWM clk100k:%u", ingenic_pwm->clk100k);
#endif
	ingenic_pwm->pwm_pins = pwm_pins;
	ingenic_pwm->chn_en_mask = 0;
#ifdef CONFIG_PWM_CHANNEL0_ENABLE
	ingenic_pwm->chn_en_mask |= 1;
#endif
#ifdef CONFIG_PWM_CHANNEL1_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 1;
#endif
#ifdef CONFIG_PWM_CHANNEL2_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 2;
#endif
#ifdef CONFIG_PWM_CHANNEL3_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 3;
#endif
#ifdef CONFIG_PWM_CHANNEL4_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 4;
#endif
#ifdef CONFIG_PWM_CHANNEL5_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 5;
#endif
#ifdef CONFIG_PWM_CHANNEL6_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 6;
#endif
#ifdef CONFIG_PWM_CHANNEL7_ENABLE
	ingenic_pwm->chn_en_mask |= 1 << 7;
#endif
	ingenic_pwm->pwm_en = 0;
	// memset(ingenic_pwm->gpio_func, 0, PWM_CHN_NUM);

	ingenic_pwm->period_ns_max = 10000 * 0xffff / ingenic_pwm->clk100k;
	ingenic_pwm->period_ns_min = 10000 * 2 / ingenic_pwm->clk100k + 1;
	dev_info(&pdev->dev, "period_ns:max=%u,min=%u\n",ingenic_pwm->period_ns_max,ingenic_pwm->period_ns_min);
	ret = pwmchip_add(&ingenic_pwm->chip);
	if (ret < 0)
	{
		dev_err(&pdev->dev, "failed to add PWM chip %d\n", ret);
		return ret;
	}
	platform_set_drvdata(pdev, ingenic_pwm);

	return 0;
}

static int ingenic_pwm_remove(struct platform_device *pdev)
{
	struct ingenic_pwm_chip *ingenic_pwm = platform_get_drvdata(pdev);
	return pwmchip_remove(&ingenic_pwm->chip);
}

#ifdef CONFIG_PM_SLEEP
static int ingenic_pwm_suspend(struct device *dev)
{
	int i;
	struct pwm_device *pwm;
	struct ingenic_pwm_chip *ingenic_pwm = dev_get_drvdata(dev);
	for (i = 0; i < PWM_CHN_NUM; ++i)
	{
		pwm = &ingenic_pwm->chip.pwms[i];
		if (test_bit(PWMF_REQUESTED, &pwm->flags))
		{
			if (test_bit(PWMF_ENABLED, &pwm->flags))
			{
				ingenic_pwm->pwm_en &= ~(1 << pwm->hwpwm);
			}
		}
	}

	clk_disable_unprepare(ingenic_pwm->clk_gate);
	clk_disable_unprepare(ingenic_pwm->clk_pwm);

	return 0;
}

static int ingenic_pwm_resume(struct device *dev)
{
	int i,ret;
	unsigned int pwm_reg;
	struct pwm_device *pwm;
	struct ingenic_pwm_chip *ingenic_pwm = dev_get_drvdata(dev);

	ret = clk_prepare_enable(ingenic_pwm->clk_gate);
	if (ret){
		dev_err(dev, "enable pwm clock gate failed!\n");
	}
	ret = clk_set_rate(ingenic_pwm->clk_pwm, PWM_CLK_RATE);
	if (ret){
		dev_err(dev, "set pwm clock rate failed!\n");
	}
	ret = clk_prepare_enable(ingenic_pwm->clk_pwm);
	if (ret){
		dev_err(dev, "enable pwm clock failed!\n");
	}
#ifdef DEBUG
	printk("PWM clk:%lu\n", clk_get_rate(ingenic_pwm->clk_pwm));
#endif
	for (i = 0; i < PWM_CHN_NUM; ++i)
	{
		pwm = &ingenic_pwm->chip.pwms[i];
		if (test_bit(PWMF_REQUESTED, &pwm->flags))
		{
			pwm_reg = readl(ingenic_pwm->io_base + PWMCCFG0);
			pwm_reg = pwm_reg & (~(0x0f << 4 * pwm->hwpwm));
			pwm_reg = pwm_reg | (CLK_n << 4 * pwm->hwpwm);
			writel(pwm_reg, ingenic_pwm->io_base + PWMCCFG0);
			writel(0, ingenic_pwm->io_base + PWMDCR0);
			writel(0, ingenic_pwm->io_base + PWMDINTC);
			if (test_bit(PWMF_ENABLED, &pwm->flags))
			{
				ingenic_pwm_enable(&ingenic_pwm->chip, pwm);
			}
		}
	}
	return 0;
}
static SIMPLE_DEV_PM_OPS(ingenic_pwm_pm_ops, ingenic_pwm_suspend, ingenic_pwm_resume);
#endif

#ifdef CONFIG_OF
static const struct of_device_id ingenic_pwm_matches[] = {
	{.compatible = "ingenic,t41-pwm", .data = NULL},
	{.compatible = "ingenic,a1-pwm", .data = NULL},
	{.compatible = "x2000-pwm", .data = NULL},
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_pwm_matches);
#else
static struct platform_device_id pla_pwm_ids[] = {
	{.name = "ingenic,t41-pwm", .driver_data = 0},
	{},
};
MODULE_DEVICE_TABLE(platform, pla_pwm_ids);
#endif

static struct platform_driver ingenic_pwm_driver = {
	.driver = {
		.name = "ingenic,t41-pwm",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ingenic_pwm_matches),
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &ingenic_pwm_pm_ops,
#endif
	},
#ifndef CONFIG_OF
	.id_table = pla_pwm_ids,
#endif
	.probe = ingenic_pwm_probe,
	.remove = ingenic_pwm_remove,
};

static int __init ingenic_pwm_init(void)
{
	return platform_driver_register(&ingenic_pwm_driver);
}

static void __exit ingenic_pwm_exit(void)
{
	platform_driver_unregister(&ingenic_pwm_driver);
}

module_init(ingenic_pwm_init);
module_exit(ingenic_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Ingenic SoC PWM driver");
MODULE_ALIAS("platform:ingenic-pwm");
