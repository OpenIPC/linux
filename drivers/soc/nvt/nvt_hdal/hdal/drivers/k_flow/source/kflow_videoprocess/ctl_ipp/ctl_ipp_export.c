/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       ctl_ipp_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     XL Wei
    @date       2020/05/08
*/

#include <linux/module.h>
#include "kflow_videoprocess/ctl_ipp.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"

EXPORT_SYMBOL(ctl_ipp_open_tsk);
EXPORT_SYMBOL(ctl_ipp_close_tsk);
EXPORT_SYMBOL(ctl_ipp_set_resume);
EXPORT_SYMBOL(ctl_ipp_set_pause);
EXPORT_SYMBOL(ctl_ipp_wait_pause_end);
EXPORT_SYMBOL(ctl_ipp_open);
EXPORT_SYMBOL(ctl_ipp_close);
EXPORT_SYMBOL(ctl_ipp_ioctl);
EXPORT_SYMBOL(ctl_ipp_set);
EXPORT_SYMBOL(ctl_ipp_get);
EXPORT_SYMBOL(ctl_ipp_query);
EXPORT_SYMBOL(ctl_ipp_init);
EXPORT_SYMBOL(ctl_ipp_uninit);
EXPORT_SYMBOL(ctl_ipp_direct_flow_cb);
EXPORT_SYMBOL(ctl_ipp_get_dir_fp);

EXPORT_SYMBOL(ctl_ipp_isp_evt_fp_reg);
EXPORT_SYMBOL(ctl_ipp_isp_evt_fp_unreg);
EXPORT_SYMBOL(ctl_ipp_isp_set);
EXPORT_SYMBOL(ctl_ipp_isp_get);

