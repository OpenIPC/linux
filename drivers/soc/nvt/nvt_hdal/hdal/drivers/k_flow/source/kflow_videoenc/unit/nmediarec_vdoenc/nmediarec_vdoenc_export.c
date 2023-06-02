/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       nmediarec_vdoenc_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     YanYu Chen
    @date       2020/05/20
*/

#include <linux/module.h>
#include "kflow_videoenc/isf_vdoenc_int.h"
#include "nmediarec_vdoenc_api.h"

EXPORT_SYMBOL(kflow_videoenc_evt_fp_reg);
EXPORT_SYMBOL(kflow_videoenc_evt_fp_unreg);
EXPORT_SYMBOL(kflow_videoenc_set);
EXPORT_SYMBOL(kflow_videoenc_get);
EXPORT_SYMBOL(VdoCodec_H26x_Lock);
EXPORT_SYMBOL(VdoCodec_H26x_UnLock);
EXPORT_SYMBOL(VdoCodec_JPEG_Lock);
EXPORT_SYMBOL(VdoCodec_JPEG_UnLock);
EXPORT_SYMBOL(NMR_VdoEnc_GetBuiltinBsData);

