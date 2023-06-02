/**
 * @file camera_sc2235.c
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
#define SENSOR_ID               0x2235
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     DVP_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			36

static int sc2235_cmos_updata_d_gain(const unsigned int d_gain);
static int sc2235_cmos_updata_a_gain(const unsigned int a_gain);
static int sc2235_cmos_updata_exp_time(unsigned int exp_time);
static int sc2235_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;
static int g_a_gain = 0;

static int sc2235_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc2235_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc2235_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=sc2235_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		sc2235_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=sc2235_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	g_fps = MAX_FPS;

	return 0;
}

static int sc2235_read_id(void)
{
    return SENSOR_ID;
}

static int sc2235_probe_id(void)
{
    u8 value;
    u32 id;

    value = sc2235_sensor_read_register(0x3107);
    id = value << 8;

    value = sc2235_sensor_read_register(0x3108);
    id |= value;

	printk(KERN_ERR "%s id:0x%x\n", __func__, id);

    //return SENSOR_ID;
	if (id != SENSOR_ID)
		goto fail;

    return SENSOR_ID;

fail:
	return 0;
}

static int sc2235_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sc2235_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc2235_get_fps(void)
{
	return g_fps;
}

static int sc2235_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sc2235_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

#if 0
static int sc2235_get_aec_delay_ms(int fps)
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

static int sc2235_get_parameter(int param, void *value)
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

static int sc2235_set_power_on(const int pwdn_pin, const int reset_pin)
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
	
	return 0;
}

static int sc2235_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int sc2235_set_fps(int fps)
{
	int ret = 0;
	int tmp=0;

	/* tmp = 72M / (reg0x320c~d=2400) / fps */
	switch (fps) {
		case 25:
			tmp = 0x04b0;
			break;
		case 20:
			tmp = 0x05dc;
			break;
		case 15:
			tmp = 0x07d0;
			break;
		case 12:
			tmp = 0x0960;
			break;
		case 10:
			tmp = 0x0bb8;
			break;
		case 8:
			tmp = 0x0ea6;
			break;
		case 5:
			tmp = 0x1770;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
#if 0
		sc2235_sensor_write_register(0x320e, tmp>>8);
		sc2235_sensor_write_register(0x320f, tmp & 0xff);
#endif
#if 0
		sc2235_sensor_write_register(0x3336, (tmp - 0x2e8) >> 8);
		sc2235_sensor_write_register(0x3337, (tmp - 0x2e8) & 0xff);
		sc2235_sensor_write_register(0x3338, tmp>>8);
		sc2235_sensor_write_register(0x3339, tmp & 0xff);
#endif

		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int sc2235_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc2235_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB sc2235_callback = 
{
	.sensor_init_func 				= sc2235_init,
	.sensor_read_reg_func			= sc2235_sensor_read_register,
	.sensor_write_reg_func			= sc2235_sensor_write_register,
	.sensor_read_id_func			= sc2235_read_id,
	.sensor_update_a_gain_func		= sc2235_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc2235_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= sc2235_cmos_updata_exp_time,
	.sensor_timer_func				= sc2235_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= sc2235_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sc2235_get_resolution,
	.sensor_get_mclk_func			= sc2235_get_mclk,
	.sensor_get_fps_func			= sc2235_get_fps,
	.sensor_get_valid_coordinate_func	= sc2235_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc2235_get_bus_type,
	.sensor_get_parameter_func		= sc2235_get_parameter,

	.sensor_set_power_on_func		= sc2235_set_power_on,
	.sensor_set_power_off_func		= sc2235_set_power_off,
	.sensor_set_fps_func			= sc2235_set_fps,
	.sensor_set_standby_in_func		= sc2235_set_standby_in,
	.sensor_set_standby_out_func	= sc2235_set_standby_out
};

AK_SENSOR_MODULE(sc2235_callback, sc2235)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
static int sc2235_curr_again_level = 0;
static int sc2235_curr_again_10x = -1;
static int sc2235_r0x3e01_value = 0;
static int is_corse7x_againmax = 0;

/*
 gain_level:
 0: <2x
 1: <8x
 2: <15.5x
 3: >=15.5x
 */
static void sc2235_ext(int gain_level)
{
	sc2235_sensor_write_register(0x3903, 0x84);
	sc2235_sensor_write_register(0x3903, 0x04);
	sc2235_sensor_write_register(0x3812, 0x00);
	switch (gain_level) {
#if 0
		case 0:
			sc2235_sensor_write_register(0x3631, 0x84);
			sc2235_sensor_write_register(0x3301, 0x05);
			sc2235_sensor_write_register(0x3633, 0x2f);
			break;
		case 1:
			sc2235_sensor_write_register(0x3631, 0x88);
			sc2235_sensor_write_register(0x3301, 0x1f);
			sc2235_sensor_write_register(0x3633, 0x23);
			break;
		case 2:
			sc2235_sensor_write_register(0x3631, 0x88);
			sc2235_sensor_write_register(0x3301, 0xff);
			sc2235_sensor_write_register(0x3633, 0x43);
			break;
#else
		case 0:
			sc2235_sensor_write_register(0x3301, 0x05);
			sc2235_sensor_write_register(0x3631, 0x84);
			sc2235_sensor_write_register(0x3622, 0xc6);
			break;
		case 1:
			sc2235_sensor_write_register(0x3301, 0x13);
			sc2235_sensor_write_register(0x3631, 0x88);
			sc2235_sensor_write_register(0x3622, 0xc6);
			break;
		case 2:
			sc2235_sensor_write_register(0x3301, 0x15);
			sc2235_sensor_write_register(0x3631, 0x88);
			sc2235_sensor_write_register(0x3622, 0xc6);
			break;
		case 3:
			sc2235_sensor_write_register(0x3301, 0xff);
			sc2235_sensor_write_register(0x3631, 0x88);
			sc2235_sensor_write_register(0x3622, 0x06);
			break;
#endif
		default:
			printk("%s gain_level error\n", __func__);
			break;
	}

	if (sc2235_r0x3e01_value < 0x05)
		sc2235_sensor_write_register(0x3314, 0x12);
	else if (sc2235_r0x3e01_value > 0x0a)
		sc2235_sensor_write_register(0x3314, 0x02);

	if (is_corse7x_againmax)
		sc2235_sensor_write_register(0x366f, 0x3a);
	else
		sc2235_sensor_write_register(0x366f, 0x2f);

	sc2235_sensor_write_register(0x3812, 0x30);
}


/*
 gain_level:
 0: <10x
 1: >=10x
 */
static void sc2235_dpc(int gain_10x)
{
	switch (gain_10x) {
		case 0:
			sc2235_sensor_write_register(0x5781, 0x04);
			sc2235_sensor_write_register(0x5785, 0x18);
			break;
		case 1:
			sc2235_sensor_write_register(0x5781, 0x02);
			sc2235_sensor_write_register(0x5785, 0x08);
			break;
		default:
			ak_sensor_print("%s err gain_10x:%d\n", __func__, gain_10x);
			break;
	}
}

static int sc2235_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static void sc2235_set_fps_async(void)
{
	if (to_fps != g_fps) {
		sc2235_sensor_write_register(0x320e, to_fps_value>>8);
		sc2235_sensor_write_register(0x320f, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int curr_2x_dgain = 0, curr_corse_gain = -1;
static unsigned int _cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
	//Ä£??????????×ª???????????ã·??????????×ª????sensor?????????
#define _2XGAIN	2048
	unsigned int  fine_gain= 0;
	unsigned int corse_gain=0,sensor_2x_again=0,sensor_2x_dgain=0;
	int tmp = 0;
	int is_10x = 0;
	//int ret = 0;

	//ak_sensor_print("a_gain =%d\n",a_gain);
	
	sc2235_set_fps_async();

	tmp = a_gain;
	if (tmp < (10 << 10))
		is_10x = 0;
	else
		is_10x = 1;

#if 0
	if (((g_a_gain < _2XGAIN) && (a_gain >= _2XGAIN)) ||
			((g_a_gain >= _2XGAIN) && (a_gain < _2XGAIN)))
	{
		ret = SKIP_NUM;
	}
#endif

	g_a_gain = a_gain;

	if(a_gain<_2XGAIN)                           //1 - 1.9375
	{
		corse_gain = 0;
		tmp = a_gain-1024;
		sc2235_curr_again_level = 0;
    }
    else if(a_gain<4096)       // 2 - 3.875
    {
    	corse_gain = 1;
		tmp = a_gain/2-1024;
		sc2235_curr_again_level = 1;
    }
    else if(a_gain<8192)       //4-7.75
    {
    	corse_gain = 3;
		tmp = a_gain/4-1024;
		sc2235_curr_again_level = 1;
    }
    else if(a_gain<15872)       //8 - 15.5
    {
        corse_gain = 7;
		tmp = a_gain/8-1024;
		sc2235_curr_again_level = 2;
    }
    else if(a_gain<32768)    // 15.5 - 31
    {
    	corse_gain = 7;
		sensor_2x_again =1;
		sensor_2x_dgain = 1;
		a_gain=a_gain>>1;
		tmp = a_gain/8-1024;
		sc2235_curr_again_level = 3;
    }
    else if(a_gain<65536)    // 32 - 62
    {
    	corse_gain = 7;
		sensor_2x_again =1;
		sensor_2x_dgain = 3;
		a_gain=a_gain>>2;
		tmp = a_gain/8-1024;
		sc2235_curr_again_level = 3;
    }
    else if(a_gain<131072)    // 64 -124
    {
    	corse_gain = 7;
		sensor_2x_again =1;
		sensor_2x_dgain = 7;
		a_gain=a_gain>>3;
		tmp = a_gain/8-1024;
		sc2235_curr_again_level = 3;
    }
    else    // 128 - 248
    {
    	corse_gain = 7;
		sensor_2x_again =1;
		sensor_2x_dgain = 0xf;
		a_gain=a_gain>>4;
		tmp = a_gain/8-1024;
		sc2235_curr_again_level = 3;
    }

	sc2235_ext(sc2235_curr_again_level);
    
    if(tmp>=0){
		fine_gain = tmp/65;
	}
	else{			
		fine_gain = 0;
	}

	fine_gain |= 0x10;

	//printk("again:%d, fine_gain:%d\n",a_gain>>2, fine_gain);
	sc2235_sensor_write_register(0x3e09, fine_gain);

	if (curr_2x_dgain != sensor_2x_dgain || 
			curr_corse_gain != corse_gain) {
		tmp = sc2235_sensor_read_register(0x3e08) & 0x3;
		tmp |= ((sensor_2x_dgain & 0x7) << 5) | (corse_gain << 2);
		sc2235_sensor_write_register(0x3e08, tmp);
		//printk("2xgain:%d, corse_gain:%d\n",sensor_2x_dgain, corse_gain);
	}

	if (curr_2x_dgain != sensor_2x_dgain) {
		tmp = sc2235_sensor_read_register(0x3e07) & 0x0e;
		tmp |= (sensor_2x_dgain >> 3) & 0x01;
		sc2235_sensor_write_register(0x3e07, tmp);
		//printk("2xgain:%d\n",sensor_2x_dgain);
	}

	if (corse_gain >= 7 && fine_gain >= 0x1f)
		is_corse7x_againmax = 1;
	else
		is_corse7x_againmax = 0;

	//sc2235_ext(sc2235_curr_again_level);

	if (is_10x != sc2235_curr_again_10x) {
		sc2235_dpc(sc2235_curr_again_10x);
		sc2235_curr_again_10x = is_10x;
	}

	curr_2x_dgain = sensor_2x_dgain;
	curr_corse_gain = corse_gain;
	return 0;
	
#if 0
	*a_gain_out = a_gain;
	*d_gain_out = sensor_d_gain;
    ag_value = corse_gain<<4|(fine_gain&0xf);
	
	//ak_sensor_print("corse=%d fine=%d\n",corse_gain,fine_gain);
	sc1135_sensor_write_register(0x3e09, Again_limit(ag_value));
	//sc1135_sensor_write_register(0x3e08, sensor_d_gain);
	if(sensor_2x_again==1)
		sc1135_sensor_write_register(0x3635, 0x0c);		//2x again
	else
		sc1135_sensor_write_register(0x3635, 0x0);		
		
	if(corse_gain<1)      //2 ±¶ÔöÒæ
	{
		sc1135_sensor_write_register(0x3630, 0xb8);
		sc1135_sensor_write_register(0x3631, 0x82);
	}
	else if(sensor_2x_again<1)      //2-16±¶ÔöÒæ
	{
		sc1135_sensor_write_register(0x3630, 0x70);
		sc1135_sensor_write_register(0x3631, 0x8e);
	}
	else
	{
		sc1135_sensor_write_register(0x3630, 0x70);
		sc1135_sensor_write_register(0x3631, 0x8c);
	}
	//ak_sensor_print(" 0x3e09 = %d\n", sc1135_sensor_read_register(0x3e09));
	//ak_sensor_print(" 0x3e0f = %d\n", sc1135_sensor_read_register(0x360f));
	
	if(sc1135_curr_2x_again!=sensor_2x_again)
	{
		sc1135_curr_2x_again=sensor_2x_again;
		return SKIP_FRAME_NUM;
	}
	else 
		return 0;
#endif
}

static int sc2235_cmos_updata_a_gain(const unsigned int a_gain)
{
#if 0
	//??????ģ???????Ļص?????
	int ret = 0;
	int tmp;
	int old_10x = sc2235_curr_again_10x;

	//printk(KERN_ERR "again=%d,%lu\n",a_gain,jiffies);
	
	if (((g_a_gain < 512) && (a_gain >= 512)) ||
			((g_a_gain >= 512) && (a_gain < 512)))
		ret = 2;

	g_a_gain  = a_gain;

	tmp = a_gain >> 4;
	sc2235_sensor_write_register(0x3e08, tmp >> 8);
	sc2235_sensor_write_register(0x3e09, tmp & 0xff);
	if (tmp < (2 << 4)) {
		sc2235_curr_again_level = 0;
		sc2235_ext(0);
	} else if (tmp < (8 << 4)) {
		sc2235_curr_again_level = 1;
		sc2235_ext(1);
	} else if (tmp < 248) {	//15.5x
		sc2235_curr_again_level = 2;
		sc2235_ext(2);
	} else {
		sc2235_curr_again_level = 3;
		sc2235_ext(3);
	}

	if (tmp < (10 << 4))
		sc2235_curr_again_10x = 0;
	else
		sc2235_curr_again_10x = 1;

	if (old_10x != sc2235_curr_again_10x)
		sc2235_dpc(sc2235_curr_again_10x);

	return ret;
#else
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain<<2;
	tmp_d_gain = 0;

	return _cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
	
#endif
}

static int sc2235_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
	unsigned char exposure_time_ext;
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
	unsigned char tem;

	sc2235_set_fps_async();

	sc2235_ext(sc2235_curr_again_level);
 	//printk("exp_time=%d ",exp_time);
	//printk(KERN_ERR "exp_time=%d,%lu\n",exp_time,jiffies);
	exposure_time_ext =(exp_time>>12)&0xf;
    exposure_time_msb =(exp_time>>4)&0xff;
    exposure_time_lsb =((exp_time)&0xf)<<4;
    tem=sc2235_sensor_read_register(0x3e02)&0x0f;
    exposure_time_lsb =exposure_time_lsb|tem;
	//printk("msb = %d  ",exposure_time_msb);
	//printk("lsb = %d\n",exposure_time_lsb);
	sc2235_sensor_write_register(0x3e01,exposure_time_msb);
	sc2235_sensor_write_register(0x3e02,exposure_time_lsb);

    tem = sc2235_sensor_read_register(0x3e00) & 0xf0;
	tem |= exposure_time_ext;
    sc2235_sensor_write_register(0x3e00, tem);
	//sc2235_ext(sc2235_curr_again_level);

	sc2235_r0x3e01_value = exposure_time_msb;

    return EXP_EFFECT_FRAMES;
}

static int sc2235_cmos_update_a_gain_timer(void)
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
			sc2235_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
