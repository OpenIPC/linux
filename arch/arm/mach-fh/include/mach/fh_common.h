#ifndef __FH_COMMON_H__
#define __FH_COMMON_H__

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/irqdomain.h>

/*change the scheduling policy and/or RT priority of `current` thread. */
void fh_setscheduler(int policy, int priority);

int fh_clk_enable(struct clk *clk);
unsigned long fh_clk_get_rate(struct clk *clk);
int fh_clk_set_rate(struct clk *clk, unsigned long rate);
int fh_clk_prepare(struct clk *clk);
void fh_clk_disable(struct clk *clk);
void fh_clk_unprepare(struct clk *clk);
int fh_clk_prepare_enable(struct clk *clk);
void fh_clk_disable_unprepare(struct clk *clk);
int fh_clk_set_parent(struct clk *clk, struct clk *parent);
struct clk *fh_clk_get_parent(struct clk *clk);
bool fh_clk_has_parent(struct clk *clk, struct clk *parent);

unsigned int fh_irq_create_mapping(struct irq_domain *domain,
				irq_hw_number_t hwirq);
bool fh_clk_is_enabled(struct clk *clk);

#endif