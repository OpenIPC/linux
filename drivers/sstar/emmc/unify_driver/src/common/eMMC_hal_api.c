/*
 * eMMC_hal_api.c- Sigmastar
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

#include "eMMC.h"
#include "eMMC_err_codes.h"

#if defined(UNIFIED_eMMC_DRIVER) && UNIFIED_eMMC_DRIVER

//========================================================
// HAL pre-processors
//========================================================

//========================================================
// HAL APIs
//========================================================
int HAL_EMMC_Get_CLK(U8 emmc_ip, unsigned int *pu32ClkKHz)
{
    *pu32ClkKHz = g_eMMCDrv[emmc_ip].u32_ClkKHz;

    return eMMC_ST_SUCCESS;
}

EXPORT_SYMBOL(HAL_EMMC_Get_CLK);

#endif // UNIFIED_eMMC_DRIVER
