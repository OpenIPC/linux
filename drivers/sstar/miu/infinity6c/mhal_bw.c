/*
 * mhal_bw.c- Sigmastar
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
#include <linux/printk.h>
#include <linux/module.h>
#if defined(CONFIG_COMPAT)
#include <linux/compat.h>
#endif
#include "MsTypes.h"
#include "mdrv_types.h"
#include "mdrv_system.h"
#include "registers.h"
#include "ms_platform.h"
#include "mhal_bw.h"

void halBWDebSelect(int bank, int id)
{
    OUTREG16((bank + REG_ID_15), (id & 0x40) ? REG_256_DEB_SEL : 0x00);
}

void halBWSetSumStep(int bank, int id, int period)
{
    // 0:   1/1M cycle
    // 1:   4/1M cycle
    // 2:   8/1M cycle
    // 3: 16/1M cycle
    // 4: 32/1M cycle
    OUTREG16((bank + REG_SUM_STEP), period); // 0x29
}

void halBWResetFunc(int bank)
{
    OUTREG16((bank + REG_DEB_SEL), 0); // reset all
}

void halBWFuncSelect(int bank, int id, int type)
{
    OUTREG16((bank + REG_DEB_SEL), ((id << 8) | type | ENABLE)); // set to read bandwidth
}

short halBWReadBus(int bank)
{
    return INREG16(bank + REG_ID_0E);
}

void halBWInit(int bank, int id)
{
    halBWDebSelect(bank, id);
    halBWResetFunc(bank);
}

void halBWEffiMinConfig(int bank, int id)
{
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, MIN_EFFICIENCY | AVG_EFFICIENCY);
}

void halBWEffiRealConfig(int bank, int id)
{
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, REAL_UTILIZTION); // 0x40
}

void halBWEffiMinPerConfig(int bank, int id)
{
    halBWResetFunc(bank);
    /*000*/
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, MIN_EFFICIENCY | AVG_EFFICIENCY); // 0x35
}

void halBWEffiAvgPerConfig(int bank, int id)
{
    //    halBWResetFunc(bank);
    /*303*/
    halBWSetSumStep(bank, id, REQ_STATUS_PERIOD_16 | BW_STATUS_PERIOD_16);
    halBWFuncSelect(bank, id, REAL_UTILIZTION); // 0x40
}

void halBWEffiMaxPerConfig(int bank, int id)
{
    //    halBWResetFunc(bank);
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, MAX_UTILIZTION); // 0x50
}

void halBWOCCRealPerConfig(int bank, int id)
{
    //    halBWResetFunc(bank);
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, REAL_OCCUPANCY); // 0x20
}

void halBWOCCMaxPerConfig(int bank, int id)
{
    //    halBWResetFunc(bank);
    halBWSetSumStep(bank, id, DEF_BY_CHIP);
    halBWFuncSelect(bank, id, MAX_OCCUPANCY); // 0x60
}
