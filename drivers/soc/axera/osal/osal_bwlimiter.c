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
#include <linux/soc/axera/ax_bw_limiter.h>
#include "osal_ax.h"

int __weak ax_bw_limiter_register_with_clk(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk)
{
	return 0;
}

int __weak ax_bw_limiter_register_with_val(SUB_SYS_BW_LIMITERS sub_sys_bw, u32 work_clk)
{
	return 0;
}

int __weak ax_bw_limiter_unregister(SUB_SYS_BW_LIMITERS sub_sys_bw, void *clk)
{
	return 0;
}

int __weak ax_bw_limiter_refresh_limiter(SUB_SYS_BW_LIMITERS sub_sys_bw)
{
	return 0;
}

int AX_OSAL_DEV_bwlimiter_register_with_clk(SUB_SYS_BW_LIMITERS sub_sys_bw, void * pclk)
{
	return ax_bw_limiter_register_with_clk(sub_sys_bw, pclk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_bwlimiter_register_with_clk);

int AX_OSAL_DEV_bwlimiter_register_with_val(SUB_SYS_BW_LIMITERS sub_sys_bw, unsigned int clk)
{
	return ax_bw_limiter_register_with_val(sub_sys_bw, clk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_bwlimiter_register_with_val);

int AX_OSAL_DEV_bwlimiter_unregister(SUB_SYS_BW_LIMITERS sub_sys_bw, void * pclk)
{
	return ax_bw_limiter_unregister(sub_sys_bw, pclk);
}

EXPORT_SYMBOL(AX_OSAL_DEV_bwlimiter_unregister);

int AX_OSAL_DEV_bwlimiter_refresh_limiter(SUB_SYS_BW_LIMITERS sub_sys_bw)
{
	return ax_bw_limiter_refresh_limiter(sub_sys_bw);
}

EXPORT_SYMBOL(AX_OSAL_DEV_bwlimiter_refresh_limiter);
