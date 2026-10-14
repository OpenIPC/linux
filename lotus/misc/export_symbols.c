// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Lotus. All rights reserved.
 *
 * Export symbols which are not exported by default
 *
 */

#include <linux/kernel.h>
#include <linux/memblock.h>
#ifdef CONFIG_ARM64
#include <asm/cacheflush.h>
#endif

char *lotus_get_cmd_line(void)
{
	return saved_command_line;
}
EXPORT_SYMBOL(lotus_get_cmd_line);

/* For osal to check if mmb has been mapped */
EXPORT_SYMBOL(memblock_is_map_memory);

#ifdef CONFIG_ARM64
/* For osal to flush dcache */
EXPORT_SYMBOL(__flush_dcache_area);
#endif
