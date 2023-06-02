#ifndef _ISP_ALG_IF_
#define _ISP_ALG_IF_

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _ISP_TRIG_MSG {
	ISP_TRIG_IQ_SIE = 0,
	ISP_TRIG_IQ_SIE_IMM,
	ISP_TRIG_IQ_IPP,
	ISP_TRIG_IQ_IPP_IMM,
	ISP_TRIG_IQ_ENC,
	ISP_TRIG_IQ_MAX,
	ISP_TRIG_MSG_MAX,
	ENUM_DUMMY4WORD(ISP_TRIG_MSG)
} ISP_TRIG_MSG;

typedef enum _ISP_SEN_MODE_TYPE {
	ISP_SEN_MODE_TYPE_UNKNOWN   = 0x00000000,
	//single frame
	ISP_SEN_MODE_LINEAR         = 0x00000001,
	ISP_SEN_MODE_BUILTIN_HDR    = 0x00000002,
	ISP_SEN_MODE_CCIR           = 0x00000004,
	ISP_SEN_MODE_CCIR_INTERLACE = 0x00000008,
	//multiple frames
	ISP_SEN_MODE_STAGGER_HDR    = 0x00000010,
	ISP_SEN_MODE_PDAF           = 0x00000020,
	ENUM_DUMMY4WORD(ISP_SEN_MODE_TYPE)
} ISP_SEN_MODE_TYPE;

typedef enum _ISP_SEN_DATA_FMT {
	ISP_SEN_DATA_FMT_UNKNOW     = 0x00000000,
	ISP_SEN_DATA_FMT_RGB        = 0x00000001,
	ISP_SEN_DATA_FMT_RGBIR      = 0x00000002,
	ISP_SEN_DATA_FMT_RCCB       = 0x00000004,
	ISP_SEN_DATA_FMT_YUV        = 0x00000008,
	ISP_SEN_DATA_FMT_Y_ONLY     = 0x00000010,
	ENUM_DUMMY4WORD(ISP_SEN_DATA_FMT)
} ISP_SEN_DATA_FMT;

typedef enum _ISP_FLOW_TYPE {
	ISP_FLOW_UNKNOWN = 0,
	ISP_FLOW_RAW,
	ISP_FLOW_CCIR,
	ISP_FLOW_MAX,
	ENUM_DUMMY4WORD(ISP_FLOW_TYPE)
} ISP_FLOW_TYPE;

typedef enum _ISP_IQ_ITEM {
	ISP_IQ_ITEM_SIE_ROI,              // datatype: CTL_SIE_ISP_ROI_RATIO
	ISP_IQ_ITEM_SIE_PARAM,            // datatype: CTL_SIE_ISP_IQ_PARAM
	ISP_IQ_ITEM_IFE_PARAM,            // datatype: CTL_IPP_ISP_IFE_IQ_PARAM
	ISP_IQ_ITEM_DCE_PARAM,            // datatype: CTL_IPP_ISP_DCE_IQ_PARAM
	ISP_IQ_ITEM_IPE_PARAM,            // datatype: CTL_IPP_ISP_IPE_IQ_PARAM
	ISP_IQ_ITEM_IME_PARAM,            // datatype: CTL_IPP_ISP_IME_IQ_PARAM
	ISP_IQ_ITEM_IFE_VIG_CENT,         // datatype: CTL_IPP_ISP_IFE_VIG_CENT_RATIO
	ISP_IQ_ITEM_DCE_DC_CENT,          // datatype: CTL_IPP_ISP_DCE_DC_CENT_RATIO
	ISP_IQ_ITEM_IFE2_FILT_TIME,       // datatype: CTL_IPP_ISP_IFE2_FILTER_TIME
	ISP_IQ_ITEM_IME_LCA_SIZE,         // datatype: CTL_IPP_ISP_IME_LCA_SIZE_RATIO
	ISP_IQ_ITEM_IPE_VA_WIN_SIZE,      // datatype: CTL_IPP_ISP_VA_WIN_SIZE_RATIO
	ISP_IQ_ITEM_ENC_3DNR_PARAM,       // datatype: KDRV_VDOENC_3DNR
	ISP_IQ_ITEM_ENC_SHARPEN_PARAM,    // datatype: KDRV_H26XENC_SPN
	ISP_IQ_ITEM_MAX,
	ENUM_DUMMY4WORD(ISP_IQ_ITEM)
} ISP_IQ_ITEM;

typedef struct _ISP_AE_TRIG_OBJ {
	UINT32 func_en;      //enum ISP_FUNC_EN
	UINT32 src_id_mask;
	BOOL reset;          //sie reset flag
} ISP_AE_TRIG_OBJ;

typedef struct _ISP_AF_TRIG_OBJ {
	UINT32 func_en;      //enum ISP_FUNC_EN
	BOOL reset;          //sie reset flag
} ISP_AF_TRIG_OBJ;

typedef struct _ISP_AWB_TRIG_OBJ {
	UINT32 func_en;      //enum ISP_FUNC_EN
	UINT32 src_id_mask;
	BOOL reset;          //sie reset flag
	ISP_SEN_DATA_FMT data_fmt;
} ISP_AWB_TRIG_OBJ;

typedef struct _ISP_IQ_SIE_TRIG_OBJ {
	BOOL reset;                    //sie reset flag
	ISP_SEN_DATA_FMT data_fmt;
	ISP_SEN_MODE_TYPE mode_type;
	UINT32 func_en;                //enum ISP_FUNC_EN
	UINT32 dupl_src_id;
	UINT32 src_id_mask;
} ISP_IQ_SIE_TRIG_OBJ;

typedef struct _ISP_IQ_IPP_TRIG_OBJ {
	BOOL reset;                    //ipp reset flag
	BOOL ipp_capture;              //0: liveview mode; 1: capture mode
	BOOL ipp_direct;               //0: dram mode; 1: direct mode
	UINT32 stripe_num;             //0: no information
	ISP_SEN_DATA_FMT data_fmt;
	ISP_SEN_MODE_TYPE mode_type;
	ISP_FLOW_TYPE flow_type;
	UINT32 func_en;                //enum ISP_FUNC_EN
	UINT32 ipp_info;               //ipp_id>>16
} ISP_IQ_IPP_TRIG_OBJ;

typedef struct _ISP_IQ_ENC_TRIG_OBJ {
	BOOL reset;                    //enc reset flag
	UINT32 func_en;                //enum ISP_FUNC_EN
} ISP_IQ_ENC_TRIG_OBJ;

typedef struct _ISP_IQ_TRIG_OBJ {
	ISP_TRIG_MSG msg;
	ISP_IQ_SIE_TRIG_OBJ sie;
	ISP_IQ_IPP_TRIG_OBJ ipp;
	ISP_IQ_ENC_TRIG_OBJ enc;
} ISP_IQ_TRIG_OBJ;

#endif
