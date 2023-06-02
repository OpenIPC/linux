#ifndef _AD_NVP6124B_H_
#define _AD_NVP6124B_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_NVP6124B_CHIP_MAX 	4

// number of input channels in one chip
#define AD_NVP6124B_VIN_MAX 	4

// number of output ports in one chip
#define AD_NVP6124B_VOUT_MAX 	2

// i2c (max) address bytes & data bytes
#define AD_NVP6124B_I2C_ADDR_LEN 1
#define AD_NVP6124B_I2C_DATA_LEN 1

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_NVP6124B_I2C_DATA_MSB ENABLE


/************************** Ext information **************************/

typedef enum {
	AD_NVP6124B_RATE_37M,
	AD_NVP6124B_RATE_74M,
	AD_NVP6124B_RATE_148M,
	AD_NVP6124B_RATE_297M
} AD_NVP6124B_RATE;

typedef struct {
	UINT32 mode;
} AD_NVP6124B_DET_RAW_MODE;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_NVP6124B_DET_VIN_MODE;

typedef struct {
	AD_NVP6124B_DET_RAW_MODE raw_mode;
	AD_NVP6124B_DET_VIN_MODE vin_mode;
} AD_NVP6124B_DET_MAP;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_NVP6124B_VIN_MODE;

typedef struct {
	UINT32 mode;
	BOOL pal;
	AD_NVP6124B_RATE rate;
} AD_NVP6124B_CH_MODE;

typedef struct {
	AD_NVP6124B_VIN_MODE vin_mode;
	AD_NVP6124B_CH_MODE ch_mode;
} AD_NVP6124B_VIN_MAP;

typedef struct {
	AD_NVP6124B_RATE rate;
	AD_MUX_NUM mux;
} AD_NVP6124B_VOUT_MODE;

typedef struct {
	UINT32 mode;
} AD_NVP6124B_PORT_MODE;

typedef struct {
	AD_NVP6124B_VOUT_MODE vout_mode;
	AD_NVP6124B_PORT_MODE port_mode;
} AD_NVP6124B_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_NVP6124B_DET_VIDEO_INFO;

typedef enum {
	AD_NVP6124B_IMAGE_PARAM_TYPE_BRIGHTNESS,
	AD_NVP6124B_IMAGE_PARAM_TYPE_CONTRAST,
	AD_NVP6124B_IMAGE_PARAM_TYPE_HUE,
	AD_NVP6124B_IMAGE_PARAM_TYPE_SATURATION,
	AD_NVP6124B_IMAGE_PARAM_TYPE_SHARPNESS,
	AD_NVP6124B_IMAGE_PARAM_TYPE_MAX,
} AD_NVP6124B_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	AD_NVP6124B_IMAGE_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6124B_IMAGE_PARAM_INFO;

typedef enum {
	AD_NVP6124B_AUDIO_PARAM_TYPE_VOLUME,
	AD_NVP6124B_AUDIO_PARAM_TYPE_MUTE,
	AD_NVP6124B_AUDIO_PARAM_TYPE_SAMPLE_RATE,
	AD_NVP6124B_AUDIO_PARAM_TYPE_MAX,
} AD_NVP6124B_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	AD_NVP6124B_AUDIO_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6124B_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	UINT32 addr;					///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6124B_PTZ_REG_INFO;

typedef struct {
	BOOL is_inited;
} AD_NVP6124B_INFO;


// external function
extern AD_DEV* ad_get_obj_nvp6124b(void);
#endif

