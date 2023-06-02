#ifndef _AWBT_API_H_
#define _AWBT_API_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "awb_alg.h"
#include "awb_ui.h"
#endif

#define AWB_CFG_NAME_LENGTH 256
#define AWB_DTSI_NAME_LENGTH 256

/**
	AWB tuning item
*/
typedef enum _AWBT_ITEM {
	AWBT_ITEM_VERSION            = 0,///< data_type: [Get]     UINT32
	AWBT_ITEM_SIZE_TAB,              ///< data_type: [Get]     AWBT_INFO
	AWBT_ITEM_RLD_CONFIG,            ///< data_type: [Set]     AWBT_CFG_INFO
	AWBT_ITEM_RLD_DTSI,              ///< data_type: [Set]     AWBT_DTSI_INFO
	AWBT_ITEM_RESERVE_04,
	AWBT_ITEM_RESERVE_05,
	AWBT_ITEM_RESERVE_06,
	AWBT_ITEM_RESERVE_07,
	AWBT_ITEM_RESERVE_08,
	AWBT_ITEM_RESERVE_09,
	AWBT_ITEM_SCENE             = 10,///< data_type: [Set/Get] AWBT_SCENE_MODE
	AWBT_ITEM_WB_RATIO,              ///< data_type: [Set/Get] AWBT_WB_RATIO
	AWBT_ITEM_OPERATION,             ///< data_type: [Set/Get] AWBT_OPERATION
	AWBT_ITEM_STITCH_ID,             ///< data_type: [Set/Get] AWBT_STITCH_ID
	AWBT_ITEM_RESERVE_14,
	AWBT_ITEM_RESERVE_15,
	AWBT_ITEM_RESERVE_16,
	AWBT_ITEM_RESERVE_17,
	AWBT_ITEM_RESERVE_18,
	AWBT_ITEM_RESERVE_19,
	AWBT_ITEM_RESERVE_20,
	AWBT_ITEM_RESERVE_21,
	AWBT_ITEM_RESERVE_22,
	AWBT_ITEM_RESERVE_23,
	AWBT_ITEM_RESERVE_24,
	AWBT_ITEM_RESERVE_25,
	AWBT_ITEM_RESERVE_26,
	AWBT_ITEM_RESERVE_27,
	AWBT_ITEM_RESERVE_28,
	AWBT_ITEM_RESERVE_29,
	AWBT_ITEM_CA_TH,           // 30,///< data_type: [Set/Get] AWBT_CA_TH
	AWBT_ITEM_TH,                    ///< data_type: [Set/Get] AWBT_TH
	AWBT_ITEM_LV,                    ///< data_type: [Set/Get] AWBT_LV
	AWBT_ITEM_CT_WEIGHT,             ///< data_type: [Set/Get] AWBT_CT_WEIGHT
	AWBT_ITEM_TARGET,                ///< data_type: [Set/Get] AWBT_TARGET
	AWBT_ITEM_CT_INFO,         // 35,///< data_type: [Set/Get] AWBT_CT_INFO
	AWBT_ITEM_MWB_GAIN,              ///< data_type: [Set/Get] AWBT_MWB_GAIN
	AWBT_ITEM_CONVERGE,              ///< data_type: [Set/Get] AWBT_CONVERGE
	AWBT_ITEM_EXPAND_BLOCK,          ///< data_type: [Set/Get] AWBT_EXPAND_BLOCK
	AWBT_ITEM_MANUAL,                ///< data_type: [Set/Get] AWBT_MANUAL
	AWBT_ITEM_STATUS,          // 40,///< data_type: [Get]     AWBT_STATUS
	AWBT_ITEM_CA,                    ///< data_type: [Get]     AWBT_CA
	AWBT_ITEM_FLAG,                  ///< data_type: [Get]     AWBT_FLAG
	AWBT_ITEM_LUMA_WEIGHT,           ///< data_type: [Set/Get] AWBT_LUMA_WEIGHT
	AWBT_ITEM_KGAIN_RATIO,           ///< data_type: [Set/Get] AWBT_ITEM_KGAIN_RATIO
	AWBT_ITEM_RESERVE_45,
	AWBT_ITEM_RESERVE_46,
	AWBT_ITEM_RESERVE_47,
	AWBT_ITEM_RESERVE_48,
	AWBT_ITEM_RESERVE_49,
	AWBT_ITEM_MAX               = 50,
	ENUM_DUMMY4WORD(AWBT_ITEM)
} AWBT_ITEM;

typedef struct AWBT_INFO {
	UINT32 size_tab[AWBT_ITEM_MAX][2];
} AWBT_INFO;

typedef struct _AWBT_CFG_INFO {
	AWB_ID id;
	CHAR path[AWB_CFG_NAME_LENGTH];
} AWBT_CFG_INFO;

typedef struct _AWBT_DTSI_INFO {
	AWB_ID id;
	CHAR node_path[AWB_DTSI_NAME_LENGTH];
	CHAR file_path[AWB_DTSI_NAME_LENGTH];
	UINT8 *buf_addr;
} AWBT_DTSI_INFO;

typedef struct _AWBT_SCENE_MODE {
	AWB_ID id;
	AWB_SCENE mode;
} AWBT_SCENE_MODE;

typedef struct _AWBT_WB_RATIO {
	AWB_ID id;
	UINT32 r;
	UINT32 b;
} AWBT_WB_RATIO;

typedef struct _AWBT_OPERATION {
	UINT32 id;
	AWB_OPERATION operation;
} AWBT_OPERATION;

typedef struct _AWBT_STITCH_ID {
	UINT32 id;
	AWB_STITCH_ID stitch_id;
} AWBT_STITCH_ID;

typedef struct _AWBT_CA_TH {
	AWB_ID id;
	AWBALG_CA_THRESHOLD ca_th;
} AWBT_CA_TH;

typedef struct _AWBT_TH {
	AWB_ID id;
	AWB_TH th;
} AWBT_TH;

typedef struct _AWBT_LV {
	AWB_ID id;
	AWB_LV lv;
} AWBT_LV;

typedef struct _AWBT_CT_WEIGHT {
	AWB_ID id;
	AWB_CT_WEIGHT ct_weight;
} AWBT_CT_WEIGHT;

typedef struct _AWBT_TARGET {
	AWB_ID id;
	AWB_TARGET target;
} AWBT_TARGET;

typedef struct _AWBT_CT_INFO {
	AWB_ID id;
	AWB_CT_INFO ct_info;
} AWBT_CT_INFO;

typedef struct _AWBT_MWB_GAIN {
	AWB_ID id;
	AWB_MWB_GAIN mwb_gain;
} AWBT_MWB_GAIN;

typedef struct _AWBT_CONVERGE {
	AWB_ID id;
	AWB_CONVERGE converge;
} AWBT_CONVERGE;

typedef struct _AWBT_EXPAND_BLOCK {
	AWB_ID id;
	AWB_EXPAND_BLOCK expand_block;
} AWBT_EXPAND_BLOCK;

typedef struct _AWBT_MANUAL {
	AWB_ID id;
	AWB_MANUAL manual;
} AWBT_MANUAL;

typedef struct _AWBT_STATUS {
	AWB_ID id;
	AWB_STATUS status;
} AWBT_STATUS;

typedef struct _AWBT_CA {
	AWB_ID id;
	AWB_CA ca;
} AWBT_CA;

typedef struct _AWBT_FLAG {
	AWB_ID id;
	AWB_FLAG flag;
} AWBT_FLAG;

typedef struct _AWBT_LUMA_WEIGHT {
	AWB_ID id;
	AWB_LUMA_WEIGHT luma_weight;
} AWBT_LUMA_WEIGHT;

typedef struct _AWBT_KGAIN_RATIO {
	AWB_ID id;
	AWB_KGAIN_RATIO k_gain;
} AWBT_KGAIN_RATIO;

#endif

