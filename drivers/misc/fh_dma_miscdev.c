#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>


#include "fh_dma_miscdev.h"

#define MEMCPY_UNIT     (4095 * 4 * 64)         //4095 xfer * 32-bit * 64 desc

//#define FH_DMA_DEBUG

#ifdef FH_DMA_DEBUG
#define PRINT_DMA_DBG(fmt, args...)     \
    do                              \
    {                               \
        printk("FH_DMA_DEBUG: ");   \
        printk(fmt, ## args);       \
    }                               \
    while(0)
#else
#define PRINT_DMA_DBG(fmt, args...)  do { } while (0)
#endif


static void fh_dma_callback(void *data)
{
    PRINT_DMA_DBG("dma transfer done, end=%lu\n", jiffies);
    complete(data);
}

static int kick_off_dma(struct dma_chan *channel, unsigned int src_offset, unsigned int dst_offset, unsigned int size)
{
    int ret;
    struct completion cmp;
    struct dma_async_tx_descriptor *dma_tx_desc = NULL;
    struct dma_device *dma_dev = channel->device;
    dma_cookie_t cookie;
    unsigned long timeout;
    unsigned long flag;

    flag = DMA_CTRL_ACK | DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP | DMA_COMPL_SKIP_SRC_UNMAP;

    PRINT_DMA_DBG("try to copy 0x%x bytes: 0x%x --> 0x%x\n", MEMCPY_UNIT, src_offset, dst_offset);

    dma_tx_desc = dma_dev->device_prep_dma_memcpy(channel, dst_offset, src_offset, size, flag);

    PRINT_DMA_DBG("device_prep_dma_memcpy end\n");

    if(!dma_tx_desc)
    {
        pr_err("ERROR: %s, device_prep_dma_memcpy fail\n", __func__);
        ret = -ENODEV;
        return ret;
    }

    init_completion(&cmp);
    dma_tx_desc->callback = fh_dma_callback;
    dma_tx_desc->callback_param = &cmp;
    PRINT_DMA_DBG("tx_submit start\n");
    cookie = dma_tx_desc->tx_submit(dma_tx_desc);
    PRINT_DMA_DBG("tx_submit end\n");
    if (dma_submit_error(cookie))
    {
        pr_err("ERROR: %s, tx_submit fail\n", __func__);
        ret = -ENODEV;
        return ret;
    }
    PRINT_DMA_DBG("dma_async_issue_pending start\n");
    dma_async_issue_pending(channel);
    PRINT_DMA_DBG("dma_async_issue_pending end, %d\n", DMA_MEMCPY_TIMEOUT);

    timeout = wait_for_completion_timeout(&cmp, msecs_to_jiffies(DMA_MEMCPY_TIMEOUT));

    PRINT_DMA_DBG("wait_for_completion_timeout end, timeout: %lu\n", timeout);

    if(!timeout)
    {
        pr_err("ERROR: %s, dma transfer fail, timeout\n", __func__);
        ret = -ENODEV;
        return ret;
    }

    ret = dma_async_is_tx_complete(channel, cookie, NULL, NULL);

    if(ret)
    {
        pr_err("ERROR: %s, dma transfer fail, incorrect status: %d\n", __func__, ret);
        ret = -ENODEV;
        return ret;
    }

    return 0;
}


static int fh_dma_start_transfer(struct dma_chan *channel, struct dma_memcpy* memcpy)
{
    int ret;
    unsigned int i;

    for(i=0; i<memcpy->size / MEMCPY_UNIT; i++)
    {
        ret = kick_off_dma(channel, memcpy->src_addr_phy + MEMCPY_UNIT*i, memcpy->dst_addr_phy + MEMCPY_UNIT*i, MEMCPY_UNIT);
        if(ret)
        {
            return ret;
        }
    }

    ret = kick_off_dma(channel, memcpy->src_addr_phy + MEMCPY_UNIT*i, memcpy->dst_addr_phy + MEMCPY_UNIT*i, memcpy->size % MEMCPY_UNIT);
    return ret;
}


static bool chan_filter(struct dma_chan *chan, void *param)
{
    struct dma_memcpy* memcpy = param;
    PRINT_DMA_DBG("chan_filter, channel id: %d\n", memcpy->chan_id);
    if(memcpy->chan_id < 0)
    {
        return false;
    }

    if(memcpy->chan_id == chan->chan_id)
    {
        return true;
    }
    else
    {
        return false;
    }

}

static int fh_dma_memcpy(struct dma_memcpy* memcpy)
{
    //fixme: ioctl should be atomic, otherwise channel will be changed.
    struct dma_chan *dma_channel;
    dma_cap_mask_t mask;
    int ret;

    PRINT_DMA_DBG("fh_dma_memcpy start\n");
    PRINT_DMA_DBG("ioctl, memcpy->size: 0x%x\n", memcpy->size);


    PRINT_DMA_DBG("fh_dma_request_channel start\n");
    dma_cap_zero(mask);
    PRINT_DMA_DBG("dma_cap_zero end\n");
    dma_cap_set(DMA_MEMCPY, mask);
    PRINT_DMA_DBG("dma_cap_set end\n");

    dma_channel = dma_request_channel(mask, chan_filter, memcpy);

    PRINT_DMA_DBG("dma_request_channel finished, channel_addr: 0x%x\n", (u32)dma_channel);

    if(!dma_channel)
    {
        pr_err("ERROR: %s, No Channel Available, channel: %d\n", __func__, memcpy->chan_id);
        return -EBUSY;
    }
    memcpy->chan_id = dma_channel->chan_id;
    PRINT_DMA_DBG("dma channel name: %s\n", dma_chan_name(dma_channel));

    ret = fh_dma_start_transfer(dma_channel, memcpy);

    if(ret)
    {
        pr_err("ERROR: %s, DMA Xfer Failed\n", __func__);
    }

    dma_channel->device->device_free_chan_resources(dma_channel);
    dma_release_channel(dma_channel);

    return ret;
}

static long fh_dma_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct dma_memcpy memcpy;


    if (unlikely(_IOC_TYPE(cmd) != DMA_IOCTL_MAGIC))
    {
        pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
               __func__, _IOC_TYPE(cmd), -ENOTTY);
        return -ENOTTY;
    }

    if (unlikely(_IOC_NR(cmd) > DMA_IOCTL_MAXNR))
    {
        pr_err("%s: ERROR: incorrect cmd num %d (error: %d)\n",
               __func__, _IOC_NR(cmd), -ENOTTY);
        return -ENOTTY;
    }

    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(ret)
    {
        pr_err("%s: ERROR: user space access is not permitted %d (error: %d)\n",
               __func__, _IOC_NR(cmd), -EACCES);
        return -EACCES;
    }

    switch(cmd)
    {

    case DMA_MEMCOPY:
		if(copy_from_user((void *)&memcpy,
							(void __user *)arg,
							sizeof(struct dma_memcpy)))
		{
			return -EFAULT;
		}
        ret = fh_dma_memcpy(&memcpy);
        break;
    }

    return ret;
}

static int fh_dma_open(struct inode *inode, struct file *file)
{
    PRINT_DMA_DBG("fh_dma_open\n");
    return 0;
}

static int fh_dma_release(struct inode *inode, struct file *filp)
{
    PRINT_DMA_DBG("fh_dma_release\n");
    return 0;
}


static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter = 0;
    if (*pos == 0)
        return &counter;
    else
    {
        *pos = 0;
        return NULL;
    }
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{

    seq_printf(sfile, "\nISP Status\n");
    seq_printf(sfile, "\nCTRL: \n");

#if 0
    int i;
    u32 data;
    seq_printf(sfile, "ipf reg:\n");
    for(i=0; i<10; i++)
    {
        data = GET_IPF_REG_V(i*4);
        seq_printf(sfile, "0x%05x, 0x%08x\n", i*4, data);
    }
#endif

    return 0;
}

static const struct seq_operations fh_dma_seq_ops =
{
    .start = v_seq_start,
    .next = v_seq_next,
    .stop = v_seq_stop,
    .show = v_seq_show
};

static int isp_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &fh_dma_seq_ops);
}

static struct file_operations fh_dma_proc_ops =
{
    .owner = THIS_MODULE,
    .open = isp_proc_open,
    .read = seq_read,
};

static const struct file_operations fh_dma_fops =
{
    .owner                  = THIS_MODULE,
    .open                   = fh_dma_open,
    .release                = fh_dma_release,
    .unlocked_ioctl         = fh_dma_ioctl,
};

static struct miscdevice fh_dma_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fh_dma_fops,
};

static int __init fh_dma_init(void)
{
    int ret;
    struct proc_dir_entry *proc_file;
    ret = misc_register(&fh_dma_device);

    if(ret < 0)
    {
        pr_err("%s: ERROR: %s registration failed",
            __func__, DEVICE_NAME);
        return -ENXIO;
    }

    proc_file = create_proc_entry(PROC_FILE, 0644, NULL);

    if (proc_file)
        proc_file->proc_fops = &fh_dma_proc_ops;
    else
        pr_err("%s: ERROR: %s proc file create failed",
               __func__, DEVICE_NAME);


    return ret;
}

static void __exit fh_dma_exit(void)
{
    remove_proc_entry(PROC_FILE, NULL);
    misc_deregister(&fh_dma_device);
}
module_init(fh_dma_init);
module_exit(fh_dma_exit);

MODULE_AUTHOR("QIN");
MODULE_DESCRIPTION("Misc Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH");
