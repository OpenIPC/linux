/*
 * Allwinner SoCs power domain test driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define DEBUG

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/pm_runtime.h>
#include <linux/pm_domain.h>

struct sunxi_pdtest_data {
	struct device *dev;
	u32 pd_on_off;
};

static struct class_compat *pd_compat_class;      /* /sys/class/pd_test-x */

static ssize_t
status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", (atomic_read(&dev->power.usage_count) > 0) ? "on" : "off");
}

static ssize_t
status_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (!strncmp(buf, "on", sizeof("on") - 1)) {
		pm_runtime_get(dev);
		if (atomic_read(&dev->power.usage_count) < 1)
			pr_debug("runtime usage_count < 1, can't not enable power domain\n");
		else
			pr_debug("enable power domain success!\n");
	} else if (!strncmp(buf, "off", sizeof("off") - 1)) {
		pm_runtime_put(dev);
		if (atomic_read(&dev->power.usage_count) > 0)
			pr_debug("runtime usage_count > 0, can't not disable power domain\n");
		else
			pr_debug("disable power domain success!\n");
	} else {
		pr_err("Only 'on' or 'off' command support!\n");
	}
	return count;
}

static DEVICE_ATTR(status, S_IWUSR | S_IRUGO,
		status_show, status_store);

static int sunxi_pdtest_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct of_phandle_args pd_args;
	char class_name[32];
	int ret;

	if (np == NULL) {
		pr_err("pd_test pdev failed to get of_node\n");
		return -ENODEV;
	}

	ret = of_parse_phandle_with_args(dev->of_node, "power-domains",
					"#power-domain-cells", 0, &pd_args);
	if (ret < 0) {
		pr_err("pd_test: power domain phandle miss!\n");
		return ret;
	}

	snprintf(class_name, sizeof(class_name), "pd_test-%d", pd_args.args[0]);

	pd_compat_class = class_compat_register(class_name);
	if (!pd_compat_class)
		return -ENOMEM;

	ret = device_create_file(dev, &dev_attr_status);
	if (ret)
		goto unregister_class;

	ret = class_compat_create_link(pd_compat_class, dev, NULL);
	if (ret)
		goto remove_file;

	pm_runtime_enable(dev);

	return ret;

remove_file:
	device_remove_file(dev, &dev_attr_status);
unregister_class:
	class_compat_unregister(pd_compat_class);
	return ret;

}

static int sunxi_pdtest_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	class_compat_remove_link(pd_compat_class, dev, NULL);
	class_compat_unregister(pd_compat_class);
	device_remove_file(dev, &dev_attr_status);
	return 0;
}

static const struct of_device_id sunxi_pdtest_match[] = {
	{ .compatible = "allwinner,sunxi-power-domain-test"},
	{}
};

static struct platform_driver sunxi_pdtest_driver = {
	.probe = sunxi_pdtest_probe,
	.remove	= sunxi_pdtest_remove,
	.driver = {
		.name = "pdtest",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_pdtest_match,
	},
};

static int __init sunxi_pdtest_init(void)
{
	int ret;
	ret = platform_driver_register(&sunxi_pdtest_driver);
	if (ret) {
		pr_warn("register sunxi power domain test driver failed\n");
		return -EINVAL;
	}

	return 0;
}

static void __exit sunxi_pdtest_exit(void)
{
	platform_driver_unregister(&sunxi_pdtest_driver);
}

module_init(sunxi_pdtest_init);
module_exit(sunxi_pdtest_exit);
MODULE_AUTHOR("Huangyongxing<huangyongxing@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner sunxi power domain driver test");
MODULE_LICENSE("GPL");
