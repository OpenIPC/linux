/*
 * Sunxi Remote processor framework
 *
 * Copyright (C) 2022 Allwinner, Inc.
 * Base on remoteproc_internal.h
 *
 * lijiajian <lijianjian@allwinner.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SUNXI_REMOTEPROC_INTERNAL_H
#define SUNXI_REMOTEPROC_INTERNAL_H

#include <linux/remoteproc.h>
#include "remoteproc_internal.h"
#include "sunxi_rproc_boot.h"

#define FW_SAVE_IN_MEM

extern int sunxi_request_firmware(const struct firmware **fw,
				const char *name, struct device *dev);

extern int sunxi_request_firmware_nowait(const char *name,
				struct device *device, gfp_t gfp, void *context,
				void (*cont)(const struct firmware *fw, void *context));

extern int sunxi_register_memory_fw(const char *name, phys_addr_t addr,
				int len);

#endif /* SUNXI_REMOTEPROC_INTERNAL_H */
