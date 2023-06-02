/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/plat/input.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __PLAT_INPUT_H__
#define __PLAT_INPUT_H__

#define MCU_RTC_BASE            (GK_VA_PMU_RTC + 0x0000)  //0xf2080000
#define MCU_IRR_BASE            (GK_VA_PMU_RTC + 0x2000)
#define MCU_FPC_BASE            (GK_VA_PMU_RTC + 0x4000)
#define MCU_GPIO_BASE           (GK_VA_PMU_RTC + 0x6000)
#define MCU_SYS_BASE            (GK_VA_PMU_RTC + 0xA000)
#define MCU_IRQ_BASE            (GK_VA_PMU_RTC + 0xDC00)

/********************************/
/*     PGPIO Register List      */
/********************************/
#define PMU_GPIO_0_PWRUP        5
#define PMU_GPIO_1_IR           7
#define PMU_GPIO_2              2
#define PMU_GPIO_3              3
#define PMU_GPIO_4              4

#define PMU_GPIO_0_DATA         (MCU_GPIO_BASE + (0x0004 << PMU_GPIO_0_PWRUP))
#define PMU_GPIO_1_DATA         (MCU_GPIO_BASE + (0x0004 << PMU_GPIO_1_IR))
#define PMU_GPIO_2_DATA         (MCU_GPIO_BASE + (0x0004 << PMU_GPIO_2))
#define PMU_GPIO_3_DATA         (MCU_GPIO_BASE + (0x0004 << PMU_GPIO_3))
#define PMU_GPIO_4_DATA         (MCU_GPIO_BASE + (0x0004 << PMU_GPIO_4))
#define PMU_GPIO_DIR            (MCU_GPIO_BASE + 0x0400)
#define PMU_GPIO_IS             (MCU_GPIO_BASE + 0x0404)
#define PMU_GPIO_IBE            (MCU_GPIO_BASE + 0x0408)
#define PMU_GPIO_IEV            (MCU_GPIO_BASE + 0x040C)
#define PMU_GPIO_IE             (MCU_GPIO_BASE + 0x0410)
#define PMU_GPIO_RIS            (MCU_GPIO_BASE + 0x0414)
#define PMU_GPIO_MIS            (MCU_GPIO_BASE + 0x0418)
#define PMU_GPIO_IC             (MCU_GPIO_BASE + 0x041C)
#define PMU_GPIO_AFSEL          (MCU_GPIO_BASE + 0x0420)

#define PMU_AUTOMOTIVE_KEY          0x10    // power on/off by key
#define PMU_AUTOMOTIVE_ACC          0x11    // power on/off by acc
#define PMU_AUTOMOTIVE_KEY_ACC      0x12    // power on/off by key & check the acc status
#define PMU_POWER_OFF_CPU           0x20    // load 51 code then power off, power on by IR
#define PMU_ALWAYS_RUNNING_POWEROFF 0x30    // load 51 code then power off, power on by IR
#define PMU_ALWAYS_RUNNING          0x31    // load 51 code but do not power off, rtc always work
typedef enum
{
    NEG_EDGE = 0,
    POS_EDGE,
    ALL_EDGE,
}GPIO_INT_EDGE_MODE_E;

typedef enum
{
    EDGE_TRIGGER = 0,
    LEVEL_TRIGGER,
}GPIO_INT_TRIGGER_MODE_E;

typedef enum
{
    LOW_LEVEL = 0,
    HIGH_LEVEL,
}GPIO_INT_LEVEL_MODE_E;

/********************************/
/*     IR Register List         */
/********************************/
#define REG_IRR_PROG                (MCU_IRR_BASE + 0x00)   /* write */  //0xf0082000
#define     IRR_RESTART_IRR         (1<<3)
#define     IRR_FALLING_DETECT      (1<<2)
#define     IRR_RISING_DETECT       (1<<1)
#define     IRR_TRANS_0XFF          (1<<0)
#define REG_IRR_PRESCALER_L         (MCU_IRR_BASE + 0x04)   /* write */
#define REG_IRR_PRESCALER_H         (MCU_IRR_BASE + 0x08)   /* write */
#define REG_IRR_IRR_READ            (MCU_IRR_BASE + 0x0C)   /* read/clear */
#define     IRR_READ_VALID          (1<<7)
#define REG_IRR_IRT_READ            (MCU_IRR_BASE + 0x10)   /* read/clear */
#define REG_IRR_IRQMASK_L           (MCU_IRR_BASE + 0x14)   /* write */
#define     IRR_RX_DATA_L_MSK       (0x3<<0)
#define REG_IRR_IRQMASK_H           (MCU_IRR_BASE + 0x18)   /* write */
#define     IRR_RX_DATA_H_MSK       (0x3f<<0)
#define     IRR_RX_DATA_H_SHIFT     (2)

/********************************/
/*     SYS/IRQ Register List    */
/********************************/
#define REG_PMU_SYS_REG_CFG1        (MCU_SYS_BASE + 0x0004) /* read/write */
#define REG_PMU_IRQ_EN              (MCU_IRQ_BASE + 0x0000)
#define     IRQ_EN_RTC      (1<<0)
#define     IRQ_EN_IRR      (1<<1)
#define     IRQ_EN_FPC      (1<<2)
#define     IRQ_EN_GPIO     (1<<3)
#define     IRQ_EN_CEC      (1<<4)
#define     IRQ_EN_ADC      (1<<5)
#define     IRQ_EN_IRT      (1<<6)
#define REG_PMU_IRQ_CLR_RTC         (MCU_IRQ_BASE + 0x0020) /* read/write */
#define REG_PMU_IRQ_CLR_IRR         (MCU_IRQ_BASE + 0x0024) /* read/write */
#define REG_PMU_IRQ_CLR_FPC         (MCU_IRQ_BASE + 0x0028) /* read/write */
#define REG_PMU_IRQ_CLR_GPIO        (MCU_IRQ_BASE + 0x002C) /* read/write */
#define REG_PMU_IRQ_CLR_CEC         (MCU_IRQ_BASE + 0x0030) /* read/write */
#define REG_PMU_IRQ_CLR_ADC         (MCU_IRQ_BASE + 0x0034) /* read/write */
#define REG_PMU_IRQ_CLR_IRT         (MCU_IRQ_BASE + 0x0038) /* read/write */
#define REG_PMU_IRQ_STATUS          (MCU_IRQ_BASE + 0x0040) /* read/write */

/* ==========================================================================*/
#define GKINPUT_TABLE_SIZE      (256)

#define GKINPUT_SOURCE_MASK     (0x0F)
#define GKINPUT_SOURCE_IR       (0x01)
#define GKINPUT_SOURCE_ADC      (0x02)
#define GKINPUT_SOURCE_GPIO     (0x04)
#define GKINPUT_SOURCE_VI       (0x08)

#define GKINPUT_TYPE_MASK       (0xF0)
#define GKINPUT_TYPE_KEY        (0x10)
#define GKINPUT_TYPE_REL        (0x20)
#define GKINPUT_TYPE_ABS        (0x40)
#define GKINPUT_TYPE_SW         (0x80)

#define GKINPUT_IR_KEY          (GKINPUT_SOURCE_IR | GKINPUT_TYPE_KEY)
#define GKINPUT_IR_REL          (GKINPUT_SOURCE_IR | GKINPUT_TYPE_REL)
#define GKINPUT_IR_ABS          (GKINPUT_SOURCE_IR | GKINPUT_TYPE_ABS)
#define GKINPUT_IR_SW           (GKINPUT_SOURCE_IR | GKINPUT_TYPE_SW)
#define GKINPUT_ADC_KEY         (GKINPUT_SOURCE_ADC | GKINPUT_TYPE_KEY)
#define GKINPUT_ADC_REL         (GKINPUT_SOURCE_ADC | GKINPUT_TYPE_REL)
#define GKINPUT_ADC_ABS         (GKINPUT_SOURCE_ADC | GKINPUT_TYPE_ABS)
#define GKINPUT_GPIO_KEY        (GKINPUT_SOURCE_GPIO | GKINPUT_TYPE_KEY)
#define GKINPUT_GPIO_REL        (GKINPUT_SOURCE_GPIO | GKINPUT_TYPE_REL)
#define GKINPUT_GPIO_ABS        (GKINPUT_SOURCE_GPIO | GKINPUT_TYPE_ABS)
#define GKINPUT_GPIO_SW         (GKINPUT_SOURCE_GPIO | GKINPUT_TYPE_SW)
#define GKINPUT_VI_KEY          (GKINPUT_SOURCE_VI | GKINPUT_TYPE_KEY)
#define GKINPUT_VI_REL          (GKINPUT_SOURCE_VI | GKINPUT_TYPE_REL)
#define GKINPUT_VI_ABS          (GKINPUT_SOURCE_VI | GKINPUT_TYPE_ABS)
#define GKINPUT_VI_SW           (GKINPUT_SOURCE_VI | GKINPUT_TYPE_SW)

#define GKINPUT_END             (0xFFFFFFFF)
/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct gk_key_table {
    u32             type;
    union {
    struct {
        u32         key_code;
        u32         key_flag;
        u32         raw_id;
    } ir_key;
    struct {
        u32         key_code;
        s32         rel_step;
        u32         raw_id;
    } ir_rel;
    struct {
        s32         abs_x;
        s32         abs_y;
        u32         raw_id;
    } ir_abs;
    struct {
        u32         key_code;
        u32         key_value;
        u32         raw_id;
    } ir_sw;
    struct {
        u32         key_code;
        u16         irq_trig;// 0 low trigger, 1 high trigger
        u16         chan;
        u16         low_level;
        u16         high_level;
    } adc_key;
    struct {
        u16         key_code;
        s16         rel_step;
        u16         irq_trig;// 0 low trigger, 1 high trigger
        u16         chan;
        u16         low_level;
        u16         high_level;
    } adc_rel;
    struct {
        s16         abs_x;
        s16         abs_y;
        u16         irq_trig;// 0 low trigger, 1 high trigger
        u16         chan;
        u16         low_level;
        u16         high_level;
    } adc_abs;
    struct {
        u32         key_code;
        u32         active_val;
        u16         can_wakeup;
        u8          id;
        u8          irq_mode;
    } gpio_key;
    struct {
        u32         key_code;
        s32         rel_step;
        u16         can_wakeup;
        u8          id;
        u8          irq_mode;
    } gpio_rel;
    struct {
        s32         abs_x;
        s32         abs_y;
        u16         can_wakeup;
        u8          id;
        u8          irq_mode;
    } gpio_abs;
    struct {
        u32         key_code;
        u32         active_val;
        u16         can_wakeup;
        u8          id;
        u8          irq_mode;
    } gpio_sw;
    struct {
        u32         reserve;
        u32         reserve0;
        u32         reserve1;
    } vi_key;
    struct {
        u32         reserve;
        u32         reserve0;
        u32         reserve1;
    } vi_rel;
    struct {
        u32         reserve;
        u32         reserve0;
        u32         reserve1;
    } vi_abs;
    struct {
        u32         reserve;
        u32         reserve0;
        u32         reserve1;
    } vi_sw;
    };
};

/* ==========================================================================*/
struct gk_input_board_info {
    struct gk_key_table *pkeymap;
    struct input_dev        *pinput_dev;
    struct platform_device  *pdev;

    int                     abx_max_x;
    int                     abx_max_y;
    int                     abx_max_pressure;
    int                     abx_max_width;
};

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

