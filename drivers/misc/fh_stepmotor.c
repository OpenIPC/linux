#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include   <mach/pmu.h>
#include	<linux/sched.h>
#include <linux/clk.h>

#include "fh_stepmotor.h"

#define FHSM_DEF_PERIOD (128)

struct fh_sm_obj_t* fh_sm_obj[MAX_FHSM_NR];
static int fh_stepmotor_probe(struct platform_device *pdev);
static int __exit fh_stepmotor_remove(struct platform_device *pdev);








static long fh_stepmotor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct fh_sm_param* param;
	struct fh_sm_lut* lut;
	struct fh_sm_obj_t* curobj;
	if (arg == 0)
		return -EINVAL;

	if (unlikely(_IOC_TYPE(cmd) != FH_SM_IOCTL_MAGIC))
	{
		pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
			   __func__, _IOC_TYPE(cmd), -ENOTTY);
		return -ENOTTY;
	}
	curobj = (struct fh_sm_obj_t*)filp->private_data;
	if (curobj == NULL) {
		return -EFAULT;
	}
	switch (cmd)
	{
	case FH_SM_SET_PARAM:
	{
		if (fh_stepmotor_is_busy(curobj)) {
			ret = -EINVAL;
			break;
		}
		param = kzalloc(sizeof(struct fh_sm_param), GFP_KERNEL);
		if (param == NULL)
		{
			ret = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (void __user *)arg, sizeof(struct fh_sm_param)))
		{
			ret = EINVAL;
			kfree(param);
			break;
		}
		fh_stepmotor_set_hw_param(curobj,param);
		kfree(param);
		ret  = 0;
		break;
	}
	case FH_SM_GET_PARAM:
	{
		param = kzalloc(sizeof(struct fh_sm_param), GFP_KERNEL);
		if (param == NULL)
		{
			ret = -ENOMEM;
			break;
		}
		fh_stepmotor_get_hw_param(curobj,param);
		if (copy_to_user( (void __user *)arg, (void *)param,sizeof(struct fh_sm_param))) {
			kfree(param);
			return -EFAULT;
		}
		kfree(param);
		ret  = 0;
		break;
	}
	
	
	case FH_SM_START_ASYNC:
	{
		int cycles = 0;
		if (fh_stepmotor_is_busy(curobj)) {
			ret = -EINVAL;
			break;
		}
		if (copy_from_user(&cycles, (void __user *)arg, sizeof(int)))
		{
			ret = EINVAL;
			break;
		}

		fh_stepmotor_start_async(curobj,cycles);
		ret  = 0;
		break;
	}
	case FH_SM_START_SYNC:
	{
		int cycles = 0;
		int fin = 0;
		if (fh_stepmotor_is_busy(curobj)) {
			ret = -EINVAL;
			break;
		}
		if (copy_from_user(&cycles, (void __user *)arg, sizeof(int)))
		{
			ret = EINVAL;
			break;
		}

		fin = fh_stepmotor_start_sync(curobj,cycles);
		if (copy_to_user( (void __user *)arg, (void *)&fin,sizeof(fin))) {
			return -EFAULT;
		}
		ret  = 0;
		break;
	}
	case FH_SM_STOP:
	{
		int fincnt = 0;
		fincnt = fh_stepmotor_stop(curobj);
		if (copy_to_user( (void __user *)arg, (void *)&fincnt,sizeof(fincnt))) {
			return -EFAULT;
		}
		ret  = 0;
		break;
	}
	case FH_SM_GET_CUR_CYCLE:
	{
		int fincnt = 0;
		fincnt = fh_stepmotor_get_current_cycle(curobj);
		if (copy_to_user( (void __user *)arg, (void *)&fincnt,sizeof(fincnt))) {
			return -EFAULT;
		}
		ret  = 0;
		break;
	}
	
	case FH_SM_GET_LUT:
	{
		struct fh_sm_lut tmplut;

		lut = kzalloc(sizeof(struct fh_sm_lut), GFP_KERNEL);
		if (lut == NULL)
		{
			ret = -ENOMEM;
			break;
		}
		lut->lutsize = fh_stepmotor_get_lutsize(curobj);
		lut->lut = kzalloc(lut->lutsize, GFP_KERNEL);
		if (lut->lut == NULL) {
			ret = EINVAL;
			kfree(lut);
			break;
		}
		fh_stepmotor_get_lut(curobj,lut);

		if (copy_from_user(&tmplut, (void __user *)arg, sizeof(struct fh_sm_lut)))
		{
			ret = EINVAL;
			kfree(lut->lut);
			kfree(lut);
			break;
		}
		if (tmplut.lutsize != 0 && tmplut.lut != NULL) {
			if (copy_to_user( (void __user *)tmplut.lut,lut->lut, lut->lutsize))
			{
				ret = EINVAL;
				kfree(lut->lut);
				kfree(lut);
				break;
			}
			kfree(lut->lut);
			lut->lut = tmplut.lut;
			
			if (copy_to_user(lut, (void __user *)arg, sizeof(struct fh_sm_lut)))
			{
				ret = EINVAL;
				kfree(lut);
				break;
			}		
		} else {
			kfree(lut->lut);
		}

		kfree(lut);
		ret  = 0;
		break;
	}
	case FH_SM_SET_LUT:
	{
		if (fh_stepmotor_is_busy(curobj)) {
			ret = -EINVAL;
			break;
		}
		lut = kzalloc(sizeof(struct fh_sm_lut), GFP_KERNEL);
		if (lut == NULL)
		{
			ret = -ENOMEM;
			break;
		}
		if (copy_from_user(lut, (void __user *)arg, sizeof(struct fh_sm_lut)))
		{
			ret = EINVAL;
			break;
		}
		if (lut->lut != NULL) {
			int* buff =  kzalloc(lut->lutsize, GFP_KERNEL);
			if (copy_from_user(buff, (void __user *)lut->lut, lut->lutsize)) {
				kfree(buff);
				ret = EINVAL;
				lut->lut = NULL;
			}
			lut->lut = buff;
		}
		
		ret = fh_stepmotor_set_lut(curobj,lut);

		if (lut->lut != NULL) {
			kfree(lut->lut);
		}
		kfree(lut);
		ret  = 0;
		break;
	}
	}
	
	
	

	return ret;
}

static int fh_stepmotor_open(struct inode *inode, struct file *file)
{
	if (strchr(file->f_path.dentry->d_name.name,'0') != NULL) {
		file->private_data = (void*)fh_sm_obj[0];
	} else if(strchr(file->f_path.dentry->d_name.name,'1') != NULL) {
		file->private_data = (void*)fh_sm_obj[1];
	}
	return 0;
}

static int fh_stepmotor_release(struct inode *inode, struct file *filp)
{
	
	return 0;
}


static const struct file_operations fh_stepmotor_fops =
{
	.owner				  = THIS_MODULE,
	.open				   = fh_stepmotor_open,
	.release				= fh_stepmotor_release,
	.unlocked_ioctl		 = fh_stepmotor_ioctl,
};
#if 0
static struct miscdevice fh_stepmotor_device =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_SM_MISC_DEVICE_NAME,
	.fops = &fh_stepmotor_fops,
};
#endif
struct miscdevice* fh_stepmotor_device[MAX_FHSM_NR];



#ifdef CONFIG_USE_OF
static struct of_device_id const fh_sm_of_match[] = {
	{ .compatible = "fh,fh-stm" },
	{}
};
MODULE_DEVICE_TABLE(of, fh_sm_of_match);
#endif
static struct platform_driver fh_stepmotor_driver = {
	.driver	= {
		.name	= FH_SM_PLAT_DEVICE_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_USE_OF
		.of_match_table = fh_sm_of_match,
#endif
	},
	.probe	= fh_stepmotor_probe,
	.remove	= __exit_p(fh_stepmotor_remove),
};


static int fh_stepmotor_probe(struct platform_device *pdev)
{
	int err;
	struct clk *sm_clk;
#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;
#else
	struct resource *res;
#endif
	int id = 0;
	static char devstr[MAX_FHSM_NR][40];
	char clkname[24] = {0};

#ifdef CONFIG_USE_OF
	id = of_alias_get_id(pdev->dev.of_node, "stm");
	fh_sm_obj[id] = kzalloc(sizeof(struct fh_sm_obj_t),GFP_KERNEL);
	if (fh_sm_obj[id] == NULL) {
		err = -ENXIO;
		return err;
	}
	fh_sm_obj[id]->irq_no = irq_of_parse_and_map(np, 0);
	if (fh_sm_obj[id]->irq_no < 0) {
		dev_warn(&pdev->dev, "sm interrupt is not available.\n");
		return fh_sm_obj[id]->irq_no;
	}

	fh_sm_obj[id]->regs = of_iomap(np, 0);
	if (fh_sm_obj[id]->regs == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}
#else
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "sm get platform source error..\n");
		return -ENODEV;
	}
	id = pdev->id;
	fh_sm_obj[id] = kzalloc(sizeof(struct fh_sm_obj_t),GFP_KERNEL);
	if (fh_sm_obj[id] == NULL) {
		err = -ENXIO;
		return err;
	}
	fh_sm_obj[id]->irq_no = platform_get_irq(pdev, 0);
	fh_sm_obj[id]->irq_no = irq_create_mapping(NULL,
		fh_sm_obj[id]->irq_no);
	if (fh_sm_obj[id]->irq_no < 0) {
		dev_warn(&pdev->dev, "sm interrupt is not available.\n");
		return fh_sm_obj[id]->irq_no;
	}

	res = request_mem_region(res->start, resource_size(res),
	pdev->name);
	if (res == NULL) {
		dev_err(&pdev->dev, "sm region already claimed\n");
		return -EBUSY;
	}
	fh_sm_obj[id]->regs = ioremap(res->start, resource_size(res));
	if (fh_sm_obj[id]->regs == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}

#endif
	sprintf(clkname,"stm%d_clk",id);
	snprintf(fh_sm_obj[id]->isrname,  sizeof(fh_sm_obj[id]->isrname), "stm-%d", id);

	sm_clk = clk_get(&pdev->dev, clkname);
	if (!IS_ERR(sm_clk))
		clk_prepare_enable(sm_clk);


	init_completion(&fh_sm_obj[id]->run_complete);

	err = request_irq(fh_sm_obj[id]->irq_no, fh_stepmotor_isr, IRQF_SHARED,
		fh_sm_obj[id]->isrname, fh_sm_obj[id]);
	if (err) {
		dev_err(&pdev->dev, "request_irq:%d failed, %d\n",
			fh_sm_obj[id]->irq_no, err);
		err = -ENXIO;
		goto fail_no_ioremap;
	}
	fh_stepmotor_init_hw_param(fh_sm_obj[id]);
	
	fh_stepmotor_device[id] = kzalloc(sizeof(struct miscdevice),GFP_KERNEL);
	fh_stepmotor_device[id]->minor = MISC_DYNAMIC_MINOR;
	fh_stepmotor_device[id]->fops = &fh_stepmotor_fops;
	memset(devstr[id],0,40);
	sprintf(devstr[id], FH_SM_MISC_DEVICE_NAME, id);
	fh_stepmotor_device[id]->name = devstr[id];
	err = misc_register(fh_stepmotor_device[id]);

	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed", __func__,
				FH_SM_DEVICE_NAME);
		err = -ENXIO;
		goto misc_error;
	}

	return 0;

misc_error:
	free_irq(fh_sm_obj[id]->irq_no, &fh_sm_obj);

fail_no_ioremap:
	iounmap(fh_sm_obj[id]->regs);

	return err;
}

static int __exit fh_stepmotor_remove(struct platform_device *pdev)
{
	misc_deregister(fh_stepmotor_device[pdev->id]);
	kfree(fh_stepmotor_device[pdev->id]);
	free_irq(fh_sm_obj[pdev->id]->irq_no, &fh_sm_obj);
	iounmap(fh_sm_obj[pdev->id]->regs);
	return 0;
}






static int __init fh_stepmotor_init(void)
{
	return platform_driver_register(&fh_stepmotor_driver);

}

static void __exit fh_stepmotor_exit(void)
{

	platform_driver_unregister(&fh_stepmotor_driver);
}
module_init(fh_stepmotor_init);
module_exit(fh_stepmotor_exit);

MODULE_AUTHOR("Fullhan");
MODULE_DESCRIPTION("Misc Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH StepMotor");
