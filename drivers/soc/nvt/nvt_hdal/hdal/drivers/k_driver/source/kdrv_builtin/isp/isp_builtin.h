#ifndef _ISP_BUILTIN_H_
#define _ISP_BUILTIN_H_

#include "kwrap/type.h"
#include "mach/rcw_macro.h"

//=============================================================================
// version
//=============================================================================
#define ISP_BUILTIN_VERSION 0x01000001

//=============================================================================
// debug message
//=============================================================================
#define ISP_BUILTIN_PRINT_VD_CNT             0  // NOTE: Set value to print VD trigger.
#define ISP_BUILTIN_PRINT_IPP_CNT            0  // NOTE: Set value to print IPP trigger.
#define ISP_BUILTIN_PRINT_EXPT_CNT           0  // NOTE: Set value to print expt.
#define ISP_BUILTIN_PRINT_GAIN_CNT           0  // NOTE: Set value to print gain.
#define ISP_BUILTIN_PRINT_AE_MSG_CNT         0  // NOTE: Set value to AE message.
#define ISP_BUILTIN_PRINT_AWB_MSG_CNT        0  // NOTE: Set value to AWB message.
#define ISP_BUILTIN_PRINT_IQ_MSG_CNT         0  // NOTE: Set value to IQ message.

//=============================================================================
// flow control
//=============================================================================
#define _2A_BUILTIN_NVT                      1  // NOTE: Set 1 to use NVT algorithm.
#define ISP_BUILTIN_TRIG_2A_IQ               1  // NOTE: Set 1 to regist cb function.
#define ISP_BUILTIN_AWB_TRIG_MODE            1  // NOTE: 0: DRAM_END; 1: DRAM_END+VD; 2: VD

//=============================================================================
// code size control
//=============================================================================
#if defined(CONFIG_NVT_FAST_ISP_FLOW)
#define NVT_FAST_ISP_FLOW 1
#else
#define NVT_FAST_ISP_FLOW 0
#endif

//=============================================================================
// struct & enum definition
//=============================================================================
#define FLGPTN_SIE_RESET      FLGPTN_BIT(1)
#define FLGPTN_PROC_DRAMEND   FLGPTN_BIT(2)
#define FLGPTN_PROC_VD        FLGPTN_BIT(3)
#define FLGPTN_SIE_RESET_2    FLGPTN_BIT(4)
#define FLGPTN_PROC_DRAMEND_2 FLGPTN_BIT(5)
#define FLGPTN_PROC_VD_2      FLGPTN_BIT(6)
#define FLGPTN_STOP           FLGPTN_BIT(7)
#define FLGPTN_IDLE           FLGPTN_BIT(31)

#define ISP_BUILTIN_ID_MAX_NUM      5
#define ISP_CA_W_WINNUM             32
#define ISP_CA_H_WINNUM             32
#define ISP_CA_MAX_WINNUM           ISP_CA_W_WINNUM*ISP_CA_H_WINNUM
#define ISP_LA_W_WINNUM             32
#define ISP_LA_H_WINNUM             32
#define ISP_LA_MAX_WINNUM           ISP_LA_W_WINNUM*ISP_LA_H_WINNUM
#define ISP_LA_HIST_NUM             64
#define ISP_HISTO_MAX_SIZE          128
#define ISP_SEN_MFRAME_MAX_NUM      2
#define ISP_BUILTIN_DTSI_MAX        2
#define SUB_NODE_LENGTH             64

// bridge usage
#define SENSOR_PRESET_NAME   0x454E5053 //MAKEFOURCC('S', 'P', 'N', 'E');
#define SENSOR_CHGMODE_FPS   0x53464353 //MAKEFOURCC('S', 'C', 'F', 'S');
#define SENSOR_PRESET_EXPT   0x54455053 //MAKEFOURCC('S', 'P', 'E', 'T');
#define SENSOR_PRESET_GAIN   0x4E475053 //MAKEFOURCC('S', 'P', 'G', 'N');
#define SENSOR_EXPT_MAX      0x584D4553 //MAKEFOURCC('S', 'E', 'M', 'X');
#define SENSOR_I2C_ID        0x44494953 //MAKEFOURCC('S', 'I', 'I', 'D');
#define SENSOR_I2C_ADDR      0x52414953 //MAKEFOURCC('S', 'I', 'A', 'R');
#define ISP_D_GAIN           0x4E474449 //MAKEFOURCC('I', 'D', 'G', 'N');
#define ISP_R_GAIN           0x4E475249 //MAKEFOURCC('I', 'R', 'G', 'N');
#define ISP_G_GAIN           0x4E474749 //MAKEFOURCC('I', 'G', 'G', 'N');
#define ISP_B_GAIN           0x4E474249 //MAKEFOURCC('I', 'B', 'G', 'N');
#define ISP_SHDR_ENABLE      0x45455349 //MAKEFOURCC('I', 'S', 'E', 'E');
#define ISP_SHDR_PATH        0x48505348 //MAKEFOURCC('I', 'S', 'P', 'H');
#define ISP_SHDR_MASK        0x4B4D5349 //MAKEFOURCC('I', 'S', 'M', 'K');
#define ISP_NR_LV            0x4C524E49 //MAKEFOURCC('I', 'N', 'R', 'L');
#define ISP_3DNR_LV          0x4C523349 //MAKEFOURCC('I', '3', 'R', 'L');
#define ISP_SHARPNESS_LV     0x4C535349 //MAKEFOURCC('I', 'S', 'S', 'L');
#define ISP_SATURATION_LV    0x4C4E5349 //MAKEFOURCC('I', 'S', 'N', 'L');
#define ISP_CONTRAST_LV      0x4C544349 //MAKEFOURCC('I', 'C', 'T', 'L');
#define ISP_BRIGHTNESS_LV    0x4C534248 //MAKEFOURCC('I', 'B', 'S', 'L');
#define ISP_NIGHT_MODE       0x4D544E49 //MAKEFOURCC('I', 'N', 'T', 'M');

#define SENSOR_PRESET_NAME_2 0x324E5053 //MAKEFOURCC('S', 'P', 'N', '2');
#define SENSOR_CHGMODE_FPS_2 0x32464353 //MAKEFOURCC('S', 'C', 'F', '2');
#define SENSOR_PRESET_EXPT_2 0x32455053 //MAKEFOURCC('S', 'P', 'E', '2');
#define SENSOR_PRESET_GAIN_2 0x32475053 //MAKEFOURCC('S', 'P', 'G', '2');
#define SENSOR_EXPT_MAX_2    0x324D4553 //MAKEFOURCC('S', 'E', 'M', '2');
#define SENSOR_I2C_ID_2      0x32494953 //MAKEFOURCC('S', 'I', 'I', '2');
#define SENSOR_I2C_ADDR_2    0x32414953 //MAKEFOURCC('S', 'I', 'A', '2');
#define ISP_PATH_2           0x32485049 //MAKEFOURCC('I', 'P', 'H', '2');
#define ISP_D_GAIN_2         0x32474449 //MAKEFOURCC('I', 'D', 'G', '2');
#define ISP_R_GAIN_2         0x32475249 //MAKEFOURCC('I', 'R', 'G', '2');
#define ISP_G_GAIN_2         0x32474749 //MAKEFOURCC('I', 'G', 'G', '2');
#define ISP_B_GAIN_2         0x32474249 //MAKEFOURCC('I', 'B', 'G', '2');
#define ISP_SHDR_ENABLE_2    0x32455349 //MAKEFOURCC('I', 'S', 'E', '2');
#define ISP_SHDR_PATH_2      0x32505348 //MAKEFOURCC('I', 'S', 'P', '2');
#define ISP_SHDR_MASK_2      0x324D5349 //MAKEFOURCC('I', 'S', 'M', '2');
#define ISP_NR_LV_2          0x32524E49 //MAKEFOURCC('I', 'N', 'R', '2');
#define ISP_3DNR_LV_2        0x32523349 //MAKEFOURCC('I', '3', 'R', '2');
#define ISP_SHARPNESS_LV_2   0x32535349 //MAKEFOURCC('I', 'S', 'S', '2');
#define ISP_SATURATION_LV_2  0x324E5349 //MAKEFOURCC('I', 'S', 'N', '2');
#define ISP_CONTRAST_LV_2    0x32544349 //MAKEFOURCC('I', 'C', 'T', '2');
#define ISP_BRIGHTNESS_LV_2  0x32534248 //MAKEFOURCC('I', 'B', 'S', '2');
#define ISP_NIGHT_MODE_2     0x32544E49 //MAKEFOURCC('I', 'N', 'T', '2');

typedef enum _ISP_BUILTIN_AE_TRIG_MSG {
	ISP_BUILTIN_AE_TRIG_RESET,
	ISP_BUILTIN_AE_TRIG_PROC,
	ISP_BUILTIN_AE_TRIG_MSG_MAX,
	ENUM_DUMMY4WORD(ISP_BUILTIN_AE_TRIG_MSG)
} ISP_BUILTIN_AE_TRIG_MSG;

typedef enum _ISP_BUILTIN_AWB_TRIG_MSG {
	ISP_BUILTIN_AWB_TRIG_RESET,
	ISP_BUILTIN_AWB_TRIG_PROC,
	ISP_BUILTIN_AWB_TRIG_MSG_MAX,
	ENUM_DUMMY4WORD(ISP_BUILTIN_AWB_TRIG_MSG)
} ISP_BUILTIN_AWB_TRIG_MSG;

typedef enum _ISP_BUILTIN_IQ_TRIG_MSG {
	ISP_BUILTIN_IQ_TRIG_SIE_RESET,
	ISP_BUILTIN_IQ_TRIG_IPP_RESET,
	ISP_BUILTIN_IQ_TRIG_SIE,
	ISP_BUILTIN_IQ_TRIG_IPP,
	ISP_BUILTIN_IQ_TRIG_CGAIN,
	ISP_BUILTIN_IQ_TRIG_ENC,
	ISP_BUILTIN_IQ_TRIG_MSG_MAX,
	ENUM_DUMMY4WORD(ISP_BUILTIN_IQ_TRIG_MSG)
} ISP_BUILTIN_IQ_TRIG_MSG;

typedef enum _ISP_BUILTIN_AE_STATUS {
	ISP_BUILTIN_AE_STATUS_STABLE     = 0,
	ISP_BUILTIN_AE_STATUS_FINE       = 1,
	ISP_BUILTIN_AE_STATUS_COARSE     = 2,
	ENUM_DUMMY4WORD(ISP_BUILTIN_AE_STATUS)
} ISP_BUILTIN_AE_STATUS;

/**
	type for ISP_CA_RST
	ca result, array size should be ca window num_x * num_y
	user need to prepare memory for these pointer
	CA Output R/G/B/Cnt/IRth/Rth/Gth/Bth @16bit for each window
	each array size should be window num_x * num_y * 2(16bit)
*/
typedef struct _ISP_BUILTIN_CA_RSLT {
	UINT16 r[ISP_CA_MAX_WINNUM];
	UINT16 g[ISP_CA_MAX_WINNUM];
	UINT16 b[ISP_CA_MAX_WINNUM];
	UINT16 ir[ISP_CA_MAX_WINNUM];
	UINT16 rth[ISP_CA_MAX_WINNUM];
	UINT16 gth[ISP_CA_MAX_WINNUM];
	UINT16 bth[ISP_CA_MAX_WINNUM];
	UINT16 acc_cnt[ISP_CA_MAX_WINNUM];
} ISP_BUILTIN_CA_RSLT;

/**
	type for ISP_LA_RST
	la result, array size should be la window num_x * num_y
	histogram array size = 64
	user need to prepare memory for these pointer
	p_buf_lum_1 buffer size: la_win_w * la_win_h * 2
	p_buf_lum_2 buffer size: la_win_w * la_win_h * 2
	p_buf_histogram buffer size: 64 * 2
*/
typedef struct _ISP_BUILTIN_LA_RSLT {
	UINT16 lum_1[ISP_LA_MAX_WINNUM];    // pre gamma result
	UINT16 lum_2[ISP_LA_MAX_WINNUM];    // post-gamma result
	UINT16 histogram[ISP_LA_HIST_NUM];  // Only support SIE 1~4
} ISP_BUILTIN_LA_RSLT;

/**
	type for KDRV_DCE_PARAM_IPL_HIST_RSLT
*/
typedef struct _ISP_BUILTIN_HISTO_RSLT {
	UINT16 hist_stcs_pre_wdr[ISP_HISTO_MAX_SIZE];           ///< histogram statistics
} ISP_BUILTIN_HISTO_RSLT;

typedef struct _ISP_BUILTIN_CGAIN {
	UINT32 r;
	UINT32 g;
	UINT32 b;
} ISP_BUILTIN_CGAIN;

typedef struct _ISP_BUILTIN_SENSOR_CTRL {
	UINT32 exp_time[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 gain_ratio[ISP_SEN_MFRAME_MAX_NUM];
} ISP_BUILTIN_SENSOR_CTRL;

typedef struct _ISP_BUILTIN_DTSI {
	CHAR iq_node_path[SUB_NODE_LENGTH];
	CHAR iq_dpc_node_path[SUB_NODE_LENGTH];
	CHAR iq_shading_node_path[SUB_NODE_LENGTH];
	CHAR iq_ldc_node_path[SUB_NODE_LENGTH];
	CHAR ae_node_path[SUB_NODE_LENGTH];
	CHAR awb_node_path[SUB_NODE_LENGTH];
} ISP_BUILTIN_DTSI;

//=============================================================================
// extern functions
//=============================================================================
extern ISP_BUILTIN_CA_RSLT *isp_builtin_get_ca(UINT32 id);
extern ISP_BUILTIN_LA_RSLT *isp_builtin_get_la(UINT32 id);
extern ISP_BUILTIN_HISTO_RSLT *isp_builtin_get_histo(UINT32 id);
extern UINT32 isp_builtin_get_i2c_id(UINT32 id);
extern UINT32 isp_builtin_get_i2c_addr(UINT32 id);
extern BOOL isp_builtin_get_shdr_enable(UINT32 id);
extern UINT32 isp_builtin_get_shdr_id_mask(UINT32 id);
extern UINT32 isp_builtin_get_sensor_name(UINT32 id);
extern UINT32 isp_builtin_get_chgmode_fps(UINT32 id);
extern UINT32 isp_builtin_get_sensor_expt_max(UINT32 id);
extern UINT32 isp_builtin_get_total_gain(UINT32 id);
extern UINT32 isp_builtin_get_ct(UINT32 id);
extern UINT32 isp_builtin_get_lv(UINT32 id);
extern UINT32 isp_builtin_get_overexposure_offset(UINT32 id);
extern UINT32 isp_builtin_get_scene_chg_w(UINT32 id);
extern UINT32 isp_builtin_get_ae_status(UINT32 id);
extern ISP_BUILTIN_CGAIN *isp_builtin_get_cgain(UINT32 id);
extern UINT32 isp_builtin_get_dgain(UINT32 id);
extern UINT32 isp_builtin_get_nr_lv(UINT32 id);
extern UINT32 isp_builtin_get_3dnr_lv(UINT32 id);
extern UINT32 isp_builtin_get_sharpness_lv(UINT32 id);
extern UINT32 isp_builtin_get_saturation_lv(UINT32 id);
extern UINT32 isp_builtin_get_contrast_lv(UINT32 id);
extern UINT32 isp_builtin_get_brightness_lv(UINT32 id);
extern UINT32 isp_builtin_get_night_mode(UINT32 id);
extern ISP_BUILTIN_SENSOR_CTRL *isp_builtin_get_sensor_gain(UINT32 id);
extern ISP_BUILTIN_SENSOR_CTRL *isp_builtin_get_sensor_expt(UINT32 id);
extern BOOL isp_builtin_get_sensor_valid(UINT32 id);
extern UINT32 isp_builtin_get_sensor_row_time(UINT32 id);
extern UINT32 isp_builtin_get_isp_version(void);
extern UINT32 isp_builtin_get_ae_version(void);
extern UINT32 isp_builtin_get_awb_version(void);
extern UINT32 isp_builtin_get_iq_version(void);
extern UINT32 isp_builtin_get_sensor_version(void);
extern UINT32 isp_builtin_get_ae_msg_cnt(void);
extern UINT32 isp_builtin_get_awb_msg_cnt(void);
extern void isp_builtin_get_ae_param(UINT32 id, UINT32 *param_addr);
extern void isp_builtin_get_awb_param(UINT32 id, UINT32 *param_addr);
extern void isp_builtin_get_iq_param(UINT32 id, UINT32 *param_addr);
extern ISP_BUILTIN_DTSI *isp_builtin_get_dtsi(UINT32 id);
extern void isp_builtin_uninit_i2c(UINT32 id);
extern INT32 isp_builtin_set_transfer_i2c(UINT32 id, void *msgs, INT32 num);
extern void isp_builtin_set_total_gain(UINT32 id, UINT32 value);
extern void isp_builtin_set_ct(UINT32 id, UINT32 value);
extern void isp_builtin_set_lv(UINT32 id, UINT32 value);
extern void isp_builtin_set_overexposure_offset(UINT32 id, UINT32 value);
extern void isp_builtin_set_scene_chg_w(UINT32 id, UINT32 value);
extern void isp_builtin_set_ae_status(UINT32 id, ISP_BUILTIN_AE_STATUS value);
extern void isp_builtin_set_cgain(UINT32 id, ISP_BUILTIN_CGAIN *value);
extern void isp_builtin_set_dgain(UINT32 id, UINT32 value);
extern void isp_builtin_set_sensor_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value);
extern void isp_builtin_set_sensor_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value);
extern ER isp_builtin_init(void);
extern ER isp_builtin_uninit(void);

#endif

