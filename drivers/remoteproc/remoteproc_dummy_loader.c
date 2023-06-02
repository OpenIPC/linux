/*
 * Remote Processor Framework Elf loader
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 * Mark Grosen <mgrosen@ti.com>
 * Fernando Guzman Lugo <fernando.lugo@ti.com>
 * Suman Anna <s-anna@ti.com>
 * Robert Tivy <rtivy@ti.com>
 * Armando Uribe De Leon <x0095078@ti.com>
 * Sjur Brændeland <sjur.brandeland@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/remoteproc.h>
#include <linux/slab.h>
#include <linux/virtio_ids.h>

#include <plat/remoteproc.h>

#include "remoteproc_internal.h"

static struct resource_table *
rproc_dummy_find_rsc_table(struct rproc *rproc, const struct firmware *fw,
							int *tablesz)
{
	struct ambarella_rproc_pdata *pdata = rproc->priv;

	return pdata->gen_rsc_table(tablesz);
}

static int rproc_dummy_load_segments(struct rproc *rproc,
				     const struct firmware *fw)
{
	return 0;
}

static int rproc_dummy_request_firmware_nowait( struct module *module, bool uevent,
	       const char *name, struct device *device, gfp_t gfp, void *context,
	       void (*cont)(const struct firmware *fw, void *context))
{
	cont(NULL, context);

	return 0;
}

static int rproc_dummy_request_firmware(const struct firmware **firmware_p,
		const char *name, struct device *device)
{
	*firmware_p = kzalloc(sizeof(**firmware_p), GFP_KERNEL);

	return 0;
}

static void rproc_dummy_release_firmware(const struct firmware *fw)
{
	kfree(fw);
}

const struct rproc_fw_ops rproc_dummy_fw_ops = {
	.request_firmware		= rproc_dummy_request_firmware,
	.request_firmware_nowait	= rproc_dummy_request_firmware_nowait,
	.release_firmware		= rproc_dummy_release_firmware,
	.load				= rproc_dummy_load_segments,
	.find_rsc_table			= rproc_dummy_find_rsc_table,
	.sanity_check			= NULL,
	.get_boot_addr			= NULL
};
