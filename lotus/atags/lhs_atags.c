// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "Param Tag: " fmt

#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/export.h>

#include "setup.h"

/****************************************************************************/
extern struct tagtable_lhs g_tag_param;

/* Tagtable defined by lhs */
static struct tagtable_lhs *tagtable[] __initdata = {
	&g_tag_param,
	NULL,
};

/****************************************************************************/
int __init parse_lotus_tags(const struct tag *tag, void *fdt)
{
	struct tagtable_lhs **tagtbl = tagtable;

	while (*tagtbl) {
		if ((*tagtbl)->tag == tag->hdr.tag)
			return (*tagtbl)->parse(tag, fdt);
		tagtbl++;
	}

	return 0;
}
