/*
 * mdrv_pwm.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/pwm.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <drv_camclk_Api.h>
#include <cam_drv_pwm.h>

#include "ms_msys.h"
#include "mhal_pwm.h"

#if defined(CONFIG_MS_PADMUX)
#include "mdrv_padmux.h"
#include "mdrv_puse.h"
#include "gpio.h"
#endif

static ssize_t group_mode_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_period_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_begin_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_end_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_round_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_enable_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_hold_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_stop_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_polarity_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_info_out(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t group_hold_mode1_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_duty_qe0_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_round_cal_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t group_round_cal_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t group_output_out(struct device *dev, struct device_attribute *attr, char *buf);
extern irqreturn_t PWM_IRQ(int irq, void *dummy);
extern void        MHal__PWM_AllGrpEnable(struct sstar_pwm_chip *sstar_chip);

DEVICE_ATTR(group_mode, 0200, NULL, group_mode_in);
DEVICE_ATTR(group_period, 0200, NULL, group_period_in);
DEVICE_ATTR(group_begin, 0200, NULL, group_begin_in);
DEVICE_ATTR(group_end, 0200, NULL, group_end_in);
DEVICE_ATTR(group_round, 0200, NULL, group_round_in);
DEVICE_ATTR(group_enable, 0200, NULL, group_enable_in);
DEVICE_ATTR(group_hold, 0200, NULL, group_hold_in);
DEVICE_ATTR(group_stop, 0200, NULL, group_stop_in);
DEVICE_ATTR(group_polarity, 0200, NULL, group_polarity_in);
DEVICE_ATTR(group_info, 0444, group_info_out, NULL);
DEVICE_ATTR(group_hold_mode1, 0200, NULL, group_hold_mode1_in);
DEVICE_ATTR(group_duty_qe0, 0200, NULL, group_duty_qe0_in);
DEVICE_ATTR(group_round_get, 0644, group_round_cal_show, group_round_cal_store);
DEVICE_ATTR(group_output, 0444, group_output_out, NULL);

static inline struct sstar_pwm_chip *to_sstar_pwm_chip(struct pwm_chip *c)
{
    return container_of(c, struct sstar_pwm_chip, chip);
}

static int sstar_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm, int duty_ns, int period_ns)
{
    struct sstar_pwm_chip *sstar_pwm = to_sstar_pwm_chip(chip);

#ifdef CONFIG_ARCH_INFINITY6E
    MHal__PWM_AllGrpEnable(sstar_pwm);
#endif

#ifdef CONFIG_PWM_NEW
    SSTAR_PWM_INFO("[PWM] %s duty_ns=%d, period_ns=%d\n", __func__, duty_ns, period_ns);
    MHal_PWMSetConfig(sstar_pwm, pwm->hwpwm, duty_ns, period_ns);
#else
    MHal_PWMSetPeriod(sstar_pwm, pwm->hwpwm, period_ns);
    MHal_PWMSetDuty(sstar_pwm, pwm->hwpwm, duty_ns);
#endif

    return 0;
}

#if defined(CONFIG_MS_PADMUX)
static int _pwm_is_pad_set(U8 u8PwmId)
{
    if (PAD_UNKNOWN != mdrv_padmux_getpad(mdrv_padmux_getpuse(PUSE_PWM, u8PwmId, PWM_NA)))
    {
        return TRUE;
    }
    else
        return FALSE;
}
#endif

static int sstar_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct sstar_pwm_chip *sstar_pwm = to_sstar_pwm_chip(chip);
    SSTAR_PWM_INFO("[PWM] %s\n", __func__);
    MHal_PWMEnable(sstar_pwm, pwm->hwpwm, 1);

#if defined(CONFIG_MS_PADMUX)
    if (0 == mdrv_padmux_active() || FALSE == _pwm_is_pad_set(pwm->hwpwm))
#endif
    {
        MHal_PWMPadSet(pwm->hwpwm, (U8)sstar_pwm->pad_ctrl[pwm->hwpwm]);
    }
    return 0;
}

static void sstar_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
    struct sstar_pwm_chip *sstar_pwm = to_sstar_pwm_chip(chip);
    SSTAR_PWM_INFO("[PWM] %s\n", __func__);
    MHal_PWMEnable(sstar_pwm, pwm->hwpwm, 0);
}

static int sstar_pwm_set_polarity(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
    struct sstar_pwm_chip *sstar_pwm = to_sstar_pwm_chip(chip);
    SSTAR_PWM_INFO("[PWM] %s %d\n", __func__, (U8)polarity);
    MHal_PWMSetPolarity(sstar_pwm, pwm->hwpwm, (U8)polarity);
    return 0;
}

static void sstar_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm, struct pwm_state *state)
{
    struct sstar_pwm_chip *sstar_pwm = to_sstar_pwm_chip(chip);
    U32                    u32Period = 0x00000000, u32Duty = 0x00000000;
    U8                     enable = 0x00, polarity = 0x00;

#ifdef CONFIG_PWM_NEW
    MHal_PWMGetConfig(sstar_pwm, pwm->hwpwm, &u32Duty, &u32Period);
#else
    MHal_PWMGetPeriod(sstar_pwm, pwm->hwpwm, &u32Period);
    MHal_PWMGetDuty(sstar_pwm, pwm->hwpwm, &u32Duty);
#endif
    MHal_PWMEnableGet(sstar_pwm, pwm->hwpwm, &enable);
    MHal_PWMGetPolarity(sstar_pwm, pwm->hwpwm, &polarity);
    state->period     = u32Period;
    state->duty_cycle = u32Duty;
    state->polarity   = polarity;
    state->enabled    = enable;
}

static const struct pwm_ops sstar_pwm_ops = {
    .config       = sstar_pwm_config,
    .enable       = sstar_pwm_enable,
    .disable      = sstar_pwm_disable,
    .set_polarity = sstar_pwm_set_polarity,
    .get_state    = sstar_pwm_get_state,
    .owner        = THIS_MODULE,
};

static int sstar_pwm_probe(struct platform_device *pdev)
{
    struct sstar_pwm_chip *sstar_pwm;
    struct resource *      res;
    u32                    clk_level;
    int                    ret = 0;
    int                    i;
#ifdef CONFIG_CAM_CLK
    u32 pwm_clkid;
#endif

    sstar_pwm = devm_kzalloc(&pdev->dev, sizeof(*sstar_pwm), GFP_KERNEL);
    if (sstar_pwm == NULL)
    {
        SSTAR_PWM_ERR("failed to allocate memory\n");
        return -ENOMEM;
    }
    if (MHal_PWMGroupCap())
    {
        struct device *sstar_class_pwm_device = NULL;
        if (!(sstar_class_pwm_device = device_create(msys_get_sysfs_class(), NULL, MKDEV(0, 0), NULL, "motor")))
        {
            SSTAR_PWM_ERR("[%s][%d] create device file fail\n", __FUNCTION__, __LINE__);
            return -ENOMEM;
        }
        SSTAR_PWM_DBG("[%s][%d] 0x%08x\n", __FUNCTION__, __LINE__, (int)sstar_pwm);
        dev_set_drvdata(sstar_class_pwm_device, (void *)sstar_pwm);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_mode);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_period);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_begin);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_end);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_round);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_enable);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_hold);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_stop);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_polarity);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_info);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_hold_mode1);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_duty_qe0);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_round_get);
        device_create_file(sstar_class_pwm_device, &dev_attr_group_output);
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        SSTAR_PWM_ERR("Can't get I/O resource regs for pwm\n");
        return ENOENT;
    }

    sstar_pwm->base = (void *)(unsigned long)res->start;

    if (IS_ERR(sstar_pwm->base))
    {
        return PTR_ERR(sstar_pwm->base);
    }
#ifdef CONFIG_CAM_CLK
    of_property_read_u32_index(pdev->dev.of_node, "camclk", 0, &pwm_clkid);
    if (!pwm_clkid)
    {
        SSTAR_PWM_ERR("[%s] Fail to get clk!\n", __func__);
        return ENOENT;
    }
    else
    {
        if (CamClkRegister("PWM", pwm_clkid, &sstar_pwm->camclk) == CAMCLK_RET_OK)
        {
            CamClkAttrGet(sstar_pwm->camclk, &sstar_pwm->stGetCfg);
            CamClkSetOnOff(sstar_pwm->camclk, 1);
            sstar_pwm->clk_freq = CamClkRateGet(pwm_clkid);
        }
        else
        {
            SSTAR_PWM_ERR("[%s] Fail CamClkRegister!\n", __func__);
            return ENOENT;
        }
    }
#else
    ret = of_property_read_u32(pdev->dev.of_node, "clk-select", &clk_level);
    if (ret < 0)
    {
        SSTAR_PWM_ERR("failed to get clock select from dts\n");
        return ret;
    }
    // sstar_pwm->clk = of_clk_get(pdev->dev.of_node, 0);
    sstar_pwm->clk = devm_clk_get(&pdev->dev, NULL);
    if (IS_ERR(sstar_pwm->clk))
    {
        SSTAR_PWM_ERR("failed to allocate and link clk to clk_core\n");
        return PTR_ERR(sstar_pwm->clk);
    }
    sstar_pwm->parent_hw = clk_hw_get_parent_by_index(__clk_get_hw(sstar_pwm->clk), clk_level);
    clk_set_parent(sstar_pwm->clk, sstar_pwm->parent_hw->clk);
    ret = clk_prepare_enable(sstar_pwm->clk);
    if (ret)
    {
        SSTAR_PWM_ERR("failed to enable clk\n");
        return ret;
    }
    sstar_pwm->clk_freq = clk_get_rate(sstar_pwm->clk);

#endif
    platform_set_drvdata(pdev, sstar_pwm);
    sstar_pwm->chip.dev  = &pdev->dev;
    sstar_pwm->chip.ops  = &sstar_pwm_ops;
    sstar_pwm->chip.base = -1;
    if (of_property_read_u32(pdev->dev.of_node, "npwm", &sstar_pwm->chip.npwm))
    {
        sstar_pwm->chip.npwm = 4;
    }
    sstar_pwm->pad_ctrl = devm_kzalloc(&pdev->dev, sstar_pwm->chip.npwm * sizeof(*sstar_pwm->pad_ctrl), GFP_KERNEL);
    if (sstar_pwm->pad_ctrl == NULL)
    {
        SSTAR_PWM_ERR("failed to allocate memory\n");
        return -ENOMEM;
    }

    if ((ret = of_property_read_u32_array(pdev->dev.of_node, "pad-ctrl", sstar_pwm->pad_ctrl, sstar_pwm->chip.npwm)))
    {
        SSTAR_PWM_ERR("read pad-ctrl failed\n");
    }
#if 1
    for (i = 0; i < sstar_pwm->chip.npwm; i++)
    {
        MHal_PWMInit(sstar_pwm, i);
        SSTAR_PWM_DBG("sstar_pwm->pad_ctrl[%d]=%d\n", i, sstar_pwm->pad_ctrl[i]);
    }
#endif

    ret = pwmchip_add(&sstar_pwm->chip);
    if (ret < 0)
    {
#ifdef CONFIG_CAM_CLK
        CamClkSetOnOff(sstar_pwm->camclk, 0);
        CamClkUnregister(sstar_pwm->camclk);
#else
        clk_disable_unprepare(sstar_pwm->clk);
        devm_clk_put(&pdev->dev, sstar_pwm->clk);
#endif
        SSTAR_PWM_ERR("pwmchip_add failed\n");
        return ret;
    }

    for (i = 0; i < PWM_GROUP_NUM; i++)
    {
        MHal_PWMGroupEnable(sstar_pwm, i, 0);
    }

    /* Register interrupt handler */
    sstar_pwm->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if (sstar_pwm->irq)
    {
        ret = request_irq(sstar_pwm->irq, PWM_IRQ, 0, "pwm-isr", sstar_pwm);
        if (ret)
        {
#ifdef CONFIG_CAM_CLK
            CamClkSetOnOff(sstar_pwm->camclk, 0);
            CamClkUnregister(sstar_pwm->camclk);
#else
            clk_disable_unprepare(sstar_pwm->clk);
            devm_clk_put(&pdev->dev, sstar_pwm->clk);
#endif
            SSTAR_PWM_ERR("pwm request irq fail(x%x)\n", ret);
            return ret;
        }
        SSTAR_PWM_INFO("[NOTICE]pwm-isr(%d) success. If not i6e or i6b0, pls confirm it on .dtsi\n", sstar_pwm->irq);
    }

    SSTAR_PWM_DBG("probe successful\n");

    return 0;
}

static int sstar_pwm_remove(struct platform_device *pdev)
{
    struct sstar_pwm_chip *sstar_pwm = dev_get_drvdata(&pdev->dev);
    int                    err;
#ifdef CONFIG_CAM_CLK
    CamClkUnregister(sstar_pwm->camclk);
#else
    clk_disable_unprepare(sstar_pwm->clk);
    if (!IS_ERR(sstar_pwm->clk))
    {
        devm_clk_put(&pdev->dev, sstar_pwm->clk);
    }
#endif
    err = pwmchip_remove(&sstar_pwm->chip);
    if (err < 0)
    {
        return err;
    }

    free_irq(sstar_pwm->irq, sstar_pwm);
    devm_kfree(&pdev->dev, sstar_pwm);
    SSTAR_PWM_INFO("remove successful\n");
    return 0;
}

static ssize_t group_mode_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    pwmId     = 0;
    int                    enable    = 0;

    nArg = sscanf(buf, "%d %d", &pwmId, &enable);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, enable)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    enable = (enable) ? 1 : 0;
    MHal_PWMGroupJoin(sstar_pwm, pwmId, enable);
    // MHal_PWMDiv(sstar_pwm, pwmId, enable);
    MHal_PWMSetDben(sstar_pwm, pwmId, 1);
    MHal_PWMEnable(sstar_pwm, pwmId, 0);
#if defined(CONFIG_MS_PADMUX)
    if (0 == mdrv_padmux_active() || FALSE == _pwm_is_pad_set(pwmId))
#endif
    {
        MHal_PWMPadSet(pwmId, (U8)sstar_pwm->pad_ctrl[pwmId]);
    }
out:
    return count;
}

static ssize_t group_period_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    pwmId     = 0;
    int                    period    = 0;

    nArg = sscanf(buf, "%d %d", &pwmId, &period);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, period)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] (pwm_id, period) = (%d, %d)\n", __FUNCTION__, __LINE__, pwmId, period);
    MHal_PWMSetPeriodEx(sstar_pwm, pwmId, period);
out:
    return count;
}

static ssize_t group_polarity_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    pwmId     = 0;
    int                    polarity  = 0;

    nArg = sscanf(buf, "%d %d", &pwmId, &polarity);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, polarity)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    polarity = (polarity) ? 1 : 0;
    SSTAR_PWM_DBG("[%s][%d] (pwm_id, polarity) = (%d, %d)\n", __FUNCTION__, __LINE__, pwmId, polarity);
    MHal_PWMSetPolarityEx(sstar_pwm, pwmId, polarity);
out:
    return count;
}

static ssize_t group_end_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    pwmId     = 0;
    int                    end[5]    = {0};
    int                    i;

    nArg = sscanf(buf, "%d %d", &pwmId, &end[0]);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, end[0.. 3])\n", __FUNCTION__, __LINE__);
        goto out;
    }
    for (i = 2; i <= nArg; i++)
        MHal_PWMSetEnd(sstar_pwm, pwmId, i - 2, end[i - 2]);
out:
    return count;
}

static ssize_t group_begin_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    pwmId     = 0;
    int                    begin[5]  = {0};
    int                    i;

    nArg = sscanf(buf, "%d %d", &pwmId, &begin[0]);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, begin[0.. 3])\n", __FUNCTION__, __LINE__);
        goto out;
    }
    for (i = 2; i <= nArg; i++)
        MHal_PWMSetBegin(sstar_pwm, pwmId, i - 2, begin[i - 2]);
out:
    return count;
}

static ssize_t group_round_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    groupId   = 0;
    int                    round     = 0;

    nArg = sscanf(buf, "%d %d", &groupId, &round);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id, round)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] (pwm_id, round) = (%d, %d)\n", __FUNCTION__, __LINE__, groupId, round);
    MHal_PWMGroupSetRound(sstar_pwm, groupId, round);
out:
    return count;
}

static ssize_t group_enable_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm     = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg          = 0x00000000;
    int                    groupId       = 0x00000000;
    int                    enable        = 0x00000000;
    U8                     enable_status = 0x00;

    nArg = sscanf(buf, "%d %d", &groupId, &enable);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (group_id, enable)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    if (!MHal_PWMGroupIsEnable(sstar_pwm, groupId, &enable_status))
    {
        SSTAR_PWM_ERR("[%s][%d] unable to get enable status of group %d\n", __FUNCTION__, __LINE__, groupId);
        goto out;
    }
    enable = (enable) ? 1 : 0;
    if (enable == enable_status)
    {
        SSTAR_PWM_ERR("[%s][%d] cannot enable/disable group %d again. enable status = %d\n", __FUNCTION__, __LINE__,
                      groupId, enable_status);
        goto out;
    }

    if (enable)
    {
        MHal_PWMGroupStop(sstar_pwm, groupId, 0);
        MHal_PWMGroupHold(sstar_pwm, groupId, 0);
        MHal_PWMGroupEnable(sstar_pwm, groupId, 1);
    }
    else
    {
        MHal_PWMGroupEnable(sstar_pwm, groupId, 0);
        MHal_PWMGroupStop(sstar_pwm, groupId, 0);
        MHal_PWMGroupHold(sstar_pwm, groupId, 0);
        MHal_PWMGroupSetRound(sstar_pwm, groupId, 0);
    }
out:
    return count;
}

static ssize_t group_hold_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    groupId   = 0;
    int                    enable    = 0;

    nArg = sscanf(buf, "%d %d", &groupId, &enable);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (group_id, enable)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] (groupId, enable) = (%d, %d)\n", __FUNCTION__, __LINE__, groupId, enable);
    MHal_PWMGroupHold(sstar_pwm, groupId, enable);
out:
    return count;
}

static ssize_t group_stop_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0;
    int                    groupId   = 0;

    nArg = sscanf(buf, "%d ", &groupId);
    if (1 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (groud_id)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] pwm_id = (%d)\n", __FUNCTION__, __LINE__, groupId);
    MHal_PWMGroupStop(sstar_pwm, groupId, 1);
    MHal_PWMGroupSetRound(sstar_pwm, groupId, 0);
out:
    return count;
}

static ssize_t group_info_out(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    return MHal_PWMGroupInfo(sstar_pwm, buf, buf + PAGE_SIZE);
}

static ssize_t group_hold_mode1_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0x00000000;
    int                    enable    = 0x00000000;

    nArg = sscanf(buf, "%d", &enable);
    if (1 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (enable)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] enable = %d\n", __FUNCTION__, __LINE__, enable);
    MHal_PWMGroupHoldM1(sstar_pwm, enable);
out:
    return count;
}

static ssize_t group_duty_qe0_in(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    int                    nArg      = 0x00000000;
    int                    groupId   = 0x00000000;
    int                    enable    = 0x00000000;

    nArg = sscanf(buf, "%d %d", &groupId, &enable);
    if (2 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (group_id, enable)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    SSTAR_PWM_DBG("[%s][%d] (groupId, enable) = (%d, %d)\n", __FUNCTION__, __LINE__, groupId, enable);
    MHal_PWMDutyQE0(sstar_pwm, enable);
out:
    return count;
}

static ssize_t group_round_cal_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int nArg    = 0;
    int groupId = 0;

    nArg = sscanf(buf, "%d", &groupId);
    if (1 != nArg)
    {
        SSTAR_PWM_ERR("[%s][%d] invalid argument (pwm_id)\n", __FUNCTION__, __LINE__);
        goto out;
    }
    MHal_PWMGroupClearRoundNum(groupId);
out:
    return count;
}

static ssize_t group_round_cal_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    return MHal_PWMGroupGetRoundNum(sstar_pwm, buf, buf + PAGE_SIZE);
}

static ssize_t group_output_out(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sstar_pwm_chip *sstar_pwm = (struct sstar_pwm_chip *)dev_get_drvdata(dev);
    char                   u8Output  = 0x00;

    SSTAR_PWM_DBG("[%s][%d] output = x%x\n", __FUNCTION__, __LINE__, u8Output);
    MHal_PWMGetOutput(sstar_pwm, &u8Output);
    return 1;
}

int CamPwmConfig(struct pwm_device *pwm_dev, enum CamPWMArgs args, struct CamPwmState *cam_state)
{
    int              ret;
    struct pwm_state state;

    switch (args)
    {
        case CAM_PWM_PERIOD:
            pwm_get_state(pwm_dev, &state);
            state.period = cam_state->period;
            ret          = pwm_apply_state(pwm_dev, &state);
            break;
        case CAM_PWM_DUTY:
            pwm_get_state(pwm_dev, &state);
            state.duty_cycle = cam_state->duty;
            ret              = pwm_apply_state(pwm_dev, &state);
            break;
        case CAM_PWM_POLAR:
            pwm_get_state(pwm_dev, &state);
            state.polarity = cam_state->polarity;
            ret            = pwm_apply_state(pwm_dev, &state);
            break;
        case CAM_PWM_ONOFF:
            pwm_get_state(pwm_dev, &state);
            state.enabled = cam_state->enabled;
            ret           = pwm_apply_state(pwm_dev, &state);
            break;
        case CAM_PWM_ALL:
            state.period     = cam_state->period;
            state.duty_cycle = cam_state->duty;
            state.polarity   = cam_state->polarity;
            state.enabled    = cam_state->enabled;
            ret              = pwm_apply_state(pwm_dev, &state);
            break;
        default:
            SSTAR_PWM_ERR("pwm config no support\n");
    }

    if (ret < 0)
    {
        SSTAR_PWM_ERR("failed to config pwm\n");
        return ret;
    }

    return 0;
}
EXPORT_SYMBOL(CamPwmConfig);

void CamPwmFree(struct pwm_device *pwm_dev)
{
    pwm_put(pwm_dev);
}
EXPORT_SYMBOL(CamPwmFree);

struct pwm_device *CamPwmRequest(int channel, const char *label)
{
    struct pwm_device *pwm_dev;

    pwm_dev = pwm_request(channel, label);
    if (IS_ERR(pwm_dev))
    {
        SSTAR_PWM_ERR("failed to request pwm dev\n");
        return NULL;
    }

    return pwm_dev;
}
EXPORT_SYMBOL(CamPwmRequest);

#ifdef CONFIG_PM_SLEEP
static int infinity_pwm_suspend(struct platform_device *pdev, pm_message_t state)
{
    struct sstar_pwm_chip *pwm = platform_get_drvdata(pdev);

    SSTAR_PWM_INFO("[PWM] suspend\n");
    if (pwm->irq)
    {
        // free_irq(pwm->irq,PWM_IRQ);
        disable_irq(pwm->irq);
        SSTAR_PWM_INFO("[NOTICE]Disable Pwm Irq %d\n", pwm->irq);
    }
    if (pwm && pwm->clk)
    {
        clk_disable_unprepare(pwm->clk);
    }
    return 0;
}

static int infinity_pwm_resume(struct platform_device *pdev)
{
    int                    i   = 0;
    struct sstar_pwm_chip *pwm = platform_get_drvdata(pdev);

    SSTAR_PWM_INFO("[PWM] resume\n");
    if (pwm && pwm->clk)
    {
        clk_prepare_enable(pwm->clk);
    }
    for (i = 0; i < pwm->chip.npwm; i++)
    {
        MHal_PWMInit(pwm, i);
    }
    if (pwm->irq)
    {
        enable_irq(pwm->irq);
    }
    return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sstar_pwm_of_match_table[] = {{.compatible = "sstar,infinity-pwm"}, {}};

MODULE_DEVICE_TABLE(of, sstar_pwm_of_match_table);

static struct platform_driver sstar_pwm_driver = {
    .remove = sstar_pwm_remove,
    .probe  = sstar_pwm_probe,
#ifdef CONFIG_PM_SLEEP
    .suspend = infinity_pwm_suspend,
    .resume  = infinity_pwm_resume,
#endif
    .driver =
        {
            .name           = "sstar-pwm",
            .owner          = THIS_MODULE,
            .of_match_table = sstar_pwm_of_match_table,
        },
};

module_platform_driver(sstar_pwm_driver);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SStar PWM Driver");
MODULE_LICENSE("GPL v2");
