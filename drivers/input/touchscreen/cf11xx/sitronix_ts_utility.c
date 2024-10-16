#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"
//hf

void st_power_up(struct sitronix_ts_data *ts)
{
	unsigned char buffer[2];

	buffer[0] = DEVICE_CONTROL_REG;
	buffer[1] = 0x00;
	stx_i2c_write(ts->client, buffer, 2);

	msleep(50);
}

void st_power_down(struct sitronix_ts_data *ts)
{
	unsigned char buffer[2];

	buffer[0] = DEVICE_CONTROL_REG;
	buffer[1] = 0x02;
	stx_i2c_write(ts->client, buffer, 2);

	msleep(50);
}

int st_get_status(struct sitronix_ts_data *ts)
{
	char buffer[8];
	int ret = -1;
	ret = stx_i2c_read(ts->client, buffer, 8, STATUS_REG);
	if (ret != 8) {
		STX_ERROR(" i2c communcate error in getting status : 0x%x",
			  ret);
		return -1;
	}
	STX_INFO(" Touch Panel status %x", buffer[0]);
	return buffer[0];
}

int st_get_dev_status(struct i2c_client *client, uint8_t *err_code,
		      uint8_t *dev_status)
{
	int ret = 0;
	uint8_t buffer[8];
	STX_DEBUG("%s,line=%d", __FUNCTION__, __LINE__);

	ret = stx_i2c_read_bytes(STATUS_REG, buffer, 8);
	if (ret < 0) {
		STX_ERROR("sitronix read status reg error (%d)", ret);
		return ret;
	} else {
		STX_INFO("sitronix status reg = %d", buffer[0]);
	}

	*err_code = (buffer[0] & 0xf0) >> 4;
	*dev_status = buffer[0] & 0xf;

	return 0;
}

int st_print_version(struct sitronix_ts_data *ts)
{
	char buffer[8];
	int ret = -1;

	ret = stx_i2c_read(ts->client, buffer, 4, FIRMWARE_REVISION_3);
	if (ret != 4) {
		STX_ERROR(
			" i2c communcate error in getting FW reversion : 0x%x",
			ret);
		return -1;
	}
	STX_INFO("Touch Panel Firmware Revision %x %x %x %x", buffer[0],
		 buffer[1], buffer[2], buffer[3]);
	snprintf(ts->ts_dev_info.fw_revision, 32, "%x.%x.%x.%x", buffer[0],
		 buffer[1], buffer[2], buffer[3]);

	ret = stx_i2c_read(ts->client, buffer, 1, FIRMWARE_VERSION);
	if (ret != 1) {
		STX_ERROR(" i2c communcate error in getting FW version : 0x%x",
			  ret);
		return -1;
	}
	STX_INFO("Touch Panel Firmware version %x", buffer[0]);

	ts->ts_dev_info.fw_version[0] = buffer[0];
	//	snprintf(ts->ts_dev_info.fw_version, 2,"%x", buffer[0]);
	return 0;
}

static int sitronix_get_xy_chs(struct sitronix_ts_data *ts)
{
	int ret = 0;
	uint8_t txflag = 0;
	uint8_t buf[2];

	ret = stx_i2c_read_bytes(MISC_Info, &txflag, 1);
	if (ret < 0) {
		STX_ERROR("%s: Read MISC_Info error!(%d)", __func__, ret);
		return ret;
	}

	ret = stx_i2c_read_bytes(XY_CHS, buf, 2);
	if (ret < 0) {
		STX_ERROR("%s: Read XY channels error!(%d)", __func__, ret);
		return ret;
	}

	txflag = (txflag & 0x4) >> 2;

	if (txflag) {
		ts->ts_dev_info.rx_chs = buf[0];
		ts->ts_dev_info.tx_chs = buf[1];
	} else {
		ts->ts_dev_info.rx_chs = buf[1];
		ts->ts_dev_info.tx_chs = buf[0];
	}
	STX_INFO("RX_chs = %d.", ts->ts_dev_info.rx_chs);
	STX_INFO("TX_chs = %d.", ts->ts_dev_info.tx_chs);

	return 0;
}

static int sitronix_get_max_touches(struct sitronix_ts_data *ts)
{
	int ret = 0;
	uint8_t buffer[1];
	STX_DEBUG("%s,line=%d", __FUNCTION__, __LINE__);

	ret = stx_i2c_read(ts->client, buffer, 1, MAX_NUM_TOUCHES);
	if (ret < 0) {
		STX_ERROR("read max touches error (%d)", ret);
		return ret;
	} else {
		ts->ts_dev_info.max_touches = buffer[0];
		if (ts->ts_dev_info.max_touches > ST_MAX_TOUCHES)
			ts->ts_dev_info.max_touches = ST_MAX_TOUCHES;
		STX_INFO("max touches = %d ", ts->ts_dev_info.max_touches);
	}
	return 0;
}

static int sitronix_get_resolution(struct sitronix_ts_data *ts)
{
	int ret = 0;
	uint8_t buffer[4];
	STX_DEBUG("%s,line=%d", __FUNCTION__, __LINE__);
	ret = stx_i2c_read_bytes(XY_RESOLUTION_HIGH, buffer, 3);
	if (ret < 0) {
		STX_ERROR("read resolution error (%d)", ret);
		return ret;
	} else {
		ts->ts_dev_info.x_res =
			((buffer[0] & (X_RES_H_BMSK << X_RES_H_SHFT)) << 4) |
			buffer[1];
		ts->ts_dev_info.y_res =
			((buffer[0] & Y_RES_H_BMSK) << 8) | buffer[2];
		STX_INFO("resolution = %d x %d", ts->ts_dev_info.x_res,
			 ts->ts_dev_info.y_res);
	}
	return 0;
}

static int sitronix_ts_get_CHIP_ID(struct sitronix_ts_data *ts)
{
	int ret = 0;
	uint8_t buffer[3];
	STX_DEBUG("%s,line=%d", __FUNCTION__, __LINE__);

	ret = stx_i2c_read_bytes(CHIP_ID, buffer, 3);
	if (ret < 0) {
		STX_ERROR("read Chip ID error (%d)", ret);
		return ret;
	} else {
		if (buffer[0] == 0) {
			if (buffer[1] + buffer[2] > 32)
				ts->ts_dev_info.chip_id = 2;
			else
				ts->ts_dev_info.chip_id = 0;
		} else
			ts->ts_dev_info.chip_id = buffer[0];
		ts->ts_dev_info.Num_X = buffer[1];
		ts->ts_dev_info.Num_Y = buffer[2];
		STX_INFO("Chip ID = %d", ts->ts_dev_info.chip_id);
		STX_INFO("Num_X = %d", ts->ts_dev_info.Num_X);
		STX_INFO("Num_Y = %d", ts->ts_dev_info.Num_Y);
	}
	return 0;
}

int st_get_touch_info(struct sitronix_ts_data *ts)
{
	int ret = 0;

	STX_DEBUG("%s,line=%d", __FUNCTION__, __LINE__);
	ret = sitronix_get_resolution(ts);
	if (ret < 0)
		return ret;
	ret = sitronix_ts_get_CHIP_ID(ts);
	if (ret < 0)
		return ret;
	ret = st_print_version(ts);
	if (ret < 0)
		return ret;
	ret = sitronix_get_max_touches(ts);
	if (ret < 0)
		return ret;
	ret = sitronix_get_xy_chs(ts);
	if (ret < 0)
		return ret;

	return 0;
}

int st_enter_glove_mode(struct sitronix_ts_data *ts)
{
	char buffer[2];
	int ret = 0;
	if (1) //!ts->glove_mode)
	{
		ret = stx_i2c_read(ts->client, buffer, 1, DEVICE_CONTROL_REG);
		if (ret < 0) {
			STX_ERROR(
				" i2c communcate error in getting DeviceControl : 0x%x",
				ret);
			return ret;
		}
		buffer[1] = buffer[0] | 0x20; //set bit5 to 1
		buffer[0] = DEVICE_CONTROL_REG;
		ret = stx_i2c_write(ts->client, buffer, 2);
		if (ret < 0) {
			STX_ERROR(
				" i2c communcate error in setting DeviceControl : 0x%x",
				ret);
			return ret;
		}
		ts->glove_mode = true;
	}
	return ret;
}

int st_leave_glove_mode(struct sitronix_ts_data *ts)
{
	char buffer[2];
	int ret = 0;
	if (ts->glove_mode) {
		ret = stx_i2c_read(ts->client, buffer, 1, DEVICE_CONTROL_REG);
		if (ret < 0) {
			STX_ERROR(
				" i2c communcate error in getting DeviceControl : 0x%x",
				ret);
			return ret;
		}
		buffer[1] = buffer[0] & 0xDF; //set bit5 to 0
		buffer[0] = DEVICE_CONTROL_REG;
		stx_i2c_write(ts->client, buffer, 2);
		if (ret < 0) {
			STX_ERROR(
				" i2c communcate error in setting DeviceControl : 0x%x",
				ret);
			return ret;
		}
		ts->glove_mode = false;
	}
	return ret;
}

int st_irq_off(void)
{
	stx_irq_disable();
	stx_gpts.is_upgrading = true;
	return 0;
}

int st_irq_on(void)
{
	stx_irq_enable();
	stx_gpts.is_upgrading = false;
	return 0;
}