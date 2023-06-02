/*
 * ambpriv_device.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 */

#ifndef _AMBPRIV_DEVICE_H_
#define _AMBPRIV_DEVICE_H_

#include <linux/device.h>

struct ambpriv_device {
	const char	* name;
	int		id;
	struct device	dev;
	u32		num_resources;
	struct resource	* resource;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};

#define to_ambpriv_device(x) container_of((x), struct ambpriv_device, dev)

extern int ambpriv_device_register(struct ambpriv_device *);
extern void ambpriv_device_unregister(struct ambpriv_device *);

extern struct bus_type ambpriv_bus_type;
extern struct device ambpriv_bus;

extern int ambpriv_add_devices(struct ambpriv_device **, int);
extern struct ambpriv_device *ambpriv_device_alloc(const char *name, int id);
extern int ambpriv_device_add_resources(struct ambpriv_device *pdev,
					 const struct resource *res,
					 unsigned int num);
extern int ambpriv_device_add_data(struct ambpriv_device *pdev, const void *data, size_t size);
extern int ambpriv_device_add(struct ambpriv_device *pdev);
extern void ambpriv_device_del(struct ambpriv_device *pdev);
extern void ambpriv_device_put(struct ambpriv_device *pdev);

extern struct ambpriv_device *of_find_ambpriv_device_by_match(struct of_device_id *match);
extern struct ambpriv_device *of_find_ambpriv_device_by_node(struct device_node *np);
extern int ambpriv_get_irq(struct ambpriv_device *dev, unsigned int num);
extern int ambpriv_get_irq_by_name(struct ambpriv_device *dev, const char *name);

struct ambpriv_driver {
	int (*probe)(struct ambpriv_device *);
	int (*remove)(struct ambpriv_device *);
	void (*shutdown)(struct ambpriv_device *);
	int (*suspend)(struct ambpriv_device *, pm_message_t state);
	int (*resume)(struct ambpriv_device *);
	struct device_driver driver;
};

extern int ambpriv_driver_register(struct ambpriv_driver *);
extern void ambpriv_driver_unregister(struct ambpriv_driver *);

#define ambpriv_get_drvdata(_dev)	dev_get_drvdata(&(_dev)->dev)
#define ambpriv_set_drvdata(_dev,data)	dev_set_drvdata(&(_dev)->dev, (data))

extern struct ambpriv_device *ambpriv_create_bundle(struct ambpriv_driver *driver,
					struct resource *res, unsigned int n_res,
					const void *data, size_t size);

extern int ambpriv_i2c_update_addr(const char *name, int bus, int addr);

#endif /* _PLATFORM_DEVICE_H_ */

