
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include    "fh_sadc_v3.h"
#include    <mach/pmu.h>
#include    <linux/sched.h>
#include    <linux/freezer.h>
#include    <linux/of_address.h>
#include    <linux/of_irq.h>
#include    <linux/of_platform.h>
#include    <linux/of_address.h>
#include    <mach/clock.h>
#include <mach/fh_sadc_plat.h>

/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define wrap_readl(wrap, offset) \
	__raw_readl(wrap->regs + offset)

#define wrap_writel(wrap, offset, val) \
	__raw_writel((val), wrap->regs + offset)


#define IOCTL_GET_SADC_DATA	          	0x3
#define IOCTL_GET_SINGLE_SADC_DATA		0x1
#define IOCTL_GET_HIT_DATA		        0x6
#define IOCTL_SADC_POWER_DOWN	        0xff

#define FH_SADC_MISC_DEVICE_NAME		"fh_sadc"
static DECLARE_WAIT_QUEUE_HEAD(fh_sadc_hit_wait);


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
int fh_sadc_enable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, SADC_CTRL0);
	control_reg |= 1 << 0;
	wrap_writel(sadc, SADC_CTRL0, control_reg);
	return 0;
}

int fh_sadc_disable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, SADC_CTRL0);
	control_reg &= ~(1 << 0);
	wrap_writel(sadc, SADC_CTRL0, control_reg);
	return 0;
}

int fh_sadc_update(void)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, SADC_CTRL1);
	reg |= 1;
	wrap_writel(sadc, SADC_CTRL1, reg);
	return 0;
}

int fh_sadc_lowpwr_enable(void)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, SADC_CTRL2);
	reg |= (1 << 0);
	wrap_writel(sadc, SADC_CTRL2, reg);
	return 0;
}

u32 fh_sadc_lowpwr_disable(void)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, SADC_CTRL2);
	reg &= (~(1 << 0));
	wrap_writel(sadc, SADC_CTRL2, reg);
	return 0;
}

int fh_sadc_continue_enable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG1);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG1, reg);
	return 0;
}

int fh_sadc_continue_disable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG1);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG1, reg);
	return 0;
}

int fh_sadc_single_enable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG0);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG0, reg);
	return 0;
}

int fh_sadc_single_disable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG0);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG0, reg);
	return 0;
}

int fh_sadc_eq_enable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG2);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG2, reg);
	return 0;
}

int fh_sadc_eq_disable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG2);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG2, reg);
	return 0;
}

int fh_sadc_hit_enable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG3);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_CONFIG3, reg);
	return 0;
}

int fh_sadc_hit_disable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel >= MAX_CHANNEL_NO) {
		pr_err("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG3);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_CONFIG3, reg);
	return 0;
}

int fh_sadc_scan_time(u32 scan_time)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 value;

	value = scan_time*(sadc->frequency/1000);
	wrap_writel(sadc, SADC_CONFIG4, value);
	return 0;
}

int fh_sadc_glitch_time(u32 glitch_time)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 value;

	value = glitch_time*(sadc->frequency/1000);
	wrap_writel(sadc, SADC_CONFIG5, value);
	return 0;
}

int fh_sadc_noise_range(u32 noise_range)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 value;

	value = (noise_range*fh_sadc_obj.activebit)/fh_sadc_obj.refvol;
	value &= 0xfff;
	if(value >= 0xff) {
		pr_err("sadc noise range error %d\n", noise_range);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_CONFIG6);
	reg &= (~(0xff << 0));
	reg |= (value << 0);
	wrap_writel(sadc, SADC_CONFIG6, reg);
	return 0;
}

int fh_sadc_zero_value(u32 zero_value)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 value;

	value = (zero_value*fh_sadc_obj.activebit)/fh_sadc_obj.refvol;
	value &= 0xfff;

	reg = wrap_readl(sadc, SADC_CONFIG6);
	reg &= (~(0xfff << 16));
	reg |= (value << 16);
	wrap_writel(sadc, SADC_CONFIG6, reg);
	return 0;
}

int fh_sadc_active_bit(u32 active_bit)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(active_bit > 0x12) {
		pr_err("sadc active bit error %d\n", active_bit);
		return -1;
	}
	reg = 0xfff >> (12-active_bit);
	reg = reg << (12-active_bit);
	wrap_writel(sadc, SADC_CONFIG7, reg);
	return 0;
}

int fh_sadc_int_enable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel > MAX_CHANNEL_NO) {
		pr_err("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg |= (1 << channel);
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}

int fh_sadc_int_disable(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel > MAX_CHANNEL_NO) {
		pr_err("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg &= (~(1 << channel));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}

int fh_sadc_press_int_en(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel > MAX_CHANNEL_NO) {
		pr_err("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg |= (1 << (channel + 8));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}

int fh_sadc_press_int_dis(u32 channel)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if(channel > MAX_CHANNEL_NO) {
		pr_err("sadc channel num error %d\n", channel);
		return -1;
	}
	reg = wrap_readl(sadc, SADC_INT_EN);
	reg &= (~(1 << (channel + 8)));
	wrap_writel(sadc, SADC_INT_EN, reg);
	return 0;
}

u32 fh_sadc_isr_read_data(struct wrap_sadc_obj *sadc, u32 channel,
		u16 *buf) {
	u32 ret_time;

	reinit_completion(&sadc->done);
	sadc->active_channel_no = 0xffffffff;
	fh_sadc_eq_enable(channel);
	fh_sadc_active_bit(12);
	fh_sadc_continue_enable(channel);
	fh_sadc_int_enable(channel);
	/*10ms*/
	fh_sadc_scan_time(10);
	fh_sadc_enable();
	fh_sadc_update();
	ret_time = wait_for_completion_timeout(&sadc->done, 500);
	if (ret_time == 0) {
		printk(KERN_ERR "sadc timeout..\n");
		return 1;
	}

	if(channel == sadc->active_channel_no)
		*buf = (sadc->channel_data[channel]*fh_sadc_obj.refvol)
		/fh_sadc_obj.activebit;
	else
		pr_err("sadc channel :%d,return channel:%d error\n", channel,
		sadc->active_channel_no);
	fh_sadc_eq_disable(channel);
	fh_sadc_continue_disable(channel);
	fh_sadc_int_disable(channel);
	fh_sadc_update();
	return 0;
}

u32 fh_sadc_isr_read_single_data(struct wrap_sadc_obj *sadc, u32 channel,
		u16 *buf) {
	u32 ret_time;

	reinit_completion(&sadc->done);
	sadc->active_channel_no = 0xffffffff;
	fh_sadc_active_bit(12);
	fh_sadc_single_enable(channel);
	fh_sadc_int_enable(channel);
	/*10ms*/
	fh_sadc_scan_time(10);
	fh_sadc_enable();
	fh_sadc_update();
	ret_time = wait_for_completion_timeout(&sadc->done, 500);
	if (ret_time == 0) {
		printk(KERN_ERR "sadc timeout..\n");
		return 1;
	}
	if(channel == sadc->active_channel_no)
		*buf = (sadc->channel_data[channel]*fh_sadc_obj.refvol)
		/fh_sadc_obj.activebit;
	else
		pr_err("sadc channel :%d,return channel:%d error\n", channel,
		sadc->active_channel_no);
	fh_sadc_single_disable(channel);
	fh_sadc_int_disable(channel);
	fh_sadc_update();
	return 0;
}

u32 fh_sadc_isr_read_hit_data(struct wrap_sadc_obj *sadc, u32 channel,
		u16 *buf) {

	sadc->active_channel_no = 0xffffffff;
	fh_sadc_active_bit(12);
	fh_sadc_continue_enable(channel);
	fh_sadc_hit_enable(channel);
	fh_sadc_press_int_en(channel);
	/*10ms*/
	fh_sadc_scan_time(10);
	fh_sadc_enable();
	fh_sadc_update();
	wait_event_freezable(fh_sadc_hit_wait, channel == sadc->active_channel_no);
	*buf = (sadc->channel_data[channel]*fh_sadc_obj.refvol)
			/ fh_sadc_obj.activebit;
	fh_sadc_continue_disable(channel);
	fh_sadc_hit_disable(channel);
	fh_sadc_press_int_dis(channel);
	fh_sadc_update();
	return 0;
}

static irqreturn_t fh_sadc_isr(int irq, void *dev_id)
{

	u32 isr_status;
	u32 isr_active;
	u32 isr_en;
	u32 temp_data =  0;
	u32 channel = 0;
	u32 data = 0;
	struct wrap_sadc_obj *sadc = (struct wrap_sadc_obj *) dev_id;

	isr_status = wrap_readl(sadc, SADC_INT_STA);
	isr_en = wrap_readl(sadc, SADC_INT_EN);
	isr_active = isr_status & isr_en;
	if (isr_active & 0xff) {
		channel = ffs(isr_active);
		channel = channel - 1;
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, SADC_DOUT0);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, SADC_DOUT1);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, SADC_DOUT2);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, SADC_DOUT3);
			break;
		default:
			break;
		}
		complete(&(sadc->done));
	} else if (isr_active & 0xffff00) {
		if(isr_active & 0xff00)
			channel = ffs(isr_active >> 8);
		else
			channel = ffs(isr_active >> 16);
		channel = channel - 1;
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT0);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT1);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT2);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, SADC_BUTTON_DOUT3);
			break;
		default:
			break;
		}
		wake_up(&fh_sadc_hit_wait);
	}

	if (channel % 2)
		/*read high 16bit*/
		data = (temp_data >> 16) & 0xfff;
	else
		/*read low 16bit*/
		data = (temp_data & 0xfff);
	sadc->channel_data[channel] = data;
	sadc->active_channel_no = channel;
	wrap_writel(sadc, SADC_INT_STA, (~isr_status));
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
	char *cur = message;
	int i;
	char *param_str[2];
	int ret;
	unsigned long param = 0;
	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 2; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n",
				__func__, i);
			pr_err("[debug],[channel],[pwr_down]\n");
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}

	if (!strcmp(param_str[0], "pwr_down")) {
		ret = (u32)kstrtoul(param_str[1], 10, &param);
		param ? fh_sadc_lowpwr_enable() : fh_sadc_lowpwr_disable();
	}

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
	u32 ret;
	u16 ad_raw_data;

	seq_printf(sfile, "\nSADC Status:\n");
	for (i = 0; i < 8; i++) {
		ret = fh_sadc_isr_read_data(&fh_sadc_obj, i, &ad_raw_data);
		if (ret != 0)
			seq_printf(sfile, "contine sadc error code:0x%x\n", ret);
		else
			seq_printf(sfile, "continue read channel: %d \tvalue: %u\n", i,
		ad_raw_data);
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
	struct resource *res;
	struct fh_sadc_platform_data *plat_data;
#endif
	struct clk  *sadc_clk;
	int irq;

	sadc_clk = clk_get(&pdev->dev, "sadc_clk");
	if (IS_ERR(sadc_clk)) {
		err = PTR_ERR(sadc_clk);
		return -EPERM;
	}
	clk_prepare_enable(sadc_clk);
	fh_sadc_obj.clk = sadc_clk;
#ifdef CONFIG_FPGA
#define FPGA_SADC_CLOCK 15000000
	fh_sadc_obj.frequency = FPGA_SADC_CLOCK;
#else
	fh_sadc_obj.frequency = clk_get_rate(sadc_clk);
#endif
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

		irq = irq_of_parse_and_map(np, 0);
		if (irq < 0) {
			dev_err(&pdev->dev, "no irq resource\n");
			goto fail_no_ioremap;
		}
		of_property_read_u32(np, "ref-vol", &fh_sadc_obj.refvol);
		of_property_read_u32(np, "active-bit", &fh_sadc_obj.activebit);
#else
		plat_data = pdev->dev.platform_data;
		if (!plat_data) {
			dev_err(&pdev->dev, "sadc get platform data error..\n");
			return -ENODEV;
		}
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(&pdev->dev, "sadc get platform source error..\n");
			return -ENODEV;
		}
		res = request_mem_region(res->start, resource_size(res), pdev->name);
		if (res == NULL) {
			dev_err(&pdev->dev, "sadc region already claimed\n");
			return -EBUSY;
		}
		fh_sadc_obj.refvol = plat_data->ref_vol;
		fh_sadc_obj.activebit = plat_data->active_bit;
		fh_sadc_obj.regs = ioremap(res->start, resource_size(res));
		if (fh_sadc_obj.regs == NULL) {
			err = -ENXIO;
			goto fail_no_ioremap;
		}
		irq = platform_get_irq(pdev, 0);
		irq = irq_create_mapping(NULL, irq);
		if (irq < 0) {
			dev_err(&pdev->dev, "no IRQ defined!\n");
			goto fail_no_ioremap;
		}
#endif

	fh_sadc_obj.irq_no = irq;
	init_completion(&fh_sadc_obj.done);
	mutex_init(&fh_sadc_obj.sadc_lock);
	fh_sadc_obj.active_channel_no = 0xffffffff;
	strncpy(fh_sadc_obj.isr_name, "sadc", sizeof("sadc"));
	err = request_irq(fh_sadc_obj.irq_no, fh_sadc_isr, 0,
			fh_sadc_obj.isr_name,
			&fh_sadc_obj);
	if (err) {
		dev_dbg(&pdev->dev, "request_irq failed, %d\n", err);
		err = -ENXIO;
		goto err_irq;
	}

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

	return 0;

misc_error:
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);

err_irq:
	iounmap(fh_sadc_obj.regs);

fail_no_ioremap:
#ifndef CONFIG_USE_OF
	release_mem_region(res->start, resource_size(res));
#endif
	return err;
}

static int __exit fh_sadc_remove(struct platform_device *pdev)
{

	struct resource *res;

	if (fh_sadc_obj.clk) {
		clk_disable_unprepare(fh_sadc_obj.clk);
		clk_put(fh_sadc_obj.clk);
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	misc_deregister(&fh_sadc_misc);
	proc_remove(fh_sadc_obj.proc_file);
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);
	iounmap(fh_sadc_obj.regs);
	release_mem_region(res->start, resource_size(res));
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
	u16 ad_raw_data;
	struct sadc_info sadc_info;
	switch (cmd) {
	case IOCTL_GET_SADC_DATA:
	{
		mutex_lock(&fh_sadc_obj.sadc_lock);
		if (copy_from_user((void *) &sadc_info, (void __user*) arg,
				sizeof(sadc_info))) {
			mutex_unlock(&fh_sadc_obj.sadc_lock);
			return -EFAULT;
		}

		fh_sadc_isr_read_data(&fh_sadc_obj, sadc_info.channel,\
			&ad_raw_data);
		sadc_info.sadc_data = ad_raw_data;
		if (put_user(sadc_info.sadc_data,
				(int __user *)(&((struct sadc_info *)arg)\
				->sadc_data))) {
			mutex_unlock(&fh_sadc_obj.sadc_lock);
			return -EFAULT;
		}
		mutex_unlock(&fh_sadc_obj.sadc_lock);
		break;
	}
	case IOCTL_GET_SINGLE_SADC_DATA:
	{
		mutex_lock(&fh_sadc_obj.sadc_lock);
		if (copy_from_user((void *) &sadc_info, (void __user *) arg,
				sizeof(sadc_info))) {
			mutex_unlock(&fh_sadc_obj.sadc_lock);
			return -EFAULT;
		}

		fh_sadc_isr_read_single_data(&fh_sadc_obj, sadc_info.channel,\
			&ad_raw_data);
		sadc_info.sadc_data = ad_raw_data;
		if (put_user(sadc_info.sadc_data,
				(int __user *)(&((struct sadc_info *)arg)\
				->sadc_data))) {
			mutex_unlock(&fh_sadc_obj.sadc_lock);
			return -EFAULT;
		}
		mutex_unlock(&fh_sadc_obj.sadc_lock);
		break;
	}
	case IOCTL_GET_HIT_DATA:
	{
		if (copy_from_user((void *) &sadc_info, (void __user*) arg,
				sizeof(sadc_info))) {
			return -EFAULT;
		}

		fh_sadc_isr_read_hit_data(&fh_sadc_obj, sadc_info.channel,\
			&ad_raw_data);
		sadc_info.sadc_data = ad_raw_data;
		if (put_user(sadc_info.sadc_data,
				(int __user *)(&((struct sadc_info *)arg)\
				->sadc_data))) {
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SADC_POWER_DOWN:
		fh_sadc_lowpwr_enable();
		break;

	default:
		break;
	}
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
		.name	= "fh_sadc",
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
