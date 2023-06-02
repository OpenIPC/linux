/**
    Header file of Serial Sensor Interface DAL library

    Exported header file of Serial Sensor Interface DAL library.

    @file       dal_ssenif.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _DAL_SSENIF_H
#define _DAL_SSENIF_H

#if defined __UITRON || defined __ECOS
#include "Type.h"
#else
#include "kwrap/type.h"
#endif


//
//	CSI ITEMs
//

/**
    SSENIF CSI Object configuration options

    This is used in DAL_SSENIFOBJ .set_csi_config() and .get_csi_config() to configure the csi configurations.
    For none-MIPICSI SSENIF object .set_csi_config() and .get_csi_config()
    are illegal function call and would output warning messages.
*/
typedef enum {
	DAL_SSENIFCSI_CFGID_BASE = 0x00100000,  //   prevent wrong usage of the config id. Should not use this id.

	DAL_SSENIFCSI_CFGID_SENSORTYPE,         ///< Select Sensor Type as Non-HDR or HDR vendor type.
	///< Please use DAL_SSENIF_SENSORTYPE as input.
	DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK, ///< Sensor normal operation target MCLK frequency.
	DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK,   ///< Sensor real operation MCLK frequency provided.
	///< This can be used to down speed sensor frequency while debugging usage.
	///< And auto re-calculates internal interface parameters.
	DAL_SSENIFCSI_CFGID_DLANE_NUMBER,       ///< Configure data lane number for pixel receiving. Valid setting value is 1 or 2 or 4.
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT,       ///< Configure image height each frame sent to SIE. This value includes OB and invalid data lines.
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE,   ///< Configure image height each frame sent to SIE1. This value includes OB and invalid data lines.
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE2,  ///< Configure image height each frame sent to SIE2. This value includes OB and invalid data lines.
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE3,  ///< Reserved
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE4,  ///< Configure image height each frame sent to SIE4. This value includes OB and invalid data lines.
	DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE5,  ///< Configure image height each frame sent to SIE5. This value includes OB and invalid data lines.
	DAL_SSENIFCSI_CFGID_CLANE_SWITCH,       ///< For [CSI]  Please use DAL_SSENIF_CFGID_CLANE_CSI_USE_C0 or
	///<            DAL_SSENIF_CFGID_CLANE_CSI_USE_C2 select clock lane.
	///< For [CSI2] Please use DAL_SSENIF_CFGID_CLANE_CSI2_USE_C4 or
	///<            DAL_SSENIF_CFGID_CLANE_CSI2_USE_C6 select clock lane.
	///< [CSI3-8] Not valid setting.
	DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD,     ///< .wait_interrupt() timeout period. Unit in ms. Default is 1000 ms.
	DAL_SSENIFCSI_CFGID_STOP_METHOD,        ///< Select the stop behavior. Pleas use DAL_SSENIF_STOPMETHOD as input.

	DAL_SSENIFCSI_CFGID_IMGID_TO_SIE,       ///< [HDR] Select IMAGE frame ID sent to SIE1. Valid for CSI only.
	DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2,      ///< [HDR] Select IMAGE frame ID sent to SIE2. Valid for CSI/CSI2 only.
	DAL_SSENIFCSI_CFGID_IMGID_TO_SIE3,      ///< Reserved
	DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4,      ///< [HDR] Select IMAGE frame ID sent to SIE4. Valid for CSI/CSI2 only.
	DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5,      ///< [HDR] Select IMAGE frame ID sent to SIE4. Valid for CSI/CSI2 only.

	DAL_SSENIFCSI_CFGID_DATALANE0_PIN,      ///< Specify sensor's data lane-0 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input
	DAL_SSENIFCSI_CFGID_DATALANE1_PIN,      ///< Specify sensor's data lane-1 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input
	DAL_SSENIFCSI_CFGID_DATALANE2_PIN,      ///< Specify sensor's data lane-2 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input
	DAL_SSENIFCSI_CFGID_DATALANE3_PIN,      ///< Specify sensor's data lane-3 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input

	DAL_SSENIFCSI_CFGID_LPKT_MANUAL,        ///<[OPTIONAL] Manual1 Format enable/disable.
	DAL_SSENIFCSI_CFGID_MANUAL_FORMAT,      ///<[OPTIONAL] Select the depacking method for Manual1 packet.
	///<           Please use DAL_SSENIFCSI_MANUAL_DEPACK as input parameter.
	DAL_SSENIFCSI_CFGID_MANUAL_DATA_ID,     ///<[OPTIONAL] 6-bits Manual1 Format data identifier(DI).
	DAL_SSENIFCSI_CFGID_LPKT_MANUAL2,       ///<[OPTIONAL] Manual2 Format enable/disable.
	DAL_SSENIFCSI_CFGID_MANUAL2_FORMAT,     ///<[OPTIONAL] Select the depacking method for Manual2 packet.
	///<           Please use DAL_SSENIFCSI_MANUAL_DEPACK as input parameter.
	DAL_SSENIFCSI_CFGID_MANUAL2_DATA_ID,    ///<[OPTIONAL] 6-bits Manual2 Format data identifier(DI).
	DAL_SSENIFCSI_CFGID_LPKT_MANUAL3,       ///<[OPTIONAL] Manual3 Format enable/disable.
	DAL_SSENIFCSI_CFGID_MANUAL3_FORMAT,     ///<[OPTIONAL] Select the depacking method for Manual3 packet.
	///<           Please use DAL_SSENIFCSI_MANUAL_DEPACK as input parameter.
	DAL_SSENIFCSI_CFGID_MANUAL3_DATA_ID,    ///<[OPTIONAL] 6-bits Manual3 Format data identifier(DI).

	DAL_SSENIFCSI_CFGID_SENSOR_REAL_HSCLK,  ///< Sensor real operation MIPI-HS clock frequency.(unit:Kbps, Decimal)
	DAL_SSENIFCSI_CFGID_IADJ,               ///<[OPTIONAL] Select the IADJ level.(valid value: 0~3)

	DAL_SSENIFCSI_CFGID_CLANE_FORCE_HS, 	///<[OPTIONAL] Clock Lanes force HS enable/disable.
	DAL_SSENIFCSI_CFGID_HSDATAO_DLY, 	    ///<[OPTIONAL] enable hs data out delay

	DAL_SSENIFCSI_CFGID_SENSOR_FS_CNT,       ///<[OPTIONAL] get fs count

	DAL_SSENIFCSI_CFGID_MAX,
	ENUM_DUMMY4WORD(DAL_SSENIFCSI_CFGID)
} DAL_SSENIFCSI_CFGID;

/**
    Select CSI manual mode pixel depack method

    This is used in SSENIFOBJ .set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL*_FORMAT)
    and .get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL*_FORMAT) to configure the csi configurations.
*/
typedef enum {
	DAL_SSENIFCSI_MANUAL_DEPACK_8,          ///< Select the depacking method as RAW8.
	DAL_SSENIFCSI_MANUAL_DEPACK_10,         ///< Select the depacking method as RAW10.
	DAL_SSENIFCSI_MANUAL_DEPACK_12,         ///< Select the depacking method as RAW12.
	DAL_SSENIFCSI_MANUAL_DEPACK_14,         ///< Select the depacking method as RAW14.

	ENUM_DUMMY4WORD(DAL_SSENIFCSI_MANUAL_DEPACK)
} DAL_SSENIFCSI_MANUAL_DEPACK;

//
//	LVDS ITEMs
//

/**
    SSENIF LVDS Object configuration options

    This is used in DAL_SSENIFOBJ .set_lvds_config() and .get_lvds_config() to configure the lvds configurations.
    For none-LVDS SSENIF object .set_lvds_config() and .get_lvds_config()
    are illegal function call and would output warning messages.
*/
typedef enum {
	DAL_SSENIFLVDS_CFGID_BASE = 0x00200000, // prevent wrong usage of the config id. Should not use this id.

	DAL_SSENIFLVDS_CFGID_SENSORTYPE,        ///< Select Sensor Type as Non-HDR or HDR vendor type. Please use DAL_SSENIF_SENSORTYPE as input.
	DAL_SSENIFLVDS_CFGID_DLANE_NUMBER,      ///< Configure data lane number for pixel receiving. Valid setting value is 1/2/4/6/8.
	DAL_SSENIFLVDS_CFGID_CLANE_SWITCH,      ///< For [LVDS]  Please use DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4 or
	///<             DAL_SSENIF_CFGID_CLANE_LVDS_USE_C2C6 select clock lane.
	///< For [LVDS2] Please use DAL_SSENIF_CFGID_CLANE_LVDS2_USE_C4 or
	///<            DAL_SSENIF_CFGID_CLANE_LVDS2_USE_C6 select clock lane.
	///< [LVDS3-8] Not valid setting.
	DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH,       ///< RAW Pixel bit Width. Valid value is 8/10/12/14/16.
	DAL_SSENIFLVDS_CFGID_VALID_WIDTH,       ///< Image Pixel Width each line. (Including front OB and invalid pixel width)
	DAL_SSENIFLVDS_CFGID_VALID_HEIGHT,      ///< Image Height each frame. (Including front OB and invalid pixel lines)
	DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD,    ///< .wait_interrupt() timeout period. Unit in ms. Default is 1000 ms.
	DAL_SSENIFLVDS_CFGID_STOP_METHOD,       ///< Select the stop behavior. Pleas use DAL_SSENIF_STOPMETHOD as input.

	DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE,      ///< [HDR] Select IMAGE frame ID sent to SIE1. Valid for LVDS only.
	DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2,     ///< [HDR] Select IMAGE frame ID sent to SIE2. Valid for LVDS/LVDS2 only.
	DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE3,     ///< [HDR] Select IMAGE frame ID sent to SIE3. Valid for LVDS only.
	DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4,     ///< [HDR] Select IMAGE frame ID sent to SIE4. Valid for LVDS/LVDS2 only.
	DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE5,     ///< [HDR] Select IMAGE frame ID sent to SIE5. Valid for LVDS/LVDS2 only.

	DAL_SSENIFLVDS_CFGID_OUTORDER_0,        ///< Select sensor data lane 0 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_1,        ///< Select sensor data lane 1 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_2,        ///< Select sensor data lane 2 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_3,        ///< Select sensor data lane 3 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_4,        ///< Select sensor data lane 4 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_5,        ///< Select sensor data lane 5 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_6,        ///< Select sensor data lane 6 connection position. Please use DAL_SSENIF_LANESEL as input.
	DAL_SSENIFLVDS_CFGID_OUTORDER_7,        ///< Select sensor data lane 7 connection position. Please use DAL_SSENIF_LANESEL as input.

	DAL_SSENIFLVDS_CFGID_PIXEL_INORDER,     ///<[OPTIONAL] Select Pixel Data bit order MSb or LSb first. Use DAL_SSENIFLVDS_PIXEL_INORDER as input.
	///<The driver would auto select a suitable selection when SENSORTYPE is set. The driver may not set this settings.
	///<If the user want to change this setting, must be configured after DAL_SSENIFLVDS_CFGID_SENSORTYPE setup.
	DAL_SSENIFLVDS_CFGID_FSET_BIT,          ///<[OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	DAL_SSENIFLVDS_CFGID_IMGID_BIT0,        ///<[OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	DAL_SSENIFLVDS_CFGID_IMGID_BIT1,        ///<[OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	DAL_SSENIFLVDS_CFGID_IMGID_BIT2,        ///<[OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	DAL_SSENIFLVDS_CFGID_IMGID_BIT3,        ///<[OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.

	DAL_SSENIFLVDS_CFGID_MAX,
	ENUM_DUMMY4WORD(DAL_SSENIFLVDS_CFGID)
} DAL_SSENIFLVDS_CFGID;

/**
    SSENIF LVDS Object lane configuration options

    This is used in DAL_SSENIFOBJ .set_lvds_laneconfig() to configure the lvds configurations.
    For none-LVDS SSENIF object .set_lvds_laneconfig()
    are illegal function call and would output warning messages.
*/
typedef enum {
	DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT,  ///< Set The HD/VD default SYNC-CODE according to sensor type.
	///< Set DAL_SSENIFLVDS_CFGID_FSET_BIT/DAL_SSENIFLVDS_CFGID_IMGID_BIT0-3 according to sensor type.

	DAL_SSENIFLVDS_LANECFGID_CTRLWORD_HD,       ///< [OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	///< Set the sensor data lane HD SYNCCODE. If user is not sure how to set this, just use the .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT).
	DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD,       ///< [OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	///< Set the sensor data lane VD SYNCCODE. If user is not sure how to set this, just use the .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT).
	DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD2,      ///< [OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	///< Set the sensor data lane VD2 SYNCCODE. If user is not sure how to set this, just use the .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT).
	DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD3,      ///< [OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	///< Set the sensor data lane VD3 SYNCCODE. If user is not sure how to set this, just use the .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT).
	DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD4,      ///< [OPTIONAL] When using .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT), Driver would assign default value according to DAL_SSENIFLVDS_CFGID_SENSORTYPE.
	///< Set the sensor data lane VD4 SYNCCODE. If user is not sure how to set this, just use the .set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT).

	ENUM_DUMMY4WORD(DAL_SSENIFLVDS_LANECFGID)
} DAL_SSENIFLVDS_LANECFGID;

/**
    Select LVDS Pixel Data bit order MSb or LSb first

    This is used in SSENIFOBJ .set_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_INORDER)
    and .get_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_INORDER) to configure the lvds configurations.
*/
typedef enum {
	DAL_SSENIFLVDS_PIXEL_INORDER_LSB,   ///< LVDS Pixel Data bit order LSb first
	DAL_SSENIFLVDS_PIXEL_INORDER_MSB,   ///< LVDS Pixel Data bit order MSb first

	ENUM_DUMMY4WORD(DAL_SSENIFLVDS_PIXEL_INORDER)
} DAL_SSENIFLVDS_PIXEL_INORDER;

//
//	SLVSEC ITEMs
//

/**
    SSENIF SLVSEC Object configuration options

    This is used in DAL_SSENIFOBJ .set_slvsec_config() and .get_slvsec_config() to configure the slvsec configurations.
    For none-SLVSEC SSENIF object .set_slvsec_config() and .get_slvsec_config()
    are illegal function call and would output warning messages.
*/
typedef enum {
	DAL_SSENIFSLVSEC_CFGID_BASE = 0x00800000, // prevent wrong usage of the config id. Should not use this id.

	DAL_SSENIFSLVSEC_CFGID_SENSORTYPE,          ///< Set DAL_SSENIF_SENSORTYPE_SONY_NONEHDR(default) or DAL_SSENIF_SENSORTYPE_SONY_LI_DOL.
	DAL_SSENIFSLVSEC_CFGID_PIXEL_DEPTH,         ///< RAW Pixel bit Width. Valid value is 8/10/12/14/16.
	DAL_SSENIFSLVSEC_CFGID_DLANE_NUMBER,        ///< Configure data lane number for pixel receiving. Valid setting value is 1/2/4/6/8.
	DAL_SSENIFSLVSEC_CFGID_VALID_WIDTH,         ///< Image Pixel Width each line. (Including front OB and invalid pixel width)
	DAL_SSENIFSLVSEC_CFGID_VALID_HEIGHT,        ///< Image Height each frame. (Including front OB and invalid pixel lines)
	DAL_SSENIFSLVSEC_CFGID_SPEED,               ///< Please use DAL_SSENIFSLVSEC_SPEED_2304 (2.304Gbps) or DAL_SSENIFSLVSEC_SPEED_1152 (1.152Gbps).
	DAL_SSENIFSLVSEC_CFGID_POST_INIT,           ///< Apply this config after sensor start working.
	DAL_SSENIFSLVSEC_CFGID_TIMEOUT_PERIOD,      ///< .wait_interrupt() timeout period. Unit in ms. Default is 1000 ms.
	DAL_SSENIFSLVSEC_CFGID_STOP_METHOD,         ///< Use DAL_SSENIF_STOPMETHOD_IMMEDIATELY or DAL_SSENIF_STOPMETHOD_FRAME_END or DAL_SSENIF_STOPMETHOD_FRAME_END2.

	DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE,        //< [HDR] Select IMAGE frame ID sent to SIE1.
	DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE2,       //< [HDR] Select IMAGE frame ID sent to SIE2.

	DAL_SSENIFSLVSEC_CFGID_DATALANE0_PIN,       ///< Specify sensor's data lane-0 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D0.
	DAL_SSENIFSLVSEC_CFGID_DATALANE1_PIN,       ///< Specify sensor's data lane-1 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D1.
	DAL_SSENIFSLVSEC_CFGID_DATALANE2_PIN,       ///< Specify sensor's data lane-2 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D2.
	DAL_SSENIFSLVSEC_CFGID_DATALANE3_PIN,       ///< Specify sensor's data lane-3 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D3.
	DAL_SSENIFSLVSEC_CFGID_DATALANE4_PIN,       ///< Specify sensor's data lane-4 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D4.
	DAL_SSENIFSLVSEC_CFGID_DATALANE5_PIN,       ///< Specify sensor's data lane-5 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D5.
	DAL_SSENIFSLVSEC_CFGID_DATALANE6_PIN,       ///< Specify sensor's data lane-6 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D6.
	DAL_SSENIFSLVSEC_CFGID_DATALANE7_PIN,       ///< Specify sensor's data lane-7 is layout to ISP's which pad pin.
	///< Please use DAL_SSENIF_LANESEL as input. Default is DAL_SSENIF_LANESEL_HSI_D7.

	ENUM_DUMMY4WORD(DAL_SSENIFSLVSEC_CFGID)
} DAL_SSENIFSLVSEC_CFGID;

/**
    SLVSEC Operation Speed Selection

    The SLVSEC controller can only operate at two speeds: 2.304 or 1.152 Gbps.
    This is used at .set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SPEED) to
    tell the SLVSEC controller the sensor's output speed.
*/
typedef enum {
	DAL_SSENIFSLVSEC_SPEED_2304,    ///< 2.304 Gbps
	DAL_SSENIFSLVSEC_SPEED_1152,    ///< 1.152 Gbps

	ENUM_DUMMY4WORD(DAL_SSENIFSLVSEC_SPEED)
} DAL_SSENIFSLVSEC_SPEED;



//
//	Vx1 ITEMs
//

/**
    SSENIF Vx1 Object configuration options

    This is used in DAL_SSENIFOBJ .set_vx1_config() and .get_vx1_config() to configure the slvsec configurations.
    For none-Vx1 SSENIF object .set_vx1_config() and .get_vx1_config()
    are illegal function call and would output warning messages.
*/
typedef enum {
	DAL_SSENIFVX1_CFGID_BASE = 0x00400000, // prevent wrong usage of the config id. Should not use this id.

	/*
	    Below are valid for all kinds of Vx1 TX&sensors.
	*/
	DAL_SSENIFVX1_CFGID_TXTYPE,             ///< This configuration must be done after Vx1Obj.init() and before first Vx1Obj.open().
	///< Select Vx1 Transmitter TYPE. Please use DAL_SSENIFVX1_TXTYPE as input parameter.
	DAL_SSENIFVX1_CFGID_SENSORTYPE,         ///< Use DAL_SSENIF_SENSORTYPE as input parameter.
	///< Default is DAL_SSENIF_SENSORTYPE_OTHERS.
	DAL_SSENIFVX1_CFGID_DATAMUX_SEL,        ///< Please use DAL_SSENIFVX1_DATAMUX as input parameter.
	DAL_SSENIFVX1_CFGID_TIMEOUT_PERIOD,     ///< .wait_interrupt() timeout period. Unit in ms. Default is 1000 ms.
	DAL_SSENIFVX1_CFGID_LOCKCHG_CALLBACK,   ///< Please use the prototype of "void (*DRV_CB)(UINT32 uiEvent)" for callback function.
	DAL_SSENIFVX1_CFGID_I2C_SPEED,          ///< Specify i2c speed on the vx1 Tx board to sensor. Please use DAL_SSENIFVX1_I2CSPEED as input parameter.
	DAL_SSENIFVX1_CFGID_I2CNACK_CHECK,      ///< If "ENABLE":  The sensor I2C NACK status would be returned at .sensor_i2c_write(), but would down speed i2c transmission.
	///< If "DISABLE": The sensor I2C NACK status would be showed on the console with better i2c transmission speed.
	///< Default is DISABLE.
	DAL_SSENIFVX1_CFGID_SENSOR_SLAVEADDR,   ///< Sensor I2C "7-bits" Slave address.(Not 8 bits)
	DAL_SSENIFVX1_CFGID_SENREG_ADDR_BC,     ///< Set Sensor I2C command address field byte count. valid setting value 1~4.
	DAL_SSENIFVX1_CFGID_SENREG_DATA_BC,     ///< Set Sensor I2C command    data field byte count. valid setting value 1~4.

	/*
	    Below are only valid for .get_vx1_config() to get information
	*/
	DAL_SSENIFVX1_CFGID_GET_PLUG,           ///< Check Vx1 Cable is plug or not.
	///< This checking mechanism is through the sublink connecttion ok or not.
	DAL_SSENIFVX1_CFGID_GET_LOCK,
	DAL_SSENIFVX1_CFGID_GET_SPEED,          ///< Get Vx1 Mainlink speed.
	DAL_SSENIFVX1_CFGID_GET_START,          ///< Check the current status is START or STOP.

	/*
	    Below are the Tx-THCV241 / MIPI Sensor dedicated settings
	*/
	DAL_SSENIFVX1_CFGID_VALID_WIDTH,        ///< Image Pixel Width each line. (Including front OB and invalid pixel width)
	///< [Limitation]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_PIXEL_DEPTH,        ///< RAW Pixel bit Width. Valid value is 10/12.
	///< [Limitation]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_VX1LANE_NUMBER,     ///< Select Vx1 Data lane number. Only Tx-THCV241 has two lanes mode.
	///< Legal setting is 1 or 2 for  DAL_SSENIF_ID_VX1 object.
	///< Legal setting is 1 only for  DAL_SSENIF_ID_VX1_2 object.
	DAL_SSENIFVX1_CFGID_MIPILANE_NUMBER,    ///< Specify the MIPI Sensor data lane number. valid value is 1/2/4.
	///< [Limitation]: For MIPI-CSI-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_HDR_ENABLE,         ///< Select the Sensor HDR is ENABLE or DISABLE.
	///< [Limitation]: For MIPI-CSI-Sensor/Tx-THCV241 only. For DAL_SSENIF_ID_VX1 only.

	DAL_SSENIFVX1_CFGID_IMGID_TO_SIE,       ///< [HDR] Select IMAGE frame ID sent to SIE1.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_IMGID_TO_SIE2,      ///< [HDR] Select IMAGE frame ID sent to SIE2.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_IMGID_TO_SIE3,      ///< [HDR] Select IMAGE frame ID sent to SIE3.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_IMGID_TO_SIE4,      ///< [HDR] Select IMAGE frame ID sent to SIE4.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL0,       ///< Setup the THCV241 PLL settings item[0]. REG0x1011
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL1,       ///< Setup the THCV241 PLL settings item[1]. REG0x1012
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL2,       ///< Setup the THCV241 PLL settings item[2]. REG0x1013
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL3,       ///< Setup the THCV241 PLL settings item[3]. REG0x1014
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL4,       ///< Setup the THCV241 PLL settings item[4]. REG0x1015
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL5,       ///< Setup the THCV241 PLL settings item[5]. REG0x1016
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL6,       ///< Setup the THCV241 PLL settings item[6]. REG0x1036
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_PLL7,       ///< Setup the THCV241 PLL settings item[7]. REG0x1037
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_241_INPUT_CLK_FREQ, ///< THCV241 input OSC frequency.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only. Valid range is 10000000 ~ 40000000.
	DAL_SSENIFVX1_CFGID_SENSOR_TARGET_MCLK, ///< Sensor normal operation target MCLK frequency.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_SENSOR_REAL_MCLK,   ///< Sensor real operation MCLK frequency provided.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_SENSOR_CKSPEED_BPS, ///< Sensor clock lane speed in bps. For example enter 445000000 is clock speed is 445Mbps.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.
	DAL_SSENIFVX1_CFGID_THCV241_CKO_OUTOUT,	///< THCV241 CKO output enable.
	///< [Limitation]: For MIPI-CSI-HDR-Sensor/Tx-THCV241 only.

	ENUM_DUMMY4WORD(DAL_SSENIFVX1_CFGID)
} DAL_SSENIFVX1_CFGID;

/**
    THINE Vx1 TX Module Selection

    This is used in .set_vx1_config(DAL_SSENIFVX1_CFGID_TXTYPE) to specify Vx1 TX moudle.
    This setting must be done "after .init()" and "before .open()" to make setting activate.
    Currently only these three Vx1 modules are supported. If THINE's new TX module is ready to use,
    Please contack to novatek to add the new TX module to driver. And novatek would confirm that the new TX is supported or not.
*/
typedef enum {
	DAL_SSENIFVX1_TXTYPE_THCV235,   ///< Vx1 Transmitter IC is THCV235.
	///< This only support Vx1 One-lane none-HDR parallel sensor.
	DAL_SSENIFVX1_TXTYPE_THCV231,   ///< Vx1 Transmitter IC is THCV231.
	///< This only support Vx1 One-lane none-HDR parallel sensor.

	DAL_SSENIFVX1_TXTYPE_THCV241,   ///< Vx1 Transmitter IC is THCV241. This is MIPI CSI input interface Vx1 Tx.
	///< Which can support 1 or 2 lanes Vx1 interface. HDR MIPI Sensor such as SONY_LI or OV/ON-SEMI is also supported.
	///< The maximum MIPI Sensor support spec is 4 lanes mipi with 1Gbps per lane by using Vx1 2 lanes.
	///< The maximum MIPI Sensor support spec is 4 lanes mipi with 500Mbps per lane by using Vx1 1 lanes.
	ENUM_DUMMY4WORD(DAL_SSENIFVX1_TXTYPE)
} DAL_SSENIFVX1_TXTYPE;


/**
    VX1 Operation mode selection

    For THCV231/THCV235, the user shall use DAL_SSENIFVX1_DATAMUX_4BYTE_RAW for RAW10/RAW12 sensors,
    Use DAL_SSENIFVX1_DATAMUX_YUV8/16 for YUV sensors.
    For THCV241, the user shall use DAL_SSENIFVX1_DATAMUX_3BYTE_RAW for RAW10/RAW12 sensors,
    Use DAL_SSENIFVX1_DATAMUX_YUV16 for YUV sensors.
*/
typedef enum {
	DAL_SSENIFVX1_DATAMUX_3BYTE_RAW,    ///< Vx1 is operating at 3BYTE Muxing Mode with RAW10/RAW12 sensor in use.
	///< 1T D[23:0] to 2T "D[23:12]"+"D[11:0]".
	///< This shall be used for THCV241 with RAW sensor.
	DAL_SSENIFVX1_DATAMUX_4BYTE_RAW,    ///< Vx1 is operating at 4BYTE Muxing Mode with RAW10/RAW12 sensor in use.(default)
	///< 1T D[31:0] to 2T "D[27:16]"+"D[11:0]".
	///< This shall be used for THCV231/THCV235 with RAW sensor.

	DAL_SSENIFVX1_DATAMUX_YUV8,         ///< 1T D[23:0] to 1T "D[23:16]D[7:0]"-16bits
	///< This shall be used for THCV231/THCV235 with YUV sensor.
	DAL_SSENIFVX1_DATAMUX_YUV16,        ///< 1T D[31:0] to 2T "D[31:16]"+"D[15:0]".
	///< This shall be used for THCV231/THCV235/THCV241 with YUV sensor.

	DAL_SSENIFVX1_DATAMUX_NONEMUXING,   ///< D[11:0] to D[11:0]. Debugging usages.
	ENUM_DUMMY4WORD(DAL_SSENIFVX1_DATAMUX)
} DAL_SSENIFVX1_DATAMUX;


/**
    Set I2C Speed on the Vx1 Tx module (To Sensor)

    Thisi is used in .set_vx1_config(DAL_SSENIFVX1_CFGID_I2C_SPEED) to assign the Vx1 Tx module's I2C speed
    to sensor.
*/
typedef enum {
	DAL_SSENIFVX1_I2CSPEED_50KHZ,   ///< I2C Speed on Vx1 Transmitter: 50KHz
	DAL_SSENIFVX1_I2CSPEED_100KHZ,  ///< I2C Speed on Vx1 Transmitter: 100KHz (default)
	DAL_SSENIFVX1_I2CSPEED_200KHZ,  ///< I2C Speed on Vx1 Transmitter: 200KHz
	DAL_SSENIFVX1_I2CSPEED_400KHZ,  ///< I2C Speed on Vx1 Transmitter: 400KHz

	ENUM_DUMMY4WORD(DAL_SSENIFVX1_I2CSPEED)
} DAL_SSENIFVX1_I2CSPEED;

/**
    Vx1 TX module's I2C transmission status

    This is used at .sensor_i2c_write()/.sensor_i2c_read() as return value for the I2C transmission status.
*/
typedef enum {
	DAL_SSENIFVX1_I2CSTS_OK,            ///< I2C command transfer done and success. (Acked)
	DAL_SSENIFVX1_I2CSTS_NACK,          ///< I2C command transfer done but got NACK.
	///< The Nack result can be got only when DAL_SSENIFVX1_CFGID_I2CNACK_CHECK is ENABLE.
	DAL_SSENIFVX1_I2CSTS_ERROR,         ///< I2C command transfer not complete due to some errors,
	///< such as vx1 cable-unplug, driver no open, ...etc.

	ENUM_DUMMY4WORD(DAL_SSENIFVX1_I2CSTS)
} DAL_SSENIFVX1_I2CSTS;


/**
    Vx1 Tx Module's GPIO pin definition

    This is used at .set_vx1_gpio() / .get_vx1_gpio() to control the gereral purpose input/output pins.
    For THCV231/THCV235, there are two GPIO pins can be used: DAL_SSENIFVX1_GPIO_3 / DAL_SSENIFVX1_GPIO_4.
    For THCV241, there are four GPIO pins can be used: DAL_SSENIFVX1_GPIO_0 / DAL_SSENIFVX1_GPIO_1 / DAL_SSENIFVX1_GPIO_2 / DAL_SSENIFVX1_GPIO_3.
*/
typedef enum {
	DAL_SSENIFVX1_GPIO_0,   ///< Select Vx1 transmitter's GPIO_0. Valid for THCV241.
	DAL_SSENIFVX1_GPIO_1,   ///< Select Vx1 transmitter's GPIO_1. Valid for THCV241.
	DAL_SSENIFVX1_GPIO_2,   ///< Select Vx1 transmitter's GPIO_1. Valid for THCV241.
	DAL_SSENIFVX1_GPIO_3,   ///< Select Vx1 transmitter's GPIO_1. Valid for THCV241/THCV231/THCV235.
	DAL_SSENIFVX1_GPIO_4,   ///< Select Vx1 transmitter's GPIO_1. Valid for THCV231/THCV235.

	ENUM_DUMMY4WORD(DAL_SSENIFVX1_GPIO)
} DAL_SSENIFVX1_GPIO;






//
//	Global ITEMs
//
#ifdef __KERNEL__
#define SSENIF_CSI_EN ENABLE
#else
#define SSENIF_CSI_EN ENABLE
#endif

/**
    SSENIF IDentification

    This is used at dal_ssenif_get_object() to retrieve the specified SSENIF driver object.
    After retrieving the SSENIF driver object, the user shall call .init() first to start accessing.
*/
typedef enum {
#if SSENIF_CSI_EN
	DAL_SSENIF_ID_CSI,      ///< DAL of Serial Sensor Interface: MIPI CSI  module.
	DAL_SSENIF_ID_CSI2,     ///< DAL of Serial Sensor Interface: MIPI CSI2 module.
#endif

	DAL_SSENIF_ID_LVDS,     ///< DAL of Serial Sensor Interface: LVDS  module.
	DAL_SSENIF_ID_LVDS2,    ///< DAL of Serial Sensor Interface: LVDS2 module.

	DAL_SSENIF_ID_MAX,

	DAL_SSENIF_ID_CSI3,     ///< DAL of Serial Sensor Interface: MIPI CSI3 module.
	DAL_SSENIF_ID_CSI4,     ///< DAL of Serial Sensor Interface: MIPI CSI4 module.
	DAL_SSENIF_ID_CSI5,     ///< DAL of Serial Sensor Interface: MIPI CSI5 module.
	DAL_SSENIF_ID_CSI6,     ///< DAL of Serial Sensor Interface: MIPI CSI6 module.
	DAL_SSENIF_ID_CSI7,     ///< DAL of Serial Sensor Interface: MIPI CSI7 module.
	DAL_SSENIF_ID_CSI8,     ///< DAL of Serial Sensor Interface: MIPI CSI8 module.
	DAL_SSENIF_ID_LVDS3,    ///< DAL of Serial Sensor Interface: LVDS3 module.
	DAL_SSENIF_ID_LVDS4,    ///< DAL of Serial Sensor Interface: LVDS4 module.
	DAL_SSENIF_ID_LVDS5,    ///< DAL of Serial Sensor Interface: LVDS5 module.
	DAL_SSENIF_ID_LVDS6,    ///< DAL of Serial Sensor Interface: LVDS6 module.
	DAL_SSENIF_ID_LVDS7,    ///< DAL of Serial Sensor Interface: LVDS7 module.
	DAL_SSENIF_ID_LVDS8,    ///< DAL of Serial Sensor Interface: LVDS8 module.
	DAL_SSENIF_ID_VX1,      ///< DAL of Serial Sensor Interface: VX1   module.
	DAL_SSENIF_ID_VX1_2,    ///< DAL of Serial Sensor Interface: VX1_2 module.
	DAL_SSENIF_ID_SLVSEC,    ///< DAL of Serial Sensor Interface: VX1_2 module.

	ENUM_DUMMY4WORD(DAL_SSENIF_ID)
} DAL_SSENIF_ID;

/**
    Wait Interrupt Flags

    This is used at .wait_interrupt() as interupt representation flags.
    Multiple flags can be ORed together, And when one of flags is hit.
    The .wait_interrupt() would be returned and the waited flag would be the API return value.
    If the waiting flag time is too long without response, the timeout abort would be triggered and the API return with
    DAL_SSENIF_INTERRUPT_ABORT flag returned. The default time period is 1000ms. This timeout value can be changed by .set_xxx_config().
*/
typedef enum {
	DAL_SSENIF_INTERRUPT_VD         = 0x00000001,   ///< Valid for CSI/LVDS/SLVSEC/Vx1
	DAL_SSENIF_INTERRUPT_VD2        = 0x00000002,   ///< Valid for LVDS/SLVSEC/Vx1
	DAL_SSENIF_INTERRUPT_VD3        = 0x00000004,   ///< Valid for LVDS/Vx1
	DAL_SSENIF_INTERRUPT_VD4        = 0x00000008,   ///< Valid for LVDS/Vx1
	DAL_SSENIF_INTERRUPT_FRAMEEND   = 0x00000010,   ///< Valid for CSI/LVDS/SLVSEC
	DAL_SSENIF_INTERRUPT_FRAMEEND2  = 0x00000020,   ///< Valid for CSI/LVDS/SLVSEC
	DAL_SSENIF_INTERRUPT_FRAMEEND3  = 0x00000040,   ///< Valid for CSI/LVDS
	DAL_SSENIF_INTERRUPT_FRAMEEND4  = 0x00000080,   ///< Valid for CSI/LVDS
	DAL_SSENIF_INTERRUPT_ABORT      = 0x80000000,   ///< Waiting event timeout abort.

	DAL_SSENIF_INTERRUPT_SIE1_VD    	= 0x00000100,
	DAL_SSENIF_INTERRUPT_SIE2_VD    	= 0x00000200,
	DAL_SSENIF_INTERRUPT_SIE4_VD    	= 0x00000400,
	DAL_SSENIF_INTERRUPT_SIE5_VD    	= 0x00000800,
	DAL_SSENIF_INTERRUPT_SIE1_FRAMEEND	= 0x00001000,
	DAL_SSENIF_INTERRUPT_SIE2_FRAMEEND 	= 0x00002000,
	DAL_SSENIF_INTERRUPT_SIE4_FRAMEEND 	= 0x00004000,
	DAL_SSENIF_INTERRUPT_SIE5_FRAMEEND 	= 0x00008000,


	ENUM_DUMMY4WORD(DAL_SSENIF_INTERRUPT)
} DAL_SSENIF_INTERRUPT;

/**
    Image Sensor Vendor

    This is used to specify the image sensor vendor. The SSENIF uses this information to configure the internal settings automatically.
    If not one of this vendors, please try DAL_SSENIF_SENSORTYPE_OTHERS first.
*/
typedef enum {
	DAL_SSENIF_SENSORTYPE_SONY_NONEHDR, ///< SONY without HDR.
	DAL_SSENIF_SENSORTYPE_SONY_LI_DOL,  ///< SONY with Line-Information(LI) DOL HDR.
	DAL_SSENIF_SENSORTYPE_SONY_VC_DOL,  ///< SONY with Virtual-Channel(VC) DOL HDR.
	DAL_SSENIF_SENSORTYPE_OMNIVISION,   ///< Omni-Vision
	DAL_SSENIF_SENSORTYPE_ONSEMI,       ///< ON-Semi
	DAL_SSENIF_SENSORTYPE_PANASONIC,    ///< Panasonic

	DAL_SSENIF_SENSORTYPE_OTHERS,       ///< Sensor vendor not specified above.
	ENUM_DUMMY4WORD(DAL_SSENIF_SENSORTYPE)
} DAL_SSENIF_SENSORTYPE;

/**
    SSENIF stop behavior

    This is used to select the SSENIF driver object's .stop() behavior.
    Please refer to application note for details.
*/
typedef enum {
	DAL_SSENIF_STOPMETHOD_IMMEDIATELY,  ///< Stop immediately.
	DAL_SSENIF_STOPMETHOD_FRAME_END,    ///< Stop at frame-end1 interrupt.(default)
	DAL_SSENIF_STOPMETHOD_FRAME_END2,   ///< Stop at frame-end2 interrupt during HDR application.
	DAL_SSENIF_STOPMETHOD_FRAME_END3,   ///< Stop at frame-end3 interrupt during HDR application.
	DAL_SSENIF_STOPMETHOD_FRAME_END4,   ///< Stop at frame-end4 interrupt during HDR application.

	DAL_SSENIF_STOPMETHOD_SIE1_FRAMEEND,
	DAL_SSENIF_STOPMETHOD_SIE2_FRAMEEND,
	DAL_SSENIF_STOPMETHOD_SIE4_FRAMEEND,
	DAL_SSENIF_STOPMETHOD_SIE5_FRAMEEND,

	ENUM_DUMMY4WORD(DAL_SSENIF_STOPMETHOD)
} DAL_SSENIF_STOPMETHOD;

/**
    Clock Lane Switch Selection

    The MIPI-CSI 1/2 and LVDS 1/2 can switch clock lane position.
    Please select to correct position according to hardware circuit layout.
*/
typedef enum {
	DAL_SSENIF_CFGID_CLANE_CSI_USE_C0,      ///< MIPI CSI(1) uses HSI_CK0 as clock lane.
	DAL_SSENIF_CFGID_CLANE_CSI_USE_C2,      ///< MIPI CSI(1) uses HSI_CK2 as clock lane.
	DAL_SSENIF_CFGID_CLANE_CSI2_USE_C4,     ///< MIPI CSI2 uses HSI_CK4 as clock lane.
	DAL_SSENIF_CFGID_CLANE_CSI2_USE_C6,     ///< MIPI CSI2 uses HSI_CK6 as clock lane.

	DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4,   ///< LVDS(1) uses HSI_CK0 as clock lane in one clock lane sensor.
	///< LVDS(1) uses HSI_CK0+CK4 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	DAL_SSENIF_CFGID_CLANE_LVDS_USE_C2C6,   ///< LVDS(1) uses HSI_CK2 as clock lane in one clock lane sensor.
	///< LVDS(1) uses HSI_CK2+CK6 as clock lanes in two clock lanes sensor.(such as HiSPi 8 lanes)
	DAL_SSENIF_CFGID_CLANE_LVDS2_USE_C4,    ///< LVDS2 uses HSI_CK4 as clock lane.
	DAL_SSENIF_CFGID_CLANE_LVDS2_USE_C6,    ///< LVDS2 uses HSI_CK6 as clock lane.

	DAL_SSENIF_CFGID_CLANE_CSI2_USE_C1,		///< MIPI CSI2 uses HSI_CK1 as clock lane.
	DAL_SSENIF_CFGID_CLANE_LVDS2_USE_C1,	///< LVDS2 uses HSI_CK1 as clock lane.

} DAL_SSENIF_CFGID_CLANE;

/**
    SSENIF Data lane definition

    This is used at MIPI-CSI/LVDS/SLVSEC to spefify the sensor data lanes' hardware layout
    to which ISP's input pad pin.
*/
typedef enum {
	DAL_SSENIF_LANESEL_HSI_D0 = 0x0001,     ///< Package pin name: HSI_D0P / HSI_D0N
	DAL_SSENIF_LANESEL_HSI_D1 = 0x0002,     ///< Package pin name: HSI_D1P / HSI_D1N
	DAL_SSENIF_LANESEL_HSI_D2 = 0x0004,     ///< Package pin name: HSI_D2P / HSI_D2N
	DAL_SSENIF_LANESEL_HSI_D3 = 0x0008,     ///< Package pin name: HSI_D3P / HSI_D3N
	DAL_SSENIF_LANESEL_HSI_D4 = 0x0010,     ///< Package pin name: HSI_D4P / HSI_D4N
	DAL_SSENIF_LANESEL_HSI_D5 = 0x0020,     ///< Package pin name: HSI_D5P / HSI_D5N
	DAL_SSENIF_LANESEL_HSI_D6 = 0x0040,     ///< Package pin name: HSI_D6P / HSI_D6N
	DAL_SSENIF_LANESEL_HSI_D7 = 0x0080,     ///< Package pin name: HSI_D7P / HSI_D7N

	DAL_SSENIF_LANESEL_HSI_LOW4  = DAL_SSENIF_LANESEL_HSI_D0 | DAL_SSENIF_LANESEL_HSI_D1 | DAL_SSENIF_LANESEL_HSI_D2 | DAL_SSENIF_LANESEL_HSI_D3,
	DAL_SSENIF_LANESEL_HSI_HIGH4 = DAL_SSENIF_LANESEL_HSI_D4 | DAL_SSENIF_LANESEL_HSI_D5 | DAL_SSENIF_LANESEL_HSI_D6 | DAL_SSENIF_LANESEL_HSI_D7,
	DAL_SSENIF_LANESEL_HSI_ALL   = DAL_SSENIF_LANESEL_HSI_LOW4 | DAL_SSENIF_LANESEL_HSI_HIGH4,
	ENUM_DUMMY4WORD(DAL_SSENIF_LANESEL)
} DAL_SSENIF_LANESEL;


/**
    DAL of Serial Sensor Interface Object Definotions

    This is used by the DAL of Serial Sensor Interface Get Object API dal_ssenif_get_object() to get the specific module control object.
*/
typedef struct {

	/* Object Common items */
	DAL_SSENIF_ID   serial_senif_id;        ///< SSENIF Object ID.
	CHAR            name[32];               ///< SSENIF Object name.

	ER(*init)(void);                        ///< SSENIF Object init. This must be invoked first.
	ER(*open)(void);                        ///< SSENIF Object open.
	BOOL (*is_opened)(void);                ///< Check SSENIF Object is opened or not.
	ER(*close)(void);                       ///< SSENIF Object close.

	ER(*start)(void);                       ///< SSENIF Object start. This shall be invoked after all the settings are done.
	ER(*stop)(void);                        ///< SSENIF Object stop.

	DAL_SSENIF_INTERRUPT(*wait_interrupt)(DAL_SSENIF_INTERRUPT waited_flag);    ///< SSENIF Object wait interrupt event.

	void (*dump_debug_information)(void);   ///< SSENIF Object  dump debug information.

	/* CSI Dedicate API */
	void (*set_csi_config)(DAL_SSENIFCSI_CFGID config_id, UINT32 value);            ///< Set SSENIF CSI Object configuration.

	UINT32(*get_csi_config)(DAL_SSENIFCSI_CFGID config_id);                         ///< Get SSENIF CSI Object configuration.

	/* LVDS Dedicate API */
	void (*set_lvds_config)(DAL_SSENIFLVDS_CFGID config_id, UINT32 value);          ///< Set SSENIF LVDS Object configuration.

	UINT32(*get_lvds_config)(DAL_SSENIFLVDS_CFGID config_id);                       ///< Get SSENIF LVDS Object configuration.

	void (*set_lvds_laneconfig)(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value); ///< Set SSENIF LVDS Object lane configuration.

	/* SLVS-EC Dedicate API */
	void (*set_slvsec_config)(DAL_SSENIFSLVSEC_CFGID config_id, UINT32 value);      ///< Set SSENIF SLVSEC Object configuration.

	UINT32(*get_slvsec_config)(DAL_SSENIFSLVSEC_CFGID config_id);                   ///< Get SSENIF SLVSEC Object configuration.

	/* VX1 Dedicate API */
	void (*set_vx1_config)(DAL_SSENIFVX1_CFGID config_id, UINT32 value);            ///< Set SSENIF Vx1 Object configuration.

	UINT32(*get_vx1_config)(DAL_SSENIFVX1_CFGID config_id);                         ///< Get SSENIF Vx1 Object configuration.

	DAL_SSENIFVX1_I2CSTS(*sensor_i2c_write)(UINT32 reg_address, UINT32 reg_data);   ///< SSENIF Vx1 Object: I2C write command.

	DAL_SSENIFVX1_I2CSTS(*sensor_i2c_read)(UINT32 reg_address, UINT32 *reg_data);   ///< SSENIF Vx1 Object: I2C read command.

	DAL_SSENIFVX1_I2CSTS(*sensor_i2c_sequential_write)(UINT32 start_address, UINT32 total_data_length, UINT32 data_buffer_address); ///< SSENIF Vx1 Object: I2C sequential write command.

	void (*set_vx1_gpio)(DAL_SSENIFVX1_GPIO pin, BOOL level);                       ///< SSENIF Vx1 Object: Set GPIO output High/Low

	BOOL (*get_vx1_gpio)(DAL_SSENIFVX1_GPIO pin);                                   ///< SSENIF Vx1 Object: Get GPIO input High/Low

} DAL_SSENIFOBJ, *PDAL_SSENIFOBJ;


/*
    Exporting API
*/

/**
    Get SSENIF driver object

    Get SSENIF driver object

    @param[in] serial_senif_id   Specify the serial sensor controller type

    @return
     - NULL:   The specified SSENIF driver object not existed.
     - Others: The specified SSENIF driver object.
*/
extern PDAL_SSENIFOBJ dal_ssenif_get_object(DAL_SSENIF_ID serial_senif_id);

#endif

