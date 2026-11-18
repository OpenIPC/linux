/*
 * Axera pwm driver
 *
 * Copyright (c) 2019-2020 Axera Technology Co., Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
//#define DEBUG
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/clk-provider.h>

#define CHANNELS_PER_PWM		4
#define PWM_TIMERN_LOADCOUNT_OFF(N)	(0x0 + (N) * 0x14)
#define PWM_TIMERN_CONTROLREG_OFF(N)	(0x8 + (N) * 0x14)
#define PWM_TIMERN_LOADCOUNT2_OFF(N)	(0xB0 + (N) * 0x4)
#define PWM_TIMERN_MODE			0x1E	/* PWM mode but not enable */
#define PWM_TIMERN_EN			0x1	/* PWM enable bit */

#define CHANNEL_CLK_SEL_FREQ		24000000	/* 24MHZ */

#undef USE_LINUX_CLK

struct pwm_pin_setting{
	ulong reg;
	u32 sleep_setting;
	u32 save_setting;
};

struct axera_pwm_chip {
	struct pwm_chip chip;
	struct resource *pc_resource;
	struct clk *pwm_clk[CHANNELS_PER_PWM];
	struct clk *pwm_pclk;
	u32 suspend_cfg[CHANNELS_PER_PWM][4];
	struct reset_control *glb_rst;
	struct reset_control *ch_rst[CHANNELS_PER_PWM];
	struct pwm_pin_setting pin_sleep_cfg[CHANNELS_PER_PWM];
	u32 pin_cfg_len;
	void __iomem *slp_timer_base;
	void __iomem	*clk_base;
	unsigned int	clk_sel_addr_offset;
	unsigned int	clk_sel_offset;
	unsigned int	clk_glb_eb_addr_offset;
	unsigned int	clk_glb_eb_offset;
	unsigned int	clk_eb_addr_offset;
	unsigned int	clk_eb_offset[CHANNELS_PER_PWM];
	unsigned int	clk_eb_flag[CHANNELS_PER_PWM];
	unsigned int	clk_p_eb_addr_offset;
	unsigned int	clk_p_eb_offset;
};

static inline struct axera_pwm_chip *to_axera_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct axera_pwm_chip, chip);
}

void timer_clk_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int val;
	struct axera_pwm_chip *our_chip = to_axera_pwm_chip(chip);
	val = (1 << our_chip->clk_eb_offset[pwm->hwpwm]);
	/*set clk_eb_1_set register*/
	writel(val, our_chip->clk_eb_addr_offset + our_chip->clk_base + 0xB0);
	our_chip->clk_eb_flag[pwm->hwpwm] = 1;
}

void timer_clk_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	unsigned int index;
	unsigned int val;
	struct axera_pwm_chip *our_chip = to_axera_pwm_chip(chip);

	val = (1 << our_chip->clk_eb_offset[pwm->hwpwm]);
	/*set clk_eb_1_clr register*/
	writel(val, our_chip->clk_eb_addr_offset + our_chip->clk_base + 0xB4);
	our_chip->clk_eb_flag[pwm->hwpwm] = 0;

	for(index = 0; index < CHANNELS_PER_PWM; index++) {
		if (our_chip->clk_eb_flag[index] == 1) {
			break;
		}
	}
}

static int pwm_axera_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct axera_pwm_chip *our_chip = to_axera_pwm_chip(chip);
	void __iomem *pwm_timer_base;
	u32 reg;
#ifdef USE_LINUX_CLK
	int ret;
#endif

	/* use reset interface */
	reset_control_deassert(our_chip->ch_rst[pwm->hwpwm]);

#ifdef USE_LINUX_CLK
	ret = clk_prepare_enable(our_chip->pwm_pclk);

	if (ret) {
		pr_err("%s %s chan %u pclk prepare enable fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return ret;
	}

	ret = clk_prepare_enable(our_chip->pwm_clk[pwm->hwpwm]);

	if (ret) {
		pr_err("%s %s chan %u clk prepare enable fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return ret;
	}

	clk_set_rate(our_chip->pwm_clk[pwm->hwpwm], CHANNEL_CLK_SEL_FREQ);
#else
	timer_clk_enable(chip, pwm);
#endif

	pwm_timer_base = devm_ioremap(chip->dev, our_chip->pc_resource->start,
				      resource_size(our_chip->pc_resource));
	if (IS_ERR(pwm_timer_base)) {
		pr_err("%s %s chan %u ioremap source fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return PTR_ERR(pwm_timer_base);
	}

	reg = readl(pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));
	reg |= PWM_TIMERN_EN;
	writel(reg, pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));

	devm_iounmap(chip->dev, pwm_timer_base);
	return 0;
}

static void pwm_axera_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct axera_pwm_chip *our_chip = to_axera_pwm_chip(chip);
	void __iomem *pwm_timer_base;
	u32 reg;
#ifdef USE_LINUX_CLK
	int chan_id = 0;
#endif

	pwm_timer_base = devm_ioremap(chip->dev, our_chip->pc_resource->start,
				      resource_size(our_chip->pc_resource));
	if (IS_ERR(pwm_timer_base)) {
		pr_err("%s %s chan %u ioremap source fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return;
	}

	reg = readl(pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));
	reg &= ~PWM_TIMERN_EN;
	writel(reg, pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));

	devm_iounmap(chip->dev, pwm_timer_base);

#ifdef USE_LINUX_CLK
	clk_disable_unprepare(our_chip->pwm_clk[pwm->hwpwm]);
	for(chan_id = 0; chan_id < CHANNELS_PER_PWM; chan_id++) {
		if (__clk_is_enabled(our_chip->pwm_clk[chan_id])) {
			break;
		}
	}
	if (chan_id == CHANNELS_PER_PWM) {
		clk_disable_unprepare(our_chip->pwm_pclk);
	}
#else
	timer_clk_disable(chip, pwm);
#endif
}

static int pwm_axera_config(struct pwm_chip *chip, struct pwm_device *pwm,
			    int duty_ns, int period_ns)
{
	struct axera_pwm_chip *our_chip = to_axera_pwm_chip(chip);
	u64 temp;
	u32 period_count;
	u32 duty_count;
	void __iomem *pwm_timer_base;
	struct pwm_state cur_state;
#ifdef USE_LINUX_CLK
	int ret;
#endif

	/* use reset interface */
	reset_control_deassert(our_chip->ch_rst[pwm->hwpwm]);

#ifdef USE_LINUX_CLK
	ret = clk_prepare_enable(our_chip->pwm_pclk);

	if (ret) {
		pr_err("%s %s chan %u pclk prepare enable fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return ret;
	}

	ret = clk_prepare_enable(our_chip->pwm_clk[pwm->hwpwm]);

	if (ret) {
		pr_err("%s %s chan %u clk prepare enable fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return ret;
	}

	clk_set_rate(our_chip->pwm_clk[pwm->hwpwm], CHANNEL_CLK_SEL_FREQ);
#else
	timer_clk_enable(chip, pwm);
#endif

	pwm_get_state(pwm, &cur_state);

	if (period_ns < (1000000000 / CHANNEL_CLK_SEL_FREQ)) {
		pr_err
		    ("%s %s chan %u period is to smaller, even smaller than input clock\n",
		     __func__, dev_name(chip->dev), pwm->hwpwm);
		return -EINVAL;
	}

	pwm_timer_base = devm_ioremap(chip->dev, our_chip->pc_resource->start,
				      resource_size(our_chip->pc_resource));
	if (IS_ERR(pwm_timer_base)) {
		pr_err("%s %s chan %u ioremap source fail\n", __func__,
		       dev_name(chip->dev), pwm->hwpwm);
		return PTR_ERR(pwm_timer_base);
	}

	/* disable pwm timer and config pwm mode */
	writel(PWM_TIMERN_MODE,
	       pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));

	temp = (u64) period_ns * CHANNEL_CLK_SEL_FREQ;
	do_div(temp, 1000000000);
	period_count = (u32) temp;
	temp = (u64) duty_ns * CHANNEL_CLK_SEL_FREQ;
	do_div(temp, 1000000000);
	duty_count = (u32) temp;

	pr_debug("%s: config %s channel %u period_count:%u duty_count:%u\n",
		 __func__, dev_name(chip->dev), pwm->hwpwm, period_count,
		 duty_count);

	writel(duty_count,
	       pwm_timer_base + PWM_TIMERN_LOADCOUNT2_OFF(pwm->hwpwm));
	writel(period_count - duty_count,
	       pwm_timer_base + PWM_TIMERN_LOADCOUNT_OFF(pwm->hwpwm));

	pr_debug("period = %u, duty = %u\n",
		readl(pwm_timer_base + PWM_TIMERN_LOADCOUNT2_OFF(pwm->hwpwm)),
		readl(pwm_timer_base + PWM_TIMERN_LOADCOUNT_OFF(pwm->hwpwm)));

	/* if current pwm is enabled, then keep enable */
	if (cur_state.enabled)
		writel((PWM_TIMERN_EN | PWM_TIMERN_MODE),
		       pwm_timer_base + PWM_TIMERN_CONTROLREG_OFF(pwm->hwpwm));

	devm_iounmap(chip->dev, pwm_timer_base);

	return 0;
}

static const struct pwm_ops pwm_axera_ops = {
	.enable = pwm_axera_enable,
	.disable = pwm_axera_disable,
	.config = pwm_axera_config,
	.owner = THIS_MODULE,
};

static int pwm_axera_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct axera_pwm_chip *chip;
	struct resource *res;
	char buf[20];
	int ret, i;
	u32 pin_sleep_cfg[CHANNELS_PER_PWM * 2];
	int len;
	unsigned int val;

	len = of_property_read_variable_u32_array(dev->of_node, "pin_sleep_setting", pin_sleep_cfg, 0, ARRAY_SIZE(pin_sleep_cfg));

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->chip.dev = &pdev->dev;
	chip->chip.ops = &pwm_axera_ops;
	chip->chip.base = -1;
	chip->chip.npwm = CHANNELS_PER_PWM;

	if(len > 0) {
		chip->pin_cfg_len = len / 2;
	}

	for(i = 0; i < len; i+= 2) {
		chip->pin_sleep_cfg[i / 2].reg = pin_sleep_cfg[i];
		chip->pin_sleep_cfg[i / 2].sleep_setting = pin_sleep_cfg[i + 1];
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODATA;
	chip->pc_resource = res;

	chip->slp_timer_base = devm_ioremap(&pdev->dev, chip->pc_resource->start, resource_size(chip->pc_resource));

	if (IS_ERR(chip->slp_timer_base)) {
		return PTR_ERR(chip->slp_timer_base);
	}

	platform_set_drvdata(pdev, chip);

#ifdef USE_LINUX_CLK

	for (i = 0; i < CHANNELS_PER_PWM; i++) {
		sprintf(buf, "pwm-ch%d-clk", i);
		chip->pwm_clk[i] = devm_clk_get(&pdev->dev, buf);
		if (IS_ERR(chip->pwm_clk[i])) {
			pr_err("axera get %s fail\n", buf);
			return PTR_ERR(chip->pwm_clk[i]);
		}
	}

	chip->pwm_pclk = devm_clk_get(&pdev->dev, "pclk");
	if (IS_ERR(chip->pwm_pclk)) {
		pr_err("axera get pclk fail\n");
		return PTR_ERR(chip->pwm_pclk);
	}
#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);

	if (!res) {
		pr_err("get res failed.\n");
		return -ENODATA;
	}

	chip->clk_base =  devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if(IS_ERR(chip->clk_base)) {
		return PTR_ERR(chip->clk_base);
	}

	if (of_property_read_u32(dev->of_node, "clk-sel-addr-offset", &chip->clk_sel_addr_offset)) {
		pr_err("get clk-sel-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-sel-offset", &chip->clk_sel_offset)) {
		pr_err("get clk-sel-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-glb-eb-addr-offset", &chip->clk_glb_eb_addr_offset)) {
		pr_err("get clk-glb-eb-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-glb-eb-offset", &chip->clk_glb_eb_offset)) {
		pr_err("get clk-glb-eb-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-eb-addr-offset", &chip->clk_eb_addr_offset)) {
		pr_err("get clk-eb-addr-offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32_array(dev->of_node, "clk-eb-offset", chip->clk_eb_offset, CHANNELS_PER_PWM)) {
		pr_err("get clk_eb_offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-p-eb-addr-offset", &chip->clk_p_eb_addr_offset)) {
		pr_err("get clk_p_eb_addr_offset failed.\n");
		return -ENODEV;
	}

	if (of_property_read_u32(dev->of_node, "clk-p-eb-offset", &chip->clk_p_eb_offset)) {
		pr_err("get clk-p-eb-offset failed.\n");
		return -ENODEV;
	}

	for(i = 0; i < CHANNELS_PER_PWM; i++) {
		chip->clk_eb_flag[i] = 0;
	}

	val = (1 << chip->clk_glb_eb_offset);
	/*clk_eb_0_set register*/
	writel(val, chip->clk_glb_eb_addr_offset + chip->clk_base + 0xAC);
	/* pclk default open, not open*/
#endif

	/* global reset */
	chip->glb_rst = devm_reset_control_get_optional(&pdev->dev, "pwm-rst");
	if (IS_ERR(chip->glb_rst)) {
		pr_err("axera get global reset failed\n");
		return PTR_ERR(chip->glb_rst);
	}

	reset_control_deassert(chip->glb_rst);

	for (i = 0; i < CHANNELS_PER_PWM; i++) {
		sprintf(buf, "pwm-ch%d-rst", i);
		chip->ch_rst[i] = devm_reset_control_get_optional(&pdev->dev, buf);
		if (IS_ERR(chip->ch_rst[i])) {
			pr_err("axera get %s fail\n", buf);
			return PTR_ERR(chip->ch_rst[i]);
		}
	}

	ret = pwmchip_add(&chip->chip);
	if (ret < 0) {
		dev_err(dev, "failed to register PWM chip\n");
		return ret;
	}

	return 0;
}

static int pwm_axera_remove(struct platform_device *pdev)
{
	struct axera_pwm_chip *chip;
	chip = platform_get_drvdata(pdev);
	devm_iounmap(&pdev->dev, chip->slp_timer_base);
	pwmchip_remove(&chip->chip);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static void pwm_pin_suspend(struct axera_pwm_chip  *our_chip)
{
	void *base;
	int i;
	for(i = 0; i < our_chip->pin_cfg_len; i++) {
		base = ioremap(our_chip->pin_sleep_cfg[i].reg, 4);
		if(!base) {
			dev_err(our_chip->chip.dev, "pwm_pin_suspend: remap failed!\n");
			return;
		}
		our_chip->pin_sleep_cfg[i].save_setting = readl(base);
		writel(our_chip->pin_sleep_cfg[i].sleep_setting, base);
		iounmap(base);
	}
}

static void pwm_pin_resume(struct axera_pwm_chip  *our_chip)
{
	void *base;
	int i;
	for(i = 0; i < our_chip->pin_cfg_len; i++) {
		base = ioremap(our_chip->pin_sleep_cfg[i].reg, 4);
		if(!base) {
			dev_err(our_chip->chip.dev, "pwm_pin_resume: remap failed!\n");
			return;
		}
		writel(our_chip->pin_sleep_cfg[i].save_setting, base);
		iounmap(base);
	}
}

static int pwm_axera_suspend(struct device *dev)
{
	struct axera_pwm_chip  *our_chip = dev_get_drvdata(dev);
	int i;
	pwm_pin_suspend(our_chip);
	for (i = 0; i < our_chip->chip.npwm; i++) {
		our_chip->suspend_cfg[i][0] = readl(our_chip->slp_timer_base + PWM_TIMERN_LOADCOUNT_OFF(i));
		our_chip->suspend_cfg[i][1] = readl(our_chip->slp_timer_base + PWM_TIMERN_CONTROLREG_OFF(i));
		our_chip->suspend_cfg[i][2] = readl(our_chip->slp_timer_base + PWM_TIMERN_LOADCOUNT2_OFF(i));
	}
	return 0;
}

static int pwm_axera_resume(struct device *dev)
{
	struct axera_pwm_chip  *our_chip = dev_get_drvdata(dev);
	int i;
	for (i = 0; i < our_chip->chip.npwm; i++) {
		writel(our_chip->suspend_cfg[i][0], our_chip->slp_timer_base + PWM_TIMERN_LOADCOUNT_OFF(i));
		writel(our_chip->suspend_cfg[i][2], our_chip->slp_timer_base + PWM_TIMERN_LOADCOUNT2_OFF(i));
		if(our_chip->suspend_cfg[i][1] & PWM_TIMERN_EN) {
			writel(our_chip->suspend_cfg[i][1] & (~PWM_TIMERN_EN), our_chip->slp_timer_base + PWM_TIMERN_CONTROLREG_OFF(i));
		}
		writel(our_chip->suspend_cfg[i][1], our_chip->slp_timer_base + PWM_TIMERN_CONTROLREG_OFF(i));
	}
	udelay(100);
	pwm_pin_resume(our_chip);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(pwm_axera_pm_ops, pwm_axera_suspend, pwm_axera_resume);

static const struct of_device_id axera_pwm_matches[] = {
	{.compatible = "axera,ax620e-pwm"},
	{},
};

static struct platform_driver pwm_axera_driver = {
	.driver = {
		   .name = "axera-pwm",
		   .pm = &pwm_axera_pm_ops,
		   .of_match_table = axera_pwm_matches,
		   },
	.probe = pwm_axera_probe,
	.remove = pwm_axera_remove,
};

module_platform_driver(pwm_axera_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axera");
MODULE_ALIAS("platform:axera-pwm");
