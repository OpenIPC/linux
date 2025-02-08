/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef	__XMEDIA_CLK_H
#define	__XMEDIA_CLK_H

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/spinlock.h>

struct platform_device;

struct xmedia_clock_data {
	struct clk_onecell_data	clk_data;
	void __iomem		*base;
};

struct xmedia_fixed_rate_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		fixed_rate;
};

struct xmedia_fixed_factor_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		mult;
	unsigned long		div;
	unsigned long		flags;
};

struct xmedia_mux_clock {
	unsigned int		id;
	const char		*name;
	const char		*const *parent_names;
	u8			num_parents;
	unsigned long		flags;
	unsigned long		offset;
	u8			shift;
	u8			width;
	u8			mux_flags;
	u32			*table;
	const char		*alias;
};

struct xmedia_gate_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		offset;
	u8			bit_idx;
	u8			gate_flags;
	const char		*alias;
};

struct xmedia_clock_data *xmedia_clk_init(struct device_node *, int);
int xmedia_clk_register_fixed_rate(const struct xmedia_fixed_rate_clock *,
				int, struct xmedia_clock_data *);
int xmedia_clk_register_fixed_factor(const struct xmedia_fixed_factor_clock *,
				int, struct xmedia_clock_data *);
int xmedia_clk_register_mux(const struct xmedia_mux_clock *, int,
				struct xmedia_clock_data *);
int xmedia_clk_register_gate(const struct xmedia_gate_clock *,
				int, struct xmedia_clock_data *);

#define xmedia_clk_unregister(type) \
static inline \
void xmedia_clk_unregister_##type(const struct xmedia_##type##_clock *clks, \
				int nums, struct xmedia_clock_data *data) \
{ \
	struct clk **clocks = data->clk_data.clks; \
	int i; \
	for (i = 0; i < nums; i++) { \
		int id = clks[i].id; \
		if (clocks[id])  \
			clk_unregister_##type(clocks[id]); \
	} \
}

xmedia_clk_unregister(fixed_rate)
xmedia_clk_unregister(fixed_factor)
xmedia_clk_unregister(mux)
xmedia_clk_unregister(gate)

#endif	/* __XMEDIA_CLK_H */
