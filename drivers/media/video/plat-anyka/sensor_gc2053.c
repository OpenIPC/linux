/**
 * @file sensor_gc2053.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author ye_guohong 
 * @date
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		1 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x6e 
#define SENSOR_ID               0x2053
#define SENSOR_MCLK             27		//dvp 27
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define SENSOR_OUTPUT_WIDTH		1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
//#define SENSOR_IO_INTERFACE     DVP_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			31
#define MIPI_MBPS		594	
#define MIPI_LANE		2

static int gc2053_cmos_updata_d_gain(const unsigned int d_gain);
static int gc2053_cmos_updata_a_gain(const unsigned int a_gain);
static int gc2053_cmos_updata_exp_time(unsigned int exp_time);
u16 gc2053_setgain(u16 iGain);
static int gc2053_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;

static int gc2053_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int gc2053_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int gc2053_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
	//	int value;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
		//int value=gc2053_sensor_read_register(preg_info->reg_addr);
		//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		gc2053_sensor_write_register(preg_info->reg_addr, preg_info->value);
		//value=gc2053_sensor_read_register(preg_info->reg_addr);
		//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}
	//gc2053_sensor_write_register(0x24,0xff);
	//gc2053_sensor_write_register(0x8c,0x00);
	//gc2053_sensor_write_register(0xfc,0xc4);
	//gc2053_sensor_write_register(0x23,0x32);

	g_fps = MAX_FPS;

	return 0;
}

static int gc2053_read_id(void)
{
	return SENSOR_ID;
}

static int gc2053_probe_id(void)
{
	u8 value;
	u32 id;

	value = gc2053_sensor_read_register(0xf0);
	id = value << 8;
	value = gc2053_sensor_read_register(0xf1);
	id |= value;

	printk(KERN_ERR "%s id:0x%x\n", __func__, id);
	
	if (id == SENSOR_ID)
		return SENSOR_ID;

	return 0;
}

static int gc2053_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int gc2053_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int gc2053_get_fps(void)
{
	return g_fps;
}

static int gc2053_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type gc2053_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int gc2053_get_parameter(int param, void *value)
{
	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {
		case GET_MIPI_MHZ:
			*((int *)value) = MIPI_MBPS;
			break;

		case GET_VSYNC_ACTIVE_MS:
			*((int *)value) = VSYNC_ACTIVE_MS;
			break;

		case GET_CUR_FPS:
			*((int *)value) = g_fps;
			break;

		case GET_MIPI_LANE:
			*((int *)value) = MIPI_LANE;
			break;

		case GET_INTERFACE:
			*((int *)value) = SENSOR_IO_INTERFACE;
			break;

		case GET_SENSOR_IO_LEVEL:
			*((int *)value) = SENSOR_IO_LEVEL;
			break;

		default:
			ret = -1;
			break;
	}

	return ret;
}

static int gc2053_set_power_on(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_as_gpio(pwdn_pin);
	ak_sensor_set_pin_dir(pwdn_pin, 1);
	ak_sensor_set_pin_level(pwdn_pin, !SENSOR_PWDN_LEVEL);    
	ak_sensor_mdelay(10);
	ak_sensor_set_pin_as_gpio(reset_pin);
	ak_sensor_set_pin_dir(reset_pin, 1);
	ak_sensor_set_pin_level(reset_pin, SENSOR_RESET_LEVEL);
	ak_sensor_mdelay(10);
	ak_sensor_set_pin_level(reset_pin, !SENSOR_RESET_LEVEL);
	ak_sensor_mdelay(20);

	ak_sensor_print("%s\n",__func__);
	return 0;
}

static int gc2053_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);

	return 0;
}

static int gc2053_set_fps(const int fps)
{
//#if 0
	int ret = 0;
	int tmp = 0;
	int sensor_interface = SENSOR_IO_INTERFACE;

	if (sensor_interface == DVP_INTERFACE) {
		/*DVP  tmp = (96M / 3200 / fps) - 1088 -16 */
		switch (fps) {
			case 30:
				tmp = 0x0010;
				break;

			case 25:
				tmp = 0x0060;
				break;

			case 20:
				tmp = 0x018c;
				break;
				
			case 15:
				tmp = 0x0380;
				break;

			case 12:
				tmp = 0x0510;
				break;

			case 10:
				tmp = 0x0768;
				break;

			case 8:
				tmp = 0x09c0;
				break;

			case 5:
				tmp = 0x1320;
				break;

			default:
				printk(KERN_ERR "%s set fps fail\n", __func__);
				ret = -EINVAL;
				break;
		}
	}else {
   		 /*MIPI  tmp = (96M / 3200 / fps) - 1088 -16 */
		switch (fps) {
			case 30:
				tmp = 0x0470;
				break;
			
			case 25:
				tmp = 0x0554;
				break;
			
			case 20:
				tmp = 0x0697;
				break;
				
			case 15:
				tmp = 0x08ca;
				break;
			
			case 12:
				tmp = 0x0afc;
				break;
			
			case 10:
				tmp = 0x0d20;
				break;
			
			case 8:
				tmp = 0x107b;
				break;
			
			case 5:
				tmp = 0x1a5e;
				break;
			
			default:
				printk(KERN_ERR "%s set fps fail\n", __func__);
				ret = -EINVAL;
				break;
		}

	}



	if (!ret) {
#if 0
		gc2053_sensor_write_register(0x07, tmp >> 8);
		gc2053_sensor_write_register(0x08, tmp & 0xff);
#endif
		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
//#endif
}

static int gc2053_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc2053_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB gc2053_callback = 
{
	.sensor_init_func 				= gc2053_init,
	.sensor_read_reg_func			= gc2053_sensor_read_register,
	.sensor_write_reg_func			= gc2053_sensor_write_register,
	.sensor_read_id_func			= gc2053_read_id,
	.sensor_update_a_gain_func		= gc2053_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= gc2053_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= gc2053_cmos_updata_exp_time,
	.sensor_timer_func				= gc2053_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= gc2053_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= gc2053_get_resolution,
	.sensor_get_mclk_func			= gc2053_get_mclk,
	.sensor_get_fps_func			= gc2053_get_fps,
	.sensor_get_valid_coordinate_func	= gc2053_get_valid_coordinate,
	.sensor_get_bus_type_func		= gc2053_get_bus_type,
	.sensor_get_parameter_func		= gc2053_get_parameter,

	.sensor_set_power_on_func		= gc2053_set_power_on,
	.sensor_set_power_off_func		= gc2053_set_power_off,
	.sensor_set_fps_func			= gc2053_set_fps,
	.sensor_set_standby_in_func		= gc2053_set_standby_in,
	.sensor_set_standby_out_func	= gc2053_set_standby_out
};

AK_SENSOR_MODULE(gc2053_callback, gc2053)
	
u8 regValTable[29][4] = {  
	                            {0x00, 0x00,0x01,0x00},
                                {0x00, 0x10,0x01,0x0c},
                                {0x00, 0x20,0x01,0x1b},
                                {0x00, 0x30,0x01,0x2c},
                                {0x00, 0x40,0x01,0x3f},
                                {0x00, 0x50,0x02,0x16},
                                {0x00, 0x60,0x02,0x35},
                                {0x00, 0x70,0x03,0x16},
                                {0x00, 0x80,0x04,0x02},
                                {0x00, 0x90,0x04,0x31},
                                {0x00, 0xa0,0x05,0x32},
                                {0x00, 0xb0,0x06,0x35},
                                {0x00, 0xc0,0x08,0x04},
                                {0x00, 0x5a,0x09,0x19},
                                {0x00, 0x83,0x0b,0x0f},
                                {0x00, 0x93,0x0d,0x12},
                                {0x00, 0x84,0x10,0x00},
                                {0x00, 0x94,0x12,0x3a},
                                {0x01, 0x2c,0x1a,0x02},
                                {0x01, 0x3c,0x1b,0x20},
                                {0x00, 0x8c,0x20,0x0f},
                                {0x00, 0x9c,0x26,0x07},
                                {0x02, 0x64,0x36,0x21},
                                {0x02, 0x74,0x37,0x3a},
                                {0x00, 0xc6,0x3d,0x02},
                                {0x00, 0xdc,0x3f,0x3f},
                                {0x02, 0x85,0x3f,0x3f},
                                {0x02, 0x95,0x3f,0x3f},
                                {0x00, 0xce,0x3f,0x3f},
						   };
	
unsigned int gainLevelTable[34] = {
                               64 ,
                               74 ,
                               89 ,
                               102,
                               127,
                               147,
                               177,
                               203,
                               260,
                               300,
                               361,
                               415,
                               504,
                               581,
                               722,
                               832,
                               1027,
                               1182,
                               1408,
                               1621,
                               1990,
                               2291,
                               2850,
                               3282,
                               4048,
                               5180,
                               5500, 
                               6744, 
                               7073, 
							   0xffffffff
							 };

static void gc2053_set_fps_async(void)
{
	if (to_fps != g_fps) {
		gc2053_sensor_write_register(0x41, to_fps_value >> 8);
		gc2053_sensor_write_register(0x42, to_fps_value  & 0xff);

		g_fps = to_fps;
	}
}

u16 gc2053_setgain(u16 iGain)
{
	u16 i;
	u16 total;
	unsigned int tol_dig_gain = 0; //d_gain = 0;
	u16 iReg;

	iReg = iGain;

	total = sizeof(gainLevelTable) / sizeof(unsigned int);
	for(i = 0; i < total; i++)
	{
	   if((gainLevelTable[i] <= iReg)&&(iReg < gainLevelTable[i+1]))
		break;
	}
	gc2053_sensor_write_register(0xfe,0x00);
	gc2053_sensor_write_register(0xb4,regValTable[i][0]);
	gc2053_sensor_write_register(0xb3,regValTable[i][1]);
	gc2053_sensor_write_register(0xb8,regValTable[i][2]);
	gc2053_sensor_write_register(0xb9,regValTable[i][3]);
	
	tol_dig_gain = iReg*64/gainLevelTable[i];
	gc2053_sensor_write_register(0xb1,(tol_dig_gain>>6));
	gc2053_sensor_write_register(0xb2,((tol_dig_gain&0x3f)<<2));
	return 0;
}

static int gc2053_cmos_updata_d_gain(const unsigned int d_gain)
{
	//?????????????Ļص?????
	return 0;
}

static int gc2053_cmos_updata_a_gain(const unsigned int a_gain)
{

	//??????ģ???????Ļص?????
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;

	tmp_a_gain = a_gain>>2;
	tmp_d_gain = 0;

	gc2053_set_fps_async();

	gc2053_setgain(tmp_a_gain);
	return 0;
}


static int gc2053_cmos_updata_exp_time(unsigned int exp_time)
{
 
	gc2053_set_fps_async();
	
	if(exp_time < 1) exp_time = 1;
	if(exp_time > 8191) exp_time = 8191;//2^13
	//Update Shutter
	gc2053_sensor_write_register(0x04, (exp_time) & 0xff);
	gc2053_sensor_write_register(0x03, (exp_time >> 8) & 0x1f);
    
    return EXP_EFFECT_FRAMES;
}

static int gc2053_cmos_update_a_gain_timer(void)
{

#define UPDATE_FPS_TIMER_PERIOD  (2) //s
	static int start = 0;
	static struct timeval stv = {.tv_sec = 0,};
	struct timeval tv;

	if ((to_fps != g_fps) && !start) {
		start = 1;
		do_gettimeofday(&stv);
	}

	if (start == 1) {
		do_gettimeofday(&tv);
		if (tv.tv_sec >= stv.tv_sec + UPDATE_FPS_TIMER_PERIOD) {
			gc2053_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
