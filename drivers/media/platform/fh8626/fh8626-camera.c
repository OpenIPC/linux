// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 camera root driver
 *
 * Owns the media device and the async notifier that binds the sensor, CSI
 * receiver and ISP/VPU subdevices and registers the capture nodes.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <media/media-device.h>
#include <media/media-devnode.h>

#include "fh8626-pipeline.h"

struct fh8626_cam_dev {
	struct platform_device		*pdev;
	struct fh8626_pipeline		pipe;
	struct v4l2_async_subdev	asd_csi;
	struct v4l2_async_subdev	asd_sensor;
	struct v4l2_async_subdev	asd_isp;
	struct v4l2_async_subdev	*async_subdevs[3];
};

static void fh8626_camera_release(struct v4l2_device *v4l2_dev)
{
	struct fh8626_pipeline *pipe =
		container_of(v4l2_dev, struct fh8626_pipeline, v4l2_dev);
	struct fh8626_cam_dev *cam =
		container_of(pipe, struct fh8626_cam_dev, pipe);
#ifdef CONFIG_MEDIA_CONTROLLER
	media_device_cleanup(&pipe->media_dev);
	mutex_destroy(&pipe->media_lock);
#endif
	kfree(cam);
}

#ifdef CONFIG_MEDIA_CONTROLLER
static void fh8626_media_release(struct media_devnode *node)
{
	struct fh8626_pipeline *pipe = dev_get_drvdata(&node->dev);

	v4l2_device_put(&pipe->v4l2_dev);
}

static long fh8626_media_ioctl(struct file *file, unsigned int cmd,
			      unsigned long arg)
{
	struct media_devnode *node = media_devnode_data(file);
	struct fh8626_pipeline *pipe = dev_get_drvdata(&node->dev);
	long ret;

	if (mutex_lock_interruptible(&pipe->media_lock))
		return -ERESTARTSYS;
	ret = media_devnode_is_registered(node) ?
		media_device_ioctl(file, cmd, arg) : -ENODEV;
	mutex_unlock(&pipe->media_lock);
	return ret;
}

static const struct media_file_operations fh8626_media_fops = {
	.owner = THIS_MODULE,
	.ioctl = fh8626_media_ioctl,
};

static void fh8626_media_unregister(struct fh8626_pipeline *pipe)
{
	mutex_lock(&pipe->media_lock);
	media_device_unregister(&pipe->media_dev);
	mutex_unlock(&pipe->media_lock);
}

static ssize_t model_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct fh8626_pipeline *pipe = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%s\n", pipe->media_dev.model);
}
static DEVICE_ATTR_RO(model);

static int fh8626_media_register(struct fh8626_pipeline *pipe)
{
	struct media_devnode *node;
	int ret;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;
	node->fops = &fh8626_media_fops;
	node->parent = pipe->media_dev.dev;
	node->release = fh8626_media_release;
	dev_set_drvdata(&node->dev, pipe);
	pipe->media_dev.devnode = node;
	pipe->media_dev.topology_version = 0;
	v4l2_device_get(&pipe->v4l2_dev);
	/* A cdev can be opened even before device registration finishes. */
	mutex_lock(&pipe->media_lock);
	ret = media_devnode_register(&pipe->media_dev, node, THIS_MODULE);
	mutex_unlock(&pipe->media_lock);
	if (ret) {
		pipe->media_dev.devnode = NULL;
		/*
		 * No-minor failure frees node before device initialization.
		 * Later failures drop its device reference and call release.
		 */
		if (ret == -ENFILE)
			v4l2_device_put(&pipe->v4l2_dev);
		return ret;
	}
	ret = device_create_file(&node->dev, &dev_attr_model);
	if (ret)
		fh8626_media_unregister(pipe);
	return ret;
}
#endif

static struct device_node *fh8626_remote_node(struct device_node *node,
					      u32 port)
{
	struct device_node *ep, *remote;

	ep = of_graph_get_endpoint_by_regs(node, port, -1);
	if (!ep)
		return NULL;
	remote = of_graph_get_remote_port_parent(ep);
	of_node_put(ep);
	return remote;
}

static int fh8626_notifier_bound(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *sd,
				 struct v4l2_async_subdev *asd)
{
	struct fh8626_pipeline *pipe =
		container_of(notifier, struct fh8626_pipeline, notifier);
	struct fh8626_cam_dev *cam =
		container_of(pipe, struct fh8626_cam_dev, pipe);

	if (asd == &cam->asd_sensor) {
		pipe->sensor_sd = sd;
	} else if (asd == &cam->asd_csi) {
		pipe->csi_sd = sd;
	} else if (asd == &cam->asd_isp) {
		pipe->isp_sd = sd;
		pipe->isp = fh8626_isp_from_subdev(sd);
		pipe->vpu = fh8626_isp_vpu(pipe->isp);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int fh8626_notifier_complete(struct v4l2_async_notifier *notifier)
{
	struct fh8626_pipeline *pipe =
		container_of(notifier, struct fh8626_pipeline, notifier);
	struct fh8626_cam_dev *cam =
		container_of(pipe, struct fh8626_cam_dev, pipe);
	unsigned int chan;
	int ret;

	if (!pipe->sensor_sd || !pipe->csi_sd || !pipe->isp_sd || !pipe->vpu)
		return -ENODEV;
	/* Interface creation in video core expects caller serialization. */
	mutex_lock(&pipe->media_lock);
	ret = fh8626_subdev_nodes_register(pipe);
	if (ret)
		goto out;
	ret = fh8626_video_register(cam->pdev, pipe);
	if (ret)
		goto out;
#ifdef CONFIG_MEDIA_CONTROLLER
	ret = media_create_pad_link(&pipe->sensor_sd->entity, 0,
		&pipe->csi_sd->entity, 0,
		MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	if (!ret)
		ret = media_create_pad_link(&pipe->csi_sd->entity, 1,
			&pipe->isp_sd->entity, 0,
			MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	for (chan = 0; !ret && chan < FH8626_VPU_CHANNELS; ++chan)
		ret = media_create_pad_link(&pipe->isp_sd->entity, 1,
			&pipe->chan_video[chan]->entity, 0,
			MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	if (ret) {
		fh8626_video_unregister(pipe);
		goto out;
	}
#endif
	fh8626_video_ready(pipe);
out:
	mutex_unlock(&pipe->media_lock);
	return ret;
}

static void fh8626_notifier_unbind(struct v4l2_async_notifier *notifier,
				 struct v4l2_subdev *sd,
				 struct v4l2_async_subdev *asd)
{
	struct fh8626_pipeline *pipe =
		container_of(notifier, struct fh8626_pipeline, notifier);

	/* Stop while the departing sensor/CSI is still available. */
	fh8626_video_unregister(pipe);
	fh8626_subdev_nodes_unregister(pipe);
#ifdef CONFIG_MEDIA_CONTROLLER
	if (pipe->sensor_sd)
		media_entity_remove_links(&pipe->sensor_sd->entity);
	if (pipe->csi_sd)
		media_entity_remove_links(&pipe->csi_sd->entity);
	if (pipe->isp_sd)
		media_entity_remove_links(&pipe->isp_sd->entity);
#endif
	if (pipe->vpu)
		fh8626_vpu_set_pipeline(pipe->vpu, NULL);
	if (pipe->sensor_sd == sd)
		pipe->sensor_sd = NULL;
	if (pipe->csi_sd == sd)
		pipe->csi_sd = NULL;
	if (pipe->isp_sd == sd) {
		pipe->isp_sd = NULL;
		pipe->isp = NULL;
		pipe->vpu = NULL;
	}
}

static const struct v4l2_async_notifier_operations fh8626_async_ops = {
	.bound = fh8626_notifier_bound,
	.complete = fh8626_notifier_complete,
	.unbind = fh8626_notifier_unbind,
};

static int fh8626_camera_probe(struct platform_device *pdev)
{
	struct fh8626_cam_dev *cam;
	struct fh8626_pipeline *pipe;
	struct device_node *csi_node = NULL;
	struct device_node *sensor_node = NULL;
	struct device_node *isp_node;
	int num_subdevs = 0;
	int ret;

	cam = kzalloc(sizeof(*cam), GFP_KERNEL);
	if (!cam)
		return -ENOMEM;

	cam->pdev = pdev;
	pipe = &cam->pipe;

	mutex_init(&pipe->stream_lock);

	strlcpy(pipe->v4l2_dev.name, "fh8626-camera",
		sizeof(pipe->v4l2_dev.name));
	pipe->v4l2_dev.release = fh8626_camera_release;
#ifdef CONFIG_MEDIA_CONTROLLER
	pipe->media_dev.dev = &pdev->dev;
	media_device_init(&pipe->media_dev);
	mutex_init(&pipe->media_lock);
	strlcpy(pipe->media_dev.model, "FH8626 camera",
		sizeof(pipe->media_dev.model));
	pipe->v4l2_dev.mdev = &pipe->media_dev;
#endif
	ret = v4l2_device_register(&pdev->dev, &pipe->v4l2_dev);
	if (ret) {
		fh8626_camera_release(&pipe->v4l2_dev);
		return ret;
	}
#ifdef CONFIG_MEDIA_CONTROLLER
	ret = fh8626_media_register(pipe);
	if (ret)
		goto err_unreg_v4l2;
#endif

	isp_node = fh8626_remote_node(pdev->dev.of_node, 0);
	if (isp_node) {
		cam->asd_isp.match_type = V4L2_ASYNC_MATCH_FWNODE;
		cam->asd_isp.match.fwnode = of_fwnode_handle(isp_node);
		cam->async_subdevs[num_subdevs++] = &cam->asd_isp;
		csi_node = fh8626_remote_node(isp_node, 0);
	}
	if (csi_node) {
		cam->asd_csi.match_type = V4L2_ASYNC_MATCH_FWNODE;
		cam->asd_csi.match.fwnode = of_fwnode_handle(csi_node);
		cam->async_subdevs[num_subdevs++] = &cam->asd_csi;
	}

	sensor_node = csi_node ? fh8626_remote_node(csi_node, 0) : NULL;
	if (sensor_node) {
		cam->asd_sensor.match_type = V4L2_ASYNC_MATCH_FWNODE;
		cam->asd_sensor.match.fwnode = of_fwnode_handle(sensor_node);
		cam->async_subdevs[num_subdevs++] = &cam->asd_sensor;
	}

	if (num_subdevs != 3) {
		ret = -ENODEV;
		goto err_put_nodes;
	}

	pipe->notifier.num_subdevs = num_subdevs;
	pipe->notifier.subdevs = cam->async_subdevs;
	pipe->notifier.ops = &fh8626_async_ops;

	ret = v4l2_async_notifier_register(&pipe->v4l2_dev, &pipe->notifier);
	if (ret) {
		dev_err(&pdev->dev,
			"failed to register async notifier: %d\n", ret);
		goto err_put_nodes;
	}

	platform_set_drvdata(pdev, cam);
	return 0;

err_put_nodes:
	of_node_put(sensor_node);
	of_node_put(csi_node);
	of_node_put(isp_node);
	if (pipe->vpu)
		fh8626_vpu_set_pipeline(pipe->vpu, NULL);
#ifdef CONFIG_MEDIA_CONTROLLER
	fh8626_media_unregister(pipe);
err_unreg_v4l2:
#endif
	v4l2_device_unregister(&pipe->v4l2_dev);
	v4l2_device_put(&pipe->v4l2_dev);
	return ret;
}

static int fh8626_camera_remove(struct platform_device *pdev)
{
	struct fh8626_cam_dev *cam = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	if (cam->pipe.notifier.num_subdevs) {
		int i;

		v4l2_async_notifier_unregister(&cam->pipe.notifier);
		for (i = 0; i < cam->pipe.notifier.num_subdevs; i++) {
			struct v4l2_async_subdev *asd =
				cam->pipe.notifier.subdevs[i];

			if (asd->match_type == V4L2_ASYNC_MATCH_FWNODE &&
			    asd->match.fwnode)
				of_node_put(to_of_node(asd->match.fwnode));
		}
	}

	if (cam->pipe.vpu)
		fh8626_vpu_set_pipeline(cam->pipe.vpu, NULL);
#ifdef CONFIG_MEDIA_CONTROLLER
	fh8626_media_unregister(&cam->pipe);
#endif
	v4l2_device_unregister(&cam->pipe.v4l2_dev);
	v4l2_device_put(&cam->pipe.v4l2_dev);
	return 0;
}

static const struct of_device_id fh8626_camera_of_match[] = {
	{ .compatible = "fullhan,fh8626v100-camera" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_camera_of_match);

static void fh8626_camera_shutdown(struct platform_device *pdev)
{
	struct fh8626_cam_dev *cam = platform_get_drvdata(pdev);

	fh8626_video_shutdown(&cam->pipe);
}

static int fh8626_camera_suspend(struct device *dev)
{
	/* Clock and reset retention across sleep is not supported. */
	return -EBUSY;
}

static const struct dev_pm_ops fh8626_camera_pm_ops = {
	.suspend = fh8626_camera_suspend,
	.freeze = fh8626_camera_suspend,
	.poweroff = fh8626_camera_suspend,
};

struct platform_driver fh8626_camera_driver = {
	.probe = fh8626_camera_probe,
	.remove = fh8626_camera_remove,
	.shutdown = fh8626_camera_shutdown,
	.driver = {
		.name = "fh8626-camera",
		.of_match_table = fh8626_camera_of_match,
		.pm = &fh8626_camera_pm_ops,
	},
};

static int __init fh8626_capture_init(void)
{
	int ret;

	ret = platform_driver_register(&fh8626_csi_driver);
	if (ret)
		return ret;

	ret = platform_driver_register(&fh8626_isp_driver);
	if (ret)
		goto err_isp;

	ret = platform_driver_register(&fh8626_camera_driver);
	if (ret)
		goto err_cam;

	return 0;

err_cam:
	platform_driver_unregister(&fh8626_isp_driver);
err_isp:
	platform_driver_unregister(&fh8626_csi_driver);
	return ret;
}

static void __exit fh8626_capture_exit(void)
{
	platform_driver_unregister(&fh8626_camera_driver);
	platform_driver_unregister(&fh8626_isp_driver);
	platform_driver_unregister(&fh8626_csi_driver);
}

module_init(fh8626_capture_init);
module_exit(fh8626_capture_exit);

MODULE_DESCRIPTION("Fullhan FH8626V100 Camera Platform Driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
