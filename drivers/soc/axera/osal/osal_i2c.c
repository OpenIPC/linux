/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/kmod.h>
#include <linux/i2c-dev.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include "osal_ax.h"
#include "osal_dev_ax.h"
#include "axdev.h"
#include "axdev_log.h"
#include "osal_lib_ax.h"

#define I2C_MAX_NUM     (16)

static struct i2c_board_info ax_i2c_info = {
	I2C_BOARD_INFO("sensor_i2c", (0x6c)),
};

static struct i2c_client *sensor_client[I2C_MAX_NUM];

int AX_OSAL_DEV_i2c_write(unsigned char i2c_dev, unsigned char dev_addr,
			     unsigned int reg_addr, unsigned int reg_addr_num,
			     unsigned int data, unsigned int data_byte_num)
{
	int ret = 0;
	int idx = 0;
	int msg_count = 0;
	struct i2c_client client;
	unsigned char tmp_buf[8] = { 0 };
	struct i2c_msg msgs[1] = { 0 };

	if (I2C_MAX_NUM <= i2c_dev) {
		return -EIO;
	}

	if (NULL == sensor_client[i2c_dev]) {
		return -EIO;
	}

	AX_OSAL_LIB_memcpy(&client, sensor_client[i2c_dev], sizeof(struct i2c_client));
	client.addr = dev_addr;	/* notes */

	/* reg_addr config */
	if (reg_addr_num == 1) {
		tmp_buf[idx++] = reg_addr & 0xff;
	} else {
		tmp_buf[idx++] = (reg_addr >> 8) & 0xff;
		tmp_buf[idx++] = reg_addr & 0xff;
	}

	/* data config */
	if (data_byte_num == 1) {
		tmp_buf[idx++] = data & 0xff;
	} else {
		tmp_buf[idx++] = (data >> 8) & 0xff;
		tmp_buf[idx++] = data & 0xff;
	}

	msgs[0].addr = client.addr;
	msgs[0].flags = 0;
	msgs[0].len = idx;
	msgs[0].buf = tmp_buf;

	msg_count = 1;
	ret = i2c_transfer(client.adapter, msgs, msg_count);
	if (ret < 0) {
		printk("msg %s i2c write error: %d\n", __func__, ret);
		return -EIO;
	}

	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_i2c_write);

int AX_OSAL_DEV_i2c_read(unsigned char i2c_dev, unsigned char dev_addr,
			    unsigned int reg_addr, unsigned int reg_addr_num,
			    unsigned int *pRegData, unsigned int data_byte_num)
{
	int ret = 0;
	int idx = 0;
	struct i2c_client client;
	int msg_count = 0;
	unsigned char tmp_buf[4] = { 0 };
	struct i2c_msg msgs[2] = { 0 };

	if (I2C_MAX_NUM <= i2c_dev) {
		return -EIO;
	}
	if (NULL == sensor_client[i2c_dev]) {
		return -EIO;
	}
	if (NULL == pRegData) {
		return -EIO;
	}

	AX_OSAL_LIB_memcpy(&client, sensor_client[i2c_dev], sizeof(struct i2c_client));
	client.addr = dev_addr;	/* notes */

	/* reg_addr config */
	if (reg_addr_num == 1) {
		tmp_buf[idx++] = reg_addr & 0xff;
	} else {
		tmp_buf[idx++] = (reg_addr >> 8) & 0xff;
		tmp_buf[idx++] = reg_addr & 0xff;
	}

	msgs[0].addr = client.addr;
	msgs[0].flags = 0;
	msgs[0].len = reg_addr_num;
	msgs[0].buf = tmp_buf;

	msgs[1].addr = client.addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = data_byte_num;
	msgs[1].buf = tmp_buf;

	msg_count = sizeof(msgs) / sizeof(msgs[0]);
	ret = i2c_transfer(client.adapter, msgs, msg_count);
	if (ret != 2) {
		printk("msg %s i2c write error: %d\n", __func__, ret);
		return -EIO;
	}

	if (data_byte_num == 1) {

		*pRegData = tmp_buf[0];
	} else {
		*pRegData = (tmp_buf[0] << 8) | (tmp_buf[1]);
	}

	return ret;
}

EXPORT_SYMBOL(AX_OSAL_DEV_i2c_read);

int AX_OSAL_DEV_i2c_dev_init(void)
{
	int i = 0;
	struct i2c_adapter *i2c_adap = NULL;

	for (i = 0; i < I2C_MAX_NUM; i++) {
		i2c_adap = i2c_get_adapter(i);
		if (NULL != i2c_adap) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
			sensor_client[i] = i2c_new_client_device(i2c_adap, &ax_i2c_info);
#else
			sensor_client[i] = i2c_new_device(i2c_adap, &ax_i2c_info);
#endif
			i2c_put_adapter(i2c_adap);
		}
	}
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DEV_i2c_dev_init);

void AX_OSAL_DEV_i2c_dev_exit(void)
{
	int i = 0;
	for (i = 0; i < I2C_MAX_NUM; i++) {
		if (NULL != sensor_client[i]) {
			i2c_unregister_device(sensor_client[i]);
		}
	}
	return;
}

EXPORT_SYMBOL(AX_OSAL_DEV_i2c_dev_exit);
