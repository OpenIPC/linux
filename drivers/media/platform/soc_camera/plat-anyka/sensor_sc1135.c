/*
 * SmartSens SC1135 (DVP RAW, 1280x960) on the ISP2 sensor framework.
 *
 * Adapted from the AnyCloud39EV300 SDK's RTOS driver
 * component/UVC_check_sensor/kernel/driver/src/camera/sensor_sc1135.c
 * (Anyka, luweichun/ye_guohong), with the behaviour of the vendor Linux
 * 3.4.35 module sensor_sc1135.ko "V2.0.00" shipped on V380E2 AK3918EV200
 * cameras, the build that uses this ISP2 callback table. Where the two
 * disagree the module is followed: frame lengths of the fps table (and
 * its 15 and 5 fps entries), the 30 ms reset pulse and the return values.
 *
 * The register init list comes from the ISP tuning file (block 0x1c of
 * style 0 of isp_sc1135.conf); this driver writes nothing after it.
 */
#include <linux/kernel.h>
#include <linux/ktime.h>

#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	1	/* PWDN pin level that powers down */
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x60
#define SENSOR_ID		0x1135
#define SENSOR_MCLK		27
#define SENSOR_REGADDR_BYTE	2
#define SENSOR_DATA_BYTE	1
#define DEFAULT_FPS		25
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	960
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

/*
 * The vendor module has no get_parameter callback, so the ISP host used
 * its defaults; keep the host's default vsync active time.
 */
#define VSYNC_ACTIVE_MS		36

/* Frames the ISP skips after the 2x analogue gain stage toggles. */
#define SKIP_FRAME_NUM		2
#define EFFECT_FRAMES		1

/* Black level window that adapts the analogue gain ceiling. */
#define SENSOR_BLC_TOP_VALUE	0x58
#define SENSOR_BLC_BOT_VALUE	0x45
#define SENSOR_MAX_AGAIN	0x7f
#define AGAIN_TIMER_PERIOD_S	5

/*
 * SC1135 ID registers 0x3107, 0x3108, 0x3109, 0x3c05 and 0x580b all read
 * back zero (the source notes they match the SC1037's).
 */
static const int sc1135_id_regs[] = { 0x3107, 0x3108, 0x3109, 0x3c05, 0x580b };

static int g_fps = DEFAULT_FPS;
static int read_id_ok;
static unsigned int g_a_gain;
static int sc1135_curr_2x_again;
static unsigned int sc1135_max_again = SENSOR_MAX_AGAIN;

static int sc1135_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc1135_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc1135_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		sc1135_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);

	return 0;
}

static int sc1135_read_id(void)
{
	return SENSOR_ID;
}

static int sc1135_probe_id(void)
{
	unsigned int i;

	/* As the vendor module, a successful probe is remembered. */
	if (read_id_ok)
		return SENSOR_ID;

	for (i = 0; i < ARRAY_SIZE(sc1135_id_regs); i++)
		if (sc1135_sensor_read_register(sc1135_id_regs[i]) != 0)
			return 0;

	read_id_ok = 1;
	return SENSOR_ID;
}

static int sc1135_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int sc1135_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc1135_get_fps(void)
{
	return g_fps;
}

static int sc1135_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type sc1135_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

/* The I/O level is not known; the host keeps its default for it. */
static int sc1135_get_parameter(int param, void *value)
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

static int sc1135_set_power_on(const int pwdn_pin, const int reset_pin)
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

	g_fps = DEFAULT_FPS;
	return 0;
}

static int sc1135_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

/*
 * Frame length per fps. 0x3338:0x3339 mirrors the frame length and
 * 0x3336:0x3337 is it less 0x2e8; the module's lengths all derive from
 * 1010 lines at 30 fps.
 */
static int sc1135_set_fps(const int fps)
{
	int vts;

	switch (fps) {
	case 30:
		vts = 0x3f2;
		break;
	case 25:
		vts = 0x4bc;
		break;
	case 15:
		vts = 0x7e4;
		break;
	case 13:	/* actually 12.5 fps */
	case 12:
		vts = 0x978;
		break;
	case 10:
		vts = 0xbd6;
		break;
	case 8:
		vts = 0xecb;
		break;
	case 5:
		vts = 0x17ac;
		break;
	default:
		pr_err("%s set fps fail\n", __func__);
		return -EINVAL;
	}

	sc1135_sensor_write_register(0x320e, vts >> 8);
	sc1135_sensor_write_register(0x320f, vts & 0xff);
	sc1135_sensor_write_register(0x3336, (vts - 0x2e8) >> 8);
	sc1135_sensor_write_register(0x3337, (vts - 0x2e8) & 0xff);
	sc1135_sensor_write_register(0x3338, vts >> 8);
	sc1135_sensor_write_register(0x3339, vts & 0xff);

	g_fps = fps;
	return 0;
}

static int sc1135_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1135_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1135_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/*
 * Lower the analogue gain ceiling one step while the black level (0x3911)
 * is above the window and raise it while it is below, then clip.
 */
static unsigned int sc1135_again_limit(unsigned int again)
{
	int blc = sc1135_sensor_read_register(0x3911);

	if (blc >= 0) {
		if (blc > SENSOR_BLC_TOP_VALUE) {
			if (sc1135_max_again > 0)
				sc1135_max_again--;
		} else if (blc < SENSOR_BLC_BOT_VALUE) {
			if (sc1135_max_again < SENSOR_MAX_AGAIN)
				sc1135_max_again++;
		}
	}

	return min(again, sc1135_max_again);
}

/*
 * a_gain is 8.8 fixed point. 0x3e09 holds the coarse gain in [6:4]
 * (0, 1, 3, 7 for 1x, 2x, 4x, 8x) and 1/16 fine steps in [3:0]; 0x3635
 * adds a further 2x stage above 16x. 0x3630/0x3631 are the analogue
 * settings the vendor driver switches with the gain range.
 */
static int sc1135_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int gain = a_gain << 2;	/* 1024 = 1x */
	int coarse, tmp, fine;
	int x2 = 0;

	g_a_gain = a_gain;

	if (gain <= 1980) {
		coarse = 0;
		tmp = (int)gain - 1024;
	} else if (gain <= 3960) {
		coarse = 1;
		tmp = (int)(gain / 2) - 1024;
	} else if (gain <= 7920) {
		coarse = 3;
		tmp = (int)(gain / 4) - 1024;
	} else if (gain <= 15840) {
		coarse = 7;
		tmp = (int)(gain / 8) - 1024;
	} else if (gain <= 31680) {
		coarse = 7;
		x2 = 1;
		tmp = (int)(gain / 16) - 1024;
	} else {
		coarse = 7;
		x2 = 1;
		tmp = 956;
	}
	fine = tmp > 0 ? tmp / 60 : 0;

	sc1135_sensor_write_register(0x3e09,
		sc1135_again_limit((coarse << 4) | (fine & 0xf)));
	sc1135_sensor_write_register(0x3635, x2 ? 0x0c : 0x00);

	if (coarse == 0) {
		sc1135_sensor_write_register(0x3630, 0xb8);
		sc1135_sensor_write_register(0x3631, 0x82);
	} else if (!x2) {
		sc1135_sensor_write_register(0x3630, 0x70);
		sc1135_sensor_write_register(0x3631, 0x8e);
	} else {
		sc1135_sensor_write_register(0x3630, 0x70);
		sc1135_sensor_write_register(0x3631, 0x8c);
	}

	if (sc1135_curr_2x_again != x2) {
		sc1135_curr_2x_again = x2;
		return SKIP_FRAME_NUM;
	}
	return 0;
}

/* Exposure in lines, 0x3e01[7:0]:0x3e02[7:4]; 0x3e02[3:0] is kept. */
static int sc1135_cmos_updata_exp_time(unsigned int exp_time)
{
	int lsb = (exp_time << 4) & 0xf0;

	lsb |= sc1135_sensor_read_register(0x3e02) & 0x0f;
	sc1135_sensor_write_register(0x3e01, (exp_time >> 4) & 0xff);
	sc1135_sensor_write_register(0x3e02, lsb);

	return EFFECT_FRAMES;
}

/* Reapply the last gain periodically so the BLC-driven ceiling tracks. */
static int sc1135_cmos_update_a_gain_timer(void)
{
	static time64_t last;
	time64_t now = ktime_get_seconds();

	if (now >= last + AGAIN_TIMER_PERIOD_S) {
		last = now;
		sc1135_cmos_updata_a_gain(g_a_gain);
	}

	return 0;
}

static AK_ISP_SENSOR_CB sc1135_callback = {
	.sensor_init_func			= sc1135_init,
	.sensor_read_reg_func			= sc1135_sensor_read_register,
	.sensor_write_reg_func			= sc1135_sensor_write_register,
	.sensor_read_id_func			= sc1135_read_id,
	.sensor_update_a_gain_func		= sc1135_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc1135_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= sc1135_cmos_updata_exp_time,
	.sensor_timer_func			= sc1135_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= sc1135_probe_id,
	.sensor_get_resolution_func		= sc1135_get_resolution,
	.sensor_get_mclk_func			= sc1135_get_mclk,
	.sensor_get_fps_func			= sc1135_get_fps,
	.sensor_get_valid_coordinate_func	= sc1135_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc1135_get_bus_type,
	.sensor_get_parameter_func		= sc1135_get_parameter,
	.sensor_set_power_on_func		= sc1135_set_power_on,
	.sensor_set_power_off_func		= sc1135_set_power_off,
	.sensor_set_fps_func			= sc1135_set_fps,
	.sensor_set_standby_in_func		= sc1135_set_standby_in,
	.sensor_set_standby_out_func		= sc1135_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(sc1135_callback, sc1135)
