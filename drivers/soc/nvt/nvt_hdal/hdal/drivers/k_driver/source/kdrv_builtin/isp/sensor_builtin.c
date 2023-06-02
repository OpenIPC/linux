#if defined(__KERNEL__)

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include "isp_builtin.h"
#include "sensor_builtin.h"

#if (NVT_FAST_ISP_FLOW)
//=============================================================================
// define
//=============================================================================
#define SENSOR_BUILTIN_I2C_NAME_1 "sensor_builtin_1"
#define SENSOR_BUILTIN_I2C_ADDR_1 0x0
#define SENSOR_BUILTIN_I2C_NAME_2 "sensor_builtin_2"
#define SENSOR_BUILTIN_I2C_ADDR_2 0x0
#define SENSOR_BUILTIN_I2C_NAME_3 "sensor_builtin_3"
#define SENSOR_BUILTIN_I2C_ADDR_3 0x0
#define SENSOR_BUILTIN_I2C_NAME_4 "sensor_builtin_4"
#define SENSOR_BUILTIN_I2C_ADDR_4 0x0
#define SENSOR_BUILTIN_I2C_NAME_5 "sensor_builtin_5"
#define SENSOR_BUILTIN_I2C_ADDR_5 0x0

//=============================================================================
// global
//=============================================================================
static UINT32 sensor_builtin_row_time[ISP_BUILTIN_ID_MAX_NUM];

static struct i2c_board_info sensor_builtin_i2c_device[ISP_BUILTIN_ID_MAX_NUM] = {
	{
		.type = SENSOR_BUILTIN_I2C_NAME_1,
		.addr = SENSOR_BUILTIN_I2C_ADDR_1
	},
	{
		.type = SENSOR_BUILTIN_I2C_NAME_2,
		.addr = SENSOR_BUILTIN_I2C_ADDR_2
	},
	{
		.type = SENSOR_BUILTIN_I2C_NAME_3,
		.addr = SENSOR_BUILTIN_I2C_ADDR_3
	},
		{
		.type = SENSOR_BUILTIN_I2C_NAME_4,
		.addr = SENSOR_BUILTIN_I2C_ADDR_4
	},
	{
		.type = SENSOR_BUILTIN_I2C_NAME_5,
		.addr = SENSOR_BUILTIN_I2C_ADDR_5
	}
};

static SENSOR_BUILTIN_I2C_INFO *sensor_builtin_i2c_info[ISP_BUILTIN_ID_MAX_NUM];
static const struct i2c_device_id sensor_builtin_sen_i2c_id[][ISP_BUILTIN_ID_MAX_NUM] = {
	{
		{ SENSOR_BUILTIN_I2C_NAME_1, 0 },
		{ }
	},
	{
		{ SENSOR_BUILTIN_I2C_NAME_2, 0 },
		{ }
	},
	{
		{ SENSOR_BUILTIN_I2C_NAME_3, 0 },
		{ }
	},
	{
		{ SENSOR_BUILTIN_I2C_NAME_4, 0 },
		{ }
	},
	{
		{ SENSOR_BUILTIN_I2C_NAME_5, 0 },
		{ }
	}
};
//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// internal functions
//=============================================================================
static INT sensor_builtin_i2c_probe_1(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!(sensor_builtin_i2c_info[0] = MALLOC(sizeof(SENSOR_BUILTIN_I2C_INFO)))) {
		printk(" malloc fail. \r\n");
		return -1;
	}

	sensor_builtin_i2c_info[0]->iic_client  = client;
	sensor_builtin_i2c_info[0]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sensor_builtin_i2c_info[0]);

	return 0;
}

static INT sensor_builtin_i2c_remove_1(struct i2c_client *client)
{
	FREE(sensor_builtin_i2c_info[0]);
	sensor_builtin_i2c_info[0] = NULL;
	return 0;
}

static INT sensor_builtin_i2c_probe_2(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!(sensor_builtin_i2c_info[1] = MALLOC(sizeof(SENSOR_BUILTIN_I2C_INFO)))) {
		printk(" malloc fail. \r\n");
		return -1;
	}

	sensor_builtin_i2c_info[1]->iic_client  = client;
	sensor_builtin_i2c_info[1]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sensor_builtin_i2c_info[1]);

	return 0;
}

static INT sensor_builtin_i2c_remove_2(struct i2c_client *client)
{
	FREE(sensor_builtin_i2c_info[1]);
	sensor_builtin_i2c_info[1] = NULL;
	return 0;
}

static INT sensor_builtin_i2c_probe_3(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!(sensor_builtin_i2c_info[2] = MALLOC(sizeof(SENSOR_BUILTIN_I2C_INFO)))) {
		printk(" malloc fail. \r\n");
		return -1;
	}

	sensor_builtin_i2c_info[2]->iic_client  = client;
	sensor_builtin_i2c_info[2]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sensor_builtin_i2c_info[2]);

	return 0;
}

static INT sensor_builtin_i2c_remove_3(struct i2c_client *client)
{
	FREE(sensor_builtin_i2c_info[2]);
	sensor_builtin_i2c_info[2] = NULL;
	return 0;
}

static INT sensor_builtin_i2c_probe_4(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!(sensor_builtin_i2c_info[3] = MALLOC(sizeof(SENSOR_BUILTIN_I2C_INFO)))) {
		printk(" malloc fail. \r\n");
		return -1;
	}

	sensor_builtin_i2c_info[3]->iic_client  = client;
	sensor_builtin_i2c_info[3]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sensor_builtin_i2c_info[3]);

	return 0;
}

static INT sensor_builtin_i2c_remove_4(struct i2c_client *client)
{
	FREE(sensor_builtin_i2c_info[3]);
	sensor_builtin_i2c_info[3] = NULL;
	return 0;
}

static INT sensor_builtin_i2c_probe_5(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!(sensor_builtin_i2c_info[4] = MALLOC(sizeof(SENSOR_BUILTIN_I2C_INFO)))) {
		printk(" malloc fail. \r\n");
		return -1;
	}

	sensor_builtin_i2c_info[4]->iic_client  = client;
	sensor_builtin_i2c_info[4]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sensor_builtin_i2c_info[4]);

	return 0;
}

static INT sensor_builtin_i2c_remove_5(struct i2c_client *client)
{
	FREE(sensor_builtin_i2c_info[4]);
	sensor_builtin_i2c_info[4] = NULL;
	return 0;
}

static struct i2c_driver sensor_builtin_i2c_driver[ISP_BUILTIN_ID_MAX_NUM] = {
	{
		.driver = {
			.name  = SENSOR_BUILTIN_I2C_NAME_1,
			.owner = THIS_MODULE,
		},
		.probe    = sensor_builtin_i2c_probe_1,
		.remove   = sensor_builtin_i2c_remove_1,
		.id_table = sensor_builtin_sen_i2c_id[0]
	},
	{
		.driver = {
			.name  = SENSOR_BUILTIN_I2C_NAME_2,
			.owner = THIS_MODULE,
		},
		.probe    = sensor_builtin_i2c_probe_2,
		.remove   = sensor_builtin_i2c_remove_2,
		.id_table = sensor_builtin_sen_i2c_id[1]
	},
	{
		.driver = {
			.name  = SENSOR_BUILTIN_I2C_NAME_3,
			.owner = THIS_MODULE,
		},
		.probe    = sensor_builtin_i2c_probe_3,
		.remove   = sensor_builtin_i2c_remove_3,
		.id_table = sensor_builtin_sen_i2c_id[2]
	},
	{
		.driver = {
			.name  = SENSOR_BUILTIN_I2C_NAME_4,
			.owner = THIS_MODULE,
		},
		.probe    = sensor_builtin_i2c_probe_4,
		.remove   = sensor_builtin_i2c_remove_4,
		.id_table = sensor_builtin_sen_i2c_id[3]
	},
	{
		.driver = {
			.name  = SENSOR_BUILTIN_I2C_NAME_5,
			.owner = THIS_MODULE,
		},
		.probe    = sensor_builtin_i2c_probe_5,
		.remove   = sensor_builtin_i2c_remove_5,
		.id_table = sensor_builtin_sen_i2c_id[4]
	}
};

static INT32 sensor_builtin_transfer_i2c(UINT32 id, struct i2c_msg *msgs, INT32 num)
{
	if (unlikely(sensor_builtin_i2c_info[id]->iic_adapter == NULL)) {
		printk(" sensen_i2c_info->ii2c_adapter fail. \r\n");
		return -1;
	}

	if (unlikely(i2c_transfer(sensor_builtin_i2c_info[id]->iic_adapter, msgs, num) != num)) {
		printk(" i2c_transfer fail. \r\n");
		return -1;
	}

	return 0;
}

static SENSOR_BUILTIN_I2C_CMD sensor_builtin_set_cmd_info(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	SENSOR_BUILTIN_I2C_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

UINT32 sensor_builtin_calc_log_2(UINT32 devider, UINT32 devident)
{
	UINT32 ratio;
	UINT32 idx, ret;
	static UINT32 log_tbl[20] = {104, 107, 112, 115, 119, 123, 128, 131, 137, 141, 147, 152, 157, 162, 168, 174, 181, 187, 193, 200};

	if (devident == 0) {
		return 1;
	}

	ret = 0;
	ratio = (devider * 100) / devident;
	while (ratio >= 200) {
		ratio /= 2;
		ret += 100;
	}

	for (idx = 0; idx < 20; idx++) {
		if (ratio < log_tbl[idx]) {
			break;
		}
	}

	return ret + idx * 5;
}

static ER sensor_builtin_write_i2c_2b1b(UINT32 id, SENSOR_BUILTIN_I2C_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[3];
	int i;

	buf[0]     = (cmd->addr >> 8) & 0xFF;
	buf[1]     = cmd->addr & 0xFF;
	buf[2]     = cmd->data[0] & 0xFF;
	msgs.addr = isp_builtin_get_i2c_addr(id);
	msgs.flags = 0;
	msgs.len   = 3;
	msgs.buf   = buf;

	i = 0;
	while(1){
		if (sensor_builtin_transfer_i2c(id, &msgs, 1) == 0) {
			break;
		}
		i++;
		if (i == 5) {
			return E_SYS;
		}
	}

	return E_OK;
}

static ER sensor_builtin_read_i2c_2b1b(UINT32 id, SENSOR_BUILTIN_I2C_CMD *cmd)
{
	struct i2c_msg  msgs[2];
	unsigned char   tmp[2], tmp2[2];
	int i;

	tmp[0]        = (cmd->addr >> 8) & 0xFF;
	tmp[1]        = cmd->addr & 0xFF;
	msgs[0].addr = isp_builtin_get_i2c_addr(id);
	msgs[1].addr = isp_builtin_get_i2c_addr(id);
	msgs[0].flags = 0;
	msgs[0].len   = 2;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].flags = 1;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	i = 0;
	while(1){
		if (sensor_builtin_transfer_i2c(id, msgs, 2) == 0) {
			break;
		}
		i++;
		if (i == 5) {
			return E_SYS;
		}
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static ER sensor_builtin_write_i2c_1b1b(UINT32 id, SENSOR_BUILTIN_I2C_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[2];
	int i;

	buf[0]     = cmd->addr & 0xFF;
	buf[1]     = cmd->data[0] & 0xFF;
	msgs.addr = isp_builtin_get_i2c_addr(id);
	msgs.flags = 0;
	msgs.len   = 2;
	msgs.buf   = buf;

	i = 0;
	while(1){
		if (sensor_builtin_transfer_i2c(id, &msgs, 1) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	return E_OK;
}

static ER sensor_builtin_read_i2c_1b1b(UINT32 id, SENSOR_BUILTIN_I2C_CMD *cmd)
{
	struct i2c_msg  msgs[2];
	unsigned char   tmp[2], tmp2[2];
	int i;

	tmp[0]        = cmd->addr & 0xFF;
	msgs[0].addr = isp_builtin_get_i2c_addr(id);
	msgs[0].flags = 0;
	msgs[0].len   = 1;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].addr = isp_builtin_get_i2c_addr(id);
	msgs[1].flags = 1;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	i = 0;
	while(1){
		if (sensor_builtin_transfer_i2c(id, msgs, 2) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static void sensor_builtin_imx290_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	SENSOR_BUILTIN_I2C_CMD cmd;
	static UINT32 pre_gain[ISP_BUILTIN_ID_MAX_NUM] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data = 0;
	UINT32 frame_cnt, total_frame;
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
	} else {
		total_frame = 1;
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		if (gain_info.gain_ratio[frame_cnt] >= 32000) {
			if (gain_info.gain_ratio[frame_cnt] >= 2000) {
				gain_info.gain_ratio[frame_cnt] = gain_info.gain_ratio[frame_cnt] >> 1;
			}
			data2[frame_cnt] = 1;
		} else {
			data2[frame_cnt] = 0;
		}

		data1[frame_cnt] = (6 * sensor_builtin_calc_log_2(gain_info.gain_ratio[frame_cnt], 1000)) / 30;
		if (data1[frame_cnt] > (0xF0)) {
			printk("gain overflow gain_ratio = %d data1[0] = 0x%.8x \r\n", gain_info.gain_ratio[frame_cnt], data1[frame_cnt]);
			data1[frame_cnt] = 0xF0;
		}
	}

	// Group hold
	cmd = sensor_builtin_set_cmd_info(0x3001, 1, 0x1, 0x0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	if (isp_builtin_get_shdr_enable(id)) {
		cmd = sensor_builtin_set_cmd_info(0x3010, 1, 0x61, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x30f0, 1, 0x64, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x30f4, 1, 0x64, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		
		cmd = sensor_builtin_set_cmd_info(0x3014, 1, data1[0] & 0xff, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		
		cmd = sensor_builtin_set_cmd_info(0x30f2, 1, data1[1] & 0xff, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		if (pre_gain[id] != data2[0]) {
			cmd = sensor_builtin_set_cmd_info(0x3016, 2, 0x8, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3009, 2, 0x0, 0x0);
			sensor_builtin_read_i2c_2b1b(id, &cmd);
			data = cmd.data[0];

			if (data2[0] == 1) {
				data = data | 0x10;
			} else {
				data = data & 0x0F;
			}
			cmd = sensor_builtin_set_cmd_info(0x3009, 2, data, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			pre_gain[id] = data2[0];
		} else {
			cmd = sensor_builtin_set_cmd_info(0x3016, 2, 0x9, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		}
	} else {
		//Set analog gain
		cmd = sensor_builtin_set_cmd_info(0x3014, 1, data1[0] & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		//Set digital gain
		if (pre_gain[id] != data2[0]) {
			cmd = sensor_builtin_set_cmd_info(0x3016, 1, 0x8, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			cmd = sensor_builtin_set_cmd_info(0x3009, 1, 0x0, 0x0);
			sensor_builtin_read_i2c_2b1b(id, &cmd);
			data = cmd.data[0];

			if (data2[0] == 1) {
				data = data | 0x10;
			} else {
				data = data & 0x0F;
			}
			cmd = sensor_builtin_set_cmd_info(0x3009, 1, data, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			pre_gain[id] = data2[0];
		} else {
			cmd = sensor_builtin_set_cmd_info(0x3016, 1, 0x9, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		}
	}
	// Group hold
	cmd = sensor_builtin_set_cmd_info(0x3001, 1, 0x0, 0x0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
}

static void sensor_builtin_imx290_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 shs1 = 0, shs2 = 0, y_out_size = 0, rhs1 = 0, t_row = 0;
	UINT32 frame_cnt, total_frame, sensor_vd;
	UINT32 sensor_hd = 4400, sensor_dft_vd = 1125, sensor_pclk = 148500000, dft_fps = 3000;

	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
		sensor_hd = 1100;
		sensor_dft_vd = 2250;
	} else {
		total_frame = 1;
		sensor_hd = 4400;
		sensor_dft_vd = 1125;
	}

	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);

	// Calculates the exposure setting
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		t_row = 100 * sensor_hd / (sensor_pclk / 100000);
		sensor_builtin_row_time[id] = t_row;
		line[frame_cnt] = sensor_ctrl->exp_time[frame_cnt] * 10 / t_row;
		//MIN_EXPOSURE_LINE          1
		//MIN_HDR_EXPOSURE_LINE      3
		if (isp_builtin_get_shdr_enable(id)) {
			if (line[frame_cnt] < 3) {
				line[frame_cnt] = 3;
			}
		} else {
			if (line[frame_cnt] < 1) {
				line[frame_cnt] = 1;
			}
		}
	}

	if (isp_builtin_get_shdr_enable(id)) {
		//MIN_HDR_EXPOSURE_LINE      3
		for(frame_cnt = 1; frame_cnt < total_frame ; frame_cnt++) {
			line[frame_cnt] = hdr_expline_clamp(line[frame_cnt], 3, (sensor_vd * 2 - 1109 * 2 - 21) - 2 - 1);
		}

		y_out_size = sensor_vd * 2 - 22;

		shs1 = 2;
		rhs1 = shs1 + line[1] + 1;

		rhs1 = rhs1 / 2 * 2 + 1;
		shs1 = rhs1 - line[1] - 1;
		shs1 = shs1 > 2 ? shs1 : 2;

		if(rhs1==9){// Purple image
			rhs1=rhs1+2;
			shs1=shs1+2;
		}

		shs2 = sensor_vd * 2 - line[0] -1;
		shs2 = shs2 > (rhs1+2) ? shs2 : (rhs1+2);
	} else {
		//MAX_VD_PERIOD              0x3FFFF
		//MIN_SHS1                   1
		if (line[0] > (0x3FFFF - 1 - 1)) {
			printk("max line overflow \r\n");
			line[0] = (0x3FFFF - 1 - 1);
		}

		if (line[0] < 1) {
			printk("min line overflow\r\n");
			line[0] = 1;
		}

		// Calculate shs1 & vmax
		if (line[0] > (sensor_vd - 1 - 1)) {
			shs1 = 1;
		} else {
			shs1 = sensor_vd - line[0] - 1;
		}
	}

	// Group hold
	cmd = sensor_builtin_set_cmd_info(0x3001, 1, 0x1, 0x0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	if (isp_builtin_get_shdr_enable(id)) {
		//Set exposure line to sensor (rhs1)
		cmd = sensor_builtin_set_cmd_info(0x3030, 1, rhs1 & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3031, 1, (rhs1 >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3032, 1, (rhs1 >> 16) & 0x03, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		
		//Set exposure line to sensor (shs2)
		cmd = sensor_builtin_set_cmd_info(0x3024, 1, shs2 & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3025, 1, (shs2 >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3026, 1, (shs2 >> 16) &0x03, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		
		// Set y out size
		cmd = sensor_builtin_set_cmd_info(0x3418, 1, y_out_size & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3419, 1, (y_out_size >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	}

	// Set exposure line to sensor (shr)
	cmd = sensor_builtin_set_cmd_info(0x3020, 1, shs1 & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x3021, 1, (shs1 >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x3022, 1, (shs1 >> 16) & 0x03, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	// Set exposure line to sensor (svr)
	cmd = sensor_builtin_set_cmd_info(0x3018, 1, sensor_vd & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x3019, 1, (sensor_vd >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x301A, 1, (sensor_vd >> 16 ) & 0x03, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	// Group hold
	cmd = sensor_builtin_set_cmd_info(0x3001, 1, 0x0, 0x0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
}

static void sensor_builtin_f35_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 a_gain_h = 0, a_gain_l = 0;
	UINT32 temp_reg0 = 0, temp_reg1 = 0,temp_reg2 = 0;
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	// Calculate sensor gain
	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
	} else {
		total_frame = 1;
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		if (gain_info.gain_ratio[frame_cnt] < 1000) {
			gain_info.gain_ratio[frame_cnt] = 1000;
		} else if (gain_info.gain_ratio[frame_cnt] > 15500) {
			gain_info.gain_ratio[frame_cnt] = 15500;
		}

		if (2000 > (gain_info.gain_ratio[frame_cnt])) {
			a_gain_h = 0;
			a_gain_l = ((gain_info.gain_ratio[frame_cnt]) - ((a_gain_h + 1) * 1000)) * 16 / 1000;
		} else if (4000 > (gain_info.gain_ratio[frame_cnt])) {
			a_gain_h = 1;
			a_gain_l = ((gain_info.gain_ratio[frame_cnt]) * 1000 / (a_gain_h + 1) - 1000000) * 16 / 1000000;
		} else if (8000 > (gain_info.gain_ratio[frame_cnt])) {
			a_gain_h = 2;
			a_gain_l = ((gain_info.gain_ratio[frame_cnt]) * 1000 / (a_gain_h * 2) - 1000000) * 16 / 1000000;
		} else {
			a_gain_h = 3;
			a_gain_l = ((gain_info.gain_ratio[frame_cnt]) * 1000 / ((a_gain_h + 1) * 2) - 1000000) * 16 / 1000000;
		}

		data1[frame_cnt] = a_gain_h;
		data2[frame_cnt] = a_gain_l;
	}

	cmd = sensor_builtin_set_cmd_info(0x00, 1, (data1[0] << 4) | (data2[0] & 0xF), 0x0);
	sensor_builtin_write_i2c_1b1b(id, &cmd);

	cmd = sensor_builtin_set_cmd_info(0x2F, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	temp_reg0 = cmd.data[0];

	cmd = sensor_builtin_set_cmd_info(0x0C, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	temp_reg1 = cmd.data[0];

	cmd = sensor_builtin_set_cmd_info(0x80, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	temp_reg2 = cmd.data[0];

	if (2 > (data1[0])) {
		cmd = sensor_builtin_set_cmd_info(0x2F, 1,(temp_reg0 | 0x20), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x0C, 1,(temp_reg1 | 0x40), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x80, 1,(temp_reg2 | 0x01), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	} else {
		cmd = sensor_builtin_set_cmd_info(0x2F, 1,(temp_reg0 & (~0x20)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x0C, 1,(temp_reg1 & (~0x40)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x80, 1,(temp_reg2 & (~0x01)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	}
}

static void sensor_builtin_f35_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM], t_row = 0, sensor_vd;
	UINT32 sensor_hd = 1200, sensor_dft_vd = 1125, sensor_pclk = 40500000, dft_fps = 3000;

	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
		sensor_hd = 600;
		sensor_dft_vd = 2250;
		sensor_pclk = 81000000;
	} else {
		total_frame = 1;
		sensor_hd = 1200;
		sensor_dft_vd = 1125;
		sensor_pclk = 40500000;
	}

	// Calculate exposure line
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Calculates the exposure setting
		if (!isp_builtin_get_shdr_enable(id)) {
			t_row = 100 * sensor_hd / (sensor_pclk / 100000);
		} else if (isp_builtin_get_shdr_enable(id)) {
			t_row = 100 * sensor_hd / (sensor_pclk / 2 / 100000);
		}

		sensor_builtin_row_time[id] = t_row;
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;

		if (isp_builtin_get_shdr_enable(id)) {
			if (0 == frame_cnt) {
				if (((line[frame_cnt]) % 2 == 0) && ((line[frame_cnt]) > 1)) {
					line[frame_cnt] -= 1;
				}
			}
		}

		// Limit minimun exposure line
		if (line[frame_cnt] < 1) {
			line[frame_cnt] = 1;
		}
	}

	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);

	//Check max vts
	if (sensor_vd > 0xFFFF) {
		printk("max vts overflow\r\n");
		sensor_vd = 0xFFFF;
	}

	//Write change mode VD
	cmd = sensor_builtin_set_cmd_info(0x22, 1, sensor_vd & 0xFF, 0);
	sensor_builtin_write_i2c_1b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x23, 1, (sensor_vd >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_1b1b(id, &cmd);

	if (!isp_builtin_get_shdr_enable(id)) {
		//Check max exp line reg
		if (line[0] > 0xFFFF ) {
			printk("max line overflow \r\n");
			line[0] = 0xFFFF;
		}

		//Check max exp line
		if (line[0] > (sensor_vd - 4)) {
			line[0] = sensor_vd - 4;
		}

		// set exposure line to sensor
		cmd = sensor_builtin_set_cmd_info(0x01, 1, (line[0] & 0xFF) , 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x02, 1, (line[0] >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd); 
	} else if (isp_builtin_get_shdr_enable(id)) {
		if (line[0] > 0xFFFF) {
			printk("max line overflow \r\n");
			line[0] = 0xFFFF;
		}

		if (line[1] > 0xFC) {
			line[1] = 0xFC;
		}

		if ((line[0] + line[1]) > sensor_vd) {
			line[0] = sensor_vd - line[1];
			if ((line[0]) % 2 == 0) {
					line[0] -= 1;
			}
		}
		// set Long exposure line to sensor
		cmd = sensor_builtin_set_cmd_info(0x01, 1, (line[0] & 0xFF) , 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x02, 1, (line[0] >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		// set Short exposure line to sensor	
		cmd = sensor_builtin_set_cmd_info(0x05, 1, ((line[1] - 1) >> 1) & 0xFF , 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	}
}

typedef struct _F37_GAIN_TABLE {
	UINT16 a_gain;
	UINT32 total_gain;
} F37_GAIN_TABLE;

// A Gain 0~32X
static const F37_GAIN_TABLE f37_gain_table[] = {
	{0x00,  1000 }, {0x01,  1063 }, {0x02,  1125 }, {0x03,  1188 }, {0x04,  1250 },
	{0x05,  1313 }, {0x06,  1375 }, {0x07,  1438 }, {0x08,  1500 }, {0x09,  1563 },
	{0x0A,  1625 }, {0x0B,  1688 }, {0x0C,  1750 }, {0x0D,  1813 }, {0x0E,  1875 },
	{0x0F,  1938 }, {0x10,  2000 }, {0x11,  2125 }, {0x12,  2250 }, {0x13,  2375 },
	{0x14,  2500 }, {0x15,  2625 }, {0x16,  2750 }, {0x17,  2875 }, {0x18,  3000 },
	{0x19,  3125 }, {0x1A,  3250 }, {0x1B,  3375 }, {0x1C,  3500 }, {0x1D,  3625 },
	{0x1E,  3750 }, {0x1F,  3875 }, {0x20,  4000 }, {0x21,  4250 }, {0x22,  4500 },
	{0x23,  4750 }, {0x24,  5000 }, {0x25,  5250 }, {0x26,  5500 }, {0x27,  5750 },
	{0x28,  6000 }, {0x29,  6250 }, {0x2A,  6500 }, {0x2B,  6750 }, {0x2C,  7000 },
	{0x2D,  7250 }, {0x2E,  7500 }, {0x2F,  7750 }, {0x30,  8000 }, {0x31,  8500 },
	{0x32,  9000 }, {0x33,  9500 }, {0x34,  10000}, {0x35,  10500}, {0x36,  11000},
	{0x37,  11500}, {0x38,  12000}, {0x39,  12500}, {0x3A,  13000}, {0x3B,  13500},
	{0x3C,  14000}, {0x3D,  14500}, {0x3E,  15000}, {0x3F,  15500}
};
#define NUM_OF_WDRGAINSET (sizeof(f37_gain_table) / sizeof(F37_GAIN_TABLE))

static void sensor_builtin_f37_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 gain_ratio;
	UINT16 a_gain = 0, i = 0;
	UINT32 reg1 = 0, reg2 = 0, reg3 = 0;
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	gain_ratio = gain_info.gain_ratio[0];

	//max_min_gain=1~15.5X
	if (gain_ratio < 1000) {
		gain_ratio = 1000;
	} else if (gain_ratio > 15500) {
		gain_ratio = 15500;
	}

	// search most suitable gain into gain table
	for (i=0; i<NUM_OF_WDRGAINSET; i++) {
		if (f37_gain_table[i].total_gain > gain_ratio)
		break;
	}

	//Black Sun Cancellation Strategy
	cmd = sensor_builtin_set_cmd_info(0x2F, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	reg1 = cmd.data[0];
	cmd = sensor_builtin_set_cmd_info(0x0C, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	reg2 = cmd.data[0];
	cmd = sensor_builtin_set_cmd_info(0x82, 1, 0x0, 0x0);
	sensor_builtin_read_i2c_1b1b(id, &cmd);
	reg3 = cmd.data[0]; 

	if(i != 0){
		a_gain = f37_gain_table[i-1].a_gain;
		//Write Sensor reg
		cmd = sensor_builtin_set_cmd_info(0x00, 1, a_gain, 0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	}

	//dark sun
	if (a_gain < 0x10) {
		cmd = sensor_builtin_set_cmd_info(0x2F, 1, (reg1 | 0x20), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x0C, 1, (reg2 | 0x40), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x82, 1, (reg3 | 0x02), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	} else {
		cmd = sensor_builtin_set_cmd_info(0x2F, 1, (reg1 & (~0x20)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x0C, 1, (reg2 & (~0x40)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x82, 1, (reg3 & (~0x02)), 0x0);
		sensor_builtin_write_i2c_1b1b(id, &cmd);
	}
}

static void sensor_builtin_f37_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 exp_time;
	UINT32 line, t_row = 0, sensor_vd;
	UINT32 sensor_hd = 3840, sensor_dft_vd = 1125, sensor_pclk = 129600000, dft_fps = 3000;

	exp_time = sensor_ctrl->exp_time[0];

	// Calculate exposure time
	t_row = 100 * sensor_hd / (sensor_pclk / 100000);
	sensor_builtin_row_time[id] = t_row;
	line = exp_time * 10 / t_row;

	if (line > 0xFFFF) {
		printk("max line overflow \r\n");
		line = 0xFFFF;
	}

	// Limit minimun exposure line
	if (line < 1) {
		printk("min line overflow\r\n");
		line = 1;
	}

	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);
	
	//Check max vts
	if (sensor_vd > 0xFFFF) {
		printk("max vts overflow\r\n");
		sensor_vd = 0xFFFF;
	}

	//Write change mode VD
	cmd = sensor_builtin_set_cmd_info(0x22, 1, sensor_vd & 0xFF, 0); 
	sensor_builtin_write_i2c_1b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x23, 1, (sensor_vd >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_1b1b(id, &cmd);

	// set exposure line to sensor
	cmd = sensor_builtin_set_cmd_info(0x01, 1, (line & 0xFF) , 0);
	sensor_builtin_write_i2c_1b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x02, 1, (line >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_1b1b(id, &cmd); 
}

static void sensor_builtin_os02k10_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 a_gain = 0, d_gain = 0;
	static UINT32 last_hcg_reg1[ISP_BUILTIN_ID_MAX_NUM] = {0}, last_hcg_reg2[ISP_BUILTIN_ID_MAX_NUM] = {0};
	UINT32 hcg_reg1 = 0, hcg_reg2 = 0, flag = 0, hcg_change_flag = 0;
	UINT32 change_frame_id[2] = {0};
	static UINT32 last_change_frame_id[ISP_BUILTIN_ID_MAX_NUM][2] = {0};
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	// Calculate sensor gain
	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
	} else {
		total_frame = 1;
	}

	if (isp_builtin_get_shdr_enable(id)) {
		if ((last_hcg_reg1[id] == 0) && (last_hcg_reg2[id] == 0)) {
			cmd = sensor_builtin_set_cmd_info(0x376C, 1, 0x0, 0x0);
			sensor_builtin_read_i2c_2b1b(id, &cmd);
			last_hcg_reg1[id] = cmd.data[0];
			cmd = sensor_builtin_set_cmd_info(0x3C55, 1, 0x0, 0x0);
			sensor_builtin_read_i2c_2b1b(id, &cmd);
			last_hcg_reg2[id] = cmd.data[0];
		}
	} else {
		cmd = sensor_builtin_set_cmd_info(0x376C, 1, 0x0, 0x0);
		sensor_builtin_read_i2c_2b1b(id, &cmd);
		last_hcg_reg1[id] = cmd.data[0];
		cmd = sensor_builtin_set_cmd_info(0x3C55, 1, 0x0, 0x0);
		sensor_builtin_read_i2c_2b1b(id, &cmd);
		last_hcg_reg2[id] = cmd.data[0];
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Min: 1000, MAX: 992000
		if (gain_info.gain_ratio[frame_cnt] < 1000) {
			gain_info.gain_ratio[frame_cnt] = 1000;
		} else if (gain_info.gain_ratio[frame_cnt] > 992000) {
			gain_info.gain_ratio[frame_cnt] = 992000;
		}

		if (!isp_builtin_get_shdr_enable(id)) {
			if (4000 <= (gain_info.gain_ratio[frame_cnt])) {
				gain_info.gain_ratio[frame_cnt] /= 4;
				hcg_change_flag = 1;
			}
		}

		if (isp_builtin_get_shdr_enable(id)) {
			if (4000 <= (gain_info.gain_ratio[frame_cnt])) {
				gain_info.gain_ratio[frame_cnt] /= 4;
				hcg_change_flag = 1;
				change_frame_id[frame_cnt] = 1;
			}
		}
	
		if (15500 >= gain_info.gain_ratio[frame_cnt]) {
			d_gain=0x0400;
			a_gain= (gain_info.gain_ratio[frame_cnt]) * 16 / 1000;
		} else {
			d_gain=(gain_info.gain_ratio[frame_cnt]) * 1024 / 15500;
			a_gain= 0xF8;
		}

		if (0x3FFF < d_gain) {
			d_gain = 0x3FFF;
		}

		data1[frame_cnt] = a_gain;
		data2[frame_cnt] = d_gain;
	}

	if (!isp_builtin_get_shdr_enable(id)) {
		if (hcg_change_flag) {
			hcg_reg1 = 0x00;
			hcg_reg2 = 0xCB;
		} else {
			hcg_reg1 = 0x10;
			hcg_reg2 = 0x08;
		}

		if ((last_hcg_reg1[id] != hcg_reg1) && (last_hcg_reg2[id] != hcg_reg2)) {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x00, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			cmd = sensor_builtin_set_cmd_info(0x3504, 1, 0x18, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			cmd = sensor_builtin_set_cmd_info(0x376C, 1, hcg_reg1, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3C55, 1, hcg_reg2, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			flag = 1;
		} else {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x00, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd); 
			flag = 0;
		}
	}	else {
		if ((hcg_change_flag) && (change_frame_id[0]) && (change_frame_id[1])) {
			if ((last_change_frame_id[id][1]) != (change_frame_id[1])) {
				hcg_reg1 = 0x30;
				hcg_reg2 = 0x08;
			} else {
				hcg_reg1 = 0x00;
				hcg_reg2 = 0xCB;
			}
			last_hcg_reg1[id] = 0x00;
			last_hcg_reg2[id] = 0xCB;
		} else if ((hcg_change_flag) && (change_frame_id[0])) {
			hcg_reg1 = 0x20;
			hcg_reg2 = 0xCB;
		} else if ((hcg_change_flag) && (change_frame_id[1])) {
			hcg_reg1 = 0x10;
			hcg_reg2 = 0xCB;
		} else {
			if ((last_change_frame_id[id][1]) != (change_frame_id[1])) {
				hcg_reg1 = 0x00;
				hcg_reg2 = 0xCB;
			} else {
				hcg_reg1 = 0x30;
				hcg_reg2 = 0x08;
			}
			last_hcg_reg1[id] = 0x30;
			last_hcg_reg2[id] = 0x08;
		}

		if ((last_hcg_reg1[id] != hcg_reg1) || (last_hcg_reg2[id] != hcg_reg2) || ((last_change_frame_id[id][1]) == (change_frame_id[1]))) {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x00, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);  

			cmd = sensor_builtin_set_cmd_info(0x376C, 1, hcg_reg1, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3C55, 1, hcg_reg2, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			flag = 1;
		} else {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x00, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			flag = 0;
		}
		last_change_frame_id[id][0] = change_frame_id[0];
		last_change_frame_id[id][1] = change_frame_id[1];
	}	

	if (!isp_builtin_get_shdr_enable(id)) {
		// Write sensor gain
		// Set analog gain
		cmd = sensor_builtin_set_cmd_info(0x3508, 1, (data1[0] >> 4) & 0x0F, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		cmd = sensor_builtin_set_cmd_info(0x3509, 1, (data1[0] & 0x0F) << 4, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		// Set Digital gain
		cmd = sensor_builtin_set_cmd_info(0x350A, 1, (data2[0] >> 10) & 0x0F, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350B, 1, (data2[0] >> 2) & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350C, 1, (data2[0] & 0x03) << 6, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x10, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x320d, 1, 0x00, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		if (flag) {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0xA0, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3504, 1, 0x08, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		} else {
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0xE0, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		}
	} else {
		if (total_frame == 2) {
			// Long Gain
			cmd = sensor_builtin_set_cmd_info(0x3508, 1, (data1[0] >> 4) & 0x0F, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3509, 1, (data1[0] & 0x0F) << 4, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			// Short Gain
			cmd = sensor_builtin_set_cmd_info(0x3548, 1, (data1[1] >> 4) & 0x0F, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3549, 1, (data1[1] & 0x0F) << 4, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			// Set Long Digital gain
			cmd = sensor_builtin_set_cmd_info(0x350A, 1, (data2[0] >> 10) & 0x0F, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x350B, 1, (data2[0] >> 2) & 0xFF, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x350C, 1, (data2[0] & 0x03) << 6, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			// Set Short Digital gain
			cmd = sensor_builtin_set_cmd_info(0x354A, 1, (data2[1] >> 10) & 0x0F, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x354B, 1, (data2[1] >> 2) & 0xFF, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x354C, 1, (data2[1] & 0x03) << 6, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0x10, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x320d, 1, 0x00, 0x0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			if (flag) {
				cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0xA0, 0x0);
				sensor_builtin_write_i2c_2b1b(id, &cmd);
				cmd = sensor_builtin_set_cmd_info(0x3504, 1, 0x08, 0x0);
				sensor_builtin_write_i2c_2b1b(id, &cmd);
			} else {
				cmd = sensor_builtin_set_cmd_info(0x3208, 1, 0xE0, 0x0);
				sensor_builtin_write_i2c_2b1b(id, &cmd);
			}
		}
	}
}

static void sensor_builtin_os02k10_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 t_row = 0, sensor_vd;
	UINT32 sensor_hd = 1440, sensor_dft_vd = 1250, sensor_pclk = 108000000, dft_fps = 3000;

	if (isp_builtin_get_shdr_enable(id)) {
		// NOTE: mode 2
		dft_fps = 3000;
		total_frame = 2;
		sensor_hd = 1224;
		sensor_dft_vd = 1470;
	} else {
		// NOTE: mode 4
		dft_fps = 6000;
		total_frame = 1;
		sensor_hd = 1440;
		sensor_dft_vd = 1250;
	}

	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);

	// Calculate exposure line
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Calculates the exposure setting
		if (isp_builtin_get_shdr_enable(id)) {
			t_row = 100 * sensor_hd / (sensor_pclk /2 / 100000);
		} else {
			t_row = 100 * sensor_hd / (sensor_pclk / 100000);
		}

		sensor_builtin_row_time[id] = t_row;

		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;

		// Limit minimun exposure line
		//#define MIN_EXPOSURE_LINE          1

		if (line[frame_cnt] < 1) {
			line[frame_cnt] = 1;
		}
	}

	// Write exposure line
	cmd = sensor_builtin_set_cmd_info(0x380E, 1, (sensor_vd >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	cmd = sensor_builtin_set_cmd_info(0x380F, 1, sensor_vd & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	if (!isp_builtin_get_shdr_enable(id)) {
		//Check max exp line reg
		//#define MAX_VD_PERIOD              0xFFFF
		//#define NON_EXPOSURE_LINE          8

		if (line[0] > (0xFFFF-8)) {
			printk("max line overflow \r\n");
			line[0] = (0xFFFF-8);
		}

		//Check min exp line
		if (line[0] < 1) {
			printk("min line overflow\r\n");
			line[0] = 1;
		}

		//Check max exp line
		if (line[0] > (sensor_vd - 8)) {
			line[0] = sensor_vd - 8;
		}

		// set exposure line to sensor
		cmd = sensor_builtin_set_cmd_info(0x3501, 1, (line[0] >> 8) & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3502, 1, line[0] & 0xFF , 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	} else {
		if (total_frame == 2) {
			if (line[0] < 1) {
				printk("long exp min line overflow\r\n");
				line[0] = 1;
			}
			if (line[1] < 1) {
				printk("short exp min line overflow\r\n");
				line[1] = 1;
			}

			//#define NON_2HDR_EXPOSURE_LINE     10
			if ((line[0]+line[1]) > (sensor_vd - 10)) {
				line[0] = sensor_vd - 10 - line[1];
			}
			// set exposure line to sensor
			//L exp
			cmd = sensor_builtin_set_cmd_info(0x3501, 1, ( line[0] >> 8 ) & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3502, 1, line[0] & 0xFF , 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			//S exp
			cmd = sensor_builtin_set_cmd_info(0x3541, 1, ( line[1] >> 8 ) & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3542, 1, line[1] & 0xFF , 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		}
	}

}

static void sensor_builtin_os05a10_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data3[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 data = 0;
	UINT32 a_gain = 0, d_gain = 0, blc_trigger = 0, trigger_value = 0;
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	// Calculate sensor gain
	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
	} else {
		total_frame = 1;
	}

	// Min:1000, Max:248000
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		if (gain_info.gain_ratio[frame_cnt] < 1000) {
			gain_info.gain_ratio[frame_cnt] = 1000;
		} else if (gain_info.gain_ratio[frame_cnt] > 248000) {
			gain_info.gain_ratio[frame_cnt] = 248000;
		}

		if ( 6400 < (gain_info.gain_ratio[frame_cnt])) {
			blc_trigger = 0;
		} else {
			blc_trigger = 1;
		}

		if ( 15500 >= gain_info.gain_ratio[frame_cnt] ) {
			d_gain=0x0400;
			a_gain= (gain_info.gain_ratio[frame_cnt]) * 128 / 1000;
		} else {
			d_gain=(gain_info.gain_ratio[frame_cnt]) * 1024 / 15500;
			a_gain= 0x07c0;
		}

		if (0x3FFF < d_gain) { 
			d_gain = 0x3FFF;
		}

		data1[frame_cnt] = a_gain;
		data2[frame_cnt] = d_gain;
		data3[frame_cnt] = blc_trigger;
	}

	if (!isp_builtin_get_shdr_enable(id)) {
		// Write sensor gain
		// Set analog gain
		cmd = sensor_builtin_set_cmd_info(0x3508, 1, ((data1[0]) >> 8) & 0x07, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3509, 1, (data1[0])& 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		// Set digital gain
		cmd = sensor_builtin_set_cmd_info(0x350A, 1, ((data2[0]) >> 8) & 0x3F, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350B, 1, (data2[0])& 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	} else {
		// Set long analog gain
		cmd = sensor_builtin_set_cmd_info(0x3508, 1, ((data1[0]) >> 8 & 0x07 ), 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3509, 1, data1[0] & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		// Set short analog gain
		cmd = sensor_builtin_set_cmd_info(0x350C, 1, ((data1[1]) >> 8 & 0x07 ), 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350D, 1, data1[1] & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		// Set long digital gain
		cmd = sensor_builtin_set_cmd_info(0x350A, 1, ( data2[0] >> 8 & 0x3F ), 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350B, 1, data2[0] & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);

		// Set short digital gain
		cmd = sensor_builtin_set_cmd_info(0x350E, 1, ( data2[1] >> 8 & 0x3F ), 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x350F, 1, data2[1] & 0xFF, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	}
	// Set BLC trigger
	cmd = sensor_builtin_set_cmd_info(0x4000, 1, 0x00, 0x0);

	sensor_builtin_read_i2c_2b1b(id, &cmd);
	data = cmd.data[0];

	if (data3[0]) {
		trigger_value = data & 0x7F;
	} else {
		trigger_value = data | 0x80;
	}

	if ( data != trigger_value) { 
		cmd = sensor_builtin_set_cmd_info(0x4000, 1, trigger_value, 0x0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	}
}

static void sensor_builtin_os05a10_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 t_row = 0;
	UINT32 l2s_distance = 0, y_out_size = 0, sensor_vd;
	UINT32 sensor_hd = 1540, sensor_dft_vd = 2337, sensor_pclk = 108000000, act_size_y = 2, act_size_h = 1944, dft_fps = 3000;

	if (isp_builtin_get_shdr_enable(id)) {
		total_frame = 2;
		sensor_hd = 720;
		sensor_dft_vd = 2496;
		sensor_pclk = 54000000;
	} else {
		total_frame = 1;
		sensor_hd = 1540;
		sensor_dft_vd = 2337;
		sensor_pclk = 108000000;
	}

	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);

	// Calculate exposure line
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Calculates the exposure setting
		if (isp_builtin_get_shdr_enable(id)) {
			t_row = 100 * sensor_hd / (sensor_pclk /2 / 100000);
		} else {
			t_row = 100 * sensor_hd / (sensor_pclk / 100000);
		}

		sensor_builtin_row_time[id] = t_row;

		line[frame_cnt] = sensor_ctrl->exp_time[frame_cnt] * 10 / t_row;

		//#define MIN_EXPOSURE_LINE          4
		// Limit minimun exposure line
		if (line[frame_cnt] < 4) {
			line[frame_cnt] = 4;
		}
	}

	cmd = sensor_builtin_set_cmd_info(0x380E, 1, (sensor_vd >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	cmd = sensor_builtin_set_cmd_info(0x380F, 1, sensor_vd & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	if (!isp_builtin_get_shdr_enable(id)) {
		//Check max exp line reg
		//#define MAX_VD_PERIOD              0xFFFF
		//#define MIN_EXPOSURE_LINE          4
		//#define NON_EXPOSURE_LINE          8
		//#define HDR_NON_EXPOSURE_LINE      4
		if (line[0] > (0xFFFF-8)) {
			printk("max line overflow \r\n");
			line[0] = (0xFFFF-8);
		}

		//Check min exp line
		if (line[0] < 4) {
			printk("min line overflow\r\n");
			line[0] = 4;
		}

		//Check max exp line
		if (line[0] > (sensor_vd - 8)) {
			line[0] = sensor_vd - 8;
		}

		// set exposure line to sensor
		cmd = sensor_builtin_set_cmd_info(0x3501, 1, ( line[0] >> 8 ) & 0xFF, 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
		cmd = sensor_builtin_set_cmd_info(0x3502, 1, line[0] & 0xFF , 0);
		sensor_builtin_write_i2c_2b1b(id, &cmd);
	} else {
		if (total_frame == 2) {
			if (line[0] > (sensor_vd - 4)) {
				line[0] = sensor_vd - 4;
			}

			if (line[1] > (sensor_vd - 4)) {
				line[1] = sensor_vd - 4;
			}

			if (line[1] >= line[0]) {
				printk("error T1(%d) must be > T2(%d)\r\n", line[0], line[1]);
			}

			if (((line[1]) + sensor_vd) >= 0xFFFF) {
				if(sensor_vd < 0xFFFA) {
					line[1] = 0xFFFF - sensor_vd -1;
				} else {
					sensor_vd = 0xFFFA;
					line[1] = 4;
				}
			}

			if ((line[1]) > 255) {
				if ((line[1] + 8) >= 0xFFFF ) {
					l2s_distance = 0xFFFF;
				} else {
					l2s_distance = line[1] + 8;
				}
			} else {
				l2s_distance = line[1] + 2;
			}

			y_out_size = act_size_y + act_size_h;

			//#define BLACKLINE 24
			//#define L2SOFFSET  8
			if (l2s_distance > sensor_vd - (y_out_size+24+8)) {
				l2s_distance = sensor_vd - (y_out_size+24+8);
			}

			if (((line[0])+l2s_distance+4)>= sensor_vd) {
				line[0] = sensor_vd-l2s_distance-5;
			}

			if (sensor_vd <= ((line[0])+(line[1])+8)) {
				line[1] = sensor_vd - line[0] - 9;
			}

			//Set long exposure line to sensor
			cmd = sensor_builtin_set_cmd_info(0x3501, 1, ( line[0] >> 8 ) & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3502, 1, ( line[0] & 0xFF), 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			//Set short exposure line to sensor
			cmd = sensor_builtin_set_cmd_info(0x3511, 1, ( line[1] >> 8 ) & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3512, 1, ( line[1] & 0xFF), 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);

			//Set L2S-distance
			cmd = sensor_builtin_set_cmd_info(0x3798, 1, ( l2s_distance >> 8 ) & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
			cmd = sensor_builtin_set_cmd_info(0x3799, 1, l2s_distance & 0xFF, 0);
			sensor_builtin_write_i2c_2b1b(id, &cmd);
		}
	}
}

typedef struct _GC4653_GAIN_TABLE {
	UINT32 gain_reg1;
	UINT32 gain_reg2;
	UINT32 gain_reg3;
	UINT32 gain_reg4;
	UINT32 gain_reg5;
	UINT32 gain_reg6;
	UINT32 gain_reg7;
	UINT32 total_gain;
} GC4653_GAIN_TABLE;

static GC4653_GAIN_TABLE gc4653_gain_table[] = // gain*1000
{
	//2b3   2b4   2b8    2b9   515    519   2d9
	{0x00, 0x00, 0x01, 0x00, 0x30, 0x1e, 0x5C,  1000},
	{0x20, 0x00, 0x01, 0x0B, 0x30, 0x1e, 0x5C,  1172},
	{0x01, 0x00, 0x01, 0x19, 0x30, 0x1d, 0x5B,  1391},
	{0x21, 0x00, 0x01, 0x2A, 0x30, 0x1e, 0x5C,  1656},
	{0x02, 0x00, 0x02, 0x00, 0x30, 0x1e, 0x5C,  2000},
	{0x22, 0x00, 0x02, 0x17, 0x30, 0x1d, 0x5B,  2359},
	{0x03, 0x00, 0x02, 0x33, 0x20, 0x16, 0x54,  2797},
	{0x23, 0x00, 0x03, 0x14, 0x20, 0x17, 0x55,  3313},
	{0x04, 0x00, 0x04, 0x00, 0x20, 0x17, 0x55,  4000},
	{0x24, 0x00, 0x04, 0x2F, 0x20, 0x19, 0x57,  4734},
	{0x05, 0x00, 0x05, 0x26, 0x20, 0x19, 0x57,  5594},
	{0x25, 0x00, 0x06, 0x28, 0x20, 0x1b, 0x59,  6625},
	{0x0c, 0x00, 0x08, 0x00, 0x20, 0x1d, 0x5B,  8000},
	{0x2C, 0x00, 0x09, 0x1E, 0x20, 0x1f, 0x5D,  9469},
	{0x0D, 0x00, 0x0B, 0x0C, 0x20, 0x21, 0x5F, 11188},
	{0x2D, 0x00, 0x0D, 0x11, 0x20, 0x24, 0x62, 13266},
	{0x1C, 0x00, 0x10, 0x00, 0x20, 0x26, 0x64, 16000},
	{0x3C, 0x00, 0x12, 0x3D, 0x18, 0x2a, 0x68, 18953},
	{0x5C, 0x00, 0x16, 0x19, 0x18, 0x2c, 0x6A, 22391},
	{0x7C, 0x00, 0x1A, 0x22, 0x18, 0x2e, 0x6C, 26531},
	{0x9C, 0x00, 0x20, 0x00, 0x18, 0x32, 0x70, 32000},
	{0xBC, 0x00, 0x25, 0x3A, 0x18, 0x35, 0x73, 37906},
	{0xDC, 0x00, 0x2C, 0x33, 0x10, 0x36, 0x74, 44797},
	{0xFC, 0x00, 0x35, 0x05, 0x10, 0x38, 0x76, 53078},
	{0x1C, 0x01, 0x40, 0x00, 0x10, 0x3c, 0x7A, 64000},
	{0x3C, 0x01, 0x4B, 0x35, 0x10, 0x42, 0x80, 75828}
};
#define NUM_OF_GAINSET (sizeof(gc4653_gain_table) / sizeof(GC4653_GAIN_TABLE))

static void sensor_builtin_gc4653_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 i, tbl_idx = NUM_OF_GAINSET-1, digital_gain = 0, decimal = 0, gain_data[3];
	SENSOR_BUILTIN_I2C_CMD cmd;
	ISP_BUILTIN_SENSOR_CTRL gain_info;

	memcpy(&gain_info, sensor_ctrl, sizeof(ISP_BUILTIN_SENSOR_CTRL));

	if (gain_info.gain_ratio[0] < 1000) {
		gain_info.gain_ratio[0] = 1000;
	} else if (gain_info.gain_ratio[0] > 75828*8) {
		gain_info.gain_ratio[0] = 75828*8;
	}

	for (i = 0; i <= (NUM_OF_GAINSET-1); i++) {
		if (gc4653_gain_table[i].total_gain > gain_info.gain_ratio[0]) {
			tbl_idx = i;
			break;
		}
	}

	// collect the gain setting
	if (tbl_idx < 1) {
		digital_gain = gain_info.gain_ratio[0] * 1000 / gc4653_gain_table[0].total_gain;
		gain_data[0] = (gc4653_gain_table[0].gain_reg6 << 16) | (gc4653_gain_table[0].gain_reg1 << 8) | (gc4653_gain_table[0].gain_reg2);
		gain_data[1] = (gc4653_gain_table[0].gain_reg7 << 16) | (gc4653_gain_table[0].gain_reg3 << 8) | (gc4653_gain_table[0].gain_reg4);
		gain_data[2] = (gc4653_gain_table[0].gain_reg5 << 16);
	} else {
		digital_gain = gain_info.gain_ratio[0] * 1000 / gc4653_gain_table[tbl_idx-1].total_gain;
		gain_data[0] = (gc4653_gain_table[tbl_idx-1].gain_reg6 << 16) | (gc4653_gain_table[tbl_idx-1].gain_reg1 << 8) | (gc4653_gain_table[tbl_idx-1].gain_reg2);
		gain_data[1] = (gc4653_gain_table[tbl_idx-1].gain_reg7 << 16) | (gc4653_gain_table[tbl_idx-1].gain_reg3 << 8) | (gc4653_gain_table[tbl_idx-1].gain_reg4);
		gain_data[2] = (gc4653_gain_table[tbl_idx-1].gain_reg5 << 16);
	}

	if (digital_gain <= 1000) {
		decimal = 64;
	} else {
		decimal = digital_gain * 64 / 1000;
	}

	cmd = sensor_builtin_set_cmd_info(0x02B3, 1, (gain_data[0] >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x02B4, 1, gain_data[0] & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x02B8, 1, (gain_data[1] >> 8) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x02B9, 1, gain_data[1] & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x0515, 1, (gain_data[2] >> 16) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x0519, 1, (gain_data[0] >> 16) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x02D9, 1, (gain_data[1] >> 16) & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x020E, 1, (decimal >> 6) & 0x0F, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x020F, 1, (decimal & 0x3F) << 2, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
}

static void sensor_builtin_gc4653_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	UINT32 line = 0;
	SENSOR_BUILTIN_I2C_CMD cmd;
	UINT32 t_row = 0, sensor_vd;
	UINT32 sensor_hd = 4800, sensor_dft_vd = 1500, sensor_pclk = 216000000, dft_fps = 3000;

	// calculate exposure line
	t_row = 100 * sensor_hd / (sensor_pclk / 100000);
	if (t_row == 0) {
		printk("t_row = 0, must >= 1 \r\n");
		t_row = 1;
	}

	sensor_builtin_row_time[id] = t_row;

	line = sensor_ctrl->exp_time[0] * 10 / t_row;
	// limit minimun exposure line
	if (line < 2) {
		line = 2;
	}

	// calculate new vd
	sensor_vd = sensor_dft_vd * dft_fps / isp_builtin_get_chgmode_fps(id);

	// check max vts
	if (sensor_vd > 0x3FFF) {
		printk("max vts overflow \r\n");
		sensor_vd = 0x3FFF;
	}

	// write change mode VD
	cmd = sensor_builtin_set_cmd_info(0x0340, 1, (sensor_vd >> 8) & 0x3F, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x0341, 1, sensor_vd & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);

	// check max exp line reg
	if (line > 0x3FFF) {
		printk("max line overflow \r\n");
		line = 0x3FFF;
	}

	// check max exp line
	if (line > (sensor_vd - 4)) {
		line = sensor_vd - 4;
	}

	// set exposure line to sensor
	cmd = sensor_builtin_set_cmd_info(0x0202, 1, (line >> 8) & 0x3F, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
	cmd = sensor_builtin_set_cmd_info(0x0203, 1, line & 0xFF, 0);
	sensor_builtin_write_i2c_2b1b(id, &cmd);
}

//=============================================================================
// external functions
//=============================================================================
UINT32 sensor_builtin_get_row_time(UINT32 id)
{
	return sensor_builtin_row_time[id];
}

void sensor_builtin_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	if (isp_builtin_get_sensor_name(id) == 0) {
		sensor_builtin_imx290_set_gain(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 1) {
		sensor_builtin_f37_set_gain(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 2) {
		sensor_builtin_os02k10_set_gain(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 3) {
		sensor_builtin_os05a10_set_gain(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 4) {
		sensor_builtin_f35_set_gain(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 5) {
		sensor_builtin_gc4653_set_gain(id, sensor_ctrl);
	}
}

void sensor_builtin_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl)
{
	if (isp_builtin_get_sensor_name(id) == 0) {
		sensor_builtin_imx290_set_expt(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 1) {
		sensor_builtin_f37_set_expt(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 2) {
		sensor_builtin_os02k10_set_expt(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 3) {
		sensor_builtin_os05a10_set_expt(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 4) {
		sensor_builtin_f35_set_expt(id, sensor_ctrl);
	} else if (isp_builtin_get_sensor_name(id) == 5) {
		sensor_builtin_gc4653_set_expt(id, sensor_ctrl);
	}
}

INT32 sensor_builtin_set_transfer_i2c(UINT32 id, void *msgs, INT32 num)
{
	return sensor_builtin_transfer_i2c(id, msgs, num);
}

void sensor_builtin_init_i2c(UINT32 id)
{
	sensor_builtin_i2c_device[id].addr = isp_builtin_get_i2c_addr(id);

	if (i2c_new_device(i2c_get_adapter(isp_builtin_get_i2c_id(id)), &sensor_builtin_i2c_device[id]) == NULL) {
		printk(" i2c_new_device fail. \r\n");
		return;
	}

	// bind i2c client driver to i2c bus
	if (i2c_add_driver(&sensor_builtin_i2c_driver[id]) != 0) {
		printk(" i2c_add_driver fail. \r\n");
		return;
	}
}

void sensor_builtin_uninit_i2c(UINT32 id)
{
	i2c_unregister_device(sensor_builtin_i2c_info[id]->iic_client);
	i2c_del_driver(&sensor_builtin_i2c_driver[id]);
}

#endif
#endif

