/*
 * SOI JX-H62 (DVP, 1280x720) on the ISP2 sensor framework.
 *
 * Adapted from the AnyCloud39EV300 SDK source
 * component/UVC_check_sensor/kernel/driver/src/camera/sensor_h62.c (RTOS
 * glue), with behaviour following the vendor Linux 3.4.35 module
 * sensor_h62.ko shipped on AK3918EV200 cameras: chip ID check, power
 * sequence and software standby, gain conversion, exposure registers and
 * the frame-length table. The register init list comes from the ISP tuning
 * file (block 0x1c of isp_h62.conf), as for the other sensors here.
 *
 * Registers are 8-bit addressed with 8-bit data.
 */
#include <linux/kernel.h>
#include <linux/log2.h>
#include "ak_sensor_common.h"

#define SENSOR_PWDN_LEVEL	1
#define SENSOR_RESET_LEVEL	0
#define SENSOR_I2C_ADDR		0x60
#define SENSOR_ID		0xa062	/* registers 0x0a:0x0b */
#define SENSOR_MCLK		27
#define SENSOR_REGADDR_BYTE	1
#define SENSOR_DATA_BYTE	1
#define SENSOR_OUTPUT_WIDTH	1280
#define SENSOR_OUTPUT_HEIGHT	720
#define SENSOR_VALID_OFFSET_X	0
#define SENSOR_VALID_OFFSET_Y	0
#define SENSOR_BUS_TYPE		BUS_TYPE_RAW
#define SENSOR_IO_LEVEL		SENSOR_IO_LEVEL_1V8

#define SENSOR_MAX_FPS		30
/*
 * Lines per second of the vendor timing: every entry of the vendor
 * frame-length table is 22500 / fps (750 lines at 30 fps, 900 at 25).
 */
#define SENSOR_LINES_PER_SEC	22500
/* 720 active lines at SENSOR_LINES_PER_SEC. */
#define VSYNC_ACTIVE_MS		32

static struct {
	bool present;	/* chip ID matched at the last probe */
	int fps;	/* frame rate currently programmed */
} h62;

static int h62_sensor_read_register(int reg)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= 0;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_read_register(&i2cdata);
}

static int h62_sensor_write_register(int reg, int data)
{
	struct ak_sensor_i2c_data i2cdata;

	i2cdata.u8DevAddr		= SENSOR_I2C_ADDR;
	i2cdata.u32RegAddr		= reg;
	i2cdata.u32RegAddrByteNum	= SENSOR_REGADDR_BYTE;
	i2cdata.u32Data			= data;
	i2cdata.u32DataByteNum		= SENSOR_DATA_BYTE;

	return ak_sensor_write_register(&i2cdata);
}

/* Frame length in lines, 0x23[7:0]:0x22[7:0]. */
static void h62_set_vts(int vts)
{
	h62_sensor_write_register(0x23, (vts >> 8) & 0xff);
	h62_sensor_write_register(0x22, vts & 0xff);
}

static int h62_init(const AK_ISP_SENSOR_INIT_PARA *para)
{
	AK_ISP_SENSOR_REG_INFO *preg_info = para->reg_info;
	int hi, lo, vts;
	int i;

	/* The vendor module reads the ID register once before the list. */
	h62_sensor_read_register(0x0a);

	for (i = 0; i < para->num; i++, preg_info++)
		h62_sensor_write_register(preg_info->reg_addr,
					  preg_info->value);

	/* Report the frame rate the init list actually programmed. */
	hi = h62_sensor_read_register(0x23);
	lo = h62_sensor_read_register(0x22);
	vts = hi >= 0 && lo >= 0 ? ((hi & 0xff) << 8) | (lo & 0xff) : 0;
	h62.fps = vts ? SENSOR_LINES_PER_SEC / vts : 0;
	if (h62.fps <= 0 || h62.fps > SENSOR_MAX_FPS)
		h62.fps = SENSOR_MAX_FPS;

	return 0;
}

static int h62_read_id(void)
{
	return SENSOR_ID;
}

static int h62_probe_id(void)
{
	int hi = h62_sensor_read_register(0x0a);
	int lo = h62_sensor_read_register(0x0b);

	h62.present = hi >= 0 && lo >= 0 &&
		      (((hi & 0xff) << 8) | (lo & 0xff)) == SENSOR_ID;

	return h62.present ? SENSOR_ID : 0;
}

static int h62_get_resolution(int *width, int *height)
{
	*width = SENSOR_OUTPUT_WIDTH;
	*height = SENSOR_OUTPUT_HEIGHT;
	return 0;
}

static int h62_get_mclk(void)
{
	return SENSOR_MCLK;
}

static int h62_get_fps(void)
{
	return h62.fps;
}

static int h62_get_valid_coordinate(int *x, int *y)
{
	*x = SENSOR_VALID_OFFSET_X;
	*y = SENSOR_VALID_OFFSET_Y;
	return 0;
}

static enum sensor_bus_type h62_get_bus_type(void)
{
	return SENSOR_BUS_TYPE;
}

static int h62_get_parameter(int param, void *value)
{
	switch ((enum sensor_get_param)param) {
	case GET_VSYNC_ACTIVE_MS:
		*(int *)value = VSYNC_ACTIVE_MS;
		return 0;
	case GET_CUR_FPS:
		*(int *)value = h62_get_fps();
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

static int h62_set_power_on(const int pwdn_pin, const int reset_pin)
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

	h62.fps = SENSOR_MAX_FPS;
	return 0;
}

static int h62_set_power_off(const int pwdn_pin, const int reset_pin)
{
	/*
	 * Software standby as in the vendor module, but only when this is
	 * the H62: the probe powers each candidate driver off again, and
	 * another sensor may answer at the same I2C address.
	 */
	if (h62.present) {
		h62_sensor_write_register(0x3d, 0x48);
		h62_sensor_write_register(0xc3, 0x00);
	}

	ak_sensor_set_pin_level(pwdn_pin, SENSOR_PWDN_LEVEL);
	ak_sensor_set_pin_dir(reset_pin, 0);
	return 0;
}

static int h62_set_fps(const int fps)
{
	int vts;

	/* Frame lengths of the vendor table; 12.5 and 8.33 fps are rounded. */
	switch (fps) {
	case 13:
	case 12:
		vts = 1800;
		break;
	case 8:
		vts = 2700;
		break;
	default:
		if (fps <= 0 || fps > SENSOR_MAX_FPS)
			return -EINVAL;
		vts = SENSOR_LINES_PER_SEC / fps;
		break;
	}

	h62_set_vts(vts);
	h62.fps = fps;
	return 0;
}

static int h62_set_standby_in(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int h62_set_standby_out(const int pwdn_pin, const int reset_pin)
{
	return 0;
}

static int h62_cmos_updata_d_gain(const unsigned int d_gain)
{
	return 0;
}

/* The vendor's coarse gain step for an integer gain n: 2^k <= n, up to 64. */
static unsigned int h62_gain_step(unsigned int n)
{
	return n >= 64 ? 64 : rounddown_pow_of_two(n);
}

/*
 * a_gain is 8.8 fixed point. Register 0x00 holds [6:4] the coarse gain
 * 2^k and [3:0] the fine gain in 1/16 steps:
 * gain = 2^k * (16 + fine) / 16. Conversion as in the vendor module,
 * rounding up to the next step.
 */
static int h62_cmos_updata_a_gain(const unsigned int a_gain)
{
	unsigned int gain = max(a_gain, 256U);
	unsigned int g16 = gain >> 4;		/* gain x16 */
	unsigned int step = h62_gain_step(gain >> 8);
	unsigned int fine = min(g16 / step - 16, 15U);

	if ((fine + 16) * step < g16) {
		if (fine < 15) {
			fine++;
		} else {
			step = h62_gain_step((gain >> 8) + 1);
			fine = 0;
		}
	}

	h62_sensor_write_register(0x00, (ilog2(step) << 4) | fine);
	return 0;
}

/* Exposure in lines, 0x02[7:0]:0x01[7:0]. */
static int h62_cmos_updata_exp_time(unsigned int exp_time)
{
	h62_sensor_write_register(0x02, (exp_time >> 8) & 0xff);
	h62_sensor_write_register(0x01, exp_time & 0xff);
	return 0;
}

static int h62_timer(void)
{
	return 0;
}

static AK_ISP_SENSOR_CB h62_callback = {
	.sensor_init_func			= h62_init,
	.sensor_read_reg_func			= h62_sensor_read_register,
	.sensor_write_reg_func			= h62_sensor_write_register,
	.sensor_read_id_func			= h62_read_id,
	.sensor_update_a_gain_func		= h62_cmos_updata_a_gain,
	.sensor_update_d_gain_func		= h62_cmos_updata_d_gain,
	.sensor_updata_exp_time_func		= h62_cmos_updata_exp_time,
	.sensor_timer_func			= h62_timer,
	.sensor_probe_id_func			= h62_probe_id,
	.sensor_get_resolution_func		= h62_get_resolution,
	.sensor_get_mclk_func			= h62_get_mclk,
	.sensor_get_fps_func			= h62_get_fps,
	.sensor_get_valid_coordinate_func	= h62_get_valid_coordinate,
	.sensor_get_bus_type_func		= h62_get_bus_type,
	.sensor_get_parameter_func		= h62_get_parameter,
	.sensor_set_power_on_func		= h62_set_power_on,
	.sensor_set_power_off_func		= h62_set_power_off,
	.sensor_set_fps_func			= h62_set_fps,
	.sensor_set_standby_in_func		= h62_set_standby_in,
	.sensor_set_standby_out_func		= h62_set_standby_out,
};

AK_SENSOR_MODULE_NAMED(h62_callback, jxh62)
