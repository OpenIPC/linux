

#ifndef __NC_RAPTOR5_FMT_H__
#define __NC_RAPTOR5_FMT_H__

typedef unsigned char           NC_U8;
typedef unsigned short          NC_U16;
typedef unsigned int            NC_U32;

typedef signed char             NC_S8;
typedef short                   NC_S16;
typedef int                     NC_S32;


typedef enum NC_DECODER_CHIP_MODEL_E
{
	NC_DECODER_6188	 = 0xd0,
	NC_DECODER_6188C = 0xd2,
	NC_DECODER_6188D = 0xd3,
} NC_DECODER_CHIP_MODEL_E;

typedef enum NC_VIDEO_SET_MODE_E
{
	UNKNOWN_Mode = 0,

	NC_VIDEO_SET_MODE_AUTO = 1,
	NC_VIDEO_SET_MODE_MANUAL,

	NC_VIDEO_SET_MODE_MAX,

} NC_VIDEO_SET_MODE_E;

typedef enum NC_CHN_INTV_E
{
	NC_DPHY_1 = 0,
	NC_DPHY_2,
	NC_DPHY_2_4MUX_6168C,

} NC_CHN_INTV_E;

typedef enum NC_DECODER_AUTO_OP_MODE_E
{
	NC_AUTO_OP_MODE_NORMAL = 0,
	NC_AUTO_OP_MODE_2MUX_NRT,
	NC_AUTO_OP_MODE_4NUX_NRT,
} NC_DECODER_AUTO_OP_MODE_E;

typedef enum NC_VIDEO_COLOR_TUNE_E
{
	NC_VIDEO_BRIGHTNESS = 1,
	NC_VIDEO_CONTRAST,
	NC_VIDEO_HUE,
	NC_VIDEO_SATURATION,
	NC_VIDEO_H_DELAY,

} NC_VIDEO_COLOR_TUNE_E;

typedef enum NC_FORMAT_STANDARD_E
{
	FMT_STD_UNKNOWN = 0,

	FMT_SD = 1,
	FMT_AHD,
	FMT_CVI,
	FMT_TVI,

	FMT_STD_MAX,

} NC_FORMAT_STANDARD_E;


typedef enum NC_FORMAT_RESOLUTION_E
{
	FMT_RESOL_UNKNOWN = 0,

	FMT_H720 = 1,
	FMT_H960,
	FMT_SH720,
	FMT_H1280,
	FMT_H1440,
	FMT_H960_EX,
	FMT_H960_2EX,
	FMT_H960_Btype_2EX,
	FMT_720P,
	FMT_720P_EX,
	FMT_720P_EX_CIF,
	FMT_720P_Btype,
	FMT_720P_Btype_EX,
	FMT_720P_Btype_2EX,
	FMT_960P,
	FMT_1080P,
	FMT_1080P_EX,
	FMT_1080P_EX_CIF,
	FMT_1080P_CIF,
	FMT_3M,
	FMT_4M,
	FMT_5M,
	FMT_5_3M,
	FMT_6M,
	FMT_8M,
	FMT_8M_CIF,

	FMT_RESOL_MAX,

} NC_FORMAT_RESOLUTION_E;

typedef enum NC_FORMAT_FPS_E
{
	FMT_FPS_UNKNOWN = 0,

	FMT_PAL = 1,
	FMT_NT,
	FMT_7_5P,
	FMT_12_5P,
	FMT_15P,
	FMT_15P_NEW,
	FMT_18P,
	FMT_18_75P,
	FMT_20P,
	FMT_25P,
	FMT_30P,
	FMT_50P,
	FMT_60P,

	FMT_FPS_MAX,

} NC_FORMAT_FPS_E;

typedef enum NC_VIVO_CH_FORMATDEF_E
{
	NC_VIVO_CH_FORMATDEF_UNKNOWN = 0,

	SD_H720_NT = 1,
	SD_H720_PAL,
	SD_H960_NT,
	SD_H960_PAL,
	SD_SH720_NT,
	SD_SH720_PAL,
	SD_H1280_NT,
	SD_H1280_PAL,
	SD_H1440_NT,
	SD_H1440_PAL,
	SD_H960_EX_NT,
	SD_H960_EX_PAL,
	SD_H960_2EX_NT,
	SD_H960_2EX_PAL,
	SD_H960_2EX_Btype_NT,
	SD_H960_2EX_Btype_PAL,

	AHD_720P_12_5P_EX_Btype,
	AHD_720P_15P_EX_Btype,
	AHD_720P_25P,
	AHD_720P_30P,
	AHD_720P_25P_EX,
	AHD_720P_30P_EX,
	AHD_720P_25P_EX_Btype,
	AHD_720P_30P_EX_Btype,
	AHD_720P_50P,
	AHD_720P_60P,
	AHD_960P_25P,
	AHD_960P_30P,
	AHD_960P_50P,
	AHD_960P_60P,

	AHD_1080P_12_5P,
	AHD_1080P_15P,
	AHD_1080P_25P,
	AHD_1080P_30P,
	AHD_1080P_50P,
	AHD_1080P_60P,

	AHD_3M_18P,
	AHD_3M_25P,
	AHD_3M_30P,
	AHD_4M_15P,
	AHD_4M_25P,
	AHD_4M_30P,
	AHD_5M_12_5P,
	AHD_5M_20P,
	AHD_5_3M_20P,
	AHD_6M_18P,
	AHD_6M_20P,
	AHD_8M_7_5P,
	AHD_8M_12_5P,
	AHD_8M_15P,
	AHD_8M_CIF_25P,
	AHD_8M_CIF_30P,
	AHD_8M_15P_NEW,

	CVI_HD_25P,
	CVI_HD_30P,
	CVI_HD_25P_EX,
	CVI_HD_30P_EX,
	CVI_HD_50P,
	CVI_HD_60P,
	CVI_FHD_25P,
	CVI_FHD_30P,
	CVI_4M_25P,
	CVI_4M_30P,
	CVI_5M_20P,
	CVI_8M_12_5P,
	CVI_8M_15P,

	TVI_HD_25P,
	TVI_HD_30P,
	TVI_HD_25P_EX,
	TVI_HD_30P_EX,
	TVI_HD_B_25P,
	TVI_HD_B_30P,
	TVI_HD_B_25P_EX,
	TVI_HD_B_30P_EX,
	TVI_HD_50P,
	TVI_HD_60P,
	TVI_960P_25P,
	TVI_960P_30P,
	TVI_960P_50P,
	TVI_960P_60P,
	TVI_FHD_25P,
	TVI_FHD_30P,
	TVI_FHD_50P,
	TVI_FHD_60P,
	TVI_3M_18P,
	TVI_4M_15P,
	TVI_4M_25P,
	TVI_4M_30P,
	TVI_5M_12_5P,
	TVI_5M_20P,
	TVI_8M_12_5P,
	TVI_8M_15P,

	NC_VI_SIGNAL_ON,
	NC_VI_SIGNAL_OFF,

	NC_VIVO_CH_FORMATDEF_MAX,

} NC_VIVO_CH_FORMATDEF_E;

typedef enum NC_AD_MODE_E
{
	NC_AD_AI = 0,
	NC_AD_AOC,

} NC_AD_MODE_E;

typedef enum NC_AD_SAMPLE_RATE_E
{
    NC_AD_SAMPLE_RATE_8000   = 8000,    /* 8K 	   sample_rate*/
	NC_AD_SAMPLE_RATE_12000  = 12000,   /* 12K 	   sample_rate*/
	NC_AD_SAMPLE_RATE_11025  = 11025,   /* 11.025K sample_rate*/
	NC_AD_SAMPLE_RATE_16000  = 16000,   /* 16K 	   sample_rate*/
	NC_AD_SAMPLE_RATE_22050  = 22050,   /* 22.050K sample_rate*/
	NC_AD_SAMPLE_RATE_24000  = 24000,   /* 24K 	   sample_rate*/
	NC_AD_SAMPLE_RATE_32000  = 32000,   /* 32K 	   sample_rate*/
	NC_AD_SAMPLE_RATE_44100  = 44100,   /* 44.1K   sample_rate*/
	NC_AD_SAMPLE_RATE_48000  = 48000,   /* 48K     sample_rate*/
	NC_AD_SAMPLE_RATE_BUTT,
} NC_AD_SAMPLE_RATE_E;

typedef enum NC_AD_BIT_WIDTH_E
{
    NC_AD_BIT_WIDTH_8   = 0,   /* 8bit width */
    NC_AD_BIT_WIDTH_16  = 1,   /* 16bit width*/
    NC_AD_BIT_WIDTH_24  = 2,   /* 24bit width*/
    NC_AD_BIT_WIDTH_BUTT,
} NC_AD_BIT_WIDTH_E;

typedef enum NC_AD_WORKSTATE_E
{
    NC_AD_WORKSTATE_COMMON  = 0,   /* common environment, Applicable to the family of voice calls. */
	NC_AD_WORKSTATE_MUSIC   = 1,   /* music environment , Applicable to the family of music environment. */
	NC_AD_WORKSTATE_NOISY   = 2,   /* noisy environment , Applicable to the noisy voice calls.  */
	NC_AD_WORKSTATE_NONE    = 3,
} NC_AD_WORKSTATE_E;

typedef enum NC_VO_WORK_MODE_E
{
	NC_VO_WORK_MODE_1MUX = 0,
	NC_VO_WORK_MODE_2MUX,
	NC_VO_WORK_MODE_4MUX,
} NC_VO_WORK_MODE_E;

typedef enum NC_VO_INTF_MODE_E
{
	NC_VO_MODE_BT656 = 0,              /* ITU-R BT.656 YUV4:2:2 */
	NC_VO_MODE_BT601,                  /* ITU-R BT.601 YUV4:2:2 */
	NC_VO_MODE_DIGITAL_CAMERA,         /* digital camera mode */
	NC_VO_MODE_BT1120_STANDARD,        /* BT.1120 progressive mode */
	NC_VO_MODE_BT1120_INTERLEAVED,     /* BT.1120 interstage mode */
} NC_VO_INTF_MODE_E;

typedef enum NC_VO_CLK_EDGE_E
{
	NC_VO_CLK_EDGE_SINGLE_UP = 0,         /* single-edge mode and in rising edge */
	NC_VO_CLK_EDGE_SINGLE_DOWN,           /* single-edge mode and in falling edge */
	NC_VO_CLK_EDGE_DOUBLE,
} NC_VO_CLK_EDGE_E;

typedef enum NC_CH_E
{
	NC_CH1 = 0,
	NC_CH2,
	NC_CH3,
	NC_CH4,
} NC_CH_E;

typedef enum NC_PORT_E
{
	NC_PORT_A = 0,
	NC_PORT_B,
	NC_PORT_C,
	NC_PORT_D,
} NC_PORT_E;


typedef enum NC_CABLE_E
{
	CABLE_3C2V = 0,
	CABLE_UTP,
	CABLE_C,
	CABLE_D,
} NC_CABLE_E;


typedef enum NC_EQ_STAGE_E
{
	EQ_STAGE_0 = 0,
	EQ_STAGE_1,
	EQ_STAGE_2,
	EQ_STAGE_3,
	EQ_STAGE_4,
	EQ_STAGE_5,
	EQ_STAGE_6,
	EQ_STAGE_7,
	EQ_STAGE_8,
	EQ_STAGE_9,
	EQ_STAGE_10,
} NC_EQ_STAGE_E;

typedef enum NC_COAX_READ_DEF_E
{
	COAX_READ_UNKNOWN = 0,
	rx_src,
	rx_slice_lev,
	tx_baud,
	tx_pel_baud,
	tx_line_pos0,
	tx_line_pos1,
	tx_line_count,
	tx_pel_line_pos0,
	tx_pel_line_pos1,
	tx_line_count_max,
	tx_mode,
	tx_sync_pos0,
	tx_sync_pos1,
	tx_even,

	COAX_RAED_MAX,

} NC_COAX_READ_DEF_E;

typedef enum NC_COAX_CMD_VER_E
{
	COAX_COAX_NORMAL,
	COAX_AHD_8BIT,
	COAX_AHD_16BIT,
	COAX_CVI_OLD,
	COAX_CVI_NEW,

}NC_COAX_CMD_VER_E;

typedef enum NC_COAX_CMD_DEF_E
{
	COAX_CMD_UNKNOWN = 0,
	COAX_CMD_IRIS_INC,
	COAX_CMD_IRIS_DEC,
	COAX_CMD_FOCUS_INC,
	COAX_CMD_FOCUS_DEC,
	COAX_CMD_ZOOM_INC,
	COAX_CMD_ZOOM_DEC,
	COAX_CMD_OSD_ON,
	COAX_CMD_PTZ_UP,
	COAX_CMD_PTZ_DOWN,
	COAX_CMD_PTZ_LEFT,
	COAX_CMD_PTZ_RIGHT,
	COAX_CMD_OSD_ENTER,
	COAX_CMD_SPECIAL_FW,
	COAX_CMD_SPECIAL_CAMEQ,
	COAX_CMD_SPECIAL_FPS,
	COAX_CMD_SPECIAL_MOTION,
	COAX_CMD_TVI_DOWNSTREAM_REQUEST,
	COAX_CMD_TVI_1M_A_B_TYPE_MODE_CHANGE,
	COAX_CMD_ISP_TEST_LOW_TO_HIGH,
	COAX_CMD_ISP_TEST_HIGH_TO_LOW,
	COAX_CMD_SPECIAL_3DNR_REQUEST,

	COAX_CMD_MAX,
} NC_COAX_CMD_DEF_E;


typedef enum NC_MIPI_LANE_E
{
	NC_MIPI_LANE_1,
	NC_MIPI_LANE_2,
	NC_MIPI_LANE_4,

	NC_MIPI_LANE_MAX,
} NC_MIPI_LANE_E;

typedef enum NC_MIPI_CLK_E
{
	NC_MIPI_CLK_94_5Mhz,
	NC_MIPI_CLK_172_125Mhz,
	NC_MIPI_CLK_175_5Mhz,
	NC_MIPI_CLK_182_25Mhz,

	NC_MIPI_CLK_MAX,
} NC_MIPI_CLK_E;

typedef enum NC_MIPI_BPS_E
{
	NC_MIPI_BPS_594Mbps,
	NC_MIPI_BPS_756Mbps,
	NC_MIPI_BPS_972Mbps,
	NC_MIPI_BPS_1242Mbps,
	NC_MIPI_BPS_1458Mbps,
	NC_MIPI_BPS_297Mbps,

	NC_MIPI_BPS_MAX,
} NC_MIPI_BPS_E;


typedef enum NC_MIPI_SWAP_E
{
	NC_MIPI_SWAP_DISABLE,
	NC_MIPI_SWAP_ENABLE,

	NC_MIPI_SWAP_MAX,
} NC_MIPI_SWAP_E;


typedef enum NC_3531D_VI_INTV_E
{
	HI3531D_VI_MIPI,
	HI3531D_VI_DEV,
	HI3531D_VI_CHN,
	HI3531D_VI_VPSS,
	HI3531D_VI_VO,
	HI3531D_VI_MAX,
} NC_3531D_VI_INTV_E;

typedef struct _nc_decoder_s{
	/**************************************************************************
	 * Common Info
	 **************************************************************************/
	NC_U8 Total_Chip_Cnt;
	NC_U8 Total_Chn_Cnt;
	NC_U8 chip_id[4];
	NC_U8 chip_rev[4];
	NC_U8 chip_addr[4];

	/**************************************************************************
	 * Common Value
	 **************************************************************************/
	// Common
	NC_U8 Dev;          // Decoder Device - 0[0x60], 1[0x62], 2[0x64], 3[0x66]
	NC_U8 Chn;			// Video Input Channel Number
	NC_U8 Vfc;          // Driver -> Application

	// Video Format Information
	char FmtStr[256];
	NC_FORMAT_STANDARD_E   FormatStandard;
	NC_FORMAT_RESOLUTION_E FormatResolution;
	NC_FORMAT_FPS_E 	   FormatFps;
	NC_VIVO_CH_FORMATDEF_E VideoFormat;

	NC_CABLE_E          VideoCable;
	NC_EQ_STAGE_E 		EqStage;

	NC_VIDEO_SET_MODE_E VideoSetMode;

	NC_U32 SamVal;
	NC_S32 Ret;
	/**************************************************************************
	 * Video
	 **************************************************************************/
	NC_U8 FmtChange;
	// Video Output
	NC_U8 PortNo;
	NC_U8 OutputChn;

	// Decoder Set, get Value
	NC_U8 Value;
	NC_U8 Param;

	// output
	NC_PORT_E		  VO_Port;
	NC_VO_WORK_MODE_E VO_MuxMode;
	NC_VO_INTF_MODE_E VO_Interface;
	NC_VO_CLK_EDGE_E  VO_Edge;
	NC_U8 			  VO_ChnSeq[4];

	// Operation Mode
	NC_VIDEO_SET_MODE_E OP_Mode;

	// Video Color Tune
	NC_VIDEO_COLOR_TUNE_E VD_Color_Tune_Sel;

	/**************************************************************************
	 * Coaxial
	 **************************************************************************/
	NC_COAX_CMD_VER_E 	 Coax_ver;
	NC_COAX_CMD_DEF_E 	 Cmd;
	NC_U8 rx_pelco_data[8];
	NC_U8 rx_data1[8];
	NC_U8 rx_data2[8];
	NC_U8 rx_data3[8];
	NC_U8 rx_data4[8];
	NC_U8 rx_data5[8];
	NC_U8 rx_data6[8];


	NC_U32	currentpacketnum;		// current packet sequnce number(0,1,2........)
	NC_U32	filepacketnum;			// file packet number = (total size/128bytes), if remain exist, file packet number++
	NC_U8 	onepacketbuf[128+32];
	NC_U32	currentFileOffset;		// Current file offset
	NC_U32	readsize;				// currnet read size
	NC_U32 	receive_addr;
	NC_S32 	result;
	NC_U32	appstatus[16];			// Application status

	/**************************************************************************
	 * AOC
	 **************************************************************************/
	NC_U32 audio_mode;			// Analog Input or Audio of Coaxial
	NC_U32 audio_sample_rate;	// 8, 16Khz
	NC_U32 audio_bit_width;		// 8, 16Bit

	/**************************************************************************
	 * Motion
	 **************************************************************************/
	NC_U8 Select;

	/**************************************************************************
	 * ETC
	 **************************************************************************/
	NC_U8 Bank;
	NC_U8 Addr;
	NC_U8 DumpData[256];

	NC_U8 sdk_mipi_lane;
	NC_U8 sdk_mipi_clock;
	NC_U8 sdk_mipi_swap;

	NC_U8 sdk_emb;

}nc_decoder_s;


#endif
