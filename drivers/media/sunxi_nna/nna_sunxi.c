#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <asm/uaccess.h>

#include <asm/io.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/time.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/nna_driver.h>
#include "nna_driver_i.h"

//#define NNA_DEBUG_IRQ
#define TIMEOUT_MS 3000
#define PRINTK_IOMMU_ADDR 0

static nna_context *nna_priv;

static struct attribute *nna_attributes[] = {
	//&dev_attr_debug.attr,
	//&dev_attr_func_runtime.attr,
	NULL
};

static struct attribute_group nna_attribute_group = {
	.name = "attr",
	.attrs = nna_attributes
};

static const struct of_device_id sunxi_nna_match[] = {
	{.compatible = "allwinner,sunxi-nna",},
	{},
};

int reg2status(int val)
{
	int ret = 0;

	switch (val) {
	case 0x150001:
	case 0x140001:
		ret = CONV_ACT_SIG;
		break;
	case 0x150011:
	case 0x100011:
		ret = CONV_ACT_POOL_SIG;
		break;
	case 0x150000:
		ret = CONV_SIG;
		break;
	case 0x1:
		ret = ELTWISE_OR_PRELU_SIG;
		break;
	case 0x11:
		ret = PRELU_POOL_SIG;
		break;
	case 0x10:
		ret = POOL_SIG;
		break;
	default:
		break;
	}
	return ret;
}

static irqreturn_t nna_interrupt(int irq, void *dev_id)
{

	__u32 ret = readl(nna_priv->io + OFFSET_NNA_GLB_S_INTR_STATUS_0);

	writel(ret, nna_priv->io + OFFSET_NNA_GLB_S_INTR_STATUS_0);

	nna_priv->nna_irq_status |= ret;
	wake_up(&nna_priv->nna_queue);
	return IRQ_HANDLED;
}


long nna_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	__s32 ret = 0;
	__s32 restime = 0;
	int i;
	struct sg_table *sgt, *sgt_bak;
	struct scatterlist *sgl, *sgl_bak;
	struct user_dma_param user_data;
	struct nna_dma_buf *nna_buf = NULL;

#if defined(NNA_DEBUG_IRQ)
	struct timeval time_start, time_end;
	unsigned int runtime;

	do_gettimeofday(&time_start);
#endif

	mutex_lock(&nna_priv->mutex);

	switch (cmd) {
	case NNA_CMD_ENABLE_IRQ:{
		NNA_INFO("enable nna  irq\n");
		enable_irq(nna_priv->irq);
		break;
	}
	case NNA_CMD_DISABLE_IRQ:{
		NNA_INFO("disable nna  irq\n");
		disable_irq(nna_priv->irq);
		break;
	}
	case NNA_CMD_SET_FREQ:{
		if (arg > NNA_CLOCK_1200M) {
			NNA_WARN("freq set too large:%ld", arg);
			break;
		}
		NNA_INFO("old clk freq:%ld\n", clk_get_rate(nna_priv->clk));
		clk_set_rate(nna_priv->clk, arg*1000000);
		sunxi_periph_reset_assert(nna_priv->clk);
		sunxi_periph_reset_deassert(nna_priv->clk);
		NNA_INFO("new clk freq:%ld\n", clk_get_rate(nna_priv->clk));
		break;
	}
	case NNA_CMD_RESET_NNA:{
		sunxi_periph_reset_assert(nna_priv->clk_rst);
		sunxi_periph_reset_deassert(nna_priv->clk_rst);
		break;
	}
	case NNA_CMD_QUERY:{
		restime = wait_event_timeout(nna_priv->nna_queue,
				     arg == (arg & nna_priv->nna_irq_status),
				     msecs_to_jiffies(TIMEOUT_MS));
		if (restime <= 0) {
			NNA_ERR("wait status timeout arg 0x%lx, status 0x%x\n", arg, nna_priv->nna_irq_status);
			ret = -1;
			nna_priv->nna_irq_status = 0;
		} else {
			nna_priv->nna_irq_status = 0;
			ret = nna_priv->nna_irq_status;
		}
		break;
	}
	case NNA_CMD_MAP_DMA_FD: {
		nna_buf = (struct nna_dma_buf *)kmalloc(sizeof(struct nna_dma_buf), GFP_KERNEL);
		if (nna_buf == NULL) {
			NNA_ERR("malloc nna_dma_buf failed\n");
			return -EFAULT;
		}

		if (copy_from_user(&user_data, (void __user *)arg,
			sizeof(struct user_dma_param))) {
			NNA_ERR("IOCTL_GET_IOMMU_ADDR copy_from_user error");
			return -EFAULT;
		}

		nna_buf->fd = user_data.fd;
		nna_buf->dma_buf = dma_buf_get(nna_buf->fd);
		if (nna_buf->dma_buf < 0) {
			NNA_ERR("ve get dma_buf error");
			return -EFAULT;
		}

		nna_buf->attachment = dma_buf_attach(nna_buf->dma_buf, nna_priv->platform_dev);
		if (nna_buf->attachment < 0) {
			NNA_ERR("ve get dma_buf_attachment error");
			goto RELEASE_DMA_BUF;
		}

		sgt = dma_buf_map_attachment(nna_buf->attachment, DMA_BIDIRECTIONAL);

		sgt_bak = kmalloc(sizeof(struct sg_table), GFP_KERNEL | __GFP_ZERO);
		if (sgt_bak == NULL)
			NNA_ERR("alloc sgt fail\n");

		if (sg_alloc_table(sgt_bak, sgt->nents, GFP_KERNEL) != 0)
			NNA_ERR("alloc table fail\n");

		sgl_bak = sgt_bak->sgl;
		for_each_sg(sgt->sgl, sgl, sgt->nents, i)  {
			sg_set_page(sgl_bak, sg_page(sgl), sgl->length, sgl->offset);
			sgl_bak = sg_next(sgl_bak);
		}

		nna_buf->sgt = sgt_bak;
		if (nna_buf->sgt < 0) {
			NNA_ERR("get sg_table error\n");
			goto RELEASE_DMA_BUF;
		}

		ret = dma_map_sg(nna_priv->platform_dev, nna_buf->sgt->sgl, nna_buf->sgt->nents,
						DMA_BIDIRECTIONAL);
		if (ret != 1) {
			NNA_ERR("dma_map_sg error\n");
			goto RELEASE_DMA_BUF;
		}

		nna_buf->phy_addr = sg_dma_address(nna_buf->sgt->sgl);
		user_data.phy_addr = (unsigned int)(nna_buf->phy_addr & 0xffffffff);

		if (copy_to_user((void __user *)arg, &user_data, sizeof(struct user_dma_param))) {
			NNA_ERR("get iommu copy_to_user error\n");
			goto RELEASE_DMA_BUF;
		}

		nna_buf->p_id = current->tgid;
		#if PRINTK_IOMMU_ADDR
		NNA_INFO("fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
				nna_buf->fd,
				nna_buf->iommu_addr,
				nna_buf->dma_buf,
				nna_buf->attachment,
				nna_buf->sgt,
				nna_buf->sgt->nents,
				nna_buf->p_id);
		#endif
		ret = 0;
		aw_mem_list_add_tail(&nna_buf->i_list, &nna_priv->list);
		break;

RELEASE_DMA_BUF:
		if (nna_buf->dma_buf > 0) {
			if (nna_buf->attachment > 0) {
				if (nna_buf->sgt > 0) {
					dma_unmap_sg(nna_priv->platform_dev, nna_buf->sgt->sgl, nna_buf->sgt->nents,
								DMA_BIDIRECTIONAL);
					dma_buf_unmap_attachment(nna_buf->attachment, nna_buf->sgt, DMA_BIDIRECTIONAL);
					sg_free_table(nna_buf->sgt);
					kfree(nna_buf->sgt);
				}

				dma_buf_detach(nna_buf->dma_buf, nna_buf->attachment);
			}
			dma_buf_put(nna_buf->dma_buf);
		}
		kfree(nna_buf);
		ret = -EINVAL;
		break;
	}
	case NNA_CMD_UNMAP_DMA_FD:
	{
		if (copy_from_user(&user_data, (void __user *)arg,
			sizeof(struct user_dma_param))) {
			NNA_ERR("copy_from_user error");
			return -EFAULT;
		}
		aw_mem_list_for_each_entry(nna_buf, &nna_priv->list, i_list) {
			if (nna_buf->fd == user_data.fd && nna_buf->p_id == current->tgid) {
				#if PRINTK_IOMMU_ADDR
				NNA_INFO("free: fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p nets:%d, pid:%d\n",
				nna_buf->fd,
				nna_buf->iommu_addr,
				nna_buf->dma_buf,
				nna_buf->attachment,
				nna_buf->sgt,
				nna_buf->sgt->nents,
				nna_buf->p_id);
				#endif
				if (nna_buf->dma_buf > 0) {
					if (nna_buf->attachment > 0) {
						if (nna_buf->sgt > 0) {
							dma_unmap_sg(nna_priv->platform_dev, nna_buf->sgt->sgl, nna_buf->sgt->nents,
										DMA_BIDIRECTIONAL);
							dma_buf_unmap_attachment(nna_buf->attachment, nna_buf->sgt,
													DMA_BIDIRECTIONAL);
							sg_free_table(nna_buf->sgt);
							kfree(nna_buf->sgt);
						}
						dma_buf_detach(nna_buf->dma_buf, nna_buf->attachment);
					}
					dma_buf_put(nna_buf->dma_buf);
				}
				aw_mem_list_del(&nna_buf->i_list);
				kfree(nna_buf);
				break;
			}
		}
		break;
	}
	/* Invalid IOCTL call */
	default:
		ret = -EINVAL;
	}

	mutex_unlock(&nna_priv->mutex);

#if defined(NNA_DEBUG_IRQ)
	do_gettimeofday(&time_end);
	runtime = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
			  (time_end.tv_usec - time_start.tv_usec);
	NNA_ERR("%s:use %u us!\n", __func__, runtime);
#endif

	return ret;
}

static int nna_open(struct inode *inode, struct file *file)
{
	mutex_lock(&nna_priv->mutex);
	if (nna_priv->refs_count > 0) {
		NNA_INFO("open device refs_count:%d", nna_priv->refs_count);
		nna_priv->refs_count++;
		mutex_unlock(&nna_priv->mutex);
		return 0;
	}
	NNA_INFO("nna_open irqnum = %d\n", nna_priv->irq);
	AW_MEM_INIT_LIST_HEAD(&nna_priv->list);
	clk_prepare_enable(nna_priv->clk);
	nna_priv->refs_count++;
	mutex_unlock(&nna_priv->mutex);
	return 0;
}

static int nna_release(struct inode *inode, struct file *file)
{
	struct aw_mem_list_head *pos, *q;
	struct nna_dma_buf *nna_buf;

	mutex_lock(&nna_priv->mutex);
	if (nna_priv->refs_count > 1) {
		nna_priv->refs_count--;
		NNA_INFO("close device refs_count:%d", nna_priv->refs_count);
		mutex_unlock(&nna_priv->mutex);
		return 0;
	}
	NNA_INFO("nna_release irqnum = %d\n", nna_priv->irq);
	clk_disable_unprepare(nna_priv->clk);
	aw_mem_list_for_each_safe(pos, q, &nna_priv->list) {
	nna_buf = aw_mem_list_entry(pos, struct nna_dma_buf, i_list);
		if (nna_buf->p_id == current->tgid) {
		   #if PRINTK_IOMMU_ADDR
			NNA_INFO("free: fd:%d, iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, sg_table:%p nets:%d, pid:%d\n",
			nna_buf->fd,
			nna_buf->iommu_addr,
			nna_buf->dma_buf,
			nna_buf->attachment,
			nna_buf->sgt,
			nna_buf->sgt->nents,
			nna_buf->p_id);
		    #endif
			if (nna_buf->dma_buf > 0) {
				if (nna_buf->attachment > 0) {
					if (nna_buf->sgt > 0) {
						dma_unmap_sg(nna_priv->platform_dev, nna_buf->sgt->sgl, nna_buf->sgt->nents,
									DMA_BIDIRECTIONAL);
						dma_buf_unmap_attachment(nna_buf->attachment, nna_buf->sgt, DMA_BIDIRECTIONAL);
						sg_free_table(nna_buf->sgt);
						kfree(nna_buf->sgt);
					}
					dma_buf_detach(nna_buf->dma_buf, nna_buf->attachment);
				}
				dma_buf_put(nna_buf->dma_buf);
			}
			aw_mem_list_del(&nna_buf->i_list);
			kfree(nna_buf);
		}
	}
	nna_priv->refs_count--;
	mutex_unlock(&nna_priv->mutex);
	return 0;
}

static const struct file_operations nna_fops = {
	.owner = THIS_MODULE,
	.open = nna_open,
	.release = nna_release,
	.unlocked_ioctl = nna_ioctl,
};

static int nna_int(nna_context *devp)
{
	alloc_chrdev_region(&devp->devid, 0, 1, "nna_chrdev");

	devp->nna_cdev = cdev_alloc();
	if (devp->nna_cdev == NULL) {
		NNA_ERR("cdev alloc error!\n");
		return -1;
	}

	cdev_init(devp->nna_cdev, &nna_fops);
	devp->nna_cdev->owner = THIS_MODULE;

	if (cdev_add(devp->nna_cdev, devp->devid, 1) != 0) {
		NNA_ERR("major number %d but add failed.\n", MAJOR(devp->devid));
		return -1;
	}

	devp->nna_class = class_create(THIS_MODULE, "nna");

	if (IS_ERR(devp->nna_class)) {
		NNA_ERR("create class error\n");
		return -1;
	}

	devp->nna_dev = device_create(devp->nna_class, NULL, devp->devid, NULL, "nna");

	if (IS_ERR(devp->nna_dev)) {
		NNA_ERR("create device error\n");
		return -1;
	}

	if (sysfs_create_group(&devp->nna_dev->kobj, &nna_attribute_group) < 0) {
		NNA_ERR("sysfs_create_file fail!\n");
		return -1;
	}
	return 0;
}

static int nna_deint(nna_context *devp)
{
	sysfs_remove_group(&devp->nna_dev->kobj, &nna_attribute_group);
	device_destroy(devp->nna_class, devp->devid);
	class_destroy(devp->nna_class);
	cdev_del(devp->nna_cdev);
	unregister_chrdev_region(devp->devid, 1);
	return 0;
}


static int nna_probe(struct platform_device *pdev)
{

	int size;
	struct resource *res;
	int ret = 0;

	NNA_INFO("start nna_probe\n");

	nna_priv = kmalloc(sizeof(nna_context), GFP_KERNEL);

	if (nna_priv == NULL) {
		NNA_ERR("malloc mem for nna device err!\n");
		return -ENOMEM;
	}
	memset(nna_priv, 0, sizeof(nna_context));

	if (nna_int(nna_priv) != 0) {
		NNA_ERR("nna init failed!\n");
		goto EXIT;
	}

	nna_priv->platform_dev = &pdev->dev;

	/*get the register addr*/
	nna_priv->io = of_iomap(pdev->dev.of_node, 0);
	if (nna_priv->io == NULL) {
		NNA_WARN("device tree not support io reigster,we try the ohter way.\n");
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (res == NULL) {
			NNA_ERR("failed to get memory register\n");
			ret = -ENXIO;
			goto EXIT;
		}

		size = (res->end - res->start) + 1;
		/* map the memory */
		nna_priv->io = ioremap(res->start, size);
		if (nna_priv->io == NULL) {
			NNA_ERR("we can't map the register addr\n");
			ret = -ENXIO;
			goto EXIT;
		}
	}

	/* get the irq */
	nna_priv->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!nna_priv->irq) {
		NNA_WARN("device tree not support irq,we try the ohter way.\n");
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (res == NULL) {
			NNA_ERR("failed to get irq resource\n");
			ret = -ENXIO;
			goto EXIT;
		}
		nna_priv->irq = res->start;
	}
	NNA_INFO("get irq num: %d\n", nna_priv->irq);

	/*get the clk*/
	nna_priv->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(nna_priv->clk)) {
		NNA_ERR("fail to get clk\n");
		goto EXIT;
	}
	nna_priv->clk_rst = of_clk_get(pdev->dev.of_node, 1);
	if (IS_ERR(nna_priv->clk)) {
		NNA_ERR("fail to get clk\n");
		goto EXIT;
	}

	/* request the irq */
	ret = request_threaded_irq(nna_priv->irq, NULL, nna_interrupt,
							   IRQF_EARLY_RESUME | IRQF_ONESHOT, dev_name(&pdev->dev), &nna_priv->devid);
	if (ret) {
		NNA_ERR("failed to install irq resource\n");
		goto EXIT;
	}

	disable_irq(nna_priv->irq);

	/*mutex init*/
	mutex_init(&nna_priv->mutex);
	init_waitqueue_head(&nna_priv->nna_queue);
	nna_priv->nna_irq_status = 0;
	return 0;

EXIT:
	if (nna_priv->clk != NULL)
		clk_put(nna_priv->clk);
	if (nna_priv->io != NULL)
		iounmap(nna_priv->io);
	if (nna_priv != NULL)
		kfree(nna_priv);

	return ret;
}

static int nna_remove(struct platform_device *pdev)
{

	NNA_INFO("nna_remove\n");
	mutex_destroy(&nna_priv->mutex);
	free_irq(nna_priv->irq, &nna_priv->devid);
	clk_put(nna_priv->clk);
	iounmap(nna_priv->io);

	nna_deint(nna_priv);
	kfree(nna_priv);

	return 0;
}


static int nna_suspend(struct platform_device *pdev, pm_message_t state)
{
	NNA_INFO("nna_suspend\n");

	mutex_lock(&nna_priv->mutex);

	enable_irq_wake(nna_priv->irq);

	mutex_unlock(&nna_priv->mutex);

	return 0;
}

static int nna_resume(struct platform_device *pdev)
{

	NNA_INFO("nna_resume\n");

	mutex_lock(&nna_priv->mutex);

	disable_irq_wake(nna_priv->irq);

	mutex_unlock(&nna_priv->mutex);

	return 0;
}

static struct platform_driver nna_driver = {
	.probe	 = nna_probe,
	.remove  = nna_remove,
	.suspend = nna_suspend,
	.resume  = nna_resume,
	.driver  = {
		.name  = "nna",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_nna_match,
	},
};

int __init nna_module_init(void)
{
	int ret;

	NNA_INFO("nna module init.\n");
	ret = platform_driver_register(&nna_driver);

	return ret;
}

static void __exit nna_module_exit(void)
{
	NNA_INFO("nna_module_exit\n");
	platform_driver_unregister(&nna_driver);
}

module_init(nna_module_init);
module_exit(nna_module_exit);

MODULE_AUTHOR("jilinglin");
MODULE_AUTHOR("jilinglin<jilinglin@allwinnertech.com>");
MODULE_DESCRIPTION("nna driver");
MODULE_LICENSE("GPL");
