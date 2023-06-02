/*
    Public header file for RDE module.

    This file is the header file that define the API and data type for SIE
    module.

    @file       sie_lib.h
    @ingroup    mIDrvIPPSIE
    @note

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _SIEP_LIB_H
#define _SIEP_LIB_H

#include "kwrap/type.h"

/**
    @addtogroup mIIPPSIE
*/
//@{

/**
    @name Define SIE function.
*/
//@{
#define SIE_SPI_TRG_EN          0x00000001 ///< Enable function
#define SIE_PATGEN_EN           0x00000002 ///< Enable function
#define SIE_DVI_EN              0x00000004 ///< Enable function
#define SIE_OB_AVG_EN           0x00000008 ///< Enable function
#define SIE_OB_SUB_SEL          0x00000010 ///< Enable function
#define SIE_OB_BYPASS_EN        0x00000020 ///< Enable function
#define SIE_OB_FRAME_AVG_EN     0x00000040 ///< Enable function
#define SIE_OB_PLANE_SUB_EN     0x00000080 ///< Enable function
#define SIE_SHDR_YOUT_EN        0x00000100 ///< Enable function
#define SIE_CGAIN_EN            0x00000200 ///< Enable function
#define SIE_DVS_DECOMP_EN       0x00000400 ///< Enable function
#define SIE_DPC_EN              0x00000800 ///< Enable function
//#define SIE_COLDEF_EN           0x00001000 ///< Enable function
#define SIE_MD_EN               0x00001000 ///< Enable function
#define SIE_PFPC_EN             0x00002000 ///< Enable function
//#define SIE_GAMMA_EN            0x00004000 ///< Enable function
#define SIE_ECS_EN              0x00008000 ///< Enable function
#define SIE_DGAIN_EN            0x00010000 ///< Enable function
#define SIE_BS_H_EN             0x00020000 ///< Enable function
#define SIE_BS_V_EN             0x00040000 ///< Enable function
#define SIE_RAWENC_EN           0x00080000 ///< Enable function
#define SIE_STCS_HISTO_Y_EN     0x00200000 ///< Enable function
#define SIE_STCS_LA_EN          0x00800000 ///< Enable function
#define SIE_STCS_CA_EN          0x01000000 ///< Enable function
#define SIE_STCS_VA_EN          0x02000000 ///< Enable function
#define SIE_STCS_ETH_EN         0x04000000 ///< Enable function
#define SIE_DRAM_OUT0_EN        0x08000000 ///< Enable function
#define SIE_COMPANDING_EN       0x10000000 ///< Enable function
#define SIE_RGBIR_FORMAT_SEL    0x20000000 ///< Bayer format Selection
#define SIE_RCCB_FORMAT_SEL     0x40000000 ///< Bayer format Selection
#define SIE_ECS_BAYER_MODE_SEL  0x80000000 ///< ECS Bayer mode Selection
//@}



/**
    @name Define SIE interrput.
*/
//@{
#define SIE_INT_VD                 0x00000001 ///< Enable interrupt
#define SIE_INT_BP1                0x00000002 ///< Enable interrupt
#define SIE_INT_BP2                0x00000004 ///< Enable interrupt
#define SIE_INT_BP3                0x00000008 ///< Enable interrupt
#define SIE_INT_ACTST              0x00000010 ///< Enable interrupt
#define SIE_INT_CRPST              0x00000020 ///< Enable interrupt
#define SIE_INT_DRAM_IN_OUT_ERR    0x00000040 ///< Enable interrupt
#define SIE_INT_DRAM_OUT0_END      0x00000080 ///< Enable interrupt
#define SIE_INT_DRAM_OUT1_END      0x00000100 ///< Enable interrupt
#define SIE_INT_DRAM_OUT2_END      0x00000200 ///< Enable interrupt
#define SIE_INT_DPCF               0x00002000 ///< Enable interrupt
#define SIE_INT_RAWENC_END         0x00004000 ///< Enable interrupt
#define SIE_INT_RAWENC_OUTOVFL     0x00008000 ///< Enable interrupt
#define SIE_INT_ACTEND             0x00010000 ///< Enable interrupt
#define SIE_INT_CROPEND            0x00020000 ///< Enable interrupt
#define SIE_INT_MD_HIT             0x00080000 ///< Enable interrupt
#define SIE_INT_ALL                0xffffffff ///< Enable interrupt
//@}

//BCC parameters
#define GAM_TBL_TAP 33
#define MAX_LC_TBL_NUM 24
#define LFN_ENTRY_NUM 8//=Q table num
#define FINE_LBN 5
#define LFN_TABLE_NUM 8


#define BCC_CAL_ENC_UNIT_NUM(a, unit)   (((a)+((unit)-1))/(unit))
#define BCC_CEIL_BYTE_TO_WORD(a)        ((((a)+3)>>2)<<2)
#define BCC_MLT_SIDE_SIZE 16
#define BCC_SGL_SIDE_SIZE 32
#define BCC_BYTE_UNIT  8
#define BCC_DSEG_PXL_NUM 32


typedef enum _SIE_ENGINE_ID {
	SIE_ENGINE_ID_1 = 0,    ///< SIE1
	SIE_ENGINE_ID_2 = 1,    ///< SIE2
	SIE_ENGINE_ID_3 = 2,    ///< SIE3
	SIE_ENGINE_ID_4 = 3,    ///< SIE4
	SIE_ENGINE_ID_5 = 4,    ///< SIE5
	SIE_ENGINE_ID_6 = 5,    ///< SIE6
	SIE_ENGINE_ID_7 = 6,    ///< SIE7
	SIE_ENGINE_ID_8 = 7,    ///< SIE7
	SIE_ENGINE_ID_MAX,
	ENUM_DUMMY4WORD(SIE_ENGINE_ID)
} SIE_ENGINE_ID;

/**
    SIE M-Clock Source Selection

    Structure of M-Clock source SIE configuration.
*/
//@{
typedef enum {
	SIE_MCLKSRC_CURR = 0, ///< SIE M-clock source as current setting(no setting, no changing)
	SIE_MCLKSRC_480,      ///< SIE M-clock source as 480MHz
	SIE_MCLKSRC_PLL4,     ///< not support @520
	SIE_MCLKSRC_PLL5,     ///< SIE M-clock source as PLL5
	SIE_MCLKSRC_PLL10,    ///< SIE M-clock source as PLL10
	SIE_MCLKSRC_PLL12,    ///< SIE M-clock source as PLL12
	SIE_MCLKSRC_PLL18,    ///< SIE M-clock source as PLL18
} SIE_MCLKSRC_SEL;

typedef enum {
	SIE_ISR_CB_IMD = 0,   // immediately callback
	SIE_ISR_CB_VDLATCH,   // va-latch callback
} SIE_ISR_CB_TYPE;

/**
SIE Enumeration

@name   SIE_Enumeration
*/
//@{

typedef enum {
	MAIN_IN_PARA_MSTR_SNR = 0,   ///< Parallel Master Sensor
	MAIN_IN_PARA_SLAV_SNR,       ///< Parallel Slave Sensor
	MAIN_IN_SELF_PATGEN,         ///< Self Pattern-Generator
	MAIN_IN_VX1_SNR,             ///< Vx1 Sensor
	MAIN_IN_CSI_1,               ///< Serial Sensor from CSI-1
	MAIN_IN_CSI_2,               ///< Serial Sensor from CSI-2
	MAIN_IN_CSI_3,               ///< Serial Sensor from CSI-3
	MAIN_IN_CSI_4,               ///< Serial Sensor from CSI-4
	MAIN_IN_CSI_5,               ///< Serial Sensor from CSI-5
	MAIN_IN_CSI_6,               ///< Serial Sensor from CSI-6
	MAIN_IN_CSI_7,               ///< Serial Sensor from CSI-7
	MAIN_IN_CSI_8,               ///< Serial Sensor from CSI-8
	MAIN_IN_DRAM,                ///< Dram Input (channel 0)
	MAIN_IN_AHD,
	MAIN_IN_2PPATGEN,
	MAIN_IN_SLVS_EC,             ///< Serial Sensor from SLVS-EC
	MAIN_IN_VX1_IF1_SNR,         ///< Vx1 Sensor, interface 1
	ENUM_DUMMY4WORD(SIE_MAIN_INPUT_SEL)
} SIE_MAIN_INPUT_SEL;

typedef enum {
	SIE_PHASE_RISING  = 0,  ///< rising edge latch/trigger
	SIE_PHASE_FALLING,      ///< falling edge latch/trigger
	ENUM_DUMMY4WORD(SIE_DATA_PHASE_SEL)
} SIE_DATA_PHASE_SEL;

typedef enum {
	SIE_FUNC_DISABLE  = 0,  ///< Those bit=1 would be set, others remain
	SIE_FUNC_ENABLE,        ///< Those bit=1 would be clear, others remain
	SIE_FUNC_SET,           ///< All bit would be configured as 0/1 ther are
	ENUM_DUMMY4WORD(SIE_FUNC_SWITCH_SEL)
} SIE_FUNC_SWITCH_SEL;

typedef enum {
	SIE_PACKBUS_8  = 0, ///< 8 bits packing
	SIE_PACKBUS_10,     ///< 10 bits packing
	SIE_PACKBUS_12,     ///< 12 bits packing
	SIE_PACKBUS_16,     ///< 16 bits packing
	ENUM_DUMMY4WORD(SIE_PACKBUS_SEL)
} SIE_PACKBUS_SEL;

typedef enum {
	SIE_OUTBIT_12  = 0, ///< 12 output bits
	SIE_OUTBIT_10,      ///< 10 output bits
	SIE_OUTBIT_8,       ///< 8	output bits
	ENUM_DUMMY4WORD(SIE_OUTBIT_SEL)
} SIE_OUTBIT_SEL;

typedef enum {
	SIE_PACK_AT_MSB  = 0, ///< data output at MSB
	SIE_PACK_AT_LSB,      ///< data output at LSB
	ENUM_DUMMY4WORD(SIE_PACK_FORMAT)
} SIE_PACK_FORMAT;

typedef enum {
	SIE_BAYER_FORMAT = 0, ///< Bayer format
	SIE_RGBIR_FORMAT,     ///< RGB-IR format
	SIE_RCCB_FORMAT,      ///< RCCB format
	ENUM_DUMMY4WORD(SIE_BAYER_TYPE)
} SIE_BAYER_TYPE;

typedef enum {
	OUT0_SOURCE_CROP_IMG = 0, ///< SIE Dram output channel 0 source = cropped image
	OUT0_SOURCE_BS_IMG,      ///< SIE Dram output channel 0 source = cropped and scaled image
	ENUM_DUMMY4WORD(SIE_OUT0_SRC_SEL)
} SIE_OUT0_SRC_SEL;

typedef enum {
	OUT3_SOURCE_VACC   = 0, ///< SIE Dram output channel 3 source = variation accumulation
	OUT3_SOURCE_BS_IMG,     ///< SIE Dram output channel 3 source = cropped and scaled image
	OUT3_SOURCE_YOUT_ACC,     ///< SIE Dram output channel 3 source = cropped and scaled image
	ENUM_DUMMY4WORD(SIE_OUT3_SRC_SEL)
} SIE_OUT3_SRC_SEL;

typedef enum {
	OUT4_SOURCE_VACC   = 0, ///< SIE Dram output channel 4 source = variation accumulation
	OUT4_SOURCE_ETH,     ///< SIE Dram output channel 4 source = cropped and scaled image
	ENUM_DUMMY4WORD(SIE_OUT4_SRC_SEL)
} SIE_OUT4_SRC_SEL;

typedef enum {
	SIE_BURST_64W  = 0,   ///< 64  words
	SIE_BURST_48W  = 1,   ///< 48  words
	SIE_BURST_32W  = 2,   ///< 32  words
	SIE_BURST_96W  = 3,   ///< 96  words
	SIE_BURST_128W = 4,   ///< 128 words
	ENUM_DUMMY4WORD(SIE_BURST_SEL)
} SIE_BURST_SEL;

typedef enum {
	SIE_BURST_OUT0_128W = 0,  ///< 128 words
	SIE_BURST_OUT0_96W  = 1,  ///< 96  words
	SIE_BURST_OUT0_64W  = 2,  ///< 64  words
	SIE_BURST_OUT0_32W  = 3,  ///< 32  words
	SIE_BURST_OUT0_160W = 4,  ///< 160 words
	SIE_BURST_OUT0_192W = 5,  ///< 192 words
	SIE_BURST_OUT0_224W = 6,  ///< 224 words
	SIE_BURST_OUT0_256W = 7,  ///< 256 words
	ENUM_DUMMY4WORD(SIE_BURST_OUT0_SEL)
} SIE_BURST_OUT0_SEL;

typedef enum { // would be removed soon
	IN0_BURST_LENGTH_64   = 0,    ///< SIE 1 Dram input channel 0 burst length = 64 words
	IN0_BURST_LENGTH_32   = 1,    ///< SIE 1 Dram input channel 0 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_IN0_BURST_SEL)
} SIE_DRAM_IN0_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	IN1_BURST_LENGTH_64   = 0,    ///< SIE 1-4 Dram input channel 1 burst length = 64 words
	IN1_BURST_LENGTH_32   = 1,    ///< SIE 1-4 Dram input channel 1 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_IN1_BURST_SEL)
} SIE_DRAM_IN1_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	IN2_BURST_LENGTH_64   = 0,    ///< SIE 1-8 Dram input channel 2 burst length = 64 words
	IN2_BURST_LENGTH_32   = 1,    ///< SIE 1-8 Dram input channel 2 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_IN2_BURST_SEL)
} SIE_DRAM_IN2_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	OUT0_BURST_LENGTH_128  = 0,    ///< SIE 1-8 Dram output channel 0 burst length = 128 words
	OUT0_BURST_LENGTH_96   = 1,    ///< SIE 1-8 Dram output channel 0 burst length = 96 words
	OUT0_BURST_LENGTH_64   = 2,    ///< SIE 1-8 Dram output channel 0 burst length = 64 words
	OUT0_BURST_LENGTH_32   = 3,    ///< SIE 1-8 Dram output channel 0 burst length = 32 words
	OUT0_BURST_LENGTH_160  = 4,    ///< SIE 1   Dram output channel 0 burst length = 160 words
	OUT0_BURST_LENGTH_192  = 5,    ///< SIE 1   Dram output channel 0 burst length = 192 words
	OUT0_BURST_LENGTH_224  = 6,    ///< SIE 1   Dram output channel 0 burst length = 224 words
	OUT0_BURST_LENGTH_256  = 7,    ///< SIE 1   Dram output channel 0 burst length = 256 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT0_BURST_SEL)
} SIE_DRAM_OUT0_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	OUT1_BURST_LENGTH_64   = 0,    ///< SIE 1-8 Dram input channel 1 burst length = 64 words
	OUT1_BURST_LENGTH_32   = 1,    ///< SIE 1-8 Dram input channel 1 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT1_BURST_SEL)
} SIE_DRAM_OUT1_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	OUT2_BURST_LENGTH_64   = 0,    ///< SIE 1-8 Dram input channel 2 burst length = 64 words
	OUT2_BURST_LENGTH_32   = 1,    ///< SIE 1-8 Dram input channel 2 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT2_BURST_SEL)
} SIE_DRAM_OUT2_BURST_SEL;// would be removed soon

typedef enum {
	BURST_MODE_NORMAL       =  0,  ///< normal
	BURST_MODE_BURST_CAP,          ///< for high-data-rate capture
	BURST_MODE_REMAIN,             ///< for Driver only
	ENUM_DUMMY4WORD(SIE_DRAM_BURST_MODE_SEL)
} SIE_DRAM_BURST_MODE_SEL;

typedef enum {
	DVI_FORMAT_CCIR601  = 0,    ///< CCIR 601
	DVI_FORMAT_CCIR656_EAV,     ///< CCIR 656 EAV
	DVI_FORMAT_CCIR656_ACT,     ///< CCIR 656 ACT
	ENUM_DUMMY4WORD(SIE_DVI_FORMAT_SEL)
} SIE_DVI_FORMAT_SEL;

typedef enum {
	DVI_MODE_SD  = 0,   ///< SD mode (8 bits)
	DVI_MODE_HD,        ///< HD mode (16bits)
	DVI_MODE_HD_INV,    ///< HD mode (16bits) with Byte Inverse
	ENUM_DUMMY4WORD(SIE_DVI_IN_MODE_SEL)
} SIE_DVI_IN_MODE_SEL;


typedef enum {
	DVI_OUT_SWAP_YUYV  = 0,   ///<
	DVI_OUT_SWAP_YVYU,        ///<
	DVI_OUT_SWAP_UYVY,        ///<
	DVI_OUT_SWAP_VYUY,        ///<
	ENUM_DUMMY4WORD(SIE_DVI_OUT_SWAP_SEL)
} SIE_DVI_OUT_SWAP_SEL;


typedef enum {
	PAT_COLORBAR    = 1,    ///< Color bar
	PAT_RANDOM,             ///< Random
	PAT_FIXED,              ///< Fixed
	PAT_HINCREASE,          ///< 1D Increment
	PAT_HVINCREASE,         ///< 2D increment
	ENUM_DUMMY4WORD(SIE_PATGEN_SEL)
} SIE_PATGEN_SEL;

typedef enum {
	ECS_MAPSIZE_65X65  = 0,   ///< 65x65
	ECS_MAPSIZE_49X49,        ///< 49x49
	ECS_MAPSIZE_33X33,        ///< 33x33
	ENUM_DUMMY4WORD(SIE_ECS_MSIZE_SEL)
} SIE_ECS_MSIZE_SEL;

typedef enum _BCC_BST_SUP_FMT {
	BCC_BST_MLT_STRP = 0,
	BCC_BST_SGL_STRP = 1,
	ENUM_DUMMY4WORD(BCC_BST_SUP_FMT)
} BCC_BST_SUP_FMT;

typedef enum {
	ROUND_FIXED  = 0,   ///< fixed rounding
	ROUND_RANDOM = 1,   ///< random rounding
	ENUM_DUMMY4WORD(SIE_BCC_ROUND_SEL)
} SIE_BCC_ROUND_SEL;

typedef enum {
	LFN_CSB_TBL_8  = 0,   ///< use first  8 table
	LFN_CSB_TBL_16 = 1,   ///< use first 16 table
	LFN_CSB_TBL_20 = 2,   ///< use first 20 table
	LFN_CSB_TBL_24 = 3,   ///< use first 24 table
	ENUM_DUMMY4WORD(LFN_CSB_TBL_NUM_SEL)
} LFN_CSB_TBL_NUM_SEL;

typedef enum {
	DIST_MEAN_RATIO = 0,
	DIST_TRUC_BIT_RATIO = 1,
	ENUM_DUMMY4WORD(BCC_DIST_CAL_TYPE)
} BCC_DIST_CAL_TYPE;

typedef enum {
	STCS_LA1_POST_CG  = 0,   ///< LA1 source from data right after  color-gain
	STCS_LA1_PRE_CG,         ///< LA1 source from data right before color-gain
	ENUM_DUMMY4WORD(SIE_STCS_LA1_SRC_SEL)
} SIE_STCS_LA1_SRC_SEL;

typedef enum {
	STCS_LA1_RGB     = 0,
	STCS_LA1_G,
	ENUM_DUMMY4WORD(SIE_STCS_LA_RGB2Y1_MOD_SEL)
} SIE_STCS_LA_RGB2Y1_MOD_SEL;

typedef enum {
	STCS_LA2_RGB     = 0,
	STCS_LA2_G,
	ENUM_DUMMY4WORD(SIE_STCS_LA_RGB2Y2_MOD_SEL)
} SIE_STCS_LA_RGB2Y2_MOD_SEL;

typedef enum {
	STCS_VA_FLTR_MIRROR  = 0,   ///<
	STCS_VA_FLTR_INVERSE,       ///<
	ENUM_DUMMY4WORD(SIE_STCS_VA_FLTR_SYMM_SEL)
} SIE_STCS_VA_FLTR_SYMM_SEL;

typedef enum {
	STCS_VA_FLTR_SIZE_1  = 0,   ///<
	STCS_VA_FLTR_SIZE_3,        ///<
	STCS_VA_FLTR_SIZE_5,        ///<
	STCS_VA_FLTR_SIZE_7,        ///<
	ENUM_DUMMY4WORD(SIE_STCS_VA_FLTR_SIZE_SEL)
} SIE_STCS_VA_FLTR_SIZE_SEL;


typedef enum {
	STCS_HISTO_POST_GAMMA     = 0,   ///<
	STCS_HISTO_PRE_GAMMA,          ///<
	ENUM_DUMMY4WORD(SIE_HISTO_SRC_SEL)
} SIE_HISTO_SRC_SEL;

typedef enum {
	STCS_CA_SRC_BEFORE_OB     = 0,   ///<
	STCS_CA_PRE_AFTER_OB,          ///<
	ENUM_DUMMY4WORD(SIE_CA_SRC_SEL)
} SIE_CA_SRC_SEL;


typedef enum {
	STCS_VA_OUT_GP_1     = 0,   ///<
	STCS_VA_OUT_GP_1n2,         ///<
	ENUM_DUMMY4WORD(SIE_VA_OUT_SEL)
} SIE_VA_OUT_SEL;


typedef enum {
	PFPC_MODE_4X8  = 0, ///< 4x8 period
	PFPC_MODE_8X4,      ///< 8x4 period
	ENUM_DUMMY4WORD(SIE_PFPC_MODE_SEL)
} SIE_PFPC_MODE_SEL;



typedef enum {
	DEF_FACT_50  = 0,      ///< defect concealment cross-channel weighting = 50%
	DEF_FACT_25,           ///< defect concealment cross-channel weighting = 25%; recommanded
	DEF_FACT_12,           ///< defect concealment cross-channel weighting = 12.5%
	DEF_FACT_06,           ///< defect concealment cross-channel weighting = 6.25%
	//DEF_FACT_0    ,      ///< defect concealment cross-channel weighting =  0%
	ENUM_DUMMY4WORD(SIE_DEF_FACT_SEL)
} SIE_DEF_FACT_SEL;






typedef enum {
	RGBIR_PIX_RG_GI,
	RGBIR_PIX_GB_IG,
	RGBIR_PIX_GI_BG,
	RGBIR_PIX_IG_GR,
	RGBIR_PIX_BG_GI,
	RGBIR_PIX_GR_IG,
	RGBIR_PIX_GI_RG,
	RGBIR_PIX_IG_GB,
	ENUM_DUMMY4WORD(SIE_RGBIR_CFAPAT_SEL)
} SIE_RGBIR_CFAPAT_SEL;



typedef enum {
	CFA_R = 0,  ///< start pixel is R
	CFA_Gr = 1, ///< start pixel is GR
	CFA_Gb = 2, ///< start pixel is GB
	CFA_B = 3,  ///< start pixel is B
	ENUM_DUMMY4WORD(SIE_CFAPAT_SEL)
} SIE_CFAPAT_SEL;



typedef enum {
	SIE_PXCLKSRC_OFF = 0, ///< SIE pixel-clock disabled
	SIE_PXCLKSRC_PAD,     ///< SIE pixel-clock enabled, source as pixel-clock-pad
	SIE_PXCLKSRC_PAD_AHD,
	SIE_PXCLKSRC_MCLK,    ///< SIE pixel-clock enabled, source as MCLK (SIE_PXCLK from SIE_MCLK, SIE2_PXCLK from SIE_MCLK2, SIE3_PXCLK from SIE_MCLK2, SIE4_PXCLK from SIE_MCLK2)
	SIE_PXCLKSRC_VX1_1X,  ///< SIE pixel-clock enabled, source as Vx1 1x clock (only for SIE2/3)
	SIE_PXCLKSRC_VX1_2X,  ///< SIE pixel-clock enabled, source as Vx1 2x clock (only for SIE2/3)
} SIE_PXCLKSRC;



/**
    SIE Pixel-Clock Source Selection

    Structure of Pixel-Clock source SIE configuration.
*/
//@{
typedef enum {
	SIE_CLKSRC_CURR = 0, ///< SIE clock source as current setting(no setting, no changing)
	SIE_CLKSRC_480,      ///< SIE clock source as 480MHz
	SIE_CLKSRC_PLL5,     ///< SIE M-clock source as PLL5
	SIE_CLKSRC_PLL13,     ///< SIE clock source as PLL13
	SIE_CLKSRC_PLL12,    ///< SIE clock source as PLL12
	SIE_CLKSRC_320,
	SIE_CLKSRC_192,
	SIE_CLKSRC_PLL10,
} SIE_CLKSRC_SEL;
//@}

/**
    @name SIE enum - Change Funciton Parameter Selection
*/
//@{
typedef enum {
	SIE_CHG_MAIN_IN = 0,
	SIE_CHG_SRC_WIN,
	SIE_CHG_ACT_WIN,
	SIE_CHG_PAT_GEN,				///< refer to "SIE_PATGEN_INFO"
	SIE_CHG_BCC_RATE,
	SIE_CHG_CROP_WIN,	   			///< refer to "SIE_CRP_WIN_INFO"        , change crop size and dram-channel-lineoffset
	SIE_CHG_IN_ADDR,                ///< refer to "SIE_CHG_IN_ADDR_INFO"    , change input address
	SIE_CHG_OUT_ADDR,               ///< refer to "SIE_CHG_OUT_ADDR_INFO"   , change output address
	SIE_CHG_OUT_ADDR_FLIP,          ///< refer to "SIE_CHG_OUT_ADDR_INFO"   , change output address + Flip
	SIE_CHG_IO_LOFS,                ///< refer to "SIE_CHG_IO_LOFS_INFO"    , change DRAM lineoffset
	SIE_CHG_OUT_SRC,                ///< refer to "SIE_CHG_OUT_SRC_INFO"    , change DRAM output source
	//na//SIE_CHG_OUT_BITDEPTH,           ///< refer to ""
	SIE_CHG_DVI,                    ///< refer to "SIE_CHG_DVI_INFO"        , change DVI
	SIE_CHG_OBDT,                   ///< refer to "SIE_OB_DT_INFO"          , change OB Detect
	SIE_CHG_OBOFS,                  ///< refer to "SIE_OB_OFS_INFO"         , change OB offset
	SIE_CHG_FLIP,                   ///< refer to "SIE_FLIP_INFO"           , change H/V flip
	SIE_CHG_DP,                     ///< refer to "SIE_CHG_DP_INFO"         , change DP concealment
	//NA//SIE_CHG_YOUT,                   ///< refer to "SIE_YOUT_INFO"           , change YOUT parameters
	SIE_CHG_ECS,                    ///< refer to "SIE_ECS_INFO"            , change ECS
	SIE_CHG_DGAIN,                  ///< refer to "SIE_DGAIN_INFO"          , change Digital-Gain
	SIE_CHG_BS_H,                   ///< refer to "SIE_BS_H_INFO"           , change Bayer-scaling H
	SIE_CHG_BS_V,                   ///< refer to "SIE_BS_V_INFO"           , change Bayer-scaling V
	SIE_CHG_CG,                     ///< refer to "SIE_CG_INFO"             , change Color Gain
	//NA//SIE_CHG_GRIDLINE,               ///< refer to "SIE_GRIDLINE_INFO"       , change Grid Line
	SIE_CHG_DRAM_BURST,             ///< refer to "SIE_DRAM_BURST_INFO"     , change Dram Burst-length
	SIE_CHG_VIG,                    ///< refer to "SIE_VIG_INFO"            , change STCS VIG
	SIE_CHG_STCS_PATH,              ///< refer to "SIE_STCS_PATH_INFO"      , change STCS path
	SIE_CHG_CALASIZE_GRP,           ///< refer to "SIE_STCS_CALASIZE_GRP_INFO", change STCS CA/LA Size
	SIE_CHG_CA_TH,                  ///< refer to "SIE_CA_TH_INFO"          , change STCS CA Threshold
	SIE_CHG_CA_IRSUB,               ///< refer to "SIE_CA_IRSUB_INFO"       , change STCS CA IR_SUB Weighting
	SIE_CHG_LA_CG,                  ///< refer to "SIE_LA_CG_INFO"          , change STCS LA color-gain
	SIE_CHG_LA_GMA,                 ///< refer to "SIE_LA_GMA_INFO"         , change STCS LA gamma
	SIE_CHG_LA_IRSUB,               ///< refer to "SIE_LA_IRSUB_INFO"       , change STCS LA IR_SUB Weighting
	//NA//SIE_CHG_VA_CROP,                ///< refer to "SIE_VA_CROP_INFO", change STCS VA Crop
	//NA//SIE_CHG_VA_WIN,                 ///< refer to "SIE_VA_WIN_INFO", change STCS VA Window
	//NA//SIE_CHG_VA_CG,                  ///< refer to "SIE_VA_CG_INFO"          , change STCS VA color-gain
	//NA//SIE_CHG_VA_GMA,                 ///< refer to "SIE_VA_GMA_INFO"         , change STCS LA gamma
	//NA//SIE_CHG_VA_FLTR_G1,             ///< refer to "SIE_VA_FLTR_GROUP_INFO"  , change STCS VA filter group-1
	//NA//SIE_CHG_VA_FLTR_G2,             ///< refer to "SIE_VA_FLTR_GROUP_INFO"  , change STCS VA filter group-2
	//NA//SIE_CHG_VA_INDEP_WIN0,          ///< refer to "SIE_VA_INDEP_WIN_INFO"   , change STCS VA independent window-0
	//NA//SIE_CHG_VA_INDEP_WIN1,          ///< refer to "SIE_VA_INDEP_WIN_INFO"   , change STCS VA independent window-1
	//NA//SIE_CHG_VA_INDEP_WIN2,          ///< refer to "SIE_VA_INDEP_WIN_INFO"   , change STCS VA independent window-2
	//NA//SIE_CHG_VA_INDEP_WIN3,          ///< refer to "SIE_VA_INDEP_WIN_INFO"   , change STCS VA independent window-3
	//NA//SIE_CHG_VA_INDEP_WIN4,          ///< refer to "SIE_VA_INDEP_WIN_INFO"   , change STCS VA independent window-4
	//NA//SIE_CHG_ETH,                    ///< refer to "SIE_ETH_INFO"            , change STCS Eth
	SIE_CHG_BCC_ADJ,                ///< refer to "SIE_BCC_ADJ_INFO"        , change BCC
	SIE_CHG_ACT_EN,                	///< refer to "BOOL"    				, change active enable
	SIE_CHG_BP1,
	SIE_CHG_BP2,
	SIE_CHG_BP3,
	SIE_CHG_INTE,
	SIE_CHG_SINGLEOUT,
	SIE_CHG_VDHD_INTERVAL,
	SIE_CHG_DECOMPANDING,
	SIE_CHG_COMPANDING,
	SIE_CHG_OUTMODE,
	SIE_CHG_RAWENC_RATE,
	SIE_CHG_ACT_ENG,
	SIE_CHG_RINGBUF,
	SIE_CHG_MD,
	SIE_CHG_STCS_OB,
	SIE_CHG_DVS_WIN,
	SIE_CHG_DVS_CODE,
	SIE_CHG_CCIR656_HEADER,
	SIE_CHG_VDHD_DELAY,
	SIE_CHG_OUT_FORMAT,
	ENUM_DUMMY4WORD(SIE_CHANGE_FUN_PARAM_SEL)
} SIE_CHANGE_FUN_PARAM_SEL;
//@}




typedef enum {
	SIE_WAIT_VD         =  0,  ///<
	SIE_WAIT_BP1,              ///<
	SIE_WAIT_BP2,              ///<
	SIE_WAIT_BP3,              ///<
	SIE_WAIT_ACTST,              ///<
	SIE_WAIT_CRPST,              ///<
	SIE_WAIT_ACTEND,              ///<
	SIE_WAIT_CRPEND,              ///<
	SIE_WAIT_DO0_END,          ///< Dram Output-0 END
	SIE_WAIT_DO1_END,          ///< Dram Output-0 END
	SIE_WAIT_DO2_END,          ///< Dram Output-0 END
	SIE_WAIT_DO3_END,          ///< Dram Output-0 END
	SIE_WAIT_DO4_END,          ///< Dram Output-0 END
	SIE_WAIT_DO5_END,          ///< Dram Output-0 END
	ENUM_DUMMY4WORD(SIE_WAIT_EVENT_SEL)
} SIE_WAIT_EVENT_SEL;


typedef enum {
	PINGPONG_BUFF_0         =  0,  ///< enable selected function bits
	PINGPONG_BUFF_1,               ///< enable selected function bits
	PINGPONG_BUFF_2,               ///< enable selected function bits
	PINGPONG_BUFF_3,               ///< enable selected function bits
	PINGPONG_BUFF_4,               ///< enable selected function bits
	PINGPONG_BUFF_5,               ///< enable selected function bits
	PINGPONG_BUFF_6,               ///< enable selected function bits
	PINGPONG_BUFF_7,               ///< enable selected function bits
	PINGPONG_BUFF_CURR,            ///< enable selected function bits
	PINGPONG_BUFF_LAST,            ///< enable selected function bits
	ENUM_DUMMY4WORD(SIE_PINGPONG_BUFF_SEL)
} SIE_PINGPONG_BUFF_SEL;

typedef enum {
	SIE_NO_FLIP             =  0,
	SIE_H_FLIP,                    ///< enable selected function bits
	SIE_V_FLIP,                    ///< enable selected function bits
	SIE_HV_FLIP,                   ///< enable selected function bits
	ENUM_DUMMY4WORD(SIE_FLIP_SEL)
} SIE_FLIP_SEL;

typedef enum {
	DRAM_OUT_MODE_NORMAL = 0,
	DRAM_OUT_MODE_SINGLE,
	ENUM_DUMMY4WORD(SIE_DRAM_OUT_MODE)
} SIE_DRAM_OUT_MODE;

typedef enum {
	MASK_COL_ROW= 0,
	MASK_8X8,
	ENUM_DUMMY4WORD(SIE_MD_MASK_MODE)
} SIE_MD_MASK_MODE;

typedef enum {
	MD_SRC_PREGAMMA= 0,
	MD_SRC_POSTGAMMA,
	ENUM_DUMMY4WORD(SIE_MD_SRC)
} SIE_MD_SRC;

typedef enum {
	TIMING_NO_DELAY = 0,
	TIMING_DELAY,
	ENUM_DUMMY4WORD(SIE_DELAY_SEL)
} SIE_DELAY_SEL;

typedef enum {
	TIMING_DELAY_0T = 0,
	TIMING_DELAY_4T,
	TIMING_DELAY_8T,
	TIMING_DELAY_16T,
	ENUM_DUMMY4WORD(SIE_TIMING_DELAY)
} SIE_TIMING_DELAY;

typedef enum {
	SIE_LOAD_FROM_SIE = 0,
	SIE_LOAD_FROM_SIE2,
	ENUM_DUMMY4WORD(SIE_LOAD_SRC)
} SIE_LOAD_SRC;

typedef struct {
	SIE_DRAM_OUT_MODE  out0mode; ///< out0 mode sel
	SIE_DRAM_OUT_MODE  out1mode; ///< out1 mode sel
	SIE_DRAM_OUT_MODE  out2mode; ///< out2 mode sel
} SIE_DRAM_OUT_CTRL;

typedef struct {
	SIE_TIMING_DELAY vddelay;
	SIE_TIMING_DELAY hddelay;
} SIE_VDHD_DELAY_INFO;

typedef struct {
	SIE_MAIN_INPUT_SEL  MainInSrc;      ///< Main Input Source
	SIE_DATA_PHASE_SEL  VdPhase; ///< VD phase
	SIE_DATA_PHASE_SEL  HdPhase; ///< HD phase
	SIE_DATA_PHASE_SEL  DataPhase; ///< Data phase
	BOOL                bVdInv;  ///< VD Invert
	BOOL                bHdInv;  ///< HD Invert
	BOOL                bDirect2RHE;  ///< HD Invert
	BOOL                bRawbitdepSel; ///< 0: 12bit, 1:10bit
} SIE_MAIN_INPUT_INFO;

typedef struct {
	UINT32 uiBp1;             ///< break point 1
	UINT32 uiBp2;             ///< break point 2
	UINT32 uiBp3;             ///< break point 3
} SIE_BREAKPOINT_INFO;

typedef struct {
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
} SIE_SRC_WIN_INFO;


typedef struct {
	BOOL        bDramOut0err;
	BOOL        bDramIn1Udfl;
	BOOL        bDramIn2Udfl;
	BOOL        bDramOut0Ovfl;
	BOOL        bDramOut1Ovfl;
	BOOL        bDramOut2Ovfl;
	BOOL        bccir656headerhit;
} SIE_ENGINE_STATUS_INFO;

typedef struct {
	UINT16      uiStX; ///< horizontal start
	UINT16      uiStY; ///< vertical start
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
	SIE_CFAPAT_SEL  CfaPat;///< CFA start pattern
} SIE_ACT_WIN_INFO;

typedef struct {
	UINT16      uiStX; ///< horizontal start
	UINT16      uiStY; ///< vertical start
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
	SIE_CFAPAT_SEL  CfaPat;///< CFA start pattern
} SIE_CRP_WIN_INFO;

typedef struct {
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
} SIE_DVS_WIN_INFO;

typedef struct {
	UINT8      uiPosCode; ///< positive code value
	UINT8     uiNegCode; ///< negative code value
	UINT8      uiNoChgCode; ///< no changed code value
} SIE_DVS_CODE_INFO;

typedef struct {
	UINT16      uiStX; ///< horizontal start
	UINT16      uiStY; ///< vertical start
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
	SIE_CFAPAT_SEL  CfaPat;///< CFA start pattern
} SIE_CA_CROP_INFO;

typedef struct {
	UINT16      uiStX; ///< horizontal start
	UINT16      uiStY; ///< vertical start
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
} SIE_LA_CROP_INFO;

typedef struct {
	UINT16      uiFctrX; ///< horizontal scaling factor
	UINT16      uiFctrY; ///< vertical scaling output size
} SIE_CA_SCAL_INFO;

typedef struct {
	UINT32      uiPixRateCnt;
	UINT32      uiPixRateMax;
} SIE_PIX_RATE_INFO;

typedef struct {
	UINT32      uiAddr;     ///< address
	UINT32      uiLofs;     ///< line-offset
	SIE_PACKBUS_SEL PackBus;    ///< pack bus bit number
} SIE_DRAM_IN0_INFO;

typedef struct {
	UINT32      uiAddr;     ///< address
	UINT32      uiSize;     ///< size
} SIE_DRAM_IN1_INFO;

typedef struct {
	UINT32      uiAddr;     ///< address
} SIE_DRAM_IN2_INFO;

typedef struct {
	BOOL             SingleOut0En;
	BOOL             SingleOut1En;
	BOOL             SingleOut2En;
} SIE_DRAM_SINGLE_OUT;

typedef struct {
	SIE_MD_SRC md_src;
	UINT8 sum_frms;
	SIE_MD_MASK_MODE mask_mode;
	UINT32 mask0;
	UINT32 mask1;
	UINT32 blkdiff_thr;
	UINT32 total_blkdiff_thr;
	UINT16 blkdiff_cnt_thr;
} SIE_MD_INFO;

typedef struct {
	SIE_PACKBUS_SEL  PackBus;          ///< pack bus bit number
	SIE_OUTBIT_SEL   OutBitdepth;
	SIE_PACK_FORMAT  PackFmt;
} SIE_DRAM_OUT0_FMT;

typedef struct {
	UINT32           uiAddr;           ///< address
	SIE_OUT0_SRC_SEL OutSrc;           ///< source
	UINT8            uiPingPongBufNum; ///< ping-pong buffer
	UINT32           uiLofs;           ///< line-offset
	SIE_PACKBUS_SEL  PackBus;          ///< pack bus bit number
	UINT32           uiFramOfset;      ///< Frame offset
	BOOL             bHFlip;           ///< H flip
	BOOL             bVFlip;           ///< V flip
	BOOL             bRingBufEn;
	UINT16           uiRingBufLen;
	UINT8            uiOutBitdepth;
	BOOL             bPackFmt;
} SIE_DRAM_OUT0_INFO;

typedef struct {
	UINT32          uiAddr;            ///< address
	UINT8           uiPingPongBufNum;  ///< ping-pong buffer
	UINT32          uiLofs;            ///< line-offset
	UINT32          uiFramOfset;      ///< Frame offset
	BOOL            bHFlip;            ///< H flip
	BOOL            bVFlip;            ///< V flip
} SIE_DRAM_OUT1_INFO;

typedef struct {
	UINT32          uiAddr;            ///< address
	UINT8           uiPingPongBufNum;  ///< ping-pong buffer
	UINT32          uiLofs;            ///< line-offset
	UINT32          uiFramOfset;       ///< Frame offset
} SIE_DRAM_OUT2_INFO;

typedef struct _SIE_BURST_LENGTH {
	SIE_BURST_OUT0_SEL  BurstLenOut0; ///< for SIE1/2/3/4/5/6/7/8
	SIE_BURST_SEL       BurstLenOut1; ///< for SIE1/2/3/4/5/6/7/8
	SIE_BURST_SEL       BurstLenOut2; ///< for SIE1/2/3/4/5/6/7/8
	SIE_BURST_SEL       BurstLenIn0; ///< for SIE1
	SIE_BURST_SEL       BurstLenIn1; ///< for SIE1/2/3/4
	SIE_BURST_SEL       BurstLenIn2; ///< for SIE1/2/3/4/5/6/7/8
	UINT32              uiBurstIntvlOut0; ///< for SIE1/2/3/4/5/6/7/8
} SIE_BURST_LENGTH;


typedef struct { // would be removed soon
	SIE_DRAM_IN0_BURST_SEL   DramIn0Burst;
	SIE_DRAM_IN1_BURST_SEL   DramIn1Burst;
	SIE_DRAM_IN2_BURST_SEL   DramIn2Burst;
	SIE_DRAM_OUT0_BURST_SEL  DramOut0Burst;
	SIE_DRAM_OUT1_BURST_SEL  DramOut1Burst;
	SIE_DRAM_OUT2_BURST_SEL  DramOut2Burst;
	UINT32                   uiDramOut0BurstIntvl;
} SIE_DRAM_BURST_INFO;// would be removed soon

typedef struct {
	BOOL            bOut0HFlip;     ///<
	BOOL            bOut0VFlip;     ///<
	BOOL            bOut1HFlip;     ///<
	BOOL            bOut1VFlip;     ///<
	BOOL            bOut2HFlip;     ///<
	BOOL            bOut2VFlip;     ///<
	BOOL            bOut3HFlip;     ///<
	BOOL            bOut3VFlip;     ///<
} SIE_FLIP_INFO;

typedef struct {
	SIE_DVI_FORMAT_SEL   DviFormat;
	SIE_DVI_IN_MODE_SEL  DviInMode;
	SIE_DVI_OUT_SWAP_SEL DviOutSwap;
	BOOL             bHdInSwap;
	BOOL             bOutSplit;
	BOOL             bFieldEn;
	BOOL             bFieldSel;
	BOOL             bCCIR656VdSel;
	UINT8            uiDataPeriod;
	UINT8            uiDataIdx;
	BOOL             bAutoAlign;
	BOOL             bOutYUV420;
} SIE_DVI_INFO;

typedef struct {
	SIE_PATGEN_SEL PatGenMode;
	UINT32          uiPatGenVal;
} SIE_PATGEN_INFO;

typedef struct {
	UINT32 VdIntervLowerThr;
	UINT32 VdIntervUpperThr;
	UINT32 HdIntervLowerThr;
	UINT32 HdIntervUpperThr;
} SIE_VDHD_INTERVAL_INFO;

typedef struct {
	UINT32      uiStX;    ///< window horizontal start
	UINT32      uiStY;    ///< window vertical start
	UINT32      uiSzX;    ///< window horizontal size
	UINT32      uiSzY;    ///< window vertical size
	UINT32      uiDivX;   ///< window horizontal subsample
	UINT32      uiThres;  ///< threshold
	UINT16      uiSubRatio;///< OB Detect Result Subtraction Ratio
} SIE_OB_DT_INFO;

typedef struct {
	UINT32      uiAvg;  ///<
	UINT32      uiCnt;  ///<
} SIE_OB_DT_RSLT_INFO;


typedef struct {
	UINT16      uiObOfs;
} SIE_OB_OFS_INFO;

typedef struct {
	SIE_DEF_FACT_SEL    DefFact;            ///<
	UINT8               DpcCfapat;
} SIE_DPC_INFO;

typedef struct {
	UINT16          *puiGmaTbl;   ///< 33x12taps gamma table
} SIE_MAIN_GAMMA_INFO;

typedef struct {
	BOOL                bDthrEn;        ///< vignette dithering enable
	BOOL                bDthrRst;       ///< vignette dithering reset
	UINT32              uiDthrLvl;      ///< vignette dithering level; 0x0=bit[1:0], 0x7=bit[8:7]
	SIE_ECS_MSIZE_SEL     MapSizeSel;     ///<
	UINT32              uiMapShift;     ///< multiplication shift bit number
	UINT32              uiHSclFctr;       ///< map horizontal scaling factor
	UINT32              uiVSclFctr;       ///< map vertical scaling factor
} SIE_ECS_INFO;

typedef struct {
	UINT32              uiActSzX;
	UINT32              uiActSzY;
} SIE_ECS_ADJ_INFO;

typedef struct {
	UINT32              uiGainIn8P8Bit;   ///< digital gain in 8.8 bits format
} SIE_DGAIN_INFO;

typedef struct {
	UINT32 uiDecompKp[11];
	UINT32 uiDecompSp[12];
	UINT32 uiDecompSb[12];
} SIE_DECOMPANDING_INFO;

typedef struct {
	UINT32 uiCompKp[11];
	UINT32 uiCompSp[12];
	UINT32 uiCompSb[12];
} SIE_COMPANDING_INFO;

typedef struct {
	BOOL            bSrcIntpV;///< vertical interpolation, only for input width<=3264
	UINT32          uiIv;     ///< bayer scaling - integration value
	UINT32          uiSv;     ///< bayer scaling - skipping value
	UINT32          uiBvR;    ///< bayer scaling - begin value for R/Gb channel
	UINT32          uiBvB;    ///< bayer scaling - begin value for B/Gr channel
	UINT32          uiOutSize;///< bayer scaling - output size
	UINT32          uiDivM[2];///< bayer scaling - division parameter M
	UINT32          uiDivS[2];///< bayer scaling - division parameter S
} SIE_BS_H_INFO;


typedef struct {
	UINT32 uiInSz;          ///< input pixel number
	UINT32 uiOutSz;         ///< output pixel number
	UINT32 uiLpf;           ///< low-pass valumn, 0~100
	UINT32 uiBinPwr;        ///< binning, 0~2
	BOOL   bAdaptiveLpf;    ///< 0: LPF is absolute value; 1: LPF is adaptive value (suggestion: 1)
} SIE_BS_H_ADJ_INFO;

typedef struct {
	UINT32          uiIv;     ///< bayer scaling - integration value
	UINT32          uiSv;     ///< bayer scaling - skipping value
	UINT32          uiBvR;    ///< bayer scaling - begin value for R/Gb channel
	UINT32          uiBvB;    ///< bayer scaling - begin value for B/Gr channel
	UINT32          uiOutSize;///< bayer scaling - output size
	UINT32          uiDivM[2];///< bayer scaling - division parameter M
	UINT32          uiDivS[2];///< bayer scaling - division parameter S
	UINT32          uiInShft; ///< bayer scaling - input shift
	UINT32          uiDivMRb[16];///< bayer scaling - division parameter M for R/B
	UINT32          uiDivSRb[16];///< bayer scaling - division parameter S for R/B
} SIE_BS_V_INFO;


typedef struct {
	UINT32 uiInSz;          ///< input pixel number
	UINT32 uiOutSz;         ///< output pixel number
	UINT32 uiLpf;           ///< low-pass valumn, 0~100
	UINT32 uiBinPwr;        ///< binning, 0~2
	BOOL   bAdaptiveLpf;    ///< 0: LPF is absolute value; 1: LPF is adaptive value (suggestion: 1)
} SIE_BS_V_ADJ_INFO;

typedef struct {
	UINT16 *puiBufR;
	UINT16 *puiBufG;
	UINT16 *puiBufB;
	UINT16 *puiBufIR;
	UINT16 *puiAccCnt;
} SIE_STCS_CA_RSLT_INFO;

typedef struct {
	UINT16              uiRGain;    ///< Color Gain, R Gain
	UINT16              uiGrGain;    ///< Color Gain, Gr Gain
	UINT16              uiGbGain;    ///< Color Gain, Gb Gain
	UINT16              uiBGain;    ///< Color Gain, B Gain
	UINT16              uiIrGain;    ///< Color Gain, B Gain
	BOOL                bGainSel;
} SIE_CG_INFO;

typedef struct {
    UINT32              uiBrcSegBitNo;      ///< Raw encoder - BRC target encoded bit number per segment
	BOOL                bBccGammaEn;
	UINT32              *p_uiBccGammaTbl;         ///< assign a UINT32 [GAM_TBL_TAP] array for BCC gamma table
	UINT32              uiDctTblIdx[8];
	UINT32              uiDctThr[4];
} SIE_BCC_PARAM_INFO;

typedef struct {
	BCC_BST_SUP_FMT     BstSupFmt;
	UINT32              uiCompRate;         ///< Raw encoder - compression rate, in %, range 40~60%
} SIE_BCC_ADJ_INFO;


typedef struct {
	UINT32            uiOvflLine;
	UINT16            uiLineCntOfMostLfn[8];
} SIE_BCC_RSLT_INFO;

typedef struct {
	BOOL        bVigEn;        ///<
	BOOL        bLaCgEn;       ///<
	BOOL        bLaGma1En;     ///<
	BOOL        bVaGma2En;     ///<
	SIE_HISTO_SRC_SEL   HistoSrcSel;    ///<
	BOOL        bCaThEn;       ///<
	BOOL        bVaCgEn;       ///<
	SIE_STCS_LA1_SRC_SEL La1SrcSel;   ///<
	SIE_VA_OUT_SEL      VaOutSel;
	BOOL        bLaThEn;
	SIE_STCS_LA_RGB2Y1_MOD_SEL LaRgb2Y1Mod;
	SIE_STCS_LA_RGB2Y2_MOD_SEL LaRgb2Y2Mod;
	SIE_CA_SRC_SEL        bCaAccmSrc;
	UINT16      Companding_shift;
} SIE_STCS_PATH_INFO;


typedef struct {
	UINT32 uiVigX;      ///< vig compensation - x center
	UINT32 uiVigY;      ///< vig compensation - y center
	UINT8  uiVigT; //would be modified as UINT32 soon        ///< vig compensation - vignette disabled area
	UINT8  uiVigTabGain;  ///< vig compensation - vignette gain factor
	UINT32 uiVigXDiv;    ///< vig compensation - vignette scaling factor in x direction. unsigned 6.6
	UINT32 uiVigYDiv;    ///< vig compensation - vignette scaling factor in y direction. unsigned 6.6
	UINT32 *puiVigLut;   ///< vig compensation - vignette LUT
} SIE_VIG_INFO;

typedef struct {
	UINT32              uiImgXSize;     ///< vig compensation - image horizontal size (ex: SIE Crop SIze)
	UINT32              uiImgYSize;     ///< vig compensation - image vertical size (ex: SIE Crop SIze)
	UINT32              uiImgXCntr;     ///< vig compensation - image horizontal center (ex: half SIE Crop SIze)
	UINT32              uiImgYCntr;     ///< vig compensation - image vertical center (ex: half SIE Crop SIze)
	UINT32              uiImgXratio;    ///< vig compensation - image horizontalratio (16 from 16:9) (ex: SIE Crop SIze, if the image is not H/V unequally scaled)
	UINT32              uiImgYratio;    ///< vig compensation - image vertical ratio (9 from 16:9) (ex: SIE Crop SIze, if the image is not H/V unequally scaled)
} SIE_VIG_ADJ_INFO;

typedef struct {
	UINT32              uiCaliImgSizeX;     ///< vig compensation - calibration image size X, in pixel
	UINT32              uiCaliImgSizeY;     ///< vig compensation - calibration image size Y, in pixel
	UINT32              uiCaliImgVigCntrX;  ///< vig compensation - calibration image center X, in pixel
	UINT32              uiCaliImgVigCntrY;  ///< vig compensation - calibration image center Y, in pixel
	UINT32              uiCaliImgRatioX;    ///< vig compensation - calibration image X:Y Ratio (ex: 16 for 16:9)
	UINT32              uiCaliImgRatioY;    ///< vig compensation - calibration image Y:Y Ratio (ex: 9  for 16:9)
	UINT32              uiCorrImgSizeX;     ///< vig compensation - correction  image size X, in pixel
	UINT32              uiCorrImgSizeY;     ///< vig compensation - correction  image size Y, in pixel
	UINT32              uiCorrImgSclRatioX; ///< vig compensation - correction  image Scaling (down) ratio X, in 1/1000 (ex: 500 for 1/2 scaling down)
	UINT32              uiCorrImgSclRatioY; ///< vig compensation - correction  image Scaling (down) ratio Y, in 1/1000 (ex: 500 for 1/2 scaling down)
	UINT32              uiCorrImgPosRatioX; ///< useless
	UINT32              uiCorrImgPosRatioY; ///< useless
} SIE_VIG_ADJ_2IMG_INFO;

typedef struct {
	UINT32              uiCaIrSubRWet;    ///< IR weight for subtraction, R channel
	UINT32              uiCaIrSubGWet;    ///< IR weight for subtraction, G channel
	UINT32              uiCaIrSubBWet;    ///< IR weight for subtraction, B channel
} SIE_CA_IRSUB_INFO;

typedef struct {
	UINT32              uiLaIrSubRWet;    ///< IR weight for subtraction, R channel
	UINT32              uiLaIrSubGWet;    ///< IR weight for subtraction, G channel
	UINT32              uiLaIrSubBWet;    ///< IR weight for subtraction, B channel
} SIE_LA_IRSUB_INFO;

typedef struct {
	UINT16              uiCaObOfs;      ///< OB ofs for CA
	UINT16              uiLaObOfs;      ///< OB ofs for LA
} SIE_STCS_OB_INFO;

typedef struct {
	UINT32              uiWinNmX;    ///< color accumulation - horizontal window number
	UINT32              uiWinNmY;    ///< color accumulation - vertical window number
	UINT32              uiWinSzX;    ///< color accumulation - horizontal window size
	UINT32              uiWinSzY;    ///< color accumulation - vertical window size
} SIE_CA_WIN_INFO;

typedef struct {
	UINT32              uiWinNmX;    ///< color accumulation - horizontal window number
	UINT32              uiWinNmY;    ///< color accumulation - vertical window number
	UINT32              uiWinSzX;    ///< color accumulation - horizontal window size
	UINT32              uiWinSzY;    ///< color accumulation - vertical window size
} SIE_LA_WIN_INFO;

typedef struct {
	UINT32              uiLaThY1Lower;
	UINT32              uiLaThY1Upper;
	UINT32              uiLaThY2Lower;
	UINT32              uiLaThY2Upper;
} SIE_LA_TH;

typedef struct {
	UINT16              uiGThLower;    ///<
	UINT16              uiGThUpper;    ///<
	UINT16              uiRGThLower;    ///<
	UINT16              uiRGThUpper;    ///<
	UINT16              uiBGThLower;    ///<
	UINT16              uiBGThUpper;    ///<
	UINT16              uiPGThLower;    ///<
	UINT16              uiPGThUpper;    ///<
} SIE_CA_TH_INFO;

typedef struct {
	UINT16              uiRGain;    ///< R  Gain
	UINT16              uiGGain;    ///< G  Gain
	UINT16              uiBGain;    ///< B  Gain
	UINT16              uiIRGain;   ///< IR Gain
} SIE_LA_CG_INFO;

typedef struct {
	UINT16              *puiHist;    ///<
} SIE_HISTO_RSLT_INFO;


typedef struct {
	UINT16          *puiGmaTbl;   ///<
} SIE_LA_GMA_INFO;


typedef struct {
	SIE_CA_CROP_INFO    StcsCaCrpInfo;
	SIE_CA_SCAL_INFO    StcsCaSmplInfo;
	SIE_CA_IRSUB_INFO   StcsCaIrSubInfo;
	SIE_LA_IRSUB_INFO   StcsLaIrSubInfo;
	SIE_CA_WIN_INFO     StcsCaWinInfo;
	SIE_LA_CROP_INFO    StcsLaCrpInfo;
	SIE_LA_WIN_INFO     StcsLaWinInfo;
	SIE_STCS_OB_INFO    StcsObInfo;
	SIE_LA_TH           StcsLaThInfo;
} SIE_STCS_CALASIZE_GRP_INFO;

typedef struct {
	UINT32      uiCaRoiStX;
	UINT32      uiCaRoiStY;
	UINT32      uiCaRoiSzX;
	UINT32      uiCaRoiSzY;
	UINT32      uiCaWinNmX;
	UINT32      uiCaWinNmY;
	UINT32      uiLaRoiStX;
	UINT32      uiLaRoiStY;
	UINT32      uiLaRoiSzX;
	UINT32      uiLaRoiSzY;
	UINT32      uiLaWinNmX;
	UINT32      uiLaWinNmY;
} SIE_STCS_CALASIZE_ADJ_INFO;


/**
    SIE Open Object.

    Open Object is used for opening SIE module.
    \n Used for sie_open()

*/
//@{
typedef struct _SIE_OPENOBJ {
	void (*pfSieIsrCb)(UINT32 uiIntpStatus); ///< isr callback function
	UINT32 uiSieClockRate;  ///< SIE clock rate in Hz; configure '0' to skip set-up
	SIE_CLKSRC_SEL SieClkSel;   ///< SIE clock source selection
	SIE_PXCLKSRC   PxClkSel;    ///<  SIE pixel-clock source selection; valid in Parallel(Master/Slave) modes, invalid in Serial mode
} SIE_OPENOBJ;
//@}

/**
    SIE Open Object.

    Open Object is used for opening SIE module.
    \n Used for sie_open()
*/
//@{
typedef struct _SIE2_OPENOBJ {
	void (*pfSie2IsrCb)(UINT32 uiIntpStatus); ///< isr callback function
	UINT32 uiSie2ClockRate;  ///< SIE clock rate in Hz; configure '0' to skip set-up
	SIE_CLKSRC_SEL Sie2ClkSel;   ///< SIE clock source selection
	SIE_PXCLKSRC   PxClkSel;    ///<  SIE pixel-clock source selection; valid in Parallel(Master/Slave) modes, invalid in Serial mode
} SIE2_OPENOBJ;
//@}


typedef struct {
	UINT32            uiDpTblAddr;///< dp table address
	UINT32            uiDpTblSize;///< dp table Size, in bytes
} SIE_CHG_DP_INFO;

typedef struct {
	UINT32                   uiIn0Addr; ///< SIE input ch0 dram addr; "zero value" for null configuration
	UINT32                   uiIn1Addr; ///< SIE input ch1 dram addr; "zero value" for null configuration
	UINT32                   uiIn2Addr; ///< SIE input ch2 dram addr; "zero value" for null configuration
	UINT32                   uiIn0Size; ///< SIE input ch0 dram size, in bytes, for cache-handle; "zero value" for null configuration
	UINT32                   uiIn1Size; ///< SIE input ch1 dram size, in bytes, for cache-handle; "zero value" for null configuration
	UINT32                   uiIn2Size; ///< SIE input ch2 dram size, in bytes, for cache-handle; "zero value" for null configuration
} SIE_CHG_IN_ADDR_INFO;

typedef struct {
	UINT32                   uiOut0Addr; ///< SIE output ch0 dram addr; "zero value" for null configuration
	UINT32                   uiOut1Addr; ///< SIE output ch1 dram addr; "zero value" for null configuration
	UINT32                   uiOut2Addr; ///< SIE output ch2 dram addr; "zero value" for null configuration
	UINT32                   uiOut0Size; ///< SIE output ch0 dram size, in bytes, for cache-handle; "zero value" for null configuration
	UINT32                   uiOut1Size; ///< SIE output ch1 dram size, in bytes, for cache-handle; "zero value" for null configuration
	UINT32                   uiOut2Size; ///< SIE output ch2 dram size, in bytes, for cache-handle; "zero value" for null configuration
} SIE_CHG_OUT_ADDR_INFO;


typedef struct {
	UINT32                   uiOut0Lofs; ///< SIE output ch0 dram line-offset, in bytes; "zero value" for null configuration
	UINT32                   uiOut1Lofs; ///< SIE output ch1 dram line-offset, in bytes; "zero value" for null configuration
	UINT32                   uiOut2Lofs; ///< SIE output ch2 dram line-offset, in bytes; "zero value" for null configuration
} SIE_CHG_IO_LOFS_INFO;

typedef struct {
	BOOL                    bOut0ChgEn; ///< enable to change output channel 0 source
	SIE_OUT0_SRC_SEL        Out0Src;    ///< output channel 0 source, only effective when bOut0ChgEn==1
	BOOL                    bOut3ChgEn; ///< enable to change output channel 3 source
	SIE_OUT3_SRC_SEL        Out3Src;    ///< output channel 3 source, only effective when bOut0ChgEn==1
	BOOL                    bOut4ChgEn; ///< enable to change output channel 3 source
	SIE_OUT4_SRC_SEL        Out4Src;    ///< output channel 3 source, only effective when bOut0ChgEn==1
} SIE_CHG_OUT_SRC_INFO;

typedef struct {
	BOOL                    ringbufEn;
	UINT32                  ringbufLen;
} SIE_CHG_RINGBUF_INFO;

typedef struct {
	BOOL                    pxclkIn;         ///< check pixel clock in,  for parallel sensor debug only
	BOOL                    sieclkIn;        ///< check sie module clock in
	UINT32                  uiLineCnt;       ///< line count, the line count is multiply by 16 for NT98520. For NT98528 the max line count will be returned.
	UINT32                  uiMaxPixelCnt;   ///< Max pixel count, it's valid only for NT98528
	UINT32                  uiCsiHdCnt;      ///< Hd count from CSI
	UINT32                  uiCsiLineEdCnt;  ///< Line end count from CSI
	UINT32                  uiCsiPixCnt;     ///< Pixel count of each line from CSI
	UINT32                  uiVdpxclkCnt;
	UINT32                  uiHdpxclkCnt;
	UINT32                  uiDvsTimestamp;
} SIE_SYS_DEBUG_INFO;

typedef struct {
	UINT8       *pbMdThresRslt;
	UINT16      uiBlkDiffCnt;
	UINT32      uiTotalBlkDiff;
} SIE_MD_RESULT_INFO;


#if 0 // unused struct or enum
typedef enum { // would be removed soon
	OUT3_BURST_LENGTH_64   = 0,    ///< SIE 1-4 Dram input channel 3 burst length = 64  words
	OUT3_BURST_LENGTH_32   = 1,    ///< SIE 1-4 Dram input channel 3 burst length = 32  words
	OUT3_BURST_LENGTH_96   = 2,    ///< SIE 1   Dram input channel 3 burst length = 96  words
	OUT3_BURST_LENGTH_128  = 3,    ///< SIE 1   Dram input channel 3 burst length = 128 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT3_BURST_SEL)
} SIE_DRAM_OUT3_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	OUT4_BURST_LENGTH_64   = 0,    ///< SIE 1 Dram input channel 4 burst length = 64 words
	OUT4_BURST_LENGTH_32   = 1,    ///< SIE 1 Dram input channel 4 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT4_BURST_SEL)
} SIE_DRAM_OUT4_BURST_SEL;// would be removed soon

typedef enum { // would be removed soon
	OUT5_BURST_LENGTH_64   = 0,    ///< SIE 1-4 Dram input channel 5 burst length = 64 words
	OUT5_BURST_LENGTH_32   = 1,    ///< SIE 1-4 Dram input channel 5 burst length = 32 words
	ENUM_DUMMY4WORD(SIE_DRAM_OUT5_BURST_SEL)
} SIE_DRAM_OUT5_BURST_SEL;// would be removed soon

typedef struct {
	UINT16      uiStX; ///< horizontal start
	UINT16      uiStY; ///< vertical start
	UINT32      uiSzX; ///< horizontal size
	UINT32      uiSzY; ///< vertical size
	SIE_CFAPAT_SEL  CfaPat;///< CFA start pattern
} SIE_VA_CROP_INFO;

typedef struct {
	UINT16      uiFctrX; ///< horizontal scaling factor
	UINT16      uiFctrY; ///< vertical scaling output size
} SIE_YOUT_SCAL_INFO;

typedef struct {
	UINT8               uiWinNumX;   ///< Y accumulation - horizontal window number
	UINT8               uiWinNumY;   ///< Y accumulation - horizontal window number
	UINT32              uiWinSzX;    ///< Y accumulation - horizontal window size
	UINT32              uiWinSzY;    ///< Y accumulation - vertical window size
} SIE_YOUT_WIN_INFO;

typedef struct {
	UINT8               uiAccmShft;  ///< Y accumulation - output division shift
	UINT8               uiAccmGain;  ///< Y accumulation - output division gain
} SIE_YOUT_ACCM_INFO;

typedef struct {
	UINT16              uiRGain;    ///< Color gain, R gain
	UINT16              uiGGain;    ///< Color gain, G gain
	UINT16              uiBGain;    ///< Color gain, B gain
	BOOL                uiGainSel;  ///< Color gain selection
} SIE_YOUT_CG_INFO;

typedef struct {
	UINT32           uiAddr;            ///< address
	SIE_OUT3_SRC_SEL OutSrc;            ///< source
	UINT8            uiPingPongBufNum;  ///< ping-pong buffer
	UINT32           uiLofs;            ///< line-offset
	SIE_PACKBUS_SEL  PackBus;            ///< pack bus bit number
	UINT32           uiFramOfset;       ///< Frame offset
	BOOL             bHFlip;           ///< H flip
	BOOL             bVFlip;           ///< V flip
} SIE_DRAM_OUT3_INFO;

typedef struct {
	UINT32           uiAddr;            ///< address
	SIE_OUT4_SRC_SEL OutSrc;            ///< source
	UINT8            uiPingPongBufNum;  ///< ping-pong buffer
	UINT32           uiLofs;            ///< line-offset
	UINT32           uiFramOfset;       ///< Frame offset
} SIE_DRAM_OUT4_INFO;

typedef struct {
	UINT32           uiAddr;            ///< address
	UINT8            uiPingPongBufNum;  ///< ping-pong buffer
	UINT32           uiLofs;            ///< line-offset
	UINT32           uiFramOfset;       ///< Frame offset
} SIE_DRAM_OUT5_INFO;

typedef struct {
	UINT32      uiTopX;       ///< top windows horizontal position (also for bottom windows)
	UINT32      uiTopY;       ///< top windows vertical position
	UINT32      uiTopIntvl;   ///< top windows horizontal interval (also for bottom windows)
	UINT32      uiThresold;      ///< threshold for all windows
	UINT32      uiNormFctr;   ///< normalization factor
	UINT32      uiLeftX;      ///< left windows horizontal position
	UINT32      uiLeftY;      ///< left windows vertical position (also for right windows)
	UINT32      uiLeftIntvl;  ///< left windows horizontal interval (also for right windows)
	UINT32      uiBotY;       ///< bottom windows vertical position
	UINT32      uiRightX;     ///< right windows horizontal position
} SIE_OB_FRAME_INFO;


typedef struct {
	UINT32      uiObfCnt;
	UINT8       uiTopRslt[8];      ///< top window result
	UINT8       uiLeftRslt[8];     ///< left window result
	UINT8       uiBotRslt[8];      ///< bottom window result
	UINT8       uiRightRslt[8];    ///< right window result
} SIE_OB_FRAME_RSLT_INFO;


typedef struct {
	UINT32      uiHSclFctr;       ///< horizontal scaling factor
	UINT32      uiVSclFctr;       ///< vertical scaling factor
	UINT32      uiShift;      ///< left shift bit number
	UINT8       *puiObpTbl;       ///< table (size=17x17=289)
} SIE_OB_PLANE_INFO;


typedef struct {
	UINT32              uiActSzX;
	UINT32              uiActSzY;
} SIE_OB_PLANE_ADJ_INFO;

typedef struct {
	UINT32              uiWinNmX;    ///< color accumulation - horizontal window number
	UINT32              uiWinNmY;    ///< color accumulation - vertical window number
	UINT32              uiWinSzX;    ///< color accumulation - horizontal window size
	UINT32              uiWinSzY;    ///< color accumulation - vertical window size
	UINT32              uiWinSpX;    ///< color accumulation - horizontal window spacing
	UINT32              uiWinSpY;    ///< color accumulation - vertical window spacing
} SIE_VA_WIN_INFO;

typedef struct {
	UINT8              uiLaRgb2yWet;
	UINT8              uiVaRgbIr2ySel;
} SIE_IR_INFO;

typedef struct {
	UINT8              uiVaRccb2ySel;
} SIE_RCCB_INFO;

typedef struct {
	SIE_YOUT_SCAL_INFO  YoutScalInfo;
	SIE_YOUT_WIN_INFO   YoutWinInfo;
	SIE_YOUT_ACCM_INFO  YoutAccmInfo;
	SIE_YOUT_CG_INFO    YoutCGInfo;
} SIE_YOUT_INFO;

typedef struct {
	UINT32              uiGridVal0;    ///< Grid line Value0
	UINT32              uiGridVal1;    ///< Grid line Value1
	UINT32              uiGridVal2;    ///< Grid line Value2
	UINT32              uiGridVal3;    ///< Grid line Value3
	UINT32              uiShftBs;
} SIE_GRIDLINE_INFO;

typedef struct {
	UINT16              uiGain0;    ///< Color gain, gain0
	UINT16              uiGain1;    ///< Color gain, gain1
	UINT16              uiGain2;    ///< Color gain, gain2
	UINT16              uiGain3;    ///< Color gain, gain3
} SIE_VA_CG_INFO;

typedef struct {
	UINT16          *puiGmaTbl;   ///<
} SIE_VA_GMA_INFO;


typedef struct {
	UINT8                   uiTapA;     ///<
	INT8                    iTapB;      ///<
	INT8                    iTapC;      ///<
	INT8                    iTapD;      ///<
	SIE_STCS_VA_FLTR_SYMM_SEL   FltrSymm;   ///<
	SIE_STCS_VA_FLTR_SIZE_SEL   FltrSize;   ///<
	UINT8                   uiDiv;      ///<
	UINT16                  uiThL;      ///<
	UINT16                  uiThH;      ///<
} SIE_VA_FLTR_INFO;

typedef struct {
	SIE_VA_FLTR_INFO        FltrH;
	SIE_VA_FLTR_INFO        FltrV;
	BOOL                    bHExtend;   ///<
	BOOL                    bLineMax;   ///<
} SIE_VA_FLTR_GROUP_INFO;


typedef struct {
	BOOL                bEn;
	UINT32              uiWinStX;    ///< horizontal window start
	UINT32              uiWinStY;    ///< vertical window start
	UINT32              uiWinSzX;    ///< horizontal window size
	UINT32              uiWinSzY;    ///< vertical window size
	BOOL                bLineMaxG1;
	BOOL                bLineMaxG2;
} SIE_VA_INDEP_WIN_INFO;

typedef struct {
	UINT32              uiVaG1H;
	UINT32              uiVaG1V;
	UINT32              uiVaG2H;
	UINT32              uiVaG2V;
} SIE_VA_INDEP_WIN_RSLT_INFO;

typedef struct {
	UINT16              uiEthLow;    ///< horizontal window start
	UINT16              uiEthMid;    ///< horizontal window start
	UINT16              uiEthHigh;    ///< horizontal window start
	BOOL                bEthOutFmt;
	BOOL                bEthOutSel;
	BOOL                bEthHOutSel;
	BOOL                bEthVOutSel;
} SIE_ETH_INFO;

typedef struct {
	SIE_YOUT_SCAL_INFO  YoutScalInfo;
	SIE_YOUT_ACCM_INFO  YoutAccmInfo;
	SIE_YOUT_WIN_INFO   YoutWinInfo;
} SIE_YOUT_GRP_INFO;

typedef struct {
	SIE_PFPC_MODE_SEL   PfpcMode;       ///< period mode
	UINT8           uiGainTbl[32];  ///< gain table
	UINT8           uiOfsTbl[32];   ///< offset table
} SIE_PFPC_INFO;

typedef struct {
	UINT32      uiYoutWinNmX;
	UINT32      uiYoutWinNmY;
	UINT32      uiCropWinSzX;
	UINT32      uiCropWinSzY;
} SIE_YOUT_ADJ_INFO;

typedef struct {
	UINT8      uiDramOut0CurID;
	UINT8      uiDramOut1CurID;
	UINT8      uiDramOut2CurID;
	UINT8      uiDramOut3CurID;
	UINT8      uiDramOut4CurID;
	UINT8      uiDramOut5CurID;
} SIE_DRAM_CURR_BUF_ID;

#endif

#endif//_SIEP_LIB_H
//@}

//@}


