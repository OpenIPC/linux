/**
    GPIO IOCTL driver
    To handle GPIO io-control driver

    Copyright Novatek Microelectronics Corp. 2018. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#define DRV_VERSION		"1.00.000"

#define UNINIT_CMD_MAGIC 0xdb
#define INIT_CMD_MAGIC   0xdc
#define RESET_CMD_MAGIC  0xdd
#define GET_CMD_MAGIC    0xde
#define SET_CMD_MAGIC    0xdf
#define GET_CMD         _IOWR(GET_CMD_MAGIC, 0, unsigned int)
#define SET_CMD         _IOW(SET_CMD_MAGIC, 0, unsigned int)
#define INIT_CMD        _IOWR(INIT_CMD_MAGIC, 0, unsigned int)
#define RESET_CMD       _IOW(RESET_CMD_MAGIC, 0, unsigned int)
#define UNINIT_CMD      _IOWR(UNINIT_CMD_MAGIC, 0, unsigned int)

struct gpio_ioctl_data{
	int gpio_num;
	int value;
};

static long gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gpio_ioctl_data val;
	switch(cmd) {
		case SET_CMD:
			if (copy_from_user(&val, (struct gpio_ioctl_data *)arg, \
				sizeof(struct gpio_ioctl_data)))
				return -EFAULT;

			gpio_request(val.gpio_num, "GPIOCTRL");
			gpio_direction_output(val.gpio_num, val.value);
			break;
		case GET_CMD:
			if(copy_from_user(&val, (struct gpio_ioctl_data *)arg, \
				sizeof(struct gpio_ioctl_data)))
				return -EFAULT;

                        val.value = gpio_get_value(val.gpio_num);
                        if(copy_to_user((struct gpio_ioctl_data *)arg, &val, \
				sizeof(struct gpio_ioctl_data)))
				return -EFAULT;
			break;
		case INIT_CMD:
			if(copy_from_user(&val, (struct gpio_ioctl_data *)arg, \
				sizeof(struct gpio_ioctl_data)))
				return -EFAULT;

			gpio_request(val.gpio_num, "GPIOCTRL");
			break;
		case RESET_CMD:
			if (copy_from_user(&val, (struct gpio_ioctl_data *)arg, \
				sizeof(struct gpio_ioctl_data)))
                                return -EFAULT;

			gpio_direction_input(val.gpio_num);
			break;
		case UNINIT_CMD:
			if(copy_from_user(&val, (struct gpio_ioctl_data *)arg, \
				sizeof(struct gpio_ioctl_data)))
				return -EFAULT;

			gpio_free(val.gpio_num);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static const struct file_operations gpio_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl	= gpio_ioctl,
};

static struct miscdevice gpio_miscdev = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "gpio_ctrl",
	.fops           = &gpio_fops,
};

static int __init gpio_ctrl_init(void)
{
	int ret;

	ret = misc_register(&gpio_miscdev);
	if (ret) {
		pr_err("misc_register fail, ret = %d\n", ret);
		return ret;
	}

	return 0;
}

static void __exit gpio_ctrl_dev_exit(void)
{
	misc_deregister(&gpio_miscdev);
}

module_init(gpio_ctrl_init);
module_exit(gpio_ctrl_dev_exit);

MODULE_AUTHOR("Novatek Microelectronics Corp.");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("GPIO IOCTL driver");
MODULE_VERSION(DRV_VERSION);