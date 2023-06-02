/**
	Public header file for kdrv_sie

	This file is the header file that define the API and data type for kdrv_sie.

	@file	   kdrv_sie.h
	@ingroup	mILibIPLCom
	@note	   Nothing (or anything need to be mentioned).

	Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#ifndef _KDRV_SIE_H_
#define _KDRV_SIE_H_

#include "kwrap/type.h"
#include "kdrv_type.h"

#define KDRV_SIE_MAX_ENG 				3	//maximum support engine number for all chip

#define KDRV_SIE_MAX_ENG_NT98560		3	//maximum support engine number for 98560

#define SSDRV_SIE_READY DISABLE

//function/statistic out maximum value
#define KDRV_SIE_CA_MAX_CROP_W 			5104
#define KDRV_SIE_CA_MAX_CROP_H 			4080
#define KDRV_SIE_LA_MAX_CROP_W 			320
#define KDRV_SIE_LA_MAX_CROP_H 			256
#define KDRV_SIE_CA_MAX_WINNUM 			32
#define KDRV_SIE_LA_MAX_WINNUM 			32
#define KDRV_SIE_LA_GAMMA_TBL_NUM		65
#define KDRV_SIE_LA_HIST_BIN			64
#define KDRV_SIE_VA_MAX_WINNUM 			16
#define KDRV_SIE_VA_MAX_WIN_SZ 			255
#define KDRV_SIE_VA_GAMMA_TAB_NUM 		33
#define KDRV_SIE_VA_INDEP_NUM 			5
#define KDRV_SIE_VA_INDEP_MAX_WIN_SZ 	511
#define KDRV_SIE_YOUT_MAX_WIN_NUM 		128
#define KDRV_SIE_DPC_MAX_PXCNT 			4096
#define KDRV_SIE_ECS_MAX_MAP_NUM 		65
#define KDRV_SIE_ECS_DITHER_MAX_LVL		7	//iq

#define KDRV_SIE_COMPANDING_MAX_LEN 	12	//iq

#define KDRV_SIE_VAR2STR(x)   (#x)
#define KDRV_SIE_DUMPD(x)	  DBG_DUMP("%s = %d\r\n", KDRV_SIE_VAR2STR(x), x)
#define KDRV_SIE_DUMPH(x)	  DBG_DUMP("%s = 0x%.8x\r\n", KDRV_SIE_VAR2STR(x), x)

/**
	sie support function.
*/
typedef enum {
	KDRV_SIE_FUNC_SPT_NONE          = 0x00000000, ///< none
	KDRV_SIE_FUNC_SPT_DIRECT        = 0x00000001, ///< direct to ipp
	KDRV_SIE_FUNC_SPT_PATGEN        = 0x00000002, ///< pattern gen
	KDRV_SIE_FUNC_SPT_DVI           = 0x00000004, ///< dvi sensor
	KDRV_SIE_FUNC_SPT_OB_AVG        = 0x00000008, ///< ob average
	KDRV_SIE_FUNC_SPT_OB_BYPASS     = 0x00000010, ///< ob bypass
	KDRV_SIE_FUNC_SPT_OB_FRAME_AVG  = 0x00000020, ///< ob frame average
	KDRV_SIE_FUNC_SPT_OB_PLANE_SUB  = 0x00000040, ///< ob plane sub
	KDRV_SIE_FUNC_SPT_YOUT       	= 0x00000080, ///< y out
	KDRV_SIE_FUNC_SPT_CGAIN         = 0x00000100, ///< color gain
	KDRV_SIE_FUNC_SPT_GRID_LINE     = 0x00000200, ///< grid line
	KDRV_SIE_FUNC_SPT_DPC           = 0x00000400, ///< defect pixel compensation
	KDRV_SIE_FUNC_SPT_PFPC          = 0x00000800, ///< period fixed pattern compensation
	KDRV_SIE_FUNC_SPT_ECS           = 0x00001000, ///< ecs
	KDRV_SIE_FUNC_SPT_DGAIN         = 0x00002000, ///< digital gain
	KDRV_SIE_FUNC_SPT_BS_H          = 0x00004000, ///< horizontal bayer scale
	KDRV_SIE_FUNC_SPT_BS_V          = 0x00008000, ///< vertical bayer scale
	KDRV_SIE_FUNC_SPT_FLIP_H        = 0x00010000, ///< horizontal flip
	KDRV_SIE_FUNC_SPT_FLIP_V        = 0x00020000, ///< vertical flip
	KDRV_SIE_FUNC_SPT_RAWENC		= 0x00040000, ///< raw compress
	KDRV_SIE_FUNC_SPT_LA         	= 0x00080000, ///< luminance accumulation output
	KDRV_SIE_FUNC_SPT_LA_HISTO      = 0x00100000, ///< luminance accumulation histogram output
	KDRV_SIE_FUNC_SPT_CA         	= 0x00200000, ///< color accumulation output
	KDRV_SIE_FUNC_SPT_VA         	= 0x00400000, ///< variation accumulation output
	KDRV_SIE_FUNC_SPT_ETH        	= 0x00800000, ///< edgh threshold output
	KDRV_SIE_FUNC_SPT_COMPANDING    = 0x01000000, ///< companding function
	KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL = 0x02000000, ///< RGBIR format Selection
	KDRV_SIE_FUNC_SPT_RCCB_FMT_SEL  = 0x04000000, ///< RCCB format Selection
	KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL  = 0x08000000, ///< RGGB format Selection
	KDRV_SIE_FUNC_SPT_SINGLE_OUT    = 0x10000000, ///< single out
	KDRV_SIE_FUNC_SPT_RING_BUF      = 0x20000000, ///< ring buffer
	KDRV_SIE_FUNC_SPT_MD      		= 0x40000000, ///< md
	KDRV_SIE_FUNC_SPT_DVS   		= 0x80000000, ///< dvs data parser
	KDRV_SIE_FUNC_SPT_VD_DLY   		= 0x100000000LLU, ///< vd delay
} KDRV_SIE_FUNC_SUPPORT;

/**
	sie clk source select
*/
typedef enum {
	KDRV_SIE_CLKSRC_CURR = 0,	///< SIE clock source/rate keep previous setting
	KDRV_SIE_CLKSRC_480,		///< SIE clock source as 480MHz	//680/520/528
	KDRV_SIE_CLKSRC_PLL5,		///< SIE clock source as PLL5	//680/520/528
	KDRV_SIE_CLKSRC_PLL13,		///< SIE clock source as PLL13	//for IPP
	KDRV_SIE_CLKSRC_PLL12,		///< SIE clock source as PLL12	//520/528
	KDRV_SIE_CLKSRC_320,		///< SIE clock source as 320M	//680/520/528
	KDRV_SIE_CLKSRC_192,		///< SIE clock source as 192M	//680/520/528
	KDRV_SIE_CLKSRC_PLL10,		///< SIE clock source as PLL10	//for CNN
	ENUM_DUMMY4WORD(KDRV_SIE_CLKSRC_SEL)
} KDRV_SIE_CLKSRC_SEL;

typedef struct {
	UINT32 rate;
	KDRV_SIE_CLKSRC_SEL clk_src_sel;		///< SIE clock source selection
} KDRV_SIE_CLK_INFO;

/**
	sie pclk source select
*/
typedef enum {
	KDRV_SIE_PXCLKSRC_OFF,		///< SIE pixel-clock disable
	KDRV_SIE_PXCLKSRC_PAD,		///< SIE pixel-clock enabled, source as pixel-clock-pad
	KDRV_SIE_PXCLKSRC_PAD_AHD,	///< SIE pixel-clock enabled, source as AHD_PAD
	KDRV_SIE_PXCLKSRC_MCLK,		///< SIE pixel-clock enabled, source as MCLK (SIE_PXCLK from SIE_MCLK, SIE2_PXCLK from SIE_MCLK2, SIE3_PXCLK from SIE_MCLK2, SIE4_PXCLK from SIE_MCLK2)
								///< Usually for Pattern Gen Only
	KDRV_SIE_PXCLKSRC_VX1_1X,	///< SIE pixel-clock enabled, source as Vx1 1x clock (only for SIE2/3)
	KDRV_SIE_PXCLKSRC_VX1_2X,	///< SIE pixel-clock enabled, source as Vx1 2x clock (only for SIE2/3)
	ENUM_DUMMY4WORD(KDRV_SIE_PXCLKSRC_SEL)
} KDRV_SIE_PXCLKSRC_SEL;

/**
    SIE M-Clock Source Selection

    Structure of M-Clock source SIE configuration.
*/
//@{
typedef enum {
	KDRV_SIE_MCLKSRC_CURR = 0, ///< SIE M-clock source as current setting(no setting, no changing)
	KDRV_SIE_MCLKSRC_480,      ///< SIE M-clock source as 480MHz
	KDRV_SIE_MCLKSRC_320,      ///< SIE M-clock source as 320MHz
	KDRV_SIE_MCLKSRC_PLL5,     ///< SIE M-clock source as PLL5
	KDRV_SIE_MCLKSRC_PLL12,    ///< SIE M-clock source as PLL12
} KDRV_SIE_MCLKSRC_SEL;

typedef enum {
	KDRV_SIE_MCLK = 0,
	KDRV_SIE_MCLK2,
	KDRV_SIE_MCLK3,
} KDRV_SIE_MCLK_ID;
/**
	type for KDRV_SIE_MCLK
*/
typedef struct {
	BOOL				clk_en;
	KDRV_SIE_MCLKSRC_SEL mclk_src_sel;
	UINT32				clk_rate;
	KDRV_SIE_MCLK_ID    mclk_id_sel;
} KDRV_SIE_MCLK_INFO;

typedef struct {
	KDRV_SIE_MCLK_INFO		mclk_info;
	KDRV_SIE_CLK_INFO		clk_info;
	KDRV_SIE_PXCLKSRC_SEL	pxclk_info;
} KDRV_SIE_CLK_HDL;

//@}

/**
	YUV channel index
*/
//@{
typedef enum _KDRV_SIE_YUV_CH{
	KDRV_SIE_YUV_Y = 0,	///< process channel Y
	KDRV_SIE_YUV_U,		///< process channel U
	KDRV_SIE_YUV_V,		///< process channel V
	KDRV_SIE_YUV_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_SIE_YUV_CH)
} KDRV_SIE_YUV_CH;

/**
	RGB channel index
*/
//@{
typedef enum {
	KDRV_SIE_RGB_R = 0,		///< process channel R
	KDRV_SIE_RGB_G,			///< process channel G
	KDRV_SIE_RGB_B,			///< process channel B
	KDRV_SIE_RGB_MAX_CH,
	ENUM_DUMMY4WORD(KDRV_SIE_RGB_CH)
} KDRV_SIE_RGB_CH;

/**
	IPL flip information
*/
typedef enum _KDRV_SIE_FLIP {
	KDRV_SIE_FLIP_NONE = 	0x00000000,		 ///< no flip
	KDRV_SIE_FLIP_H	= 		0x00000001,		 ///< H flip
	KDRV_SIE_FLIP_V	= 		0x00000002,		 ///< V flip
	KDRV_SIE_FLIP_H_V  = 	0x00000003,		 ///<H & V flip(Flip_H | Flip_V) don't modify
	KDRV_SIE_FLIP_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_FLIP)
} KDRV_SIE_FLIP;

/**
	IPL RAW Image start pixel
*/
typedef enum _KDRV_SIE_PIX {
	// RGB start pixel
	KDRV_SIE_RGGB_PIX_R = 0,	///< start pixel R
	KDRV_SIE_RGGB_PIX_GR,		///< start pixel Gr
	KDRV_SIE_RGGB_PIX_GB,		///< start pixel Gb
	KDRV_SIE_RGGB_PIX_B,		///< start pixel B
	KDRV_SIE_RGGB_PIX_MAX,

	// RGBIR data format
	KDRV_SIE_RGBIR_PIX_RG_GI,
	KDRV_SIE_RGBIR_PIX_GB_IG,
	KDRV_SIE_RGBIR_PIX_GI_BG,
	KDRV_SIE_RGBIR_PIX_IG_GR,
	KDRV_SIE_RGBIR_PIX_BG_GI,
	KDRV_SIE_RGBIR_PIX_GR_IG,
	KDRV_SIE_RGBIR_PIX_GI_RG,
	KDRV_SIE_RGBIR_PIX_IG_GB,
	KDRV_SIE_RGBIR_PIX_MAX,

	// RCCB start pixel
	KDRV_SIE_RCCB_PIX_RC,	///< start pixel RC
	KDRV_SIE_RCCB_PIX_CR,	///< start pixel CR
	KDRV_SIE_RCCB_PIX_CB,	///< start pixel CB
	KDRV_SIE_RCCB_PIX_BC,	///< start pixel BC
	KDRV_SIE_RCCB_PIX_MAX,

	ENUM_DUMMY4WORD(KDRV_SIE_PIX)
} KDRV_SIE_PIX;


/**
	IPL RAW Image bits
*/
typedef enum _KDRV_SIE_RAW_BIT {
	KDRV_SIE_RAW_BIT_8 = 8,		///< 8 bits
	KDRV_SIE_RAW_BIT_10 = 10,	///< 10 bits
	KDRV_SIE_RAW_BIT_12 = 12,	///< 12 bits
	KDRV_SIE_RAW_BIT_16 = 16,	///< 16 bits
	KDRV_SIE_RAW_BIT_MAX_CNT,
	ENUM_DUMMY4WORD(KDRV_SIE_RAW_BIT)
} KDRV_SIE_RAW_BIT;

/**
	IPL YUV Image type
*/
typedef enum {
	KDRV_SIE_YUV444 = 0,	///< 3 channel, yuv 444 planar
	KDRV_SIE_YUV422,		///< 3 channel, yuv 422 planar
	KDRV_SIE_YUV420,		///< 3 channel, yuv 420 planar
	KDRV_SIE_Y_PACK_UV444,	///< 2 channel, y planar uv pack, 444 format
	KDRV_SIE_Y_PACK_UV422,	///< 2 channel, y planar uv pack, 422 format
	KDRV_SIE_Y_PACK_UV420,	///< 2 channel, y planar uv pack, 420 format
	KDRV_SIE_PACK_YUV444,	///< 1 channel, yuv pack, 444 format
	KDRV_SIE_Y_ONLY,		///< 1 channel, y only
	ENUM_DUMMY4WORD(KDRV_SIE_YUV_TYPE)
} KDRV_SIE_YUV_TYPE;

/**
	IPL process ID
*/
typedef enum _KDRV_SIE_PROC_ID {
	KDRV_SIE_ID_1 = 0,		///< process id 1
	KDRV_SIE_ID_2,			///< process id 2
	KDRV_SIE_ID_3,			///< process id 3
	KDRV_SIE_ID_4,			///< process id 4,
	KDRV_SIE_ID_5,			///< process id 5,
	KDRV_SIE_ID_6,			///< process id 6,
	KDRV_SIE_ID_7,			///< process id 7,
	KDRV_SIE_ID_8,			///< process id 8,
	KDRV_SIE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(KDRV_SIE_PROC_ID)
} KDRV_SIE_PROC_ID;


/**
	sie signal receive mode
*/
typedef enum {
	KDRV_SIE_IN_PARA_MSTR_SNR = 0,	///< Parallel Master Sensor
	KDRV_SIE_IN_PARA_SLAV_SNR,		///< Parallel Slave Sensor
	KDRV_SIE_IN_PATGEN,				///< Self Pattern-Generator
	KDRV_SIE_IN_VX1_IF0_SNR,		///< Vx1 Sensor
	KDRV_SIE_IN_CSI_1,				///< Serial Sensor from CSI-1
	KDRV_SIE_IN_CSI_2,				///< Serial Sensor from CSI-2
	KDRV_SIE_IN_CSI_3,				///< Serial Sensor from CSI-3
	KDRV_SIE_IN_CSI_4,				///< Serial Sensor from CSI-4
	KDRV_SIE_IN_CSI_5,				///< Serial Sensor from CSI-5
	KDRV_SIE_IN_CSI_6,				///< Serial Sensor from CSI-6
	KDRV_SIE_IN_CSI_7,				///< Serial Sensor from CSI-7
	KDRV_SIE_IN_CSI_8,				///< Serial Sensor from CSI-8
	KDRV_SIE_IN_AHD,
	KDRV_SIE_IN_SLVS_EC,			///< Serial Sensor from SLVS-EC
	KDRV_SIE_IN_VX1_IF1_SNR,		///< Vx1 Sensor, interface 1
	ENUM_DUMMY4WORD(KDRV_SIE_ACT_MODE)
} KDRV_SIE_ACT_MODE;

/**
	sie kdrv interrupt type
*/
typedef enum {
	KDRV_SIE_INT_CLR			= 0,
	KDRV_SIE_INT_VD				= 0x00000001,	///< interrupt: VD
	KDRV_SIE_INT_BP1			= 0x00000002,	///< interrupt: Break point 1
	KDRV_SIE_INT_BP2			= 0x00000004,	///< interrupt: Break point 2
	KDRV_SIE_INT_BP3			= 0x00000008,	///< interrupt: Break point 3
	KDRV_SIE_INT_ACTST			= 0x00000010,	///< interrupt: Active window start
	KDRV_SIE_INT_CRPST			= 0x00000020,	///< interrupt: Crop window start
	KDRV_SIE_INT_DRAM_OUT0_END	= 0x00000080,	///< interrupt: Dram output channel 0 end
	KDRV_SIE_INT_DRAM_OUT1_END	= 0x00000100,	///< interrupt: Dram output channel 1 end
	KDRV_SIE_INT_DRAM_OUT2_END	= 0x00000200,	///< interrupt: Dram output channel 2 end
	KDRV_SIE_INT_DRAM_OUT3_END	= 0x00000400,	///< interrupt: Dram output channel 3 end
	KDRV_SIE_INT_DRAM_OUT4_END	= 0x00000800,	///< interrupt: Dram output channel 4 end
	KDRV_SIE_INT_DRAM_OUT5_END	= 0x00001000,	///< interrupt: Dram output channel 5 end
	KDRV_SIE_INT_ACTEND			= 0x00010000,	///< interrupt: Raw active end
	KDRV_SIE_INT_CROPEND		= 0x00020000,	///< interrupt: Raw crop end
	KDRV_SIE_INT_ALL			= 0xffffffff,	///< interrupt: all
} KDRV_SIE_INT;

/**
	struct for kdrv_sie_open
*/
typedef struct {
	KDRV_SIE_ACT_MODE act_mode;			///< SIE active mode, based on sensor type
	KDRV_SIE_CLKSRC_SEL clk_src_sel;	///< SIE clock source selection
	KDRV_SIE_PXCLKSRC_SEL pclk_src_sel;	///< SIE pixel clk source selection
	UINT32 data_rate;					///< Sensor output data rate(bytes per seconds)
} KDRV_SIE_OPENCFG;

/**
	sie kdrv trig type, start/stop sie
*/
typedef enum {
	KDRV_SIE_TRIG_STOP = 0,	///< Stop sie
	KDRV_SIE_TRIG_START,	///< Start sie
	ENUM_DUMMY4WORD(KDRV_SIE_TRIG_TYPE)
} KDRV_SIE_TRIG_TYPE;

/**
	struct for kdrv_sie_open
*/
typedef struct {
	KDRV_SIE_TRIG_TYPE trig_type;	///< trigger operation
	BOOL wait_end;					///< wait trigger end
} KDRV_SIE_TRIG_INFO;

/**
	struct for kdrv_sie_bp
*/
typedef struct {
	UINT32 bp1;
	UINT32 bp2;
	UINT32 bp3;
} KDRV_SIE_BP_INFO;

/**
	struct for kdrv_sie_crp and kdrv_sie_act
*/
typedef struct {
	URECT win;
	KDRV_SIE_PIX cfa_pat;	///< raw cfa start pixel for active and crop window
} KDRV_SIE_ACT_CRP_WIN;

/*
	Note: KDRV_SIE_CH3~CH5 are not support in 520/528
*/
typedef enum  {
	KDRV_SIE_CH0 = 0,
	KDRV_SIE_CH1,
	KDRV_SIE_CH2,
	KDRV_SIE_CH_MAX,
	KDRV_SIE_CH_ALL = 0xff,
	ENUM_DUMMY4WORD(KDRV_SIE_CH_ID)
} KDRV_SIE_CH_ID;

/**
	callback function prototype
	UINT32 id
	UINT32 msg_type
	void* in_data
	void* out_data
*/
typedef INT32 (*KDRV_SIE_ISRCB)(UINT32, UINT32, void*, void*);

/**
	sie channel 0 output destination
*/
typedef enum {
	KDRV_SIE_OUT_DEST_DIRECT = 0,	///< ch0 output direct to ipl_in
	KDRV_SIE_OUT_DEST_DRAM,			///< ch0 output to dram
	KDRV_SIE_OUT_DEST_BOTH,			///< ch0 output to both ipl_in and dram
	KDRV_SIE_OUT_DEST_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_OUT_DEST)
} KDRV_SIE_OUT_DEST;

/**
	type for KDRV_SIE_DATA_FMT
*/
typedef enum {
	/** Bayer
		bayer input is automatically handle by sie
		select output packbud, 8/10/12/16 decide sie_ch0 output bit

		When raw_encode enable, need to select 12bit
		Compress rate is 70%, lofs should be width*12/8*(0.7)
	*/
	KDRV_SIE_BAYER_8,
	KDRV_SIE_BAYER_10,
	KDRV_SIE_BAYER_12,
	KDRV_SIE_BAYER_16,
	KDRV_SIE_BAYER_16_MSB = KDRV_SIE_BAYER_16,
	KDRV_SIE_BAYER_16_LSB,

	/** CCIR
		select ccir input format
		sie always output y to ch0, uv to ch1
	*/
	KDRV_SIE_YUV_422_SPT,
	KDRV_SIE_YUV_422_NOSPT,
	KDRV_SIE_YUV_420_SPT,

	/*
		Y ONLY
		sie output y to ch0
	*/
	KDRV_SIE_Y_8,

	KDRV_SIE_DATA_FMT_MAX,

} KDRV_SIE_DATA_FMT;

/**
	type for KDRV_SIE_SET_YUV_ORDER
*/
typedef enum {
	KDRV_SIE_YUYV = 0,
	KDRV_SIE_YVYU,
	KDRV_SIE_UYVY,
	KDRV_SIE_VYUY,
	ENUM_DUMMY4WORD(KDRV_SIE_YUV_ORDER)
} KDRV_SIE_YUV_ORDER;


typedef enum {
	KDRV_DVI_FORMAT_CCIR601  = 0,    ///< CCIR 601
	KDRV_DVI_FORMAT_CCIR656_EAV,     ///< CCIR 656 EAV
	KDRV_DVI_FORMAT_CCIR656_ACT,     ///< CCIR 656 ACT
	ENUM_DUMMY4WORD(KDRV_SIE_DVI_FORMAT_SEL)
} KDRV_SIE_DVI_FORMAT_SEL;

typedef enum {
	KDRV_DVI_MODE_SD  = 0,   ///< SD mode (8 bits)
	KDRV_DVI_MODE_HD,        ///< HD mode (16bits)
	KDRV_DVI_MODE_HD_INV,    ///< HD mode (16bits) with Byte Inverse
	ENUM_DUMMY4WORD(KDRV_SIE_DVI_IN_MODE_SEL)
} KDRV_SIE_DVI_IN_MODE_SEL;

/**
	type for KDRV_SIE_SET_SIGNAL
*/
typedef enum {
	KDRV_SIE_PHASE_RISING = 0,
	KDRV_SIE_PHASE_FALLING,
	ENUM_DUMMY4WORD(KDRV_SIE_SIGNAL_PHASE)
} KDRV_SIE_SIGNAL_PHASE;

typedef struct {
	KDRV_SIE_SIGNAL_PHASE vd_phase;
	KDRV_SIE_SIGNAL_PHASE hd_phase;
	KDRV_SIE_SIGNAL_PHASE data_phase;
	BOOL vd_inverse;
	BOOL hd_inverse;
} KDRV_SIE_SIGNAL;

typedef struct {
	KDRV_SIE_CH_ID ch_id;
	UINT32 lofs;
} KDRV_SIE_OUT_LOFS;

typedef struct {
	KDRV_SIE_CH_ID ch_id;
	UINT32 addr;
} KDRV_SIE_OUT_ADDR;

typedef enum {
	KDRV_SIE_PAT_COLORBAR = 1,	///< Color bar
	KDRV_SIE_PAT_RANDOM,        ///< Random
	KDRV_SIE_PAT_FIXED,         ///< Fixed
	KDRV_SIE_PAT_HINCREASE,     ///< 1D Increment
	KDRV_SIE_PAT_HVINCREASE,    ///< 2D increment
	ENUM_DUMMY4WORD(KDRV_SIE_PATGEN_SEL)
} KDRV_SIE_PATGEN_SEL;

typedef struct {
	KDRV_SIE_PATGEN_SEL	mode;
	UINT32          	val;
	USIZE				src_win;
} KDRV_SIE_PATGEN_INFO;

/**
	type for KDRV_SIE_SET_OB
*/
typedef struct {
	BOOL bypass_enable;	    ///< ob bypass enable, note that ECS/DGain will substract ob_val before function and add it back while end of the function
	UINT32 ob_ofs;			///< ob offset
} KDRV_SIE_OB_PARAM;

/**
	type for KDRV_SIE_SET_CA_ROI
*/
typedef struct {
    UINT32 roi_base;
	URECT roi;				///< ca region of interest, source is sie scale out
	                        ///< when setting, roi need be 'ratio', and roi change to 'size' for getting
} KDRV_SIE_CA_ROI;

typedef enum {
	KDRV_SIE_CA_SRC_AFTER_OB = 0,
	KDRV_SIE_CA_SRC_BEFORE_OB,
	ENUM_DUMMY4WORD(KDRV_SIE_CA_SRC_SEL)
} KDRV_SIE_CA_SRC_SEL;

/**
	type for KDRV_SIE_SET_CA

	buffer calculation:
	CA Output R/G/B/Cnt/IRth/Rth/Gth/Bth @16bit for each window
	buffer_size = win_num_w * win_num_h * 8 * (16/8) bytes
	max_buffer_size = 32 * 32 * 8 * 16/8
*/
typedef struct {
	BOOL enable;			///< ca enable, ca output use sie channel 1
	USIZE win_num;			///< ca window number, 1x1 ~ 32x32

	BOOL th_enable;			///< ca threshold enable
	UINT16 g_th_l;			///< G threshold lower bound
	UINT16 g_th_u;			///< G threshold upper bound
	UINT16 r_th_l;			///< R threshold lower bound
	UINT16 r_th_u;			///< R threshold upper bound
	UINT16 b_th_l;			///< B threshold lower bound
	UINT16 b_th_u;			///< B threshold upper bound
	UINT16 p_th_l;			///< P threshold lower bound
	UINT16 p_th_u;			///< P threshold upper bound

	UINT32 irsub_r_weight;	///< IR weight for R channel, 0~255
	UINT32 irsub_g_weight;	///< IR weight for G channel, 0~255
	UINT32 irsub_b_weight;	///< IR weight for B channel, 0~255
	UINT32 ca_ob_ofs;
	KDRV_SIE_CA_SRC_SEL ca_src;
} KDRV_SIE_CA_PARAM;
/**
	type for KDRV_SIE_GET_CA_RST
	ca result, array size should be window num_x * num_y
	user need to prepare memory for these pointer
*/
typedef struct {
	UINT16 buf_r[KDRV_SIE_CA_MAX_WINNUM*KDRV_SIE_CA_MAX_WINNUM];
	UINT16 buf_g[KDRV_SIE_CA_MAX_WINNUM*KDRV_SIE_CA_MAX_WINNUM];
	UINT16 buf_b[KDRV_SIE_CA_MAX_WINNUM*KDRV_SIE_CA_MAX_WINNUM];
	UINT16 buf_ir[KDRV_SIE_CA_MAX_WINNUM*KDRV_SIE_CA_MAX_WINNUM];
	UINT16 acc_cnt[KDRV_SIE_CA_MAX_WINNUM*KDRV_SIE_CA_MAX_WINNUM];
} KDRV_SIE_CA_RST;

/**
	type for KDRV_SIE_SET_LA

	buffer calculation:
	LA Output PreGamma Lum/PostGamma Lum @16bit for each window, Histogram 64bin@16bit
	buffer_size = win_num_w * win_num_h * 2 * (16/8) + 64 * (16/8) bytes
	max_buffer_size = (32 * 32 * 2 * 16/8) + (64 * 16/8)
*/
typedef enum {
	KDRV_SIE_LA_SRC_POST_CG = 0,
	KDRV_SIE_LA_SRC_PRE_CG,
	ENUM_DUMMY4WORD(KDRV_SIE_LA_SRC_SEL)
} KDRV_SIE_LA_SRC_SEL;

typedef enum {
	KDRV_SIE_LA_HIST_SRC_POST_GMA = 0,
	KDRV_SIE_LA_HIST_SRC_PRE_GMA,
	ENUM_DUMMY4WORD(KDRV_SIE_LA_HIST_SRC_SEL)
} KDRV_SIE_LA_HIST_SRC_SEL;

typedef struct {
	UINT32 roi_base;
	URECT roi;					///< la region of interest, source is sie ca crop window
	                            ///< when setting, roi need be 'ratio', and roi change to 'size' for getting
} KDRV_SIE_LA_ROI;


typedef enum {
	STCS_LA_RGB     = 0,        ///< use RGB channel, transfer to Y
	STCS_LA_G,                  ///< use G channel as Y
	ENUM_DUMMY4WORD(KDRV_SIE_LA_RGB2Y_MOD_SEL)
} KDRV_SIE_LA_RGB2Y_MOD_SEL;

typedef struct {
	BOOL enable;				///< la enable, la output use sie channel 2
	USIZE win_num;				///< la window number, 1x1 ~ 32x32
	KDRV_SIE_LA_SRC_SEL la_src;	///< la source selection
	KDRV_SIE_LA_RGB2Y_MOD_SEL la_rgb2y1mod;
	KDRV_SIE_LA_RGB2Y_MOD_SEL la_rgb2y2mod;

	BOOL cg_enable;				///< la color gain enable, in 3.7 bits format
	UINT16 r_gain;
	UINT16 g_gain;
	UINT16 b_gain;

	BOOL gamma_enable;			///< la gamma enable
	UINT32 gamma_tbl_addr;		///< la gamma table, size 65

	BOOL histogram_enable;					///< la histogram enable
	KDRV_SIE_LA_HIST_SRC_SEL histogram_src;	///< la histogram src, TRUE --> data before gamma, FALSE --> data after gamma
	UINT32 irsub_r_weight;	///< IR weight for R channel, 0~255
	UINT32 irsub_g_weight;	///< IR weight for G channel, 0~255
	UINT32 irsub_b_weight;	///< IR weight for B channel, 0~255
	UINT32 la_ob_ofs;
	BOOL lath_enable;
	UINT8  lathy1lower;
	UINT8  lathy1upper;
	UINT8  lathy2lower;
	UINT8  lathy2upper;
} KDRV_SIE_LA_PARAM;

/**
	type for KDRV_SIE_GET_LA_RST
	la result, array size should be window num_x * num_y
	histogram array size = 64
	user need to prepare memory for these pointer
*/
typedef struct {
	UINT16 buf_lum_1[KDRV_SIE_LA_MAX_WINNUM*KDRV_SIE_LA_MAX_WINNUM];    // pre gamma result
	UINT16 buf_lum_2[KDRV_SIE_LA_MAX_WINNUM*KDRV_SIE_LA_MAX_WINNUM];    // post-gamma result
	UINT16 buf_histogram[KDRV_SIE_LA_HIST_BIN];					  		// Only SIE 1~4
} KDRV_SIE_LA_RST;

typedef struct {
	BOOL lum_sel; /// 0: y1, 1: y2(post-gamma)
	UINT16 lum[KDRV_SIE_LA_MAX_WINNUM*KDRV_SIE_LA_MAX_WINNUM];        // pre gamma result
} KDRV_SIE_LA_ACCM;

/**
	type for KDRV_SIE_GET_MD_RST
	md_th_rslt, array size should be 32x32x8bit
	user need to prepare memory for these pointer
*/
typedef struct {
	UINT8       *md_th_rslt;
	UINT16      blk_dif_cnt;
	UINT32      total_blk_diff;
} KDRV_SIE_MD_RST;

/**
	type for KDRV_SIE_SET_CCIR
*/
typedef struct {
	KDRV_SIE_YUV_ORDER yuv_order;
	KDRV_SIE_DVI_FORMAT_SEL fmt;
	KDRV_SIE_DVI_IN_MODE_SEL dvi_mode;
	BOOL filed_enable;				///< for CCIR interlaced
	BOOL filed_sel;					///< select data for CCIR interlaced
	BOOL ccir656_vd_sel;			///< 656 vd mode, 0 --> interlaced(field change as VD), 1 --> progressive(SAV + V Blank status)
	BOOL auto_align;				///< CCIR656 mux sensor data auto alignment
	UINT8 data_period;				///< for mux sensor, 0 --> 1 input(YUYV), 1 --> 2 input(YYUUYYVV)
	UINT8 data_idx;					///< select data idx 0/1 when mux sensor input
} KDRV_SIE_CCIR_INFO;

/**
	KDRV_SIE_SET_ENCODE

	Only control Enable/Disable
	Enable will cause ch5 output sideinfo, and Channel lineoffset can set to 70%
	note that encode can only enable when data_fmt = KDRV_SIE_BAYER_12

	Channel 5 Setting
	lineoffset = align_ceil_32(scl_size.w) * 16/8
	buffer_size = lineoffset * scl_size.h
*/

typedef enum {
	KDRV_SIE_ENC_50 = 0,
	KDRV_SIE_ENC_58,
	KDRV_SIE_ENC_66,
	ENUM_DUMMY4WORD(KDRV_SIE_ENC_RATE_SEL)
} KDRV_SIE_ENC_RATE_SEL;

typedef struct {
    BOOL enable;
	KDRV_SIE_ENC_RATE_SEL enc_rate;
} KDRV_SIE_RAW_ENCODE;
/**
	KDRV_SIE_SET_DGAIN

	digital gain in 3.7 bits format
*/

typedef struct {
	BOOL enable;
	UINT32 gain;
} KDRV_SIE_DGAIN;

/**
	KDRV_SIE_SET_CGAIN
*/
typedef struct {
	BOOL enable;
	BOOL sel_37_fmt;	// 0 --> 2.8 bit gain format, 1 --> 3.7 bit gain format
	UINT16 r_gain; 		// r/gr/gb/b/Ir
	UINT16 gr_gain;
	UINT16 gb_gain;
	UINT16 b_gain;
	UINT16 ir_gain;
	//UINT16 gr_gain;
	//UINT16 gb_gain;
	//UINT16 b_gain;
} KDRV_SIE_CGAIN;

/**
	KDRV_SIE_SET_DPC
*/
typedef enum {
	KDRV_SIE_50F00_PERCENT = 0,		///< cross-channel weighting = 50% for defect concealment
	KDRV_SIE_25F00_PERCENT,			///< cross-channel weighting = 25% for defect concealment
	KDRV_SIE_12F50_PERCENT,			///< cross-channel weighting = 12.5% for defect concealment
	KDRV_SIE_6F25_PERCENT,			///< cross-channel weighting = 6.25% for defect concealment
	ENUM_DUMMY4WORD(KDRV_SIE_DPC_WEIGTH)
} KDRV_SIE_DPC_WEIGTH;

typedef struct {
	BOOL enable;							///< dpc function enable
	UINT32 tbl_addr;	///< dpc table addr
	KDRV_SIE_DPC_WEIGTH weight;				///< dpc weigth select
	UINT32 dp_total_size;					//defect pixel total size
} KDRV_SIE_DPC;

/**
	KDRV_SIE_SET_ECS
*/
typedef enum {
	KDRV_SIE_ECS_MAP_65x65 = 0,
	KDRV_SIE_ECS_MAP_49x49,
	KDRV_SIE_ECS_MAP_33x33,
	ENUM_DUMMY4WORD(KDRV_SIE_ECS_MAP_SEL)
} KDRV_SIE_ECS_MAP_SEL;

/**
	KDRV_SIE_SET_ECS
*/
typedef enum {
	KDRV_SIE_ECS_3CH_10B = 0,
	KDRV_SIE_ECS_4CH_8B,
	ENUM_DUMMY4WORD(KDRV_SIE_ECS_BAYER_MODE_SEL)
} KDRV_SIE_ECS_BAYER_MODE_SEL;

typedef struct {
	BOOL enable;
	BOOL sel_37_fmt;
	UINT32 map_tbl_addr;
	KDRV_SIE_ECS_MAP_SEL map_sel;

	BOOL dthr_enable;
	BOOL dthr_reset;
	UINT32 dthr_level;	///< dithering level; 0x0=bit[1:0], 0x7=bit[8:7]
	KDRV_SIE_ECS_BAYER_MODE_SEL bayer_mode;
} KDRV_SIE_ECS;

typedef struct {
	UINT32 decomp_kneepts[KDRV_SIE_COMPANDING_MAX_LEN-1];
	UINT32 decomp_strpts[KDRV_SIE_COMPANDING_MAX_LEN];
	UINT32 decomp_shiftpts[KDRV_SIE_COMPANDING_MAX_LEN];
} KDRV_SIE_DECOMPANDING_INFO;

typedef struct {
	UINT32 comp_kneepts[KDRV_SIE_COMPANDING_MAX_LEN-1];
	UINT32 comp_strpts[KDRV_SIE_COMPANDING_MAX_LEN];
	UINT32 comp_shiftpts[KDRV_SIE_COMPANDING_MAX_LEN];
} KDRV_SIE_COMPANDING_INFO;

typedef struct {
	BOOL enable;
	KDRV_SIE_DECOMPANDING_INFO	decomp_info;
	KDRV_SIE_COMPANDING_INFO	comp_info;
	UINT16 comp_shift;
} KDRV_SIE_COMPANDING;

typedef struct {
	BOOL ch0singleout_enable;
	BOOL ch1singleout_enable;
	BOOL ch2singleout_enable;
} KDRV_SIE_SINGLE_OUT_CTRL;

typedef enum {
	KDRV_SIE_NORMAL_OUT = 0,
	KDRV_SIE_SINGLE_OUT,
	ENUM_DUMMY4WORD(KDRV_SIE_OUTPUT_MODE_TYPE)
} KDRV_SIE_OUTPUT_MODE_TYPE;

typedef struct {
	KDRV_SIE_OUTPUT_MODE_TYPE  out0_mode; ///< out0 mode sel
	KDRV_SIE_OUTPUT_MODE_TYPE  out1_mode; ///< out1 mode sel
	KDRV_SIE_OUTPUT_MODE_TYPE  out2_mode; ///< out2 mode sel
} KDRV_SIE_DRAM_OUT_CTRL;

typedef struct {
	BOOL enable;
	UINT32 ring_buf_len;
} KDRV_SIE_RINGBUF_INFO;

typedef enum {
	KDRV_SIE_MD_MASK_COL_ROW = 0,
	KDRV_SIE_MD_MASK_8X8,
	ENUM_DUMMY4WORD(KDRV_SIE_MD_MASK_MODE)
} KDRV_SIE_MD_MASK_MODE;

typedef enum {
	KDRV_SIE_MD_SRC_PREGAMMA= 0,
	KDRV_SIE_MD_SRC_POSTGAMMA,
	ENUM_DUMMY4WORD(KDRV_SIE_MD_SRC)
} KDRV_SIE_MD_SRC;

typedef struct {
	BOOL enable;
	KDRV_SIE_MD_SRC md_src;
	UINT8 sum_frms;	//range: 0~15 (1~16 frames)
	KDRV_SIE_MD_MASK_MODE mask_mode;
	UINT32 mask0;
	UINT32 mask1;
	UINT32 blkdiff_thr;
	UINT32 total_blkdiff_thr; //clamp when > 32bit
	UINT16 blkdiff_cnt_thr;
} KDRV_SIE_MD;

typedef struct {
	UINT32 VdIntervLowerThr;
	UINT32 VdIntervUpperThr;
	UINT32 HdIntervLowerThr;
	UINT32 HdIntervUpperThr;
} KDRV_SIE_VDHD_INTERVAL_INFO;

typedef struct {
	UINT32 kdrv_sie_ir_level;	///< Legal range : 0~255
	UINT32 kdrv_sie_ir_sat;     ///< Legal range : 0~1023
} KDRV_SIE_RGBIR_INFO;

typedef enum {
	KDRV_SIE_SPT_BAYER_8         = (1 << KDRV_SIE_BAYER_8),
	KDRV_SIE_SPT_BAYER_10        = (1 << KDRV_SIE_BAYER_10),
	KDRV_SIE_SPT_BAYER_12        = (1 << KDRV_SIE_BAYER_12),
	KDRV_SIE_SPT_BAYER_16        = (1 << KDRV_SIE_BAYER_16),
	KDRV_SIE_SPT_BAYER_16_MSB    = (1 << KDRV_SIE_BAYER_16_MSB),
	KDRV_SIE_SPT_BAYER_16_LSB    = (1 << KDRV_SIE_BAYER_16_LSB),
	KDRV_SIE_SPT_YUV_422_SPT     = (1 << KDRV_SIE_YUV_422_SPT),
	KDRV_SIE_SPT_YUV_422_NOSPT   = (1 << KDRV_SIE_YUV_422_NOSPT),
	KDRV_SIE_SPT_YUV_420_SPT     = (1 << KDRV_SIE_YUV_420_SPT),
	KDRV_SIE_SPT_Y_8             = (1 << KDRV_SIE_Y_8),
	ENUM_DUMMY4WORD(KDRV_SIE_DATAFORMAT_SPT)
} KDRV_SIE_DATAFORMAT_SPT;

/**
	SIE limitation infor
	sync to CTL_SIE_LIMIT
*/
typedef struct {
	UINT32 max_clk_rate;		//maximum sie clock rate
	UINT32 max_spt_id;			//maximum sie id
	UINT32 out_max_ch;			//output maximum channel number
	USIZE ca_win_max_num;		//ca window maximum number, proposal[CA_WIN_NUMX & CA_WIN_NUMY]
	USIZE la_win_max_num;		//la window maximum number, proposal[LA_WIN_NUMX & LA_WIN_NUMY]
	USIZE yout_win_max_num;		//y out window maximum number
	UINT32 la_hist_bin_num;		//la histogram maximum bin number, proposal[HISTO_Y_RSLT_0~63]
	USIZE pat_gen_src_win_align; // for src width/height, proposal[SRC_WIDTH & SRC_HEIGHT]
	URECT act_win_align;		// active window limit
	USIZE act_win_min;
	USIZE act_win_max;
	URECT crp_win_align;		// crop window limit
	USIZE crp_win_min;
	USIZE crp_win_max;
	USIZE scale_in_align;		// scale input limit
	USIZE scale_in_max;
	USIZE scale_out_align;		// scale output limit
	USIZE scale_out_min;
	USIZE scale_out_max;
	USIZE scale_out_ratio_max;
	USIZE ca_crp_win_align;		//ca crop window size w/h size align, proposal[CA_CROP_SZX & CA_CROP_SZY]
	USIZE la_crp_win_align;		//la crop window size w/h size align, proposal[LA_CROP_SZX & LA_CROP_SZY]
	UINT32 out_lofs_align[KDRV_SIE_CH_MAX];	//output lineoofset align, proposal[DRAM_OUT0_OFSO & DRAM_OUT1_OFSO & DRAM_OUT2_OFSO]
	UINT64 support_func;		//reference to KDRV_SIE_FUNC_SUPPORT
	UINT32 ring_buf_len_max;	//ring buffer length maximum, proposal[DRAM_OUT0_RINGBUF_LEN]
	UINT32 support_output_fmt;	// KDRV_SIE_DATAFORMAT_SPT
} KDRV_SIE_LIMIT;

/**
    KDRV SIE all IQ function enable/disable operation for IPL
*/
typedef struct _KDRV_SIE_IPL_FUNC_EN_ {
	UINT32 ipl_ctrl_func;   ///<It labels which function will be controlled by KFLOW
	BOOL   enable;          ///
} KDRV_SIE_IPL_FUNC_EN;

/**
	type for KDRV_SIE_DMA_STS
*/
typedef enum {
	KDRV_SIE_DMA_STS_IDLE = 0,
	KDRV_SIE_DMA_STS_RUN = 1,
	ENUM_DUMMY4WORD(KDRV_SIE_DMA_STS)
} KDRV_SIE_DMA_STS;

/**
	type for KDRV_SIE_DVS_CODE
*/
typedef struct {
	UINT8 positive;
	UINT8 negative;
	UINT8 nochange;
} KDRV_SIE_DVS_CODE;

/**
	type for KDRV_SIE_VD_DLY
*/
typedef enum {
	KDRV_SIE_VD_DLY_NONE,
	KDRV_SIE_VD_DLY_AUTO,

	ENUM_DUMMY4WORD(KDRV_SIE_VD_DLY)
} KDRV_SIE_VD_DLY;

/**
	KDRV_SIE_SYS_INFO
	vd_cnt_clk / vd_cnt_clk_base = vd time (s)
	hd_cnt_clk / hd_cnt_clk_base = hd time (s)
*/
typedef struct {
	/* vd */
	UINT32 vd_cnt_clk_base;		// units : Hz
	UINT32 vd_cnt_clk;			// units : vd_cnt_clk_base (apb clk) [0x1e0]
	UINT32 vd_cnt_pxl;			// units : pixel [0x368]
	UINT32 vd_pxl_max;			// units : pixel [0x10,bit[13..0]]
	/* hd */
	UINT32 hd_cnt_clk_base;		// units : Hz
	UINT32 hd_cnt_clk;			// units : hd_cnt_clk_base (apb clk) [0x1e4]
	UINT32 hd_cnt_line;			// units : line [0x364,bit[15..0]]
	UINT32 hd_line_end;			// units : line [0x364,bit[31..16]]
} KDRV_SIE_SYS_INFO;

/**
	data for kdrv_sie Set/Get Item
*/
typedef enum {
	KDRV_SIE_ITEM_OPENCFG = 0,	///< data_type: [Set]     KDRV_SIE_OPENCFG, it can be used before kdrv_sie_open
	KDRV_SIE_ITEM_ISRCB,    	///< data_type: [Set]     KDRV_SIE_ISRCB,[Get]N.S.
	KDRV_SIE_ITEM_MCLK,			///< data_type: [Set/Get] KDRV_SIE_MCLK_INFO, it can be used before kdrv_sie_open
	KDRV_SIE_ITEM_PXCLK,        ///< data_type: [Set/Get] KDRV_SIE_PXCLKSRC_SEL
	KDRV_SIE_ITEM_SIECLK,       ///< data_type: [Set/Get] KDRV_SIE_CLK_INFO, it can be used before kdrv_sie_open
	KDRV_SIE_ITEM_ACT_WIN,		///< data_type: [Set/Get] URECT
	KDRV_SIE_ITEM_CROP_WIN,		///< data_type: [Set/Get] URECT
	KDRV_SIE_ITEM_OUT_DEST,		///< data_type: [Set/Get] KDRV_SIE_OUT_DEST
	KDRV_SIE_ITEM_DATA_FMT,		///< data_type: [Set/Get] KDRV_SIE_DATA_FMT
	KDRV_SIE_ITEM_YUV_ORDER,	///< data_type: [Set/Get] KDRV_SIE_YUV_ORDER
	KDRV_SIE_ITEM_SIGNAL,		///< data_type: [Set/Get] KDRV_SIE_SIGNAL
	KDRV_SIE_ITEM_FLIP,			///< data_type: [Set/Get] KDRV_SIE_FLIP
	KDRV_SIE_ITEM_INTE,			///< data_type: [Set/Get] KDRV_SIE_INTE
	KDRV_SIE_ITEM_CH0_LOF,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_CH1_LOF,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_CH2_LOF,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_CH0_ADDR,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_CH1_ADDR,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_CH2_ADDR,		///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_PATGEN_INFO,	///< data_type: [Set/Get] KDRV_SIE_PATGEN_INFO
	KDRV_SIE_ITEM_SCALEOUT,		///< data_type: [Set/Get] USIZE
	KDRV_SIE_ITEM_OB,			///< data_type: [Set/Get] KDRV_SIE_OB_PARAM
	KDRV_SIE_ITEM_CA,			///< data_type: [Set/Get] KDRV_SIE_CA_PARAM
	KDRV_SIE_ITEM_CA_ROI,		///< data_type: [Set/Get] KDRV_SIE_CA_ROI
	KDRV_SIE_ITEM_CA_RSLT,		///< data_type:     [Get] KDRV_SIE_CA_RST
	KDRV_SIE_ITEM_LA,			///< data_type: [Set/Get] KDRV_SIE_LA_PARAM
	KDRV_SIE_ITEM_LA_ROI,	    ///< data_type: [Set/Get] KDRV_SIE_LA_ROI
	KDRV_SIE_ITEM_LA_RSLT,		///< data_type:     [Get] KDRV_SIE_LA_RST
	KDRV_SIE_ITEM_LA_ACCM,		///< data_type:     [Get] KDRV_SIE_LA_ACCM
	KDRV_SIE_ITEM_ENCODE,		///< data_type: [Set/Get] KDRV_SIE_RAW_ENCODE
	KDRV_SIE_ITEM_DGAIN,		///< data_type: [Set/Get] KDRV_SIE_DGAIN
	KDRV_SIE_ITEM_CGAIN,		///< data_type: [Set/Get] KDRV_SIE_CGAIN
	KDRV_SIE_ITEM_DPC,			///< data_type: [Set/Get] KDRV_SIE_DPC
	KDRV_SIE_ITEM_ECS,			///< data_type: [Set/Get] KDRV_SIE_ECS
	KDRV_SIE_ITEM_CCIR,			///< data_type: [Set/Get] KDRV_SIE_CCIR_INFO
	KDRV_SIE_ITEM_LOAD,			///< data_type: [Set]     NULL
	KDRV_SIE_ITEM_DMA_OUT_EN,	///< data_type: [Set/Get] BOOL
	KDRV_SIE_ITEM_BP1,          ///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_BP2,          ///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_BP3,          ///< data_type: [Set/Get] UINT32
	KDRV_SIE_ITEM_LIMIT,     	///< data_type:     [Get] KDRV_SIE_LIMIT, return sie size align limitation, it can be used before kdrv_sie_open
	KDRV_SIE_ITEM_COMPANDING,   ///< data_type: [Set/Get] KDRV_SIE_COMPANDING
	KDRV_SIE_ITEM_SINGLEOUT,    ///< data_type: [Set/Get] KDRV_SIE_SINGLE_OUT_CTRL, kdrv will return current status of single-out enable(getting from register)
	KDRV_SIE_ITEM_OUTPUT_MODE,  ///< data_type: [Set/Get] KDRV_SIE_DRAM_OUT_CTRL
	KDRV_SIE_ITEM_RING_BUF,     ///< data_type: [Set/Get] KDRV_SIE_RINGBUF_INFO
	KDRV_SIE_ITEM_IR_INFO,		///< data_type:     [Get] KDRV_SIE_RGBIR_INFO, output Ir-Level and saturation gain for rgbir 4x4 pattern
	KDRV_SIE_ITEM_MD,     		///< data_type: [Set/Get] KDRV_SIE_MD
	KDRV_SIE_ITEM_MD_RSLT, 		///< data_type: [Set/Get] KDRV_SIE_MD_RST
	KDRV_SIE_ITEM_REF_LOAD_ID, 	///< data_type: [Set] 	  KDRV_SIE_PROC_ID
	KDRV_SIE_ITEM_DMA_ABORT, 	///< data_type: [Set] 	  BOOL
	KDRV_SIE_ITEM_DMA_STS, 		///< data_type:     [Get] KDRV_SIE_DMA_STS
	KDRV_SIE_ITEM_DVS_CODE, 	///< data_type: [Set/Get] KDRV_SIE_DVS_CODE, dvs code for dvs data (cannot runtime change, must set before CTL_SIE_TRIG_START)
	KDRV_SIE_ITEM_DVS_EN,	 	///< data_type: [Set/Get] BOOL, ENABLE : SIE parsing dvs data
	KDRV_SIE_ITEM_DVS_SZ,	 	///< data_type: [Set/Get] USIZE, dvs data dram output size
	KDRV_SIE_ITEM_VD_DLY,	 	///< data_type: [Set/Get] KDRV_SIE_VD_DLY
	KDRV_SIE_ITEM_SYS_INFO,	 	///< data_type:     [Get] KDRV_SIE_SYS_INFO, get sie hw debug information (for debug check only)
	KDRV_SIE_ITEM_CCIR_H,		///< data_type: [Set]     UINT16, set ccir header (for debug check only)
	KDRV_SIE_ITEM_CCIR_H_STS,	///< data_type:     [Get] UINT32, get ccir header status (for debug check only)
	KDRV_SIE_PARAM_MAX,
	KDRV_SIE_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_SIE_PARAM_ID)
} KDRV_SIE_PARAM_ID;
STATIC_ASSERT((KDRV_SIE_PARAM_MAX & KDRV_SIE_PARAM_REV) == 0);
#define KDRV_SIE_IGN_CHK KDRV_SIE_PARAM_REV	//only support set/get function

typedef enum {
	RESERVED = 0,
	ENUM_DUMMY4WORD(KDRV_SIE_TRIGGER_PARAM)
} KDRV_SIE_TRIGGER_PARAM;

typedef struct {
	INT32 (*callback)(VOID *callback_info, VOID *user_data);
	INT32 (*reserve_buf)(UINT32 phy_addr);
	INT32 (*free_buf)(UINT32 phy_addr);
} KDRV_SIE_CALLBACK_FUNC;

typedef struct {
	BOOL pxclk_en;
	BOOL wait_end;				///< wait vd-come, then disable sie_clk
} KDRV_SIE_SUSPEND_PARAM;

typedef struct {
	BOOL pxclk_en;
} KDRV_SIE_RESUME_PARAM;

INT32 kdrv_sie_open(UINT32 chip, UINT32 engine);
INT32 kdrv_sie_close(UINT32 chip, UINT32 engine);
INT32 kdrv_sie_resume(UINT32 id, void *param);
INT32 kdrv_sie_suspend(UINT32 id, void *param);
INT32 kdrv_sie_set(UINT32 id, KDRV_SIE_PARAM_ID parm_id, VOID *param);
INT32 kdrv_sie_get(UINT32 id, KDRV_SIE_PARAM_ID parm_id, VOID *param);
INT32 kdrv_sie_trigger(UINT32 id, KDRV_SIE_TRIGGER_PARAM *rpc_param, KDRV_SIE_CALLBACK_FUNC *cb_func, VOID *user_data);

void kdrv_sie_init(void);
void kdrv_sie_uninit(void);

UINT32 kdrv_sie_buf_query(UINT32 engine_num);
INT32 kdrv_sie_buf_init(UINT32 input_addr, UINT32 buf_size);
INT32 kdrv_sie_buf_uninit(void);
INT32 kdrv_sie_get_sie_limit(KDRV_SIE_PROC_ID id, void *data); // KDRV_SIE_LIMIT
INT32 kdrv_sie_dump_fb_info(INT fd, UINT32 en);
void kdrv_sie_set_checksum_en(UINT32 id, UINT32 en); // set check sum enable, for get debug info

#endif //_KDRV_SIE_H_
