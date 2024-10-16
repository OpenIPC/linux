/*
 * xr806/cmd_proto.h
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * cmd protocol implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __XR_CMD_PROTO_H__
#define __XR_CMD_PROTO_H__

/*XR_WIFI_HOST_HAND_WAY */
struct cmd_para_hand_way {
	u8 id;
};

/*XR_WIFI_HOST_DATA_TEST */
struct cmd_para_data_test {
	u16 len;
	u8 data[0];
};

/*XR_WIFI_HOST_HAND_WAY_RES */
struct cmd_para_hand_way_res {
	u8 id;
	u8 mac[6];
	u8 ip_addr[4];
	u8 netmask[4];
	u8 gw[4];
};

struct cmd_para_data_test_res {
	u16 len;
	u8 data[0];
};

/* opcode */
enum cmd_host_opcode {
	XR_WIFI_HOST_HAND_WAY = 0,
	XR_WIFI_HOST_DATA_TEST,
	XR_WIFI_HOST_KERNEL_MAX,
};

enum cmd_dev_opcode {
	XR_WIFI_DEV_HAND_WAY_RES = 0,
	XR_WIFI_DEV_DATA_TEST_RES,
	XR_WIFI_DEV_KERNEL_MAX,
};

/* config payload */
struct cmd_payload {
	u16 type;
	u16 len;
	u8 param[0];
};

#define CMD_HEAD_SIZE (sizeof(struct cmd_payload))
#endif
