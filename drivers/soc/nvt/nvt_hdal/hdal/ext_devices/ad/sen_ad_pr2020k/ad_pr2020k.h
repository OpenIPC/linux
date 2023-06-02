#ifndef _AD_PR2020K_H_
#define _AD_PR2020K_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_PR2020K_CHIP_MAX 	4

// number of input channels in one chip
#define AD_PR2020K_VIN_MAX 		1

// number of output ports in one chip
#define AD_PR2020K_VOUT_MAX 	1

// i2c (max) address bytes & data bytes
#define AD_PR2020K_I2C_ADDR_LEN 1
#define AD_PR2020K_I2C_DATA_LEN 1

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_PR2020K_I2C_DATA_MSB ENABLE

/************************** Ext information **************************/

typedef enum {
	AD_PR2020K_VMODE_720P_25,
	AD_PR2020K_VMODE_720P_30,
	AD_PR2020K_VMODE_1080P_25,
	AD_PR2020K_VMODE_1080P_30,
} AD_PR2020K_VMODE;

typedef struct {
	UINT32 mode;
} AD_PR2020K_DET_RAW_MODE;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_PR2020K_DET_VIN_MODE;

typedef struct {
	AD_PR2020K_DET_RAW_MODE raw_mode;
	AD_PR2020K_DET_VIN_MODE vin_mode;
} AD_PR2020K_DET_MAP;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_PR2020K_VIN_MODE;

typedef struct {
	UINT32 mode;
} AD_PR2020K_CH_MODE;

typedef struct {
	AD_PR2020K_VIN_MODE vin_mode;
	AD_PR2020K_CH_MODE ch_mode;
} AD_PR2020K_VIN_MAP;

typedef struct {
} AD_PR2020K_VOUT_MODE;

typedef struct {
} AD_PR2020K_PORT_MODE;

typedef struct {
	AD_PR2020K_VOUT_MODE vout_mode;
	AD_PR2020K_PORT_MODE port_mode;
} AD_PR2020K_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_PR2020K_DET_VIDEO_INFO;

typedef enum {
	AD_PR2020K_AUDIO_PARAM_TYPE_VOLUME,
	AD_PR2020K_AUDIO_PARAM_TYPE_MUTE,
	AD_PR2020K_AUDIO_PARAM_TYPE_SAMPLE_RATE,
	AD_PR2020K_AUDIO_PARAM_TYPE_MAX,
} AD_PR2020K_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	AD_PR2020K_AUDIO_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_PR2020K_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	UINT32 addr;					///< [in]

	UINT32 val;						///< [in/out]
} AD_PR2020K_PTZ_REG_INFO;

typedef struct {
	BOOL is_inited;
} AD_PR2020K_INFO;


// external function
extern AD_DEV* ad_get_obj_pr2020k(void);
#endif

