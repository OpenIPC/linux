/*
 * rpmsg_dualos.c- Sigmastar
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

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/rpmsg.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/refcount.h>

#include "sstar_rpmsg.h"
#include "sstar_rpmsg_bootup.h"
#include "rpmsg_dualos.h"

#include "rpmsg_internal.h"

#define rpmsg_dualos_info(fmt, a...) printk(KERN_INFO "rpmsg_dualos %s:%d: " fmt, __FILE__, __LINE__, ##a)

#define rpmsg_dualos_err(fmt, a...) printk(KERN_ERR "rpmsg_dualos %s:%d: " fmt, __FILE__, __LINE__, ##a)

#define RPMSG_DEV_MAX (MINORMASK + 1)

static dev_t         rpmsg_major;
static struct class *rpmsg_class;

static DEFINE_IDA(rpmsg_ctrl_ida);
static DEFINE_IDA(rpmsg_ept_ida);
static DEFINE_IDA(rpmsg_minor_ida);

#define dev_to_eptdev(dev)     container_of(dev, struct rpmsg_eptdev, dev)
#define cdev_to_eptdev(i_cdev) container_of(i_cdev, struct rpmsg_eptdev, cdev)

#define dev_to_ctrldev(dev)     container_of(dev, struct rpmsg_ctrldev, dev)
#define cdev_to_ctrldev(i_cdev) container_of(i_cdev, struct rpmsg_ctrldev, cdev)

typedef struct RPMsgDualosRProc_s
{
    struct list_head     list;
    struct rpmsg_device *channel;
    bool                 adaptor_online;

    struct dualos_rpmsg_node *server;
    struct task_struct *      worker;
    wait_queue_head_t         bootup_done_wq;

    int stage;

    struct dualos_rpmsg_node *client;
    struct mutex              client_mutex;
    unsigned int              index;

    refcount_t refcnt;
} RPMsgDualosRProc_t;

/**
 * struct rpmsg_ctrldev - control device for instantiating endpoint devices
 * @rpdev:	underlaying rpmsg device
 * @cdev:	cdev for the ctrl device
 * @dev:	device for the ctrl device
 */
struct rpmsg_ctrldev
{
    struct cdev      cdev;
    struct device    dev;
    struct list_head eptdevList;
    struct mutex     eptdevMutex;

    struct list_head eptdevKernList;
    struct mutex     eptdevKernMutex;
};

static struct rpmsg_ctrldev *dualos_ctrldev;

/**
 * struct rpmsg_eptdev - endpoint device context
 * @dev:	endpoint device
 * @cdev:	cdev for the endpoint device
 * @rpdev:	underlaying rpmsg device
 * @chinfo:	info used to open the endpoint
 * @ept_lock:	synchronization of @ept modifications
 * @ept:	rpmsg endpoint reference, when open
 * @queue_lock:	synchronization of @queue operations
 * @queue:	incoming message queue
 * @readq:	wait object for incoming queue
 */
struct rpmsg_eptdev
{
    struct device dev;
    struct cdev   cdev;

    struct rpmsg_channel_info chinfo;

    struct mutex           ept_lock;
    struct rpmsg_endpoint *ept;

    spinlock_t          queue_lock;
    struct sk_buff_head queue;
    wait_queue_head_t   readq;

    int                 max_payload_length;
    rpmsg_device_type_t type;
    int                 dev_id;
    bool                destroyed;
};

typedef struct RPMsgDualosEPTDevKern_s
{
    struct list_head list;

    struct rpmsg_channel_info chinfo;

    struct mutex           ept_lock;
    struct rpmsg_endpoint *ept;

    spinlock_t          queue_lock;
    struct sk_buff_head queue;
    wait_queue_head_t   readq;

    int                 max_payload_length;
    rpmsg_device_type_t type;
    int                 dev_id;

    rpmsg_rx_cb_t cb;
    void *        priv;

    struct task_struct *worker;
} RPMsgDualosEPTDevKern_t;

struct ss_rpmsg_endpoint_info_kern
{
    struct list_head              list;
    struct ss_rpmsg_endpoint_info info;
};

static LIST_HEAD(g_stRProcList);
static struct mutex      g_stRProcListMutex;
static wait_queue_head_t g_stRProcWaitq;
static wait_queue_head_t g_stAdaptorOnlineWaitq;

static void RPMsgDualosEPTDevKernAdd(RPMsgDualosEPTDevKern_t *eptdevKern)
{
    struct rpmsg_ctrldev *   ctrldev = dualos_ctrldev;
    RPMsgDualosEPTDevKern_t *tmp;

    mutex_lock(&ctrldev->eptdevKernMutex);
    list_for_each_entry(tmp, &ctrldev->eptdevKernList, list)
    {
        if (tmp == eptdevKern)
        {
            pr_err(
                "BUG: eptdev_kern is already in list:0x%x,0x%x,0x%x,0x%x vs "
                "0x%x,0x%x,0x%x,0x%x!\n",
                tmp->type, tmp->dev_id, tmp->chinfo.src, tmp->chinfo.dst, eptdevKern->type, eptdevKern->dev_id,
                eptdevKern->chinfo.src, eptdevKern->chinfo.dst);
            mutex_unlock(&ctrldev->eptdevKernMutex);
            BUG_ON(1);
            return;
        }
    }
    list_add_tail(&eptdevKern->list, &ctrldev->eptdevKernList);
    mutex_unlock(&ctrldev->eptdevKernMutex);
    return;
}

static void RPMsgDualosEPTDevKernDel(RPMsgDualosEPTDevKern_t *eptdevKern)
{
    struct rpmsg_ctrldev *   ctrldev = dualos_ctrldev;
    RPMsgDualosEPTDevKern_t *tmp;

    mutex_lock(&ctrldev->eptdevKernMutex);
    list_for_each_entry(tmp, &ctrldev->eptdevKernList, list)
    {
        if (tmp == eptdevKern)
        {
            list_del_init(&tmp->list);
            break;
        }
    }
    mutex_unlock(&ctrldev->eptdevKernMutex);
    return;
}

static void RPMsgDualosEPTDevKernDestroy(rpmsg_device_type_t type, int dev_id)
{
    struct rpmsg_ctrldev *   ctrldev = dualos_ctrldev;
    RPMsgDualosEPTDevKern_t *eptdevKern;

    mutex_lock(&ctrldev->eptdevKernMutex);
    list_for_each_entry(eptdevKern, &ctrldev->eptdevKernList, list)
    {
        if (eptdevKern->type == type && eptdevKern->dev_id == dev_id)
        {
            mutex_lock(&eptdevKern->ept_lock);
            if (eptdevKern->ept)
            {
                rpmsg_destroy_ept(eptdevKern->ept);
                eptdevKern->ept = NULL;
            }
            mutex_unlock(&eptdevKern->ept_lock);

            /* wake up worker */
            wake_up_interruptible(&eptdevKern->readq);
        }
    }
    mutex_unlock(&ctrldev->eptdevKernMutex);
    return;
}

static int RPMsgDualosEPTDevKernWorker(void *priv)
{
    struct sk_buff *         skb;
    unsigned long            flags;
    RPMsgDualosEPTDevKern_t *eptdevKern;
    DEFINE_WAIT(wait);
    bool         eptGone = false;
    unsigned int src;

    eptdevKern = (RPMsgDualosEPTDevKern_t *)priv;
    while (1)
    {
        if (kthread_should_stop())
            break;

        mutex_lock(&eptdevKern->ept_lock);
        if (eptdevKern->ept == NULL)
            eptGone = true;
        mutex_unlock(&eptdevKern->ept_lock);

        if (eptGone)
        {
            eptdevKern->cb(NULL, NULL, 0, eptdevKern->priv, 0);
            break;
        }

        spin_lock_irqsave(&eptdevKern->queue_lock, flags);
        if (skb_queue_empty(&eptdevKern->queue))
        {
            prepare_to_wait(&eptdevKern->readq, &wait, TASK_INTERRUPTIBLE);
            spin_unlock_irqrestore(&eptdevKern->queue_lock, flags);

            schedule();

            finish_wait(&eptdevKern->readq, &wait);
            spin_lock_irqsave(&eptdevKern->queue_lock, flags);
        }

        skb = skb_dequeue(&eptdevKern->queue);
        spin_unlock_irqrestore(&eptdevKern->queue_lock, flags);

        if (!skb || skb->len <= 4)
            continue;

        src = *((unsigned int *)((unsigned char *)skb->data));
        eptdevKern->cb(NULL, (char *)skb->data + 4, skb->len - 4, eptdevKern->priv, src);
    }
    return 0;
}

static void RPMsgDualosRProcAdd(RPMsgDualosRProc_t *rproc)
{
    mutex_lock(&g_stRProcListMutex);
    refcount_set(&rproc->refcnt, 1);
    list_add_tail(&rproc->list, &g_stRProcList);
    wake_up_all(&g_stRProcWaitq);
    mutex_unlock(&g_stRProcListMutex);
}

static RPMsgDualosRProc_t *RPMsgDualosRProcGet(rpmsg_device_type_t type, int dev_id)
{
    RPMsgDualosRProc_t *tmp;
    RPMsgDualosRProc_t *rproc = NULL;

    mutex_lock(&g_stRProcListMutex);
    list_for_each_entry(tmp, &g_stRProcList, list)
    {
        if (tmp->channel->type == type && tmp->channel->dev_id == dev_id)
        {
            rproc = tmp;
            refcount_inc(&rproc->refcnt);
            break;
        }
    }
    mutex_unlock(&g_stRProcListMutex);

    return rproc;
}

static void RPMsgDualosRProcPut(RPMsgDualosRProc_t *rproc)
{
    if (refcount_dec_and_test(&rproc->refcnt))
    {
        pr_err("Bad reference count of rproc(0x%x,0x%x\n", rproc->channel->type, rproc->channel->dev_id);
    }
    return;
}

/**
 * After this function return, no one could see device
 * specified by type and dev_id.
 */
static void RPMsgDualosRProcDel(rpmsg_device_type_t type, int dev_id)
{
    RPMsgDualosRProc_t *tmp, *next;
    RPMsgDualosRProc_t *rproc = NULL;
    bool                done;

    do
    {
        done = true;
        mutex_lock(&g_stRProcListMutex);
        list_for_each_entry_safe(tmp, next, &g_stRProcList, list)
        {
            if (tmp->channel->type == type && tmp->channel->dev_id == dev_id)
            {
                rproc = tmp;
                if (refcount_dec_and_test(&rproc->refcnt))
                {
                    list_del_init(&rproc->list);

                    if (rproc->client)
                    {
                        dualos_rpmsg_node_release(rproc->client);
                        rproc->client = NULL;
                    }

                    if (rproc->server)
                    {
                        if (rproc->worker)
                        {
                            kthread_stop(rproc->worker);
                            rproc->worker = NULL;
                        }
                        dualos_rpmsg_node_release(rproc->server);
                        rproc->server = NULL;
                    }
                    kfree(rproc);
                }
                else
                {
                    done = false;
                    refcount_inc(&rproc->refcnt);
                }
                break;
            }
        }
        mutex_unlock(&g_stRProcListMutex);
        msleep(1);
    } while (!done);

    return;
}

static inline int dualos_rpmsg_check_devtype(u32 type)
{
    switch (type)
    {
        case RPMSG_MODE_RC_PORT:
        case RPMSG_MODE_EP_PORT:
        case RPMSG_MODE_SHM:
            return 0;
        default:
            break;
    }
    return -1;
}

int dualos_rpmsg_wait_remote_device(rpmsg_device_type_t type, int dev_id, signed long msecs)
{
    RPMsgDualosRProc_t *tmp;
    int                 timeout = msecs_to_jiffies(msecs);
    bool                found   = false;
    DEFINE_WAIT(wait);

    while (1)
    {
        mutex_lock(&g_stRProcListMutex);
        list_for_each_entry(tmp, &g_stRProcList, list)
        {
            if (tmp->channel->type == type && tmp->channel->dev_id == dev_id)
            {
                found = true;
                break;
            }
        }

        if (found || !timeout)
        {
            mutex_unlock(&g_stRProcListMutex);
            break;
        }

        prepare_to_wait(&g_stRProcWaitq, &wait, TASK_UNINTERRUPTIBLE);
        mutex_unlock(&g_stRProcListMutex);

        timeout = schedule_timeout(timeout);
        finish_wait(&g_stRProcWaitq, &wait);
    }

    if (found)
        return 0;

    return -EAGAIN;
}
EXPORT_SYMBOL(dualos_rpmsg_wait_remote_device);

void dualos_rpmsg_remote_adaptor_online(rpmsg_device_type_t type, int dev_id)
{
    RPMsgDualosRProc_t *rproc = NULL, *tmp;

    mutex_lock(&g_stRProcListMutex);
    list_for_each_entry(tmp, &g_stRProcList, list)
    {
        if (tmp->channel->type == type && tmp->channel->dev_id == dev_id)
        {
            rproc = tmp;
            break;
        }
    }

    if (rproc)
    {
        rproc->adaptor_online = true;
        wake_up_all(&g_stAdaptorOnlineWaitq);
    }
    mutex_unlock(&g_stRProcListMutex);
}
EXPORT_SYMBOL(dualos_rpmsg_remote_adaptor_online);

struct rpmsg_endpoint *dualos_rpmsg_create_ept(rpmsg_device_type_t type, int dev_id, rpmsg_rx_cb_t cb, void *priv,
                                               struct rpmsg_channel_info chinfo)
{
    RPMsgDualosRProc_t *   rproc;
    struct rpmsg_endpoint *ept;

    rproc = RPMsgDualosRProcGet(type, dev_id);
    if (!rproc)
        return ERR_PTR(-EINVAL);

    ept = rpmsg_create_ept(rproc->channel, cb, priv, chinfo);
    RPMsgDualosRProcPut(rproc);

    return ept;
}
EXPORT_SYMBOL(dualos_rpmsg_create_ept);

static int rpmsg_ept_kern_cb(struct rpmsg_device *rpdev, void *buf, int len, void *priv, u32 addr)
{
    RPMsgDualosEPTDevKern_t *eptdevKern = priv;
    struct sk_buff *         skb;

    skb = alloc_skb(len + sizeof(addr), GFP_ATOMIC);
    if (!skb)
        return -ENOMEM;

    memcpy(skb_put(skb, sizeof(addr)), &addr, sizeof(addr));
    memcpy(skb_put(skb, len), buf, len);

    spin_lock(&eptdevKern->queue_lock);
    skb_queue_tail(&eptdevKern->queue, skb);
    spin_unlock(&eptdevKern->queue_lock);

    /* wake up any blocking processes, waiting for new data */
    wake_up_interruptible(&eptdevKern->readq);

    return 0;
}

void *RPMsgDualosCreateEptdev(rpmsg_device_type_t type, int dev_id, rpmsg_rx_cb_t cb, void *priv,
                              struct rpmsg_channel_info chinfo)
{
    RPMsgDualosRProc_t *     rproc;
    struct rpmsg_endpoint *  ept;
    RPMsgDualosEPTDevKern_t *eptdevKern;
    char                     name[64];

    rproc = RPMsgDualosRProcGet(type, dev_id);
    if (!rproc)
        return ERR_PTR(-EPIPE);

    eptdevKern = kzalloc(sizeof(*eptdevKern), GFP_KERNEL);
    if (!eptdevKern)
    {
        RPMsgDualosRProcPut(rproc);
        return ERR_PTR(-ENOMEM);
    }

    ept = rpmsg_create_ept(rproc->channel, rpmsg_ept_kern_cb, eptdevKern, chinfo);
    if (!ept)
    {
        RPMsgDualosRProcPut(rproc);
        kfree(eptdevKern);
        return ERR_PTR(-EINVAL);
    }

    eptdevKern->ept    = ept;
    eptdevKern->chinfo = chinfo;
    eptdevKern->type   = type;
    eptdevKern->dev_id = dev_id;

    eptdevKern->max_payload_length = rproc->channel->max_payload_length;
    eptdevKern->cb                 = cb;
    eptdevKern->priv               = priv;

    mutex_init(&eptdevKern->ept_lock);
    spin_lock_init(&eptdevKern->queue_lock);
    skb_queue_head_init(&eptdevKern->queue);
    init_waitqueue_head(&eptdevKern->readq);

    snprintf(name, sizeof(name), "eptdevKern%x_%x", type, dev_id);
    eptdevKern->worker = kthread_create(RPMsgDualosEPTDevKernWorker, (void *)eptdevKern, name);
    if (IS_ERR(eptdevKern->worker))
    {
        int err = PTR_ERR(eptdevKern->worker);

        eptdevKern->worker = NULL;

        rpmsg_destroy_ept(eptdevKern->ept);
        eptdevKern->ept = NULL;

        RPMsgDualosRProcPut(rproc);
        kfree(eptdevKern);
        return ERR_PTR(err);
    }
    wake_up_process(eptdevKern->worker);

    RPMsgDualosEPTDevKernAdd(eptdevKern);
    RPMsgDualosRProcPut(rproc);

    return eptdevKern;
}
EXPORT_SYMBOL(RPMsgDualosCreateEptdev);

void RPMsgDualosDestroyEptdev(void *handle)
{
    RPMsgDualosEPTDevKern_t *eptdevKern;

    eptdevKern = (RPMsgDualosEPTDevKern_t *)handle;
    if (eptdevKern->worker)
    {
        kthread_stop(eptdevKern->worker);
        eptdevKern->worker = NULL;
    }

    mutex_lock(&eptdevKern->ept_lock);
    if (eptdevKern->ept)
    {
        rpmsg_destroy_ept(eptdevKern->ept);
        eptdevKern->ept = NULL;
    }
    mutex_unlock(&eptdevKern->ept_lock);

    /* Discard all SKBs */
    skb_queue_purge(&eptdevKern->queue);

    RPMsgDualosEPTDevKernDel(eptdevKern);
    kfree(eptdevKern);
}
EXPORT_SYMBOL(RPMsgDualosDestroyEptdev);

int RPMsgDualosEptdevSendto(void *handle, void *data, int len, u32 dst)
{
    RPMsgDualosEPTDevKern_t *eptdevKern;
    int                      ret;

    eptdevKern = (RPMsgDualosEPTDevKern_t *)handle;
    mutex_lock(&eptdevKern->ept_lock);
    if (!eptdevKern->ept)
    {
        ret = -EPIPE;
    }
    else
    {
        ret = rpmsg_sendto(eptdevKern->ept, data, len, dst);
    }
    mutex_unlock(&eptdevKern->ept_lock);

    return ret;
}
EXPORT_SYMBOL(RPMsgDualosEptdevSendto);

int RPMsgDualosEptdevTrySendto(void *handle, void *data, int len, u32 dst)
{
    RPMsgDualosEPTDevKern_t *eptdevKern;
    int                      ret;

    eptdevKern = (RPMsgDualosEPTDevKern_t *)handle;
    mutex_lock(&eptdevKern->ept_lock);
    if (!eptdevKern->ept)
    {
        ret = -EPIPE;
    }
    else
    {
        ret = rpmsg_trysendto(eptdevKern->ept, data, len, dst);
    }
    mutex_unlock(&eptdevKern->ept_lock);

    return ret;
}
EXPORT_SYMBOL(RPMsgDualosEptdevTrySendto);

int dualos_rpmsg_destroy_ept(struct rpmsg_endpoint *ept)
{
    if (!ept)
        return 0;

    rpmsg_destroy_ept(ept);
    return 0;
}
EXPORT_SYMBOL(dualos_rpmsg_destroy_ept);

static int dualos_rpmsg_node_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
    struct dualos_rpmsg_node *dnode = priv;
    struct sk_buff *          skb;

    skb = alloc_skb(len + sizeof(src), GFP_ATOMIC);
    if (!skb)
        return -ENOMEM;

    memcpy(skb_put(skb, sizeof(src)), &src, sizeof(src));
    memcpy(skb_put(skb, len), data, len);

    spin_lock(&dnode->queue_lock);
    skb_queue_tail(&dnode->queue, skb);
    spin_unlock(&dnode->queue_lock);

    wake_up_all(&dnode->wq);
    return 0;
}

static struct dualos_rpmsg_node *dualos_rpmsg_node_alloc_inner(void)
{
    struct dualos_rpmsg_node *dnode;

    dnode = kzalloc(sizeof(*dnode), GFP_KERNEL);
    if (!dnode)
    {
        pr_err("dualos_rpmsg_node_alloc: out of memory!\n");
        return NULL;
    }

    init_waitqueue_head(&dnode->wq);
    spin_lock_init(&dnode->queue_lock);
    skb_queue_head_init(&dnode->queue);

    return dnode;
}

struct dualos_rpmsg_node *dualos_rpmsg_node_alloc(rpmsg_device_type_t type, int dev_id, const char *name, int addr)
{
    struct dualos_rpmsg_node *dnode;
    struct rpmsg_channel_info info;

    dnode = dualos_rpmsg_node_alloc_inner();
    if (!dnode)
        return NULL;

    snprintf(info.name, sizeof(info.name), name);
    info.src = addr;
    info.dst = RPMSG_ADDR_ANY;

    dnode->ept = dualos_rpmsg_create_ept(type, dev_id, dualos_rpmsg_node_cb, dnode, info);
    if (IS_ERR_OR_NULL(dnode->ept))
    {
        pr_err("Rpmsg node (%s, %d) dualos_rpmsg_create_ept error\n", name, addr);
        goto err_create_ept;
    }
    return dnode;
err_create_ept:
    kfree(dnode);
    return NULL;
}
EXPORT_SYMBOL(dualos_rpmsg_node_alloc);

void dualos_rpmsg_node_release(struct dualos_rpmsg_node *dnode)
{
    if (!dnode)
        return;

    if (dnode->ept)
    {
        dualos_rpmsg_destroy_ept(dnode->ept);
        dnode->ept = NULL;
    }

    skb_queue_purge(&dnode->queue);
    kfree(dnode);
    return;
}
EXPORT_SYMBOL(dualos_rpmsg_node_release);

struct sk_buff *dualos_rpmsg_recv(struct dualos_rpmsg_node *dnode, unsigned char **data, unsigned int *src,
                                  signed long timeout)
{
    struct sk_buff *skb;
    unsigned long   flags;
    int             ret;
    DEFINE_WAIT(wait);

    spin_lock_irqsave(&dnode->queue_lock, flags);
    if (skb_queue_empty(&dnode->queue))
    {
        spin_unlock_irqrestore(&dnode->queue_lock, flags);

        prepare_to_wait(&dnode->wq, &wait, TASK_UNINTERRUPTIBLE);
        ret = schedule_timeout(timeout);
        finish_wait(&dnode->wq, &wait);
        spin_lock_irqsave(&dnode->queue_lock, flags);
    }
    skb = skb_dequeue(&dnode->queue);
    spin_unlock_irqrestore(&dnode->queue_lock, flags);

    if (!skb)
    {
        return NULL;
    }

    if (src)
        *src = *((unsigned int *)((unsigned char *)skb->data));
    if (data)
        *data = (unsigned char *)skb->data + 4;

    return skb;
}
EXPORT_SYMBOL(dualos_rpmsg_recv);

int dualos_rpmsg_sendto(struct dualos_rpmsg_node *dnode, int dst, char *buffer, int size, int timeout)
{
    int ret;

    ret = rpmsg_sendto(dnode->ept, buffer, size, dst);
    if (ret)
    {
        pr_err(
            "dualos node rpmsg_trysendto timeout %d, "
            "remote_addr=%d\n",
            ret, dst);
    }
    return ret;
}
EXPORT_SYMBOL(dualos_rpmsg_sendto);

static int rpmsg_eptdev_destroy(struct device *dev)
{
    struct rpmsg_eptdev *eptdev = dev_to_eptdev(dev);

    mutex_lock(&eptdev->ept_lock);
    if (eptdev->ept)
    {
        rpmsg_destroy_ept(eptdev->ept);
        eptdev->ept = NULL;
    }

    if (eptdev->destroyed)
    {
        mutex_unlock(&eptdev->ept_lock);
        return 0;
    }
    eptdev->destroyed = true;
    mutex_unlock(&eptdev->ept_lock);

    /* wake up any blocked readers */
    wake_up_interruptible(&eptdev->readq);

    cdev_device_del(&eptdev->cdev, dev);
    put_device(&eptdev->dev);

    return 0;
}

static int RPMsgEptdevDestroyEpt(struct device *dev, void *data)
{
    struct rpmsg_eptdev *eptdev = dev_to_eptdev(dev);
    struct rpmsg_device *rpdev  = (struct rpmsg_device *)data;

    if (rpdev && (rpdev->type != eptdev->type || rpdev->dev_id != eptdev->dev_id))
        return 0;

    mutex_lock(&eptdev->ept_lock);
    if (eptdev->ept)
    {
        rpmsg_destroy_ept(eptdev->ept);
        eptdev->ept = NULL;
    }
    else
    {
        mutex_unlock(&eptdev->ept_lock);
        return 0;
    }
    mutex_unlock(&eptdev->ept_lock);

    /* wake up any blocked readers */
    wake_up_interruptible(&eptdev->readq);
    return 0;
}

static int rpmsg_ept_cb(struct rpmsg_device *rpdev, void *buf, int len, void *priv, u32 addr)
{
    struct rpmsg_eptdev *eptdev = priv;
    struct sk_buff *     skb;

    skb = alloc_skb(len, GFP_ATOMIC);
    if (!skb)
        return -ENOMEM;

    memcpy(skb_put(skb, len), buf, len);

    spin_lock(&eptdev->queue_lock);
    skb_queue_tail(&eptdev->queue, skb);
    spin_unlock(&eptdev->queue_lock);

    /* wake up any blocking processes, waiting for new data */
    wake_up_interruptible(&eptdev->readq);

    return 0;
}

static int rpmsg_eptdev_open(struct inode *inode, struct file *filp)
{
    struct rpmsg_eptdev *  eptdev = cdev_to_eptdev(inode->i_cdev);
    struct rpmsg_endpoint *ept;
    struct device *        dev   = &eptdev->dev;
    RPMsgDualosRProc_t *   rproc = NULL;

    get_device(dev);

    mutex_lock(&eptdev->ept_lock);
    if (eptdev->ept != NULL)
    {
        mutex_unlock(&eptdev->ept_lock);
        put_device(dev);
        return -EBUSY;
    }

    rproc = RPMsgDualosRProcGet(eptdev->type, eptdev->dev_id);
    if (!rproc)
    {
        dev_err(dev, "Remote side(0x%x,0x%x) of %s is gone!\n", eptdev->type, eptdev->dev_id, eptdev->chinfo.name);
        mutex_unlock(&eptdev->ept_lock);
        put_device(dev);
        return -EPIPE;
    }

    ept = rpmsg_create_ept(rproc->channel, rpmsg_ept_cb, eptdev, eptdev->chinfo);
    if (!ept)
    {
        dev_err(dev, "failed to open %s\n", eptdev->chinfo.name);
        RPMsgDualosRProcPut(rproc);

        mutex_unlock(&eptdev->ept_lock);
        put_device(dev);
        return -EINVAL;
    }
    eptdev->max_payload_length = rproc->channel->max_payload_length;
    RPMsgDualosRProcPut(rproc);

    eptdev->ept = ept;
    mutex_unlock(&eptdev->ept_lock);

    filp->private_data = eptdev;
    return 0;
}

static int rpmsg_eptdev_release(struct inode *inode, struct file *filp)
{
    struct rpmsg_eptdev *eptdev = cdev_to_eptdev(inode->i_cdev);
    struct device *      dev    = &eptdev->dev;

    /* Close the endpoint, if it's not already destroyed by the parent */
    mutex_lock(&eptdev->ept_lock);
    if (eptdev->ept)
    {
        rpmsg_destroy_ept(eptdev->ept);
        eptdev->ept = NULL;
    }
    mutex_unlock(&eptdev->ept_lock);

    /* Discard all SKBs */
    skb_queue_purge(&eptdev->queue);

    put_device(dev);

    return 0;
}

static ssize_t rpmsg_eptdev_read(struct file *filp, char __user *buf, size_t len, loff_t *f_pos)
{
    struct rpmsg_eptdev *eptdev = filp->private_data;
    unsigned long        flags;
    struct sk_buff *     skb;
    int                  use;

    if (!eptdev->ept)
        return -EPIPE;

    spin_lock_irqsave(&eptdev->queue_lock, flags);

    /* Wait for data in the queue */
    if (skb_queue_empty(&eptdev->queue))
    {
        spin_unlock_irqrestore(&eptdev->queue_lock, flags);

        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;

        /* Wait until we get data or the endpoint goes away */
        if (wait_event_interruptible(eptdev->readq, !skb_queue_empty(&eptdev->queue) || !eptdev->ept))
            return -ERESTARTSYS;

        /* We lost the endpoint while waiting */
        if (!eptdev->ept)
            return -EPIPE;

        spin_lock_irqsave(&eptdev->queue_lock, flags);
    }

    skb = skb_dequeue(&eptdev->queue);
    spin_unlock_irqrestore(&eptdev->queue_lock, flags);
    if (!skb)
        return -EFAULT;

    use = min_t(size_t, len, skb->len);
    if (copy_to_user(buf, skb->data, use))
        use = -EFAULT;

    kfree_skb(skb);

    return use;
}

static ssize_t rpmsg_eptdev_write(struct file *filp, const char __user *buf, size_t len, loff_t *f_pos)
{
    struct rpmsg_eptdev *eptdev = filp->private_data;
    void *               kbuf;
    int                  ret;

    if (len > eptdev->max_payload_length)
        len = eptdev->max_payload_length;

    kbuf = memdup_user(buf, len);
    if (IS_ERR(kbuf))
        return PTR_ERR(kbuf);

    if (mutex_lock_interruptible(&eptdev->ept_lock))
    {
        ret = -ERESTARTSYS;
        goto free_kbuf;
    }

    if (!eptdev->ept)
    {
        ret = -EPIPE;
        goto unlock_eptdev;
    }

    if (eptdev->chinfo.dst != RPMSG_ADDR_ANY)
    {
        if (filp->f_flags & O_NONBLOCK)
            ret = rpmsg_trysendto(eptdev->ept, kbuf, len, eptdev->chinfo.dst);
        else
            ret = rpmsg_sendto(eptdev->ept, kbuf, len, eptdev->chinfo.dst);
    }
    else
    {
        if (filp->f_flags & O_NONBLOCK)
            ret = rpmsg_trysend(eptdev->ept, kbuf, len);
        else
            ret = rpmsg_send(eptdev->ept, kbuf, len);
    }

unlock_eptdev:
    mutex_unlock(&eptdev->ept_lock);

free_kbuf:
    kfree(kbuf);
    return ret < 0 ? ret : len;
}

static unsigned int rpmsg_eptdev_poll(struct file *filp, poll_table *wait)
{
    struct rpmsg_eptdev *eptdev = filp->private_data;
    unsigned int         mask   = 0;

    if (!eptdev->ept)
        return POLLERR;

    poll_wait(filp, &eptdev->readq, wait);

    if (!skb_queue_empty(&eptdev->queue))
        mask |= POLLIN | POLLRDNORM;

    mask |= rpmsg_poll(eptdev->ept, filp, wait);

    return mask;
}

static long rpmsg_eptdev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    struct rpmsg_eptdev *eptdev = fp->private_data;

    if (cmd != SS_RPMSG_DESTROY_EPT_IOCTL)
        return -EINVAL;

    return rpmsg_eptdev_destroy(&eptdev->dev);
}

static const struct file_operations rpmsg_eptdev_fops = {
    .owner          = THIS_MODULE,
    .open           = rpmsg_eptdev_open,
    .release        = rpmsg_eptdev_release,
    .read           = rpmsg_eptdev_read,
    .write          = rpmsg_eptdev_write,
    .poll           = rpmsg_eptdev_poll,
    .unlocked_ioctl = rpmsg_eptdev_ioctl,
    .compat_ioctl   = rpmsg_eptdev_ioctl,
};

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

    return sprintf(buf, "%s\n", eptdev->chinfo.name);
}
static DEVICE_ATTR_RO(name);

static ssize_t src_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

    return sprintf(buf, "%d\n", eptdev->chinfo.src);
}
static DEVICE_ATTR_RO(src);

static ssize_t dst_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

    return sprintf(buf, "%d\n", eptdev->chinfo.dst);
}
static DEVICE_ATTR_RO(dst);

static struct attribute *rpmsg_eptdev_attrs[] = {&dev_attr_name.attr, &dev_attr_src.attr, &dev_attr_dst.attr, NULL};
ATTRIBUTE_GROUPS(rpmsg_eptdev);

static void rpmsg_eptdev_release_device(struct device *dev)
{
    struct rpmsg_eptdev *               eptdev  = dev_to_eptdev(dev);
    struct rpmsg_ctrldev *              ctrldev = dualos_ctrldev;
    struct ss_rpmsg_endpoint_info_kern *tmp, *next;

    mutex_lock(&ctrldev->eptdevMutex);
    list_for_each_entry_safe(tmp, next, &ctrldev->eptdevList, list)
    {
        if ((tmp->info.id + 1) == MINOR(dev->devt))
        {
            list_del_init(&tmp->list);
            kfree(tmp);
            break;
        }
    }
    mutex_unlock(&ctrldev->eptdevMutex);

    ida_simple_remove(&rpmsg_ept_ida, dev->id);
    ida_simple_remove(&rpmsg_minor_ida, MINOR(eptdev->dev.devt));
    kfree(eptdev);
}

static int rpmsg_eptdev_create(struct rpmsg_ctrldev *ctrldev, struct rpmsg_channel_info chinfo,
                               struct ss_rpmsg_endpoint_info *eptinfo)
{
    struct rpmsg_eptdev *eptdev;
    struct device *      dev;
    int                  ret;

    eptdev = kzalloc(sizeof(*eptdev), GFP_KERNEL);
    if (!eptdev)
        return -ENOMEM;

    dev               = &eptdev->dev;
    eptdev->chinfo    = chinfo;
    eptdev->type      = eptinfo->mode;
    eptdev->dev_id    = eptinfo->target_id;
    eptdev->destroyed = false;

    mutex_init(&eptdev->ept_lock);
    spin_lock_init(&eptdev->queue_lock);
    skb_queue_head_init(&eptdev->queue);
    init_waitqueue_head(&eptdev->readq);

    device_initialize(dev);
    dev->class  = rpmsg_class;
    dev->parent = &ctrldev->dev;
    dev->groups = rpmsg_eptdev_groups;
    dev_set_drvdata(dev, eptdev);

    cdev_init(&eptdev->cdev, &rpmsg_eptdev_fops);
    eptdev->cdev.owner = THIS_MODULE;

    ret = ida_simple_get(&rpmsg_minor_ida, 0, RPMSG_DEV_MAX, GFP_KERNEL);
    if (ret < 0)
        goto free_eptdev;
    dev->devt = MKDEV(MAJOR(rpmsg_major), ret);

    ret = ida_simple_get(&rpmsg_ept_ida, 0, 0, GFP_KERNEL);
    if (ret < 0)
        goto free_minor_ida;
    dev->id = ret;
    dev_set_name(dev, "rpmsg%d", ret);
    eptinfo->id = ret;

    ret = cdev_device_add(&eptdev->cdev, dev);
    if (ret)
    {
        dev_err(dev, "cdev_device_add failed: %d\n", ret);
        goto free_ept_ida;
    }

    /* We can now rely on the release function for cleanup */
    dev->release = rpmsg_eptdev_release_device;

    return ret;

free_ept_ida:
    ida_simple_remove(&rpmsg_ept_ida, dev->id);
free_minor_ida:
    ida_simple_remove(&rpmsg_minor_ida, MINOR(dev->devt));
free_eptdev:
    put_device(dev);
    kfree(eptdev);

    return ret;
}

static int rpmsg_ctrldev_open(struct inode *inode, struct file *filp)
{
    struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);

    get_device(&ctrldev->dev);
    filp->private_data = ctrldev;

    return 0;
}

static int rpmsg_ctrldev_release(struct inode *inode, struct file *filp)
{
    struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);

    put_device(&ctrldev->dev);

    return 0;
}

static long rpmsg_ctrldev_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    struct rpmsg_ctrldev *ctrldev = fp->private_data;
    void __user *         argp    = (void __user *)arg;

    switch (cmd)
    {
        case SS_RPMSG_CREATE_EPT_IOCTL:
        {
            struct ss_rpmsg_endpoint_info_kern *eptinfo_kern;
            struct ss_rpmsg_endpoint_info_kern *tmp;
            struct rpmsg_channel_info           chinfo;
            int                                 ret   = 0;
            bool                                found = false;

            eptinfo_kern = kzalloc(sizeof(struct ss_rpmsg_endpoint_info_kern), GFP_KERNEL);
            if (!eptinfo_kern)
                return -ENOMEM;

            if (copy_from_user(&eptinfo_kern->info, argp, sizeof(struct ss_rpmsg_endpoint_info)))
            {
                kfree(eptinfo_kern);
                return -EFAULT;
            }

            mutex_lock(&ctrldev->eptdevMutex);
            list_for_each_entry(tmp, &ctrldev->eptdevList, list)
            {
                if (strncmp(tmp->info.name, eptinfo_kern->info.name, sizeof(tmp->info.name) - 1) != 0)
                    continue;

                if (tmp->info.src != eptinfo_kern->info.src)
                    continue;

                if (tmp->info.dst != eptinfo_kern->info.dst)
                    continue;

                if (tmp->info.mode != eptinfo_kern->info.mode)
                    continue;

                if (tmp->info.target_id != eptinfo_kern->info.target_id)
                    continue;

                found                 = true;
                eptinfo_kern->info.id = tmp->info.id;
            }

            if (!found)
            {
                memcpy(chinfo.name, eptinfo_kern->info.name, RPMSG_NAME_SIZE);
                chinfo.name[RPMSG_NAME_SIZE - 1] = '\0';
                chinfo.src                       = eptinfo_kern->info.src;
                chinfo.dst                       = eptinfo_kern->info.dst;

                ret = rpmsg_eptdev_create(ctrldev, chinfo, &eptinfo_kern->info);
                if (ret)
                {
                    kfree(eptinfo_kern);
                    mutex_unlock(&ctrldev->eptdevMutex);
                    return ret;
                }

                list_add_tail(&eptinfo_kern->list, &ctrldev->eptdevList);
            }
            mutex_unlock(&ctrldev->eptdevMutex);

            if (copy_to_user(argp, &eptinfo_kern->info, sizeof(struct ss_rpmsg_endpoint_info)))
            {
                ret = -EFAULT;
            }

            if (found)
                kfree(eptinfo_kern);
            return ret;
        }
        case SS_RPMSG_DEVICES_INFO_IOCTL:
        {
            struct ss_rpmsg_devices_info info;
            RPMsgDualosRProc_t *         tmp;
            u32                          i = 0, count;
            u32 *                        ids;

            if (copy_from_user(&info, argp, sizeof(info)))
                return -EFAULT;

            if (dualos_rpmsg_check_devtype(info.mode) < 0)
                return -EINVAL;

            if ((info.count != 0 && info.buffer == 0) || (info.count == 0 && info.buffer != 0))
                return -EINVAL;

            count = 256;
            ids   = (u32 *)kzalloc(count * sizeof(u32), GFP_KERNEL);
            if (!ids)
                return -ENOMEM;

            mutex_lock(&g_stRProcListMutex);
            list_for_each_entry(tmp, &g_stRProcList, list)
            {
                if (tmp->channel->type == info.mode)
                {
                    if (i < count)
                    {
                        ids[i++] = tmp->channel->dev_id;
                    }
                    else
                    {
                        dev_warn(&ctrldev->dev,
                                 "Number of %d "
                                 "type slaves exceeds %d!\n",
                                 info.mode, count);
                        break;
                    }
                }
            }
            mutex_unlock(&g_stRProcListMutex);

            if (info.count == 0 && info.buffer == 0)
            {
                kfree(ids);
                info.count = i;
                if (copy_to_user((void *)argp, (void *)&info, sizeof(info)))
                {
                    return -EFAULT;
                }
                return 0;
            }

            if (info.count > i)
                info.count = i;

            if (copy_to_user((void *)info.buffer, ids, i * sizeof(u32)))
            {
                kfree(ids);
                return -EFAULT;
            }
            kfree(ids);

            if (copy_to_user((void *)argp, (void *)&info, sizeof(info)))
            {
                return -EFAULT;
            }
            return 0;
        }
        case SS_RPMSG_QUERY_IOCTL:
        {
            struct ss_rpmsg_query_info info;
            RPMsgDualosRProc_t *       rproc;

            if (copy_from_user(&info, argp, sizeof(info)))
                return -EFAULT;

            if (dualos_rpmsg_check_devtype(info.mode) < 0)
                return -EINVAL;

            if (info.mode != RPMSG_MODE_RC_PORT || info.param != RPMSG_QUERY_STAGE)
                return -EINVAL;

            rproc = RPMsgDualosRProcGet(info.mode, info.target_id);
            if (!rproc)
                return -ENODEV;

            info.value = rproc->stage;

            RPMsgDualosRProcPut(rproc);
            if (copy_to_user((void *)argp, (void *)&info, sizeof(info)))
            {
                return -EFAULT;
            }
            return 0;
        }
        case SS_RPMSG_LOAD_IMAGE_IOCTL:
        {
            struct ss_rpmsg_image_info info;
            RPMsgDualosRProc_t *       rproc;
            struct bootup_image_info * req;
            struct bootup_return *     resp;
            struct sk_buff *           skb;
            int                        ret;

            if (copy_from_user(&info, argp, sizeof(info)))
                return -EFAULT;
            info.name[sizeof(info.name) - 1] = '\0';

            if (info.mode != RPMSG_MODE_RC_PORT)
                return -EINVAL;

            rproc = RPMsgDualosRProcGet(info.mode, info.target_id);
            if (!rproc)
                return -ENODEV;

            mutex_lock(&rproc->client_mutex);
            req                        = (struct bootup_image_info *)kzalloc(sizeof(*req), GFP_KERNEL);
            req->common.header.version = SSTAR_RPMSG_DUALOS_VERSION;
            req->common.header.index   = rproc->index++;
            req->common.type           = BOOTUP_TYPE_LOAD_IMAGE;

            snprintf(req->name, sizeof(req->name), "%s", info.name);
            req->addr_phys      = info.addr_phys;
            req->load_addr_phys = info.load_addr_phys;
            req->size           = info.size;

            ret = dualos_rpmsg_sendto(rproc->client, EPT_ADDR_BOOTUP_SERVER, (char *)req, sizeof(*req), 0);
            if (ret)
            {
                rpmsg_dualos_err("dalos_rpmsg_sendto return error %d\n", ret);
                kfree(req);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            skb = dualos_rpmsg_recv(rproc->client, (unsigned char **)&resp, NULL, LOAD_IMAGE_TIMEOUT * HZ);
            if (!skb)
            {
                kfree(req);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            if (resp->common.header.index != req->common.header.index)
            {
                rpmsg_dualos_err("index not match(%d-%d)\n", resp->common.header.index, req->common.header.index);
                kfree(req);
                kfree_skb(skb);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            ret = resp->ret;
            kfree(req);
            kfree_skb(skb);
            mutex_unlock(&rproc->client_mutex);
            RPMsgDualosRProcPut(rproc);
            return ret ? ret : 0;
        }
        case SS_RPMSG_BOOTUP_IOCTL:
        {
            struct ss_rpmsg_bootup_info info;
            RPMsgDualosRProc_t *        rproc;
            struct bootup_info *        req;
            struct bootup_return *      resp;
            struct sk_buff *            skb;
            int                         ret;

            if (copy_from_user(&info, argp, sizeof(info)))
                return -EFAULT;
            info.name[sizeof(info.name) - 1] = '\0';

            if (info.mode != RPMSG_MODE_RC_PORT)
                return -EINVAL;

            rproc = RPMsgDualosRProcGet(info.mode, info.target_id);
            if (!rproc)
                return -ENODEV;

            mutex_lock(&rproc->client_mutex);
            req                        = (struct bootup_info *)kzalloc(sizeof(*req), GFP_KERNEL);
            req->common.header.version = SSTAR_RPMSG_DUALOS_VERSION;
            req->common.header.index   = rproc->index++;

            req->common.type = BOOTUP_TYPE_BOOTUP;
            snprintf(req->name, sizeof(req->name), "%s", info.name);
            req->entry                   = info.entry;
            req->bootargs_addr_phys      = info.bootargs_addr_phys;
            req->bootargs_load_addr_phys = info.bootargs_load_addr_phys;
            req->bootargs_size           = info.bootargs_size;

            ret = dualos_rpmsg_sendto(rproc->client, EPT_ADDR_BOOTUP_SERVER, (char *)req, sizeof(*req), 0);
            if (ret)
            {
                rpmsg_dualos_err("dalos_rpmsg_sendto return error %d\n", ret);
                kfree(req);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            skb = dualos_rpmsg_recv(rproc->client, (unsigned char **)&resp, NULL, BOOTUP_TIMEOUT * HZ);
            if (!skb)
            {
                kfree(req);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            if (resp->common.header.index != req->common.header.index)
            {
                rpmsg_dualos_err("index not match(%d-%d)\n", resp->common.header.index, req->common.header.index);
                kfree(req);
                kfree_skb(skb);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }

            ret = resp->ret;
            kfree(req);
            kfree_skb(skb);
            mutex_unlock(&rproc->client_mutex);

            if (ret)
            {
                RPMsgDualosRProcPut(rproc);
                return ret;
            }

            if (info.flags & RPMSG_BOOTUP_FLAG_WAIT_APP)
            {
                DEFINE_WAIT(wait);

                prepare_to_wait(&rproc->bootup_done_wq, &wait, TASK_UNINTERRUPTIBLE);
                if (rproc->stage != RPMSG_STAGE_APP)
                    schedule_timeout(BOOTUP_DONE_TIMEOUT * HZ);
                finish_wait(&rproc->bootup_done_wq, &wait);
                ret = rproc->stage != RPMSG_STAGE_APP ? -EIO : 0;
            }
            RPMsgDualosRProcPut(rproc);
            return ret;
        }
        case SS_RPMSG_BOOTUP_DONE_IOCTL:
        {
            struct ss_rpmsg_bootup_done done;
            RPMsgDualosRProc_t *        rproc;
            struct bootup_stage_notify *req;
            int                         ret;

            if (copy_from_user(&done, argp, sizeof(done)))
                return -EFAULT;

            if (done.mode != RPMSG_MODE_EP_PORT)
                return -EINVAL;

            rproc = RPMsgDualosRProcGet(done.mode, done.target_id);
            if (!rproc)
                return -ENODEV;

            mutex_lock(&rproc->client_mutex);
            req                        = (struct bootup_stage_notify *)kzalloc(sizeof(*req), GFP_KERNEL);
            req->common.header.version = SSTAR_RPMSG_DUALOS_VERSION;
            req->common.header.index   = rproc->index++;

            req->common.type = BOOTUP_TYPE_STAGE_NOTIFY;
            req->stage       = RPMSG_STAGE_APP;

            ret = dualos_rpmsg_sendto(rproc->client, EPT_ADDR_BOOTUP_SERVER, (char *)req, sizeof(*req), 0);
            if (ret)
            {
                rpmsg_dualos_err("dalos_rpmsg_sendto return error %d\n", ret);
                kfree(req);
                mutex_unlock(&rproc->client_mutex);
                RPMsgDualosRProcPut(rproc);
                return -EIO;
            }
            kfree(req);
            mutex_unlock(&rproc->client_mutex);
            RPMsgDualosRProcPut(rproc);
            return 0;
        }
        default:
            break;
    }
    return -EINVAL;
};

static const struct file_operations rpmsg_ctrldev_fops = {
    .owner          = THIS_MODULE,
    .open           = rpmsg_ctrldev_open,
    .release        = rpmsg_ctrldev_release,
    .unlocked_ioctl = rpmsg_ctrldev_ioctl,
    .compat_ioctl   = rpmsg_ctrldev_ioctl,
};

static void rpmsg_ctrldev_release_device(struct device *dev)
{
    struct rpmsg_ctrldev *ctrldev = dev_to_ctrldev(dev);

    ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
    ida_simple_remove(&rpmsg_minor_ida, MINOR(dev->devt));
    kfree(ctrldev);
}

static int rpmsg_dualos_worker(void *arg)
{
    struct sk_buff *             skb;
    struct bootup_common_header *common;
    unsigned int                 src;
    RPMsgDualosRProc_t *         rproc;

    rproc = (RPMsgDualosRProc_t *)arg;
    while (1)
    {
        if (kthread_should_stop())
            break;

        skb = dualos_rpmsg_recv(rproc->server, (unsigned char **)&common, &src, MAX_SCHEDULE_TIMEOUT);
        if (!skb)
        {
            continue;
        }

        if (common->type == BOOTUP_TYPE_STAGE_NOTIFY)
        {
            struct bootup_stage_notify *done = (struct bootup_stage_notify *)common;

            rproc->stage = done->stage;
            if (rproc->stage == RPMSG_STAGE_APP)
                wake_up_all(&rproc->bootup_done_wq);
        }
        kfree(skb);
    }
    return 0;
}

static int rpmsg_dualos_probe(struct rpmsg_device *rpdev)
{
    int                       ret = 0;
    RPMsgDualosRProc_t *      rproc;
    struct rpmsg_channel_info info;

    rproc = (RPMsgDualosRProc_t *)kzalloc(sizeof(*rproc), GFP_KERNEL);
    if (!rproc)
        return -ENOMEM;

    mutex_init(&rproc->client_mutex);
    init_waitqueue_head(&rproc->bootup_done_wq);

    rproc->channel = rpdev;
    rproc->index   = 0;
    if (rpdev->type == Rpmsg_Device_RC_PORT)
    {
        rproc->stage = RPMSG_STAGE_RTOS;
    }

    rproc->client = dualos_rpmsg_node_alloc_inner();
    if (!rproc->client)
    {
        pr_err(
            "rpmsg_dualos_probe: failed create client node "
            "for channel(type=%d,dev_id=%d), \n",
            rproc->channel->type, rproc->channel->dev_id);
    }

    snprintf(info.name, sizeof(info.name), "%s", "bootup_client");
    info.src           = RPMSG_ADDR_ANY;
    info.dst           = RPMSG_ADDR_ANY;
    rproc->client->ept = rpmsg_create_ept(rproc->channel, dualos_rpmsg_node_cb, rproc->client, info);
    if (rproc->client->ept == NULL)
    {
        dualos_rpmsg_node_release(rproc->client);
        kfree(rproc);
        return -ENOMEM;
    }

    rproc->server = dualos_rpmsg_node_alloc_inner();
    if (!rproc->server)
    {
        pr_err(
            "rpmsg_dualos_probe: failed create server node "
            "for channel(type=%d,dev_id=%d), \n",
            rproc->channel->type, rproc->channel->dev_id);
    }
    else
    {
        snprintf(info.name, sizeof(info.name), "%s", "bootup_server");
        info.src           = EPT_ADDR_BOOTUP_SERVER;
        info.dst           = RPMSG_ADDR_ANY;
        rproc->server->ept = rpmsg_create_ept(rproc->channel, dualos_rpmsg_node_cb, rproc->server, info);
        if (rproc->server->ept == NULL)
        {
            dualos_rpmsg_node_release(rproc->server);
            dualos_rpmsg_node_release(rproc->client);
            kfree(rproc);
            return -ENOMEM;
        }

        rproc->worker = kthread_create(rpmsg_dualos_worker, (void *)rproc, "bootup_worker");
        if (IS_ERR_VALUE(rproc->worker))
        {
            pr_err("failed to create worker for channel(type=%d,dev_id=%d), \n", rproc->channel->type,
                   rproc->channel->dev_id);
            dualos_rpmsg_node_release(rproc->server);
            rproc->server = NULL;
        }
        wake_up_process(rproc->worker);
    }
    dev_set_drvdata(&rpdev->dev, dualos_ctrldev);

    if (rpdev->type == Rpmsg_Device_EP_PORT)
    {
        struct bootup_stage_notify *req;

        mutex_lock(&rproc->client_mutex);
        req                        = (struct bootup_stage_notify *)kzalloc(sizeof(*req), GFP_KERNEL);
        req->common.header.version = SSTAR_RPMSG_DUALOS_VERSION;
        req->common.header.index   = rproc->index++;

        req->common.type = BOOTUP_TYPE_STAGE_NOTIFY;
        req->stage       = RPMSG_STAGE_LINUX;

        ret = dualos_rpmsg_sendto(rproc->client, EPT_ADDR_BOOTUP_SERVER, (char *)req, sizeof(*req), 0);
        if (ret)
        {
            rpmsg_dualos_err("dalos_rpmsg_sendto return error %d\n", ret);
        }
        kfree(req);
        mutex_unlock(&rproc->client_mutex);
    }

    RPMsgDualosRProcAdd(rproc);
    pr_info("rpmsg_dualos_probe: add new channel(type=%d,dev_id=%d)\n", rproc->channel->type, rproc->channel->dev_id);
    return ret;
}

static void rpmsg_dualos_remove(struct rpmsg_device *rpdev)
{
    struct rpmsg_ctrldev *ctrldev = dev_get_drvdata(&rpdev->dev);
    int                   ret;

    /*
     * Delete rproc from global list,
     * then no one could see it after
     * RPMsgDualosRProcDel() return
     */
    RPMsgDualosRProcDel(rpdev->type, rpdev->dev_id);

    /*
     * Close all kernel space RPMsg Endpoints for this
     * remote processor
     */
    RPMsgDualosEPTDevKernDestroy(rpdev->type, rpdev->dev_id);

    /*
     * Close all userspace RPMsg Endpoints for this
     * remote processor
     */
    mutex_lock(&ctrldev->eptdevMutex);
    ret = device_for_each_child(&ctrldev->dev, rpdev, RPMsgEptdevDestroyEpt);
    mutex_unlock(&ctrldev->eptdevMutex);

    if (ret)
        dev_warn(&rpdev->dev, "failed to nuke endpoints: %d\n", ret);
}

static struct rpmsg_device_id rpmsg_driver_dualos_id_table[] = {
    {.name = "rpmsg_dualos"},
    {},
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_dualos_id_table);

static int rpmsg_dualos_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
    dev_dbg(&rpdev->dev, "rpmsg_dualos_cb receives %d from src 0x%x\n", len, src);
    return 0;
}

static struct rpmsg_driver rpmsg_dualos_driver = {
    .probe    = rpmsg_dualos_probe,
    .remove   = rpmsg_dualos_remove,
    .id_table = rpmsg_driver_dualos_id_table,
    .callback = rpmsg_dualos_cb,
    .drv =
        {
            .name = "rpmsg_dualos",
        },
};

static int rpmsg_dualos_init(void)
{
    struct rpmsg_ctrldev *ctrldev;
    struct device *       dev;
    int                   ret;

    init_waitqueue_head(&g_stRProcWaitq);
    init_waitqueue_head(&g_stAdaptorOnlineWaitq);
    mutex_init(&g_stRProcListMutex);

    ret = alloc_chrdev_region(&rpmsg_major, 0, RPMSG_DEV_MAX, "rpmsg");
    if (ret < 0)
    {
        pr_err("rpmsg: failed to allocate char dev region\n");
        return ret;
    }

    rpmsg_class = class_create(THIS_MODULE, "rpmsg");
    if (IS_ERR(rpmsg_class))
    {
        pr_err("failed to create rpmsg class\n");
        unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
        return PTR_ERR(rpmsg_class);
    }

    ret = register_rpmsg_driver(&rpmsg_dualos_driver);
    if (ret < 0)
    {
        pr_err("rpmsg_dualos: failed to register rpmsg driver\n");
        class_destroy(rpmsg_class);
        unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
    }

    ctrldev = kzalloc(sizeof(*ctrldev), GFP_KERNEL);
    if (!ctrldev)
        return -ENOMEM;

    INIT_LIST_HEAD(&ctrldev->eptdevList);
    mutex_init(&ctrldev->eptdevMutex);

    INIT_LIST_HEAD(&ctrldev->eptdevKernList);
    mutex_init(&ctrldev->eptdevKernMutex);

    dev = &ctrldev->dev;
    device_initialize(dev);
    dev->parent = NULL;
    dev->class  = rpmsg_class;

    cdev_init(&ctrldev->cdev, &rpmsg_ctrldev_fops);
    ctrldev->cdev.owner = THIS_MODULE;

    ret = ida_simple_get(&rpmsg_minor_ida, 0, RPMSG_DEV_MAX, GFP_KERNEL);
    if (ret < 0)
        goto free_ctrldev;

    dev->devt = MKDEV(MAJOR(rpmsg_major), ret);

    ret = ida_simple_get(&rpmsg_ctrl_ida, 0, 0, GFP_KERNEL);
    if (ret < 0)
        goto free_minor_ida;
    dev->id = ret;
    dev_set_name(&ctrldev->dev, "rpmsg_ctrl%d", ret);

    ret = cdev_device_add(&ctrldev->cdev, dev);
    if (ret)
    {
        dev_err(dev, "cdev_device_add failed: %d\n", ret);
        goto free_ctrl_ida;
    }

    /* We can now rely on the release function for cleanup */
    dev->release = rpmsg_ctrldev_release_device;

    dualos_ctrldev = ctrldev;

    pr_info("rpmsg_dualos init success!\n");
    return ret;
free_ctrl_ida:
    ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
free_minor_ida:
    ida_simple_remove(&rpmsg_minor_ida, MINOR(dev->devt));
free_ctrldev:
    put_device(dev);
    kfree(ctrldev);
    return ret;
}
postcore_initcall(rpmsg_dualos_init);

MODULE_ALIAS("rpmsg:rpmsg_dualos");
MODULE_LICENSE("GPL v2");
