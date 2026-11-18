// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Axera Inc.
 */
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/atomic.h>

#define COMMON_SYS_DEB_GPIO_LP_INT_CLR0_SET_ADDR_OFFSET	0x2F4
#define COMMON_SYS_DEB_GPIO_LP_INT_CLR1_SET_ADDR_OFFSET	0x328
#define COMMON_SYS_DEB_GPIO_LP_INT_CLR2_SET_ADDR_OFFSET	0x35c
#define COMMON_SYS_DEB_GPIO_LP_INT_CLR0_CLR_ADDR_OFFSET	0x2F8
#define COMMON_SYS_DEB_GPIO_LP_INT_CLR1_CLR_ADDR_OFFSET	0x32c
#define COMMON_SYS_DEB_GPIO_LP_INT_CLR2_CLR_ADDR_OFFSET	0x360
#define COMMON_SYS_DEB_GPIO_LP_RISE_EN0_SET_ADDR_OFFSET	0x300
#define COMMON_SYS_DEB_GPIO_LP_RISE_EN1_SET_ADDR_OFFSET	0x334
#define COMMON_SYS_DEB_GPIO_LP_RISE_EN2_SET_ADDR_OFFSET	0x368
#define COMMON_SYS_DEB_GPIO_LP_FALL_EN0_SET_ADDR_OFFSET	0x30C
#define COMMON_SYS_DEB_GPIO_LP_FALL_EN1_SET_ADDR_OFFSET	0x340
#define COMMON_SYS_DEB_GPIO_LP_FALL_EN2_SET_ADDR_OFFSET	0x374
#define COMMON_SYS_DEB_GPIO_LP_INT_EN0_SET_ADDR_OFFSET	0x2E8
#define COMMON_SYS_DEB_GPIO_LP_INT_EN1_SET_ADDR_OFFSET	0x31c
#define COMMON_SYS_DEB_GPIO_LP_INT_EN2_SET_ADDR_OFFSET	0x350
#define COMMON_SYS_DEB_GPIO_LP_INT_O0_ADDR_OFFSET	0x314
#define COMMON_SYS_DEB_GPIO_LP_INT_O1_ADDR_OFFSET	0x348
#define COMMON_SYS_DEB_GPIO_LP_INT_O2_ADDR_OFFSET	0x37c
#define BIT_COMMON_GPIO_LP_EIC_EN_SET			BIT(5)
#define COMMON_SYS_EIC_EN_SET_ADDR_OFFSET		0x214

#define LP_GPIO_MAX	8
#define LP_GPIO_DEVICE_NAME "lp-gpio"
#define LP_GPIO_TRIGGER_FALLING	0x0
#define LP_GPIO_TRIGGER_RISING	0x1

#define GPIO_NUM_PER_REG	32

struct axera_wait_queue_head_t {
	atomic_t wq_status;
	int gpio_num;
	wait_queue_head_t wq;
};

static struct axera_wait_queue_head_t wait[LP_GPIO_MAX];
struct deb_gpio_lp_res {
	struct resource *sys_res;
	int irq;
	int type;
	int major;
	int lp_status_num;
	void __iomem *sys_cfg;
	int gpio_num[LP_GPIO_MAX];
	int lp_trigger_type[LP_GPIO_MAX];
	volatile void __iomem * int_status_addr[3];
	volatile void __iomem * int_clr_set_addr[3];
	volatile void __iomem * int_clr_clr_addr[3];
	volatile void __iomem * lp_rise_en_set_addr[3];
	volatile void __iomem * lp_fall_en_set_addr[3];
	volatile void __iomem * lp_int_en_set_addr[3];
	struct class *lpgpio_class;
	struct wakeup_source *ws;
};

static int lp_gpio_int_enabled(struct deb_gpio_lp_res *deb_lp_res, int gpio_num)
{
	return readl(deb_lp_res->int_status_addr[gpio_num / GPIO_NUM_PER_REG]) & (1 << (gpio_num % GPIO_NUM_PER_REG));
}

static irqreturn_t deb_gpio_lp_handler(int irq, void *dev_id)
{
	struct deb_gpio_lp_res *deb_lp_res;
	int index = 0;
	int gpio_num;
	int int_status = 0;
	void __iomem *sys_cfg;

	deb_lp_res = (struct deb_gpio_lp_res *)dev_id;
	sys_cfg = deb_lp_res->sys_cfg;

	pm_wakeup_ws_event(deb_lp_res->ws, 0, false);

	for(index = 0; index < deb_lp_res->lp_status_num; index++) {

		gpio_num = deb_lp_res->gpio_num[index];

		int_status = lp_gpio_int_enabled(deb_lp_res, gpio_num);

		if (int_status){
			atomic_inc(&wait[index].wq_status);
			wake_up_interruptible(&wait[index].wq);
			writel(int_status, deb_lp_res->int_clr_set_addr[gpio_num / GPIO_NUM_PER_REG]);
			writel(int_status, deb_lp_res->int_clr_clr_addr[gpio_num / GPIO_NUM_PER_REG]);
		}

	}

	return IRQ_HANDLED;
}

static int gpio_wakeup_config(struct platform_device *pdev)
{
	struct deb_gpio_lp_res *deb_lp_res;
	void __iomem *sys_cfg;
	int index = 0;
	int gpio_num;

	deb_lp_res = platform_get_drvdata(pdev);

	if (!deb_lp_res) {
		return -ENODATA;
	}

	sys_cfg = ioremap(deb_lp_res->sys_res->start,
			resource_size(deb_lp_res->sys_res));
	deb_lp_res->sys_cfg = sys_cfg;

	if (!sys_cfg) {
		return -ENODATA;
	}

	deb_lp_res->int_status_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_O0_ADDR_OFFSET;
	deb_lp_res->int_status_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_O1_ADDR_OFFSET;
	deb_lp_res->int_status_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_O2_ADDR_OFFSET;

	deb_lp_res->int_clr_set_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR0_SET_ADDR_OFFSET;
	deb_lp_res->int_clr_set_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR1_SET_ADDR_OFFSET;
	deb_lp_res->int_clr_set_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR2_SET_ADDR_OFFSET;

	deb_lp_res->int_clr_clr_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR0_CLR_ADDR_OFFSET;
	deb_lp_res->int_clr_clr_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR1_CLR_ADDR_OFFSET;
	deb_lp_res->int_clr_clr_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_CLR2_CLR_ADDR_OFFSET;

	deb_lp_res->lp_rise_en_set_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_RISE_EN0_SET_ADDR_OFFSET;
	deb_lp_res->lp_rise_en_set_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_RISE_EN1_SET_ADDR_OFFSET;
	deb_lp_res->lp_rise_en_set_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_RISE_EN2_SET_ADDR_OFFSET;

	deb_lp_res->lp_fall_en_set_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_FALL_EN0_SET_ADDR_OFFSET;
	deb_lp_res->lp_fall_en_set_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_FALL_EN1_SET_ADDR_OFFSET;
	deb_lp_res->lp_fall_en_set_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_FALL_EN2_SET_ADDR_OFFSET;

	deb_lp_res->lp_int_en_set_addr[0] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_EN0_SET_ADDR_OFFSET;
	deb_lp_res->lp_int_en_set_addr[1] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_EN1_SET_ADDR_OFFSET;
	deb_lp_res->lp_int_en_set_addr[2] = sys_cfg + COMMON_SYS_DEB_GPIO_LP_INT_EN2_SET_ADDR_OFFSET;

	if (request_irq(deb_lp_res->irq, deb_gpio_lp_handler, IRQF_SHARED, "deb_gpio_lp", deb_lp_res)) {
		iounmap(deb_lp_res->sys_cfg);
		return -ENODATA;
	}

	/* eic enable gpio_lp for wakeup */
	writel(BIT_COMMON_GPIO_LP_EIC_EN_SET, sys_cfg + COMMON_SYS_EIC_EN_SET_ADDR_OFFSET);

	for(index = 0; index < deb_lp_res->lp_status_num; index++) {

		gpio_num = deb_lp_res->gpio_num[index];

		/* whatever we clear interrupt status */
		writel((1 << (gpio_num % GPIO_NUM_PER_REG)), deb_lp_res->int_clr_set_addr[gpio_num / GPIO_NUM_PER_REG]);
		writel((1 << (gpio_num % GPIO_NUM_PER_REG)), deb_lp_res->int_clr_clr_addr[gpio_num / GPIO_NUM_PER_REG]);

		/* set risen or falling */
		if (deb_lp_res->lp_trigger_type[index] == LP_GPIO_TRIGGER_RISING) {
			writel((1 << (gpio_num % GPIO_NUM_PER_REG)), deb_lp_res->lp_rise_en_set_addr[gpio_num / GPIO_NUM_PER_REG]);
		} else if (deb_lp_res->lp_trigger_type[index] == LP_GPIO_TRIGGER_FALLING) {
			writel((1 << (gpio_num % GPIO_NUM_PER_REG)), deb_lp_res->lp_fall_en_set_addr[gpio_num / GPIO_NUM_PER_REG]);
		}

		/* enable int */
		writel((1 << (gpio_num % GPIO_NUM_PER_REG)), deb_lp_res->lp_int_en_set_addr[gpio_num / GPIO_NUM_PER_REG]);
	}

	return 0;
}

static __poll_t lpgpio_poll(struct file *file, poll_table *pts)
{
	int index = iminor(file_inode(file));
	__poll_t mask = 0;

	poll_wait(file, &wait[index].wq, pts);

	if (atomic_read(&wait[index].wq_status)) {
		atomic_dec(&wait[index].wq_status);
		mask = POLLIN | POLLRDNORM;
	} else
		mask = 0;

	return mask;
}

const struct file_operations lpgpio_fops = {
	.owner = THIS_MODULE,
	.poll = lpgpio_poll,
};

static int axera_deb_gpio_lp_probe(struct platform_device *pdev)
{
	struct deb_gpio_lp_res *deb_lp_res;
	struct device_node *np = pdev->dev.of_node;
	int index = 0;
	int lp_status_num = 0;
	struct device *lpgpio_dev;

	deb_lp_res = devm_kzalloc(&pdev->dev, sizeof(*deb_lp_res), GFP_KERNEL);
	if (deb_lp_res == NULL)
		return -ENOMEM;

	deb_lp_res->sys_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!deb_lp_res->sys_res)
		return -ENODATA;

	deb_lp_res->irq = platform_get_irq_byname(pdev, "wakeup_int");
	if (deb_lp_res->irq < 0) {
		return -ENODATA;
	}

	if ((lp_status_num = of_property_count_elems_of_size(np, "lp-status", 8)) < 0) {
		return -ENODATA;
	}

	deb_lp_res->lp_status_num = lp_status_num;

	for(index = 0; index < lp_status_num; index++) {
		atomic_set(&wait[index].wq_status, 0);
		if (of_property_read_u32_index(np, "lp-status",
			2 * index, &deb_lp_res->gpio_num[index]) < 0) {
			return -ENODEV;
		}
		if (of_property_read_u32_index(np, "lp-status",
			2 * index + 1, &deb_lp_res->lp_trigger_type[index]) < 0) {
			return -ENODEV;
		}
	}

	deb_lp_res->major = register_chrdev(0, LP_GPIO_DEVICE_NAME, &lpgpio_fops);
	if (deb_lp_res->major < 0)
		return -ENODEV;

	deb_lp_res->lpgpio_class = class_create(THIS_MODULE, LP_GPIO_DEVICE_NAME);
	if (IS_ERR(deb_lp_res->lpgpio_class)) {
		unregister_chrdev(deb_lp_res->major, LP_GPIO_DEVICE_NAME);
		return PTR_ERR(deb_lp_res->lpgpio_class);
	}

	for (index = 0; index < lp_status_num; index++) {
		wait[index].gpio_num = deb_lp_res->gpio_num[index];
		init_waitqueue_head(&wait[index].wq);
		lpgpio_dev = device_create(deb_lp_res->lpgpio_class, NULL, MKDEV(deb_lp_res->major,index), NULL,
					"%s-%d", LP_GPIO_DEVICE_NAME, deb_lp_res->gpio_num[index]);
		if (IS_ERR(lpgpio_dev)) {
			while(index--)
				device_destroy(deb_lp_res->lpgpio_class, MKDEV(deb_lp_res->major, index));
			class_destroy(deb_lp_res->lpgpio_class);
			unregister_chrdev(deb_lp_res->major, LP_GPIO_DEVICE_NAME);
			return PTR_ERR(lpgpio_dev);
		}
	}

	deb_lp_res->ws = wakeup_source_register("wakeup_int");
	if (!deb_lp_res->ws) {
		pr_err("%s: create wakeup_int ws fail\n", __func__);
		return -ENODATA;
	}

	platform_set_drvdata(pdev, deb_lp_res);

	if (gpio_wakeup_config(pdev) < 0) {
		return -ENODATA;
	}

	return 0;
}

static int axera_deb_gpio_lp_remove(struct platform_device *pdev)
{
	struct deb_gpio_lp_res *deb_lp_res;
	deb_lp_res = platform_get_drvdata(pdev);
	int index = 0;

	if (!deb_lp_res) {
		return -ENODATA;
	}

	free_irq(deb_lp_res->irq, NULL);

	if (deb_lp_res->sys_cfg)
		iounmap(deb_lp_res->sys_cfg);

	if (deb_lp_res->lpgpio_class && deb_lp_res->major >= 0 ) {
		for (index = 0; index < deb_lp_res->lp_status_num; index++) {
			device_destroy(deb_lp_res->lpgpio_class, MKDEV(deb_lp_res->major, index));
		}
		class_destroy(deb_lp_res->lpgpio_class);
		unregister_chrdev(deb_lp_res->major, LP_GPIO_DEVICE_NAME);
	}

	return 0;
}

static const struct of_device_id axera_deb_gpio_lp_matches[] = {
	{.compatible = "axera, deb-gpio-lp"},
	{},
};

static struct platform_driver axera_deb_gpio_lp_driver = {
	.driver = {
		   .name = "axera, deb-gpio-lp",
		   .of_match_table = axera_deb_gpio_lp_matches,
		   },
	.probe = axera_deb_gpio_lp_probe,
	.remove = axera_deb_gpio_lp_remove,
};

module_platform_driver(axera_deb_gpio_lp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axera");
MODULE_ALIAS("platform:axera-deb-gpio-lp");
