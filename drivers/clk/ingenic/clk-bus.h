#ifndef __INGENIC_CLK_BUS_DIVIDER_H
#define __INGENIC_CLK_BUS_DIVIDER_H

#define BUS_DIV_SELF	BIT(6) + 1
#define BUS_DIV_ONE	    BIT(6) + 2
#define BUS_DIV_TWO	    BIT(6) + 3
#define BUS_DIV_THREE	BIT(6) + 4
#define BUS_DIV_FORE	BIT(6) + 5


struct clk *clk_register_bus_divider_table(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift1, u8 width1, u8 shift2, u8 width2,
		void __iomem *busy_reg, u8 busy_shift, int ce_shift,
		u8 clk_divider_flags, u8 divider_flags,
		const struct clk_div_table *table,
		spinlock_t *lock);

struct clk *clk_register_bus_divider(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift1, u8 width1, u8 shift2, u8 width2,
		void __iomem *busy_reg, u8 busy_shift, int ce_shift,
		u8 clk_divider_flags, u8 divider_flags, spinlock_t *lock);
#endif
