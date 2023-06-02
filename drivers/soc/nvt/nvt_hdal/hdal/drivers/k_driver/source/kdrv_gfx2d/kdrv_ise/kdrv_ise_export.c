/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       kdrv_ise_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     XL Wei
    @date       2020/05/14
*/

#include <linux/module.h>
#include "kdrv_gfx2d/kdrv_ise.h"

EXPORT_SYMBOL(kdrv_ise_init);
EXPORT_SYMBOL(kdrv_ise_uninit);
EXPORT_SYMBOL(kdrv_ise_open);
EXPORT_SYMBOL(kdrv_ise_close);
EXPORT_SYMBOL(kdrv_ise_set);
EXPORT_SYMBOL(kdrv_ise_get);
EXPORT_SYMBOL(kdrv_ise_trigger);
EXPORT_SYMBOL(kdrv_ise_dma_abort);

EXPORT_SYMBOL(kdrv_ise_linked_list_trigger);
EXPORT_SYMBOL(kdrv_ise_get_linked_list_buf_size);

