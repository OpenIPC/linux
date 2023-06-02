/*
 * Novatek NA51055 PWM driver.
 *
 * Copyright (C) 2020 Novatek MicroElectronics Corp.
 *
 *
 * ----------------------------------------------------------------------------
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>

#include <plat-na51055/pwm.h>
#include <plat-na51055/pwm-int.h>
#include <mach/nvt-io.h>

#ifdef CONFIG_OF
static const struct of_device_id nvt_pwm_match[] = {
	{ .compatible = "nvt,nvt_pwm" },
	{},
};

MODULE_DEVICE_TABLE(of, nvt_pwm_match);
#endif

struct nvt_pwm {
	struct pwm_chip chip;
	void __iomem *base;
	int irq;
};

static struct completion vPWMFlgId[PWM_PWMCH_BITS];

static struct clk *pwm_clk[PWM_PWMCH_BITS];

static int div = 3;
static int check = 0;

#define to_nvt_chip(chip)	container_of(chip, struct nvt_pwm, chip)

#define REG_CTRL(pwm)		(((pwm) * 0x8) + 0x00)
#define REG_PERIOD(pwm)		(((pwm) * 0x8) + 0x04)
#define REG_EXT_PERIOD(pwm)	(((pwm) * 0x4) + 0x230)
#define REG_INT_ENABLE		0xe0
#define REG_INT_STS         0xf0
#define REG_ENABLE          0x100
#define REG_DISABLE         0x104
#define REG_LOAD		    0x108
//#define REG_CLK_RELOAD      0x10c

#define REG_INT2CORE1       0x250
#define REG_INT2CORE2       0x254
#define REG_INT_ENABLE1     0x258
#define REG_INT_STS1        0x268

#define CTRL_ENABLE(pwm)	BIT(pwm)
#define CTRL_INVERT		BIT(28)
#define CTRL_RELOAD(pwm)	BIT(pwm)

#define PWM_SELECT_CPU1 1
//#define PWM_SELECT_CPU2 1

static inline void nvt_pwm_write_reg(struct nvt_pwm *pwm_dev,
					 int reg, u32 val)
{
	//__raw_writel(val, pwm_dev->base + reg);
	writel(val, pwm_dev->base + reg);
}

static inline u32 nvt_pwm_read_reg(struct nvt_pwm *pwm_dev, int reg)
{
	//return __raw_readl(pwm_dev->base + reg);
	return readl(pwm_dev->base + reg);
}

/*
	Check PWM ID

	Check whether the PWM ID is valid or not

	@param[in] uiStartBit   Start search bit
	@param[in] uiPWMId      PWM ID (bitwise), one ID at a time

	@return Check ID status
		- @b PWM_INVALID_ID: Not a valid PWM ID
		- @b Other: ID offset (0 ~ 2 for PWM, 4 for CCNT)
*/
static u32 pwm_isValidId(u32 uiStartBit, u32 uiPWMId)
{
	u32 i;

	for(i = uiStartBit; i < PWM_ALLCH_BITS; i++) {
		if (uiPWMId & (1<<i))
			return i;
	}
	return PWM_INVALID_ID;
}

/**
	Set PWM parameters.

	This function sets PWM duty cycles, repeat count and signal level.\n
	After set, the specified pwm channel can be started and stopped by API\n
	functions pwm_pwmEnable() and pwm_pwmDisable(). If the on cycle is not PWM_FREE_RUN,\n
	PWM will issue an interrupt after all cycles are done.

	@param[in] uiPWMId  pwm id, one id at a time
	@param[in] pPWMCfg  PWM parameter descriptor

	@return Set parameter status.
		- @b E_OK: Success
		- @b E_PAR: Invalid PWM ID
*/
static int pwm_pwmConfig(struct nvt_pwm *pwm_dev, u32 uiPWMId, PPWM_CFG pPWMCfg)
{
	u32 uiOffset;
	u32 reg_pwm_period_buf;
	u32 org_pwm_period_buf;
	u32 exp_period_buf = {0};

	if ((uiOffset = pwm_isValidId(0, uiPWMId)) == PWM_INVALID_ID) {
		pr_warn("invalid PWM ID 0x%08x\r\n", uiPWMId);
		return 1;
	}

	if (uiOffset < 16)
		org_pwm_period_buf = nvt_pwm_read_reg(pwm_dev, REG_PERIOD(uiOffset));
	else
		org_pwm_period_buf = nvt_pwm_read_reg(pwm_dev, REG_PERIOD(uiOffset) + 0x20);

	if (pPWMCfg->base_period < 2) {
		pr_warn("invalid PWM base period %d MUST 2~255\r\n", pPWMCfg->base_period);
		return 1;
	}

	reg_pwm_period_buf = ((pPWMCfg->rise & 0xFF) + ((pPWMCfg->fall & 0xFF) << 8) + ((pPWMCfg->base_period & 0xFF) << 16));

	if (uiOffset < 8) {
		exp_period_buf = nvt_pwm_read_reg(pwm_dev, REG_EXT_PERIOD(uiOffset));
		exp_period_buf = ((pPWMCfg->rise >> 8 & 0xFF) + ((pPWMCfg->fall >> 8 & 0xFF) << 8) + ((pPWMCfg->base_period >> 8 & 0xFF) << 16));
		nvt_pwm_write_reg(pwm_dev, REG_EXT_PERIOD(uiOffset), exp_period_buf);
	}

	if (uiOffset < 16)
		nvt_pwm_write_reg(pwm_dev, REG_PERIOD(uiOffset), reg_pwm_period_buf);
	else
		nvt_pwm_write_reg(pwm_dev, REG_PERIOD(uiOffset) + 0x20, reg_pwm_period_buf);

	return 0;
}

static irqreturn_t nvt_pwm_isr(int this_irq, void *dev_id)
{
	struct nvt_pwm *pwm_dev = dev_id;
	u32 sts_reg, i=0;

#ifdef PWM_SELECT_CPU1
	sts_reg = nvt_pwm_read_reg(pwm_dev, REG_INT_STS);
	nvt_pwm_write_reg(pwm_dev, REG_INT_STS, sts_reg);
#endif

#ifdef PWM_SELECT_CPU2
	sts_reg = nvt_pwm_read_reg(pwm_dev, REG_INT_STS1);
	nvt_pwm_write_reg(pwm_dev, REG_INT_STS1, sts_reg);
#endif

	while (sts_reg) {
		if (sts_reg & 0x01)
		    complete(&vPWMFlgId[i]);

		i++;
	    sts_reg >>= 1;
	}

	return IRQ_HANDLED;
}

static int nvt_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		int duty_ns, int period_ns)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	unsigned long clkdiv = 0;
	PWM_CFG pwmcfg;
	u64 src_freq = 120000000;
	u64 p = 0;
    u64 f = 0;

	if ((pwm->hwpwm >= PWMID_NO_0) && (pwm->hwpwm <= PWMID_NO_7)) {
		if ((period_ns >= 67) && (period_ns <= 1000000000)) {
			clkdiv = div;
			p = src_freq * period_ns;
			do_div(p, 1000000000);
			do_div(p, (clkdiv + 1));
			pwmcfg.base_period = (u32)p;

			while (pwmcfg.base_period > 65535) {
				clkdiv += 4;
				p = src_freq * period_ns;
				do_div(p, 1000000000);
				do_div(p, (clkdiv + 1));
				pwmcfg.base_period = (u32)p;
			}
			pwmcfg.rise = 0;

			if (duty_ns == 0) {
				pwmcfg.fall = duty_ns;
			} else {
				if (period_ns/duty_ns != 0) {
                    f = (u64)duty_ns * (u64)pwmcfg.base_period;
                    do_div(f, period_ns);
					pwmcfg.fall = (u32)f;
				}
			}
		} else {
        		dev_err(chip->dev, "not support this output frequency PWM%d\n", pwm->hwpwm);
        		return -EINVAL;
		}
	} else {
		if ((period_ns >= 67) && (period_ns <= (36*1000000))) {
			clkdiv = div;
			p = src_freq * period_ns;
			do_div(p, 1000000000);
			do_div(p, (clkdiv + 1));
			pwmcfg.base_period = (u32)p;

			while (pwmcfg.base_period > 255) {
				clkdiv += 4;
				p = src_freq * period_ns;
				do_div(p, 1000000000);
				do_div(p, (clkdiv + 1));
				pwmcfg.base_period = (u32)p;
			}
			pwmcfg.rise = 0;

			if (duty_ns == 0) {
				pwmcfg.fall = duty_ns;
			} else {
				if (period_ns/duty_ns != 0)
					pwmcfg.fall = (duty_ns * pwmcfg.base_period)/period_ns;
			}

		} else {
			dev_err(chip->dev, "not support this output frequency in PWM%d\n", pwm->hwpwm);
				return -EINVAL;
		}
	}

	if(check == 0)
		div = clkdiv;

	clk_set_rate(pwm_clk[pwm->hwpwm], 120000000/(clkdiv+1));
	pwm_pwmConfig(pwm_dev, BIT(pwm->hwpwm), &pwmcfg);

	return 0;
}

static int nvt_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	int err;
	u32 val;

    if (nvt_get_chip_id() == CHIP_NA51084) {
#ifdef PWM_SELECT_CPU1
        val = nvt_pwm_read_reg(pwm_dev, REG_INT2CORE1);
        val |= CTRL_ENABLE(pwm->hwpwm);
        nvt_pwm_write_reg(pwm_dev, REG_INT2CORE1, val);
#endif

#ifdef PWM_SELECT_CPU2
        val = nvt_pwm_read_reg(pwm_dev, REG_INT2CORE2);
        val |= CTRL_ENABLE(pwm->hwpwm);
        nvt_pwm_write_reg(pwm_dev, REG_INT2CORE2, val);
#endif
    }

#if 0
    if (pwm->hwpwm < 4 && pwm->hwpwm >= 0)
        nvt_pwm_write_reg(pwm_dev, REG_CLK_RELOAD, 0x1);
    else if (pwm->hwpwm < 8 && pwm->hwpwm >= 4)
        nvt_pwm_write_reg(pwm_dev, REG_CLK_RELOAD, 0x2);
    else if (pwm->hwpwm < 12 && pwm->hwpwm >= 8)
        nvt_pwm_write_reg(pwm_dev, REG_CLK_RELOAD, 0x4);
    else
        ;
#endif

	err = clk_enable(pwm_clk[pwm->hwpwm]);
	if (err < 0) {
		dev_err(chip->dev, "failed to prepare clock\n");
		return err;
	}

#ifdef PWM_SELECT_CPU1
	val = nvt_pwm_read_reg(pwm_dev, REG_INT_ENABLE);
	val |= CTRL_ENABLE(pwm->hwpwm);
	nvt_pwm_write_reg(pwm_dev, REG_INT_ENABLE, val);
#endif

#ifdef PWM_SELECT_CPU2
	val = nvt_pwm_read_reg(pwm_dev, REG_INT_ENABLE1);
	val |= CTRL_ENABLE(pwm->hwpwm);
	nvt_pwm_write_reg(pwm_dev, REG_INT_ENABLE1, val);
#endif

	val = nvt_pwm_read_reg(pwm_dev, REG_ENABLE);
	val |= CTRL_ENABLE(pwm->hwpwm);
	nvt_pwm_write_reg(pwm_dev, REG_ENABLE, val);

  	/*wait for enable be 1*/
  	while((nvt_pwm_read_reg(pwm_dev, REG_ENABLE) & CTRL_ENABLE(pwm->hwpwm)) != CTRL_ENABLE(pwm->hwpwm));

	if(pwm->cycle != 0)
		wait_for_completion(&vPWMFlgId[pwm->hwpwm]);

	check = 1;

	return 0;
}

static void nvt_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	u32 val;

    val = nvt_pwm_read_reg(pwm_dev, REG_ENABLE);
    if (!(val & (1 << pwm->hwpwm))) {
        pr_warn("PWM%d is no enable, do noting\r\n", pwm->hwpwm);
        return;
    }

	val = nvt_pwm_read_reg(pwm_dev, REG_DISABLE);
	val |= (1 << (pwm->hwpwm));
	nvt_pwm_write_reg(pwm_dev, REG_DISABLE, val);

  	/*wait done*/
  	wait_for_completion(&vPWMFlgId[pwm->hwpwm]);

#ifdef PWM_SELECT_CPU1
	val = nvt_pwm_read_reg(pwm_dev, REG_INT_ENABLE);
	val &= ~(1 << (pwm->hwpwm));
	nvt_pwm_write_reg(pwm_dev, REG_INT_ENABLE, val);
#endif

#ifdef PWM_SELECT_CPU2
	val = nvt_pwm_read_reg(pwm_dev, REG_INT_ENABLE1);
	val &= ~(1 << (pwm->hwpwm));
	nvt_pwm_write_reg(pwm_dev, REG_INT_ENABLE1, val);
#endif

	clk_disable(pwm_clk[pwm->hwpwm]);

    if (nvt_get_chip_id() == CHIP_NA51084) {
#ifdef PWM_SELECT_CPU1
        val = nvt_pwm_read_reg(pwm_dev, REG_INT2CORE1);
        val &= ~(1 << (pwm->hwpwm));
        nvt_pwm_write_reg(pwm_dev, REG_INT2CORE1, val);
#endif

#ifdef PWM_SELECT_CPU2
        val = nvt_pwm_read_reg(pwm_dev, REG_INT2CORE2);
        val &= ~(1 << (pwm->hwpwm));
        nvt_pwm_write_reg(pwm_dev, REG_INT2CORE2, val);
#endif

    }

  	check = 0;
  	div = 3;
}

static int nvt_pwm_set_polarity(struct pwm_chip *chip,
				   struct pwm_device *pwm,
				   enum pwm_polarity polarity)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	u32 val;

	if(pwm->hwpwm < 16)
	    val = nvt_pwm_read_reg(pwm_dev, REG_PERIOD(pwm->hwpwm));
	else
	    val = nvt_pwm_read_reg(pwm_dev, REG_PERIOD(pwm->hwpwm)+0x20);

	if (polarity == PWM_POLARITY_INVERSED)
		val |= CTRL_INVERT;
	else
		val &= ~CTRL_INVERT;

	if(pwm->hwpwm < 16)
	    nvt_pwm_write_reg(pwm_dev, REG_PERIOD(pwm->hwpwm), val);
	else
	    nvt_pwm_write_reg(pwm_dev, REG_PERIOD(pwm->hwpwm)+0x20, val);

	return 0;
}

static void nvt_pwm_reload(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	u32 val = 0;

	val |= (1 << (pwm->hwpwm));
	nvt_pwm_write_reg(pwm_dev, REG_LOAD, val);
}

static int nvt_pwm_set_cycle(struct pwm_chip *chip, struct pwm_device *pwm, int cycle)
{
	struct nvt_pwm *pwm_dev = to_nvt_chip(chip);
	u32 reg_pwm_ctrl_buf;

	printk("cycle is %d\n", cycle);

	if(cycle > 65535) {
		pr_warn("invalid cylce, should be less than 65535\n");
		return -EINVAL;
	}
	reg_pwm_ctrl_buf = cycle;

	if(pwm->hwpwm < 16)
		nvt_pwm_write_reg(pwm_dev, REG_CTRL(pwm->hwpwm), reg_pwm_ctrl_buf);
	else
		nvt_pwm_write_reg(pwm_dev, REG_CTRL(pwm->hwpwm) + 0x20, reg_pwm_ctrl_buf);

	return 0;
}

static struct pwm_ops nvt_pwm_ops = {
	.enable = nvt_pwm_enable,
	.disable = nvt_pwm_disable,
	.config = nvt_pwm_config,
	.set_polarity = nvt_pwm_set_polarity,
	.set_cycle = nvt_pwm_set_cycle,
	.reload = nvt_pwm_reload,
	.owner = THIS_MODULE,
};

static int nvt_pwm_probe(struct platform_device *pdev)
{
	struct nvt_pwm *chip;
	struct resource *r;
    const struct of_device_id *match;
	int ret,i;

    pr_err("comm pwm driver probe\n");

	match = of_match_device(nvt_pwm_match, &pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "Platform device not found \n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(chip->base)){
		dev_err(&pdev->dev, "failed to get base mem\n");
		return PTR_ERR(chip->base);
	}

	chip->irq = platform_get_irq(pdev, 0);
	if (unlikely(chip->irq < 0)) {
		printk("%s fails: platform_get_irq not OK", __FUNCTION__);
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, chip->irq, nvt_pwm_isr, 0, pdev->name, chip);
	if (ret) {
		dev_err(&pdev->dev, "failure requesting irq %i, ret=%d\n", chip->irq, ret);
		return -ENODEV;
	}

	chip->chip.dev = &pdev->dev;
	chip->chip.ops = &nvt_pwm_ops;
	chip->chip.of_xlate = of_pwm_xlate_with_flags;
	chip->chip.of_pwm_n_cells = 3;
	chip->chip.base = -1;
	chip->chip.npwm = PWMID_NO_TOTAL_CNT;

	ret = pwmchip_add(&chip->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add PWM chip\n");
		return ret;
	}

	for(i = 0; i < PWM_PWMCH_BITS; i++ )
		init_completion(&vPWMFlgId[i]);

	platform_set_drvdata(pdev, chip);

	return ret;
}

static int nvt_pwm_remove(struct platform_device *pdev)
{
	struct nvt_pwm *chip;

	chip = platform_get_drvdata(pdev);
	if (chip == NULL)
		return -ENODEV;

	return pwmchip_remove(&chip->chip);
}

static struct platform_driver nvt_pwm_driver = {
	.probe		= nvt_pwm_probe,
	.remove		= nvt_pwm_remove,
	.driver		= {
		.name	= "nvt_pwm",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = nvt_pwm_match,
#endif
	},
};
//module_platform_driver(nt96660_pwm_driver);
static int __init nvt_pwm_init_driver(void)
{
	int i;

	pwm_clk[0] = clk_get(NULL, "pwm_clk.0");
	pwm_clk[1] = clk_get(NULL, "pwm_clk.1");
	pwm_clk[2] = clk_get(NULL, "pwm_clk.2");
	pwm_clk[3] = clk_get(NULL, "pwm_clk.3");
	pwm_clk[4] = clk_get(NULL, "pwm_clk.4");
	pwm_clk[5] = clk_get(NULL, "pwm_clk.5");
	pwm_clk[6] = clk_get(NULL, "pwm_clk.6");
	pwm_clk[7] = clk_get(NULL, "pwm_clk.7");
	pwm_clk[8] = clk_get(NULL, "pwm_clk.8");
	pwm_clk[9] = clk_get(NULL, "pwm_clk.9");
	pwm_clk[10] = clk_get(NULL, "pwm_clk.10");
	pwm_clk[11] = clk_get(NULL, "pwm_clk.11");

	for (i = 0; i < PWM_PWMCH_BITS; i++)
	{
		if (IS_ERR(pwm_clk[i])) {
		    pr_warn("clock source %d not specified\n", i);
		    return PTR_ERR(pwm_clk[i]);
		}
		clk_prepare(pwm_clk[i]);
	}

	return platform_driver_register(&nvt_pwm_driver);
}
module_init(nvt_pwm_init_driver);

static void __exit nvt_pwm_exit_driver(void)
{
	int i;

	for (i = 0; i < PWM_PWMCH_BITS; i++)
	{
		clk_unprepare(pwm_clk[i]);
	}

	platform_driver_unregister(&nvt_pwm_driver);
}
module_exit(nvt_pwm_exit_driver);


MODULE_DESCRIPTION("NVT PWM Driver");
MODULE_AUTHOR("Novatek");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.00.009");