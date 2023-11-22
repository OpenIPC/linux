/*
 * mhal_spinand_chip_config.c- Sigmastar
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
#include "mhal_spinand.h"
#include "reg_spinand.h"

extern hal_fsp_t _hal_fsp;

void HAL_SPINAND_Chip_Config(void)
{
    _hal_fsp.u32FspBaseAddr  = I3_RIU_PM_BASE + BK_PM_FSP;
    _hal_fsp.u32QspiBaseAddr = I3_RIU_PM_BASE + BK_PM_QSPI;
}
