/**
    Public header file for ipl utility

    This file is the header file that define the API and data type for ipl utility tool.

    @file       ipl_util.h
    @ingroup    mILibIPLCom
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _IPL_UTIL_INT_H_
#define _IPL_UTIL_INT_H_

#include "plat/top.h"
#include "comm/timer.h"
#include "kwrap/type.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/util.h"
#include "kwrap/list.h"
#include "kflow_videoprocess/ctl_ipp.h"
#include "kflow_videoprocess/ctl_ipp_util.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"
#if defined(__FREERTOS)
#include <string.h>
#endif

/**
	atomic operations
*/
#if defined(__LINUX)
/* use linux kernel atomic api */
#else
/* use gcc api */
typedef INT32 atomic_t;

#define ATOMIC_INIT(i) {(i)}
#define atomic_inc(x) __sync_add_and_fetch(x, 1)
#define atomic_dec(x) __sync_sub_and_fetch(x, 1)
#define atomic_read(x) __sync_add_and_fetch(x, 0)
#define atomic_set(x, v) __sync_bool_compare_and_swap(x, *x, v)
#define atomic_add(i, x) __sync_add_and_fetch(x, i)
#define atomic_xchg(x, v) __sync_val_compare_and_swap(x, *x, v)

#endif

/**
	vos list head type
*/
typedef struct vos_list_head CTL_IPP_LIST_HEAD;

/**
	page size of FLGPTN
*/
#define CTL_IPP_UTIL_FLAG_PAGE_SIZE (sizeof(FLGPTN) * 8)

/**
	offset of member in type
*/
#define CTL_IPP_UTIL_OFFSETOF(TYPE, MEMBER)  ((UINT32)&((TYPE *)0)->MEMBER)


/**
	default ratio for calculation
*/
#define CTL_IPP_RATIO_UNIT_DFT (1000)
#define CTL_IPP_LCA_H_MIN	(40)
#define CTL_IPP_LCA_V_MIN	(30)

/**
	pattern paste size & position conversion
		pos_per: percentage of pattern image offset in background
		sz: pattern image size (in pixel)
		sz_per: percentage of pattern image size in background
*/
#define CTL_IPP_PATTERN_PASTE_SIZE(sz, sz_per) ALIGN_CEIL((sz) * 100 / (sz_per), 4)
#define CTL_IPP_PATTERN_PASTE_OFS(pos_per, sz, sz_per) 				\
(																	\
	(((pos_per) + (sz_per)) == 100) ?								\
		CTL_IPP_PATTERN_PASTE_SIZE(sz, sz_per) - (sz) : 			\
		((pos_per) * CTL_IPP_PATTERN_PASTE_SIZE(sz, sz_per) / 100) 	\
)

/**
	IFE Ctrl Parameter
*/

typedef struct {
	/* input image info
		addr[0~1] support most 2 frame input
	*/
	VDO_PXLFMT in_fmt;
	UINT8 shdr_enable;
	UINT32 in_addr[2];
	UINT32 in_lofs[2];
	USIZE in_size;
	CTL_IPP_IN_CROP_MODE in_crp_mode;
	URECT in_crp_window;

	/* output image info */
	UINT32 out_lofs;
	VDO_PXLFMT out_fmt;

	/* raw decode */
	UINT8 decode_enable;
	UINT8 decode_ratio;

	/* flip */
	CTL_IPP_FLIP_TYPE flip;

	/* dirct hdr ring buffer line number */
	UINT32 ring_buf_line_num;
} CTL_IPP_IFE_CTRL;

/**
	DCE Ctrl Parameter
*/
#define CTL_IPP_DCE_STRP_NUM_MAX (16)	/*  */

typedef struct {
	/* input image info
		addr[0]:	y channel address
		addr[1]:	uv channel address
		lofs[0]:	y channel lineoffset
		lofs[1]:	uv channel lineoffset
	*/
	VDO_PXLFMT in_fmt;
	UINT32 in_addr[2];
	UINT32 in_lofs[2];
	USIZE in_size;

	/* output image info(d2d output fmt = input fmt)
		addr[0]:	y channel address
		addr[1]:	uv channel address
		lofs[0]:	y channel lineoffset
		lofs[1]:	uv channel lineoffset
	*/
	UINT32 out_addr[2];
	UINT32 out_lofs[2];
	URECT out_crp_window;

	/* stripe info
		strp_h_arr is used to keep first 16 real stripe size when calculate done
		be care of that real stripe max number is 64
	*/
	UINT8 strp_num;
	UINT16 strp_h_arr[CTL_IPP_DCE_STRP_NUM_MAX];
	CTL_IPP_STRP_RULE_SELECT strp_rule;

	/* wdr enable */
	UINT8 wdr_enable;
	UINT32 wdr_in_addr;
	UINT32 wdr_out_addr;

	/* gdc enable */
	UINT8 gdc_enable;
} CTL_IPP_DCE_CTRL;

/**
	IPE Ctrl Parameter
*/

typedef struct {
	/* input image info
		addr[0]:	y channel address
		addr[1]:	uv channel address
		lofs[0]:	y channel lineoffset
		lofs[1]:	uv channel lineoffset
	*/
	VDO_PXLFMT in_fmt;
	UINT32 in_addr[2];
	UINT32 in_lofs[2];
	USIZE in_size;

	/* output image info */
	UINT8 out_enable;
	VDO_PXLFMT out_fmt;
	UINT32 out_addr[2];

	/* Eth parameter */
	CTL_IPP_ISP_ETH eth;

	/* lce parameter */
	UINT32 lce_in_addr;
	UINT32 lce_out_addr;

	/* va parameter */
	UINT8 va_enable;
	UINT32 va_out_addr;
} CTL_IPP_IPE_CTRL;

/**
	IME Ctrl Parameter
*/
#define CTL_IPP_IME_PATH_ID_MAX (CTL_IPP_OUT_PATH_ID_MAX)

typedef struct {
	BOOL enable;
	UINT32 bgn_lofs;
	USIZE bgn_size;
	UPOINT region_ofs;
} CTL_IPP_IME_OUT_REGION;

typedef struct {
	UINT8 enable;
	UINT8 dma_enable;
	UINT8 flip_enable;
	VDO_PXLFMT fmt;
	UINT32 addr[3];
	UINT32 lofs[3];
	USIZE size;
	URECT crp_window;
	UINT8 one_buf_mode_enable;
	UINT8 md_enable;
	UINT32 md_addr;
	UINT32 h_align;
	CTL_IPP_IME_OUT_REGION region;
} CTL_IPP_IME_OUT_IMG;

typedef struct {
	/* input image info */
	VDO_PXLFMT in_fmt;
	UINT32 in_addr[3];
	UINT32 in_lofs[3];
	USIZE in_size;

	/* output image info */
	CTL_IPP_IME_OUT_IMG out_img[CTL_IPP_IME_PATH_ID_MAX];
	CTL_IPP_SCL_METHOD_SEL out_scl_method_sel;
	CTL_IPP_OUT_COLOR_SPACE out_color_space_sel;

	/* lca input/output info */
	UINT32 lca_in_addr;
	UINT32 lca_in_lofs;
	USIZE lca_in_size;

	UINT8 lca_out_enable;
	UINT32 lca_out_addr;
	UINT32 lca_out_lofs;
	USIZE lca_out_size;

	/* lca gray average, update when ime frame end*/
	UINT32 gray_avg_u;
	UINT32 gray_avg_v;

	/* 3DNR input/output info */
	UINT8 tplnr_enable;
	UINT8 tplnr_out_ms_roi_enable;

	VDO_PXLFMT tplnr_in_ref_fmt;
	UINT8 tplnr_in_ref_path;
	USIZE tplnr_in_ref_size;
	UINT32 tplnr_in_ref_addr[3];
	UINT32 tplnr_in_ref_lofs[3];
	UINT8 tplnr_in_ref_flip_enable;

	UINT8 tplnr_in_dec_enable;

	UINT32 tplnr_in_mv_addr;
	UINT32 tplnr_in_ms_addr;
	UINT32 tplnr_in_fcvg_addr;

	UINT32 tplnr_out_mv_addr;
	UINT32 tplnr_out_ms_addr;
	UINT32 tplnr_out_ms_roi_addr;
	UINT32 tplnr_out_fcvg_addr;

	CTL_IPP_ISP_3DNR_STA tplnr_sta_info;

	/* privacy mask */
	UINT8 pm_enable;
	UINT8 pm_pixel_enable;
	UINT32 pm_in_addr;
	UINT32 pm_in_lofs;
	USIZE pm_in_size;

	UINT32 pm_out_addr;

	CTL_IPP_PM_PXL_BLK pm_pxl_blk;

	/* data stamp */
	UINT8 ds_enable;

	/* low delay mode */
	UINT8 low_delay_enable;
	UINT8 low_delay_path;
	UINT32 low_delay_bp;

	/* dram status */
	UINT8 dram_chk_bit;

	/* pattern paste */
	UINT32 pat_paste_enable;
	UINT32 pat_paste_bgn_color[3];
	URECT pat_paste_win;
	CTL_IPP_DS_CST pat_paste_cst_info;
	CTL_IPP_DS_PLT pat_paste_plt_info;
	CTL_IPP_DS_INFO pat_paste_info;
} CTL_IPP_IME_CTRL;

/**
	IFE2 Ctrl Parameter
*/

typedef struct {
	/* input image info */
	VDO_PXLFMT in_fmt;
	UINT32 in_addr;
	UINT32 in_lofs;
	USIZE in_size;

	/* output image info */
	UINT32 out_addr;
	UINT32 out_lofs;

	/* gray average, update when ife2 frame end*/
	UINT32 gray_avg_u;
	UINT32 gray_avg_v;
} CTL_IPP_IFE2_CTRL;


typedef struct {
	CTL_IPP_IFE_CTRL ife_ctrl;
	CTL_IPP_DCE_CTRL dce_ctrl;
	CTL_IPP_IPE_CTRL ipe_ctrl;
	CTL_IPP_IME_CTRL ime_ctrl;
	UINT8 is_fastboot_addr;
} CTL_IPP_BASEINFO;


typedef struct {
	CHAR name[16];
	CTL_IPP_LIST_HEAD free_list_head;
	CTL_IPP_LIST_HEAD used_list_head;
	UINT32 start_addr;
	UINT32 total_size;
	UINT32 blk_num;
	UINT32 blk_size;
	UINT32 cur_free_num;
	UINT32 max_used_num;
	vk_spinlock_t lock;
	ID flg_id;
} CTL_IPP_MEM_POOL;


/**

     ctl_ipp_util_yuvsize

     @note depend on Y width, height and format to calculate buffer size
     @param[in] fmt			image type
     @param[in] y_width		image width(pixel)
     @param[in] y_height	image height(pixel)
     @return
		- @b UINT32    image buffer size
*/
UINT32 ctl_ipp_util_yuvsize(VDO_PXLFMT fmt, UINT32 y_width, UINT32 y_height);

/**

	ctl_ipp_util_y2uvlof

	@note depend on Y lineoffset and format to calculate UV lineoffset
     @param[in] fmt			image type
     @param[in] y_lof		image lineoffset(byte)
     @return
		- @b UINT32    uv lineoffset
*/
UINT32 ctl_ipp_util_y2uvlof(VDO_PXLFMT fmt, UINT32 y_lof);

/**

	ctl_ipp_util_y2uvwidth

	@note depend on Y width and format to calculate UV width
     @param[in] fmt			image type
     @param[in] y_w			image width(pixel)
     @return
		- @b UINT32    uv width
*/
UINT32 ctl_ipp_util_y2uvwidth(VDO_PXLFMT fmt, UINT32 y_w);

/**

	ctl_ipp_util_y2uvheight

	@note depend on Y height and format to calculate UV height
     @param[in] fmt			image type
     @param[in] y_h			image height(pixel)
     @return
		- @b UINT32    uv height
*/
UINT32 ctl_ipp_util_y2uvheight(VDO_PXLFMT fmt, UINT32 y_h);

/**

	ctl_ipp_util_ratio2value

	@note depend on ratio to calculate value
     @param[in] base		base value
     @param[in] ratio		ratio
     @param[in] unit		unit of ratio
     @param[in] align		alignment
     @return
		- @b UINT32    value calculate based on ratio, unit and alignment
						align((base * ratio / unit), x)
*/
UINT32 ctl_ipp_util_ratio2value(UINT32 base, UINT32 ratio, UINT32 unit, UINT32 align);

/**
	ctl_ipp_util_lca_subout_size

	@note calculate lca subout size base on ratio parameter
*/
USIZE ctl_ipp_util_lca_subout_size(UINT32 w, UINT32 h, CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio, UINT32 strp);


/**

	ctl_ipp_util_get_syst_timestamp

	@note get sys timestamp
     @return
		- @b UINT64    timestamp [63..32]sec, [31..0]usec
*/
UINT64 ctl_ipp_util_get_syst_timestamp(void);

/**

	ctl_ipp_util_get_syst_counter

	@note get sys counter
     @return
		- @b UINT32    [31..0]usec
*/
UINT32 ctl_ipp_util_get_syst_counter(void);

/**

	ctl_ipp_util_youtsize

	@note calculate sie yout size
	 @param[in] w		yout window number width
	 @param[in] h		yout window number height
     @return
		- @b UINT32    yout size(byte)
*/
UINT32 ctl_ipp_util_youtsize(UINT32 w, UINT32 h);

/**

	ctl_ipp_util_3dnr_ms_roi_size/width/height/lofs

	@note calculate ime 3dnr ms roi
	 @param[in] w		image width
	 @param[in] h		image height
     @return
		- @b UINT32    ime 3dnr ms roi size/width/height/lofs(byte)
*/
UINT32 ctl_ipp_util_3dnr_ms_roi_size(UINT32 w, UINT32 h);
UINT32 ctl_ipp_util_3dnr_ms_roi_width(UINT32 w);
UINT32 ctl_ipp_util_3dnr_ms_roi_height(UINT32 h);
UINT32 ctl_ipp_util_3dnr_ms_roi_lofs(UINT32 w);

/**

	ctl_ipp_util_pxlfmt_by_flip

	@note adjust pixel start by flip
	 @param[in] pixfmt	start pixel format
	 @param[in] flip	flip function
     @return
		- @b UINT32    pixfmt(byte)
*/
UINT32 ctl_ipp_info_pxlfmt_by_flip(UINT32 pixfmt, CTL_IPP_FLIP_TYPE flip);

/**

	ctl_ipp_util_pxlfmt_by_crop

	@note adjust pixel start by crop position
	 @param[in] pixfmt	start pixel format
	 @param[in] crp_x   crop start x position
	 @param[in] crp_y   crop start y position
     @return
		- @b VDO_PXLFMT    pixfmt(byte)
*/
VDO_PXLFMT ctl_ipp_info_pxlfmt_by_crop(VDO_PXLFMT pixfmt, UINT32 crp_x, UINT32 crp_y);

/**

	ctl_ipp_util_youtsize

	@note calculate sie yout size
	 @param[in] w				image width
	 @param[in] h				iamge height
	 @param[in] out_bit_sel		0 --> output 2 bit, 1 --> output 8 bit
	 @param[in] subsample_en	subsample enable, if true w/2, h/2
     @return
		- @b UINT32    eth size(byte)
*/
UINT32 ctl_ipp_util_ethsize(UINT32 w, UINT32 h, BOOL out_bit_sel, BOOL subsample_en);

/**

     ctl_ipp_util_rawsize

     @note depend on width, height and format to calculate raw buffer size
     @param[in] fmt			image type
     @param[in] w			image width(pixel)
     @param[in] h			image height(pixel)
     @return
		- @b UINT32    raw image buffer size
*/
UINT32 ctl_ipp_util_rawsize(VDO_PXLFMT fmt, UINT32 w, UINT32 h);

/**

	ctl_ipp_util_os_malloc_wrap

	@note allocate buffer for differenct platform
	 @param[in] want_size		size to allocated
     @return
		- @b void*    buffer
*/
void *ctl_ipp_util_os_malloc_wrap(UINT32 want_size);

/**

	ctl_ipp_util_os_mfree_wrap

	@note free buffer for differenct platform
	 @param[in] p_buf		buffer to release
*/
void ctl_ipp_util_os_mfree_wrap(void *p_buf);

/* get direct mode maxima support width */
INT32 ctl_ipp_util_get_max_stripe(void);

/* init and get chip_id, prevent nvt_get_chip_id call malloc in direct mode(ISR) */
void ctl_ipp_util_init_chip_id(void);
UINT32 ctl_ipp_util_get_chip_id(void);


#endif // _IPL_UTIL_INT_H_
