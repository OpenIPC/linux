#ifndef _AD_DUMMY_H_
#define _AD_DUMMY_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_DUMMY_CHIP_MAX 	4

// number of input channels in one chip
#define AD_DUMMY_VIN_MAX 	4

// number of output ports in one chip
#define AD_DUMMY_VOUT_MAX 	4

// i2c (max) address bytes & data bytes
#define AD_DUMMY_I2C_ADDR_LEN 1
#define AD_DUMMY_I2C_DATA_LEN 1

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_DUMMY_I2C_DATA_MSB ENABLE


/************************** Ext information **************************/

typedef struct {
	UINT32 mode;
} AD_DUMMY_DET_RAW_MODE;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_DUMMY_DET_VIN_MODE;

typedef struct {
	AD_DUMMY_DET_RAW_MODE raw_mode;
	AD_DUMMY_DET_VIN_MODE vin_mode;
} AD_DUMMY_DET_MAP;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_DUMMY_VIN_MODE;

typedef struct {
	UINT32 mode;
} AD_DUMMY_CH_MODE;

typedef struct {
	AD_DUMMY_VIN_MODE vin_mode;
	AD_DUMMY_CH_MODE ch_mode;
} AD_DUMMY_VIN_MAP;

typedef struct {
	AD_MUX_NUM mux;
} AD_DUMMY_VOUT_MODE;

typedef struct {
	UINT32 mode;
} AD_DUMMY_PORT_MODE;

typedef struct {
	AD_DUMMY_VOUT_MODE vout_mode;
	AD_DUMMY_PORT_MODE port_mode;
} AD_DUMMY_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_DUMMY_DET_VIDEO_INFO;

typedef enum {
	AD_DUMMY_IMAGE_PARAM_TYPE_BRIGHTNESS,
	AD_DUMMY_IMAGE_PARAM_TYPE_CONTRAST,
	AD_DUMMY_IMAGE_PARAM_TYPE_HUE,
	AD_DUMMY_IMAGE_PARAM_TYPE_SATURATION,
	AD_DUMMY_IMAGE_PARAM_TYPE_SHARPNESS,
	AD_DUMMY_IMAGE_PARAM_TYPE_MAX,
} AD_DUMMY_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	AD_DUMMY_IMAGE_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_DUMMY_IMAGE_PARAM_INFO;

typedef enum {
	AD_DUMMY_AUDIO_PARAM_TYPE_VOLUME,
	AD_DUMMY_AUDIO_PARAM_TYPE_MUTE,
	AD_DUMMY_AUDIO_PARAM_TYPE_SAMPLE_RATE,
	AD_DUMMY_AUDIO_PARAM_TYPE_MAX,
} AD_DUMMY_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	AD_DUMMY_AUDIO_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_DUMMY_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	UINT32 addr;					///< [in]

	UINT32 val;						///< [in/out]
} AD_DUMMY_PTZ_REG_INFO;

typedef struct {
	BOOL is_inited;
} AD_DUMMY_INFO;


// external function
extern AD_DEV* ad_get_obj_dummp(void);
#endif

