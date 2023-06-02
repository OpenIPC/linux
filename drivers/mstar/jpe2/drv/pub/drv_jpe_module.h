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

#ifndef __DRV_JPE_MODULE_H__
#define __DRV_JPE_MODULE_H__

#include "drv_jpe_io.h"
#include "drv_jpe_io_st_kernel.h"

#define JPE_OFFSET                 0x00264000

#if defined(__I_SW__) // #if defined(__I_SW__)
JpeDev_t*  JpeProbe(JpeDev_t* pDev);
JpeCtx_t*  JpeOpen(JpeDev_t* pDev, JpeCtx_t* pCtx);
JPE_IOC_RET_STATUS_e  JpeRelease(JpeDev_t* pDev, JpeCtx_t* pCtx);
JPE_IOC_RET_STATUS_e  JpeRemove(JpeDev_t* pDev);
JPE_IOC_RET_STATUS_e JpeCtxActions(JpeCtx_t*, unsigned int, void*);

#elif defined(__KERNEL__)
JPE_IOC_RET_STATUS_e JpeCtxActions(JpeCtx_t*, unsigned int, void*);

#endif // #if defined(__I_SW__)

#endif // __DRV_JPE_MODULE_H__