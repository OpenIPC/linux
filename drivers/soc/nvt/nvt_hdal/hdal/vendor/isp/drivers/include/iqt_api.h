#ifndef _IQT_API_H_
#define _IQT_API_H_

#if defined(__KERNEL__) || defined(__FREERTOS)
#include "iq_alg.h"
#include "iq_ui.h"
#endif

#define IQ_CFG_NAME_LENGTH 256
#define IQ_DTSI_NAME_LENGTH 256

/**
	IQ tuning item
*/
typedef enum _IQT_ITEM {
	IQT_ITEM_VERSION             = 0,///< data_type: [Get]     UINT32
	IQT_ITEM_SIZE_TAB,               ///< data_type: [Get]     IQT_INFO
	IQT_ITEM_RLD_CONFIG,             ///< data_type: [Set]     IQT_CFG_INFO
	IQT_ITEM_RLD_DTSI,               ///< data_type: [Set]     IQT_DTSI_INFO
	IQT_ITEM_RESERVE_04,
	IQT_ITEM_RESERVE_05,
	IQT_ITEM_RESERVE_06,
	IQT_ITEM_RESERVE_07,
	IQT_ITEM_RESERVE_08,
	IQT_ITEM_RESERVE_09,
	IQT_ITEM_NR_LV              = 10,///< data_type: [Set/Get] IQT_NR_LV
	IQT_ITEM_SHARPNESS_LV,           ///< data_type: [Set/Get] IQT_SHARPNESS_LV
	IQT_ITEM_SATURATION_LV,          ///< data_type: [Set/Get] IQT_SATURATION_LV
	IQT_ITEM_CONTRAST_LV,            ///< data_type: [Set/Get] IQT_CONTRAST_LV
	IQT_ITEM_BRIGHTNESS_LV,          ///< data_type: [Set/Get] IQT_BRIGHTNESS_LV
	IQT_ITEM_NIGHT_MODE         = 15,///< data_type: [Set/Get] IQT_NIGHT_MODE
	IQT_ITEM_YCC_FORMAT,             ///< data_type: [Set/Get] IQT_YCC_FORMAT
	IQT_ITEM_OPERATION,              ///< data_type: [Set/Get] IQT_OPERATION
	IQT_ITEM_IMAGEEFFECT,            ///< data_type: [Set/Get] IQT_IMAGEEFFECT
	IQT_ITEM_CCID,                   ///< data_type: [Set/Get] IQT_CCID
	IQT_ITEM_HUE_SHIFT          = 20,///< data_type: [Set/Get] IQT_HUE_SHIFT
	IQT_ITEM_SHDR_TONE_LV,           ///< data_type: [Set/Get] IQT_SHDR_TONE_LV
	IQT_ITEM_3DNR_LV,                ///< data_type: [Set/Get] IQT_3DNR_LV
	IQT_ITEM_RESERVE_23,
	IQT_ITEM_RESERVE_24,
	IQT_ITEM_RESERVE_25,
	IQT_ITEM_SHADING_INTER_PARAM,    ///< data_type: [Set/Get] IQT_SHADING_INTER_PARAM
	IQT_ITEM_SHADING_EXT_PARAM,      ///< data_type: [Set/Get] IQT_SHADING_EXT_PARAM
	IQT_ITEM_CST_PARAM,              ///< data_type: [Set/Get] IQT_CST_PARAM
	IQT_ITEM_STRIPE_PARAM,           ///< data_type: [Set/Get] IQT_STRIPE_PARAM
	IQT_ITEM_YCURVE_PARAM       = 30,///< data_type: [Set/Get] IQT_YCURVE_PARAM
	IQT_ITEM_OB_PARAM,               ///< data_type: [Set/Get] IQT_OB_PARAM
	IQT_ITEM_NR_PARAM,               ///< data_type: [Set/Get] IQT_NR_PARAM
	IQT_ITEM_CFA_PARAM,              ///< data_type: [Set/Get] IQT_CFA_PARAM
	IQT_ITEM_VA_PARAM,               ///< data_type: [Set/Get] IQT_VA_PARAM
	IQT_ITEM_GAMMA_PARAM        = 35,///< data_type: [Set/Get] IQT_GAMMA_PARAM
	IQT_ITEM_CCM_PARAM,              ///< data_type: [Set/Get] IQT_CCM_PARAM
	IQT_ITEM_COLOR_PARAM,            ///< data_type: [Set/Get] IQT_COLOR_PARAM
	IQT_ITEM_CONTRAST_PARAM,         ///< data_type: [Set/Get] IQT_CONTRAST_PARAM
	IQT_ITEM_EDGE_PARAM,             ///< data_type: [Set/Get] IQT_EDGE_PARAM
	IQT_ITEM_3DNR_PARAM         = 40,///< data_type: [Set/Get] IQT_3DNR_PARAM
	IQT_ITEM_DPC_PARAM,              ///< data_type: [Set/Get] IQT_DPC_PARAM
	IQT_ITEM_SHADING_PARAM,          ///< data_type: [Set/Get] IQT_SHADING_PARAM
	IQT_ITEM_LDC_PARAM,              ///< data_type: [Set/Get] IQT_LDC_PARAM
	IQT_ITEM_RESERVE_44,
	IQT_ITEM_WDR_PARAM          = 45,///< data_type: [Set/Get] IQT_WDR_PARAM
	IQT_ITEM_RESERVE_46,
	IQT_ITEM_SHDR_PARAM,             ///< data_type: [Set/Get] IQT_SHDR_PARAM
	IQT_ITEM_RGBIR_PARAM,            ///< data_type: [Set/Get] IQT_RGBIR_PARAM
	IQT_ITEM_COMPANDING_PARAM,       ///< data_type: [Set/Get] IQT_COMPANDING_PARAM
	IQT_ITEM_EDGE_DBG           = 50,///< data_type: [Get]     IQT_EDGE_DBG
	IQT_ITEM_3DNR_DBG,               ///< data_type: [Get]     IQT_3DNR_DBG
	IQT_ITEM_SHDR_MODE,              ///< data_type: [Set/Get] IQT_SHDR_MODE
	IQT_ITEM_3DNR_MISC_PARAM,        ///< data_type: [Set/Get] IQT_3DNR_MISC_PARAM
	IQT_ITEM_RESERVE_54,
	IQT_ITEM_DR_LEVEL           = 55,///< data_type: [Get]     IQT_DR_LEVEL
	IQT_ITEM_RGBIR_ENH_PARAM,        ///< data_type: [Set/Get] IQT_RGBIR_ENH_PARAM
	IQT_ITEM_RGBIR_ENH_ISO,          ///< data_type: [Get]     IQT_RGBIR_ENH_ISO
	IQT_ITEM_MD_PARAM,               ///< data_type: [Set/Get] IQT_MD_PARAM
	IQT_ITEM_POST_SHARPEN_PARAM,     ///< data_type: [Set/Get] IQT_POST_SHARPEN_PARAM
	IQT_ITEM_RESERVE_60,
	IQT_ITEM_RESERVE_61,
	IQT_ITEM_RESERVE_62,
	IQT_ITEM_RESERVE_63,
	IQT_ITEM_RESERVE_64,
	IQT_ITEM_RESERVE_65,
	IQT_ITEM_RESERVE_66,
	IQT_ITEM_RESERVE_67,
	IQT_ITEM_RESERVE_68,
	IQT_ITEM_RESERVE_69,
	IQT_ITEM_MAX                = 70,
	ENUM_DUMMY4WORD(IQT_ITEM)
} IQT_ITEM;

// Struct of Parameter
typedef struct IQT_INFO {
	UINT32 size_tab[IQT_ITEM_MAX][2];
} IQT_INFO;

typedef struct _IQT_CFG_INFO {
	IQ_ID id;
	CHAR path[IQ_CFG_NAME_LENGTH];
} IQT_CFG_INFO;

typedef struct _IQT_DTSI_INFO {
	IQ_ID id;
	CHAR node_path[IQ_DTSI_NAME_LENGTH];
	CHAR file_path[IQ_DTSI_NAME_LENGTH];
	UINT8 *buf_addr;
} IQT_DTSI_INFO;

typedef struct _IQT_NR_LV {
	IQ_ID id;
	IQ_UI_NR_LV lv;
} IQT_NR_LV;

typedef struct _IQT_3DNR_LV {
	IQ_ID id;
	IQ_UI_3DNR_LV lv;
} IQT_3DNR_LV;

typedef struct _IQT_SHARPNESS_LV {
	IQ_ID id;
	IQ_UI_SHARPNESS_LV lv;
} IQT_SHARPNESS_LV;

typedef struct _IQT_SATURATION_LV {
	IQ_ID id;
	IQ_UI_SATURATION_LV lv;
} IQT_SATURATION_LV;

typedef struct _IQT_CONTRAST_LV {
	IQ_ID id;
	IQ_UI_CONTRAST_LV lv;
} IQT_CONTRAST_LV;

typedef struct _IQT_BRIGHTNESS_LV {
	IQ_ID id;
	IQ_UI_BRIGHTNESS_LV lv;
} IQT_BRIGHTNESS_LV;

typedef struct _IQT_NIGHT_MODE {
	IQ_ID id;
	IQ_UI_NIGHT_MODE mode;
} IQT_NIGHT_MODE;

typedef struct _IQT_YCC_FORMAT {
	IQ_ID id;
	IQ_UI_YCC_FORMAT format;
} IQT_YCC_FORMAT;

typedef struct _IQT_OPERATION {
	IQ_ID id;
	IQ_UI_OPERATION operation;
} IQT_OPERATION;

typedef struct _IQT_IMAGEEFFECT {
	IQ_ID id;
	IQ_UI_IMAGEEFFECT effect;
} IQT_IMAGEEFFECT;

typedef struct _IQT_CCID {
	IQ_ID id;
	IQ_UI_CCID ccid;
} IQT_CCID;

typedef struct _IQT_HUE_SHIFT {
	IQ_ID id;
	IQ_UI_HUE_SHIFT hue_shift;
} IQT_HUE_SHIFT;

typedef struct _IQT_SHDR_TONE_LV {
	IQ_ID id;
	IQ_UI_SHDR_TONE_LV lv;
} IQT_SHDR_TONE_LV;

typedef struct _IQT_OB_PARAM {
	IQ_ID id;
	IQ_OB_PARAM ob;
} IQT_OB_PARAM;

typedef struct _IQT_NR_PARAM {
	IQ_ID id;
	IQ_NR_PARAM nr;
} IQT_NR_PARAM;

typedef struct _IQT_CFA_PARAM {
	IQ_ID id;
	IQ_CFA_PARAM cfa;
} IQT_CFA_PARAM;

typedef struct _IQT_VA_PARAM {
	IQ_ID id;
	IQ_VA_PARAM va;
} IQT_VA_PARAM;

typedef struct _IQT_GAMMA_PARAM {
	IQ_ID id;
	IQ_GAMMA_PARAM gamma;
} IQT_GAMMA_PARAM;

typedef struct _IQT_CCM_PARAM {
	IQ_ID id;
	IQ_CCM_PARAM ccm;
} IQT_CCM_PARAM;

typedef struct _IQT_COLOR_PARAM {
	IQ_ID id;
	IQ_COLOR_PARAM color;
} IQT_COLOR_PARAM;

typedef struct _IQT_CONTRAST_PARAM {
	IQ_ID id;
	IQ_CONTRAST_PARAM contrast;
} IQT_CONTRAST_PARAM;

typedef struct _IQT_EDGE_PARAM {
	IQ_ID id;
	IQ_EDGE_PARAM edge;
} IQT_EDGE_PARAM;

typedef struct _IQT_3DNR_PARAM {
	IQ_ID id;
	IQ_3DNR_PARAM _3dnr;
} IQT_3DNR_PARAM;

typedef struct _IQT_3DNR_MISC_PARAM {
	IQ_ID id;
	IQ_3DNR_MISC_PARAM _3dnr_misc;
} IQT_3DNR_MISC_PARAM;

typedef struct _IQT_DPC_PARAM {
	IQ_ID id;
	IQ_DPC_PARAM dpc;
} IQT_DPC_PARAM;

typedef struct _IQT_SHADING_PARAM {
	IQ_ID id;
	IQ_SHADING_PARAM shading;
} IQT_SHADING_PARAM;

typedef struct _IQT_SHADING_INTER_PARAM {
	IQ_ID id;
	IQ_SHADING_INTER_PARAM shading_inter;
} IQT_SHADING_INTER_PARAM;

typedef struct _IQT_SHADING_EXT_PARAM {
	IQ_ID id;
	IQ_SHADING_EXT_PARAM_IF shading_ext_if;
} IQT_SHADING_EXT_PARAM;

typedef struct _IQT_LDC_PARAM {
	IQ_ID id;
	IQ_LDC_PARAM ldc;
} IQT_LDC_PARAM;

typedef struct _IQT_WDR_PARAM {
	IQ_ID id;
	IQ_WDR_PARAM wdr;
} IQT_WDR_PARAM;

typedef struct _IQT_SHDR_PARAM {
	IQ_ID id;
	IQ_SHDR_PARAM shdr;
} IQT_SHDR_PARAM;

typedef struct _IQT_RGBIR_PARAM {
	IQ_ID id;
	IQ_RGBIR_PARAM rgbir;
} IQT_RGBIR_PARAM;

typedef struct _IQT_COMPANDING_PARAM {
	IQ_ID id;
	IQ_COMPANDING_PARAM companding;
} IQT_COMPANDING_PARAM;

typedef struct _IQT_POST_SHARPEN_PARAM {
	IQ_ID id;
	IQ_POST_SHARPEN_PARAM post_sharpen;
} IQT_POST_SHARPEN_PARAM;

typedef struct _IQT_RGBIR_ENH_PARAM {
	IQ_ID id;
	IQ_RGBIR_ENH_PARAM rgbir_enh;
} IQT_RGBIR_ENH_PARAM;

typedef struct _IQT_DR_LEVEL {
	IQ_ID id;
	UINT32 dr_level;
} IQT_DR_LEVEL;

typedef struct _IQT_RGBIR_ENH_ISO {
	IQ_ID id;
	UINT32 rgbir_enh_iso;
} IQT_RGBIR_ENH_ISO;

typedef struct _IQT_MD_PARAM {
	IQ_ID id;
	IQ_MD_PARAM md;
} IQT_MD_PARAM;

typedef struct _IQT_CST_PARAM {
	IQ_ID id;
	INT16 cst_coef[IQ_CST_LEN];
} IQT_CST_PARAM;

typedef enum _IQT_STRIPE_TYPE {
	IQT_STRIPE_AUTO           = 0,     //Auto calculation
	IQT_STRIPE_MANUAL_2STRIPE = 2,     //Force 2 stripe
	IQT_STRIPE_MANUAL_3STRIPE = 3,     //Force 3 stripe
	IQT_STRIPE_MANUAL_4STRIPE = 4,     //Force 4 stripe
	IQT_STRIPE_MANUAL_5STRIPE = 5,     //Force 5 stripe
	IQT_STRIPE_MANUAL_6STRIPE = 6,     //Force 6 stripe
	IQT_STRIPE_MANUAL_7STRIPE = 7,     //Force 7 stripe
	IQT_STRIPE_MANUAL_8STRIPE = 8,     //Force 8 stripe
	IQT_STRIPE_MANUAL_9STRIPE = 9,     //Force 9 stripe
	ENUM_DUMMY4WORD(IQT_STRIPE_TYPE)
} IQT_STRIPE_TYPE;

typedef struct _IQT_STRIPE_PARAM {
	IQ_ID id;
	IQT_STRIPE_TYPE stripe_type;
} IQT_STRIPE_PARAM;

typedef struct _IQT_YCURVE_PARAM {
	IQ_ID id;
	IQ_YCURVE_PARAM ycurve;
} IQT_YCURVE_PARAM;

// NOTE: kernel
#if defined(__KERNEL__) || defined(__FREERTOS)
typedef struct {
	IQ_ID id;
	CTL_IPE_ISP_EDGEDBG edge_dbg;
} IQT_EDGE_DBG;

typedef struct {
	IQ_ID id;
	CTL_IME_ISP_TMNR_DBG _3dnr_dbg;
} IQT_3DNR_DBG;
#endif
// NOTE: kernel

typedef enum _IQT_SHDR_MODE_SEL {
	IQT_SHDR_MODE_FUSION = 0,
	IQT_SHDR_MODE_SHORT = 1,
	IQT_SHDR_MODE_LONG = 3,
	ENUM_DUMMY4WORD(IQT_SHDR_MODE_SEL)
} IQT_SHDR_MODE_SEL;

typedef struct {
	IQ_ID id;
	IQT_SHDR_MODE_SEL shdr_mode;
} IQT_SHDR_MODE;

#endif

