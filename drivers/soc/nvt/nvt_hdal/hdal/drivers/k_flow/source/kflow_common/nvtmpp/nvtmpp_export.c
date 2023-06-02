/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nvtmpp_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     Lincy Lin
    @date       2020/04/28
*/

#include <linux/module.h>
#include "nvtmpp_int.h"

EXPORT_SYMBOL(nvtmpp_sys_init);
EXPORT_SYMBOL(nvtmpp_sys_create_fixed_pool);
EXPORT_SYMBOL(nvtmpp_vb_set_conf);
EXPORT_SYMBOL(nvtmpp_vb_get_conf);
EXPORT_SYMBOL(nvtmpp_vb_init);
EXPORT_SYMBOL(nvtmpp_vb_exit);
EXPORT_SYMBOL(nvtmpp_vb_relayout);
EXPORT_SYMBOL(nvtmpp_vb_create_pool);
EXPORT_SYMBOL(nvtmpp_vb_destroy_pool);
EXPORT_SYMBOL(nvtmpp_vb_get_block);
EXPORT_SYMBOL(nvtmpp_vb_lock_block);
EXPORT_SYMBOL(nvtmpp_vb_unlock_block);
EXPORT_SYMBOL(nvtmpp_vb_get_max_free_size);
EXPORT_SYMBOL(nvtmpp_vb_add_module);
EXPORT_SYMBOL(nvtmpp_vb_check_mem_corrupt);
EXPORT_SYMBOL(nvtmpp_dump_mem_range);
EXPORT_SYMBOL(nvtmpp_dump_status);
EXPORT_SYMBOL(nvtmpp_vb_blk2pa);
EXPORT_SYMBOL(nvtmpp_vb_blk2va);
EXPORT_SYMBOL(nvtmpp_vb_va2blk);



