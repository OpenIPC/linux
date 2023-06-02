/**
    Image operation module.

    This module supports rendering osds and masks on video frame.

    @file       videosprite.h
    @ingroup    mIAppVideosprite

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEO_SPRITE_VO_H_
#define _VIDEO_SPRITE_VO_H_

//#include "uitron_wrapper/platform.h"
#include <kwrap/type.h>
#include "kflow_common/type_vdo.h"
#include "videosprite_internal.h"

typedef struct {
	UINT32                    dirty;
	VDS_INTERNAL_EXT_STAMP    *stamp;
} VDS_TO_VO_STAMP;

typedef struct {
	UINT32                    dirty;
	VDS_INTERNAL_EXT_MASK     *mask;
} VDS_TO_VO_MASK;

extern int vds_render_vo(VDS_QUERY_STAGE stage, UINT32 y, UINT32 uv, void* p_data, UINT32 w, UINT32 h, UINT32 *palette, UINT32 *loff, VDO_PXLFMT fmt);

#endif //_VIDEO_SPRITE_VO_H_
