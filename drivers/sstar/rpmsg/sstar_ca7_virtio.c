/*
 * sstar_ca7_virtio.c- Sigmastar
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
 * sstar_ca7_virtio.c
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
#include "cam_inter_os.h"
#include "irqs.h"

#include "drv_ca7.h"

#define TARGET_BITS_CORE0 (1 << 16)
#define TARGET_BITS_CORE1 (1 << 17)
#define NSATT_BITS_GROUP0 (0 << 15)
#define NSATT_BITS_GROUP1 (1 << 15)
#define SGIINTID_BITS_08  (8)
#define SGIINTID_BITS_09  (9)
#define SGIINTID_BITS_10  (10)
#define SGIINTID_BITS_11  (11)
#define SGIINTID_BITS_15  (15)

#define RTOS_BUF_SIZE 0x00800000

typedef struct
{
    unsigned int arg0_l;
    unsigned int arg0_h;
    unsigned int arg1_l;
    unsigned int arg1_h;
    unsigned int arg2_l;
    unsigned int arg2_h;
    unsigned int arg3_l;
    unsigned int arg3_h;
    unsigned int ret_l;
    unsigned int ret_h;
} interos_call_mbox_args_t;

struct sstar_virdev
{
    struct virtio_device  vdev;
    unsigned int          vring[2];
    struct virtqueue *    vq[2];
    int                   base_vq_id;
    int                   num_of_vqs;
    struct notifier_block nb;

    wait_queue_head_t   wq;
    struct task_struct *task;
};

struct sstar_ca7_virtio_vproc
{
    char *       rproc_name;
    struct mutex lock;
    int          vdev_nums;
#define MAX_VDEV_NUMS 1
    struct sstar_virdev ivdev[MAX_VDEV_NUMS];
};

struct sstar_ca7_virtio_mbox
{
    const char *                  name;
    struct blocking_notifier_head notifier;
};

static struct sstar_ca7_virtio_mbox rpmsg_mbox = {
    .name = "rtos",
};

static struct sstar_ca7_virtio_vproc sstar_ca7_virtio_vprocs[] = {
    {
        .rproc_name = "rtos",
    },
};

static u32 in_idx, out_idx;

/*
 * For now, allocate 256 buffers of 512 bytes for each side. each buffer
 * will then have 16B for the msg header and 496B for the payload.
 * This will require a total space of 256KB for the buffers themselves, and
 * 3 pages for every vring (the size of the vring depends on the number of
 * buffers it supports).
 */
#define RPMSG_NUM_BUFS   (512)
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

#define to_sstar_virdev(vd)    container_of(vd, struct sstar_virdev, vdev)
#define to_sstar_rpdev(vd, id) container_of(vd, struct sstar_ca7_virtio_vproc, ivdev[id])

struct sstar_ca7_virtio_vq_info
{
    __u16                          num;   /* number of entries in the virtio_ring */
    __u16                          vq_id; /* a globaly unique index of this virtqueue */
    void *                         addr;  /* address where we mapped the virtio ring */
    struct sstar_ca7_virtio_vproc *rpdev;
};

static unsigned long sstar_ca7_virtio_smc(u32 type)
{
    struct arm_smccc_res res;

    arm_smccc_smc(type, 0, 0, 0, 0, 0, 0, 0, &res);
    return res.a0;
}

void sstar_ca7_virtio_reroute_smc(void)
{
    sstar_ca7_virtio_smc(INTEROS_SC_L2R_RPMSG_NOTIFY);
}

irqreturn_t sstar_ca7_virtio_isr(int irq, void *param)
{
    struct sstar_ca7_virtio_vproc *vproc = &sstar_ca7_virtio_vprocs[0];

    ++in_idx;
    dsb(sy);
    wake_up_all(&vproc->ivdev[0].wq);
    return IRQ_HANDLED;
}

static u64 sstar_ca7_virtio_get_features(struct virtio_device *vdev)
{
    /* VIRTIO_RPMSG_F_NS has been made private */
    return 1 << 0;
}

static int sstar_ca7_virtio_finalize_features(struct virtio_device *vdev)
{
    /* Give virtio_ring a chance to accept features */
    vring_transport_features(vdev);
    return 0;
}

/* kick the remote processor, and let it know which virtqueue to poke at */
static bool sstar_ca7_virtio_notify(struct virtqueue *vq)
{
    // clear HST2 to HST7 bit
    CLRREG16(GET_REG_ADDR(BASE_REG_CPUINT_PA, 0x02), BIT7);
    // set HST2 to HST7 bit
    SETREG16(GET_REG_ADDR(BASE_REG_CPUINT_PA, 0x02), BIT7);
    // read a register to ensure that the previous write command was compeleted
    INREG16(BASE_REG_MAILBOX_PA);
    return true;
}

static int sstar_ca7_virtio_callback(struct notifier_block *this, unsigned long index, void *data)
{
    u32                  id = 0;
    struct sstar_virdev *virdev;

    virdev = container_of(this, struct sstar_virdev, nb);

    /*
     * We can't known which virtqueue triggers the interrupt,
     * so let's iterate all the virtqueues.
     */
    id = virdev->base_vq_id;
    for (; id < virdev->num_of_vqs; id++)
        vring_interrupt(id, virdev->vq[id]);

    return NOTIFY_DONE;
}

int sstar_ca7_virtio_register_nb(const char *name, struct notifier_block *nb)
{
    if ((name == NULL) || (nb == NULL))
        return -EINVAL;

    if (!strcmp(rpmsg_mbox.name, name))
        blocking_notifier_chain_register(&(rpmsg_mbox.notifier), nb);
    else
        return -ENOENT;

    return 0;
}

int sstar_ca7_virtio_unregister_nb(const char *name, struct notifier_block *nb)
{
    if ((name == NULL) || (nb == NULL))
        return -EINVAL;

    if (!strcmp(rpmsg_mbox.name, name))
        blocking_notifier_chain_unregister(&(rpmsg_mbox.notifier), nb);
    else
        return -ENOENT;

    return 0;
}

static struct virtqueue *rp_find_vq(struct virtio_device *vdev, unsigned int            index,
                                    void (*callback)(struct virtqueue *vq), const char *name)
{
    struct sstar_virdev *            virdev = to_sstar_virdev(vdev);
    struct sstar_ca7_virtio_vproc *  rpdev  = to_sstar_rpdev(virdev, virdev->base_vq_id / 2);
    struct sstar_ca7_virtio_vq_info *rpvq;
    struct virtqueue *               vq;
    int                              err;

    rpvq = kmalloc(sizeof(*rpvq), GFP_KERNEL);
    if (!rpvq)
        return ERR_PTR(-ENOMEM);

    /* ioremap'ing normal memory, so we cast away sparse's complaints */
    rpvq->addr = (__force void *)ioremap_wc(virdev->vring[index], RPMSG_RING_SIZE);
    if (!rpvq->addr)
    {
        err = -ENOMEM;
        goto free_rpvq;
    }

    memset(rpvq->addr, 0, RPMSG_RING_SIZE);

    pr_debug("vring%d: phys 0x%x, virt 0x%p\n", index, virdev->vring[index], rpvq->addr);

    vq = vring_new_virtqueue(index, RPMSG_NUM_BUFS / 2, RPMSG_VRING_ALIGN, vdev, true, false, rpvq->addr,
                             sstar_ca7_virtio_notify, callback, name);
    if (!vq)
    {
        pr_err("vring_new_virtqueue failed\n");
        err = -ENOMEM;
        goto unmap_vring;
    }

    virdev->vq[index] = vq;
    vq->priv          = rpvq;
    /* system-wide unique id for this virtqueue */
    rpvq->vq_id = virdev->base_vq_id + index;
    rpvq->rpdev = rpdev;
    mutex_init(&rpdev->lock);

    return vq;

unmap_vring:
    /* iounmap normal memory, so make sparse happy */
    iounmap((__force void __iomem *)rpvq->addr);
free_rpvq:
    kfree(rpvq);
    return ERR_PTR(err);
}

static void sstar_ca7_virtio_del_vqs(struct virtio_device *vdev)
{
    struct virtqueue *             vq, *n;
    struct sstar_virdev *          virdev = to_sstar_virdev(vdev);
    struct sstar_ca7_virtio_vproc *rpdev  = to_sstar_rpdev(virdev, virdev->base_vq_id / 2);

    list_for_each_entry_safe(vq, n, &vdev->vqs, list)
    {
        struct sstar_ca7_virtio_vq_info *rpvq = vq->priv;

        iounmap(rpvq->addr);
        vring_del_virtqueue(vq);
        kfree(rpvq);
    }

    if (&virdev->nb)
        sstar_ca7_virtio_unregister_nb((const char *)rpdev->rproc_name, &virdev->nb);
}

static int sstar_ca7_virtio_find_vqs(struct virtio_device *vdev, unsigned int nvqs, struct virtqueue *vqs[],
                                     vq_callback_t *callbacks[], const char *const names[], const bool *ctx,
                                     struct irq_affinity *desc)
{
    struct sstar_virdev *          virdev = to_sstar_virdev(vdev);
    struct sstar_ca7_virtio_vproc *rpdev  = to_sstar_rpdev(virdev, virdev->base_vq_id / 2);
    int                            i, err;

    /* we maintain two virtqueues per remote processor (for RX and TX) */
    if (nvqs != 2)
        return -EINVAL;

    for (i = 0; i < nvqs; ++i)
    {
        vqs[i] = rp_find_vq(vdev, i, callbacks[i], names[i]);
        if (IS_ERR(vqs[i]))
        {
            err = PTR_ERR(vqs[i]);
            goto error;
        }
    }

    virdev->num_of_vqs = nvqs;

    virdev->nb.notifier_call = sstar_ca7_virtio_callback;
    sstar_ca7_virtio_register_nb((const char *)rpdev->rproc_name, &virdev->nb);

    return 0;

error:
    sstar_ca7_virtio_del_vqs(vdev);
    return err;
}

static void sstar_ca7_virtio_reset(struct virtio_device *vdev)
{
    dev_dbg(&vdev->dev, "reset !\n");
}

static u8 sstar_ca7_virtio_get_status(struct virtio_device *vdev)
{
    return 0;
}

static void sstar_ca7_virtio_set_status(struct virtio_device *vdev, u8 status)
{
    dev_dbg(&vdev->dev, "%s new status: %d\n", __func__, status);
}

static void sstar_ca7_virtio_vproc_release(struct device *dev)
{
    /* this handler is provided so driver core doesn't yell at us */
}

static void *sstar_ca7_virtio_alloc_buffer(struct virtio_device *vdev, size_t size)
{
    void *bufs_va;

    bufs_va = ioremap_wc(CONFIG_CA7_MEM_PHY_ADDR + RTOS_BUF_SIZE, size);

    if (!bufs_va)
    {
        return NULL;
    }

    memset(bufs_va, 0, size);

    pr_info("buffers: va 0x%px, pa 0x%x, size=0x%zx\n", bufs_va, CONFIG_CA7_MEM_PHY_ADDR + RTOS_BUF_SIZE, size);
    return bufs_va;
}

static void sstar_ca7_virtio_free_buffer(struct virtio_device *vdev, void *addr, size_t size)
{
    return;
}

static struct virtio_config_ops sstar_ca7_virtio_config_ops = {
    .get_features      = sstar_ca7_virtio_get_features,
    .finalize_features = sstar_ca7_virtio_finalize_features,
    .find_vqs          = sstar_ca7_virtio_find_vqs,
    .del_vqs           = sstar_ca7_virtio_del_vqs,
    .reset             = sstar_ca7_virtio_reset,
    .set_status        = sstar_ca7_virtio_set_status,
    .get_status        = sstar_ca7_virtio_get_status,
    .alloc_buffer      = sstar_ca7_virtio_alloc_buffer,
    .free_buffer       = sstar_ca7_virtio_free_buffer,
};

static const struct of_device_id sstar_ca7_virtio_dt_ids[] = {{
                                                                  .compatible = "sstar,sstar-ca7-virtio",
                                                              },
                                                              {/* sentinel */}};
MODULE_DEVICE_TABLE(of, sstar_ca7_virtio_dt_ids);

#define SHARE_SIZE (0x6000)
static u32 sstar_ca7_virtio_share_area;
static int set_vring_phy_buf(struct platform_device *pdev, struct sstar_ca7_virtio_vproc *rpdev, int vdev_nums)
{
    struct resource *res;
    resource_size_t  size;
    unsigned int     start, end;
    int              i, ret = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res && sstar_ca7_virtio_share_area != 0x0)
    {
        res = request_mem_region(sstar_ca7_virtio_share_area, SHARE_SIZE, "rpmsg");
    }

    if (res)
    {
        size  = resource_size(res);
        start = res->start;
        end   = res->start + size;
        for (i = 0; i < vdev_nums; i++)
        {
            rpdev->ivdev[i].vring[0] = start;
            rpdev->ivdev[i].vring[1] = start + RPMSG_RING_SIZE;
            start += RPMSG_RING_SIZE * 2;
            if (start > end)
            {
                pr_err("Too small memory size %x!\n", (u32)size);
                ret = -EINVAL;
                break;
            }
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
    u32                            idx;
    struct sstar_ca7_virtio_vproc *vproc = (struct sstar_ca7_virtio_vproc *)arg;

    DEFINE_WAIT(wait);
    while (1)
    {
        if (kthread_should_stop())
            break;

        prepare_to_wait(&vproc->ivdev[0].wq, &wait, TASK_UNINTERRUPTIBLE);
        idx = in_idx;
        if (idx == out_idx)
            schedule();
        finish_wait(&vproc->ivdev[0].wq, &wait);

        while (out_idx != idx)
        {
            blocking_notifier_call_chain(&(rpmsg_mbox.notifier), 0, NULL);
            out_idx++;
        }
    }
    return 0;
}

static int sstar_ca7_virtio_probe(struct platform_device *pdev)
{
    int                 i, j, ret = 0;
    struct device_node *np = pdev->dev.of_node;

    BLOCKING_INIT_NOTIFIER_HEAD(&(rpmsg_mbox.notifier));

    pr_info("RPMSG is ready for cross core communication!\n");

    for (i = 0; i < ARRAY_SIZE(sstar_ca7_virtio_vprocs); i++)
    {
        struct sstar_ca7_virtio_vproc *rpdev = &sstar_ca7_virtio_vprocs[i];
        struct device_node *           intr_node;
        struct irq_domain *            intr_domain;
        struct irq_fwspec              fwspec;
        unsigned int                   virq_num;

        intr_node   = of_find_compatible_node(NULL, NULL, "sstar,main-intc");
        intr_domain = irq_find_host(intr_node);
        if (!intr_domain)
            return -ENXIO;

        fwspec.param_count = 3;
        fwspec.param[0]    = 0; // GIC_SPI
        fwspec.param[1]    = INT_FIQ_HST_1_3;
        fwspec.param[2]    = IRQ_TYPE_EDGE_RISING;
        fwspec.fwnode      = of_node_to_fwnode(intr_node);
        virq_num           = irq_create_fwspec_mapping(&fwspec);
        ret = request_irq(virq_num, sstar_ca7_virtio_isr, IRQF_TRIGGER_RISING | IRQF_SHARED, "rtos-rpmsg", rpdev);
        if (ret)
        {
            pr_err("%s: register interrupt %d failed\n", __FUNCTION__, INT_FIQ_HST_1_3);
            return ret;
        }

        ret = of_property_read_u32_index(np, "vdev-nums", i, &rpdev->vdev_nums);
        if (ret)
            rpdev->vdev_nums = 1;
        if (rpdev->vdev_nums > MAX_VDEV_NUMS)
        {
            pr_err("vdev-nums exceed the max %d\n", MAX_VDEV_NUMS);
            return -EINVAL;
        }

        if (!strcmp(rpdev->rproc_name, "rtos"))
        {
            ret = set_vring_phy_buf(pdev, rpdev, rpdev->vdev_nums);
            if (ret)
            {
                pr_err("No vring buffer.\n");
                return -ENOMEM;
            }
        }
        else
        {
            pr_err("No remote rtos processor.\n");
            return -ENODEV;
        }

        for (j = 0; j < rpdev->vdev_nums; j++)
        {
            pr_debug("%s rpdev%d vdev%d: vring0 0x%x, vring1 0x%x\n", __func__, i, rpdev->vdev_nums,
                     rpdev->ivdev[j].vring[0], rpdev->ivdev[j].vring[1]);
            rpdev->ivdev[j].vdev.id.device   = VIRTIO_ID_RPMSG;
            rpdev->ivdev[j].vdev.id.vendor   = (RPMsg_Device_CA7 << 16) | 0x0;
            rpdev->ivdev[j].vdev.config      = &sstar_ca7_virtio_config_ops;
            rpdev->ivdev[j].vdev.dev.parent  = &pdev->dev;
            rpdev->ivdev[j].vdev.dev.release = sstar_ca7_virtio_vproc_release;
            rpdev->ivdev[j].base_vq_id       = j * 2;
            init_waitqueue_head(&rpdev->ivdev[j].wq);
            rpdev->ivdev[j].task = kthread_create(rpmsg_irq_handler, rpdev, "rpmsg_irq/%d", j);
            if (IS_ERR(rpdev->ivdev[j].task))
            {
                pr_err("%s failed to create irq worker for vdev %d:%d, err %ld", __func__, i, j,
                       PTR_ERR(rpdev->ivdev[j].task));
                return PTR_ERR(rpdev->ivdev[j].task);
            }
            set_user_nice(rpdev->ivdev[j].task, MIN_NICE);
            wake_up_process(rpdev->ivdev[j].task);

            ret = register_virtio_device(&rpdev->ivdev[j].vdev);
            if (ret)
            {
                kthread_stop(rpdev->ivdev[j].task);
                pr_err("%s failed to register rpdev: %d\n", __func__, ret);
                return ret;
            }
        }
    }
    return ret;
}

static struct platform_driver sstar_ca7_virtio_driver = {
    .driver =
        {
            .owner          = THIS_MODULE,
            .name           = "sstar-ca7-virtio",
            .of_match_table = sstar_ca7_virtio_dt_ids,
        },
    .probe = sstar_ca7_virtio_probe,
};

static int sstar_ca7_virtio_shm_init(void)
{
    interos_call_mbox_args_t *ptr_mbox_args;
    u32                       marker;
    int                       count = 100;

    ptr_mbox_args         = (interos_call_mbox_args_t *)(BASE_REG_MAILBOX_PA + BK_REG(0x60) + IO_OFFSET);
    ptr_mbox_args->arg0_l = 0;
    ptr_mbox_args->arg0_h = 0;
    ptr_mbox_args->ret_l  = 0;
    ptr_mbox_args->ret_h  = 0;

    // set HST2 to HST7 bit
    SETREG16(GET_REG_ADDR(BASE_REG_CPUINT_PA, 0x02), BIT7);

    while (count > 0)
    {
        marker = (ptr_mbox_args->arg0_h << 16) + ptr_mbox_args->arg0_l;
        if (marker == 0xf1f1f1f1)
            break;
        udelay(1000);
        --count;
    }

    if (marker == 0xf3f2f1f0)
    {
        sstar_ca7_virtio_share_area =
            (u32)((ptr_mbox_args->ret_h << 16) + ptr_mbox_args->ret_l) + (CONFIG_CA7_MEM_PHY_ADDR - MIU0_LOW_BASE);
    }
    else
    {
        pr_err("Failed to get address of rpmsg share area\n");
        return 0;
    }
    printk(KERN_INFO "address of rpmsg share area:0x%x,0x%x,%lu\n", sstar_ca7_virtio_share_area,
           (u32)((ptr_mbox_args->ret_h << 16) + ptr_mbox_args->ret_l), RPMSG_RING_SIZE);
    return 0;
}

static int __init sstar_ca7_virtio_init(void)
{
    int ret;

    ret = ca7_start();
    if (ret < 0)
        pr_err("Start CA7 fail");

    sstar_ca7_virtio_shm_init();

    ret = platform_driver_register(&sstar_ca7_virtio_driver);
    if (ret)
        pr_err("Unable to initialize rpmsg driver\n");
    else
        pr_info("sstar rpmsg driver is registered.\n");

    return ret;
}

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SSTAR I7 ca7 virtio device driver");
MODULE_LICENSE("GPL");
module_init(sstar_ca7_virtio_init);
