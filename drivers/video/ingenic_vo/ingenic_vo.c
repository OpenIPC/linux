/*
 * Copyright (c) 2015 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * Input file for Ingenic VO driver
 *
 * This  program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <soc/gpio.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <soc/base.h>
#include "ingenic_vo.h"

//#define DEBUG

static char *clk_name = "mpll";
#ifdef	DEBUG
static int debug_vo = 1;
static int vo_timeout_restart = 0;

#define VO_DEBUG(format, ...) { if (debug_vo) printk(format, ## __VA_ARGS__);}
#else
#define VO_DEBUG(format, ...) do{ } while(0)
#endif


struct vo_reg_struct jz_vo_regs_name[] = {
	{" VO_ADDR_VER		     ",VO_ADDR_VER          },
	{" VO_ADDR_TOP_CON	     ",VO_ADDR_TOP_CON      },
	{" VO_ADDR_TOP_STATUS    ",VO_ADDR_TOP_STATUS   },
	{ " VO_ADDR_INT_EN       ",VO_ADDR_INT_EN       },
	{ " VO_ADDR_INT_VAL      ",VO_ADDR_INT_VAL      },
	{ " VO_ADDR_INT_CLC      ",VO_ADDR_INT_CLC      },
	{ " VO_ADDR_VC0_SIZE     ",VO_ADDR_VC0_SIZE     },
	{ " VO_ADDR_VC0_STRIDE   ",VO_ADDR_VC0_STRIDE   },
	{ " VO_ADDR_VC0_Y_ADDR   ",VO_ADDR_VC0_Y_ADDR   },
	{ " VO_ADDR_VC0_UV_ADDR  ",VO_ADDR_VC0_UV_ADDR  },
	{ " VO_ADDR_VC0_BLK_INFO ",VO_ADDR_VC0_BLK_INFO },
	{ " VO_ADDR_VC0_CON_PAR  ",VO_ADDR_VC0_CON_PAR  },
	{ " VO_ADDR_VC0_VBLK_PAR ",VO_ADDR_VC0_VBLK_PAR },
	{ " VO_ADDR_VC0_HBLK_PAR ",VO_ADDR_VC0_HBLK_PAR },
	{ " VO_ADDR_VC0_TRAN_EN  ",VO_ADDR_VC0_TRAN_EN  },
	{ " VO_ADDR_VC0_TIME_OUT ",VO_ADDR_VC0_TIME_OUT },
	{ " VO_ADDR_VC0_ADDR_INFO",VO_ADDR_VC0_ADDR_INFO},
	{ " VO_ADDR_VC0_YUV_CLIP ",VO_ADDR_VC0_YUV_CLIP },
};

static int vo_dump_regs(struct jz_vo *vo)
{
	int i = 0;
	int num = 0;

	if (vo == NULL) {
		dev_err(vo->dev, "vo is NULL!\n");
		return -1;
	}
	printk("----- dump regs -----\n");
	num = sizeof(jz_vo_regs_name) / sizeof(struct vo_reg_struct);

	for (i = 0; i < num; i++) {
		printk("vo_reg: %s: \t0x%08x\r\n", jz_vo_regs_name[i].name, reg_read(vo, jz_vo_regs_name[i].addr));
	}

	return 0;

}
static void reg_bit_set(struct jz_vo *vo, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg = reg_read(vo, offset);
	reg |= bit;
	reg_write(vo, offset, reg);
}

static void reg_bit_clr(struct jz_vo *vo, int offset, unsigned int bit)
{
	unsigned int reg = 0;
	reg= reg_read(vo, offset);
	reg &= ~(bit);
	reg_write(vo, offset, reg);
}

static int soft_reset_vo(struct jz_vo *vo)
{
//1. Soft reset.
    __reset_vo_dma();
    while ((reg_read(vo, VO_ADDR_TOP_STATUS) & 0x1) != 1);
	__reset_vo();
	mdelay(5);
    __release_reset_vo();
	__release_vo_dma();

	return 0;
}
static int work_init_vo(struct jz_vo *vo, struct vo_param *vo_param)
{
	clk_prepare_enable(vo->clk);
	clk_prepare_enable(vo->clk_div);
	clk_prepare_enable(vo->ahb0_gate);
	soft_reset_vo(vo);

//2. Config par: SIZE, STRIDE, CON_PAR, VBLK_PAR, HBLK_PAR.  1920*1080
    reg_write(vo,VO_ADDR_VC0_SIZE, (vo_param->src_h<<16 | vo_param->src_w));
	VO_DEBUG("VO_ADDR_VC0_SIZE : %08x\n",reg_read(vo,VO_ADDR_VC0_SIZE));
    reg_write(vo,VO_ADDR_VC0_STRIDE, (vo_param->src_uv_strid<<16 | vo_param->src_y_strid));
    reg_write(vo,VO_ADDR_VC0_CON_PAR, (vo_param->line_full_thr<<16 | 1<<12 | vo_param->bt_mode));  //12bit 0:display  1:transmit
    reg_write(vo,VO_ADDR_VC0_VBLK_PAR, (vo_param->vbb_num<<16 | vo_param->vfb_num));
	reg_write(vo,VO_ADDR_VC0_HBLK_PAR, vo_param->hfb_num);
	vo_dump_regs(vo);

//3. Optional config par: INT_EN, BLK_INFO, TIME_OUT.
	reg_write(vo,VO_ADDR_INT_EN, 1 << 12 | 1 << 8 | 1);
	
	//vo_dump_regs(vo);

	return 0;
}

static int vo_reg_set(struct jz_vo *vo, struct vo_param *vo_param)
{
// 5. Set trans enable(TRAN_EN bit0) to start working.
	reg_write(vo, VO_ADDR_VC0_TRAN_EN, 0x00000001);
// 4.Input image addr
   	while(reg_read(vo, VO_ADDR_VC0_ADDR_INFO) & (1 << 3));
	while((reg_read(vo,VO_ADDR_VC0_ADDR_INFO) & 0x8)==0){
		reg_write(vo,VO_ADDR_TOP_STATUS,0x00000001);
		reg_write(vo, VO_ADDR_VC0_Y_ADDR, vo_param->src_addr_y);
		reg_write(vo, VO_ADDR_VC0_UV_ADDR, vo_param->src_addr_uv);
	}
	return 0;
}

static int vo_stop(struct jz_vo *vo, struct vo_param *vo_param)
{
    __reset_vo_dma();
    while((reg_read(vo, VO_ADDR_TOP_STATUS) & 0x1) == 1);
    __reset_vo();
    mdelay(5);
    __release_reset_vo();
	__release_vo_dma();

    return 0;
}

static int vo_start(struct jz_vo *vo, struct vo_param *vo_param)
{
	int ret = 0;
	struct vo_param *ip = vo_param;
	if ((vo == NULL) || (vo_param == NULL)) {
		dev_err(vo->dev, "vo: vo is NULL or vo_param is NULL\n");
		return -1;
	}

   vo_reg_set(vo, ip);

#ifdef DEBUG
    vo_dump_regs(vo);
#endif
    VO_DEBUG("vo_start\n");
	mutex_lock(&vo->irq_mutex);
	ret = wait_for_completion_interruptible_timeout(&vo->done_vo, msecs_to_jiffies(20000));
	if (ret < 0) {
		printk("vo: done_vo wait_for_completion_interruptible_timeout err %d\n", ret);
	    mutex_unlock(&vo->irq_mutex);
		goto err_vo_wait_for_done;
	} else if (ret == 0) {
		ret = -1;
		printk("vo: done_vo wait_for_completion_interruptible_timeout timeout %d\n", ret);
		vo_dump_regs(vo);
	    mutex_unlock(&vo->irq_mutex);
		goto err_vo_wait_for_done;
	} else {
		;
	}
	mutex_unlock(&vo->irq_mutex);
    VO_DEBUG("vo: exit vo_start %d\n", current->pid);
    return 0;

err_vo_wait_for_done:
	clk_disable_unprepare(vo->ahb0_gate);

	return ret;

}

static long vo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct vo_param iparam;
	struct miscdevice *dev = filp->private_data;
	struct jz_vo *vo = container_of(dev, struct jz_vo, misc_dev);

    int ret = -1;

    if (_IOC_TYPE(cmd) != JZVO_IOC_MAGIC) {
		dev_err(vo->dev, "invalid cmd!\n");
		return -EFAULT;
	}

	mutex_lock(&vo->mutex);
	switch (cmd) {
		case IOCTL_VO_INIT:
			printk("vo: init\n");
			if (copy_from_user(&iparam, (void *)arg, sizeof(struct vo_param)))
			{
				dev_err(vo->dev, "IOCTL_VO_START:copy_from_user error!!!\n");
				ret = -EFAULT;
				break;
			}
			ret = work_init_vo(vo, &iparam);
			if (ret < 0) {
				printk("vo: error vo stop ret = %d\n", ret);
			}
	//		printk("src_w:%d src_h:%d.bt_mode:%d,line_full_thr:%d,vfb_num:%d,vbb_num:%d,hfb_num:%d,src_addr_y:%d,src_addr_uv:%d,src_y_strid:%d,src_addr_uv:%d,\n",iparam.src_w,iparam.src_h,iparam.bt_mode,iparam.line_full_thr,iparam.vfb_num,iparam.vbb_num,iparam.hfb_num,iparam.src_addr_y,iparam.src_addr_uv,iparam.src_y_strid,iparam.src_uv_strid);
            break;

		case IOCTL_VO_START:
			if (copy_from_user(&iparam, (void *)arg, sizeof(struct vo_param))) {
				dev_err(vo->dev, "copy_from_user error!!!\n");
				ret = -EFAULT;
				break;
			}
			ret = vo_start(vo, &iparam);
			if (ret < 0) {
				printk("vo: error vo start ret = %d\n", ret);
			}
			break;
		case IOCTL_VO_END:
			ret = vo_stop(vo, &iparam);
			if (ret < 0) {
				printk("vo: error vo stop ret = %d\n", ret);
			}
            break;
		case IOCTL_VO_BUF_FLUSH_CACHE:
			{
				struct vo_flush_cache_para fc;
				if (copy_from_user(&fc, (void *)arg, sizeof(fc))) {
					dev_err(vo->dev, "copy_from_user error!!!\n");
					ret = -EFAULT;
					break;
				}
				dma_cache_sync(NULL, fc.addr, fc.size, DMA_BIDIRECTIONAL);
			}
			break;
	    default:
			dev_err(vo->dev, "invalid command: 0x%08x\n", cmd);
			ret = -EINVAL;
	}

	mutex_unlock(&vo->mutex);
	return ret;
}

static int vo_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_vo *vo = container_of(dev, struct jz_vo, misc_dev);

	VO_DEBUG("vo: %s pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&vo->mutex);

	mutex_unlock(&vo->mutex);
	return ret;
}

static int vo_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	struct miscdevice *dev = filp->private_data;
	struct jz_vo *vo = container_of(dev, struct jz_vo, misc_dev);

	VO_DEBUG("vo: %s  pid: %d, tgid: %d filp: %p\n",
			__func__, current->pid, current->tgid, filp);
	mutex_lock(&vo->mutex);

	mutex_unlock(&vo->mutex);
	return ret;
}

static struct file_operations vo_ops = {
	.owner = THIS_MODULE,
	.open = vo_open,
	.release = vo_release,
	.unlocked_ioctl = vo_ioctl,
};


static irqreturn_t vo_irq_handler(int irq, void *data)
{
	struct jz_vo *vo;
	int status;
	vo = (struct jz_vo *)data;
    status = reg_read(vo, VO_ADDR_INT_VAL);

   	VO_DEBUG("----- %s, status= 0x%08x\n", __func__, status);

    if((reg_read(vo, VO_ADDR_INT_VAL) & (1 << 12 | 1 << 8 | 1)) ){
		complete(&vo->done_vo);
	}

    __vo_irq_clear(status);
    __release_vo_irq_clear(status);

	return IRQ_HANDLED;
}

static int vo_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_vo *vo;
    int rc = 0;

	VO_DEBUG("%s\n", __func__);
	vo = (struct jz_vo *)devm_kzalloc(&pdev->dev, sizeof(struct jz_vo), GFP_KERNEL);
	if (!vo) {
		dev_err(&pdev->dev, "alloc jz_vo failed!\n");
		return -ENOMEM;
	}
	sprintf(vo->name, "vo");
	vo->misc_dev.name = vo->name;
	vo->misc_dev.fops = &vo_ops;
	vo->dev = &pdev->dev;

	mutex_init(&vo->mutex);
	mutex_init(&vo->irq_mutex);
	init_completion(&vo->done_vo);

	vo->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!vo->res) {
		dev_err(&pdev->dev, "failed to get dev resources: %d\n", ret);
		ret = -EINVAL;
	}

    pdev->id = of_alias_get_id(pdev->dev.of_node, "vo");

    vo->iomem = devm_ioremap_resource(&pdev->dev, vo->res);
    if (IS_ERR(vo->iomem)) {
        dev_err(&pdev->dev, "failed to map io baseaddress. \n");
        ret = -ENODEV;
        goto err_ioremap;
    }

	vo->irq = platform_get_irq(pdev, 0);
	if (devm_request_irq(&pdev->dev, vo->irq, vo_irq_handler, IRQF_SHARED, vo->name, vo)) {
		dev_err(&pdev->dev, "request irq failed\n");
		ret = -EINVAL;
		goto err_req_irq;
	}
	
	vo->clk_div = devm_clk_get(vo->dev, "div_vo");
	if (IS_ERR(vo->clk)) {
		dev_err(&pdev->dev, "vo clk get failed!\n");
		goto err_get_vo_clk;
	}

	vo->ahb0_gate = devm_clk_get(vo->dev, "gate_ahb0");
	if (IS_ERR(vo->ahb0_gate)) {
		dev_err(&pdev->dev, "vo clk get failed!\n");
		ret = -EINVAL;
        goto err_get_ahb_clk;
	}
//	vo->clk_mux = devm_clk_get(vo->dev, "mux_vo");
//	if (IS_ERR(vo->clk)) {
//		dev_err(&pdev->dev, "vo clk get failed!\n");
//		goto err_get_vo_clk;
//	}

	ret = clk_set_parent(vo->clk_mux, clk_get(NULL, clk_name));
    if (ret){
        printk("clk_set_parent failed!!! parent name = %s\n", clk_name);
    }
    vo->clk = devm_clk_get(vo->dev, "gate_vo");
	if (IS_ERR(vo->clk)) {
		dev_err(&pdev->dev, "vo clk get failed!\n");
		goto err_get_vo_clk;
	}
	clk_set_rate(vo->clk_div, 54000000);

    
	if ((rc = clk_prepare_enable(vo->clk)) < 0) {
		dev_err(&pdev->dev, "Enable vo gate clk failed\n");
		goto err_prepare_vo_clk;
	}
	if ((rc = clk_prepare_enable(vo->ahb0_gate)) < 0) {
		dev_err(&pdev->dev, "Enable vo ahb0 clk failed\n");
		goto err_prepare_ahb_clk;
	}
    dev_set_drvdata(&pdev->dev, vo);

    ret = misc_register(&vo->misc_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "register misc device failed!\n");
		goto err_set_drvdata;
	}

	return 0;

err_set_drvdata:
	devm_free_irq(&pdev->dev, vo->irq, vo);
err_get_vo_clk:
	devm_clk_put(&pdev->dev, vo->clk);
	devm_clk_put(&pdev->dev, vo->clk_div);
	devm_clk_put(&pdev->dev, vo->ahb0_gate);
err_get_ahb_clk:
	devm_clk_put(&pdev->dev, vo->ahb0_gate);
err_prepare_vo_clk:
	clk_disable_unprepare(vo->clk);
err_prepare_ahb_clk:
	clk_disable_unprepare(vo->ahb0_gate);
err_req_irq:
    devm_free_irq(&pdev->dev, vo->irq, vo);
err_ioremap:
	devm_iounmap(&pdev->dev, vo->iomem);
	kfree(vo);

	return ret;
}

static int vo_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct jz_vo *vo;
	VO_DEBUG("%s\n", __func__);

	vo = dev_get_drvdata(&pdev->dev);

    free_irq(vo->irq, vo);

    devm_iounmap(&pdev->dev, vo->iomem);

	misc_deregister(&vo->misc_dev);
	if (ret < 0) {
		dev_err(vo->dev, "misc_deregister error %d\n", ret);
		return ret;
	}

    if (vo->pbuf.vaddr_alloc) {
		kfree((void *)(vo->pbuf.vaddr_alloc));
		vo->pbuf.vaddr_alloc = 0;
	}

    clk_disable_unprepare(vo->clk);
    clk_disable_unprepare(vo->clk_div);

	clk_disable_unprepare(vo->ahb0_gate);

	if (vo) {
		kfree(vo);
	}

	return 0;
}

static const struct of_device_id ingenic_vo_dt_match[] = {
	{ .compatible = "ingenic,t40-vo", .data = NULL },
	{ .compatible = "ingenic,t41-vo", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, ingenic_vo_dt_match);

static struct platform_driver jz_vo_driver = {
	.probe	= vo_probe,
	.remove = vo_remove,
	.driver = {
		.name = "jz-vo",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(ingenic_vo_dt_match),
	},
};

module_platform_driver(jz_vo_driver)

MODULE_DESCRIPTION("JZ VO driver");
MODULE_LICENSE("GPL");
