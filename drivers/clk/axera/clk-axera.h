/*
 * Axera clock driver
 *
 * Copyright (c) 2019-2020 Axera Technology Co., Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef	__AXERA_CLK_H_
#define	__AXERA_CLK_H_

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/spinlock.h>

struct platform_device;

struct axera_clock_data {
	struct clk_onecell_data	clk_data;
	void __iomem		*base;
};

struct axera_fixed_rate_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		fixed_rate;
};

struct axera_fixed_factor_clock {
	unsigned int		id;
	char			*name;
	const char		*parent_name;
	unsigned long		mult;
	unsigned long		div;
	unsigned long		flags;
};

struct axera_mux_clock {
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

struct axera_phase_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_names;
	unsigned long		flags;
	unsigned long		offset;
	u8			shift;
	u8			width;
	u32			*phase_degrees;
	u32			*phase_regvals;
	u8			phase_num;
};

struct axera_divider_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		offset;
	unsigned long		update_offset;
	u8			update;
	u8			shift;
	u8			width;
	u8			div_flags;
	struct clk_div_table	*table;
	const char		*alias;
};


struct axera_gate_clock {
	unsigned int		id;
	const char		*name;
	const char		*parent_name;
	unsigned long		flags;
	unsigned long		offset;
	u8			bit_idx;
	u8			gate_flags;
	u16			set_offset;
	u8			set_bit;
	u16			clr_offset;
	u8			clr_bit;
	const char		*alias;
};

struct clkgate_separated {
	struct clk_hw	hw;
	void __iomem	*enable;	/* enable register */
	u8		bit_idx;	/* bits in enable/disable register */
	u8		flags;
	void __iomem *set;      /*set enable register */
	u8 set_bit;             /*set enable bit in set enable register */
	void __iomem *clr;      /*set clear register */
	u8 clr_bit;             /*set clear bit in set clear register */
	spinlock_t *lock;
	const char *name;
};

typedef enum {
        AX620X_PLL_CPLL,
        AX620X_PLL_CPULL,
        AX620X_PLL_HPLL,
        AX620X_PLL_NPLL,
        AX620X_PLL_VPLL0,
        AX620X_PLL_VPLL1,
        AX620X_PLL_EPLL,
        AX620X_PLL_DPLL,
        AX620X_PLL_NR
} AX_PLL_ID_E;

struct axera_pll_cfg_reg {
        AX_PLL_ID_E id;
        const char      *name;
        const char      *parent_name;
        ulong re_open_reg;
        ulong on_reg;
        ulong cfg0_reg;
        ulong cfg1_reg;
        ulong lock_sts_reg;
        ulong rdy_sts_reg;
        u8 re_open_bit;
        ulong ref_rate;
        ulong min_rate;
 	ulong max_rate;
};


struct axera_pll_cfg {
    int id;
    void __iomem *re_open_reg;
    void __iomem *on_reg;
    void __iomem *cfg0_reg;
    void __iomem *cfg1_reg;
    void __iomem *lock_sts_reg;
    void __iomem *rdy_sts_reg;
    u8 re_open_bit;
    struct clk_hw   hw;
    ulong ref_rate;
    ulong min_rate;
    ulong max_rate;
};

struct clk *axera_register_clkgate(struct device *, const char *,
				const char *, unsigned long,
				void __iomem *, u8, u8,
				void __iomem *, u8,
				void __iomem *, u8,
				spinlock_t *);
struct clk *axera_register_clkdiv(struct device *dev, const char *name,
	const char *parent_name, unsigned long flags,
	void __iomem *reg, void  __iomem *reg_update,
	u8 shift, u8 width, u32 mask_bit, spinlock_t *lock);

struct axera_clock_data *axera_clk_alloc(struct platform_device *, int);
struct axera_clock_data *axera_clk_init(struct device_node *, int);
int axera_clk_register_fixed_rate(const struct axera_fixed_rate_clock *,
				int, struct axera_clock_data *);
int axera_clk_register_fixed_factor(const struct axera_fixed_factor_clock *,
				int, struct axera_clock_data *);
int axera_clk_register_mux(const struct axera_mux_clock *, int,
				struct axera_clock_data *);
int axera_clk_register_divider(const struct axera_divider_clock *,
				int, struct axera_clock_data *);
void axera_clk_register_gate(const struct axera_gate_clock *,
				int, struct axera_clock_data *);

void axera_clk_register_pll(struct axera_pll_cfg_reg *clks, int nums,
				struct axera_clock_data *data);

#define axera_clk_unregister(type) \
static inline \
void axera_clk_unregister_##type(const struct axera_##type##_clock *clks, \
				int nums, struct axera_clock_data *data) \
{ \
	struct clk **clocks = data->clk_data.clks; \
	int i; \
	for (i = 0; i < nums; i++) { \
		int id = clks[i].id; \
		if (clocks[id])  \
			clk_unregister_##type(clocks[id]); \
	} \
}

axera_clk_unregister(fixed_rate)
axera_clk_unregister(fixed_factor)
axera_clk_unregister(mux)
axera_clk_unregister(divider)
axera_clk_unregister(gate)

#endif	/* __axera_CLK_H */

