/**
 * @file camera_sc2232h.c
 * @brief camera driver file
 * Copyright (C) 2017 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author 
 * @date
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"
#include <linux/delay.h>

#define SENSOR_PWDN_LEVEL		0
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x60
#define SENSOR_ID               0xCB71
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     2
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

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			28
#define MIPI_MBPS		390
#define MIPI_LANE		2

#define SNS_DG_EN		0

static int sc2232h_cmos_updata_d_gain(const unsigned int d_gain);
static int sc2232h_cmos_updata_a_gain(const unsigned int a_gain);
static int sc2232h_cmos_updata_exp_time(unsigned int exp_time);
static int sc2232h_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;
//static int g_a_gain = 0;

// flag the sensor between sc2232h and sc2315e
static int support_sc2315e = 0;

static int sc2232h_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc2232h_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc2232h_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
	   sc2232h_sensor_write_register(preg_info->reg_addr, preg_info->value);
        preg_info++;
	}
	//close DPC for anti-false color
	sc2232h_sensor_write_register(0x5000, 0x06); //for dpc
	sc2232h_sensor_write_register(0x5780, 0xff); //for dpc
	sc2232h_sensor_write_register(0x5781, 0x60); //for dpc
	sc2232h_sensor_write_register(0x5785, 0x30); //for dpc

	g_fps = MAX_FPS;

	if (support_sc2315e) {
		/// sc2315e
		sc2232h_sensor_write_register (0x363b, 0x06);
	} else {
		/// sc2232
		sc2232h_sensor_write_register (0x363b, 0x26);
	}
	
	return 0;
}

static int sc2232h_read_id(void)
{
    return SENSOR_ID;
}

static int sc2232h_probe_id(void)
{
    u8 value;
    u32 id;

    u32 const id_sc2232h = 0xcb0701;
    u32 const id_sc2315e = 0x223820;

    /// to sc2232h
    /// [0x3107,0x3108,0x3109] = 0xcb0701
    /// to sc2315e
    /// [0x3107,0x3108,0x3109] = 0x223820

    value = sc2232h_sensor_read_register(0x3107);
    id = value << 16;    
    value = sc2232h_sensor_read_register(0x3108);
    id |= value << 8;
    value = sc2232h_sensor_read_register(0x3109);
    id |= value;
    

    //printk(KERN_ERR "%s id:0x%x\n", __func__, id);

    if (id_sc2315e == id) {
	    support_sc2315e = 1;
	    return SENSOR_ID;
    } else if (id_sc2232h == id) {
	    return SENSOR_ID;
    }

    return 0;
}

static int sc2232h_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sc2232h_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc2232h_get_fps(void)
{
	return g_fps;
}

static int sc2232h_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sc2232h_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

#if 0
static int sc2232h_get_aec_delay_ms(int fps)
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

static int sc2232h_get_parameter(int param, void *value)
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

static int sc2232h_set_power_on(const int pwdn_pin, const int reset_pin)
{  
	ak_sensor_set_pin_as_gpio(pwdn_pin);
    ak_sensor_set_pin_dir(pwdn_pin, 1);
    ak_sensor_set_pin_level(pwdn_pin, !SENSOR_PWDN_LEVEL);    
    ak_sensor_mdelay(10);
    ak_sensor_set_pin_as_gpio(reset_pin);
    ak_sensor_set_pin_dir(reset_pin, 1);
    ak_sensor_set_pin_level(reset_pin, SENSOR_RESET_LEVEL);
    ak_sensor_mdelay(30);
    ak_sensor_set_pin_level(reset_pin, !SENSOR_RESET_LEVEL);
    ak_sensor_mdelay(20);
	
	return 0;
}

static int sc2232h_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int sc2232h_set_fps(int fps)
{
	int ret = 0;
	int tmp=0;

	/* tmp = 78M / (reg0x320c~d=2080) / fps */
	switch (fps) {
		case 30:
			tmp = 0x4e2;//0x049D;
			break;
		case 25:
			tmp = 0x5dc;//0x058A;
			break;
		case 20:
			tmp = 0x753;//0x06EC;
			break;
		case 15:
			tmp = 0x9c4;//0x093B;
			break;
		case 12:
			tmp = 0x0bb8;//0x0B8A;
			break;
		case 10:
			tmp = 0x0ea6;//0x0DD9;
			break;
		case 8:
			tmp = 0x124f;//0x114F;
			break;
		case 5:
			tmp = 0x1d4c;//0x1BB2;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
#if 0
		sc2232h_sensor_write_register(0x320e, tmp>>8);
		sc2232h_sensor_write_register(0x320f, tmp & 0xff);
#endif

		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int sc2232h_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc2232h_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB sc2232h_callback = 
{
	.sensor_init_func 				= sc2232h_init,
	.sensor_read_reg_func			= sc2232h_sensor_read_register,
	.sensor_write_reg_func			= sc2232h_sensor_write_register,
	.sensor_read_id_func			= sc2232h_read_id,
	.sensor_update_a_gain_func		= sc2232h_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc2232h_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= sc2232h_cmos_updata_exp_time,
	.sensor_timer_func				= sc2232h_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= sc2232h_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sc2232h_get_resolution,
	.sensor_get_mclk_func			= sc2232h_get_mclk,
	.sensor_get_fps_func			= sc2232h_get_fps,
	.sensor_get_valid_coordinate_func	= sc2232h_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc2232h_get_bus_type,
	.sensor_get_parameter_func		= sc2232h_get_parameter,

	.sensor_set_power_on_func		= sc2232h_set_power_on,
	.sensor_set_power_off_func		= sc2232h_set_power_off,
	.sensor_set_fps_func			= sc2232h_set_fps,
	.sensor_set_standby_in_func		= sc2232h_set_standby_in,
	.sensor_set_standby_out_func	= sc2232h_set_standby_out
};

AK_SENSOR_MODULE(sc2232h_callback, sc2232h)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
static int sc2232h_curr_again_level = 0;
static int sc2232h_curr_again_15x5 = -1;
static int sc2232h_r0x3e02_value = 0;
static int sc2232h_r0x3e01_value = 0;
static int sc2232h_r0x3e00_value =0;
//static int is_corse7x_againmax = 0;

/*
 gain_level:
 0: <2x
 1: <4x
 2: <8x
 3: <15.5x
 4: >=15.5x
 */
static void sc2232h_ext(int gain_level)
{
	sc2232h_sensor_write_register(0x3812, 0x00);
	switch (gain_level) {
		case 0:
			sc2232h_sensor_write_register(0x3301, 0x12);
			sc2232h_sensor_write_register(0x3632, 0x08); 
			break;
		case 1:
			sc2232h_sensor_write_register(0x3301, 0x20);
			sc2232h_sensor_write_register(0x3632, 0x08);
			break;
		case 2:
			sc2232h_sensor_write_register(0x3301, 0x28);
			sc2232h_sensor_write_register(0x3632, 0x08);
			break;
		case 3:
			sc2232h_sensor_write_register(0x3301, 0x64);
			sc2232h_sensor_write_register(0x3632, 0x08);
			break;
		case 4:
			sc2232h_sensor_write_register(0x3301, 0x64);
			sc2232h_sensor_write_register(0x3632, 0x48);
			break;
		default:
			sc2232h_sensor_write_register(0x3301, 0x64);
			sc2232h_sensor_write_register(0x3632, 0x48);
			printk("%s gain_level error\n", __func__);
			break;
	}

    if (sc2232h_r0x3e00_value == 0)
    {
	    if (sc2232h_r0x3e01_value < 0x25)
		    sc2232h_sensor_write_register(0x3314, 0x14);
	    else if (sc2232h_r0x3e01_value > 0x45)
		    sc2232h_sensor_write_register(0x3314, 0x04);
    }

	sc2232h_sensor_write_register(0x3812, 0x30);
}


/*
 gain_level:
 0: <15x5
 1: >=15x5
 */
static void sc2232h_dpc(int gain_15x5)
{
	switch (gain_15x5) {
		case 0:
			sc2232h_sensor_write_register(0x5781, 0x60);
			sc2232h_sensor_write_register(0x5785, 0x30);
			break;
		case 1:
			sc2232h_sensor_write_register(0x5781, 0x01);
			sc2232h_sensor_write_register(0x5785, 0x02);
			break;
		default:
			ak_sensor_print("%s err gain_15x5:%d\n", __func__, gain_15x5);
			break;
	}
}

static int sc2232h_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static void sc2232h_set_fps_async(void)
{
	if (to_fps != g_fps) {
		sc2232h_sensor_write_register(0x320e, to_fps_value>>8);
		sc2232h_sensor_write_register(0x320f, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int curr_corse_Dgain = 0, curr_corse_Again = -1;
static unsigned int _cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
#define _2XGAIN	2048
	unsigned int  fine_gain= 0;
	unsigned int corse_Again=0,fine_Again=0x10,corse_Dgain=0,fine_Dgain=0x80;

	int tmp = 0;
	int is_15x5 = 0;
	//int ret = 0;

	//ak_sensor_print("a_gain =%d\n",a_gain);
	
	sc2232h_set_fps_async();

    tmp = a_gain;
	if (tmp < 15872)
		is_15x5 = 0;
	else
		is_15x5 = 1;

	//g_a_gain = a_gain;

	if(a_gain<_2XGAIN)          //1- 1.9375
	{
		corse_Again = 0;
		corse_Dgain = 0x0;
		fine_Dgain = 0x80;
		tmp = a_gain-1024;
		sc2232h_curr_again_level = 0;
    }
    else if(a_gain<4096)       // 2 - 3.875
    {
    	corse_Again = 1;
		corse_Dgain = 0x0;
		fine_Dgain = 0x80;
		tmp = a_gain/2-1024;
		sc2232h_curr_again_level = 1;
    }
    else if(a_gain<8192)       //4-7.75
    {
    	corse_Again = 3;
		corse_Dgain = 0x0;
		fine_Dgain = 0x80;
		tmp = a_gain/4-1024;
		sc2232h_curr_again_level = 2;
    }
    else if(a_gain<15872)       //8 - 15.5
    {
        corse_Again = 7;
		corse_Dgain = 0x0;
		fine_Dgain = 0x80;
		tmp = a_gain/8-1024;
		sc2232h_curr_again_level = 3;
    }  
#if SNS_DG_EN	
    else if(a_gain<31744)    // 15.5 - 31
    {
    	corse_Again = 7;
		fine_Again = 0x1f;
		corse_Dgain = 0x0;
		tmp = a_gain*10/155-1024;
		sc2232h_curr_again_level = 4;
    }
    else if(a_gain<63488)    // 31 - 62
    {
    	corse_Again = 7;
		fine_Again = 0x1f;
		corse_Dgain = 0x1;
		tmp = a_gain/31-1024;
		sc2232h_curr_again_level = 4;
    }
    else if(a_gain<126976)    // 62 -124
    {
    	corse_Again = 7;
		fine_Again = 0x1f;
		corse_Dgain = 0x3;
		tmp = a_gain/62-1024;
		sc2232h_curr_again_level = 4;
    }
    else    // 124 - 248
    {
    	corse_Again = 7;
		fine_Again = 0x1f;
		corse_Dgain = 0x7;
		tmp = a_gain/124-1024;
		sc2232h_curr_again_level = 4;
    }
#else
	else						//15.5 --
    {
        corse_Again = 7;
		corse_Dgain = 0x0;
		fine_Dgain = 0x80;
		tmp = 960;
		sc2232h_curr_again_level = 4;
    }
#endif

	sc2232h_ext(sc2232h_curr_again_level);
    
    if(tmp>=0){
		fine_gain = tmp/64;
	}
	else{			
		fine_gain = 0;
	}
	
#if SNS_DG_EN	
	if(a_gain<15872)
	{
		fine_Again = 0x10 + fine_gain;
		sc2232h_sensor_write_register(0x3e09, fine_Again);
		
		if (curr_corse_Again != corse_Again) 
		{
			tmp = sc2232h_sensor_read_register(0x3e08) & 0xe3;
			tmp |= corse_Again<<2 ;
			sc2232h_sensor_write_register(0x3e08, tmp);
		}
		sc2232h_sensor_write_register(0x3e06, corse_Dgain);
		sc2232h_sensor_write_register(0x3e07, fine_Dgain);
	}
	else
	{
		fine_Dgain = 0x80 +0x8*fine_gain;
		sc2232h_sensor_write_register(0x3e07, fine_Dgain);
		if (curr_corse_Dgain != corse_Dgain) 
		{
			tmp = sc2232h_sensor_read_register(0x3e06);
			tmp |= corse_Dgain;
			sc2232h_sensor_write_register(0x3e06, tmp);
		}
		sc2232h_sensor_write_register(0x3e08, corse_Again);
		sc2232h_sensor_write_register(0x3e09, fine_Again);
	}
#else		
	fine_Again = 0x10 + fine_gain;
	sc2232h_sensor_write_register(0x3e09, fine_Again);
	
	if (curr_corse_Again != corse_Again) 
	{
		tmp = sc2232h_sensor_read_register(0x3e08) & 0xe3;
		tmp |= corse_Again<<2 ;
		sc2232h_sensor_write_register(0x3e08, tmp);
	}
	sc2232h_sensor_write_register(0x3e06, corse_Dgain);
	sc2232h_sensor_write_register(0x3e07, fine_Dgain);
#endif

	if (is_15x5 != sc2232h_curr_again_15x5) {
		//sc2232h_dpc(is_15x5);
		sc2232h_curr_again_15x5 = is_15x5;
	}

	curr_corse_Dgain = corse_Dgain;
	curr_corse_Again = corse_Again;
	return 0;
}

static int sc2232h_cmos_updata_a_gain(const unsigned int a_gain)
{

	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain<<2;
	tmp_d_gain = 0;

	return _cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
}

static int sc2232h_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
	unsigned char exposure_time_ext;
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
	unsigned char tem;

	sc2232h_set_fps_async();

	sc2232h_ext(sc2232h_curr_again_level);
 	printk("exp_time=%d ",exp_time);
	printk(KERN_ERR "exp_time=%d,%lu\n",exp_time,jiffies);
	exposure_time_ext =(exp_time>>12)&0xf;
    exposure_time_msb =(exp_time>>4)&0xff;
    exposure_time_lsb =((exp_time)&0xf)<<4;
    tem=sc2232h_sensor_read_register(0x3e02)&0x0f;
    exposure_time_lsb =exposure_time_lsb|tem;
	//printk("msb = %d  ",exposure_time_msb);
	//printk("lsb = %d\n",exposure_time_lsb);
	sc2232h_sensor_write_register(0x3e01,exposure_time_msb);
	sc2232h_sensor_write_register(0x3e02,exposure_time_lsb);

    tem = sc2232h_sensor_read_register(0x3e00) & 0xf0;
	tem |= exposure_time_ext;
    sc2232h_sensor_write_register(0x3e00, tem);
	//sc2232h_ext(sc2232h_curr_again_level);

	sc2232h_r0x3e02_value = exposure_time_lsb;
	sc2232h_r0x3e01_value = exposure_time_msb;
	sc2232h_r0x3e00_value = exposure_time_ext;

    return EXP_EFFECT_FRAMES;
}

static int sc2232h_cmos_update_a_gain_timer(void)
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
			sc2232h_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
