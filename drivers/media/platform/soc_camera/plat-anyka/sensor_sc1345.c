/*
 * SmartSens SC1345 (DVP, 1280x720) on the ISP2 sensor framework.
 *
 * Ported from the behaviour of the vendor module sensor_SC1345.ko shipped
 * on Gwell AK3918EV200 cameras (Linux 3.4.35, no source available): same
 * chip ID check, power sequence, gain table and exposure/VTS handling.
 * The register init list comes from the ISP tuning file (block 0x1c of
 * isp_sc1345_dvp.conf), as for the other sensors here.
 */
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	0
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x60
#define SENSOR_ID		0x1345
#define SENSOR_CHIP_ID		0xda23	/* registers 0x3107:0x3108 */
#define SENSOR_MCLK		24
#define SENSOR_REGADDR_BYTE	2
#define SENSOR_DATA_BYTE	1
#define SENSOR_BASE_FPS		30	/* frame timing of the init list */
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW

#define EFFECT_FRAMES		1
#define VSYNC_ACTIVE_MS		28
/* The vendor module reports 0, 1.8 V. */
#define SENSOR_IO_LEVEL		SENSOR_IO_LEVEL_1V8

static struct {
	int hts;	/* line length, 0x320c:0x320d */
	int vts;	/* frame length currently programmed, 0x320e:0x320f */
	int pclk;	/* hts * vts * SENSOR_BASE_FPS after init */
	int target_vts;	/* frame length wanted for the requested fps */
	int exp_time;	/* exposure in lines, as handed in by the AE */
} sc1345;

static int sc1345_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int sc1345_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int sc1345_read_pair(int reg)
{
	int hi = sc1345_sensor_read_register(reg) & 0xff;

	return (hi << 8) | (sc1345_sensor_read_register(reg + 1) & 0xff);
}

static int sc1345_get_hts(void)
{
	if (!sc1345.hts)
		sc1345.hts = sc1345_read_pair(0x320c);
	return sc1345.hts;
}

static int sc1345_get_vts(void)
{
	if (!sc1345.vts)
		sc1345.vts = sc1345_read_pair(0x320e);
	return sc1345.vts;
}

/* Exposure is programmed in half lines, 0x3e01[7:0]:0x3e02[7:4]. */
static void sc1345_set_exp_ctrl(int half_lines)
{
	int msb, lsb;

	if (half_lines < 0)
		half_lines = 0;
	if (half_lines < 0x1000) {
		msb = (half_lines >> 4) & 0xff;
		lsb = (half_lines << 4) & 0xff;
	} else {
		msb = 0xff;
		lsb = 0xf0;
	}
	lsb |= sc1345_sensor_read_register(0x3e02) & 0x0f;
	sc1345_sensor_write_register(0x3e01, msb);
	sc1345_sensor_write_register(0x3e02, lsb);
}

static void sc1345_set_vts(int vts)
{
	if (vts == sc1345.vts)
		return;
	sc1345_sensor_write_register(0x320e, (vts >> 8) & 0xff);
	sc1345_sensor_write_register(0x320f, vts & 0xff);
	sc1345.vts = sc1345_read_pair(0x320e);
}

/*
 * Grow the frame before a longer exposure and shrink it after a shorter
 * one, so the exposure never exceeds the frame for even one frame.
 */
static void sc1345_update_vts_and_exposure(void)
{
	int cur = sc1345_get_vts();
	int vts = sc1345.target_vts ? sc1345.target_vts : cur;
	int lines = sc1345.exp_time;

	if (lines > vts - 8)
		lines = vts - 8;

	if (vts >= cur) {
		sc1345_set_vts(vts);
		sc1345_set_exp_ctrl(lines * 2);
	} else {
		sc1345_set_exp_ctrl(lines * 2);
		sc1345_set_vts(vts);
	}
}

static int sc1345_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int i;

	for (i = 0; i < para->num; i++, preg_info++)
		sc1345_sensor_write_register(preg_info->reg_addr,
					     preg_info->value);

	sc1345.hts = sc1345_read_pair(0x320c);
	sc1345.vts = sc1345_read_pair(0x320e);
	sc1345.pclk = sc1345.hts * sc1345.vts * SENSOR_BASE_FPS;
	sc1345.target_vts = 0;

	return 0;
}

static int sc1345_read_id(void)
{
	return SENSOR_ID;
}

static int sc1345_probe_id(void)
{
	int id = sc1345_read_pair(0x3107);

	return id == SENSOR_CHIP_ID ? SENSOR_ID : 0;
}

static int sc1345_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int sc1345_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int sc1345_get_fps(void)
{
	int frame = sc1345_get_hts() * sc1345_get_vts();

	if (!frame || !sc1345.pclk)
		return SENSOR_BASE_FPS;
	return sc1345.pclk / frame;
}

static int sc1345_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type sc1345_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int sc1345_get_parameter(int param, void *value)
{
	switch ((enum sensor_get_param)param) {
	case GET_VSYNC_ACTIVE_MS:
		*(int *)value = VSYNC_ACTIVE_MS;
		return 0;
	case GET_CUR_FPS:
		*(int *)value = sc1345_get_fps();
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

static int sc1345_set_power_on(const int pwdn_pin, const int reset_pin)
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

	/* Timing is reread after the next init list. */
	sc1345.hts = 0;
	sc1345.vts = 0;
	return 0;
}

static int sc1345_set_power_off(const int pwdn_pin, const int reset_pin)
{
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

static int sc1345_set_fps(const int fps)
{
	int hts = sc1345_get_hts();

	if (fps <= 0 || !hts || !sc1345.pclk)
		return -EINVAL;
	sc1345.target_vts = sc1345.pclk / (hts * fps);
	sc1345_update_vts_and_exposure();
	return 0;
}

static int sc1345_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1345_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int sc1345_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 1;
}

/*
 * a_gain is 8.8 fixed point. Coarse gain doubles per step (0x3e08), fine
 * gain 0x20..0x3f spans each octave (0x3e09); 0x363a/0x3622 are the
 * analogue settings the vendor driver switches per octave.
 */
static int sc1345_cmos_updata_a_gain(const unsigned int a_gain)
{
	static const struct {
		int base;	/* gain x10000 at the start of the octave */
		int span;	/* divisor for 31 fine steps */
		int coarse;
		int r363a;
		int r3622;
	} octaves[] = {
		{ 10000,  9688, 0x03, 12, 0xc6 },
		{ 20000, 19375, 0x07, 22, 0xc6 },
		{ 40000, 38750, 0x0f, 22, 0xd6 },
		{ 80000, 77500, 0x1f, 29, 0xd6 },
	};
	int gain = (int)((a_gain * 10000) >> 8);
	int coarse, fine, r363a, r3622;
	int i;

	if (gain < 10000) {
		coarse = 0x03;
		fine = 0x20;
		r363a = 12;
		r3622 = 0xc6;
	} else if (gain > 157499) {
		coarse = 0x1f;
		fine = 0x3f;
		r363a = 29;
		r3622 = 0xd6;
	} else {
		for (i = ARRAY_SIZE(octaves) - 1; gain < octaves[i].base; i--)
			;
		coarse = octaves[i].coarse;
		fine = (gain - octaves[i].base) * 31 / octaves[i].span + 0x20;
		r363a = octaves[i].r363a;
		r3622 = octaves[i].r3622;
	}

	sc1345_sensor_write_register(0x3e08, coarse);
	sc1345_sensor_write_register(0x3e09, fine);
	/* Group hold around the analogue settings. */
	sc1345_sensor_write_register(0x3812, 0x00);
	sc1345_sensor_write_register(0x363a, r363a);
	sc1345_sensor_write_register(0x3622, r3622);
	sc1345_sensor_write_register(0x3812, 0x30);

	return EFFECT_FRAMES;
}

static int sc1345_cmos_updata_exp_time(unsigned int exp_time)
{
	sc1345.exp_time = exp_time;
	sc1345_update_vts_and_exposure();
	return EFFECT_FRAMES;
}

static int sc1345_cmos_update_a_gain_timer(void)
{
	return 0;
}

static AK_ISP_SENSOR_CB sc1345_callback = {
	.sensor_init_func			= sc1345_init,
	.sensor_read_reg_func			= sc1345_sensor_read_register,
	.sensor_write_reg_func			= sc1345_sensor_write_register,
	.sensor_read_id_func			= sc1345_read_id,
	.sensor_update_a_gain_func		= sc1345_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= sc1345_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= sc1345_cmos_updata_exp_time,
	.sensor_timer_func		= sc1345_cmos_update_a_gain_timer,
	.sensor_probe_id_func			= sc1345_probe_id,
	.sensor_get_resolution_func		= sc1345_get_resolution,
	.sensor_get_mclk_func			= sc1345_get_mclk,
	.sensor_get_fps_func			= sc1345_get_fps,
	.sensor_get_valid_coordinate_func	= sc1345_get_valid_coordinate,
	.sensor_get_bus_type_func		= sc1345_get_bus_type,
	.sensor_get_parameter_func		= sc1345_get_parameter,
	.sensor_set_power_on_func		= sc1345_set_power_on,
	.sensor_set_power_off_func		= sc1345_set_power_off,
	.sensor_set_fps_func			= sc1345_set_fps,
	.sensor_set_standby_in_func		= sc1345_set_standby_in,
	.sensor_set_standby_out_func		= sc1345_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(sc1345_callback, sc1345)
