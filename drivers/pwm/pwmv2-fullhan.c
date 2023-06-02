#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/decompress/mm.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/interrupt.h>

#include <linux/uaccess.h>
#include "pwmv2-fullhan.h"

#define FH_PWM_DEBUG
#ifdef FH_PWM_DEBUG
#define PRINT_DBG(fmt,args...)  printk(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif

#define STATUS_INT			(1<<31)
#define STATUS_FINALL0			(1<<0)
#define STATUS_FINALL1			(1<<1)
#define STATUS_FINALL2			(1<<2)
#define STATUS_FINALL3			(1<<3)
#define STATUS_FINALL4			(1<<4)
#define STATUS_FINALL5			(1<<5)
#define STATUS_FINALL6			(1<<6)
#define STATUS_FINALL7			(1<<7)
#define STATUS_FINONCE0			(1<<8)
#define STATUS_FINONCE1			(1<<9)
#define STATUS_FINONCE2			(1<<10)
#define STATUS_FINONCE3			(1<<11)
#define STATUS_FINONCE4			(1<<12)
#define STATUS_FINONCE5			(1<<13)
#define STATUS_FINONCE6			(1<<14)
#define STATUS_FINONCE7			(1<<15)

#define OFFSET_PWM_BASE(n)		(0x100 + 0x100 * n)

#define OFFSET_PWM_GLOBAL_CTRL0		(0x000)
#define OFFSET_PWM_GLOBAL_CTRL1		(0x004)
#define OFFSET_PWM_GLOBAL_CTRL2		(0x008)
#define OFFSET_PWM_INT_ENABLE		(0x010)
#define OFFSET_PWM_INT_STATUS		(0x014)

#define OFFSET_PWM_CTRL(n)		(0x000 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG0(n)		(0x004 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG1(n)		(0x008 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG2(n)		(0x00c + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG3(n)		(0x010 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG4(n)		(0x014 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS0(n)		(0x020 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS1(n)		(0x024 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS2(n)		(0x028 + OFFSET_PWM_BASE(n))

struct fh_pwm_driver
{
	unsigned int irq;
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
	struct proc_dir_entry *proc_file;
};

struct fh_pwm_driver *fh_pwm_drv;

static void fh_pwm_output_mask(unsigned int mask)
{
	writel(mask, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
}

static void fh_pwm_output_enable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
	reg |= (1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
}

static void fh_pwm_output_disable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
	reg |= 1 << (8 + n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
	reg &= ~(1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
}

static void fh_pwm_config_enable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
	reg |= (1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
}

static void fh_pwm_config_disable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
	reg &= ~(1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
}

static void fh_pwm_shadow_enable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
	reg |= (1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
}

static void fh_pwm_shadow_disable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
	reg &= ~(1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
}

static void fh_pwm_interrupt_finishall_enable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
	reg |= (1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
}

static void fh_pwm_interrupt_finishall_disable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
	reg &= ~(1 << n);
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
}

static void fh_pwm_interrupt_finishonce_enable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
	reg |= (1 << (n + 8));
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
}

static void fh_pwm_interrupt_finishonce_disable(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
	reg &= ~(1 << (n + 8));
	writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
}

static unsigned int fh_pwm_interrupt_get_status(void)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);
	reg &= readl(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);

	return reg;
}

static void fh_pwm_interrupt_finishonce_clear(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);

	reg &= ~(1 << (n + 8));

	return writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);
}

static void fh_pwm_interrupt_finishall_clear(unsigned int n)
{
	unsigned int reg;

	reg = readl(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);

	reg &= ~(1 << n);

	return writel(reg, fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);
}

static void fh_pwm_set_config(struct fh_pwm_chip_data *chip_data)
{
	unsigned int clk_rate = clk_get_rate(fh_pwm_drv->clk);
	unsigned int ctrl = 0, period, duty, delay, phase;

	fh_pwm_config_disable(chip_data->id);

	period = chip_data->config.period_ns / (NSEC_PER_SEC / clk_rate);
	duty = chip_data->config.duty_ns / (NSEC_PER_SEC / clk_rate);
	delay = chip_data->config.delay_ns / (NSEC_PER_SEC / clk_rate);
	phase = chip_data->config.phase_ns / (NSEC_PER_SEC / clk_rate);

	if (period > 0x1ffffff) {
		pr_err("PWM: period exceed 24-bit\n");
		period = 0x1ffffff;
	}

	if (duty > 0x1ffffff) {
		pr_err("PWM: duty exceed 24-bit\n");
		duty = 0x1ffffff;
	}

	PRINT_DBG("set period: 0x%x\n", period);
	PRINT_DBG("set duty: 0x%x\n", duty);
	PRINT_DBG("set phase: 0x%x\n", phase);
	PRINT_DBG("set delay: 0x%x\n", delay);

	writel(period, fh_pwm_drv->base + OFFSET_PWM_CFG0(chip_data->id));
	writel(duty, fh_pwm_drv->base + OFFSET_PWM_CFG1(chip_data->id));
	writel(phase, fh_pwm_drv->base + OFFSET_PWM_CFG2(chip_data->id));
	writel(delay, fh_pwm_drv->base + OFFSET_PWM_CFG3(chip_data->id));
	writel(chip_data->config.pulses,
			fh_pwm_drv->base + OFFSET_PWM_CFG4(chip_data->id));

	if (chip_data->config.delay_ns)
		ctrl |= 1 << 3;

	if(!chip_data->config.pulses)
		ctrl |= 1 << 0;

	ctrl |= (chip_data->config.stop & 0x3) << 1;

	writel(ctrl, fh_pwm_drv->base + OFFSET_PWM_CTRL(chip_data->id));

	PRINT_DBG("set ctrl: 0x%x\n", ctrl);

	ctrl = readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);

	ctrl |= (chip_data->config.stop >> 4) << (8 + chip_data->id);

	writel(ctrl, fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);

	writel(chip_data->config.pulses,
		fh_pwm_drv->base + OFFSET_PWM_CFG4(chip_data->id));
	PRINT_DBG("set pulses: 0x%x\n", chip_data->config.pulses);

	if (chip_data->config.finish_once)
		fh_pwm_interrupt_finishonce_enable(chip_data->id);
	else
		fh_pwm_interrupt_finishonce_disable(chip_data->id);

	if (chip_data->config.finish_all)
		fh_pwm_interrupt_finishall_enable(chip_data->id);
	else
		fh_pwm_interrupt_finishall_disable(chip_data->id);

	if (chip_data->config.shadow_enable)
		fh_pwm_shadow_enable(chip_data->id);
	else
		fh_pwm_shadow_disable(chip_data->id);

	fh_pwm_config_enable(chip_data->id);

}

static void fh_pwm_get_config(struct fh_pwm_chip_data *chip_data)
{
	unsigned int clk_rate = clk_get_rate(fh_pwm_drv->clk);
	unsigned int ctrl = 0, period, duty, delay, phase, pulses,
			status0, status1, status2;

	period = readl(fh_pwm_drv->base + OFFSET_PWM_CFG0(chip_data->id));
	duty = readl(fh_pwm_drv->base + OFFSET_PWM_CFG1(chip_data->id));
	phase = readl(fh_pwm_drv->base + OFFSET_PWM_CFG2(chip_data->id));
	delay = readl(fh_pwm_drv->base + OFFSET_PWM_CFG3(chip_data->id));
	pulses = readl(fh_pwm_drv->base + OFFSET_PWM_CFG4(chip_data->id));
	ctrl = readl(fh_pwm_drv->base + OFFSET_PWM_CTRL(chip_data->id));
	status0 = readl(fh_pwm_drv->base + OFFSET_PWM_STATUS0(chip_data->id));
	status1 = readl(fh_pwm_drv->base + OFFSET_PWM_STATUS1(chip_data->id));
	status2 = readl(fh_pwm_drv->base + OFFSET_PWM_STATUS2(chip_data->id));


	PRINT_DBG("==============================\n");
	PRINT_DBG("pwm%d register config:\n", chip_data->id);
	PRINT_DBG("\t\tperiod: 0x%x\n", period);
	PRINT_DBG("\t\tduty: 0x%x\n", duty);
	PRINT_DBG("\t\tphase: 0x%x\n", phase);
	PRINT_DBG("\t\tdelay: 0x%x\n", delay);
	PRINT_DBG("\t\tpulses: 0x%x\n", pulses);
	PRINT_DBG("\t\tctrl: 0x%x\n", ctrl);
	PRINT_DBG("\t\tstatus0: 0x%x\n", status0);
	PRINT_DBG("\t\tstatus1: 0x%x\n", status1);
	PRINT_DBG("\t\tstatus2: 0x%x\n", status2);

	chip_data->config.period_ns = period * (NSEC_PER_SEC / clk_rate);
	chip_data->config.duty_ns = duty * (NSEC_PER_SEC / clk_rate);

	PRINT_DBG("\t\tclk_rate: %d\n", clk_rate);
	PRINT_DBG("\t\tconfig.period_ns: %d\n", chip_data->config.period_ns);
	PRINT_DBG("\t\tconfig.duty_ns: %d\n", chip_data->config.duty_ns);
	PRINT_DBG("==============================\n\n");

	chip_data->config.phase_ns = phase * (NSEC_PER_SEC / clk_rate);
	chip_data->config.delay_ns = delay * (NSEC_PER_SEC / clk_rate);
	chip_data->config.pulses = pulses;
	chip_data->config.stop = (ctrl >> 1) & 0x3;
	chip_data->config.percent = chip_data->config.duty_ns /
			(chip_data->config.period_ns / 100);

	chip_data->status.busy = (status2 >> 4) & 0x1;
	chip_data->status.error = (status2 >> 3) & 0x1;
	chip_data->status.total_cnt = status1;
	chip_data->status.done_cnt = status0;
}

int fh_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		  int duty_ns, int period_ns)
{
	struct fh_pwm_chip_data *chip_data;

	chip_data = kzalloc(sizeof(struct fh_pwm_chip_data), GFP_KERNEL);
	if (chip_data == NULL) {
		pr_err("pwm[%d], chip data malloc failed\n", pwm->hwpwm);
		return -ENOMEM;
	}

	chip_data->id = pwm->hwpwm;
	chip_data->config.duty_ns = duty_ns;
	chip_data->config.period_ns = period_ns;

	fh_pwm_set_config(chip_data);

	kfree(chip_data);

	return 0;
}

int fh_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	fh_pwm_output_enable(pwm->hwpwm);
	return 0;
}

void fh_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	fh_pwm_output_disable(pwm->hwpwm);
}

static int fh_pwm_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_pwm_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static long fh_pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct fh_pwm_chip_data *chip_data;
	unsigned int val;

	if (unlikely(_IOC_TYPE(cmd) != PWM_IOCTL_MAGIC)) {
		pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
			__func__, _IOC_TYPE(cmd), -ENOTTY);
		return -ENOTTY;
	}

	if (unlikely(_IOC_NR(cmd) > PWM_IOCTL_MAXNR)) {
		pr_err("%s: ERROR: incorrect cmd num %d (error: %d)\n",
			__func__, _IOC_NR(cmd), -ENOTTY);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE,
			(void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ,
			(void __user *)arg, _IOC_SIZE(cmd));

	if (ret) {
		pr_err("%s: ERROR: user space access is not permitted %d (error: %d)\n",
		       __func__, _IOC_NR(cmd), -EACCES);
		return -EACCES;
	}

	switch (cmd) {
	case SET_PWM_ENABLE:
		get_user(val, (unsigned int __user *)&arg);
		fh_pwm_output_enable(val);
		break;
	case ENABLE_PWM:
		chip_data = (struct fh_pwm_chip_data __user *)arg;
		ret = copy_from_user(
			fh_pwm_drv->chip.pwms[chip_data->id].chip_data,
			(struct fh_pwm_chip_data __user *)arg,
			sizeof(struct fh_pwm_chip_data));
		fh_pwm_output_enable(chip_data->id);
		break;
	case DISABLE_PWM:
		chip_data = (struct fh_pwm_chip_data __user *)arg;
		ret = copy_from_user(
			fh_pwm_drv->chip.pwms[chip_data->id].chip_data,
			(struct fh_pwm_chip_data __user *)arg,
			sizeof(struct fh_pwm_chip_data));
		fh_pwm_output_disable(chip_data->id);
		break;
	case SET_PWM_DUTY_CYCLE:
		chip_data = (struct fh_pwm_chip_data __user *)arg;
		ret = copy_from_user(
			fh_pwm_drv->chip.pwms[chip_data->id].chip_data,
			(struct fh_pwm_chip_data __user *)arg,
			sizeof(struct fh_pwm_chip_data));
		printk("ioctl: SET_PWM_DUTY_CYCLE, "
			"pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
			chip_data->id, chip_data->config.duty_ns,
			chip_data->config.period_ns);
		fh_pwm_set_config(chip_data);
		break;
	case GET_PWM_DUTY_CYCLE:
		chip_data = (struct fh_pwm_chip_data __user *)arg;
		ret = copy_from_user(
			fh_pwm_drv->chip.pwms[chip_data->id].chip_data,
			(struct fh_pwm_chip_data __user *)arg,
			sizeof(struct fh_pwm_chip_data));
		printk("ioctl: GET_PWM_DUTY_CYCLE, "
			"pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
			chip_data->id, chip_data->config.duty_ns,
			chip_data->config.period_ns);
		fh_pwm_get_config(chip_data);
		break;
	case SET_PWM_DUTY_CYCLE_PERCENT:
		chip_data = (struct fh_pwm_chip_data __user *)arg;
		ret = copy_from_user(
			fh_pwm_drv->chip.pwms[chip_data->id].chip_data,
			(struct fh_pwm_chip_data __user *)arg,
			sizeof(struct fh_pwm_chip_data));
		if(chip_data->config.percent > 100)
		{
			pr_err("ERROR: percentage is over 100\n");
			return -EIO;
		}
		chip_data->config.duty_ns = chip_data->config.period_ns *
				chip_data->config.percent / 100;
		printk("ioctl: SET_PWM_DUTY_CYCLE_PERCENT, "
			"pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
			chip_data->id, chip_data->config.duty_ns,
			chip_data->config.period_ns);
		fh_pwm_set_config(chip_data);
		break;
	}


	return ret;
}

static const struct file_operations fh_pwm_fops =
{
	.owner			= THIS_MODULE,
	.open			= fh_pwm_open,
	.release		= fh_pwm_release,
	.unlocked_ioctl		= fh_pwm_ioctl,
};

static struct miscdevice fh_pwm_misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fh_pwm_fops,
};

static const struct pwm_ops fh_pwm_ops =
{
	.config = fh_pwm_config,
	.enable = fh_pwm_enable,
	.disable = fh_pwm_disable,
	.owner = THIS_MODULE,
};


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

static ssize_t fh_pwm_proc_write(struct file *filp,
                                 const char *buf, size_t len, loff_t *off)
{
	int i;
	char message[64] = {0};
	char *const delim = ",";
	char *cur = message;
	char *param_str[8];
	unsigned int param[8];
	struct fh_pwm_chip_data *chip_data;

	len = (len > 64) ? 64 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 8; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n",
                               __func__, i);
			pr_err("id, switch_mask, duty_ns, period_ns, "
                               "numofpules, delay_ns, phase_ns, stop_status\n");
			pr_err("eg. echo '0,1,5000,10000,0,0,1000,0' > /proc/driver/pwm\n");
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
			param[i] = (unsigned int)simple_strtoul(param_str[i], NULL, 10);
			if (param[i] < 0)
			{
				pr_err("%s: ERROR: parameter[%d] is incorrect\n", __func__, i);
				pr_err("id, switch_mask, duty_ns, period_ns, numofpules, "
                                       "delay_ns, phase_ns, stop_status\n");
				pr_err("eg. echo '0,1,5000,10000,0,0,1000,0' > /proc/driver/pwm\n");
				return -EINVAL;
			}
		}
	}

	chip_data = kzalloc(sizeof(struct fh_pwm_chip_data), GFP_KERNEL);
	if (chip_data == NULL) {
		pr_err("pwm[%d], chip data malloc failed\n", i);
		return 0;
	}

	chip_data->id = param[0];
	chip_data->config.duty_ns = param[2];
	chip_data->config.period_ns = param[3];
	chip_data->config.pulses = param[4];
	chip_data->config.delay_ns = param[5];
	chip_data->config.phase_ns = param[6];
	chip_data->config.stop = param[7];

	fh_pwm_set_config(chip_data);

	printk("set pwm %d, enable: 0x%x, duty cycle: %u ns, period cycle: %u,"
			"numofpulse: %d, delay: %d ns, phase: %d ns, stop: %d\n",
			param[0], param[1], param[2], param[3],
					param[4], param[5], param[6], param[7]);

	fh_pwm_output_mask(param[1]);

	kfree(chip_data);

	return len;
}

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
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
	seq_printf(sfile, "\nPWM Status:\n");

	seq_printf(sfile, "global_ctrl0: 0x%x\n",
                   readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0));
	seq_printf(sfile, "global_ctrl1: 0x%x\n",
                   readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1));
	seq_printf(sfile, "global_ctrl2: 0x%x\n",
                   readl(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2));

	for (i = 0; i < fh_pwm_drv->chip.npwm; i++) {
		struct fh_pwm_chip_data *chip_data;

		chip_data = pwm_get_chip_data(&fh_pwm_drv->chip.pwms[i]);
		fh_pwm_get_config(chip_data);

		seq_printf(sfile, "id: %d \t%s, duty_ns: %u, period_ns: %u\n",
			   chip_data->id,
			   (chip_data->status.busy) ? "ENABLE" : "DISABLE",
			   chip_data->config.duty_ns,
			   chip_data->config.period_ns);
	}
	return 0;
}

static const struct seq_operations pwm_seq_ops =
{
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_pwm_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &pwm_seq_ops);
}


static struct file_operations fh_pwm_proc_ops =
{
	.owner	= THIS_MODULE,
	.open	= fh_pwm_proc_open,
	.read	= seq_read,
	.write	= fh_pwm_proc_write,
	.release = seq_release,
};

static irqreturn_t fh_pwm_interrupt(int this_irq, void *param)
{
	unsigned int status, stat_once, stat_all;
	struct fh_pwm_chip_data *chip_data;
	unsigned int irq;

	status = fh_pwm_interrupt_get_status();

	status &= 0xffff;

	stat_once = (status >> 8) & 0xff;
	stat_all = status & 0xff;

	if(stat_once) {
		irq = fls(stat_once);
		chip_data = pwm_get_chip_data(&fh_pwm_drv->chip.pwms[irq - 1]);
		if(chip_data && chip_data->finishonce_callback) {
			/* chip_data->finishonce_callback(chip_data); */
		} else {
			pr_err("callback is empty, status: 0x%x\n", status);
		}
		fh_pwm_interrupt_finishonce_clear(irq - 1);
		fh_pwm_interrupt_finishonce_disable(irq - 1);
		fh_pwm_interrupt_finishonce_enable(irq - 1);
	}

	if(stat_all) {
		irq = fls(stat_all);
		chip_data = pwm_get_chip_data(&fh_pwm_drv->chip.pwms[irq - 1]);
		if(chip_data && chip_data->finishonce_callback) {
			/* chip_data->finishall_callback(chip_data); */
		} else {
			pr_err("callback is empty, status: 0x%x\n", status);
		}
		fh_pwm_interrupt_finishall_clear(irq - 1);
		fh_pwm_interrupt_finishall_disable(irq - 1);
		fh_pwm_interrupt_finishall_enable(irq - 1);
	}

	return IRQ_HANDLED;
}

static int __devinit fh_pwm_probe(struct platform_device *pdev)
{
	int err, i;
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		err = -ENXIO;
		goto fail_no_mem_resource;
	}

	res = request_mem_region(res->start, resource_size(res), pdev->name);
	if (res == NULL) {
		err = -EBUSY;
		goto fail_no_mem_resource;
	}

	fh_pwm_drv = kzalloc(sizeof(struct fh_pwm_driver), GFP_KERNEL);

	fh_pwm_drv->base = ioremap(res->start, resource_size(res));
	if (fh_pwm_drv->base == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}

	fh_pwm_drv->clk = clk_get(&pdev->dev, "pwm_clk");

	if (IS_ERR(fh_pwm_drv->clk)) {
		err = PTR_ERR(fh_pwm_drv->clk);
		goto fail_no_clk;
	}

	clk_enable(fh_pwm_drv->clk);

	PRINT_DBG("%s: clk_rate: %lu\n", __func__, clk_get_rate(fh_pwm_drv->clk));

	err = platform_get_irq(pdev, 0);
	if (err < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		goto fail_no_clk;
	}

	fh_pwm_drv->irq = err;

	err = request_irq(fh_pwm_drv->irq,
                          fh_pwm_interrupt, IRQF_DISABLED, pdev->name, fh_pwm_drv);
	if (err) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", fh_pwm_drv->irq);
		goto fail_no_clk;
	}

	err = misc_register(&fh_pwm_misc);
	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed",
		       __func__, DEVICE_NAME);
		return -ENXIO;
	}

	fh_pwm_drv->chip.dev = &pdev->dev;
	fh_pwm_drv->chip.ops = &fh_pwm_ops;
	fh_pwm_drv->chip.base = pdev->id;
	fh_pwm_drv->chip.npwm = CONFIG_FH_PWM_NUM;

	err = pwmchip_add(&fh_pwm_drv->chip);
	if (err < 0) {
		pr_err("%s: ERROR: %s pwmchip_add failed",
		       __func__, DEVICE_NAME);
		return err;
	}

	for (i = 0; i < fh_pwm_drv->chip.npwm; i++) {
		struct fh_pwm_chip_data *chip_data;

		chip_data = kzalloc(sizeof(struct fh_pwm_chip_data), GFP_KERNEL);
		if (chip_data == NULL) {
			pr_err("pwm[%d], chip data malloc failed\n", i);
			continue;
		}

		chip_data->id = i;

		pwm_set_chip_data(&fh_pwm_drv->chip.pwms[i], chip_data);
	}

	fh_pwm_output_mask(0);

	platform_set_drvdata(pdev, fh_pwm_drv);

	pr_info("PWM driver, Number: %d, IO base addr: 0x%p\n",
			fh_pwm_drv->chip.npwm, fh_pwm_drv->base);

	fh_pwm_drv->proc_file = create_proc_entry(FH_PWM_PROC_FILE, 0644, NULL);

	if (fh_pwm_drv->proc_file)
		fh_pwm_drv->proc_file->proc_fops = &fh_pwm_proc_ops;
	else
		pr_err("%s: ERROR: %s proc file create failed",
		       __func__, DEVICE_NAME);

	dev_dbg(&pdev->dev, "PWM probe successful, IO base addr: %p\n",
		fh_pwm_drv->base);
	return 0;

fail_no_clk:
	iounmap(fh_pwm_drv->base);
fail_no_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_no_mem_resource:
	return err;
}

static int __exit fh_pwm_remove(struct platform_device *pdev)
{
	int err, i;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	for (i = 0; i < fh_pwm_drv->chip.npwm; i++)
		kfree(fh_pwm_drv->chip.pwms[i].chip_data);

	err = pwmchip_remove(&fh_pwm_drv->chip);
	if (err < 0)
		return err;

	dev_dbg(&pdev->dev, "pwm driver removed\n");

	fh_pwm_output_mask(0);
	clk_disable(fh_pwm_drv->clk);
	free_irq(fh_pwm_drv->irq, NULL);
	iounmap(fh_pwm_drv->base);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	misc_deregister(&fh_pwm_misc);

	free(fh_pwm_drv);
	fh_pwm_drv = NULL;

	return 0;
}

static struct platform_driver fh_pwm_driver =
{
	.driver	=
	{
		.name = DEVICE_NAME,
		.owner = THIS_MODULE,
	},
	.probe		= fh_pwm_probe,
	.remove		= __exit_p(fh_pwm_remove),
};

static int __init fh_pwm_init(void)
{
	return platform_driver_register(&fh_pwm_driver);
}

static void __exit fh_pwm_exit(void)
{

	platform_driver_unregister(&fh_pwm_driver);

}

module_init(fh_pwm_init);
module_exit(fh_pwm_exit);


MODULE_AUTHOR("fullhan");

MODULE_DESCRIPTION("FH PWM driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);
