
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/kobject.h>
#include <mach/gpio.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <plat-anyka/user_gpio.h>

struct user_gpio_attribute {
	struct attribute attr;
	struct user_gpio_info *gpio;
};

struct user_gpio_driver_info {
	struct kobject kobj;
	struct attribute **attrs;
};

static struct user_gpio_driver_info driver_info;

static void user_gpio_release(struct kobject *kobj)
{
	printk(KERN_ERR "%s\n", __func__);
}

static ssize_t	user_gpio_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	int level;
	struct user_gpio_attribute  *puser_gpio_attr_arrays;

	puser_gpio_attr_arrays = container_of(attr, struct user_gpio_attribute, attr);
	level = ak_gpio_getpin(puser_gpio_attr_arrays->gpio->info.pin);
	sprintf(buf, "%d\n", level);
	return (strlen(buf) + 1);
}

static ssize_t	user_gpio_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
	int ret;
	long int level;
	struct gpio_info info;
	struct user_gpio_attribute  *puser_gpio_attr_arrays;

	puser_gpio_attr_arrays = container_of(attr, struct user_gpio_attribute, attr);
	ret = strict_strtol(buf, 10, &level);
	if (ret) {
		printk(KERN_ERR "string can not trans to long int\n");
		return 0;
	}

	//printk(KERN_ERR "%s store:%ld\n", puser_gpio_attr_arrays->gpio->name, level);

	memcpy(&info, &puser_gpio_attr_arrays->gpio->info, sizeof(info));
	info.value = level;
	ak_gpio_set(&info);

	return count;
}

static const struct sysfs_ops user_gpio_sysfs_ops = {
	.show	= user_gpio_show,
	.store	= user_gpio_store,
};

static struct kobj_type user_gpio_ktype = {
	.release	= user_gpio_release,
	.sysfs_ops	= &user_gpio_sysfs_ops,
};

static int user_gpio_probe(struct platform_device *pd)
{
	int i;
	int n;
	int retval;
	struct ak_user_gpio_pdata *pdata = pd->dev.platform_data;
	struct user_gpio_info *pgpios = pdata->user_gpios;
	int nr_user_gpios    = pdata->nr_user_gpios;
	struct user_gpio_attribute  *puser_gpio_attr_arrays;

	driver_info.attrs = kzalloc(sizeof(struct attribute *) * (nr_user_gpios + 1)
								, GFP_KERNEL);
	if (!driver_info.attrs) {
		printk(KERN_ERR "can not zalloc for attrs address\n");
		retval = -ENOMEM;
		goto fail_alloc_attrs_address;
	}

	puser_gpio_attr_arrays = kzalloc(sizeof(struct user_gpio_attribute) *
								nr_user_gpios, GFP_KERNEL);
	if (!puser_gpio_attr_arrays) {
		printk(KERN_ERR "can not zalloc for all attrs\n");
		retval = -ENOMEM;
		goto fail_alloc_attr_arrays;
	}

	for (i = 0, n = 0; i < nr_user_gpios; i++) {
		struct user_gpio_attribute  *pgpio_attr = puser_gpio_attr_arrays + i;
		struct user_gpio_info *pgpio = pgpios + i;
		struct gpio_info *info;

		pgpio_attr->attr.name = pgpio->name;
		pgpio_attr->attr.mode = 0666;
		pgpio_attr->gpio = pgpio;

		if (pgpio_attr->gpio->info.pin < 0)
			continue;

		*(driver_info.attrs + n) = &(pgpio_attr->attr);
		n++;

		info = &pgpio_attr->gpio->info;
		ak_gpio_set(info);
	}

	retval = kobject_init_and_add(&driver_info.kobj, &user_gpio_ktype, NULL, "user-gpio");
	if (retval) {
		printk(KERN_ERR "can not init kobject\n");
		goto fail_kobject_add;
	}

	retval = sysfs_create_files(&driver_info.kobj, (const struct attribute **)driver_info.attrs);
	if (retval)
		goto fail_create_files;

	return 0;

fail_create_files:
	kobject_put(&driver_info.kobj);
fail_kobject_add:
	kfree(puser_gpio_attr_arrays);
fail_alloc_attr_arrays:
	kfree(driver_info.attrs);
fail_alloc_attrs_address:
	return retval;
}

static int user_gpio_remove(struct platform_device *pd)
{
	struct user_gpio_attribute  *puser_gpio_attr_arrays;
	struct attribute  **pattrs;

	sysfs_remove_files(&driver_info.kobj, (const struct attribute **)driver_info.attrs);
	kobject_put(&driver_info.kobj);

	pattrs = driver_info.attrs;
	if (*pattrs) {
		puser_gpio_attr_arrays = container_of(*pattrs, struct user_gpio_attribute, attr);
		kfree(puser_gpio_attr_arrays);
	}

	kfree(driver_info.attrs);

	return 0;
}

static struct platform_driver user_gpio_device_driver = {
	.probe		= user_gpio_probe,
	.remove		= __devexit_p(user_gpio_remove),
	.driver		= {
		.name	= "user_gpio",
		.owner	= THIS_MODULE,
	}
};

static int __init user_gpio_init(void)
{
	return platform_driver_register(&user_gpio_device_driver);
}

static void __exit user_gpio_exit(void)
{
	platform_driver_unregister(&user_gpio_device_driver);
}

module_init(user_gpio_init);
module_exit(user_gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ygh");
MODULE_DESCRIPTION("Export GPIOs");
MODULE_ALIAS("platform:user-keys");
