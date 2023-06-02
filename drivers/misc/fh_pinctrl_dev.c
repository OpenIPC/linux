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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>
#include "fh_pinctrl_dev.h"

#define FH_PINCTRL_PROC_FILE "driver/pinctrl"

#undef  FH_PINCTRL_DEBUG
#ifdef FH_PINCTRL_DEBUG
#define PRINT_DBG(fmt,args...)  printk(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif

struct proc_dir_entry *pinctrl_proc_file;

static int fh_pinctrl_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_pinctrl_release(struct inode *inode, struct file *filp)
{
	return 0;
}


static long fh_pinctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	if (unlikely(_IOC_TYPE(cmd) != PINCTRL_IOCTL_MAGIC))
	{
		pr_err("%s: ERROR: incorrect magic num %d (error: %d)\n",
			   __func__, _IOC_TYPE(cmd), -ENOTTY);
		return -ENOTTY;
	}

	if (unlikely(_IOC_NR(cmd) > PINCTRL_IOCTL_MAXNR))
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

	}

	return ret;
}

static const struct file_operations fh_pinctrl_fops =
{
	.owner 			= THIS_MODULE,
	.open 			= fh_pinctrl_open,
	.release 		= fh_pinctrl_release,
	.unlocked_ioctl 	= fh_pinctrl_ioctl,
};

static struct miscdevice fh_pinctrl_misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &fh_pinctrl_fops,
};

static void del_char(char* str,char ch)
{
	char *p = str;
	char *q = str;
	while(*q)
	{
		if (*q !=ch)
		{
			*p++ = *q;
		}
		q++;
	}
	*p='\0';
}

static ssize_t fh_pinctrl_proc_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
	int i;
	char message[32] = {0};
	char * const delim = ",";
	char *cur = message;
	char* param_str[4];
	unsigned int param[4];

	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for(i=0; i<4; i++)
	{
		param_str[i] = strsep(&cur, delim);
		if(!param_str[i])
		{
			pr_err("%s: ERROR: parameter[%d] is empty\n", __func__, i);
			pr_err("[dev/mux], [dev name], [mux name], [func sel]\n");
			return -EINVAL;
		}
		else
		{
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}

        if(!strcmp(param_str[0], "dev"))
        {
        	fh_pinctrl_sdev(param_str[1], 0);
        }
        else if(!strcmp(param_str[0], "mux"))
        {
		param[3] = (u32)simple_strtoul(param_str[3], NULL, 10);
		if(param[3] < 0)
		{
			pr_err("ERROR: parameter[3] is incorrect\n");
			return -EINVAL;
		}
        	fh_pinctrl_smux(param_str[1], param_str[2], param[3], 0);
        }
        else
        {
		pr_err("ERROR: parameter[0] is incorrect\n"
			"[dev/mux], [dev name], [mux name], [func sel]\n");
		return -EINVAL;
        }

	return len;
}

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
	if (*pos == 0)
		return &counter;
	else
	{
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

	fh_pinctrl_prt(sfile);
	return 0;
}

static const struct seq_operations isp_seq_ops =
{
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_pinctrl_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &isp_seq_ops);
}

static struct file_operations fh_pinctrl_proc_ops =
{
	.owner = THIS_MODULE,
	.open = fh_pinctrl_proc_open,
	.read = seq_read,
	.write = fh_pinctrl_proc_write,
	.release = seq_release,
};


static int __devinit fh_pinctrl_probe(struct platform_device *pdev)
{
	int err;

	err = misc_register(&fh_pinctrl_misc);

	if(err < 0)
	{
		pr_err("%s: ERROR: %s registration failed",
			   __func__, DEVICE_NAME);
		return -ENXIO;
	}

	pinctrl_proc_file = create_proc_entry(FH_PINCTRL_PROC_FILE, 0644, NULL);

	if (pinctrl_proc_file)
		pinctrl_proc_file->proc_fops = &fh_pinctrl_proc_ops;
	else
		pr_err("%s: ERROR: %s proc file create failed",
				__func__, DEVICE_NAME);

	return 0;
}

static int __exit fh_pinctrl_remove(struct platform_device *pdev)
{
	misc_deregister(&fh_pinctrl_misc);
	return 0;
}

static struct platform_driver fh_pinctrl_driver =
{
	.driver	=
	{
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe 		= fh_pinctrl_probe,
	.remove		= __exit_p(fh_pinctrl_remove),
};

static int __init fh_pinctrl_dev_init(void)
{
	return platform_driver_register(&fh_pinctrl_driver);
}

static void __exit fh_pinctrl_dev_exit(void)
{

	platform_driver_unregister(&fh_pinctrl_driver);

}

module_init(fh_pinctrl_dev_init);
module_exit(fh_pinctrl_dev_exit);


MODULE_AUTHOR("fullhan");

MODULE_DESCRIPTION("FH PINCTRL driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(MISC_DYNAMIC_MINOR);
