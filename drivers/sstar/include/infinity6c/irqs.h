/*
 * irqs.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

#ifndef __IRQS_H
#define __IRQS_H

#include "gpi-irqs.h"
#include "pmsleep-irqs.h"

/* [GIC irqchip]
        ID   0 -  15 : SGI
        ID  16 -  31 : PPI
        ID  32 -  63 : SPI:ARM_INTERNAL
        ID  64 - 127 : SPI:MS_IRQ (GIC_HWIRQ_MS_START)
        ID 128 - 159 : SPI:MS_FIQ
   [PMSLEEP irqchip]
        ID   0 -  31 : MS_PM_IRQ    */

#define GIC_SGI_NR              16
#define GIC_PPI_NR              16
#define GIC_SPI_ARM_INTERNAL_NR 32
#define GIC_HWIRQ_MS_START      (GIC_SGI_NR + GIC_PPI_NR + GIC_SPI_ARM_INTERNAL_NR)

/*  The folloing list are used in dtsi and get number by of_irq,
if need to get the interrupt number for request_irq(), manual calculate the number is
GIC_SGI_NR+GIC_PPI_NR+X=32+X        */

// NOTE(Spade): We count from GIC_SPI_ARM_INTERNAL because interrupt delcaration in dts is from SPI 0
/* MS_NON_PM_IRQ 32-127 */
#define GIC_SPI_MS_IRQ_START     GIC_SPI_ARM_INTERNAL_NR
#define INT_IRQ_NONPM_TO_MCU51   (GIC_SPI_MS_IRQ_START + 0)
#define INT_IRQ_FIQ_FROM_PM      (GIC_SPI_MS_IRQ_START + 1)
#define INT_IRQ_PM_SLEEP         (GIC_SPI_MS_IRQ_START + 2)
#define INT_IRQ_SAR_GPIO_WK      (GIC_SPI_MS_IRQ_START + 3)
#define INT_IRQ_MIU              (GIC_SPI_MS_IRQ_START + 4)
#define INT_IRQ_FSP              (GIC_SPI_MS_IRQ_START + 5)
#define INT_IRQ_SAR1             (GIC_SPI_MS_IRQ_START + 6)
#define INT_IRQ_RTC              (GIC_SPI_MS_IRQ_START + 7) // rtc0
#define INT_IRQ_DIG_TOP          (GIC_SPI_MS_IRQ_START + 8)
#define INT_IRQ_XTAL_TOP         (GIC_SPI_MS_IRQ_START + 9)
#define INT_IRQ_SAR_KP           (GIC_SPI_MS_IRQ_START + 10)
#define INT_IRQ_POWER_NOT_GOOD_0 (GIC_SPI_MS_IRQ_START + 11)
#define INT_IRQ_POC_ATOP         (GIC_SPI_MS_IRQ_START + 12) // poc_atop_wrapper
#define INT_IRQ_POWER_NOT_GOOD_1 (GIC_SPI_MS_IRQ_START + 13)
#define INT_IRQ_MMU              (GIC_SPI_MS_IRQ_START + 14)
#define INT_IRQ_OTP              (GIC_SPI_MS_IRQ_START + 15)
#define INT_IRQ_DUMMY_16         (GIC_SPI_MS_IRQ_START + 16) // DUMMY_16
#define INT_IRQ_CMDQ             (GIC_SPI_MS_IRQ_START + 17)
#define INT_IRQ_SD               (GIC_SPI_MS_IRQ_START + 18)
#define INT_IRQ_EMAC             (GIC_SPI_MS_IRQ_START + 19)
#define INT_IRQ_SC_TOP           (GIC_SPI_MS_IRQ_START + 20)
#define INT_IRQ_FUART_EMPTY      (GIC_SPI_MS_IRQ_START + 21)
#define INT_IRQ_PS               (GIC_SPI_MS_IRQ_START + 22)
#define INT_IRQ_WADR_ERROR       (GIC_SPI_MS_IRQ_START + 23)
#define INT_IRQ_ISP0_DMA         (GIC_SPI_MS_IRQ_START + 24) // ISP_0_DMA
#define INT_IRQ_ISP_0            (GIC_SPI_MS_IRQ_START + 25)
#define INT_IRQ_MHE_1            (GIC_SPI_MS_IRQ_START + 26)
#define INT_IRQ_HEMCU            (GIC_SPI_MS_IRQ_START + 27)
#define INT_IRQ_FUART1_28        (GIC_SPI_MS_IRQ_START + 28) // fuart1_int
#define INT_IRQ_JPE              (GIC_SPI_MS_IRQ_START + 29)
#define INT_IRQ_USB              (GIC_SPI_MS_IRQ_START + 30)
#define INT_IRQ_UHC              (GIC_SPI_MS_IRQ_START + 31)
#define INT_IRQ_OTG              (GIC_SPI_MS_IRQ_START + 32)
#define INT_IRQ_MIPI_CSI2        (GIC_SPI_MS_IRQ_START + 33)
#define INT_IRQ_FUART_0          (GIC_SPI_MS_IRQ_START + 34)
#define INT_IRQ_URDMA_0          (GIC_SPI_MS_IRQ_START + 35)
#define INT_IRQ_MIIC_0           (GIC_SPI_MS_IRQ_START + 36)
#define INT_IRQ_FUART0_EMPTY     (GIC_SPI_MS_IRQ_START + 37)
#define INT_IRQ_MSPI_0           (GIC_SPI_MS_IRQ_START + 38)
#define INT_IRQ_MSPI_1           (GIC_SPI_MS_IRQ_START + 39)
#define INT_IRQ_BDMA_1           (GIC_SPI_MS_IRQ_START + 40) // bdma in interrupt table start with 1
#define INT_IRQ_BDMA_2           (GIC_SPI_MS_IRQ_START + 41)
#define INT_IRQ_BACH             (GIC_SPI_MS_IRQ_START + 42) // AU_SYSTEM_INT
#define INT_IRQ_URDMA1_0         (GIC_SPI_MS_IRQ_START + 43) // URDMA1_int
#define INT_IRQ_EMAC_SCATTER     (GIC_SPI_MS_IRQ_START + 44) // emac_scatter_dma_irq
#define INT_IRQ_CMDQ_1           (GIC_SPI_MS_IRQ_START + 45) // CMDQ1
#define INT_IRQ_IMI              (GIC_SPI_MS_IRQ_START + 46)
#define INT_IRQ_FUART            (GIC_SPI_MS_IRQ_START + 47)
#define INT_IRQ_URDMA            (GIC_SPI_MS_IRQ_START + 48)
#define INT_IRQ_MIU0             (GIC_SPI_MS_IRQ_START + 49)
#define INT_IRQ_CMDQ_ISP_0       (GIC_SPI_MS_IRQ_START + 50) // SC_isp0_cmdq_int
#define INT_IRQ_RIU_ERROR_RESP   (GIC_SPI_MS_IRQ_START + 51)
#define INT_IRQ_PWM              (GIC_SPI_MS_IRQ_START + 52)
#define INT_IRQ_DLA_TOP_0        (GIC_SPI_MS_IRQ_START + 53) // ipu2top_irq0
#define INT_IRQ_FCIE_TOP         (GIC_SPI_MS_IRQ_START + 54)
#define INT_IRQ_IVE              (GIC_SPI_MS_IRQ_START + 55)
#define INT_IRQ_GPI_OUT          (GIC_SPI_MS_IRQ_START + 56)
#define INT_IRQ_ISP0_VIF         (GIC_SPI_MS_IRQ_START + 57) // isq0_vif_int
#define INT_IRQ_SC1_TOP_INT      (GIC_SPI_MS_IRQ_START + 58)
#define INT_IRQ_SC2_TOP_INT      (GIC_SPI_MS_IRQ_START + 59)
#define INT_IRQ_FUART_EMPTY_INT  (GIC_SPI_MS_IRQ_START + 60)
#define INT_IRQ_SC3_TOP_INT      (GIC_SPI_MS_IRQ_START + 61)
#define INT_IRQ_SC4_TOP_INT      (GIC_SPI_MS_IRQ_START + 62)
#define INT_IRQ_LDC_FEYE_0       (GIC_SPI_MS_IRQ_START + 63)
#define GIC_SPI_MS_IRQ_END       (GIC_SPI_MS_IRQ_START + 128)
#define GIC_SPI_MS_IRQ_NR        (GIC_SPI_MS_IRQ_END - GIC_SPI_MS_IRQ_START)
#define GIC_SPI_MS_IRQ_STRNR     (GIC_SPI_MS_IRQ_END - 64 - GIC_SPI_MS_IRQ_START)

/* MS_NON_PM_FIQ 128-159 */
#define GIC_SPI_MS_FIQ_START     (GIC_SPI_MS_IRQ_END)
#define INT_FIQ_TIMER_0          (GIC_SPI_MS_FIQ_START + 0)
#define INT_FIQ_TIMER_1          (GIC_SPI_MS_FIQ_START + 1)
#define INT_FIQ_WDT              (GIC_SPI_MS_FIQ_START + 2)
#define INT_FIQ_TIMER_2          (GIC_SPI_MS_FIQ_START + 3)
#define INT_FIQ_IR_RC            (GIC_SPI_MS_FIQ_START + 4)
#define INT_FIQ_CPU0TO2_TOP      (GIC_SPI_MS_FIQ_START + 5)
#define INT_FIQ_PM_XIU_TIMEOUT   (GIC_SPI_MS_FIQ_START + 6)
#define INT_FIQ_SAR_GPIO_3       (GIC_SPI_MS_FIQ_START + 7)
#define INT_FIQ_SAR_GPIO_2       (GIC_SPI_MS_FIQ_START + 8)
#define INT_FIQ_SAR_GPIO_1       (GIC_SPI_MS_FIQ_START + 9)
#define INT_FIQ_SAR_GPIO_0       (GIC_SPI_MS_FIQ_START + 10)
#define INT_FIQ_IR               (GIC_SPI_MS_FIQ_START + 11)
#define INT_FIQ_HST_3_1          (GIC_SPI_MS_FIQ_START + 12)
#define INT_FIQ_HST_2_1          (GIC_SPI_MS_FIQ_START + 13)
#define INT_FIQ_HST_1_3          (GIC_SPI_MS_FIQ_START + 14)
#define INT_FIQ_HST_1_2          (GIC_SPI_MS_FIQ_START + 15)
#define INT_FIQ_FIQ_FROM_PM      (GIC_SPI_MS_FIQ_START + 16)
#define INT_FIQ_HST_0_2          (GIC_SPI_MS_FIQ_START + 17)
#define INT_FIQ_HST_2_0          (GIC_SPI_MS_FIQ_START + 18)
#define INT_FIQ_DUMMY_19         (GIC_SPI_MS_FIQ_START + 19)
#define INT_FIQ_CMD_XIU_TIMEOUT  (GIC_SPI_MS_FIQ_START + 20)
#define INT_FIQ_LAN_ESD          (GIC_SPI_MS_FIQ_START + 21)
#define INT_FIQ_XIU_TIMEOUT      (GIC_SPI_MS_FIQ_START + 22)
#define INT_FIQ_SD_CDZ_0         (GIC_SPI_MS_FIQ_START + 23)
#define INT_FIQ_SD_CDZ_1         (GIC_SPI_MS_FIQ_START + 24)
#define INT_FIQ_POWER_NOT_GOOD_0 (GIC_SPI_MS_FIQ_START + 25)
#define INT_FIQ_PM_ERROR_RESP    (GIC_SPI_MS_FIQ_START + 26)
#define INT_FIQ_DUMMY_27         (GIC_SPI_MS_FIQ_START + 27)
#define INT_FIQ_DUMMY_28         (GIC_SPI_MS_FIQ_START + 28)
#define INT_FIQ_DUMMY_29         (GIC_SPI_MS_FIQ_START + 29)
#define INT_FIQ_DUMMY_30         (GIC_SPI_MS_FIQ_START + 30)
#define INT_FIQ_DUMMY_31         (GIC_SPI_MS_FIQ_START + 31)
#define GIC_SPI_MS_FIQ_END       (GIC_SPI_MS_FIQ_START + 32)
#define GIC_SPI_MS_FIQ_NR        (GIC_SPI_MS_FIQ_END - GIC_SPI_MS_FIQ_START)

#endif // __ARCH_ARM_ASM_IRQS_H
