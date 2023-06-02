#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/clk.h>

#include "fh_clk_miscdev.h"

//#define FH_CLK_DEBUG

#if defined(FH_CLK_DEBUG)
#define PRINT_CLK_DBG(fmt, args...)   \
    do                                \
    {                                 \
        printk("FH_CLK_DEBUG: "); \
        printk(fmt, ##args);      \
    } while (0)
#else
#define PRINT_CLK_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

static int fh_clk_miscdev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_clk_miscdev_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static long fh_clk_miscdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -ENODEV;
	struct clk *clk;
	struct clk_usr uclk;

	if (unlikely(_IOC_TYPE(cmd) != CLK_IOCTL_MAGIC))
	{
		pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
			   __func__, _IOC_TYPE(cmd), -ENOTTY);
		return -ENOTTY;
	}

	if (unlikely(_IOC_NR(cmd) > CLK_IOCTL_MAXNR))
	{
		pr_err("%s: ERROR: incorrect cmd num %d (error: %d)\n",
			   __func__, _IOC_NR(cmd), -ENOTTY);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
	{
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	}
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
	{
		ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if(ret)
	{
		pr_err("%s: ERROR: user space access is not permitted %d (error: %d)\n",
			   __func__, _IOC_NR(cmd), -EACCES);
		return -EACCES;
	}

	switch(cmd)
	{
	case ENABLE_CLK:
		if (copy_from_user((void *)&uclk,
				(void __user *)arg,
				sizeof(struct clk_usr)))
			return -EFAULT;
		clk = clk_get(NULL, uclk.name);
		if(!IS_ERR(clk)) {
			clk_enable(clk);
			ret = 0;
		}
		break;
	case DISABLE_CLK:
		if (copy_from_user((void *)&uclk,
				(void __user *)arg,
				sizeof(struct clk_usr)))
			return -EFAULT;
		clk = clk_get(NULL, uclk.name);
		if (!IS_ERR(clk)) {
			clk_disable(clk);
			ret = 0;
		}
		break;
	case SET_CLK_RATE:
		if (copy_from_user((void *)&uclk,
				(void __user *)arg,
				sizeof(struct clk_usr)))
			return -EFAULT;
		clk = clk_get(NULL, uclk.name);
		ret = PTR_ERR(clk);
		if (!IS_ERR(clk))
			ret = clk_set_rate(clk, uclk.frequency);

		PRINT_CLK_DBG("%s, set clk: %s, rate: %lu\n",
                              __func__, uclk.name, uclk.frequency);
		break;
	case GET_CLK_RATE:
		if (copy_from_user((void *)&uclk,
				(void __user *)arg,
				sizeof(struct clk_usr)))
			return -EFAULT;
		clk = clk_get(NULL, uclk.name);
		ret = PTR_ERR(clk);
		if (!IS_ERR(clk)) {
			uclk.frequency = clk_get_rate(clk);
			ret = 0;
		}
		PRINT_CLK_DBG("%s, get clk: %s, rate: %lu\n",
                              __func__, uclk.name, uclk.frequency);
	}


	return ret;
}

static const struct file_operations fh_clk_fops =
{
	.owner 			= THIS_MODULE,
	.open 			= fh_clk_miscdev_open,
	.release 		= fh_clk_miscdev_release,
	.unlocked_ioctl 	= fh_clk_miscdev_ioctl,
};

static struct miscdevice fh_clk_miscdev =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fh_clk_fops,
};

int __init fh_clk_miscdev_init(void)
{
	int err;

	err = misc_register(&fh_clk_miscdev);

	if(err < 0)
	{
		pr_err("%s: ERROR: %s registration failed, ret=%d",
			   __func__, DEVICE_NAME, err);
		return -ENXIO;
	}

	pr_info("CLK misc driver init successfully\n");
	return 0;
}


static void __exit fh_clk_miscdev_exit(void)
{
    misc_deregister(&fh_clk_miscdev);
}
module_init(fh_clk_miscdev_init);
module_exit(fh_clk_miscdev_exit);

MODULE_AUTHOR("QIN");
MODULE_DESCRIPTION("Misc Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: FH");
