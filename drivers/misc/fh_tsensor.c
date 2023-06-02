#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fh_rtc_v2.h>

#define DEVICE_NAME "fh_tsensor"

#define TSENSOR_IOCTL_MAGIC		't'
#define TSENSOR_GET_TEMP 		_IOWR(TSENSOR_IOCTL_MAGIC, 1, int)
#define TSENSOR_GET_DATA 		_IOWR(TSENSOR_IOCTL_MAGIC, 2, int)

static const struct file_operations fh_tsensor_fops;

static struct miscdevice fh_tsensor_miscdev = {
	.fops = &fh_tsensor_fops,
	.name = DEVICE_NAME,
	.minor = MISC_DYNAMIC_MINOR,
};

static void create_proc(void);
static void remove_proc(void);

long fh_tsensor_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int data, temp;

	if (unlikely(_IOC_TYPE(cmd) != TSENSOR_IOCTL_MAGIC))
		return -ENOTTY;

	switch (cmd)
	{
	case TSENSOR_GET_TEMP:
		if (fh_rtc_get_tsensor_data(&data, &temp))
			return -EIO;
		if (copy_to_user((void __user *)arg, &temp, sizeof(temp)))
			return -EIO;
		break;
	case TSENSOR_GET_DATA:
		if (fh_rtc_get_tsensor_data(&data, &temp))
			return -EIO;
		if (copy_to_user((void __user *)arg, &data, sizeof(data)))
			return -EIO;
		break;
	default:
		return -ENOTTY;
	}

	return 0;
}

static int fh_tsensor_probe(struct platform_device *pdev)
{
	create_proc();
	return misc_register(&fh_tsensor_miscdev);
}

static int fh_tsensor_remove(struct platform_device *pdev)
{
	remove_proc();
	misc_deregister(&fh_tsensor_miscdev);
	return 0;
}

int fh_rtc_get_tsensor_data(int *data, int *temp);

static int fh_tsensor_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static int fh_tsensor_close(struct inode *ip, struct file *fp)
{
	return 0;
}

static const struct file_operations fh_tsensor_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.unlocked_ioctl = fh_tsensor_ioctl,
	.release = fh_tsensor_close,
	.open = fh_tsensor_open,
};

static const struct of_device_id fh_tsensor_of_match[] = {
	{.compatible = "fh,fh-tsensor",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_tsensor_of_match);

static struct platform_driver fh_tsensor_driver = {
	.driver = {
		.name = DEVICE_NAME,
		.of_match_table = fh_tsensor_of_match,
	},
	.probe = fh_tsensor_probe,
	.remove = fh_tsensor_remove,
};

module_platform_driver(fh_tsensor_driver);

MODULE_AUTHOR("Fullhan");
MODULE_DESCRIPTION("Fullhan tsensor device driver");
MODULE_LICENSE("GPL");

/****************************debug proc*****************************/

#define PROC_NAME "driver/"DEVICE_NAME
static struct proc_dir_entry *proc_file;

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter;
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
	int data, temp, ret;

	ret = fh_rtc_get_tsensor_data(&data, &temp);
	if (ret) {
		seq_printf(sfile, "get_tsensor_data error %d\n", ret);
		return 0;
	}

	seq_printf(sfile, "raw data: %d\n", data);
	seq_printf(sfile, "temp    : %d.%01d\n", temp / 10, abs(temp % 10));

	return 0;
}

static const struct seq_operations fh_dwi2s_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &fh_dwi2s_seq_ops);
}

static struct file_operations fh_tsensor_proc_ops =
{
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
};

static void create_proc(void)
{
	proc_file = proc_create(PROC_NAME, 0644, NULL, &fh_tsensor_proc_ops);

	if (proc_file == NULL)
		pr_err("%s: ERROR: %s proc file create failed",
			   __func__, DEVICE_NAME);
}

static void remove_proc(void)
{
	remove_proc_entry(PROC_NAME, NULL);
}
