#ifndef _AET_API_H_
#define _AET_API_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "ae_alg.h"
#include "ae_ui.h"
#endif

#define AE_CFG_NAME_LENGTH 256
#define AE_DTSI_NAME_LENGTH 256

/**
	AE tuning item
*/
typedef enum _AET_ITEM {
	AET_ITEM_VERSION             = 0,///< data_type: [Get]     UINT32
	AET_ITEM_SIZE_TAB,               ///< data_type: [Get]     AET_INFO
	AET_ITEM_RLD_CONFIG,             ///< data_type: [Set]     AET_CFG_INFO
	AET_ITEM_RLD_DTSI,               ///< data_type: [Set]     AET_DTSI_INFO
	AET_ITEM_RESERVE_04,
	AET_ITEM_RESERVE_05,
	AET_ITEM_RESERVE_06,
	AET_ITEM_RESERVE_07,
	AET_ITEM_RESERVE_08,
	AET_ITEM_RESERVE_09,
	AET_ITEM_RESERVE_10         = 10,
	AET_ITEM_RESERVE_11,
	AET_ITEM_FREQUENCY,              ///< data_type: [Set/Get] AET_FREQUENCY_MODE
	AET_ITEM_METER,                  ///< data_type: [Set/Get] AET_METER_MODE
	AET_ITEM_EV,                     ///< data_type: [Set/Get] AET_EV_OFFSET
	AET_ITEM_ISO,                    ///< data_type: [Set/Get] AET_ISO_VALUE
	AET_ITEM_RESERVE_16,
	AET_ITEM_LONGEXP,                ///< data_type: [Set/Get] AET_LONGEXP_MODE
	AET_ITEM_LONGEXP_EXPT,           ///< data_type: [Set/Get] AET_LONGEXP_EXPT_VALUE
	AET_ITEM_LONGEXP_ISO,            ///< data_type: [Set/Get] AET_LONGEXP_ISO_VALUE
	AET_ITEM_OPERATION,              ///< data_type: [Set/Get] AET_OPERATION
	AET_ITEM_RESERVE_21,
	AET_ITEM_RESERVE_22,
	AET_ITEM_RESERVE_23,
	AET_ITEM_RESERVE_24,
	AET_ITEM_RESERVE_25,
	AET_ITEM_RESERVE_26,
	AET_ITEM_RESERVE_27,
	AET_ITEM_STITCH_ID,              ///< data_type: [Set/Get] AET_STITCH_ID
	AET_ITEM_APERTURE_BOUND,         ///< data_type: [Set/Get] AET_APERTURE_BOUND
	AET_ITEM_BASE_ISO           = 30,///< data_type: [Set/Get] UINT32
	AET_ITEM_BASE_GAIN_RATIO,        ///< data_type: [Set/Get] UINT32
	AET_ITEM_EXPECT_LUM,             ///< data_type: [Set/Get] AET_EXPECT_LUM
	AET_ITEM_LA_CLAMP,               ///< data_type: [Set/Get] AET_LA_CLAMP
	AET_ITEM_OVER_EXPOSURE,          ///< data_type: [Set/Get] AET_OVER_EXPOSURE
	AET_ITEM_CONVERGENCE,      // 35,///< data_type: [Set/Get] AET_CONVERGENCE
	AET_ITEM_CURVE_GEN_MOVIE,        ///< data_type: [Set/Get] AET_CURVE_GEN_MOVIE
	AET_ITEM_METER_WIN,              ///< data_type: [Set/Get] AET_METER_WINDOW
	AET_ITEM_LUM_GAMMA,              ///< data_type: [Set/Get] AET_LUM_GAMMA
	AET_ITEM_SHDR,                   ///< data_type: [Set/Get] AET_SHDR
	AET_ITEM_IRIS_CFG,         // 40,///< data_type: [Set/Get] AET_IRIS_CFG
	AET_ITEM_CURVE_GEN_PHOTO,        ///< data_type: [Set/Get] AET_CURVE_GEN_PHOTO
	AET_ITEM_MANUAL,                 ///< data_type: [Set/Get] AET_MANUAL
	AET_ITEM_STATUS,                 ///< data_type: [Get]     AET_STATUS_INFO
	AET_ITEM_PRIORITY,               ///< data_type: [Set/Get] AET_PRIORITY
	AET_ITEM_ROI_WIN,          // 45,///< data_type: [Set/Get] AET_ROI_WIN
	AET_ITEM_SMART_IR,               ///< data_type: [Set/Get] AET_SMART_IR
	AET_ITEM_EXPT_BOUND,             ///< data_type: [Set/Get] AET_EXPT_BOUND
	AET_ITEM_GAIN_BOUND,             ///< data_type: [Set/Get] AET_GAIN_BOUND
	AET_ITEM_LA_WIN,                 ///< data_type: [Set/Get] AET_LA_WIN
	AET_ITEM_RESERVE_50,
	AET_ITEM_RESERVE_51,
	AET_ITEM_RESERVE_52,
	AET_ITEM_RESERVE_53,
	AET_ITEM_RESERVE_54,
	AET_ITEM_RESERVE_55,
	AET_ITEM_RESERVE_56,
	AET_ITEM_RESERVE_57,
	AET_ITEM_RESERVE_58,
	AET_ITEM_RESERVE_59,
	AET_ITEM_MAX                = 60,
	ENUM_DUMMY4WORD(AET_ITEM)
} AET_ITEM;

typedef struct AET_INFO {
	UINT32 size_tab[AET_ITEM_MAX][2];
} AET_INFO;

typedef struct _AET_CFG_INFO {
	AE_ID id;
	CHAR path[AE_CFG_NAME_LENGTH];
} AET_CFG_INFO;

typedef struct _AET_DTSI_INFO {
	AE_ID id;
	CHAR node_path[AE_DTSI_NAME_LENGTH];
	CHAR file_path[AE_DTSI_NAME_LENGTH];
	UINT8 *buf_addr;
} AET_DTSI_INFO;

typedef struct _AET_FREQUENCY_MODE {
	UINT32 id;
	AE_FREQUENCY mode;
} AET_FREQUENCY_MODE;

typedef struct _AET_METER_MODE {
	UINT32 id;
	AE_METER mode;
} AET_METER_MODE;

typedef struct _AET_EV_OFFSET {
	UINT32 id;
	AE_EV offset;
} AET_EV_OFFSET;

typedef struct _AET_ISO_VALUE {
	UINT32 id;
	AE_ISO value;
} AET_ISO_VALUE;

typedef struct _AET_LONGEXP_MODE {
	UINT32 id;
	AE_CAP_LONGEXP mode;
} AET_LONGEXP_MODE;

typedef struct _AET_LONGEXP_EXPT_VALUE {
	UINT32 id;
	UINT32 value;
} AET_LONGEXP_EXPT_VALUE;

typedef struct _AET_LONGEXP_ISO_VALUE {
	UINT32 id;
	UINT32 value;
} AET_LONGEXP_ISO_VALUE;

typedef struct _AET_OPERATION {
	UINT32 id;
	AE_OPERATION operation;
} AET_OPERATION;

typedef struct _AET_BASE_ISO {
	AE_ID id;
	UINT32 base_iso;
} AET_BASE_ISO;

typedef struct _AET_BASE_GAIN_RATIO {
	AE_ID id;
	UINT32 base_gain_ratio;
} AET_BASE_GAIN_RATIO;

typedef struct _AET_EXPECT_LUM {
	AE_ID id;
	AE_EXPECT_LUM expect_lum;
} AET_EXPECT_LUM;

typedef struct _AET_LA_CLAMP {
	AE_ID id;
	AE_LA_CLAMP la_clamp;
} AET_LA_CLAMP;

typedef struct _AET_OVER_EXPOSURE {
	AE_ID id;
	AE_OVER_EXPOSURE over_exposure;
} AET_OVER_EXPOSURE;

typedef struct _AET_CONVERGENCE {
	AE_ID id;
	AE_CONVERGENCE convergence;
} AET_CONVERGENCE;

typedef struct _AET_CURVE_GEN_MOVIE {
	AE_ID id;
	AE_CURVE_GEN_MOVIE curve_gen_movie;
} AET_CURVE_GEN_MOVIE;

typedef struct _AET_METER_WINDOW {
	AE_ID id;
	AE_METER_WINDOW meter_window;
} AET_METER_WINDOW;

typedef struct _AET_LUM_GAMMA {
	AE_ID id;
	AE_LUM_GAMMA lum_gamma;
} AET_LUM_GAMMA;

typedef struct _AET_SHDR {
	AE_ID id;
	AE_SHDR shdr;
} AET_SHDR;

typedef struct _AET_IRIS_CFG {
	AE_ID id;
	AE_IRIS_CFG iris_cfg;
} AET_IRIS_CFG;

typedef struct _AET_CURVE_GEN_PHOTO {
	AE_ID id;
	AE_CURVE_GEN_PHOTO curve_gen_photo;
} AET_CURVE_GEN_PHOTO;

typedef struct _AET_MANUAL {
	AE_ID id;
	AE_MANUAL manual;
} AET_MANUAL;

typedef struct _AET_STATUS_INFO {
	AE_ID id;
	AE_STATUS_INFO status_info;
} AET_STATUS_INFO;

typedef struct _AET_PRIORITY {
	AE_ID id;
	UINT32 expt;
	UINT32 iso;
	UINT32 aperture;
} AET_PRIORITY;

typedef struct _AET_ROI_WIN {
	AE_ID id;
	AE_ROI_WIN roi_win;
} AET_ROI_WIN;

typedef struct _AET_SMART_IR {
	AE_ID id;
	AE_SMART_IR_WEIGHT weight;
} AET_SMART_IR;

typedef struct _AET_EXPT_BOUND {
	AE_ID id;
	AE_BOUNDARY bound;
} AET_EXPT_BOUND;

typedef struct _AET_GAIN_BOUND {
	AE_ID id;
	AE_BOUNDARY bound;
} AET_GAIN_BOUND;

typedef struct _AET_LA_WIN {
	AE_ID id;
	AE_LA_WIN la_win;
} AET_LA_WIN;

typedef struct _AET_APERTURE_BOUND {
	AE_ID id;
	AE_APERTURE_BOUND aperture_bound;
} AET_APERTURE_BOUND;

typedef struct _AET_STITCH_ID {
	AE_ID id;
	UINT32 stitch_id;
} AET_STITCH_ID;


#endif

