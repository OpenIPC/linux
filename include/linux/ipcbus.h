/*
 * IPC bus.
 */

#ifndef __LINUX_IPCBUS_H
#define __LINUX_IPCBUS_H

#include <linux/device.h>
#include <linux/kernel.h>

struct ipc_driver {
	int (*match)(struct device *);
	int (*probe)(struct device *);
	int (*remove)(struct device *);
	void (*shutdown)(struct device *);
	int (*suspend)(struct device *, pm_message_t);
	int (*resume)(struct device *);

	struct device_driver driver;
	struct device *devices;
};

#define to_ipc_driver(x) container_of((x), struct ipc_driver, driver)


int ipc_register_driver(struct ipc_driver *);
void ipc_unregister_driver(struct ipc_driver *);

#endif /* __LINUX_IPCBUS_H */
