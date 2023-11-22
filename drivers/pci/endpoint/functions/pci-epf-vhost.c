// SPDX-License-Identifier: GPL-2.0
/**
 * Endpoint Function Driver to implement VHOST functionality
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/vhost.h>
#include <linux/vringh.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>

#if defined(CONFIG_ARCH_SSTAR)
#include <linux/rpmsg.h>
#endif

#include <uapi/linux/virtio_pci.h>

#include "pcie-designware.h"
#include "ms_platform.h"
#include "registers.h"
#include "drv_pcieif.h"
#include "pcie-sstar.h"

#define MAX_VQS		8

#define VHOST_QUEUE_STATUS_ENABLE	BIT(0)

#define VHOST_DEVICE_CONFIG_SIZE	1024
#define EPF_VHOST_MAX_INTERRUPTS	(MAX_VQS + 1)

#define EPF_VHOST_REG_BASE0 (MIU0_BASE + 0x200000)
#define EPF_VHOST_REG_SIZE0 (0x00100000)

#define EPF_VHOST_REG_BASE1 (MIU0_BASE + 0x300000)
#define EPF_VHOST_REG_SIZE1 (0x00100000)

static struct workqueue_struct *kpcivhost_workqueue;

struct epf_vhost_queue {
	wait_queue_head_t wq;
	struct task_struct *task;
	struct vhost_virtqueue *vq;
	struct epf_vhost *vhost;
	phys_addr_t phys_addr;
	void __iomem *addr;
	unsigned int size;
};

struct epf_vhost {
	const struct pci_epc_features *epc_features;
	struct epf_vhost_queue vqueue[MAX_VQS];
	struct delayed_work cfs_work;
	struct epf_vhost_reg *reg;
	struct config_group group;
	size_t msix_table_offset;
	struct vhost_dev vdev;
	struct pci_epf *epf;
	struct vring vring;
	wait_queue_head_t wq;
	struct task_struct *task;
	int msix_bar;
	int shm_bar;
	void *shm;
	phys_addr_t shm_phys;
	struct pci_epf_bar  epf_bar[6];
	bool pcie_boot;
};

static inline struct epf_vhost *to_epf_vhost_from_ci(struct config_item *item)
{
	return container_of(to_config_group(item), struct epf_vhost, group);
}

#define to_epf_vhost(v) container_of((v), struct epf_vhost, vdev)

struct epf_vhost_reg_queue {
	u8 cmd;
	u8 cmd_status;
	u16 status;
	u16 num_buffers;
	u16 msix_vector;
	u64 queue_addr;
} __packed;

enum queue_cmd {
	QUEUE_CMD_NONE,
	QUEUE_CMD_ACTIVATE,
	QUEUE_CMD_DEACTIVATE,
	QUEUE_CMD_NOTIFY,
};

enum queue_cmd_status {
	QUEUE_CMD_STATUS_NONE,
	QUEUE_CMD_STATUS_OKAY,
	QUEUE_CMD_STATUS_ERROR,
};

struct epf_vhost_reg {
	u64 host_features;
	u64 guest_features;
	u16 msix_config;
	u16 num_queues;
	u8 device_status;
	u8 config_generation;
	u32 isr;
	u8 cmd;
	u8 cmd_status;
	struct epf_vhost_reg_queue vq[MAX_VQS];
	u64 shm_bar_base_addr;
	u32 trigger_irq;
	u32 tvq_available_idx;
	u32 rvq_available_idx;
	u32 tvq_ring_used_idx;
	u32 rvq_ring_used_idx;
	u32 shm_bar_offset;
	u32 shm_bar_size;
} __packed;

enum host_cmd {
	HOST_CMD_NONE,
	HOST_CMD_SET_STATUS,
	HOST_CMD_FINALIZE_FEATURES,
	HOST_CMD_RESET,
};

enum host_cmd_status {
	HOST_CMD_STATUS_NONE,
	HOST_CMD_STATUS_OKAY,
	HOST_CMD_STATUS_ERROR,
};

static struct pci_epf_header epf_vhost_header = {
	.vendorid	= 0x104c,
	.deviceid	= 0xb500,
	.subsys_id	= 0x0007,
	.subsys_vendor_id = PCI_ANY_ID,
	.baseclass_code	= PCI_CLASS_OTHERS,
	.interrupt_pin	= PCI_INTERRUPT_INTA,
};

/* pci_epf_vhost_cmd_handler - Handle commands from remote EPF virtio driver
 * @work: The work_struct holding the pci_epf_vhost_cmd_handler() function that
 *   is scheduled
 *
 * Handle commands from the remote EPF virtio driver and sends notification to
 * the vhost client driver. The remote EPF virtio driver sends commands when the
 * virtio driver status is updated or when the feature negotiation is complete or
 * if the virtio driver wants to reset the device.
 */
static int pci_epf_vhost_cmd_handler(void *arg)
{
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;
	struct vhost_dev *vdev;
	struct device *dev;
	u8 command;
	DEFINE_WAIT(wait);

	vhost = (struct epf_vhost *)arg;
	vdev = &vhost->vdev;
	dev = &vhost->epf->dev;
	reg = vhost->reg;

	while (1) {
		if (kthread_should_stop())
			break;

		prepare_to_wait(&vhost->wq, &wait, TASK_UNINTERRUPTIBLE);
		command = reg->cmd;
		if (!command)
			schedule();
		command = reg->cmd;
		finish_wait(&vhost->wq, &wait);

		if (!command)
			continue;

		reg->cmd = 0;

		switch (command) {
		case HOST_CMD_SET_STATUS:
			blocking_notifier_call_chain(&vdev->notifier, NOTIFY_SET_STATUS,
						     NULL);
			reg->cmd_status = HOST_CMD_STATUS_OKAY;
			break;
		case HOST_CMD_FINALIZE_FEATURES:
			vdev->features = reg->guest_features;
			blocking_notifier_call_chain(&vdev->notifier,
						     NOTIFY_FINALIZE_FEATURES, 0);
			reg->cmd_status = HOST_CMD_STATUS_OKAY;
			break;
		case HOST_CMD_RESET:
			blocking_notifier_call_chain(&vdev->notifier, NOTIFY_RESET, 0);
			reg->cmd_status = HOST_CMD_STATUS_OKAY;
			break;
		default:
			dev_err(dev, "UNKNOWN command: %d\n", command);
			break;
		}
	}
	return 0;
}

/* pci_epf_vhost_queue_activate - Map virtqueue local address to remote
 *   virtqueue address provided by EPF virtio
 * @vqueue: struct epf_vhost_queue holding the local virtqueue address
 *
 * In order for the local system to access the remote virtqueue, the address
 * reserved in local system should be mapped to the remote virtqueue address.
 * Map local virtqueue address to remote virtqueue address here.
 */
static int pci_epf_vhost_queue_activate(struct epf_vhost_queue *vqueue)
{
	struct epf_vhost_reg_queue *reg_queue;
	struct vhost_virtqueue *vq;
	struct epf_vhost_reg *reg;
	phys_addr_t vq_phys_addr;
	struct epf_vhost *vhost;
	struct pci_epf *epf;
	struct pci_epc *epc;
	struct device *dev;
	u64 vq_remote_addr;
	size_t vq_size;
	u8 func_no;
	int ret;

	vhost = vqueue->vhost;
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;
	func_no = epf->func_no;

	vq = vqueue->vq;
	reg = vhost->reg;
	reg_queue = &reg->vq[vq->index];
	vq_phys_addr = vqueue->phys_addr;
	vq_remote_addr = reg_queue->queue_addr;
	vq_size = vqueue->size;

	if (!vhost->shm) {
		ret = pci_epc_map_addr(epc, func_no, vq_phys_addr,
				       vq_remote_addr, vq_size);
		if (ret) {
			dev_err(dev, "Failed to map outbound address\n");
			return ret;
		}
	}

	reg_queue->status |= VHOST_QUEUE_STATUS_ENABLE;

	return 0;
}

/* pci_epf_vhost_queue_deactivate - Unmap virtqueue local address from remote
 *   virtqueue address
 * @vqueue: struct epf_vhost_queue holding the local virtqueue address
 *
 * Unmap virtqueue local address from remote virtqueue address.
 */
static void pci_epf_vhost_queue_deactivate(struct epf_vhost_queue *vqueue)
{
	struct epf_vhost_reg_queue *reg_queue;
	struct vhost_virtqueue *vq;
	struct epf_vhost_reg *reg;
	phys_addr_t vq_phys_addr;
	struct epf_vhost *vhost;
	struct pci_epf *epf;
	struct pci_epc *epc;
	u8 func_no;

	vhost = vqueue->vhost;

	epf = vhost->epf;
	epc = epf->epc;
	func_no = epf->func_no;
	vq_phys_addr = vqueue->phys_addr;

	if (!vhost->shm)
		pci_epc_unmap_addr(epc, func_no, vq_phys_addr);

	reg = vhost->reg;
	vq = vqueue->vq;
	reg_queue = &reg->vq[vq->index];
	reg_queue->status &= ~VHOST_QUEUE_STATUS_ENABLE;
}

/* pci_epf_vhost_queue_cmd_handler - Handle commands from remote EPF virtio
 *   driver sent for a particular virtqueue
 * @work: The work_struct holding the pci_epf_vhost_queue_cmd_handler()
 *   function that is scheduled
 *
 * Handle commands from the remote EPF virtio driver sent for a particular
 * virtqueue to activate/de-activate a virtqueue or to send notification to
 * the vhost client driver.
 */
static int pci_epf_vhost_queue_cmd_handler(void *arg)
{
	struct epf_vhost_reg_queue *reg_queue;
	struct epf_vhost_queue *vqueue;
	struct vhost_virtqueue *vq;
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;
	struct device *dev;
	u8 command;
	int ret;
	DEFINE_WAIT(wait);

	vqueue = (struct epf_vhost_queue *)arg;
	vhost = vqueue->vhost;
	reg = vhost->reg;
	vq = vqueue->vq;
	reg_queue = &reg->vq[vq->index];
	dev = &vhost->epf->dev;

	while (1) {
		if (kthread_should_stop())
			break;

		prepare_to_wait(&vqueue->wq, &wait, TASK_UNINTERRUPTIBLE);
		command = reg_queue->cmd;
		if (!command)
			schedule();
		command = reg_queue->cmd;
		finish_wait(&vqueue->wq, &wait);

		if (!command)
			continue;

		reg_queue->cmd = 0;
		vq = vqueue->vq;

		switch (command) {
		case QUEUE_CMD_ACTIVATE:
			ret = pci_epf_vhost_queue_activate(vqueue);
			if (ret)
				reg_queue->cmd_status = QUEUE_CMD_STATUS_ERROR;
			else
				reg_queue->cmd_status = QUEUE_CMD_STATUS_OKAY;
			break;
		case QUEUE_CMD_DEACTIVATE:
			pci_epf_vhost_queue_deactivate(vqueue);
			reg_queue->cmd_status = QUEUE_CMD_STATUS_OKAY;
			break;
		case QUEUE_CMD_NOTIFY:
			vhost_virtqueue_callback(vqueue->vq);
			reg_queue->cmd_status = QUEUE_CMD_STATUS_OKAY;
			break;
		default:
			dev_err(dev, "UNKNOWN QUEUE command: %d\n", command);
			break;
		}
	}
	return 0;
}

/* pci_epf_vhost_write - Write data to buffer provided by remote virtio driver
 * @vdev: Vhost device that communicates with remove virtio device
 * @dst: Buffer address present in the memory of the remote system to which
 *   data should be written
 * @src: Buffer address in the local device provided by the vhost client driver
 * @len: Length of the data to be copied from @src to @dst
 *
 * Write data to buffer provided by remote virtio driver from buffer provided
 * by vhost client driver.
 */
static int pci_epf_vhost_write(struct vhost_dev *vdev, u64 dst, void *src, int len)
{
	const struct pci_epc_features *epc_features;
	struct epf_vhost *vhost;
	phys_addr_t phys_addr;
	struct pci_epf *epf;
	struct pci_epc *epc;
	void __iomem *addr;
	struct device *dev;
	int offset, ret;
	u64 dst_addr;
	size_t align;
	u8 func_no;

	vhost = to_epf_vhost(vdev);
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;
	func_no = epf->func_no;
	epc_features = vhost->epc_features;
	align = epc_features->align;

	offset = dst & (align - 1);
	dst_addr = dst & ~(align - 1);

	if (vhost->shm) {
		offset += dst_addr - vhost->reg->shm_bar_base_addr;
		memcpy(vhost->shm + offset, src, len);
		return 0;
	}

	addr = pci_epc_mem_alloc_addr(epc, &phys_addr, len);
	if (!addr) {
		dev_err(dev, "Failed to allocate outbound address\n");
		return -ENOMEM;
	}

	ret = pci_epc_map_addr(epc, func_no, phys_addr, dst_addr, len);
	if (ret) {
		dev_err(dev, "Failed to map outbound address\n");
		goto ret;
	}

	memcpy_toio(addr + offset, src, len);

	pci_epc_unmap_addr(epc, func_no, phys_addr);

ret:
	pci_epc_mem_free_addr(epc, phys_addr, addr, len);

	return ret;
}

/* ntb_vhost_read - Read data from buffer provided by remote virtio driver
 * @vdev: Vhost device that communicates with remove virtio device
 * @dst: Buffer address in the local device provided by the vhost client driver
 * @src: Buffer address in the remote device provided by the remote virtio
 *   driver
 * @len: Length of the data to be copied from @src to @dst
 *
 * Read data from buffer provided by remote virtio driver to address provided
 * by vhost client driver.
 */
static int pci_epf_vhost_read(struct vhost_dev *vdev, void *dst, u64 src, int len)
{
	const struct pci_epc_features *epc_features;
	struct epf_vhost *vhost;
	phys_addr_t phys_addr;
	struct pci_epf *epf;
	struct pci_epc *epc;
	void __iomem *addr;
	struct device *dev;
	int offset, ret;
	u64 src_addr;
	size_t align;
	u8 func_no;

	vhost = to_epf_vhost(vdev);
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;
	func_no = epf->func_no;
	epc_features = vhost->epc_features;
	align = epc_features->align;

	offset = src & (align - 1);
	src_addr = src & ~(align - 1);

	if (vhost->shm) {
		u64 offset = src - vhost->reg->shm_bar_base_addr;
		memcpy(dst, vhost->shm + offset, len);
		return 0;
	}

	addr = pci_epc_mem_alloc_addr(epc, &phys_addr, len);
	if (!addr) {
		dev_err(dev, "Failed to allocate outbound address\n");
		return -ENOMEM;
	}

	ret = pci_epc_map_addr(epc, func_no, phys_addr, src_addr, len);
	if (ret) {
		dev_err(dev, "Failed to map outbound address\n");
		goto ret;
	}

	memcpy_fromio(dst, addr + offset, len);

	pci_epc_unmap_addr(epc, func_no, phys_addr);

ret:
	pci_epc_mem_free_addr(epc, phys_addr, addr, len);

	return ret;
}

/* pci_epf_vhost_notify - Send notification to the remote virtqueue
 * @vq: The local vhost virtqueue corresponding to the remote virtio virtqueue
 *
 * Use endpoint core framework to raise MSI-X interrupt to notify the remote
 * virtqueue.
 */
static void  pci_epf_vhost_notify(struct vhost_virtqueue *vq)
{
	struct epf_vhost_reg_queue *reg_queue;
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;
	struct vhost_dev *vdev;
	struct pci_epf *epf;
	struct pci_epc *epc;
	u8 func_no;

	vdev = vq->dev;
	vhost = to_epf_vhost(vdev);
	epf = vhost->epf;
	func_no = epf->func_no;
	epc = epf->epc;
	reg = vhost->reg;
	reg_queue = &reg->vq[vq->index];

	pci_epc_raise_irq(epc, func_no, PCI_EPC_IRQ_MSIX,
			  reg_queue->msix_vector + 1);
}

/* pci_epf_vhost_del_vqs - Delete all the vqs associated with the vhost device
 * @vdev: Vhost device that communicates with remove virtio device
 *
 * Delete all the vqs associated with the vhost device and free the memory
 * address reserved for accessing the remote virtqueue.
 */
static void pci_epf_vhost_del_vqs(struct vhost_dev *vdev)
{
	struct epf_vhost_queue *vqueue;
	struct vhost_virtqueue *vq;
	phys_addr_t vq_phys_addr;
	struct epf_vhost *vhost;
	void __iomem *vq_addr;
	unsigned int vq_size;
	struct pci_epf *epf;
	struct pci_epc *epc;
	int i;

	vhost = to_epf_vhost(vdev);
	epf = vhost->epf;
	epc = epf->epc;

	for (i = 0; i < vdev->nvqs; i++) {
		vq = vdev->vqs[i];
		if (IS_ERR_OR_NULL(vq))
			continue;

		vringh_mmiov_cleanup(&vq->wiov);
		vringh_mmiov_cleanup(&vq->riov);
		vqueue = &vhost->vqueue[i];
		if (vqueue->task) {
			kthread_stop(vqueue->task);
			vqueue->task = NULL;
		}
		vq_phys_addr = vqueue->phys_addr;
		vq_addr = vqueue->addr;
		vq_size = vqueue->size;
		pci_epc_mem_free_addr(epc, vq_phys_addr, vq_addr, vq_size);
		kfree(vq);
	}
}

/* pci_epf_vhost_create_vq - Create a new vhost virtqueue
 * @vdev: Vhost device that communicates with remove virtio device
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
pci_epf_vhost_create_vq(struct vhost_dev *vdev, int index,
			unsigned int num_bufs,
			void (*callback)(struct vhost_virtqueue *))
{
	struct epf_vhost_reg_queue *reg_queue;
	struct epf_vhost_queue *vqueue;
	struct epf_vhost_reg *reg;
	struct vhost_virtqueue *vq;
	phys_addr_t vq_phys_addr;
	struct epf_vhost *vhost;
	struct vringh *vringh;
	void __iomem *vq_addr;
	unsigned int vq_size;
	struct vring *vring;
	struct pci_epf *epf;
	struct pci_epc *epc;
	struct device *dev;
	int ret;

	vhost = to_epf_vhost(vdev);
	vqueue = &vhost->vqueue[index];
	reg = vhost->reg;
	reg_queue = &reg->vq[index];
	epf = vhost->epf;
	epc = epf->epc;
	dev = &epf->dev;

	vq = kzalloc(sizeof(*vq), GFP_KERNEL);
	if (!vq)
		return ERR_PTR(-ENOMEM);

	vq->dev = vdev;
	vq->callback = callback;
	vq->num = num_bufs;
	vq->index = index;
	vq->notify = pci_epf_vhost_notify;
	vq->type = VHOST_TYPE_MMIO;
	vringh_mmiov_init(&vq->wiov, NULL, 0);
	vringh_mmiov_init(&vq->riov, NULL, 0);

	vqueue->vq = vq;
	vqueue->vhost = vhost;

	vringh = &vq->vringh;
	vring = &vringh->vring;
	reg_queue->num_buffers = num_bufs;

	vq_size = vring_size(num_bufs, VIRTIO_PCI_VRING_ALIGN);
	if (vhost->shm) {
		if (index == 0) {
			vq_addr = vhost->shm;
			vq_phys_addr = vhost->shm_phys;
		} else {
			vq_addr = vhost->shm + 0x4000;
			vq_phys_addr = vhost->shm_phys + 0x4000;
		}
	} else {
		vq_addr = pci_epc_mem_alloc_addr(epc, &vq_phys_addr, vq_size);
	}
	if (!vq_addr) {
		dev_err(dev, "Failed to allocate virtqueue address\n");
		ret = -ENOMEM;
		goto err_mem_alloc_addr;
	}

	vring_init(vring, num_bufs, vq_addr, VIRTIO_PCI_VRING_ALIGN);
	ret = vringh_init_with_initial_idx_mmio(
		vringh, 0, num_bufs, false, vring->desc, vring->avail,
		vring->used,
		index == 0 ? reg->tvq_available_idx : reg->rvq_available_idx,
		index == 0 ? reg->tvq_ring_used_idx : reg->rvq_ring_used_idx);
	if (ret) {
		dev_err(dev, "Failed to init vringh\n");
		goto err_init_mmio;
	}

	vqueue->phys_addr = vq_phys_addr;
	vqueue->addr = vq_addr;
	vqueue->size = vq_size;

	init_waitqueue_head(&vqueue->wq);
	vqueue->task = kthread_create(pci_epf_vhost_queue_cmd_handler,
				      vqueue, "vqueue%d_handler", index);
	if (IS_ERR(vqueue->task)) {
		ret =  PTR_ERR(vqueue->task);
		vqueue->task = NULL;
		dev_err(dev, "%s failed to create cmd handler for vqueue %d, err = %ld",
		       __func__, index, PTR_ERR(vqueue->task));
		goto err_init_mmio;
	}
	set_user_nice(vqueue->task, MIN_NICE);
	wake_up_process(vqueue->task);

	return vq;

err_init_mmio:
	pci_epc_mem_free_addr(epc, vq_phys_addr, vq_addr, vq_size);

err_mem_alloc_addr:
	kfree(vq);

	return ERR_PTR(ret);
}

/* pci_epf_vhost_create_vqs - Create vhost virtqueues for vhost device
 * @vdev: Vhost device that communicates with the remote virtio device
 * @nvqs: Number of vhost virtqueues to be created
 * @num_bufs: The number of buffers that should be supported by the vhost
 *   virtqueue (number of descriptors in the vhost virtqueue)
 * @vqs: Pointers to all the created vhost virtqueues
 * @callback: Callback function associated with the virtqueue
 * @names: Names associated with each virtqueue
 *
 * Create vhost virtqueues for vhost device. This acts as a wrapper to
 * pci_epf_vhost_create_vq() which creates individual vhost virtqueue.
 */
static int pci_epf_vhost_create_vqs(struct vhost_dev *vdev, unsigned int nvqs,
				    unsigned int num_bufs,
				    struct vhost_virtqueue *vqs[],
				    vhost_vq_callback_t *callbacks[],
				    const char * const names[])
{
	struct epf_vhost *vhost;
	struct pci_epf *epf;
	struct device *dev;
	int ret, i;

	vhost = to_epf_vhost(vdev);
	epf = vhost->epf;
	dev = &epf->dev;

	for (i = 0; i < nvqs; i++) {
		vqs[i] = pci_epf_vhost_create_vq(vdev, i, num_bufs,
						 callbacks[i]);
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
	pci_epf_vhost_del_vqs(vdev);
	return ret;
}

/* pci_epf_vhost_set_features - vhost_config_ops to set vhost device features
 * @vdev: Vhost device that communicates with the remote virtio device
 * @features: Features supported by the vhost client driver
 *
 * vhost_config_ops invoked by the vhost client driver to set vhost device
 * features.
 */
static int pci_epf_vhost_set_features(struct vhost_dev *vdev, u64 features)
{
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;

	vhost = to_epf_vhost(vdev);
	reg = vhost->reg;

	reg->host_features = features;

	return 0;
}

/* ntb_vhost_set_status - vhost_config_ops to set vhost device status
 * @vdev: Vhost device that communicates with the remote virtio device
 * @status: Vhost device status configured by vhost client driver
 *
 * vhost_config_ops invoked by the vhost client driver to set vhost device
 * status.
 */
static int pci_epf_vhost_set_status(struct vhost_dev *vdev, u8 status)
{
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;

	vhost = to_epf_vhost(vdev);
	reg = vhost->reg;

	reg->device_status = status;

	return 0;
}

/* ntb_vhost_get_status - vhost_config_ops to get vhost device status
 * @vdev: Vhost device that communicates with the remote virtio device
 *
 * vhost_config_ops invoked by the vhost client driver to get vhost device
 * status set by the remote virtio driver.
 */
static u8 pci_epf_vhost_get_status(struct vhost_dev *vdev)
{
	struct epf_vhost_reg *reg;
	struct epf_vhost *vhost;

	vhost = to_epf_vhost(vdev);
	reg = vhost->reg;

	return reg->device_status;
}

static const struct vhost_config_ops pci_epf_vhost_ops = {
	.create_vqs	= pci_epf_vhost_create_vqs,
	.del_vqs	= pci_epf_vhost_del_vqs,
	.write		= pci_epf_vhost_write,
	.read		= pci_epf_vhost_read,
	.set_features	= pci_epf_vhost_set_features,
	.set_status	= pci_epf_vhost_set_status,
	.get_status	= pci_epf_vhost_get_status,
};

/* pci_epf_vhost_write_header - Write to PCIe standard configuration space
 *   header
 * @vhost: EPF vhost containing the vhost device that communicates with the
 *   remote virtio device
 *
 * Invokes endpoint core framework's pci_epc_write_header() to write to the
 * standard configuration space header.
 */
static int pci_epf_vhost_write_header(struct epf_vhost *vhost)
{
	struct pci_epf_header *header;
	struct vhost_dev *vdev;
	struct pci_epc *epc;
	struct pci_epf *epf;
	struct device *dev;
	u8 func_no;
	int ret;

	vdev = &vhost->vdev;
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;
	func_no = epf->func_no;
	header = epf->header;

	ret = pci_epc_write_header(epc, func_no, header);
	if (ret) {
		dev_err(dev, "Configuration header write failed\n");
		return ret;
	}

	return 0;
}

/* pci_epf_vhost_release_dev - Callback function to free device
 * @dev: Device in vhost_dev that has to be freed
 *
 * Callback function from device core invoked to free the device after
 * all references have been removed. This frees the allocated memory for
 * struct ntb_vhost.
 */
static void pci_epf_vhost_release_dev(struct device *dev)
{
	struct epf_vhost *vhost;
	struct vhost_dev *vdev;

	vdev = to_vhost_dev(dev);
	vhost = to_epf_vhost(vdev);

	kfree(vhost);
}

/* pci_epf_vhost_register - Register a vhost device
 * @vhost: EPF vhost containing the vhost device that communicates with the
 *   remote virtio device
 *
 * Invoked vhost_register_device() to register a vhost device after populating
 * the deviceID and vendorID of the vhost device.
 */
static int pci_epf_vhost_register(struct epf_vhost *vhost)
{
	struct vhost_dev *vdev;
	struct pci_epf *epf;
	struct device *dev;
	int ret;

	vdev = &vhost->vdev;
	epf = vhost->epf;
	dev = &epf->dev;

	vdev->dev.parent = dev;
	vdev->dev.release = pci_epf_vhost_release_dev;
	vdev->id.device = vhost->epf->header->subsys_id;
	vdev->id.vendor = vhost->epf->header->subsys_vendor_id;
#if defined(CONFIG_ARCH_SSTAR)
	/* we use vendor id to pass type and dev_id to upper layer */
	vdev->id.vendor = Rpmsg_Device_EP_PORT << 16 | epf->epc->portid;
#endif
	vdev->ops = &pci_epf_vhost_ops;

	ret = vhost_register_device(vdev);
	if (ret) {
		dev_err(dev, "Failed to register vhost device\n");
		return ret;
	}

	return 0;
}

static void pci_epf_vhost_bar_space_free(struct pci_epf *epf,
					 enum pci_barno bar)
{
	struct epf_vhost *vhost = epf_get_drvdata(epf);

	if (vhost->epf_bar[bar].addr) {
		memunmap(vhost->epf_bar[bar].addr);
	}
	vhost->epf_bar[bar].phys_addr = 0;
	vhost->epf_bar[bar].addr = NULL;
	vhost->epf_bar[bar].size = 0;
	vhost->epf_bar[bar].barno = 0;
	vhost->epf_bar[bar].flags = 0;

	epf->bar[bar].phys_addr = 0;
	epf->bar[bar].addr = NULL;
	epf->bar[bar].size = 0;
	epf->bar[bar].barno = 0;
	epf->bar[bar].flags = 0;
}

static void *pci_epf_vhost_memremap(dma_addr_t offset, size_t size, bool cache)
{
	dma_addr_t phys_addr = 0;
	void *addr = NULL;
	int ret;
	int i, j;
	struct sg_table *sg_table;
	struct scatterlist *psg;
	int page_count = 0;
	int page_total = 0;
	struct page **pages;
	pgprot_t prot;

	if (!size)
		return NULL;

	if (cache)
		prot = PAGE_KERNEL;
	else
		prot = pgprot_writecombine(PAGE_KERNEL);

	sg_table = kmalloc(sizeof(struct sg_table),
			   in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (!sg_table) {
		printk(KERN_ERR "%s: kmalloc return NULL!\n", __FUNCTION__);
		return NULL;
	}

	ret = sg_alloc_table(sg_table, 1,
			     in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);
	if (unlikely(ret)) {
		printk(KERN_ERR "%s: sg_alloc_table return %d!\n", __FUNCTION__,
		       ret);
		kfree(sg_table);
		return NULL;
	}

	phys_addr = offset;
	sg_set_page(sg_table->sgl, pfn_to_page(__phys_to_pfn(phys_addr)),
		    PAGE_ALIGN(size), 0);

	for_each_sg (sg_table->sgl, psg, sg_table->nents, i) {
		page_count += psg->length / PAGE_SIZE;
	}

	pages = vmalloc(sizeof(struct page *) * page_count);
	if (pages == NULL) {
		printk(KERN_ERR "%s: vmalloc return NULL!\n", __FUNCTION__);
		sg_free_table(sg_table);
		kfree(sg_table);
		return NULL;
	}

	for_each_sg (sg_table->sgl, psg, sg_table->nents, i) {
		page_count = PAGE_ALIGN(psg->length) / PAGE_SIZE;
		for (j = 0; j < page_count; j++)
			pages[page_total + j] = sg_page(psg) + j;
		page_total += page_count;
	}
	addr = vmap(pages, page_count, VM_MAP, prot);

	vfree(pages);
	sg_free_table(sg_table);
	kfree(sg_table);

	return addr;
}

static void *pci_epf_vhost_bar_space_setup(struct pci_epf *epf,
					   enum pci_barno bar)
{
	void *space;
	struct device *dev = epf->epc->dev.parent;
	struct epf_vhost *vhost = epf_get_drvdata(epf);
	dma_addr_t phys_addr;
	size_t size;

	if (epf->epc->portid == 0) {
		phys_addr = EPF_VHOST_REG_BASE0;
		space = pci_epf_vhost_memremap(EPF_VHOST_REG_BASE0,
					       EPF_VHOST_REG_SIZE0, false);
		size = EPF_VHOST_REG_SIZE0;
	} else if (epf->epc->portid == 1) {
		phys_addr = EPF_VHOST_REG_BASE1;
		space = pci_epf_vhost_memremap(EPF_VHOST_REG_BASE1,
					       EPF_VHOST_REG_SIZE1, false);
		size = EPF_VHOST_REG_SIZE1;
	} else {
	    dev_err(dev, "Unknown EPC portid 0x%x\n", epf->epc->portid);
	    return NULL;
	}

	if (!vhost->pcie_boot) {
		memset(space, 0, size);
	}

	/* keep the original physical addr, virtual addr, and size */
	vhost->epf_bar[bar].phys_addr = phys_addr;
	vhost->epf_bar[bar].addr = space;
	vhost->epf_bar[bar].size = size;
	vhost->epf_bar[bar].barno = bar;
	vhost->epf_bar[bar].flags = PCI_BASE_ADDRESS_SPACE_MEMORY;

	epf->bar[bar].phys_addr = ALIGN(phys_addr, size);
	space = space +
		(epf->bar[bar].phys_addr - vhost->epf_bar[bar].phys_addr);
	epf->bar[bar].addr = space;
	epf->bar[bar].size = size;
	epf->bar[bar].barno = bar;
	epf->bar[bar].flags = PCI_BASE_ADDRESS_SPACE_MEMORY;

	dev_err(dev,
		"-BAR[%d]:\r\n\tPA_base %llx, VA_base %px\r\n\tPA_align %llx, VA_align %px\r\n",
		bar, vhost->epf_bar[bar].phys_addr, vhost->epf_bar[bar].addr,
		epf->bar[bar].phys_addr, epf->bar[bar].addr);
	return space;
}

#if 0
/**
 * pci_epf_vhost_alloc_space() - allocate memory for the PCI EPF register space
 * @size: the size of the memory that has to be allocated
 * @bar: the BAR number corresponding to the allocated register space
 *
 * Invoke to allocate memory for the PCI EPF register space.
 */
static void *pci_epf_vhost_alloc_space(struct pci_epf *epf, size_t size, enum pci_barno bar,
				       size_t align)
{
        void *space;
        struct device *dev = epf->epc->dev.parent;
        struct epf_vhost *vhost = epf_get_drvdata(epf);
        dma_addr_t phys_addr;

        if (size < 128)
                size = 128;

        if (align)
                size = ALIGN(size, align);
        else
                size = roundup_pow_of_two(size);

        /* giving twice size to make sure the target addr of Inbound ATU is BAR size aligned */
        space = dma_alloc_coherent(dev, size << 1, &phys_addr, GFP_KERNEL);
        if (!space) {
                dev_err(dev, "failed to allocate mem space\n");
                return NULL;
        }
        memset(space, 0, size << 1);

        /* keep the original physical addr, virtual addr, and size */
        vhost->epf_bar[bar].phys_addr = phys_addr;
        vhost->epf_bar[bar].addr = space;
        vhost->epf_bar[bar].size = size << 1;
        vhost->epf_bar[bar].barno = bar;
        vhost->epf_bar[bar].flags = PCI_BASE_ADDRESS_SPACE_MEMORY;

        epf->bar[bar].phys_addr = ALIGN(phys_addr, size);
        space = space + (epf->bar[bar].phys_addr - vhost->epf_bar[bar].phys_addr);
        epf->bar[bar].addr = space;
        epf->bar[bar].size = size;
        epf->bar[bar].barno = bar;
        epf->bar[bar].flags = PCI_BASE_ADDRESS_SPACE_MEMORY;

        dev_err(dev, "-BAR[%d]:\r\n\tPA_base %llx, VA_base %px\r\n\tPA_align %llx, VA_align %px\r\n",
                bar, vhost->epf_bar[bar].phys_addr, vhost->epf_bar[bar].addr,
                epf->bar[bar].phys_addr, epf->bar[bar].addr);
        return space;
}

/**
 * pci_epf_free_space() - free the allocated PCI EPF register space
 * @addr: the virtual address of the PCI EPF register space
 * @bar: the BAR number corresponding to the register space
 *
 * Invoke to free the allocated PCI EPF register space.
 */
static void pci_epf_vhost_free_space(struct pci_epf *epf, void *addr, enum pci_barno bar)
{
        struct device *dev = epf->epc->dev.parent;
        struct epf_vhost *vhost = epf_get_drvdata(epf);

        if (!addr)
                return;

        dma_free_coherent(dev, vhost->epf_bar[bar].size, vhost->epf_bar[bar].addr,
                          vhost->epf_bar[bar].phys_addr);

        vhost->epf_bar[bar].phys_addr = 0;
        vhost->epf_bar[bar].addr = NULL;
        vhost->epf_bar[bar].size = 0;
        vhost->epf_bar[bar].barno = 0;
        vhost->epf_bar[bar].flags = 0;

        epf->bar[bar].phys_addr = 0;
        epf->bar[bar].addr = NULL;
        epf->bar[bar].size = 0;
        epf->bar[bar].barno = 0;
        epf->bar[bar].flags = 0;
}
#endif
/* pci_epf_vhost_configure_bar - Configure BAR of EPF device
 * @vhost: EPF vhost containing the vhost device that communicates with the
 *   remote virtio device
 *
 * Allocate memory for the standard virtio configuration space and map it to
 * the first free BAR.
 */
static int pci_epf_vhost_configure_bar(struct epf_vhost *vhost)
{
	size_t msix_table_size = 0, pba_size = 0, align, bar_size;
	const struct pci_epc_features *epc_features;
	struct pci_epf_bar *epf_bar;
	struct vhost_dev *vdev;
	struct pci_epf *epf;
	struct pci_epc *epc;
	struct device *dev;
	bool msix_capable;
	u32 config_size;
	int barno, ret;
	void *base, *shm;
	u64 size;
	u32 shm_bar_offset;
	u32 shm_bar_size;

	vdev = &vhost->vdev;
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;

	epc_features = vhost->epc_features;
	barno = BAR_0;
	if (barno < 0) {
		dev_err(dev, "Failed to get free BAR\n");
		return barno;
	}

	size = epc_features->bar_fixed_size[barno];
	align = epc_features->align;
	/* Check if epc_features is populated incorrectly */
	if (align && (!IS_ALIGNED(size, align))) {
		return -EINVAL;
	}

	config_size = sizeof(struct epf_vhost_reg) + VHOST_DEVICE_CONFIG_SIZE;
	config_size = ALIGN(config_size, 4096);
	shm_bar_offset = config_size;
	shm_bar_size = (32 + 256) * 1024;

	msix_capable = epc_features->msix_capable;
	if (msix_capable) {
		msix_table_size = PCI_MSIX_ENTRY_SIZE * epf->msix_interrupts;
		vhost->msix_table_offset = shm_bar_offset + shm_bar_size;
		vhost->msix_bar = barno;
		/* Align to QWORD or 8 Bytes */
		pba_size = ALIGN(DIV_ROUND_UP(epf->msix_interrupts, 8), 8);
	}

	bar_size = config_size + msix_table_size + pba_size;

	if (!align)
		bar_size = roundup_pow_of_two(bar_size);
	else
		bar_size = ALIGN(bar_size, align);

	if (!size)
		size = bar_size;
	else if (size < bar_size) {
		return -EINVAL;
	}

	base = pci_epf_vhost_bar_space_setup(epf, barno);
	if (!base) {
		dev_err(dev, "Failed to allocate configuration region\n");
		return -ENOMEM;
	}

	epf_bar = &epf->bar[barno];
	ret = pci_epc_set_bar(epc, epf->func_no, epf_bar);
	if (ret) {
		dev_err(dev, "Failed to set BAR: %d\n", barno);
		goto err_set_bar1;
	}

	shm = base + shm_bar_offset;
	vhost->shm = shm;
	vhost->shm_phys = epf_bar->phys_addr + shm_bar_offset;

	vhost->reg = base;
	vhost->reg->shm_bar_offset = shm_bar_offset;
	vhost->reg->shm_bar_size = shm_bar_size;
	dev_info(dev, "vhost->shm=%px,0x%llx\n", vhost->shm, vhost->shm_phys);

	return 0;

err_set_bar1:
	barno = BAR_0;
	pci_epf_vhost_bar_space_free(epf, barno);
	vhost->reg = NULL;

	return ret;
}

/* pci_epf_vhost_configure_interrupts - Configure MSI/MSI-X capability of EPF
 *   device
 * @vhost: EPF vhost containing the vhost device that communicates with the
 *   remote virtio device
 *
 * Configure MSI/MSI-X capability of EPF device. This will be used to interrupt
 * the vhost virtqueue.
 */
static int pci_epf_vhost_configure_interrupts(struct epf_vhost *vhost)
{
	const struct pci_epc_features *epc_features;
	struct pci_epf *epf;
	struct pci_epc *epc;
	struct device *dev;
	int ret;

	epc_features = vhost->epc_features;
	epf = vhost->epf;
	dev = &epf->dev;
	epc = epf->epc;

	if (epc_features->msi_capable) {
		ret = pci_epc_set_msi(epc, epf->func_no,
				      EPF_VHOST_MAX_INTERRUPTS);
		if (ret) {
			dev_err(dev, "MSI configuration failed\n");
			return ret;
		}
	}

	if (epc_features->msix_capable) {
		ret = pci_epc_set_msix(epc, epf->func_no,
				       EPF_VHOST_MAX_INTERRUPTS,
				       vhost->msix_bar,
				       vhost->msix_table_offset);
		if (ret) {
			dev_err(dev, "MSI-X configuration failed\n");
			return ret;
		}
	}

	return 0;
}

/* pci_epf_vhost_cfs_link - Link vhost client driver with EPF vhost to get
 *   the deviceID and driverID to be writtent to the PCIe config space
 * @epf_vhost_item: Config item representing the EPF vhost created by this
 *   driver
 * @epf: Endpoint function device that is bound to the endpoint controller
 *
 * This is invoked when the user creates a softlink between the vhost client
 * to the EPF vhost. This gets the deviceID and vendorID data from the vhost
 * client and copies it to the subys_id and subsys_vendor_id of the EPF
 * header. This will be used by the remote virtio to bind a virtio client
 * driver.
 */
static int pci_epf_vhost_cfs_link(struct config_item *epf_vhost_item,
				  struct config_item *driver_item)
{
	struct vhost_driver_item *vdriver_item;
	struct epf_vhost *vhost;

	vdriver_item = to_vhost_driver_item(driver_item);
	vhost = to_epf_vhost_from_ci(epf_vhost_item);

	vhost->epf->header->subsys_id = vdriver_item->device;
	vhost->epf->header->subsys_vendor_id = vdriver_item->vendor;

	return 0;
}

static struct configfs_item_operations pci_epf_vhost_cfs_ops = {
	.allow_link	= pci_epf_vhost_cfs_link,
};

static const struct config_item_type pci_epf_vhost_cfs_type = {
	.ct_item_ops	= &pci_epf_vhost_cfs_ops,
	.ct_owner	= THIS_MODULE,
};

/* pci_epf_vhost_cfs_work - Delayed work function to create configfs directory
 *   to perform EPF vhost specific initializations
 * @work: The work_struct holding the pci_epf_vhost_cfs_work() function that
 *   is scheduled
 *
 * This is a delayed work function to create configfs directory to perform EPF
 * vhost specific initializations. This configfs directory will be a
 * sub-directory to the directory created by the user to create pci_epf device.
 */
static void pci_epf_vhost_cfs_work(struct work_struct *work)
{
	struct epf_vhost *vhost = container_of(work, struct epf_vhost,
					       cfs_work.work);
	struct pci_epf *epf = vhost->epf;
	struct device *dev = &epf->dev;
	struct config_group *group;
	struct vhost_dev *vdev;
	int ret;

	if (!epf->group) {
		queue_delayed_work(kpcivhost_workqueue, &vhost->cfs_work,
				   msecs_to_jiffies(50));
		return;
	}

	vdev = &vhost->vdev;
	group = &vhost->group;
	config_group_init_type_name(group, dev_name(dev), &pci_epf_vhost_cfs_type);
	ret = configfs_register_group(epf->group, group);
	if (ret) {
		dev_err(dev, "Failed to register configfs group %s\n", dev_name(dev));
		return;
	}
}

/* pci_epf_vhost_probe - Initialize struct epf_vhost when a new EPF device is
 *   created
 * @epf: Endpoint function device that is bound to this driver
 *
 * Probe function to initialize struct epf_vhost when a new EPF device is
 * created.
 */
static int pci_epf_vhost_probe(struct pci_epf *epf)
{
	struct epf_vhost *vhost;
	struct device *dev;

	dev = &epf->dev;
	vhost = kzalloc(sizeof(*vhost), GFP_KERNEL);
	if (!vhost)
		return -ENOMEM;
	init_waitqueue_head(&vhost->wq);

	epf->header = &epf_vhost_header;
	vhost->epf = epf;

	epf_set_drvdata(epf, vhost);
	INIT_DELAYED_WORK(&vhost->cfs_work, pci_epf_vhost_cfs_work);
	queue_delayed_work(kpcivhost_workqueue, &vhost->cfs_work,
			   msecs_to_jiffies(50));
	return 0;
}

/* pci_epf_vhost_remove - Free the initializations performed by
 *   pci_epf_vhost_probe()
 * @epf: Endpoint function device that is bound to this driver
 *
 * Free the initializations performed by pci_epf_vhost_probe().
 */
static int pci_epf_vhost_remove(struct pci_epf *epf)
{
	struct epf_vhost *vhost;

	vhost = epf_get_drvdata(epf);
	cancel_delayed_work_sync(&vhost->cfs_work);

	return 0;
}

static irqreturn_t pci_epf_vhost_irq_handler(int irq, void *arg)
{
	struct pci_epf *epf = (struct pci_epf *)arg;
	struct epf_vhost *vhost = epf_get_drvdata(epf);
	int i;

	if (sstar_pcieif_memwr_hit_intr_sta(epf->epc->portid)) {
		//pr_debug("wr_addr_match irq\r\n");
		sstar_pcieif_clr_memwr_hit_intr_sta(epf->epc->portid);
		wake_up_all(&vhost->wq);
		for (i = 0; i < vhost->vdev.nvqs; i++)
			wake_up_all(&(vhost->vqueue[i].wq));
	}

	return IRQ_HANDLED;
}

/* pci_epf_vhost_bind - Bind callback to initialize the PCIe EP controller
 * @epf: Endpoint function device that is bound to the endpoint controller
 *
 * pci_epf_vhost_bind() is invoked when an endpoint controller is bound to
 * endpoint function. This function initializes the endpoint controller
 * with vhost endpoint function specific data.
 */
static int pci_epf_vhost_bind(struct pci_epf *epf)
{
	const struct pci_epc_features *epc_features;
	struct epf_vhost *vhost;
	struct pci_epc *epc;
	struct device *dev;
	struct device *epc_parent;
	struct device_node *np;
	struct dw_pcie_ep *ep;
	int ret;
	struct vhost_dev *vdev;

	vhost = epf_get_drvdata(epf);
	vdev = &vhost->vdev;
	dev = &epf->dev;
	epc = epf->epc;
	epc_parent = epc->dev.parent;

	np = epc_parent->of_node;
	vhost->pcie_boot = of_property_read_bool(np, "ep,dis_set_bar_quirk");
	dev_info(dev, "pcie_boot = %d\n", vhost->pcie_boot);

	epc_features = pci_epc_get_features(epc, epf->func_no);
	if (!epc_features) {
		dev_err(dev, "Fail to get EPC features\n");
		return -EINVAL;
	}
	vhost->epc_features = epc_features;

	ret = pci_epf_vhost_write_header(vhost);
	if (ret) {
		dev_err(dev, "Failed to bind VHOST config header\n");
		return ret;
	}

	ret = pci_epf_vhost_configure_bar(vhost);
	if (ret) {
		dev_err(dev, "Failed to configure BAR\n");
		return ret;
	}

	ret = pci_epf_vhost_configure_interrupts(vhost);
	if (ret) {
		dev_err(dev, "Failed to configure BAR\n");
		return ret;
	}

	ret = pci_epf_vhost_register(vhost);
	if (ret) {
		dev_err(dev, "Failed to bind VHOST config header\n");
		goto err1;
	}

	vhost->task = kthread_create(pci_epf_vhost_cmd_handler,
				     vhost, "vhost_handler");
	if (IS_ERR(vhost->task)) {
		ret =  PTR_ERR(vhost->task);
		vhost->task = NULL;
		dev_err(dev, "%s failed to create cmd handler for vhost, err = %d",
			__func__, ret);
		goto err2;
	}
	set_user_nice(vhost->task, MIN_NICE);
	wake_up_process(vhost->task);

	ep = (struct dw_pcie_ep *)epc_get_drvdata(epc);
	if (ep->irq > 0) {
		ss_miu_addr_t queue_cmd = vhost->epf->bar[BAR_0].phys_addr;

		/* trigger interrupt when RC write the command */
		queue_cmd += offsetof(struct epf_vhost_reg, trigger_irq);
		queue_cmd = Chip_Phys_to_MIU(queue_cmd);
		sstar_pcieif_enable_memwr_hit_intr(epc->portid, queue_cmd);
		dev_err(dev, "wr_match_addr 0x%llx, irq %d", queue_cmd, ep->irq);
		ret = devm_request_irq(&epc->dev, ep->irq, pci_epf_vhost_irq_handler,
				       IRQF_SHARED, "pci-epf-vhost", epf);
		if (ret) {
			dev_err(dev, "failed to request irq, ret = %d\n", ret);
			goto err3;
		}
	}

	if (vhost->pcie_boot) {
		blocking_notifier_call_chain(&vdev->notifier, NOTIFY_RESET, 0);

		vdev->features = vhost->reg->guest_features;
		blocking_notifier_call_chain(&vdev->notifier,
					     NOTIFY_FINALIZE_FEATURES, 0);

		blocking_notifier_call_chain(&vdev->notifier, NOTIFY_SET_STATUS,
					     NULL);
	}
	return 0;
err3:
	if (vhost->task) {
		kthread_stop(vhost->task);
		vhost->task = NULL;
	}
err2:
	if (device_is_registered(&vhost->vdev.dev))
		vhost_unregister_device(&vhost->vdev);
err1:
	if (vhost->reg) {
		struct pci_epf_bar *epf_bar;

		epf_bar = &epf->bar[BAR_0];
		pci_epc_clear_bar(epc, epf->func_no, epf_bar);
		pci_epf_vhost_bar_space_free(epf, BAR_0);
		vhost->reg = NULL;
		vhost->shm = NULL;
	}

	return ret;
}

/* pci_epf_vhost_unbind - Inbind callback to cleanup the PCIe EP controller
 * @epf: Endpoint function device that is bound to the endpoint controller
 *
 * pci_epf_vhost_unbind() is invoked when the binding between endpoint
 * controller is removed from endpoint function. This will unregister vhost
 * device and cancel pending cmd_handler work.
 */
static void pci_epf_vhost_unbind(struct pci_epf *epf)
{
	struct epf_vhost *vhost;
	struct vhost_dev *vdev;
	struct pci_epc *epc = epf->epc;
	struct pci_epf_bar *epf_bar;

	vhost = epf_get_drvdata(epf);
	vdev = &vhost->vdev;

	if (vhost->task) {
		kthread_stop(vhost->task);
		vhost->task = NULL;
	}

	if (device_is_registered(&vdev->dev))
		vhost_unregister_device(vdev);

	pci_epc_stop(epc);

	if (vhost->reg) {
		epf_bar = &epf->bar[BAR_0];
		pci_epc_clear_bar(epc, epf->func_no, epf_bar);
		pci_epf_vhost_bar_space_free(epf, BAR_0);
		vhost->reg = NULL;
		vhost->shm = NULL;
	}
}

static struct pci_epf_ops epf_ops = {
	.bind	= pci_epf_vhost_bind,
	.unbind	= pci_epf_vhost_unbind,
};

static const struct pci_epf_device_id pci_epf_vhost_ids[] = {
	{
		.name = "pci-epf-vhost0",
	},
	{
		.name = "pci-epf-vhost1",
	},
	{ },
};

static struct pci_epf_driver epf_vhost_driver = {
	.driver.name	= "pci_epf_vhost",
	.probe		= pci_epf_vhost_probe,
	.remove		= pci_epf_vhost_remove,
	.id_table	= pci_epf_vhost_ids,
	.ops		= &epf_ops,
	.owner		= THIS_MODULE,
};

static int __init pci_epf_vhost_init(void)
{
	int ret;

	kpcivhost_workqueue = alloc_workqueue("kpcivhost", WQ_MEM_RECLAIM |
					      WQ_HIGHPRI, 0);
	ret = pci_epf_register_driver(&epf_vhost_driver);
	if (ret) {
		pr_err("Failed to register pci epf vhost driver --> %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(pci_epf_vhost_init);

static void __exit pci_epf_vhost_exit(void)
{
	pci_epf_unregister_driver(&epf_vhost_driver);
}
module_exit(pci_epf_vhost_exit);

MODULE_DESCRIPTION("PCI EPF VHOST DRIVER");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
