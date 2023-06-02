#ifndef _AD_NVP6188_H_
#define _AD_NVP6188_H_

#include "ad_drv.h"

/************************ Common information ************************/

// maximum number of chips which is supported by the external decoder driver
#define AD_NVP6188_CHIP_MAX 	4

// number of input channels in one chip
#define AD_NVP6188_VIN_MAX		4

// number of output ports in one chip
#define AD_NVP6188_VOUT_MAX 	2

// i2c (max) address bytes & data bytes
#define AD_NVP6188_I2C_ADDR_LEN 1
#define AD_NVP6188_I2C_DATA_LEN 1

// i2c data order. DISABLE: LSB, ENABLE: MSB
#define AD_NVP6188_I2C_DATA_MSB ENABLE


/************************** Ext information **************************/


typedef enum {
	AD_NVP6188_NC_MIPI_CLK_37_125Mhz,
	AD_NVP6188_NC_MIPI_CLK_74_25Mhz,
	AD_NVP6188_NC_MIPI_CLK_94_5Mhz,
	AD_NVP6188_NC_MIPI_CLK_148_5Mhz,
	AD_NVP6188_NC_MIPI_CLK_172_125Mhz,
	AD_NVP6188_NC_MIPI_CLK_175_5Mhz,
	AD_NVP6188_NC_MIPI_CLK_182_25Mhz,

	AD_NVP6188_NC_MIPI_CLK_MAX,
} AD_NVP6188_NC_MIPI_CLK_E;

// rewrite - NC_MIPI_BPS_E
typedef enum {
	AD_NVP6188_NC_MIPI_BPS_594Mbps,		// 74.25Mpps
	AD_NVP6188_NC_MIPI_BPS_756Mbps,
	AD_NVP6188_NC_MIPI_BPS_972Mbps,
	AD_NVP6188_NC_MIPI_BPS_1242Mbps,
	AD_NVP6188_NC_MIPI_BPS_1458Mbps,
	AD_NVP6188_NC_MIPI_BPS_297Mbps,		// 37.125Mpps
	AD_NVP6188_NC_MIPI_BPS_1188Mbps,	// 148.5Mpps

	AD_NVP6188_NC_MIPI_BPS_MAX,
} AD_NVP6188_NC_MIPI_BPS_E;

typedef enum {
	AD_NVP6188_PATTERN_COLOR_BLUE,
	AD_NVP6188_PATTERN_COLOR_WHITE,
	AD_NVP6188_PATTERN_COLOR_YELLOW,
	AD_NVP6188_PATTERN_COLOR_CYAN,
	AD_NVP6188_PATTERN_COLOR_GREEN,
	AD_NVP6188_PATTERN_COLOR_MAGENTA,
	AD_NVP6188_PATTERN_COLOR_RED,
	AD_NVP6188_PATTERN_COLOR_BLUE_DUMMY, // same as AD_NVP6188_PATTERN_COLOR_BLUE
	AD_NVP6188_PATTERN_COLOR_BLACK,
	AD_NVP6188_PATTERN_COLOR_GRAY,
	AD_NVP6188_PATTERN_COLOR_GRID,
	AD_NVP6188_PATTERN_COLOR_HORIZONTAL,
	AD_NVP6188_PATTERN_COLOR_VERTICAL,
	AD_NVP6188_PATTERN_COLOR_BW,
	AD_NVP6188_PATTERN_COLOR_MAX,
} AD_NVP6188_PATTERN_COLOR;

typedef enum {
	AD_NVP6188_STD_AHD,
	AD_NVP6188_STD_TVI,
	AD_NVP6188_STD_CVI,
} AD_NVP6188_STD;

typedef struct {
	UINT32 mode;
} AD_NVP6188_DET_RAW_MODE;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
	AD_NVP6188_STD std;
} AD_NVP6188_DET_VIN_MODE;

typedef struct {
	AD_NVP6188_DET_RAW_MODE raw_mode;
	AD_NVP6188_DET_VIN_MODE vin_mode;
} AD_NVP6188_DET_MAP;

typedef struct {
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
	AD_NVP6188_STD std;
} AD_NVP6188_VIN_MODE;

typedef struct {
	UINT32 FormatStandard; // NC_FORMAT_STANDARD_E
	UINT32 FormatResolution; // NC_FORMAT_RESOLUTION_E
	UINT32 FormatFps; // NC_FORMAT_FPS_E
} AD_NVP6188_CH_MODE;

typedef struct {
	AD_NVP6188_VIN_MODE vin_mode;
	AD_NVP6188_CH_MODE ch_mode;
} AD_NVP6188_VIN_MAP;

typedef struct {
} AD_NVP6188_VOUT_MODE;

typedef struct {
	UINT32 mode;
} AD_NVP6188_PORT_MODE;

typedef struct {
	AD_NVP6188_VOUT_MODE vout_mode;
	AD_NVP6188_PORT_MODE port_mode;
} AD_NVP6188_VOUT_MAP;

typedef struct {
	BOOL vloss;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} AD_NVP6188_DET_VIDEO_INFO;

typedef enum {
	AD_NVP6188_IMAGE_PARAM_TYPE_BRIGHTNESS,
	AD_NVP6188_IMAGE_PARAM_TYPE_CONTRAST,
	AD_NVP6188_IMAGE_PARAM_TYPE_HUE,
	AD_NVP6188_IMAGE_PARAM_TYPE_SATURATION,
	AD_NVP6188_IMAGE_PARAM_TYPE_SHARPNESS,
	AD_NVP6188_IMAGE_PARAM_TYPE_MAX,
} AD_NVP6188_IMAGE_PARAM_TYPE;

/*******************************************************************************
* vin_id: video input id                                                       *
* vout_id: video output id                                                     *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	AD_NVP6188_IMAGE_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6188_IMAGE_PARAM_INFO;

typedef enum {
	AD_NVP6188_AUDIO_PARAM_TYPE_VOLUME,
	AD_NVP6188_AUDIO_PARAM_TYPE_MUTE,
	AD_NVP6188_AUDIO_PARAM_TYPE_SAMPLE_RATE,
	AD_NVP6188_AUDIO_PARAM_TYPE_MAX,
} AD_NVP6188_AUDIO_PARAM_TYPE;

/*******************************************************************************
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	AD_NVP6188_AUDIO_PARAM_TYPE type;	///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6188_AUDIO_PARAM_INFO;

/*******************************************************************************
* vin_id: video input id                                                       *
* addr: read/write register address                                            *
* val: parameter value                                                         *
*******************************************************************************/
typedef struct {
	UINT32 vin_id;					///< [in]
	UINT32 addr;					///< [in]

	UINT32 val;						///< [in/out]
} AD_NVP6188_PTZ_REG_INFO;

typedef struct {
	BOOL is_inited;
	AD_NVP6188_DET_VIN_MODE det_vin_mode[AD_NVP6188_VIN_MAX];
} AD_NVP6188_INFO;


// external function
extern AD_DEV* ad_get_obj_nvp6188(void);
#endif

