/*
* linux/drivers/usb/phy/phy-ambarella.c
*
* History:
*	2014/01/28 - [Cao Rongrong] created file
*
* Copyright (C) 2012 by Ambarella, Inc.
* http://www.ambarella.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/usb/otg.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>
#include <plat/rct.h>

#define DRIVER_NAME "ambarella_phy"

/* PORT is termed as usb slot which is outside the chip, and
 * PHY is termed as usb interface which is inside the chip */

#define PHY_TO_DEVICE_PORT	0 /* rotue D+/D- signal to device port */
#define PHY_TO_HOST_PORT	1 /* rotue D+/D- signal to host port */

/* PORT_TYPE_XXX is only for the device port */
#define PORT_TYPE_DEVICE	0 /* we should work as device */
#define PORT_TYPE_OTG		1 /* we should work as host */

struct ambarella_phy {
	struct usb_phy phy;
	void __iomem *pol_reg;
	void __iomem *ana_reg;
	void __iomem *own_reg;
	u8 host_phy_num;
	bool owner_invert;

	int gpio_id;
	bool id_is_otg;
	int gpio_md;
	bool md_host_active;
	int gpio_hub;
	bool hub_active;
	u8 port_type;	/* the behavior of the device port working */
	u8 phy_route;	/* route D+/D- signal to device or host port */

#ifdef CONFIG_PM
	u32 pol_val;
	u32 own_val;
#endif
};

#define to_ambarella_phy(p) container_of((p), struct ambarella_phy, phy)

static inline bool ambarella_usb0_is_host(struct ambarella_phy *amb_phy)
{
	bool is_host;

	if (amb_phy->owner_invert)
		is_host = !(amba_rct_readl(amb_phy->own_reg) & USB0_IS_HOST_MASK);
	else
		is_host = !!(amba_rct_readl(amb_phy->own_reg) & USB0_IS_HOST_MASK);

	return is_host;
};

static inline void ambarella_switch_to_host(struct ambarella_phy *amb_phy)
{
	if (amb_phy->owner_invert)
		amba_rct_clrbitsl(amb_phy->own_reg, USB0_IS_HOST_MASK);
	else
		amba_rct_setbitsl(amb_phy->own_reg, USB0_IS_HOST_MASK);
}

static inline void ambarella_switch_to_device(struct ambarella_phy *amb_phy)
{
	if (amb_phy->owner_invert)
		amba_rct_setbitsl(amb_phy->own_reg, USB0_IS_HOST_MASK);
	else
		amba_rct_clrbitsl(amb_phy->own_reg, USB0_IS_HOST_MASK);
}

static inline void ambarella_check_otg(struct ambarella_phy *amb_phy)
{
	/* if D+/D- is routed to device port which is working
	 * as otg, we need to switch the phy to host mode. */
	if (amb_phy->phy_route == PHY_TO_DEVICE_PORT) {
		if (amb_phy->port_type == PORT_TYPE_OTG)
			ambarella_switch_to_host(amb_phy);
		else
			ambarella_switch_to_device(amb_phy);
	}
}

static void __iomem *ambarella_phy_get_reg(struct platform_device *pdev, int index)
{
	struct resource *mem;
	void __iomem *reg;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, index);
	if (!mem) {
		dev_err(&pdev->dev, "No mem resource: %d!\n", index);
		return NULL;
	}

	reg = devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!reg) {
		dev_err(&pdev->dev, "devm_ioremap() failed: %d\n", index);
		return NULL;
	}

	return reg;
}

static int ambarella_phy_proc_show(struct seq_file *m, void *v)
{
	struct ambarella_phy *amb_phy = m->private;
	const char *port_status;
	const char *phy_status;
	int len = 0;

	if (amb_phy->phy_route == PHY_TO_HOST_PORT)
		port_status = "HOST";
	else
		port_status = "DEVICE";

	if (ambarella_usb0_is_host(amb_phy))
		phy_status = "HOST";
	else
		phy_status = "DEVICE";

	len += seq_printf(m, "Possible parameter: host device\n");
	len += seq_printf(m, "Current status:\n");
	len += seq_printf(m, "\tport is %s: phy is %s\n\n",
		port_status, phy_status);

	return len;
}

static int ambarella_phy_proc_write(struct file *file,
	const char __user *buffer, size_t count, loff_t *ppos)
{
	struct ambarella_phy *amb_phy = PDE_DATA(file_inode(file));
	char n, str[32];

	n = (count < 32) ? count : 32;

	if (copy_from_user(str, buffer, n))
		return -EFAULT;

	str[n - 1] = '\0';

	if (!strcasecmp(str, "host")) {
		if (gpio_is_valid(amb_phy->gpio_md)) {
			amb_phy->phy_route = PHY_TO_HOST_PORT;
			gpio_direction_output(amb_phy->gpio_md,
						amb_phy->md_host_active);
		}

		ambarella_switch_to_host(amb_phy);
	} else if (!strcasecmp(str, "device")) {
		if (gpio_is_valid(amb_phy->gpio_md)) {
			amb_phy->phy_route = PHY_TO_DEVICE_PORT;
			gpio_direction_output(amb_phy->gpio_md,
						!amb_phy->md_host_active);
		}

		ambarella_check_otg(amb_phy);
	} else {
		pr_err("Invalid argument!\n");
	}

	return count;
}

static int ambarella_phy_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ambarella_phy_proc_show, PDE_DATA(inode));
}

static const struct file_operations proc_phy_switcher_fops = {
	.open = ambarella_phy_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = ambarella_phy_proc_write,
};

static irqreturn_t ambarella_otg_detect_irq(int irq, void *dev_id)
{
	struct ambarella_phy *amb_phy = dev_id;

	if (gpio_is_valid(amb_phy->gpio_id)) {
		if (gpio_get_value_cansleep(amb_phy->gpio_id) == amb_phy->id_is_otg)
			amb_phy->port_type = PORT_TYPE_OTG;
		else
			amb_phy->port_type = PORT_TYPE_DEVICE;
	} else {
		amb_phy->port_type = PORT_TYPE_DEVICE;
	}

	ambarella_check_otg(amb_phy);

	return IRQ_HANDLED;
}

static int ambarella_init_phy_switcher(struct ambarella_phy *amb_phy)
{
	struct usb_phy *phy = &amb_phy->phy;
	int irq, rval = 0;

	/* only usb0 support to switch between host and device */
	proc_create_data("usbphy0", S_IRUGO|S_IWUSR,
		get_ambarella_proc_dir(), &proc_phy_switcher_fops, amb_phy);

	/* request gpio for PHY HUB reset */
	if (gpio_is_valid(amb_phy->gpio_hub)) {
		rval = devm_gpio_request(phy->dev, amb_phy->gpio_hub, "hub reset");
		if (rval < 0) {
			dev_err(phy->dev, "Failed to request hub reset pin %d\n", rval);
			return rval;
		}
		gpio_direction_output(amb_phy->gpio_hub, !amb_phy->hub_active);
	}

	if (gpio_is_valid(amb_phy->gpio_id)) {
		rval = devm_gpio_request_one(phy->dev,
			amb_phy->gpio_id, GPIOF_DIR_IN, "otg_id");
		if (rval < 0){
			dev_err(phy->dev, "Failed to request id pin %d\n", rval);
			return rval;
		}

		if (gpio_get_value_cansleep(amb_phy->gpio_id) == amb_phy->id_is_otg)
			amb_phy->port_type = PORT_TYPE_OTG;
		else
			amb_phy->port_type = PORT_TYPE_DEVICE;

		irq = gpio_to_irq(amb_phy->gpio_id);

		rval = devm_request_threaded_irq(phy->dev, irq, NULL,
			ambarella_otg_detect_irq,
			IRQ_TYPE_EDGE_BOTH | IRQF_ONESHOT,
			"usb_otg_id", amb_phy);
		if (rval) {
			dev_err(phy->dev, "request usb_otg_id irq failed: %d\n", rval);
			return rval;
		}

	} else {
		amb_phy->port_type = PORT_TYPE_DEVICE;
	}

	/* rotue D+/D- signal to host or device port if control gpio existed */
	if (gpio_is_valid(amb_phy->gpio_md)) {
		rval = devm_gpio_request(phy->dev,
				amb_phy->gpio_md, "phy_switcher");
		if (rval) {
			dev_err(phy->dev, "request phy_switcher gpio failed\n");
			return rval;
		}

		/* if usb0 is configured as host, route D+/D- signal to
		 * host port, otherwise route them to device port. */
		if (ambarella_usb0_is_host(amb_phy)) {
			amb_phy->phy_route = PHY_TO_HOST_PORT;
			gpio_direction_output(amb_phy->gpio_md,
						amb_phy->md_host_active);
		} else {
			amb_phy->phy_route = PHY_TO_DEVICE_PORT;
			gpio_direction_output(amb_phy->gpio_md,
						!amb_phy->md_host_active);
		}
	} else {
		amb_phy->phy_route = PHY_TO_DEVICE_PORT;
	}

	ambarella_check_otg(amb_phy);

	return 0;
}

static int ambarella_init_host_phy(struct platform_device *pdev,
			struct ambarella_phy *amb_phy)
{
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flags;
	int ocp, rval;

	/* get register for overcurrent polarity */
	amb_phy->pol_reg = ambarella_phy_get_reg(pdev, 1);
	if (!amb_phy->pol_reg)
		return -ENXIO;

	/* get register for usb phy owner configure */
	amb_phy->own_reg = ambarella_phy_get_reg(pdev, 2);
	if (!amb_phy->own_reg)
		return -ENXIO;

	/* see RCT Programming Guide for detailed info about ocp and owner */
	rval = of_property_read_u32(np, "amb,ocp-polarity", &ocp);
	if (rval == 0) {
		amba_clrbitsl(amb_phy->pol_reg, 0x1 << 13);
		amba_setbitsl(amb_phy->pol_reg, ocp << 13);
	}

	amb_phy->owner_invert = !!of_find_property(np, "amb,owner-invert", NULL);
	if (of_find_property(np, "amb,owner-mask", NULL))
		amba_setbitsl(amb_phy->own_reg, 0x1);

	amb_phy->gpio_id = of_get_named_gpio_flags(np, "id-gpios", 0, &flags);
	amb_phy->id_is_otg = !!(flags & OF_GPIO_ACTIVE_LOW);

	amb_phy->gpio_md = of_get_named_gpio_flags(np, "md-gpios", 0, &flags);
	amb_phy->md_host_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	amb_phy->gpio_hub = of_get_named_gpio_flags(np, "hub-gpios", 0, &flags);
	amb_phy->hub_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	ambarella_init_phy_switcher(amb_phy);

	return 0;
}

static int ambarella_phy_init(struct usb_phy *phy)
{
	struct ambarella_phy *amb_phy = to_ambarella_phy(phy);
	u32 ana_val = 0x3006;

	/* If there are 2 PHYs, no matter which PHY need to be initialized,
	 * we initialize all of them at the same time */

	if (!(amba_readl(amb_phy->ana_reg) & ana_val)) {
		amba_setbitsl(amb_phy->ana_reg, ana_val);
		mdelay(1);
	}

	return 0;
}

static int ambarella_phy_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ambarella_phy *amb_phy;
	int host_phy_num, rval;

	amb_phy = devm_kzalloc(&pdev->dev, sizeof(*amb_phy), GFP_KERNEL);
	if (!amb_phy) {
		dev_err(&pdev->dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}

	amb_phy->phy.otg = devm_kzalloc(&pdev->dev, sizeof(struct usb_otg),
					GFP_KERNEL);
	if (!amb_phy->phy.otg) {
		dev_err(&pdev->dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}

	amb_phy->phy.dev = &pdev->dev;
	amb_phy->phy.label = DRIVER_NAME;
	amb_phy->phy.init = ambarella_phy_init;

	/* get register for usb phy power on */
	amb_phy->ana_reg = ambarella_phy_get_reg(pdev, 0);
	if (!amb_phy->ana_reg)
		return -ENXIO;

	rval = of_property_read_u32(np, "amb,host-phy-num", &host_phy_num);
	if (rval < 0)
		amb_phy->host_phy_num = 0;
	else
		amb_phy->host_phy_num = host_phy_num;

	if (amb_phy->host_phy_num > 0)
		ambarella_init_host_phy(pdev, amb_phy);

	platform_set_drvdata(pdev, &amb_phy->phy);

	rval = usb_add_phy_dev(&amb_phy->phy);
	if (rval < 0)
		return rval;

	return 0;
}

static int ambarella_phy_remove(struct platform_device *pdev)
{
	struct ambarella_phy *amb_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&amb_phy->phy);

	return 0;
}

static void ambarella_phy_shutdown(struct platform_device *pdev)
{
	struct ambarella_phy *amb_phy = platform_get_drvdata(pdev);

	if (amb_phy->host_phy_num && gpio_is_valid(amb_phy->gpio_md)) {
		gpio_direction_output(amb_phy->gpio_md,
					!amb_phy->md_host_active);
	}

}

#ifdef CONFIG_PM
static int ambarella_phy_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct ambarella_phy *amb_phy = platform_get_drvdata(pdev);

	amb_phy->pol_val = amba_readl(amb_phy->pol_reg);
	amb_phy->own_val = amba_readl(amb_phy->own_reg);

	return 0;
}

static int ambarella_phy_resume(struct platform_device *pdev)
{
	struct ambarella_phy *amb_phy = platform_get_drvdata(pdev);

	amba_writel(amb_phy->pol_reg, amb_phy->pol_val);
	amba_writel(amb_phy->own_reg, amb_phy->own_val);

	return 0;
}
#endif

static const struct of_device_id ambarella_phy_dt_ids[] = {
	{ .compatible = "ambarella,usbphy", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_phy_dt_ids);

static struct platform_driver ambarella_phy_driver = {
	.probe = ambarella_phy_probe,
	.remove = ambarella_phy_remove,
	.shutdown = ambarella_phy_shutdown,
#ifdef CONFIG_PM
	.suspend = ambarella_phy_suspend,
	.resume	 = ambarella_phy_resume,
#endif
	.driver = {
		.name = DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_phy_dt_ids,
	 },
};

/* We have to call ambarella_phy_module_init() before the drives using USB PHY
 * like EHCI/OHCI/UDC, and after GPIO drivers including external GPIO chip, so
 * we use subsys_initcall_sync here. */
static int __init ambarella_phy_module_init(void)
{
	return platform_driver_register(&ambarella_phy_driver);
}
subsys_initcall_sync(ambarella_phy_module_init);

static void __exit ambarella_phy_module_exit(void)
{
	platform_driver_unregister(&ambarella_phy_driver);
}
module_exit(ambarella_phy_module_exit);


MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella USB PHY driver");
MODULE_LICENSE("GPL");

