/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "osal_ax.h"

unsigned int AX_OSAL___percpu *AX_OSAL_alloc_percpu_u32(void)
{
	return alloc_percpu(unsigned int);
}
EXPORT_SYMBOL(AX_OSAL_alloc_percpu_u32);

unsigned int *AX_OSAL_this_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr)
{
	return this_cpu_ptr(ptr);
}
EXPORT_SYMBOL(AX_OSAL_this_cpu_ptr_u32);

unsigned int *AX_OSAL_get_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr)
{
	return get_cpu_ptr(ptr);
}
EXPORT_SYMBOL(AX_OSAL_get_cpu_ptr_u32);

void AX_OSAL_put_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr)
{
	put_cpu_ptr(ptr);
}
EXPORT_SYMBOL(AX_OSAL_put_cpu_ptr_u32);

unsigned int *AX_OSAL_per_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr, int cpu)
{
	return per_cpu_ptr(ptr, cpu);
}
EXPORT_SYMBOL(AX_OSAL_per_cpu_ptr_u32);

void AX_OSAL_free_percpu_u32(unsigned int AX_OSAL___percpu *ptr)
{
	free_percpu(ptr);
}
EXPORT_SYMBOL(AX_OSAL_free_percpu_u32);
