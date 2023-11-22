/* SPDX-License-Identifier: GPL-2.0 */
/*
 * remote processor messaging bus internals
 *
 * Copyright (c) 2020 The Linux Foundation.
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 */

#ifndef __RPMSG_INTERNAL_H__
#define __RPMSG_INTERNAL_H__

#include <linux/rpmsg.h>
#include <linux/poll.h>

/* The feature bitmap for virtio rpmsg */
#define VIRTIO_RPMSG_F_NS	0 /* RP supports name service notifications */
#define VIRTIO_RPMSG_F_AS	1 /* RP supports address service notifications */

/**
 * struct rpmsg_hdr - common header for all rpmsg messages
 * @src: source address
 * @dst: destination address
 * @reserved: reserved for future use
 * @len: length of payload (in bytes)
 * @flags: message flags
 * @data: @len bytes of message payload data
 *
 * Every message sent(/received) on the rpmsg bus begins with this header.
 */
struct rpmsg_hdr {
	u32 src;
	u32 dst;
	u32 reserved;
	u16 len;
	u16 flags;
	u8 data[0];
} __packed;

/**
 * struct rpmsg_ns_msg - dynamic name service announcement message
 * @name: name of remote service that is published
 * @addr: address of remote service that is published
 * @flags: indicates whether service is created or destroyed
 *
 * This message is sent across to publish a new service, or announce
 * about its removal. When we receive these messages, an appropriate
 * rpmsg channel (i.e device) is created/destroyed. In turn, the ->probe()
 * or ->remove() handler of the appropriate rpmsg driver will be invoked
 * (if/as-soon-as one is registered).
 */
struct rpmsg_ns_msg {
	char name[RPMSG_NAME_SIZE];
	u32 addr;
	u32 flags;
} __packed;

/**
 * struct rpmsg_as_msg - dynamic address service announcement message
 * @name: name of the created channel
 * @dst: destination address to be used by the backend rpdev
 * @src: source address of the backend rpdev (the one that sent name service
 * announcement message)
 * @flags: indicates whether service is created or destroyed
 *
 * This message is sent (by virtio_rpmsg_bus) when a new channel is created
 * in response to name service announcement message by backend rpdev to create
 * a new channel. This sends the allocated source address for the channel
 * (destination address for the backend rpdev) to the backend rpdev.
 */
struct rpmsg_as_msg {
	char name[RPMSG_NAME_SIZE];
	u32 dst;
	u32 src;
	u32 flags;
} __packed;

/**
 * enum rpmsg_ns_flags - dynamic name service announcement flags
 *
 * @RPMSG_NS_CREATE: a new remote service was just created
 * @RPMSG_NS_DESTROY: a known remote service was just destroyed
 */
enum rpmsg_ns_flags {
	RPMSG_NS_CREATE		= 0,
	RPMSG_NS_DESTROY	= 1,
	RPMSG_AS_ANNOUNCE	= 2,
};

/**
 * enum rpmsg_as_flags - dynamic address service announcement flags
 *
 * @RPMSG_AS_ASSIGN: address has been assigned to the newly created channel
 * @RPMSG_AS_FREE: assigned address is freed from the channel and no longer can
 * be used
 */
enum rpmsg_as_flags {
	RPMSG_AS_ASSIGN		= 1,
	RPMSG_AS_FREE		= 2,
};

/*
 * We're allocating buffers of 512 bytes each for communications. The
 * number of buffers will be computed from the number of buffers supported
 * by the vring, upto a maximum of 512 buffers (256 in each direction).
 *
 * Each buffer will have 16 bytes for the msg header and 496 bytes for
 * the payload.
 *
 * This will utilize a maximum total space of 256KB for the buffers.
 *
 * We might also want to add support for user-provided buffers in time.
 * This will allow bigger buffer size flexibility, and can also be used
 * to achieve zero-copy messaging.
 *
 * Note that these numbers are purely a decision of this driver - we
 * can change this without changing anything in the firmware of the remote
 * processor.
 */
#define MAX_RPMSG_NUM_BUFS	(512)
#define MAX_RPMSG_BUF_SIZE	(512)

/*
 * Local addresses are dynamically allocated on-demand.
 * We do not dynamically assign addresses from the low 1024 range,
 * in order to reserve that address range for predefined services.
 */
#define RPMSG_RESERVED_ADDRESSES	(1024)

/* Address 53 is reserved for advertising remote services */
#define RPMSG_NS_ADDR			(53)

/* Address 54 is reserved for advertising address services */
#define RPMSG_AS_ADDR			(54)
#define to_rpmsg_device(d) container_of(d, struct rpmsg_device, dev)
#define to_rpmsg_driver(d) container_of(d, struct rpmsg_driver, drv)

/**
 * struct rpmsg_device_ops - indirection table for the rpmsg_device operations
 * @create_ept:		create backend-specific endpoint, required
 * @announce_create:	announce presence of new channel, optional
 * @announce_destroy:	announce destruction of channel, optional
 *
 * Indirection table for the operations that a rpmsg backend should implement.
 * @announce_create and @announce_destroy are optional as the backend might
 * advertise new channels implicitly by creating the endpoints.
 */
struct rpmsg_device_ops {
	struct rpmsg_endpoint *(*create_ept)(struct rpmsg_device *rpdev,
					    rpmsg_rx_cb_t cb, void *priv,
					    struct rpmsg_channel_info chinfo);

	int (*announce_create)(struct rpmsg_device *ept);
	int (*announce_destroy)(struct rpmsg_device *ept);
};

/**
 * struct rpmsg_endpoint_ops - indirection table for rpmsg_endpoint operations
 * @destroy_ept:	see @rpmsg_destroy_ept(), required
 * @send:		see @rpmsg_send(), required
 * @sendto:		see @rpmsg_sendto(), optional
 * @send_offchannel:	see @rpmsg_send_offchannel(), optional
 * @trysend:		see @rpmsg_trysend(), required
 * @trysendto:		see @rpmsg_trysendto(), optional
 * @trysend_offchannel:	see @rpmsg_trysend_offchannel(), optional
 * @poll:		see @rpmsg_poll(), optional
 * @get_signals:	see @rpmsg_get_signals(), optional
 * @set_signals:	see @rpmsg_set_signals(), optional
 *
 * Indirection table for the operations that a rpmsg backend should implement.
 * In addition to @destroy_ept, the backend must at least implement @send and
 * @trysend, while the variants sending data off-channel are optional.
 */
struct rpmsg_endpoint_ops {
	void (*destroy_ept)(struct rpmsg_endpoint *ept);

	int (*send)(struct rpmsg_endpoint *ept, void *data, int len);
	int (*sendto)(struct rpmsg_endpoint *ept, void *data, int len, u32 dst);
	int (*send_offchannel)(struct rpmsg_endpoint *ept, u32 src, u32 dst,
				  void *data, int len);

	int (*trysend)(struct rpmsg_endpoint *ept, void *data, int len);
	int (*trysendto)(struct rpmsg_endpoint *ept, void *data, int len, u32 dst);
	int (*trysend_offchannel)(struct rpmsg_endpoint *ept, u32 src, u32 dst,
			     void *data, int len);
	__poll_t (*poll)(struct rpmsg_endpoint *ept, struct file *filp,
			     poll_table *wait);
	int (*get_signals)(struct rpmsg_endpoint *ept);
	int (*set_signals)(struct rpmsg_endpoint *ept, u32 set, u32 clear);
};

/**
 * struct rpmsg_virtproc_ops - indirection table for rpmsg_virtproc operations
 * @create_channel: Create a new rpdev channel
 * @delete_channel: Delete the rpdev channel
 * @owner: Owner of the module holding the ops
 */
struct rpmsg_virtproc_ops {
	struct device *(*create_channel)(struct device *dev, const char *name);
	void (*delete_channel)(struct device *dev);
	struct module *owner;
};
int rpmsg_register_device(struct rpmsg_device *rpdev);
int rpmsg_unregister_device(struct device *parent,
			    struct rpmsg_channel_info *chinfo);

struct device *rpmsg_find_device(struct device *parent,
				 struct rpmsg_channel_info *chinfo);

struct config_group *
rpmsg_cfs_add_virtproc_group(struct device *dev,
			     const struct rpmsg_virtproc_ops *ops);

/**
 * rpmsg_chrdev_register_device() - register chrdev device based on rpdev
 * @rpdev:	prepared rpdev to be used for creating endpoints
 *
 * This function wraps rpmsg_register_device() preparing the rpdev for use as
 * basis for the rpmsg chrdev.
 */
static inline int rpmsg_chrdev_register_device(struct rpmsg_device *rpdev)
{
	strcpy(rpdev->id.name, "rpmsg_chrdev");
	rpdev->driver_override = "rpmsg_chrdev";

	return rpmsg_register_device(rpdev);
}

#endif
