/*
 * linux/arch/arm/mach-ak39/pm.c
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
//#include <linux/anyka_cpufreq.h>
#include <asm/cacheflush.h>
#include <plat/l2.h>
#include <plat/l2_exebuf.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <mach/pm.h>

void check_poweroff(void);

static suspend_state_t target_state;

#define ak39_pm_debug_init() do { } while(0)

static int ak39_pm_valid_state(suspend_state_t state)
{
	switch (state) {
		case PM_SUSPEND_ON:
		case PM_SUSPEND_STANDBY:
		case PM_SUSPEND_MEM:
		    return 1;
		default:
		    return 0;
	}
}

/*
 * Called after processes are frozen, but before we shutdown devices.
 */
static int ak39_pm_begin(suspend_state_t state)
{
    	target_state = state;
    	return 0;
}

static int ak39_pm_gpios_wakeup_enable(struct ak_pm_pdata *pdata)
{
	struct ak_wakeup_gpio *pgpio = pdata->gpios;
	int i;

	for (i = 0; i < pdata->nr_gpios; i++, pgpio++) {
		ak_gpio_wakeup_pol(pgpio->pin, pgpio->wakeup_pol);	/* AK_RISING_TRIGGERED or AK_FALLING_TRIGGERED */

		ak_gpio_wakeup(pgpio->pin, AK_WAKEUP_ENABLE);
	}

	return 0;
}

#define ak39_pm_gpio_status_clear()	do {		\
		REG32(GPI0_WAKEUP_STACLR) = 0x00000000;		\
		REG32(GPI0_WAKEUP_STACLR) = 0xFFFFFFFF ;	\
													\
		while(REG32(GPI0_WAKEUP_STA) != 0)			\
		REG32(GPI0_WAKEUP_STACLR) = 0xFFFFFFFF ;  	\
		REG32(GPI0_WAKEUP_STACLR) = 0x00000000;		\
	} while (0)

void L2_LINK(standby) L2FUNC_NAME(standby)(unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4)
{
//	unsigned long val,read_val;
	
	
	DISABLE_CACHE_MMU();// invalidate and disable mmu
   
	// check this bit and unitil both are empty 
	while(!((REG32(PHY_RAM_CFG_REG4) & (FIFO_R_EMPTY | FIFO_CMD_EMPTY)) == (FIFO_R_EMPTY | FIFO_CMD_EMPTY)));
	
	PM_DELAY(0x10);//at least more than 1 tck	
	
	REG32(PHY_RAM_CFG_REG4) &= ~(AUTO_REFRESH_EN);// setup periodic of refresh interval and disable auto-refresh
    
	DDR2_ENTER_SELFREFRESH();// send all bank precharge
	PM_DELAY(0x10);//at least more than 1 tck

	ak39_pm_gpio_status_clear();

	PM_DELAY(0x2000);  //at least more than 3 tck only for selfresh	

	REG32(0x21800000) = 0x1 ;  //goto standby	
		
	// the system is standby ......

	/* instruction:
	 *	PM_DELAY(0x1) = 128.8ns  when mem clk = 200M 
	 */
	PM_DELAY(0x10); // at least more than 1 tck prior exit self  refresh
    
	// exit DDR2 self-refresch
	DDR2_EXIT_SELFREFRESH();
	
	// send auto refresh and open odt high
	DDR2_ENTER_AUTOREFRESH();

	// enable auto-refresh
	REG32(PHY_RAM_CFG_REG4) |= AUTO_REFRESH_EN;
	PM_DELAY(0x100);

	// enable ICache & DCache, mmu
	ENABLE_CACHE_MMU();
}

static int ak39_pm_enter(suspend_state_t state)
{
	unsigned long flags;
	
	local_irq_save(flags);
	ak39_pm_debug_init();
	flush_cache_all();	
	SPECIFIC_L2BUF_EXEC(standby, 0,0,0,0);
	local_irq_restore(flags);
	return 0;
}

/*
 * Called right prior to thawing processes.
 */
static void ak39_pm_end(void)
{
    target_state = PM_SUSPEND_ON;
}

static struct platform_suspend_ops ak39_pm_ops = {
	.valid	= ak39_pm_valid_state,
	.begin	= ak39_pm_begin,
	.enter	= ak39_pm_enter,
	.end	= ak39_pm_end,
};

static int __init ak39_pm_probe(struct platform_device *pdev)
{
	struct ak_pm_pdata *pdata;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "cannot get platform data\n");
		return -ENOENT;
	}

	ak39_pm_gpios_wakeup_enable(pdata);

	suspend_set_ops(&ak39_pm_ops);


	return 0;
}

static int __exit ak39_pm_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver ak39_pm_driver = {
	.driver = {
		.name	 = "pm-anyka",
		.owner	 = THIS_MODULE,
	},
	.remove = __exit_p(davinci_pm_remove),
};

static int __init ak39_pm_init(void)
{
	return platform_driver_probe(&ak39_pm_driver, ak39_pm_probe);
}
late_initcall(ak39_pm_init);

