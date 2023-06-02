/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ctl_ise_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     XL Wei
    @date       2020/05/12
*/
#include <linux/module.h>
#include "kflow_videoprocess/ctl_ise.h"
#include "ctl_ise_int.h"

#if CTL_ISE_MODULE_ENABLE
EXPORT_SYMBOL(ctl_ise_open);
EXPORT_SYMBOL(ctl_ise_close);
EXPORT_SYMBOL(ctl_ise_ioctl);
EXPORT_SYMBOL(ctl_ise_set);
EXPORT_SYMBOL(ctl_ise_get);
EXPORT_SYMBOL(ctl_ise_query);
EXPORT_SYMBOL(ctl_ise_init);
EXPORT_SYMBOL(ctl_ise_uninit);
EXPORT_SYMBOL(ctl_ise_dump_all);
#endif
