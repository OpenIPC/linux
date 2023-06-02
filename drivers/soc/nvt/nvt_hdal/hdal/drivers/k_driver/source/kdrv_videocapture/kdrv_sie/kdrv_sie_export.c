/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       kdrv_sie_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2020/04/28
*/

#include <linux/module.h>
#include "kdrv_sie_int.h"
#include "kdrv_sie_debug_int.h"

EXPORT_SYMBOL(kdrv_sie_open);
EXPORT_SYMBOL(kdrv_sie_close);
EXPORT_SYMBOL(kdrv_sie_resume);
EXPORT_SYMBOL(kdrv_sie_suspend);
EXPORT_SYMBOL(kdrv_sie_set);
EXPORT_SYMBOL(kdrv_sie_get);
EXPORT_SYMBOL(kdrv_sie_trigger);
EXPORT_SYMBOL(kdrv_sie_buf_query);
EXPORT_SYMBOL(kdrv_sie_buf_init);
EXPORT_SYMBOL(kdrv_sie_buf_uninit);
EXPORT_SYMBOL(kdrv_sie_init);
EXPORT_SYMBOL(kdrv_sie_uninit);
EXPORT_SYMBOL(kdrv_sie_get_sie_limit);
EXPORT_SYMBOL(kdrv_sie_dump_fb_info);
//dbg
EXPORT_SYMBOL(kdrv_sie_dump_info);
EXPORT_SYMBOL(kdrv_sie_dbg_set_dbg_level);
EXPORT_SYMBOL(kdrv_sie_set_checksum_en);
