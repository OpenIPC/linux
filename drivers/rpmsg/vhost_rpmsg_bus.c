// SPDX-License-Identifier: GPL-2.0
/*
 * Vhost-based remote processor messaging bus
 *
 * Based on virtio_rpmsg_bus.c
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 *
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/configfs.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rpmsg.h>
#include <linux/vhost.h>
#include <linux/virtio_ids.h>

#include "rpmsg_internal.h"

/**
 * struct virtproc_info - virtual remote processor state
 * @vdev:	the virtio device
 * @rvq:	rx vhost_virtqueue
 * @svq:	tx vhost_virtqueue
 * @buf_size:   size of one rx or tx buffer
 * @tx_lock:	protects svq, sbufs and sleepers, to allow concurrent senders.
 *		sending a message might require waking up a dozing remote
 *		processor, which involves sleeping, hence the mutex.
 * @endpoints:	idr of local endpoints, allows fast retrieval
 * @endpoints_lock: lock of the endpoints set
 * @sendq:	wait queue of sending contexts waiting for a tx buffers
 * @sleepers:	number of senders that are waiting for a tx buffer
 * @as_ept:	the bus's address service endpoint
 * @nb:		notifier block for receiving notifications from vhost device
 *              driver
 * @list:	maintain list of client drivers bound to rpmsg vhost device
 * @list_lock:  mutex to protect updating the list
 *
 * This structure stores the rpmsg state of a given vhost remote processor
 * device (there might be several virtio proc devices for each physical
 * remote processor).
 */
struct virtproc_info {
	struct vhost_dev *vdev;
	struct vhost_virtqueue *rvq, *svq;
	unsigned int buf_size;
	/* mutex to protect sending messages */
	struct mutex tx_lock;
	/* mutex to protect receiving messages */
	struct mutex rx_lock;
	struct idr endpoints;
	/* mutex to protect receiving accessing idr */
	struct mutex endpoints_lock;
	wait_queue_head_t sendq;
	atomic_t sleepers;
	struct rpmsg_endpoint *as_ept;
	struct notifier_block nb;
	struct list_head list;
	struct device *def_channel;
	/* mutex to protect updating pending rpdev in vrp */
	struct mutex list_lock;
#if defined(CONFIG_ARCH_SSTAR)
	rpmsg_device_type_t dev_type;
	int dev_id;
#endif
};

/**
 * @vrp: the remote processor this channel belongs to
 */
struct vhost_rpmsg_channel {
	struct rpmsg_device rpdev;

	struct virtproc_info *vrp;
};

#define to_vhost_rpmsg_channel(_rpdev) \
	container_of(_rpdev, struct vhost_rpmsg_channel, rpdev)

static void vhost_rpmsg_destroy_ept(struct rpmsg_endpoint *ept);
static int vhost_rpmsg_send(struct rpmsg_endpoint *ept, void *data, int len);
static int vhost_rpmsg_sendto(struct rpmsg_endpoint *ept, void *data, int len,
			      u32 dst);
static int vhost_rpmsg_send_offchannel(struct rpmsg_endpoint *ept, u32 src,
				       u32 dst, void *data, int len);
static int vhost_rpmsg_trysend(struct rpmsg_endpoint *ept, void *data, int len);
static int vhost_rpmsg_trysendto(struct rpmsg_endpoint *ept, void *data,
				 int len, u32 dst);
static int vhost_rpmsg_trysend_offchannel(struct rpmsg_endpoint *ept, u32 src,
					  u32 dst, void *data, int len);

static const struct rpmsg_endpoint_ops vhost_endpoint_ops = {
	.destroy_ept = vhost_rpmsg_destroy_ept,
	.send = vhost_rpmsg_send,
	.sendto = vhost_rpmsg_sendto,
	.send_offchannel = vhost_rpmsg_send_offchannel,
	.trysend = vhost_rpmsg_trysend,
	.trysendto = vhost_rpmsg_trysendto,
	.trysend_offchannel = vhost_rpmsg_trysend_offchannel,
};

/**
 * __ept_release() - deallocate an rpmsg endpoint
 * @kref: the ept's reference count
 *
 * This function deallocates an ept, and is invoked when its @kref refcount
 * drops to zero.
 *
 * Never invoke this function directly!
 */
static void __ept_release(struct kref *kref)
{
	struct rpmsg_endpoint *ept = container_of(kref, struct rpmsg_endpoint,
						  refcount);
	/*
	 * At this point no one holds a reference to ept anymore,
	 * so we can directly free it
	 */
	kfree(ept);
}

/**
 * __rpmsg_create_ept() - Create rpmsg endpoint
 * @vrp: virtual remote processor of the vhost device where endpoint has to be
 *       created
 * @rpdev: rpmsg device on which endpoint has to be created
 * @cb: callback associated with the endpoint
 * @priv: private data for the driver's use
 * @addr: channel_info with the local rpmsg address to bind with @cb
 *
 * Allows drivers to create an endpoint, and bind a callback with some
 * private data, to an rpmsg address.
 */
static struct rpmsg_endpoint *__rpmsg_create_ept(struct virtproc_info *vrp,
						 struct rpmsg_device *rpdev,
						 rpmsg_rx_cb_t cb,
						 void *priv, u32 addr)
{
	int id_min, id_max, id;
	struct rpmsg_endpoint *ept;
	struct device *dev = rpdev ? &rpdev->dev : &vrp->vdev->dev;

	ept = kzalloc(sizeof(*ept), GFP_KERNEL);
	if (!ept)
		return NULL;

	kref_init(&ept->refcount);
	mutex_init(&ept->cb_lock);

	ept->rpdev = rpdev;
	ept->cb = cb;
	ept->priv = priv;
	ept->ops = &vhost_endpoint_ops;

	/* do we need to allocate a local address ? */
	if (addr == RPMSG_ADDR_ANY) {
		id_min = RPMSG_RESERVED_ADDRESSES;
		id_max = 0;
	} else {
		id_min = addr;
		id_max = addr + 1;
	}

	mutex_lock(&vrp->endpoints_lock);

	/* bind the endpoint to an rpmsg address (and allocate one if needed) */
	id = idr_alloc(&vrp->endpoints, ept, id_min, id_max, GFP_KERNEL);
	if (id < 0) {
		dev_err(dev, "idr_alloc failed: %d\n", id);
		goto free_ept;
	}
	ept->addr = id;

	mutex_unlock(&vrp->endpoints_lock);

	return ept;

free_ept:
	mutex_unlock(&vrp->endpoints_lock);
	kref_put(&ept->refcount, __ept_release);
	return NULL;
}

/**
 * vhost_rpmsg_create_ept() - Create rpmsg endpoint
 * @rpdev: rpmsg device on which endpoint has to be created
 * @cb: callback associated with the endpoint
 * @priv: private data for the driver's use
 * @chinfo: channel_info with the local rpmsg address to bind with @cb
 *
 * Wrapper to __rpmsg_create_ept() to create rpmsg endpoint
 */
static struct rpmsg_endpoint
*vhost_rpmsg_create_ept(struct rpmsg_device *rpdev, rpmsg_rx_cb_t cb, void *priv,
			struct rpmsg_channel_info chinfo)
{
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(rpdev);

	return __rpmsg_create_ept(vch->vrp, rpdev, cb, priv, chinfo.src);
}

/**
 * __rpmsg_destroy_ept() - destroy an existing rpmsg endpoint
 * @vrp: virtproc which owns this ept
 * @ept: endpoing to destroy
 *
 * An internal function which destroy an ept without assuming it is
 * bound to an rpmsg channel. This is needed for handling the internal
 * name service endpoint, which isn't bound to an rpmsg channel.
 * See also __rpmsg_create_ept().
 */
static void
__rpmsg_destroy_ept(struct virtproc_info *vrp, struct rpmsg_endpoint *ept)
{
	/* make sure new inbound messages can't find this ept anymore */
	mutex_lock(&vrp->endpoints_lock);
	idr_remove(&vrp->endpoints, ept->addr);
	mutex_unlock(&vrp->endpoints_lock);

	/* make sure in-flight inbound messages won't invoke cb anymore */
	mutex_lock(&ept->cb_lock);
	ept->cb = NULL;
	mutex_unlock(&ept->cb_lock);

	kref_put(&ept->refcount, __ept_release);
}

/**
 * vhost_rpmsg_destroy_ept() - destroy an existing rpmsg endpoint
 * @ept: endpoing to destroy
 *
 * Wrapper to __rpmsg_destroy_ept() to destroy rpmsg endpoint
 */
static void vhost_rpmsg_destroy_ept(struct rpmsg_endpoint *ept)
{
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(ept->rpdev);

	__rpmsg_destroy_ept(vch->vrp, ept);
}

/**
 * vhost_rpmsg_announce_create() - Announce creation of new channel
 * @rpdev: rpmsg device on which new endpoint channel is created
 *
 * Send a message to the remote processor's name service about the
 * creation of this channel.
 */
static int vhost_rpmsg_announce_create(struct rpmsg_device *rpdev)
{
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(rpdev);
	struct virtproc_info *vrp = vch->vrp;
	struct device *dev = &rpdev->dev;
	int err = 0;

	/* need to tell remote processor's name service about this channel ? */
	if (rpdev->ept && vhost_has_feature(vrp->vdev, VIRTIO_RPMSG_F_NS)) {
		struct rpmsg_ns_msg nsm;

		strncpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
		nsm.addr = rpdev->ept->addr;
		nsm.flags = RPMSG_NS_CREATE | RPMSG_AS_ANNOUNCE;

		err = rpmsg_sendto(rpdev->ept, &nsm, sizeof(nsm), RPMSG_NS_ADDR);
		if (err)
			dev_err(dev, "failed to announce service %d\n", err);
	}

	return err;
}

/**
 * vhost_rpmsg_announce_destroy() - Announce deletion of channel
 * @rpdev: rpmsg device on which this endpoint channel is created
 *
 * Send a message to the remote processor's name service about the
 * deletion of this channel.
 */
static int vhost_rpmsg_announce_destroy(struct rpmsg_device *rpdev)
{
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(rpdev);
	struct virtproc_info *vrp = vch->vrp;
	struct device *dev = &rpdev->dev;
	int err = 0;

	/* tell remote processor's name service we're removing this channel */
	if (rpdev->announce && rpdev->ept &&
	    vhost_has_feature(vrp->vdev, VIRTIO_RPMSG_F_NS)) {
		struct rpmsg_ns_msg nsm;

		strncpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
		nsm.addr = rpdev->ept->addr;
		nsm.flags = RPMSG_NS_DESTROY;

		err = rpmsg_sendto(rpdev->ept, &nsm, sizeof(nsm), RPMSG_NS_ADDR);
		if (err)
			dev_err(dev, "failed to announce service %d\n", err);
	}

	return err;
}

static const struct rpmsg_device_ops vhost_rpmsg_ops = {
	.create_ept = vhost_rpmsg_create_ept,
	.announce_create = vhost_rpmsg_announce_create,
	.announce_destroy = vhost_rpmsg_announce_destroy,
};

/**
 * vhost_rpmsg_release_device() - Callback to free vhost_rpmsg_channel
 * @dev: struct device of rpmsg_device
 *
 * Invoked from device core after all references to "dev" is removed
 * to free the wrapper vhost_rpmsg_channel.
 */
static void vhost_rpmsg_release_device(struct device *dev)
{
	struct rpmsg_device *rpdev = to_rpmsg_device(dev);
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(rpdev);

	kfree(vch);
}

/**
 * vhost_rpmsg_create_channel - Create an rpmsg channel
 * @dev: struct device of vhost_dev
 * @name: name of the rpmsg channel to be created
 *
 * Create an rpmsg channel using its name. Invokes rpmsg_register_device()
 * only if status is VIRTIO_CONFIG_S_DRIVER_OK or else just adds it to
 * list of pending rpmsg devices. This is because if the rpmsg client
 * driver is already loaded when rpmsg is being registered, it'll try
 * to start accessing virtqueue which will be ready only after VIRTIO
 * sets status as VIRTIO_CONFIG_S_DRIVER_OK.
 */
struct device *vhost_rpmsg_create_channel(struct device *dev, const char *name)
{
	struct vhost_rpmsg_channel *vch;
	struct rpmsg_device *rpdev;
	struct virtproc_info *vrp;
	struct vhost_dev *vdev;
	u8 status;
	int ret;

	vdev = to_vhost_dev(dev);
	status = vhost_get_status(vdev);
	vrp = vhost_get_drvdata(vdev);

	vch = kzalloc(sizeof(*vch), GFP_KERNEL);
	if (!vch)
		return ERR_PTR(-ENOMEM);

	/* Link the channel to our vrp */
	vch->vrp = vrp;

	/* Assign public information to the rpmsg_device */
	rpdev = &vch->rpdev;
	rpdev->src = RPMSG_ADDR_ANY;
	rpdev->dst = RPMSG_ADDR_ANY;
	rpdev->ops = &vhost_rpmsg_ops;
	rpdev->max_payload_length = vrp->buf_size - sizeof(struct rpmsg_hdr);
	rpdev->type = vrp->dev_type;
	rpdev->dev_id = vrp->dev_id;

	rpdev->announce = true;

	strncpy(rpdev->id.name, name, RPMSG_NAME_SIZE);

	rpdev->dev.parent = &vrp->vdev->dev;
	rpdev->dev.release = vhost_rpmsg_release_device;
	if (!(status & VIRTIO_CONFIG_S_DRIVER_OK)) {
		mutex_lock(&vrp->list_lock);
		list_add_tail(&rpdev->list, &vrp->list);
		mutex_unlock(&vrp->list_lock);
	} else {
		ret = rpmsg_register_device(rpdev);
		if (ret)
			return ERR_PTR(-EINVAL);
	}

	return &rpdev->dev;
}
EXPORT_SYMBOL_GPL(vhost_rpmsg_create_channel);

/**
 * vhost_rpmsg_delete_channel - Delete an rpmsg channel
 * @dev: struct device of rpmsg_device
 *
 * Delete channel created using vhost_rpmsg_create_channel()
 */
void vhost_rpmsg_delete_channel(struct device *dev)
{
	struct rpmsg_device *rpdev = to_rpmsg_device(dev);
	struct vhost_rpmsg_channel *vch;
	struct virtproc_info *vrp;
	struct vhost_dev *vdev;
	u8 status;

	vch = to_vhost_rpmsg_channel(rpdev);
	vrp = vch->vrp;
	vdev = vrp->vdev;
	status = vhost_get_status(vdev);

	if (!(status & VIRTIO_CONFIG_S_DRIVER_OK)) {
		mutex_lock(&vrp->list_lock);
		list_del(&rpdev->list);
		mutex_unlock(&vrp->list_lock);
		kfree(vch);
	} else {
		device_unregister(dev);
	}
}
EXPORT_SYMBOL_GPL(vhost_rpmsg_delete_channel);

static const struct rpmsg_virtproc_ops vhost_rpmsg_virtproc_ops = {
	.create_channel = vhost_rpmsg_create_channel,
	.delete_channel = vhost_rpmsg_delete_channel,
};

/**
 * rpmsg_upref_sleepers() - enable "tx-complete" interrupts, if needed
 * @vrp: virtual remote processor state
 *
 * This function is called before a sender is blocked, waiting for
 * a tx buffer to become available.
 *
 * If we already have blocking senders, this function merely increases
 * the "sleepers" reference count, and exits.
 *
 * Otherwise, if this is the first sender to block, we also enable
 * virtio's tx callbacks, so we'd be immediately notified when a tx
 * buffer is consumed (we rely on virtio's tx callback in order
 * to wake up sleeping senders as soon as a tx buffer is used by the
 * remote processor).
 */
static void rpmsg_upref_sleepers(struct virtproc_info *vrp)
{
	/* are we the first sleeping context waiting for tx buffers ? */
	if (atomic_inc_return(&vrp->sleepers) == 1)
		/* enable "tx-complete" interrupts before dozing off */
		vhost_virtqueue_enable_cb(vrp->svq);
}

/**
 * rpmsg_downref_sleepers() - disable "tx-complete" interrupts, if needed
 * @vrp: virtual remote processor state
 *
 * This function is called after a sender, that waited for a tx buffer
 * to become available, is unblocked.
 *
 * If we still have blocking senders, this function merely decreases
 * the "sleepers" reference count, and exits.
 *
 * Otherwise, if there are no more blocking senders, we also disable
 * virtio's tx callbacks, to avoid the overhead incurred with handling
 * those (now redundant) interrupts.
 */
static void rpmsg_downref_sleepers(struct virtproc_info *vrp)
{
	/* are we the last sleeping context waiting for tx buffers ? */
	if (atomic_dec_and_test(&vrp->sleepers))
		/* disable "tx-complete" interrupts */
		vhost_virtqueue_disable_cb(vrp->svq);
}

/**
 * rpmsg_send_offchannel_raw() - send a message across to the remote processor
 * @rpdev: the rpmsg channel
 * @src: source address
 * @dst: destination address
 * @data: payload of message
 * @len: length of payload
 * @wait: indicates whether caller should block in case no TX buffers available
 *
 * This function is the base implementation for all of the rpmsg sending API.
 *
 * It will send @data of length @len to @dst, and say it's from @src. The
 * message will be sent to the remote processor which the @rpdev channel
 * belongs to.
 *
 * The message is sent using one of the TX buffers that are available for
 * communication with this remote processor.
 *
 * If @wait is true, the caller will be blocked until either a TX buffer is
 * available, or 15 seconds elapses (we don't want callers to
 * sleep indefinitely due to misbehaving remote processors), and in that
 * case -ERESTARTSYS is returned. The number '15' itself was picked
 * arbitrarily; there's little point in asking drivers to provide a timeout
 * value themselves.
 *
 * Otherwise, if @wait is false, and there are no TX buffers available,
 * the function will immediately fail, and -ENOMEM will be returned.
 *
 * Normally drivers shouldn't use this function directly; instead, drivers
 * should use the appropriate rpmsg_{try}send{to, _offchannel} API
 * (see include/linux/rpmsg.h).
 *
 * Returns 0 on success and an appropriate error value on failure.
 */
static int rpmsg_send_offchannel_raw(struct rpmsg_device *rpdev,
				     u32 src, u32 dst,
				     void *data, int len, bool wait)
{
	struct vhost_rpmsg_channel *vch = to_vhost_rpmsg_channel(rpdev);
	struct virtproc_info *vrp = vch->vrp;
	struct vhost_virtqueue *svq = vrp->svq;
	struct vhost_dev *vdev = svq->dev;
	struct device *dev = &rpdev->dev;
	struct rpmsg_hdr msg;
	int length;
	u16 head;
	u64 base;
	int err;

	/*
	 * We currently use fixed-sized buffers, and therefore the payload
	 * length is limited.
	 *
	 * One of the possible improvements here is either to support
	 * user-provided buffers (and then we can also support zero-copy
	 * messaging), or to improve the buffer allocator, to support
	 * variable-length buffer sizes.
	 */
	if (len > vrp->buf_size - sizeof(struct rpmsg_hdr)) {
		dev_err(dev, "message is too big (%d)\n", len);
		return -EMSGSIZE;
	}

	mutex_lock(&vrp->tx_lock);
	/* grab a buffer */
	base = vhost_virtqueue_get_outbuf(svq, &head, &length);
	if (!base && !wait) {
		dev_err(dev, "Failed to get buffer for OUT transfers\n");
		err = -ENOMEM;
		goto out;
	}

	/* no free buffer ? wait for one (but bail after 15 seconds) */
	while (!base) {
		/* enable "tx-complete" interrupts, if not already enabled */
		rpmsg_upref_sleepers(vrp);

		/*
		 * sleep until a free buffer is available or 15 secs elapse.
		 * the timeout period is not configurable because there's
		 * little point in asking drivers to specify that.
		 * if later this happens to be required, it'd be easy to add.
		 */
		err = wait_event_interruptible_timeout
			(vrp->sendq, (base =
				      vhost_virtqueue_get_outbuf(svq, &head,
								 &length)),
			 msecs_to_jiffies(15000));

		/* disable "tx-complete" interrupts if we're the last sleeper */
		rpmsg_downref_sleepers(vrp);

		/* timeout ? */
		if (!err) {
			dev_err(dev, "timeout waiting for a tx buffer\n");
			err = -ERESTARTSYS;
			goto out;
		}
	}

	msg.len = len;
	msg.flags = 0;
	msg.src = src;
	msg.dst = dst;
	msg.reserved = 0;
	/*
	 * Perform two writes, one for rpmsg header and other for actual buffer
	 * data, instead of squashing the data into one buffer and then send
	 * them to the vhost layer.
	 */
	err = vhost_write(vdev, base, &msg, sizeof(struct rpmsg_hdr));
	if (err) {
		dev_err(dev, "Failed to write rpmsg header to remote buffer\n");
		goto out;
	}

	err = vhost_write(vdev, base + sizeof(struct rpmsg_hdr), data, len);
	if (err) {
		dev_err(dev, "Failed to write buffer data to remote buffer\n");
		goto out;
	}

	dev_dbg(dev, "TX From 0x%x, To 0x%x, Len %d, Flags %d, Reserved %d\n",
		msg.src, msg.dst, msg.len, msg.flags, msg.reserved);
#if defined(CONFIG_DYNAMIC_DEBUG)
	dynamic_hex_dump("rpmsg_virtio TX: ", DUMP_PREFIX_NONE, 16, 1,
			 &msg, sizeof(msg) + msg.len, true);
#endif

	vhost_virtqueue_put_buf(svq, head, len + sizeof(struct rpmsg_hdr));

	/* tell the remote processor it has a pending message to read */
	vhost_virtqueue_kick(vrp->svq);

out:
	mutex_unlock(&vrp->tx_lock);

	return err;
}

static int vhost_rpmsg_send(struct rpmsg_endpoint *ept, void *data, int len)
{
	struct rpmsg_device *rpdev = ept->rpdev;
	u32 src = ept->addr, dst = rpdev->dst;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, true);
}

static int vhost_rpmsg_sendto(struct rpmsg_endpoint *ept, void *data, int len,
			      u32 dst)
{
	struct rpmsg_device *rpdev = ept->rpdev;
	u32 src = ept->addr;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, true);
}

static int vhost_rpmsg_send_offchannel(struct rpmsg_endpoint *ept, u32 src,
				       u32 dst, void *data, int len)
{
	struct rpmsg_device *rpdev = ept->rpdev;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, true);
}

static int vhost_rpmsg_trysend(struct rpmsg_endpoint *ept, void *data, int len)
{
	struct rpmsg_device *rpdev = ept->rpdev;
	u32 src = ept->addr, dst = rpdev->dst;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, false);
}

static int vhost_rpmsg_trysendto(struct rpmsg_endpoint *ept, void *data,
				 int len, u32 dst)
{
	struct rpmsg_device *rpdev = ept->rpdev;
	u32 src = ept->addr;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, false);
}

static int vhost_rpmsg_trysend_offchannel(struct rpmsg_endpoint *ept, u32 src,
					  u32 dst, void *data, int len)
{
	struct rpmsg_device *rpdev = ept->rpdev;

	return rpmsg_send_offchannel_raw(rpdev, src, dst, data, len, false);
}

/**
 * rpmsg_recv_single - Invoked when a buffer is received from remote VIRTIO dev
 * @vrp: virtual remote processor of the vhost device which has received a msg
 * @dev: struct device of vhost_dev
 * @msg: pointer to the rpmsg_hdr
 * @len: length of the received buffer
 *
 * Invoked when a buffer is received from remote VIRTIO device. It gets the
 * destination address from rpmsg_hdr and invokes the callback of the endpoint
 * corresponding to the address
 */
static int rpmsg_recv_single(struct virtproc_info *vrp, struct device *dev,
			     struct rpmsg_hdr *msg, unsigned int len)
{
	struct rpmsg_endpoint *ept;

	dev_dbg(dev, "From: 0x%x, To: 0x%x, Len: %d, Flags: %d, Reserved: %d\n",
		msg->src, msg->dst, msg->len, msg->flags, msg->reserved);
#if defined(CONFIG_DYNAMIC_DEBUG)
	dynamic_hex_dump("rpmsg_virtio RX: ", DUMP_PREFIX_NONE, 16, 1,
			 msg, sizeof(*msg) + msg->len, true);
#endif

	/*
	 * We currently use fixed-sized buffers, so trivially sanitize
	 * the reported payload length.
	 */
	if (len > vrp->buf_size ||
	    msg->len > (len - sizeof(struct rpmsg_hdr))) {
		dev_warn(dev, "inbound msg too big: (%d, %d)\n", len, msg->len);
		return -EINVAL;
	}

	/* use the dst addr to fetch the callback of the appropriate user */
	mutex_lock(&vrp->endpoints_lock);

	ept = idr_find(&vrp->endpoints, msg->dst);

	/* let's make sure no one deallocates ept while we use it */
	if (ept)
		kref_get(&ept->refcount);

	mutex_unlock(&vrp->endpoints_lock);

	if (ept) {
		/* make sure ept->cb doesn't go away while we use it */
		mutex_lock(&ept->cb_lock);

		if (ept->cb)
			ept->cb(ept->rpdev, msg->data, msg->len, ept->priv,
				msg->src);

		mutex_unlock(&ept->cb_lock);

		/* farewell, ept, we don't need you anymore */
		kref_put(&ept->refcount, __ept_release);
	} else {
		dev_warn(dev, "msg received with no recipient\n");
	}

	return 0;
}

/**
 * vhost_rpmsg_recv_done - Callback of the receive virtqueue
 * @rvq: Receive virtqueue
 *
 * Invoked when the remote VIRTIO device sends a notification on the receive
 * virtqueue. It gets base address of the input buffer and repeatedly calls
 * rpmsg_recv_single() until no more buffers are left to be read.
 */
static void vhost_rpmsg_recv_done(struct vhost_virtqueue *rvq)
{
	struct vhost_dev *vdev = rvq->dev;
	struct virtproc_info *vrp = vhost_get_drvdata(vdev);
	unsigned int len, msgs_received = 0;
	struct device *dev = &vdev->dev;
	struct rpmsg_hdr *msg;
	u64 base;
	u16 head;
	int err;

	base = vhost_virtqueue_get_inbuf(rvq, &head, &len);
	if (!base) {
		dev_err(dev, "uhm, incoming signal, but no used buffer ?\n");
		return;
	}

	vhost_virtqueue_disable_cb(rvq);
	while (base) {
		msg =  kzalloc(len, GFP_KERNEL);
		if (!msg)
			return;

		vhost_read(rvq->dev, msg, base, len);

		err = rpmsg_recv_single(vrp, dev, msg, len);
		if (err) {
			kfree(msg);
			break;
		}

		kfree(msg);
		vhost_virtqueue_put_buf(rvq, head, len);
		msgs_received++;

		base = vhost_virtqueue_get_inbuf(rvq, &head, &len);
	}
	vhost_virtqueue_enable_cb(rvq);

	dev_dbg(dev, "Received %u messages\n", msgs_received);

	/* tell the remote processor we added another available rx buffer */
	if (msgs_received)
		vhost_virtqueue_kick(vrp->rvq);
}

/**
 * vhost_rpmsg_xmit_done - Callback of the receive virtqueue
 * @svq: Send virtqueue
 *
 * This is invoked whenever the remote processor completed processing
 * a TX msg we just sent it, and the buffer is put back to the used ring.
 *
 * Normally, though, we suppress this "tx complete" interrupt in order to
 * avoid the incurred overhead.
 */
static void vhost_rpmsg_xmit_done(struct vhost_virtqueue *svq)
{
	struct vhost_dev *vdev = svq->dev;
	struct virtproc_info *vrp = vhost_get_drvdata(vdev);
	struct device *dev = &vdev->dev;

	dev_dbg(dev, "%s\n", __func__);

	/* wake up potential senders that are waiting for a tx buffer */
	wake_up_interruptible(&vrp->sendq);
}

/**
 * vhost_rpmsg_as_cb - Callback of address service announcement
 * @data: rpmsg_as_msg sent by remote VIRTIO device
 * @len: length of the received message
 * @priv: private data for the driver's use
 * @src: source address of the remote VIRTIO device that sent the AS
 *       announcement
 *
 * Invoked when a address service announcement arrives to assign the
 * destination address of the rpmsg device.
 */
static int vhost_rpmsg_as_cb(struct rpmsg_device *rpdev, void *data, int len,
			     void *priv, u32 hdr_src)
{
	struct virtproc_info *vrp = priv;
	struct device *dev = &vrp->vdev->dev;
	struct rpmsg_channel_info chinfo;
	struct rpmsg_as_msg *msg = data;
	struct rpmsg_device *rpmsg_dev;
	struct device *rdev;
	int ret = 0;
	u32 flags;
	u32 src;
	u32 dst;

#if defined(CONFIG_DYNAMIC_DEBUG)
	dynamic_hex_dump("AS announcement: ", DUMP_PREFIX_NONE, 16, 1,
			 data, len, true);
#endif

	if (len == sizeof(*msg)) {
		src = msg->src;
		dst = msg->dst;
		flags = msg->flags;
	} else {
		dev_err(dev, "malformed AS msg (%d)\n", len);
		return -EINVAL;
	}

	/*
	 * the name service ept does _not_ belong to a real rpmsg channel,
	 * and is handled by the rpmsg bus itself.
	 * for sanity reasons, make sure a valid rpdev has _not_ sneaked
	 * in somehow.
	 */
	if (rpdev) {
		dev_err(dev, "anomaly: ns ept has an rpdev handle\n");
		return -EINVAL;
	}

	/* don't trust the remote processor for null terminating the name */
	msg->name[RPMSG_NAME_SIZE - 1] = '\0';

	dev_info(dev, "%sing dst addr 0x%x to channel %s src 0x%x\n",
		 flags & RPMSG_AS_ASSIGN ? "Assign" : "Free",
		 dst, msg->name, src);

	strncpy(chinfo.name, msg->name, sizeof(chinfo.name));
	chinfo.src = src;
	chinfo.dst = RPMSG_ADDR_ANY;

	/* Find a similar channel */
	rdev = rpmsg_find_device(dev, &chinfo);
	if (!rdev) {
		ret = -ENODEV;
		goto err_find_device;
	}

	rpmsg_dev = to_rpmsg_device(rdev);
	if (flags & RPMSG_AS_ASSIGN) {
		if (rpmsg_dev->dst != RPMSG_ADDR_ANY) {
			dev_err(dev, "Address bound to channel %s src 0x%x\n",
				msg->name, src);
			ret = -EBUSY;
			goto err_find_device;
		}
		rpmsg_dev->dst = dst;
	} else {
		rpmsg_dev->dst = RPMSG_ADDR_ANY;
	}

err_find_device:
	put_device(rdev);

	return ret;
}

/**
 * vhost_rpmsg_finalize_feature - Perform initializations for negotiated
 *   features
 * @vrp: virtual remote processor of the vhost device where the feature has been
 *       negotiated
 *
 * Invoked when features negotiation between VHOST and VIRTIO device is
 * completed.
 */
static int vhost_rpmsg_finalize_feature(struct virtproc_info *vrp)
{
	struct vhost_dev *vdev = vrp->vdev;

	/* if supported by the remote processor, enable the address service */
	if (vhost_has_feature(vdev, VIRTIO_RPMSG_F_AS)) {
		/* a dedicated endpoint handles the name service msgs */
		vrp->as_ept = __rpmsg_create_ept(vrp, NULL, vhost_rpmsg_as_cb,
						 vrp, RPMSG_AS_ADDR);
		if (!vrp->as_ept) {
			dev_err(&vdev->dev, "failed to create the as ept\n");
			return -ENOMEM;
		}
	} else {
		dev_err(&vdev->dev, "Address Service not supported\n");
		return -ENOMEM;
	}

	return 0;
}

/**
 * vhost_rpmsg_set_status - Perform initialization when remote VIRTIO device
 *   updates status
 * @vrp: virtual remote processor of the vhost device whose status has been
 *       updated
 *
 * Invoked when the remote VIRTIO device updates status. If status is set
 * as VIRTIO_CONFIG_S_DRIVER_OK, invoke rpmsg_register_device() for every
 * un-registered rpmsg device.
 */
static int vhost_rpmsg_set_status(struct virtproc_info *vrp)
{
	struct vhost_dev *vdev = vrp->vdev;
	struct rpmsg_device *rpdev, *tmp;
	u8 status;
	int ret;

	status = vhost_get_status(vdev);

	if (status & VIRTIO_CONFIG_S_DRIVER_OK) {
		mutex_lock(&vrp->list_lock);
		list_for_each_entry_safe(rpdev, tmp, &vrp->list, list) {
			list_del_init(&rpdev->list);
			ret = rpmsg_register_device(rpdev);
			if (ret) {
				mutex_unlock(&vrp->list_lock);
				return -EINVAL;
			}
		}
		mutex_unlock(&vrp->list_lock);
	}

	return 0;
}

/**
 * vhost_rpmsg_notifier - Notifier to notify updates from remote VIRTIO device
 * @nb: notifier block associated with this virtual remote processor
 * @notify_reason: Indicate the updates (finalize feature or set status) by
 *                 remote host
 * @data: un-used here
 *
 * Invoked when the remote VIRTIO device updates status or finalize features.
 */
static int vhost_rpmsg_notifier(struct notifier_block *nb, unsigned long notify_reason,
				void *data)
{
	struct virtproc_info *vrp = container_of(nb, struct virtproc_info, nb);
	struct vhost_dev *vdev = vrp->vdev;
	int ret;

	switch (notify_reason) {
	case NOTIFY_FINALIZE_FEATURES:
		ret = vhost_rpmsg_finalize_feature(vrp);
		if (ret)
			dev_err(&vdev->dev, "failed to finalize features\n");
		break;
	case NOTIFY_SET_STATUS:
		ret = vhost_rpmsg_set_status(vrp);
		if (ret)
			dev_err(&vdev->dev, "failed to set status\n");
		break;
	default:
		dev_err(&vdev->dev, "Unsupported notification 0x%lx\n",
			notify_reason);
		break;
	}

	return NOTIFY_OK;
}

static unsigned int vhost_rpmsg_features[] = {
	VIRTIO_RPMSG_F_AS,
	VIRTIO_RPMSG_F_NS,
};

/**
 * vhost_rpmsg_set_features - Sets supported features on the VHOST device
 *
 * Build supported features from the feature table and invoke
 * vhost_set_features() to set the supported features on the VHOST device
 */
static int vhost_rpmsg_set_features(struct vhost_dev *vdev)
{
	unsigned int feature_table_size;
	unsigned int feature;
	u64 device_features = 0;
	int ret, i;

	feature_table_size =  ARRAY_SIZE(vhost_rpmsg_features);
	for (i = 0; i < feature_table_size; i++) {
		feature = vhost_rpmsg_features[i];
		WARN_ON(feature >= 64);
		device_features |= (1ULL << feature);
	}

	ret = vhost_set_features(vdev, device_features);
	if (ret)
		return ret;

	return 0;
}

/**
 * vhost_rpmsg_probe - Create virtual remote processor for the VHOST device
 * @vdev - VHOST device with vendor ID and device ID supported by this driver
 *
 * Invoked when VHOST device is registered with vendor ID and device ID
 * supported by this driver. Creates and initializes the virtual remote
 * processor for the VHOST device
 */
static int vhost_rpmsg_probe(struct vhost_dev *vdev)
{
	vhost_vq_callback_t *vq_cbs[] = { vhost_rpmsg_xmit_done, vhost_rpmsg_recv_done };
	static const char * const names[] = { "output", "input" };
	struct device *dev = &vdev->dev;
	struct vhost_virtqueue *vqs[2];
	struct config_group *group;
	struct virtproc_info *vrp;
	int err;

	vrp = devm_kzalloc(dev, sizeof(*vrp), GFP_KERNEL);
	if (!vrp)
		return -ENOMEM;

	vrp->vdev = vdev;
#if defined(CONFIG_ARCH_SSTAR)
	vrp->dev_type = vdev->id.vendor >> 16;
	vrp->dev_id = vdev->id.vendor & 0x0000ffff;
#endif

	idr_init(&vrp->endpoints);
	mutex_init(&vrp->endpoints_lock);
	mutex_init(&vrp->tx_lock);
	mutex_init(&vrp->rx_lock);
	mutex_init(&vrp->list_lock);
	init_waitqueue_head(&vrp->sendq);

	err = vhost_rpmsg_set_features(vdev);
	if (err) {
		dev_err(dev, "Failed to set features\n");
		return err;
	}

	/* We expect two vhost_virtqueues, tx and rx (and in this order) */
	err = vhost_create_vqs(vdev, 2, MAX_RPMSG_NUM_BUFS / 2, vqs, vq_cbs,
			       names);
	if (err) {
		dev_err(dev, "Failed to create virtqueues\n");
		return err;
	}

	vrp->svq = vqs[0];
	vrp->rvq = vqs[1];

	vrp->buf_size = MAX_RPMSG_BUF_SIZE;

	/* suppress "tx-complete" interrupts */
	vhost_virtqueue_disable_cb(vrp->svq);

	vhost_set_drvdata(vdev, vrp);

	vrp->nb.notifier_call = vhost_rpmsg_notifier;
	vhost_register_notifier(vdev, &vrp->nb);
	INIT_LIST_HEAD(&vrp->list);

	group = rpmsg_cfs_add_virtproc_group(dev,
					     &vhost_rpmsg_virtproc_ops);
	if (IS_ERR(group)) {
		err = PTR_ERR(group);
		goto err;
	}

	vrp->def_channel = vhost_rpmsg_create_channel(dev, "rpmsg_dualos");
	if (!vrp->def_channel) {
	    dev_err(&vdev->dev, "failed to create default channel\n");
	}
	dev_info(&vdev->dev, "vhost rpmsg host is online\n");

	return 0;

err:
	vhost_del_vqs(vdev);

	return err;
}

static int vhost_rpmsg_remove_device(struct device *dev, void *data)
{
	device_unregister(dev);

	return 0;
}

static int vhost_rpmsg_remove(struct vhost_dev *vdev)
{
	struct virtproc_info *vrp = vhost_get_drvdata(vdev);
	int ret;

	ret = device_for_each_child(&vdev->dev, NULL, vhost_rpmsg_remove_device);
	if (ret)
		dev_warn(&vdev->dev, "can't remove rpmsg device: %d\n", ret);

	if (vrp->as_ept)
		__rpmsg_destroy_ept(vrp, vrp->as_ept);

	idr_destroy(&vrp->endpoints);

	vhost_del_vqs(vdev);

	kfree(vrp);
	return 0;
}

static struct vhost_device_id vhost_rpmsg_id_table[] = {
	{ VIRTIO_ID_RPMSG, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct vhost_driver vhost_rpmsg_driver = {
	.driver.name	= KBUILD_MODNAME,
	.driver.owner	= THIS_MODULE,
	.id_table	= vhost_rpmsg_id_table,
	.probe		= vhost_rpmsg_probe,
	.remove		= vhost_rpmsg_remove,
};

static int __init vhost_rpmsg_init(void)
{
	int ret;

	ret = vhost_register_driver(&vhost_rpmsg_driver);
	if (ret)
		pr_err("Failed to register vhost rpmsg driver: %d\n", ret);

	return ret;
}
subsys_initcall(vhost_rpmsg_init);

static void __exit vhost_rpmsg_exit(void)
{
	vhost_unregister_driver(&vhost_rpmsg_driver);
}
module_exit(vhost_rpmsg_exit);

MODULE_DEVICE_TABLE(vhost, vhost_rpmsg_id_table);
MODULE_DESCRIPTION("Vhost-based remote processor messaging bus");
MODULE_LICENSE("GPL v2");
