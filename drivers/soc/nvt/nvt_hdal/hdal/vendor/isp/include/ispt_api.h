#ifndef _ISPT_API_H_
#define _ISPT_API_H_

/**
	ISP tuning item
*/
typedef enum _ISPT_ITEM {
	ISPT_ITEM_VERSION            = 0,///< data_type: [Get]     UINT32
	ISPT_ITEM_SIZE_TAB,              ///< data_type: [Get]     ISPT_INFO
	ISPT_ITEM_FUNC,                  ///< data_type: [Get]     ISPT_FUNC
	ISPT_ITEM_YUV,                   ///< data_type: [Set/Get] UINT32, ISPT_YUV_INFO
	ISPT_ITEM_RAW,                   ///< data_type: [Set/Get] UINT32, ISPT_RAW_INFO
	ISPT_ITEM_FRAME,           //  5,///< data_type: [Set/Get] ISPT_MEMORY_INFO
	ISPT_ITEM_MEMORY,                ///< data_type: [Get]     ISPT_MEMORY_INFO
	ISPT_ITEM_SENSOR_INFO,           ///< data_type: [Get]     ISPT_SENSOR_INFO
	ISPT_ITEM_SENSOR_REG,            ///< data_type: [Set/Get] ISPT_SENSOR_REG
	ISPT_ITEM_SENSOR_MODE_INFO,      ///< data_type: [Get]     ISPT_SENSOR_MODE_INFO
	ISPT_ITEM_3DNR_STA,        // 10,///< data_type: [Get]     ISPT_3DNR_STA_INFO
	ISPT_ITEM_EMU_NEW_BUF,           ///< data_type: [Get]     ISPT_EMU_NEW_BUF
	ISPT_ITEM_EMU_RUN,               ///< data_type: [Get]     ISPT_EMU_RUN
	ISPT_ITEM_CHIP_INFO,             ///< data_type: [Get]     ISPT_CHIP_INFO
	ISPT_ITEM_RESERVE_14,
	ISPT_ITEM_RESERVE_15,
	ISPT_ITEM_RESERVE_16,
	ISPT_ITEM_RESERVE_17,
	ISPT_ITEM_RESERVE_18,
	ISPT_ITEM_RESERVE_19,
	ISPT_ITEM_RESERVE_20       = 20,
	ISPT_ITEM_RESERVE_21,
	ISPT_ITEM_RESERVE_22,
	ISPT_ITEM_RESERVE_23,
	ISPT_ITEM_RESERVE_24,
	ISPT_ITEM_RESERVE_25,
	ISPT_ITEM_RESERVE_26,
	ISPT_ITEM_RESERVE_27,
	ISPT_ITEM_CA_DATA,               ///< data_type: [Get]     ISPT_CA_DATA
	ISPT_ITEM_LA_DATA,               ///< data_type: [Get]     ISPT_LA_DATA
	ISPT_ITEM_VA_DATA,         // 30,///< data_type: [Get]     ISPT_VA_DATA
	ISPT_ITEM_VA_INDEP_DATA,         ///< data_type: [Get]     ISPT_VA_INDEP_DATA
	ISPT_ITEM_WAIT_VD,               ///< data_type: [Get]     ISPT_WAIT_VD
	ISPT_ITEM_SENSOR_EXPT,           ///< data_type: [Set/Get] ISPT_SENSOR_EXPT
	ISPT_ITEM_SENSOR_GAIN,           ///< data_type: [Set/Get] ISPT_SENSOR_GAIN
	ISPT_ITEM_D_GAIN,          // 35,///< data_type: [Set]     ISPT_D_GAIN
	ISPT_ITEM_C_GAIN,                ///< data_type: [Set]     ISPT_C_GAIN
	ISPT_ITEM_TOTAL_GAIN,            ///< data_type: [Set]     ISPT_TOTAL_GAIN
	ISPT_ITEM_LV,                    ///< data_type: [Set]     ISPT_LV
	ISPT_ITEM_CT,                    ///< data_type: [Set]     ISPT_CT
	ISPT_ITEM_MOTOR_IRIS,      // 40,///< data_type: [Set]     ISPT_MOTOR_IRIS
	ISPT_ITEM_MOTOR_FOCUS,           ///< data_type: [Set]     ISPT_MOTOR_FOCUS
	ISPT_ITEM_MOTOR_ZOOM,            ///< data_type: [Set]     ISPT_MOTOR_ZOOM
	ISPT_ITEM_MOTOR_MISC,            ///< data_type: [Set]     ISPT_MOTOR_MISC
	ISPT_ITEM_SENSOR_DIRECTION,      ///< data_type: [Set/Get] ISPT_SENSOR_DIRECTION
	ISPT_ITEM_SENSOR_SLEEP,    // 45,///< data_type: [Set]     UINT32
	ISPT_ITEM_SENSOR_WAKEUP,         ///< data_type: [Set]     UINT32
	ISPT_ITEM_HISTO_DATA,            ///< data_type: [Get]     ISPT_HISTO_DATA
	ISPT_ITEM_IR_INFO,               ///< data_type: [Get]     ISPT_IR_INFO
	ISPT_ITEM_MD_DATA,               ///< data_type: [Get]     ISPT_MD_DATA
	ISPT_ITEM_MD_STA,                ///< data_type: [Get]     ISPT_MD_STA
	ISPT_ITEM_RESERVE_51,
	ISPT_ITEM_RESERVE_52,
	ISPT_ITEM_RESERVE_53,
	ISPT_ITEM_RESERVE_54,
	ISPT_ITEM_RESERVE_55,
	ISPT_ITEM_RESERVE_56,
	ISPT_ITEM_RESERVE_57,
	ISPT_ITEM_RESERVE_58,
	ISPT_ITEM_RESERVE_59,
	ISPT_ITEM_MAX               = 60,
	ENUM_DUMMY4WORD(ISPT_ITEM)
} ISPT_ITEM;

// Struct of Parameter
typedef struct ISPT_INFO {
	UINT32 size_tab[ISPT_ITEM_MAX][2];
} ISPT_INFO;

typedef struct _ISPT_FUNC {
	UINT32 id;
	ISP_FUNC_INFO func_info;
} ISPT_FUNC;

typedef struct _ISPT_YUV_INFO {
	UINT32 id;
	ISP_YUV_INFO yuv_info;
} ISPT_YUV_INFO;

typedef struct _ISPT_RAW_INFO {
	UINT32 id;
	ISP_RAW_INFO raw_info;
} ISPT_RAW_INFO;

typedef struct _ISPT_MEMORY_INFO {
	UINT32 size;
	UINT32 addr;
	UINT8 buf[32*1024];
} ISPT_MEMORY_INFO;

typedef struct _ISPT_SENSOR_INFO {
	ISP_SENSOR_INFO sensor_info;
} ISPT_SENSOR_INFO;

typedef struct _ISPT_SENSOR_REG {
	UINT32 id;
	UINT32 addr;
	UINT32 data;
} ISPT_SENSOR_REG;

typedef struct _ISPT_SENSOR_MODE_INFO {
	UINT32 id;
	ISP_SENSOR_MODE_INFO info;
} ISPT_SENSOR_MODE_INFO;

typedef struct _ISPT_3DNR_STA_INFO {
	UINT32 id;
	ISP_3DNR_STA_INFO _3dnr_sta_info;
} ISPT_3DNR_STA_INFO;

#if defined(__KERNEL__) || defined(__FREERTOS)
typedef struct _ISPT_EMU_NEW_BUF {
	UINT32 id;
	INT32 rt;
	CTL_SIE_ISP_SIM_BUF_NEW buf_new;
} ISPT_EMU_NEW_BUF;

typedef struct _ISPT_EMU_RUN {
	UINT32 id;
	INT32 rt;
	CTL_SIE_ISP_SIM_BUF_PUSH buf_push;
} ISPT_EMU_RUN;
#endif

typedef struct _ISPT_CHIP_INFO {
	ISP_CHIP_INFO info;
} ISPT_CHIP_INFO;

typedef struct _ISPT_CA_DATA {
	UINT32 id;
	ISP_CA_RSLT ca_rslt;
} ISPT_CA_DATA;

typedef struct _ISPT_LA_DATA {
	UINT32 id;
	ISP_LA_RSLT la_rslt;
} ISPT_LA_DATA;

typedef struct _ISPT_VA_DATA {
	UINT32 id;
	ISP_VA_RSLT va_rslt;
} ISPT_VA_DATA;

typedef struct _ISPT_VA_INDEP_DATA {
	UINT32 id;
	ISP_VA_INDEP_RSLT va_indep_rslt;
} ISPT_VA_INDEP_DATA;

typedef struct _ISPT_WAIT_VD {
	UINT32 id;
	UINT32 timeout;
} ISPT_WAIT_VD;

typedef struct _ISPT_SENSOR_EXPT {
	UINT32 id;
	UINT32 time[ISP_SEN_MFRAME_MAX_NUM];
} ISPT_SENSOR_EXPT;

typedef struct _ISPT_SENSOR_GAIN {
	UINT32 id;
	UINT32 ratio[ISP_SEN_MFRAME_MAX_NUM];
} ISPT_SENSOR_GAIN;

typedef struct _ISPT_D_GAIN {
	UINT32 id;
	UINT32 gain;
} ISPT_D_GAIN;

typedef struct _ISPT_C_GAIN {
	UINT32 id;
	UINT32 gain[3];
} ISPT_C_GAIN;

typedef struct _ISPT_TOTAL_GAIN {
	UINT32 id;
	UINT32 gain;
} ISPT_TOTAL_GAIN;

typedef struct _ISPT_LV {
	UINT32 id;
	UINT32 lv;
} ISPT_LV;

typedef struct _ISPT_CT {
	UINT32 id;
	UINT32 ct;
} ISPT_CT;

typedef struct _ISPT_MOTOR_IRIS {
	UINT32 cmd_type;
	MTR_CTL_CMD ctl_cmd;
} ISPT_MOTOR_IRIS;

typedef struct _ISPT_MOTOR_FOCUS {
	UINT32 cmd_type;
	MTR_CTL_CMD ctl_cmd;
} ISPT_MOTOR_FOCUS;

typedef struct _ISPT_MOTOR_ZOOM {
	UINT32 cmd_type;
	MTR_CTL_CMD ctl_cmd;
} ISPT_MOTOR_ZOOM;

typedef struct _ISPT_MOTOR_MISC {
	UINT32 cmd_type;
	MTR_CTL_LCMD ctl_cmd;
} ISPT_MOTOR_MISC;

typedef struct _ISPT_SENSOR_DIRECTION {
	UINT32 id;
	ISP_SENSOR_DIRECTION direction;
} ISPT_SENSOR_DIRECTION;

typedef struct _ISPT_HISTO_DATA {
	UINT32 id;
	ISP_HISTO_RSLT histo_rslt;
} ISPT_HISTO_DATA;

typedef struct _ISPT_IR_INFO {
	UINT32 id;
	ISP_RGBIR_INFO info;
} ISPT_IR_INFO;

typedef struct _ISPT_MD_DATA {
	UINT32 id;
	ISP_MD_RSLT md_rslt;
} ISPT_MD_DATA;

typedef struct _ISPT_MD_STA_INFO {
	UINT32 id;
	ISP_MD_STA_INFO md_sta_info;
} ISPT_MD_STA_INFO;

#endif

