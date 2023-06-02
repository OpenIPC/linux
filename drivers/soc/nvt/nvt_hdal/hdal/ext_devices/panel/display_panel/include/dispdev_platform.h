#ifndef __IDE_PLATFORM_H_
#define __IDE_PLATFORM_H__

//#include "dispdev_panlcomn.h"
#if (defined __UITRON || defined __ECOS)
#include "dispdev_ioctrl.h"
#include "kernel.h"
#include "ide.h"
#include "dsi.h"
#include "top.h"
#include "pll.h"
#include "display.h"
#include "sif.h"
#include "gpio.h"
#include "Utility.h"


#elif defined __FREERTOS
#include <kwrap/error_no.h>
#include <libfdt.h>
#include <compiler.h>
#include <rtosfdt.h>
#include "top.h"
//#include "rcw_macro.h"
#include "display.h"
#include "dispdev_ioctrl.h"
#include "ide.h"
#include "dsi.h"
#include "gpio.h"
#include "pll.h"
#include "comm/sif_api.h"
#include "comm/timer.h"
#include "dispdev_dbg.h"

#define _FPGA_EMULATION_ 0
#define _EMULATION_ 0
#else
#include <plat-na51089/top.h>
#include <mach/rcw_macro.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include "display.h"
#include "dispdev_ioctrl.h"
#include "ide.h"
#include "dsi.h"
#include "comm/sif_api.h"
#include "dispdev_dbg.h"

#define _FPGA_EMULATION_ 0
#define _EMULATION_ 0
#endif


extern PINMUX_LCDINIT dispdev_platform_get_disp_mode(UINT32 pin_func_id);
extern void dispdev_platform_set_pinmux(UINT32 pin_func_id, UINT32 pinmux);
extern void dispdev_platform_delay_ms(UINT32 ms);
extern void dispdev_platform_delay_us(UINT32 us);
extern UINT32 dispdev_platform_request_gpio(UINT32 id, CHAR *str);
extern void dispdev_platform_set_gpio_ouput(UINT32 id, BOOL high);
extern void dispdev_platform_set_gpio_input(UINT32 id);
extern void dispdev_platform_free_gpio(UINT32 id);

#endif
