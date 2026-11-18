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
#include <asm/io.h>
#include <linux/uaccess.h>
#include "osal_ax.h"
#include "osal_dev_ax.h"

void *AX_OSAL_DEV_ioremap(unsigned long phys_addr, unsigned long size)
{
	return ioremap(phys_addr, size);
}

EXPORT_SYMBOL(AX_OSAL_DEV_ioremap);

void *AX_OSAL_DEV_ioremap_nocache(unsigned long phys_addr, unsigned long size)
{
	return ioremap_wc(phys_addr, size);
}

EXPORT_SYMBOL(AX_OSAL_DEV_ioremap_nocache);

void *AX_OSAL_DEV_ioremap_cache(unsigned long phys_addr, unsigned long size)
{
	return ioremap_cache(phys_addr, size);
}

EXPORT_SYMBOL(AX_OSAL_DEV_ioremap_cache);

void AX_OSAL_DEV_iounmap(void * addr)
{
	iounmap(addr);
}

EXPORT_SYMBOL(AX_OSAL_DEV_iounmap);

unsigned long AX_OSAL_DEV_copy_from_user(void * to, const void * from, unsigned long n)
{
	return copy_from_user(to, from, n);
}

EXPORT_SYMBOL(AX_OSAL_DEV_copy_from_user);

unsigned long AX_OSAL_DEV_copy_to_user(void * to, const void * from, unsigned long n)
{
	return copy_to_user(to, from, n);
}

EXPORT_SYMBOL(AX_OSAL_DEV_copy_to_user);
