#ifndef __AK_L2_EXEBUF__
#define __AK_L2_EXEBUF__

#include <linux/compiler.h>


#define AK_PM_GPIO_WAKEUP                   0x1
#define AK_PM_RTC_WAKEUP                    0x2
#define AK_PM_AIN0_WAKEUP                   0x3
#define AK_PM_RTC_TIMER_WAKEUP              0x4
#define AK_PM_USB_DP_WAKEUP                 0x5
#define AK_PM_WAKEUP_GPIO2                  2
#define AK_PM_WAKEUP_GPIO8                  8
#define AK_PM_WAKEUP_GPIO9                  9
#define AK_PM_WAKEUP_GPIO16                 16
#define AK_PM_WAKEUP_GPIO20                 20
#define AK_PM_WAKEUP_GPIO22                 22
#define AK_PM_WAKEUP_GPIO24                 24
#define AK_PM_WAKEUP_GPIO28                 28
#define AK_PM_WAKEUP_GPIO31                 31
#define AK_PM_WAKEUP_GPIO46                 46
#define AK_PM_WAKEUP_GPIO49                 49
#define AK_PM_WAKEUP_GPIO51                 51
#define AK_PM_WAKEUP_GPIO53                 53
#define AK_PM_WAKEUP_GPIO55                 55
#define AK_PM_WAKEUP_GPIO56                 56
#define AK_PM_WAKEUP_GPIO61                 61
#define AK_PM_WAKEUP_GPIO67                 67
#define AK_PM_WAKEUP_GPIO68                 68
#define AK_PM_WAKEUP_GPIO70                 70
#define AK_PM_WAKEUP_GPIO72                 72
#define AK_PM_WAKEUP_GPIO75                 75
#define AK_PM_WAKEUP_GPIO78                 78
#define AK_PM_WAKEUP_GPIO84                 84
#define AK_PM_WAKEUP_GPIO85                 85
#define AK_PM_WAKEUP_GPIO86                 86


struct pm_wakeup_info{
    int wakeup_mode[5];
    int wakeup_gpio_index[25];
    int wakeup_gpio_cnt;
    int wakeup_gpio_edge[25];
    int wakeup_gpio_edge_cnt;
    int wakeup_ain0_edge;
    int wakeup_mode_cnt;
};

#define WAKE_UP_GPIO_TRIGGER_STA_CLR_REG        (AK_VA_SYSCTRL+0x0040)
#define WAKE_UP_GPIO_POLARITY_SEL_REG           (AK_VA_SYSCTRL+0x003c)
#define WAKE_UP_GPIO_FUNC_ENA_REG               (AK_VA_SYSCTRL+0x0044)
#define OTHER_WAKE_UP_SRC_CTRL_REG              (AK_VA_SYSCTRL+0x0034)
#define OTHER_WAKE_UP_SRC_TRIGGER_STA_REG       (AK_VA_SYSCTRL+0x0038)
#define USB_OPERATE_MODE_I2S_MODE_CTRL_REG      (AK_VA_SYSCTRL+0x0058)
#define AUDIO_CODEC_CFG1_REG                    (AK_VA_SYSCTRL+0x00a0)


#if 1
#define DISABLE_CACHE_MMU() do { \
    __asm__ __volatile__(   \
        "tci_loop: mrc  p15, 0, r15, c7, c14, 3\n\t" /* test, clean,\
                                            invalidate D cache */\
        "bne  tci_loop\n\t"                                     \
        "mcr  p15, 0, %0, c8, c7, 0\n\t"    /* invalidate  TLBs */  \
        "mcr  p15, 0, %0, c7, c5, 0\n\t"    /* invalidate I caches */   \
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x1000\n\t"      /* disable Icache */            \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x0004\n\t"      /* disable Dcache*/     \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x0001\n\t"      /* disable mmu*/        \
        "ldr  %1, =l2_phys_run\n\t"     /* load 0x480000xx address */   \
        "b  suspend_turn_off_mmu\n\t"                               \
        "   .align 5\n\t"               /* 32 byte aligned */           \
        "suspend_turn_off_mmu:\n\t"\
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mov  pc, %1\n\t"       /* jumpto 0x480000xx then run */        \
        "l2_phys_run:\n\t"      /* mark the real running addr--> L2 buff */\
        ::"r"(0),"r"(1));   \
    } while(0)

#else
#define DISABLE_CACHE_MMU() do { \
    __asm__ __volatile__(   \
        "tci_loop: mrc  p15, 0, r15, c7, c14, 3\n\t" /* test, clean,\
                                            invalidate D cache */\
        "bne  tci_loop\n\t"                                     \
        "mcr  p15, 0, %0, c8, c7, 0\n\t"    /* invalidate  I & D  TLBs */   \
        "mcr  p15, 0, %0, c7, c5, 0\n\t"    /* invalidate I caches */   \
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x1000\n\t"      /* disable Icache */            \
        "bic  %0, %0, #0x0005\n\t"      /* disable Dcache,mmu*/     \
        "ldr  %1, =l2_phys_run\n\t"     /* load 0x480000xx address */   \
        "b  suspend_turn_off_mmu\n\t"                               \
        "   .align 5\n\t"               /* 32 byte aligned */           \
        "suspend_turn_off_mmu:\n\t"\
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mov  pc, %1\n\t"       /* jumpto 0x480000xx then run */        \
        "l2_phys_run:\n\t"      /* mark the real running addr--> L2 buff */\
        "ldr r2, =0x21000010\n\t"  \
        "ldr r1, =( (0x1<<25)| (0x1<<23) |(0x1<<21) | (0x1<<10) )\n\t"  \
        "str r1, [r2]\n\t"  \
        "ldr r1, =5\n\t"  \
        "loop_timedelay1:\n\t"  \
        "subs r1, r1, #1\n\t"  \
        "bne loop_timedelay1\n\t"  \
        "ldr r2, =0x21000010\n\t"  \
        "ldr r1, =( (0x1<<26) | (0x1<<23) |(0x1<<22) )\n\t"  \
        "str r1, [r2]\n\t"  \
        "andeq r0, r0, r0\n\t"  \
        "andeq r0, r0, r0\n\t"  \
        "andeq r0, r0, r0\n\t"  \
        "andeq r0, r0, r0\n\t"  \
        "ldr r2, =0x21000078\n\t"  \
        "ldr r1, [r2]\n\t"  \
        "orr r1, r1, #0x80000000\n\t"  \
        "str r1, [r2]\n\t"  \
        "ldr r2, =0x21800000\n\t"  \
        "ldr r1, =1\n\t"    \
        "str r1, [r2]\n\t"  \
        "ldr r1, =0x5000\n\t"  \
        "loop_timedelay2:\n\t"  \
        "subs r1, r1, #1\n\t"   \
        "bne loop_timedelay2\n\t"   \
        ::"r"(0),"r"(1));   \
    } while(0)

#endif


#if 1
#define ENABLE_CACHE_MMU()  do { \
    __asm__ __volatile__(   \
        "mcr  p15, 0, %0, c8, c7, 0\n\t"    /* invalidate I & D TLBs */ \
        "mcr  p15, 0, %0, c7, c7, 0\n\t"    /* invalidate I & D caches */\
        "mcr  p15, 0, %0, c7, c10, 4\n\t"   /* Drain write buffer */        \
        "mrc  p15, 0, %0, c1, c0, 0\n\t"    \
        "orr  %0, %0, #0x1000\n\t"          \
        "orr  %0, %0, #0x0005\n\t"          \
        "b  resume_turn_on_mmu\n\t"         \
        "   .align 5\n\t"                   \
        "resume_turn_on_mmu:\n\t"           \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"    \
        ::"r"(2));  \
    } while(0)
#else
#define ENABLE_CACHE_MMU()  do { \
    __asm__ __volatile__(   \
        "ldr r2, =0x21000078\n\t"           \
        "ldr r1, [r2]\n\t"          \
        "bic r1, r1, #0x80000000\n\t"           \
        "str r1, [r2]\n\t"          \
        "ldr r2, =0x21000010\n\t"           \
        "ldr r1, =( (0x1<<25) | (0x1<<23) | (0x1<<22) |     \
                        (0x1<<21) | (0x1<<20) )\n\t"     \
        "str r1, [r2]\n\t"          \
        "ldr r1, =30\n\t"           \
        "delay_txnrcycle:\n\t"          \
        "subs r1, r1, #1\n\t"           \
        "bne delay_txnrcycle\n\t"           \
        "ldr r2, =0x21000010\n\t"           \
        "ldr r1, =( (0x1<<25) | (0x1<<23) | (0x1<<22) \
                    | (0x1<<21) | (0x1<<20) )\n\t"            \
        "str r1, [r2]\n\t"          \
        "ldr r2, =0x2100000c\n\t"           \
        "ldr r1, [r2]\n\t"          \
        "orr r1, r1, #0x1\n\t"          \
        "str r1, [r2]\n\t"          \
        "ldr r1, =200\n\t"          \
        "delay_200cycle:\n\t"           \
        "subs r1, r1, #1\n\t"           \
        "bne delay_200cycle\n\t"            \
        "andeq r0, r0, r0\n\t"          \
        "andeq r0, r0, r0\n\t"          \
        "andeq r0, r0, r0\n\t"          \
        "andeq r0, r0, r0\n\t"          \
        "mcr  p15, 0, %0, c8, c7, 0\n\t"    /* invalidate I & D TLBs */ \
        "mcr  p15, 0, %0, c7, c7, 0\n\t"    /* invalidate I & D caches */\
        "mcr  p15, 0, %0, c7, c10, 4\n\t"   /* Drain write buffer */        \
        "mrc  p15, 0, %0, c1, c0, 0\n\t"    \
        "orr  %0, %0, #0x1000\n\t"          \
        "orr  %0, %0, #0x0005\n\t"          \
        "b  resume_turn_on_mmu\n\t"         \
        "   .align 5\n\t"                   \
        "resume_turn_on_mmu:\n\t"           \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"    \
        ::"r"(2));  \
    } while(0)

#endif


#define PM_DELAY(time)  do { \
    __asm__ __volatile__(   \
        "1:\n\t"    \
        "subs %0, %0, #1\n\t"   \
        "bne 1b\n\t"            \
        ::"r"(time));   \
    } while(0)


#define L2_LINK(flag)       __section(.l2mem_##flag)
#define L2FUNC_NAME(name)   l2_enter_##name


#define SPECIFIC_L2BUF_EXEC(flag, param1,param2,param3,param4) do {\
    extern char _end_##flag[], _start_##flag[];\
    int len;\
    len = _end_##flag - _start_##flag;\
    l2_exec_buf(_start_##flag,len, param1,param2,param3,param4);\
}while(0)

/*
 * copy from ddr2 to l2 memory to run, and exit standby
 */
int l2_exec_buf(const char *vaddr, int len, unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4);

#endif  /* __AK_L2_EXEBUF__ */


/*end of file*/
