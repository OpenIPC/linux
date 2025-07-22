#include <linux/module.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <asm/delay.h>
//#include <mach/jzcpm_pwc.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include "../soc_vpu.h"
#include "helix.h"
//#include "helix_x264_enc.h"

//#define DUMP_HELIX_REG

struct jz_vpu_helix {
	struct vpu          vpu;
	char                name[16];
	int                 irq;
	void __iomem        *iomem;
	struct clk          *clk;
	struct clk          *clk_gate;
	struct clk          *ahb1_gate;
	enum jz_vpu_status  vpu_status;
	struct completion   done;
	spinlock_t          slock;
	struct mutex        mutex;
	pid_t               owner_pid;
	unsigned int        status;
	unsigned int        bslen;
	void*               cpm_pwc;
	unsigned int        cmpx;
};

static long vpu_reset(struct device *dev)
{
	struct jz_vpu_helix *vpu = dev_get_drvdata(dev);
	int timeout = 0x7fff;
	unsigned int srbc = cpm_inl(CPM_SRBC);

	cpm_set_bit(CPM_HELIX_STP(vpu->vpu.idx), CPM_SRBC);
	while (!(cpm_inl(CPM_SRBC) & (1 << CPM_HELIX_ACK(vpu->vpu.idx))) && --timeout) udelay(20);

	if (timeout == 0) {
		dev_warn(vpu->vpu.dev, "[%d:%d] wait stop ack timeout\n",
			 current->tgid, current->pid);
		cpm_outl(srbc, CPM_SRBC);
		return -1;
	} else {
		cpm_outl(srbc | (1 << CPM_HELIX_SR(vpu->vpu.idx)), CPM_SRBC);
		cpm_outl(srbc, CPM_SRBC);
	}

	return 0;
}

static long vpu_open(struct device *dev)
{
	struct jz_vpu_helix *vpu = dev_get_drvdata(dev);
	unsigned long slock_flag = 0;

	spin_lock_irqsave(&vpu->slock, slock_flag);
	if (cpm_inl(CPM_OPCR) & OPCR_IDLE)
		return -EBUSY;

#ifndef CONFIG_FPGA_TEST
	clk_enable(vpu->clk);
	clk_enable(vpu->ahb1_gate);
	clk_enable(vpu->clk_gate);
#else
	void *ivdc_iomap = ioremap(0x10000000, 0x1000);
	int value;
	value = readl(ivdc_iomap+0x28);
	writel((value & 0xfffffff6), ivdc_iomap+0x28);
	iounmap(ivdc_iomap);
#endif
	//cpm_pwc_enable(vpu->cpm_pwc);

	__asm__ __volatile__ (
			"mfc0  $2, $16,  7   \n\t"
			"ori   $2, $2, 0x340 \n\t"
			"andi  $2, $2, 0x3ff \n\t"
			"mtc0  $2, $16,  7  \n\t"
			"nop                  \n\t");

	vpu_reset(dev);
	enable_irq(vpu->irq);
	vpu->vpu_status = VPU_STATUS_OPEN;
	spin_unlock_irqrestore(&vpu->slock, slock_flag);
	while(try_wait_for_completion(&vpu->done));

	dev_dbg(dev, "[%d:%d] open\n", current->tgid, current->pid);

	return 0;
}

static long vpu_release(struct device *dev)
{
	struct jz_vpu_helix *vpu = dev_get_drvdata(dev);
	unsigned long slock_flag = 0;

	spin_lock_irqsave(&vpu->slock, slock_flag);
	disable_irq_nosync(vpu->irq);

	__asm__ __volatile__ (
			"mfc0  $2, $16,  7   \n\t"
			"andi  $2, $2, 0xbf \n\t"
			"mtc0  $2, $16,  7  \n\t"
			"nop                  \n\t");

	cpm_clear_bit(CPM_HELIX_SR(vpu->vpu.idx),CPM_SRBC);

#ifndef CONFIG_FPGA_TEST
    clk_disable(vpu->clk);
	clk_disable(vpu->clk_gate);
	clk_disable(vpu->ahb1_gate);
#endif
	//cpm_pwc_disable(vpu->cpm_pwc);
	/* Clear completion use_count here to avoid a unhandled irq after vpu off */
	vpu->done.done = 0;
	vpu->vpu_status = VPU_STATUS_CLOSE;
	spin_unlock_irqrestore(&vpu->slock, slock_flag);

	dev_dbg(dev, "[%d:%d] close\n", current->tgid, current->pid);

	return 0;
}

#ifdef DUMP_HELIX_REG
void dump_helix_state(struct jz_vpu_helix *vpu, unsigned int base, unsigned int len)
{
	unsigned int i;
	for(i = 0; i <= len; i++) {
		printk("addr:0x%x, state:0x%x\n", base + (i << 2), vpu_readl(vpu, base + (i << 2)));
	}
}

void helix_show_internal_state(struct jz_vpu_helix *vpu)
{
	unsigned int cfgc_base = 0;
	unsigned int vdma_base = 0x10000;
	unsigned int efe_base = 0x40000;
	unsigned int vmau_base = 0x80000;
	unsigned int sde_base = 0x90000;

	printk("----------------helix_show_internal_state start------------\n");
	//CFGC
	printk("-----------CFGC_REG----------\n");
	dump_helix_state(vpu, cfgc_base, 32);

	//VDMA
	printk("-----------VDMA_REG-----------\n");
	dump_helix_state(vpu, vdma_base, 3);

	//EFE
	printk("-----------EFE_REG------------\n");
	dump_helix_state(vpu, efe_base, 15);

	//VMAU
	printk("-----------VMAU_REG------------\n");
	dump_helix_state(vpu, vmau_base, 31);

	//SDE
	printk("-----------SDE_REG-------------\n");
	dump_helix_state(vpu, sde_base, 7);
	printk("----------------helix_show_internal_state end------------\n");
}
#endif

static long vpu_start(struct device *dev, const struct channel_node * const cnode)
{
	struct jz_vpu_helix *vpu = dev_get_drvdata(dev);
	//struct channel_list *clist = list_entry(cnode->clist, struct channel_list, list);

#ifdef DUMP_HELIX_REG
	dev_info(vpu->vpu.dev, "------%s(%d)helix_show_internal_state start------\n", __func__, __LINE__);
	helix_show_internal_state(vpu);
	dev_info(vpu->vpu.dev, "------%s(%d)helix_show_internal_state end------\n", __func__, __LINE__);
#endif

    vpu_writel(vpu, REG_SCH_GLBC, SCH_GLBC_HIAXI | SCH_INTE_RESERR | SCH_INTE_ACFGERR
            | SCH_INTE_BSERR | SCH_INTE_ENDF);

	if (cnode->frame_type == FRAME_TYPE_IVDC) {
		void *ivdc_iomap = ioremap(IVDC_BASE_ADDR, 0x1000);
		writel(1, ivdc_iomap+0x78);
		writel(1, ivdc_iomap+0x70);
		iounmap(ivdc_iomap);
	}

	vpu_writel(vpu, REG_VDMA_TASKRG_T21, VDMA_ACFG_DHA(cnode->dma_addr)
			| VDMA_ACFG_RUN);

	dev_dbg(vpu->vpu.dev, "[%d:%d] start vpu\n", current->tgid, current->pid);

	return 0;
}

static long vpu_wait_complete(struct device *dev, struct channel_node * const cnode)
{
	long ret = 0, bsreadcnt = 3;
	struct jz_vpu_helix *vpu = dev_get_drvdata(dev);

hard_vpu_wait_restart:
	ret = wait_for_completion_interruptible_timeout(&vpu->done, msecs_to_jiffies(cnode->mdelay));
	if ((ret > 0) && (cnode->codecdir == HWJPEGDEC)) {
		vpu->bslen = vpu_readl(vpu, REG_JPGC_MCUS);
		vpu->cmpx = 0;
	}
	while ((bsreadcnt-- > 0) && (ret > 0) && (vpu->bslen == 0)) {
		if (cnode->codecdir == HWJPEGDEC) {
			vpu->bslen = vpu_readl(vpu, REG_JPGC_MCUS);
			vpu->cmpx = 0;
		} else if (cnode->codecdir == HWJPEGENC) {
			vpu->bslen = vpu_readl(vpu, REG_JPGC_STAT) & 0xffffff;
			vpu->cmpx = 0;
		} else if (cnode->codecdir == HWH264ENC) {
			vpu->bslen = vpu_readl(vpu, REG_SDE_CFG9);
			vpu->cmpx = vpu_readl(vpu, REG_EFE_SSAD);
		}
		if (vpu->bslen == 0) {
			msleep(2);
			continue;
		}
	}
	if ((ret > 0) && vpu->bslen) {
		ret = 0;
		dev_dbg(vpu->vpu.dev, "[%d:%d] wait complete finish\n", current->tgid, current->pid);
	} else if (ret == -ERESTARTSYS) {
		dev_dbg(vpu->vpu.dev, "[%d:%d]:fun:%s,line:%d vpu is interrupt\n", current->tgid, current->pid, __func__, __LINE__);
		goto hard_vpu_wait_restart;
	} else {
		dev_warn(dev, "[%d:%d] wait_for_completion timeout\n", current->tgid, current->pid);
		//dev_warn(dev, "vpu_stat = %x\n", vpu_readl(vpu,REG_SCH_STAT));
		//dev_warn(dev, "vdma_task = %x\n", vpu_readl(vpu,REG_VDMA_TASKST));
		//dev_warn(dev, "ret = %ld\n", ret);
#ifdef DUMP_HELIX_REG
		dev_info(vpu->vpu.dev, "------%s(%d)helix_show_internal_state start------\n", __func__, __LINE__);
		helix_show_internal_state(vpu);
		dev_info(vpu->vpu.dev, "------%s(%d)helix_show_internal_state end------\n", __func__, __LINE__);
#endif
		if (vpu_reset(dev) < 0) {
			dev_warn(dev, "vpu reset failed\n");
		}
		ret = -1;
	}
	cnode->output_len = vpu->bslen;
	cnode->status = vpu->status;
	cnode->cmpx = vpu->cmpx;

	//dev_info(dev, "[file:%s,fun:%s,line:%d] ret = %ld, status = %x, bslen = %d, cnode->cmpx=%d\n", __FILE__, __func__, __LINE__, ret, cnode->status, cnode->output_len, cnode->cmpx);

	return ret;
}

static long vpu_suspend(struct device *dev)
{
	return 0;
}

static long vpu_resume(struct device *dev)
{
	return 0;
}

static struct vpu_ops vpu_ops = {
	.owner		= THIS_MODULE,
	.open		= vpu_open,
	.release	= vpu_release,
	.start_vpu	= vpu_start,
	.wait_complete	= vpu_wait_complete,
	.reset		= vpu_reset,
	.suspend	= vpu_suspend,
	.resume		= vpu_resume,
};

static irqreturn_t vpu_interrupt(int irq, void *dev)
{
	struct jz_vpu_helix *vpu = dev;
	unsigned int vpu_stat;
	unsigned long vflag = 0;

	vpu_stat = vpu_readl(vpu,REG_SCH_STAT);

	spin_lock_irqsave(&vpu->slock, vflag);
	CLEAR_VPU_BIT(vpu, REG_SCH_GLBC, SCH_INTE_MASK);
	if(vpu_stat) {
		if(vpu_stat & SCH_STAT_ENDFLAG) {
			if(vpu_stat & SCH_STAT_JPGEND) {
				dev_dbg(vpu->vpu.dev, "JPG successfully done!\n");
				vpu->bslen = vpu_readl(vpu, REG_JPGC_STAT) & 0xffffff;
				vpu->cmpx = 0;
				CLEAR_VPU_BIT(vpu,REG_JPGC_STAT,JPGC_STAT_ENDF);
			} else {
				dev_dbg(vpu->vpu.dev, "SCH successfully done!\n");
				vpu->bslen = vpu_readl(vpu, REG_SDE_CFG9);
				vpu->cmpx = vpu_readl(vpu, REG_EFE_SSAD);
				CLEAR_VPU_BIT(vpu,REG_SDE_STAT,SDE_STAT_BSEND);
				CLEAR_VPU_BIT(vpu,REG_DBLK_GSTA,DBLK_STAT_DOEND);
			}
			vpu->status = vpu_stat;
			complete(&vpu->done);
		} else {
			check_vpu_status(SCH_STAT_ORESERR, "out fo resolution error!\n");
			check_vpu_status(SCH_STAT_BSERR, "BS error!\n");
			check_vpu_status(SCH_STAT_ACFGERR, "ACFG error!\n");
			vpu->bslen = 0;
			vpu->status = vpu_stat;
			vpu->cmpx = 0;
		}
	}

	spin_unlock_irqrestore(&vpu->slock, vflag);

	return IRQ_HANDLED;
}

static int vpu_probe(struct platform_device *pdev)
{
	int ret;
	struct resource	*regs;
	struct jz_vpu_helix *vpu;

	vpu = kzalloc(sizeof(struct jz_vpu_helix), GFP_KERNEL);
	if (!vpu) {
		dev_err(&pdev->dev, "kzalloc vpu space failed\n");
		ret = -ENOMEM;
		goto err_kzalloc_vpu;
	}

	vpu->vpu.vpu_id = VPU_HELIX_ID | (1 << pdev->id);
	vpu->vpu.idx = pdev->id;
	sprintf(vpu->name, "%s%d", pdev->name, pdev->id);

	vpu->irq = platform_get_irq(pdev, 0);
	if(vpu->irq < 0) {
		dev_err(&pdev->dev, "get irq failed\n");
		ret = vpu->irq;
		goto err_get_vpu_irq;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "No iomem resource\n");
		ret = -ENXIO;
		goto err_get_vpu_resource;
	}

	vpu->iomem = ioremap(regs->start, resource_size(regs));
	if (!vpu->iomem) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENXIO;
		goto err_get_vpu_iomem;
	}

#ifndef CONFIG_FPGA_TEST
/*	vpu->ahb1_gate = clk_get(&pdev->dev, "ahb1");
	if (IS_ERR(vpu->ahb1_gate)) {
		dev_err(&pdev->dev, "ahb1_gate get failed\n");
		ret = PTR_ERR(vpu->ahb1_gate);
		goto err_get_ahb1_clk_gate;
	}
*/

	vpu->clk_gate = clk_get(&pdev->dev, "avpu");
	if (IS_ERR(vpu->clk_gate)) {
		dev_err(&pdev->dev, "clk_gate %s get failed\n", vpu->name);
		ret = PTR_ERR(vpu->clk_gate);
		goto err_get_vpu_clk_gate;
	}

    vpu->clk = clk_get(&pdev->dev,"cgu_vpu");
    if (IS_ERR(vpu->clk)) {
        dev_err(&pdev->dev, "clk get failed\n");
        ret = PTR_ERR(vpu->clk);
        goto err_get_vpu_clk_cgu;
    }
    clk_set_rate(vpu->clk,450000000);
	clk_enable(vpu->clk);
	//clk_enable(vpu->ahb1_gate);
	clk_enable(vpu->clk_gate);
#endif
	spin_lock_init(&vpu->slock);
	mutex_init(&vpu->mutex);
	init_completion(&vpu->done);

	ret = request_irq(vpu->irq, vpu_interrupt, IRQF_DISABLED, vpu->name, vpu);
	if (ret < 0) {
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_vpu_request_irq;
	}
	disable_irq_nosync(vpu->irq);
#if 0
	vpu->cpm_pwc = cpm_pwc_get(PWC_VPU);
	if(!vpu->cpm_pwc) {
		dev_err(&pdev->dev, "get %s fail!\n",PWC_VPU);
		goto err_vpu_request_power;
	}
#endif

	vpu->vpu_status = VPU_STATUS_CLOSE;
	vpu->vpu.dev = &pdev->dev;
	vpu->vpu.ops = &vpu_ops;

	if ((ret = vpu_register(&vpu->vpu.vlist)) < 0) {
		dev_err(&pdev->dev, "vpu_register failed\n");
		goto err_vpu_register;
	}
	platform_set_drvdata(pdev, vpu);

	return 0;

err_vpu_register:
#if 0
	cpm_pwc_put(vpu->cpm_pwc);
err_vpu_request_power:
#endif
	free_irq(vpu->irq, vpu);
err_vpu_request_irq:
	clk_put(vpu->clk);
err_get_vpu_clk_cgu:
	clk_put(vpu->clk_gate);
err_get_vpu_clk_gate:
	clk_put(vpu->ahb1_gate);
err_get_ahb1_clk_gate:
	iounmap(vpu->iomem);
err_get_vpu_iomem:
err_get_vpu_resource:
err_get_vpu_irq:
	kfree(vpu);
err_kzalloc_vpu:
	return ret;
}

static int vpu_remove(struct platform_device *dev)
{
	struct jz_vpu_helix *vpu = platform_get_drvdata(dev);

	vpu_unregister(&vpu->vpu.vlist);
	//cpm_pwc_put(vpu->cpm_pwc);
	free_irq(vpu->irq, vpu);

#ifndef CONFIG_FPGA_TEST
    clk_put(vpu->clk);
	clk_put(vpu->clk_gate);
	clk_put(vpu->ahb1_gate);
#endif
	iounmap(vpu->iomem);
	kfree(vpu);

	return 0;
}

static int vpu_suspend_platform(struct platform_device *dev, pm_message_t state)
{
	struct jz_vpu_helix *vpu = platform_get_drvdata(dev);

	return vpu_suspend(vpu->vpu.dev);
}

static int vpu_resume_platform(struct platform_device *dev)
{
	struct jz_vpu_helix *vpu = platform_get_drvdata(dev);

	return vpu_resume(vpu->vpu.dev);
}

static struct platform_driver jz_vpu_driver = {
	.probe		= vpu_probe,
	.remove		= vpu_remove,
	.driver		= {
		.name	= "helix",
	},
	.suspend	= vpu_suspend_platform,
	.resume		= vpu_resume_platform,
};

static int __init vpu_init(void)
{
	return platform_driver_register(&jz_vpu_driver);
}

static void __exit vpu_exit(void)
{
	platform_driver_unregister(&jz_vpu_driver);
}

module_init(vpu_init);
module_exit(vpu_exit);

MODULE_DESCRIPTION("T30 VPU helix driver");
MODULE_AUTHOR("Justin <ptkang@ingenic.cn>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("20171010");
