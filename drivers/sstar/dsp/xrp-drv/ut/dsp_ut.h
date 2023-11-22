/*
 * dsp_ut.h- Sigmastar
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
#include <linux/firmware.h>
#include <linux/wait.h>
#include <linux/atomic.h>
#include "mdrv_dsp.h"
#ifndef _dsp_ut_h_
#define _dsp_ut_h_
typedef struct dsp_ut_core
{
    int                    core;
    MDRV_DSP_AddressMap_t  addrMap[E_MDRV_DSP_ADDRESS_SET_NUM];
    int                    jtag;
    int                    resetVector;
    int                    prid;
    int                    dfs;
    char                   fwPath[256];
    const struct firmware* firmware; // NOLINT
    phys_addr_t            comm_phys;
    char                   irqName[64];
    wait_queue_head_t      dspWQ;
    atomic_t               dspIrqCnt;
} dsp_ut_core;

int __weak  dsp_ut_init(void);
void __weak dsp_ut_exit(void);
#endif
