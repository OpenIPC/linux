#include <linux/init.h>
#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/ctype.h>

#include <linux/dma-mapping.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <asm/io.h>
#include <soc/ddr.h>
#include <soc/base.h>
#include <soc/cpm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "pm.h"
#include "pm_sleep.h"
#include "pm_fastboot.h"
#if defined CONFIG_KP_AXP
#include <linux/regulator/machine.h>
#endif


#define SLEEP_LEVEL		(*(volatile unsigned int *)0xb0000034 & 0xf)
#define SLEEP_LEVEL_MAGIC	((*(volatile unsigned int *)0xb0000034 >> 16) & 0xffff)
#define SLEEP_MAGIC		(0x0915)

#if 0
#ifdef DEBUG_PM
static void t41_pm_gate_check(void)
{
    unsigned int gate0 = cpm_inl(CPM_CLKGR);
    unsigned int gate1 = cpm_inl(CPM_CLKGR1);
    int i;
    int x;

    printk("gate0 = 0x%08x\n", gate0);
    printk("gate1 = 0x%08x\n", gate1);
    for (i = 0; i < 32; i++) {
        x = (gate0 >> i) & 1;
        if (x == 0)
            printk("warning : bit[%d] in clk gate0 is enabled\n", i);
    }
    for (i = 0; i < 32; i++) {
        x = (gate1 >> i) & 1;
        if (x == 0)
            printk("warning : bit[%d] in clk gate1 is enabled\n", i);
    }

}
#endif
#endif

void load_func_to_tcsm(unsigned int tcsm_addr,unsigned int *f_addr,unsigned int size)
{
    unsigned int instr;
    int offset;
    int i;
#ifdef DEBUG_PM
    printk("tcsm addr = %#x %p size = %d\n",tcsm_addr,f_addr,size);
#endif
    for(i = 0;i < size / 4;i++) {
        instr = f_addr[i];
        if((instr >> 26) == 2){
            offset = instr & 0x3ffffff;
            offset = (offset << 2) - ((unsigned int)f_addr & 0xfffffff);
            if(offset > 0) {
                offset = ((unsigned int)tcsm_addr & 0xfffffff) + offset;
                instr = (2 << 26) | (offset >> 2);
            }
        }
        (*(volatile unsigned int *)(tcsm_addr + i*4)) = instr;
    }

    for(i = 0;i < size / 4;i++) {
        instr = f_addr[i];
        if((instr >> 26) == 2){
            offset = instr & 0x3ffffff;
            offset = (offset << 2) - ((unsigned int)f_addr & 0xfffffff);
            if(offset > 0) {
                offset = ((unsigned int)tcsm_addr & 0xfffffff) + offset;
                instr = (2 << 26) | (offset >> 2);
            }
        }
        if((*(volatile unsigned int *)(tcsm_addr + i*4)) != instr){
            printk("copy sleep code error\n");
            while(1);
        }
    }
}
#if 0
static int soc_pm_idle(void)
{
    printk("soc pm idle \n");

    load_func_to_sram();

    soc_pm_idle_config();

    return 0;
}

static int soc_pm_idle_pd(void)
{
    printk("soc pm idle pd \n");

    load_func_to_sram();

    soc_pm_idle_pd_config();

    soc_set_reset_entry();

    return 0;
}

static void soc_pm_sleep(void)
{
    printk("soc pm sleep \n");

    load_func_to_sram();

    soc_pm_sleep_config();

    soc_set_reset_entry();
}
#endif
static void soc_pm_fastboot(void)
{
    soc_pm_fastboot_config();

    load_func_to_rtc_ram();

    sys_save();

    load_func_to_oram();
}

static void goto_sleep(unsigned int sleep_addr)
{
    mb();
    save_goto(sleep_addr);
    mb();
}


static unsigned long pm_suspend_start_time;
int t41_pm_enter(suspend_state_t state)
{
    unsigned int sleep_addr = 0;
    unsigned int sleep_level = 0;

    sleep_param->uart_base = (UART0_IOBASE | 0xa0000000) + UART_OFF * bc_idx;
    sleep_param->state = state;


    printk("t41 pm enter\n");
    soc_pm_fastboot();


    sleep_addr = FASTBOOT_SLEEP_CODE_ADDR;
    sleep_level = FASTBOOT;

    {
        long nj = jiffies - pm_suspend_start_time;
        unsigned msec;

        msec = jiffies_to_msecs(abs(nj));
        pr_info("PM: %s completion time %d.%03d seconds\n", "Suspend",
                msec / 1000, msec % 1000);
    }

    goto_sleep((unsigned int )sleep_addr);
    if (sleep_level == FASTBOOT) {
        soc_pm_wakeup_fastboot();
    } else {
        soc_pm_wakeup_idle_sleep();
    }
    return 0;
}

static int t41_pm_begin(suspend_state_t state)
{
    printk("t41 suspend begin\n");
    pm_suspend_start_time = jiffies;
    return 0;
}

static void t41_pm_end(void)
{
    printk("t41 pm end!\n");
}
#if defined CONFIG_KP_AXP
static int t41_suspend_prepare(void)
{
    return regulator_suspend_prepare(PM_SUSPEND_MEM);
}
static void t41_suspend_finish(void)
{
    if (regulator_suspend_finish())
        pr_err("%s: Suspend finish failed\n", __func__);
}
#endif

static int ingenic_pm_valid(suspend_state_t state)
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
static const struct platform_suspend_ops t41_pm_ops = {
    .valid		= ingenic_pm_valid,
    .begin		= t41_pm_begin,
    .enter		= t41_pm_enter,
    .end		= t41_pm_end,
#if defined CONFIG_KP_AXP
    .prepare = t41_suspend_prepare,
    .finish = t41_suspend_finish,
#endif
};


int bc_idx = 0;
static int __init suspend_console_setup(char *str)
{

    char buf[32];
    char *s;

    strncpy(buf, str, sizeof(buf) - 1);

    for (s = buf; *s; s++)
        if (isdigit(*s) || *s == ',')
            break;

    bc_idx = simple_strtoul(s, NULL, 10);

    return 0;
}

__setup("console=", suspend_console_setup);

/*
 * Initialize suspend interface
 */
static int __init pm_init(void)
{

    suspend_set_ops(&t41_pm_ops);


    return 0;
}

late_initcall(pm_init);
