/**
 * @file camera_ar0130.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author xia_wenting 
 * @date 2011-09-21
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		1 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x20 
#define SENSOR_ID               0x0130
#define SENSOR_MCLK             27
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        2
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	960
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define EFFECT_FRAMES 	1
#define VSYNC_ACTIVE_MS			36

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

static int ar0130_cmos_updata_d_gain(const unsigned int d_gain);
static int ar0130_cmos_updata_a_gain(const unsigned int a_gain);
static int ar0130_cmos_updata_exp_time(unsigned int exp_time);

static int g_fps = 25;
static int to_fps = 25;
static int to_fps_value = 0;

static int ar0130_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int ar0130_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;
//	ak_sensor_print("%s~~~~~~~~~~reg:0x%x,data:0x%x\n",__func__,reg,data);
	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int ar0130_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
//int value;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=ar0130_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		if (preg_info->reg_addr == 0xfd) {
			ak_sensor_mdelay(preg_info->value);
		} 
		else
			ar0130_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=ar0130_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	g_fps = 25;
	to_fps = 25;
	return 0;
}

static int ar0130_read_id(void)
{
    return SENSOR_ID;
}

static int ar0130_probe_id(void)
{
    u16 value;

    value = ar0130_sensor_read_register(0x30a0);
	if (value != 0x0001)
		goto fail;
    value = ar0130_sensor_read_register(0x319a);
	if (value != 0x100b)
		goto fail;
    value = ar0130_sensor_read_register(0x319c);
	if (value != 0x0014)
		goto fail;

    return SENSOR_ID;
fail:
	return 0;
}

static int ar0130_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int ar0130_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int ar0130_get_fps(void)
{
	return g_fps;
}

static int ar0130_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type ar0130_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int ar0130_get_parameter(int param, void *value)
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

static int ar0130_set_power_on(const int pwdn_pin, const int reset_pin)
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

	return 0;
}

static int ar0130_set_power_off(const int pwdn_pin, const int reset_pin)
{
//    u8 Reg0x3d = 0x48;
//   u8 Reg0xc3 = 0x00;

	//sccb software standby mode
    
//    ar0130_sensor_write_register(0x3d, Reg0x3d);
//   ar0130_sensor_write_register(0xc3, Reg0xc3);

    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int ar0130_set_fps(int fps)
{
	int ret = 0;
	int tmp=0;

	switch (fps) {
		case 30:
			tmp = 0x3e6;
			break;

		case 25:
			tmp = 0x4ae;
			break;

		case 13:    //actual: 12.5fps
		case 12:
			tmp = 0x95c;
			break;

		case 10:
			tmp = 0xbb3;
			break;

		case 8: 
			tmp = 0xea0;
			//ar0130_sensor_write_register(0x300a, 0xe0a);	//8.333 fps
			break;

		case 5: 
			tmp = 0x1767;

		default:
			ak_sensor_print("%s set fps fail\n", __func__);
			ret = -EINVAL;
			break;
	}

	if (!ret) {
		//ar0130_sensor_write_register(0x300a, 0x3e6);
		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int ar0130_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int ar0130_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB ar0130_callback = 
{
	.sensor_init_func 				= ar0130_init,
	.sensor_read_reg_func			= ar0130_sensor_read_register,
	.sensor_write_reg_func			= ar0130_sensor_write_register,
	.sensor_read_id_func			= ar0130_read_id,
	.sensor_update_a_gain_func		= ar0130_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= ar0130_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= ar0130_cmos_updata_exp_time,
	
	.sensor_probe_id_func			= ar0130_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= ar0130_get_resolution,
	.sensor_get_mclk_func			= ar0130_get_mclk,
	.sensor_get_fps_func			= ar0130_get_fps,
	.sensor_get_valid_coordinate_func	= ar0130_get_valid_coordinate,
	.sensor_get_bus_type_func		= ar0130_get_bus_type,
	.sensor_get_parameter_func		= ar0130_get_parameter,

	.sensor_set_power_on_func		= ar0130_set_power_on,
	.sensor_set_power_off_func		= ar0130_set_power_off,
	.sensor_set_fps_func			= ar0130_set_fps,
	.sensor_set_standby_in_func		= ar0130_set_standby_in,
	.sensor_set_standby_out_func	= ar0130_set_standby_out
};

AK_SENSOR_MODULE(ar0130_callback, ar0130)

///////////////////////////////////////////////////////////////////////////////////////////

static unsigned int ar0130_cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
	//ģ??????????ת???????????㷨??????????ת????sensor????????ʽ
	unsigned int ag_interget_table[17] = {0,  1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8,
        8, 8, 8, 8, 8};
    unsigned int ag_integer, ag_fraction, tmp, reg_ag;
	unsigned int ag_return;
 
	tmp =  a_gain / 256;
	if(tmp == 0)
	{
		tmp = 1;
		ak_sensor_print("Div ERROR 000!\n");
	}
	
    tmp =  OV_CLIP3(0, 16, tmp);
    ag_integer =  ag_interget_table[tmp];
 
    ag_fraction = (a_gain / ag_integer) >>3;
    
 
    /*if (((ag_fraction + 16) * ag_integer) < a_gain)
    {
        if (ag_fraction < 15)
        {
            ag_fraction++;
        }
        else if (ag_integer < 16)
        {
            tmp++;
            ag_integer  =  ag_interget_table[tmp];
            ag_fraction = 0;
        }
        else
        {
        }
    }*/
 
    switch (ag_integer)
    {
        case 1 :
            reg_ag = 0x1300;
            break;
        case 2 :
            reg_ag = 0x1310;
            break;
        case 4 :
            reg_ag = 0x1320;
            break;
        case 8 :
            reg_ag = 0x1330;
            break;
        default:
            reg_ag = 0x1300;
            break;
    }

    ag_return = ag_integer;
	*a_gain_out = ag_return ; 

	ar0130_sensor_write_register(0x30b0, reg_ag);
	ar0130_sensor_write_register(0x305e, ag_fraction);
	ak_sensor_print("gain=%d, again=%d, dgain=%d\n", a_gain, ag_integer, ag_fraction);	 
	return ag_return;
}

static void ar0130_set_fps_async(void)
{
	if (to_fps != g_fps) {
		ar0130_sensor_write_register(0x300a, to_fps_value);

		g_fps = to_fps;
	}
}

static int ar0130_cmos_updata_d_gain(const unsigned int d_gain)
{
	//?????????????Ļص?????
	return 0;
}

static int ar0130_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int tmp_d_gain=0;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	ar0130_set_fps_async();

	ar0130_cmos_gains_convert(a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
	return 0;
}


static int ar0130_cmos_updata_exp_time(unsigned int exp_time)
{

	ar0130_set_fps_async();

	//ak_sensor_print("exp_time=%d\n",exp_time);
	ar0130_sensor_write_register(0x3012, exp_time);
	//ak_sensor_print("exp_time=%d, 0x305e=%x\n", exp_time, ar0130_sensor_read_register(0x305e));
    return EFFECT_FRAMES;
}
