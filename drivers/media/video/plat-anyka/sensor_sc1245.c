/**
 * @file camera_sc1245.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author luweichun
 * @date 2015-11-05
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"
//#include <linux/delay.h>

#define SENSOR_PWDN_LEVEL		1 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x60 
#define SENSOR_ID               0x1245
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define SKIP_NUM		1
#define EFFECT_FRAMES 	1
#define VSYNC_ACTIVE_MS			36

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

static int sc1245_cmos_updata_d_gain(const unsigned int d_gain);
static int sc1245_cmos_updata_a_gain(const unsigned int a_gain);
static int sc1245_cmos_updata_exp_time(unsigned int exp_time);
static int sc1245_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;
//static int g_a_gain = 0;
static int g_r3020 = -1;
static int sc1245_curr_again_level = 0;

static int sc1245_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc1245_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc1245_init_post(void)
{
	sc1245_sensor_write_register(0x3e00, 0x00);
	sc1245_sensor_write_register(0x3e01, 0x5d);
	sc1245_sensor_write_register(0x3e02, 0x80);
	sc1245_sensor_write_register(0x3e03, 0x0b);
	sc1245_sensor_write_register(0x3e08, 0x03);
	sc1245_sensor_write_register(0x3e09, 0x10);

	g_r3020 = sc1245_sensor_read_register(0x3020);
	printk(KERN_ERR "g_r3020:%d\n", g_r3020);

	if (g_r3020 == 0x03) {
		sc1245_sensor_write_register(0x3303, 0x18);
		sc1245_sensor_write_register(0x3309, 0x98);
		sc1245_sensor_write_register(0x3635, 0x80);
		sc1245_sensor_write_register(0x3633, 0x46);
		sc1245_sensor_write_register(0x3301, 0x03);
		sc1245_sensor_write_register(0x3622, 0xd6);
	} else if (g_r3020 == 0x02) {
		sc1245_sensor_write_register(0x3303, 0x20);
		sc1245_sensor_write_register(0x3309, 0xa0);
		sc1245_sensor_write_register(0x3635, 0x84);
		sc1245_sensor_write_register(0x3633, 0x42);
		sc1245_sensor_write_register(0x3301, 0x04);
		sc1245_sensor_write_register(0x3622, 0xd6);
	}

	sc1245_sensor_write_register(0x0100, 0x01);

	return 0;
}

static int sc1245_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//int value=sc1245_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		sc1245_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=sc1245_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	sc1245_init_post();
	g_fps = MAX_FPS;
	to_fps = MAX_FPS;

	return 0;
}

static int sc1245_read_id(void)
{
    return SENSOR_ID;
}

static int sc1245_probe_id(void)
{
    u8 value;
    u32 id;

    value = sc1245_sensor_read_register(0x3107);
    id = value << 8;

    value = sc1245_sensor_read_register(0x3108);
    id |= value;

	//printk("%s id:0x%x\n", __func__, id);
	if (id != SENSOR_ID)
		goto fail;

    value = sc1245_sensor_read_register(0x3109);
	//printk("%s reg[0x3109]:0x%x\n", __func__, value);
	if (value != 0x01)
		goto fail;
            
    return SENSOR_ID;

fail:
	return 0;
}

static int sc1245_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sc1245_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc1245_get_fps(void)
{
	return g_fps;
}

static int sc1245_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sc1245_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

#if 0
static int sc1245_get_aec_delay_ms(int fps)
{
	/*
	 * maxfps = 30, delay 5ms to do aec
	 * */

	int base_frame_interval_ms  = 33;//1000 / 30;
	int base_fps_aec_delay_ms = 5;
	int frame_interval_ms = 1000 / fps;
	int aec_delay_ms = frame_interval_ms - base_frame_interval_ms + base_fps_aec_delay_ms;
	return aec_delay_ms;
}
#endif

static int sc1245_get_parameter(int param, void *value)
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

static int sc1245_set_power_on(const int pwdn_pin, const int reset_pin)
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

	printk(KERN_ERR "%s\n",__func__);
	return 0;
}

static int sc1245_set_power_off(const int pwdn_pin, const int reset_pin)
{
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);


    return 0;
}

static int sc1245_set_fps(const int fps)
{

	int ret = 0;
	int tmp=0;

	switch (fps) {
		case 25:
			tmp = 0x0384; ///< 900 1796
			break;

		case 20:
			tmp = 0x464; ///< 1124 2244
			break;

		case 15:
			tmp = 0x5db; ///< 1499 2994
			break;

		case 13:	//actual: 12.5fps
		case 12:
			tmp = 0x0708;
			break;
		case 10:
			tmp = 0x08ca;
			break;
		case 8:
			tmp = 0x0a8c;
			break;
		case 5:
			tmp = 0x1194;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
#if 0
		sc1245_sensor_write_register(0x320e, tmp>>8);
		sc1245_sensor_write_register(0x320f, tmp & 0xff);
#endif

		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int sc1245_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1245_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB sc1245_callback = 
{
	.sensor_init_func 				= sc1245_init,
	.sensor_read_reg_func			= sc1245_sensor_read_register,
	.sensor_write_reg_func			= sc1245_sensor_write_register,
	.sensor_read_id_func			= sc1245_read_id,
	.sensor_update_a_gain_func		= sc1245_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc1245_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= sc1245_cmos_updata_exp_time,
	.sensor_timer_func				= sc1245_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= sc1245_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sc1245_get_resolution,
	.sensor_get_mclk_func			= sc1245_get_mclk,
	.sensor_get_fps_func			= sc1245_get_fps,
	.sensor_get_valid_coordinate_func	= sc1245_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc1245_get_bus_type,
	.sensor_get_parameter_func		= sc1245_get_parameter,

	.sensor_set_power_on_func		= sc1245_set_power_on,
	.sensor_set_power_off_func		= sc1245_set_power_off,
	.sensor_set_fps_func			= sc1245_set_fps,
	.sensor_set_standby_in_func		= sc1245_set_standby_in,
	.sensor_set_standby_out_func	= sc1245_set_standby_out
};

AK_SENSOR_MODULE(sc1245_callback, sc1135)




static int sc1245_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static void sc1245_ext(int gain_level)
{
	sc1245_sensor_write_register(0x3903, 0x84);
	sc1245_sensor_write_register(0x3903, 0x04);
	sc1245_sensor_write_register(0x3812, 0x00);

	if (g_r3020 == 0x03) {
		switch (gain_level) {
			case 0:
				sc1245_sensor_write_register(0x3633, 0x46);
				sc1245_sensor_write_register(0x3301, 0x03);
				sc1245_sensor_write_register(0x3622, 0xd6);
				sc1245_sensor_write_register(0x3635, 0x89);
				//printk(KERN_ERR "sc1245_ext(0)\n");
				break;
			case 1:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x09);
				sc1245_sensor_write_register(0x3622, 0xd6);
				sc1245_sensor_write_register(0x3635, 0x86);
				//printk(KERN_ERR "sc1245_ext(1)\n");
				break;
			case 2:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x0a);
				sc1245_sensor_write_register(0x3622, 0xd6);
				sc1245_sensor_write_register(0x3635, 0x84);
				//printk(KERN_ERR "sc1245_ext(2)\n");
				break;
			case 3:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x0c);
				sc1245_sensor_write_register(0x3622, 0x16);
				sc1245_sensor_write_register(0x3635, 0x82);
				//printk(KERN_ERR "sc1245_ext(3)\n");
				break;
			case 4:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x32);
				sc1245_sensor_write_register(0x3622, 0x16);
				sc1245_sensor_write_register(0x3635, 0x80);
				//printk(KERN_ERR "sc1245_ext(3)\n");
				break;
			default:
				printk("%s gain_level error\n", __func__);
				break;
		}
	} else if (g_r3020 == 0x02) {
		switch (gain_level) {
			case 0:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x04);
				sc1245_sensor_write_register(0x3622, 0xd6);
				//printk(KERN_ERR "sc1245_ext(0)\n");
				break;
			case 1:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x05);
				sc1245_sensor_write_register(0x3622, 0xd6);
				//printk(KERN_ERR "sc1245_ext(1)\n");
				break;
			case 2:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x05);
				sc1245_sensor_write_register(0x3622, 0xd6);
				//printk(KERN_ERR "sc1245_ext(2)\n");
				break;
			case 3:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x06);
				sc1245_sensor_write_register(0x3622, 0x16);
				//printk(KERN_ERR "sc1245_ext(3)\n");
				break;
			case 4:
				sc1245_sensor_write_register(0x3633, 0x42);
				sc1245_sensor_write_register(0x3301, 0x32);
				sc1245_sensor_write_register(0x3622, 0x16);
				//printk(KERN_ERR "sc1245_ext(3)\n");
				break;
			default:
				printk("%s gain_level error\n", __func__);
				break;
		}
	}

	sc1245_sensor_write_register(0x3812, 0x30);
}

static void sc1245_set_fps_async(void)
{
	if (to_fps != g_fps) {
		sc1245_sensor_write_register(0x320e, to_fps_value>>8);
		sc1245_sensor_write_register(0x320f, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int curr_2x_dgain = 0, curr_corse_gain = -1;
static unsigned int _cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
#define _2XGAIN 2048
	unsigned int  fine_gain= 0;
	unsigned int corse_gain=0,/*sensor_2x_again=0,*/sensor_2x_dgain=0;
	int tmp = 0;
	//int is_10x = 0;
	//int ret = 0;

	sc1245_set_fps_async();

	if(a_gain<_2XGAIN)                           //1 - 1.9375
	{
		corse_gain = 0;
		tmp = a_gain-1024;
		sc1245_curr_again_level = 0;
	}
	else if(a_gain<4096)       // 2 - 3.875
	{
		corse_gain = 1;
		tmp = a_gain/2-1024;
		sc1245_curr_again_level = 1;
	}
	else if(a_gain<8192)       //4-7.75
	{
		corse_gain = 3;
		tmp = a_gain/4-1024;
		sc1245_curr_again_level = 2;
	}
	else if(a_gain<16384)       //8 - 15.5
	{
		corse_gain = 7;
		tmp = a_gain/8-1024;
		sc1245_curr_again_level = 3;
	}
	else if(a_gain<32768)    // 16 - 31
	{
		corse_gain = 7;
		//sensor_2x_again =1;
		sensor_2x_dgain = 1;
		a_gain=a_gain>>1;
		tmp = a_gain/8-1024;
		sc1245_curr_again_level = 4;
	}
	else if(a_gain<65536)    // 32 - 62
	{
		corse_gain = 7;
		//sensor_2x_again =1;
		sensor_2x_dgain = 3;
		a_gain=a_gain>>2;
		tmp = a_gain/8-1024;
		sc1245_curr_again_level = 4;
	}
	else if(a_gain<131072)    // 64 -124
	{
		corse_gain = 7;
		//sensor_2x_again =1;
		sensor_2x_dgain = 7;
		a_gain=a_gain>>3;
		tmp = a_gain/8-1024;
		sc1245_curr_again_level = 4;
	}
	else    // 128 - 248
	{
		corse_gain = 7;
		//sensor_2x_again =1;
		sensor_2x_dgain = 0xf;
		a_gain=a_gain>>4;
		tmp = a_gain/8-1024;
		sc1245_curr_again_level = 4;
	}

	if(tmp>=0){
		fine_gain = tmp/65;
	}
	else{
		fine_gain = 0;
	}

	fine_gain |= 0x10;

	sc1245_sensor_write_register(0x3e09,fine_gain);

	if (curr_2x_dgain != sensor_2x_dgain ||
			curr_corse_gain != corse_gain) {
		tmp = sc1245_sensor_read_register(0x3e08) & 0x3;
		tmp |= ((sensor_2x_dgain & 0x7) << 5) | (corse_gain << 2);
		sc1245_sensor_write_register(0x3e08, tmp);
		//printk("2xgain:%d, corse_gain:%d\n",sensor_2x_dgain, corse_gain);
	}

	if (curr_2x_dgain != sensor_2x_dgain) {
		tmp = sc1245_sensor_read_register(0x3e07) & 0x0e;
		tmp |= (sensor_2x_dgain >> 3) & 0x01;
		sc1245_sensor_write_register(0x3e07, tmp);
		//printk("2xgain:%d\n",sensor_2x_dgain);
	}

	sc1245_ext(sc1245_curr_again_level);

	curr_2x_dgain = sensor_2x_dgain;
	curr_corse_gain = corse_gain;

	return 0;
}

static int sc1245_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	//msleep(10);
	tmp_a_gain = a_gain<<2;
	tmp_d_gain = 0;

	return _cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
}


static int sc1245_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
	unsigned char tem;

	sc1245_set_fps_async();

 	//printk("exp_time=%d ",exp_time);
	//msleep(10);
    exposure_time_msb =(exp_time>>4)&0xff;
    exposure_time_lsb =((exp_time)&0xf)<<4;
    tem=sc1245_sensor_read_register(0x3e02)&0x0f;
    exposure_time_lsb =exposure_time_lsb|tem;
	//printk("msb = %d  ",exposure_time_msb);
	//printk("lsb = %d\n",exposure_time_lsb);
	sc1245_sensor_write_register(0x3e01,exposure_time_msb);
	sc1245_sensor_write_register(0x3e02,exposure_time_lsb);
	
	sc1245_ext(sc1245_curr_again_level);

    return EFFECT_FRAMES;
}

static int sc1245_cmos_update_a_gain_timer(void)
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
			sc1245_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
