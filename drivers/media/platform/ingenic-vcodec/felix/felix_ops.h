#ifndef __FELIX_OPS_H__
#define __FELIX_OPS_H__

extern const struct v4l2_ioctl_ops ingenic_vdec_ioctl_ops;
extern const struct v4l2_m2m_ops ingenic_vdec_m2m_ops;

int ingenic_vcodec_vdec_queue_init(void *priv, struct vb2_queue *src_vq,
	struct vb2_queue *dst_vq);

int ingenic_vcodec_init_default_params(struct ingenic_vdec_ctx *ctx);

int ingenic_vcodec_deinit_default_params(struct ingenic_vdec_ctx *ctx);

#endif
