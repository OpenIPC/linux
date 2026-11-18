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
#include <linux/ax_vfb.h>

int ax_display_register(int index, struct ax_display_funcs *display_funs,
			void *data)
{
	printk("unsupport register display%d\n", index);

	return 0;
}
EXPORT_SYMBOL(ax_display_register);

int ax_display_unregister(int index)
{
	printk("unsupport unregister display%d\n", index);

	return 0;
}
EXPORT_SYMBOL(ax_display_unregister);

int ax_vfb_register(struct ax_fb_device *vfbdev)
{
	printk("unsupport %s\n", __func__);

	return 0;
}
EXPORT_SYMBOL(ax_vfb_register);

int ax_vfb_unregister(struct ax_fb_device *vfbdev)
{
	printk("unsupport %s\n", __func__);
	return 0;
}
EXPORT_SYMBOL(ax_vfb_unregister);
