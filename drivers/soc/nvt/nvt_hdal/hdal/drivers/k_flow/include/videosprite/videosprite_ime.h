/**
    Image operation module.

    This module supports rendering osds and masks on video frame.

    @file       videosprite.h
    @ingroup    mIAppVideosprite

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEO_SPRITE_IME_H_
#define _VIDEO_SPRITE_IME_H_

#include <kwrap/type.h>
#include "kflow_common/type_vdo.h"
#include "videosprite_internal.h"

typedef struct {
	UINT32                    en;
	VDS_INTERNAL_MASK_TYPE    type;
	UINT32                    is_mosaic;
	UPOINT                    pos[4];
	UINT32                    color[3];
	UINT8                     alpha;
	UINT32                    thickness;
	UINT32                    mosaic_blk_size;
} VDS_INTERNAL_IME_MASK;

typedef struct {
	UINT32                   dirty;
	VDS_INTERNAL_IME_MASK    *data;
} VDS_TO_IME_MASK;

typedef struct {
	UINT32        en;
	VDO_PXLFMT    fmt;
	UINT32        addr;
	UPOINT        pos;
	USIZE         size;
	UINT32        ckey_en;
	UINT32        ckey_val;
	UINT16        bweight0;
	UINT16        bweight1;
} VDS_INTERNAL_IME_STAMP;

typedef struct {
	UINT32                    dirty;
	VDS_INTERNAL_IME_STAMP    *data;
} VDS_TO_IME_STAMP;

typedef struct {
	VDS_INTERNAL_EXT_STAMP    *stamp;
} VDS_TO_IME_GRH_STAMP;

typedef struct {
	UINT32                    dirty;
	VDS_TO_IME_GRH_STAMP      data;
} VDS_TO_IME_EXT_STAMP;

typedef struct {
	VDS_INTERNAL_EXT_MASK     *mask;
} VDS_TO_IME_GRH_MASK;

typedef struct {
	UINT32                    dirty;
	VDS_TO_IME_GRH_MASK       data;
} VDS_TO_IME_EXT_MASK;

extern int vds_max_ime_stamp;
extern int vds_max_ime_mask;

extern int vds_render_ime_context(VDS_QUERY_STAGE stage, UINT32 y, UINT32 uv, void* p_data, UINT32 w, UINT32 h, UINT32 *palette);

#endif //_VIDEO_SPRITE_IME_H_