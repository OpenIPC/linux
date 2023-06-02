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

#ifndef __HAL_JPE_DEF_H__
#define __HAL_JPE_DEF_H__


#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL 0
#endif

#define JPE_MSG_ERR     3
#define JPE_MSG_WARNING 4
#define JPE_MSG_DEBUG   5
#define JPE_MSG_LEVEL    JPE_MSG_ERR //JPE_MSG_WARNING

#define JPE_MSG_ENABLE

#ifdef  JPE_MSG_ENABLE
#define JPE_MSG_FUNC_ENABLE
#define JPE_STRINGIFY(x) #x
#define JPE_TOSTRING(x) JPE_STRINGIFY(x)
#ifdef JPE_MSG_FUNC_ENABLE
#define JPE_MSG_TITLE   "[JPE, %s] "
#define JPE_MSG_FUNC    __func__
#else
#define JPE_MSG_TITLE   "[JPE] %s"
#define JPE_MSG_FUNC    ""
#endif

////////////////////////////////////// Define for Linux pletform ///////////////////////////////
#if defined(__linux__)  //#if defined(__linux__)
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/types.h>
#include "ms_platform.h"
#include "cam_os_wrapper.h"
//#include "mdrv_jpe.h"
#define ms_dprintf JPE_MSG

//#define JPE_REGS_TRACE
#define _REGW(base,idx)          (*(((volatile u16*)(base))+2*(idx)))
#if defined(JPE_REGS_TRACE)
#define PRINT(s,idx,v,cmt)          printk(#s":bank%d[%02x]=%04x %s\n",(idx&0x80)?1:0,idx&0x7F,v, cmt)
#define REGW(base,idx,v,cmt)   do{_REGW(base,idx)=(v);PRINT(w,idx,v,cmt);}while(0)
#define REGR(base,idx,cmt)     ({u16 v=_REGW(base,idx);PRINT(r,idx,v,cmt); v;})
#else
#define REGW(base,idx,val,cmt) _REGW(base,idx)=(val)
#define REGR(base,idx,cmt)     _REGW(base,idx)
#endif


#define JPE_MSG(dbglv, _fmt, _args...)                          \
    do if(dbglv <= JPE_MSG_LEVEL) {                              \
        printk(KERN_SOH JPE_TOSTRING(dbglv) JPE_MSG_TITLE  _fmt, JPE_MSG_FUNC, ## _args);   \
    } while(0)

////////////////////////////////////// Define for RTK platform ///////////////////////////////
#elif defined(__I_SW__)
#include <stdio.h>
#include <stdarg.h>
#include <string.h> // for memset
#include "kernel.h"
#include "vm_types.ht"
#include "cam_os_wrapper.h"
#include "sys_sys_isw_uart.h"
#include "hal_drv_util.h"

#define _REGW(base,idx)          (*(((volatile u16*)(base))+2*(idx)))
#if defined(JPE_REGS_TRACE)
#define PRINT(s,idx,v)          UartSendTrace(#s":bank%d[%02x]=%04x\n",(idx&0x80)?1:0,idx&0x7F,v)
#define REGW(base,idx,v,cmt)   do{_REGW(base,idx)=(v);PRINT(w,idx,v,cmt);}while(0)
#define REGR(base,idx,cmt)     ({u16 v=_REGW(base,idx);PRINT(r,idx,v,cmt); v;})
#else
#define REGW(base,idx,val,cmt) _REGW(base,idx)=(val)
#define REGR(base,idx,cmt)     _REGW(base,idx)
#endif

#define Chip_Phys_to_MIU(addr)    HalUtilPHY2MIUAddr((unsigned int)(addr))

#define JPE_MSG(dbglv, _fmt, _args...)                          \
    do if(dbglv <= JPE_MSG_LEVEL) {                              \
        UartSendTrace(JPE_TOSTRING(dbglv) JPE_MSG_TITLE  _fmt, JPE_MSG_FUNC, ## _args);   \
    } while(0)

#else

#define     JPE_MSG(dbglv, _fmt, _args...)

#endif
#endif  //#if defined(__linux__)


typedef enum
{
    JPE_DEV_INIT = 0,
    JPE_DEV_BUSY,
    JPE_DEV_ENC_DONE,
    JPE_DEV_OUTBUF_FULL,
    JPE_DEV_INBUF_FULL
} JpeDevStatus_e;

typedef enum
{
    JPE_IOC_RET_SUCCESS             = 0,
    JPE_IOC_RET_BAD_QTABLE          = 1,
    JPE_IOC_RET_BAD_QP              = 2,
    JPE_IOC_RET_BAD_BITSOFFSET      = 3,
    JPE_IOC_RET_BAD_BANKNUM         = 4,
    JPE_IOC_RET_BAD_INBUF           = 5,
    JPE_IOC_RET_BAD_OUTBUF          = 6,
    JPE_IOC_RET_BAD_NULLPTR         = 7,
    JPE_IOC_RET_BAD_BANKCNT         = 8,
    JPE_IOC_RET_BAD_LASTZZ          = 9,
    JPE_IOC_RET_UNKOWN_COMMAND      = 10,
    JPE_IOC_RET_BAD_VIRTUAL_MEM     = 11,
    JPE_IOC_RET_NEED_DRIVER_INIT    = 12,
    JPE_IOC_RET_FMT_NOT_SUPPORT     = 13,
    JPE_IOC_RET_HW_IS_RUNNING       = 14,
    JPE_IOC_RET_FAIL                = 15
} JPE_IOC_RET_STATUS_e;


#endif // __HAL_JPE_DEF_H__