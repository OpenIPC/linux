/**
    @brief Header file of hdal gximage module.\n
    This file contains the functions which is related to hdal gximage in the chip.

    @file hd_gximage.h

    @ingroup gximage

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _HD_GXIMAGE_H_
#define _HD_GXIMAGE_H_

#define GFX_USER_VERSION 20181022

#define GFX_MAX_PLANE_NUM                 3
#define GFX_MAX_MULTI_OUT_NUM             3

#define MAX_JOB_IN_A_LIST 8

typedef enum {
	GFX_USER_CMD_NULL,
	GFX_USER_CMD_NULL1,
	GFX_USER_CMD_NULL2,
	GFX_USER_CMD_NULL3,
	GFX_USER_CMD_INIT,
	GFX_USER_CMD_UNINIT,
	GFX_USER_CMD_COPY,
	GFX_USER_CMD_DMA_COPY,
	GFX_USER_CMD_SCALE,
	GFX_USER_CMD_ROTATE,
	GFX_USER_CMD_COLOR_TRANSFORM,
	GFX_USER_CMD_DRAW_LINE,
	GFX_USER_CMD_DRAW_RECT,
	GFX_USER_CMD_ARITHMETIC,
	GFX_USER_CMD_JOB_LIST,
	GFX_USER_CMD_ISE_SCALE_Y8,
	GFX_USER_CMD_AFFINE,
	GFX_USER_CMD_I8_COLORKEY,
	GFX_USER_CMD_SCALE_LINK_LIST_BUF,
	GFX_USER_CMD_SCALE_LINK_LIST,
	GFX_USER_CMD_ALPHA_BLEND,
	GFX_USER_CMD_RAW_GRAPHIC,
	GFX_USER_CMD_SET_ABORT,
	GFX_USER_CMD_MAX,
	ENUM_DUMMY4WORD(GFX_USER_CMD)
} GFX_USER_CMD;

typedef enum _GFX_GRPH_ENGINE {
	GFX_GRPH_ENGINE_1 = 0,
	GFX_GRPH_ENGINE_2,
	GFX_GRPH_ENGINE_MAX,
	ENUM_DUMMY4WORD(GFX_GRPH_ENGINE)
} GFX_GRPH_ENGINE;

typedef enum {
	GFX_ARITH_BIT_NULL,
	GFX_ARITH_BIT_8,
	GFX_ARITH_BIT_16,
	GFX_ARITH_BIT_MAX,
	ENUM_DUMMY4WORD(GFX_ARITH_BIT)
} GFX_ARITH_BIT;

typedef enum {
	GFX_ARITH_OP_NULL,
	GFX_ARITH_OP_PLUS,
	GFX_ARITH_OP_MINUS,
	GFX_ARITH_OP_MULTIPLY,
	GFX_ARITH_OP_MAX,
	ENUM_DUMMY4WORD(GFX_ARITH_OP)
} GFX_ARITH_OP;

typedef enum {
	GFX_ROTATE_ANGLE_NULL,
	GFX_ROTATE_ANGLE_90,
	GFX_ROTATE_ANGLE_180,
	GFX_ROTATE_ANGLE_270,
	GFX_ROTATE_ANGLE_MIRROR_X,
	GFX_ROTATE_ANGLE_MIRROR_Y,
	GFX_ROTATE_ANGLE_MAX,
	ENUM_DUMMY4WORD(GFX_ROTATE_ANGLE)
} GFX_ROTATE_ANGLE;

typedef enum _GFX_SCALE_METHOD {
	GFX_SCALE_METHOD_NULL,
	GFX_SCALE_METHOD_BICUBIC,
	GFX_SCALE_METHOD_BILINEAR,
	GFX_SCALE_METHOD_NEAREST,
	GFX_SCALE_METHOD_INTEGRATION,
	GFX_SCALE_METHOD_MAX,
	ENUM_DUMMY4WORD(GFX_SCALE_METHOD)
} GFX_SCALE_METHOD;

typedef enum _GFX_ISE_ENGINE {
	GFX_ISE_ENGINE_1 = 0,
	GFX_ISE_ENGINE_2,
	GFX_ISE_ENGINE_MAX,
	ENUM_DUMMY4WORD(GFX_ISE_ENGINE)
} GFX_ISE_ENGINE;

typedef enum _GFX_RECT_TYPE {
	GFX_RECT_TYPE_NULL,
	GFX_RECT_TYPE_SOLID,
	GFX_RECT_TYPE_HOLLOW,
	GFX_RECT_TYPE_MAX,
	ENUM_DUMMY4WORD(GFX_RECT_TYPE)
} GFX_RECT_TYPE;


typedef enum {
	GFX_SET_DRV_ABORT,
	ENUM_DUMMY4WORD(GFX_KDRV_CMD)
} GFX_KDRV_CMD;

typedef struct _GFX_IMG_BUF {
	UINT32                     w;
	UINT32                     h;
	VDO_PXLFMT                 format;
	UINT32                     p_phy_addr[GFX_MAX_PLANE_NUM];
	UINT32                     lineoffset[GFX_MAX_PLANE_NUM];
	UINT32                     palette[16];
} GFX_IMG_BUF;

typedef struct _GFX_COPY {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     colorkey;
	UINT32                     alpha;
	UINT32                     flush;
	GFX_GRPH_ENGINE            engine;
} GFX_COPY;

typedef struct _GFX_DMA_COPY {
	UINT32                     p_src;
	UINT32                     p_dst;
	UINT32                     length;
} GFX_DMA_COPY;

typedef struct _GFX_SCALE {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	INT32                      dst_w;
	INT32                      dst_h;
	GFX_SCALE_METHOD           method;
	INT32                      flush;
} GFX_SCALE;

typedef struct _GFX_ROTATE {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     angle;
	UINT32                     flush;
} GFX_ROTATE;

typedef struct _GFX_COLOR_TRANSFORM {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	UINT32                     p_tmp_buf;
	UINT32                     tmp_buf_size;
	UINT32                     p_lookup_table[GFX_MAX_PLANE_NUM];
} GFX_COLOR_TRANSFORM;

typedef struct _GFX_DRAW_LINE {
	GFX_IMG_BUF                dst_img;
	UINT32                     color;
	UINT32                     start_x;
	UINT32                     start_y;
	UINT32                     end_x;
	UINT32                     end_y;
	UINT32                     thickness;
} GFX_DRAW_LINE;

typedef struct _GFX_DRAW_RECT {
	GFX_IMG_BUF                dst_img;
	GFX_RECT_TYPE              type;
	UINT32                     color;
	UINT32                     x;
	UINT32                     y;
	UINT32                     w;
	UINT32                     h;
	UINT32                     thickness;
	UINT32                     flush;
} GFX_DRAW_RECT;

typedef struct _GFX_ARITHMETIC {
	UINT32                     p_op1;
	UINT32                     p_op2;
	UINT32                     p_out;
	UINT32                     size;
	GFX_ARITH_OP               operation;
	GFX_ARITH_BIT              bits;
} GFX_ARITHMETIC;

typedef struct _GFX_ISE_SCALE_Y8 {
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_w;
	INT32                      dst_h;
	UINT32                     p_src;
	UINT32                     src_loff;
	UINT32                     p_dst;
	UINT32                     dst_loff;
	GFX_SCALE_METHOD           method;
	GFX_ISE_ENGINE             engine;
} GFX_ISE_SCALE_Y8;

typedef struct _GFX_AFFINE {
	INT32                      w;
	INT32                      h;
	INT32                      plane_num;
	UINT32                     p_src[GFX_MAX_PLANE_NUM];
	UINT32                     src_loff[GFX_MAX_PLANE_NUM];
	UINT32                     p_dst[GFX_MAX_PLANE_NUM];
	UINT32                     dst_loff[GFX_MAX_PLANE_NUM];
	UINT32                     uvpack[GFX_MAX_PLANE_NUM];
	float                      fCoeffA[GFX_MAX_PLANE_NUM];
	float                      fCoeffB[GFX_MAX_PLANE_NUM];
	float                      fCoeffC[GFX_MAX_PLANE_NUM];
	float                      fCoeffD[GFX_MAX_PLANE_NUM];
	float                      fCoeffE[GFX_MAX_PLANE_NUM];
	float                      fCoeffF[GFX_MAX_PLANE_NUM];
} GFX_AFFINE;

typedef struct _GFX_SCALE_LINK_LIST_BUF {
	UINT32                     p_addr;
	UINT32                     length;
} GFX_SCALE_LINK_LIST_BUF;

typedef struct _GFX_SCALE_DMA_FLUSH {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      src_x;
	INT32                      src_y;
	INT32                      src_w;
	INT32                      src_h;
	INT32                      dst_x;
	INT32                      dst_y;
	INT32                      dst_w;
	INT32                      dst_h;
	GFX_SCALE_METHOD           method;
	int                        in_buf_flush;
	int                        out_buf_flush;
} GFX_SCALE_DMA_FLUSH;

typedef struct _GFX_ALPHA_BLEND {
	GFX_IMG_BUF                src_img;
	GFX_IMG_BUF                dst_img;
	INT32                      dst_x;
	INT32                      dst_y;
	UINT32                     p_alpha;
	UINT32                     flush;
} GFX_ALPHA_BLEND;

typedef struct _GFX_RAW_GRAPHIC {
	UINT32                     engine;
	UINT32                     p_addr;
	UINT32                     length;
	UINT32                     flush;
} GFX_RAW_GRAPHIC;

typedef struct _GFX_ABORT {
	UINT32                     engine;
} GFX_ABORT;

typedef struct _GFX_USER_DATA {
	UINT32                         version;
	GFX_USER_CMD                   cmd;
	union {
		GFX_COPY                   copy;
		GFX_DMA_COPY               dma_copy;
		GFX_SCALE                  scale;
		GFX_ROTATE                 rotate;
		GFX_COLOR_TRANSFORM        color_transform;
		GFX_DRAW_LINE              draw_line;
		GFX_DRAW_RECT              draw_rect;
		GFX_ARITHMETIC             arithmetic;
		GFX_ISE_SCALE_Y8           ise_scale_y8;
		GFX_AFFINE                 affine;
		GFX_SCALE_LINK_LIST_BUF    scale_link_list_buf;
		GFX_SCALE_DMA_FLUSH        ise_scale_dma_flush;
		GFX_ALPHA_BLEND            alpha_blend;
		GFX_RAW_GRAPHIC            raw_graphic;
		GFX_ABORT                  abort;
	} data;
} GFX_USER_DATA;

#if defined(__FREERTOS)
extern int nvt_gfx_init(void);
extern int nvt_gfx_ioctl(int f, unsigned int cmd, void *arg);
#endif
#endif //_HD_GXIMAGE_H_
