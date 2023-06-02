#ifndef __AMBXC_CTRLS_H__
#define __AMBXC_CTRLS_H__

extern int ambxc_init_ctrls(struct ambxc_ctx *ctx);
extern void ambxc_release_ctrls(struct ambxc_ctx *ctx);
extern const struct v4l2_ioctl_ops ambxc_ioctl_ops;

#endif /* __AMBXC_CTRLS_H__ */
