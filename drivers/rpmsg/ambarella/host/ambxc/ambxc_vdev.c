#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/rpmsg.h>

#include <linux/platform_device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-vmalloc.h>

#include "ambxc.h"
#include "ambxc_ctrls.h"

#define AMBXC_MODULE_NAME "ambxc"

struct ambxc_dev *g_dev;

extern struct rpmsg_driver rpmsg_ambxc_ctl_driver;
extern struct rpmsg_driver rpmsg_xnfs_driver;

extern struct ambxc_fmt formats[];
extern struct v4l2_frmsize_discrete framesize[];

extern void output_buf_queued(void);
extern ssize_t xnfs_read(char __user *buf, size_t len);
extern ssize_t xnfs_write(char __user *buf, size_t len);

static void ambxc_dev_release(struct device *dev)
{
}

static struct platform_device ambxc_pdev = {
	.name		= AMBXC_NAME,
	.dev.release	= ambxc_dev_release,
};

struct ambxc_q_data* get_q_data(struct ambxc_ctx *ctx,
				  enum v4l2_buf_type type)
{
	switch (type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return &ctx->q_data[V4L2_M2M_SRC];
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		return &ctx->q_data[V4L2_M2M_DST];
	default:
		BUG();
	}
	return NULL;
}

/*
 * mem2mem callbacks
 */

static int job_ready(void *priv)
{
#if 0
	struct ambxc_ctx *ctx = priv;

	if (v4l2_m2m_num_src_bufs_ready(ctx->m2m_ctx)
	    || v4l2_m2m_num_dst_bufs_ready(ctx->m2m_ctx)) {
		dprintk(ctx->dev, "Not enough buffers available\n");
		return 0;
	}
#endif

	return 1;
}

static void job_abort(void *priv)
{
	// struct ambxc_ctx *ctx = priv;
}

static void ambxc_lock(void *priv)
{
	struct ambxc_ctx *ctx = priv;
	struct ambxc_dev *dev = ctx->dev;

	mutex_lock(&dev->dev_mutex);
}

static void ambxc_unlock(void *priv)
{
	struct ambxc_ctx *ctx = priv;
	struct ambxc_dev *dev = ctx->dev;

	mutex_unlock(&dev->dev_mutex);
}


/* device_run() - prepares and starts the device
 *
 * This simulates all the immediate preparations required before starting
 * a device. This will be called by the framework when it decides to schedule
 * a particular instance.
 */
/***************************************************************************/

static int thread_capture(void *data)
{
	struct ambxc_dev *dev = data;
	struct ambxc_ctx *ctx = dev->ctx;
	struct vb2_buffer *vb;
	char *p;
	ssize_t n;

	set_freezable();

	for (;;) {

		if (kthread_should_stop()) {
			try_to_freeze();
			break;
		}

		wait_event_interruptible(dev->queue_capture,
					 v4l2_m2m_next_dst_buf(ctx->m2m_ctx));

		vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		p = vb2_plane_vaddr(vb, 0);
		n = xnfs_read(p, BUF_SIZE);

		if (n > 0) {

			vb->v4l2_buf.field = V4L2_FIELD_NONE;
			do_gettimeofday(&vb->v4l2_buf.timestamp);
			vb2_set_plane_payload(vb, 0, n);
			vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
		}
		if (n == 0) {
			const struct v4l2_event ev = {
				.type = V4L2_EVENT_EOS
			};
			dprintk(ctx->dev, "got EOS from itron, and queue it to userspace\n");
			v4l2_event_queue_fh(& ctx->fh, & ev);
		}
	}

	return 0;
}

static int thread_feed(void *data)
{
	struct ambxc_dev *dev = data;
	struct ambxc_ctx *ctx = dev->ctx;
	struct vb2_buffer *vb;
	char *p;
	ssize_t n;

	set_freezable();

	for (;;) {

		if (kthread_should_stop()) {
			try_to_freeze();
			break;
		}

		wait_event_interruptible(dev->queue_feed,
					 v4l2_m2m_next_src_buf(ctx->m2m_ctx));

		vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		p = vb2_plane_vaddr(vb, 0);
		n = vb2_get_plane_payload(vb, 0);
		xnfs_write(p, n);

		v4l2_m2m_buf_done(vb, VB2_BUF_STATE_DONE);
	}

	return 0;
}

static void device_run(void *priv)
{
	// struct ambxc_ctx *ctx = priv;
}

/*
 * Queue operations
 */

static int ambxc_queue_setup(struct vb2_queue *vq,
				const struct v4l2_format *fmt,
				unsigned int *nbuffers, unsigned int *nplanes,
				unsigned int sizes[], void *alloc_ctxs[])
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(vq);
	struct ambxc_q_data *q_data;
	unsigned int size, count = *nbuffers;

	q_data = get_q_data(ctx, vq->type);

	size = BUF_SIZE;

	while (size * count > MEM2MEM_VID_MEM_LIMIT)
		(count)--;

	*nplanes = 1;
	*nbuffers = count;
	sizes[0] = size;

	dprintk(ctx->dev, "get %d buffer(s) of size %d each.\n", count, size);

	return 0;
}

static int ambxc_buf_prepare(struct vb2_buffer *vb)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct ambxc_q_data *q_data;

	q_data = get_q_data(ctx, vb->vb2_queue->type);

	if (vb2_plane_size(vb, 0) < q_data->sizeimage) {
		dprintk(ctx->dev, "%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, 0), (long)q_data->sizeimage);
		return -EINVAL;
	}

	return 0;
}

static void ambxc_buf_queue(struct vb2_buffer *vb)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	v4l2_m2m_buf_queue(ctx->m2m_ctx, vb);

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type))
		wake_up_interruptible(&ctx->dev->queue_feed);
	else
		wake_up_interruptible(&ctx->dev->queue_capture);
}

static void ambxc_wait_prepare(struct vb2_queue *q)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(q);
	ambxc_unlock(ctx);
}

static void ambxc_wait_finish(struct vb2_queue *q)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(q);
	ambxc_lock(ctx);
}

extern void do_xc_start(struct ambxc_ctx *ctx);
static int start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(q);

	dprintk(ctx->dev, "start streaming type:%d\n", q->type);

	if ((q->type == V4L2_BUF_TYPE_VIDEO_CAPTURE &&
	     ctx->m2m_ctx->out_q_ctx.q.streaming) ||
	    (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	     ctx->m2m_ctx->cap_q_ctx.q.streaming)) {
		do_xc_start(ctx);
	}

	return 0;
}

static int stop_streaming(struct vb2_queue *q)
{
	struct ambxc_ctx *ctx = vb2_get_drv_priv(q);

	dprintk(ctx->dev, "stop streaming type:%d\n", q->type);

	return 0;
}

static struct vb2_ops ambxc_qops = {
	.queue_setup		= ambxc_queue_setup,
	.buf_prepare		= ambxc_buf_prepare,
	.buf_queue		= ambxc_buf_queue,
	.wait_prepare		= ambxc_wait_prepare,
	.wait_finish		= ambxc_wait_finish,
	.start_streaming	= start_streaming,
	.stop_streaming		= stop_streaming,
};

static int queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	struct ambxc_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->ops = &ambxc_qops;
	src_vq->mem_ops = &vb2_vmalloc_memops;
	src_vq->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->ops = &ambxc_qops;
	dst_vq->mem_ops = &vb2_vmalloc_memops;
	dst_vq->timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

/*
 * File operations
 */
static int ambxc_open(struct file *file)
{
	struct ambxc_dev *dev = video_drvdata(file);
	struct ambxc_ctx *ctx = NULL;
	int rc = 0;

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;
#if 0
	if ((atomic_read(&dev->ref_cnt) == 2)) {
		rc = -EBUSY;
		goto open_unlock;
	}
#endif

	if ((atomic_read(&dev->ref_cnt) > 0)) {
		rc = 0;
		atomic_inc(&dev->ref_cnt);
		ctx = dev->ctx;
		file->private_data = &ctx->fh;
		goto open_unlock;
	}

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		rc = -ENOMEM;
		goto open_unlock;
	}

	v4l2_fh_init(&ctx->fh, video_devdata(file));
	file->private_data = &ctx->fh;
	ctx->dev = dev;

	if (ambxc_init_ctrls(ctx))
		goto open_unlock;

	ctx->fh.ctrl_handler = &ctx->hdl;

	ctx->q_data[V4L2_M2M_SRC].fmt = &formats[0];
	ctx->q_data[V4L2_M2M_SRC].sizeimage = BUF_SIZE;
	ctx->q_data[V4L2_M2M_SRC].width = framesize[0].width;
	ctx->q_data[V4L2_M2M_SRC].height = framesize[0].height;
	ctx->q_data[V4L2_M2M_DST] = ctx->q_data[V4L2_M2M_SRC];

	ctx->m2m_ctx = v4l2_m2m_ctx_init(dev->m2m_dev, ctx, &queue_init);

	if (IS_ERR(ctx->m2m_ctx)) {
		rc = PTR_ERR(ctx->m2m_ctx);
		ambxc_release_ctrls(ctx);
		kfree(ctx);
		goto open_unlock;
	}

	v4l2_fh_add(&ctx->fh);

	atomic_inc(&dev->ref_cnt);
	dev->ctx = ctx;
	init_waitqueue_head(&dev->queue_capture);
	init_waitqueue_head(&dev->queue_feed);
	dev->kthread_capture = kthread_run(thread_capture, dev,
					   dev->v4l2_dev.name);
	dev->kthread_feed = kthread_run(thread_feed, dev, dev->v4l2_dev.name);

	dprintk(dev, "Created instance %p, m2m_ctx: %p\n", ctx, ctx->m2m_ctx);

open_unlock:
	mutex_unlock(&dev->dev_mutex);
	return rc;
}

static int ambxc_release(struct file *file)
{
	struct ambxc_dev *dev = video_drvdata(file);
	struct ambxc_ctx *ctx = file2ctx(file);

	if ((atomic_read(&dev->ref_cnt) == 1)) {
#if 0
		dprintk(dev, "Releasing instance %p\n", ctx);

		kthread_stop(dev->kthread);
		if (IS_ERR(dev->kthread)) {
			v4l2_err(&dev->v4l2_dev, "kernel_thread() failed\n");
			return PTR_ERR(dev->kthread);
		}

		v4l2_fh_del(&ctx->fh);
		v4l2_fh_exit(&ctx->fh);
		v4l2_ctrl_handler_free(&ctx->hdl);
		mutex_lock(&dev->dev_mutex);
		v4l2_m2m_ctx_release(ctx->m2m_ctx);
		mutex_unlock(&dev->dev_mutex);
		kfree(ctx);
#endif
	}

	// atomic_dec(&dev->ref_cnt);

	return 0;
}

static unsigned int ambxc_poll(struct file *file,
				 struct poll_table_struct *wait)
{
	struct ambxc_ctx *ctx = file2ctx(file);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int ambxc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ambxc_dev *dev = video_drvdata(file);
	struct ambxc_ctx *ctx = file2ctx(file);
	int res;

	if (mutex_lock_interruptible(&dev->dev_mutex))
		return -ERESTARTSYS;

	res = v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);

	mutex_unlock(&dev->dev_mutex);

	return res;
}

static const struct v4l2_file_operations ambxc_fops = {
	.owner		= THIS_MODULE,
	.open		= ambxc_open,
	.release	= ambxc_release,
	.poll		= ambxc_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= ambxc_mmap,
};

static struct v4l2_m2m_ops m2m_ops = {
	.device_run	= device_run,
	.job_ready	= job_ready,
	.job_abort	= job_abort,
	.lock		= ambxc_lock,
	.unlock		= ambxc_unlock,
};

static struct video_device ambxc_videodev = {
	.name		= AMBXC_NAME,
	.vfl_dir	= VFL_DIR_M2M,
	.fops		= &ambxc_fops,
	.ioctl_ops	= &ambxc_ioctl_ops,
	.minor		= -1,
	.release	= video_device_release,
};

static int ambxc_probe(struct platform_device *pdev)
{
	struct ambxc_dev *dev;
	struct video_device *vfd;
	int ret;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->irqlock);

	ret = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (ret)
		return ret;

	atomic_set(&dev->inst, 0);
	atomic_set(&dev->ref_cnt, 0);
	mutex_init(&dev->dev_mutex);

	vfd = video_device_alloc();
	if (!vfd) {
		v4l2_err(&dev->v4l2_dev, "Failed to allocate video device\n");
		ret = -ENOMEM;
		goto unreg_dev;
	}

	*vfd = ambxc_videodev;
	vfd->lock = &dev->dev_mutex;

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, 0);
	if (ret) {
		v4l2_err(&dev->v4l2_dev, "Failed to register video device\n");
		goto rel_vdev;
	}

	video_set_drvdata(vfd, dev);
	snprintf(vfd->name, sizeof(vfd->name), "%s", ambxc_videodev.name);
	dev->vfd = vfd;
	// v4l2_device_set_name(&dev->v4l2_dev, vfd->name, &dev->inst);

	v4l2_info(&dev->v4l2_dev, AMBXC_MODULE_NAME
			"Device registered as /dev/%d\n", vfd->num);

	platform_set_drvdata(pdev, dev);

	dev->m2m_dev = v4l2_m2m_init(&m2m_ops);
	if (IS_ERR(dev->m2m_dev)) {
		v4l2_err(&dev->v4l2_dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(dev->m2m_dev);
		goto err_m2m;
	}

	g_dev = dev; // FIXME;
	return 0;

err_m2m:
	v4l2_m2m_release(dev->m2m_dev);
	video_unregister_device(dev->vfd);
rel_vdev:
	video_device_release(vfd);
unreg_dev:
	v4l2_device_unregister(&dev->v4l2_dev);

	return ret;
}

static int ambxc_remove(struct platform_device *pdev)
{
	struct ambxc_dev *dev = platform_get_drvdata(pdev);

	v4l2_info(&dev->v4l2_dev, "Removing " AMBXC_MODULE_NAME);
	v4l2_m2m_release(dev->m2m_dev);
	video_unregister_device(dev->vfd);
	v4l2_device_unregister(&dev->v4l2_dev);

	return 0;
}

static struct platform_driver ambxc_pdrv = {
	.probe		= ambxc_probe,
	.remove		= ambxc_remove,
	.driver		= {
		.name	= AMBXC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ambxc_init(void)
{
	int ret;

	ret = register_rpmsg_driver(&rpmsg_ambxc_ctl_driver);
	if (ret)
		return ret;
	ret = register_rpmsg_driver(&rpmsg_xnfs_driver);
	if (ret)
		return ret;

	ret = platform_device_register(&ambxc_pdev);
	if (ret)
		return ret;

	ret = platform_driver_register(&ambxc_pdrv);
	if (ret)
		platform_device_unregister(&ambxc_pdev);

	return 0;
}

static void __exit ambxc_exit(void)
{
	platform_driver_unregister(&ambxc_pdrv);
	platform_device_unregister(&ambxc_pdev);
	unregister_rpmsg_driver(&rpmsg_xnfs_driver);
	unregister_rpmsg_driver(&rpmsg_ambxc_ctl_driver);
}

module_init(ambxc_init);
module_exit(ambxc_exit);

MODULE_DESCRIPTION("Ambarella A8 Video Codec Driver");
MODULE_AUTHOR("Tzu-Jung Lee, <tjlee@ambarella.com>");
MODULE_VERSION("0.0.1");
