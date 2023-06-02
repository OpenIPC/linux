/**
 * @file camera_imx307.c
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
#define SENSOR_I2C_ADDR         0x34
#define SENSOR_ID               0x0307
#define SENSOR_MCLK             36
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define DETAULT_FPS				30
#define SENSOR_OUTPUT_WIDTH		(1920 + SENSOR_VALID_OFFSET_X)
#define SENSOR_OUTPUT_HEIGHT	(1080 + SENSOR_VALID_OFFSET_Y)
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		        2
#define EXP_EFFECT_FRAMES 	    1
#define VSYNC_ACTIVE_MS		    30
#define MIPI_MBPS          450
#define MIPI_LANE		  2

static int imx307_cmos_updata_d_gain(const unsigned int d_gain);
static int imx307_cmos_updata_a_gain(const unsigned int a_gain);
static int imx307_cmos_updata_exp_time(unsigned int exp_time);
static int imx307_cmos_update_a_gain_timer(void);

static int g_fps        = DETAULT_FPS;
static int to_fps       = DETAULT_FPS;
static int to_fps_value = 0;

static int imx307_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int imx307_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;
#if 0
	int tmp;
	
	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;	

    tmp = ak_sensor_read_register(&i2cdata);
	if (tmp == data)
		return 0;
#endif

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int imx307_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++)
	{
		value=imx307_sensor_read_register(preg_info->reg_addr);
		//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		imx307_sensor_write_register(preg_info->reg_addr, preg_info->value);
		value=imx307_sensor_read_register(preg_info->reg_addr);
		//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

    g_fps = MAX_FPS;
	return 0;
}

static int imx307_read_id(void)
{
    return SENSOR_ID;
}

static int imx307_probe_id(void)
{
    u8  value1,value2;
    u32 id = 0;
	
    value1 = imx307_sensor_read_register(0x3004);
	value2 = imx307_sensor_read_register(0x3008);

    if (0x10 == value1 && 0xA0 == value2)
    {
        id = 0x0307;
    }
	
	printk(KERN_ERR "%s id:0x%x, 0x%x, 0x%x\n", __func__, id, value1, value2);

	if (id != SENSOR_ID)
		goto fail;

    return SENSOR_ID;

fail:
	return 0;
}

static int imx307_get_resolution(int *width, int *height)
{
	*width  = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int imx307_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int imx307_get_fps(void)
{
	return g_fps;
}

static int imx307_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type imx307_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int imx307_get_parameter(int param, void *value)
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

static int imx307_set_power_on(const int pwdn_pin, const int reset_pin)
{   
	ak_sensor_set_pin_as_gpio(reset_pin);
	ak_sensor_set_pin_dir(reset_pin, 1);
	ak_sensor_set_pin_level(reset_pin, SENSOR_RESET_LEVEL);
	ak_sensor_mdelay(30);
	ak_sensor_set_pin_level(reset_pin, !SENSOR_RESET_LEVEL);
	ak_sensor_mdelay(20);   //modify reset time, used to solve no camera interrupt problem
	
	return 0;
}

static int imx307_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);

	return 0;
}

static int imx307_set_fps(int fps)
{
	int ret = 0;
	int tmp = 0;

	/* tmp = ? / (reg0x3021~2=4400) / fps */
	switch (fps) {
		case 30:
			 tmp = 0x0465;
			 break;
		case 25:
			tmp = 0x0546;
			break;
		case 20:
			tmp = 0x0697;
			break;
		case 15:
			tmp = 0x08ca;
			break;
		case 12:
			tmp = 0x0a8c;
			break;
		case 10:
			tmp = 0x0d2f;
			break;
		case 8:
			tmp = 0x107a;
			break;
		default:
			printk(KERN_ERR "%s set fps=%d fail\n", __func__, fps);
			ret = -EINVAL;
			break;
	}

	if (!ret) 
	{
		//g_fps = fps;
		to_fps       = fps;
		to_fps_value = tmp;
		
	}

	return ret;
}

static int imx307_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int imx307_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB imx307_callback = 
{
	.sensor_init_func 				    = imx307_init,
	.sensor_read_reg_func			    = imx307_sensor_read_register,
	.sensor_write_reg_func			    = imx307_sensor_write_register,
	.sensor_read_id_func			    = imx307_read_id,
	.sensor_update_a_gain_func		    = imx307_cmos_updata_a_gain,
	.sensor_update_d_gain_func		    = imx307_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	    = imx307_cmos_updata_exp_time,
	.sensor_timer_func				    = imx307_cmos_update_a_gain_timer,

	.sensor_probe_id_func			    = imx307_probe_id,	//use IIC bus
	.sensor_get_resolution_func	        = imx307_get_resolution,
	.sensor_get_mclk_func			    = imx307_get_mclk,
	.sensor_get_fps_func			    = imx307_get_fps,
	.sensor_get_valid_coordinate_func	= imx307_get_valid_coordinate,
	.sensor_get_bus_type_func		    = imx307_get_bus_type,
	.sensor_get_parameter_func		    = imx307_get_parameter,

	.sensor_set_power_on_func		    = imx307_set_power_on,
	.sensor_set_power_off_func		    = imx307_set_power_off,
	.sensor_set_fps_func			    = imx307_set_fps,
	.sensor_set_standby_in_func		    = imx307_set_standby_in,
	.sensor_set_standby_out_func	    = imx307_set_standby_out
};

AK_SENSOR_MODULE(imx307_callback, imx307)


static int imx307_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

#if 1
static void imx307_set_fps_async(void)
{
	if (to_fps != g_fps) {
		imx307_sensor_write_register(0x3019, to_fps_value>>8);
		imx307_sensor_write_register(0x3018, to_fps_value & 0xff);

		g_fps = to_fps;
	}
}
#endif

static int gain_table[241]=	// from hisi
{
	// INDEX	again 0~71;		dgain >71
	1024,1059,1097,1135,1175,1217,1259,1304,1349,1397,1446,1497,1549,1604,1660,1719,1779,1842,1906, //5.4dB
	1973,2043,2114,2189,2266,2345,2428,2513,2601,2693,2788,2886,2987,3092,3201,3313,3430,3550, 3675, //11.1dB
	3804,3938,4076,4219,4368,4521,4680,4845,5015,5191,5374,5562,5758,5960,6170,6387,6611,6843, 7084, //16.8dB
	7333,7591,7857,8133,8419,8715,9021,9338,9667,10006,10358,10722,11099,11489,11893,12311,12743, //21.9dB
	13191,13655,14135,14631,15146,15678,16229,16799,17390,18001,18633,19288,19966,20668,21394,22146, //26.7dB
	22924,23730,24564,25427,26320,27245,28203,29194,30220,31282,32381,33519,34697,35917,37179,38485, //31.5dB
	39838,41238,42687,44187,45740,47347,49011,50734,52517,54362,56273,58250,60297,62416,64610,66880, //36.3dB
	69230,71663,74182,76789,79487,82281,85172,88165,91264,94471,97791,101228,104785,108468,112279,  //40.8dB
	116225,120310,124537,128914,133444,138134,142988,148013,153215,158599,164172,169942,175914,182096, //45dB
	188495,195119,201976,209074,216421,224027,231900,240049,248485,257217,266256,275613,285299,295325, //49.2db
	305703,316446,327567,339078,350994,363329,376097,389314,402995,417157,431817,446992,462700,478961,//53.4db
	495793,513216,531251,549921,569246,589250,609958,631393,653582,676550,700326,724936,750412,776783,//57.6db
	804081,832338,861589,891867,923209,955652,989236,1024000,1059986,1097236,1135795,1175709,1217026,1259795,//61.8db
	1304067,1349895,1397333,1446438,1497269,1549887,1604353,1660734,1719095,1779509,1842044,1906777,1973786,2043149,//66db
	2119949,2189273,2266209,2345848,2428287,2513622,2601956,2693395,2788046,2886024,2987445,3092431,3201105,3313599,//70.2db
	3430046,3550585,3675361,3804521,3938220,4076617,//72db
	
};
static unsigned int _cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
		unsigned int *a_gain_out ,unsigned int *d_gain_out)
{
	#define DEAD_INDEX	240
	int tmp = 0;
	int i = 0;
	
	/*
	 * gain(reg) = (gain(dB) * 10 / 3)
	 *
	 * */
	 
#if 0
	if (a_gain <= 18432)
	tmp = ((a_gain * 10) >> 8) / 3;
#endif
    imx307_set_fps_async();
	a_gain <<= 2;
	if (a_gain >= gain_table[DEAD_INDEX]) {
		a_gain = gain_table[DEAD_INDEX];
		tmp = DEAD_INDEX;
	}

	for (i = 1; i <= DEAD_INDEX; i++) {
		if (a_gain < gain_table[i]) {
			tmp = i - 1;
			break;
		}
	}
	imx307_sensor_write_register(0x3014, tmp);
	return 0;
}

static int imx307_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain ;
	tmp_d_gain = 0;

	return _cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
}

static int imx307_cmos_updata_exp_time(unsigned int exp_time)
{
	unsigned int exposure_time_ext;
	unsigned int exposure_time_msb;
	imx307_set_fps_async();
	if((to_fps_value - 0x2) < exp_time)
	{
		exp_time = to_fps_value - 0x2;
	}
	
	exp_time = to_fps_value -(1 + exp_time);
	
	exposure_time_ext =(exp_time>>8)&0xff;
	exposure_time_msb =(exp_time)&0xff;
	imx307_sensor_write_register(0x3021,exposure_time_ext);
	imx307_sensor_write_register(0x3020,exposure_time_msb);
	return EXP_EFFECT_FRAMES;
}

static int imx307_cmos_update_a_gain_timer(void)
{
    return 0;
}
