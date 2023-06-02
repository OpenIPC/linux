#ifndef _IME_ENG_INT_CAL_H_
#define _IME_ENG_INT_CAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
#include "kwrap/type.h"
#include <mach/rcw_macro.h>
#include "kwrap/error_no.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "rcw_macro.h"
#include "kwrap/error_no.h"
#else
#endif


/**
    IME structure - privacy mask coordinate
*/
typedef struct _IME_PM_POINT {
	INT32 coord_x; ///< coordinate of horizontal direction
	INT32 coord_y; ///< coordinate of vertical direction
} IME_PM_POINT;


typedef struct _IME_TMNR_STATISTIC_PARAM_ {
	UINT32 sample_step_hori;  ///< step size of statistical sampling in horizontal direction
	UINT32 sample_step_vert;  ///< step size of statistical sampling in vertical direction
	UINT32 sample_num_x;   ///< total number of statistical sampling in horizontal direction
	UINT32 sample_num_y;   ///< total number of statistical sampling in vertical direction
	UINT32 sample_st_x;    ///< start point of horizontal direction for statistical sampling process
	UINT32 sample_st_y;    ///< start point of vertical direction for statistical sampling process
} KDRV_IME_TMNR_STATISTIC_PARAM;

typedef struct _IME_TMNR_BUF_SIZE_INFO_ {
	UINT32 in_size_h;               ///< input image width
	UINT32 in_size_v;               ///< input image height

	UINT32 in_sta_max_num;          ///< statistic data maxima number

	UINT32 get_mv_lofs;             ///< get motion vector buffer lineoffset
	UINT32 get_mv_size;             ///< get motion vector buffer size

	UINT32 get_ms_lofs;             ///< get motion status buffer lineoffset
	UINT32 get_ms_size;             ///< get motion status buffer size

	UINT32 get_ms_roi_lofs;         ///< get motion status buffer lineoffset for ROI
	UINT32 get_ms_roi_size;         ///< get motion status buffer size for ROI

	KDRV_IME_TMNR_STATISTIC_PARAM get_sta_param;    ///< get statistic parameters   UINT32 get_sta_lofs;                            ///< get statistic data buffer lineoffset
	UINT32 get_sta_lofs;                            ///< get statistic data buffer lineoffset
	UINT32 get_sta_size;                            ///< get statistic data buffer size

	UINT32 get_fcp_lofs;            ///< get fast converge buffer lineoffset
	UINT32 get_fcp_size;            ///< get fast converge buffer size
} IME_TMNR_BUF_SIZE_INFO;

typedef struct _IME_TMNR_PATCH_SEL_INFO_ {
	UINT32 mix_ratio[2];   ///< Mix ratio in patch selection
	UINT32 mix_th[2];  ///< Mix threshold in patch selection
	UINT32 edge_wet;   ///< Start point of edge adjustment
	UINT32 edge_th[2];    ///< Edge adjustment threshold in patch selection

	UINT32 mix_slope[2]; ///< Mix slope in patch selection
	UINT32 edge_slope;  ///< Edge adjustment slope in patch selection
} IME_TMNR_PATCH_SEL_INFO;


extern UINT32 get_coef_val[17];

extern UINT32 ime_eng_cal_scaling_isd_init_kernel_number(UINT32 in_size, UINT32 out_size);

extern VOID ime_eng_cal_convex_hull_coefs(IME_PM_POINT *p_pm_cvx_point, INT32 pnt_num, INT32 *p_coefs_line1, INT32 *p_coefs_line2, INT32 *p_coefs_line3, INT32 *p_coefs_line4);

extern BOOL ime_eng_verify_isd_output_size(UINT32 stp_max, UINT32 in_size_h, UINT32 out_size_h);

extern ER ime_eng_cal_tmnr_extra_buffer_info(IME_TMNR_BUF_SIZE_INFO *p_tmnr_param);

extern ER ime_eng_cal_tmnr_patch_selection_info(IME_TMNR_PATCH_SEL_INFO *p_tmnr_ps_param);

#ifdef __cplusplus
}
#endif


#endif


