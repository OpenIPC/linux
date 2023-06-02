/*
 * ambpriv.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/pm_runtime.h>
#include <linux/ambpriv_device.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include "base.h"

#define to_ambpriv_driver(drv)	(container_of((drv), struct ambpriv_driver, driver))

struct device ambpriv_bus = {
	.init_name	= "ambpriv",
};

static int of_ambpriv_match_device(struct device *dev, void *match)
{
	return !!of_match_device(match, dev);
}

struct ambpriv_device *of_find_ambpriv_device_by_match(struct of_device_id *match)
{
	struct device *dev;

	dev = bus_find_device(&ambpriv_bus_type, NULL, match, of_ambpriv_match_device);
	return dev ? to_ambpriv_device(dev) : NULL;
}
EXPORT_SYMBOL(of_find_ambpriv_device_by_match);

static int of_ambpriv_node_device(struct device *dev, void *data)
{
	return dev->of_node == data;
}

struct ambpriv_device *of_find_ambpriv_device_by_node(struct device_node *np)
{
	struct device *dev;

	dev = bus_find_device(&ambpriv_bus_type, NULL, np, of_ambpriv_node_device);
	return dev ? to_ambpriv_device(dev) : NULL;
}
EXPORT_SYMBOL(of_find_ambpriv_device_by_node);

int ambpriv_get_irq(struct ambpriv_device *dev, unsigned int num)
{
	struct resource *r = NULL;
	int i;

	if (dev == NULL)
		return -ENODEV;

	for (i = 0; i < dev->num_resources; i++) {
		r = &dev->resource[i];
		if ((resource_type(r) == IORESOURCE_IRQ) && num-- == 0)
			break;
	}

	if (i == dev->num_resources)
		return -ENXIO;

	if (r && r->flags & IORESOURCE_BITS)
		irqd_set_trigger_type(irq_get_irq_data(r->start),
				      r->flags & IORESOURCE_BITS);

	return r->start;
}
EXPORT_SYMBOL(ambpriv_get_irq);

int ambpriv_get_irq_by_name(struct ambpriv_device *dev, const char *name)
{
	struct resource *r = NULL;
	int i;

	if (dev == NULL)
		return -ENODEV;

	for (i = 0; i < dev->num_resources; i++) {
		r = &dev->resource[i];

		if (unlikely(!r->name))
			continue;

		if ((resource_type(r) == IORESOURCE_IRQ) && !strcmp(r->name, name))
			break;
	}

	if (i == dev->num_resources)
		return -ENXIO;

	return r->start;
}
EXPORT_SYMBOL(ambpriv_get_irq_by_name);

static struct ambpriv_device *of_ambpriv_device_alloc(struct device_node *np,
		struct device *parent)
{
	struct ambpriv_device *dev;
	const __be32 *reg_prop;
	struct resource *res;
	int psize, i, num_reg = 0, num_irq;

	dev = ambpriv_device_alloc("", -1);
	if (!dev)
		return NULL;

	reg_prop = of_get_property(np, "reg", &psize);
	if (reg_prop)
		num_reg = psize / sizeof(u32) / 2;


	num_irq = of_irq_count(np);

	if (num_reg || num_irq) {
		res = kzalloc(sizeof(*res) * (num_irq + num_reg), GFP_KERNEL);
		if (!res) {
			ambpriv_device_put(dev);
			return NULL;
		}

		dev->resource = res;
		dev->num_resources = num_reg + num_irq;

		for (i = 0; i < num_reg; i++, res++, reg_prop += 2) {
			res->start = be32_to_cpup(reg_prop);
			res->end = res->start + be32_to_cpup(reg_prop + 1) - 1;
			res->flags = IORESOURCE_MEM;
		}

		of_irq_to_resource_table(np, res, num_irq);
	}

	dev->dev.of_node = of_node_get(np);
	dev->dev.parent = parent ? : &ambpriv_bus;
	of_device_make_bus_id(&dev->dev);
	dev->name = dev_name(&dev->dev);

	return dev;
}

static struct ambpriv_device *of_ambpriv_device_create_pdata(
		struct device_node *np, struct device *parent)
{
	struct ambpriv_device *dev;

	dev = of_ambpriv_device_alloc(np, parent);
	if (!dev)
		return NULL;

	if (ambpriv_device_add(dev) < 0) {
		ambpriv_device_put(dev);
		return NULL;
	}

	return dev;
}

static int of_ambpriv_bus_create(struct device_node *bus,
		const struct of_device_id *matches, struct device *parent)
{
	struct device_node *child;
	struct ambpriv_device *dev;
	int rc = 0;

	/* Make sure it has a compatible property */
	if (!of_get_property(bus, "compatible", NULL))
		return 0;

	dev = of_ambpriv_device_create_pdata(bus, parent);
	if (!dev || !of_match_node(matches, bus))
		return 0;

	for_each_child_of_node(bus, child) {
		rc = of_ambpriv_bus_create(child, matches, &dev->dev);
		if (rc) {
			of_node_put(child);
			break;
		}
	}

	return rc;
}

static const struct of_device_id of_ambpriv_bus_match_table[] = {
	{ .compatible = "ambpriv-bus", },
	{} /* Empty terminated list */
};

static int __init of_ambpriv_populate(void)
{
	struct device_node *root, *child;
	int rval;

	root = of_find_node_by_path("/iav");
	if (!root)
		return -EINVAL;

	for_each_child_of_node(root, child) {
		rval = of_ambpriv_bus_create(child, of_ambpriv_bus_match_table, NULL);
		if (rval) {
			of_node_put(child);
			break;
		}
	}

	of_node_put(root);

	return 0;
}
late_initcall(of_ambpriv_populate);


int ambpriv_add_devices(struct ambpriv_device **devs, int num)
{
	int i, ret = 0;

	for (i = 0; i < num; i++) {
		ret = ambpriv_device_register(devs[i]);
		if (ret) {
			while (--i >= 0)
				ambpriv_device_unregister(devs[i]);
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL(ambpriv_add_devices);

struct ambpriv_object {
	struct ambpriv_device ambdev;
	char name[1];
};

void ambpriv_device_put(struct ambpriv_device *ambdev)
{
	if (ambdev)
		put_device(&ambdev->dev);
}
EXPORT_SYMBOL(ambpriv_device_put);

static void ambpriv_device_release(struct device *dev)
{
	struct ambpriv_object *pa;

	pa = container_of(dev, struct ambpriv_object, ambdev.dev);
	kfree(pa->ambdev.dev.platform_data);
	kfree(pa->ambdev.resource);
	kfree(pa);
}

struct ambpriv_device *ambpriv_device_alloc(const char *name, int id)
{
	struct ambpriv_object *pa;

	pa = kzalloc(sizeof(struct ambpriv_object) + strlen(name), GFP_KERNEL);
	if (pa) {
		strcpy(pa->name, name);
		pa->ambdev.name = pa->name;
		pa->ambdev.id = id;
		device_initialize(&pa->ambdev.dev);
		pa->ambdev.dev.release = ambpriv_device_release;
	}

	return pa ? &pa->ambdev : NULL;
}
EXPORT_SYMBOL(ambpriv_device_alloc);

int ambpriv_device_add_resources(struct ambpriv_device *ambdev,
				  const struct resource *res, unsigned int num)
{
	struct resource *r;

	if (!res)
		return 0;

	r = kmemdup(res, sizeof(struct resource) * num, GFP_KERNEL);
	if (r) {
		ambdev->resource = r;
		ambdev->num_resources = num;
		return 0;
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(ambpriv_device_add_resources);

int ambpriv_device_add_data(struct ambpriv_device *ambdev, const void *data,
			     size_t size)
{
	void *d;

	if (!data)
		return 0;

	d = kmemdup(data, size, GFP_KERNEL);
	if (d) {
		ambdev->dev.platform_data = d;
		return 0;
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(ambpriv_device_add_data);

int ambpriv_device_add(struct ambpriv_device *ambdev)
{
	int i, ret = 0;

	if (!ambdev)
		return -EINVAL;

	if (!ambdev->dev.parent)
		ambdev->dev.parent = &ambpriv_bus;

	ambdev->dev.bus = &ambpriv_bus_type;

	if (ambdev->id != -1)
		dev_set_name(&ambdev->dev, "%s.%d", ambdev->name,  ambdev->id);
	else
		dev_set_name(&ambdev->dev, "%s", ambdev->name);

	for (i = 0; i < ambdev->num_resources; i++) {
		struct resource *p, *r = &ambdev->resource[i];

		if (r->name == NULL)
			r->name = dev_name(&ambdev->dev);

		p = r->parent;
		if (!p) {
			if (resource_type(r) == IORESOURCE_MEM)
				p = &iomem_resource;
			else if (resource_type(r) == IORESOURCE_IO)
				p = &ioport_resource;
		}

		if (p && insert_resource(p, r)) {
			printk(KERN_ERR
			       "%s: failed to claim resource %d\n",
			       dev_name(&ambdev->dev), i);
			ret = -EBUSY;
			goto failed;
		}
	}

	pr_debug("Registering ambpriv device '%s'. Parent at %s\n",
		 dev_name(&ambdev->dev), dev_name(ambdev->dev.parent));

	ret = device_add(&ambdev->dev);
	if (ret == 0)
		return ret;

 failed:
	while (--i >= 0) {
		struct resource *r = &ambdev->resource[i];
		unsigned long type = resource_type(r);

		if (type == IORESOURCE_MEM || type == IORESOURCE_IO)
			release_resource(r);
	}

	return ret;
}
EXPORT_SYMBOL(ambpriv_device_add);

void ambpriv_device_del(struct ambpriv_device *ambdev)
{
	int i;

	if (ambdev) {
		device_del(&ambdev->dev);

		for (i = 0; i < ambdev->num_resources; i++) {
			struct resource *r = &ambdev->resource[i];
			unsigned long type = resource_type(r);

			if (type == IORESOURCE_MEM || type == IORESOURCE_IO)
				release_resource(r);
		}
	}
}
EXPORT_SYMBOL(ambpriv_device_del);

int ambpriv_device_register(struct ambpriv_device *ambdev)
{
	device_initialize(&ambdev->dev);
	return ambpriv_device_add(ambdev);
}
EXPORT_SYMBOL(ambpriv_device_register);

void ambpriv_device_unregister(struct ambpriv_device *ambdev)
{
	ambpriv_device_del(ambdev);
	ambpriv_device_put(ambdev);
}
EXPORT_SYMBOL(ambpriv_device_unregister);

static int ambpriv_drv_probe(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	return drv->probe(dev);
}

static int ambpriv_drv_remove(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	return drv->remove(dev);
}

static void ambpriv_drv_shutdown(struct device *_dev)
{
	struct ambpriv_driver *drv = to_ambpriv_driver(_dev->driver);
	struct ambpriv_device *dev = to_ambpriv_device(_dev);

	drv->shutdown(dev);
}

int ambpriv_driver_register(struct ambpriv_driver *drv)
{
	drv->driver.bus = &ambpriv_bus_type;
	if (drv->probe)
		drv->driver.probe = ambpriv_drv_probe;
	if (drv->remove)
		drv->driver.remove = ambpriv_drv_remove;
	if (drv->shutdown)
		drv->driver.shutdown = ambpriv_drv_shutdown;

	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(ambpriv_driver_register);

void ambpriv_driver_unregister(struct ambpriv_driver *drv)
{
	driver_unregister(&drv->driver);
}
EXPORT_SYMBOL(ambpriv_driver_unregister);

struct ambpriv_device * __init_or_module ambpriv_create_bundle(
			struct ambpriv_driver *driver,
			struct resource *res, unsigned int n_res,
			const void *data, size_t size)
{
	struct ambpriv_device *ambdev;
	int error;

	ambdev = of_find_ambpriv_device_by_match(
			(struct of_device_id *)driver->driver.of_match_table);
	if (ambdev)
		goto register_drv;

	ambdev = ambpriv_device_alloc(driver->driver.name, -1);
	if (!ambdev) {
		error = -ENOMEM;
		goto err_out;
	}

	error = ambpriv_device_add_resources(ambdev, res, n_res);
	if (error)
		goto err_pdev_put;

	error = ambpriv_device_add_data(ambdev, data, size);
	if (error)
		goto err_pdev_put;

	error = ambpriv_device_add(ambdev);
	if (error)
		goto err_pdev_put;

register_drv:
	error = ambpriv_driver_register(driver);
	if (error)
		goto err_pdev_del;

	return ambdev;

err_pdev_del:
	ambpriv_device_del(ambdev);
err_pdev_put:
	ambpriv_device_put(ambdev);
err_out:
	return ERR_PTR(error);
}
EXPORT_SYMBOL(ambpriv_create_bundle);

static int ambpriv_match(struct device *dev, struct device_driver *drv)
{
	struct ambpriv_device *ambdev = to_ambpriv_device(dev);

	if (of_driver_match_device(dev, drv))
		return 1;

	return (strcmp(ambdev->name, drv->name) == 0);
}

#ifdef CONFIG_PM_SLEEP

static int ambpriv_pm_prepare(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (drv && drv->pm && drv->pm->prepare)
		ret = drv->pm->prepare(dev);

	return ret;
}

static void ambpriv_pm_complete(struct device *dev)
{
	struct device_driver *drv = dev->driver;

	if (drv && drv->pm && drv->pm->complete)
		drv->pm->complete(dev);
}

#else /* !CONFIG_PM_SLEEP */

#define ambpriv_pm_prepare		NULL
#define ambpriv_pm_complete		NULL

#endif /* !CONFIG_PM_SLEEP */

#ifdef CONFIG_SUSPEND

int __weak ambpriv_pm_suspend(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->suspend)
		ret = drv->pm->suspend(dev);

	return ret;
}

int __weak ambpriv_pm_suspend_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->suspend_noirq)
		ret = drv->pm->suspend_noirq(dev);

	return ret;
}

int __weak ambpriv_pm_resume(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->resume)
		ret = drv->pm->resume(dev);

	return ret;
}

int __weak ambpriv_pm_resume_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->resume_noirq)
		ret = drv->pm->resume_noirq(dev);

	return ret;
}

#else /* !CONFIG_SUSPEND */

#define ambpriv_pm_suspend		NULL
#define ambpriv_pm_resume		NULL
#define ambpriv_pm_suspend_noirq	NULL
#define ambpriv_pm_resume_noirq	NULL

#endif /* !CONFIG_SUSPEND */

#ifdef CONFIG_HIBERNATION

static int ambpriv_pm_freeze(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->freeze)
		ret = drv->pm->freeze(dev);

	return ret;
}

static int ambpriv_pm_freeze_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->freeze_noirq)
		ret = drv->pm->freeze_noirq(dev);

	return ret;
}

static int ambpriv_pm_thaw(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->thaw)
		ret = drv->pm->thaw(dev);

	return ret;
}

static int ambpriv_pm_thaw_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->thaw_noirq)
		ret = drv->pm->thaw_noirq(dev);

	return ret;
}

static int ambpriv_pm_poweroff(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->poweroff)
		ret = drv->pm->poweroff(dev);

	return ret;
}

static int ambpriv_pm_poweroff_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->poweroff_noirq)
		ret = drv->pm->poweroff_noirq(dev);

	return ret;
}

static int ambpriv_pm_restore(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->restore)
		ret = drv->pm->restore(dev);

	return ret;
}

static int ambpriv_pm_restore_noirq(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;

	if (!drv)
		return 0;

	if (drv->pm && drv->pm->restore_noirq)
		ret = drv->pm->restore_noirq(dev);

	return ret;
}

#else /* !CONFIG_HIBERNATION */

#define ambpriv_pm_freeze		NULL
#define ambpriv_pm_thaw		NULL
#define ambpriv_pm_poweroff		NULL
#define ambpriv_pm_restore		NULL
#define ambpriv_pm_freeze_noirq	NULL
#define ambpriv_pm_thaw_noirq		NULL
#define ambpriv_pm_poweroff_noirq	NULL
#define ambpriv_pm_restore_noirq	NULL

#endif /* !CONFIG_HIBERNATION */

#ifdef CONFIG_PM_RUNTIME

int __weak ambpriv_pm_runtime_suspend(struct device *dev)
{
	return pm_generic_runtime_suspend(dev);
};

int __weak ambpriv_pm_runtime_resume(struct device *dev)
{
	return pm_generic_runtime_resume(dev);
};

int __weak ambpriv_pm_runtime_idle(struct device *dev)
{
	return pm_generic_runtime_idle(dev);
};

#else /* !CONFIG_PM_RUNTIME */

#define ambpriv_pm_runtime_suspend NULL
#define ambpriv_pm_runtime_resume NULL
#define ambpriv_pm_runtime_idle NULL

#endif /* !CONFIG_PM_RUNTIME */

static const struct dev_pm_ops ambpriv_dev_pm_ops = {
	.prepare = ambpriv_pm_prepare,
	.complete = ambpriv_pm_complete,
	.suspend = ambpriv_pm_suspend,
	.resume = ambpriv_pm_resume,
	.freeze = ambpriv_pm_freeze,
	.thaw = ambpriv_pm_thaw,
	.poweroff = ambpriv_pm_poweroff,
	.restore = ambpriv_pm_restore,
	.suspend_noirq = ambpriv_pm_suspend_noirq,
	.resume_noirq = ambpriv_pm_resume_noirq,
	.freeze_noirq = ambpriv_pm_freeze_noirq,
	.thaw_noirq = ambpriv_pm_thaw_noirq,
	.poweroff_noirq = ambpriv_pm_poweroff_noirq,
	.restore_noirq = ambpriv_pm_restore_noirq,
	.runtime_suspend = ambpriv_pm_runtime_suspend,
	.runtime_resume = ambpriv_pm_runtime_resume,
	.runtime_idle = ambpriv_pm_runtime_idle,
};

struct bus_type ambpriv_bus_type = {
	.name		= "ambpriv",
	.match		= ambpriv_match,
	.pm		= &ambpriv_dev_pm_ops,
};
EXPORT_SYMBOL(ambpriv_bus_type);

int __init ambpriv_bus_init(void)
{
	int error;

	error = device_register(&ambpriv_bus);
	if (error)
		return error;
	error = bus_register(&ambpriv_bus_type);
	if (error)
		device_unregister(&ambpriv_bus);
	return error;
}

core_initcall(ambpriv_bus_init);

