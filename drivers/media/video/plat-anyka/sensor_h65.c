/**
 * @file camera_h65.c
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
#define SENSOR_I2C_ADDR         0x60 
#define SENSOR_ID               0xa065
#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	960
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define MIPI_MHZ		432
#define MIPI_LANE		1
#define EFFECT_FRAMES 	1
#define VSYNC_ACTIVE_MS			36

#define PCLK_FREQ  (43200000)  

static int _target_frame_height = 0;
static int _target_exp_ctrl = 0;

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

static int h65_cmos_updata_d_gain(const unsigned int d_gain);
static int h65_cmos_updata_a_gain(const unsigned int a_gain);
static int h65_cmos_updata_exp_time(unsigned int exp_time);

static int g_fps = 25;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;

static int h65_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int h65_sensor_write_register(int reg, int data)
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

static int h65_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
//int value;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=h65_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		if (preg_info->reg_addr == 0xffff)
			ak_sensor_mdelay(preg_info->value);
		else
			h65_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=h65_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}
	g_fps = MAX_FPS;
	to_fps = MAX_FPS;

	return 0;
}

static int h65_read_id(void)
{
	return SENSOR_ID;
}

static int h65_probe_id(void)
{
    u8 value;
    u32 id;

    value = h65_sensor_read_register(0x0a);
    id = value << 8;
    value = h65_sensor_read_register(0x0b);
    id |= value;    

	if (id == 0x0a65)
		return SENSOR_ID;

    return 0;
}

static int h65_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int h65_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int h65_get_fps(void)
{
	return g_fps;
}

static int h65_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type h65_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int h65_get_parameter(int param, void *value)
{
	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {
		case GET_MIPI_MHZ:
			*((int *)value) = MIPI_MHZ;
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

static int h65_set_power_on(const int pwdn_pin, const int reset_pin)
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

	g_fps = 25;
	ak_sensor_print("%s\n",__func__);
	return 0;
}

static int h65_set_power_off(const int pwdn_pin, const int reset_pin)
{
#if 0
    u8 Reg0x3d = 0x48;
    u8 Reg0xc3 = 0x00;
#endif

#if 0
	//sccb software standby mode
    
    h65_sensor_write_register(0x3d, Reg0x3d);
    h65_sensor_write_register(0xc3, Reg0xc3);
#endif

    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int h65_set_fps(const int fps)
{
	int ret = 0;
	int tmp = 0;

	switch (fps) {
	case 30:
		//h65_sensor_write_register(0x23,0x03);
		//h65_sensor_write_register(0x22,0xe8);
		tmp = 0x03e8;
		break;

	case 25:
		//h65_sensor_write_register(0x23,0x04);
		//h65_sensor_write_register(0x22,0xb0);
		tmp = 0x04b0;
		break;

	case 15:
		//h65_sensor_write_register(0x23,0x07);
		//h65_sensor_write_register(0x22,0xd0);
		tmp = 0x07d0;
		break;

	case 13:	//actual: 12.5fps
	case 12:
		//h65_sensor_write_register(0x23,0x09);
		//h65_sensor_write_register(0x22,0x60);
		tmp = 0x0960;
		break;

	case 10:
		//h65_sensor_write_register(0x23,0x0b);
		//h65_sensor_write_register(0x22,0xb8);
		tmp = 0x0bb8;
		break;
	case 8:
		//h65_sensor_write_register(0x23,0x0e);
		//h65_sensor_write_register(0x22,0xa6);
		tmp = 0x0ea6;
		break;

	default:
		ak_sensor_print("%s set fps fail\n", __func__);
		ret = -EINVAL;
		break;
	}

	if (!ret){
		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;	
	}

	return ret;
}

static int h65_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int h65_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}


static int __on_timer(void) {

	/// 高温补偿逻辑。
	int const reg_4f = h65_sensor_read_register (0x4f);
	int const reg_4b = h65_sensor_read_register (0x4b);
	int const value = (reg_4f & 0x3) * 0x100 + reg_4b;

//	printk (KERN_ERR "4f=%02x 4b=%02x val=%02x\r\n", reg_4f, reg_4b, value);

	if (value > 0xc8) {
		h65_sensor_write_register (0x7f,0x56);
		h65_sensor_write_register (0x8e,0x03);
	} else {
		h65_sensor_write_register(0x7f,0x52);
		h65_sensor_write_register(0x8e,0x00);
	}

	return 0;
}


static AK_ISP_SENSOR_CB h65_callback = 
{
	.sensor_init_func 				= h65_init,
	.sensor_read_reg_func			= h65_sensor_read_register,
	.sensor_write_reg_func			= h65_sensor_write_register,
	.sensor_read_id_func			= h65_read_id,
	.sensor_update_a_gain_func		= h65_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= h65_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= h65_cmos_updata_exp_time,
	.sensor_timer_func				= __on_timer,

	.sensor_probe_id_func			= h65_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= h65_get_resolution,
	.sensor_get_mclk_func			= h65_get_mclk,
	.sensor_get_fps_func			= h65_get_fps,
	.sensor_get_valid_coordinate_func	= h65_get_valid_coordinate,
	.sensor_get_bus_type_func		= h65_get_bus_type,
	.sensor_get_parameter_func		= h65_get_parameter,

	.sensor_set_power_on_func		= h65_set_power_on,
	.sensor_set_power_off_func		= h65_set_power_off,
	.sensor_set_fps_func			= h65_set_fps,
	.sensor_set_standby_in_func		= h65_set_standby_in,
	.sensor_set_standby_out_func	= h65_set_standby_out
};

AK_SENSOR_MODULE(h65_callback, h65)

///////////////////////////////////////////////////////////////////////////////////////////

static unsigned int h65_cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
	//ģ??????????ת???????????㷨??????????ת????sensor????????ʽ
	unsigned int ag_interget_table[17] = {0,  1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8,
        8, 8, 8, 8, 16};
    unsigned int ag_integer, ag_fraction, tmp;
	unsigned int ag_return;
 
	tmp =  a_gain / 16;
	if(tmp == 0)
	{
		tmp = 1;
		ak_sensor_print("Div ERROR 000!\n");
	}
	//ak_sensor_print("tmp  = %d \n",tmp);
	if(tmp<16)
	{
       tmp =  OV_CLIP3(0, 16, tmp);
       ag_integer =  ag_interget_table[tmp];
	}
	else if ((16<=tmp)&&(tmp<32))
		ag_integer = 16;
	else if ((32<=tmp)&&(tmp<64))
		ag_integer = 32;
	else// if((64<=tmp)&&(tmp<128))
		ag_integer = 64;

	
    ag_fraction = (a_gain / ag_integer) - 16;
    ag_fraction =  OV_CLIP3(0, 15, ag_fraction);
 
    if (((ag_fraction + 16) * ag_integer) < a_gain)
    {
        if (ag_fraction < 15)
        {
            ag_fraction++;
        }
       // else if (ag_integer < 16)
       else if (ag_integer <128)
        {
            tmp++;
			/*
            ag_integer  =  ag_interget_table[tmp];
            ag_fraction = 0;
            */
		    if(tmp<16)
			{
		       tmp =  OV_CLIP3(0, 16, tmp);
		       ag_integer =  ag_interget_table[tmp];
			}
			else if ((16<=tmp)&&(tmp<32))
				ag_integer = 16;
			else if ((32<=tmp)&&(tmp<64))
				ag_integer = 32;
			else if((64<=tmp)&&(tmp<128))
				ag_integer = 64;
				
				ag_fraction = 0;
        }
        else
        {
        }
    }
 
    switch (ag_integer)
    {
        case 1 :
            ag_integer = 0x00;
            break;
        case 2 :
            ag_integer = 0x10;
            break;
        case 4 :
            ag_integer = 0x20;
            break;
        case 8 :
            ag_integer = 0x30;
            break;
		case 16 :
            ag_integer = 0x40;
            break;
        case 32 :
            ag_integer = 0x50;
			break;
		case 64 :
            ag_integer = 0x60;
			break;
		default: 
			ag_integer = 0x00;
            break;
    }

    ag_return = ag_integer|ag_fraction;
	*a_gain_out = ag_return ; 
 
	return ag_return;
}

static void h65_set_fps_async(void)
{
	if (to_fps != g_fps) {
		h65_sensor_write_register(0x23, to_fps_value>>8);
		h65_sensor_write_register(0x22, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static int h65_cmos_updata_d_gain(const unsigned int d_gain)
{
	//?????????????Ļص?????
	return 0;
}

static int h65_cmos_updata_a_gain(const unsigned int a_gain)
{
	//??????ģ???????Ļص?????
	unsigned short ag_value;
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain>>4;
	tmp_d_gain = 0;
	
	h65_set_fps_async();

	//ag_value = cmos_gains_update(isp,isp->aec_param.current_a_gain);
	h65_cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
	ag_value = tmp_a_gain_out;
	//ak_sensor_print("a_gain=%d\n",a_gain);
	h65_sensor_write_register(0x00, tmp_a_gain_out);
	// updata_auto_exposure_num++;
	//isp->aec_param.current_exposure_time = tmp_exposure_time;
	return 0;
}


static int h65_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
//	ak_sensor_print("exp_time=%d\n",exp_time);
    exposure_time_msb =(exp_time>>8)&0xff;
    exposure_time_lsb = exp_time&0xff;
	
	h65_set_fps_async();
	
//	ak_sensor_print("msb = %d\n",exposure_time_msb);
//	ak_sensor_print("lsb = %d\n",exposure_time_lsb);
	h65_sensor_write_register(0x02,exposure_time_msb);
	h65_sensor_write_register(0x01,exposure_time_lsb);
//	ak_sensor_print("0x16=%d\n",h65_sensor_read_register(0x16));

    return EFFECT_FRAMES;
}
