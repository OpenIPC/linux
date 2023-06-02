#ifndef __INGENIC_CLK_CGU_DIVIDER_H
#define __INGENIC_CLK_CGU_DIVIDER_H


struct clk *clk_register_cgu_divider_table(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width, u8 busy_shift,
		int en_shift, u8 stop_shift, u8 clk_divider_flags,
		const struct clk_div_table *table,
		spinlock_t *lock);

struct clk *clk_register_cgu_divider(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width, u8 busy_shift,
		int en_shift, u8 stop_shift,
		u8 clk_divider_flags, spinlock_t *lock);

#endif /* __SAMSUNG_CLK_PLL_H */
