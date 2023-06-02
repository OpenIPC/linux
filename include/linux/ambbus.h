/*
 * AMB bus.
 */

#ifndef __LINUX_AMBBUS_H
#define __LINUX_AMBBUS_H

#include <linux/device.h>
#include <linux/kernel.h>

struct amb_driver {
	int (*match)(struct device *);
	int (*probe)(struct device *);
	int (*remove)(struct device *);
	void (*shutdown)(struct device *);
	int (*suspend)(struct device *, pm_message_t);
	int (*resume)(struct device *);

	struct device_driver driver;
	struct device *devices;
};

#define to_amb_driver(x) container_of((x), struct amb_driver, driver)


int amb_register_driver(struct amb_driver *);
void amb_unregister_driver(struct amb_driver *);

#endif /* __LINUX_AMBBUS_H */
