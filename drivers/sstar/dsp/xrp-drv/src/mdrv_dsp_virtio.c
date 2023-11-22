/*
 * mdrv_dsp_virtio.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
/*
 * mdrv_dsp_virtio.c
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_ring.h>
#include <linux/arm-smccc.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>

#include "ms_platform.h"
#include "mstar_chip.h"
#include "registers.h"
#include "drv_dualos.h"
#include "irqs.h"

#include "mdrv_dsp.h"

#define DSP_SHARE_SIZE      (0x6000)
#define DSP_HOST_TO_DSP_IRQ 1
#define DSP_DSP_TO_HOST_IRQ 7

struct MdrvDSPVirtioDev
{
    struct virtio_device  vdev;
    u64                   vring[2];
    struct virtqueue *    vq[2];
    int                   num_of_vqs;
    struct notifier_block nb;

    wait_queue_head_t   wq;
    struct task_struct *task;
};

struct MdrvDSPVirtioDevProc
{
    MDRV_DSP_CoreID_e       eCoreID;
    struct mutex            lock;
    struct MdrvDSPVirtioDev ivdev;
};

/*
 * For now, allocate 256 buffers of 512 bytes for each side. each buffer
 * will then have 16B for the msg header and 496B for the payload.
 * This will require a total space of 256KB for the buffers themselves, and
 * 3 pages for every vring (the size of the vring depends on the number of
 * buffers it supports).
 */
#define RPMSG_NUM_BUFS   (8)
#define RPMSG_BUF_SIZE   (512)
#define RPMSG_BUFS_SPACE (RPMSG_NUM_BUFS * RPMSG_BUF_SIZE)

/*
 * The alignment between the consumer and producer parts of the vring.
 * Note: this is part of the "wire" protocol. If you change this, you need
 * to update your BIOS image as well
 */
#define RPMSG_VRING_ALIGN (4096)

/* With 256 buffers, our vring will occupy 3 pages */
#define RPMSG_RING_SIZE ((DIV_ROUND_UP(vring_size(RPMSG_NUM_BUFS / 2, RPMSG_VRING_ALIGN), PAGE_SIZE)) * PAGE_SIZE)

#define toMdrvDSPVirtioDev(vd)     container_of(vd, struct MdrvDSPVirtioDev, vdev)
#define toMdrvDSPVirtioDevProc(vd) container_of(vd, struct MdrvDSPVirtioDevProc, ivdev)

struct MdrvDSPVirtio_vq_info
{
    __u16                        num;   /* number of entries in the virtio_ring */
    __u16                        vq_id; /* a globaly unique index of this virtqueue */
    void *                       addr;  /* address where we mapped the virtio ring */
    struct MdrvDSPVirtioDevProc *rproc;
};

struct MdrvDSPVirtioContext
{
    u64   shmPhys;
    void *shmVirt;
    u64   shmDSP;

    unsigned int virq_num;
    u32          in_idx, out_idx;

    struct blocking_notifier_head notifier;
    struct MdrvDSPVirtioDevProc   rproc;

    int initialized;
};

struct MdrvDSPVirtioContext g_DSPVirtioContext[E_MDRV_DSP_CORE_NUM];

#if 0
static u64   gMdrvDSPVirtioShm;
static void *gMdrvDSPVirtioShmVirt;
static u64   gMdrvDSPVirtioShmBase;
#endif

irqreturn_t MdrvDSPVirtioISR(int irq, void *param)
{
    struct MdrvDSPVirtioDevProc *rproc = (struct MdrvDSPVirtioDevProc *)param;
    struct MdrvDSPVirtioContext *context;
    MDRV_DSP_IRQStat_t           stIRQStat = {0};

    context = &g_DSPVirtioContext[rproc->eCoreID];
    Mdrv_DSP_GetIRQStat(E_MDRV_DSP_CORE0, &stIRQStat, E_MDRV_DSP_IRQ_STAT_BEFOREMSK | E_MDRV_DSP_IRQ_STAT_FORCEVALID,
                        DSP_DSP_TO_HOST_IRQ);
    if (stIRQStat.bStatBeforeMsk || stIRQStat.bForceValid)
    {
        context->in_idx++;
        dsb(sy);
        wake_up_all(&rproc->ivdev.wq);

        stIRQStat.bClearIrq   = TRUE;
        stIRQStat.bForceValid = FALSE;
        Mdrv_DSP_SetIRQStat(E_MDRV_DSP_CORE0, &stIRQStat, E_MDRV_DSP_IRQ_STAT_CLEAR | E_MDRV_DSP_IRQ_STAT_FORCEVALID,
                            DSP_DSP_TO_HOST_IRQ);
    }

    return IRQ_HANDLED;
}

static u64 MdrvDSPVirtioGetFeatures(struct virtio_device *vdev)
{
    /* VIRTIO_RPMSG_F_NS has been made private */
    return 1 << 0;
}

static int MdrvDSPVirtioFinalizeFeatures(struct virtio_device *vdev)
{
    /* Give virtio_ring a chance to accept features */
    vring_transport_features(vdev);
    return 0;
}

/* kick the remote processor, and let it know which virtqueue to poke at */
static bool MdrvDSPVirtio_notify(struct virtqueue *vq)
{
    Mdrv_DSP_InterruptTrigger(E_MDRV_DSP_CORE0, DSP_HOST_TO_DSP_IRQ);
    return true;
}

static int MdrvDSPVirtioCallback(struct notifier_block *this, unsigned long index, void *data)
{
    u32                      id = 0;
    struct MdrvDSPVirtioDev *virdev;

    virdev = container_of(this, struct MdrvDSPVirtioDev, nb);

    /*
     * We can't known which virtqueue triggers the interrupt,
     * so let's iterate all the virtqueues.
     */
    for (; id < virdev->num_of_vqs; id++)
        vring_interrupt(id, virdev->vq[id]);

    return NOTIFY_DONE;
}

int MdrvDSPVirtioRegisterNB(struct MdrvDSPVirtioDevProc *rproc, struct notifier_block *nb)
{
    struct MdrvDSPVirtioContext *context;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    blocking_notifier_chain_register(&context->notifier, nb);
    return 0;
}

int MdrvDSPVirtioUnregisterNB(struct MdrvDSPVirtioDevProc *rproc, struct notifier_block *nb)
{
    struct MdrvDSPVirtioContext *context;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    blocking_notifier_chain_unregister(&context->notifier, nb);
    return 0;
}

static struct virtqueue *MdrvDSPVirtioFindVq(struct virtio_device *vdev, unsigned int            index,
                                             void (*callback)(struct virtqueue *vq), const char *name)
{
    struct MdrvDSPVirtioDev *     virdev = toMdrvDSPVirtioDev(vdev);
    struct MdrvDSPVirtioDevProc * rproc  = toMdrvDSPVirtioDevProc(virdev);
    struct MdrvDSPVirtio_vq_info *rpvq;
    struct virtqueue *            vq;
    int                           err;
    struct MdrvDSPVirtioContext * context;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    rpvq    = kmalloc(sizeof(*rpvq), GFP_KERNEL);
    if (!rpvq)
        return ERR_PTR(-ENOMEM);

    rpvq->addr = context->shmVirt + (virdev->vring[index] - context->shmPhys);
    pr_debug("find_vq:%px, %llx, %llx, %llx\n", context->shmVirt, virdev->vring[index],
             (virdev->vring[index] - context->shmPhys), context->shmPhys);
    if (!rpvq->addr)
    {
        err = -ENOMEM;
        goto free_rpvq;
    }

    memset(rpvq->addr, 0, RPMSG_RING_SIZE);

    pr_debug("vring%d: phys 0x%llx, virt 0x%p\n", index, virdev->vring[index], rpvq->addr);

    vq = vring_new_virtqueue(index, RPMSG_NUM_BUFS / 2, RPMSG_VRING_ALIGN, vdev, true, false, rpvq->addr,
                             MdrvDSPVirtio_notify, callback, name);
    if (!vq)
    {
        pr_err("vring_new_virtqueue failed\n");
        err = -ENOMEM;
        goto unmap_vring;
    }

    virdev->vq[index] = vq;
    vq->priv          = rpvq;
    /* system-wide unique id for this virtqueue */
    rpvq->vq_id = index;
    rpvq->rproc = rproc;
    mutex_init(&rproc->lock);

    return vq;

unmap_vring:
    /* iounmap normal memory, so make sparse happy */
    iounmap((__force void __iomem *)rpvq->addr);
free_rpvq:
    kfree(rpvq);
    return ERR_PTR(err);
}

static void MdrvDSPVirtioDelVqs(struct virtio_device *vdev)
{
    struct virtqueue *           vq, *n;
    struct MdrvDSPVirtioDev *    virdev = toMdrvDSPVirtioDev(vdev);
    struct MdrvDSPVirtioDevProc *rproc  = toMdrvDSPVirtioDevProc(virdev);

    list_for_each_entry_safe(vq, n, &vdev->vqs, list)
    {
        struct MdrvDSPVirtio_vq_info *rpvq = vq->priv;

        vring_del_virtqueue(vq);
        kfree(rpvq);
    }

    MdrvDSPVirtioUnregisterNB(rproc, &virdev->nb);
}

static int MdrvDSPVirtioFindVqs(struct virtio_device *vdev, unsigned int nvqs, struct virtqueue *vqs[],
                                vq_callback_t *callbacks[], const char *const names[], const bool *ctx,
                                struct irq_affinity *desc)
{
    struct MdrvDSPVirtioDev *    virdev = toMdrvDSPVirtioDev(vdev);
    struct MdrvDSPVirtioDevProc *rproc  = toMdrvDSPVirtioDevProc(virdev);
    int                          i, err;

    /* we maintain two virtqueues per remote processor (for RX and TX) */
    if (nvqs != 2)
        return -EINVAL;

    for (i = 0; i < nvqs; ++i)
    {
        vqs[i] = MdrvDSPVirtioFindVq(vdev, i, callbacks[i], names[i]);
        if (IS_ERR(vqs[i]))
        {
            err = PTR_ERR(vqs[i]);
            goto error;
        }
    }

    virdev->num_of_vqs = nvqs;

    virdev->nb.notifier_call = MdrvDSPVirtioCallback;
    MdrvDSPVirtioRegisterNB(rproc, &virdev->nb);

    return 0;

error:
    MdrvDSPVirtioDelVqs(vdev);
    return err;
}

static void MdrvDSPVirtioReset(struct virtio_device *vdev)
{
    dev_dbg(&vdev->dev, "reset !\n");
}

static u8 MdrvDSPVirtioGetStatus(struct virtio_device *vdev)
{
    return 0;
}

static void MdrvDSPVirtioSetStatus(struct virtio_device *vdev, u8 status)
{
    dev_dbg(&vdev->dev, "%s new status: %d\n", __func__, status);
}

static void MdrvDSPVirtioDevProcRelease(struct device *dev)
{
    /* this handler is provided so driver core doesn't yell at us */
}

static void *MdrvDSPVirtioAllocBuffer(struct virtio_device *vdev, size_t size)
{
    struct MdrvDSPVirtioDev *    virdev = toMdrvDSPVirtioDev(vdev);
    struct MdrvDSPVirtioDevProc *rproc  = toMdrvDSPVirtioDevProc(virdev);
    struct MdrvDSPVirtioContext *context;
    void *                       bufs_va;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    bufs_va = context->shmVirt + RPMSG_RING_SIZE * 2;

    if (!bufs_va)
    {
        return NULL;
    }

    memset(bufs_va, 0, size);
    return bufs_va;
}

static void MdrvDSPVirtioFreeBuffer(struct virtio_device *vdev, void *addr, size_t size)
{
    return;
}

static dma_addr_t MdrvDSPVirtioMapVirt(struct virtio_device *vdev, void *data)
{
    struct MdrvDSPVirtioDev *    virdev = toMdrvDSPVirtioDev(vdev);
    struct MdrvDSPVirtioDevProc *rproc  = toMdrvDSPVirtioDevProc(virdev);
    struct MdrvDSPVirtioContext *context;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    return (data - context->shmVirt) + context->shmDSP;
}

static struct virtio_config_ops MdrvDSPVirtioConfigOps = {
    .get_features      = MdrvDSPVirtioGetFeatures,
    .finalize_features = MdrvDSPVirtioFinalizeFeatures,
    .find_vqs          = MdrvDSPVirtioFindVqs,
    .del_vqs           = MdrvDSPVirtioDelVqs,
    .reset             = MdrvDSPVirtioReset,
    .set_status        = MdrvDSPVirtioSetStatus,
    .get_status        = MdrvDSPVirtioGetStatus,
    .alloc_buffer      = MdrvDSPVirtioAllocBuffer,
    .free_buffer       = MdrvDSPVirtioFreeBuffer,
    .map_virt          = MdrvDSPVirtioMapVirt,
};

static void FreeVringPhyBuf(struct MdrvDSPVirtioDevProc *rproc)
{
    struct MdrvDSPVirtioContext *context;

    context = &g_DSPVirtioContext[rproc->eCoreID];
    if (context->shmPhys != 0x0)
    {
        release_mem_region(context->shmPhys, DSP_SHARE_SIZE);
    }
}

static int SetVringPhyBuf(struct MdrvDSPVirtioDevProc *rproc)
{
    struct resource *            res;
    resource_size_t              size;
    unsigned long                start, end;
    int                          ret = 0;
    struct MdrvDSPVirtioContext *context;
    char                         buf[64];

    context = &g_DSPVirtioContext[rproc->eCoreID];
    res     = NULL;
    if (context->shmPhys != 0x0)
    {
        snprintf(buf, sizeof(buf), "dsp%d_rpmsg", rproc->eCoreID);
        res = request_mem_region(context->shmPhys, DSP_SHARE_SIZE, buf);
    }

    if (res)
    {
        size  = resource_size(res);
        start = res->start;
        end   = res->start + size;

        rproc->ivdev.vring[0] = start;
        rproc->ivdev.vring[1] = start + RPMSG_RING_SIZE;
        start += RPMSG_RING_SIZE * 2;

        pr_debug("setVringPhy:%llx,%llx,%lx\n", rproc->ivdev.vring[0], rproc->ivdev.vring[1], RPMSG_RING_SIZE);
        if (start > end)
        {
            release_mem_region(context->shmPhys, DSP_SHARE_SIZE);
            pr_err("Too small memory size %x!\n", (u32)size);
            return -EINVAL;
        }
    }
    else
    {
        return -ENOMEM;
    }

    return ret;
}

static int rpmsg_irq_handler(void *arg)
{
    u32                          idx;
    struct MdrvDSPVirtioDevProc *rproc = (struct MdrvDSPVirtioDevProc *)arg;
    struct MdrvDSPVirtioContext *context;
    DEFINE_WAIT(wait);

    context = &g_DSPVirtioContext[rproc->eCoreID];
    while (1)
    {
        if (kthread_should_stop())
            break;

        prepare_to_wait(&rproc->ivdev.wq, &wait, TASK_UNINTERRUPTIBLE);
        idx = context->in_idx;
        if (idx == context->out_idx)
            schedule();
        finish_wait(&rproc->ivdev.wq, &wait);

        while (context->out_idx != idx)
        {
            blocking_notifier_call_chain(&context->notifier, 0, NULL);
            context->out_idx++;
        }
    }
    return 0;
}

static int MdrvDSPVirtioRegister(MDRV_DSP_CoreID_e eCoreID)
{
    struct MdrvDSPVirtioContext *context;
    int                          ret = 0;
    struct MdrvDSPVirtioDevProc *rproc;
    char                         buf[64];

    context = &g_DSPVirtioContext[eCoreID];

    BLOCKING_INIT_NOTIFIER_HEAD(&context->notifier);
    rproc = &context->rproc;

    pr_info("RPMSG is ready for cross core communication!\n");

    Mdrv_DSP_GetDspIrqNumById(eCoreID, &context->virq_num);
    snprintf(buf, sizeof(buf), "dsp%d-rpmsg", eCoreID);
    ret = request_irq(context->virq_num, MdrvDSPVirtioISR, IRQF_ONESHOT | IRQF_SHARED, buf, rproc);
    if (ret)
    {
        pr_err("%s: register interrupt %d failed\n", __FUNCTION__, INT_FIQ_HST_1_3);
        return ret;
    }

    ret = SetVringPhyBuf(rproc);
    if (ret)
    {
        free_irq(context->virq_num, rproc);
        pr_err("No vring buffer.\n");
        return -ENOMEM;
    }

    pr_debug("%s rproc vdev: vring0 0x%llx, vring1 0x%llx\n", __func__, rproc->ivdev.vring[0], rproc->ivdev.vring[1]);
    rproc->ivdev.vdev.id.device   = VIRTIO_ID_RPMSG;
    rproc->ivdev.vdev.id.vendor   = (RPMsg_Device_DSP << 16) | eCoreID;
    rproc->ivdev.vdev.config      = &MdrvDSPVirtioConfigOps;
    rproc->ivdev.vdev.dev.parent  = NULL;
    rproc->ivdev.vdev.dev.release = MdrvDSPVirtioDevProcRelease;

    init_waitqueue_head(&rproc->ivdev.wq);

    rproc->ivdev.task = kthread_create(rpmsg_irq_handler, rproc, "rpmsg_dsp/%d", eCoreID);
    if (IS_ERR(rproc->ivdev.task))
    {
        FreeVringPhyBuf(rproc);
        free_irq(context->virq_num, rproc);
        pr_err("%s failed to create irq worker, err %ld", __func__, PTR_ERR(rproc->ivdev.task));
        return PTR_ERR(rproc->ivdev.task);
    }
    set_user_nice(rproc->ivdev.task, MIN_NICE);
    wake_up_process(rproc->ivdev.task);

    ret = register_virtio_device(&rproc->ivdev.vdev);
    if (ret)
    {
        kthread_stop(rproc->ivdev.task);
        FreeVringPhyBuf(rproc);
        free_irq(context->virq_num, rproc);
        pr_err("%s failed to register rproc: %d\n", __func__, ret);
        return ret;
    }

    return 0;
}

static int MdrvDSPVirtioUnregister(MDRV_DSP_CoreID_e eCoreID)
{
    struct MdrvDSPVirtioContext *context;
    struct MdrvDSPVirtioDevProc *rproc;

    context = &g_DSPVirtioContext[eCoreID];
    rproc   = &context->rproc;

    unregister_virtio_device(&rproc->ivdev.vdev);
    kthread_stop(rproc->ivdev.task);
    FreeVringPhyBuf(rproc);
    free_irq(context->virq_num, rproc);
    return 0;
}

int MdrvDSPVirtioInit(MDRV_DSP_CoreID_e eCoreID, u64 shmPhys, void *shmVirt, u64 shmDSP)
{
    struct MdrvDSPVirtioContext *context;
    int                          ret;

    if (eCoreID >= E_MDRV_DSP_CORE_NUM)
        return -EINVAL;

    context = &g_DSPVirtioContext[eCoreID];
    if (context->initialized)
        return -EBUSY;

    context->shmPhys = shmPhys;
    context->shmVirt = shmVirt;
    context->shmDSP  = shmDSP;

    ret = MdrvDSPVirtioRegister(eCoreID);
    if (ret)
        return ret;

    context->initialized = 1;
    pr_info("MdrvDSPVirtioInit:eCoreID=%d,shmPhys=0x%llx,shmVirt=0x%px,shmDSP=0x%llx\n", eCoreID, shmPhys, shmVirt,
            shmDSP);
    return 0;
}

int MdrvDSPVirtioDeinit(MDRV_DSP_CoreID_e eCoreID)
{
    struct MdrvDSPVirtioContext *context;
    int                          ret;

    if (eCoreID >= E_MDRV_DSP_CORE_NUM)
        return -EINVAL;

    context = &g_DSPVirtioContext[eCoreID];
    if (!context->initialized)
        return 0;

    ret = MdrvDSPVirtioUnregister(eCoreID);
    if (ret)
        return ret;

    memset(context, 0, sizeof(*context));
    return 0;
}

EXPORT_SYMBOL(MdrvDSPVirtioInit);
EXPORT_SYMBOL(MdrvDSPVirtioDeinit);
