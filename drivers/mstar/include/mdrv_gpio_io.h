////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   mdrv_gpio_io.h
/// @brief  GPIO Driver Interface
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <asm/types.h>
#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
struct GPIO_Reg
{
    U32 u32Reg;
    U8 u8Enable;
    U8 u8BitMsk;
} __attribute__ ((packed));

typedef struct GPIO_Reg GPIO_Reg_t;

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define IOCTL_GPIO_INIT_NR                              (0)
#define IOCTL_GPIO_SET_NR                               (1)
#define IOCTL_GPIO_OEN_NR                               (2)
#define IOCTL_GPIO_ODN_NR                               (3)
#define IOCTL_GPIO_READ_NR                              (4)
#define IOCTL_GPIO_PULL_HIGH_NR                        (5)
#define IOCTL_GPIO_PULL_LOW_NR                         (6)
#define IOCTL_GPIO_INOUT_NR                            (7)
#define IOCTL_GPIO_WREGB_NR                            (8)

#define GPIO_IOC_MAGIC               'g'
#define IOCTL_GPIO_INIT               _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_INIT_NR)
#define IOCTL_GPIO_SET                _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_SET_NR)
#define IOCTL_GPIO_OEN                _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_OEN_NR)
#define IOCTL_GPIO_ODN                _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_ODN_NR)
#define IOCTL_GPIO_READ               _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_READ_NR)
#define IOCTL_GPIO_PULL_HIGH          _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_PULL_HIGH_NR)
#define IOCTL_GPIO_PULL_LOW           _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_PULL_LOW_NR)
#define IOCTL_GPIO_INOUT              _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_INOUT_NR)
#define IOCTL_GPIO_WREGB              _IO(GPIO_IOC_MAGIC, IOCTL_GPIO_WREGB_NR)

#define GPIO_IOC_MAXNR               9

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

void __mod_gpio_init(void);

