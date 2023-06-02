#ifndef __FH_WDT_PLATFORM_DATA
#define __FH_WDT_PLATFORM_DATA

#include <linux/platform_device.h>

struct fh_wdt_platform_reset {
	unsigned int spi0_cs_pin;
	unsigned int spi0_rst_bit;
	unsigned int sd0_rst_bit;
	unsigned int uart0_rst_bit;
};

struct fh_wdt_platform_data {
	void (*resume)(void);
	void (*pause)(void);
	irqreturn_t (*intr)(void *pri);
	struct fh_wdt_platform_reset *plat_info;
};

struct fh_wdt_t {
	spinlock_t		lock;
	void __iomem		*regs;
	struct clk		*clk;
	unsigned long		in_use;
	unsigned long		next_heartbeat;
	struct timer_list	timer;
	int			expect_close;
	struct fh_wdt_platform_data *plat_data;
};

#endif

