/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "osal_pm_ax.h"

int AX_OSAL_PM_WakeupLock(char * lock_name)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_PM_WakeupLock);

int AX_OSAL_PM_WakeupUnlock(char * lock_name)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_PM_WakeupUnlock);

int AX_OSAL_PM_SetLevel(int pm_level)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_PM_SetLevel);

int AX_OSAL_PM_GetLevel(int * pm_level)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_PM_GetLevel);
