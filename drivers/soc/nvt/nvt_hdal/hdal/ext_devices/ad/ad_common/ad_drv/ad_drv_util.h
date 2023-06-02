#ifndef _AD_DRV_UTIL_H_
#define _AD_DRV_UTIL_H_

#define _AD_DRV_TOKEN_BIND(a, b)  a ## _ ## b
#define AD_DRV_TOKEN_BIND(a, b)  _AD_DRV_TOKEN_BIND(a, b)

#define _AD_DRV_TOKEN_STR(a)  #a
#define AD_DRV_TOKEN_STR(a)  _AD_DRV_TOKEN_STR(a)

#define AD_DRV_MODEL_BIND(name) AD_DRV_TOKEN_BIND(name, _MODEL_NAME)
#define AD_DRV_MODEL_BIND_STR(name) AD_DRV_TOKEN_STR(AD_DRV_MODEL_BIND(name))

#define __MODULE__ AD_DRV_MODEL_BIND(AD_DRV)
#define __DBGLVL__ 8        // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"      // *=All

#define AD_DRV_MODULE_VERSION_VAR AD_DRV_MODEL_BIND(ad_drv_version)

#if defined(__LINUX)
#include <linux/delay.h>
#include <linux/string.h>

#define ad_drv_debug_level AD_DRV_TOKEN_BIND(__MODULE__, debug_level)
#define ad_drv_debug_flag AD_DRV_TOKEN_BIND(__MODULE__, debug_flag)
#define AD_DRV_DEBUG_LEVEL_TYPE unsigned int
#define AD_DRV_DEBUG_FLAG_TYPE char
#include "kwrap/debug.h"

	#if defined (_BSP_NA51000_)
	#include "kwrap/stdio.h"

	#define AD_DUMP DBG_DUMP
	#define AD_WRN DBG_WRN
	#define AD_ERR DBG_ERR
	#define AD_IND DBG_IND
	#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
	#define vos_util_delay_ms msleep
	#define vos_util_delay_us udelay

	#define AD_DRV_MODULE_VERSION(_major, _minor, _bugfix, _ext) 	\
	char AD_DRV_MODULE_VERSION_VAR[] = AD_DRV_TOKEN_STR(_major) "." AD_DRV_TOKEN_STR(_minor) "." AD_DRV_TOKEN_STR(_bugfix) "." AD_DRV_TOKEN_STR(_ext);

	#elif defined (_BSP_NA51055_) || defined (_BSP_NA51089_)
	#include <linux/gpio.h>
	#include <plat/nvt-gpio.h>
	#include "kwrap/util.h"
	#include "kwrap/verinfo.h"

	#define AD_DUMP(fmt, args...) printk(KERN_CONT fmt, ##args)
	#define AD_WRN DBG_WRN
	#define AD_ERR DBG_ERR
	#define AD_IND DBG_IND

	#define AD_DRV_MODULE_VERSION(_major, _minor, _bugfix, _ext) 	\
	VOS_MODULE_VERSION(_MODEL_NAME, _major, _minor, _bugfix, _ext); \
	char AD_DRV_MODULE_VERSION_VAR[] = AD_DRV_TOKEN_STR(_major) "." AD_DRV_TOKEN_STR(_minor) "." AD_DRV_TOKEN_STR(_bugfix) "." AD_DRV_TOKEN_STR(_ext);
	#endif

#elif defined(__FREERTOS)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kwrap/util.h"
#include "kwrap/verinfo.h"
#include "plat/gpio.h"
#include "kflow_videocapture/ctl_sen.h"

#define ad_drv_debug_level AD_DRV_TOKEN_BIND(__MODULE__, debug_level)
#define ad_drv_debug_flag AD_DRV_TOKEN_BIND(__MODULE__, debug_flag)
#define AD_DRV_DEBUG_LEVEL_TYPE unsigned int
#define AD_DRV_DEBUG_FLAG_TYPE char
#include "kwrap/debug.h"

#define AD_DUMP DBG_DUMP
#define AD_WRN DBG_WRN
#define AD_ERR DBG_ERR
#define AD_IND DBG_IND
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define printk AD_DUMP
#define msleep vos_util_delay_ms
#define udelay vos_util_delay_us
#define module_param(name, type, perm)
#define module_param_array(name, type, nump, perm)
#define MODULE_DESCRIPTION(desc)
#define MODULE_LICENSE(license)
#define bool BOOL
#define module_init(x)
#define module_exit(x)
#define DEFINE_SPINLOCK(x) struct {} x

#define AD_DRV_MODULE_VERSION(_major, _minor, _bugfix, _ext) 	\
VOS_MODULE_VERSION(_MODEL_NAME, _major, _minor, _bugfix, _ext); \
char AD_DRV_MODULE_VERSION_VAR[] = AD_DRV_TOKEN_STR(_major) "." AD_DRV_TOKEN_STR(_minor) "." AD_DRV_TOKEN_STR(_bugfix) "." AD_DRV_TOKEN_STR(_ext);

#elif defined(__UITRON) || defined(__ECOS)
#include <stdio.h>
#include "Delay.h"

#define _AD_DRV_TOKEN_BIND_UITRON(a, b)  a ## b
#define AD_DRV_TOKEN_BIND_UITRON(a, b)  _AD_DRV_TOKEN_BIND_UITRON(a, b)
#include "DebugModule.h"
#define ad_drv_debug_level _DBGLVL_
#define ad_drv_debug_flag _DBGFLT_
#define AD_DRV_DEBUG_LEVEL_TYPE UINT32
#define AD_DRV_DEBUG_FLAG_TYPE char

#define AD_DUMP DBG_DUMP
#define AD_WRN DBG_WRN
#define AD_ERR DBG_ERR
#define AD_IND DBG_IND
#define printk(fmt, args...) AD_DUMP(fmt "\r", ##args)
#define msleep Delay_DelayMs
#define udelay Delay_DelayUs
#define vos_util_delay_ms Delay_DelayMs
#define vos_util_delay_us Delay_DelayUs
#define module_param(name, type, perm)
#define module_param_array(name, type, nump, perm)
#define MODULE_DESCRIPTION(desc)
#define MODULE_LICENSE(license)
#define bool BOOL
#define module_init(x)
#define module_exit(x)
#define DEFINE_SPINLOCK(x) struct {} x
#define unlikely

#define AD_DRV_MODULE_VERSION(_major, _minor, _bugfix, _ext) 	\
char AD_DRV_MODULE_VERSION_VAR[] = AD_DRV_TOKEN_STR(_major) "." AD_DRV_TOKEN_STR(_minor) "." AD_DRV_TOKEN_STR(_bugfix) "." AD_DRV_TOKEN_STR(_ext);

#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) > (b) ? (b) : (a))
#endif

#endif //_AD_DRV_UTIL_H_

