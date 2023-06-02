#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <mach/pwm_timer.h>
#include <mach-anyka/anyka_types.h>

#define AK_PWM_SELET_DEV	0x0600	
#define AK_PWM_SET			0x0601	

struct ak_duty{
   int numerator;
   int denominator;
};

struct ak_pwm_cycle {
	int frq;
	struct ak_duty duty;
};

struct ak_pwm_data_st {
	struct ak_pwm_timer *pwm;
	struct ak_pwm_cycle ak_pwm_value;
};


static int ak_pwm_open(struct inode *inode, struct file *file)
{
	int ret=0;
	struct ak_pwm_data_st *ak_pwm;
	ak_pwm = kmalloc(sizeof(struct ak_pwm_data_st), GFP_KERNEL);
	if(!ak_pwm) {
		printk(KERN_ERR "%s alloc the priv device fail.\n", __func__);
		ret = -ENOMEM;
	}
    memset(ak_pwm,0,sizeof(struct ak_pwm_data_st));
	ak_pwm->ak_pwm_value.duty.denominator = 100;
    ak_pwm->ak_pwm_value.duty.numerator = 1;
	ak_pwm->ak_pwm_value.frq = 92;
	file->private_data = ak_pwm;
	return ret;
}

static int ak_pwm_close(struct inode *inode, struct file *file)
{
	struct ak_pwm_data_st *ak_pwm = (struct ak_pwm_data_st *)file->private_data;

	if(ak_pwm->pwm){
		ak_timer_disable(ak_pwm->pwm);
		ak_pwm_disable(ak_pwm->pwm);
		ak_pwm_release(ak_pwm->pwm);
	}
	
	if(ak_pwm)
		kfree(ak_pwm);
	
	return 0;
}

static long ak_pwm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret=0;
	struct ak_pwm_data_st *ak_pwm = (struct ak_pwm_data_st *)file->private_data;
    struct ak_pwm_timer *pwm_st;
	switch (cmd){
		case AK_PWM_SELET_DEV:
			{
				/*
				int pwm_id;
				ret = copy_from_user(&pwm_id, (void *)arg, sizeof(long));
				if (ret) {
					printk(KERN_ERR "%s copy failed\n", __func__);
					return -1;
				}
				*/
				pwm_st = ak_pwm_request(arg);
				if(!pwm_st){
					printk(KERN_ERR"pwm[%d] is working now. request fail.\n", (int)arg);
					return -1;
				}
                ak_pwm->pwm = pwm_st;
			}
			break;

		case AK_PWM_SET:
			{
				int i = 0;
                unsigned int tmp = 0;
				unsigned int high=0; 
                unsigned int low = 0;
                unsigned int hl = 0;
				struct ak_pwm_cycle bb;
				ret = copy_from_user(&bb, (void *)arg, sizeof(struct ak_pwm_cycle));
				if (ret) {
					printk(KERN_ERR "%s copy failed\n", __func__);
					return -1;
				}
				ak_pwm->ak_pwm_value.frq = bb.frq;
				ak_pwm->ak_pwm_value.duty.numerator = bb.duty.numerator;
				ak_pwm->ak_pwm_value.duty.denominator = bb.duty.denominator;
                
				tmp = REAL_CRYSTAL_FREQ / ak_pwm->ak_pwm_value.frq;
				for (i = 1; i<=AK_PWM_TIMER_PRE_DIV_MAX; i++) {
					 hl = tmp / i;
					//round off for high
					high = hl * ak_pwm->ak_pwm_value.duty.numerator + ak_pwm->ak_pwm_value.duty.denominator / 2;
                    high /= ak_pwm->ak_pwm_value.duty.denominator;
                    if(high >= 1 && high < hl)
                        ;
                    else if(high >= hl)
                        high = hl - 1;
                    else
                        high = 1;
                    
					low = hl - high;
					if ((high <= 65535) && (low <= 65535))
						break;
				}
				
				//printk(KERN_ERR "duty_num:%u,duty_den:%u, div:%u, high:%u, low:%u\n", ak_pwm->ak_pwm_value.duty.numerator,ak_pwm->ak_pwm_value.duty.denominator, i, high, low);
				
				ret = ak_pwm_config(ak_pwm->pwm, high - 1, low - 1, ak_pwm->ak_pwm_value.frq);
				if(ret){
					printk("pwm set err! %d <= frq <= %d\n", PWM_MIN_FREQ, PWM_MAX_FREQ);
					return -1;
				}
				ak_pwm_enable(ak_pwm->pwm);
				ak_timer_enable(ak_pwm->pwm);
				
			}
			break;
			
		default:
			break;
		
	}
	return ret;
}

static struct file_operations ak_pwm_fops = {
	.owner = THIS_MODULE,
	.open = ak_pwm_open,
	.release = ak_pwm_close,
	.unlocked_ioctl = ak_pwm_ioctl,
};

static struct miscdevice ak_pwm_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ak_pwm",
	.fops = &ak_pwm_fops,
	.mode	= S_IRWXO,
};

static int ak_pwm_probe(struct platform_device *pdev)
{
	int ret = 0;
	
	printk(KERN_ERR "%s\n", __func__);
	/*register to miscdevice subsystem*/
	ret = misc_register(&ak_pwm_dev);
	if(ret) {
		printk(KERN_ERR "%s reg miscdev failed.\n", __func__);
		ret = -ENOENT;
	}

	return ret;
}

static int ak_pwm_remove(struct platform_device *pdev)
{
	misc_deregister(&ak_pwm_dev);

	return 0;
}

static struct platform_driver ak_pwm_driver = {
	.driver		= {
		.name	= "ak-pwm",
		.owner	= THIS_MODULE,
	},
	.probe		= ak_pwm_probe,
	.remove		= ak_pwm_remove,
};

static int __init ak_pwm_init(void)
{
	return platform_driver_register(&ak_pwm_driver);
}
module_init(ak_pwm_init);

static void __exit ak_pwm_exit(void)
{
	platform_driver_unregister(&ak_pwm_driver);
}

module_exit(ak_pwm_exit);

MODULE_DESCRIPTION("AK39 PWM based Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ak-pwm");

