/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SUNXI_RPROC_BOOT_H
#define SUNXI_RPROC_BOOT_H

#include <linux/clk.h>

struct sunxi_core;

struct sunxi_remote_core_ops {
	int (*init)(struct sunxi_core *core);
	void (*deinit)(struct sunxi_core *core);
	int (*start)(struct sunxi_core *core);
	int (*is_start)(struct sunxi_core *core);
	int (*stop)(struct sunxi_core *core);
	void (*set_start_addr)(struct sunxi_core *core, u32 addr);
	void (*set_freq)(struct sunxi_core *core, u32 freq);
};

struct sunxi_core {
	const char *name;
	struct sunxi_remote_core_ops *ops;
	void *priv;
};

struct sunxi_core *
sunxi_remote_core_find(const char *name);

int sunxi_core_init(struct sunxi_core *core);
void sunxi_core_deinit(struct sunxi_core *core);
int sunxi_core_start(struct sunxi_core *core);
int sunxi_core_is_start(struct sunxi_core *core);
int sunxi_core_stop(struct sunxi_core *core);
void sunxi_core_set_start_addr(struct sunxi_core *core, u32 addr);
void sunxi_core_set_freq(struct sunxi_core *core, u32 freq);

#endif
