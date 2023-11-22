// SPDX-License-Identifier: GPL-2.0
/**
 * Virtio PCI driver - Configurable PCIe Endpoint device Support
 *
 * The Configurable PCIe Endpoint device is present on an another system running
 * Linux and configured using drivers/pci/endpoint/functions/pci-epf-vhost.c
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/delay.h>
#if defined(CONFIG_ARCH_SSTAR)
#include <linux/rpmsg.h>
#endif
#include "virtio_pci_common.h"

#define DRV_MODULE_NAME		"virtio-pci-epf"

#define MAX_VQS		8

#define HOST_FEATURES_LOWER	0x00
#define HOST_FEATURES_UPPER	0x04
#define GUEST_FEATURES_LOWER	0x08
#define GUEST_FEATURES_UPPER	0x0C
#define MSIX_CONFIG		0x10
#define NUM_QUEUES		0x12
#define DEVICE_STATUS		0x14
#define CONFIG_GENERATION	0x15
#define ISR			0x16
#define HOST_CMD		0x1A
enum host_cmd {
	HOST_CMD_NONE,
	HOST_CMD_SET_STATUS,
	HOST_CMD_FINALIZE_FEATURES,
	HOST_CMD_RESET,
};

#define HOST_CMD_STATUS		0x1B
enum host_cmd_status {
	HOST_CMD_STATUS_NONE,
	HOST_CMD_STATUS_OKAY,
	HOST_CMD_STATUS_ERROR,
};

#define QUEUE_BASE		0x1C
#define CMD			0x00
enum queue_cmd {
	QUEUE_CMD_NONE,
	QUEUE_CMD_ACTIVATE,
	QUEUE_CMD_DEACTIVATE,
	QUEUE_CMD_NOTIFY,
};

#define CMD_STATUS		0x01
enum queue_cmd_status {
	QUEUE_CMD_STATUS_NONE,
	QUEUE_CMD_STATUS_OKAY,
	QUEUE_CMD_STATUS_ERROR,
};

#define STATUS			0x2
#define STATUS_ACTIVE		BIT(0)

#define NUM_BUFFERS		0x04
#define MSIX_VECTOR		0x06
#define ADDR_LOWER		0x08
#define ADDR_UPPER		0x0C
#define QUEUE_CMD(n)		(QUEUE_BASE + CMD + (n) * 16)
#define QUEUE_CMD_STATUS(n)	(QUEUE_BASE + CMD_STATUS + (n) * 16)
#define QUEUE_STATUS(n)		(QUEUE_BASE + STATUS + (n) * 16)
#define QUEUE_NUM_BUFFERS(n)	(QUEUE_BASE + NUM_BUFFERS + (n) * 16)
#define QUEUE_MSIX_VECTOR(n)	(QUEUE_BASE + MSIX_VECTOR + (n) * 16)
#define QUEUE_ADDR_LOWER(n)	(QUEUE_BASE + ADDR_LOWER + (n) * 16)
#define QUEUE_ADDR_UPPER(n)	(QUEUE_BASE + ADDR_UPPER + (n) * 16)

#define SHM_BAR_BASE_ADDRESS_LOWER	(QUEUE_BASE + MAX_VQS * 16)
#define SHM_BAR_BASE_ADDRESS_UPPER	(QUEUE_BASE + MAX_VQS * 16 + 4)

#define SHM_BAR_OFFSET                  (QUEUE_BASE + MAX_VQS * 16 + 8 + 5 * 4)
#define SHM_BAR_SIZE                    (QUEUE_BASE + MAX_VQS * 16 + 8 + 5 * 4 + 4)

#define TRIGGER_IRQ_ADDRESS	(QUEUE_BASE + MAX_VQS * 16 + 8)

#define DEVICE_CFG_SPACE	0x110

#define COMMAND_TIMEOUT	50000 /* 5 Sec */

struct virtio_pci_epf {
	/* mutex to protect sending commands to EPF vhost */
	struct mutex lock;
	struct virtio_pci_device vp_dev;
};

#define to_virtio_pci_epf(dev) container_of((dev), struct virtio_pci_epf, vp_dev)

/* virtio_pci_epf_send_command - Send commands to the remote EPF device running
 *   vhost driver
 * @vp_dev: Virtio PCIe device that communicates with the endpoint device
 * @command: The command that has to be sent to the remote endpoint device
 *
 * Helper function to send commands to the remote endpoint function device
 * running vhost driver.
 */
static int virtio_pci_epf_send_command(struct virtio_pci_device *vp_dev,
				       u32 command)
{
	struct virtio_pci_epf *pci_epf;
	void __iomem *ioaddr;
	ktime_t timeout;
	bool timedout;
	int ret = 0;
	u8 status;

	pci_epf = to_virtio_pci_epf(vp_dev);
	ioaddr = vp_dev->ioaddr;

	mutex_lock(&pci_epf->lock);
	writeb(command, ioaddr + HOST_CMD);
	barrier();
	writel(1, ioaddr + TRIGGER_IRQ_ADDRESS);
	timeout = ktime_add_ms(ktime_get(), COMMAND_TIMEOUT);
	while (1) {
		timedout = ktime_after(ktime_get(), timeout);
		status = readb(ioaddr + HOST_CMD_STATUS);

		if (status == HOST_CMD_STATUS_ERROR) {
			ret = -EINVAL;
			break;
		}

		if (status == HOST_CMD_STATUS_OKAY)
			break;

		if (WARN_ON(timedout)) {
			ret = -ETIMEDOUT;
			break;
		}

		usleep_range(5, 10);
	}

	writeb(HOST_CMD_STATUS_NONE, ioaddr + HOST_CMD_STATUS);
	mutex_unlock(&pci_epf->lock);

	return ret;
}

/* virtio_pci_epf_send_queue_command - Send commands to the remote EPF device
 *   for configuring virtqueue
 * @vp_dev: Virtio PCIe device that communicates with the endpoint device
 * @vq: The virtqueue that has to be configured on the remote endpoint device
 * @command: The command that has to be sent to the remote endpoint device
 *
 * Helper function to send commands to the remote endpoint function device for
 * configuring the virtqueue.
 */
static int virtio_pci_epf_send_queue_command(struct virtio_pci_device *vp_dev,
					     struct virtqueue *vq, u8 command)
{
	void __iomem *ioaddr;
	ktime_t timeout;
	bool timedout;
	int ret = 0;
	u8 status;

	ioaddr = vp_dev->ioaddr;

	mutex_lock(&vq->lock);
	writeb(command, ioaddr + QUEUE_CMD(vq->index));
	barrier();
	writel(vq->index + 1, ioaddr + TRIGGER_IRQ_ADDRESS);
	timeout = ktime_add_ms(ktime_get(), COMMAND_TIMEOUT);
	while (1) {
		timedout = ktime_after(ktime_get(), timeout);
		status = readb(ioaddr + QUEUE_CMD_STATUS(vq->index));

		if (status == QUEUE_CMD_STATUS_ERROR) {
			ret = -EINVAL;
			break;
		}

		if (status == QUEUE_CMD_STATUS_OKAY)
			break;

		if (WARN_ON(timedout)) {
			ret = -ETIMEDOUT;
			break;
		}

		usleep_range(5, 10);
	}

	writeb(QUEUE_CMD_STATUS_NONE, ioaddr + QUEUE_CMD_STATUS(vq->index));
	mutex_unlock(&vq->lock);

	return ret;
}

/* virtio_pci_epf_get_features - virtio_config_ops to get EPF vhost device
 *   features
 * @vdev: Virtio device that communicates with the remote EPF vhost device
 *
 * virtio_config_ops to get EPF vhost device features. The device features
 * are accessed using BAR mapped registers.
 */
static u64 virtio_pci_epf_get_features(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;
	u64 features;

	vp_dev = to_vp_device(vdev);
	ioaddr = vp_dev->ioaddr;

	features = readl(ioaddr + HOST_FEATURES_UPPER);
	features <<= 32;
	features |= readl(ioaddr + HOST_FEATURES_LOWER);

	return features;
}

/* virtio_pci_epf_finalize_features - virtio_config_ops to finalize features
 *   with remote EPF vhost device
 * @vdev: Virtio device that communicates with the remote vhost device
 *
 * Indicate the negotiated features to the remote EPF vhost device by sending
 * HOST_CMD_FINALIZE_FEATURES command.
 */
static int virtio_pci_epf_finalize_features(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;
	struct device *dev;
	int ret;

	vp_dev = to_vp_device(vdev);
	dev = &vp_dev->pci_dev->dev;
	ioaddr = vp_dev->ioaddr;

	/* Give virtio_ring a chance to accept features. */
	vring_transport_features(vdev);

	writel(lower_32_bits(vdev->features), ioaddr + GUEST_FEATURES_LOWER);
	writel(upper_32_bits(vdev->features), ioaddr + GUEST_FEATURES_UPPER);

	ret = virtio_pci_epf_send_command(vp_dev, HOST_CMD_FINALIZE_FEATURES);
	if (ret) {
		dev_err(dev, "Failed to set configuration event vector\n");
		return VIRTIO_MSI_NO_VECTOR;
	}

	return 0;
}

/* virtio_pci_epf_get - Copy the device configuration space data from
 *   EPF device to buffer provided by virtio driver
 * @vdev: Virtio device that communicates with the remote vhost device
 * @offset: Offset in the device configuration space
 * @buf: Buffer address from virtio driver where configuration space
 *   data has to be copied
 * @len: Length of the data from device configuration space to be copied
 *
 * Copy the device configuration space data to buffer provided by virtio
 * driver.
 */
static void virtio_pci_epf_get(struct virtio_device *vdev, unsigned int offset,
			       void *buf, unsigned int len)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;

	vp_dev = to_vp_device(vdev);
	ioaddr = vp_dev->ioaddr;

	memcpy_fromio(buf, ioaddr + DEVICE_CFG_SPACE + offset, len);
}

/* virtio_pci_epf_set - Copy the device configuration space data from buffer
 *   provided by virtio driver to EPF device
 * @vdev: Virtio device that communicates with the remote vhost device
 * @offset: Offset in the device configuration space
 * @buf: Buffer address provided by virtio driver which has the configuration
 *   space data to be copied
 * @len: Length of the data from device configuration space to be copied
 *
 * Copy the device configuration space data from buffer provided by virtio
 * driver to the EPF device.
 */
static void virtio_pci_epf_set(struct virtio_device *vdev, unsigned int offset,
			       const void *buf, unsigned int len)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;

	vp_dev = to_vp_device(vdev);
	ioaddr = vp_dev->ioaddr;

	memcpy_toio(ioaddr + DEVICE_CFG_SPACE + offset, buf, len);
}

/* virtio_pci_epf_get_status - EPF virtio_config_ops to get device status
 * @vdev: Virtio device that communicates with the remote vhost device
 *
 * EPF virtio_config_ops to get device status. The remote EPF vhost device
 * populates the vhost device status in BAR mapped region.
 */
static u8 virtio_pci_epf_get_status(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;

	vp_dev = to_vp_device(vdev);
	ioaddr = vp_dev->ioaddr;

	return readb(ioaddr + DEVICE_STATUS);
}

/* virtio_pci_epf_set_status - EPF virtio_config_ops to set device status
 * @vdev: Virtio device that communicates with the remote vhost device
 *
 * EPF virtio_config_ops to set device status. This function updates the
 * status in scratchpad register and sends a notification to the vhost
 * device using HOST_CMD_SET_STATUS command.
 */
static void virtio_pci_epf_set_status(struct virtio_device *vdev, u8 status)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;
	struct device *dev;
	int ret;

	vp_dev = to_vp_device(vdev);
	dev = &vp_dev->pci_dev->dev;
	ioaddr = vp_dev->ioaddr;

	/* We should never be setting status to 0. */
	if (WARN_ON(!status))
		return;

	writeb(status, ioaddr + DEVICE_STATUS);

	ret = virtio_pci_epf_send_command(vp_dev, HOST_CMD_SET_STATUS);
	if (ret)
		dev_err(dev, "Failed to set device status\n");
}

/* virtio_pci_epf_reset - EPF virtio_config_ops to reset the device
 * @vdev: Virtio device that communicates with the remote vhost device
 *
 * EPF virtio_config_ops to reset the device. This sends HOST_CMD_RESET
 * command to reset the device.
 */
static void virtio_pci_epf_reset(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev;
	void __iomem *ioaddr;
	struct device *dev;
	int ret;

	vp_dev = to_vp_device(vdev);
	dev = &vp_dev->pci_dev->dev;
	ioaddr = vp_dev->ioaddr;

	ret = virtio_pci_epf_send_command(vp_dev, HOST_CMD_RESET);
	if (ret)
		dev_err(dev, "Failed to reset device\n");
}

/* virtio_pci_epf_config_vector - virtio_pci_device ops to set config vector
 * @vp_dev: Virtio PCI device managed by virtio_pci_common.c
 *
 * virtio_pci_device ops to set config vector. This writes the config MSI-X
 * vector to the BAR mapped region.
 */
static u16 virtio_pci_epf_config_vector(struct virtio_pci_device *vp_dev,
					u16 vector)
{
	void __iomem *ioaddr = vp_dev->ioaddr;

	writew(vector, ioaddr + MSIX_CONFIG);

	return readw(ioaddr + MSIX_CONFIG);
}

/* virtio_pci_epf_notify - Send notification to the remote vhost virtqueue
 * @vq: The local virtio virtqueue corresponding to the remote vhost virtqueue
 *   where the notification has to be sent
 *
 * Send notification to the remote vhost virtqueue by using QUEUE_CMD_NOTIFY
 * command.
 */
static bool virtio_pci_epf_notify(struct virtqueue *vq)
{
	struct virtio_pci_device *vp_dev;
	struct device *dev;
	int ret;

	vp_dev = vq->priv;
	dev = &vp_dev->pci_dev->dev;

	ret = virtio_pci_epf_send_queue_command(vp_dev, vq, QUEUE_CMD_NOTIFY);
	if (ret) {
		dev_err(dev, "Notifying virtqueue: %d Failed\n", vq->index);
		return false;
	}

	return true;
}

/* virtio_pci_epf_setup_vq - Configure virtqueue
 * @vp_dev: Virtio PCI device managed by virtio_pci_common.c
 * @info: Wrapper to virtqueue to maintain list of all queues
 * @callback: Callback function that has to be associated with virtqueue
 * @vq: The local virtio virtqueue corresponding to the remote vhost virtqueue
 *   where the notification has to be sent
 *
 * Configure virtqueue with the number of buffers provided by EPF vhost device
 * and associate a callback function for each of these virtqueues.
 */
static struct virtqueue *
virtio_pci_epf_setup_vq(struct virtio_pci_device *vp_dev,
			struct virtio_pci_vq_info *info, unsigned int index,
			void (*callback)(struct virtqueue *vq),
			const char *name, bool ctx, u16 msix_vec)
{
	u16 queue_num_buffers;
	void __iomem *ioaddr;
	struct virtqueue *vq;
	struct device *dev;
	dma_addr_t vq_addr;
	u16 status;
	int err;

	dev = &vp_dev->pci_dev->dev;
	ioaddr = vp_dev->ioaddr;

	status = readw(ioaddr + QUEUE_STATUS(index));
	if (status & STATUS_ACTIVE) {
		dev_err(dev, "Virtqueue %d is already active\n", index);
		return ERR_PTR(-ENOENT);
	}

	queue_num_buffers = readw(ioaddr + QUEUE_NUM_BUFFERS(index));
	if (!queue_num_buffers) {
		dev_err(dev, "Virtqueue %d is not available\n", index);
		return ERR_PTR(-ENOENT);
	}

	info->msix_vector = msix_vec;

	vq = vring_create_virtqueue(index, queue_num_buffers,
				    VIRTIO_PCI_VRING_ALIGN, &vp_dev->vdev, true,
				    false, ctx, virtio_pci_epf_notify, callback,
				    name);
	if (!vq) {
		dev_err(dev, "Failed to create Virtqueue %d\n", index);
		return ERR_PTR(-ENOMEM);
	}
	mutex_init(&vq->lock);

	vq_addr = virtqueue_get_desc_addr(vq);
	writel(lower_32_bits(vq_addr), ioaddr + QUEUE_ADDR_LOWER(index));
	writel(upper_32_bits(vq_addr), ioaddr + QUEUE_ADDR_UPPER(index));

	vq->priv = vp_dev;
	writew(QUEUE_CMD_ACTIVATE, ioaddr + QUEUE_CMD(index));

	err = virtio_pci_epf_send_queue_command(vp_dev, vq, QUEUE_CMD_ACTIVATE);
	if (err) {
		dev_err(dev, "Failed to activate Virtqueue %d\n", index);
		goto out_del_vq;
	}

	if (msix_vec != VIRTIO_MSI_NO_VECTOR)
		writew(msix_vec, ioaddr + QUEUE_MSIX_VECTOR(index));

	return vq;

out_del_vq:
	vring_del_virtqueue(vq);

	return ERR_PTR(err);
}

/* virtio_pci_epf_del_vq - Free memory allocated for virtio virtqueues
 * @info: Wrapper to virtqueue to maintain list of all queues
 *
 * Free memory allocated for a virtqueue represented by @info
 */
static void virtio_pci_epf_del_vq(struct virtio_pci_vq_info *info)
{
	struct virtio_pci_device *vp_dev;
	struct virtqueue *vq;
	void __iomem *ioaddr;
	unsigned int index;

	vq = info->vq;
	vp_dev = to_vp_device(vq->vdev);
	ioaddr = vp_dev->ioaddr;
	index = vq->index;

	if (vp_dev->msix_enabled)
		writew(VIRTIO_MSI_NO_VECTOR, ioaddr + QUEUE_MSIX_VECTOR(index));

	writew(QUEUE_CMD_DEACTIVATE, ioaddr + QUEUE_CMD(index));
	vring_del_virtqueue(vq);
}

static const struct virtio_config_ops virtio_pci_epf_config_ops = {
	.get		= virtio_pci_epf_get,
	.set		= virtio_pci_epf_set,
	.get_status	= virtio_pci_epf_get_status,
	.set_status	= virtio_pci_epf_set_status,
	.reset		= virtio_pci_epf_reset,
	.find_vqs	= vp_find_vqs,
	.del_vqs	= vp_del_vqs,
	.get_features	= virtio_pci_epf_get_features,
	.finalize_features = virtio_pci_epf_finalize_features,
	.bus_name	= vp_bus_name,
	.set_vq_affinity = vp_set_vq_affinity,
	.get_vq_affinity = vp_get_vq_affinity,
};

/* virtio_pci_epf_release_dev - Callback function to free device
 * @dev: Device in virtio_device that has to be freed
 *
 * Callback function from device core invoked to free the device after
 * all references have been removed. This frees the allocated memory for
 * struct virtio_pci_epf.
 */
static void virtio_pci_epf_release_dev(struct device *dev)
{
	struct virtio_pci_device *vp_dev;
	struct virtio_pci_epf *pci_epf;
	struct virtio_device *vdev;

	vdev = dev_to_virtio(dev);
	vp_dev = to_vp_device(vdev);
	pci_epf = to_virtio_pci_epf(vp_dev);

	kfree(pci_epf);
}

int virtio_pci_get_domain_nr_of_host_bridge(struct pci_dev *pdev)
{
    struct pci_bus *bus = pdev->bus;

    while (bus->parent) {
        bus = bus->parent;
    }

    return bus->domain_nr;
}

/* virtio_pci_epf_probe - Initialize struct virtio_pci_epf when a new PCIe
 *   device is created
 * @pdev: The pci_dev that is created by the PCIe core during enumeration
 * @id: pci_device_id of the @pdev
 *
 * Probe function to initialize struct virtio_pci_epf when a new PCIe device is
 * created.
 */
static int virtio_pci_epf_probe(struct pci_dev *pdev,
				const struct pci_device_id *id)
{
	struct virtio_pci_device *vp_dev, *reg_dev = NULL;
	struct virtio_pci_epf *pci_epf;
	struct device *dev;
	int err;
	int domain_nr;
	u64 shm_base;
	u64 shm_size;
	void *shm;

	if (pci_is_bridge(pdev))
		return -ENODEV;

	pci_epf = kzalloc(sizeof(*pci_epf), GFP_KERNEL);
	if (!pci_epf)
		return -ENOMEM;

	dev = &pdev->dev;
	vp_dev = &pci_epf->vp_dev;
	vp_dev->vdev.dev.parent = dev;
	vp_dev->vdev.dev.release = virtio_pci_epf_release_dev;
	vp_dev->pci_dev = pdev;
	INIT_LIST_HEAD(&vp_dev->virtqueues);
	spin_lock_init(&vp_dev->lock);
	mutex_init(&pci_epf->lock);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(dev, "Cannot enable PCI device\n");
		goto err_enable_device;
	}

	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(dev, "Cannot set DMA mask\n");
		goto err_dma_set_mask;
	}

	err = pci_request_regions(pdev, DRV_MODULE_NAME);
	if (err) {
		dev_err(dev, "Cannot obtain PCI resources\n");
		goto err_dma_set_mask;
	}

	pci_set_master(pdev);

	vp_dev->ioaddr = pci_ioremap_bar(pdev, 0);
	if (!vp_dev->ioaddr) {
		dev_err(dev, "Failed to read BAR0\n");
		goto err_ioremap1;
	}

	shm_base = pci_resource_start(pdev, 0) + readl(vp_dev->ioaddr +
						       SHM_BAR_OFFSET);
	shm_size = readl(vp_dev->ioaddr + SHM_BAR_SIZE);
	shm = vp_dev->ioaddr + readl(vp_dev->ioaddr + SHM_BAR_OFFSET);

	vp_dev->vdev.mem_virt = shm;
	vp_dev->vdev.mem_phys = shm_base;
	vp_dev->vdev.mem_offset = 0x0;

	writel(lower_32_bits(shm_base), vp_dev->ioaddr + SHM_BAR_BASE_ADDRESS_LOWER);
	writel(upper_32_bits(shm_base), vp_dev->ioaddr + SHM_BAR_BASE_ADDRESS_UPPER);

	pci_set_drvdata(pdev, vp_dev);
	vp_dev->isr = vp_dev->ioaddr + ISR;

	/*
	 * we use the subsystem vendor/device id as the virtio vendor/device
	 * id.  this allows us to use the same PCI vendor/device id for all
	 * virtio devices and to identify the particular virtio driver by
	 * the subsystem ids
	 */
	vp_dev->vdev.id.vendor = pdev->subsystem_vendor;
	vp_dev->vdev.id.device = pdev->subsystem_device;

	vp_dev->vdev.config = &virtio_pci_epf_config_ops;
#if defined(CONFIG_ARCH_SSTAR)
	/* we use vendor id to pass type and dev_id to upper layer */
	domain_nr = virtio_pci_get_domain_nr_of_host_bridge(pdev);
	if (domain_nr == 1)
		vp_dev->vdev.id.vendor =
			Rpmsg_Device_RC_PORT << 16 | (pdev->bus->number + 128);
	else
		vp_dev->vdev.id.vendor =
			Rpmsg_Device_RC_PORT << 16 | pdev->bus->number;
#endif
	vp_dev->config_vector = virtio_pci_epf_config_vector;
	vp_dev->setup_vq = virtio_pci_epf_setup_vq;
	vp_dev->del_vq = virtio_pci_epf_del_vq;

	err = register_virtio_device(&vp_dev->vdev);
	reg_dev = vp_dev;
	if (err) {
		dev_err(dev, "Failed to register VIRTIO device\n");
		goto err_register_virtio;
	}

	return 0;

err_register_virtio:
	iounmap(shm);

err_ioremap1:
	pci_release_regions(pdev);

err_dma_set_mask:
	pci_disable_device(pdev);

err_enable_device:
	if (reg_dev)
		put_device(&vp_dev->vdev.dev);
	else
		kfree(pci_epf);

	return err;
}

/* virtio_pci_epf_remove - Free the initializations performed by virtio_pci_epf_probe()
 * @pdev: The pci_dev that is created by the PCIe core during enumeration
 *
 * Free the initializations performed by virtio_pci_epf_probe().
 */
void virtio_pci_epf_remove(struct pci_dev *pdev)
{
	struct virtio_pci_device *vp_dev;

	vp_dev = pci_get_drvdata(pdev);

	unregister_virtio_device(&vp_dev->vdev);
	pci_iounmap(pdev, vp_dev->ioaddr);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
}

static const struct pci_device_id virtio_pci_epf_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_DRA74x),
	},
	{ PCI_DEVICE(PCI_VENDOR_ID_TI, PCI_DEVICE_ID_TI_DRA72x),
	},
	{ }
};
MODULE_DEVICE_TABLE(pci, virtio_pci_epf_table);

static struct pci_driver virtio_pci_epf_driver = {
	.name		= DRV_MODULE_NAME,
	.id_table	= virtio_pci_epf_table,
	.probe		= virtio_pci_epf_probe,
	.remove		= virtio_pci_epf_remove,
	.sriov_configure = pci_sriov_configure_simple,
};
module_pci_driver(virtio_pci_epf_driver);

MODULE_DESCRIPTION("VIRTIO PCI EPF DRIVER");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
