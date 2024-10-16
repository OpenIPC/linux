/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-rp/vin_rp.c
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>

#include "vin_rp.h"
#include "../vin-isp/sunxi_isp.h"
#include "../platform/platform_cfg.h"

extern struct isp_dev *glb_isp[VIN_MAX_ISP];

/*isp rpbuf*/
static void isp_rpbuf_available_cb(struct rpbuf_buffer *buffer, void *priv)
{
}

static int isp_rpbuf_rx_cb(struct rpbuf_buffer *buffer,
				    void *data, int data_len, void *priv)
{
	struct isp_dev *isp = priv;
	void *buf_va = rpbuf_buffer_va(buffer);

	if (data_len > ISP_LOAD_DRAM_SIZE) {
		vin_err("melis ask for 0x%x data, it more than isp load_data 0x%x\n", data_len, ISP_LOAD_DRAM_SIZE);
		return -EINVAL;
	}

	isp->load_flag = 1;
	memcpy(&isp->load_shadow[0], buf_va, data_len);

	vin_log(VIN_LOG_RP, "receive load buffer from melis\n");

	return 0;
}

static int isp_rpbuf_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	return 0;
}

static const struct rpbuf_buffer_cbs isp_rpbuf_cbs = {
	.available_cb = isp_rpbuf_available_cb,
	.rx_cb = isp_rpbuf_rx_cb,
	.destroyed_cb = isp_rpbuf_destroyed_cb,
};

int isp_save_rpbuf_send(struct isp_dev *isp, void *vir_buf)
{
	int ret;
	const char *buf_name;
	void *buf_va;
	int buf_len, offset;
	int data_len = ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_STATISTIC_SIZE;

	if (!isp->save_buffer) {
		vin_err("save rpbuf is null\n");
		return -ENOENT;
	}

	buf_name = rpbuf_buffer_name(isp->save_buffer);
	buf_va = rpbuf_buffer_va(isp->save_buffer);
	buf_len = rpbuf_buffer_len(isp->save_buffer);

	if (buf_len > roundup(data_len, 0x1000)) {
		vin_err("%s: data size(0x%x) out of buffer length(0x%x)\n",
			buf_name, roundup(data_len, 0x1000), buf_len);
		return -EINVAL;
	}

	if (!rpbuf_buffer_is_available(isp->save_buffer)) {
		vin_err("%s not available\n", buf_name);
		return -EACCES;
	}

	memcpy(buf_va, vir_buf, ISP_STAT_TOTAL_SIZE);
	memcpy(buf_va + ISP_STAT_TOTAL_SIZE, isp->isp_save_load.vir_addr + ISP_SAVE_LOAD_REG_SIZE, ISP_SAVE_LOAD_STATISTIC_SIZE);

	offset = 0;
	ret = rpbuf_transmit_buffer(isp->save_buffer, offset, data_len);
	if (ret < 0) {
		vin_err("%s: rpbuf_transmit_buffer failed\n", buf_name);
		return ret;
	}

	vin_log(VIN_LOG_RP, "send save buffer to melis\n");

	return data_len;
}

int isp_rpbuf_create(struct isp_dev *isp)
{
	const char *load_name, *save_name;
	int load_len, save_len;

	load_name = ISP_RPBUF_LOAD_NAME;
	load_len = roundup(ISP_RPBUF_LOAD_LEN, 0x1000);
	isp->load_buffer = rpbuf_alloc_buffer(isp->controller, load_name, load_len,
						NULL, &isp_rpbuf_cbs, (void *)isp);
	if (!isp->load_buffer) {
		vin_err("rpbuf_alloc_buffer load failed\n");
		return -ENOENT;
	}

	save_name = ISP_RPBUF_SAVE_NAME;
	save_len = roundup(ISP_RPBUF_SAVE_LEN, 0x1000);
	isp->save_buffer = rpbuf_alloc_buffer(isp->controller, save_name, save_len,
						NULL, NULL, (void *)isp);
	if (!isp->save_buffer) {
		vin_err("rpbuf_alloc_buffer save failed\n");
		return -ENOENT;
	}

	return 0;
}

int isp_rpbuf_destroy(struct isp_dev *isp)
{
	int ret;

	ret = rpbuf_free_buffer(isp->load_buffer);
	if (ret < 0) {
		vin_err("rpbuf_free_buffer load failed\n");
		return ret;
	}
	ret = rpbuf_free_buffer(isp->save_buffer);
	if (ret < 0) {
		vin_err("rpbuf_free_buffer save failed\n");
		return ret;
	}
	isp->load_buffer = NULL;
	isp->save_buffer = NULL;

	return 0;
}

/*isp rpmsg*/
int isp_rpmsg_send(struct rpmsg_device *rpdev, void *data, int len)
{
	int ret = 0;

	if (!rpdev)
		return -1;

	vin_log(VIN_LOG_RP, "rpmsg_send, cmd is %d\n", *((unsigned int *)data + 0));

	ret = rpmsg_send(rpdev->ept, data, len);
	if (ret) {
		vin_err("rpmsg_send failed, cmd is %d\n", *((unsigned int *)data + 0));
	}

	return ret;
}

int isp_rpmsg_trysend(struct rpmsg_device *rpdev, void *data, int len)
{
	int ret = 0;

	vin_log(VIN_LOG_RP, "rpmsg_send, cmd is %d\n", *((unsigned int *)data + 0));

	ret = rpmsg_trysend(rpdev->ept, data, len);
	if (ret) {
		vin_err("rpmsg_send failed, cmd is %d\n", *((unsigned int *)data + 0));
	}

	return ret;
}

static int isp_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct isp_dev *isp = priv;
	unsigned int *p = data;
	vin_log(VIN_LOG_RP, "isp_rpmsg_cb, cmd is %d\n", p[0]);

	switch (p[0]) {
	case ISP_REQUEST_SENSOR_INFO:
		isp_config_sensor_info(isp);
		break;
	case ISP_SET_SENSOR_EXP_GAIN:
		isp_sensor_set_exp_gain(isp, data);
		break;
	case ISP_SET_STAT_EN:
		isp_stat_enable(&isp->h3a_stat, p[1]);
		break;
	case ISP_SET_SAVE_AE:
		isp_save_ae(p + 1);
		break;
	}

	return 0;
}

static int isp_rpmsg_probe(struct rpmsg_device *rpdev)
{

	glb_isp[0]->rpmsg = rpdev;

	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;
	rpdev->ept->priv = glb_isp[0];

	vin_log(VIN_LOG_RP, "rpmsg probe!\n");

	return 0;
}

static void isp_rpmsg_remove(struct rpmsg_device *rpdev)
{
	glb_isp[0]->rpmsg = NULL;
}

static struct rpmsg_device_id rpmsg_driver_sample_id_table[] = {
	{ .name	= ISP_RPMSG_NAME },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_sample_id_table);

struct rpmsg_driver rpmsg_vin_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_sample_id_table,
	.probe		= isp_rpmsg_probe,
	.callback	= isp_rpmsg_cb,
	.remove 	= isp_rpmsg_remove,
};
EXPORT_SYMBOL(rpmsg_vin_client);
