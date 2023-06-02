/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-03-22     wangyl       first version
 *
 */

#ifndef __TEE_CORE_KERN_API_H__
#define __TEE_CORE_KERN_API_H__


static int tee_ioctl_version_kern(struct tee_context *ctx,
			     struct tee_ioctl_version_data *uvers)
{
	struct tee_ioctl_version_data vers;

	ctx->teedev->desc->ops->get_version(ctx->teedev, &vers);

	if (ctx->teedev->desc->flags & TEE_DESC_PRIVILEGED)
		vers.gen_caps |= TEE_GEN_CAP_PRIVILEGED;

	memcpy(uvers, &vers, sizeof(vers));

	return 0;
}


static int tee_ioctl_shm_alloc_kern(struct tee_context *ctx,
			       struct tee_ioctl_shm_alloc_data *udata)
{
	struct tee_ioctl_shm_alloc_data data;
	struct tee_shm *shm;

	memcpy(&data, udata, sizeof(data));

	/* Currently no input flags are supported */
	if (data.flags)
		return -EINVAL;

	shm = tee_shm_alloc(ctx, data.size, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	data.id = shm->id;
	data.flags = shm->flags;
	data.size = shm->size;

	memcpy(udata, &data, sizeof(data));

	return (int)shm->kaddr;
}

static int
tee_ioctl_shm_register_kern(struct tee_context *ctx,
		       struct tee_ioctl_shm_register_data *udata)
{
	long ret;
	struct tee_ioctl_shm_register_data data;
	struct tee_shm *shm;

	memcpy(&data, udata, sizeof(data));

	/* Currently no input flags are supported */
	if (data.flags)
		return -EINVAL;

	shm = tee_shm_register(ctx, data.addr, data.length,
			       TEE_SHM_DMA_BUF | TEE_SHM_USER_MAPPED);
	if (IS_ERR(shm))
		return PTR_ERR(shm);

	data.id = shm->id;
	data.flags = shm->flags;
	data.length = shm->size;

	memcpy(udata, &data, sizeof(data));

	ret = tee_shm_get_fd(shm);
	/*
	 * When user space closes the file descriptor the shared memory
	 * should be freed or if tee_shm_get_fd() failed then it will
	 * be freed immediately.
	 */
	tee_shm_put(shm);
	return ret;
}

static int tee_ioctl_shm_register_fd_kern(struct tee_context *ctx,
			struct tee_ioctl_shm_register_fd_data *udata)
{
	struct tee_ioctl_shm_register_fd_data data;
	struct tee_shm *shm;
	long ret;

	memcpy(&data, udata, sizeof(data));

	/* Currently no input flags are supported */
	if (data.flags)
		return -EINVAL;

	shm = tee_shm_register_fd(ctx, data.fd);
	if (IS_ERR_OR_NULL(shm))
		return -EINVAL;

	data.id = shm->id;
	data.flags = shm->flags;
	data.size = shm->size;

	memcpy(udata, &data, sizeof(data));

	ret = tee_shm_get_fd(shm);

	/*
	 * When user space closes the file descriptor the shared memory
	 * should be freed or if tee_shm_get_fd() failed then it will
	 * be freed immediately.
	 */
	tee_shm_put(shm);
	return ret;
}

static int params_from_user_kern(struct tee_context *ctx, struct tee_param *params,
			    size_t num_params,
			    struct tee_ioctl_param __user *uparams)
{
	size_t n;

	for (n = 0; n < num_params; n++) {
		struct tee_shm *shm;
		struct tee_ioctl_param ip;

		memcpy(&ip, uparams + n, sizeof(ip));

		/* All unused attribute bits has to be zero */
		if (ip.attr & ~TEE_IOCTL_PARAM_ATTR_MASK)
			return -EINVAL;

		params[n].attr = ip.attr;
		switch (ip.attr & TEE_IOCTL_PARAM_ATTR_TYPE_MASK) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_NONE:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			params[n].u.value.a = ip.a;
			params[n].u.value.b = ip.b;
			params[n].u.value.c = ip.c;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			/*
			 * If we fail to get a pointer to a shared memory
			 * object (and increase the ref count) from an
			 * identifier we return an error. All pointers that
			 * has been added in params have an increased ref
			 * count. It's the callers responibility to do
			 * tee_shm_put() on all resolved pointers.
			 */
			shm = tee_shm_get_from_id(ctx, ip.c);
			if (IS_ERR(shm))
				return PTR_ERR(shm);

			/*
			 * Ensure offset + size does not overflow offset
			 * and does not overflow the size of the referred
			 * shared memory object.
			 */
			if ((ip.a + ip.b) < ip.a ||
			    (ip.a + ip.b) > shm->size) {
				tee_shm_put(shm);
				tee_shm_free(shm);
				return -EINVAL;
			}

			params[n].u.memref.shm_offs = ip.a;
			params[n].u.memref.size = ip.b;
			params[n].u.memref.shm = shm;
			break;
		default:
			/* Unknown attribute */
			return -EINVAL;
		}
	}
	return 0;
}

static int params_to_user_kern(struct tee_ioctl_param __user *uparams,
			  size_t num_params, struct tee_param *params)
{
	size_t n;

	for (n = 0; n < num_params; n++) {
		struct tee_ioctl_param __user *up = uparams + n;
		struct tee_param *p = params + n;

		switch (p->attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			up->a = p->u.value.a;
			up->b = p->u.value.b;
			up->c = p->u.value.c;
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			up->b = (u64)p->u.memref.size;
		default:
			break;
		}
	}
	return 0;
}

static int tee_ioctl_open_session_kern(struct tee_context *ctx,
				  struct tee_ioctl_buf_data *ubuf)
{
	int rc;
	size_t n;
	struct tee_ioctl_buf_data buf;
	struct tee_ioctl_open_session_arg __user *uarg;
	struct tee_ioctl_open_session_arg arg;
	struct tee_ioctl_param __user *uparams = NULL;
	struct tee_param *params = NULL;
	bool have_session = false;

	if (!ctx->teedev->desc->ops->open_session)
		return -EINVAL;

	memcpy(&buf, ubuf, sizeof(buf));

	if (buf.buf_len > TEE_MAX_ARG_SIZE ||
	    buf.buf_len < sizeof(struct tee_ioctl_open_session_arg))
		return -EINVAL;

	uarg = u64_to_user_ptr(buf.buf_ptr);
	memcpy(&arg, uarg, sizeof(arg));

	if (sizeof(arg) + TEE_IOCTL_PARAM_SIZE(arg.num_params) != buf.buf_len)
		return -EINVAL;

	if (arg.num_params) {
		params = kcalloc(arg.num_params, sizeof(struct tee_param),
				 GFP_KERNEL);
		if (!params)
			return -ENOMEM;
		uparams = uarg->params;
		rc = params_from_user_kern(ctx, params, arg.num_params, uparams);
		if (rc)
			goto out;
	}

	rc = ctx->teedev->desc->ops->open_session(ctx, &arg, params);
	if (rc)
		goto out;
	have_session = true;

	uarg->session = arg.session;
	uarg->ret = arg.ret;
	uarg->ret_origin = arg.ret_origin;

	rc = params_to_user_kern(uparams, arg.num_params, params);
out:
	/*
	 * If we've succeeded to open the session but failed to communicate
	 * it back to user space, close the session again to avoid leakage.
	 */
	if (rc && have_session && ctx->teedev->desc->ops->close_session)
		ctx->teedev->desc->ops->close_session(ctx, arg.session);

	if (params) {
		/* Decrease ref count for all valid shared memory pointers */
		for (n = 0; n < arg.num_params; n++)
			if (tee_param_is_memref(params + n) &&
			    params[n].u.memref.shm) {
				tee_shm_put(params[n].u.memref.shm);
				tee_shm_free(params[n].u.memref.shm);
			}
		kfree(params);
	}

	return rc;
}

static int tee_ioctl_invoke_kern(struct tee_context *ctx,
			    struct tee_ioctl_buf_data *ubuf)
{
	int rc;
	size_t n;
	struct tee_ioctl_buf_data buf;
	struct tee_ioctl_invoke_arg __user *uarg;
	struct tee_ioctl_invoke_arg arg;
	struct tee_ioctl_param __user *uparams = NULL;
	struct tee_param *params = NULL;

	if (!ctx->teedev->desc->ops->invoke_func)
		return -EINVAL;

	memcpy(&buf, ubuf, sizeof(buf));

	if (buf.buf_len > TEE_MAX_ARG_SIZE ||
	    buf.buf_len < sizeof(struct tee_ioctl_invoke_arg))
		return -EINVAL;

	uarg = u64_to_user_ptr(buf.buf_ptr);
	memcpy(&arg, uarg, sizeof(arg));

	if (sizeof(arg) + TEE_IOCTL_PARAM_SIZE(arg.num_params) != buf.buf_len)
		return -EINVAL;

	if (arg.num_params) {
		params = kcalloc(arg.num_params, sizeof(struct tee_param),
				 GFP_KERNEL);
		if (!params)
			return -ENOMEM;
		uparams = uarg->params;
		rc = params_from_user_kern(ctx, params, arg.num_params, uparams);
		if (rc)
			goto out;
	}

	rc = ctx->teedev->desc->ops->invoke_func(ctx, &arg, params);
	if (rc)
		goto out;

	uarg->ret = arg.ret;
	uarg->ret_origin = arg.ret_origin;

	rc = params_to_user_kern(uparams, arg.num_params, params);
out:
	if (params) {
		/* Decrease ref count for all valid shared memory pointers */
		for (n = 0; n < arg.num_params; n++)
			if (tee_param_is_memref(params + n) &&
			    params[n].u.memref.shm) {
				tee_shm_put(params[n].u.memref.shm);
				tee_shm_free(params[n].u.memref.shm);
			}
		kfree(params);
	}
	return rc;
}

static int tee_ioctl_cancel_kern(struct tee_context *ctx,
			    struct tee_ioctl_cancel_arg *uarg)
{
	struct tee_ioctl_cancel_arg arg;

	if (!ctx->teedev->desc->ops->cancel_req)
		return -EINVAL;

	memcpy(&arg, uarg, sizeof(arg));

	return ctx->teedev->desc->ops->cancel_req(ctx, arg.cancel_id,
						  arg.session);
}

static int
tee_ioctl_close_session_kern(struct tee_context *ctx,
			struct tee_ioctl_close_session_arg *uarg)
{
	struct tee_ioctl_close_session_arg arg;

	if (!ctx->teedev->desc->ops->close_session)
		return -EINVAL;

	memcpy(&arg, uarg, sizeof(arg));

	return ctx->teedev->desc->ops->close_session(ctx, arg.session);
}

long tee_ioctl_kern(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct tee_context *ctx = filp->private_data;
	void __user *uarg = (void __user *)arg;

	pr_debug("%x\n", cmd);
	switch (cmd) {
	case TEE_IOC_VERSION:
		return tee_ioctl_version_kern(ctx, uarg);
	case TEE_IOC_SHM_ALLOC:
		return tee_ioctl_shm_alloc_kern(ctx, uarg);
	case TEE_IOC_SHM_REGISTER:
		return tee_ioctl_shm_register_kern(ctx, uarg);
	case TEE_IOC_SHM_REGISTER_FD:
		return tee_ioctl_shm_register_fd_kern(ctx, uarg);
	case TEE_IOC_OPEN_SESSION:
		return tee_ioctl_open_session_kern(ctx, uarg);
	case TEE_IOC_INVOKE:
		return tee_ioctl_invoke_kern(ctx, uarg);
	case TEE_IOC_CANCEL:
		return tee_ioctl_cancel_kern(ctx, uarg);
	case TEE_IOC_CLOSE_SESSION:
		return tee_ioctl_close_session_kern(ctx, uarg);
	default:
		return -EINVAL;
	}
}
EXPORT_SYMBOL_GPL(tee_ioctl_kern);

#endif
