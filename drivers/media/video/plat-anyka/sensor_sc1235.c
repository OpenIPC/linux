/**
 * @file camera_sc1235.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author luweichun
 * @date 2015-11-05
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		0 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x60 
#define SENSOR_ID               0x1235
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	960
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 2
#define EFFECT_FRAMES 	1
#define VSYNC_ACTIVE_MS			36

static int sc1235_cmos_updata_d_gain(const unsigned int d_gain);
static int sc1235_cmos_updata_a_gain(const unsigned int a_gain);
static int sc1235_cmos_updata_exp_time(unsigned int exp_time);
static int sc1235_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;
static int g_a_gain = 0;

static int sc1235_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc1235_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc1235_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=sc1235_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		sc1235_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=sc1235_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	g_fps = MAX_FPS;
	to_fps = MAX_FPS;
	return 0;
}

static int sc1235_read_id(void)
{
    return SENSOR_ID;
}

static int sc1235_probe_id(void)
{
    u8 value;
    u32 id;

    value = sc1235_sensor_read_register(0x3107);
    id = value << 8;

    value = sc1235_sensor_read_register(0x3108);
    id |= value;

	printk(KERN_ERR "%s id:0x%x\n", __func__, id);

    //return SENSOR_ID;
	if (id != SENSOR_ID)
		goto fail;

    return SENSOR_ID;

fail:
	return 0;
}

static int sc1235_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sc1235_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc1235_get_fps(void)
{
	return g_fps;
}

static int sc1235_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sc1235_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int sc1235_get_parameter(int param, void *value)
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

static int sc1235_set_power_on(const int pwdn_pin, const int reset_pin)
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

static int sc1235_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int sc1235_set_fps(int fps)
{
	int ret = 0;
	int tmp=0;

	/* tmp = 54M / (reg0x320c~d=2160) / fps */
	switch (fps) {
		case 25:
			tmp = 0x03e8;
			break;
		case 13:    //actual: 12.5fps
		case 12:
			tmp = 0x07e0;
			break;
		case 10:
			tmp = 0x09c4;
			break;
		case 8:
			tmp = 0x0c35;
			break;
		case 5:
			tmp = 0x1388;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
#if 0
		sc1235_sensor_write_register(0x320e, tmp>>8);
		sc1235_sensor_write_register(0x320f, tmp & 0xff);
#endif
#if 0
		sc1235_sensor_write_register(0x3336, (tmp - 0x2e8) >> 8);
		sc1235_sensor_write_register(0x3337, (tmp - 0x2e8) & 0xff);
		sc1235_sensor_write_register(0x3338, tmp>>8);
		sc1235_sensor_write_register(0x3339, tmp & 0xff);
#endif

		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int sc1235_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1235_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB sc1235_callback = 
{
	.sensor_init_func 				= sc1235_init,
	.sensor_read_reg_func			= sc1235_sensor_read_register,
	.sensor_write_reg_func			= sc1235_sensor_write_register,
	.sensor_read_id_func			= sc1235_read_id,
	.sensor_update_a_gain_func		= sc1235_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc1235_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= sc1235_cmos_updata_exp_time,
	.sensor_timer_func				= sc1235_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= sc1235_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sc1235_get_resolution,
	.sensor_get_mclk_func			= sc1235_get_mclk,
	.sensor_get_fps_func			= sc1235_get_fps,
	.sensor_get_valid_coordinate_func	= sc1235_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc1235_get_bus_type,
	.sensor_get_parameter_func		= sc1235_get_parameter,

	.sensor_set_power_on_func		= sc1235_set_power_on,
	.sensor_set_power_off_func		= sc1235_set_power_off,
	.sensor_set_fps_func			= sc1235_set_fps,
	.sensor_set_standby_in_func		= sc1235_set_standby_in,
	.sensor_set_standby_out_func	= sc1235_set_standby_out
};

AK_SENSOR_MODULE(sc1235_callback, sc1235)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
static int sc1235_curr_again_level = 0;
static int sc1235_curr_again_10x = -1;

/*
 gain_level:
 0: <2x
 1: <4x
 2: >4x
 */
static void sc1235_ext(int gain_level)
{
	sc1235_sensor_write_register(0x3903, 0x84);
	sc1235_sensor_write_register(0x3903, 0x04);
	sc1235_sensor_write_register(0x3812, 0x00);
	switch (gain_level) {
		case 0:
			sc1235_sensor_write_register(0x3631, 0x84);
			sc1235_sensor_write_register(0x3301, 0x05);
			sc1235_sensor_write_register(0x3633, 0x2f);
			break;
		case 1:
			sc1235_sensor_write_register(0x3631, 0x88);
			sc1235_sensor_write_register(0x3301, 0x1f);
			sc1235_sensor_write_register(0x3633, 0x23);
			break;
		case 2:
			sc1235_sensor_write_register(0x3631, 0x88);
			sc1235_sensor_write_register(0x3301, 0xff);
			sc1235_sensor_write_register(0x3633, 0x43);
			break;
		default:
			printk("%s gain_level error\n", __func__);
			break;
	}

	sc1235_sensor_write_register(0x3812, 0x30);
}


/*
 gain_level:
 0: <10x
 1: >=10x
 */
static void sc1235_dpc(int gain_10x)
{
	switch (gain_10x) {
		case 0:
			sc1235_sensor_write_register(0x5781, 0x04);
			sc1235_sensor_write_register(0x5785, 0x18);
			break;
		case 1:
			sc1235_sensor_write_register(0x5781, 0x02);
			sc1235_sensor_write_register(0x5785, 0x08);
			break;
		default:
			ak_sensor_print("%s err gain_10x:%d\n", __func__, gain_10x);
			break;
	}
}

static void sc1235_set_fps_async(void)
{
	if (to_fps != g_fps) {
		sc1235_sensor_write_register(0x320e, to_fps_value>>8);
		sc1235_sensor_write_register(0x320f, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int sc1235_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static int sc1235_cmos_updata_a_gain(const unsigned int a_gain)
{
	//??????ģ???????Ļص?????
	int tmp;
	int old_10x = sc1235_curr_again_10x;

	sc1235_set_fps_async();

	//printk(KERN_ERR "again=%d,%lu\n",a_gain,jiffies);
	g_a_gain  = a_gain;

	tmp = a_gain >> 4;
	sc1235_sensor_write_register(0x3e08, tmp >> 8);
	sc1235_sensor_write_register(0x3e09, tmp & 0xff);
	if (tmp < (2 << 4)) {
		sc1235_curr_again_level = 0;
		sc1235_ext(0);
	} else if (tmp < (4 << 4)) {
		sc1235_curr_again_level = 1;
		sc1235_ext(1);
	} else {
		sc1235_curr_again_level = 2;
		sc1235_ext(2);
	}

	if (tmp < (10 << 4))
		sc1235_curr_again_10x = 0;
	else
		sc1235_curr_again_10x = 1;

	if (old_10x != sc1235_curr_again_10x)
		sc1235_dpc(sc1235_curr_again_10x);

	return 0;
}

static int sc1235_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
	unsigned char exposure_time_ext;
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
	unsigned char tem;

	sc1235_set_fps_async();

 	//printk("exp_time=%d ",exp_time);
	//printk(KERN_ERR "exp_time=%d,%lu\n",exp_time,jiffies);
	exposure_time_ext =(exp_time>>12)&0xf;
    exposure_time_msb =(exp_time>>4)&0xff;
    exposure_time_lsb =((exp_time)&0xf)<<4;
    tem=sc1235_sensor_read_register(0x3e02)&0x0f;
    exposure_time_lsb =exposure_time_lsb|tem;
	//printk("msb = %d  ",exposure_time_msb);
	//printk("lsb = %d\n",exposure_time_lsb);
	sc1235_sensor_write_register(0x3e01,exposure_time_msb);
	sc1235_sensor_write_register(0x3e02,exposure_time_lsb);

    tem = sc1235_sensor_read_register(0x3e00) & 0xf0;
	tem |= exposure_time_ext;
    sc1235_sensor_write_register(0x3e00, tem);
	sc1235_ext(sc1235_curr_again_level);

    return EFFECT_FRAMES;
}

static int sc1235_cmos_update_a_gain_timer(void)
{
	return 0;
}
