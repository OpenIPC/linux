// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626-owned subdevice nodes. Standard format, control and event dispatch
 * derives from drivers/media/v4l2-core/v4l2-subdev.c (Laurent Pinchart).
 * Node storage, supplier controls and TRY state survive supplier unbind.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <media/fh8626-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include "fh8626-pipeline.h"
#include "fh8626-event.h"

struct fh8626_subdev_node {
	struct video_device vdev;
	struct kref ref;
	struct mutex lock;
	struct fh8626_subdev_lifetime lifetime;
	struct module *supplier;
};

static int fh8626_subdev_open(struct file *file)
{
	struct video_device *vd = video_devdata(file);
	struct v4l2_subdev *sd = video_get_drvdata(vd);
	struct v4l2_subdev_fh *fh;
	int ret = 0;

	if (mutex_lock_interruptible(vd->lock))
		return -ERESTARTSYS;
	if (!video_is_registered(vd)) {
		ret = -ENODEV;
		goto out;
	}
	fh = kzalloc(sizeof(*fh), GFP_KERNEL);
	if (!fh) {
		ret = -ENOMEM;
		goto out;
	}
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	fh->pad = v4l2_subdev_alloc_pad_config(sd);
	if (sd->entity.num_pads && !fh->pad) {
		kfree(fh);
		ret = -ENOMEM;
		goto out;
	}
#endif
	v4l2_fh_init(&fh->vfh, vd);
	v4l2_fh_add(&fh->vfh);
	file->private_data = &fh->vfh;
out:
	mutex_unlock(vd->lock);
	return ret;
}

static int fh8626_subdev_close(struct file *file)
{
	struct v4l2_fh *vfh = file->private_data;
	struct v4l2_subdev_fh *fh = to_v4l2_subdev_fh(vfh);

	v4l2_fh_del(vfh);
	v4l2_fh_exit(vfh);
#ifdef CONFIG_VIDEO_V4L2_SUBDEV_API
	v4l2_subdev_free_pad_config(fh->pad);
#endif
	kfree(fh);
	return 0;
}

#if defined(CONFIG_VIDEO_V4L2_SUBDEV_API)
static int check_format(struct v4l2_subdev *sd,
			struct v4l2_subdev_format *format)
{
	if (format->which != V4L2_SUBDEV_FORMAT_TRY &&
	    format->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	if (format->pad >= sd->entity.num_pads)
		return -EINVAL;

	return 0;
}

#endif

static long subdev_do_ioctl(struct file *file, unsigned int cmd, void *arg)
{
	struct video_device *vdev = video_devdata(file);
	struct v4l2_subdev *sd = vdev_to_v4l2_subdev(vdev);
	struct v4l2_fh *vfh = file->private_data;
#if defined(CONFIG_VIDEO_V4L2_SUBDEV_API)
	struct v4l2_subdev_fh *subdev_fh = to_v4l2_subdev_fh(vfh);
	int rval;
#endif

	switch (cmd) {
	case VIDIOC_QUERYCTRL:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_queryctrl(vfh->ctrl_handler, arg);

	case VIDIOC_QUERY_EXT_CTRL:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_query_ext_ctrl(vfh->ctrl_handler, arg);

	case VIDIOC_QUERYMENU:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_querymenu(vfh->ctrl_handler, arg);

	case VIDIOC_G_CTRL:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_g_ctrl(vfh->ctrl_handler, arg);

	case VIDIOC_S_CTRL:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_s_ctrl(vfh, vfh->ctrl_handler, arg);

	case VIDIOC_G_EXT_CTRLS:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_g_ext_ctrls(vfh->ctrl_handler, arg);

	case VIDIOC_S_EXT_CTRLS:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_s_ext_ctrls(vfh, vfh->ctrl_handler, arg);

	case VIDIOC_TRY_EXT_CTRLS:
		if (!vfh->ctrl_handler)
			return -ENOTTY;
		return v4l2_try_ext_ctrls(vfh->ctrl_handler, arg);


	case VIDIOC_SUBSCRIBE_EVENT:
		return v4l2_subdev_call(sd, core, subscribe_event, vfh, arg);

	case VIDIOC_UNSUBSCRIBE_EVENT:
		return v4l2_subdev_call(sd, core, unsubscribe_event, vfh, arg);


#if defined(CONFIG_VIDEO_V4L2_SUBDEV_API)
	case VIDIOC_SUBDEV_G_FMT: {
		struct v4l2_subdev_format *format = arg;

		rval = check_format(sd, format);
		if (rval)
			return rval;

		memset(format->reserved, 0, sizeof(format->reserved));
		memset(format->format.reserved, 0, sizeof(format->format.reserved));
		return v4l2_subdev_call(sd, pad, get_fmt, subdev_fh->pad, format);
	}

	case VIDIOC_SUBDEV_S_FMT: {
		struct v4l2_subdev_format *format = arg;

		rval = check_format(sd, format);
		if (rval)
			return rval;

		memset(format->reserved, 0, sizeof(format->reserved));
		memset(format->format.reserved, 0, sizeof(format->format.reserved));
		return v4l2_subdev_call(sd, pad, set_fmt, subdev_fh->pad, format);
	}


	case VIDIOC_SUBDEV_ENUM_MBUS_CODE: {
		struct v4l2_subdev_mbus_code_enum *code = arg;

		if (code->which != V4L2_SUBDEV_FORMAT_TRY &&
		    code->which != V4L2_SUBDEV_FORMAT_ACTIVE)
			return -EINVAL;

		if (code->pad >= sd->entity.num_pads)
			return -EINVAL;

		memset(code->reserved, 0, sizeof(code->reserved));
		return v4l2_subdev_call(sd, pad, enum_mbus_code, subdev_fh->pad,
					code);
	}

	case VIDIOC_SUBDEV_ENUM_FRAME_SIZE: {
		struct v4l2_subdev_frame_size_enum *fse = arg;

		if (fse->which != V4L2_SUBDEV_FORMAT_TRY &&
		    fse->which != V4L2_SUBDEV_FORMAT_ACTIVE)
			return -EINVAL;

		if (fse->pad >= sd->entity.num_pads)
			return -EINVAL;

		memset(fse->reserved, 0, sizeof(fse->reserved));
		return v4l2_subdev_call(sd, pad, enum_frame_size, subdev_fh->pad,
					fse);
	}

	case VIDIOC_SUBDEV_G_FRAME_INTERVAL: {
		struct v4l2_subdev_frame_interval *fi = arg;

		if (fi->pad >= sd->entity.num_pads)
			return -EINVAL;

		memset(fi->reserved, 0, sizeof(fi->reserved));
		return v4l2_subdev_call(sd, video, g_frame_interval, arg);
	}

	case VIDIOC_SUBDEV_S_FRAME_INTERVAL: {
		struct v4l2_subdev_frame_interval *fi = arg;

		if (fi->pad >= sd->entity.num_pads)
			return -EINVAL;

		memset(fi->reserved, 0, sizeof(fi->reserved));
		return v4l2_subdev_call(sd, video, s_frame_interval, arg);
	}

	case VIDIOC_SUBDEV_ENUM_FRAME_INTERVAL: {
		struct v4l2_subdev_frame_interval_enum *fie = arg;

		if (fie->which != V4L2_SUBDEV_FORMAT_TRY &&
		    fie->which != V4L2_SUBDEV_FORMAT_ACTIVE)
			return -EINVAL;

		if (fie->pad >= sd->entity.num_pads)
			return -EINVAL;

		memset(fie->reserved, 0, sizeof(fie->reserved));
		return v4l2_subdev_call(sd, pad, enum_frame_interval, subdev_fh->pad,
					fie);
	}

#endif
	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}


static long fh8626_subdev_do_ioctl(struct file *file, unsigned int cmd,
				  void *arg)
{
	struct video_device *vd = video_devdata(file);
	struct v4l2_subdev *sd = video_get_drvdata(vd);
	long ret;

	/* Never sleep for an event while holding the disconnect mutex. */
	if (cmd == VIDIOC_DQEVENT) {
		if (!(sd->flags & V4L2_SUBDEV_FL_HAS_EVENTS))
			return -ENOIOCTLCMD;
		return fh8626_dequeue_event(file, cmd, arg);
	}
	if (mutex_lock_interruptible(vd->lock))
		return -ERESTARTSYS;
	ret = video_is_registered(vd) ? subdev_do_ioctl(file, cmd, arg) :
		-ENODEV;
	mutex_unlock(vd->lock);
	return ret;
}

static long fh8626_subdev_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	return video_usercopy(file, cmd, arg, fh8626_subdev_do_ioctl);
}

static __poll_t fh8626_subdev_poll(struct file *file, poll_table *wait)
{
	struct video_device *vd = video_devdata(file);
	struct v4l2_subdev *sd = video_get_drvdata(vd);
	struct v4l2_fh *fh = file->private_data;

	poll_wait(file, &fh->wait, wait);
	if (!video_is_registered(vd))
		return EPOLLERR | EPOLLHUP;
	if (!(sd->flags & V4L2_SUBDEV_FL_HAS_EVENTS))
		return EPOLLERR;
	return v4l2_event_pending(fh) ? EPOLLPRI : 0;
}

static const struct v4l2_file_operations fh8626_subdev_fops = {
	.owner = THIS_MODULE,
	.open = fh8626_subdev_open,
	.release = fh8626_subdev_close,
	.unlocked_ioctl = fh8626_subdev_ioctl,
	.poll = fh8626_subdev_poll,
};

static void fh8626_subdev_node_free(struct kref *ref)
{
	struct fh8626_subdev_node *node =
		container_of(ref, struct fh8626_subdev_node, ref);
	struct module *supplier = node->supplier;

	kref_put(node->lifetime.ref, node->lifetime.release);
	mutex_destroy(&node->lock);
	kfree(node);
	module_put(supplier);
}

static void fh8626_subdev_node_release(struct video_device *vd)
{
	struct fh8626_subdev_node *node =
		container_of(vd, struct fh8626_subdev_node, vdev);

	kref_put(&node->ref, fh8626_subdev_node_free);
}

void fh8626_video_withdraw_interface(struct video_device *vd)
{
#ifdef CONFIG_MEDIA_CONTROLLER
	struct media_interface *intf;
	struct media_device *mdev = vd->v4l2_dev->mdev;

	if (!mdev || !vd->intf_devnode)
		return;
	intf = &vd->intf_devnode->intf;
	mutex_lock(&mdev->graph_mutex);
	__media_remove_intf_links(intf);
	/*
	 * Withdraw the graph object now, under its lock. The allocation stays
	 * owned by video core until final release, where mdev == NULL makes
	 * graph removal a no-op even if an old fd survives a rebind.
	 */
	media_gobj_destroy(&intf->graph_obj);
	mutex_unlock(&mdev->graph_mutex);
#endif
}

void fh8626_subdev_nodes_unregister(struct fh8626_pipeline *pipe)
{
	struct v4l2_subdev *sd;

	/* Async notifier callbacks serialize publication and withdrawal. */
	list_for_each_entry(sd, &pipe->v4l2_dev.subdevs, list) {
		struct video_device *vd = sd->devnode;

		if (!vd)
			continue;
		/* The temporary reference keeps vd->lock alive across unregister. */
		get_device(&vd->dev);
		mutex_lock(vd->lock);
		sd->devnode = NULL;
		fh8626_video_withdraw_interface(vd);
		video_unregister_device(vd);
		fh8626_video_wake_events(vd);
		mutex_unlock(vd->lock);
		put_device(&vd->dev);
	}
}

int fh8626_subdev_nodes_register(struct fh8626_pipeline *pipe)
{
	struct v4l2_subdev *sd;
	int ret;

	list_for_each_entry(sd, &pipe->v4l2_dev.subdevs, list) {
		struct fh8626_subdev_lifetime *life = v4l2_get_subdev_hostdata(sd);
		struct fh8626_subdev_node *node;
		struct video_device *vd;

		if (!(sd->flags & V4L2_SUBDEV_FL_HAS_DEVNODE) || sd->devnode)
			continue;
		if (!life) {
			ret = -EINVAL;
			goto err;
		}
		node = kzalloc(sizeof(*node), GFP_KERNEL);
		if (!node) {
			ret = -ENOMEM;
			goto err;
		}
		node->supplier = sd->owner == THIS_MODULE ? NULL : sd->owner;
		if (!try_module_get(node->supplier)) {
			kfree(node);
			ret = -ENODEV;
			goto err;
		}
		node->lifetime = *life;
		kref_get(life->ref);
		kref_init(&node->ref);
		mutex_init(&node->lock);
		vd = &node->vdev;
		vd->lock = &node->lock;
		vd->v4l2_dev = &pipe->v4l2_dev;
		vd->fops = &fh8626_subdev_fops;
		vd->release = fh8626_subdev_node_release;
		vd->ctrl_handler = sd->ctrl_handler;
		strlcpy(vd->name, sd->name, sizeof(vd->name));
		video_set_drvdata(vd, sd);
		/* Keep storage while registration may invoke its release callback. */
		kref_get(&node->ref);
		ret = video_register_device(vd, VFL_TYPE_SUBDEV, -1);
		if (ret) {
			/* Before device_register(), the caller owns node cleanup. */
			if (vd->minor == -1)
				fh8626_subdev_node_release(vd);
			kref_put(&node->ref, fh8626_subdev_node_free);
			goto err;
		}
		kref_put(&node->ref, fh8626_subdev_node_free);
		sd->devnode = vd;
#ifdef CONFIG_MEDIA_CONTROLLER
		sd->entity.info.dev.major = VIDEO_MAJOR;
		sd->entity.info.dev.minor = vd->minor;
		if (!vd->intf_devnode ||
		    !media_create_intf_link(&sd->entity, &vd->intf_devnode->intf,
				MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE)) {
			ret = -ENOMEM;
			goto err;
		}
#endif
	}
	return 0;
err:
	fh8626_subdev_nodes_unregister(pipe);
	return ret;
}

