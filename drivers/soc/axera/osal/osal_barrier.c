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
#include <asm/barrier.h>
#include "osal_ax.h"

void AX_OSAL_SYNC_mb(void)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		smp_mb();
	} else {
		mb();
	}
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mb);

void AX_OSAL_SYNC_rmb(void)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		smp_rmb();
	} else {
		rmb();
	}
}

EXPORT_SYMBOL(AX_OSAL_SYNC_rmb);

void AX_OSAL_SYNC_wmb(void)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		smp_wmb();
	} else {
		wmb();
	}
}

EXPORT_SYMBOL(AX_OSAL_SYNC_wmb);

void AX_OSAL_SYNC_isb(void)
{
	isb();
}

EXPORT_SYMBOL(AX_OSAL_SYNC_isb);

void AX_OSAL_SYNC_dsb(void)
{
	dsb(sy);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_dsb);

void AX_OSAL_SYNC_dmb(void)
{
	dmb(sy);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_dmb);
