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

#ifndef ___DRV_JPE_DEV_H__
#define ___DRV_JPE_DEV_H__

#include "drv_jpe_io_st_kernel.h"

int JpeDevRegister(JpeDev_t*, JpeCtx_t*);
int JpeDevUnregister(JpeDev_t*, JpeCtx_t*);
int JpeDevPowerOn(JpeDev_t*, int);
int JpeDevPushJob(JpeDev_t*, JpeCtx_t*);
int JpeDevIsrFnx(JpeDev_t*);

#endif // ___DRV_JPE_DEV_H__