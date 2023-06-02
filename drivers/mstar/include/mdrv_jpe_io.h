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

#ifndef _MDRV_JPE_IO_H_
#define _MDRV_JPE_IO_H_

#define IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define IOC_JPE_VOID    0x20000000      /* no parameters */
#define IOC_JPE_OUT     0x40000000      /* copy out parameters */
#define IOC_JPE_IN      0x80000000      /* copy in parameters */
#define IOC_JPE_INOUT   (IOC_JPE_IN|IOC_JPE_OUT)

#define _IO_JPE(x,y)    (IOC_JPE_VOID|((x)<<8)|(y))
#define _IOR_JPE(x,y,t) (IOC_JPE_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define _IOW_JPE(x,y,t) (IOC_JPE_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

//JPE IOC COMMANDS
#define JPE_IOC_MAGIC 'J'
// Initialize *pJpeInfo
#define JPE_IOC_INIT                _IO_JPE(JPE_IOC_MAGIC, 0)
// Set up JPE RIU and fire JPE
#define JPE_IOC_ENCODE_FRAME        _IO_JPE(JPE_IOC_MAGIC, 1)
// Get output buffer status
#define JPE_IOC_GETBITS             _IOR_JPE(JPE_IOC_MAGIC, 2, __u32)
//
#define JPE_IOC_GET_CAPS            _IOR_JPE(JPE_IOC_MAGIC, 3, __u32)
//
#define JPE_IOC_SET_OUTBUF          _IOR_JPE(JPE_IOC_MAGIC, 4, __u32)

#endif // _MDRV_JPE_IO_H_