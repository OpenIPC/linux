/**
 * @file sensor_gc1054.c
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
#define SENSOR_I2C_ADDR         0x42 
#define SENSOR_ID               0x1054
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define EFFECT_FRAMES 			1
#define VSYNC_ACTIVE_MS			29

#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_3V3 //SENSOR_IO_LEVEL_1V8

#define SKIP_NUM 		2
#define MIPI_MBPS		312 // 180	// 192 for 2lane
#define MIPI_LANE		1                                            
#define PCLK_FREQ  (31200000)  
#define EFFECT_FRAMES 	1

static int gc1054_cmos_updata_d_gain(const unsigned int d_gain);
static int gc1054_cmos_updata_a_gain(const unsigned int a_gain);
static int gc1054_cmos_updata_exp_time(unsigned int exp_time);
static int gc1054_cmos_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;

static int gc1054_app_exp_time = 0;
static int gc1054_current_exp_time = 0;

static int gc1054_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int gc1054_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}
static int gc1054_probe_id(void);

static int gc1054_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
	//	int value;
	gc1054_probe_id();

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
		gc1054_sensor_write_register(preg_info->reg_addr, preg_info->value);
		preg_info++;
	}

	// ygh gc1054_sensor_write_register(0x24,0xff);

	g_fps = MAX_FPS;
	to_fps = MAX_FPS;

	return 0;
}

static int gc1054_read_id(void)
{
	return SENSOR_ID;
}

static int gc1054_probe_id(void)
{
	u8 value;
	u32 id;

	value = gc1054_sensor_read_register(0xf0);
	id = value << 8;
	value = gc1054_sensor_read_register(0xf1);
	id |= value;
	pr_err("ID:%x\n",id);

	//if (id == SENSOR_ID)
		//return SENSOR_ID;
	if (id == 0x1054)
		return SENSOR_ID;

	return 0;
}

static int gc1054_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int gc1054_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int gc1054_get_fps(void)
{
	return g_fps;
}

static int gc1054_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type gc1054_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int gc1054_get_parameter(int param, void *value)
{
	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {
		case GET_MIPI_MHZ:
			*((int *)value) = MIPI_MBPS;
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

static int gc1054_set_power_on(const int pwdn_pin, const int reset_pin)
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

	g_fps = MAX_FPS;
	ak_sensor_print("%s\n",__func__);
	return 0;
}

static int gc1054_set_power_off(const int pwdn_pin, const int reset_pin)
{
    return 0;
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);

	return 0;
}

static int get_vb(int fps100)
{
	int vb;

	vb = (724 + 16 + 10) * 3000 / fps100;
	vb = vb - 724 - 16;//VB

	return vb;
}

static int get_max_exp_current_fps(void)
{
	int max_exp;

	max_exp = (724 + 16 + 10) * 30 / g_fps;

	return max_exp;
}

static int gc1054_set_fps(const int fps)
{
	/*
	 * gc1054:
	 * VTS = window_height + 16 +VB
	 *
	 * window_height: {0x0d,0x0e} = 724, @30fps
	 * VB: {0x07,0x08} = 10, @30fps
	 */

	int tmp;
	int tmp_fps;

	if (fps > MAX_FPS)
		return -EINVAL;

	tmp_fps = fps * 100;

	if (fps == 12)
		tmp_fps = 1250;
	else if (fps == 14)
		tmp_fps = 1428;

	tmp = get_vb(tmp_fps);//VB

	to_fps = fps;
	to_fps_value = tmp;

	return 0;
}

static int gc1054_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1054_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB gc1054_callback = 
{
	.sensor_init_func 				= gc1054_init,
	.sensor_read_reg_func			= gc1054_sensor_read_register,
	.sensor_write_reg_func			= gc1054_sensor_write_register,
	.sensor_read_id_func			= gc1054_read_id,
	.sensor_update_a_gain_func		= gc1054_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= gc1054_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= gc1054_cmos_updata_exp_time,
	.sensor_timer_func				= gc1054_cmos_timer,

	.sensor_probe_id_func			= gc1054_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= gc1054_get_resolution,
	.sensor_get_mclk_func			= gc1054_get_mclk,
	.sensor_get_fps_func			= gc1054_get_fps,
	.sensor_get_valid_coordinate_func	= gc1054_get_valid_coordinate,
	.sensor_get_bus_type_func		= gc1054_get_bus_type,
	.sensor_get_parameter_func		= gc1054_get_parameter,

	.sensor_set_power_on_func		= gc1054_set_power_on,
	.sensor_set_power_off_func		= gc1054_set_power_off,
	.sensor_set_fps_func			= gc1054_set_fps,
	.sensor_set_standby_in_func		= gc1054_set_standby_in,
	.sensor_set_standby_out_func	= gc1054_set_standby_out
};

AK_SENSOR_MODULE(gc1054_callback, gc1054)

///////////////////////////////////////////////////////////////////////////////////////////

#define ANALOG_GAIN_1 64 // 1.00x
#define ANALOG_GAIN_2 91 // 1.42x
#define ANALOG_GAIN_3 127 // 1.99x
#define ANALOG_GAIN_4 182 // 2.85x
#define ANALOG_GAIN_5 258 // 4.03x
#define ANALOG_GAIN_6 369 // 5.77x
#define ANALOG_GAIN_7 516 // 8.06x
#define ANALOG_GAIN_8 738 // 11.53x
#define ANALOG_GAIN_9 1051 // 16.42x
#define ANALOG_GAIN_10 1485 //23.3X
#define ANALOG_GAIN_11 2084 //32.57X

u16 gc1054_setgain(u16 iGain)
{
	u16 iReg,temp;
//	if(GC1054_Lock)
//	return;
//#ifdef GC1054_DRIVER_TRACE
//	SENSORDB("GC1054_SetGain iGain = %d \n",iGain);
//#endif
	gc1054_sensor_write_register(0xfe, 0x01);
	gc1054_sensor_write_register(0xb1, 0x01);
	gc1054_sensor_write_register(0xb2, 0x00);
	iReg = iGain;
	if(iReg < 0x40)
		iReg = 0x40;
	else if((ANALOG_GAIN_1<= iReg)&&(iReg < ANALOG_GAIN_2))
	{
		//analog gain
		gc1054_sensor_write_register(0xb6, 0x00);
		temp = iReg;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 1x , GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_2<= iReg)&&(iReg < ANALOG_GAIN_3))
	{
		gc1054_sensor_write_register(0xb6, 0x01);
		temp = 64*iReg/ANALOG_GAIN_2;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 1.4x , GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_3<= iReg)&&(iReg < ANALOG_GAIN_4))
	{
		gc1054_sensor_write_register(0xb6, 0x02);
		temp = 64*iReg/ANALOG_GAIN_3;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 2.5x , GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_4<= iReg)&&(iReg < ANALOG_GAIN_5))
	{
		gc1054_sensor_write_register(0xb6, 0x03);
		temp = 64*iReg/ANALOG_GAIN_4;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 3.54x , GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_5<= iReg)&&(iReg < ANALOG_GAIN_6))
	{
		gc1054_sensor_write_register(0xb6, 0x04);
		temp = 64*iReg/ANALOG_GAIN_5;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 4.9x , GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_6<= iReg)&&(iReg < ANALOG_GAIN_7))
	{
		gc1054_sensor_write_register(0xb6, 0x05);//
		temp = 64*iReg/ANALOG_GAIN_6;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain6.9x, GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_7<= iReg)&&(iReg < ANALOG_GAIN_8))
	{
		gc1054_sensor_write_register(0xb6, 0x06);//
		temp = 64*iReg/ANALOG_GAIN_7;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 9.7x, GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_8<= iReg)&&(iReg < ANALOG_GAIN_9))
	{
		gc1054_sensor_write_register(0xb6, 0x07);
		temp = 64*iReg/ANALOG_GAIN_8;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 13.6x,GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_9<= iReg)&&(iReg < ANALOG_GAIN_10))
	{
		gc1054_sensor_write_register(0xb6, 0x08);
		temp = 64*iReg/ANALOG_GAIN_9;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 19.5x ,GC1054 add pregain = %d\n",temp);
	}
	else if((ANALOG_GAIN_10<= iReg)&&(iReg < ANALOG_GAIN_11))
	{
		gc1054_sensor_write_register(0xb6, 0x09);
		temp = 64*iReg/ANALOG_GAIN_10;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 27x ,GC1054 add pregain = %d\n",temp);
	}
	else if(ANALOG_GAIN_11<= iReg)
	{
		gc1054_sensor_write_register(0xb6, 0x0a);
		temp = 64*iReg/ANALOG_GAIN_11;
		gc1054_sensor_write_register(0xb1, temp>>6);
		gc1054_sensor_write_register(0xb2, (temp<<2)&0xfc);
		//SENSORDB("GC1054 analogic gain 39x ,GC1054 add pregain = %d\n",temp);
	}

	gc1054_sensor_write_register(0xfe, 0x00);

	return 0;
}

static void gc1054_set_fps_async(void)
{
	if (to_fps != g_fps) {
		printk("%s g_fps:%d, to_fps:%d, value:0x%x\n", __func__, g_fps, to_fps, to_fps_value);
		gc1054_sensor_write_register(0x07, to_fps_value>>8);
		gc1054_sensor_write_register(0x08, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int gc1054_cmos_updata_d_gain(const unsigned int d_gain)
{
	//?????????????Ļص?????
	return 0;
}

static int gc1054_cmos_updata_a_gain(const unsigned int a_gain)
{
	//??????ģ???????Ļص?????
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;

	gc1054_set_fps_async();

	tmp_a_gain = a_gain>>2;
	tmp_d_gain = 0;

	gc1054_setgain(tmp_a_gain);
	return 0;
}

static int gc1054_cmos_updata_exp_time(unsigned int exp_time)
{
	int max_exp_current_fps;

    //?????ع?ʱ???Ļص?????
//    unsigned char exposure_time_msb;
//	unsigned char exposure_time_lsb;

	gc1054_set_fps_async();

//	printk("exp_time=%d\n",exp_time);
//    exposure_time_msb =(exp_time>>8)&0xff;
 //   exposure_time_lsb = exp_time&0xff;
//	printk("msb = %d\n",exposure_time_msb);
//	printk("lsb = %d\n",exposure_time_lsb);
	//gc1054_sensor_write_register(0x02,exposure_time_msb);
	//gc1054_sensor_write_register(0x01,exposure_time_lsb);
//	printk("0x16=%d\n",gc1054_sensor_read_register(0x16));

	if(exp_time < 1) exp_time = 1;
	if(exp_time > 8191) exp_time = 8191;//2^13
	gc1054_app_exp_time = exp_time;
	max_exp_current_fps = get_max_exp_current_fps();
	if (exp_time > max_exp_current_fps)
		exp_time = max_exp_current_fps;
	gc1054_current_exp_time = exp_time;

	//Update Shutter
	gc1054_sensor_write_register(0x04, (exp_time) & 0xff);
	gc1054_sensor_write_register(0x03, (exp_time >> 8) & 0x1f);
    
    return EFFECT_FRAMES;
}

static int gc1054_cmos_timer(void)
{
#define UPDATE_FPS_TIMER_PERIOD  (2) //s
	static int start = 0;
	static struct timeval stv = {.tv_sec = 0,};
	struct timeval tv;

	if (((to_fps != g_fps) || (gc1054_app_exp_time != gc1054_current_exp_time)) &&
			!start) {
		start = 1;
		do_gettimeofday(&stv);
	}

	if (start == 1) {
		do_gettimeofday(&tv);
		if (tv.tv_sec >= stv.tv_sec + UPDATE_FPS_TIMER_PERIOD) {
			/*设置帧率与曝光不同步会导致真实曝光可能没有打满*/
			if ((gc1054_app_exp_time != gc1054_current_exp_time) &&
					(gc1054_app_exp_time <= get_max_exp_current_fps()))
				gc1054_cmos_updata_exp_time(gc1054_app_exp_time);

			gc1054_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
