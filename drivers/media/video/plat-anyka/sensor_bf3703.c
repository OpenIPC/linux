/**
 * @file camera_bf3703.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author luweichun
 * @date 2015-11-05
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		1 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0xdd 
#define SENSOR_ID               0x3703
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		640
#define SENSOR_OUTPUT_HEIGHT	480
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_YUV

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 2
#define EFFECT_FRAMES 	1
#define VSYNC_ACTIVE_MS			36

/////////////////////////////////BF3703 INTERAL REGISTERS//////////////////
//register_rule
#define BF_Rup			1
#define BF_Rdown		0

//	bit7	bit6	bit5	bit4	bit3	bit2	bit1	bit0
//register bit
#define BF_Rbit0		0
#define BF_Rbit1		1
#define BF_Rbit2		2
#define BF_Rbit3		3
#define BF_Rbit4		4
#define BF_Rbit5		5
#define BF_Rbit6		6
#define BF_Rbit7		7

/*---------register address--------*/
//infomation
#define BF_R_ID_MSB			0xfc
#define BF_R_ID_LSB			0xfd

#define BF_R_Device_addrss	0xfe

//BF mod control
#define BF_R_reset			0x12
#define BF_R_Standby		0x09

#define BF_R_data_Transfor	0xf1 //Enable the Data transfor

//BF Setting 
#define BF_R_ClkRC			0x11

//BF Common Control
#define BF_R_com2			0x09
#define BF_R_com3			0xdc
#define BF_R_com4			0x0b
#define BF_R_com5			0x0a
#define BF_R_com6			0x10
#define BF_R_com7			0x12
#define BF_R_com8			0x13
#define BF_R_com10			0x15

#define BF_R_TDreg			0x20

//BF test 
#define BF_R_Test			0xbb
#define BF_R_light			0x32

/*----------------------------------BF3703 mod switch-----------------------------------------*/
//Setting Resolution
#define BF_VGA_Resolution_YUV		0		// 30fps VGA YUV
#define BF_VGA_Resolution_Basic		1		// 30fps VGA RGB565/ RGB555/ RGB444
#define	BF_VGA_Resolution_RGB_R		2		// 30fps VGA Raw Bayer RGB mode
#define BF_VGA_Resolution_RGB_P		3		// 30fps VGA Processed RGB mode

#define BF_QVGA_Resolution_YUV		4
#define BF_QVGA_Resolution_Basic	5
#define BF_QVGA_Resolution_RGB_R	6
#define BF_QVGA_Resolution_RGB_P	7
//????QVGA ?Ƚ????⻹֧?ֶ???֮һ?Ĳ???????????Ϊ?˲????Ȳ??????壬????Ԥ??4????λ??????8 9 10 11
/*------------Array Control---------------*/
//Setting Mirror
#define BF_Mirror_H			12
#define BF_Mirror_V			13
#define BF_Mirror_nor		14

#define BF_Subsample_all	15
#define BF_Subsample_half	16

//Time Setting
#define BF_Mclk_normal		17
#define BF_Vclk_normal		18
#define BF_Vclk_half		19

//Exposure
#define BF_Exposure_AUTO	20


/*------------Common Control---------------*/
//com2
#define BF_com2_DCHoutput		0
#define BF_com2_Vclkoutput		1
#define BF_com2_Hsycoutput		2
#define BF_com2_Dataoutput		3
#define BF_com2_standbymod		4

//com3
#define BF_com3_Procress_RGB		5
#define BF_com3_Procress_color		6
#define BF_com3_Procress_L1GRGR		7
#define BF_com3_Procress_L1RGRG		8
#define BF_com3_Procress_L1BGBG		9
#define BF_com3_Procress_L1GBGB		10
#define BF_com3_HREF_swtich_on		11
#define BF_com3_HREF_switch_off		12
//com4

//com5

//com6

//com7

//com8
#define BF_com8_INT_TIM				13
#define BF_com8_AGC					14
#define BF_com8_AWB					15
#define BF_com8_AEC					16

//com10

static int bf3703_cmos_updata_d_gain(const unsigned int d_gain);
static int bf3703_cmos_updata_a_gain(const unsigned int a_gain);
static int bf3703_cmos_updata_exp_time(unsigned int exp_time);
static int bf3703_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;

static int bf3703_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int bf3703_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static AK_ISP_SENSOR_REG_INFO BF3703_YUV_640X480[]=
{
{0x12,0x80},
{0x15,0x02},
{0x09,0x01},
{0x1e,0x00},
{0x13,0x00},
{0x01,0x14},
{0x02,0x22},
{0x8c,0x02},
{0x8d,0x64},
{0x87,0x18},
{0x13,0x07},
{0x05,0x1f},
{0x06,0x60},
{0x14,0x20},
{0x27,0x03},
{0x06,0xe0},
{0x11,0x80},
{0x2b,0x00},
{0x92,0xea},  
{0x93,0x01},
{0x9d,0x99},
{0x9e,0x7f},
{0xeb,0x30},
{0xbb,0x20},
{0xf5,0x21},
{0xe1,0x3c},
{0x16,0x03},
{0x2f,0xf6},
{0x33,0x20},
{0x34,0x08},
{0x35,0x58},
{0x65,0x58},
{0x66,0x52},
{0x36,0x05},
{0x37,0xf6},
{0x38,0x46},
{0x9b,0xf6},
{0x9c,0x46},
{0xbc,0x01},
{0xbd,0xf6},
{0xbe,0x46},
{0x70,0x6f},
{0x72,0x4f},
{0x73,0x2f},
{0x74,0x27},
{0x77,0x90},
{0x7a,0x4e},
{0x7b,0x28},
{0x82,0x14},
{0x83,0x23},
{0x9a,0x23},
{0x84,0x1a},
{0x85,0x20},
{0x89,0x02},
{0x8a,0x64},
{0x86,0x28},
{0x96,0xa6},
{0x97,0x0c},
{0x98,0x18},
{0x80,0x55},
{0x24,0x88},
{0x25,0x78},
{0x69,0x00},
{0x94,0x0a},
{0x1F,0x20},
{0x22,0x20},
{0x26,0x20},
{0x56,0x40},
{0x61,0xd3},
{0x79,0x48},
{0x3b,0x60},
{0x3c,0x28},
{0x39,0x80},
{0x3f,0xA0},
{0x39,0x80},
{0x40,0x58},
{0x41,0x54},
{0x42,0x4E},
{0x43,0x44},
{0x44,0x3E},
{0x45,0x39},
{0x46,0x35},
{0x47,0x31},
{0x48,0x2E},
{0x49,0x2B},
{0x4b,0x29},
{0x4c,0x27},
{0x4e,0x23},
{0x4f,0x20},
{0x50,0x1E},
{0x51,0x0d},
{0x52,0x21},
{0x53,0x14},
{0x54,0x15},
{0x57,0x8d},
{0x58,0x78},
{0x59,0x5f},
{0x5a,0x84},
{0x5b,0x25},
{0x5c,0x0e},
{0x5d,0x95},
{0x60,0x20},
{0xb0,0xd0},
{0xb1,0xc0},
{0xb2,0xa8},
{0xb3,0x88},
{0x6a,0x81},
{0x23,0x66},
{0xa0,0x03},
{0x06,0xe0},
{0xa1,0x31},
{0xa2,0x0e},
{0xa3,0x26},
{0xa4,0x0d},
{0xa5,0x26},
{0xa6,0x06},
{0xa7,0x80},
{0xa8,0x7e},
{0xa9,0x28},
{0xaa,0x28},
{0xab,0x28},
{0xac,0x3c},
{0xad,0xf0},
{0xc8,0x18},
{0xc9,0x20},
{0xca,0x17},
{0xcb,0x1f},
{0xaf,0x00},
{0xc5,0x18},
{0xc6,0x00},
{0xc7,0x20},
{0xae,0x83},
{0xcc,0x40},
{0xcd,0x50},
{0xee,0x4c},
{0x8e,0x03},
{0x8f,0x96},
{0x3a,0x02},
{0xff,0xff},
};

static int bf3703_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = BF3703_YUV_640X480;//para->reg_info;
	for (i = 0; i < sizeof(BF3703_YUV_640X480) / sizeof(BF3703_YUV_640X480[0]); i++) {
//value=bf3703_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		bf3703_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=bf3703_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	g_fps = MAX_FPS;
	to_fps = MAX_FPS;
	return 0;
}

static int bf3703_read_id(void)
{
    return SENSOR_ID;
}

static int bf3703_probe_id(void)
{
    u8 value;
    u32 id;

    value = bf3703_sensor_read_register(BF_R_ID_MSB);
    id = value << 8;

    value = bf3703_sensor_read_register(BF_R_ID_LSB);
    id |= value;

	printk(KERN_ERR "%s id:0x%x\n", __func__, id);

    //return SENSOR_ID;
	if (id != SENSOR_ID)
		goto fail;

    return SENSOR_ID;

fail:
	return 0;
}

static int bf3703_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int bf3703_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int bf3703_get_fps(void)
{
	return g_fps;
}

static int bf3703_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type bf3703_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int bf3703_get_parameter(int param, void *value)
{
	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {
		case GET_VSYNC_ACTIVE_MS:
			*((int *)value) = VSYNC_ACTIVE_MS;
			break;

		case GET_CUR_FPS:
			*((int *)value) = g_fps;
			break;

		default:
			ret = -1;
			break;
	}

	return ret;
}

static int bf3703_set_power_on(const int pwdn_pin, const int reset_pin)
{  
    ak_sensor_set_pin_as_gpio(pwdn_pin);
    ak_sensor_set_pin_dir(pwdn_pin, 1);
    ak_sensor_set_pin_level(pwdn_pin, !SENSOR_PWDN_LEVEL);    
    ak_sensor_mdelay(10);
    ak_sensor_set_pin_as_gpio(reset_pin);
    ak_sensor_set_pin_dir(reset_pin, 1);
    ak_sensor_set_pin_level(reset_pin, SENSOR_RESET_LEVEL);
    ak_sensor_mdelay(30);   //modify reset time, used to solve no camera interrupt problem
    ak_sensor_set_pin_level(reset_pin, !SENSOR_RESET_LEVEL);
    ak_sensor_mdelay(20);

	g_fps = MAX_FPS;
	
	return 0;
}

static int bf3703_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int bf3703_set_fps(int fps)
{
	int ret = 0;

	switch (fps) {
		case 25:
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int bf3703_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int bf3703_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB bf3703_callback = 
{
	.sensor_init_func 				= bf3703_init,
	.sensor_read_reg_func			= bf3703_sensor_read_register,
	.sensor_write_reg_func			= bf3703_sensor_write_register,
	.sensor_read_id_func			= bf3703_read_id,
	.sensor_update_a_gain_func		= bf3703_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= bf3703_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= bf3703_cmos_updata_exp_time,
	.sensor_timer_func				= bf3703_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= bf3703_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= bf3703_get_resolution,
	.sensor_get_mclk_func			= bf3703_get_mclk,
	.sensor_get_fps_func			= bf3703_get_fps,
	.sensor_get_valid_coordinate_func	= bf3703_get_valid_coordinate,
	.sensor_get_bus_type_func		= bf3703_get_bus_type,
	.sensor_get_parameter_func		= bf3703_get_parameter,

	.sensor_set_power_on_func		= bf3703_set_power_on,
	.sensor_set_power_off_func		= bf3703_set_power_off,
	.sensor_set_fps_func			= bf3703_set_fps,
	.sensor_set_standby_in_func		= bf3703_set_standby_in,
	.sensor_set_standby_out_func	= bf3703_set_standby_out
};

AK_SENSOR_MODULE(bf3703_callback, bf3703)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
static int bf3703_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static int bf3703_cmos_updata_a_gain(const unsigned int a_gain)
{
	return 0;
}

static int bf3703_cmos_updata_exp_time(unsigned int exp_time)
{
    return EFFECT_FRAMES;
}

static int bf3703_cmos_update_a_gain_timer(void)
{
	return 0;
}
