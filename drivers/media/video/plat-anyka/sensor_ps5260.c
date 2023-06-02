/**
 * @file camera_ps5260.c
 * @brief camera driver file
 * Copyright (C) 2011 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author dong_feilong
 * @date 2015-11-05
 * @version 1.0
 * @ref
 */ 
#include "ak_sensor_common.h"
#include <linux/delay.h>


#define SENSOR_PWDN_LEVEL		0
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x90 
#define SENSOR_ID               0x5260
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
#define VSYNC_ACTIVE_MS			36
#define MIPI_MBPS		480
#define MIPI_LANE		2

#define AG_BASE		(4)		// Ratio
#define AG_HS_NODE	(4*1024)	// 6.0x (U.9)
#define AG_LS_NODE	(3*1024)	// 5.0x (U.9)
#define NE_PATCH_LB	(25)
#define NE_PATCH_UB	(200)
#define NE_PATCH_SCALE	(38)

static int ps5260_cmos_updata_d_gain(const unsigned int d_gain);
static int ps5260_cmos_updata_a_gain(const unsigned int a_gain);
static int ps5260_cmos_updata_exp_time(unsigned int exp_time);
static int ps5260_cmos_update_a_gain_timer(void);

static int g_fps = MAX_FPS;
static int to_fps = MAX_FPS;
static int to_fps_value = 0;

static unsigned int g_total_lines = 1350;	// Total exposure lines, xxx = Bank1 (0xA<<8|0xB), based on initial setting
static unsigned int g_lines = 4;	// Exposure lines, xxx = Bank1 (0xA<<8|0xB) - (0xC<<8|0xD), based on initial setting
static unsigned int g_sghd_flg = 0;	// HS/LS mode flag, xxx = Bank1 0x18, based on initial setting
static unsigned int g_sns_ver = 0; // Sensor version
static unsigned int g_sns_const1 = 0;	// Exposure constant, xxx = Bank1 (0xE<<8|0xF) + (0x5F<<8|0x60)

// PS5260 Gain Table for 1.0x ~ 128x (U.9)
static const unsigned int ps5260_gain_table[97] = {    
	1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 	
	1792, 1856, 1920, 1984, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 	
	3072, 3200, 3328, 3456, 3584, 3712, 3840, 3968, 4096, 4352, 4608, 4864, 	
	5120, 5376, 5632, 5888, 6144, 6400, 6656, 6912, 7168, 7424, 7680, 7936, 	
	8192, 8704, 9216, 9728, 10240, 10752, 11264, 11776, 12288, 12800, 13312, 	
	13824, 14336, 14848, 15360, 15872, 16384, 17408, 18432, 19456, 20480, 21504, 	
	22528, 23552, 24576, 25600, 26624, 27648, 28672, 29696, 30720, 31744, 32768, 	
	34816, 36864, 38912, 40960, 43008, 45056, 47104, 49152, 51200, 53248, 55296, 	
	57344, 59392, 61440, 63488, 65536,
 };




static int ps5260_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int ps5260_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int ps5260_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;

	for (i = 0; i < para->num; i++) {
//value=ps5260_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		ps5260_sensor_write_register(preg_info->reg_addr, preg_info->value);
//value=ps5260_sensor_read_register(preg_info->reg_addr);
//printk(KERN_ERR "after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}
	
	ps5260_sensor_write_register(0xEF, 0x01);	// Bank1	
	g_sns_ver = ps5260_sensor_read_register(0x01)&0x0F;	// Get Sensor Version	
	g_sns_const1 = ((ps5260_sensor_read_register(0xE) & 0x1f)<<8|ps5260_sensor_read_register(0xF)) + ((ps5260_sensor_read_register(0x10)&0x04)<<6|ps5260_sensor_read_register(0x12));	
	ps5260_sensor_write_register(0xEF, 0x00);	// Bank0	
	
	ps5260_sensor_write_register(0xEF, 0x01);	// Bank1



    g_fps = MAX_FPS;
	return 0;
}

static int ps5260_read_id(void)
{
    return SENSOR_ID;
}

static int ps5260_probe_id(void)
{
    u8 value;
    u32 id;

    ps5260_sensor_write_register(0xef, 0x00);

    value = ps5260_sensor_read_register(0x00);
    id = value << 8;

    value = ps5260_sensor_read_register(0x01);
    id |= value;

	printk(KERN_ERR "%s id:0x%x,id:0x%x\n", __func__, id, SENSOR_ID);
	
	if (id != SENSOR_ID)
		goto fail;
           
    return SENSOR_ID;

fail:
	return 0;
}

static int ps5260_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int ps5260_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int ps5260_get_fps(void)
{
	return g_fps;
}

static int ps5260_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type ps5260_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int ps5260_get_parameter(int param, void *value)
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
static int ps5260_set_power_on(const int pwdn_pin, const int reset_pin)
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

static int ps5260_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int ps5260_set_fps(int fps)
{
	int ret = 0;
	int tmp=0;

    /* tmp = 80M / (bank1,(reg0x27~28)/2=2110) / fps */
	switch (fps) {
		/*case 30:
			tmp = 0x0464;
			break;*/
		case 25:
			tmp = 0x0545;
			break;
		case 20:
			tmp = 0x0696;
			break;
		case 15:
			tmp = 0x08c9;
			break;
		case 12:
			tmp = 0x0afb;
			break;
		case 10:
			tmp = 0x0d2e;
			break;
		case 8:
			tmp = 0x1079;
			break;
		default:
			printk(KERN_ERR"%s set fps=%d fail\n", __func__, fps);
			ret = -1;
			break;
	}

    if (!ret) {
		to_fps = fps;
		to_fps_value = tmp;
    }
	
	return ret;
}

static int ps5260_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int ps5260_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB ps5260_callback = 
{
	.sensor_init_func 				= ps5260_init,
	.sensor_read_reg_func			= ps5260_sensor_read_register,
	.sensor_write_reg_func			= ps5260_sensor_write_register,
	.sensor_read_id_func			= ps5260_read_id,
	.sensor_update_a_gain_func		= ps5260_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= ps5260_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= ps5260_cmos_updata_exp_time,
	.sensor_timer_func				= ps5260_cmos_update_a_gain_timer,

	.sensor_probe_id_func			= ps5260_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= ps5260_get_resolution,
	.sensor_get_mclk_func			= ps5260_get_mclk,
	.sensor_get_fps_func			= ps5260_get_fps,
	.sensor_get_valid_coordinate_func	= ps5260_get_valid_coordinate,
	.sensor_get_bus_type_func		= ps5260_get_bus_type,
	.sensor_get_parameter_func		= ps5260_get_parameter,

	.sensor_set_power_on_func		= ps5260_set_power_on,
	.sensor_set_power_off_func		= ps5260_set_power_off,
	.sensor_set_fps_func			= ps5260_set_fps,
	.sensor_set_standby_in_func		= ps5260_set_standby_in,
	.sensor_set_standby_out_func	= ps5260_set_standby_out
};

AK_SENSOR_MODULE(ps5260_callback, ps5260)
///////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////

static int ps5260_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

static void ps5260_set_fps_async(void)
{
	if (to_fps != g_fps) {
		ps5260_sensor_write_register(0xEF, 0x01);
		ps5260_sensor_write_register(0x0a, to_fps_value>>8);
		ps5260_sensor_write_register(0x0b, to_fps_value & 0xff);

		ps5260_sensor_write_register(0x09, 0x01);
		g_fps = to_fps;
	}
}

static int ps5260_cmos_updata_a_gain(const unsigned int a_gain)
{
    unsigned int i;
    unsigned int gain_idx, sghd_patch = 1;	
    int gain = a_gain<<2;


    //printk(KERN_ERR"ps5260_cmos_updata_a_gain: a_gain = %d***************\n", a_gain);
	ps5260_set_fps_async();
	
	// Check gain
    if (gain > ps5260_gain_table[96])
        gain = ps5260_gain_table[96];
    else if(gain < ps5260_gain_table[0])
        gain = ps5260_gain_table[0];

    // search most suitable gain into gain table
    for (i=0; i<96; i++) {
        if (ps5260_gain_table[i]> gain)
            break;
    }
	
	// Check HS/LS mode
	if(ps5260_gain_table[i] > AG_HS_NODE)
	{
		if(g_sghd_flg == 1)	sghd_patch = 0;
		g_sghd_flg = 0;	// HS Mode
	}
	else if(ps5260_gain_table[i] < AG_LS_NODE)
	{
		if(g_sghd_flg == 0)	sghd_patch = 0;
		g_sghd_flg = 1;	// LS Mode	
	}
	
	// Calculate gain index
    gain_idx = (g_sghd_flg == 0)? (i-16):i;
	if(gain_idx < 5)
	{
		gain_idx = 5;

	}
	ps5260_sensor_write_register(0xEF, 0x01);//bank1
    ps5260_sensor_write_register(0x83, gain_idx);
    ps5260_sensor_write_register(0x18, g_sghd_flg);

    ps5260_sensor_write_register(0x09, 0x01);//update flag


	return 0;
}

static int ps5260_cmos_updata_exp_time(unsigned int exp_time)
{
	unsigned int ny, ne, ne_patch;	
	int lines = exp_time;
	
	//printk(KERN_ERR"ps5260_cmos_updata_exp_time: exp_time = %d***************\n", exp_time);

	ps5260_set_fps_async();

	g_total_lines = to_fps_value;	// LPF 

	g_lines = ((g_total_lines - 4) > lines)?((3 < lines)?lines:3):(g_total_lines - 4);	// 4 <= Ny <= LPF-3	
	ny = g_total_lines - g_lines;		// ne_patch update.
	ne_patch = NE_PATCH_LB + ((ny*NE_PATCH_SCALE)>>8);
	if (ne_patch > NE_PATCH_UB)	
		ne_patch = NE_PATCH_UB;	
	ne = g_sns_const1 - ne_patch;   
	
	//printk(KERN_ERR"ps5260_cmos_updata_exp_time: ny:%d,ne:%d ne_patch:%d ***************\n", ny,ne,ne_patch);
	
	ps5260_sensor_write_register(0xEF, 0x01);//bank1
	ps5260_sensor_write_register(0x0C, (ny>>8));	//Cmd_OffNy1[15:8] 
	ps5260_sensor_write_register(0x0D, ny&0xFF);	//Cmd_OffNy1[7:0] 
	ps5260_sensor_write_register(0x0E, (ne>>8));	//Cmd_OffNe1[11:8]
	ps5260_sensor_write_register(0x0F, ne&0xFF);	//Cmd_OffNe1[7:0]  
	ps5260_sensor_write_register(0x10, ((ne_patch&0x0100)>>6));	// 	
	ps5260_sensor_write_register(0x12, ne_patch&0xFF);	//
	
	ps5260_sensor_write_register(0x09, 0x01);//update flag 

    return EXP_EFFECT_FRAMES;
}

static int ps5260_cmos_update_a_gain_timer(void)
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
			ps5260_set_fps_async();
			stv.tv_sec = tv.tv_sec;
			start = 0;
		}
	}

	return 0;
}

