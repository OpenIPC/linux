/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       kdrv_ipp_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     liwei cheng
    @date       2020/05/08
*/

#include <linux/module.h>
#include "kdrv_videoprocess/kdrv_ipp.h"

EXPORT_SYMBOL(kdrv_ipp_buf_query);
EXPORT_SYMBOL(kdrv_ipp_init);
EXPORT_SYMBOL(kdrv_ipp_uninit);
EXPORT_SYMBOL(kdrv_ipp_open);
EXPORT_SYMBOL(kdrv_ipp_close);
EXPORT_SYMBOL(kdrv_ipp_set);
EXPORT_SYMBOL(kdrv_ipp_get);
EXPORT_SYMBOL(kdrv_ipp_trigger);
EXPORT_SYMBOL(kdrv_ipp_direct_trigger);
EXPORT_SYMBOL(kdrv_ipp_direct_drop_old_job);

