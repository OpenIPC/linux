/*
 * SmartSens SC2232 (chip ID 0x2232; DVP, 1920x1080) on the ISP2 sensor
 * framework. Not the SC2232H (ID 0xcb07..), which sensor_sc2232h_sc2135e.c
 * handles.
 *
 * Ported from the disassembled vendor module sensor_sc2232.ko shipped on
 * V380 AK3918EV200 cameras (Linux 3.4.35); no source is available. Same
 * chip ID check, power sequence, gain/exposure maths, per-gain analogue
 * and DPC settings, and deferred frame length (VTS) switching. The register
 * init list comes from the ISP tuning file (block 0x1c of isp_sc2232.conf)
 * and ends with streaming on.
 */
#include <linux/ktime.h>
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	0
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x60
#define SENSOR_ID		0x2232	/* registers 0x3107:0x3108 */
#define SENSOR_MCLK		24
#define SENSOR_REGADDR_BYTE	2
#define SENSOR_DATA_BYTE	1
#define SENSOR_OUTPUT_WIDTH	1920
#define SENSOR_OUTPUT_HEIGHT	1080
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

/* The init list runs HTS 2200 x VTS 1500 at 15 fps. */
#define SENSOR_BASE_FPS		15
#define VSYNC_ACTIVE_MS		36
#define EXP_EFFECT_FRAMES	1

/* Seconds a pending frame rate change waits for an AE update. */
#define FPS_TIMER_SEC		2

static struct {
	int fps;		/* frame rate programmed into the sensor */
	int to_fps;		/* frame rate requested by set_fps */
	int to_vts;		/* frame length for to_fps */
	int r3e00;		/* exposure registers last written */
	int r3e01;
	int r3e02;
	int again_level;	/* coarse gain octave, 0..3 */
	int coarse_gain;	/* 0x3e08[4:2] last written, -1: unknown */
	int again_10x;		/* DPC set for gain >= 10x, -1: unknown */
	bool fps_timer_run;
	time64_t fps_timer_sec;
} sc2232 = {
	.fps		= SENSOR_BASE_FPS,
	.to_fps		= SENSOR_BASE_FPS,
	.coarse_gain	= -1,
	.again_10x	= -1,
};

static int sc2232_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc2232_sensor_write_register(int reg, int data)
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
 * Analogue settings that follow the gain octave (0x3301) and, at 1x
 * coarse gain, the exposure (0x3314), inside a group hold.
 */
static void sc2232_ext(int level)
{
	static const u8 r3301[] = { 0x06, 0x14, 0x18, 0x13 };

	sc2232_sensor_write_register(0x3812, 0x00);

	if (level >= 0 && level < (int)ARRAY_SIZE(r3301))
		sc2232_sensor_write_register(0x3301, r3301[level]);
	else
		pr_err("%s gain_level error\n", __func__);

	if (sc2232.r3e00 == 0) {
		if (sc2232.r3e01 <= 4)
			sc2232_sensor_write_register(0x3314, 0x12);
		else if (sc2232.r3e01 > 10)
			sc2232_sensor_write_register(0x3314, 0x02);
	}

	sc2232_sensor_write_register(0x3812, 0x30);
}

/* Program the frame length of a pending set_fps request. */
static void sc2232_set_fps_async(void)
{
	if (sc2232.to_fps == sc2232.fps)
		return;

	sc2232_sensor_write_register(0x320e, (sc2232.to_vts >> 8) & 0xff);
	sc2232_sensor_write_register(0x320f, sc2232.to_vts & 0xff);
	sc2232.fps = sc2232.to_fps;
}

static int sc2232_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		sc2232_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);

	/* The vendor module repeats these after the list. */
	sc2232_sensor_write_register(0x3306, 0x48);
	sc2232_sensor_write_register(0x3632, 0x08);
	sc2232_sensor_write_register(0x5780, 0xff);

	sc2232.fps = SENSOR_BASE_FPS;
	/* The list reprogrammed gain and DPC; rewrite them on next update. */
	sc2232.coarse_gain = -1;
	sc2232.again_10x = -1;

	return 0;
}

static int sc2232_read_id(void)
{
	return SENSOR_ID;
}

static int sc2232_probe_id(void)
{
	int id = (sc2232_sensor_read_register(0x3107) << 8) & 0xff00;

	id |= sc2232_sensor_read_register(0x3108) & 0xff;
	pr_debug("%s id:0x%x\n", __func__, id);

	return id == SENSOR_ID ? SENSOR_ID : 0;
}

static int sc2232_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int sc2232_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc2232_get_fps(void)
{
	return sc2232.fps;
}

static int sc2232_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type sc2232_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

/*
 * The vendor module answers only VSYNC_ACTIVE_MS and CUR_FPS. It is a DVP
 * sensor, so report that too; the IO level is left to the default.
 */
static int sc2232_get_parameter(int param, void *value)
{
	switch ((enum sensor_get_param)param) {
	case GET_VSYNC_ACTIVE_MS:
		*(int *)value = VSYNC_ACTIVE_MS;
		return 0;
	case GET_CUR_FPS:
		*(int *)value = sc2232.fps;
		return 0;
	case GET_INTERFACE:
		*(int *)value = DVP_INTERFACE;
		return 0;
	default:
		return -1;
	}
}

static int sc2232_set_power_on(const int pwdn_pin, const int reset_pin)
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

	sc2232.fps = SENSOR_BASE_FPS;
	return 0;
}

static int sc2232_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

/*
 * Only the vendor's three rates. The frame length is written later, with
 * the next gain or exposure update or from the timer.
 */
static int sc2232_set_fps(const int fps)
{
	int vts;

	switch (fps) {
	case 15:
		vts = 1500;
		break;
	case 12:
		vts = 1800;	/* as the vendor: really 12.5 fps */
		break;
	case 10:
		vts = 2250;
		break;
	default:
		pr_err("%s set fps=%d fail\n", __func__, fps);
		return -EINVAL;
	}

	sc2232.to_fps = fps;
	sc2232.to_vts = vts;
	return 0;
}

static int sc2232_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc2232_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc2232_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/*
 * a_gain is 8.8 fixed point. Coarse gain 0x3e08[4:2] = 0/1/3/7 for 1x, 2x,
 * 4x, 8x; fine gain 0x3e09 = 0x10..0x1f in 1/16 steps of the octave.
 * 0x5781/0x5785 retune the DPC at 10x.
 */
static int sc2232_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int gain = a_gain << 2;	/* 1x = 0x400 */
	int coarse, fine, again_10x;

	sc2232_set_fps_async();

	/*
	 * The vendor table ends below 16x; past it the vendor module writes
	 * a garbage fine gain at 1x coarse. Saturate instead.
	 */
	if (a_gain >= 0x1000)
		gain = 0x3fff;

	again_10x = gain >= 0x2800;

	if (gain < 0x800) {
		fine = gain - 0x400;
		coarse = 0;
		sc2232.again_level = 0;
	} else if (gain < 0x1000) {
		fine = (gain >> 1) - 0x400;
		coarse = 1;
		sc2232.again_level = 1;
	} else if (gain < 0x2000) {
		fine = (gain >> 2) - 0x400;
		coarse = 3;
		sc2232.again_level = 2;
	} else {
		fine = (gain >> 3) - 0x400;
		coarse = 7;
		sc2232.again_level = 3;
	}

	sc2232_ext(sc2232.again_level);

	fine = fine < 0 ? 0 : fine / 65;
	sc2232_sensor_write_register(0x3e09, fine | 0x10);

	if (coarse != sc2232.coarse_gain) {
		int r3e08 = sc2232_sensor_read_register(0x3e08) & 0xe3;

		sc2232_sensor_write_register(0x3e08, r3e08 | (coarse << 2));
		sc2232.coarse_gain = coarse;
	}

	if (again_10x != sc2232.again_10x) {
		if (again_10x) {
			sc2232_sensor_write_register(0x5781, 0x02);
			sc2232_sensor_write_register(0x5785, 0x08);
		} else {
			sc2232_sensor_write_register(0x5781, 0x04);
			sc2232_sensor_write_register(0x5785, 0x18);
		}
		sc2232.again_10x = again_10x;
	}

	return 0;
}

/* Exposure in lines: 0x3e00[3:0]:0x3e01[7:0]:0x3e02[7:4]. */
static int sc2232_cmos_updata_exp_time(unsigned int exp_time)
{
	int r3e00, r3e01, r3e02;

	sc2232_set_fps_async();
	/* As the vendor: 0x3314 follows the previous exposure. */
	sc2232_ext(sc2232.again_level);

	r3e01 = (exp_time >> 4) & 0xff;
	r3e02 = ((exp_time << 4) & 0xf0) |
		(sc2232_sensor_read_register(0x3e02) & 0x0f);
	sc2232_sensor_write_register(0x3e01, r3e01);
	sc2232_sensor_write_register(0x3e02, r3e02);

	r3e00 = (exp_time >> 12) & 0x0f;
	sc2232_sensor_write_register(0x3e00, r3e00 |
		(sc2232_sensor_read_register(0x3e00) & 0xf0));

	sc2232.r3e00 = r3e00;
	sc2232.r3e01 = r3e01;
	sc2232.r3e02 = r3e02;

	return EXP_EFFECT_FRAMES;
}

/* Apply a pending frame rate change that no AE update has picked up. */
static int sc2232_cmos_update_a_gain_timer(void)
{
	if (sc2232.to_fps != sc2232.fps && !sc2232.fps_timer_run) {
		sc2232.fps_timer_run = true;
		sc2232.fps_timer_sec = ktime_get_seconds();
	}

	if (sc2232.fps_timer_run) {
		time64_t now = ktime_get_seconds();

		if (now >= sc2232.fps_timer_sec + FPS_TIMER_SEC) {
			sc2232_set_fps_async();
			sc2232.fps_timer_sec = now;
			sc2232.fps_timer_run = false;
		}
	}

	return 0;
}

static AK_ISP_SENSOR_CB sc2232_callback = {
	.sensor_init_func			= sc2232_init,
	.sensor_read_reg_func			= sc2232_sensor_read_register,
	.sensor_write_reg_func			= sc2232_sensor_write_register,
	.sensor_read_id_func			= sc2232_read_id,
	.sensor_update_a_gain_func		= sc2232_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc2232_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= sc2232_cmos_updata_exp_time,
	.sensor_timer_func			= sc2232_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= sc2232_probe_id,
	.sensor_get_resolution_func		= sc2232_get_resolution,
	.sensor_get_mclk_func			= sc2232_get_mclk,
	.sensor_get_fps_func			= sc2232_get_fps,
	.sensor_get_valid_coordinate_func	= sc2232_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc2232_get_bus_type,
	.sensor_get_parameter_func		= sc2232_get_parameter,
	.sensor_set_power_on_func		= sc2232_set_power_on,
	.sensor_set_power_off_func		= sc2232_set_power_off,
	.sensor_set_fps_func			= sc2232_set_fps,
	.sensor_set_standby_in_func		= sc2232_set_standby_in,
	.sensor_set_standby_out_func		= sc2232_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(sc2232_callback, sc2235p)
