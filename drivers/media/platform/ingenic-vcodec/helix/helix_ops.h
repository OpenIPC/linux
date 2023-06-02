#ifndef __INGENIC_HELIX_OPS_H__
#define __INGENIC_HELIX_OPS_H__


extern const struct v4l2_ioctl_ops ingenic_venc_ioctl_ops;
extern const struct v4l2_m2m_ops ingenic_venc_m2m_ops;



int ingenic_vcodec_enc_init_default_params(struct ingenic_venc_ctx *ctx);
int ingenic_vcodec_enc_deinit_default_params(struct ingenic_venc_ctx *ctx);

int ingenic_vcodec_enc_ctrls_setup(struct ingenic_venc_ctx *ctx);
int ingenic_vcodec_enc_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq);


#endif
