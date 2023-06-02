#ifndef _AD_TC358743_H_
#define _AD_TC358743_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_TC358743_CHIP_MAX 	4

// number of input channels in one chip
#define AD_TC358743_VIN_MAX 	1

// number of output ports in one chip
#define AD_TC358743_VOUT_MAX 	1

// i2c (max) address bytes & data bytes
#define AD_TC358743_I2C_ADDR_LEN 2
#define AD_TC358743_I2C_DATA_LEN 4

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_TC358743_I2C_DATA_MSB DISABLE


/************************** Ext information **************************/

typedef struct {
} AD_TC358743_DET_RAW_MODE;

typedef struct {
} AD_TC358743_DET_VIN_MODE;

typedef struct {
} AD_TC358743_DET_MAP;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	UINT32 lane_num;
} AD_TC358743_VIN_MODE;

typedef struct {
	UINT32 fbd;
	UINT32 prd;
	UINT32 fifo_delay;
} AD_TC358743_CH_MODE;

typedef struct {
	AD_TC358743_VIN_MODE vin_mode;
	AD_TC358743_CH_MODE ch_mode;
} AD_TC358743_VIN_MAP;

typedef struct {
} AD_TC358743_VOUT_MODE;

typedef struct {
} AD_TC358743_PORT_MODE;

typedef struct {
} AD_TC358743_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_TC358743_DET_VIDEO_INFO;

/*******************************************************************************
* bank_addr: decoder register which used to switch the bank number             *
* bank: id-bank number which will be wrote to bank_addr. len-number of         *
*   registers to be read in this bank. is_end-set to TRUE next to the last     *
*   command                                                                    *
* dump_per_row: number of register to be dump in a row                         *
*******************************************************************************/
typedef struct {
} AD_TC358743_REG_BANK;

typedef struct {
} AD_TC358743_REG_DUMP_INFO;

typedef enum {
	AD_TC358743_IMAGE_PARAM_TYPE_MAX,
} AD_TC358743_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358743_IMAGE_PARAM_INFO;

typedef enum {
	AD_TC358743_AUDIO_PARAM_TYPE_MAX,
} AD_TC358743_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358743_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
} AD_TC358743_PTZ_REG_INFO;

typedef struct {
	BOOL connect; // current connected status
	BOOL _422;
	UINT32 pxclk;
	UINT32 h_total;
	UINT32 v_total;
	UINT32 h_blank;
	UINT32 v_blank;
	UINT32 fbd;
	UINT32 prd;
	UINT32 fifo_delay;
} AD_TC358743_INFO;


// external function
extern AD_DEV* ad_get_obj_tc358743(void);
#endif

