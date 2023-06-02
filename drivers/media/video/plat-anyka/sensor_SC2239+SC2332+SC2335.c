/**
 * @file camera_sc2239.c
 * @brief camera driver file
 * Copyright (C) 2017 Anyka (Guangzhou) Microelectronics Technology Co., Ltd
 * @author 
 * @date
 * @version 1.0
 * @ref
 *
 * 命名规则：
 * gain 标识 1X=256 单位的变量；
 * gaain 标识 1X=1000 单位的变量；
 *
 */

 
#include "ak_sensor_common.h"
#include <linux/delay.h>

#define SENSOR_PWDN_LEVEL		0
#define SENSOR_RESET_LEVEL		0
#define SENSOR_I2C_ADDR         0x60

#define SENSOR_SC2239_UCODE     0xcb10
#define SENSOR_SC2332_UCODE     0xcb17
#define SENSOR_SC2335_UCODE     0xcb14
#define SENSOR_SC2239_ID        0x2239 ///< 序列号适配
#define SENSOR_SC2335_ID        0x2335
#define SENSOR_SC2332_ID        0x2332
static  u32 _SENSOR_ID =        0; ///< 数值为 0 表示未检测到，当检测到某一款 SENSOR 时此值将固定为一个非 0 的值。

#define SENSOR_MCLK             24
#define SENSOR_REGADDR_BYTE     2
#define SENSOR_DATA_BYTE        1
#define MAX_FPS					30
#define SENSOR_OUTPUT_WIDTH		1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE         BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE     MIPI_INTERFACE
//#define SENSOR_IO_INTERFACE   DVP_INTERFACE
#define SENSOR_IO_LEVEL         SENSOR_IO_LEVEL_1V8

#define OV_MAX(a, b)            (((a) < (b) ) ?  (b) : (a))
#define OV_MIN(a, b)            (((a) > (b) ) ?  (b) : (a))
#define OV_CLIP3(low, high, x)  (OV_MAX(OV_MIN((x), high), low))

#define SKIP_NUM 		2
#define EXP_EFFECT_FRAMES 		1
#define VSYNC_ACTIVE_MS			28
#define MIPI_MHZ		780
#define MIPI_1LANE		1
#define MIPI_2LANE      2

#define PCLK_FREQ  (74250000)

static int _target_frame_vts = 0; ///< 缓存帧长设置，用于限制曝光与帧长大于曝光的逻辑。
static int _target_exp_ctrl = 0;
static int _reg_frame_hts = 0;

/**
 * 通过 I2C 总线读取一个数据。
 */
static int __i2c_read (int reg) {

	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = 0;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_read_register (&i2cdata);
}

/**
 * 通过 I2C 总线写入一个数据。
 */
static int __i2c_write (int reg, int data) {

	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr           = SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr          = reg;
	i2cdata.u32RegAddrByteNum   = SENSOR_REGADDR_BYTE;
	i2cdata.u32Data             = data;
	i2cdata.u32DataByteNum      = SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}


/**
 * 读取芯片唯一码。
 */
static int __read_ucode (void) {

	int const msb = __i2c_read (0x3107);
	int const lsb = __i2c_read (0x3108);
	int ucode = (msb << 8) | lsb;

	if (0xffff == ucode) {
		ucode = 0; ///< Force to Zero indicated Invalid.
	}

	return ucode;
}


/**
 * 获取曝光控制的值，注意，思特威读取是实际的 1/2。
 */
static int _reg_exp_time = 0;
//static int __get_exp_ctrl (void) {
//
//	if (0 == _reg_exp_time) {
//		/// 通过缓冲变量避免 I2C 频繁读取造成额外性能开销。
//		int const exp_time_ext = __i2c_read  (0x3e00) & 0xf;
//		int const exp_time_msb = __i2c_read  (0x3e01);
//		int const exp_time_lsb = (__i2c_read (0x3e02) & 0xf0) >> 4;
//		_reg_exp_time = exp_time_ext << 12 | exp_time_msb << 4 | exp_time_lsb;
//	}
//
//	return _reg_exp_time;
//}

/**
 * 设置半行曝光控制。
 * 如果上层传入曝光单位是行，传入前需要进行 X2 处理。
 */
static void __set_half_exp_ctrl (int exp) {

	int const exp_time_ext = (exp >> 12) & 0xf;
	int const exp_time_msb = (exp >> 4) & 0xff;
	int const exp_time_lsb = ((exp) & 0xf) << 4 ;

    __i2c_write (0x3e00, exp_time_ext);
	__i2c_write (0x3e01, exp_time_msb);
	__i2c_write (0x3e02, exp_time_lsb);

	_reg_exp_time = exp;
}

/**
 * 获取半帧宽，注意：由于 SENSOR 前后有兼容性差异，\n
 * 部分 SENSOR 该寄存器获取是半帧宽度，SC2239 读取是全帧宽度。\n
 * 为了兼容性这里统一获取半帧宽度。
 */
static int __get_frame_half_hts (void) {

	/// 该参数初始化导入配置以后不会再修改，因此可以通过变量缓存。

	if (0 == _reg_frame_hts) {
		_reg_frame_hts = __i2c_read (0x320c) << 8 | __i2c_read (0x320d);
		if (SENSOR_SC2239_ID == _SENSOR_ID || SENSOR_SC2332_ID == _SENSOR_ID) {
			/// SC2239 比较特别，从该寄存器读出来的值是一行的宽度，\n
			/// 这里为了适配较多以往驱动接口，统一按半行返回。
			_reg_frame_hts /= 2;
		}
	}

	return _reg_frame_hts;
}

/**
 * 获取全帧宽度快速定义。
 */
#define __get_frame_hts() (2 * __get_frame_half_hts())

/**
 * 获取帧高。
 */
static int _reg_frame_vts = 0; ///< 缓冲帧高参数，避免 I2C 频繁读取造成效率下降。
static int __get_frame_vts (void) {

	if (0 == _reg_frame_vts) {
		_reg_frame_vts = __i2c_read (0x320e) << 8 | __i2c_read (0x320f);
	}

	return _reg_frame_vts;
}

static void __set_frame_vts (int vts) {

	int const msb = vts >> 8;
	int const lsb = vts & 0xff;

	if (vts == _reg_frame_vts) {
		/// VTS 没有变化时不作更新处理。
		return;
	}

	__i2c_write (0x320e, msb);
	__i2c_write (0x320f, lsb);

	/// 回读设置。
	_reg_frame_vts = __i2c_read (0x320e) << 8 | __i2c_read (0x320f);
}


/**
 * 设置 ana gain，对应 [0x3e08,0x3e09] 寄存器。
 */
static int _reg_ana_gain = 0; ///< 寄存器缓冲。
static void __set_reg_ana_gain (int const gain) {

	if (gain == _reg_ana_gain) {
		/// gain 没有变化时不作更新处理。
		return;
	}

	__i2c_write (0x3e08, (gain >> 8) & 0xff);
	__i2c_write (0x3e09, gain & 0xff);
	/// 回读缓存寄存器的值，优化多次写入的内存开销。
	_reg_ana_gain = (__i2c_read (0x3e08) << 8) | __i2c_read (0x3e09);

	printk (KERN_DEBUG "Set Reg{3e08,3e09}=%04x.\r\n", _reg_ana_gain);
}


/**
 * 获取 ana gain，对应 [0x3e08,0x3e09] 寄存器。
 */
static __get_reg_ana_gain (void) {

	if (0 == _reg_ana_gain) {
		/// 初始化一次寄存器缓冲值。
		_reg_ana_gain = (__i2c_read (0x3e08) << 8) | __i2c_read (0x3e09);
	}

	/// 返回缓冲寄存器的值。
	return _reg_ana_gain;
}



/**
 * 同时更新帧长与曝光
 */
static void update_frame_vts_and_exp_ctrl (void) {

	int const cur_frame_vts = __get_frame_vts ();

	/// 将要设置的帧长与曝光需要匹配，帧率优先，曝光需要满足帧长前提。
	int const target_frame_vts = _target_frame_vts;
	int const max_exp_ctrl = target_frame_vts - 8; ///< 最大曝光不超过 VTS - 8。

	/// 注意，曝光控制单位是半行，因此设入的时候需要 X2。
	int const target_exp_ctrl = _target_exp_ctrl <= max_exp_ctrl ? _target_exp_ctrl : max_exp_ctrl;

	printk (KERN_DEBUG "VTS = %d->%d \r\n", cur_frame_vts, target_frame_vts);
	printk (KERN_DEBUG "EXP = %d/%d \r\n", target_exp_ctrl, _target_exp_ctrl);

	//printk(KERN_ERR "Active FrameHeight=%d  ExpCtrl=%d\r\n", target_frame_height, target_exp_ctrl);
	if (target_frame_vts >= cur_frame_vts) {
		/// 帧长比原来变长了，这个时候需要先设帧长，腾出空间设置曝光。
		__set_frame_vts (target_frame_vts);
		__set_half_exp_ctrl (target_exp_ctrl * 2);
	} else {
		/// 帧长比原来变短了，这个时候先设曝光，再设置帧率减小帧间隔。
		__set_half_exp_ctrl (target_exp_ctrl * 2);
		__set_frame_vts (target_frame_vts);
	}
}

static int sc2239_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	int i;
//	int value;
	AK_ISP_SENSOR_REG_INFO *preg_info;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
	   __i2c_write (preg_info->reg_addr, preg_info->value);
        preg_info++;
	}

	return 0;
}

/**
 * 匹配 UCODE 并对应 ID 转换。
 */
static int __probe_id (void) {

    int const ucode = __read_ucode ();

    /// 对应适配设备 ID。
	switch (ucode) {
		case SENSOR_SC2239_UCODE:
			_SENSOR_ID = SENSOR_SC2239_ID;
			break;

		case SENSOR_SC2335_UCODE:
			_SENSOR_ID = SENSOR_SC2335_ID;
			break;

        case SENSOR_SC2332_UCODE:
            _SENSOR_ID = SENSOR_SC2332_ID;
            break;
           
		default:
			break;
	}

    return _SENSOR_ID;
}


static int __read_id (void) {

	if (0 == _SENSOR_ID) {
		_SENSOR_ID = __probe_id ();
	}

	if (0 == _SENSOR_ID) {
		return 0xffff; ///< 返回一个非法 ID 使上层匹配错误。
	}

	return _SENSOR_ID;
}



static int sc2239_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int sc2239_get_mclk(void) {
	return SENSOR_MCLK;
}

static int __get_fps (void) {

	int const fps = PCLK_FREQ / (__get_frame_hts () * __get_frame_vts ());
	/// 读取实时的帧率。
	//printk ("<0> FPS=%d\r\n", fps);
	return fps;
}

static int sc2239_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type sc2239_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int sc2239_get_parameter(int param, void *value) {

	int ret = 0;
	enum sensor_get_param name = (enum sensor_get_param)param;

	switch (name) {

		case GET_VSYNC_ACTIVE_MS:
			*((int *)value) = VSYNC_ACTIVE_MS;
			break;

		case GET_CUR_FPS:
			*((int *)value) = __get_fps ();
			break;

		case GET_MIPI_MHZ:
			*((int *)value) = MIPI_MHZ;
			break;

		case GET_MIPI_LANE:
            if(_SENSOR_ID == SENSOR_SC2332_ID){
                *((int *)value) = MIPI_2LANE;
            }else{
                *((int *)value) = MIPI_1LANE;
            }    
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

static int sc2239_set_power_on(const int pwdn_pin, const int reset_pin)
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

static int sc2239_set_power_off(const int pwdn_pin, const int reset_pin)
{
	//sccb software standby mode
    
    ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
    ak_sensor_set_pin_dir(reset_pin, 0);

    return 0;
}

static int __set_fps (const int fps) {

	int vts = 0;
	/// FPS = PCLK / FRAMEW / FRAMEH
	vts = PCLK_FREQ;
	vts /= __get_frame_hts ();
	vts /= fps;

	_target_frame_vts = vts;
	update_frame_vts_and_exp_ctrl ();

	return 0;
}

static int sc2239_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc2239_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////


static int __sc2239_updata_d_gain(const unsigned int d_gain) {

	/// 这里传入的 dgain 精度位 1/256，统一转换成 1/1000 运算。

	int const max_dgain = 31500; ///< SC2239 最大 DGAIN = 31.5X

	unsigned char dig_gain = 0x00; ///< x1 dgain
	unsigned char dig_fine_gain = 0x80; ///< x1 dgain
	int x = 0, frac = 0, range = 0;
	int gain = d_gain * 1000 / 256; ///< 传进来1X=256，需要转换为1X=1000计量运算。


	if (gain > max_dgain) {
		/// 不超过最大 AGAIN
		gain = max_dgain;
	}

	x = gain / 1000; ///< 倍数整数。
	frac = gain % 1000; ///< 小数部分。
	range = 0xfc - 0x80;


	if (x < 1) {
		/// 最小为 1X again
		dig_gain = 0x00;
		dig_fine_gain = 0x80;

	} else if (x < 2) {
		/// 1 - 1.969
		dig_gain = 0x00;
		dig_fine_gain = (gain - 1000) * range / (1969 - 1000) + 0x80;

	} else if (x < 4) {
		/// 2 - 3.938
		dig_gain = 0x01;
		dig_fine_gain = (gain - 2000) * range / (3938 - 2000) + 0x80;

	} else if (x < 8) {
		/// 4 - 7.875
		dig_gain = 0x03;
		dig_fine_gain = (gain - 4000) * range / (7875 - 4000) + 0x80;

	} else if (x < 16) {
		/// 8 - 15.75
		dig_gain = 0x07;
		dig_fine_gain = (gain - 8000) * range / (15750 - 8000) + 0x80;

	} else if (x < 32) {
		/// 16 - 31.5
		dig_gain = 0x0f;
		dig_fine_gain = (gain - 16000) * range / (31500 - 16000) + 0x80;
	}

//	printk (KERN_ERR "Set SC2239 D-Gain %d.%03d {16'h3e06,16'h3e07} = {8'h%02x, 8'h%02x}.\r\n",
//			x, frac, dig_gain, dig_fine_gain);


	__i2c_write (0x3e06, dig_gain);
	__i2c_write (0x3e07, dig_fine_gain);

	return 0;
}


/**
 * 同步设置 SC2239、SC2332 的 A-Gain。
 *
 * @param[IN] gaain
 *  1X=1000 单位的增益值。
 *
 * @param[IN] max_gaain
 *  1X=1000 单位的最大增益值。
 *
 */
static void __sc2239_sc2332_set_a_gaain (int gaain, int const max_gaain) {

	//int const max_again = 15750; ///< SC2239 最大 AGAIN = 15.75X
	int const min_gaain = 1000; ///< 最小 1X 增益。
	int ana_gain = 0x03; ///< x1 again
	int ana_fine_gain = 0x20; ///< x1 again
	int reg_ana_gain = 0;
	int x = 0, frac = 0, range = 0;

	//printk(KERN_ERR "Set again = %d\r\n", a_gain);

	gaain = gaain > max_gaain ? max_gaain : gaain; ///< 不超过最大增益。
	gaain = gaain < min_gaain ? min_gaain : gaain; ///< 不低于最小增益。

	x = gaain / 1000; ///< 倍数整数。
	frac = gaain % 1000; ///< 小数部分，用于打印。
	range = 0x3f - 0x20;

	if (x < 1) {

		/// 最小为 1X again
		ana_gain = 0x03;
		ana_fine_gain = 0x20;

	} else if (x < 2) {
		/// 1 - 1.969
		ana_gain = 0x03;
		ana_fine_gain = (gaain - 1000) * range / (1969 - 1000) + 0x20;

	} else if (x < 4) {
		/// 2 - 3.938
		ana_gain = 0x07;
		ana_fine_gain = (gaain - 2000) * range / (3938 - 2000) + 0x20;

	} else if (x < 8) {
		/// 4 - 7.875
		ana_gain = 0x0f;
		ana_fine_gain = (gaain - 4000) * range / (7875 - 4000) + 0x20;

	} else {
		/// 8 - 15.75
		ana_gain = 0x1f;
		ana_fine_gain = (gaain - 8000) * range / (15750 - 8000) + 0x20;

	}

	/// 换算成寄存器的值。
	reg_ana_gain = ana_gain << 8 | ana_fine_gain;
	if (__get_reg_ana_gain () != reg_ana_gain) {
		/// 出现增益变更时输出一下当前增益。
		printk (KERN_DEBUG "Set A-Gagin=%d.%03dX/%d.%03dX.\r\n", x, frac, max_gaain / 1000, max_gaain % 1000);

	}

	/// 设置寄存器。
	__set_reg_ana_gain (ana_gain << 8 | ana_fine_gain);
}


/**
 * 缓存 SC2332 A-Gain。
 * 放大 1000 倍运算，如 15.75，该值为 15750
 * SC2332 最大增益会由于高温补偿变化，这里会缓存该数据用于作为下一次增益设置的范围参考。
 */
static int _sc2332_max_a_gaain = 15750; ///< 缓存当前 SC2332 最大 A-Gain。



/**
 * 设读取寄存器{0x3974}的值为 R；当前帧最大模拟增益用 MaxAgCur 表示，下一帧需要设置的最大模拟增益用 MaxAgNxt 表示。
 * 当 R>=0x1d时，MaxAgNxt = MaxAgCur – 16xfinegain(即最大模拟增益减 16 档模拟 finegain，finegain 为 1/32)，
 * 当 R <0x18时，MaxAgNxt = MaxAgCur + 16xfinegain(即最大模拟增益加 16 档模拟 finegain, finegain 为 1/32)
 * 备注：MaxAgNxt 最小限制到 1x（即 0x3e08[4:0]=0x03，0x3e09[7:0]=0x20），最大就是默认 15.75x
 * 即 0x3e08[4:0]=0x1f，0x3e09[7:0]=0x3f)。加减的 finegain step 可以适当调整。
 * 注意： MaxAgCur 只针对 sensor 的模拟增益，数字增益可以单独控制，无需限制。
 *
 * @return MaxAgNxt
 */
static int __timer_sc2332_calc_max_gaain_ctrl () {

	static int MaxAgCur = 15750; ///< SC2332 最大 AGAIN = 15.75X
	int MaxAgNxt = MaxAgCur;
	int const reg3974 = __i2c_read (0x3974); ///< 读取 3974 寄存器

	//printk(KERN_ERR "Set again = %d\r\n", a_gain);
	/// 根据高温逻辑修正最大 A-Gain
	/// 最大 Again 值得不大于 15.75X，不小于 2X。
    if (reg3974 >= 0x1c) {
    	MaxAgNxt = MaxAgCur - (1000 * 16 / 32);
    	MaxAgNxt = MaxAgNxt < 2000 ? 2000 : MaxAgNxt; ///< 最小增益 X2 限制。

    } else if (reg3974 < 0x18){
    	MaxAgNxt = MaxAgCur + (1000 * 16 / 32);
    	MaxAgNxt = MaxAgNxt > 15750 ? 15750 : MaxAgNxt; ///< 最大增益 X15.75 限制。

    } else{
    	/// 当处于 0x18 和 0x1d 中间时保持原来的最大增益。
    	MaxAgNxt = MaxAgCur;
    }

    if (MaxAgCur != MaxAgNxt) {
    	/// 增益最大值变更时输出一下结果。
		printk (KERN_DEBUG "Reg3974=%02x MaxAg=%d.%03dX->%d.%03dX.\r\n",
			reg3974, MaxAgCur / 1000, MaxAgCur % 1000, MaxAgNxt / 1000, MaxAgNxt % 1000);
    }

    /// 更新并返回当前最大增益。
    return (MaxAgCur = MaxAgNxt);
}



static int __on_timer (void) {

    _reg_frame_hts = __i2c_read (0x320c) << 8 | __i2c_read (0x320d);
	if (SENSOR_SC2239_ID == _SENSOR_ID || SENSOR_SC2332_ID == _SENSOR_ID) {
		/// SC2239 比较特别，从该寄存器读出来的值是一行的宽度，\n
		/// 这里为了适配较多以往驱动接口，统一按半行返回。
		_reg_frame_hts /= 2;
	}
        
	if (SENSOR_SC2332_ID == _SENSOR_ID) {

		/// 在定时器过程中同步更新 SC2332 缓存的 A-Gain。
		/// 并在定时器中同步更新。
		_sc2332_max_a_gaain = __timer_sc2332_calc_max_gaain_ctrl ();

	}

	return 0;
}



static int __sc2335_updata_a_gain (const unsigned int a_gain) {

	unsigned char dig_gain = 0x00;
	unsigned char dig_fine_gain = 0x80;
	unsigned char ana_gain = 0x03;
	unsigned char ana_fine_gain = 0x40;
	unsigned char reg_0x3040 = 0x00;
	unsigned int gainShift = 0;
	int gain = a_gain * 4;//1x=1024，a_gain 传进来的是 1X=256

	if (gain <= 16256)//calc again, dgain=1x
	{
		gain = gain >> 4;

		for (gainShift = 0; (gain >> gainShift) > 0; gainShift++)
		{
		}

		ana_fine_gain = gain >> (gainShift - 7);

		for (gainShift;gainShift > 7; gainShift--)
		{
			ana_gain = (ana_gain << 1) | 0x01;
		}
	}
	else
	{
		ana_gain = 0x1f;
		ana_fine_gain = 0x7f;

		gain = gain / 127;	//gain/15.75>>3
		for (gainShift = 0; (gain >> gainShift) > 0; gainShift++)
		{
		}

		dig_fine_gain = gain >> (gainShift - 8);

		for (gainShift; gainShift > 8; gainShift--)
		{
			dig_gain = (dig_gain << 1) | 0x01;
		}
	}

	//printf("0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
	//	dig_gain, dig_fine_gain, ana_gain, ana_fine_gain);


    __i2c_write  (0x3e06, dig_gain);
    __i2c_write  (0x3e07, dig_fine_gain);
    __i2c_write  (0x3e08, ana_gain);
    __i2c_write  (0x3e09, ana_fine_gain);

    return 0;
}


static int __updata_a_gain (const unsigned int gain) {

	/// 这里传入的 again 精度位 1/256，统一转换成 1/1000 运算。
	int const gaain = gain * 1000 / 256;

	if (SENSOR_SC2239_ID == _SENSOR_ID) {

		u8 reg3634 = 0x24; ///< 对应 AGAIN 的整数关系对寄存器 3634 联动操作。

		__sc2239_sc2332_set_a_gaain (gaain, 15750); ///< SC2239 最大 AGAIN = 15.75X

		/// 寄存器 3634 对应 2X，4X，8X 的值。
		if (gaain < 1000) {
			reg3634 = 0x64;
		} else if (gaain < 2000) {
			reg3634 = 0x64;
		} else if (gaain < 4000) {
			reg3634 = 0x64;
		} else if (gaain < 8000) {
			reg3634 = 0x44;
		} else {
			reg3634 = 0x24;
		}

		printk (KERN_DEBUG "Reg3634=%02x.\r\n", reg3634);
		__i2c_write (0x3634, reg3634);

	} else if (SENSOR_SC2332_ID == _SENSOR_ID) {

		/// 由于 SC2332 需要一直观察寄存器 3974，需要通过异步设置，
		__sc2239_sc2332_set_a_gaain (gaain, _sc2332_max_a_gaain);

    } else if (SENSOR_SC2335_ID == _SENSOR_ID) {

		return __sc2335_updata_a_gain (gain);
	}

	return 0;
}

static int __updata_d_gain(const unsigned int a_gain) {

	if (SENSOR_SC2239_ID == _SENSOR_ID || SENSOR_SC2332_ID == _SENSOR_ID) {
		return __sc2239_updata_d_gain (a_gain);
	} else if (SENSOR_SC2335_ID == _SENSOR_ID) {

	}

	return 0;
}



static int __update_exp(unsigned int exp_time) {

	_target_exp_ctrl = exp_time;
	update_frame_vts_and_exp_ctrl ();

	/// 思特威 SENSOR 特点 N+2 生效。
    return 2;
}




static AK_ISP_SENSOR_CB _callback = {

	.sensor_init_func 				= sc2239_init,
	.sensor_read_reg_func			= __i2c_read ,
	.sensor_write_reg_func			= __i2c_write ,
	.sensor_read_id_func			= __read_id,
	.sensor_update_a_gain_func		= __updata_a_gain,
	.sensor_update_d_gain_func		= __updata_d_gain,
	.sensor_updata_exp_time_func	= __update_exp,
	.sensor_timer_func				= __on_timer,

	.sensor_probe_id_func			= __probe_id,	//use IIC bus
	.sensor_get_resolution_func		= sc2239_get_resolution,
	.sensor_get_mclk_func			= sc2239_get_mclk,
	.sensor_get_fps_func			= __get_fps,
	.sensor_get_valid_coordinate_func	= sc2239_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc2239_get_bus_type,
	.sensor_get_parameter_func		= sc2239_get_parameter,

	.sensor_set_power_on_func		= sc2239_set_power_on,
	.sensor_set_power_off_func		= sc2239_set_power_off,
	.sensor_set_fps_func			= __set_fps,
	.sensor_set_standby_in_func		= sc2239_set_standby_in,
	.sensor_set_standby_out_func	= sc2239_set_standby_out
};

AK_SENSOR_MODULE(_callback, sc2xxx)


