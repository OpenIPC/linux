/**
    KDRV Header file of Serial Sensor Interface

    Exported KDRV header file of Serial Sensor Interface.

    @file       kdrv_ssenif.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _KDRV_SSENIF_H_
#define _KDRV_SSENIF_H_

#include "kwrap/type.h"
#include "kdrv_type.h"

typedef enum {
	KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR, ///< SONY without HDR.
	KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL,  ///< SONY with Line-Information(LI) DOL HDR.
	KDRV_SSENIF_SENSORTYPE_SONY_VC_DOL, ///< SONY with Virtual-Channel(VC) DOL HDR.
	KDRV_SSENIF_SENSORTYPE_OMNIVISION,   ///< Omni-Vision
	KDRV_SSENIF_SENSORTYPE_ONSEMI,       ///< ON-Semi
	KDRV_SSENIF_SENSORTYPE_PANASONIC,    ///< Panasonic
	KDRV_SSENIF_SENSORTYPE_OTHERS,       ///< Sensor vendor not specified above.

	ENUM_DUMMY4WORD(KDRV_SSENIF_SENSORTYPE)
} KDRV_SSENIF_SENSORTYPE;

typedef enum {
	KDRV_SSENIF_CLANE_CSI0_USE_C0,		///< MIPI CSI0 uses HSI_CK0 as clock lane.
	KDRV_SSENIF_CLANE_CSI0_USE_C2,		///< MIPI CSI0 uses HSI_CK2 as clock lane.
	KDRV_SSENIF_CLANE_CSI1_USE_C4,		///< MIPI CSI1 uses HSI_CK4 as clock lane.
	KDRV_SSENIF_CLANE_CSI1_USE_C6,		///< MIPI CSI1 uses HSI_CK6 as clock lane.

	KDRV_SSENIF_CLANE_LVDS0_USE_C0C4,	///< LVDS0 uses HSI_CK0 as clock lane in one clock lane sensor.
										///< LVDS0 uses HSI_CK0+CK4 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	KDRV_SSENIF_CLANE_LVDS0_USE_C2C6,	///< LVDS0 uses HSI_CK2 as clock lane in one clock lane sensor.
										///< LVDS0 uses HSI_CK2+CK6 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	KDRV_SSENIF_CLANE_LVDS1_USE_C4,		///< LVDS1 uses HSI_CK4 as clock lane.
	KDRV_SSENIF_CLANE_LVDS1_USE_C6,		///< LVDS1 uses HSI_CK6 as clock lane.

	KDRV_SSENIF_CLANE_CSI1_USE_C1,		///< MIPI CSI1 uses HSI_CK1 as clock lane.
	KDRV_SSENIF_CLANE_LVDS1_USE_C1,		///< LVDS1 uses HSI_CK1 as clock lane.

	ENUM_DUMMY4WORD(KDRV_SSENIF_CLANE)
} KDRV_SSENIF_CLANE;

typedef enum {
	KDRV_SSENIF_STOPMETHOD_IMMEDIATELY,  ///< Stop immediately.
	KDRV_SSENIF_STOPMETHOD_FRAME_END,    ///< Stop at frame-end1 interrupt.(default)
	KDRV_SSENIF_STOPMETHOD_FRAME_END2,   ///< Stop at frame-end2 interrupt during HDR application.
	KDRV_SSENIF_STOPMETHOD_FRAME_END3,   ///< Stop at frame-end3 interrupt during HDR application.
	KDRV_SSENIF_STOPMETHOD_FRAME_END4,   ///< Stop at frame-end4 interrupt during HDR application.

	KDRV_SSENIF_STOPMETHOD_SIE1_FRAMEEND,
	KDRV_SSENIF_STOPMETHOD_SIE2_FRAMEEND,
	KDRV_SSENIF_STOPMETHOD_SIE4_FRAMEEND,
	KDRV_SSENIF_STOPMETHOD_SIE5_FRAMEEND,

	ENUM_DUMMY4WORD(KDRV_SSENIF_STOPMETHOD)
} KDRV_SSENIF_STOPMETHOD;

typedef enum {
	KDRV_SSENIF_LANESEL_HSI_D0 = 0x0001,     ///< Package pin name: HSI_D0P / HSI_D0N
	KDRV_SSENIF_LANESEL_HSI_D1 = 0x0002,     ///< Package pin name: HSI_D1P / HSI_D1N
	KDRV_SSENIF_LANESEL_HSI_D2 = 0x0004,     ///< Package pin name: HSI_D2P / HSI_D2N
	KDRV_SSENIF_LANESEL_HSI_D3 = 0x0008,     ///< Package pin name: HSI_D3P / HSI_D3N
	KDRV_SSENIF_LANESEL_HSI_D4 = 0x0010,     ///< Package pin name: HSI_D4P / HSI_D4N
	KDRV_SSENIF_LANESEL_HSI_D5 = 0x0020,     ///< Package pin name: HSI_D5P / HSI_D5N
	KDRV_SSENIF_LANESEL_HSI_D6 = 0x0040,     ///< Package pin name: HSI_D6P / HSI_D6N
	KDRV_SSENIF_LANESEL_HSI_D7 = 0x0080,     ///< Package pin name: HSI_D7P / HSI_D7N

	KDRV_SSENIF_LANESEL_HSI_LOW4  = KDRV_SSENIF_LANESEL_HSI_D0 | KDRV_SSENIF_LANESEL_HSI_D1 | KDRV_SSENIF_LANESEL_HSI_D2 | KDRV_SSENIF_LANESEL_HSI_D3,
	KDRV_SSENIF_LANESEL_HSI_HIGH4 = KDRV_SSENIF_LANESEL_HSI_D4 | KDRV_SSENIF_LANESEL_HSI_D5 | KDRV_SSENIF_LANESEL_HSI_D6 | KDRV_SSENIF_LANESEL_HSI_D7,
	KDRV_SSENIF_LANESEL_HSI_ALL   = KDRV_SSENIF_LANESEL_HSI_LOW4 | KDRV_SSENIF_LANESEL_HSI_HIGH4,
	ENUM_DUMMY4WORD(KDRV_SSENIF_LANESEL)
} KDRV_SSENIF_LANESEL;

typedef enum {
	KDRV_SSENIF_INTERRUPT_VD         = 0x00000001,   ///< Valid for CSI/LVDS/SLVSEC/Vx1
	KDRV_SSENIF_INTERRUPT_VD2        = 0x00000002,   ///< Valid for LVDS/SLVSEC/Vx1
	KDRV_SSENIF_INTERRUPT_VD3        = 0x00000004,   ///< Valid for LVDS/Vx1
	KDRV_SSENIF_INTERRUPT_VD4        = 0x00000008,   ///< Valid for LVDS/Vx1
	KDRV_SSENIF_INTERRUPT_FRAMEEND   = 0x00000010,   ///< Valid for CSI/LVDS/SLVSEC
	KDRV_SSENIF_INTERRUPT_FRAMEEND2  = 0x00000020,   ///< Valid for CSI/LVDS/SLVSEC
	KDRV_SSENIF_INTERRUPT_FRAMEEND3  = 0x00000040,   ///< Valid for CSI/LVDS
	KDRV_SSENIF_INTERRUPT_FRAMEEND4  = 0x00000080,   ///< Valid for CSI/LVDS
	KDRV_SSENIF_INTERRUPT_ABORT      = 0x80000000,   ///< Waiting event timeout abort.

	KDRV_SSENIF_INTERRUPT_SIE1_VD    	= 0x00000100,
	KDRV_SSENIF_INTERRUPT_SIE2_VD    	= 0x00000200,
	KDRV_SSENIF_INTERRUPT_SIE4_VD    	= 0x00000400,
	KDRV_SSENIF_INTERRUPT_SIE5_VD    	= 0x00000800,
	KDRV_SSENIF_INTERRUPT_SIE1_FRAMEEND = 0x00001000,
	KDRV_SSENIF_INTERRUPT_SIE2_FRAMEEND = 0x00002000,
	KDRV_SSENIF_INTERRUPT_SIE4_FRAMEEND = 0x00004000,
	KDRV_SSENIF_INTERRUPT_SIE5_FRAMEEND = 0x00008000,

	ENUM_DUMMY4WORD(KDRV_SSENIF_INTERRUPT)
} KDRV_SSENIF_INTERRUPT;


typedef enum {
	KDRV_SSENIFCSI_MANUAL_DEPACK_8,			///< Select the depacking method as RAW8.
	KDRV_SSENIFCSI_MANUAL_DEPACK_10,		///< Select the depacking method as RAW10.
	KDRV_SSENIFCSI_MANUAL_DEPACK_12,		///< Select the depacking method as RAW12.
	KDRV_SSENIFCSI_MANUAL_DEPACK_14,		///< Select the depacking method as RAW14.

	ENUM_DUMMY4WORD(KDRV_SSENIFCSI_MANUAL_DEPACK)
} KDRV_SSENIFCSI_MANUAL_DEPACK;

typedef enum {
	KDRV_SSENIFLVDS_PIXEL_INORDER_LSB,	///< LVDS Pixel Data bit order LSb first
	KDRV_SSENIFLVDS_PIXEL_INORDER_MSB,	///< LVDS Pixel Data bit order MSb first

	ENUM_DUMMY4WORD(KDRV_SSENIFLVDS_PIXEL_INORDER)
} KDRV_SSENIFLVDS_PIXEL_INORDER;

typedef enum {
	KDRV_SSENIFCSI_CLANE_METHOD_NORMAL,     ///< clock lane method normal
	KDRV_SSENIFCSI_CLANE_METHOD_FORCE_HS,   ///< clock lane method force high speed.

	ENUM_DUMMY4WORD(KDRV_SSENIFCSI_CLANE_METHOD)
} KDRV_SSENIFCSI_CLANE_METHOD;

#define KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) ((lane_sel<<20)|(value&0xFFFF))



/**
	THINE Vx1 TX Module Selection

	This is used in kdrv_ssenif_set(KDRV_SSENIF_VX1_TXTYPE) to specify Vx1 TX moudle.
	This setting must be done after kdrv_ssenif_open and before first kdrv_ssenif_get(KDRV_SSENIF_VX1_GET_PLUG) to make setting activate.
	Currently only these three Vx1 modules are supported. If THINE's new TX module is ready to use,
	Please contack to novatek to add the new TX module to driver. And novatek would confirm that the new TX is supported or not.
*/
typedef enum {
	KDRV_SSENIFVX1_TXTYPE_THCV235,	///< Vx1 Transmitter IC is THCV235.
									///< This only support Vx1 One-lane none-HDR parallel sensor.
	KDRV_SSENIFVX1_TXTYPE_THCV231,	///< Vx1 Transmitter IC is THCV231.
									///< This only support Vx1 One-lane none-HDR parallel sensor.

	KDRV_SSENIFVX1_TXTYPE_THCV241,	///< Vx1 Transmitter IC is THCV241. This is MIPI CSI input interface Vx1 Tx.
									///< Which can support 1 or 2 lanes Vx1 interface. HDR MIPI Sensor such as SONY_LI or OV/ON-SEMI is also supported.
									///< The maximum MIPI Sensor support spec is 4 lanes mipi with 1Gbps per lane by using Vx1 2 lanes.
									///< The maximum MIPI Sensor support spec is 4 lanes mipi with 500Mbps per lane by using Vx1 1 lanes.
	ENUM_DUMMY4WORD(KDRV_SSENIFVX1_TXTYPE)
} KDRV_SSENIFVX1_TXTYPE;

/**
	VX1 Operation mode selection

	For THCV231/THCV235, the user shall use KDRV_SSENIFVX1_DATAMUX_4BYTE_RAW for RAW10/RAW12 sensors,
						Use KDRV_SSENIFVX1_DATAMUX_YUV8/16 for YUV sensors.
	For THCV241, the user shall use KDRV_SSENIFVX1_DATAMUX_3BYTE_RAW for RAW10/RAW12 sensors,
						Use KDRV_SSENIFVX1_DATAMUX_YUV16 for YUV sensors.
*/
typedef enum {
	KDRV_SSENIFVX1_DATAMUX_3BYTE_RAW,	///< Vx1 is operating at 3BYTE Muxing Mode with RAW10/RAW12 sensor in use.
										///< 1T D[23:0] to 2T "D[23:12]"+"D[11:0]".
										///< This shall be used for THCV241 with RAW sensor.
	KDRV_SSENIFVX1_DATAMUX_4BYTE_RAW,	///< Vx1 is operating at 4BYTE Muxing Mode with RAW10/RAW12 sensor in use.(default)
										///< 1T D[31:0] to 2T "D[27:16]"+"D[11:0]".
										///< This shall be used for THCV231/THCV235 with RAW sensor.

	KDRV_SSENIFVX1_DATAMUX_YUV8,		///< 1T D[23:0] to 1T "D[23:16]D[7:0]"-16bits
										///< This shall be used for THCV231/THCV235 with YUV sensor.
	KDRV_SSENIFVX1_DATAMUX_YUV16,		///< 1T D[31:0] to 2T "D[31:16]"+"D[15:0]".
										///< This shall be used for THCV231/THCV235/THCV241 with YUV sensor.

	KDRV_SSENIFVX1_DATAMUX_NONEMUXING,	///< D[11:0] to D[11:0]. Debugging usages.
	ENUM_DUMMY4WORD(KDRV_SSENIFVX1_DATAMUX)
} KDRV_SSENIFVX1_DATAMUX;

typedef enum {
	KDRV_SSENIFVX1_I2CSPEED_50KHZ,	///< I2C Speed on Vx1 Transmitter: 50KHz
	KDRV_SSENIFVX1_I2CSPEED_100KHZ,	///< I2C Speed on Vx1 Transmitter: 100KHz (default)
	KDRV_SSENIFVX1_I2CSPEED_200KHZ,	///< I2C Speed on Vx1 Transmitter: 200KHz
	KDRV_SSENIFVX1_I2CSPEED_400KHZ,	///< I2C Speed on Vx1 Transmitter: 400KHz

	ENUM_DUMMY4WORD(KDRV_SSENIFVX1_I2CSPEED)
} KDRV_SSENIFVX1_I2CSPEED;

typedef struct {
	UINT32  reg_start_addr;		///< sensor register access start address
	UINT32  data_size;			///< sensor data register size in byte count
	void   *data_buffer;
} KDRV_SSENIFVX1_I2CCMD, *PKDRV_SSENIFVX1_I2CCMD;




typedef enum {
	KDRV_SSENIFGLO_SIEMCLK_SOURCE_480,		///< Select SIE MCLK SOURCE as 480 MHz
	KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5,	///< Select SIE MCLK SOURCE as PLL5
	KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10,	///< Select SIE MCLK SOURCE as PLL10
	KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12,	///< Select SIE MCLK SOURCE as PLL12
	KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL18,	///< Select SIE MCLK SOURCE as PLL18

	ENUM_DUMMY4WORD(KDRV_SSENIFGLO_SIEMCLK_SOURCE)
} KDRV_SSENIFGLO_SIEMCLK_SOURCE;





/**
    KDRV SSENIF parameter select id

    This is used in kdrv_ssenif_set and .kdrv_ssenif_get to configure the SSENIF configuration parameter.
*/
typedef enum {
	/*
		CSI Group
	*/
	KDRV_SSENIF_CSI_BASE = 0x01000000,
	KDRV_SSENIF_CSI_WAIT_INTERRUPT,		///< [csi][set/get] Wait interrupt event. Please use KDRV_SSENIF_INTERRUPT as input param.
	KDRV_SSENIF_CSI_SENSORTYPE,			///< [csi][set/get] Sensor type. Please use KDRV_SSENIF_SENSORTYPE for input param.
	KDRV_SSENIF_CSI_SENSOR_TARGET_MCLK,	///< [csi][set/get]	Sensor normal operation target MCLK frequency.
	KDRV_SSENIF_CSI_SENSOR_REAL_MCLK,	///< [csi][set/get]	Sensor real operation MCLK frequency provided.
										///<				This can be used to down speed sensor frequency while debugging usage.
										///<				And auto re-calculates internal interface parameters.
	KDRV_SSENIF_CSI_DLANE_NUMBER,		///< [csi][set/get]	Data lane number
	KDRV_SSENIF_CSI_VALID_HEIGHT,		///< [csi][set/get]	Image Height each frame. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_CSI_VALID_HEIGHT_SIE,	///< [csi][set/get]	Image Height each frame for SIE1. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_CSI_VALID_HEIGHT_SIE2,	///< [csi][set/get]	Image Height each frame for SIE2. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_CSI_VALID_HEIGHT_SIE3,	///< Reserved
	KDRV_SSENIF_CSI_VALID_HEIGHT_SIE4,	///< [csi][set/get]	Image Height each frame for SIE4. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_CSI_VALID_HEIGHT_SIE5,	///< [csi][set/get]	Image Height each frame for SIE5. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_CSI_CLANE_SWITCH,		///< [csi][set/get]	For [CSI]  Please use KDRV_SSENIF_CLANE_CSI0_USE_C0 or
										///<				KDRV_SSENIF_CLANE_CSI0_USE_C2 select clock lane.
										///<				For [CSI2] Please use KDRV_SSENIF_CLANE_CSI1_USE_C4 or
										///<				KDRV_SSENIF_CLANE_CSI1_USE_C6 select clock lane.
										///<				[CSI3-8] Not valid setting.
	KDRV_SSENIF_CSI_TIMEOUT_PERIOD,		///< [csi][set/get]	KDRV_SSENIF_CSI_WAIT_INTERRUPT timeout period. Unit in ms. Default is 1000 ms.
	KDRV_SSENIF_CSI_STOP_METHOD,		///< [csi][set/get]	Select the stop behavior. Pleas use KDRV_SSENIF_STOPMETHOD as input.
	KDRV_SSENIF_CSI_IMGID_TO_SIE,		///< [csi][set/get][hdr] Select IMAGE frame ID sent to SIE1. Valid for CSI only.
	KDRV_SSENIF_CSI_IMGID_TO_SIE2,		///< [csi][set/get][hdr] Select IMAGE frame ID sent to SIE2. Valid for CSI/CSI2-SDR only.
	KDRV_SSENIF_CSI_IMGID_TO_SIE3,		///< Reserved
	KDRV_SSENIF_CSI_IMGID_TO_SIE4,		///< [csi][set/get][hdr] Select IMAGE frame ID sent to SIE4. Valid for CSI/CSI2-HDR only.
	KDRV_SSENIF_CSI_IMGID_TO_SIE5,		///< [csi][set/get][hdr] Select IMAGE frame ID sent to SIE5. Valid for CSI/CSI2-HDR only.
	KDRV_SSENIF_CSI_DATALANE0_PIN,		///< [csi][set/get]	Specify sensor's data lane-0 is layout to ISP's which pad pin.
										///<				Please use KDRV_SSENIF_LANESEL as input
	KDRV_SSENIF_CSI_DATALANE1_PIN,		///< [csi][set/get]	Specify sensor's data lane-1 is layout to ISP's which pad pin.
										///<				Please use KDRV_SSENIF_LANESEL as input
	KDRV_SSENIF_CSI_DATALANE2_PIN,		///< [csi][set/get]	Specify sensor's data lane-2 is layout to ISP's which pad pin.
										///<				Please use KDRV_SSENIF_LANESEL as input
	KDRV_SSENIF_CSI_DATALANE3_PIN,		///< [csi][set/get]	Specify sensor's data lane-3 is layout to ISP's which pad pin.
										///<				Please use KDRV_SSENIF_LANESEL as input
	KDRV_SSENIF_CSI_LPKT_MANUAL,		///< [csi][set/get][optional] Manual1 Format enable/disable.
	KDRV_SSENIF_CSI_MANUAL_FORMAT,		///< [csi][set/get][optional] Select the depacking method for Manual1 packet.
										///<				Please use KDRV_SSENIFCSI_MANUAL_DEPACK as input parameter.
	KDRV_SSENIF_CSI_MANUAL_DATA_ID,		///< [csi][set/get][optional] 6-bits Manual1 Format data identifier(DI).
	KDRV_SSENIF_CSI_LPKT_MANUAL2,		///< [csi][set/get][optional] Manual2 Format enable/disable.
	KDRV_SSENIF_CSI_MANUAL2_FORMAT,		///< [csi][set/get][optional] Select the depacking method for Manual2 packet.
										///<				Please use KDRV_SSENIFCSI_MANUAL_DEPACK as input parameter.
	KDRV_SSENIF_CSI_MANUAL2_DATA_ID,	///< [csi][set/get][optional] 6-bits Manual2 Format data identifier(DI).
	KDRV_SSENIF_CSI_LPKT_MANUAL3,		///< [csi][set/get][optional] Manual3 Format enable/disable.
	KDRV_SSENIF_CSI_MANUAL3_FORMAT,		///< [csi][set/get][optional] Select the depacking method for Manual3 packet.
										///<				Please use KDRV_SSENIFCSI_MANUAL_DEPACK as input parameter.
	KDRV_SSENIF_CSI_MANUAL3_DATA_ID,	///< [csi][set/get][optional] 6-bits Manual3 Format data identifier(DI).

	KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK,  ///< [csi][set/get] Sensor real operation MIPI-HS frequency.(unit:Kbps)
	KDRV_SSENIF_CSI_IADJ,               ///< [csi][set][optional] Select IADJ level. (valid value: 0~3)
	KDRV_SSENIF_CSI_CLANE_CMETHOD,		///< [csi][set][optional] clock lane control method, force high speed or normal.
	KDRV_SSENIF_CSI_HSDATAOUT_DLY,      ///< [csi][set][optional] Set the enable hs data out delay.
	KDRV_SSENIF_CSI_SENSOR_FS_CNT,      ///< [csi][get][optional] Get the fs count.

	/*
		LVDS Group
	*/
	KDRV_SSENIF_LVDS_BASE = 0x02000000,
	KDRV_SSENIF_LVDS_WAIT_INTERRUPT,	///< [lvds][set/get] Wait interrupt event. Please use KDRV_SSENIF_INTERRUPT as input param.
	KDRV_SSENIF_LVDS_SENSORTYPE,		///< [lvds][set/get] Sensor type. Please use KDRV_SSENIF_SENSORTYPE for input param.
	KDRV_SSENIF_LVDS_DLANE_NUMBER,		///< [lvds][set/get] Data lane number
	KDRV_SSENIF_LVDS_VALID_WIDTH,		///< [lvds][set/get] Image Pixel Width each line. (Including front OB and invalid pixel width)
	KDRV_SSENIF_LVDS_VALID_HEIGHT,		///< [lvds][set/get] Image Height each frame. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_LVDS_PIXEL_DEPTH,		///< [lvds][set/get] RAW Pixel bit Width. Valid value is 8/10/12/14/16.
	KDRV_SSENIF_LVDS_CLANE_SWITCH,		///< [lvds][set/get] For [LVDS]  Please use KDRV_SSENIF_CLANE_LVDS0_USE_C0C4 or
										///<				KDRV_SSENIF_CLANE_LVDS0_USE_C2C6 select clock lane.
										///<				For [LVDS2] Please use KDRV_SSENIF_CLANE_LVDS1_USE_C4 or
										///<				KDRV_SSENIF_CLANE_LVDS1_USE_C6 select clock lane.
	KDRV_SSENIF_LVDS_TIMEOUT_PERIOD,	///< [lvds][set/get] KDRV_SSENIF_LVDS_WAIT_INTERRUPT timeout period. Unit in ms. Default is 1000 ms.
	KDRV_SSENIF_LVDS_STOP_METHOD,		///< [lvds][set/get] Select the stop behavior. Pleas use KDRV_SSENIF_STOPMETHOD as input.
	KDRV_SSENIF_LVDS_IMGID_TO_SIE,		///< [lvds][set/get][hdr] Select IMAGE frame ID sent to SIE1. Valid for LVDS only.
	KDRV_SSENIF_LVDS_IMGID_TO_SIE2,		///< [lvds][set/get][hdr] Select IMAGE frame ID sent to SIE2. Valid for LVDS/LVDS2 only.
	KDRV_SSENIF_LVDS_IMGID_TO_SIE3,		///< Reserved
	KDRV_SSENIF_LVDS_IMGID_TO_SIE4,		///< [lvds][set/get][hdr] Select IMAGE frame ID sent to SIE4. Valid for LVDS/LVDS2 only.
	KDRV_SSENIF_LVDS_IMGID_TO_SIE5,		///< [lvds][set/get][hdr] Select IMAGE frame ID sent to SIE4. Valid for LVDS/LVDS2 only.
	KDRV_SSENIF_LVDS_OUTORDER_0,		///< [lvds][set/get] Select sensor data lane 0 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_1,		///< [lvds][set/get] Select sensor data lane 1 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_2,		///< [lvds][set/get] Select sensor data lane 2 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_3,		///< [lvds][set/get] Select sensor data lane 3 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_4,		///< [lvds][set/get] Select sensor data lane 4 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_5,		///< [lvds][set/get] Select sensor data lane 5 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_6,		///< [lvds][set/get] Select sensor data lane 6 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_OUTORDER_7,		///< [lvds][set/get] Select sensor data lane 7 connection position. Please use KDRV_SSENIF_LANESEL as input.
	KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT,	///< [lvds][set]	 Set The HD/VD default SYNC-CODE & KDRV_SSENIF_LVDS_FSET_BIT/KDRV_SSENIF_LVDS_IMGID_BIT0-3  according to sensor type.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.

	KDRV_SSENIF_LVDS_PIXEL_INORDER,		///< [lvds][set/get][OPTIONAL] Select Pixel Data bit order MSb or LSb first. Use KDRV_SSENIFLVDS_PIXEL_INORDER as input.
										///<				The driver would auto select a suitable selection when SENSORTYPE is set. The driver may not set this settings.
										///<				If the user want to change this setting, must be configured after KDRV_SSENIF_LVDS_SENSORTYPE setup.
	KDRV_SSENIF_LVDS_FSET_BIT,			///< [lvds][set/get][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
	KDRV_SSENIF_LVDS_IMGID_BIT0,		///< [lvds][set/get][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
	KDRV_SSENIF_LVDS_IMGID_BIT1,		///< [lvds][set/get][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
	KDRV_SSENIF_LVDS_IMGID_BIT2,		///< [lvds][set/get][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
	KDRV_SSENIF_LVDS_IMGID_BIT3,		///< [lvds][set/get][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.

	KDRV_SSENIF_LVDS_DLANE_CW_HD,		///< [lvds][set][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.
	KDRV_SSENIF_LVDS_DLANE_CW_VD,		///< [lvds][set][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.
	KDRV_SSENIF_LVDS_DLANE_CW_VD2,		///< [lvds][set][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.
	KDRV_SSENIF_LVDS_DLANE_CW_VD3,		///< [lvds][set][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.
	KDRV_SSENIF_LVDS_DLANE_CW_VD4,		///< [lvds][set][optional] When using KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, Driver would assign default value according to KDRV_SSENIF_LVDS_SENSORTYPE.
										///<				Please use KDRV_SSENIFLVDS_CONTROL_WORD(lane_sel, value) for input param.

	/*
		Vx1 Group
	*/
	KDRV_SSENIF_VX1_BASE = 0x04000000,
	KDRV_SSENIF_VX1_WAIT_INTERRUPT,		///< [vx1][set/get] Wait interrupt event. Please use KDRV_SSENIF_INTERRUPT as input param.
	KDRV_SSENIF_VX1_TXTYPE,				///< [vx1][set/get] This configuration must be done before first kdrv_ssenif_get(KDRV_SSENIF_VX1_GET_PLUG).
										///<				Select Vx1 Transmitter TYPE. Please use KDRV_SSENIFVX1_TXTYPE as input parameter.
	KDRV_SSENIF_VX1_SENSORTYPE,			///< [vx1][set/get] Use KDRV_SSENIF_SENSORTYPE as input parameter.
										///<				Default is KDRV_SSENIF_SENSORTYPE_OTHERS.
	KDRV_SSENIF_VX1_DATAMUX_SEL,		///< [vx1][set/get] Please use KDRV_SSENIFVX1_DATAMUX as input parameter.
	KDRV_SSENIF_VX1_TIMEOUT_PERIOD,		///< [vx1][set/get] .wait_interrupt() timeout period. Unit in ms. Default is 1000 ms.
	KDRV_SSENIF_VX1_LOCKCHG_CALLBACK,	///< [vx1][set]		Please use the prototype of "void (*callback_api_name)(UINT32 ui_event)" for callback function.
	KDRV_SSENIF_VX1_I2C_SPEED,			///< [vx1][set/get] Specify i2c speed on the vx1 Tx board to sensor. Please use KDRV_SSENIFVX1_I2CSPEED as input parameter.
	KDRV_SSENIF_VX1_I2CNACK_CHECK,		///< [vx1][set/get] If "ENABLE":  The sensor I2C NACK status would be returned at .sensor_i2c_write(), but would down speed i2c transmission.
										///< If "DISABLE": The sensor I2C NACK status would be showed on the console with better i2c transmission speed.
										///< Default is DISABLE.
	KDRV_SSENIF_VX1_SENSOR_SLAVEADDR,	///< [vx1][set/get] Sensor I2C "7-bits" Slave address.(Not 8 bits)
	KDRV_SSENIF_VX1_SENREG_ADDR_BC,		///< [vx1][set/get] Set Sensor I2C command address field byte count. valid setting value 1~4.
	KDRV_SSENIF_VX1_SENREG_DATA_BC,		///< [vx1][set/get] Set Sensor I2C command    data field byte count. valid setting value 1~4.
	KDRV_SSENIF_VX1_I2C_WRITE,			///< [vx1][set] Please use PKDRV_SSENIFVX1_I2CCMD as input parameter
	KDRV_SSENIF_VX1_I2C_READ,			///< [vx1][get] Please use PKDRV_SSENIFVX1_I2CCMD as input parameter

	KDRV_SSENIF_VX1_GPIO_0,				///< [vx1][set/get] Valid for THCV241.
	KDRV_SSENIF_VX1_GPIO_1,				///< [vx1][set/get] Valid for THCV241.
	KDRV_SSENIF_VX1_GPIO_2,				///< [vx1][set/get] Valid for THCV241.
	KDRV_SSENIF_VX1_GPIO_3,				///< [vx1][set/get] Valid for THCV241/THCV231/THCV235.
	KDRV_SSENIF_VX1_GPIO_4,				///< [vx1][set/get] Valid for THCV231/THCV235.

	KDRV_SSENIF_VX1_GET_PLUG,			///< [vx1][get] Check Vx1 Cable is plug or not.
										///<			This checking mechanism is through the sublink connecttion ok or not.
	KDRV_SSENIF_VX1_GET_LOCK,			///< [vx1][get] Get vx1 mainlink lock status
	KDRV_SSENIF_VX1_GET_SPEED,			///< [vx1][get] Get Vx1 Mainlink speed.
	KDRV_SSENIF_VX1_GET_START,			///< [vx1][get] Check the current status is START or STOP.

	/*
		Below are the Tx-THCV241 / MIPI Sensor dedicated settings
	*/
	KDRV_SSENIF_VX1_VALID_WIDTH,		///< [vx1][set/get] Image Pixel Width each line. (Including front OB and invalid pixel width)
										///<  [limit]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_PIXEL_DEPTH,		///< [vx1][set/get] RAW Pixel bit Width. Valid value is 10/12.
										///<	[limit]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_VX1LANE_NUMBER,		///< [vx1][set/get] Select Vx1 Data lane number. Only Tx-THCV241 has two lanes mode.
										///<		Legal setting is 1 or 2 for  KDRV_SSENIF_ENGINE_VX1_0 object.
										///<		Legal setting is 1 only for  KDRV_SSENIF_ENGINE_VX1_1 object.
	KDRV_SSENIF_VX1_MIPILANE_NUMBER,	///< [vx1][set/get] Specify the MIPI Sensor data lane number. valid value is 1/2/4.
										///<  [limit]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_HDR_ENABLE,			///< [vx1][set/get] Select the Sensor HDR is ENABLE or DISABLE.
										///<  [limit]: For MIPI-CSI-Sensor/Tx-THCV241 only. For KDRV_SSENIF_ENGINE_VX1_0 only.
	KDRV_SSENIF_VX1_IMGID_TO_SIE,		///< [vx1][set/get][hdr] Select IMAGE frame ID sent to SIE1.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_IMGID_TO_SIE2,		///< [vx1][set/get][hdr] Select IMAGE frame ID sent to SIE2.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_IMGID_TO_SIE3,		///< [vx1][set/get][hdr] Select IMAGE frame ID sent to SIE3.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_IMGID_TO_SIE4,		///< [vx1][set/get][hdr] Select IMAGE frame ID sent to SIE4.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_241_INPUT_CLK_FREQ, ///< [vx1][set/get] Assign 241 input oscillator frequency. Input value in Hertz.
										///<  [limit]: Input clock valid range is 10000000~40000000. For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_SENSOR_TARGET_MCLK,	///< [vx1][set/get] Sensor normal operation target MCLK frequency.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_SENSOR_REAL_MCLK,	///< [vx1][set/get] Sensor real operation MCLK frequency provided.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_SENSOR_CKSPEED_BPS,	///< [vx1][set/get] Sensor clock lane speed in bps. For example enter 445000000 is clock speed is 445Mbps.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.

	KDRV_SSENIF_VX1_THCV241_PLL0,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[0]. REG0x1011
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL1,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[1]. REG0x1012
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL2,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[2]. REG0x1013
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL3,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[3]. REG0x1014
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL4,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[4]. REG0x1015
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL5,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[5]. REG0x1016
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL6,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[6]. REG0x1036
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_PLL7,		///< [vx1][set/get][optional] Setup the THCV241 PLL settings item[7]. REG0x1037
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	KDRV_SSENIF_VX1_THCV241_CKO_OUTPUT,	///< [vx1][set] ENABLE / DISABLE the THCV241's CKO pin output.
										///<  [limit]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	/*
		SLVSEC Group
	*/
	KDRV_SSENIF_SLVSEC_BASE = 0x08000000,

	KDRV_SSENIF_SLVSEC_WAIT_INTERRUPT,	///< [slvsec][set/get] Wait interrupt event. Please use KDRV_SSENIF_INTERRUPT as input param.
	KDRV_SSENIF_SLVSEC_SENSORTYPE,		///< [slvsec][set/get] Set KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR(default) or KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL.
	KDRV_SSENIF_SLVSEC_PIXEL_DEPTH,		///< [slvsec][set/get] RAW Pixel bit Width. Valid value is 8/10/12/14/16.
	KDRV_SSENIF_SLVSEC_DLANE_NUMBER,	///< [slvsec][set/get] Configure data lane number for pixel receiving. Valid setting value is 1/2/4/6/8.
	KDRV_SSENIF_SLVSEC_VALID_WIDTH,		///< [slvsec][set/get] Image Pixel Width each line. (Including front OB and invalid pixel width)
	KDRV_SSENIF_SLVSEC_VALID_HEIGHT,	///< [slvsec][set/get] Image Height each frame. (Including front OB and invalid pixel lines)
	KDRV_SSENIF_SLVSEC_SPEED,			///< [slvsec][set/get] Only input of 2304 or 1152 are allowed.
	KDRV_SSENIF_SLVSEC_POST_INIT,		///< [slvsec][set] Apply this to set after sensor start working. Input value dont care.
	KDRV_SSENIF_SLVSEC_TIMEOUT_PERIOD,	///< [slvsec][set/get] wait interrupt timeout period. Unit in ms. Default is 1000 ms.
	KDRV_SSENIF_SLVSEC_STOP_METHOD,		///< [slvsec][set/get] Use KDRV_SSENIF_STOPMETHOD_IMMEDIATELY or KDRV_SSENIF_STOPMETHOD_FRAME_END or KDRV_SSENIF_STOPMETHOD_FRAME_END2.

	KDRV_SSENIF_SLVSEC_IMGID_TO_SIE,	///< [slvsec][set/get][hdr] Select IMAGE frame ID sent to SIE1.
	KDRV_SSENIF_SLVSEC_IMGID_TO_SIE2,	///< [slvsec][set/get][hdr] Select IMAGE frame ID sent to SIE2.

	KDRV_SSENIF_SLVSEC_DATALANE0_PIN,	///< [slvsec][set/get] Specify sensor's data lane-0 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D0.
	KDRV_SSENIF_SLVSEC_DATALANE1_PIN,	///< [slvsec][set/get] Specify sensor's data lane-1 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D1.
	KDRV_SSENIF_SLVSEC_DATALANE2_PIN,	///< [slvsec][set/get] Specify sensor's data lane-2 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D2.
	KDRV_SSENIF_SLVSEC_DATALANE3_PIN,	///< [slvsec][set/get] Specify sensor's data lane-3 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D3.
	KDRV_SSENIF_SLVSEC_DATALANE4_PIN,	///< [slvsec][set/get] Specify sensor's data lane-4 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D4.
	KDRV_SSENIF_SLVSEC_DATALANE5_PIN,	///< [slvsec][set/get] Specify sensor's data lane-5 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D5.
	KDRV_SSENIF_SLVSEC_DATALANE6_PIN,	///< [slvsec][set/get] Specify sensor's data lane-6 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D6.
	KDRV_SSENIF_SLVSEC_DATALANE7_PIN,	///< [slvsec][set/get] Specify sensor's data lane-7 is layout to ISP's which pad pin.
										///<		Please use KDRV_SSENIF_LANESEL as input. Default is KDRV_SSENIF_LANESEL_HSI_D7.

	/*
		Global Group
	*/
	KDRV_SSENIF_GLOBAL_BASE = 0x80000000,
	KDRV_SSENIF_SIEMCLK_ENABLE,			///< [global][set] Set SIEMCLK ENABLE/DISABLE
	KDRV_SSENIF_SIEMCLK_SOURCE,			///< [global][set] Set SIEMCLK Souce. Please use KDRV_SSENIFGLO_SIEMCLK_SOURCE
	KDRV_SSENIF_SIEMCLK_FREQUENCY,		///< [global][set] Set SIEMCLK Frequency. Unit in Hertz.
	KDRV_SSENIF_SIEMCLK2_ENABLE,		///< [global][set] Set SIEMCLK2 ENABLE/DISABLE
	KDRV_SSENIF_SIEMCLK2_SOURCE,		///< [global][set] Set SIEMCLK2 Souce. Please use KDRV_SSENIFGLO_SIEMCLK_SOURCE
	KDRV_SSENIF_SIEMCLK2_FREQUENCY,		///< [global][set] Set SIEMCLK2 Frequency. Unit in Hertz.
	KDRV_SSENIF_SIEMCLK3_ENABLE,		///< [global][set] Set SIEMCLK3 ENABLE/DISABLE
	KDRV_SSENIF_SIEMCLK3_SOURCE,		///< [global][set] Set SIEMCLK3 Souce. Please use KDRV_SSENIFGLO_SIEMCLK_SOURCE
	KDRV_SSENIF_SIEMCLK3_FREQUENCY,		///< [global][set] Set SIEMCLK3 Frequency. Unit in Hertz.
	KDRV_SSENIF_PLL05_ENABLE,			///< [global][set/get] Set PLL5 ENABLE/DISABLE
	KDRV_SSENIF_PLL05_FREQUENCY,		///< [global][set/get] Set PLL5 Frequency
	KDRV_SSENIF_PLL12_ENABLE,			///< [global][set/get] Set PLL12 ENABLE/DISABLE
	KDRV_SSENIF_PLL12_FREQUENCY,		///< [global][set/get] Set PLL12 Frequency
	KDRV_SSENIF_PLL18_ENABLE,			///< [global][set/get] Set PLL18 ENABLE/DISABLE
	KDRV_SSENIF_PLL18_FREQUENCY,		///< [global][set/get] Set PLL18 Frequency
	KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY,///< [global][set] Set SIEMCLK1/2 Sync Frequency. Unit in Hertz.

	ENUM_DUMMY4WORD(KDRV_SSENIF_PARAM_ID)
} KDRV_SSENIF_PARAM_ID;



/*!
 * @fn INT32 kdrv_ssenif_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_SSENIF_ENGINE_*.
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ssenif_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ssenif_close(UINT32 chip, UINT32 engine)
 * @brief close		hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware. Please use KDRV_SSENIF_ENGINE_*.
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ssenif_close(UINT32 chip, UINT32 engine);


/*!
 * @fn INT32 kdrv_ssenif_trigger(UINT32 id, BOOL enable)
 * @brief trigger hardware engine start/stop.
 * @param id		the handler of hardware
 * @param enable	ENABLE(/TRUE/1): Start engine.  DISABLE(/FALSE/0): Stop engine.
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_ssenif_trigger(UINT32 id, BOOL enable);

/*!
 * @fn INT32 kdrv_ssenif_set(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id		the handler of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_ssenif_set(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param);
#define kdrv_ssenif_set(id, param_id, p_param) __kdrv_ssenif_set((UINT32)(id), (KDRV_SSENIF_PARAM_ID)(param_id), (VOID *)(p_param))

/*!
 * @fn INT32 kdrv_ssenif_get(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id		the handler of hardware
 * @param param_id	the id of parameters
 * @param p_param	shall the (UINT32 *) or the address the get structure.
 * @return return 0 on success, -1 on error
 */
INT32 __kdrv_ssenif_get(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param);
#define kdrv_ssenif_get(id, param_id, p_param) __kdrv_ssenif_get((UINT32)(id), (KDRV_SSENIF_PARAM_ID)(param_id), (VOID *)(p_param))

#endif


