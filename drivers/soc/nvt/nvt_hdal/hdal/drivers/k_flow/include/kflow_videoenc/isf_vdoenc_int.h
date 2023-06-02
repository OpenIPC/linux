/*
    Copyright   Novatek Microelectronics Corp. 2005~2014.  All rights reserved.

    @file       isf_vdoenc_int.h
    @ingroup    mVdoIn

    @note       Nothing.

    @date       2018/10/15
*/

#ifndef __ISF_VDOENC_INT_H__
#define __ISF_VDOENC_INT_H__
#include "kflow_common/isp_if.h"


typedef enum {
	KFLOW_VIDEOENC_WAIT_FLG_NONE = 0x00000000
} KFLOW_VIDEOENC_WAIT_FLG;

typedef enum {
	KFLOW_VIDEOENC_ISP_ITEM_TNR   = 0,      // support get/set, using KDRV_VDOENC_3DNR struct
	KFLOW_VIDEOENC_ISP_ITEM_RATIO = 1,      // support get    , using KDRV_VDOENC_ISP_RATIO struct
	KFLOW_VIDEOENC_ISP_ITEM_SPN   = 2,      // support get/set, using KDRV_H26XENC_SPN struct
	ENUM_DUMMY4WORD(KFLOW_VIDEOENC_ISP_ITEM)
} KFLOW_VIDEOENC_ISP_ITEM;

extern ER kflow_videoenc_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, KFLOW_VIDEOENC_WAIT_FLG wait_flg);
extern ER kflow_videoenc_evt_fp_unreg(CHAR *name);
extern ER kflow_videoenc_set(ISP_ID id, KFLOW_VIDEOENC_ISP_ITEM item, void *data);
extern ER kflow_videoenc_get(ISP_ID id, KFLOW_VIDEOENC_ISP_ITEM item, void *data);

extern void			VdoCodec_H26x_Lock(void);
extern void			VdoCodec_H26x_UnLock(void);
extern void			VdoCodec_JPEG_Lock(void);
extern void			VdoCodec_JPEG_UnLock(void);
#endif //__ISF_VDOENC_INT_H__


