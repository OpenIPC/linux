#ifndef __INGENIC_POWER_GATE_H
#define __INGENIC_CLK_CGU_DIVIDER_H


#define POWER_GATE_WAIT	BIT(1)

struct clk *power_register_gate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 ctrl_bit, u8 wait_bit,
		u8 clk_gate_flags, unsigned long power_flags, spinlock_t *lock);

void power_unregister_gate(struct clk *clk);

#endif /* __SAMSUNG_CLK_PLL_H */
