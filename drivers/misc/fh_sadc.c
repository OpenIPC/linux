#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/miscdevice.h>
#include <mach/fh_sadc_plat.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include   "fh_sadc.h"
#include   <mach/pmu.h>

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define wrap_readl(wrap, name) \
	__raw_readl(&(((struct wrap_sadc_reg *)wrap->regs)->name))

#define wrap_writel(wrap, name, val) \
	__raw_writel((val), &(((struct wrap_sadc_reg *)wrap->regs)->name))


#define IOCTL_GET_SADC_DATA	1
#define IOCTL_SADC_POWER_DOWN	0xff
#define SADC_WRAP_BASE			(0xf1200000)
#define SADC_IRQn			(23)
#define SADC_MAX_CONTROLLER		(1)
#define SADC_STATUS_COLESD		(0)
#define SADC_STATUS_OPEN		(1)
#define FH_SADC_PLAT_DEVICE_NAME		"fh_sadc"
#define FH_SADC_MISC_DEVICE_NAME		"fh_sadc"

/****************************************************************************
* ADT section
*  add definition of user defined Data Type that only be used in this file  here
***************************************************************************/
struct sadc_info {
	int channel;
	int sadc_data;
};

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************

 *  static fun;
 *****************************************************************************/
static u32 fh_sadc_isr_read_data(struct wrap_sadc_obj *sadc,\
	 u32 channel, u16 *buf);
static int fh_sadc_open(struct inode *inode, struct file *file);
static int fh_sadc_release(struct inode *inode, struct file *filp);
static long fh_sadc_ioctl(struct file *filp, unsigned int cmd,\
	 unsigned long arg);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static struct wrap_sadc_obj fh_sadc_obj;

static const struct file_operations fh_sadc_fops = {
	.owner = THIS_MODULE,
	.open = fh_sadc_open,
	.release = fh_sadc_release,
	.unlocked_ioctl = fh_sadc_ioctl,
};

static struct miscdevice fh_sadc_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_SADC_MISC_DEVICE_NAME,
	.fops = &fh_sadc_fops,
};

/*****************************************************************************
 *
 *
 *		function body
 *
 *
 *****************************************************************************/
u32 fh_sadc_isr_read_data(struct wrap_sadc_obj *sadc, u32 channel,
		u16 *buf) {
	u32 xainsel = 1 << channel;
	u32 xversel = 0;
	u32 xpwdb = 1;
	/*cnt*/
	u32 sel2sam_pre_cnt = 2;
	u32 sam_cnt = 2;
	u32 sam2sel_pos_cnt = 2;
	/*time out*/
	u32 eoc_tos = 0xff;
	u32 eoc_toe = 0xff;
	u32 time_out = 0xffff;
	/*set isr en..*/
	u32 sadc_isr = 0x01;
	/*start*/
	u32 sadc_cmd = 0x01;
	/*get data*/
	u32 temp_data = 0;
	u32 ret_time;

	reinit_completion(&sadc->done);
	/*control...*/
	wrap_writel(sadc, sadc_control, xainsel | (xversel << 8) \
			| (xpwdb << 12));

	wrap_writel(sadc, sadc_cnt,
			sel2sam_pre_cnt | (sam_cnt << 8) | \
			(sam2sel_pos_cnt << 16));

	wrap_writel(sadc, sadc_timeout,
			eoc_tos | (eoc_toe << 8) | (time_out << 16));

	wrap_writel(sadc, sadc_ier, sadc_isr);

	wrap_writel(sadc, sadc_cmd, sadc_cmd);

	ret_time = wait_for_completion_timeout(&sadc->done, 5000);
	if (ret_time == 0) {
		printk(KERN_ERR "sadc timeout..\n");
		return SADC_TIMEOUT;
	}

	switch (channel) {
	case 0:
	case 1:
		/*read channel 0 1*/
		temp_data = wrap_readl(sadc, sadc_dout0);
		break;

	case 2:
	case 3:
		/*read channel 2 3*/
		temp_data = wrap_readl(sadc, sadc_dout1);
		break;

	case 4:
	case 5:
		/*read channel 4 5*/
		temp_data = wrap_readl(sadc, sadc_dout2);
		break;

	case 6:
	case 7:
		/*read channel 6 7*/
		temp_data = wrap_readl(sadc, sadc_dout3);
		break;
	default:
		break;
	}
	if (channel % 2) {
		/*read low 16bit*/
		*buf = (u16) (temp_data & 0xffff);
	} else {
		/*read high 16bit*/
		*buf = (u16) (temp_data >> 16);
	}
	return 0;

}

int fh_sadc_enable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, sadc_control);
	control_reg |= 1 << 12;
	wrap_writel(sadc, sadc_control, control_reg);
	return 0;
}

int fh_sadc_disable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, sadc_control);
	control_reg &= ~(1 << 12);
	wrap_writel(sadc, sadc_control, control_reg);
	return 0;
}

static irqreturn_t fh_sadc_isr(int irq, void *dev_id)
{

	u32 isr_status;
	struct wrap_sadc_obj *sadc = (struct wrap_sadc_obj *) dev_id;

	isr_status = wrap_readl(sadc, sadc_int_status);
	if (isr_status & 0x01) {

		u32 sadc_isr = 0x00;
		wrap_writel(sadc, sadc_ier, sadc_isr);

		wrap_writel(sadc, sadc_int_status, isr_status);
		complete(&(sadc->done));
	} else {

		printk(KERN_ERR "sadc maybe error!\n");

	}
	return IRQ_HANDLED;
}

long fh_sadc_get_value(int channel)
{
	unsigned int ret;
	long w = 0;
	u16 ad_raw_data;
	if (channel < 0) {
		printk(KERN_ERR "ERROR: %s, sadc channel no %d is incorrect\n",
				__func__, channel);
		return 0;
	}

	fh_sadc_enable();

	ret = fh_sadc_isr_read_data(&fh_sadc_obj, channel, &ad_raw_data);

	if (ret != 0) {
		printk(KERN_INFO "sadc error code:0x%x\n", ret);
	} else {
		w = ad_raw_data * fh_sadc_obj.refvol / fh_sadc_obj.activebit;
		printk(KERN_INFO "the value of sadc is: %ld\n", w);
	}

	return w;
}

static void del_char(char *str, char ch)
{
	char *p = str;
	char *q = str;
	while (*q) {
		if (*q != ch)
			*p++ = *q;
		q++;
	}
	*p = '\0';
}

static ssize_t fh_sadc_proc_write(struct file *filp, const char *buf,
	 size_t len, loff_t *off)
{
	char message[32] = {0};
	char * const delim = ",";
	char *cur = message, *power_str;
	int power;
	unsigned int str_len;
	str_len = 10;
	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	power_str = strsep(&cur, delim);
	if (!power_str) {
		pr_err("%s: ERROR: parameter is empty\n", __func__);
		return -EINVAL;
	} else {
		del_char(power_str, ' ');
		del_char(power_str, '\n');
		power = kstrtouint(power_str, 0, &str_len);
		if (power < 0) {
			pr_err("%s: ERROR: parameter is incorrect\n",\
			 __func__);
			return -EINVAL;
		}
	}

	power ? fh_sadc_enable() : fh_sadc_disable();

	return len;
}

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter;
	counter = 0;
	if (*pos == 0)
		return &counter;
	else {
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
	int i;
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 ret;

	reg = (wrap_readl(sadc, sadc_control) & 0x1000);

	seq_printf(sfile, "\nSADC Status:\n");
	seq_printf(sfile, "Power %s\n\n", reg ? "up" : "down");

	for (i = 0; i < 8; i++) {
		u16 ad_raw_data;
		ret = fh_sadc_isr_read_data(&fh_sadc_obj, i, &ad_raw_data);
		if (ret != 0)
			seq_printf(sfile, "sadc error code:0x%x\n", ret);
		else
			seq_printf(sfile, "channel: %d \tvalue: %u\n", i,
		ad_raw_data * fh_sadc_obj.refvol / fh_sadc_obj.activebit);
	}
	return 0;
}

static const struct seq_operations isp_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_sadc_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &isp_seq_ops);
}

static const struct file_operations fh_sadc_proc_ops = {
	.owner = THIS_MODULE,
	.open = fh_sadc_proc_open,
	.read = seq_read,
	.write = fh_sadc_proc_write,
	.release = seq_release,
};


static struct of_device_id const fh_sadc_of_match[] = {
	{ .compatible = "fh,fh-sadc" },
	{}
};
MODULE_DEVICE_TABLE(of, fh_sadc_of_match);


static int fh_sadc_probe(struct platform_device *pdev)
{
	int err;
#ifdef CONFIG_USE_OF
	const struct of_device_id *match;
	struct device_node *np = pdev->dev.of_node;
#else
	struct fh_sadc_platform_data *plat_data;
	struct resource *res;
#endif
	struct clk *sadc_clk;


	sadc_clk = clk_get(&pdev->dev, "sadc_clk");
	if (IS_ERR(sadc_clk)) {
		err = PTR_ERR(sadc_clk);
		return -EPERM;
	}
	clk_prepare_enable(sadc_clk);
	fh_sadc_obj.clk = sadc_clk;
#ifdef CONFIG_USE_OF
		match = of_match_device(fh_sadc_of_match, &pdev->dev);
		if (!match) {
			pr_info("Failed to find sadc controller\n");
			return -ENODEV;
		}

		fh_sadc_obj.regs = of_iomap(np, 0);
		if (fh_sadc_obj.regs == NULL) {
			err = -ENXIO;
			return err;
		}


		fh_sadc_obj.irq_no = irq_of_parse_and_map(np, 0);
		if (fh_sadc_obj.irq_no < 0) {
			dev_err(&pdev->dev, "no irq resource\n");
			goto fail_no_ioremap;
		}
		of_property_read_u32(np, "ref-vol", &fh_sadc_obj.refvol);
		of_property_read_u32(np, "active-bit", &fh_sadc_obj.activebit);
#else
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(&pdev->dev, "sadc get platform source error..\n");
			return -ENODEV;
		}
		plat_data = pdev->dev.platform_data;
		if (!plat_data) {
			dev_err(&pdev->dev, "sadc get platform data error..\n");
			return -ENODEV;
		}

		fh_sadc_obj.refvol = plat_data->ref_vol;
		fh_sadc_obj.activebit = plat_data->active_bit;
		fh_sadc_obj.irq_no = platform_get_irq(pdev, 0);
		fh_sadc_obj.irq_no = irq_create_mapping(NULL,
			fh_sadc_obj.irq_no);
		if (fh_sadc_obj.irq_no < 0) {
			dev_warn(&pdev->dev, "sadc interrupt is not available.\n");
			return fh_sadc_obj.irq_no;
		}

		res = request_mem_region(res->start, resource_size(res),
			pdev->name);
		if (res == NULL) {
			dev_err(&pdev->dev, "sadc region already claimed\n");
			return -EBUSY;
		}

		fh_sadc_obj.regs = ioremap(res->start, resource_size(res));
		if (fh_sadc_obj.regs == NULL) {
			err = -ENXIO;
			goto fail_no_ioremap;
		}
#endif
	strncpy(fh_sadc_obj.isr_name, "sadc", sizeof("sadc"));
	err = request_irq(fh_sadc_obj.irq_no, fh_sadc_isr, IRQF_NO_THREAD,
		fh_sadc_obj.isr_name, &fh_sadc_obj);

	if (err) {
		dev_err(&pdev->dev, "request_irq:%d failed, %d\n",
			fh_sadc_obj.irq_no, err);
		err = -ENXIO;
		goto fail_no_ioremap;
	}

	init_completion(&fh_sadc_obj.done);
	mutex_init(&fh_sadc_obj.sadc_lock);
	fh_sadc_obj.active_channel_no = 0;


	err = misc_register(&fh_sadc_misc);

	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed", __func__,
				FH_SADC_MISC_DEVICE_NAME);
		err = -ENXIO;
		goto misc_error;
	}

	fh_sadc_obj.proc_file = proc_create(FH_SADC_PROC_FILE,
		S_IRUGO, NULL, &fh_sadc_proc_ops);

	if (!fh_sadc_obj.proc_file)
		pr_err("%s: ERROR: %s proc file create failed",
		__func__, "SADC");

	pr_info("SADC probe successful, IO base addr: %p\n",
		fh_sadc_obj.regs);
	return 0;

misc_error:
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);

fail_no_ioremap:
	iounmap(fh_sadc_obj.regs);


	return err;
}

static int __exit fh_sadc_remove(struct platform_device *pdev)
{
	struct resource *res;

	if (fh_sadc_obj.clk) {
		clk_disable_unprepare(fh_sadc_obj.clk);
		clk_put(fh_sadc_obj.clk);
	}
	misc_deregister(&fh_sadc_misc);
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);
	proc_remove(fh_sadc_obj.proc_file);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));
	iounmap(fh_sadc_obj.regs);
	return 0;

}


static int fh_sadc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_sadc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long fh_sadc_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg) {

	u32 ad_data;
	u32 control_reg;
	u16 ad_raw_data;
	struct sadc_info sadc_info;

	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	mutex_lock(&sadc->sadc_lock);
	if (cmd == IOCTL_GET_SADC_DATA) {
		if (copy_from_user((void *) &sadc_info, (void __user*) arg,
				sizeof(sadc_info))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
		if ((wrap_readl(sadc, sadc_status2) & 0x10)) {
				mutex_unlock(&sadc->sadc_lock);
				return -EBUSY;
			}
		fh_sadc_isr_read_data(&fh_sadc_obj, sadc_info.channel,\
			&ad_raw_data);
		ad_data = ad_raw_data * fh_sadc_obj.refvol;
		ad_data /= fh_sadc_obj.activebit;
		sadc_info.sadc_data = ad_data;
		if (put_user(sadc_info.sadc_data,
				(int __user *)(&((struct sadc_info *)arg)\
				->sadc_data))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
	}

	else if (cmd == IOCTL_SADC_POWER_DOWN) {
		control_reg = wrap_readl(sadc, sadc_control);
		control_reg &= ~(1 << 12);
		wrap_writel(sadc, sadc_control, control_reg);
	}
	mutex_unlock(&sadc->sadc_lock);
	return 0;
}


/*******************
 *
 *
 *add platform cause of i need the board info...
 *in the probe function. i will register the sadc
 *misc drive...then the app can open the sadc misc device..
 *
 ******************/
static struct platform_driver fh_sadc_driver = {
	.driver	= {
		.name	= FH_SADC_PLAT_DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = fh_sadc_of_match,
	},
	.probe	= fh_sadc_probe,
	.remove	= __exit_p(fh_sadc_remove),
};



static int __init fh_sadc_init(void)
{
	return platform_driver_register(&fh_sadc_driver);
}

static void __exit fh_sadc_exit(void)
{

	platform_driver_unregister(&fh_sadc_driver);

}

module_init(fh_sadc_init);
module_exit(fh_sadc_exit);

MODULE_DESCRIPTION("fullhan sadc device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tangyh@fullhan.com");
MODULE_ALIAS("platform:FH_sadc");
