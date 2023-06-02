

#ifndef _IME_ENG_H_
#define _IME_ENG_H_

#ifdef __cplusplus
extern "C" {
#endif



#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kdrv_type.h"

#include "ime_eng_3dnr_base.h"
#include "ime_eng_control_base.h"
#include "ime_eng_adas_base.h"
#include "ime_eng_compression_base.h"
#include "ime_eng_dbcs_base.h"
#include "ime_eng_in_out_path_base.h"
#include "ime_eng_lca_base.h"
#include "ime_eng_osd_base.h"
#include "ime_eng_pm_base.h"
#include "ime_eng_yuvcvt_base.h"
#include "ime_eng_cal.h"
#include "ime_eng_lcaf_base.h"
#include "ime_eng_limt.h"


#define IME_ENG_REG_NUM         688 // for nt98560

#define IME_ENG_UTIL_BIT_ALL    (0xFFFFFFFF)
#define IME_ENG_UTIL_BIT(n)     (1 << n)


#define IME_LOADTYPE_START          0
#define IME_LOADTYPE_FMEND          1
#define IME_LOADTYPE_DIRECT_START   2



/*****************************************************************************/
typedef void (*IME_ISR_CB)(void *eng, UINT32 status, void *reserve);

typedef struct _IME_ENG_HANDLE_ {
	UINT32 chip_id;
	UINT32 eng_id;
	CHAR name[16];

	UINT32 clock_rate;
	void *pclk;         /* linux struct clk*, not used in freertos */
	UINT32 reg_io_base;
	UINT32 irq_id;

	IME_ISR_CB isr_cb;
} IME_ENG_HANDLE;

typedef struct _IME_ENG_CTL_ {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	IME_ENG_HANDLE *p_eng;
} IME_ENG_CTL;

extern INT32 ime_eng_init(UINT32 chip_num, UINT32 eng_num);
extern INT32 ime_eng_release(void);
extern INT32 ime_eng_init_resource(IME_ENG_HANDLE *p_eng);

IME_ENG_HANDLE *ime_eng_get_handle(UINT32 chip_id, UINT32 eng_id);
extern VOID ime_eng_reg_isr_callback(IME_ENG_HANDLE *p_eng, IME_ISR_CB cb);
extern INT32 ime_eng_open(IME_ENG_HANDLE *p_eng);
extern INT32 ime_eng_close(IME_ENG_HANDLE *p_eng);

extern VOID ime_eng_trig_single_hw_reg(IME_ENG_HANDLE *p_eng);
extern VOID ime_eng_trig_ll_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 ll_addr);
extern VOID ime_eng_write_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 reg_ofs, UINT32 val);
extern VOID ime_eng_stop_single_hw_reg(IME_ENG_HANDLE *p_eng);
extern VOID ime_eng_isr_hw_reg(UINT32 dev_id);
extern VOID ime_eng_load_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 load_type);
extern VOID ime_eng_get_single_output_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 *p_get_en, UINT32 *p_get_ch);
extern VOID ime_eng_hard_reset_hw_reg(IME_ENG_HANDLE *p_eng);

/*****************************************************************************/

/**
    IME enum - system flag clear selection
*/
typedef enum _IME_FLAG_CLEAR_SEL {
	IME_FLAG_NO_CLEAR = 0,    ///< No clear flag
	IME_FLAG_CLEAR    = 1,    ///< Clear flag
	ENUM_DUMMY4WORD(IME_FLAG_CLEAR_SEL)
} IME_FLAG_CLEAR_SEL;


typedef enum {
	IME_ENG_INTERRUPT_LL_END              = 0x00000001,
	IME_ENG_INTERRUPT_LL_ERR              = 0x00000002,
	IME_ENG_INTERRUPT_LL_LATE             = 0x00000004,
	IME_ENG_INTERRUPT_LL_JEND             = 0x00000008,
	IME_ENG_INTERRUPT_BP1                 = 0x00000010,
	IME_ENG_INTERRUPT_BP2                 = 0x00000020,
	IME_ENG_INTERRUPT_BP3                 = 0x00000040,
	IME_ENG_INTERRUPT_TMNR_SLICE_END      = 0x00000080,
	IME_ENG_INTERRUPT_TMNR_MOT_END        = 0x00000100,
	IME_ENG_INTERRUPT_TMNR_MV_END         = 0x00000200,
	IME_ENG_INTERRUPT_TMNR_STA_END        = 0x00000400,
	IME_ENG_INTERRUPT_P1_ENC_OVR          = 0x00000800,
	IME_ENG_INTERRUPT_TMNR_ENC_OVR        = 0x00001000,
	IME_ENG_INTERRUPT_TMNR_DEC_ERR        = 0x00002000,
	IME_ENG_INTERRUPT_FRM_ERR             = 0x00004000,
	IME_ENG_INTERRUPT_FRM_START           = 0x20000000,  ///< frame-start
	IME_ENG_INTERRUPT_STRP_END            = 0x40000000,  ///< stripe-end
	IME_ENG_INTERRUPT_FRM_END             = 0x80000000,  ///< frame-end
	IME_ENG_INTERRUPT_ALL                 = (IME_ENG_INTERRUPT_LL_END | IME_ENG_INTERRUPT_LL_ERR | IME_ENG_INTERRUPT_LL_LATE | IME_ENG_INTERRUPT_LL_JEND | IME_ENG_INTERRUPT_BP1 |
											IME_ENG_INTERRUPT_BP2 | IME_ENG_INTERRUPT_BP3 | IME_ENG_INTERRUPT_TMNR_SLICE_END | IME_ENG_INTERRUPT_TMNR_MOT_END | IME_ENG_INTERRUPT_TMNR_MV_END |
											IME_ENG_INTERRUPT_TMNR_STA_END | IME_ENG_INTERRUPT_P1_ENC_OVR | IME_ENG_INTERRUPT_TMNR_ENC_OVR | IME_ENG_INTERRUPT_TMNR_DEC_ERR | IME_ENG_INTERRUPT_FRM_ERR |
											IME_ENG_INTERRUPT_FRM_START | IME_ENG_INTERRUPT_STRP_END | IME_ENG_INTERRUPT_FRM_END),
} IME_ENG_INTERRUPT;

typedef enum {
	IME_ENG_SINGLE_OUT_CH_LCA           = IME_ENG_UTIL_BIT(0),
	IME_ENG_SINGLE_OUT_CH_REFOUT        = IME_ENG_UTIL_BIT(1),
	IME_ENG_SINGLE_OUT_CH_3DNR_MS       = IME_ENG_UTIL_BIT(2),
	IME_ENG_SINGLE_OUT_CH_3DNR_MS_ROI   = IME_ENG_UTIL_BIT(3),
	IME_ENG_SINGLE_OUT_CH_3DNR_MV       = IME_ENG_UTIL_BIT(4),
	IME_ENG_SINGLE_OUT_CH_3DNR_STA      = IME_ENG_UTIL_BIT(5),
	IME_ENG_SINGLE_OUT_CH_PATH1         = IME_ENG_UTIL_BIT(6),
	IME_ENG_SINGLE_OUT_CH_PATH2         = IME_ENG_UTIL_BIT(7),
	IME_ENG_SINGLE_OUT_CH_PATH3         = IME_ENG_UTIL_BIT(8),
	IME_ENG_SINGLE_OUT_CH_PATH4         = IME_ENG_UTIL_BIT(9),
	IME_ENG_SINGLE_OUT_CH_3DNR_FCVG     = IME_ENG_UTIL_BIT(10),
} IME_ENG_SINGLE_OUT_CH;

typedef enum {
	IME_ENG_SCL_BICUBIC = 0,
	IME_ENG_SCL_BILINEAR,
	IME_ENG_SCL_NEAREST,
	IME_ENG_SCL_INTEGRATION,
	IME_ENG_SCL_MAX
} IME_ENG_SCL_METHOD;

typedef enum  {
	IME_ENG_SCL_DOWN = 0,  ///< scaling down enable
	IME_ENG_SCL_UP = 1,    ///< scaling up enable
} IME_ENG_SCL_UD;



typedef enum {
	IME_ENG_PXLFMT_Y = 0,
	IME_ENG_PXLFMT_YUV420,          ///< Y, UV-PACKED 420
	IME_ENG_PXLFMT_YUV420_PLANAR,   ///< Y, U, V 420 Seperate plane
	IME_ENG_PXLFMT_YUV420_COMPRESS, ///< Y, UV-PACKED 420 COMPRESS
	IME_ENG_PXLFMT_MAX
} IME_ENG_PXLFMT;

typedef struct {
	UINT32 ofs;
	UINT32 val;
} IME_ENG_REG;



typedef struct {
	UINT32 h_n;         ///< h stripe size in each stripe, used for fixed size mode
	UINT32 h_l;         ///< h stripe size of last stripe, used for fixed size mode
	UINT32 h_m;         ///< h stripe number
	UINT32 v_n;         ///< v stripe size in each stripe, used for fixed size mode
	UINT32 v_l;         ///< v stripe size of last stripe, used for fixed size mode
	UINT32 v_m;         ///< v stripe number
} IME_ENG_STRIPE_HV_INFO;

typedef struct {
	IME_ENG_STRIPE_HV_INFO hv_info; ///< calculate by api(ime_eng_cal_hv_stripe)
	UINT32 h_varied_en; ///< h stripe varied size enable
	UINT32 h_varied[8]; ///< h stripe size, 8 entries, used for varied size mode
	UINT32 ovlp_h_size;
	UINT32 prt_h_size;
} IME_ENG_STRIPE_INFO;

typedef struct {
	UINT32 y_min;
	UINT32 y_max;
	UINT32 uv_min;
	UINT32 uv_max;
} IME_ENG_PATH_CLAMP_INFO;

typedef struct {
	UINT32 isd_coef_ctl;    ///< 0:without user coefficient control, 1:with user coefficient control
	UINT32 h_scl_ud;        ///< 0:scale down, 1:scale up
	UINT32 h_scl_dr;
	UINT32 h_scl_ftr;
	UINT32 isd_h_base;
	UINT32 isd_h_ftr[3];
	UINT32 isd_h_coef_nums;
	UINT32 isd_h_coefs[17];
	UINT32 isd_h_coefs_all_sum;
	UINT32 isd_h_coefs_half_sum;

	UINT32 v_scl_ud;        ///< 0:scale down, 1:scale up
	UINT32 v_scl_dr;
	UINT32 v_scl_ftr;
	UINT32 isd_v_base;
	UINT32 isd_v_ftr[3];
	UINT32 isd_v_coef_nums;
	UINT32 isd_v_coefs[17];
	UINT32 isd_v_coefs_all_sum;
	UINT32 isd_v_coefs_half_sum;
} IME_ENG_PATH_SCL_FACTOR;

typedef struct {
	BOOL h_en;
	BOOL v_en;
	UINT32 h_coef;
	UINT32 v_coef;
} IME_ENG_PATH_SCL_FILTER;

typedef struct {
	UINT32 dma_en;
	UINT32 addr[3];
	UINT32 lofs[3];
	IME_ENG_PXLFMT fmt;
	USIZE scl_size;
	URECT crp_window;
	IME_ENG_PATH_CLAMP_INFO clamp;
	IME_ENG_SCL_METHOD scl_method;
	IME_ENG_PATH_SCL_FACTOR scl_factor;
	IME_ENG_PATH_SCL_FILTER scl_filter;
	UINT32 scl_enh_fact;
	UINT32 scl_enh_bit;

	UINT32 enc_smode;
	UINT32 sprt_en;
	UINT32 sprt_pos;
	UINT32 addr_2[3];
	UINT32 lofs_2[3];
} IME_ENG_PATH_INFO;


typedef struct _IME_ENG_GET_STRIPE_OVERLAP_INFO_
{
    UINT16 get_overlap_size;
    UINT16 get_partition_size;
    UINT16 get_stp_size_unit;
} IME_ENG_GET_STRIPE_OVERLAP_INFO;

typedef struct _IME_ENG_CAL_STRIPE_OVERLAP_INFO_
{
    BOOL tmnr_en;       /// tmnr function enable
    BOOL yuyv_en;       /// for path2/3 output YUV422 format (yuyv,yvyu, uyvy, vyuy cases)
    BOOL ycc_comp_en;   /// for path1 and tmnr reference encoder
    UINT32 max_h_scale_down_rate;  /// for path2/3 and lca-subout horizontal scaling down rate; get max size of output path, max_scale_down_rate = ((in_size - 1) << 16) / (out_size - 1)
} IME_ENG_CAL_STRIPE_OVERLAP_INFO;



extern ER ime_eng_cal_hv_stripe(UINT32 w, UINT32 h, UINT32 ovlp, IME_ENG_OPMODE mode, IME_ENG_STRIPE_HV_INFO *p_info);
extern void ime_eng_cal_overlap_size(IME_ENG_OPMODE mode, IME_ENG_CAL_STRIPE_OVERLAP_INFO *p_stp_cal_info, IME_ENG_GET_STRIPE_OVERLAP_INFO *p_get_stp_overlap_info);
extern ER ime_eng_cal_scl_filter(UINT32 in_w, UINT32 in_h, UINT32 out_w, UINT32 out_h, IME_ENG_PATH_SCL_FILTER *p_info);
extern ER ime_eng_cal_scl_factor(UINT32 in_w, UINT32 in_h, UINT32 out_w, UINT32 out_h, UINT32 scl_method, IME_ENG_PATH_SCL_FACTOR *p_info);



extern UINT32 ime_eng_get_reg_base_buf_size(VOID);
extern UINT32 ime_eng_get_reg_flag_buf_size(VOID);
extern VOID ime_eng_set_reg_buf(UINT32 reg_base_addr, UINT32 reg_flag_addr);



extern VOID ime_eng_wait_flag_frame_end(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_frame_end(VOID);

extern VOID ime_eng_wait_flag_linked_list_end(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_linked_list_end(VOID);

extern VOID ime_eng_wait_flag_linked_list_job_end(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_linked_list_job_end(VOID);

extern VOID ime_eng_wait_flag_breakpoint1(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_breakpoint1(VOID);

extern VOID ime_eng_wait_flag_breakpoint2(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_breakpoint2(VOID);

extern VOID ime_eng_wait_flag_breakpoint3(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_breakpoint3(VOID);

extern VOID ime_eng_wait_flag_frame_start(IME_FLAG_CLEAR_SEL is_clear_flag);
extern VOID ime_eng_clear_flag_frame_start(VOID);


extern UINT32 ime_eng_get_avg_u_hw_reg(IME_ENG_HANDLE *p_eng);
extern UINT32 ime_eng_get_avg_v_hw_reg(IME_ENG_HANDLE *p_eng);


extern UINT32 ime_eng_get_dram_out_status_hw_reg(IME_ENG_HANDLE *p_eng);
extern VOID ime_eng_clear_dram_out_status_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 clr_bit);

extern VOID ime_eng_global_load_hw_reg(IME_ENG_HANDLE *p_eng, UINT32 set_en);

extern VOID ime_eng_set_dma_channel_enable_hw_reg(IME_ENG_HANDLE *p_eng, BOOL set_en);
extern UINT8 ime_eng_get_dma_channel_status_hw_reg(IME_ENG_HANDLE *p_eng);


#if (defined(_NVT_EMULATION_) == ON)

extern BOOL ime_end_time_out_status;
//extern void ime_emu_set_dma_en(BOOL set_en);
//extern UINT32 ime_emu_get_dma_status(void);

#endif



#ifdef __cplusplus
}
#endif

#endif

