/* SPDX-License-Identifier: GPL-2.0 */

#ifndef	__LOTUS_CLK_H
#define	__LOTUS_CLK_H

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/spinlock.h>

struct platform_device;

struct lotus_clock_data {
	struct clk_onecell_data	clk_data;
	void __iomem		*base;
};

struct lotus_fixed_rate_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		fixed_rate;
};

struct lotus_fixed_factor_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		mult;
	unsigned long		div;
	unsigned long		flags;
};

struct lotus_mux_clock {
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

struct lotus_gate_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		offset;
	u8			bit_idx;
	u8			gate_flags;
	const char		*alias;
};

struct lotus_clock_data *lotus_clk_init(struct device_node *np, int nr_clks);
int lotus_clk_register_fixed_rate(const struct lotus_fixed_rate_clock *clks,
					int nums, struct lotus_clock_data *data);
int lotus_clk_register_fixed_factor(const struct lotus_fixed_factor_clock *clks,
					int nums, struct lotus_clock_data *data);
int lotus_clk_register_mux(const struct lotus_mux_clock *clks,
				int nums, struct lotus_clock_data *data);
int lotus_clk_register_gate(const struct lotus_gate_clock *clks,
				int nums, struct lotus_clock_data *data);
#define lotus_clk_unregister(type) \
static inline \
void lotus_clk_unregister_##type(const struct lotus_##type##_clock *clks, \
				int nums, struct lotus_clock_data *data) \
{ \
	struct clk **clocks = data->clk_data.clks; \
	int i; \
	for (i = 0; i < nums; i++) { \
		int id = clks[i].id; \
		if (clocks[id])  \
			clk_unregister_##type(clocks[id]); \
	} \
}

lotus_clk_unregister(fixed_rate)
lotus_clk_unregister(fixed_factor)
lotus_clk_unregister(mux)
lotus_clk_unregister(gate)

#endif	/* __LOTUS_CLK_H */
