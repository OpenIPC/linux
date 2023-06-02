/**
    Header file of isf_vdoenc internal header

    @file       isf_vdoenc_int.h

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _ISF_VDOENC_INTERNAL_H
#define _ISF_VDOENC_INTERNAL_H

#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/flag.h"
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"

#ifndef __KERNEL__
#define __RTOS__TODO__  0
#else
#define __RTOS__TODO__  1
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <malloc.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)

#define msecs_to_jiffies(x)	0
#define debug_msg 			vk_printk

#include <string.h>
#define DELAY_M_SEC(x)              vos_task_delay_ms(x)
#define DELAY_U_SEC(x)              vos_task_delay_us(x)

#else
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "comm/nvtmem.h"
#else
#include "frammap/frammap_if.h"
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/printk.h>
//#define debug_msg 			vk_printk

#include <linux/delay.h>
#define DELAY_M_SEC(x)              msleep(x) //require delay.h
#define DELAY_U_SEC(x)              udelay(x) //require delay.h
#endif

extern ISF_UNIT isf_vdodec;

extern UINT32 g_vdodec_path_max_count;    // dynamic setting for actual used path count, using PATH_MAX_COUNT to access this variable
#define PATH_MAX_COUNT g_vdodec_path_max_count

#endif//_ISF_VDOENC_INTERNAL_H

//@}
