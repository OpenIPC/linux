#ifndef __AK_TIMER_COMMON_H__
#define __AK_TIMER_COMMON_H__

/************************  Hardware Timer macro *********************/
#define AK_TIMER0_CTRL1         (AK_VA_SYSCTRL + 0xB4)
#define AK_TIMER0_CTRL2         (AK_VA_SYSCTRL + 0xB8)
#define AK_TIMER1_CTRL1         (AK_VA_SYSCTRL + 0xBC)
#define AK_TIMER1_CTRL2         (AK_VA_SYSCTRL + 0xC0)
#define AK_TIMER2_CTRL1         (AK_VA_SYSCTRL + 0xC4)
#define AK_TIMER2_CTRL2         (AK_VA_SYSCTRL + 0xC8)
#define AK_TIMER3_CTRL1         (AK_VA_SYSCTRL + 0xCC)
#define AK_TIMER3_CTRL2         (AK_VA_SYSCTRL + 0xD0)
#define AK_TIMER4_CTRL1         (AK_VA_SYSCTRL + 0xD4)
#define AK_TIMER4_CTRL2         (AK_VA_SYSCTRL + 0xD8)

#ifdef CONFIG_MACH_AK37D
#define AK_TIMER5_CTRL1         (AK_VA_SYSCTRL + 0x1C0)
#define AK_TIMER5_CTRL2         (AK_VA_SYSCTRL + 0x1C4)
#define AK_TIMER6_CTRL1         (AK_VA_SYSCTRL + 0x1C8)
#define AK_TIMER6_CTRL2         (AK_VA_SYSCTRL + 0x1CC)
#endif

#ifdef CONFIG_MACH_AK39EV330
#define AK_TIMER5_CTRL1         (AK_VA_SYSCTRL + 0xF4)
#define AK_TIMER5_CTRL2         (AK_VA_SYSCTRL + 0xF8)
#define AK_TIMER6_CTRL1         (AK_VA_SYSCTRL + 0xFC)
#define AK_TIMER6_CTRL2         (AK_VA_SYSCTRL + 0x100)
#define AK_TIMER7_CTRL1         (AK_VA_SYSCTRL + 0x104)
#define AK_TIMER7_CTRL2         (AK_VA_SYSCTRL + 0x108)
#define AK_TIMER8_CTRL1         (AK_VA_SYSCTRL + 0x10C)
#define AK_TIMER8_CTRL2         (AK_VA_SYSCTRL + 0x110)
#define AK_TIMER9_CTRL1         (AK_VA_SYSCTRL + 0x114)
#define AK_TIMER9_CTRL2         (AK_VA_SYSCTRL + 0x118)
#endif

#ifdef CONFIG_MACH_AK37E
#define AK_TIMER5_CTRL1         (AK_VA_SYSCTRL + 0x01C0)
#define AK_TIMER5_CTRL2         (AK_VA_SYSCTRL + 0x01C4)
#define AK_TIMER6_CTRL1         (AK_VA_SYSCTRL + 0x01C8)
#define AK_TIMER6_CTRL2         (AK_VA_SYSCTRL + 0x01CC)
#define AK_TIMER7_CTRL1         (AK_VA_SYSCTRL + 0x0200)
#define AK_TIMER7_CTRL2         (AK_VA_SYSCTRL + 0x0204)
#define AK_TIMER8_CTRL1         (AK_VA_SYSCTRL + 0x0208)
#define AK_TIMER8_CTRL2         (AK_VA_SYSCTRL + 0x020C)
#define AK_TIMER9_CTRL1         (AK_VA_SYSCTRL + 0x0210)
#define AK_TIMER9_CTRL2         (AK_VA_SYSCTRL + 0x0214)
#endif

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
#define AK_TIMER5_CTRL1         (AK_VA_SYSCTRL + 0xF4)
#define AK_TIMER5_CTRL2         (AK_VA_SYSCTRL + 0xF8)
#define AK_TIMER6_CTRL1         (AK_VA_SYSCTRL + 0xFC)
#define AK_TIMER6_CTRL2         (AK_VA_SYSCTRL + 0x100)
#define AK_TIMER7_CTRL1         (AK_VA_SYSCTRL + 0x104)
#define AK_TIMER7_CTRL2         (AK_VA_SYSCTRL + 0x108)
#define AK_TIMER8_CTRL1         (AK_VA_SYSCTRL + 0x10C)
#define AK_TIMER8_CTRL2         (AK_VA_SYSCTRL + 0x110)
#define AK_TIMER9_CTRL1         (AK_VA_SYSCTRL + 0x114)
#define AK_TIMER9_CTRL2         (AK_VA_SYSCTRL + 0x118)
#endif

/********************** CE and CS mapping ***************************/
#ifdef CONFIG_MACH_AK37D
#define AK_CE_CTRL1             AK_TIMER5_CTRL1
#define AK_CE_CTRL2             AK_TIMER5_CTRL2
#define AK_CS_CTRL1             AK_TIMER6_CTRL1
#define AK_CS_CTRL2             AK_TIMER6_CTRL2
#define AK_CE_TIMER_INDEX       5
#define AK_CS_TIMER_INDEX       6
#endif

#ifdef CONFIG_MACH_AK37E
#define AK_CE_CTRL1             AK_TIMER8_CTRL1
#define AK_CE_CTRL2             AK_TIMER8_CTRL2
#define AK_CS_CTRL1             AK_TIMER9_CTRL1
#define AK_CS_CTRL2             AK_TIMER9_CTRL2
#define AK_CE_TIMER_INDEX       8
#define AK_CS_TIMER_INDEX       9
#endif

#ifdef CONFIG_MACH_AK39EV330
#define AK_CE_CTRL1             AK_TIMER5_CTRL1
#define AK_CE_CTRL2             AK_TIMER5_CTRL2
#define AK_CS_CTRL1             AK_TIMER6_CTRL1
#define AK_CS_CTRL2             AK_TIMER6_CTRL2
#define AK_CE_TIMER_INDEX       5
#define AK_CS_TIMER_INDEX       6
#endif

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
#define AK_CE_CTRL1             AK_TIMER8_CTRL1
#define AK_CE_CTRL2             AK_TIMER8_CTRL2
#define AK_CS_CTRL1             AK_TIMER9_CTRL1
#define AK_CS_CTRL2             AK_TIMER9_CTRL2
#define AK_CE_TIMER_INDEX       8
#define AK_CS_TIMER_INDEX       9
#endif

#define TIMER_CLK_INPUT         (12000000)
#define TIMER_CNT               (TIMER_CLK_INPUT/HZ)
#define TIMER_CNT_MASK          (0x3F<<26)
#define TIMER_USEC_SHIFT               16

/* define timer control register2 bits */
#define TIMER_CLEAR_BIT         (1<<30)
#define TIMER_FEED_BIT          (1<<29)
#define TIMER_ENABLE_BIT         (1<<28)
#define TIMER_STATUS_BIT        (1<<27)
#define TIMER_READ_SEL_BIT      (1<<26)

/* define working mode */
#define MODE_AUTO_RELOAD_TIMER  (0x0<<24)
#define MODE_ONE_SHOT_TIMER     (0x1<<24)
#define MODE_PWM                (0x2<<24)   



#endif
