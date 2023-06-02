/**
    NVT secure mode
    To handle secure SMC function
    @file       smc.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/arm-smccc.h>
#include <asm/mach/map.h>
#include <asm/hardware/cache-l2x0.h>
#include <plat/hardware.h>
#include "common.h"
#include "mach/smc_func_id/hw_smc_id.h"
#include "mach/smc_func_id/software_smc_id.h"
#include "mach/smc_func_id/custom_smc_id.h"

/* Simple wrapper functions to be able to use a function pointer */
static void nvt_smccc_smc(unsigned long a0, unsigned long a1,
							unsigned long a2, unsigned long a3,
							unsigned long a4, unsigned long a5,
							unsigned long a6, unsigned long a7,
							struct arm_smccc_res *res)
{
	arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

void nvt_smc_l2_write_sec(unsigned long val, unsigned reg)
{
	/*
	 * We can't write to secure registers as we are in non-secure
	 * mode, until we have some SMI service available.
	 */
	struct arm_smccc_res res = {0};
	unsigned long smc_op = 0;
	unsigned long mask = 0xFFFFFFFF;

	switch (reg) {
	case L2X0_CTRL:
		smc_op = NVT_L2CACHE_CTRL;
		break;

	case L2X0_AUX_CTRL:
		smc_op = NVT_L2CACHE_AUX_CTRL;
		break;

	case L2X0_DEBUG_CTRL:
		smc_op = NVT_L2CACHE_DBG_CTRL;
		break;

	case L310_PREFETCH_CTRL:
		smc_op = NVT_L2CACHE_PREFETCH_CTRL;
		break;

	case L310_TAG_LATENCY_CTRL:
		smc_op = NVT_L2CACHE_TAG_LATENCY_CTRL;
		break;

	case L310_DATA_LATENCY_CTRL:
		smc_op = NVT_L2CACHE_DATA_LATENCY_CTRL;
		break;

	case L310_POWER_CTRL:
		pr_info_once("NVT L2C310: ROM does not support power control setting\n");
		return;

	default:
		WARN_ONCE(1, "NVT L2C310: ignoring write to reg 0x%x\n", reg);
		return;
	}
	nvt_smccc_smc(smc_op, val, mask, 0, 0, 0, 0, 0, &res);
}
