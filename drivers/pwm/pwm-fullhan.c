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

#include <asm/uaccess.h>
#include "pwm-fullhan.h"

#undef  FH_PWM_DEBUG
#ifdef FH_PWM_DEBUG
#define PRINT_DBG(fmt,args...)  printk(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif

struct fh_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *clk;
	struct proc_dir_entry *proc_file;
};

struct fh_pwm_chip fh_pwm = {

};


static int pwm_get_duty_cycle_ns(struct fh_pwm_chip_data *chip_data)
{
	u32 reg, period, duty;
	u32 clk_rate = clk_get_rate(fh_pwm.clk);

	reg = readl(fh_pwm.base + REG_PWM_CMD(chip_data->id));
	period = reg & 0x0fff;
	duty = (reg >> 16) & 0xfff;
	duty = period - duty;       //reverse duty cycle

	if (period == 0)
		period = duty;

	chip_data->counter_ns = duty * 1000000000 / clk_rate;
	chip_data->period_ns = period * 1000000000 / clk_rate;

	PRINT_DBG("get duty: %d, period: %d, reg: 0x%x\n", duty, period, reg);

	return 0;
}

static int pwm_set_duty_cycle_ns(struct fh_pwm_chip_data *chip_data)
{
	u32 period, duty, reg, clk_rate, duty_revert;
	clk_rate = clk_get_rate(fh_pwm.clk);
	if (!clk_rate) {
		pr_err("PWM: clock rate is 0\n");
		return -EINVAL;
	}
	period = chip_data->period_ns / (1000000000 / clk_rate);

	if (period < 8) {
		pr_err("PWM: min period is 8\n");
		return -EINVAL;
	}

	duty = chip_data->counter_ns / (1000000000 / clk_rate);

	if (period < duty) {
		pr_err("PWM: period < duty\n");
		return -EINVAL;
	}

	duty_revert = period - duty;

	if (duty == period)
		reg = (duty & 0xfff) << 16 | (0 & 0xfff);
	else
		reg = (duty_revert & 0xfff) << 16 | (period & 0xfff);

	PRINT_DBG("set duty_revert: %d, period: %d, reg: 0x%x\n", duty_revert, period, reg);

	writel(reg, fh_pwm.base + REG_PWM_CMD(chip_data->id));
	return 0;
}


static int pwm_set_duty_cycle_percent(struct fh_pwm_chip_data *chip_data)
{
	u32 period, duty, reg, clk_rate, duty_revert;
	clk_rate = clk_get_rate(fh_pwm.clk);
	if (!clk_rate) {
		pr_err("PWM: clock rate is 0\n");
		return -EINVAL;
	}

	if (chip_data->percent > 100 || chip_data->percent < 0) {
		pr_err("PWM: pwm->percent is out of range\n");
		return -EINVAL;
	}

	period = chip_data->period_ns / (1000000000 / clk_rate);

	if (period < 8) {
		pr_err("PWM: min period is 8\n");
		return -EINVAL;
	}

	duty = period * 100 / chip_data->percent;

	if (period < duty) {
		pr_err("PWM: period < duty\n");
		return -EINVAL;
	}

	duty_revert = period - duty;

	if (duty == period)
		reg = (duty & 0xfff) << 16 | (0 & 0xfff);
	else
		reg = (duty_revert & 0xfff) << 16 | (period & 0xfff);

	PRINT_DBG("set duty_revert: %d, period: %d, reg: 0x%x\n", duty_revert, period, reg);

	writel(reg, fh_pwm.base + REG_PWM_CMD(chip_data->id));
	return 0;
}


int fh_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
		  int duty_ns, int period_ns)
{
	struct fh_pwm_chip_data *chip_data;

	chip_data = pwm_get_chip_data(pwm);
	if (!chip_data) {
		pr_err("%s: ERROR: PWM %d does NOT exist\n",
		       __func__, pwm->hwpwm);
		return -ENXIO;
	}
	chip_data->counter_ns = duty_ns;
	chip_data->period_ns = period_ns;
	pwm_set_duty_cycle_ns(chip_data);
	return 0;
}

static int _fh_pwm_enable(struct fh_pwm_chip_data *chip_data)
{
	int i;
	unsigned int reg = 0;
	chip_data->working = 1;

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		chip_data = pwm_get_chip_data(&fh_pwm.chip.pwms[i]);
		reg |= chip_data->working << i;
	}

	writel(reg, fh_pwm.base + REG_PWM_CTRL);
	return 0;
}

int fh_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct fh_pwm_chip_data *chip_data;

	chip_data = pwm_get_chip_data(pwm);
	if (!chip_data) {
		pr_err("%s: ERROR: PWM %d does NOT exist\n",
		       __func__, pwm->hwpwm);
		return -ENXIO;
	}

	_fh_pwm_enable(chip_data);

	return 0;
}

static int _fh_pwm_disable(struct fh_pwm_chip_data *chip_data)
{
	int i;
	unsigned int reg = 0;
	chip_data->working = 0;

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		chip_data = pwm_get_chip_data(&fh_pwm.chip.pwms[i]);
		reg |= chip_data->working << i;
	}

	writel(reg, fh_pwm.base + REG_PWM_CTRL);
	return 0;
}

void fh_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct fh_pwm_chip_data *chip_data;

	chip_data = pwm_get_chip_data(pwm);
	if (!chip_data) {
		pr_err("%s: ERROR: PWM %d does NOT exist\n",
		       __func__, pwm->hwpwm);
		return;
	}

	_fh_pwm_disable(chip_data);
}

static int fh_pwm_open(struct inode *inode, struct file *file)
{
	int i;
	struct fh_pwm_chip_data *chip_data;
	struct pwm_device *pwm;

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		pwm = &fh_pwm.chip.pwms[i];

		if (!pwm) {
			pr_err("%s: ERROR: PWM %d does NOT exist\n",
			       __func__, i);
			return -ENXIO;
		}
		chip_data = pwm_get_chip_data(pwm);
		if (!chip_data)
			chip_data = kzalloc(sizeof(*chip_data), GFP_KERNEL);

		chip_data->id = pwm->hwpwm;
		chip_data->working = 0;
		pwm->chip_data = chip_data;
	}
	return 0;
}

static int fh_pwm_release(struct inode *inode, struct file *filp)
{
	int i;
	struct fh_pwm_chip_data *chip_data;
	struct pwm_device *pwm;

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		pwm = &fh_pwm.chip.pwms[i];

		if (!pwm) {
			pr_err("%s: ERROR: PWM %d does NOT exist\n",
			       __func__, i);
			return -ENOTTY;
		}
		chip_data = pwm_get_chip_data(pwm);
		free(chip_data);
		pwm_set_chip_data(pwm, NULL);
	}
	return 0;
}


static long fh_pwm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct fh_pwm_chip_data *pwm;

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
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (ret) {
		pr_err("%s: ERROR: user space access is not permitted %d (error: %d)\n",
		       __func__, _IOC_NR(cmd), -EACCES);
		return -EACCES;
	}

	switch (cmd) {
	case ENABLE_PWM:
		pwm = (struct fh_pwm_chip_data __user *)arg;
		_fh_pwm_enable(pwm);
		break;
	case DISABLE_PWM:
		pwm = (struct fh_pwm_chip_data __user *)arg;
		_fh_pwm_disable(pwm);
		break;
	case SET_PWM_DUTY_CYCLE:
		pwm = (struct fh_pwm_chip_data __user *)arg;
		printk("ioctl: pwm addr: %p, pwm->period: %d ns\n", pwm, pwm->period_ns);
		pwm_set_duty_cycle_ns(pwm);
		break;
	case GET_PWM_DUTY_CYCLE:
		pwm = (struct fh_pwm_chip_data __user *)arg;
		printk("ioctl: pwm->id: %d, pwm->counter: %d, pwm->period: %d\n", pwm->id, pwm->counter_ns,
		       pwm->period_ns);
		pwm_get_duty_cycle_ns(pwm);
		break;
	case SET_PWM_DUTY_CYCLE_PERCENT:
		pwm = (struct fh_pwm_chip_data __user *)arg;
		printk("ioctl: pwm->id: %d, pwm->counter: %d, pwm->period: %d\n", pwm->id, pwm->counter_ns,
		       pwm->period_ns);
		pwm_set_duty_cycle_percent(pwm);
		break;
	}


	return ret;
}

static const struct file_operations fh_pwm_fops = {
	.owner 			= THIS_MODULE,
	.open 			= fh_pwm_open,
	.release 		= fh_pwm_release,
	.unlocked_ioctl = fh_pwm_ioctl,
};

static struct miscdevice fh_pwm_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fh_pwm_fops,
};

static const struct pwm_ops fh_pwm_ops = {
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

static ssize_t fh_pwm_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	int i;
	char message[32] = {0};
	char *const delim = ",";
	char *cur = message;
	char *param_str[4];
	unsigned int param[4];
	struct fh_pwm_chip_data *chip_data;

	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 4; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n", __func__, i);
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
			param[i] = (u32)simple_strtoul(param_str[i], NULL, 10);
			if (param[i] < 0) {
				pr_err("%s: ERROR: parameter[%d] is incorrect\n", __func__, i);
				return -EINVAL;
			}
		}
	}

	printk("set pwm %d to %s, duty cycle: %u ns, period cycle: %u\n", param[0],
	       param[1] ? "enable" : "disable", param[2], param[3]);
	chip_data = pwm_get_chip_data(&fh_pwm.chip.pwms[param[0]]);
	chip_data->counter_ns = param[2];
	chip_data->period_ns = param[3];

	param[1] ? fh_pwm_enable(&fh_pwm.chip, &fh_pwm.chip.pwms[param[0]]) : fh_pwm_disable(&fh_pwm.chip,
			&fh_pwm.chip.pwms[param[0]]);
	pwm_set_duty_cycle_ns(chip_data);

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

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		struct fh_pwm_chip_data *chip_data;

		chip_data = pwm_get_chip_data(&fh_pwm.chip.pwms[i]);
		seq_printf(sfile, "id: %d \t%s, duty_ns: %u, period_ns: %u\n",
			   chip_data->id,
			   (chip_data->working) ? "ENABLE" : "DISABLE",
			   chip_data->counter_ns,
			   chip_data->period_ns);
	}
	return 0;
}

static const struct seq_operations isp_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_pwm_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &isp_seq_ops);
}


static struct file_operations fh_pwm_proc_ops = {
	.owner = THIS_MODULE,
	.open = fh_pwm_proc_open,
	.read = seq_read,
	.write = fh_pwm_proc_write,
	.release = seq_release,
};

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

	fh_pwm.base = ioremap(res->start, resource_size(res));
	if (fh_pwm.base == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}

	fh_pwm.clk = clk_get(&pdev->dev, "pwm_clk");

	if (IS_ERR(fh_pwm.clk)) {
		err = PTR_ERR(fh_pwm.clk);
		goto fail_no_clk;
	}

	clk_enable(fh_pwm.clk);

	err = misc_register(&fh_pwm_misc);
	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed",
		       __func__, DEVICE_NAME);
		return -ENXIO;
	}

	fh_pwm.chip.dev = &pdev->dev;
	fh_pwm.chip.ops = &fh_pwm_ops;
	fh_pwm.chip.base = pdev->id;
	fh_pwm.chip.npwm = CONFIG_FH_PWM_NUM;

	err = pwmchip_add(&fh_pwm.chip);
	if (err < 0) {
		pr_err("%s: ERROR: %s pwmchip_add failed",
		       __func__, DEVICE_NAME);
		return err;
	}

	for (i = 0; i < fh_pwm.chip.npwm; i++) {
		struct fh_pwm_chip_data *chip_data;

		chip_data = kzalloc(sizeof(struct fh_pwm_chip_data), GFP_KERNEL);
		if (chip_data == NULL) {
			pr_err("pwm[%d], chip data malloced failed\n", i);
			continue;
		}

		chip_data->id = i;
		chip_data->working = 0;

		pwm_set_chip_data(&fh_pwm.chip.pwms[i], chip_data);
	}

	platform_set_drvdata(pdev, &fh_pwm);

	/* disable pwm at startup. Avoids zero value. */
	writel(0x0, fh_pwm.base + REG_PWM_CTRL);

	pr_info("PWM driver, Number: %d, IO base addr: 0x%p\n",
		fh_pwm.chip.npwm, fh_pwm.base);

	fh_pwm.proc_file = create_proc_entry(FH_PWM_PROC_FILE, 0644, NULL);

	if (fh_pwm.proc_file)
		fh_pwm.proc_file->proc_fops = &fh_pwm_proc_ops;
	else
		pr_err("%s: ERROR: %s proc file create failed",
		       __func__, DEVICE_NAME);

	dev_dbg(&pdev->dev, "PWM probe successful, IO base addr: %p\n",
		fh_pwm.base);
	return 0;

fail_no_clk:
	iounmap(fh_pwm.base);
fail_no_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_no_mem_resource:
	return err;
}

static int __exit fh_pwm_remove(struct platform_device *pdev)
{
	int err, i;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	for (i = 0; i < fh_pwm.chip.npwm; i++)
		kfree(fh_pwm.chip.pwms[i].chip_data);

	err = pwmchip_remove(&fh_pwm.chip);
	if (err < 0)
		return err;

	dev_dbg(&pdev->dev, "pwm driver removed\n");

	writel(0x0, fh_pwm.base + REG_PWM_CTRL);
	clk_disable(fh_pwm.clk);

	iounmap(fh_pwm.base);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	misc_deregister(&fh_pwm_misc);
	return 0;
}

static struct platform_driver fh_pwm_driver = {
	.driver	=
	{
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe 		= fh_pwm_probe,
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
