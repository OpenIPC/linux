#ifndef __AXERA_DW_APB_TIMER_H_
#define __AXERA_DW_APB_TIMER_H_

#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>

#define CLK_SEL_FREQ	24000000
#define HZ_PER_USEC	24

#define AXERA_APB_TIMER_COUNT		2
#define PER_TIMER_CHANNEL_COUNT 	4

#define APBTMR_N_LOAD_COUNT(N)		(0x00 + (N) * 0x14)
#define APBTMR_N_CONTROL(N)		(0x08 + (N) * 0x14)
#define APBTMR_N_EOI(N)			(0x0c + (N) * 0x14)

#define APBTMR_CONTROL_ENABLE		(1 << 0)
/* 1: periodic, 0:free running. */
#define APBTMR_CONTROL_MODE_PERIODIC	(1 << 1)
#define APBTMR_CONTROL_INT		(1 << 2)

struct axera_work_struct {
	struct work_struct timer_work;
	unsigned int channel_id;
};

struct axera_timer_handle {
	u32 handle;
	unsigned int channel_id;
};

struct axera_dw_apb_timer {
	void __iomem	*base;
	void __iomem	*clk_base;
	unsigned int	clk_sel_addr_offset;
	unsigned int	clk_sel_offset;
	unsigned int	clk_glb_eb_addr_offset;
	unsigned int	clk_glb_eb_offset;
	unsigned int	clk_eb_addr_offset;
	unsigned int	clk_eb_offset;
	unsigned int	clk_p_eb_addr_offset;
	unsigned int	clk_p_eb_offset;
	int		irq[PER_TIMER_CHANNEL_COUNT];
	int		start_irq;
	int 		dev_id;
	void (*eoi)(struct axera_dw_apb_timer *, unsigned int channel_id);
	void (*timer_handle[PER_TIMER_CHANNEL_COUNT])(void *);
	void *arg[PER_TIMER_CHANNEL_COUNT];
	int timer_status[PER_TIMER_CHANNEL_COUNT];
	spinlock_t info_lock[PER_TIMER_CHANNEL_COUNT];
	struct axera_work_struct timer_work[PER_TIMER_CHANNEL_COUNT];
	struct axera_timer_handle handle[PER_TIMER_CHANNEL_COUNT];
	struct reset_control *preset;
	struct reset_control *reset;
	struct clk *pclk;
	struct clk *clk;
};

struct axera_irq_data {
	struct axera_dw_apb_timer *timer;
	unsigned int channel_id;
};

u32 *timer_open(unsigned int dev_id, unsigned int channel_id);
int timer_set(u32 *timer_handle, u32 usec, void (*handle)(void *), void *arg);
void timer_close(u32 *timer_handle);

#endif