/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/pm_opp.h>
#include <linux/devfreq.h>
#include <linux/pm_qos.h>

#include <linux/platform_device.h>
#include "linux/device.h"
#include <linux/version.h>

#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"
#include "osal_lib_ax.h"

static int AX_OSAL_DEV_comon_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct dev_pm_opp *opp;
	unsigned long rate;
	unsigned long volt;
	struct platform_device *pdev = to_platform_device(dev);

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		return PTR_ERR(opp);
	}

	rate = dev_pm_opp_get_freq(opp);
	volt = dev_pm_opp_get_voltage(opp);

	((struct AX_DEVFREQ_DEV_PROFILE *)pdev->axera_devdfs_ptr)->target((void *) dev, rate, volt);
	dev_pm_opp_put(opp);
	return 0;
}

static int AX_OSAL_DEV_comon_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	return 0;
}

static int AX_OSAL_DEV_comon_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct platform_device *pdev = to_platform_device(dev);
	*freq = ((struct AX_DEVFREQ_DEV_PROFILE *)pdev->axera_devdfs_ptr)->get_cur_freq((void *) dev);
	return 0;
}

static void AX_OSAL_DEV_comon_exit(struct device *dev)
{
	return;
}

int AX_OSAL_DEV_pm_opp_of_add_table(void * pdev)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return dev_pm_opp_of_add_table(dev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_pm_opp_of_add_table);

void AX_OSAL_DEV_pm_opp_of_remove_table(void * pdev)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	dev_pm_opp_remove_table(dev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_pm_opp_of_remove_table);

int AX_OSAL_DEV_devm_devfreq_add_device(void * pdev, struct AX_DEVFREQ_DEV_PROFILE *ax_profile,
					   const char *governor_name, void *data)
{
	struct devfreq *axdevdfs;
	struct devfreq_dev_profile *pdevfile;
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	pdevfile = devm_kzalloc(dev, sizeof(struct devfreq_dev_profile), GFP_KERNEL);

	pdevfile->initial_freq = ax_profile->initial_freq;
	pdevfile->polling_ms = ax_profile->polling_ms;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	pdevfile->timer = ax_profile->timer;
	pdevfile->is_cooling_device = ax_profile->is_cooling_device;
#endif

	pdevfile->target = AX_OSAL_DEV_comon_target;
	pdevfile->get_dev_status = AX_OSAL_DEV_comon_get_dev_status;
	pdevfile->get_cur_freq = AX_OSAL_DEV_comon_get_cur_freq;
	pdevfile->exit = AX_OSAL_DEV_comon_exit;
	pdevfile->freq_table = ax_profile->freq_table;
	pdevfile->max_state = ax_profile->max_state;

	pvdev->axera_devdfs_ptr = ax_profile;
	axdevdfs = devm_devfreq_add_device(dev, pdevfile, governor_name, data);
	if (!axdevdfs) {
		printk("cannot add device to devfreq!\n");
		return -1;
	}

	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_devm_devfreq_add_device);

int AX_OSAL_DEV_pm_opp_of_disable(void * pdev, unsigned long freq)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return dev_pm_opp_disable(dev, freq);
}

EXPORT_SYMBOL(AX_OSAL_DEV_pm_opp_of_disable);

void AX_OSAL_DEV_pm_opp_of_remove(void * pdev, unsigned long freq)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	dev_pm_opp_remove(dev, freq);
}

EXPORT_SYMBOL(AX_OSAL_DEV_pm_opp_of_remove);

int AX_OSAL_DEV_pm_opp_of_add(void * pdev, unsigned long freq, unsigned long volt)
{
	struct platform_device *pvdev = (struct platform_device *)pdev;
	struct device *dev = &pvdev->dev;
	return dev_pm_opp_add(dev, freq, volt);
}

EXPORT_SYMBOL(AX_OSAL_DEV_pm_opp_of_add);