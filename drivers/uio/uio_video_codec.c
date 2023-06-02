/**
 * drivers/uio/uio_video_codec.c
 *
 * Userspace I/O driver for anyka soc video hardware codec.
 * Based on uio_pdrv.c by Uwe Kleine-Koenig,
 *
 * Jacky Lau
 * 2011-07-05
 *
 * Copyright (C) 2011 by Anyka Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

#include <linux/akuio_driver.h>
#include <mach/reg.h>
#include <mach/l2cache.h>

#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <mach/reset.h>



#define DRIVER_NAME "uio_vcodec"

#define AL5_BASE_OFFSET 0x8000
#define AL5_INTERRUPT_MASK (AL5_BASE_OFFSET + 0x14)
#define AL5_INTERRUPT (AL5_BASE_OFFSET + 0x18)

#define init_MUTEX(sem)		sema_init(sem, 1)
#define init_MUTEX_LOCKED(sem)	sema_init(sem, 0)

/* IRQs of hw codec */
const unsigned VIDEO_IRQ_MASK = (1 << IRQ_VIDEO_ENCODER);
const int MASK_BITS_NUM = sizeof(VIDEO_IRQ_MASK) * 8;

#if 0
/* platform data of this driver */
struct uio_platdata {
	struct uio_info *uioinfo;
	struct semaphore vcodec_sem;
    unsigned int open_count;
};
#endif

struct r_irq
{
	struct list_head list;
	u32 bitfield;
};


struct uio_platdata {
	struct uio_info *uioinfo;
	wait_queue_head_t irq_queue; //irq queue
	int unblock;
	struct list_head irq_masks;//iqr mask list
	spinlock_t i_lock; //lock for list
	struct kmem_cache* cache;
	unsigned int open_count;
	int irq;
	void __iomem* regs;
};

#if 0
struct al5r_codec_chan
{
  wait_queue_head_t irq_queue;
  struct list_head irq_numbers;
  int unblock;
  struct al5r_codec_desc* codec;
};


struct al5r_codec_desc
{
  struct device* device;
  void __iomem* regs;             /* Base addr for regs */
  unsigned long regs_size;        /* end addr for regs */
  struct cdev cdev;
  /* one for one mapping in the no mcu case */
  struct al5r_codec_chan* chan;
  struct list_head irq_masks;
  spinlock_t i_lock;
  struct kmem_cache* cache;
};
#endif

int channel_is_ready(struct uio_platdata* codec)
{
	unsigned long flags;
	int ret = codec->unblock;

	spin_lock_irqsave(&codec->i_lock, flags);
	ret = ret || !list_empty(&codec->irq_masks);
	spin_unlock_irqrestore(&codec->i_lock, flags);
	return ret;
}

irqreturn_t al5r_hardirq_handler(int irq, void* data)
{
	struct uio_platdata* codec = (struct uio_platdata*)data;
	u32 unmasked_irq_bitfield, irq_bitfield;
	u32 mask;
	unsigned long flags;
	struct r_irq* i_callback;
	int callback_nb;
	int i = 0;
	int al5_interrupt_nb = 20;

	mask = readl(codec->regs + AL5_INTERRUPT_MASK);
	unmasked_irq_bitfield = readl(codec->regs + AL5_INTERRUPT);
	irq_bitfield = unmasked_irq_bitfield & mask;

	if(irq_bitfield == 0)
	{
		printk("bitfield is 0\n");
		return IRQ_NONE;
	}
	writel(unmasked_irq_bitfield, codec->regs + AL5_INTERRUPT);
	readl(codec->regs + AL5_INTERRUPT);

	for(i = 0; i < al5_interrupt_nb; ++i)
	{
		callback_nb = 1U << i;

		if(irq_bitfield & callback_nb)
		{
			i_callback = kmem_cache_alloc(codec->cache, GFP_ATOMIC);

			if(!i_callback)
			{
				printk("ENOMEM: Missed interrupt\n");
				return IRQ_NONE;
			}
			i_callback->bitfield = i;
			spin_lock_irqsave(&codec->i_lock, flags);
			list_add_tail(&i_callback->list, &codec->irq_masks);
			spin_unlock_irqrestore(&codec->i_lock, flags);
		}
	}

	spin_lock_irqsave(&codec->i_lock, flags);

	//if(codec->chan)
	wake_up_interruptible(&codec->irq_queue);//wake up wait_irq

	spin_unlock_irqrestore(&codec->i_lock, flags);

  	return IRQ_HANDLED;
}


static int wait_irq(struct uio_platdata* codec, unsigned long arg)
{
	int callback;
	struct r_irq* i_callback;
	unsigned long flags;
	int ret;

	ret = wait_event_interruptible(codec->irq_queue,
	                             channel_is_ready(codec));

	if(ret == -ERESTARTSYS)
		return ret;

	if(codec->unblock)
	{
		printk("Unblocking channel\n");
		return -EINTR;
	}

	spin_lock_irqsave(&codec->i_lock, flags);
	i_callback = list_first_entry(&codec->irq_masks,
	                        struct r_irq, list);
	callback = i_callback->bitfield;
	list_del(&i_callback->list);
	kmem_cache_free(codec->cache, i_callback);
	spin_unlock_irqrestore(&codec->i_lock, flags);

	if(copy_to_user((void*)arg, &callback, sizeof(__u32)))
		return -EFAULT;

  	return ret;
}

static int unblock_channel(struct uio_platdata* codec)
{
	codec->unblock = 1;
	wake_up_interruptible(&codec->irq_queue);
	return 0;
}


/**
 * @brief     Handle hw codec irq
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  irq :  irq id
 * @param     [in]  dev_id :  platform data
 * @return    irqreturn_t :  return handle result
 * @retval    IRQ_HANDLE :  irq handled successful.
 */
 #if 0
static irqreturn_t uio_vcodec_irq_handler(int irq, void *dev_id)
{
	struct uio_platdata *pdata = dev_id;
	int i;

	for (i = 0; i < MASK_BITS_NUM; i++)
	{
		if ((1 << i) & VIDEO_IRQ_MASK)
			disable_irq_nosync(i);
	}

	up (&(pdata->vcodec_sem));

	return IRQ_HANDLED;
}
#endif
/**
 * @brief     Handle hw codec ioctl
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  uioinfo :  information of uio driver
 * @param     [in]  cmd :  ioctl request code
 * @param     [in]  arg :  argument
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int uio_vcodec_ioctl(struct uio_info *uioinfo, unsigned int cmd, unsigned long arg)
{
	struct uio_platdata *pdata = uioinfo->priv;
	int err;

	switch (cmd) {
	case AKUIO_SYSREG_WRITE:
	{
		struct akuio_sysreg_write_t reg_write;

		if (copy_from_user(&reg_write, (void __user *)arg, sizeof(struct akuio_sysreg_write_t)))
			return -EFAULT;

		sys_ctrl_reg_set(reg_write.paddr, reg_write.mask, reg_write.val);

		err = 0;
	}
	break;

	#if 0
	case AKUIO_WAIT_IRQ:
	{
		int i;

		for (i = 0; i < MASK_BITS_NUM; i++)
		{
			if ((1 << i) & VIDEO_IRQ_MASK)
				enable_irq(i);
		}
		
		down (&pdata->vcodec_sem);
		err = 0;
	}
	break;
	#endif

	case AKUIO_WAIT_IRQ:
		wait_irq(pdata, arg);
		err = 0;
		break;

	case AKUIO_UNBLOCK_CHANNEL:
    	unblock_channel(pdata);
		err = 0;
		break;

	case AKUIO_INVALIDATE_L2CACHE:
		l2cache_invalidate ();
		flush_cache_all();
		err = 0;
		break;

	case AKUIO_INVALIDATE_L1CACHE:
//		flush_cache_all();
		err = 0;
		break;

	case AKUIO_VIDEO_RESET:
		ak_soft_reset(AK_SRESET_VIDEO);
		err = 0;
		printk("%s AKUIO_VIDEO_RESET\n", __func__);
		break;

	default:
		printk(KERN_ERR "[%s] Unknow cmd: %u\n", __func__, cmd);
		err = -EINVAL;
		break;
	}

	return err;
}

/**
 * @brief     When application open the akuio device, request all irq of the hw codec and disable the irq immediately
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  uioinfo :  information of uio driver
 * @param     [in]  inode :  inode of the device
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int uio_vcodec_open(struct uio_info *uioinfo, struct inode *inode)
{
	struct uio_platdata *pdata = uioinfo->priv;
	int ret = 0;
	//int i;
    //Add code here to make sure uio0 can be multi-opened.
    if( pdata->open_count++ > 0 ) {
        DBG("uio0 has opened! open_count=%d\n", pdata->open_count );
        return 0;
    }    

	INIT_LIST_HEAD(&pdata->irq_masks);

	#if 0
	init_MUTEX_LOCKED(&(pdata->vcodec_sem));

	for (i = 0; i < MASK_BITS_NUM; i++)
	{
		if ((1 << i) & VIDEO_IRQ_MASK) {
			ret = request_irq(i, uio_vcodec_irq_handler, IRQF_DISABLED, "VIDEO HW CODEC", pdata);
			disable_irq_nosync(i);
		}
	}
	#endif
	
	ak_soft_reset(AK_SRESET_VIDEO);
	
	ret = request_irq(pdata->irq, al5r_hardirq_handler, IRQF_DISABLED, 
			"VIDEO HW CODEC", pdata);
	if(ret)
	{
		printk("Failed to request IRQ #%d -> :%d\n",pdata->irq, ret);
		return -1;
	}
	
	return 0;
}

/**
 * @brief     When application close the akuio device, free all irq of the hw codec
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  uioinfo :  information of uio driver
 * @param     [in]  inode :  inode of the device
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int uio_vcodec_release(struct uio_info *uioinfo, struct inode *inode)
{
	struct uio_platdata *pdata = uioinfo->priv;
	struct r_irq *pos, *n;
	//int i;
    //Add code here to make sure uio0 can be multi-opened.
	if(--pdata->open_count != 0) {
	    DBG("uio0 does's closed to 0! open_count=%d\n", pdata->open_count );
	    return 0;
	}	

	#if 0
	for (i = 0; i < MASK_BITS_NUM; i++)
	{
		if ((1 << i) & VIDEO_IRQ_MASK)
			free_irq(i, pdata);
	}
	#endif

	free_irq(pdata->irq, pdata);
	
	list_for_each_entry_safe(pos, n, &pdata->irq_masks, list) {
		list_del(&pos->list);
		kmem_cache_free(pdata->cache, pos);
		
	}

	return 0;
}

/**
 * @brief     Init the device which was probed
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  pdev :  the device definition
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int uio_vcodec_probe(struct platform_device *pdev)
{
	struct uio_info *uioinfo = pdev->dev.platform_data;
	struct uio_platdata *pdata;
	struct uio_mem *uiomem;
	int ret = -ENODEV;
	int i;

	if (!uioinfo || !uioinfo->name || !uioinfo->version) {
		dev_dbg(&pdev->dev, "%s: err_uioinfo\n", __func__);
		goto err_uioinfo;
	}

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = -ENOMEM;
		dev_dbg(&pdev->dev, "%s: err_alloc_pdata\n", __func__);
		goto err_alloc_pdata;
	}

	pdata->uioinfo = uioinfo;

	uiomem = &uioinfo->mem[0];

	for (i = 0; i < pdev->num_resources; ++i) {
		struct resource *r = &pdev->resource[i];
		if (r->flags != IORESOURCE_MEM)
			continue;

		if (uiomem >= &uioinfo->mem[MAX_UIO_MAPS]) {
			dev_warn(&pdev->dev, "device has more than "
					__stringify(MAX_UIO_MAPS)
					" I/O memory resources.\n");
			break;
		}

		uiomem->memtype = UIO_MEM_PHYS;
		uiomem->addr = r->start;
		uiomem->size = r->end - r->start + 1;
		pdata->regs = ioremap_nocache(uiomem->addr, uiomem->size);
		++uiomem;
		
	}

	while (uiomem < &uioinfo->mem[MAX_UIO_MAPS]) {
		uiomem->size = 0;
		++uiomem;
	}

    /* open count */
    pdata->open_count = 0;
        
	/* irq */
	pdata->uioinfo->irq = UIO_IRQ_CUSTOM;
	
	/* file handle */
	pdata->uioinfo->open = uio_vcodec_open;
	pdata->uioinfo->release = uio_vcodec_release;
	pdata->uioinfo->ioctl = uio_vcodec_ioctl;
	pdata->uioinfo->priv = pdata;

	spin_lock_init(&pdata->i_lock);
	pdata->cache = kmem_cache_create("al_codec_ram",
									 sizeof(struct r_irq),
									 0, SLAB_HWCACHE_ALIGN, NULL);
	/*ak39 video irq */
	pdata->irq = IRQ_VIDEO_ENCODER;

	init_waitqueue_head(&pdata->irq_queue);
	
	ret = uio_register_device(&pdev->dev, pdata->uioinfo);

	if (ret) {
		kfree(pdata);
err_alloc_pdata:
err_uioinfo:
		return ret;
	}

	platform_set_drvdata(pdev, pdata);

	return 0;
}

/**
 * @brief     De-init the device which will be removed
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param     [in]  pdev :  the device definition
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int uio_vcodec_remove(struct platform_device *pdev)
{
	struct uio_platdata *pdata = platform_get_drvdata(pdev);

	iounmap(pdata->regs);
	uio_unregister_device(pdata->uioinfo);

	kfree(pdata);

	return 0;
}

/* driver definition */
static struct platform_driver uio_vcodec = {
	.probe = uio_vcodec_probe,
	.remove = uio_vcodec_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

/**
 * @brief     kernel module init function, register the driver to kernel
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param
 * @return    int :  return 0 when handle successful, otherwise return negative
 * @retval     0 :  handled successful.
 * @retval    <0 :  handle failed.
 */
static int __init uio_vcodec_init(void)
{
	return platform_driver_register(&uio_vcodec);
}

/**
 * @brief     kernel module finally function, unregister the driver from kernel
 * @author    Jacky Lau
 * @date      2011-07-05
 * @param
 * @return    void
 * @retval
 */
static void __exit uio_vcodec_exit(void)
{
	platform_driver_unregister(&uio_vcodec);
}
module_init(uio_vcodec_init);
module_exit(uio_vcodec_exit);

MODULE_AUTHOR("Jacky Lau");
MODULE_DESCRIPTION("Userspace driver for anyka video hw codec");
MODULE_LICENSE("GPL v2");

