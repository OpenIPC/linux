// SPDX-License-Identifier: GPL-2.0
/*
 * FH8626V100 pad settings applied before platform devices are created.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#define pr_fmt(fmt) "fh8626-pads: " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#include "pads.h"

struct fh8626_startup_pad {
	const char *name;
	u32 reg;
	u32 mask;
	u32 value;
};

static const struct fh8626_startup_pad fh8626_startup_pads[] __initconst = {
	/* GPIO0 (pad 35); the drive field is left unchanged. */
	{ "GPIO0", 0x10c, 0x0f111100, 0x01101100 },
};

int __init fh8626_init_pads(void)
{
	int i, ret;

	if (!of_machine_is_compatible("fullhan,fh8626v100"))
		return 0;

	/*
	 * Set the USB power-switch pull after PMU initialization and before
	 * the GPIO regulator probes; the regulator owns direction and level.
	 */
	for (i = 0; i < ARRAY_SIZE(fh8626_startup_pads); i++) {
		ret = fh8626_pmu_update_bits(fh8626_startup_pads[i].reg,
					     fh8626_startup_pads[i].mask,
					     fh8626_startup_pads[i].value);
		if (ret) {
			pr_err("failed to configure %s pad: %d\n",
			       fh8626_startup_pads[i].name, ret);
			return ret;
		}
	}

	return 0;
}
