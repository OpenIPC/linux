///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
//
// @file   mdrv_aesdma_io.h
// @author MStar Semiconductor Inc.
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _MDRV_AESDMA_IO_H
#define _MDRV_AESDMA_IO_H

#define IOCTL_AESDMA_GSESSION     (102)
#define IOCTL_AESDMA_FSESSION     (103)
#define IOCTL_AESDMA_CRYPT        (104)

#define CIOCGSESSION    _IO('c', IOCTL_AESDMA_GSESSION)
#define CIOCFSESSION    _IO('c', IOCTL_AESDMA_FSESSION)
#define CIOCCRYPT       _IO('c', IOCTL_AESDMA_CRYPT)


#endif
