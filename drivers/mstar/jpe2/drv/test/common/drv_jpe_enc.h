////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
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

#ifndef __DRV_JPE_ENC_H__
#define __DRV_JPE_ENC_H__

#define JPE_DRIVER_VER 1
//#define JPE_DRIVER_VER 2

#if JPE_DRIVER_VER == 1
#include "mdrv_jpe_io.h"
#include "mdrv_jpe_io_st.h"
#else
#include "drv_jpe_io.h"
#include "drv_jpe_io_st.h"
#endif

typedef struct
{
    JpeCfg_t            tJpeCfg;
    int                 nEncodeSize;
    unsigned short      nClkSelect;
} JpeParam_t;

#if defined(__I_SW__)
void JpeEncode(JpeParam_t* pParam, JpeDev_t* pDev);
#else
void JpeEncode(JpeParam_t* pParam, int nDev);
#endif
#endif // __DRV_JPE_ENC_H__