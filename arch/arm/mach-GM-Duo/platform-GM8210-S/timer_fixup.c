#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <mach/ftpmu010.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/fiq.h>
#include <mach/platform/board.h>

#define CONFIG_MAX_TICK     0xffffffff
static int timer_init_ready = 0;

/*
 * Get the max ms the this timer
 */
unsigned int video_gettime_max_ms(void)
{
    return (CONFIG_MAX_TICK / APB0_CLK_IN) * 1000;
}

/* calculate the max hz in max_ms
 */
unsigned int video_gettime_max_hz(void)
{
    unsigned int max_hz;

    /* calculate the max hz in max_ms. Actually it is (video_gettime_max_ms() / 1000) * APB0_CLK_IN
     */
    max_hz = video_gettime_max_ms() * (APB0_CLK_IN / 1000);

    return max_hz;
}

unsigned int video_gettime_ms(void)
{
    //0~35 sec round
    unsigned int timer_base = TIMER_FTTMR010_VA_BASE;

    if (timer_init_ready == 1)
    {
        unsigned int ft_apb_clk = APB0_CLK_IN;
        unsigned int value = *(volatile unsigned int *)(timer_base + 0x20);
        unsigned int tick_diff;

        tick_diff = CONFIG_MAX_TICK - video_gettime_max_hz();   //counter shift
        return (value - tick_diff) / (ft_apb_clk / 1000);
    }
    else
    {
        //printk("TIMER not ready\n");
        return 0;
    }
}

unsigned int video_gettime_us(void)
{
    //0~35 sec round
    unsigned int timer_base = TIMER_FTTMR010_VA_BASE;

    if (timer_init_ready == 1)
    {
        unsigned int ft_apb_clk = APB0_CLK_IN;
        unsigned int value = *(volatile unsigned int *)(timer_base + 0x20);
        unsigned int tick_diff;

        tick_diff = CONFIG_MAX_TICK - video_gettime_max_hz();   //counter shift

        return (value - tick_diff) / (ft_apb_clk / 1000000);
    }
    else
    {
        //printk("TIMER not ready\n");
        return 0;
    }
}

/*
 * Entry Point of this module. It may be declared by arch_initcall section
 */
static int __init fixup_timer_init(void)
{
    //use timer3, 42s will overflow in AHB clock 200HZ
    unsigned int ft_apb_clk = APB0_CLK_IN;
    unsigned int timer_base = TIMER_FTTMR010_VA_BASE;
    unsigned int max_tick;

    if (*(unsigned int *)(timer_base + 0x30) & (0x1 << 6))
    {
        printk("Timer3 is alredy been used!\n");
        return 0;
    }

    max_tick = video_gettime_max_ms() * (ft_apb_clk / 1000);    //the max hz count in TmxCounter

    printk("Video Timer(timer3) Max %dms in 0x%x HZ.\n", video_gettime_max_ms(), max_tick);

    /* the following configuration is for TIMER3 */
    *(volatile unsigned int *)(timer_base + 0x20) = CONFIG_MAX_TICK - max_tick; //keep the start count in start counter
    *(volatile unsigned int *)(timer_base + 0x24) = CONFIG_MAX_TICK - max_tick; //reLoad counter
    *(volatile unsigned int *)(timer_base + 0x38) |= (0x7 << 6);   //IRQ disable
    *(volatile unsigned int *)(timer_base + 0x30) |= ((0x1 << 6) | (0x1 << 11));    //enable timer3, count up
    timer_init_ready = 1;

    return 0;
}

static void fixup_timer_exit(void)
{
    if (timer_init_ready == 1)
    {
        unsigned int timer_base = TIMER_FTTMR010_VA_BASE;
        *(volatile unsigned int *)(timer_base + 0x30) &= ~(0x1 << 6);//disable timer2, count up
        timer_init_ready = 0;
    }
	printk("%s: cleaning up\n",__func__);
}

module_init(fixup_timer_init);
module_exit(fixup_timer_exit);

EXPORT_SYMBOL(video_gettime_ms);
EXPORT_SYMBOL(video_gettime_us);
EXPORT_SYMBOL(video_gettime_max_ms);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("FIXUP timer driver");
