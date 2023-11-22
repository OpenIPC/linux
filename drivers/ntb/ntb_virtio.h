/* SPDX-License-Identifier: GPL-2.0 */
/**
 * NTB VIRTIO/VHOST Header
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#ifndef __LINUX_NTB_VIRTIO_H
#define __LINUX_NTB_VIRTIO_H

#define VIRTIO_TYPE			0
enum virtio_type {
	TYPE_VIRTIO = 1,
	TYPE_VHOST,
};

#define VHOST_VENDORID			1
#define VHOST_DEVICEID			2
#define VHOST_FEATURES_UPPER		3
#define VHOST_FEATURES_LOWER		4
#define VIRTIO_FEATURES_UPPER		5
#define VIRTIO_FEATURES_LOWER		6
#define VHOST_MW0_SIZE_LOWER		7
#define VHOST_MW0_SIZE_UPPER		8
#define VIRTIO_MW0_LOWER_ADDR		9
#define VIRTIO_MW0_UPPER_ADDR		10
#define VHOST_DEVICE_STATUS		11
#define VHOST_CONFIG_GENERATION		12

#define VHOST_COMMAND			13
enum host_cmd {
	HOST_CMD_NONE,
	HOST_CMD_SET_STATUS,
	HOST_CMD_FINALIZE_FEATURES,
	HOST_CMD_RESET,
};

#define VHOST_COMMAND_STATUS		14
enum host_cmd_status {
	HOST_CMD_STATUS_NONE,
	HOST_CMD_STATUS_OKAY,
	HOST_CMD_STATUS_ERROR,
};

#define VHOST_QUEUE_BASE		15
#define VHOST_QUEUE_NUM_BUFFERS(n)	(VHOST_QUEUE_BASE + (n))

#define VHOST_DEVICE_CFG_SPACE		23

#define NTB_LINK_DOWN_TIMEOUT		10	/* 10 milli-sec */
#define COMMAND_TIMEOUT			1000	/* 1 sec */

#define MAX_VQS				8

#endif /* __LINUX_NTB_VIRTIO_H */
