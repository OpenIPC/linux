/**
 * @file kdrv_md_lmt.h
 * @brief parameter limitation of KDRV MD
 * @author CVAI
 * @date in the year 2019
 */

#ifndef _KDRV_MD_LIMIT_H_
#define _KDRV_MD_LIMIT_H_

//=====================================================
// input / output limitation
#define MD_WIDTH_MIN          60
#define MD_WIDTH_MAX          960
#define MD_WIDTH_ALIGN        2

#define MD_HEIGHT_MIN         60
#define MD_HEIGHT_MAX         1022
#define MD_HEIGHT_ALIGN       2

#define MD_OFSI0_ALIGN      0x00000004
#define MD_OFSI1_ALIGN      0x00000004
#define MD_IN_ADDR_ALIGN    0x00000004
#define MD_OUT_ADDR_ALIGN   0x00000004

//=====================================================
// MdMatch limitation
#define MD_LBSPTH_MAX         255
#define MD_DCOLOUR_MAX        15
#define MD_RCOLOUR_MAX        255
#define MD_DLBSP_MAX          15
#define MD_RLBSP_MAX          15
#define MD_MODEL_NUM_MAX      8
#define MD_MODEL_NUM_ALIGN    4
#define MD_T_ALPHA_MAX        255
#define MD_DW_SHIFT_MAX       4
#define MD_DLAST_ALPHA_MAX    1023
#define MD_DLT_ALPHA_MAX      1023
#define MD_DST_ALPHA_MAX      1023
#define MD_UV_THRES_MAX       255
#define MD_S_ALPHA_MAX        1023
#define MD_DBG_LUMDIFF_MAX    0x0FFFFFFF

//=====================================================
// Morph limitation
#define MD_TH_ERO_MAX         8
#define MD_TH_DIL_MAX         8

//=====================================================
// Update limitation
#define MD_MAX_T              255
#define MD_MAX_FG_Frm         255
#define MD_DEHOST_DTH_MAX     256
#define MD_DEHOST_STH_MAX     255
#define MD_STABLE_FRM_MAX     255
#define MD_UPDATE_DYN_MAX     255
#define MD_VA_DISTTH_MAX      255
#define MD_T_DISTTH_MAX       255
#define MD_DBG_FRMID_MAX      255
#define MD_DBG_RND_MAX        0x7FFF

//=====================================================
// ROI limitation
#define MD_ROI_X_MAX          1023
#define MD_ROI_Y_MAX          1023
#define MD_ROI_W_MAX          1023
#define MD_ROI_H_MAX          1023
#define MD_ROI_UV_THRES_MAX   255
#define MD_ROI_LBSPTH_MAX     255
#define MD_ROI_DCOLOUR_MAX    15
#define MD_ROI_RCOLOUR_MAX    255
#define MD_ROI_DLBSP_MAX      15
#define MD_ROI_RLBSP_MAX      15
#define MD_ROI_MAX_T          255

#endif