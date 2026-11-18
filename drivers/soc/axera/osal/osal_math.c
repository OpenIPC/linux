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
#include <linux/string.h>
#include <linux/version.h>
#include <linux/random.h>

#include "osal_lib_ax.h"

u64 AX_OSAL_LIB_div_u64(u64 dividend, unsigned int divisor)
{
	return div_u64(dividend, divisor);
}

EXPORT_SYMBOL(AX_OSAL_LIB_div_u64);

s64 AX_OSAL_LIB_div_s64(s64 dividend, int divisor)
{
	return div_s64(dividend, divisor);
}

EXPORT_SYMBOL(AX_OSAL_LIB_div_s64);

u64 AX_OSAL_LIB_div64_u64(u64 dividend, u64 divisor)
{
	return div64_u64(dividend, divisor);
}

EXPORT_SYMBOL(AX_OSAL_LIB_div64_u64);

s64 AX_OSAL_LIB_LIB_div64_s64(s64 dividend, s64 divisor)
{
	return div64_s64(dividend, divisor);
}

EXPORT_SYMBOL(AX_OSAL_LIB_LIB_div64_s64);

u64 AX_OSAL_LIB_div_u64_rem(u64 dividend, unsigned int divisor)
{
	unsigned int remainder;

	div_u64_rem(dividend, divisor, &remainder);

	return remainder;
}

EXPORT_SYMBOL(AX_OSAL_LIB_div_u64_rem);

s64 AX_OSAL_LIB_div_s64_rem(s64 dividend, int divisor)
{
	int remainder;

	div_s64_rem(dividend, divisor, &remainder);

	return remainder;
}

EXPORT_SYMBOL(AX_OSAL_LIB_div_s64_rem);

u64 AX_OSAL_LIB_div64_u64_rem(u64 dividend, u64 divisor)
{
	unsigned long long remainder;

	div64_u64_rem(dividend, divisor, &remainder);

	return remainder;
}

EXPORT_SYMBOL(AX_OSAL_LIB_div64_u64_rem);

unsigned int AX_OSAL_LIB_random()
{
	return get_random_int();
}

EXPORT_SYMBOL(AX_OSAL_LIB_random);
