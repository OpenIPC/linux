#ifndef _AD_TC358840_H_
#define _AD_TC358840_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_TC358840_CHIP_MAX 	4

// number of input channels in one chip
#define AD_TC358840_VIN_MAX 	1

// number of output ports in one chip
#define AD_TC358840_VOUT_MAX 	1

// i2c (max) address bytes & data bytes
#define AD_TC358840_I2C_ADDR_LEN 2
#define AD_TC358840_I2C_DATA_LEN 4

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_TC358840_I2C_DATA_MSB DISABLE


/************************** Ext information **************************/

typedef struct {
} AD_TC358840_DET_RAW_MODE;

typedef struct {
} AD_TC358840_DET_VIN_MODE;

typedef struct {
} AD_TC358840_DET_MAP;

typedef struct {
} AD_TC358840_VIN_MODE;

typedef struct {
} AD_TC358840_CH_MODE;

typedef struct {
} AD_TC358840_VIN_MAP;

typedef struct {
} AD_TC358840_VOUT_MODE;

typedef struct {
} AD_TC358840_PORT_MODE;

typedef struct {
} AD_TC358840_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_TC358840_DET_VIDEO_INFO;

typedef enum {
	AD_TC358840_IMAGE_PARAM_TYPE_MAX,
} AD_TC358840_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358840_IMAGE_PARAM_INFO;

typedef enum {
	AD_TC358840_AUDIO_PARAM_TYPE_MAX,
} AD_TC358840_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358840_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358840_PTZ_REG_INFO;

typedef struct {
	BOOL connect; // current connected status
	BOOL _422;
	UINT32 pxclk;
	UINT32 h_total;
	UINT32 v_total;
	UINT32 h_blank;
	UINT32 v_blank;
} AD_TC358840_INFO;


// external function
extern AD_DEV* ad_get_obj_tc358840(void);
#endif

