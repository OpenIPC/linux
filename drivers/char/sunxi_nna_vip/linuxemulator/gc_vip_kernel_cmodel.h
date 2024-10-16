/******************************************************************************\
|* Copyright (c) 2017-2022 by Vivante Corporation.  All Rights Reserved.      *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
\******************************************************************************/


#ifndef _GC_VIP_KERNEL_CMODEL_H
#define _GC_VIP_KERNEL_CMODEL_H

#include <stdio.h>
#ifdef __cplusplus
#define externC extern "C"
#else
#define externC
#endif

typedef char            cmBool;
typedef unsigned char   cmUInt8;
typedef unsigned int    cmUInt32;
typedef size_t          cmSize;

typedef struct cmMemoryInfo
{
    cmUInt8 *   logical;
    cmUInt32    physical;
    size_t      bytes;
}
cmMemoryInfo;

typedef void (*IRQ_HANDLER)(void * context);

externC int CModelConstructor(cmSize Memory);
externC void CModelDestructor(void);
externC void CModelWriteRegister(cmUInt32 Address, cmUInt32 Data);
externC cmUInt32 CModelReadRegister(cmUInt32 Address);
externC cmMemoryInfo * CModelAllocate(cmSize Bytes, cmBool Contiguous);
externC void CModelFree(cmMemoryInfo * Memory);
externC cmUInt32 CModelPhysical(void * Logical);
externC void * CModelLogical(cmUInt32 Physical);
externC cmBool CModelWaitInterrupt(cmUInt32 MillseSeconds);
externC cmBool CModelRegisterIrq(IRQ_HANDLER handler, void * private_data);

#endif
