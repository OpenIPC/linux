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

#ifndef _MDRV_ISP_IO_H
#define _MDRV_ISP_IO_H

//=============================================================================
// Includs
//=============================================================================

//=============================================================================
// Defines
//=============================================================================

#define ISP_IOCTL_MAGIC 'I'
#define IOCTL_ISP_CLOCK_CTL			_IO(ISP_IOCTL_MAGIC, 0x1)
#define IOCTL_ISP_IQ_RGBCCM         _IO(ISP_IOCTL_MAGIC, 0x2)
#define IOCTL_ISP_IQ_YUVCCM         _IO(ISP_IOCTL_MAGIC, 0x3)
#define IOCTL_ISP_GET_ISP_FLAG      _IO(ISP_IOCTL_MAGIC, 0x4)
#define IOCTL_ISP_GET_INFO          _IO(ISP_IOCTL_MAGIC, 0x5)
#define IOCTL_ISP_GET_MEM_INFO      _IO(ISP_IOCTL_MAGIC, 0x6)
#define IOCTL_ISP_GET_AE_IMG_INFO   _IO(ISP_IOCTL_MAGIC, 0x7)
#define IOCTL_ISP_SET_AE_DGAIN      _IO(ISP_IOCTL_MAGIC, 0x8)
#define IOCTL_ISP_SET_FIFO_MASK     _IO(ISP_IOCTL_MAGIC, 0x9)
#define IOCTL_ISP_UPDATE_AE_IMG_INFO   _IO(ISP_IOCTL_MAGIC, 0xa)
#define IOCTL_ISP_TRIGGER_WDMA      _IO(ISP_IOCTL_MAGIC, 0xb)
#define IOCTL_ISP_SKIP_FRAME        _IO(ISP_IOCTL_MAGIC, 0xc)
#define IOCTL_ISP_INIT              _IO(ISP_IOCTL_MAGIC, 0xd)
#endif //_MDRV_GFLIP_IO_H
