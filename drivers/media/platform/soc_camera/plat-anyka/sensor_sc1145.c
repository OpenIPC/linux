/*
 * SmartSens SC1145 (DVP, 1280x720) on the ISP2 sensor framework.
 *
 * Adapted from the AnyCloud39EV300 SDK's RTOS driver
 * component/UVC_check_sensor/kernel/driver/src/camera/1sensor_sc1145.c,
 * with behaviour taken from the vendor Linux 3.4.35 module
 * sensor_sc1145.ko shipped on V380 AK3918EV200 cameras (the June 2019
 * build that registers a full AK_ISP_SENSOR_CB): chip ID check, power
 * sequence, frame-rate table, exposure, and the analogue gain conversion
 * with its BLC-driven gain ceiling. The register init list comes from the
 * ISP tuning file (block 0x1c of isp_sc1145.conf), as for the other
 * sensors here.
 */
#include <linux/kernel.h>
#include <linux/log2.h>
#include <linux/ktime.h>
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	1
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x60
#define SENSOR_ID		0x1145	/* registers 0x3107:0x3108 */
#define SENSOR_ID_REV		0x01	/* register 0x3109 */
#define SENSOR_MCLK		24
#define SENSOR_REGADDR_BYTE	2
#define SENSOR_DATA_BYTE	1
#define SENSOR_DEFAULT_FPS	25	/* frame length of the init list */
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

/* Frame length at 30 fps; longer frames at the same line time are slower. */
#define SENSOR_VTS_30FPS	750
/*
 * 720 active lines at 30 * 750 lines/s. Not reported by the vendor
 * module, which leaves get_parameter unset.
 */
#define VSYNC_ACTIVE_MS		32

#define EXP_EFFECT_FRAMES	1
#define GAIN_SKIP_FRAMES	2	/* after a 2x/4x stage change */
#define GAIN_TIMER_PERIOD_S	5

/* 0x3e09 codes the BLC watchdog may cap the analogue gain at. */
static const u8 sc1145_gain_map[] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

#define BLC_HIGH		0x1c	/* above: lower the gain ceiling */
#define BLC_LOW			0x19	/* below: raise it again */

static struct {
	int fps;
	unsigned int a_gain;		/* last gain from the AE, 8.8 */
	int stage, prev_stage;		/* extra analogue stage: 1, 2 or 4 */
	unsigned int max_gain_idx;	/* into sc1145_gain_map */
	time64_t gain_time;		/* last periodic gain refresh */
} sc1145 = {
	.fps		= SENSOR_DEFAULT_FPS,
	.stage		= 1,
	.prev_stage	= 1,
	.max_gain_idx	= ARRAY_SIZE(sc1145_gain_map) - 1,
};

static int sc1145_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc1145_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc1145_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		sc1145_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);
	return 0;
}

static int sc1145_read_id(void)
{
	return SENSOR_ID;
}

static int sc1145_probe_id(void)
{
	int id;

	id = (sc1145_sensor_read_register(0x3107) & 0xff) << 8;
	id |= sc1145_sensor_read_register(0x3108) & 0xff;
	if (id != SENSOR_ID)
		return 0;
	if ((sc1145_sensor_read_register(0x3109) & 0xff) != SENSOR_ID_REV)
		return 0;
	return SENSOR_ID;
}

static int sc1145_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int sc1145_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc1145_get_fps(void)
{
	return sc1145.fps;
}

static int sc1145_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type sc1145_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

/* The IO level is left to the bridge's default; nothing gives it. */
static int sc1145_get_parameter(int param, void *value)
{
	switch ((enum sensor_get_param)param) {
	case GET_VSYNC_ACTIVE_MS:
		*(int *)value = VSYNC_ACTIVE_MS;
		return 0;
	case GET_CUR_FPS:
		*(int *)value = sc1145.fps;
		return 0;
	case GET_INTERFACE:
		*(int *)value = DVP_INTERFACE;
		return 0;
	default:
		return -1;
	}
}

static int sc1145_set_power_on(const int pwdn_pin, const int reset_pin)
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

	sc1145.fps = SENSOR_DEFAULT_FPS;
	return 0;
}

static int sc1145_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

/*
 * The frame length goes to 0x320e:0x320f and, as in the vendor table,
 * to 0x3338:0x3339, with its excess over the 30 fps frame in
 * 0x3336:0x3337.
 */
static int sc1145_set_fps(const int fps)
{
	int vts, extra;

	switch (fps) {
	case 30:
		vts = 750;
		break;
	case 25:
		vts = 900;
		break;
	case 15:
		vts = 1500;
		break;
	case 13:	/* actually 12.5 fps */
	case 12:
		vts = 1800;
		break;
	case 10:
		vts = 2250;
		break;
	case 8:
		vts = 2700;
		break;
	default:
		pr_err("%s: unsupported fps %d\n", __func__, fps);
		return -EINVAL;
	}

	extra = vts - SENSOR_VTS_30FPS;
	sc1145_sensor_write_register(0x320e, vts >> 8);
	sc1145_sensor_write_register(0x320f, vts & 0xff);
	sc1145_sensor_write_register(0x3338, vts >> 8);
	sc1145_sensor_write_register(0x3339, vts & 0xff);
	sc1145_sensor_write_register(0x3336, extra >> 8);
	sc1145_sensor_write_register(0x3337, extra & 0xff);
	sc1145.fps = fps;
	return 0;
}

static int sc1145_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1145_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1145_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/*
 * Lower the gain ceiling one step while the black level (0x3911) is high,
 * raise it one step while it is low, and clip @code to it.
 */
static unsigned int sc1145_limit_gain(unsigned int code)
{
	unsigned int blc = sc1145_sensor_read_register(0x3911) & 0xffff;
	unsigned int max;

	if (blc > BLC_HIGH) {
		if (sc1145.max_gain_idx > 0)
			sc1145.max_gain_idx--;
	} else if (blc < BLC_LOW) {
		if (sc1145.max_gain_idx < ARRAY_SIZE(sc1145_gain_map) - 1)
			sc1145.max_gain_idx++;
	}

	max = sc1145_gain_map[sc1145.max_gain_idx];
	return min(code, max);
}

/*
 * a_gain is 8.8 fixed point. 0x3e09[6:4] selects 1x/2x/4x/8x and
 * 0x3e09[3:0] a fine step of 1/16 within that octave; beyond 8x the
 * extra 2x and 4x stages are switched in 0x3610/0x3605.
 */
static int sc1145_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int gain = a_gain >> 4;	/* 1x = 16 */
	unsigned int idx, coarse, fine, code;
	int ret;

	sc1145.a_gain = a_gain;

	if (gain < 16)
		gain = 16;
	idx = min(gain >> 4, 60U);
	coarse = min(rounddown_pow_of_two(idx), 32UL);
	fine = min(gain * 17 / (16 * coarse) - 17, 15U);

	switch (coarse) {
	case 1:
		code = 0x00;
		sc1145.stage = 1;
		break;
	case 2:
		code = 0x10;
		sc1145.stage = 1;
		break;
	case 4:
		code = 0x30;
		sc1145.stage = 1;
		break;
	case 8:
		code = 0x70;
		sc1145.stage = 1;
		break;
	case 16:
		code = 0x70;
		sc1145.stage = 2;
		break;
	default:	/* 32 */
		code = 0x70;
		sc1145.stage = 4;
		break;
	}
	code |= fine;

	ret = sc1145.stage != sc1145.prev_stage ? GAIN_SKIP_FRAMES : 0;

	switch (sc1145.stage) {
	case 1:
		sc1145_sensor_write_register(0x3610, 0x03);
		sc1145_sensor_write_register(0x3605, 0x0c);
		break;
	case 2:
		sc1145_sensor_write_register(0x3610, 0x1b);
		sc1145_sensor_write_register(0x3605, 0x0c);
		break;
	default:
		sc1145_sensor_write_register(0x3610, 0x1b);
		sc1145_sensor_write_register(0x3605, 0x3c);
		break;
	}
	sc1145.prev_stage = sc1145.stage;

	sc1145_sensor_write_register(0x3e09, sc1145_limit_gain(code));
	return ret;
}

/* Exposure in lines, 0x3e01[7:0]:0x3e02[7:4]. */
static int sc1145_cmos_updata_exp_time(unsigned int exp_time)
{
	int lsb = (exp_time << 4) & 0xf0;

	lsb |= sc1145_sensor_read_register(0x3e02) & 0x0f;
	sc1145_sensor_write_register(0x3e01, (exp_time >> 4) & 0xff);
	sc1145_sensor_write_register(0x3e02, lsb);
	return EXP_EFFECT_FRAMES;
}

/* Reapply the gain now and then, so the BLC ceiling keeps tracking. */
static int sc1145_cmos_update_a_gain_timer(void)
{
	time64_t now = ktime_get_seconds();

	if (now >= sc1145.gain_time + GAIN_TIMER_PERIOD_S) {
		sc1145.gain_time = now;
		if (sc1145.a_gain)
			sc1145_cmos_updata_a_gain(sc1145.a_gain);
	}
	return 0;
}

static AK_ISP_SENSOR_CB sc1145_callback = {
	.sensor_init_func			= sc1145_init,
	.sensor_read_reg_func			= sc1145_sensor_read_register,
	.sensor_write_reg_func			= sc1145_sensor_write_register,
	.sensor_read_id_func			= sc1145_read_id,
	.sensor_update_a_gain_func		= sc1145_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc1145_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= sc1145_cmos_updata_exp_time,
	.sensor_timer_func			= sc1145_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= sc1145_probe_id,
	.sensor_get_resolution_func		= sc1145_get_resolution,
	.sensor_get_mclk_func			= sc1145_get_mclk,
	.sensor_get_fps_func			= sc1145_get_fps,
	.sensor_get_valid_coordinate_func	= sc1145_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc1145_get_bus_type,
	.sensor_get_parameter_func		= sc1145_get_parameter,
	.sensor_set_power_on_func		= sc1145_set_power_on,
	.sensor_set_power_off_func		= sc1145_set_power_off,
	.sensor_set_fps_func			= sc1145_set_fps,
	.sensor_set_standby_in_func		= sc1145_set_standby_in,
	.sensor_set_standby_out_func		= sc1145_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(sc1145_callback, sc1145)
