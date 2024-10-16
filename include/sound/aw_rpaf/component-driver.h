/*
 * sound\aw_rpaf\component-driver.h -- Remote Process Audio Framework Layer
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _SUNXI_RPAF_COMPONENT_DRIVER_H
#define _SUNXI_RPAF_COMPONENT_DRIVER_H

#include <uapi/sound/asound.h>

#define SUNXI_RPAF_DEVICE_NAME_LEN 32

struct snd_soc_rpaf_misc_priv {
	char name[SUNXI_RPAF_DEVICE_NAME_LEN];
	struct device *dev;
	uint32_t dsp_id;
	/* for status info */
	struct attribute_group attr_group;
};

extern int32_t  snd_soc_rpaf_misc_register_device(struct device *dev, int32_t  dsp_id);
extern int32_t  snd_soc_rpaf_misc_deregister_device(struct device *dev, int32_t  dsp_id);

#endif
