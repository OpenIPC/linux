/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * Author: Arun R Murthy <arun.murthy@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#include "mhal_pwm.h"

static inline struct mstar_pwm_chip *to_mstar_pwm_chip(struct pwm_chip *c)
{
	return container_of(c, struct mstar_pwm_chip, chip);
}

static int mstar_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm, int duty_ns, int period_ns)
{
    struct mstar_pwm_chip *ms_pwm = to_mstar_pwm_chip(chip);

    MS_PWM_DBG("[PWN] %s duty_ns=%d, period_ns=%d\n", __func__, duty_ns, period_ns);

    DrvPWMSetPeriod(ms_pwm, pwm->hwpwm, period_ns);
    DrvPWMSetDuty(ms_pwm, pwm->hwpwm, duty_ns);
//    DrvPWMPadSet(pwm->hwpwm, (U8)ms_pwm->pad_ctrl[pwm->hwpwm]);
	return 0;
}

static int mstar_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct mstar_pwm_chip *ms_pwm = to_mstar_pwm_chip(chip);
    MS_PWM_DBG("[PWM] %s\n", __func__);
    DrvPWMEnable(ms_pwm, pwm->hwpwm, 1);
	DrvPWMPadSet(pwm->hwpwm, (U8)ms_pwm->pad_ctrl[pwm->hwpwm]);
	return 0;
}

static void mstar_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct mstar_pwm_chip *ms_pwm = to_mstar_pwm_chip(chip);
    MS_PWM_DBG("[PWM] %s\n", __func__);
    DrvPWMEnable(ms_pwm, pwm->hwpwm, 0);
}

static int mstar_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
    struct mstar_pwm_chip *ms_pwm = to_mstar_pwm_chip(chip);
    MS_PWM_DBG("[PWM] %s %d\n", __func__, (U8)polarity);
    DrvPWMSetPolarity(ms_pwm, pwm->hwpwm, (U8)polarity);
	return 0;
}


static const struct pwm_ops mstar_pwm_ops = {
	.config = mstar_pwm_config,
	.enable = mstar_pwm_enable,
	.disable = mstar_pwm_disable,
	.set_polarity = mstar_pwm_set_polarity,
	.owner = THIS_MODULE,
};

static int ms_pwm_probe(struct platform_device *pdev)
{
	struct mstar_pwm_chip *ms_pwm;
    struct resource *res;
	int ret=0, i=0;

	ms_pwm = devm_kzalloc(&pdev->dev, sizeof(*ms_pwm), GFP_KERNEL);
	if (ms_pwm == NULL)
    {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,	"Can't get I/O resource regs for pwm\n");
		return 0;
	}

	//ms_pwm->base = devm_ioremap_resource(&pdev->dev, res);
    ms_pwm->base = (void *)res->start;

	if (IS_ERR(ms_pwm->base))
		return PTR_ERR(ms_pwm->base);

	ms_pwm->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ms_pwm->clk))
		return PTR_ERR(ms_pwm->clk);

	ret = clk_prepare_enable(ms_pwm->clk);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, ms_pwm);

	ms_pwm->chip.dev = &pdev->dev;
	ms_pwm->chip.ops = &mstar_pwm_ops;
	ms_pwm->chip.base = -1;
    if(of_property_read_u32(pdev->dev.of_node, "npwm", &ms_pwm->chip.npwm))
    	ms_pwm->chip.npwm = 4;

    ms_pwm->pad_ctrl = devm_kzalloc(&pdev->dev, sizeof(*ms_pwm->pad_ctrl), GFP_KERNEL);
	if (ms_pwm->pad_ctrl == NULL)
    {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

    if((ret=of_property_read_u32_array(pdev->dev.of_node, "pad-ctrl", ms_pwm->pad_ctrl, ms_pwm->chip.npwm)))
        dev_err(&pdev->dev, "read pad-ctrl failed\n");

    for(i=0; i<ms_pwm->chip.npwm; i++)
    {
        MS_PWM_DBG("ms_pwm->pad_ctrl[%d]=%d\n", i, ms_pwm->pad_ctrl[i]);
    }

	ret = pwmchip_add(&ms_pwm->chip);
	if (ret < 0)
    {
        clk_disable_unprepare(ms_pwm->clk);
        dev_err(&pdev->dev, "pwmchip_add failed\n");
		return ret;
    }

	dev_info(&pdev->dev, "probe successful\n");

	return 0;
}

static int ms_pwm_remove(struct platform_device *pdev)
{
	struct mstar_pwm_chip *ms_pwm = dev_get_drvdata(&pdev->dev);
	int err;

    clk_disable_unprepare(ms_pwm->clk);

	err = pwmchip_remove(&ms_pwm->chip);
	if (err < 0)
		return err;

	dev_info(&pdev->dev, "remove successful\n");
	return 0;
}

static const struct of_device_id ms_pwm_of_match_table[] = {
    { .compatible = "mstar,infinity3-pwm" },
    {}
};

MODULE_DEVICE_TABLE(of, ms_pwm_of_match_table);

static struct platform_driver ms_pwm_driver = {
    .remove = ms_pwm_remove,
    .probe = ms_pwm_probe,
    .driver = {
        .name = "mstar-i3pwm",
        .owner = THIS_MODULE,
        .of_match_table = ms_pwm_of_match_table,
    },
};

module_platform_driver(ms_pwm_driver);

MODULE_AUTHOR("MStar Semiconductor, Inc.");
MODULE_DESCRIPTION("MStar PWM Driver");
MODULE_LICENSE("GPL v2");
