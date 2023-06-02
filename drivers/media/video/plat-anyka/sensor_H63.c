/**
 * @file camera_h63.c
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
#define SENSOR_I2C_ADDR         0x80 
#define SENSOR_ID               0x0a63
#define SENSOR_MCLK             27
#define SENSOR_REGADDR_BYTE     1
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW

#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			31
#define MIPI_MHZ		216
#define MIPI_LANE		1
#define EFFECT_FRAMES 	1


#define PCLK_FREQ  (43200000)  

static int _target_frame_height = 0;
static int _target_exp_ctrl = 0;
static int frame_width = 0;

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

static int h63_cmos_updata_d_gain(const unsigned int d_gain);
static int h63_cmos_updata_a_gain(const unsigned int a_gain);
static int h63_cmos_updata_exp_time(unsigned int exp_time);


static int __i2c_read (int reg) {

	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int __i2c_write (int reg, int data) {

	struct ak_sensor_i2c_data i2cdata;
//	ak_sensor_print("%s~~~~~~~~~~reg:0x%x,data:0x%x\n",__func__,reg,data);
	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

/**
 * 获取曝光控制的值。
 */
static int get_exp_ctrl (void) 
{

	int const reg_exp_ctrl_15_8 = __i2c_read (0x02);
	int const reg_exp_ctrl_7_0 = __i2c_read (0x01);

	return (reg_exp_ctrl_15_8 << 8) | reg_exp_ctrl_7_0;
}

static void set_exp_ctrl (int v) {

	//printk(KERN_ERR "Active Exp Ctrl = %d\r\n",v);
	__i2c_write (0x02, v >> 8);
	__i2c_write (0x01, v  & 0xff);
}

/**
 * 从寄存器读出是半帧的宽度。
 */
static int __get_frame_half_width () {

	if (0 == frame_width) {
		/// 这里重复读取 I2C 会导致 I2C 超时，
		/// 通过缓冲见

		int const reg_frame_w_15_8 = __i2c_read (0x21);
		int const reg_frame_w_7_0 = __i2c_read (0x20);
		frame_width =  (reg_frame_w_15_8 << 8) | reg_frame_w_7_0;
	}

//	printk(KERN_ERR "Get Frame Width = %d\r\n", frame_width);
	return frame_width;
        
}

/**
 * 适配运算 FPS 时需要转换成全帧的宽度。
 */
#define __get_frame_width() (2 * __get_frame_half_width ())


static int _cache_height = 0;
static int __get_frame_height () {

	if (0 == _cache_height){
		_cache_height = (__i2c_read (0x23) << 8) | __i2c_read (0x22);
	}
	
	return _cache_height;
}

static void __set_frame_height (int const frameh) {

	if (frameh != _cache_height) {
		__i2c_write (0x23, frameh >> 8);
		__i2c_write (0x22, frameh  & 0xff);
		_cache_height = frameh;
	}
}

/**
 * 同时更新帧长与曝光
 */
static void update_frame_height_and_exp_ctrl ()  {

	int const cur_frame_height = __get_frame_height ();

	/// 将要设置的帧长与曝光需要匹配，帧率优先，曝光需要满足帧长前提。
	int const target_frame_height = _target_frame_height;
	int const target_exp_ctrl = _target_exp_ctrl <= target_frame_height - 1 ? _target_exp_ctrl : target_frame_height - 1;

	//printk(KERN_ERR "Active FrameHeight=%d  ExpCtrl=%d\r\n", target_frame_height, target_exp_ctrl);
	if (target_frame_height >= cur_frame_height) {
		/// 帧长比原来变长了，这个时候需要先设帧长，腾出空间设置曝光。
		__set_frame_height (target_frame_height);
		set_exp_ctrl (target_exp_ctrl);
	} else {
		/// 帧长比原来变短了，这个时候先设曝光，再设置帧率减小帧间隔。
		set_exp_ctrl (target_exp_ctrl);
		__set_frame_height (target_frame_height);
	}
}


static int h63_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
	AK_ISP_SENSOR_REG_INFO *preg_info;
int value;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
//value=h63_sensor_read_register(preg_info->reg_addr);
//ak_sensor_print("before w reg:0x%.x, val:0x%.x, to write:0x%.x\n", preg_info->reg_addr, value, preg_info->value);
		//ak_sensor_print("before w reg:0x%.x,to write:0x%.x\n", preg_info->reg_addr, preg_info->value);

		if (preg_info->reg_addr == 0xffff)
			ak_sensor_mdelay(preg_info->value);
		else
			__i2c_write(preg_info->reg_addr, preg_info->value);
		//value=__i2c_read(preg_info->reg_addr);
		//ak_sensor_print("after w reg:0x%.x, val:0x%.x\n", preg_info->reg_addr, value);
		preg_info++;
	}

	/// 初始化帧长与曝光的初始化值。
	_target_frame_height = __get_frame_height ();
	_target_exp_ctrl = get_exp_ctrl ();
	
	return 0;
}

static int h63_read_id(void)
{
	return SENSOR_ID;
}

static int h63_probe_id(void)
{
    u8 value;
    u32 id;

    value = __i2c_read(0x0a);
    id = value << 8;
    value = __i2c_read(0x0b);
    id |= value;    

	if (id == SENSOR_ID)
		return SENSOR_ID;

    return 0;
}

static int h63_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int h63_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int h63_get_fps(void)
{
	int fps = PCLK_FREQ;

	fps /= __get_frame_width ();
	fps /= __get_frame_height ();
	
	return fps;
}

static int h63_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type h63_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int h63_get_parameter(int param, void *value)
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
			*((int *)value) = h63_get_fps ();
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
static int h63_set_power_on(const int pwdn_pin, const int reset_pin)
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

	ak_sensor_print("%s\n",__func__);
	return 0;
}

static int h63_set_power_off(const int pwdn_pin, const int reset_pin)
{
#if 0
    u8 Reg0x3d = 0x48;
    u8 Reg0xc3 = 0x00;
#endif

#if 0
	//sccb software standby mode
    
    __i2c_write(0x3d, Reg0x3d);
    __i2c_write(0xc3, Reg0xc3);
#endif

    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int h63_set_fps(const int fps)
{
	int target_frame_height = 0;


	/// FPS = PCLK / FRAMEW / FRAMEH
	/// 计算出将要设置的帧长度。
	target_frame_height = PCLK_FREQ / __get_frame_width ();
	target_frame_height /= fps;

	/// 这里只是简单登记当前需要设置的帧率，
	/// 由于出现帧率与曝光不匹配会挂掉的问题，
	/// 这里不能直接就去设置帧率生效，等到曝光设置时一起设置。
//	_setup_frame_ps = fps;
	_target_frame_height = target_frame_height;

	printk(KERN_ERR "Set Frame Height = %d\r\n", _target_frame_height);
	update_frame_height_and_exp_ctrl ();

	return 0;
}

static int h63_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int h63_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int __on_timer(void) {

	int const reg_95 = __i2c_read(0x95);
    int const reg_frame_w_15_8 = __i2c_read (0x21);
	int const reg_frame_w_7_0 = __i2c_read (0x20);
	frame_width =  (reg_frame_w_15_8 << 8) | reg_frame_w_7_0;
	
    if (reg_95 > 0x80) {
		__i2c_write(0x8e,0x01);
		printk("write 0x8e 0x01\n");
	}else{
		__i2c_write(0x8e,0x00);
		printk("write 0x8e 0x00\n");
	}
}


static AK_ISP_SENSOR_CB h63_callback = 
{
	.sensor_init_func 				= h63_init,
	.sensor_read_reg_func			= __i2c_read,
	.sensor_write_reg_func			= __i2c_write,
	.sensor_read_id_func			= h63_read_id,
	.sensor_update_a_gain_func		= h63_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= h63_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= h63_cmos_updata_exp_time,
    .sensor_timer_func				= __on_timer,
    
	.sensor_probe_id_func			= h63_probe_id,	//use IIC bus
	.sensor_get_resolution_func		= h63_get_resolution,
	.sensor_get_mclk_func			= h63_get_mclk,
	.sensor_get_fps_func			= h63_get_fps,
	.sensor_get_valid_coordinate_func	= h63_get_valid_coordinate,
	.sensor_get_bus_type_func		= h63_get_bus_type,
    .sensor_get_parameter_func		= h63_get_parameter,                                                                       

	.sensor_set_power_on_func		= h63_set_power_on,
	.sensor_set_power_off_func		= h63_set_power_off,
	.sensor_set_fps_func			= h63_set_fps,
	.sensor_set_standby_in_func		= h63_set_standby_in,
	.sensor_set_standby_out_func	= h63_set_standby_out
};

AK_SENSOR_MODULE(h63_callback, h63)

///////////////////////////////////////////////////////////////////////////////////////////

static unsigned int h63_cmos_gains_convert(unsigned int a_gain, unsigned int d_gain,
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

static int h63_cmos_updata_d_gain(const unsigned int d_gain)
{	
	return 0;
	//?????????????Ļص?????
	printk("================33================\n\n");
	printk("d_gain : %d\n",d_gain);	
	printk("==================================\n");
	
}

static int h63_cmos_updata_a_gain(const unsigned int a_gain)
{
//	return 0;
	//??????ģ???????Ļص?????
	unsigned short ag_value;
	unsigned int tmp_a_gain;
	unsigned int tmp_d_gain;
	unsigned int tmp_a_gain_out;
	unsigned int tmp_d_gain_out;

	tmp_a_gain = a_gain>>4;
	tmp_d_gain = 0;

	//ag_value = cmos_gains_update(isp,isp->aec_param.current_a_gain);
	h63_cmos_gains_convert(tmp_a_gain, tmp_d_gain,  &tmp_a_gain_out ,&tmp_d_gain_out);
	ag_value = tmp_a_gain_out;
	//ak_sensor_print("a_gain=%d\n",a_gain);
	__i2c_write(0x00, tmp_a_gain_out);

	printk("================33================\n\n");
	printk("tmp_a_gain : %d\n",tmp_a_gain);	
	printk("tmp_a_gain_out : %d\n",tmp_a_gain_out);
	printk("----------------------------------\n");
	printk("tmp_a_gain_out : 0x%x\n",tmp_a_gain_out);
	printk("==================================\n");
	// updata_auto_exposure_num++;
	//isp->aec_param.current_exposure_time = tmp_exposure_time;
	return EFFECT_FRAMES;
}


static int h63_cmos_updata_exp_time(unsigned int exp_time)
{
//	return 0;
    //?????ع?ʱ???Ļص?????
//  unsigned char exposure_time_msb;
//	unsigned char exposure_time_lsb;
//	ak_sensor_print("exp_time=%d\n",exp_time);
//  exposure_time_msb =(exp_time>>8)&0xff;
//  exposure_time_lsb = exp_time&0xff;
//	ak_sensor_print("msb = %d\n",exposure_time_msb);
//	ak_sensor_print("lsb = %d\n",exposure_time_lsb);
//	h63_sensor_write_register(0x02,exposure_time_msb);
//	h63_sensor_write_register(0x01,exposure_time_lsb);

	_target_exp_ctrl = exp_time;
	update_frame_height_and_exp_ctrl ();
	
	printk("================33================\n\n");
	printk("exp_time : 0x%x\n",exp_time);	
	printk("==================================\n");
//	ak_sensor_print("0x16=%d\n",h63_sensor_read_register(0x16));
    return 0;
}
