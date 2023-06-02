#ifndef _AD_INT_H_
#define _AD_INT_H_

//#include "sensor_cmd_if.h"
#include "kflow_videocapture/ctl_sen.h"
#define AD_MODULE_VERSION "1.01.011"

#define AD_CHIP_MAX CTL_SEN_ID_MAX
#define AD_PIN_CTRL_MAX 20
typedef struct {
	UINT32 pin;
	UINT32 lvl;
	UINT32 hold_time;
} AD_OUT_PIN_CTL;

typedef struct {
	UINT32 mclk_sel;
    UINT32 total_ctl_cnt;
    AD_OUT_PIN_CTL pin_ctrl[AD_PIN_CTRL_MAX];
} AD_PWR_CTL; //for linux/rtos

typedef struct {
	UINT32 ofst_x;
	UINT32 ofst_y;
} AD_CROP_OFST; //sie crop start


typedef struct {
	UINT32 chip_id;
	UINT32 vin_id;
	UINT32 vout_id;
	BOOL inited;
} AD_ID_MAP;

typedef struct {
	UINT32 vin_id;
	UINT32 data;
} AD_INFO_CFG;

/*******************************************************************************
* Data format                                                                  *
*******************************************************************************/
typedef enum {
	AD_DATA_FMT_422 = 0,
	AD_DATA_FMT_420,
	AD_DATA_FMT_MAX,
} AD_DATA_FMT;

/*******************************************************************************
* Data protocol                                                                  *
*******************************************************************************/
typedef enum {
	AD_DATA_PROTCL_CCIR601 = 0,
	AD_DATA_PROTCL_CCIR656,
	AD_DATA_PROTCL_CCIR709,
	AD_DATA_PROTCL_CCIR1120,
	AD_DATA_PROTCL_MAX,
} AD_DATA_PRTCL;

typedef enum {
	AD_DATA_ORDER_YUYV = 0,
	AD_DATA_ORDER_YVYU,
	AD_DATA_ORDER_UYVY,
	AD_DATA_ORDER_VYUY,
	AD_DATA_ORDER_MAX,
} AD_DATA_ORDER;

/*******************************************************************************
* Bus type	                                                                   *
*******************************************************************************/
typedef enum {
	AD_BUS_TYPE_PARALLEL = 0,
	AD_BUS_TYPE_SERIAL,
	AD_BUS_TYPE_MAX,
} AD_BUS_TYPE;


/*******************************************************************************
* Data bit depth                                                               *
*******************************************************************************/
typedef enum {
	AD_DATA_DEPTH_8BIT = 0,
	AD_DATA_DEPTH_16BIT,
	AD_DATA_DEPTH_MAX,
} AD_DATA_DEPTH;
/*******************************************************************************
* video output mux number                                                      *
*******************************************************************************/
typedef enum {
	AD_MUX_NUM_UNKNOWN = 0,
	AD_MUX_NUM_1 = 1,
	AD_MUX_NUM_2 = 2,
	AD_MUX_NUM_4 = 4,
	AD_MUX_NUM_8 = 8,
	AD_MUX_NUM_MAX,
} AD_MUX_NUM;

/*******************************************************************************
* MIPI lane number                                                             *
*******************************************************************************/
typedef enum {
	AD_MIPI_LANE_NUM_1 = 1,
	AD_MIPI_LANE_NUM_2 = 2,
	AD_MIPI_LANE_NUM_4 = 4,
} AD_MIPI_LANE_NUM;

/*******************************************************************************
* video detect type                                                            *
*******************************************************************************/
typedef enum {
	AD_TYPE_AUTO = 0, //
	AD_TYPE_AHD = 0x1, //bit 0
	AD_TYPE_TVI = 0x2, //bit 1
	AD_TYPE_CVI = 0x4, //bit 2
	AD_TYPE_SDI = 0x8, // bit 3
	AD_TYPE_MAX = 0x0f, // bit 0~3
} AD_DET_TYPE;


/*******************************************************************************
* ad information                        									   *
*******************************************************************************/
typedef struct {
	UINT32 mode_idx;
	UINT32 width;
	UINT32 height;
	UINT32 fps;
	BOOL prog;
} VDO_INFO;

typedef struct {
	BOOL active;
	BOOL vloss;

	VDO_INFO ui_format;
	VDO_INFO det_format;
	VDO_INFO cur_format;
} AD_VIN;

typedef struct {
	BOOL active;
	AD_BUS_TYPE bus_type;
	AD_DATA_FMT fmt;
	AD_DATA_PRTCL protocal;
	AD_DATA_DEPTH depth;
	AD_MUX_NUM mux_num;
	UINT32 clk_rate;
	UINT32 data_rate;

	// mipi
	UINT32 data_lane_num; // this mean the real HW data lanes, driver can use less than it.
	UINT32 virtual_ch;

	UINT32 vout2vin_map;
} AD_VOUT;

typedef struct {
	UINT32 mode; //[in]
	UINT32 out_id; //[in]
	AD_BUS_TYPE bus_type; //[out]
	AD_MUX_NUM mux_num; //[out] parallel
	UINT32 in_clock; //[out]
	UINT32 out_clock; //[out] parallel
	UINT32 data_rate; //[out]
	AD_MIPI_LANE_NUM mipi_lane_num; //[out] serial  //[out] serial, this mean the real data lanes which driver used
	AD_DATA_PRTCL protocal; //[out]
	AD_DATA_ORDER yuv_order; //[out]
	AD_CROP_OFST crop_start;
} AD_MODE_INFO;


#define AD_STS_UNKNOWN	0x00000000
#define AD_STS_INIT 	0x00000001
#define AD_STS_OPEN 	0x00000002

typedef struct {
	char *name;

	UINT32 vin_max;
	UINT32 vout_max;

	AD_VIN *vin;
	AD_VOUT *vout;

	UINT32 i2c_addr;
	UINT32 status;		//keep current status
	UINT32 rev[5];		//0: sensor_id
} AD_INFO;


/*******************************************************************************
* AD_CFGID item                         									   *
*******************************************************************************/
typedef struct {
	UINT32 vout_id;					///< [in]input port id
	AD_DATA_FMT fmt; 				///< [in/out]
	AD_DATA_PRTCL protocal;			///< [in/out]
} AD_VOUT_DATA_FMT_INFO;

typedef struct {
	UINT32 vin_id;					///< [in]input port id
	UINT32 vout_id;					///< [in]output port id
	UINT32 id;						///< [in/out]virtual channel id
} AD_VIRTUAL_CHANNEL_INFO;

typedef struct {
	UINT32 vin_id;					///< [in]input port id
	UINT32 vout_id;					///< [in]output port id
	UINT32 manual_idx;				///< [in] manual index, 0~2, CSI only support 3 manual setting
	CTL_SEN_MIPI_MANUAL_INFO data_info;	///< [in/out]manual CSI data type
} AD_MIPI_MANUAL_DATA_TYPE;

typedef enum {
	AD_CFGID_DATA_FORMAT = 0,			///< AD_VOUT_DATA_FMT_INFO
	AD_CFGID_VIRTUAL_CH_ID,			///< AD_VIRTUAL_CHANNEL_INFO
	AD_CFGID_MIPI_MANUAL_ID, 		///< AD_VIRTUAL_CHANNEL_INFO
	AD_CFGID_MODE_IDX_SEL, 			///< MODE_INFO : get mode_idx from driver
	AD_CFGID_MODE_INFO,				///< AD_MODE_INFO : get mode_info from driver
	AD_CFGID_I2C_FUNC, 				///<
    AD_CFGID_DET_CAM_TYPE,          ///< AD_DET_TYPE : assign multi-type for detection
	AD_CFGID_MAX,
} AD_CFGID;

/*******************************************************************************
* i2c_ch: i2c channel number of each chip group                                *
* i2c_addr: i2c slave address of each chip in the group                        *
*******************************************************************************/
typedef struct {
	UINT32 i2c_ch;
	UINT32 i2c_addr;
	AD_PWR_CTL pwr_info; //for Linux & Rtos
} AD_INIT_INFO;

/*******************************************************************************
* vin_id:input port index                                                      *
* vout_id:output port index                                                    *
* mode: sensor mode, just for driver reference                                 *
* ui_info: specify input setting                                               *
* cur_info: real ad setting                                                    *
* cur_update: should be set TRUE after changing mode correctly                  *
*******************************************************************************/
typedef struct {
	UINT32 vin_id; // [in]
	UINT32 vout_id; // [in]
	UINT32 mode; // [in]
	VDO_INFO ui_info; // [in]
	VDO_INFO cur_info; // [out]
	BOOL cur_update; //[out]
} AD_CHGMODE_INFO;

/*******************************************************************************
* vin_id:input port index                                                      *
* det_info: input detection                                                    *
* vin_loss: video signal loss                                                  *
*******************************************************************************/
typedef struct {
	UINT32 vin_id; //[in]
	VDO_INFO det_info; // [out]
	BOOL vin_loss; //[out]
} AD_DECET_INFO;

typedef enum {
	AD_I2C_W = 0,
	AD_I2C_R,
	AD_I2C_MAX,
} AD_I2C_RW;

typedef struct {
	UINT8 *data; // [in/out] put reg_addr+reg_value in data[0]~data[addr_len+data_len-1], I2C trasfer data[0] first; for I2c reading, received data will put in data[addr_len]~data[addr_len+data_len-1]
	UINT32 addr_len; //[in]
	UINT32 data_len; //[in]
} AD_I2C_INFO;

typedef ER (*ad_i2c_rw_if)(UINT32 chip_id, AD_I2C_RW rw_flg, AD_I2C_INFO i2c_data);

typedef struct {
	ER(*open)(UINT32 chip_id, void *ext_data);								//ext_data type: AD_INIT_INFO
	ER(*close)(UINT32 chip_id, void *ext_data);								//ext_data type: NULL
	ER(*init)(UINT32 chip_id, void *ext_data);								//ext_data type: NULL
	ER(*uninit)(UINT32 chip_id, void *ext_data);							//ext_data type: NULL
	ER(*get_cfg)(UINT32 chip_id, AD_CFGID item, void *data);				//reference AD_CFGID
	ER(*set_cfg)(UINT32 chip_id, AD_CFGID item, void *data);				//reference AD_CFGID
	ER(*chgmode)(UINT32 chip_id, void *ext_data);							//ext_data type: AD_CHGMODE_INFO
	ER(*det_plug_in)(UINT32 chip_id, void *ext_data);						//ext_data type: NULL
	ER(*i2c_write)(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
	ER(*i2c_read)(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
	ER(*pwr_ctl)(UINT32 chip_id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
	ER(*dbg_info)(char *str_cmd);	//ext_data type: NULL
} AD_OP;

typedef struct {
	UINT32 chip_max;
	AD_INFO *chip;

	UINT32 slav_tab_max;
	UINT32 *slav_addr_tab;

	AD_OP op;
} AD_DEV;
#endif
