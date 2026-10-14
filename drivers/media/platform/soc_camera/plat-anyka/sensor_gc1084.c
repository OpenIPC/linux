/**
 * @file sensor_gc1084.c
 * @brief GalaxyCore GC1084 on a DVP bus, 1280x720 @30fps.
 *
 * carries what the vendor .ko carried: the gain ladder, the exposure and
 * frame-rate arithmetic, and the accessors.
 */
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL		1
#define SENSOR_RESET_LEVEL		0
/* 8-bit write address: helpers do u8DevAddr >> 1 -> 7-bit 0x37 */
#define SENSOR_I2C_ADDR			0x6E
#define SENSOR_ID			0x1084
/* The conf tag reads base27M@pclk=36Mhz, so mclk is 27 not the usual 24. */
#define SENSOR_MCLK			27
/* GC1084 addresses registers with 16 bits, unlike its gc1054/gc2053
 * neighbours which use 8. Getting this wrong makes every access miss. */
#define SENSOR_REGADDR_BYTE		2
#define SENSOR_DATA_BYTE		1
#define MAX_FPS				30
#define SENSOR_OUTPUT_WIDTH		1280
#define SENSOR_OUTPUT_HEIGHT		720
#define SENSOR_VALID_OFFSET_X		0
#define SENSOR_VALID_OFFSET_Y		0
#define SENSROR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_INTERFACE		DVP_INTERFACE
/* The IO level is a board property the driver never reads back. 3V3 is the
 * common case for a DVP GC1084 and is what the neighbouring Anyka sensor
 * drivers declare. */
#define SENSOR_IO_LEVEL			SENSOR_IO_LEVEL_3V3
/* One frame at 30fps is 33.3ms; the ISP uses this to time 3A work. */
#define VSYNC_ACTIVE_MS			33

#define REG_CHIP_ID_HI			0x03f0
#define REG_CHIP_ID_LO			0x03f1
#define REG_EXP_HI			0x0d03
#define REG_EXP_LO			0x0d04
#define REG_VTS_HI			0x0d41
#define REG_VTS_LO			0x0d42

#define REG_AGAIN_M			0x00d1
#define REG_AGAIN_L			0x00d0
#define REG_AGAIN_H			0x0dc1
#define REG_AGAIN_LATCH			0x031d
#define REG_AGAIN_0155			0x0155
#define REG_COL_AGAIN_H			0x00b8
#define REG_COL_AGAIN_L			0x00b9
#define REG_DGAIN_H			0x00b1
#define REG_DGAIN_L			0x00b2

#define AGAIN_LATCH_OPEN		0x2e
#define AGAIN_LATCH_CLOSE		0x28

/* Frame length at 30fps, from the sensor init table: 0x0d41/42 = 0x02ee. */
#define SENSOR_VTS_30FPS		750
/* 0x0d03/04 is 14 bits wide. */
#define SENSOR_EXP_MAX			0x3fff

static int g_fps = MAX_FPS;
static int to_fps;

static int gc1084_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

static int gc1084_sensor_read_register(const int reg)
{
	struct ak_sensor_i2c_data i2cdata;
	int ret;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	/* sensor_read_register returns the byte as s32; it does not fill u32Data. */
	ret = ak_sensor_read_register(&i2cdata);
	if (ret < 0)
		return -1;

	return ret & 0xff;
}

static int gc1084_read_id(void)
{
	return SENSOR_ID;
}

static int gc1084_probe_id(void)
{
	int hi = gc1084_sensor_read_register(REG_CHIP_ID_HI);
	int lo = gc1084_sensor_read_register(REG_CHIP_ID_LO);
	int id = ((hi & 0xff) << 8) | (lo & 0xff);

	if (id != SENSOR_ID) {
		ak_sensor_print("gc1084: chip id 0x%04x, expected 0x%04x\n",
				id, SENSOR_ID);
		return -1;
	}

	/* ak_sensor.c matches read_id_func() against probe_id_func(); both must
	 * return the chip id on success, not 0.
	 */
	return id;
}

/*
 * The ISP passes the register script in, having read it out of the tuning
 * blob. This driver never chooses those values; a DVP-tuned conf is what makes
 * this a DVP sensor.
 */
static int gc1084_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info;
	int i, ret;

	if (gc1084_probe_id() != SENSOR_ID)
		return -ENODEV;

	preg_info = para->reg_info;
	for (i = 0; i < para->num; i++) {
		ret = gc1084_sensor_write_register(preg_info[i].reg_addr,
						 preg_info[i].value & 0xff);
		if (ret != SENSOR_REGADDR_BYTE + SENSOR_DATA_BYTE)
			return ret < 0 ? ret : -EIO;
	}

	g_fps = MAX_FPS;
	to_fps = 0;

	return 0;
}

/*
 * Analogue gain.
 *
 * gainLevelTable is Q6 fixed point, 64 == 1.0x, 25 steps from 1.0x to 64.0x.
 * A step selects the six analogue registers below; the remainder between the
 * step and the requested gain is applied as sensor digital gain in 0x00b1/b2.
 *
 * Both tables are reproduced from the vendor module's .data (gainLevelTable at
 * +0x58, regValTable at +0xc0); the values were re-derived from the binary
 * rather than taken on trust.
 */
#define GC1084_AGAIN_STEPS	25

static const unsigned int gc1084_gain_level[GC1084_AGAIN_STEPS] = {
	  64,   76,   90,  106,  128,  152,  179,  212,  256,  303,
	 358,  425,  512,  607,  716,  848, 1024, 1214, 1434, 1699,
	2048, 2427, 2865, 3393, 4096,
};

struct gc1084_again_regs {
	unsigned char r00d1, r00d0, r0dc1, r0155, r00b8, r00b9;
};

static const struct gc1084_again_regs gc1084_again_tbl[GC1084_AGAIN_STEPS] = {
	{0x00, 0x00, 0x00, 0x80, 0x01, 0x00}, {0x0a, 0x00, 0x00, 0x80, 0x01, 0x0b},
	{0x00, 0x01, 0x00, 0x80, 0x01, 0x19}, {0x0a, 0x01, 0x00, 0x80, 0x01, 0x2a},
	{0x00, 0x02, 0x00, 0x80, 0x02, 0x00}, {0x0a, 0x02, 0x00, 0x80, 0x02, 0x17},
	{0x00, 0x03, 0x00, 0x80, 0x02, 0x33}, {0x0a, 0x03, 0x00, 0x80, 0x03, 0x14},
	{0x00, 0x04, 0x00, 0x90, 0x04, 0x00}, {0x0a, 0x04, 0x00, 0x90, 0x04, 0x2f},
	{0x00, 0x05, 0x00, 0x90, 0x05, 0x26}, {0x0a, 0x05, 0x00, 0x90, 0x06, 0x28},
	{0x00, 0x06, 0x00, 0xa0, 0x08, 0x00}, {0x0a, 0x06, 0x00, 0xa0, 0x09, 0x1e},
	{0x12, 0x46, 0x00, 0xa0, 0x0b, 0x0c}, {0x19, 0x66, 0x00, 0xa0, 0x0d, 0x10},
	{0x00, 0x04, 0x01, 0xa0, 0x10, 0x00}, {0x0a, 0x04, 0x01, 0xa0, 0x12, 0x3d},
	{0x00, 0x05, 0x01, 0xb0, 0x16, 0x19}, {0x0a, 0x05, 0x01, 0xc0, 0x1a, 0x23},
	{0x00, 0x06, 0x01, 0xc0, 0x20, 0x00}, {0x0a, 0x06, 0x01, 0xc0, 0x25, 0x3b},
	{0x12, 0x46, 0x01, 0xc0, 0x2c, 0x30}, {0x19, 0x66, 0x01, 0xd0, 0x35, 0x01},
	{0x20, 0x06, 0x01, 0xe0, 0x3f, 0x3f},
};

static int gc1084_setgain(unsigned int gain_q6)
{
	const struct gc1084_again_regs *r;
	unsigned int pregain;
	int i;

	if (gain_q6 < gc1084_gain_level[0])
		gain_q6 = gc1084_gain_level[0];

	/* Highest step whose gain does not exceed the request. */
	for (i = GC1084_AGAIN_STEPS - 1; i > 0; i--)
		if (gc1084_gain_level[i] <= gain_q6)
			break;

	r = &gc1084_again_tbl[i];

	gc1084_sensor_write_register(REG_AGAIN_M, r->r00d1);
	gc1084_sensor_write_register(REG_AGAIN_L, r->r00d0);
	gc1084_sensor_write_register(REG_AGAIN_LATCH, AGAIN_LATCH_OPEN);
	gc1084_sensor_write_register(REG_AGAIN_H, r->r0dc1);
	gc1084_sensor_write_register(REG_AGAIN_LATCH, AGAIN_LATCH_CLOSE);
	gc1084_sensor_write_register(REG_AGAIN_0155, r->r0155);
	gc1084_sensor_write_register(REG_COL_AGAIN_H, r->r00b8);
	gc1084_sensor_write_register(REG_COL_AGAIN_L, r->r00b9);

	pregain = (gain_q6 << 6) / gc1084_gain_level[i];
	gc1084_sensor_write_register(REG_DGAIN_H, pregain >> 6);
	gc1084_sensor_write_register(REG_DGAIN_L, (pregain << 2) & 0xfc);

	return 0;
}

/* The ISP runs analogue gain in Q8 (ONE_X_GAIN 0x100); the ladder is Q6. */
static int gc1084_cmos_updata_a_gain(const unsigned int a_gain)
{
	return gc1084_setgain((a_gain & 0x3ffff) >> 2);
}

static int gc1084_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/* Returns the number of frames the ISP must wait before the value is live. */
static int gc1084_cmos_updata_exp_time(unsigned int exp_time)
{
	if (exp_time < 1)
		exp_time = 1;
	else if (exp_time > SENSOR_EXP_MAX)
		exp_time = SENSOR_EXP_MAX;

	gc1084_sensor_write_register(REG_EXP_LO, exp_time & 0xff);
	gc1084_sensor_write_register(REG_EXP_HI, (exp_time >> 8) & 0xff);

	return 1;
}

/*
 * Frame rate is changed by stretching the frame length, so it is only safe on
 * a frame boundary. Defer to the timer callback exactly as the vendor driver
 * does rather than writing from whatever context asked.
 */
static int gc1084_set_fps(int fps)
{
	if (fps < 1 || fps > MAX_FPS ||
	    SENSOR_VTS_30FPS * MAX_FPS / fps > 0x3fff)
		return -EINVAL;

	to_fps = fps;

	return 0;
}

static int gc1084_set_fps_async(int fps)
{
	unsigned int vts;
	int ret;

	if (fps < 1 || fps > MAX_FPS)
		return -EINVAL;

	vts = SENSOR_VTS_30FPS * MAX_FPS / fps;
	if (vts > 0x3fff)
		return -EINVAL;

	ret = gc1084_sensor_write_register(REG_VTS_HI, (vts >> 8) & 0x3f);
	if (ret != SENSOR_REGADDR_BYTE + SENSOR_DATA_BYTE)
		return ret < 0 ? ret : -EIO;
	ret = gc1084_sensor_write_register(REG_VTS_LO, vts & 0xff);
	if (ret != SENSOR_REGADDR_BYTE + SENSOR_DATA_BYTE)
		return ret < 0 ? ret : -EIO;

	g_fps = fps;

	return 0;
}

static int gc1084_cmos_timer(void)
{
	int ret;

	if (to_fps) {
		ret = gc1084_set_fps_async(to_fps);
		if (ret)
			return ret;
		to_fps = 0;
	}

	return 0;
}

static int gc1084_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;

	return 0;
}

static int gc1084_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int gc1084_get_fps(void)
{
	return g_fps;
}

static int gc1084_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;

	return 0;
}

static enum sensor_bus_type gc1084_get_bus_type(void)
{
	return SENSROR_BUS_TYPE;
}

static int gc1084_get_parameter(int param, void *value)
{
	enum sensor_get_param name = (enum sensor_get_param)param;
	int ret = 0;

	switch (name) {
	case GET_MIPI_MHZ:
	case GET_MIPI_LANE:
		/* DVP part: this SoC has no MIPI receiver at all. */
		*((int *)value) = 0;
		break;

	case GET_INTERFACE:
		*((int *)value) = SENSOR_IO_INTERFACE;
		break;

	case GET_SENSOR_IO_LEVEL:
		*((int *)value) = SENSOR_IO_LEVEL;
		break;

	case GET_VSYNC_ACTIVE_MS:
		*((int *)value) = VSYNC_ACTIVE_MS;
		break;

	case GET_CUR_FPS:
		*((int *)value) = g_fps;
		break;

	default:
		ret = -1;
		break;
	}

	return ret;
}

static int gc1084_set_power_on(const int pwdn_pin, const int reset_pin)
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

	return 0;
}

static int gc1084_set_power_off(const int pwdn_pin, const int reset_pin)
{
	/*
	 * Reset is left deasserted on purpose: pin_avdd and pin_power are
	 * AKSENSOR_INVALID_GPIO, so asserting it gates no rail, and
	 * gc1084_set_power_on() pulses reset before the part is used again.
	 */
	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);

	return 0;
}

static int gc1084_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int gc1084_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static AK_ISP_SENSOR_CB gc1084_callback =
{
	.sensor_init_func		= gc1084_init,
	.sensor_read_reg_func		= gc1084_sensor_read_register,
	.sensor_write_reg_func		= gc1084_sensor_write_register,
	.sensor_read_id_func		= gc1084_read_id,
	.sensor_update_a_gain_func	= gc1084_cmos_updata_a_gain,
	.sensor_update_d_gain_func	= gc1084_cmos_updata_d_gain,
	.sensor_updata_exp_time_func	= gc1084_cmos_updata_exp_time,
	.sensor_timer_func		= gc1084_cmos_timer,

	.sensor_probe_id_func		= gc1084_probe_id,
	.sensor_get_resolution_func	= gc1084_get_resolution,
	.sensor_get_mclk_func		= gc1084_get_mclk,
	.sensor_get_fps_func		= gc1084_get_fps,
	.sensor_get_valid_coordinate_func = gc1084_get_valid_coordinate,
	.sensor_get_bus_type_func	= gc1084_get_bus_type,
	.sensor_get_parameter_func	= gc1084_get_parameter,

	.sensor_set_power_on_func	= gc1084_set_power_on,
	.sensor_set_power_off_func	= gc1084_set_power_off,
	.sensor_set_fps_func		= gc1084_set_fps,
	.sensor_set_standby_in_func	= gc1084_set_standby_in,
	.sensor_set_standby_out_func	= gc1084_set_standby_out
};

AK_SENSOR_MODULE(gc1084_callback, gc1084)
