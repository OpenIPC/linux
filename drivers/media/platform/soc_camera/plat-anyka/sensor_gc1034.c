/*
 * GalaxyCore GC1034 (DVP, 1280x720 @ 25 fps) on the ISP2 sensor framework.
 *
 * Ported from the disassembled vendor module sensor_gc1034.ko shipped on
 * AK3918EV200 V380 cameras (Linux 3.4.35, no source available): same I2C
 * address, chip ID check, power sequence, gain table, exposure limits and
 * deferred frame-rate switching. The register init list comes from the
 * ISP tuning file (block 0x1c of isp_gc1034_0.conf); it leaves the sensor
 * on register page 0, which the exposure and VB writes below rely on.
 */
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	1	/* PWDN high = powered down */
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x42
#define SENSOR_ID		0x1034
#define SENSOR_CHIP_ID		0x1034	/* registers 0xf0:0xf1 */
#define SENSOR_MCLK		27
#define SENSOR_REGADDR_BYTE	1
#define SENSOR_DATA_BYTE	1
#define SENSOR_BASE_FPS		25	/* frame timing of the init list */
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

#define EFFECT_FRAMES		1
#define VSYNC_ACTIVE_MS		36
#define SENSOR_MAX_EXP		0x1fff	/* 13-bit shutter, 0x03[4:0]:0x04 */

/* Page 0 registers. */
#define REG_PAGE_SELECT		0xfe
#define REG_EXP_H		0x03
#define REG_EXP_L		0x04
#define REG_VB_H		0x07
#define REG_VB_L		0x08
#define REG_PAD_DRV		0x24

/* Page 1 gain registers. */
#define REG_ANALOG_GAIN		0xb6	/* analogue gain step 0..10 */
#define REG_PREGAIN_H		0xb1	/* pre-gain, 1.0 = 0x40 */
#define REG_PREGAIN_L		0xb2

static int g_fps = SENSOR_BASE_FPS;	/* frame rate currently programmed */
static int to_fps = SENSOR_BASE_FPS;	/* frame rate asked for by set_fps */
static int to_fps_value;		/* vertical blanking for to_fps */

static int gc1034_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int gc1034_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

/*
 * The frame rate is changed by the next gain or exposure update rather
 * than by set_fps itself, as in the vendor driver.
 */
static void gc1034_set_fps_async(void)
{
	if (to_fps == g_fps)
		return;
	gc1034_sensor_write_register(REG_VB_H, to_fps_value >> 8);
	gc1034_sensor_write_register(REG_VB_L, to_fps_value & 0xff);
	g_fps = to_fps;
}

static int gc1034_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		gc1034_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);

	/* Written by the vendor driver after every init list. */
	gc1034_sensor_write_register(REG_PAD_DRV, 0xff);

	g_fps = SENSOR_BASE_FPS;
	to_fps = SENSOR_BASE_FPS;

	return 0;
}

static int gc1034_read_id(void)
{
	return SENSOR_ID;
}

static int gc1034_probe_id(void)
{
	int id;

	id = (gc1034_sensor_read_register(0xf0) & 0xff) << 8;
	id |= gc1034_sensor_read_register(0xf1) & 0xff;

	return id == SENSOR_CHIP_ID ? SENSOR_ID : 0;
}

static int gc1034_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int gc1034_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int gc1034_get_fps(void)
{
	return g_fps;
}

static int gc1034_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type gc1034_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

/*
 * The vendor module answers only VSYNC_ACTIVE_MS and CUR_FPS. The GC1034
 * is a DVP-only part, so the interface is reported as DVP as well (that is
 * also what the camera host assumes when it gets no answer). The IO level
 * is left unanswered, as in the vendor module.
 */
static int gc1034_get_parameter(int param, void *value)
{
	switch ((enum sensor_get_param)param) {
	case GET_VSYNC_ACTIVE_MS:
		*(int *)value = VSYNC_ACTIVE_MS;
		return 0;
	case GET_CUR_FPS:
		*(int *)value = g_fps;
		return 0;
	case GET_INTERFACE:
		*(int *)value = DVP_INTERFACE;
		return 0;
	default:
		return -1;
	}
}

static int gc1034_set_power_on(const int pwdn_pin, const int reset_pin)
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

	g_fps = SENSOR_BASE_FPS;
	pr_debug("%s\n", __func__);
	return 0;
}

static int gc1034_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

/*
 * Frame length = window height 724 (0x0d:0x0e) + 16 + VB (0x07:0x08);
 * VB 10 gives 750 lines at 25 fps. Only the vendor's fixed rates are
 * accepted; 12 and 13 fps share the 12.5 fps blanking.
 */
static int gc1034_set_fps(const int fps)
{
	int vb;

	switch (fps) {
	case 25:
		vb = 10;
		break;
	case 12:
	case 13:
		vb = 760;
		break;
	case 10:
		vb = 1136;
		break;
	case 8:
		vb = 1605;
		break;
	default:
		pr_err("%s set fps fail\n", __func__);
		return -EINVAL;
	}

	to_fps = fps;
	to_fps_value = vb;
	return 0;
}

static int gc1034_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1034_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1034_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/*
 * gain is in 1/64 steps (0x40 = 1x). Each analogue step has a nominal
 * gain (x64) from the table below; the remainder goes into the pre-gain
 * 0xb1 (integer part) : 0xb2[7:2], also 0x40 = 1x. Below 1x only the pre-gain is reset.
 */
static void gc1034_setgain(u16 gain)
{
	static const u16 analog_gain[] = {
		64,	/* 1.00x */
		91,	/* 1.42x */
		127,	/* 1.99x */
		182,	/* 2.85x */
		258,	/* 4.03x */
		369,	/* 5.77x */
		516,	/* 8.06x */
		738,	/* 11.53x */
		1051,	/* 16.42x */
		1485,	/* 23.20x */
		2084,	/* 32.56x */
	};
	u16 pregain;
	int step;

	gc1034_sensor_write_register(REG_PAGE_SELECT, 0x01);
	gc1034_sensor_write_register(REG_PREGAIN_H, 0x01);
	gc1034_sensor_write_register(REG_PREGAIN_L, 0x00);

	if (gain >= analog_gain[0]) {
		for (step = ARRAY_SIZE(analog_gain) - 1;
		     gain < analog_gain[step]; step--)
			;
		pregain = 64 * gain / analog_gain[step];

		gc1034_sensor_write_register(REG_ANALOG_GAIN, step);
		gc1034_sensor_write_register(REG_PREGAIN_H, pregain >> 6);
		gc1034_sensor_write_register(REG_PREGAIN_L,
					     (pregain << 2) & 0xfc);
	}

	gc1034_sensor_write_register(REG_PAGE_SELECT, 0x00);
}

/* a_gain is 8.8 fixed point. */
static int gc1034_cmos_updata_a_gain(const unsigned int a_gain)
{
	gc1034_set_fps_async();
	gc1034_setgain((u16)(a_gain >> 2));
	return 0;
}

static int gc1034_cmos_updata_exp_time(unsigned int exp_time)
{
	gc1034_set_fps_async();

	if (!exp_time)
		exp_time = 1;
	else if (exp_time > SENSOR_MAX_EXP)
		exp_time = SENSOR_MAX_EXP;

	gc1034_sensor_write_register(REG_EXP_L, exp_time & 0xff);
	gc1034_sensor_write_register(REG_EXP_H, exp_time >> 8);

	return EFFECT_FRAMES;
}

/* The vendor module has no timer callback; the ISP skips a NULL one. */
static AK_ISP_SENSOR_CB gc1034_callback = {
	.sensor_init_func			= gc1034_init,
	.sensor_read_reg_func			= gc1034_sensor_read_register,
	.sensor_write_reg_func			= gc1034_sensor_write_register,
	.sensor_read_id_func			= gc1034_read_id,
	.sensor_update_a_gain_func		= gc1034_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= gc1034_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= gc1034_cmos_updata_exp_time,
	.sensor_probe_id_func			= gc1034_probe_id,
	.sensor_get_resolution_func		= gc1034_get_resolution,
	.sensor_get_mclk_func			= gc1034_get_mclk,
	.sensor_get_fps_func			= gc1034_get_fps,
	.sensor_get_valid_coordinate_func	= gc1034_get_valid_coordinate,
	.sensor_get_bus_type_func		= gc1034_get_bus_type,
	.sensor_get_parameter_func		= gc1034_get_parameter,
	.sensor_set_power_on_func		= gc1034_set_power_on,
	.sensor_set_power_off_func		= gc1034_set_power_off,
	.sensor_set_fps_func			= gc1034_set_fps,
	.sensor_set_standby_in_func		= gc1034_set_standby_in,
	.sensor_set_standby_out_func		= gc1034_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(gc1034_callback, gc1034)
