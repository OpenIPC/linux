// SPDX-License-Identifier: GPL-2.0
/**
 * configfs to configure RPMSG
 *
 * Copyright (C) 2020 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/configfs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/idr.h>
#include <linux/slab.h>

#include "rpmsg_internal.h"

static struct config_group *channel_group;
static struct config_group *virtproc_group;

enum rpmsg_channel_status {
	STATUS_FREE,
	STATUS_BUSY,
};

struct rpmsg_channel {
	struct config_item item;
	struct device *dev;
	enum rpmsg_channel_status status;
};

struct rpmsg_channel_group {
	struct config_group group;
};

struct rpmsg_virtproc_group {
	struct config_group group;
	struct device *dev;
	const struct rpmsg_virtproc_ops *ops;
};

static inline
struct rpmsg_channel *to_rpmsg_channel(struct config_item *channel_item)
{
	return container_of(channel_item, struct rpmsg_channel, item);
}

static inline struct rpmsg_channel_group
*to_rpmsg_channel_group(struct config_group *channel_group)
{
	return container_of(channel_group, struct rpmsg_channel_group, group);
}

static inline
struct rpmsg_virtproc_group *to_rpmsg_virtproc_group(struct config_item *item)
{
	return container_of(to_config_group(item), struct rpmsg_virtproc_group,
			    group);
}

/**
 * rpmsg_virtproc_channel_link() - Create softlink of rpmsg client device
 *   directory to virtproc configfs directory
 * @virtproc_item: Config item representing configfs entry of virtual remote
 *   processor
 * @channel_item: Config item representing configfs entry of rpmsg client
 *   driver
 *
 * Bind rpmsg client device to virtual remote processor by creating softlink
 * between rpmsg client device directory to virtproc configfs directory
 * in order to create a new rpmsg channel.
 */
static int rpmsg_virtproc_channel_link(struct config_item *virtproc_item,
				       struct config_item *channel_item)
{
	struct rpmsg_virtproc_group *vgroup;
	struct rpmsg_channel *channel;
	struct config_group *cgroup;
	struct device *dev = NULL;

	vgroup = to_rpmsg_virtproc_group(virtproc_item);
	channel = to_rpmsg_channel(channel_item);

	if (channel->status == STATUS_BUSY)
		return -EBUSY;

	cgroup = channel_item->ci_group;

	if (vgroup->ops && vgroup->ops->create_channel) {
		dev = vgroup->ops->create_channel(vgroup->dev,
						  cgroup->cg_item.ci_name);
		if (IS_ERR_OR_NULL(dev))
			return PTR_ERR(dev);
	}

	channel->dev = dev;
	channel->status = STATUS_BUSY;

	return 0;
}

/**
 * rpmsg_virtproc_channel_unlink() - Remove softlink of rpmsg client device
 *   directory from virtproc configfs directory
 * @virtproc_item: Config item representing configfs entry of virtual remote
 *   processor
 * @channel_item: Config item representing configfs entry of rpmsg client
 *   driver
 *
 * Unbind rpmsg client device from virtual remote processor by removing softlink
 * of rpmsg client device directory from virtproc configfs directory which
 * deletes the rpmsg channel.
 */
static void rpmsg_virtproc_channel_unlink(struct config_item *virtproc_item,
					  struct config_item *channel_item)
{
	struct rpmsg_virtproc_group *vgroup;
	struct rpmsg_channel *channel;

	channel = to_rpmsg_channel(channel_item);
	vgroup = to_rpmsg_virtproc_group(virtproc_item);

	if (vgroup->ops && vgroup->ops->delete_channel && channel->dev)
		vgroup->ops->delete_channel(channel->dev);

	channel->status = STATUS_FREE;
}

static struct configfs_item_operations rpmsg_virtproc_item_ops = {
	.allow_link	= rpmsg_virtproc_channel_link,
	.drop_link	= rpmsg_virtproc_channel_unlink,
};

static const struct config_item_type rpmsg_virtproc_item_type = {
	.ct_item_ops	= &rpmsg_virtproc_item_ops,
	.ct_owner	= THIS_MODULE,
};

/**
 * rpmsg_cfs_add_virtproc_group() - Add new configfs directory for virtproc
 *   device
 * @dev: Device representing the virtual remote processor
 * @ops: rpmsg_virtproc_ops to create or delete rpmsg channel
 *
 * Add new configfs directory for virtproc device. The rpmsg client driver's
 * configfs entry can be linked with this directory for creating a new
 * rpmsg channel and the link can be removed for deleting the rpmsg channel.
 */
struct config_group *
rpmsg_cfs_add_virtproc_group(struct device *dev,
			     const struct rpmsg_virtproc_ops *ops)
{
	struct rpmsg_virtproc_group *vgroup;
	struct config_group *group;
	struct device *vdev;
	int ret;

	vgroup = kzalloc(sizeof(*vgroup), GFP_KERNEL);
	if (!vgroup)
		return ERR_PTR(-ENOMEM);

	group = &vgroup->group;
	config_group_init_type_name(group, dev_name(dev),
				    &rpmsg_virtproc_item_type);
	ret = configfs_register_group(virtproc_group, group);
	if (ret)
		goto err_register_group;

	if (!try_module_get(ops->owner)) {
		ret = -EPROBE_DEFER;
		goto err_module_get;
	}

	vdev = get_device(dev);
	vgroup->dev = vdev;
	vgroup->ops = ops;

	return group;

err_module_get:
	configfs_unregister_group(group);

err_register_group:
	kfree(vgroup);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(rpmsg_cfs_add_virtproc_group);

/**
 * rpmsg_cfs_remove_virtproc_group() - Remove the configfs directory for
 *   virtproc device
 * @group: config_group of the virtproc device
 *
 * Remove the configfs directory for virtproc device.
 */
void rpmsg_cfs_remove_virtproc_group(struct config_group *group)
{
	struct rpmsg_virtproc_group *vgroup;

	if (!group)
		return;

	vgroup = container_of(group, struct rpmsg_virtproc_group, group);
	put_device(vgroup->dev);
	module_put(vgroup->ops->owner);
	configfs_unregister_group(&vgroup->group);
	kfree(vgroup);
}
EXPORT_SYMBOL(rpmsg_cfs_remove_virtproc_group);

static const struct config_item_type rpmsg_channel_item_type = {
	.ct_owner	= THIS_MODULE,
};

/**
 * rpmsg_channel_make() - Allow user to create sub-directory of rpmsg client
 *   driver
 * @name: Name of the sub-directory created by the user.
 *
 * Invoked when user creates a sub-directory to the configfs directory
 * representing the rpmsg client driver. This can be linked with the virtproc
 * directory for creating a new rpmsg channel.
 */
static struct config_item *
rpmsg_channel_make(struct config_group *group, const char *name)
{
	struct rpmsg_channel *channel;

	channel = kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel)
		return ERR_PTR(-ENOMEM);

	channel->status = STATUS_FREE;

	config_item_init_type_name(&channel->item, name, &rpmsg_channel_item_type);
	return &channel->item;
}

/**
 * rpmsg_channel_drop() - Allow user to delete sub-directory of rpmsg client
 *   driver
 * @item: Config item representing the sub-directory the user created returned
 *   by rpmsg_channel_make()
 *
 * Invoked when user creates a sub-directory to the configfs directory
 * representing the rpmsg client driver. This can be linked with the virtproc
 * directory for creating a new rpmsg channel.
 */
static void rpmsg_channel_drop(struct config_group *group, struct config_item *item)
{
	struct rpmsg_channel *channel;

	channel = to_rpmsg_channel(item);
	kfree(channel);
}

static struct configfs_group_operations rpmsg_channel_group_ops = {
	.make_item     = &rpmsg_channel_make,
	.drop_item      = &rpmsg_channel_drop,
};

static const struct config_item_type rpmsg_channel_group_type = {
	.ct_group_ops	= &rpmsg_channel_group_ops,
	.ct_owner	= THIS_MODULE,
};

/**
 * rpmsg_cfs_add_channel_group() - Create a configfs directory for each
 *   registered rpmsg client driver
 * @name: The name of the rpmsg client driver
 *
 * Create a configfs directory for each registered rpmsg client driver. The
 * user can create sub-directory within this directory for creating
 * rpmsg channels to be used by the rpmsg client driver.
 */
struct config_group *rpmsg_cfs_add_channel_group(const char *name)
{
	struct rpmsg_channel_group *cgroup;
	struct config_group *group;
	int ret;

	cgroup = kzalloc(sizeof(*cgroup), GFP_KERNEL);
	if (!cgroup)
		return ERR_PTR(-ENOMEM);

	group = &cgroup->group;
	config_group_init_type_name(group, name, &rpmsg_channel_group_type);
	ret = configfs_register_group(channel_group, group);
	if (ret)
		return ERR_PTR(ret);

	return group;
}
EXPORT_SYMBOL(rpmsg_cfs_add_channel_group);

/**
 * rpmsg_cfs_remove_channel_group() - Remove the configfs directory associated
 *   with the rpmsg client driver
 * @group: Config group representing the rpmsg client driver
 *
 * Remove the configfs directory associated with the rpmsg client driver.
 */
void rpmsg_cfs_remove_channel_group(struct config_group *group)
{
	struct rpmsg_channel_group *cgroup;

	if (IS_ERR_OR_NULL(group))
		return;

	cgroup = to_rpmsg_channel_group(group);
	configfs_unregister_default_group(group);
	kfree(cgroup);
}
EXPORT_SYMBOL(rpmsg_cfs_remove_channel_group);

static const struct config_item_type rpmsg_channel_type = {
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type rpmsg_virtproc_type = {
	.ct_owner	= THIS_MODULE,
};

static const struct config_item_type rpmsg_type = {
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem rpmsg_cfs_subsys = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "rpmsg",
			.ci_type = &rpmsg_type,
		},
	},
	.su_mutex = __MUTEX_INITIALIZER(rpmsg_cfs_subsys.su_mutex),
};

static int __init rpmsg_cfs_init(void)
{
	int ret;
	struct config_group *root = &rpmsg_cfs_subsys.su_group;

	config_group_init(root);

	ret = configfs_register_subsystem(&rpmsg_cfs_subsys);
	if (ret) {
		pr_err("Error %d while registering subsystem %s\n",
		       ret, root->cg_item.ci_namebuf);
		goto err;
	}

	channel_group = configfs_register_default_group(root, "channel",
							&rpmsg_channel_type);
	if (IS_ERR(channel_group)) {
		ret = PTR_ERR(channel_group);
		pr_err("Error %d while registering channel group\n",
		       ret);
		goto err_channel_group;
	}

	virtproc_group =
		configfs_register_default_group(root, "virtproc",
						&rpmsg_virtproc_type);
	if (IS_ERR(virtproc_group)) {
		ret = PTR_ERR(virtproc_group);
		pr_err("Error %d while registering virtproc group\n",
		       ret);
		goto err_virtproc_group;
	}

	return 0;

err_virtproc_group:
	configfs_unregister_default_group(channel_group);

err_channel_group:
	configfs_unregister_subsystem(&rpmsg_cfs_subsys);

err:
	return ret;
}
postcore_initcall(rpmsg_cfs_init);

static void __exit rpmsg_cfs_exit(void)
{
	configfs_unregister_default_group(virtproc_group);
	configfs_unregister_default_group(channel_group);
	configfs_unregister_subsystem(&rpmsg_cfs_subsys);
}
module_exit(rpmsg_cfs_exit);

MODULE_DESCRIPTION("PCI RPMSG CONFIGFS");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
