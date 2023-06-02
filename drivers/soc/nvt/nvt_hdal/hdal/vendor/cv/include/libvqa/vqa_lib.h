#ifndef _VQA_H
#define _VQA_H

#include "vendor_isp.h"

#define VQA_VERSION         0x80200917
#define VQA_MAX_CH_NUM	  	1
#define VQA_DEBUG_MODE	  	0
#define VQA_IMG_SIZE 		240*135

#if VQA_DEBUG_MODE
#include    <basetsd.h>
#else
//#include    "ipl_header_infor_yuv.h"
//#include    "ipl_hal_ime_info.h"
//#include    "ErrorNo.h"
//#include    "md_module.h"
#include "hd_type.h"
#endif

typedef HD_RESULT (* clrcst_vendor_isp_init_t)(VOID);
typedef HD_RESULT (* clrcst_vendor_isp_uninit_t)(VOID);
typedef HD_RESULT (* clrcst_vendor_isp_get_common_t)(ISPT_ITEM item, VOID *p_param);

typedef struct _vqa_res_t {
	UINT32 res_too_light;
	UINT32 res_too_dark;
	UINT32 res_blur;
	UINT32 reserved[7];          ///< Reserved words
}vqa_res_t;

typedef struct _enable_param_t {
	UINT32 en_auto_adj_param;
	UINT32 en_ref_md;
	UINT32 en_too_light;
	UINT32 en_too_dark;
	UINT32 en_blur;
	UINT32 reserved[7];          ///< Reserved words
}enable_param_t;

typedef struct _global_param_t {
	UINT32 width;
	UINT32 height;
	UINT32 mb_x_size;
	UINT32 mb_y_size;
	UINT32 auto_adj_period;
	UINT32 g_alarm_frame_num;
	UINT32 reserved[7];          ///< Reserved words
}global_param_t;

typedef struct _light_param_t {
	UINT32 too_light_strength_th;
	UINT32 too_light_cover_th;
	UINT32 too_light_alarm_times;

	UINT32 too_dark_strength_th;
	UINT32 too_dark_cover_th;
	UINT32 too_dark_alarm_times;

	UINT32 reserved[7];          ///< Reserved words
}light_param_t;

typedef struct _contrast_param_t {
	UINT32 blur_strength;
	UINT32 blur_cover_th;
	UINT32 blur_alarm_times;
	UINT32 reserved[7];          ///< Reserved words
}contrast_param_t;

typedef struct _vqa_param_t {
	enable_param_t enable_param;
	global_param_t global_param;
	light_param_t light_param;
	contrast_param_t contrast_param;
	UINT32 reserved[7];          ///< Reserved words
}vqa_param_t;

/**
    Color-Cast parameter

    Structure of parameter of Color-Cast
*/
//@{
typedef struct _CLRCST_PARM_S {
    UINT32 cl_collect_time;
    UINT32 cl_warning_max;
    UINT32 cl_diff_th;
    UINT32 cl_diff_area;
    UINT32 cl_mode;
    UINT32 cl_backup_cnt;
    UINT32 cl_is_debug;
	UINT32 cl_max_yuv_width;
	UINT32 cl_max_yuv_height;
	UINT32 sensor_ch;
	clrcst_vendor_isp_init_t vendor_isp_init;
    clrcst_vendor_isp_uninit_t vendor_isp_uninit;
	clrcst_vendor_isp_get_common_t vendor_isp_get_common;
} CLRCST_PARM_S;

/**
    Color-Cast Result

    Result
*/
typedef enum {
    CL_NORMAL_S          = 0,    ///< normal
    CL_ALARM_S           = 1,    ///< abnormal
    CL_ERROR_S           = 2,    ///< error
} CLRCST_STATUS_S;


#if VQA_DEBUG_MODE
extern void		vqa_init(UINT32 ch, UINT32 buf);
#else
extern VOID		vqa_init(UINT32 ch, UINT32 buf);
#endif

#if VQA_DEBUG_MODE
extern void		vqa_set_param(UINT32 ch, vqa_param_t *vqa_param);
#else
extern VOID		vqa_set_param(UINT32 ch, vqa_param_t *vqa_param);
#endif

extern INT32	vqa_run(UINT32 ch, UINT32 sc_hw_evt, UINT8 *img_y, UINT32 width, UINT32 height, UINT32 src_evt_addr, vqa_res_t *vqa_res);

#if VQA_DEBUG_MODE
extern void		vqa_release(UINT32 ch);
#else
extern UINT32	vqa_calc_buf_size(VOID);

#endif

extern VOID vqa_set_debug(UINT32 is_debug);
extern UINT32 vqa_get_version(UINT32 ver);

extern INT vqa_clrcst_init(CLRCST_PARM_S *cl_parm);
extern UINT32 vqa_clrcst_detect(UINT32 va_addr, UINT32 yuv_width, UINT32 yuv_height);
extern VOID vqa_clrcst_uninit(CLRCST_PARM_S *cl_parm);

#endif
