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
#define INT_IRQ_PM_UART          (GIC_SPI_MS_IRQ_START + 4)
#define INT_IRQ_FSP              (GIC_SPI_MS_IRQ_START + 5)
#define INT_IRQ_SAR1             (GIC_SPI_MS_IRQ_START + 6)
#define INT_IRQ_RTC              (GIC_SPI_MS_IRQ_START + 7) // rtc0
#define INT_IRQ_DIG_TOP          (GIC_SPI_MS_IRQ_START + 8)
#define INT_IRQ_PIR_RX           (GIC_SPI_MS_IRQ_START + 9)
#define INT_IRQ_SAR_KP           (GIC_SPI_MS_IRQ_START + 10)
#define INT_IRQ_POWER_NOT_GOOD_0 (GIC_SPI_MS_IRQ_START + 11)
#define INT_IRQ_DUMMY_12         (GIC_SPI_MS_IRQ_START + 12)
#define INT_IRQ_POWER_NOT_GOOD_1 (GIC_SPI_MS_IRQ_START + 13)
#define INT_IRQ_MMU              (GIC_SPI_MS_IRQ_START + 14)
#define INT_IRQ_OTP              (GIC_SPI_MS_IRQ_START + 15)
#define INT_IRQ_IRQ_FROM_PM      (GIC_SPI_MS_IRQ_START + 16)
#define INT_IRQ_CMDQ             (GIC_SPI_MS_IRQ_START + 17)
#define INT_IRQ_SD               (GIC_SPI_MS_IRQ_START + 18)
#define INT_IRQ_FCIE             (GIC_SPI_MS_IRQ_START + 19)
#define INT_IRQ_SC_TOP           (GIC_SPI_MS_IRQ_START + 20)
#define INT_IRQ_MHE_2            (GIC_SPI_MS_IRQ_START + 21)
#define INT_IRQ_PS               (GIC_SPI_MS_IRQ_START + 22)
#define INT_IRQ_WADR_ERROR       (GIC_SPI_MS_IRQ_START + 23)
#define INT_IRQ_USB3_GP2TOP      (GIC_SPI_MS_IRQ_START + 24)
#define INT_IRQ_ISP_0            (GIC_SPI_MS_IRQ_START + 25)
#define INT_IRQ_MHE_1            (GIC_SPI_MS_IRQ_START + 26)
#define INT_IRQ_HEMCU            (GIC_SPI_MS_IRQ_START + 27)
#define INT_IRQ_DUMMY_28         (GIC_SPI_MS_IRQ_START + 28)
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
#define INT_IRQ_DUMMY_39         (GIC_SPI_MS_IRQ_START + 39)
#define INT_IRQ_BDMA_1           (GIC_SPI_MS_IRQ_START + 40) // bdma in interrupt table start with 1
#define INT_IRQ_BDMA_2           (GIC_SPI_MS_IRQ_START + 41)
#define INT_IRQ_BACH             (GIC_SPI_MS_IRQ_START + 42) // AU_SYSTEM_INT
#define INT_IRQ_LDC_FEYE_0       (GIC_SPI_MS_IRQ_START + 43)
#define INT_IRQ_JPD_0            (GIC_SPI_MS_IRQ_START + 44)
#define INT_IRQ_JPE_1            (GIC_SPI_MS_IRQ_START + 45)
#define INT_IRQ_IMI              (GIC_SPI_MS_IRQ_START + 46)
#define INT_IRQ_FUART            (GIC_SPI_MS_IRQ_START + 47)
#define INT_IRQ_URDMA            (GIC_SPI_MS_IRQ_START + 48)
#define INT_IRQ_MIU              (GIC_SPI_MS_IRQ_START + 49)
#define INT_IRQ_DUMMY_50         (GIC_SPI_MS_IRQ_START + 50)
#define INT_IRQ_RIU_ERROR_RESP   (GIC_SPI_MS_IRQ_START + 51)
#define INT_IRQ_PWM              (GIC_SPI_MS_IRQ_START + 52)
#define INT_IRQ_DLA_TOP_0        (GIC_SPI_MS_IRQ_START + 53) // ipu2top_irq0
#define INT_IRQ_JPD_1            (GIC_SPI_MS_IRQ_START + 54)
#define INT_IRQ_IVE              (GIC_SPI_MS_IRQ_START + 55)
#define INT_IRQ_GPI_OUT          (GIC_SPI_MS_IRQ_START + 56)
#define INT_IRQ_ISP0_VIF         (GIC_SPI_MS_IRQ_START + 57) // isq0_vif_int
#define INT_IRQ_SC1_TOP_INT      (GIC_SPI_MS_IRQ_START + 58)
#define INT_IRQ_SC2_TOP_INT      (GIC_SPI_MS_IRQ_START + 59)
#define INT_IRQ_DUMMY_60         (GIC_SPI_MS_IRQ_START + 60)
#define INT_IRQ_BDMA_3           (GIC_SPI_MS_IRQ_START + 61)
#define INT_IRQ_DUMMY_62         (GIC_SPI_MS_IRQ_START + 62)
#define INT_IRQ_DUMMY_63         (GIC_SPI_MS_IRQ_START + 63)
#define INT_IRQ_USB_INT_P1       (GIC_SPI_MS_IRQ_START + 64)
#define INT_IRQ_UHC_INT_P1       (GIC_SPI_MS_IRQ_START + 65)
#define INT_IRQ_MHE_3            (GIC_SPI_MS_IRQ_START + 66)
#define INT_IRQ_DISP0            (GIC_SPI_MS_IRQ_START + 67) // mop0 3
#define INT_IRQ_DISP1            (GIC_SPI_MS_IRQ_START + 68) // disp_top0
#define INT_IRQ_DISP2            (GIC_SPI_MS_IRQ_START + 69) // gop_top0
#define INT_IRQ_DISP3            (GIC_SPI_MS_IRQ_START + 70) // hdmi_tx
#define INT_IRQ_DISP4            (GIC_SPI_MS_IRQ_START + 71) // mop1
#define INT_IRQ_DISP5            (GIC_SPI_MS_IRQ_START + 72) // disp_top1
#define INT_IRQ_DISP6            (GIC_SPI_MS_IRQ_START + 73) // gop_top1
#define INT_IRQ_DISP7            (GIC_SPI_MS_IRQ_START + 74) // dac
#define INT_IRQ_DISP8            (GIC_SPI_MS_IRQ_START + 75) // dp
#define INT_IRQ_DISP9            (GIC_SPI_MS_IRQ_START + 76) // lcd  12
#define INT_IRQ_DUMMY_77         (GIC_SPI_MS_IRQ_START + 77)
#define INT_IRQ_MIU_0            (GIC_SPI_MS_IRQ_START + 78)
#define INT_IRQ_DUMMY_79         (GIC_SPI_MS_IRQ_START + 79)
#define INT_IRQ_FUART1_EMPTY     (GIC_SPI_MS_IRQ_START + 80)
#define INT_IRQ_ISP0_DMA         (GIC_SPI_MS_IRQ_START + 81) // isp0_dma_int
#define INT_IRQ_FUART_1          (GIC_SPI_MS_IRQ_START + 82)
#define INT_IRQ_UARTDMA_1        (GIC_SPI_MS_IRQ_START + 83)
#define INT_IRQ_DUMMY_84         (GIC_SPI_MS_IRQ_START + 84)
#define INT_IRQ_DUMMY_85         (GIC_SPI_MS_IRQ_START + 85)
#define INT_IRQ_DUMMY_86         (GIC_SPI_MS_IRQ_START + 86)
#define INT_IRQ_CMDQ_1           (GIC_SPI_MS_IRQ_START + 87) // cmdq1
#define INT_IRQ_GE               (GIC_SPI_MS_IRQ_START + 88)
#define INT_IRQ_DUMMY_89         (GIC_SPI_MS_IRQ_START + 89)
#define INT_IRQ_DUMMY_90         (GIC_SPI_MS_IRQ_START + 90)
#define INT_IRQ_DUMMY_91         (GIC_SPI_MS_IRQ_START + 91)
#define INT_IRQ_DUMMY_92         (GIC_SPI_MS_IRQ_START + 92)
#define INT_IRQ_DUMMY_93         (GIC_SPI_MS_IRQ_START + 93)
#define INT_IRQ_DUMMY_94         (GIC_SPI_MS_IRQ_START + 94)
#define INT_IRQ_DUMMY_95         (GIC_SPI_MS_IRQ_START + 95)
#define INT_IRQ_FUART_2          (GIC_SPI_MS_IRQ_START + 96)
#define INT_IRQ_URDMA_2          (GIC_SPI_MS_IRQ_START + 97)
#define INT_IRQ_FUART2_EMPTY     (GIC_SPI_MS_IRQ_START + 98)
#define INT_IRQ_DUMMY_99         (GIC_SPI_MS_IRQ_START + 99)
#define INT_IRQ_DUMMY_100        (GIC_SPI_MS_IRQ_START + 100)
#define INT_IRQ_DUMMY_101        (GIC_SPI_MS_IRQ_START + 101)
#define INT_IRQ_DISP10           (GIC_SPI_MS_IRQ_START + 102)
#define INT_IRQ_DUMMY_103        (GIC_SPI_MS_IRQ_START + 103)
#define INT_IRQ_MIU_CH_0         (GIC_SPI_MS_IRQ_START + 104)
#define INT_IRQ_DUMMY_105        (GIC_SPI_MS_IRQ_START + 105)
#define INT_IRQ_DUMMY_106        (GIC_SPI_MS_IRQ_START + 106)
#define INT_IRQ_DUMMY_107        (GIC_SPI_MS_IRQ_START + 107)
#define INT_IRQ_DUMMY_108        (GIC_SPI_MS_IRQ_START + 108)
#define INT_IRQ_DUMMY_109        (GIC_SPI_MS_IRQ_START + 109)
#define INT_IRQ_CMDQ_NET         (GIC_SPI_MS_IRQ_START + 110)
#define INT_IRQ_CMDQ_DIG         (GIC_SPI_MS_IRQ_START + 111)
#define INT_IRQ_CMDQ_CMD         (GIC_SPI_MS_IRQ_START + 112)
#define INT_IRQ_CMDQ_CMD1        (GIC_SPI_MS_IRQ_START + 113)
#define INT_IRQ_CMDQ_ISP_0       (GIC_SPI_MS_IRQ_START + 114)
#define INT_IRQ_DUMMY_115        (GIC_SPI_MS_IRQ_START + 115)
#define INT_IRQ_DUMMY_116        (GIC_SPI_MS_IRQ_START + 116)
#define INT_IRQ_SATA1_GP2TOP     (GIC_SPI_MS_IRQ_START + 118)
#define INT_IRQ_SATA2_GP2TOP     (GIC_SPI_MS_IRQ_START + 117)
#define INT_IRQ_DUMMY_119        (GIC_SPI_MS_IRQ_START + 119)
#define INT_IRQ_NET_GP_BDMA_0    (GIC_SPI_MS_IRQ_START + 120)
#define INT_IRQ_NET_GP_BDMA_1    (GIC_SPI_MS_IRQ_START + 121)
#define INT_IRQ_NET_GP_BDMA_2    (GIC_SPI_MS_IRQ_START + 122)
#define INT_IRQ_NET_GP_BDMA_3    (GIC_SPI_MS_IRQ_START + 123)
#define INT_IRQ_NET_GP_TOP       (GIC_SPI_MS_IRQ_START + 124)
#define INT_IRQ_DUMMY_125        (GIC_SPI_MS_IRQ_START + 125)
#define INT_IRQ_DUMMY_126        (GIC_SPI_MS_IRQ_START + 126)
#define INT_IRQ_DUMMY_127        (GIC_SPI_MS_IRQ_START + 127)
#define GIC_SPI_MS_IRQ_END       (GIC_SPI_MS_IRQ_START + 128)
#define GIC_SPI_MS_IRQ_NR        (GIC_SPI_MS_IRQ_END - GIC_SPI_MS_IRQ_START)
#define INT_IRQ_BANK2_BASE       (INT_IRQ_USB_INT_P1 - GIC_SPI_MS_IRQ_START)

/* MS_NON_PM_FIQ 128-159 */
#define GIC_SPI_MS_FIQ_START     GIC_SPI_MS_IRQ_END
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
