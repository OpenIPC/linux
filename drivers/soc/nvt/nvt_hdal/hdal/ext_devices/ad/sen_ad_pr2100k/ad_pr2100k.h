#ifndef _AD_PR2100K_H_
#define _AD_PR2100K_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_PR2100K_CHIP_MAX 	2

// number of input channels in one chip
#define AD_PR2100K_VIN_MAX 	2

// number of output ports in one chip
#define AD_PR2100K_VOUT_MAX 	2

// i2c (max) address bytes & data bytes
#define AD_PR2100K_I2C_ADDR_LEN 1
#define AD_PR2100K_I2C_DATA_LEN 1

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_PR2100K_I2C_DATA_MSB ENABLE


/************************** Ext information **************************/

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
	UINT32 std;
} AD_PR2100K_VIN_MODE;

typedef struct {
	UINT32 mode;
	UINT32 std;
} AD_PR2100K_CH_MODE;

typedef struct {
	AD_PR2100K_VIN_MODE vin_mode;
	AD_PR2100K_CH_MODE ch_mode;
} AD_PR2100K_VIN_MAP;

typedef struct {
} AD_PR2100K_VOUT_MODE;

typedef struct {
} AD_PR2100K_PORT_MODE;

typedef struct {
	AD_PR2100K_VOUT_MODE vout_mode;
	AD_PR2100K_PORT_MODE port_mode;
} AD_PR2100K_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
	UINT32 std;
} AD_PR2100K_DET_VIDEO_INFO;

typedef enum {
	AD_PR2100K_IMAGE_PARAM_TYPE_MAX,
} AD_PR2100K_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	AD_PR2100K_IMAGE_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_PR2100K_IMAGE_PARAM_INFO;

typedef enum {
	AD_PR2100K_AUDIO_PARAM_TYPE_MAX,
} AD_PR2100K_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	AD_PR2100K_AUDIO_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_PR2100K_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	UINT32 addr;					///< [in]

	UINT32 val;						///< [in/out]
} AD_PR2100K_PTZ_REG_INFO;

typedef enum {
	AD_PR2100K_PATTERN_GEN_MODE_DISABLE,
	AD_PR2100K_PATTERN_GEN_MODE_AUTO,
	AD_PR2100K_PATTERN_GEN_MODE_FORCE,
} AD_PR2100K_PATTERN_GEN_MODE;

typedef struct {
	UINT32 count;
	UINT32 mode;
	UINT32 scan;
	UINT32 gain[4];
	UINT32 std;
	UINT32 state;
	UINT32 force;
} AD_PR2100K_WATCHDOG_INFO;

typedef struct {
	BOOL is_inited;
	AD_PR2100K_VIN_MODE raw_mode[AD_PR2100K_VIN_MAX];
	AD_PR2100K_VIN_MODE det_vin_mode[AD_PR2100K_VIN_MAX];
	AD_PR2100K_VIN_MODE cur_vin_mode[AD_PR2100K_VIN_MAX];
	UINT32 mipi_lane_num;
	UINT32 virtual_ch_id[AD_PR2100K_VIN_MAX];
	AD_PR2100K_PATTERN_GEN_MODE pat_gen_mode[AD_PR2100K_VIN_MAX];
	AD_PR2100K_WATCHDOG_INFO wdi[AD_PR2100K_VIN_MAX];
} AD_PR2100K_INFO;


// external function
extern AD_DEV* ad_get_obj_pr2100k(void);
#endif

