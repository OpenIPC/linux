/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/mach/irqs.h
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

#ifndef __MACH_IRQS_H
#define __MACH_IRQS_H

#include <mach/hardware.h>
#define NR_IRQS                     128

#define NR_VIC_IRQ_SIZE             (32)

#define VIC1_INT_VEC(x)             ((x) + (NR_VIC_IRQ_SIZE * 0))
#define VIC2_INT_VEC(x)             ((x) + (NR_VIC_IRQ_SIZE * 1))
//#define VIC3_INT_VEC(x)           ((x) + (NR_VIC_IRQ_SIZE * 2))
//#define VIC4_INT_VEC(x)           ((x) + (NR_VIC_IRQ_SIZE * 3))

/* The following are ARCH specific IRQ numbers */

#define VIC_INSTANCES           2

#define NR_VIC_IRQS             (VIC_INSTANCES * NR_VIC_IRQ_SIZE)
#define GPIO_INT_VEC(x)         (NR_VIC_IRQS + x)

// VIC Instance 1
#define SSI_SLAVE_IRQ            0
#define ETH_IRQ                  1
#define ICORE_ERROR_IRQ          2
#define GPIO2_IRQ                4
#define CD2ND_BIT_CD_IRQ         5
#define USBVBUS_IRQ              6
#define VO_IRQ                   7
#define DSP_VO1_IRQ              7
#define SD_IRQ                   8
#define IDC_IRQ                  9
#define SSI_IRQ                 10
#define WDT_IRQ                 11
#define IRIF_IRQ                12
#define CFCD1_IRQ               13
#define SDCD_IRQ                14  /* SD0 SD card card detection. */
#define UART1_IRQ               15
#define GPIO0_IRQ               16
#define GPIO1_IRQ               17
#define TIMER1_IRQ              18
#define TIMER2_IRQ              19
#define TIMER3_IRQ              20
#define DMA_IRQ                 21
#define FIOCMD_IRQ              22
#define FIODMA_IRQ              23
#define VI_IRQ                  24
#define DSP_VI_IRQ              24
#define VCORE_IRQ               25
#define DSP_VCORE_IRQ           25
#define USBC_IRQ                26
#define UART2_IRQ               27
#define HIF2_IRQ                28
#define I2STX_IRQ               29
#define I2SRX_IRQ               30
#define UART0_IRQ               31

// VIC Instance 2
#define DMA_FIOS_IRQ            (32 + 7)
#define VO0_TV_SYNC_IRQ         (32 + 16)
#define VO1_LCD_SYNC_IRQ        (32 + 17)
#define DSP_VO0_IRQ             (32 + 18)
#define AES_IRQ                 (32 + 19)
#define DES_IRQ                 (32 + 20)
#define MDMA_IRQ                (32 + 22)
#define MOTOR_IRQ               (32 + 23)
#define ADC_LEVEL_IRQ           (32 + 24)
#define IDC_HDMI_IRQ            (32 + 26)
#define ICORE_LAST_PIXEL_IRQ    (32 + 27)
#define ICORE_VSYNC_IRQ         (32 + 28)
#define ICORE_SENSOR_VSYNC_IRQ  (32 + 29)
#define HDMI_IRQ                (32 + 30)
#define SSI2_IRQ                (32 + 31)

#define MAX_IRQ_NUM             (SSI2_IRQ + 1)
#define IR_IRQ                  HDMI_IRQ


//#define MAX_IRQ_NUM             (MOTOR_IRQ + 1)

#define GPIO_IRQ(x)             ((x) + NR_VIC_IRQS)


#define VIC_IRQ(x)              (x)


/*
 * VIC trigger types (SW definition).
 */
#define VIRQ_RISING_EDGE    0
#define VIRQ_FALLING_EDGE   1
#define VIRQ_BOTH_EDGES     2
#define VIRQ_LEVEL_LOW      3
#define VIRQ_LEVEL_HIGH     4

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define VIC_IRQ_STA_OFFSET      0x30
#define VIC_FIQ_STA_OFFSET      0x34
#define VIC_RAW_STA_OFFSET      0x18
#define VIC_INT_SEL_OFFSET      0x0c
#define VIC_INTEN_OFFSET        0x10
#define VIC_INTEN_CLR_OFFSET    0x14
#define VIC_SOFTEN_OFFSET       0x1c
#define VIC_SOFTEN_CLR_OFFSET   0x20
#define VIC_PROTEN_OFFSET       0x24
#define VIC_SENSE_OFFSET        0x00
#define VIC_BOTHEDGE_OFFSET     0x08
#define VIC_EVENT_OFFSET        0x04
#define VIC_EDGE_CLR_OFFSET     0x38

#define VIC1_BASE               (GK_VA_VIC1)
#define VIC2_BASE               (GK_VA_VIC2)

#define VIC1_REG(x)             (VIC1_BASE + (x))
#define VIC2_REG(x)             (VIC2_BASE + (x))

#define VIC_IRQ_STA_REG         VIC1_REG(VIC_IRQ_STA_OFFSET)
#define VIC_FIQ_STA_REG         VIC1_REG(VIC_FIQ_STA_OFFSET)
#define VIC_RAW_STA_REG         VIC1_REG(VIC_RAW_STA_OFFSET)
#define VIC_INT_SEL_REG         VIC1_REG(VIC_INT_SEL_OFFSET)
#define VIC_INTEN_REG           VIC1_REG(VIC_INTEN_OFFSET)
#define VIC_INTEN_CLR_REG       VIC1_REG(VIC_INTEN_CLR_OFFSET)
#define VIC_SOFTEN_REG          VIC1_REG(VIC_SOFTEN_OFFSET)
#define VIC_SOFTEN_CLR_REG      VIC1_REG(VIC_SOFTEN_CLR_OFFSET)
#define VIC_PROTEN_REG          VIC1_REG(VIC_PROTEN_OFFSET)
#define VIC_SENSE_REG           VIC1_REG(VIC_SENSE_OFFSET)
#define VIC_BOTHEDGE_REG        VIC1_REG(VIC_BOTHEDGE_OFFSET)
#define VIC_EVENT_REG           VIC1_REG(VIC_EVENT_OFFSET)
#define VIC_EDGE_CLR_REG        VIC1_REG(VIC_EDGE_CLR_OFFSET)

#if (VIC_INSTANCES >= 2)

#define VIC2_IRQ_STA_REG        VIC2_REG(VIC_IRQ_STA_OFFSET)
#define VIC2_FIQ_STA_REG        VIC2_REG(VIC_FIQ_STA_OFFSET)
#define VIC2_RAW_STA_REG        VIC2_REG(VIC_RAW_STA_OFFSET)
#define VIC2_INT_SEL_REG        VIC2_REG(VIC_INT_SEL_OFFSET)
#define VIC2_INTEN_REG          VIC2_REG(VIC_INTEN_OFFSET)
#define VIC2_INTEN_CLR_REG      VIC2_REG(VIC_INTEN_CLR_OFFSET)
#define VIC2_SOFTEN_REG         VIC2_REG(VIC_SOFTEN_OFFSET)
#define VIC2_SOFTEN_CLR_REG     VIC2_REG(VIC_SOFTEN_CLR_OFFSET)
#define VIC2_PROTEN_REG         VIC2_REG(VIC_PROTEN_OFFSET)
#define VIC2_SENSE_REG          VIC2_REG(VIC_SENSE_OFFSET)
#define VIC2_BOTHEDGE_REG       VIC2_REG(VIC_BOTHEDGE_OFFSET)
#define VIC2_EVENT_REG          VIC2_REG(VIC_EVENT_OFFSET)
#define VIC2_EDGE_CLR_REG       VIC2_REG(VIC_EDGE_CLR_OFFSET)

#endif

struct gk_irq_info
{
    int irq_gpio;
    int irq_line;
    int irq_type;
    int irq_gpio_val;
    int irq_gpio_mode;
};

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************
#ifndef __ASSEMBLY__

#ifdef __cplusplus
extern "C" {
#endif

struct pt_regs;

extern void gk_init_irq(void) __init;
extern  void gk_vic_handle_irq(struct pt_regs *regs);

#ifdef __cplusplus
}
#endif
#endif

#endif  /* __MACH_IRQS_H */

