/**
 * @file camera_sp2305.c
 * @brief camera driver file
 * Copyright (C) 2017 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author 
 * @date
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"
#include <linux/delay.h>

#define SENSOR_PWDN_LEVEL		1
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x7a ///< I2CADDR 上拉时 ADDR=0x7a，下拉时 ADDR=0x78。
#define SENSOR_ID               0x2735
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			31
#define MIPI_MBPS		360
#define MIPI_LANE		2

static int sp2305_cmos_updata_d_gain(const unsigned int d_gain);
static int sp2305_cmos_updata_a_gain(const unsigned int a_gain);
static int sp2305_cmos_updata_exp_time(unsigned int exp_time);
static int sp2305_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;


static int sp2305_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sp2305_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sp2305_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//  value = sp2305_sensor_read_register(preg_info->reg_addr);
//  printk(KERN_ERR "before w reg:0x%x, val:0x%x, to write:0x%x\n", preg_info->reg_addr, value, preg_info->value);
		if(preg_info->reg_addr ==0x20)
		{
			sp2305_sensor_write_register(preg_info->reg_addr, preg_info->value);
			ak_sensor_mdelay(5);
		}
		else
			sp2305_sensor_write_register(preg_info->reg_addr, preg_info->value);

//		if(preg_info->reg_addr ==0x01)
//			printk(KERN_ERR "after write reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, preg_info->value);
//		value=sp2305_sensor_read_register(preg_info->reg_addr);
//		printk(KERN_ERR "after read reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);


		preg_info++;
	}

	g_fps = MAX_FPS;
	
	return 0;
}

static int sp2305_read_id(void)
{
    return SENSOR_ID;
}

static int sp2305_probe_id(void)
{
    u8 value;
    u32 id;
    sp2305_sensor_write_register(0xfd, 0x00);
    value = sp2305_sensor_read_register(0x02);
	printk(KERN_ERR "ox02  %s value:0x%x\n", __func__, value);
    id = value << 8;

    value = sp2305_sensor_read_register(0x03);
	printk(KERN_ERR "ox03  %s value:0x%x\n", __func__, value);
	value = value&0xff;
    id |= value;


	printk(KERN_ERR "%s id:0x%x\n", __func__, id);

    //return SENSOR_ID;
	if (id != SENSOR_ID)
		goto fail;

    return SENSOR_ID;

fail:
	return 0;
}

static int sp2305_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sp2305_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sp2305_get_fps(void)
{
	return g_fps;
}

static int sp2305_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sp2305_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

#if 0
static int sp2305_get_aec_delay_ms(int fps)
{
	int aec_delay_ms;

	switch (fps) {
		case 25:
			aec_delay_ms = 5;
			break;
		case 20:
			aec_delay_ms = 15;
			break;
		case 15:
			aec_delay_ms = 32;
			break;
		case 12:
			aec_delay_ms = 45;
			break;
		case 10:
			aec_delay_ms = 65;
			break;
		case 8:
			aec_delay_ms = 89;
			break;
		case 5:
			aec_delay_ms = 165;
			break;
		default:
			printk(KERN_ERR "%s fps=%d error\n", __func__, fps);
			aec_delay_ms = 5;
			break;
	}
	return aec_delay_ms;
}
#endif

static int sp2305_get_parameter(int param, void *value)
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

static int sp2305_set_power_on(const int pwdn_pin, const int reset_pin)
{  
	ak_sensor_set_pin_as_gpio(pwdn_pin);
	ak_sensor_set_pin_as_gpio(reset_pin);
    ak_sensor_set_pin_dir(pwdn_pin, 1);
    ak_sensor_set_pin_dir(reset_pin, 1);

    ak_sensor_set_pin_level(reset_pin, SENSOR_RESET_LEVEL); ///< Reset
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL); ///< Power Down
    ak_sensor_mdelay(10);
    ak_sensor_set_pin_level(pwdn_pin, !SENSOR_PWDN_LEVEL); ///< 正常工作。
    ak_sensor_mdelay(5);
    ak_sensor_set_pin_level(reset_pin, !SENSOR_RESET_LEVEL); ///< 正常工作。
    ak_sensor_mdelay(20);
	
	return 0;
}

static int sp2305_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int sp2305_set_fps(int fps)
{
	//return 0;
	int ret = 0;
	int tmp=0;

	/* tmp = 78M / (reg0x320c~d=2080) / fps */
	switch (fps) {
		case 25:
			tmp = 0x0557;
			break;
		case 20:
			tmp = 0x06ad;
			break;
		case 15:
			tmp = 0x08e7;
			break;
		case 12:
			tmp = 0x0b21;
			break;
		case 10:
			tmp = 0x0d5a;
			break;
		case 8:
			tmp = 0x10b1;
			break;
		case 5:
			tmp = 0x1ab5;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
#if 0
		sp2305_sensor_write_register(0xfd,0x01);
		sp2305_sensor_write_register(0x05, tmp>>8);
		sp2305_sensor_write_register(0x06, tmp & 0xff);
		sp2305_sensor_write_register(0x01,0x01);
#endif


		//g_fps = fps;
		to_fps = fps;
		//tmp = tmp -0x0451;
		to_fps_value = tmp;
	}

	return ret;
}

static int sp2305_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sp2305_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB sp2305_callback = 
{
	.sensor_init_func 				= sp2305_init,
	.sensor_read_reg_func			= sp2305_sensor_read_register,
	.sensor_write_reg_func			= sp2305_sensor_write_register,
	.sensor_read_id_func			= sp2305_read_id,
	.sensor_update_a_gain_func		= sp2305_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sp2305_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= sp2305_cmos_updata_exp_time,
	.sensor_timer_func				= sp2305_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= sp2305_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sp2305_get_resolution,
	.sensor_get_mclk_func			= sp2305_get_mclk,
	.sensor_get_fps_func			= sp2305_get_fps,
	.sensor_get_valid_coordinate_func	= sp2305_get_valid_coordinate,
	.sensor_get_bus_type_func		= sp2305_get_bus_type,
	.sensor_get_parameter_func		= sp2305_get_parameter,

	.sensor_set_power_on_func		= sp2305_set_power_on,
	.sensor_set_power_off_func		= sp2305_set_power_off,
	.sensor_set_fps_func			= sp2305_set_fps,
	.sensor_set_standby_in_func		= sp2305_set_standby_in,
	.sensor_set_standby_out_func	= sp2305_set_standby_out
};

AK_SENSOR_MODULE(sp2305_callback, sp2305)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
//static int is_corse7x_againmax = 0;



static int sp2305_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static void sp2305_set_fps_async(void)
{
	if (to_fps != g_fps) {
		sp2305_sensor_write_register(0xfd,0x01);
		sp2305_sensor_write_register(0x0d,0x10);
		sp2305_sensor_write_register(0x0e, to_fps_value>>8);
		sp2305_sensor_write_register(0x0f, to_fps_value & 0xff);
		sp2305_sensor_write_register(0x01,0x01);

		g_fps = to_fps;
	}
}
#define _1XGAIN		(256)
#define _2XGAIN		(_1XGAIN*2)
#define _4XGAIN		(_1XGAIN*4)
#define _8XGAIN		(_1XGAIN*8)
#define _16XGAIN	(_1XGAIN*16)

static unsigned char curr_corse_gain = 0;
static unsigned int _cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{

	unsigned char fine_gain= 0;
	unsigned int a_tmp_gain = 0;
	sp2305_set_fps_async();

	d_gain = 0;

	if(a_gain<_1XGAIN)
	{
		fine_gain = 0x10;
	}else if(a_gain<_2XGAIN)
	{
		fine_gain = (a_gain >>4);
		
	}else if(a_gain<_4XGAIN) 
	{
		fine_gain =  ((a_gain>>4)&0xfe);
		
	}else if(a_gain<_8XGAIN)
	{
		fine_gain =  ((a_gain>>4)&0xfc);
		
	}else if(a_gain<_16XGAIN)
	{
		fine_gain =  ((a_gain>>4)&0xf8);
	}else
	{
		fine_gain =  0xf8;
	}
	
	printk("again:%d, fine_gain:0x%0.2x, curr_corse_gain:0x%0.2x,\n",a_gain, fine_gain,curr_corse_gain);
	if (curr_corse_gain != fine_gain)
	{
		sp2305_sensor_write_register(0xfd, 0x01);
		sp2305_sensor_write_register(0x24, fine_gain);
		sp2305_sensor_write_register(0x01, 0x01);
		curr_corse_gain = fine_gain;
	}
	return 0;
}

static int sp2305_cmos_updata_a_gain(const unsigned int a_gain)
{

	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain;
	tmp_d_gain = 0;

	return _cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
}

static int sp2305_cmos_updata_exp_time(unsigned int exp_time)
{
    //??????????????????

    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;


	sp2305_set_fps_async();

	//printk(KERN_ERR "exp_time=%d,%lu\n",exp_time,jiffies);
    exposure_time_msb =(exp_time>>8)&0xff;
    exposure_time_lsb =((exp_time)&0xff);
    
	sp2305_sensor_write_register(0xfd,0x01);
	sp2305_sensor_write_register(0x03,exposure_time_msb);
	sp2305_sensor_write_register(0x04,exposure_time_lsb);
	sp2305_sensor_write_register(0x01,0x01);


    return EXP_EFFECT_FRAMES;
}

static int sp2305_cmos_update_a_gain_timer(void)
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
			sp2305_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
