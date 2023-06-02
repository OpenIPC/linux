#ifndef _IQ_DTSI_H_
#define _IQ_DTSI_H_

#define SUB_NODE_LENGTH 64

typedef enum _IQ_DTSI_ITEM {
	IQ_DTSI_ITEM_OB_PARAM               =  0,///< data_type: [Set/Get] IQ_OB_PARAM
	IQ_DTSI_ITEM_NR_PARAM,                   ///< data_type: [Set/Get] IQ_NR_PARAM
	IQ_DTSI_ITEM_CFA_PARAM,                  ///< data_type: [Set/Get] IQ_CFA_PARAM
	IQ_DTSI_ITEM_VA_PARAM,                   ///< data_type: [Set/Get] IQ_VA_PARAM, not ready
	IQ_DTSI_ITEM_GAMMA_PARAM,                ///< data_type: [Set/Get] IQ_GAMMA_PARAM
	IQ_DTSI_ITEM_CCM_PARAM              =  5,///< data_type: [Set/Get] IQ_CCM_PARAM
	IQ_DTSI_ITEM_COLOR_PARAM,                ///< data_type: [Set/Get] IQ_COLOR_PARAM
	IQ_DTSI_ITEM_CONTRAST_PARAM,             ///< data_type: [Set/Get] IQ_CONTRAST_PARAM
	IQ_DTSI_ITEM_EDGE_PARAM,                 ///< data_type: [Set/Get] IQ_EDGE_PARAM
	IQ_DTSI_ITEM_3DNR_PARAM,                 ///< data_type: [Set/Get] IQ_3DNR_PARAM
	IQ_DTSI_ITEM_WDR_PARAM              = 10,///< data_type: [Set/Get] IQ_WDR_PARAM
	IQ_DTSI_ITEM_SHDR_PARAM,                 ///< data_type: [Set/Get] IQ_SHDR_PARAM
	IQ_DTSI_ITEM_RGBIR_PARAM,                ///< data_type: [Set/Get] IQ_RGBIR_PARAM, not ready
	IQ_DTSI_ITEM_COMPANDING_PARAM,           ///< data_type: [Set/Get] IQ_COMPANDING_PARAM
	IQ_DTSI_ITEM_RGBIR_ENH_PARAM,            ///< data_type: [Set/Get] IQ_RGBIR_ENH_PARAM, not ready
	IQ_DTSI_ITEM_POST_SHARPEN_PARAM     = 15,///< data_type: [Set/Get] IQ_POST_SHARPEN_PARAM, not ready
	// depend on module parameter
	IQ_DTSI_ITEM_DPC_PARAM,                  ///< data_type: [Set/Get] IQ_DPC_PARAM
	IQ_DTSI_ITEM_SHADING_PARAM,              ///< data_type: [Set/Get] IQ_SHADING_PARAM
	IQ_DTSI_ITEM_SHADING_INTER_PARAM,        ///< data_type: [Set/Get] IQ_SHADING_PARAM
	IQ_DTSI_ITEM_SHADING_EXT_PARAM,          ///< data_type: [Set/Get] IQ_SHADING_PARAM
	IQ_DTSI_ITEM_LDC_PARAM              = 20,///< data_type: [Set/Get] IQ_LDC_PARAM
	IQ_DTSI_ITEM_YCURVE_PARAM,               ///< data_type: [Set/Get] IQ_YCURVE_PARAM
	IQ_DTSI_ITEM_MAX_NUM,
	ENUM_DUMMY4WORD(IQ_DTSI_ITEM)
} IQ_DTSI_ITEM;

typedef struct _IQ_DTSI {
	CHAR sub_node_name[SUB_NODE_LENGTH];
	INT32 size;
} IQ_DTSI;

extern void iq_dtsi_load(UINT32 id, void *param);

#endif

