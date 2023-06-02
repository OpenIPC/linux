/**
    SIE module fast boot driver

    @file       sie_init.c
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

// include files for FW
#if defined(__KERNEL__)
#include <linux/module.h>
#include "sie_init_int.h"

EXPORT_SYMBOL(sie_builtin_init);
EXPORT_SYMBOL(sie_fb_reg_buf_out_cb);
EXPORT_SYMBOL(sie_fb_reg_isr_Cb);
EXPORT_SYMBOL(sie_fb_get_rdy_addr);
EXPORT_SYMBOL(sie_fb_buf_release);
EXPORT_SYMBOL(sie_fb_ring_buf_release);
EXPORT_SYMBOL(sie_init_plat_read_dtsi_array);
EXPORT_SYMBOL(sie_fb_upd_timestp);
EXPORT_SYMBOL(kdrv_sie_builtin_plat_malloc);
EXPORT_SYMBOL(kdrv_sie_builtin_plat_free);
EXPORT_SYMBOL(sie_set_dbg_func);
#endif

//@}
