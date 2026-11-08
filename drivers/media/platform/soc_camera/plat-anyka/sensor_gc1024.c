/*
 * GalaxyCore GC1024 (DVP, 1280x720) on the ISP2 sensor framework.
 *
 * Ported from the behaviour of the vendor module sensor_gc1024.ko for
 * AK3918EV200 (Linux 3.4.35, no source available), recovered from its
 * disassembly: same I2C address, chip ID check, power sequence, gain
 * steps, exposure limits and per-fps vertical blanking. The register init
 * list comes from the ISP tuning file (block 0x1c of isp_gc1024.conf), as
 * for the other sensors here.
 *
 * The vendor module used the older control-based interface, so MCLK, fps,
 * resolution and the power sequence come from its g_ctl/s_ctl handlers.
 * It also had a mirror/flip control (P0:0x17 bit 0 mirror, bit 1 flip,
 * base 0x14, written with 0xf2 = 0x00 around it); the ISP2 callback table
 * has no slot for it, so it is not carried over.
 */
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	1
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x78
#define SENSOR_ID		0x1004	/* registers 0xf0:0xf1 */
#define SENSOR_MCLK		24
#define SENSOR_REGADDR_BYTE	1
#define SENSOR_DATA_BYTE	1
#define SENSOR_BASE_FPS		25	/* frame timing of the init list */
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

/* The vendor module returns 0 from the gain and exposure updates. */
#define EFFECT_FRAMES		0
/*
 * Not reported by the vendor module: estimated from the 728-row window
 * of a ~925-line frame at 25 fps.
 */
#define VSYNC_ACTIVE_MS		31
/* Not reported by the vendor module either: the camera driver default. */
#define SENSOR_IO_LEVEL		SENSOR_IO_LEVEL_1V8

#define SENSOR_MAX_EXP		0x1fff	/* 0x03[4:0]:0x04[7:0] */

static int g_fps = SENSOR_BASE_FPS;

static int gc1024_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int gc1024_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int gc1024_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		gc1024_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);

	/* Pad drive / output enable, written by the vendor after the list. */
	gc1024_sensor_write_register(0x24, 0xff);

	return 0;
}

static int gc1024_read_id(void)
{
	/*
	 * The vendor read_id read the chip ID like probe_id below; here it
	 * must be a constant, since probing compares the two and the tuning
	 * file is checked against it.
	 */
	return SENSOR_ID;
}

static int gc1024_probe_id(void)
{
	int id;

	id = (gc1024_sensor_read_register(0xf0) & 0xff) << 8;
	id |= gc1024_sensor_read_register(0xf1) & 0xff;

	return id == SENSOR_ID ? SENSOR_ID : 0;
}

static int gc1024_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int gc1024_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int gc1024_get_fps(void)
{
	return g_fps;
}

static int gc1024_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type gc1024_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int gc1024_get_parameter(int param, void *value)
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
	case GET_SENSOR_IO_LEVEL:
		*(int *)value = SENSOR_IO_LEVEL;
		return 0;
	default:
		return -1;
	}
}

static int gc1024_set_power_on(const int pwdn_pin, const int reset_pin)
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
	return 0;
}

static int gc1024_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

/*
 * Only the vertical blanking (0x07:0x08) changes; line length and window
 * stay as the init list set them. 0x00b5 is the init list's 25 fps value.
 */
static int gc1024_set_fps(const int fps)
{
	int vb;

	switch (fps) {
	case 8:
		vb = 0x0865;
		break;
	case 10:
		vb = 0x0622;
		break;
	case 12:
	case 13:
		vb = 0x0453;
		break;
	case 25:
		vb = 0x00b5;
		break;
	case 30:
		vb = 0x0022;
		break;
	default:
		pr_err("%s: set fps %d fail\n", __func__, fps);
		return -EINVAL;
	}

	gc1024_sensor_write_register(0x07, vb >> 8);
	gc1024_sensor_write_register(0x08, vb & 0xff);
	g_fps = fps;
	return 0;
}

static int gc1024_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1024_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1024_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/*
 * Analogue gain steps (P0:0xb6, index into this table) and their gain in
 * 1/64 units; the rest is made up with the global gain 0xb1[3:0].0xb2[7:2]
 * (1/64 steps). Each step is used from its own gain up to the next one.
 */
static const unsigned int gc1024_gain_steps[] = {
	64, 105, 119, 206, 235, 398, 465, 775, 1149, 1590, 2339,
};

/* gain is the total gain in 1/64 units. */
static void gc1024_set_gain(unsigned int gain)
{
	unsigned int global;
	int step;

	gc1024_sensor_write_register(0xb1, 0x01);
	gc1024_sensor_write_register(0xb2, 0x00);
	if (gain < 64)
		return;

	for (step = ARRAY_SIZE(gc1024_gain_steps) - 1;
	     gain < gc1024_gain_steps[step]; step--)
		;

	global = (gain * 64) / gc1024_gain_steps[step];
	gc1024_sensor_write_register(0xb6, step);
	/* Below the top step the global gain stays under 2x. */
	if (step == ARRAY_SIZE(gc1024_gain_steps) - 1)
		gc1024_sensor_write_register(0xb1, (global & 0xffff) >> 6);
	else
		gc1024_sensor_write_register(0xb1, 0x01);
	gc1024_sensor_write_register(0xb2, (global << 2) & 0xfc);
}

/* a_gain is 8.8 fixed point; the vendor keeps 16 bits of a_gain >> 2. */
static int gc1024_cmos_updata_a_gain(const unsigned int a_gain)
{
	gc1024_set_gain((a_gain >> 2) & 0xffff);
	return EFFECT_FRAMES;
}

/* Exposure in lines, 0x03[4:0]:0x04[7:0]; no frame length coupling. */
static int gc1024_cmos_updata_exp_time(unsigned int exp_time)
{
	if (!exp_time)
		exp_time = 1;
	else if (exp_time > SENSOR_MAX_EXP)
		exp_time = SENSOR_MAX_EXP;

	gc1024_sensor_write_register(0x04, exp_time & 0xff);
	gc1024_sensor_write_register(0x03, (exp_time >> 8) & 0x1f);
	return EFFECT_FRAMES;
}

static int gc1024_cmos_update_a_gain_timer(void)
{
	return 0;
}

static AK_ISP_SENSOR_CB gc1024_callback = {
	.sensor_init_func			= gc1024_init,
	.sensor_read_reg_func			= gc1024_sensor_read_register,
	.sensor_write_reg_func			= gc1024_sensor_write_register,
	.sensor_read_id_func			= gc1024_read_id,
	.sensor_update_a_gain_func		= gc1024_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= gc1024_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= gc1024_cmos_updata_exp_time,
	.sensor_timer_func			= gc1024_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= gc1024_probe_id,
	.sensor_get_resolution_func		= gc1024_get_resolution,
	.sensor_get_mclk_func			= gc1024_get_mclk,
	.sensor_get_fps_func			= gc1024_get_fps,
	.sensor_get_valid_coordinate_func	= gc1024_get_valid_coordinate,
	.sensor_get_bus_type_func		= gc1024_get_bus_type,
	.sensor_get_parameter_func		= gc1024_get_parameter,
	.sensor_set_power_on_func		= gc1024_set_power_on,
	.sensor_set_power_off_func		= gc1024_set_power_off,
	.sensor_set_fps_func			= gc1024_set_fps,
	.sensor_set_standby_in_func		= gc1024_set_standby_in,
	.sensor_set_standby_out_func		= gc1024_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(gc1024_callback, gc1004)
