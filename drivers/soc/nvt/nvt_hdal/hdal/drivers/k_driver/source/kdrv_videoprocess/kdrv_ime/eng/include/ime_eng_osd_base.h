


#ifndef _IME_ENG_OSD_BASE_H_
#define _IME_ENG_OSD_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "ime_eng_int_comm.h"
#include "ime_eng_int.h"


/**
    IME enum - parameter mode
*/
typedef enum {
	IME_PARAM_AUTO_MODE = 0,    ///< Auto parameter mode
	IME_PARAM_USER_MODE = 1,    ///< User parameter mode
	ENUM_DUMMY4WORD(IME_PARAM_MODE)
} IME_PARAM_MODE;


/**
    IME enum - data stamp processing selection
*/
typedef enum _IME_DS_SETNUM {
	IME_DS_SET0  = 0,  ///< Set0
	IME_DS_SET1  = 1,  ///< Set1
	IME_DS_SET2  = 2,  ///< Set2
	IME_DS_SET3  = 3,  ///< Set3
	ENUM_DUMMY4WORD(IME_DS_SETNUM)
} IME_DS_SETNUM;
/**
    IME enum - data stamp color key mode selection
*/
typedef enum _IME_DS_CK_MODE_SEL_ {
	IME_DS_CK_MODE_RGB   = 0,  ///< color key mode selection, RGB mode
	IME_DS_CK_MODE_ARGB  = 1,  ///< color key mode selection, ARGB mode
	ENUM_DUMMY4WORD(IME_DS_CK_MODE_SEL)
} IME_DS_CK_MODE_SEL;

/**
    IME enum - data stamp color LUT mode selection
*/
typedef enum _IME_DS_FMT_SEL {
	IME_DS_FMT_RGB565   = 0,  ///< data stamp format, RGB565
	IME_DS_FMT_RGB1555  = 1,  ///< data stamp format, RGB1555
	IME_DS_FMT_RGB4444  = 2,  ///< data stamp format, RGB4444
	ENUM_DUMMY4WORD(IME_DS_FMT_SEL)
} IME_DS_FMT_SEL;

/**
    IME enum - data stamp color palette mode selection
*/
typedef enum _IME_DS_PLT_SEL_ {
	IME_DS_PLT_1BIT   = 0,  ///< color palette, 1bit mode
	IME_DS_PLT_2BIT   = 1,  ///< color palette, 2bit mode
	IME_DS_PLT_4BIT   = 2,  ///< color palette, 4bit mode
	ENUM_DUMMY4WORD(IME_DS_PLT_SEL)
} IME_DS_PLT_SEL;


/**
    IME structure - image cooridinate for horizontal and vertical directions
*/
typedef struct _IME_POS_INFO {
	UINT32 pos_x;
	UINT32 pos_y;
} IME_POS_INFO;


/**
    IME structure - YUV color LUT parameters of data stamp
*/
typedef struct _IME_STAMP_CST_INFO {
	IME_PARAM_MODE  ds_cst_param_mode;    ///< parameter mode, recommended: IME_PARAM_AUTO_MODE
	UINT32  ds_cst_coef0; ///< coefficient of color space transform
	UINT32  ds_cst_coef1; ///< coefficient of color space transform
	UINT32  ds_cst_coef2; ///< coefficient of color space transform
	UINT32  ds_cst_coef3; ///< coefficient of color space transform
} IME_STAMP_CST_INFO;


/**
    IME structure - image parameters for data stamp
*/
typedef struct _IME_STAMP_IMAGE_INFO {
	IME_SIZE_INFO   ds_img_size;  ///< Image size
	IME_DS_FMT_SEL  ds_fmt;     ///< Image format selection
	IME_POS_INFO    ds_pos;      ///< Blending position
	UINT32          ds_lofs;   ///< Data lineoffset
	UINT32          ds_addr;   ///< Data address
} IME_STAMP_IMAGE_INFO;


/**
    IME structure - image parameters for data stamp

    Blending weighting Usage:
    if RGB565, weight = uiBldWt0 + (uiBldWt1 << 4).
    if ARGB1555, A = 0, by using uiBldWt0 to index internal weighting table; A = 1, by using uiBldWt1 to index internal weighting table
    if ARGB1555, by using value of A to index internal weighting table
    Internal weighting table: [0,  17,  34,  51,  68,  85,  102,  119,  136,  153,  170,  187,  204,  221,  238,  255]
*/
typedef struct _IME_STAMP_IQ_INFO {
	IME_FUNC_EN        ds_ck_enable;        ///< color key enable
	IME_FUNC_EN        ds_plt_enable;       ///< color palette enable
	IME_DS_CK_MODE_SEL ds_ck_mode;      ///< color key mode selection
	UINT16             ds_ck_a;         ///< Color key for alpha channel
	UINT16             ds_ck_r;         ///< Color key for R channel
	UINT16             ds_ck_g;         ///< Color key for G channel
	UINT16             ds_ck_b;         ///< Color key for B channel
	UINT32             ds_bld_wet0;     ///< Blending weighting, if RGB565, range: [0, 16]; others, range: [0, 15]
	UINT32             ds_bld_wet1;     ///< Blending weighting, range: [0, 15]
} IME_STAMP_IQ_INFO;


typedef struct _IME_STAMP_PLT_INFO_ {
	IME_DS_PLT_SEL  ds_plt_mode;        ///< color palette mode selection
	UINT8           *ds_plt_tab_a;   ///< color palette table, channel a
	UINT8           *ds_plt_tab_r;   ///< color palette table, channel r
	UINT8           *ds_plt_tab_g;   ///< color palette table, channel g
	UINT8           *ds_plt_tab_b;   ///< color palette table, channel b
} IME_STAMP_PLT_INFO;


extern ER ime_eng_set_data_stamp_enable_buf_reg(IME_DS_SETNUM set_num, BOOL set_en);

extern ER ime_eng_set_data_stamp_cst_enable_buf_reg(BOOL set_en);

extern ER ime_eng_set_data_stamp_color_key_enable_buf_reg(IME_DS_SETNUM set_num, BOOL set_en);

extern ER ime_eng_set_data_stamp_color_palette_buf_reg(IME_STAMP_PLT_INFO *p_set_info);

extern ER ime_eng_set_data_stamp_color_coefs_param_buf_reg(UINT32 c0, UINT32 c1, UINT32 c2, UINT32 c3);

extern ER ime_eng_set_data_stamp_image_param_buf_reg(IME_DS_SETNUM set_num, IME_STAMP_IMAGE_INFO *p_set_info);

extern ER ime_eng_set_data_stamp_param_buf_reg(IME_DS_SETNUM set_num, IME_STAMP_IQ_INFO *p_set_info);


#if 0
//-------------------------------------------------------------------------------
// data stamp
extern VOID ime_eng_set_osd_cst_enable_buf_reg(UINT32 set_en);


// data stamp0
extern VOID ime_eng_set_osd_set0_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set0_image_size_buf_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_osd_set0_format_buf_reg(UINT32 set_fmt);
extern VOID ime_eng_set_osd_set0_blend_weight_buf_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_eng_set_osd_set0_position_buf_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_eng_set_osd_set0_color_key_buf_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_eng_set_osd_set0_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_osd_set0_dma_addr_buf_reg(UINT32 addr);
extern VOID ime_eng_set_osd_set0_color_key_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set0_color_palette_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set0_color_key_mode_buf_reg(UINT32 set_mode);

//-------------------------------------------------------------------------------
// data stamp1
extern VOID ime_eng_set_osd_set1_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set1_image_size_buf_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_osd_set1_blend_weight_buf_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_eng_set_osd_set1_format_buf_reg(UINT32 fmt);
extern VOID ime_eng_set_osd_set1_position_buf_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_eng_set_osd_set1_color_key_buf_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_eng_set_osd_set1_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_osd_set1_dma_addr_buf_reg(UINT32 addr);
extern VOID ime_eng_set_osd_set1_color_key_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set1_color_palette_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set1_color_key_mode_buf_reg(UINT32 set_mode);


//-------------------------------------------------------------------------------
// data stamp2
extern VOID ime_eng_set_osd_set2_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set2_image_size_buf_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_osd_set2_blend_weight_buf_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_eng_set_osd_set2_format_buf_reg(UINT32 fmt);
extern VOID ime_eng_set_osd_set2_position_buf_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_eng_set_osd_set2_color_key_buf_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_eng_set_osd_set2_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_osd_set2_dma_addr_buf_reg(UINT32 addr);
extern VOID ime_eng_set_osd_set2_color_key_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set2_color_palette_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set2_color_key_mode_buf_reg(UINT32 set_mode);



//-------------------------------------------------------------------------------
// data stamp3
extern VOID ime_eng_set_osd_set3_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set3_image_size_buf_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_osd_set3_blend_weight_buf_reg(UINT32 wet0, UINT32 wet1);
extern VOID ime_eng_set_osd_set3_format_buf_reg(UINT32 fmt);
extern VOID ime_eng_set_osd_set3_position_buf_reg(UINT32 pos_h, UINT32 pos_v);
extern VOID ime_eng_set_osd_set3_color_key_buf_reg(UINT32 ck_a, UINT32 ck_r, UINT32 ck_g, UINT32 ck_b);
extern VOID ime_eng_set_osd_set3_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_osd_set3_dma_addr_buf_reg(UINT32 addr);
extern VOID ime_eng_set_osd_set3_color_key_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set3_color_palette_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_osd_set3_color_key_mode_buf_reg(UINT32 set_mode);


// data stamp CST coefficients
extern VOID ime_eng_set_osd_cst_coef_buf_reg(UINT32 c0, UINT32 c1, UINT32 c2, UINT32 c3);
extern VOID ime_eng_set_osd_color_palette_mode_buf_reg(UINT32 set_mode);
extern VOID ime_eng_set_osd_color_palette_lut_buf_reg(UINT8 *p_lut_a, UINT8 *p_lut_r, UINT8 *p_lut_g, UINT8 *p_lut_b);


#if (IME_GET_API_EN == 1)

extern VOID ime_eng_get_osd_set0_image_size_buf_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);
extern VOID ime_eng_get_osd_set0_lineoffset_buf_reg(UINT32 *g_get_lofs);
extern VOID ime_eng_get_osd_set0_dma_addr_buf_reg(UINT32 *p_get_addr);
extern UINT32 ime_eng_get_osd_set0_enable_buf_reg(VOID);

extern VOID ime_eng_get_osd_set1_image_size_buf_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_eng_get_osd_set1_lineoffset_buf_reg(UINT32 *p_lofs);
extern VOID ime_eng_get_osd_set1_dma_addr_buf_reg(UINT32 *p_addr);
extern UINT32 ime_eng_get_osd_set1_enable_buf_reg(VOID);

extern VOID ime_eng_get_osd_set2_image_size_buf_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_eng_get_osd_set2_lineoffset_buf_reg(UINT32 *p_lofs);
extern VOID ime_eng_get_osd_set2_dma_addr_buf_reg(UINT32 *p_addr);
extern UINT32 ime_eng_get_osd_set2_enable_buf_reg(VOID);

extern VOID ime_eng_get_osd_set3_image_size_buf_reg(UINT32 *p_size_h, UINT32 *p_size_v);
extern VOID ime_eng_get_osd_set3_lineoffset_buf_reg(UINT32 *p_lofs);
extern VOID ime_eng_get_osd_set3_dma_addr_buf_reg(UINT32 *p_addr);
extern UINT32 ime_eng_get_osd_set3_enable_buf_reg(VOID);


#endif

#endif

#ifdef __cplusplus
}
#endif

#endif

