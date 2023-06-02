/**
    Image operation module.

    This module supports rendering osds and masks on video frame.

    @file       videosprite.h
    @ingroup    mIAppVideosprite

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEO_SPRITE_INTERNAL_H_
#define _VIDEO_SPRITE_INTERNAL_H_

#include <kwrap/type.h>
#include "kflow_common/type_vdo.h"

typedef enum {
	VDS_QS_NULL,
	VDS_QS_IME_STAMP,
	VDS_QS_IME_MASK,
	VDS_QS_IME_EXT_STAMP,
	VDS_QS_IME_EXT_MASK,
	VDS_QS_ENC_EXT_STAMP,
	VDS_QS_ENC_EXT_MASK,
	VDS_QS_ENC_COE_STAMP,
	VDS_QS_ENC_JPG_STAMP,
	VDS_QS_VO_STAMP,
	VDS_QS_VO_MASK,
	ENUM_DUMMY4WORD(VDS_QUERY_STAGE)
} VDS_QUERY_STAGE;

typedef enum _VDS_INTERNAL_MASK_TYPE {
	VDS_INTERNAL_MASK_TYPE_NULL,
	VDS_INTERNAL_MASK_TYPE_SOLID,
	VDS_INTERNAL_MASK_TYPE_HOLLOW,
	VDS_INTERNAL_MASK_TYPE_INCONTINUOUS,
	VDS_INTERNAL_MASK_TYPE_MAX,
	ENUM_DUMMY4WORD(VDS_INTERNAL_MASK_TYPE)
} VDS_INTERNAL_MASK_TYPE;

typedef struct {
	UINT32        en;
	VDO_PXLFMT    fmt;
	UPOINT        pos;
	USIZE         size;
	UINT32        addr;
	UINT32        alpha;
} VDS_INTERNAL_EXT_STAMP;

typedef struct {
	UINT32                 en;
	VDS_INTERNAL_MASK_TYPE type;
	UINT32                 color;
	union{
		struct{
			UPOINT      pos[4];
			UINT32      alpha;
		}normal;
		struct{
			UPOINT      pos;
			UINT32      h_line_len;
			UINT32      h_hole_len;
			UINT32      h_thickness;
			UINT32      v_line_len;
			UINT32      v_hole_len;
			UINT32      v_thickness;
		}incontinuous;
	}data;
	UINT32        thickness;
} VDS_INTERNAL_EXT_MASK;

extern int vds_max_coe_stamp;
extern int vds_max_ime_stamp;
extern int vds_max_ime_mask;

extern int   nvt_vds_init(void);
extern void  nvt_vds_exit(void);
extern int   nvt_vds_ioctl (int fd, unsigned int cmd, void *arg);
extern int   vds_init(void);
extern int   vds_uninit(void);
extern void* vds_lock_context(VDS_QUERY_STAGE stage, UINT32 id, UINT32 *ime_flag);
extern int   vds_unlock_context(VDS_QUERY_STAGE stage, UINT32 id, UINT32 ime_flag);
extern int   vds_info_show(void *m, void *v);
extern int   vds_cmd_show(void *m, void *v);
extern void  vds_cmd_write(char *cmd_line, int size);
extern void  vds_save_write(int phase, int pid, int vid, char *directory);

extern UINT32* vds_get_ime_palette(int vid);
extern UINT32* vds_get_coe_palette(int vid);
extern UINT32* vds_get_vo_palette(int vid);

extern void vds_save_latency(VDS_QUERY_STAGE stage, UINT32 id, UINT32 lock, UINT64 time);
extern void vds_latency_show(void *m, void *v, int clear);

#endif //_VIDEO_SPRITE_INTERNAL_H_
