// SPDX-License-Identifier: GPL-2.0
/**
 * configfs to configure VHOST
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/configfs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/vhost.h>
#include <linux/slab.h>

/* VHOST driver like net, scsi etc., */
static struct config_group *vhost_driver_group;

/* VHOST device like PCIe EP, NTB etc., */
static struct config_group *vhost_device_group;

struct vhost_driver_group {
	struct config_group group;
};

struct vhost_device_item {
	struct config_group group;
	struct vhost_dev *vdev;
};

static inline
struct vhost_device_item *to_vhost_device_item(struct config_item *item)
{
	return container_of(to_config_group(item), struct vhost_device_item,
			    group);
}

/**
 * vhost_cfs_device_link() - Create softlink of driver directory to device
 *   directory
 * @device_item: Represents configfs entry of vhost_dev
 * @driver_item: Represents configfs of a particular entry of
 *   vhost_device_id table in vhost driver
 *
 * Bind a vhost driver to vhost device in order to assign a particular
 * device ID and vendor ID
 */
static int vhost_cfs_device_link(struct config_item *device_item,
				 struct config_item *driver_item)
{
	struct vhost_driver_item *vdriver_item;
	struct vhost_device_item *vdevice_item;
	struct vhost_dev *vdev;
	int ret;

	vdriver_item = to_vhost_driver_item(driver_item);
	vdevice_item = to_vhost_device_item(device_item);

	vdev = vdevice_item->vdev;
	vdev->id.device = vdriver_item->device;
	vdev->id.vendor = vdriver_item->vendor;

	ret = vhost_register_device(vdev);
	if (ret)
		return ret;

	return 0;
}

/**
 * vhost_cfs_device_unlink() - Delete softlink of driver directory from device
 *   directory
 * @device_item: Represents configfs entry of vhost_dev
 * @driver_item: Represents configfs of a particular entry of
 *   vhost_device_id table in vhost driver
 *
 * Un-bind vhost driver from vhost device.
 */
static void vhost_cfs_device_unlink(struct config_item *device_item,
				    struct config_item *driver_item)
{
	struct vhost_driver_item *vdriver_item;
	struct vhost_device_item *vdevice_item;
	struct vhost_dev *vdev;

	vdriver_item = to_vhost_driver_item(driver_item);
	vdevice_item = to_vhost_device_item(device_item);

	vdev = vdevice_item->vdev;
	vhost_unregister_device(vdev);
}

static struct configfs_item_operations vhost_cfs_device_item_ops = {
	.allow_link	= vhost_cfs_device_link,
	.drop_link	= vhost_cfs_device_unlink,
};

static const struct config_item_type vhost_cfs_device_item_type = {
	.ct_item_ops	= &vhost_cfs_device_item_ops,
	.ct_owner	= THIS_MODULE,
};

/**
 * vhost_cfs_add_device_item() - Create configfs directory for new vhost_dev
 * @vdev: vhost device for which configfs directory has to be created
 *
 * Create configfs directory for new vhost device. Drivers that create
 * vhost device can invoke this API if they require the vhost device to
 * be assigned a device ID and vendorID by the user.
 */
struct config_group *vhost_cfs_add_device_item(struct vhost_dev *vdev)
{
	struct device *dev = &vdev->dev;
	struct vhost_device_item *vdevice_item;
	struct config_group *group;
	const char *name;
	int ret;

	vdevice_item = kzalloc(sizeof(*vdevice_item), GFP_KERNEL);
	if (!vdevice_item)
		return ERR_PTR(-ENOMEM);

	name = dev_name(dev->parent);
	group = &vdevice_item->group;
	config_group_init_type_name(group, name, &vhost_cfs_device_item_type);

	ret = configfs_register_group(vhost_device_group, group);
	if (ret)
		return ERR_PTR(ret);

	vdevice_item->vdev = vdev;

	return group;
}
EXPORT_SYMBOL(vhost_cfs_add_device_item);

/**
 * vhost_cfs_remove_device_item() - Remove configfs directory for the vhost_dev
 * @vdev: vhost device for which configfs directory has to be removed
 *
 * Remove configfs directory for the vhost device.
 */
void vhost_cfs_remove_device_item(struct config_group *group)
{
	struct vhost_device_item *vdevice_item;

	if (!group)
		return;

	vdevice_item = container_of(group, struct vhost_device_item, group);
	configfs_unregister_group(&vdevice_item->group);
	kfree(vdevice_item);
}
EXPORT_SYMBOL(vhost_cfs_remove_device_item);

static const struct config_item_type vhost_driver_item_type = {
	.ct_owner	= THIS_MODULE,
};

/**
 * vhost_cfs_add_driver_item() - Add configfs directory for an entry in
 *   vhost_device_id
 * @driver_group: configfs directory corresponding to the vhost driver
 * @vendor: vendor ID populated in vhost_device_id table by vhost driver
 * @device: device ID populated in vhost_device_id table by vhost driver
 *
 * Add configfs directory for each entry in vhost_device_id populated by
 * vhost driver. Store the device ID and vendor ID in a local data structure
 * and use it when user links this directory with a vhost device configfs
 * directory.
 */
struct config_group *
vhost_cfs_add_driver_item(struct config_group *driver_group, u32 vendor,
			  u32 device)
{
	struct vhost_driver_item *vdriver_item;
	struct config_group *group;
	char name[20];
	int ret;

	vdriver_item = kzalloc(sizeof(*vdriver_item), GFP_KERNEL);
	if (!vdriver_item)
		return ERR_PTR(-ENOMEM);

	vdriver_item->vendor = vendor;
	vdriver_item->device = device;

	snprintf(name, sizeof(name), "%08x:%08x", vendor, device);
	group = &vdriver_item->group;

	config_group_init_type_name(group, name, &vhost_driver_item_type);
	ret = configfs_register_group(driver_group, group);
	if (ret)
		return ERR_PTR(ret);

	return group;
}
EXPORT_SYMBOL(vhost_cfs_add_driver_item);

/**
 * vhost_cfs_remove_driver_item() - Remove configfs directory corresponding
 *   to an entry in vhost_device_id
 * @group: Configfs group corresponding to an entry in vhost_device_id
 *
 * Remove configfs directory corresponding to an entry in vhost_device_id
 */
void vhost_cfs_remove_driver_item(struct config_group *group)
{
	struct vhost_driver_item *vdriver_item;

	if (!group)
		return;

	vdriver_item = container_of(group, struct vhost_driver_item, group);
	configfs_unregister_group(&vdriver_item->group);
	kfree(vdriver_item);
}
EXPORT_SYMBOL(vhost_cfs_remove_driver_item);

static const struct config_item_type vhost_driver_group_type = {
	.ct_owner	= THIS_MODULE,
};

/**
 * vhost_cfs_add_driver_group() - Add configfs directory for vhost driver
 * @name: Name of the vhost driver as populated in driver structure
 *
 * Add configfs directory for vhost driver.
 */
struct config_group *vhost_cfs_add_driver_group(const char *name)
{
	struct vhost_driver_group *vdriver_group;
	struct config_group *group;

	vdriver_group = kzalloc(sizeof(*vdriver_group), GFP_KERNEL);
	if (!vdriver_group)
		return ERR_PTR(-ENOMEM);

	group = &vdriver_group->group;

	config_group_init_type_name(group, name, &vhost_driver_group_type);
	configfs_register_group(vhost_driver_group, group);

	return group;
}
EXPORT_SYMBOL(vhost_cfs_add_driver_group);

/**
 * vhost_cfs_remove_driver_group() - Remove configfs directory for vhost driver
 * @group: Configfs group corresponding to the vhost driver
 *
 * Remove configfs directory for vhost driver.
 */
void vhost_cfs_remove_driver_group(struct config_group *group)
{
	if (IS_ERR_OR_NULL(group))
		return;

	configfs_unregister_default_group(group);
}
EXPORT_SYMBOL(vhost_cfs_remove_driver_group);

static const struct config_item_type vhost_driver_type = {
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type vhost_device_type = {
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type vhost_type = {
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem vhost_cfs_subsys = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "vhost",
			.ci_type = &vhost_type,
		},
	},
	.su_mutex = __MUTEX_INITIALIZER(vhost_cfs_subsys.su_mutex),
};

static int __init vhost_cfs_init(void)
{
	int ret;
	struct config_group *root = &vhost_cfs_subsys.su_group;

	config_group_init(root);

	ret = configfs_register_subsystem(&vhost_cfs_subsys);
	if (ret) {
		pr_err("Error %d while registering subsystem %s\n",
		       ret, root->cg_item.ci_namebuf);
		goto err;
	}

	vhost_driver_group =
		configfs_register_default_group(root, "vhost-client",
						&vhost_driver_type);
	if (IS_ERR(vhost_driver_group)) {
		ret = PTR_ERR(vhost_driver_group);
		pr_err("Error %d while registering channel group\n",
		       ret);
		goto err_vhost_driver_group;
	}

	vhost_device_group =
		configfs_register_default_group(root, "vhost-transport",
						&vhost_device_type);
	if (IS_ERR(vhost_device_group)) {
		ret = PTR_ERR(vhost_device_group);
		pr_err("Error %d while registering virtproc group\n",
		       ret);
		goto err_vhost_device_group;
	}

	return 0;

err_vhost_device_group:
	configfs_unregister_default_group(vhost_driver_group);

err_vhost_driver_group:
	configfs_unregister_subsystem(&vhost_cfs_subsys);

err:
	return ret;
}
core_initcall(vhost_cfs_init);

static void __exit vhost_cfs_exit(void)
{
	configfs_unregister_default_group(vhost_device_group);
	configfs_unregister_default_group(vhost_driver_group);
	configfs_unregister_subsystem(&vhost_cfs_subsys);
}
module_exit(vhost_cfs_exit);

MODULE_DESCRIPTION("PCI VHOST CONFIGFS");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
