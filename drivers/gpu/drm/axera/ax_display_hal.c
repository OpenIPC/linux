/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>

#include <drm/drm_print.h>

#include "ax_drm_crtc.h"

struct ax_display_mgr display_mgr;

int ax_display_register(int index, struct ax_display_funcs *display_funs,
			void *data)
{
	int ret = 0;
	struct ax_display_dev *display_dev;

	if (!display_funs) {
		DRM_ERROR("display_funs is NULL\n");
		return -EINVAL;
	}

	display_dev = &display_mgr.display_dev[index];
	if (display_dev->status != AX_DISP_DEV_STATUS_UNREGISTERED) {
		DRM_ERROR("display has registered\n");
		return -EPERM;
	}

	display_dev->status = AX_DISP_DEV_STATUS_REGISTERED;
	display_dev->display_funs = display_funs;
	display_dev->data = data;

	DRM_INFO("register display%d done\n", index);

	return ret;
}

EXPORT_SYMBOL(ax_display_register);

int ax_display_unregister(int index)
{
	int ret = 0;
	struct ax_display_dev *display_dev;

	display_dev = &display_mgr.display_dev[index];
	if (display_dev->status == AX_DISP_DEV_STATUS_UNREGISTERED) {
		DRM_ERROR("display%d is unregistered\n", index);
		return 0;
	}

	if (display_dev->status == AX_DISP_DEV_STATUS_USED) {
		DRM_ERROR("display%d in use\n", index);
		return -EPERM;
	}

	display_dev->status = AX_DISP_DEV_STATUS_UNREGISTERED;
	display_dev->display_funs = NULL;
	display_dev->data = NULL;

	DRM_INFO("unregister display%d done\n", index);

	return ret;
}

EXPORT_SYMBOL(ax_display_unregister);

void ax_display_dpu_open(void)
{
	struct ax_display_dev *display_dev = &display_mgr.display_dev[0];
	struct ax_display_funcs *dp_funs = display_dev->display_funs;
	if (dp_funs->dispc_enable)
		dp_funs->dispc_enable(display_dev->data, AX_DISP_CTRL_TYPE_LOW_LATENCY);
}
EXPORT_SYMBOL(ax_display_dpu_open);

void ax_display_dpu_close(void)
{
	struct ax_display_dev *display_dev = &display_mgr.display_dev[0];
	struct ax_display_funcs *dp_funs = display_dev->display_funs;
	if (dp_funs->dispc_disable)
		dp_funs->dispc_disable(display_dev->data, AX_DISP_CTRL_TYPE_LOW_LATENCY);
}
EXPORT_SYMBOL(ax_display_dpu_close);
