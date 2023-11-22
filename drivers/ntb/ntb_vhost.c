// SPDX-License-Identifier: GPL-2.0
/**
 * NTB Client Driver to implement VHOST functionality
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/module.h>
#include <linux/ntb.h>
#include <linux/slab.h>
#include <linux/vhost.h>
#include <linux/virtio_pci.h>
#include <linux/vringh.h>

#include "ntb_virtio.h"

static struct workqueue_struct *kntbvhost_workqueue;

struct ntb_vhost_queue {
	struct delayed_work db_handler;
	struct vhost_virtqueue *vq;
	void __iomem *vq_addr;
};

struct ntb_vhost {
	struct ntb_vhost_queue vqueue[MAX_VQS];
	struct work_struct link_cleanup;
	struct delayed_work cmd_handler;
	struct delayed_work link_work;
	resource_size_t peer_mw_size;
	struct config_group *group;
	phys_addr_t peer_mw_addr;
	struct vhost_dev vdev;
	struct ntb_dev *ndev;
	struct vring vring;
	struct device *dev;
	u64 virtio_addr;
	u64 features;
};

#define to_ntb_vhost(v) container_of((v), struct ntb_vhost, vdev)

/* ntb_vhost_finalize_features - Indicate features are finalized to vhost client
 *   driver
 * @ntb: NTB vhost device that communicates with the remote virtio device
 *
 * Invoked when the remote virtio device sends HOST_CMD_FINALIZE_FEATURES
 * command once the feature negotiation is complete. This function sends
 * notification to the vhost client driver.
 */
static void ntb_vhost_finalize_features(struct ntb_vhost *ntb)
{
	struct vhost_dev *vdev;
	struct ntb_dev *ndev;
	u64 features;

	vdev = &ntb->vdev;
	ndev = ntb->ndev;

	features = ntb_spad_read(ndev, VIRTIO_FEATURES_UPPER);
	features <<= 32;
	features |= ntb_spad_read(ndev, VIRTIO_FEATURES_LOWER);
	vdev->features = features;
	blocking_notifier_call_chain(&vdev->notifier, NOTIFY_FINALIZE_FEATURES, 0);
}

/* ntb_vhost_cmd_handler - Handle commands from remote NTB virtio driver
 * @work: The work_struct holding the ntb_vhost_cmd_handler() function that is
 *   scheduled
 *
 * Handle commands from the remote NTB virtio driver and sends notification to
 * the vhost client driver. The remote virtio driver sends commands when the
 * virtio driver status is updated or when the feature negotiation is complet
 * or if the virtio driver wants to reset the device.
 */
static void ntb_vhost_cmd_handler(struct work_struct *work)
{
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;
	struct device *dev;
	u8 command;

	ntb = container_of(work, struct ntb_vhost, cmd_handler.work);
	vdev = &ntb->vdev;
	ndev = ntb->ndev;
	dev = ntb->dev;

	command = ntb_spad_read(ndev, VHOST_COMMAND);
	if (!command)
		goto reset_handler;

	ntb_spad_write(ndev, VHOST_COMMAND, 0);

	switch (command) {
	case HOST_CMD_SET_STATUS:
		blocking_notifier_call_chain(&vdev->notifier, NOTIFY_SET_STATUS, 0);
		ntb_spad_write(ndev, VHOST_COMMAND_STATUS, HOST_CMD_STATUS_OKAY);
		break;
	case HOST_CMD_FINALIZE_FEATURES:
		ntb_vhost_finalize_features(ntb);
		ntb_spad_write(ndev, VHOST_COMMAND_STATUS, HOST_CMD_STATUS_OKAY);
		break;
	case HOST_CMD_RESET:
		blocking_notifier_call_chain(&vdev->notifier, NOTIFY_RESET, 0);
		ntb_spad_write(ndev, VHOST_COMMAND_STATUS, HOST_CMD_STATUS_OKAY);
		break;
	default:
		dev_err(dev, "UNKNOWN command: %d\n", command);
		break;
	}

reset_handler:
	queue_delayed_work(kntbvhost_workqueue, &ntb->cmd_handler,
			   msecs_to_jiffies(1));
}

/* ntb_vhost_del_vqs - Delete all the vqs associated with the vhost device
 * @vdev: Vhost device that communicates with the remote virtio device
 *
 * Delete all the vqs associated with the vhost device.
 */
void ntb_vhost_del_vqs(struct vhost_dev *vdev)
{
	struct ntb_vhost_queue *vqueue;
	struct vhost_virtqueue *vq;
	struct ntb_vhost *ntb;
	int i;

	ntb = to_ntb_vhost(vdev);

	for (i = 0; i < vdev->nvqs; i++) {
		vq = vdev->vqs[i];
		if (IS_ERR_OR_NULL(vq))
			continue;

		vqueue = &ntb->vqueue[i];
		cancel_delayed_work_sync(&vqueue->db_handler);
		iounmap(vqueue->vq_addr);
		kfree(vq);
	}
}

/* ntb_vhost_vq_db_work - Handle doorbell event receive for a virtqueue
 * @work: The work_struct holding the ntb_vhost_vq_db_work() function for every
 *   created virtqueue
 *
 * This function is invoked when the remote virtio driver sends a notification
 * to the virtqueue. (virtqueue_kick() on the remote virtio driver). This
 * function invokes the vhost client driver's virtqueue callback.
 */
static void ntb_vhost_vq_db_work(struct work_struct *work)
{
	struct ntb_vhost_queue *vqueue;

	vqueue = container_of(work, struct ntb_vhost_queue, db_handler.work);
	vhost_virtqueue_callback(vqueue->vq);
}

/* ntb_vhost_notify - Send notification to the remote virtqueue
 * @vq: The local vhost virtqueue corresponding to the remote virtio virtqueue
 *
 * Use NTB doorbell to send notification for the remote virtqueue
 */
static void ntb_vhost_notify(struct vhost_virtqueue *vq)
{
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;

	vdev = vq->dev;
	ntb = to_ntb_vhost(vdev);

	ntb_peer_db_set(ntb->ndev, vq->index);
}

/* ntb_vhost_create_vq - Create a new vhost virtqueue
 * @vdev: Vhost device that communicates with the remote virtio device
 * @index: Index of the vhost virtqueue
 * @num_bufs: The number of buffers that should be supported by the vhost
 *   virtqueue (number of descriptors in the vhost virtqueue)
 * @callback: Callback function associated with the virtqueue
 *
 * Create a new vhost virtqueue which can be used by the vhost client driver
 * to access the remote virtio. This sets up the local address of the vhost
 * virtqueue but shouldn't be accessed until the virtio sets the status to
 * VIRTIO_CONFIG_S_DRIVER_OK.
 */
static struct vhost_virtqueue *
ntb_vhost_create_vq(struct vhost_dev *vdev, int index, unsigned int num_bufs,
		    void (*callback)(struct vhost_virtqueue *))
{
	struct ntb_vhost_queue *vqueue;
	unsigned int vq_size, offset;
	struct vhost_virtqueue *vq;
	phys_addr_t vq_phys_addr;
	phys_addr_t peer_mw_addr;
	struct vringh *vringh;
	struct ntb_vhost *ntb;
	void __iomem *vq_addr;
	struct ntb_dev *ndev;
	struct vring *vring;
	struct device *dev;
	int ret;

	ntb = to_ntb_vhost(vdev);
	vqueue = &ntb->vqueue[index];
	peer_mw_addr = ntb->peer_mw_addr;
	ndev = ntb->ndev;
	dev = ntb->dev;

	vq =  kzalloc(sizeof(*vq), GFP_KERNEL);
	if (!vq)
		return ERR_PTR(-ENOMEM);

	vq->dev = vdev;
	vq->callback = callback;
	vq->num = num_bufs;
	vq->index = index;
	vq->notify = ntb_vhost_notify;
	vq->type = VHOST_TYPE_MMIO;

	vringh = &vq->vringh;
	vring = &vringh->vring;

	ntb_spad_write(ndev, VHOST_QUEUE_NUM_BUFFERS(index), num_bufs);
	vq_size = vring_size(num_bufs, VIRTIO_PCI_VRING_ALIGN);
	offset = index * vq_size;
	if (offset + vq_size > ntb->peer_mw_size) {
		dev_err(dev, "Not enough vhost memory for allocating vq\n");
		ret = -ENOMEM;
		goto err_out_of_bound;
	}

	vq_phys_addr = peer_mw_addr + offset;
	vq_addr = ioremap_wc(vq_phys_addr, vq_size);
	if (!vq_addr) {
		dev_err(dev, "Fail to ioremap virtqueue address\n");
		ret = -ENOMEM;
		goto err_out_of_bound;
	}

	vqueue->vq = vq;
	vqueue->vq_addr = vq_addr;

	vring_init(vring, num_bufs, vq_addr, VIRTIO_PCI_VRING_ALIGN);
	ret = vringh_init_mmio(vringh, ntb->features, num_bufs, false,
			       vring->desc, vring->avail, vring->used);
	if (ret) {
		dev_err(dev, "Failed to init vringh\n");
		goto err_init_mmio;
	}

	INIT_DELAYED_WORK(&vqueue->db_handler, ntb_vhost_vq_db_work);

	return vq;

err_init_mmio:
	iounmap(vq_addr);

err_out_of_bound:
	kfree(vq);

	return ERR_PTR(ret);
}

/* ntb_vhost_create_vqs - Create vhost virtqueues for vhost device
 * @vdev: Vhost device that communicates with the remote virtio device
 * @nvqs: Number of vhost virtqueues to be created
 * @num_bufs: The number of buffers that should be supported by the vhost
 *   virtqueue (number of descriptors in the vhost virtqueue)
 * @vqs: Pointers to all the created vhost virtqueues
 * @callback: Callback function associated with the virtqueue
 * @names: Names associated with each virtqueue
 *
 * Create vhost virtqueues for vhost device. This acts as a wrapper to
 * ntb_vhost_create_vq() which creates individual vhost virtqueue.
 */
static int ntb_vhost_create_vqs(struct vhost_dev *vdev, unsigned int nvqs,
				unsigned int num_bufs,
				struct vhost_virtqueue *vqs[],
				vhost_vq_callback_t *callbacks[],
				const char * const names[])
{
	struct ntb_vhost *ntb;
	struct device *dev;
	int ret, i;

	ntb = to_ntb_vhost(vdev);
	dev = ntb->dev;

	for (i = 0; i < nvqs; i++) {
		vqs[i] = ntb_vhost_create_vq(vdev, i, num_bufs, callbacks[i]);
		if (IS_ERR_OR_NULL(vqs[i])) {
			ret = PTR_ERR(vqs[i]);
			dev_err(dev, "Failed to create virtqueue\n");
			goto err;
		}
	}

	vdev->nvqs = nvqs;
	vdev->vqs = vqs;

	return 0;

err:
	ntb_vhost_del_vqs(vdev);

	return ret;
}

/* ntb_vhost_write - Write data to buffer provided by remote virtio driver
 * @vdev: Vhost device that communicates with the remote virtio device
 * @dst: Buffer address in the remote device provided by the remote virtio
 *   driver
 * @src: Buffer address in the local device provided by the vhost client driver
 * @len: Length of the data to be copied from @src to @dst
 *
 * Write data to buffer provided by remote virtio driver from buffer provided
 * by vhost client driver.
 */
static int ntb_vhost_write(struct vhost_dev *vdev, u64 dst, void *src, int len)
{
	phys_addr_t peer_mw_addr, phys_addr;
	struct ntb_vhost *ntb;
	unsigned int offset;
	struct device *dev;
	u64 virtio_addr;
	void *addr;

	ntb = to_ntb_vhost(vdev);
	dev = ntb->dev;

	peer_mw_addr = ntb->peer_mw_addr;
	virtio_addr = ntb->virtio_addr;

	offset = dst -  virtio_addr;
	if (offset + len > ntb->peer_mw_size) {
		dev_err(dev, "Overflow of vhost memory\n");
		return -EINVAL;
	}

	phys_addr = peer_mw_addr + offset;
	addr = ioremap_wc(phys_addr, len);
	if (!addr) {
		dev_err(dev, "Failed to ioremap vhost address\n");
		return -ENOMEM;
	}

	memcpy_toio(addr, src, len);
	iounmap(addr);

	return 0;
}

/* ntb_vhost_read - Read data from buffers provided by remote virtio driver
 * @vdev: Vhost device that communicates with the remote virtio device
 * @dst: Buffer address in the local device provided by the vhost client driver
 * @src: Buffer address in the remote device provided by the remote virtio
 *   driver
 * @len: Length of the data to be copied from @src to @dst
 *
 * Read data from buffers provided by remote virtio driver to address provided
 * by vhost client driver.
 */
static int ntb_vhost_read(struct vhost_dev *vdev, void *dst, u64 src, int len)
{
	phys_addr_t peer_mw_addr, phys_addr;
	struct ntb_vhost *ntb;
	unsigned int offset;
	struct device *dev;
	u64 virtio_addr;
	void *addr;

	ntb = to_ntb_vhost(vdev);
	dev = ntb->dev;

	peer_mw_addr = ntb->peer_mw_addr;
	virtio_addr = ntb->virtio_addr;

	offset = src - virtio_addr;
	if (offset + len > ntb->peer_mw_size) {
		dev_err(dev, "Overflow of vhost memory\n");
		return -EINVAL;
	}

	phys_addr = peer_mw_addr + offset;
	addr = ioremap_wc(phys_addr, len);
	if (!addr) {
		dev_err(dev, "Failed to ioremap vhost address\n");
		return -ENOMEM;
	}

	memcpy_fromio(dst, addr, len);
	iounmap(addr);

	return 0;
}

/* ntb_vhost_release - Callback function to free device
 * @dev: Device in vhost_dev that has to be freed
 *
 * Callback function from device core invoked to free the device after
 * all references have been removed. This frees the allocated memory for
 * struct ntb_vhost.
 */
static void ntb_vhost_release(struct device *dev)
{
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;

	vdev = to_vhost_dev(dev);
	ntb = to_ntb_vhost(vdev);

	kfree(ntb);
}

/* ntb_vhost_set_features - vhost_config_ops to set vhost device features
 * @vdev: Vhost device that communicates with the remote virtio device
 * @features: Features supported by the vhost client driver
 *
 * vhost_config_ops invoked by the vhost client driver to set vhost device
 * features.
 */
static int ntb_vhost_set_features(struct vhost_dev *vdev, u64 features)
{
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;

	ntb = to_ntb_vhost(vdev);
	ndev = ntb->ndev;

	ntb_spad_write(ndev, VHOST_FEATURES_LOWER, lower_32_bits(features));
	ntb_spad_write(ndev, VHOST_FEATURES_UPPER, upper_32_bits(features));
	ntb->features = features;

	return 0;
}

/* ntb_vhost_set_status - vhost_config_ops to set vhost device status
 * @vdev: Vhost device that communicates with the remote virtio device
 * @status: Vhost device status configured by vhost client driver
 *
 * vhost_config_ops invoked by the vhost client driver to set vhost device
 * status.
 */
static int ntb_vhost_set_status(struct vhost_dev *vdev, u8 status)
{
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;

	ntb = to_ntb_vhost(vdev);
	ndev = ntb->ndev;

	ntb_spad_write(ndev, VHOST_DEVICE_STATUS, status);

	return 0;
}

/* ntb_vhost_get_status - vhost_config_ops to get vhost device status
 * @vdev: Vhost device that communicates with the remote virtio device
 *
 * vhost_config_ops invoked by the vhost client driver to get vhost device
 * status set by the remote virtio driver.
 */
static u8 ntb_vhost_get_status(struct vhost_dev *vdev)
{
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;

	ntb = to_ntb_vhost(vdev);
	ndev = ntb->ndev;

	return ntb_spad_read(ndev, VHOST_DEVICE_STATUS);
}

static const struct vhost_config_ops ops = {
	.create_vqs	= ntb_vhost_create_vqs,
	.del_vqs	= ntb_vhost_del_vqs,
	.write		= ntb_vhost_write,
	.read		= ntb_vhost_read,
	.set_features	= ntb_vhost_set_features,
	.set_status	= ntb_vhost_set_status,
	.get_status	= ntb_vhost_get_status,
};

/* ntb_vhost_link_cleanup - Cleanup once link to the remote host is lost
 * @ntb: NTB vhost device that communicates with the remote virtio device
 *
 * Performs the cleanup that has to be done once the link to the remote host
 * is lost or when the NTB vhost driver is removed.
 */
static void ntb_vhost_link_cleanup(struct ntb_vhost *ntb)
{
	cancel_delayed_work_sync(&ntb->link_work);
}

/* ntb_vhost_link_cleanup_work - Cleanup once link to the remote host is lost
 * @work: The work_struct holding the ntb_vhost_link_cleanup_work() function
 *   that is scheduled
 *
 * Performs the cleanup that has to be done once the link to the remote host
 * is lost. This acts as a wrapper to ntb_vhost_link_cleanup() for the cleanup
 * operation.
 */
static void ntb_vhost_link_cleanup_work(struct work_struct *work)
{
	struct ntb_vhost *ntb;

	ntb = container_of(work, struct ntb_vhost, link_cleanup);
	ntb_vhost_link_cleanup(ntb);
}

/* ntb_vhost_link_work - Initialization once link to the remote host is
 *   established
 * @work: The work_struct holding the ntb_vhost_link_work() function that is
 *   scheduled
 *
 * Performs the NTB vhost initialization that has to be done once the link to
 * the remote host is established. Initializes the scratchpad registers with
 * data required for the remote NTB virtio driver to establish communication
 * with this vhost driver.
 */
static void ntb_vhost_link_work(struct work_struct *work)
{
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;
	struct device *dev;
	u64 virtio_addr;
	u32 type;

	ntb = container_of(work, struct ntb_vhost, link_work.work);
	vdev = &ntb->vdev;
	ndev = ntb->ndev;
	dev = ntb->dev;

	/*
	 * Device will be registered when vhost client driver is linked to
	 * vhost transport device driver in configfs.
	 */
	if (!device_is_registered(&vdev->dev))
		goto out;

	/*
	 * This is unlikely to happen if "vhost" configfs is used for
	 * registering vhost device.
	 */
	if (vdev->id.device == 0 && vdev->id.vendor == 0) {
		dev_err(dev, "vhost device is registered without valid ID\n");
		goto out;
	}

	ntb_spad_write(ndev, VHOST_VENDORID, vdev->id.vendor);
	ntb_spad_write(ndev, VHOST_DEVICEID, vdev->id.device);
	ntb_peer_spad_write(ndev, NTB_DEF_PEER_IDX, VIRTIO_TYPE, TYPE_VHOST);

	type = ntb_spad_read(ndev, VIRTIO_TYPE);
	if (type != TYPE_VIRTIO)
		goto out;

	virtio_addr = ntb_spad_read(ndev,  VIRTIO_MW0_UPPER_ADDR);
	virtio_addr <<= 32;
	virtio_addr |= ntb_spad_read(ndev, VIRTIO_MW0_LOWER_ADDR);
	ntb->virtio_addr = virtio_addr;

	INIT_DELAYED_WORK(&ntb->cmd_handler, ntb_vhost_cmd_handler);
	queue_work(kntbvhost_workqueue, &ntb->cmd_handler.work);

	return;

out:
	if (ntb_link_is_up(ndev, NULL, NULL) == 1)
		schedule_delayed_work(&ntb->link_work,
				      msecs_to_jiffies(NTB_LINK_DOWN_TIMEOUT));
}

/* ntb_vhost_event_callback - Callback to link event interrupt
 * @data: Private data specific to NTB vhost driver
 *
 * Callback function from NTB HW driver whenever both the hosts in the NTB
 * setup has invoked ntb_link_enable().
 */
static void ntb_vhost_event_callback(void *data)
{
	struct ntb_vhost *ntb;
	struct ntb_dev *ndev;

	ntb = data;
	ndev = ntb->ndev;

	if (ntb_link_is_up(ntb->ndev, NULL, NULL) == 1)
		schedule_delayed_work(&ntb->link_work, 0);
	else
		schedule_work(&ntb->link_cleanup);
}

/* ntb_vhost_vq_db_callback - Callback to doorbell interrupt to handle vhost
 *   virtqueue work
 * @data: Private data specific to NTB vhost driver
 * @vector: Doorbell vector on which interrupt is received
 *
 * Callback function from NTB HW driver whenever remote virtio driver has sent
 * a notification using doorbell. This schedules work corresponding to the
 * virtqueue for which notification has been received.
 */
static void ntb_vhost_vq_db_callback(void *data, int vector)
{
	struct ntb_vhost_queue *vqueue;
	struct ntb_vhost *ntb;

	ntb = data;
	vqueue = &ntb->vqueue[vector - 1];

	schedule_delayed_work(&vqueue->db_handler, 0);
}

static const struct ntb_ctx_ops ntb_vhost_ops = {
	.link_event = ntb_vhost_event_callback,
	.db_event = ntb_vhost_vq_db_callback,
};

/* ntb_vhost_configure_mw - Get memory window address and size
 * @ntb: NTB vhost device that communicates with the remote virtio device
 *
 * Get address and size of memory window 0 and update the size of
 * memory window 0 to scratchpad register in order for virtio driver
 * to get the memory window size.
 *
 * TODO: Add support for multiple memory windows.
 */
static int ntb_vhost_configure_mw(struct ntb_vhost *ntb)
{
	struct ntb_dev *ndev;
	struct device *dev;
	int ret;

	ndev = ntb->ndev;
	dev = ntb->dev;

	ret = ntb_peer_mw_get_addr(ndev, 0, &ntb->peer_mw_addr, &ntb->peer_mw_size);
	if (ret) {
		dev_err(dev, "Failed to get memory window address\n");
		return ret;
	}

	ntb_spad_write(ndev, VHOST_MW0_SIZE_LOWER, lower_32_bits(ntb->peer_mw_size));
	ntb_spad_write(ndev, VHOST_MW0_SIZE_UPPER, upper_32_bits(ntb->peer_mw_size));

	return 0;
}

/* ntb_vhost_probe - Initialize struct ntb_vhost when a new NTB device is
 *   created
 * @client: struct ntb_client * representing the ntb vhost client driver
 * @ndev: NTB device created by NTB HW driver
 *
 * Probe function to initialize struct ntb_vhost when a new NTB device is
 * created. Also get the supported MW0 size and MW0 address and write the MW0
 * address to the self scratchpad for the remote NTB virtio driver to read.
 */
static int ntb_vhost_probe(struct ntb_client *self, struct ntb_dev *ndev)
{
	struct device *dev = &ndev->dev;
	struct config_group *group;
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;
	int ret;

	ntb = kzalloc(sizeof(*ntb), GFP_KERNEL);
	if (!ntb)
		return -ENOMEM;

	ntb->ndev = ndev;
	ntb->dev = dev;

	ret = ntb_vhost_configure_mw(ntb);
	if (ret) {
		dev_err(dev, "Failed to configure memory window\n");
		goto err;
	}

	ret = ntb_set_ctx(ndev, ntb, &ntb_vhost_ops);
	if (ret) {
		dev_err(dev, "Failed to set NTB vhost context\n");
		goto err;
	}

	vdev = &ntb->vdev;
	vdev->dev.parent = dev;
	vdev->dev.release = ntb_vhost_release;
	vdev->ops = &ops;

	group = vhost_cfs_add_device_item(vdev);
	if (IS_ERR(group)) {
		dev_err(dev, "Failed to add configfs entry for vhost device\n");
		goto err;
	}

	ntb->group = group;

	INIT_DELAYED_WORK(&ntb->link_work, ntb_vhost_link_work);
	INIT_WORK(&ntb->link_cleanup, ntb_vhost_link_cleanup_work);

	ntb_link_enable(ndev, NTB_SPEED_AUTO, NTB_WIDTH_AUTO);

	return 0;

err:
	kfree(ntb);

	return ret;
}

/* ntb_vhost_free - Free the initializations performed by ntb_vhost_probe()
 * @client: struct ntb_client * representing the ntb vhost client driver
 * @ndev: NTB device created by NTB HW driver
 *
 * Free the initializations performed by ntb_vhost_probe().
 */
void ntb_vhost_free(struct ntb_client *client, struct ntb_dev *ndev)
{
	struct config_group *group;
	struct vhost_dev *vdev;
	struct ntb_vhost *ntb;

	ntb = ndev->ctx;
	vdev = &ntb->vdev;
	group = ntb->group;

	ntb_vhost_link_cleanup(ntb);
	ntb_link_disable(ndev);
	ntb_vhost_del_vqs(vdev);
	vhost_cfs_remove_device_item(group);
	if (device_is_registered(&vdev->dev))
		vhost_unregister_device(vdev);
}

static struct ntb_client ntb_vhost_client = {
	.ops = {
		.probe = ntb_vhost_probe,
		.remove = ntb_vhost_free,
	},
};

static int __init ntb_vhost_init(void)
{
	int ret;

	kntbvhost_workqueue = alloc_workqueue("kntbvhost", WQ_MEM_RECLAIM |
					      WQ_HIGHPRI, 0);
	if (!kntbvhost_workqueue) {
		pr_err("Failed to allocate kntbvhost_workqueue\n");
		return -ENOMEM;
	}

	ret = ntb_register_client(&ntb_vhost_client);
	if (ret) {
		pr_err("Failed to register ntb vhost driver --> %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(ntb_vhost_init);

static void __exit ntb_vhost_exit(void)
{
	ntb_unregister_client(&ntb_vhost_client);
	destroy_workqueue(kntbvhost_workqueue);
}
module_exit(ntb_vhost_exit);

MODULE_DESCRIPTION("NTB VHOST Driver");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
