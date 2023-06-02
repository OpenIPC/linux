/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_net_platform.h

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/platform.h"

#if defined(__FREERTOS)
#define EXPORT_SYMBOL(a)
#define MODULE_AUTHOR(a)
#define MODULE_LICENSE(a)
#define MODULE_VERSION(a)
#include <string.h>         // for memset, strncmp
#include <stdio.h>          // sscanf
#else
//#include <linux/delay.h>
#include <linux/module.h>
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include <malloc.h>
#define module_param_named(a, b, c, d)
#define MODULE_PARM_DESC(a, b)

#define debug_msg 			vk_printk

#include <string.h>
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#define DELAY_M_SEC(x)		vos_util_delay_ms(x)
#define DELAY_U_SEC(x)		vos_util_delay_us(x)
#if defined(_BSP_NA51055_)
#include "kflow_common/nvtmpp.h"
#include "rtos_na51055/top.h"     // TODO: include RTOS .h  first
#elif defined(_BSP_NA51089_)
#include "kflow_common/nvtmpp.h"
#include "rtos_na51089/top.h"     // TODO: include RTOS .h  first
#endif

#else
#if defined(_BSP_NA51055_)
#include "kflow_common/nvtmpp.h"
//#include <plat-na51055/top.h>   // TODO: linux should include this, but there's no CHIP_NA51084 yet...
#include "rtos_na51055/top.h"     // TODO: include RTOS .h  first
#elif defined(_BSP_NA51089_)
#include "kflow_common/nvtmpp.h"
#include "rtos_na51089/top.h"     // TODO: include RTOS .h  first
#else
//#include "frammap/frammap_if.h"
#include "mach/fmem.h"
#include <asm/io.h>  /* for ioremap and iounmap */
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/printk.h>
#define debug_msg 			vk_printk

#include <linux/delay.h>
#define DELAY_M_SEC(x)              msleep(x) //require delay.h
#define DELAY_U_SEC(x)              udelay(x) //require delay.h
#endif



#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)

#include "kflow_common/nvtmpp.h"
#include "comm/hwclock.h" //for hwclock_get_longcounter()
#define _nvt_ai_get_counter() hwclock_get_longcounter()

#else //_BSP_NA51068_

#include <plat/nvt_jiffies.h> //for get_nvt_jiffies()
#include <linux/vmalloc.h>
#include <linux/slab.h>
#define _nvt_ai_get_counter() get_nvt_jiffies_us()

#endif


extern UINT32 nvt_ai_va2pa(UINT32 addr);

extern UINT32 nvt_ai_pa2va_remap(UINT32 pa, UINT32 sz);

extern UINT32 nvt_ai_pa2va_remap_wo_sync(UINT32 pa, UINT32 sz);

extern VOID nvt_ai_pa2va_unmap(UINT32 va, UINT32 pa);


