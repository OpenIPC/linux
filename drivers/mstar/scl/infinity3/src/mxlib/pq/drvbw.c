// $Change: 617839 $
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////


#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif
#include "MsCommon.h"
#include "MsTypes.h"
#include "hwreg_utility2.h"
#include "drvXC_IOPort.h"
#include "apiXC.h"
#include "color_reg.h"
#include "drvPQ_Define.h"
#include "drvPQ.h"
#include "QualityMode.h"


extern  MS_PQ_Mode_Info  _stMode_Info[PQ_MAX_WINDOW];

#define BW_DBG(x) //x



typedef struct
{
    MS_U8 *pIPTable;
    MS_U8 u8TabNums;
    MS_U8 u8TabIdx;
} TAB_Info;

#include "Cedric_QualityMap_BW.c"

static  MS_U8 *pBwTable=(void*)BWTABLE;

static void _MDrv_BW_LoadTable(MS_U8 u8TabIdx)
{
    TAB_Info tab_Info;
    tab_Info.pIPTable  = pBwTable;
    tab_Info.u8TabNums = BWTABLE_NUMS;
    tab_Info.u8TabIdx = u8TabIdx;

    BW_DBG(printf("[BW]LoadTable\n"));

    _MDrv_BW_DumpTable(&tab_Info);
}

void MDrv_BW_LoadInitTable(void)
{
    TAB_Info tab_Info;
    tab_Info.pIPTable = (void*)BWTABLE_COM;
    tab_Info.u8TabNums = 1;
    tab_Info.u8TabIdx = 0;

    BW_DBG(printf("[BW]LoadCommTable\n"));
    _MDrv_BW_DumpTable(&tab_Info);
}

#ifdef __AEONR2__ //janus
void MDrv_BW_LoadTableByContext(PQ_WIN eWindow)
{
    MS_U16 u16Input_HSize;
    MS_U16 u16Input_VSize;
    MS_U16 u16Input_VFreq;
    MS_BOOL bIsInterlaced;
    MS_U8 u8TabIdx;
    PQ_INPUT_SOURCE_TYPE pqInputSourceType;

    u16Input_HSize = _stMode_Info[eWindow].u16input_hsize;
    u16Input_VSize = _stMode_Info[eWindow].u16input_vsize;
    u16Input_VFreq = _stMode_Info[eWindow].u16input_vfreq;
    bIsInterlaced  = _stMode_Info[eWindow].bInterlace;
    pqInputSourceType = MDrv_PQ_GetInputSourceType(eWindow);

    if ( QM_IsSourceMultiMedia(pqInputSourceType) )
    {
        if(u16Input_HSize>1280 && u16Input_VSize>=900)
            u8TabIdx = BWTABLE_MM;
        else
            u8TabIdx = BWTABLE_MM_720p;
    }
    else if (u16Input_HSize>=1440 && u16Input_VSize >= 900 && !bIsInterlaced && (u16Input_VFreq >= 490))
    {
        // 1920x1080@50/60p
        if( QM_IsSourceVGA(pqInputSourceType) )
        {
            u8TabIdx = BWTABLE_PC_mode;
        }
        else
        {
            u8TabIdx = BWTABLE_1080p_mode;
        }
    }
    else
    {
        if ( QM_IsSourceATV(pqInputSourceType) || QM_IsSourceAV(pqInputSourceType) || QM_IsSourceSV(pqInputSourceType) )
            u8TabIdx = BWTABLE_Normal_mode;
        else
            u8TabIdx = BWTABLE_case6;
    }

    pBwTable=_MDrv_BW_DDR_Speed();

    _MDrv_BW_LoadTable(u8TabIdx);
}

#else

void MDrv_BW_LoadTableByContext(PQ_WIN eWindow)
{

    MS_U16 u16Input_HSize;
    MS_U16 u16Input_VSize;
    MS_BOOL bIsInterlaced;
    MS_U8 u8TabIdx;
    MS_U16 u16Input_VFreq;
    MS_U16  u16input_vtotal;

    u16Input_HSize = _stMode_Info[eWindow].u16input_hsize;
    u16Input_VSize = _stMode_Info[eWindow].u16input_vsize;
    bIsInterlaced  = _stMode_Info[eWindow].bInterlace;
    u16Input_VFreq = _stMode_Info[eWindow].u16input_vfreq;
    u16input_vtotal = _stMode_Info[eWindow].u16input_vtotal;

//![for temporary A3-WinCE6 porting, 20120109]
#if (defined(MSOS_TYPE_CE) || defined(MSOS_TYPE_LINUX_KERNEL))

    if (u16Input_HSize>=1440 && u16Input_VSize >= 900)
    {
        u8TabIdx = BWTABLE_1080p_mode;
    }
    else
    {
        // printf(">>>444  BWTABLE_Normal_mode\n");
        u8TabIdx = BWTABLE_Normal_mode;
    }

#else

    #if ((CHIP_FAMILY_TYPE == CHIP_FAMILY_S7LD) && !ENABLE_MIU_1)

        if(QM_IsSourceMultiMedia(MDrv_PQ_GetInputSourceType(eWindow)))
        {
            u8TabIdx = BWTABLE_MM;
        }
        else if (u16Input_HSize>=1440 && u16Input_VSize >= 900 && !bIsInterlaced && (u16Input_VFreq >= 490))
        {
            // 1920x1080@50/60p
            if(QM_IsSourceVGA(MDrv_PQ_GetInputSourceType(eWindow)))
            {
                u8TabIdx = BWTABLE_PC_mode;
            }
            else
            {
                u8TabIdx = BWTABLE_1080p_mode;
            }
        }
        else
        {
            if(QM_IsSourceDTV(MDrv_PQ_GetInputSourceType(eWindow)) && u16Input_VSize >= 720)  //DTV 720P and 1080i case
            {
                //printf(">>>111  BWTABLE_720p_1080i_mode\n");
                u8TabIdx = BWTABLE_720p_1080i_mode;
            }
            else
            {
                //printf(">>>222  BWTABLE_Normal_mode\n");
                u8TabIdx = BWTABLE_Normal_mode;
            }
        }

    #elif (CHIP_FAMILY_TYPE == CHIP_FAMILY_S7LD)

        if(u16Input_HSize>=1024 && u16Input_VSize >= 768 && !bIsInterlaced && (u16Input_VFreq >= 490) && QM_IsSourceVGA(MDrv_PQ_GetInputSourceType(eWindow)))
        {
            u8TabIdx = BWTABLE_PC_mode;
        }
        else if (u16Input_HSize>=1440 && u16Input_VSize >= 900 && !bIsInterlaced)
        {
            u8TabIdx = BWTABLE_1080p_mode;
        }
        else if(QM_IsSourceDTV(MDrv_PQ_GetInputSourceType(eWindow)) && u16Input_VSize >= 720)  //DTV 720P and 1080i case
        {
            //printf(">>>333  BWTABLE_720p_1080i_mode\n");
            u8TabIdx = BWTABLE_720p_1080i_mode;
        }
        else
        {
           // printf(">>>444  BWTABLE_Normal_mode\n");
            u8TabIdx = BWTABLE_Normal_mode;
        }

    #elif (CHIP_FAMILY_TYPE == CHIP_FAMILY_A6)

        if(QM_IsSourceMultiMedia(MDrv_PQ_GetInputSourceType(eWindow)) && u16Input_VSize > 2000)  //MPO case
        {
            //printf(">>>333  BWTABLE_720p_1080i_mode\n");
            u8TabIdx = BWTABLE_mpo_mode;
        }
        else if (u16Input_HSize>=1440 && u16Input_VSize >= 900 && !bIsInterlaced)
        {
            if(QM_IsSourceVGA(MDrv_PQ_GetInputSourceType(eWindow)) && (u16input_vtotal ==1120))
            {
                //printf("print u16input_vtotal = %d\n",u16input_vtotal);
                //patch for special vga 1080p timing with vtotal 1120, htt 2576
                u8TabIdx = BWTABLE_S1080p_mode;
            }
            else
            {
                u8TabIdx = BWTABLE_1080p_mode;
            }
        }
        else
        {
            if(QM_IsSourceDTV(MDrv_PQ_GetInputSourceType(eWindow)) && u16Input_VSize > 720)  //DTV 1080i case
            {
                //printf(">>>333  BWTABLE_720p_1080i_mode\n");
                u8TabIdx = BWTABLE_1080i_mode;
            }
            else
            {
               // printf(">>>444  BWTABLE_Normal_mode\n");
                u8TabIdx = BWTABLE_Normal_mode;
            }
        }

    #else

        if (u16Input_HSize>=1440 && u16Input_VSize >= 900)
        {
            u8TabIdx = BWTABLE_1080p_mode;
        }
        else
        {
            // printf(">>>444  BWTABLE_Normal_mode\n");
            u8TabIdx = BWTABLE_Normal_mode;
        }

    #endif

#endif


    _MDrv_BW_LoadTable(u8TabIdx);

}
#endif
