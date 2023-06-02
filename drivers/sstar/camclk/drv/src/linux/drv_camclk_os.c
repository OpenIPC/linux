/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#define  DRV_CAMCLK_C
#include "cam_os_wrapper.h"
#include "drv_camclk_st.h"
#include "drv_camclk.h"
#include "drv_camclk_Api.h"
#include "hal_camclk_if.h"
#include "camclk_dbg.h"
//==================================
#include <linux/ioport.h>
#include <asm/io.h>
#ifdef CONFIG_SS_DUALOS
void _DrvCamClkOsGetShareMemorySize(DrvCamClkShareMemoryType_e enType, u32 *u32Size)
{
    if(enType == DRV_CAMCLK_SHAREMEM_TOPCURRENT)
    {
        *u32Size = CAM_CLK_REF_CNT_SIZE;
    }
    else
    {
        *u32Size = 0;
    }
}
unsigned long signal_rtos(u32 type, u32 arg1, u32 arg2, u32 arg3);
void _DrvCamClkOsGetShareMemoryAddr(void **pMem,u32 u32Size)
{
    static struct resource  *_rtkres;
    // *pMem = XX();
    CAMCLKDBG("[%s @ %d]id:%lx\n", __FUNCTION__, __LINE__,CAM_CLK_INTEROS_SHAREMEM);
    *pMem = (void *)signal_rtos(CAM_CLK_INTEROS_SHAREMEM, 0, 0,0);
    if(*pMem && (u32)(*pMem)!=-1)
    {
        CAMCLKDBG("[%s @ %d]addr :%p\n", __FUNCTION__, __LINE__,*pMem);
        //*pMem = CamOsPhyMemMap(*pMem,u32Size,0);
        _rtkres = request_mem_region((u32)*pMem, u32Size, "camclkcur");
        *pMem = ioremap(_rtkres->start, resource_size(_rtkres));
        CAMCLKDBG("[%s @ %d]addr :%p\n", __FUNCTION__, __LINE__,*pMem);
        CAM_CLK_INVALIDATE(*pMem);
    }
}
void _DrvCamClkOsSyncShareMemory(void *pMem)
{
    HalCamClkSrcId_e eId;
    HalCamClkTopClk_t *pstTopCurrent = pMem;
    CAMCLKDBG("[%s @ %d]addr :%p\n", __FUNCTION__, __LINE__,pMem);

    if(pMem)
    {
        //DUALOS_LOCK_INIT;
        //DUALOS_LOCK;
        CAM_CLK_LOCK_SEM();
        for(eId = 0;eId<HAL_CAMCLK_SRC_Id_MAX;eId++)
        {
            if(gCamClkTopCurrent[eId].u8RefCnt)
            {
                pstTopCurrent[eId].u8RefCnt += gCamClkTopCurrent[eId].u8RefCnt;
            }
        }
        gCamClkTopCurrent = pstTopCurrent;
        CAM_CLK_UNLOCK_SEM();
        //DUALOS_UNLOCK;
    }
}
#endif
//======================================
#ifdef CONFIG_CAM_CLK_PROFILING
extern void recode_timestamp(int mark, const char* name);
//extern void BootTimestampRecord(int mark, const char* name);
void DrvCamClkOsProf(int mark, const char* name)
{
    recode_timestamp(mark,name);
}
#endif
CAMCLK_RET_e DrvCamClkOsGetShareMemory(DrvCamClkShareMemoryType_e enType)
{
    CAMCLK_RET_e bRet = CAMCLK_RET_OK;
#ifdef CONFIG_SS_DUALOS
    u32 u32Size;
    void *shMem = NULL;
    _DrvCamClkOsGetShareMemorySize(enType,&u32Size);
    if(u32Size)
    {
        _DrvCamClkOsGetShareMemoryAddr(&shMem,u32Size);
        _DrvCamClkOsSyncShareMemory(shMem);
    }
#endif
    return bRet;
}
void DrvCamClkOsPutShareMemory(DrvCamClkShareMemoryType_e enType,void *pMem)
{
#ifdef CONFIG_SS_DUALOS
    u32 u32Size;

    _DrvCamClkOsGetShareMemorySize(enType,&u32Size);
    if(u32Size)
    {
    }
#endif
}
