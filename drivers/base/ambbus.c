/*
 * AMB bus.
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/ambbus.h>

static struct device amb_bus = {
	.init_name	= "ambbus"
};

struct amb_dev {
	struct device dev;
	struct device *next;
};

#define to_amb_dev(x) container_of((x), struct amb_dev, dev)

static int amb_bus_match(struct device *dev, struct device_driver *driver)
{
	struct amb_driver *amb_driver = to_amb_driver(driver);

	if (dev->platform_data == amb_driver) {
		if (!amb_driver->match || amb_driver->match(dev))
			return 1;
		dev->platform_data = NULL;
	}
	return 0;
}

static int amb_bus_probe(struct device *dev)
{
	struct amb_driver *amb_driver = dev->platform_data;

	if (amb_driver->probe)
		return amb_driver->probe(dev);

	return 0;
}

static int amb_bus_remove(struct device *dev)
{
	struct amb_driver *amb_driver = dev->platform_data;

	if (amb_driver->remove)
		return amb_driver->remove(dev);

	return 0;
}

static void amb_bus_shutdown(struct device *dev)
{
	struct amb_driver *amb_driver = dev->platform_data;

	if (amb_driver->shutdown)
		amb_driver->shutdown(dev);
}

static int amb_bus_suspend(struct device *dev, pm_message_t state)
{
	struct amb_driver *amb_driver = dev->platform_data;

	if (amb_driver->suspend)
		return amb_driver->suspend(dev, state);

	return 0;
}

static int amb_bus_resume(struct device *dev)
{
	struct amb_driver *amb_driver = dev->platform_data;

	if (amb_driver->resume)
		return amb_driver->resume(dev);

	return 0;
}

static struct bus_type amb_bus_type = {
	.name		= "ambbus",
	.match		= amb_bus_match,
	.probe		= amb_bus_probe,
	.remove		= amb_bus_remove,
	.shutdown	= amb_bus_shutdown,
	.suspend	= amb_bus_suspend,
	.resume		= amb_bus_resume
};

static void amb_dev_release(struct device *dev)
{
	kfree(to_amb_dev(dev));
}

void amb_unregister_driver(struct amb_driver *amb_driver)
{
	struct device *dev = amb_driver->devices;

	while (dev) {
		struct device *tmp = to_amb_dev(dev)->next;
		device_unregister(dev);
		dev = tmp;
	}
	driver_unregister(&amb_driver->driver);
}
EXPORT_SYMBOL(amb_unregister_driver);

int amb_register_driver(struct amb_driver *amb_driver)
{
	int error;
	unsigned int id;

	amb_driver->driver.bus	= &amb_bus_type;
	amb_driver->devices	= NULL;

	error = driver_register(&amb_driver->driver);
	if (error)
		return error;

	for (id = 0; id < 1; id++) {
		struct amb_dev *amb_dev;

		amb_dev = kzalloc(sizeof *amb_dev, GFP_KERNEL);
		if (!amb_dev) {
			error = -ENOMEM;
			break;
		}

		amb_dev->dev.parent	= &amb_bus;
		amb_dev->dev.bus	= &amb_bus_type;

		dev_set_name(&amb_dev->dev, "%s",
			     amb_driver->driver.name);
		amb_dev->dev.platform_data	= amb_driver;
		amb_dev->dev.release		= amb_dev_release;

		amb_dev->dev.coherent_dma_mask = DMA_BIT_MASK(24);
		amb_dev->dev.dma_mask = &amb_dev->dev.coherent_dma_mask;

		error = device_register(&amb_dev->dev);
		if (error) {
			put_device(&amb_dev->dev);
			break;
		}

		if (amb_dev->dev.platform_data) {
			amb_dev->next = amb_driver->devices;
			amb_driver->devices = &amb_dev->dev;
		} else
			device_unregister(&amb_dev->dev);
	}

	if (!error && !amb_driver->devices)
		error = -ENODEV;

	if (error)
		amb_unregister_driver(amb_driver);

	return error;
}
EXPORT_SYMBOL(amb_register_driver);

static int __init amb_bus_init(void)
{
	int error;

	error = bus_register(&amb_bus_type);
	if (!error) {
		error = device_register(&amb_bus);
		if (error)
			bus_unregister(&amb_bus_type);
	}
	return error;
}

device_initcall(amb_bus_init);
