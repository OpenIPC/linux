/*
 * drv_camclk.h- Sigmastar
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

#ifndef __DRV_CAMCLK_H__
#define __DRV_CAMCLK_H__
#include "drv_camclk_DataType.h"
#include "drv_camclk_st.h"
#include "camclk_id.h"
#include "cam_inter_os.h"
extern CamOsTsem_t gstTsem;
extern u32         gu32CamclkOsLock;
extern u32         gu32CamclkOsLockNum;
#define CAM_CLK_REF_CNT_SIZE (sizeof(HalCamClkTopClk_t) * HAL_CAMCLK_SRC_Id_MAX)
#ifdef CONFIG_SS_DUALOS
#define CAM_CLK_INVALIDATE(addr) (CamOsMemInvalidate((void *)addr, CAM_CLK_REF_CNT_SIZE))
#define CAM_CLK_FLUSH(addr)      (CamOsMemFlush((void *)addr, CAM_CLK_REF_CNT_SIZE))
#define CAM_CLK_OSLOCK()         CamInterOsLock(&gu32CamclkOsLock, gu32CamclkOsLockNum)
#define CAM_CLK_OSUNLOCK()       CamInterOsUnlock(&gu32CamclkOsLock, gu32CamclkOsLockNum)
//#define CAM_CLK_REQOSLOCK()        (gu32CamclkOsLockNum = CamInterOsRequestLock())
#define CAM_CLK_REQOSLOCK() (gu32CamclkOsLockNum = 1)
//#define CAM_CLK_FREEOSLOCK()        (CamInterOsFreeLock(gu32CamclkOsLockNum))
#define CAM_CLK_FREEOSLOCK()
#else
#define CAM_CLK_INVALIDATE(addr)
#define CAM_CLK_FLUSH(addr)
#define CAM_CLK_OSLOCK()
#define CAM_CLK_OSUNLOCK()
#define CAM_CLK_REQOSLOCK()
#define CAM_CLK_FREEOSLOCK()
#endif
#define CAM_CLK_LOCK_SEM()   CamOsTsemDown(&gstTsem)
#define CAM_CLK_UNLOCK_SEM() CamOsTsemUp(&gstTsem)

#define CAM_CLK_INTEROS_PREFIX   (0x434C4B00) // CLK
#define CAM_CLK_INTEROS_SHAREMEM (CAM_CLK_INTEROS_PREFIX | 0x1)

#ifdef CONFIG_CAM_CLK_PROFILING
extern void DrvCamClkOsProf(int mark, const char *name);
#define CAM_CLK_RECORD(name) DrvCamClkOsProf(0, name)
#else
#define CAM_CLK_RECORD(name)
#endif
//------------------------------------------------------------------------------
/// @brief Init Clk Hardware
/// @param[in] Void.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkInit(void);

//------------------------------------------------------------------------------
/// @brief deInit Clk Hardware
/// @param[in] Void.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkDeinit(void);
CAMCLK_RET_e DrvCamClkImplInit(void);
CAMCLK_RET_e DrvCamClkImplDeinit(void);
CAMCLK_RET_e DrvCamClkImplGetIdAllParent(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetClk(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetRate(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetRoundRateParent(void *pCfg);
CAMCLK_RET_e DrvCamClkImplSetOnOff(void *pCfg);
CAMCLK_RET_e DrvCamClkImplSetParent(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetOnOff(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetHwOnOff(HalCamClkSrcId_e u32Id, u8 *p8bEn);
CAMCLK_RET_e DrvCamClkImplGetParent(void *pCfg);
CAMCLK_RET_e DrvCamClkImplGetHwSelect(HalCamClkSrcId_e u32Id, u16 *p16Select);
u32          DrvCamclkDebugHandlerShow(char *buf);
u32          DrvCamclkDebugClkShow(char *buf);
u32          DrvCamclkDebugDebugLvlShow(char *buf);
void         DrvCamclkDebugDebugLvlStore(const char *buf);
void         DrvCamClkDebugInit(void);
void         DrvCamClkSysfsInit(void *device);
CAMCLK_RET_e DrvCamClkOsGetShareMemory(DrvCamClkShareMemoryType_e enType);
void         DrvCamClkOsPutShareMemory(DrvCamClkShareMemoryType_e enType);
void         DrvCamClkOsPrepareShareMemory(DrvCamClkShareMemoryType_e enType);
void         DrvCamClkOsDestroyShareMemory(DrvCamClkShareMemoryType_e enType);
int          DrvCamClkImplDisableUnuseClk(void);
CAMCLK_RET_e DrvCamClkImplGetAdjustRoundRate(DrvCamClkGetRoundRateParent_t *pstCfg);
CAMCLK_RET_e DrvCamClkImplSetAdjustRate(DrvCamClkSetAdjRoundRate_t *pstCfg);
CAMCLK_RET_e DrvCamClkResume(void);
CAMCLK_RET_e DrvCamClkImplRestoreClk(void);
#endif
