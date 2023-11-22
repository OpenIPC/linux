/*
 * mhal_bw.h- Sigmastar
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
#ifndef __HAL_BW__
#define __HAL_BW__

#include "mdrv_types.h"
#include "mdrv_miu.h"

#define MIU_ARB_CLIENT_NUM (0x40) // 16 clients per group, total 4 groups
#define MIU_IDX(c)         (((c - '0') > MIU_NUM) ? 0 : c - '0')

#define INTERVAL (4)
#define DURATION (600)
#define KMSG     (1)

#ifndef MIU_NUM
#define MIU_NUM (1)
#endif

// 0x15 [7]
#define REG_256_DEB_SEL (BIT7)

#define REG_DEB_SEL (0xD << 2)
// 0xD[7:4]
#define REESPONSE_TIME  (0x0 << 4)
#define MIN_EFFICIENCY  (0x3 << 4)
#define REAL_UTILIZTION (0x4 << 4)
#define MAX_UTILIZTION  (0x5 << 4)
#define REAL_OCCUPANCY  (0x2 << 4)
#define MAX_OCCUPANCY   (0x6 << 4)
// 0xD[2]
#define AVG_EFFICIENCY (0x1 << 2)
// 0xD[0]
//    #define ENABLE ( 0x1 << 0 )

// 0x29[2:0]
#define REG_SUM_STEP         (0x29 << 2)
#define REQ_STATUS_PERIOD_1  (0)
#define REQ_STATUS_PERIOD_4  (1)
#define REQ_STATUS_PERIOD_8  (2)
#define REQ_STATUS_PERIOD_16 (3)
#define REQ_STATUS_PERIOD_32 (4)

#define BW_AVG_PERIOD_1  (0 << 4)
#define BW_AVG_PERIOD_4  (1 << 4)
#define BW_AVG_PERIOD_8  (2 << 4)
#define BW_AVG_PERIOD_16 (3 << 4)
#define BW_AVG_PERIOD_32 (4 << 4)

#define BW_STATUS_PERIOD_1  (0 << 8)
#define BW_STATUS_PERIOD_4  (1 << 8)
#define BW_STATUS_PERIOD_8  (2 << 8)
#define BW_STATUS_PERIOD_16 (3 << 8)
#define BW_STATUS_PERIOD_32 (4 << 8)
#define DEF_BY_CHIP         (0)

const char* halClientIDTName(int id);
void        halBWResetFunc(int bank);
void        halBWDebSelect(int bank, int id);
void        halBWFuncSelect(int bank, int id, int type);
void        halBWSetSumStep(int bank, int id, int type);
short       halBWReadBus(int bank);
void        halBWInit(int bank, int id);
void        halBWEffiMinConfig(int bank, int id);
void        halBWEffiRealConfig(int bank, int id);
void        halBWEffiMinPerConfig(int bank, int id);
void        halBWEffiAvgPerConfig(int bank, int id);
void        halBWEffiMaxPerConfig(int bank, int id);
void        halBWOCCRealPerConfig(int bank, int id);
void        halBWOCCMaxPerConfig(int bank, int id);
#endif
