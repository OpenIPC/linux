/**
    OSD operation module.

    This module supports rendering images and masks on video frame.

    @file       videosprite.h
    @ingroup    mIAppVideosprite

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEOSPRITE_H_
#define _VIDEOSPRITE_H_

#ifdef __KERNEL__
#include <kwrap/type.h>
#include "kflow_common/type_vdo.h"
#else
#include <stdio.h>
#include <sys/types.h>
#endif

#define VDS_USER_VERSION 20180823
#define OSG_MASK_TYPE_INCONTINUOUS 0x0F

typedef enum {
	VDS_PHASE_NULL,
	VDS_PHASE_IME_STAMP,
	VDS_PHASE_IME_MASK,
	VDS_PHASE_IME_MOSAIC,
	VDS_PHASE_IME_EXT_STAMP,
	VDS_PHASE_IME_EXT_MASK,
	VDS_PHASE_COE_STAMP,
	VDS_PHASE_COE_EXT_STAMP,
	VDS_PHASE_COE_EXT_MASK,
	VDS_PHASE_VO_STAMP,
	VDS_PHASE_VO_MASK,
	VDS_PHASE_MAX,
	ENUM_DUMMY4WORD(VDS_PHASE)
} VDS_PHASE;

typedef enum {
	VDS_USER_CMD_NULL,
	VDS_USER_CMD_NULL1,
	VDS_USER_CMD_NULL2,
	VDS_USER_CMD_NULL3,
	VDS_USER_CMD_START,
	VDS_USER_CMD_STOP,
	VDS_USER_CMD_GET_STAMP_BUF,
	VDS_USER_CMD_SET_STAMP_BUF,
	VDS_USER_CMD_GET_STAMP_IMG,
	VDS_USER_CMD_SET_STAMP_IMG,
	VDS_USER_CMD_GET_STAMP_ATTR,
	VDS_USER_CMD_SET_STAMP_ATTR,
	VDS_USER_CMD_GET_MASK_ATTR,
	VDS_USER_CMD_SET_MASK_ATTR,
	VDS_USER_CMD_GET_MOSAIC_ATTR,
	VDS_USER_CMD_SET_MOSAIC_ATTR,
	VDS_USER_CMD_CLOSE,
	VDS_USER_CMD_RESET,
	VDS_USER_CMD_ALLOCATE,
	VDS_USER_CMD_SET_CFG_MAX,
	VDS_USER_CMD_SET_PALETTE,
	VDS_USER_CMD_SET_QP,
	VDS_USER_CMD_SET_COLOR_INVERT,
	VDS_USER_CMD_SET_ENCODER_BUILTIN_MASK,
	VDS_USER_CMD_GET_PALETTE,
	VDS_USER_CMD_OPEN,
	VDS_USER_CMD_MAX,
	ENUM_DUMMY4WORD(VDS_USER_CMD)
} VDS_USER_CMD;

typedef enum _VDS_BUF_TYPE {
	VDS_BUF_TYPE_NULL,
	VDS_BUF_TYPE_SINGE,
	VDS_BUF_TYPE_PING_PONG,
	VDS_BUF_TYPE_MAX,
	ENUM_DUMMY4WORD(VDS_BUF_TYPE)
} VDS_BUF_TYPE;

typedef enum {
	VDS_IMG_FMT_NULL,
	VDS_IMG_FMT_PICTURE_RGB565,
	VDS_IMG_FMT_PICTURE_ARGB1555,
	VDS_IMG_FMT_PICTURE_ARGB4444,
	VDS_IMG_FMT_PICTURE_ARGB8888,
	VDS_IMG_FMT_PICTURE_PALETTE1,
	VDS_IMG_FMT_PICTURE_PALETTE2,
	VDS_IMG_FMT_PICTURE_PALETTE4,
	VDS_IMG_FMT_MAX,
	ENUM_DUMMY4WORD(VDS_IMG_FMT)
} VDS_IMG_FMT;

typedef enum _VDS_COPY_HW {
	VDS_COPY_HW_NULL,
	VDS_COPY_HW_CPU,
	VDS_COPY_HW_DMA,
	VDS_COPY_HW_MAX,
	ENUM_DUMMY4WORD(VDS_COPY_HW)
} VDS_COPY_HW;

typedef enum _VDS_MASK_TYPE {
	VDS_MASK_TYPE_NULL,
	VDS_MASK_TYPE_SOLID,
	VDS_MASK_TYPE_HOLLOW,
	VDS_MASK_TYPE_INCONTINUOUS,
	VDS_MASK_TYPE_MAX,
	ENUM_DUMMY4WORD(VDS_MASK_TYPE)
} VDS_MASK_TYPE;

typedef struct _VDS_STAMP_BUF {
	VDS_BUF_TYPE       type;
	UINT32             size;
	UINT32             p_addr;
} VDS_STAMP_BUF;

typedef struct _VDS_STAMP_IMG {
	VDS_IMG_FMT         fmt;
	UINT32         	    w;
	UINT32         	    h;
	UINT32           	p_addr;
	VDS_COPY_HW         copy;
} VDS_STAMP_IMG;

typedef struct _VDS_STAMP_ATTR {
	INT32     x;
	INT32     y;
	UINT32    alpha;
	UINT32    colorkey_en;
	UINT32    colorkey_val;
	UINT32    qp_en;
	UINT32    qp_fix;
	UINT32    qp_val;
	UINT8     qp_lpm_mode;
	UINT8     qp_tnr_mode;
	UINT8     qp_fro_mode;
	UINT32    layer;
	UINT32    region;
	UINT8     gcac_enable;
	UINT8     gcac_blk_width;
	UINT8     gcac_blk_height;
	UINT8     gcac_blk_num;
	UINT8     gcac_org_color_level;
	UINT8     gcac_inv_color_level;
	UINT8     gcac_nor_diff_th;
	UINT8     gcac_inv_diff_th;
	UINT8     gcac_sta_only_mode;
	UINT8     gcac_full_eval_mode;
	UINT8     gcac_eval_lum_targ;
} VDS_STAMP_ATTR;

typedef struct _OSG_INCONTINUOUS_MASK {
	UINT32              type;
	UINT32              color;
	UINT32              x;
	UINT32              y;
	UINT32              h_line_len;
	UINT32              h_hole_len;
	UINT32              v_line_len;
	UINT32              v_hole_len;
	UINT32              h_thickness;
	UINT32              v_thickness;
} OSG_INCONTINUOUS_MASK;

typedef struct _VDS_MASK_ATTR {
	VDS_MASK_TYPE       type;
	UINT32           	color;
	union{
		struct{
			UINT32     alpha;
			UINT32     x[4];
			UINT32     y[4];
		}normal;
		struct{
			UINT32      x;
			UINT32      y;
			UINT32      h_line_len;
			UINT32      h_hole_len;
			UINT32      h_thickness;
			UINT32      v_line_len;
			UINT32      v_hole_len;
			UINT32      v_thickness;
		}incontinuous;
	}data;
	UINT32              thickness;	
} VDS_MASK_ATTR;

typedef struct _VDS_MOSAIC_ATTR {
	UINT32    		    x[4];
	UINT32    		    y[4];
	UINT32       		mosaic_blk_w;
	UINT32       		mosaic_blk_h;
} VDS_MOSAIC_ATTR;

typedef struct _VDS_CFG_MAX {
	unsigned int        max_prc_path;
	unsigned int        max_enc_path;
	unsigned int        max_out_path;
	unsigned int        max_stamp_img;
	unsigned int        max_prc_stamp[2];
	unsigned int        max_prc_mask[2];
	unsigned int        max_enc_stamp[2];
	unsigned int        max_enc_mask[2];
	unsigned int        max_out_stamp[2];
	unsigned int        max_out_mask[2];
} VDS_CFG_MAX;

typedef struct _VDS_QP {
	UINT8               lpm_mode;
	UINT8               tnr_mode;
	UINT8               fro_mode;
} VDS_QP;

typedef struct __VDS_QP {
	UINT32    magic1;
	UINT32    magic2;
	VDS_QP    data;
} _VDS_QP;

typedef struct _VDS_COLOR_INVERT {
	UINT8               blk_num;
	UINT8               org_color_level;
	UINT8               inv_color_level;
	UINT8               nor_diff_th;
	UINT8               inv_diff_th;
	UINT8               sta_only_mode;
	UINT8               full_eval_mode;
	UINT8               eval_lum_targ;
} VDS_COLOR_INVERT;

typedef struct __VDS_COLOR_INVERT {
	UINT32              magic1;
	UINT32              magic2;
	VDS_COLOR_INVERT    data;
} _VDS_COLOR_INVERT;

typedef struct _VDS_ENC_MASK {
	VDS_MASK_TYPE    type;
	UINT32           color;
	UINT32           alpha;
	UINT32           x;
	UINT32           y;
	UINT32           w;
	UINT32           h;
	UINT32           thickness;
	UINT32           layer;
	UINT32           region;
} VDS_ENC_MASK;

typedef struct __VDS_ENC_MASK {
	UINT32          magic1;
	UINT32          magic2;
	VDS_ENC_MASK    data;
} _VDS_ENC_MASK;

typedef struct _VDS_PALETTE {
	UINT32             size;
	UINT32             p_addr;
} VDS_PALETTE;

typedef struct _VDS_USER_DATA {
	UINT32          version;
	VDS_USER_CMD    cmd;
	VDS_PHASE       phase;
	UINT32          rgn;
	UINT32          vid;
	union {
		VDS_STAMP_BUF      buf;
		VDS_STAMP_IMG      img;
		VDS_STAMP_ATTR     stamp;
		VDS_MASK_ATTR      mask;
		VDS_MOSAIC_ATTR    mosaic;
		VDS_CFG_MAX        cfg;
		VDS_PALETTE        palette;
		VDS_QP             qp;
		VDS_COLOR_INVERT   color_invert;
		VDS_ENC_MASK       enc_mask;
	} data;  
} VDS_USER_DATA;

#if defined(__FREERTOS)
extern int nvt_vds_init(void);
extern int nvt_vds_ioctl (int fd, unsigned int cmd, void *arg);
#endif
#endif //_VIDEOSPRITE_H_

