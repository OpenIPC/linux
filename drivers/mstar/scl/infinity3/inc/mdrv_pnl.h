////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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

#ifndef _MDRV_PNL_H
#define _MDRV_PNL_H


//-------------------------------------------------------------------------------------------------
//  Defines & Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    unsigned long u32RiuBase;
}ST_MDRV_PNL_INIT_CONFIG;



typedef struct
{
    unsigned short u16Vsync_St; ///< vsync start point
    unsigned short u16Vsync_End;///< vsync end point
    unsigned short u16Vde_St;   ///< Vdata enable start point
    unsigned short u16Vde_End;  ///< Vdata enable end point
    unsigned short u16Vfde_St;  ///< V framecolor data enable start point
    unsigned short u16Vfde_End; ///< V framecolor data enable end point
    unsigned short u16Vtt;      ///< V total
    unsigned short u16Hsync_St; ///< hsync start point
    unsigned short u16Hsync_End;///< hsync end point
    unsigned short u16Hde_St;   ///< Hdata enable start point
    unsigned short u16Hde_End;  ///< Hdata enable end point
    unsigned short u16Hfde_St;  ///< H framecolor data enable start point
    unsigned short u16Hfde_End; ///< H framecolor data enable end point
    unsigned short u16Htt;      ///< H total
    unsigned short u16VFreqx10; ///< FPS x10
}ST_MDRV_PNL_TIMING_CONFIG;


//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef _MDRV_PNL_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE void MDrv_PNL_Release(void);
INTERFACE unsigned char MDrv_PNL_Init(ST_MDRV_PNL_INIT_CONFIG *pCfg);
INTERFACE void MDrv_PNL_Exit(unsigned char bCloseISR);
INTERFACE unsigned char MDrv_PNL_Set_Timing_Config(ST_MDRV_PNL_TIMING_CONFIG *pCfg);
INTERFACE unsigned char MDrv_PNL_Set_LPLL_Config(ST_MDRV_PNL_TIMING_CONFIG *pCfg);
INTERFACE void MDrv_PNL_Resume(void);
#undef INTERFACE

#endif
