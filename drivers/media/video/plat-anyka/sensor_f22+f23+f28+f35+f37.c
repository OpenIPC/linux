/**
 * @file sensor_f22_f23_f28_f35_f37.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author  luo_zhanzhao
 * @date
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		1 
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x80
#define SENSOR_ID_F22           (0x0f22)
#define SENSOR_ID_F23           (0x0f23)
#define SENSOR_ID_F28           (0x0f28)
#define SENSOR_ID_F35           (0x0f35)
#define SENSOR_ID_F37           (0x0f37)

/**
 * 数值为 0 表示未检测到，当检测到某一款 SENSOR 时此值将固定为一个非 0 的值。
 */
static  u32 SENSOR_ID =         0;

#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					25
#define SENSOR_OUTPUT_WIDTH		1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		    BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     DVP_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_3V3

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES	 	1
#define VSYNC_ACTIVE_MS			31

//sensor_ucode,sensor_id,sensor_mclk,sensor_io_interface,mipi_mbps,mipi_lane,pclk
static struct ak_sensor_parameter sensor_parameter[] = 
{
    {0x0f22, 0x0f22, 24, MIPI_INTERFACE, 192, 2, 43200000},
    {0x0f23, 0x0f23, 24, MIPI_INTERFACE, 192, 1, 43200000},
    {0X0f28, 0X0f28, 24, MIPI_INTERFACE, 192, 1, 43200000},
    {0x0f35, 0x0f35, 24, MIPI_INTERFACE, 192, 2, 43200000},
    {0X0f37, 0x0f37, 24, MIPI_INTERFACE, 720, 1, 43200000},
};

static int f22_cmos_updata_d_gain(const unsigned int d_gain);
static int f22_cmos_updata_a_gain(const unsigned int a_gain);
static int f22_cmos_updata_exp_time(unsigned int exp_time);
static int f22_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;
static struct ak_sensor_parameter *current_sensor_parameter = NULL;

static int f22_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int f22_sensor_write_register(int reg, int data)
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

static int f22_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
//int value;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=f22_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		if (preg_info->reg_addr == 0xffff)
			ak_sensor_mdelay(preg_info->value);
		else
			f22_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=f22_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	g_fps = to_fps = MAX_FPS;

	return 0;
}



static int f22_probe_id (void)
{
    u8 value = 0;
    u32 id = 0;
    int i=0;
	/// ��ȡ 0x0a 0x0b ��ȡ��ǰ SENSOR ����š�
    value = f22_sensor_read_register(0x0a);
    id = value << 8;
    value = f22_sensor_read_register(0x0b);
    id |= value;

//    for(i=0; i<(sizeof(sensor_parameter)/sizeof(struct ak_sensor_parameter));i++)
    for(i=0;i<5;i++)
    {
        if(id == sensor_parameter[i].sensor_ucode)
        {
            current_sensor_parameter = &sensor_parameter[i];
            break;
        }
    }
    
    SENSOR_ID = current_sensor_parameter->sensor_id;
    return SENSOR_ID;
 /*   
	switch (id) {
	case SENSOR_ID_F22:
	case SENSOR_ID_F23:
	case SENSOR_ID_F28:
	case SENSOR_ID_F35:
	case SENSOR_ID_F37:
		SENSOR_ID = id; ///< ��¼��ǰ SENSOR ��š�
		break;

	default:
		break;
	}

    printk (KERN_ERR "%s id:0x%x\n", __func__, SENSOR_ID);
    return SENSOR_ID;*/
}


static int f22_read_id(void)
{
	if (0 == SENSOR_ID) {
		SENSOR_ID = f22_probe_id ();
	}

	if (0 == SENSOR_ID) {
		return -1;
	}

	return SENSOR_ID;
}


static int f22_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int f22_get_mclk(void)
{
    return current_sensor_parameter->sensor_mclk;
//	return SENSOR_MCLK;
}

static int f22_get_fps(void)
{
	return g_fps;
}

static int f22_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type f22_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int f22_get_parameter(int param, void *value)
{
	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {

		case GET_MIPI_MHZ:
			*((int *)value) = current_sensor_parameter->mipi_mbps;
            //*((int *)value) = MIPI_MHZ;
			break;

		case GET_VSYNC_ACTIVE_MS:
			*((int *)value) = VSYNC_ACTIVE_MS;
			break;

		case GET_CUR_FPS:
			*((int *)value) = g_fps;
			break;

		case GET_MIPI_LANE:
			*((int *)value) = current_sensor_parameter->mipi_lane;
			break;

		case GET_INTERFACE:
			*((int *)value) = current_sensor_parameter->sensor_io_interface;
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

static int f22_set_power_on(const int pwdn_pin, const int reset_pin)
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

	printk(KERN_ERR"%s\n",__func__);
	return 0;
}

static int f22_set_power_off(const int pwdn_pin, const int reset_pin)
{
#if 0
    u8 Reg0x3d = 0x48;
    u8 Reg0xc3 = 0x00;
#endif

#if 0
	//sccb software standby mode
    
    f22_sensor_write_register(0x3d, Reg0x3d);
    f22_sensor_write_register(0xc3, Reg0xc3);
#endif

    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int f22_set_fps(const int fps)
{
	int ret = 0;
	int tmp = 0;
	int const sensor_if = current_sensor_parameter->sensor_io_interface;

	if (sensor_if == DVP_INTERFACE) {
		/* actual mclk=24MHz pclk=76.8MHz, but should 30fps * (1280*2) * 1125 = 76.8MHz
	 	* so all follow set fps is too higher than "FPS"
		*/
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
				ak_sensor_print("%s set fps fail\n", __func__);
				ret = -EINVAL;
				break;
		}
	}else if (sensor_if == MIPI_INTERFACE) {
		/* actual mclk=24MHz pclk=76.8MHz, but should 30fps * (1381*2) * 1125 = 768MHz
		 * so all follow set fps is too higher than "FPS"
		 */
		switch (fps) {
			case 25:
				tmp = 0x0465;
				break;

			case 20:
				tmp = 0x056e;
				break;

			case 15:
				tmp = 0x073d;
				break;

			case 12:
				tmp = 0x08b0;
				break;

			case 10:
				tmp = 0x0adc;
				break;

			case 8:
				tmp = 0x0d93;
				break;

			case 5:
				tmp = 0x15b9;
				break;

			default:
				ak_sensor_print("%s set fps fail\n", __func__);
				ret = -EINVAL;
				break;
		}

	}

	if (!ret) {
#if 0
		f22_sensor_write_register(0x23,tmp >> 8);
		f22_sensor_write_register(0x22,tmp & 0xff);
#endif
		//g_fps = fps;
		to_fps = fps;
		to_fps_value = tmp;
	}

	return ret;
}

static int f22_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int f22_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}


static AK_ISP_SENSOR_CB f22_callback =
{
	.sensor_init_func 				= f22_init,
	.sensor_read_reg_func			= f22_sensor_read_register,
	.sensor_write_reg_func			= f22_sensor_write_register,
	.sensor_read_id_func			= f22_read_id,
	.sensor_update_a_gain_func		= f22_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= f22_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= f22_cmos_updata_exp_time,
	.sensor_timer_func				= f22_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= f22_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= f22_get_resolution,
	.sensor_get_mclk_func			= f22_get_mclk,
	.sensor_get_fps_func			= f22_get_fps,
	.sensor_get_valid_coordinate_func	= f22_get_valid_coordinate,
	.sensor_get_bus_type_func		= f22_get_bus_type,
	.sensor_get_parameter_func		= f22_get_parameter,

	.sensor_set_power_on_func		= f22_set_power_on,
	.sensor_set_power_off_func		= f22_set_power_off,
	.sensor_set_fps_func			= f22_set_fps,
	.sensor_set_standby_in_func		= f22_set_standby_in,
	.sensor_set_standby_out_func	= f22_set_standby_out,
};

AK_SENSOR_MODULE(f22_callback, f22)

///////////////////////////////////////////////////////////////////////////////////////////

static void f22_set_fps_async(void)
{
	if (to_fps != g_fps) {
		f22_sensor_write_register(0x23, to_fps_value >> 8);
		f22_sensor_write_register(0x22, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}

static unsigned int f22_cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
	//ģ??????????ת???????????�??????????ת????sensor????????ʽ
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

static int f22_cmos_updata_d_gain(const unsigned int d_gain)
{
	//?????????????Ļص?????
	return 0;
}

/**
 * 根据 SENSOR 4A/4C/4F 三个值获取 BLC Gb 的值。
 */
static int get_blc_gb (int reg0x4a, int reg0x4c, int reg0x4f) {

	int blc_gb = 0;

	reg0x4a >>= 6;
	reg0x4a &= 0x01; ///< 0X4A BIT6
	reg0x4f >>= 2;
	reg0x4f &= 0x03; ///< 0X4F BIT[3:2]
	reg0x4c &= 0xff;

	blc_gb = (reg0x4a << 10) | (reg0x4f << 8) | reg0x4c;
	return blc_gb;

}


static int f22_cmos_updata_a_gain(const unsigned int a_gain)
{
	//??????ģ???????Ļص?????
	//unsigned short ag_value;
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain>>4;
	tmp_d_gain = 0;

	f22_set_fps_async();

	//ag_value = cmos_gains_update(isp,isp->aec_param.current_a_gain);
	f22_cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
  
	if (SENSOR_ID_F23 == SENSOR_ID) {

		/// 背光补偿调整策略。
		int const blc_gb = get_blc_gb (
				f22_sensor_read_register (0x4a),
				f22_sensor_read_register (0x4c),
				f22_sensor_read_register (0x4f));
		int const blc_gb_thrd = get_blc_gb (0x05, 0x93, 0);

		if (blc_gb > blc_gb_thrd) {
			f22_sensor_write_register(0x8e, 0x04);
		} else {
			f22_sensor_write_register(0x8e, 0x00);
		}

		/// F23 太阳黑子策略。
		if (tmp_a_gain_out >= 0x30) {
			f22_sensor_write_register(0x0c, 0x40);
			f22_sensor_write_register(0x66, 0x44);
		} else {
			f22_sensor_write_register(0x66, 0x04);
			f22_sensor_write_register(0x0c, 0x00);
		}

	} else if (SENSOR_ID_F37 == SENSOR_ID) {
		/// F37 太阳黑子策略。

		int reg0x2f = f22_sensor_read_register (0x2f);
		int reg0x0c = f22_sensor_read_register (0x0c);
		int reg0x82 = f22_sensor_read_register (0x82);
		
		if (tmp_a_gain_out < 0x10) {
			reg0x2f |= (1<<5);
			reg0x0c |= (1<<6);
			reg0x82 |= (1<<1);
		} else {
			reg0x2f &= ~(1<<5);
			reg0x0c &= ~(1<<6);
			reg0x82 &= ~(1<<1);
		}
		
		f22_sensor_write_register (0x2f, reg0x2f);
		f22_sensor_write_register (0x0c, reg0x0c);
		f22_sensor_write_register (0x82, reg0x82);

	}
	
	//ak_sensor_print("a_gain=%d\n",a_gain);	
	f22_sensor_write_register(0x00, tmp_a_gain_out);
	
	// updata_auto_exposure_num++;
	//isp->aec_param.current_exposure_time = tmp_exposure_time;
	return 0;
}


static int f22_cmos_updata_exp_time(unsigned int exp_time)
{
    //?????ع?ʱ???Ļص?????
    unsigned char exposure_time_msb;
	unsigned char exposure_time_lsb;
//	ak_sensor_print("exp_time=%d\n",exp_time);
    exposure_time_msb =(exp_time>>8)&0xff;
    exposure_time_lsb = exp_time&0xff;

	f22_set_fps_async();

//	ak_sensor_print("msb = %d\n",exposure_time_msb);
//	ak_sensor_print("lsb = %d\n",exposure_time_lsb);
	f22_sensor_write_register(0x02,exposure_time_msb);
	f22_sensor_write_register(0x01,exposure_time_lsb);
//	ak_sensor_print("0x16=%d\n",f22_sensor_read_register(0x16));
    return EXP_EFFECT_FRAMES;
}

static int f22_cmos_update_a_gain_timer(void)
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
			f22_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}
