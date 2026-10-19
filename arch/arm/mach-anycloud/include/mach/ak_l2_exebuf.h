#ifndef __AK_L2_EXEBUF__
#define __AK_L2_EXEBUF__

#include <linux/compiler.h>


//#define AK_PM_BUG
#ifdef AK_PM_BUG
#define AK_BUG pr_err
#else
#define AK_BUG(fmt, arg...)
#endif

#define AK_PM_GPIO_WAKEUP                   0x1
#define AK_PM_RTC_WAKEUP                    0x2
#define AK_PM_AIN0_WAKEUP                   0x3
#define AK_PM_RTC_TIMER_WAKEUP              0x4
#define AK_PM_USB_DP_WAKEUP                 0x5

#define AK_PM_PWR_WAKEUP                    0x1
#define AK_PM_RTC_INT_WAKEUP                0x2
#define AK_PM_BUTTON_WAKEUP                 0x3



#if defined(CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
#define AK_PM_WAKEUP_GPIO71                 71
#define AK_PM_WAKEUP_GPIO70                 70
#define AK_PM_WAKEUP_GPIO2                  2
#define AK_PM_WAKEUP_GPIO3                  3
#define AK_PM_WAKEUP_GPIO10                 10
#define AK_PM_WAKEUP_GPIO11                 11
#define AK_PM_WAKEUP_GPIO24                 24
#define AK_PM_WAKEUP_GPIO25                 25
#define AK_PM_WAKEUP_GPIO26                 26
#define AK_PM_WAKEUP_GPIO27                 27
#define AK_PM_WAKEUP_GPIO37                 37
#define AK_PM_WAKEUP_GPIO42                 42
#define AK_PM_WAKEUP_GPIO44                 44
#define AK_PM_WAKEUP_GPIO45                 45
#define AK_PM_WAKEUP_GPIO48                 48
#define AK_PM_WAKEUP_GPIO49                 49
#define AK_PM_WAKEUP_GPIO50                 50
#define AK_PM_WAKEUP_GPIO51                 51
#define AK_PM_WAKEUP_GPIO52                 52

#define AK_PM_WAKEUP_GPIO55                 55
#define AK_PM_WAKEUP_GPIO56                 56
#define AK_PM_WAKEUP_GPIO58                 58
#define AK_PM_WAKEUP_GPIO65                 65
#define AK_PM_WAKEUP_GPIO66                 66
#define AK_PM_WAKEUP_GPIO67                 67
#define AK_PM_WAKEUP_GPIO68                 68
#define AK_PM_WAKEUP_GPIO69                 69

#else
#define AK_PM_WAKEUP_GPIO71                 71
#define AK_PM_WAKEUP_GPIO70                 70
#define AK_PM_WAKEUP_GPIO2                  2
#define AK_PM_WAKEUP_GPIO3                  3
#define AK_PM_WAKEUP_GPIO10                 10
#define AK_PM_WAKEUP_GPIO11                 11
#define AK_PM_WAKEUP_GPIO24                 24
#define AK_PM_WAKEUP_GPIO25                 25
#define AK_PM_WAKEUP_GPIO26                 26
#define AK_PM_WAKEUP_GPIO27                 27
#define AK_PM_WAKEUP_GPIO37                 37
#define AK_PM_WAKEUP_GPIO42                 42
#define AK_PM_WAKEUP_GPIO44                 44
#define AK_PM_WAKEUP_GPIO45                 45
#define AK_PM_WAKEUP_GPIO48                 48
#define AK_PM_WAKEUP_GPIO49                 49
#define AK_PM_WAKEUP_GPIO50                 50
#define AK_PM_WAKEUP_GPIO51                 51
#define AK_PM_WAKEUP_GPIO52                 52

#define AK_PM_WAKEUP_GPIO55                 55
#define AK_PM_WAKEUP_GPIO56                 56
#define AK_PM_WAKEUP_GPIO58                 58
#define AK_PM_WAKEUP_GPIO65                 65
#define AK_PM_WAKEUP_GPIO66                 66
#define AK_PM_WAKEUP_GPIO67                 67
#define AK_PM_WAKEUP_GPIO68                 68
#define AK_PM_WAKEUP_GPIO69                 69



#endif


struct pm_wakeup_info{
    int wakeup_mode[5];
    int wakeup_gpio_index[25];
    int wakeup_gpio_cnt;
    int wakeup_gpio_edge[25];
    int wakeup_gpio_edge_cnt;
    int wakeup_ain0_edge;
    int wakeup_mode_cnt;

    int mem_wakeup_mode[4];
    int mem_pwr_wakeup_edge;
    int mem_wakeup_mode_cnt;
    int mem_pwr_td0;
    u32 mem_pwr_button_time;
    unsigned int *ak_pm_resume_data_base;

};

#define WAKE_UP_GPIO_TRIGGER_STA_CLR_REG        (AK_VA_SYSCTRL+0x0040)
#define WAKE_UP_GPIO_POLARITY_SEL_REG           (AK_VA_SYSCTRL+0x003c)
#define WAKE_UP_GPIO_FUNC_ENA_REG               (AK_VA_SYSCTRL+0x0044)
#define OTHER_WAKE_UP_SRC_CTRL_REG              (AK_VA_SYSCTRL+0x0034)
#define OTHER_WAKE_UP_SRC_TRIGGER_STA_REG       (AK_VA_SYSCTRL+0x0038)
#define USB_OPERATE_MODE_I2S_MODE_CTRL_REG      (AK_VA_SYSCTRL+0x0058)
#define AUDIO_CODEC_CFG1_REG                    (AK_VA_SYSCTRL+0x00a0)

#define WAKE_UP_IO_TRIGGER_STA_CLR_REG        (AK_VA_SYSCTRL+0x0244)
#define WAKE_UP_IO_POLARITY_SEL_REG           (AK_VA_SYSCTRL+0x023c)
#define WAKE_UP_IO_FUNC_ENA_REG               (AK_VA_SYSCTRL+0x0240)




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


#if 0

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
#else

/******************MEM**********************/
#define SPECIFIC_L2BUF_EXEC(flag,state, param1,param2,param3,param4) do {\
    extern char _end_##flag[], _start_##flag[];\
    int len;\
    int pm_state;\
    len = _end_##flag - _start_##flag;\
    pm_state = state;\
    l2_exec_buf(_start_##flag,len,pm_state, param1,param2,param3,param4);\
}while(0)

/*
 * copy from ddr2 to l2 memory to run, and exit standby
 */
int l2_exec_buf(const char *vaddr, int len,int pm_state,unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4);

#endif

#endif  /* __AK_L2_EXEBUF__ */

/*
 * pmc_cfg_pwr_td0
 */
int pmc_cfg_pwr_td0(unsigned int pwr_td0_ms);

// pmc_read_pwr_td0
unsigned int pmc_read_pwr_td0(void);

// pmc_poweron_button_press_time_cfg
void pmc_poweron_button_press_time_cfg(unsigned int poweron_press_time);

// pmc_read_poweron_button_press_time
unsigned int pmc_read_poweron_button_press_time(void);

typedef enum _PMC_PWR_WAKEUP_TRIG_MODE
{
    PMC_PWR_WAKEUP_RISING_EDGE_TRIG = 0,
    PMC_PWR_WAKEUP_FALLING_EDGE_TRIG,
    PMC_PWR_WAKEUP_HIGH_LEVEL_TRIG,
    PMC_PWR_WAKEUP_LOW_LEVEL_TRIG,
}T_PMC_PWR_WAKEUP_TRIG_MODE;

typedef enum _PMC_WAKEUP_SOURCE
{
    PMC_PWR_WAKEUP_PIN_WAKEUP = 0,
    PMC_PWR_BUTTON_WAKEUP,
    PMC_RTC_ALARM_WAKEUP,
}T_PMC_WAKEUP_SOURCE;

// pmc_power_wakeup_cfg
void pmc_power_wakeup_cfg(T_PMC_PWR_WAKEUP_TRIG_MODE pwr_wakeup_trig_mode);
// pmc_power_wakeup_enable
void pmc_power_wakeup_enable(void);
// pmc_rtc_int_wakeup_enable
void pmc_rtc_int_wakeup_enable(void);
// pmc_rtc_int_wakeup_disable
void pmc_rtc_int_wakeup_disable(void);
// pmc_software_poweroff
void pmc_software_poweroff(void);
// pmc_read_all_reg_val
void pmc_read_all_reg_val(void);
// read_rtc_interrupt
u8 read_rtc_interrupt(void);
// pmc_read_wakeup_source
T_PMC_WAKEUP_SOURCE pmc_read_wakeup_source(void);
// gpio80_set_pwr_seq
void gpio80_set_pwr_seq(u32 seq);
// PowerSaveBootCfg
void PowerSaveBootCfg(void);
// pmc_clear_poweroff_bit
void pmc_clear_poweroff_bit(void);










//PMC Register
#define PMC_CONTROL_REG0    (0xc)
#define PMC_CONTROL_REG1    (0xd)
#define PMC_STATUS_REG      (0xe)
#define PMC_PAD_CONFIG_REG  (0xf)



#define CHIP_CONF_BASE_ADDR    0x08000000
//#define AUDIO_CODEC_CFG1_REG    (CHIP_CONF_BASE_ADDR + 0x009C) 
// module SYS interrupt status
#define SYSINT_STATUS_REG               (CHIP_CONF_BASE_ADDR + 0x00000030)
// Always On PMU Control 0
#define ALWAYS_ON_PMU_CTRL0_REG         (CHIP_CONF_BASE_ADDR + 0x000000DC)
// Always On PMU Control 1
#define ALWAYS_ON_PMU_CTRL1_REG         (CHIP_CONF_BASE_ADDR + 0x000000E0)
// RTC CFG register
#define RTC_CFG_REG                     (CHIP_CONF_BASE_ADDR + 0x00000050)
// RTC CFG register
#define RTC_RECE_DAT_REG                    (CHIP_CONF_BASE_ADDR + 0x00000054)      // RTC RECE DATA register


#define INT_RTC_RDY_BIT               0x80

#define POWER_SAVE_SYMBOL            (0x15<<8)
/*end of file*/
