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

#define MIU_IDX(c) (((c - '0') > MIU_NUM) ? 0 : c - '0')

#define INTERVAL (4)
#define DURATION (500)
#define KMSG     (1)

#ifndef MIU_NUM
#define MIU_NUM (1)
#endif

#ifndef MIU_BW_BANK_NUM
#define MIU_BW_BANK_NUM (1)
#endif

typedef enum
{
    MIU_BW_WCMD_EFFI = 0,
    MIU_BW_WCMD_AVG,
    MIU_BW_WCMD_MAX,
    MIU_BW_WCMD_AVG_EFFI,
    MIU_BW_WCMD_MAX_EFFI,
    MIU_BW_RCMD_EFFI,
    MIU_BW_RCMD_AVG,
    MIU_BW_RCMD_MAX,
    MIU_BW_RCMD_AVG_EFFI,
    MIU_BW_RCMD_MAX_EFFI,

    MIU_BW_WCMD_CMD_ACT,
    MIU_BW_WCMD_BA_RWS,
    MIU_BW_WCMD_BA_ORI,
    MIU_BW_WCMD_BA_NOR,
    MIU_BW_WCMD_BW_AVG,
    MIU_BW_WCMD_BW_MAX,
    MIU_BW_WCMD_BW_CMD,
    MIU_BW_WCMD_BW_ACT,
    MIU_BW_WCMD_BW_W2P,
    MIU_BW_WCMD_BW_RWT,
    MIU_BW_WCMD_BW_END,
    MIU_BW_RCMD_CMD_ACT,
    MIU_BW_RCMD_BA_RWS,
    MIU_BW_RCMD_BA_ORI,
    MIU_BW_RCMD_BA_NOR,
    MIU_BW_RCMD_BW_AVG,
    MIU_BW_RCMD_BW_MAX,
    MIU_BW_RCMD_BW_CMD,
    MIU_BW_RCMD_BW_ACT,
    MIU_BW_RCMD_BW_R2P,
    MIU_BW_RCMD_BW_RWT,
    MIU_BW_RCMD_BW_END,
} eMiuBWCmd;

typedef enum
{
    MIU_BW_HDR_DDRINFO = 0,
} eMiuBWHdrSel;

#define REG_BW_RPT (0x03 << 2)

#define REG_BW_RPT (0x03 << 2)

#define REG_BW_WEFFI (0x08 << 2)
#define REG_BW_REFFI (0x0D << 2)

#define REG_BW_RPT_WID (0x0E << 2)
#define REG_BW_RPT_RID (0x0F << 2)

const char * halClientIDTName(int id);
void         halBWResetFunc(int bank, int wcmd);
void         halBWDebSelect(int bank, int id);
void         halBWFuncSelect(int bank, int id, int type);
void         halBWSetSumStep(int bank, int id, int type);
short        halBWReadBus(int bank, int cmd, int id);
unsigned int halBWReadPercentValue(int bank, int cmd, int val, int dec);
void         halBWInit(int bank, int id, int wcmd);
void         halBWEffiMinConfig(int bank, int id, int wcmd);
void         halBWEffiRealConfig(int bank, int id, int wcmd);
void         halBWEffiMinPerConfig(int bank, int id, int wcmd);
void         halBWEffiAvgPerConfig(int bank, int id, int wcmd);
void         halBWEffiMaxPerConfig(int bank, int id, int wcmd);
void         halBWOCCRealPerConfig(int bank, int id, int wcmd);
void         halBWOCCMaxPerConfig(int bank, int id, int wcmd);
void         halBWMeasureRegsStore(const char *buf, int cmd_sel, int wcmd);
void         halBWMeasureTrigger(int hdr_sel, int cli_id, int wcmd);
void         halBWGetMeasureHeader(char *header, int hdr_sel, int wcmd);
void         halBWGetMeasureStat(short miu_ch, char *header, int hdr_size, int hdr_sel, int cli_id, int wcmd);

#ifdef CONFIG_CAM_CLK
unsigned int halBWGetDramInfo(char *str, char *end, unsigned int dram_freq, unsigned int miupll_freq);
#else
unsigned int halBWGetDramInfo(char *str, char *end);
#endif
int halSetCheckIPBwLock(const char *buf);
#endif
