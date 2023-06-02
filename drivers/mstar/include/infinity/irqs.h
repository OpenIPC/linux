/*------------------------------------------------------------------------------
    Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/

#ifndef __IRQS_H
#define __IRQS_H

/* [GIC]
        ID   0 -  15 : SGI
        ID  16 -  31 : PPI
        ID  32 -  63 : ARM_INTERNAL
        ID  64 - 127 : MS_IRQ (GIC_HWIRQ_MS_START)
        ID 128 - 159 : MS_FIQ
   [PMSLEEP]
        ID   0 -  31 : MS_PM_IRQ
*/

#define GIC_SGI_NR                 16
#define GIC_PPI_NR                 16
#define GIC_SPI_ARM_INTERNAL_NR    32
#define GIC_HWIRQ_MS_START        (GIC_SGI_NR + GIC_PPI_NR + GIC_SPI_ARM_INTERNAL_NR)

//NOTE(Spade): We count from GIC_SPI_ARM_INTERNAL because interrupt delcaration in dts is from SPI 0
/* MS_NON_PM_IRQ 32-95 */
#define GIC_SPI_MS_IRQ_START       GIC_SPI_ARM_INTERNAL_NR
#define INT_IRQ_NONPM_TO_MCU51    (GIC_SPI_MS_IRQ_START +  0)
#define INT_IRQ_FIQ_FROM_PM       (GIC_SPI_MS_IRQ_START +  1)
#define INT_IRQ_PM_SLEEP          (GIC_SPI_MS_IRQ_START +  2)
#define INT_IRQ_DUMMY_03          (GIC_SPI_MS_IRQ_START +  3)
#define INT_IRQ_DUMMY_04          (GIC_SPI_MS_IRQ_START +  4)
#define INT_IRQ_FSP               (GIC_SPI_MS_IRQ_START +  5)
#define INT_IRQ_DUMMY_06          (GIC_SPI_MS_IRQ_START +  6)
#define INT_IRQ_POWER_0_NG        (GIC_SPI_MS_IRQ_START +  7)
#define INT_IRQ_POWER_1_NG        (GIC_SPI_MS_IRQ_START +  8)
#define INT_IRQ_DUMMY_09          (GIC_SPI_MS_IRQ_START +  9)
#define INT_IRQ_DUMMY_10          (GIC_SPI_MS_IRQ_START + 10)
#define INT_IRQ_DUMMY_11          (GIC_SPI_MS_IRQ_START + 11)
#define INT_IRQ_PM_ERROR_RESP     (GIC_SPI_MS_IRQ_START + 12)
#define INT_IRQ_WAKE_ON_LAN       (GIC_SPI_MS_IRQ_START + 13)
#define INT_IRQ_DUMMY_14          (GIC_SPI_MS_IRQ_START + 14)
#define INT_IRQ_DUMMY_15          (GIC_SPI_MS_IRQ_START + 15)
#define INT_IRQ_IRQ_FROM_PM       (GIC_SPI_MS_IRQ_START + 16)
#define INT_IRQ_CMDQ              (GIC_SPI_MS_IRQ_START + 17)
#define INT_IRQ_FCIE              (GIC_SPI_MS_IRQ_START + 18)
#define INT_IRQ_SDIO              (GIC_SPI_MS_IRQ_START + 19)
#define INT_IRQ_SC_TOP            (GIC_SPI_MS_IRQ_START + 20)
#define INT_IRQ_VHE               (GIC_SPI_MS_IRQ_START + 21)
#define INT_IRQ_PS                (GIC_SPI_MS_IRQ_START + 22) //?
#define INT_IRQ_WADR_ERROR        (GIC_SPI_MS_IRQ_START + 23) //?
#define INT_IRQ_PM                (GIC_SPI_MS_IRQ_START + 24)
#define INT_IRQ_ISP               (GIC_SPI_MS_IRQ_START + 25)
#define INT_IRQ_EMAC              (GIC_SPI_MS_IRQ_START + 26)
#define INT_IRQ_HEMCU             (GIC_SPI_MS_IRQ_START + 27) //?
#define INT_IRQ_MFE               (GIC_SPI_MS_IRQ_START + 28)
#define INT_IRQ_JPE               (GIC_SPI_MS_IRQ_START + 29)
#define INT_IRQ_USB               (GIC_SPI_MS_IRQ_START + 30)
#define INT_IRQ_UHC               (GIC_SPI_MS_IRQ_START + 31)
#define INT_IRQ_OTG               (GIC_SPI_MS_IRQ_START + 32)
#define INT_IRQ_MIPI_CSI2         (GIC_SPI_MS_IRQ_START + 33)
#define INT_IRQ_UART_0            (GIC_SPI_MS_IRQ_START + 34)
#define INT_IRQ_UART_1            (GIC_SPI_MS_IRQ_START + 35)
#define INT_IRQ_MIIC_0            (GIC_SPI_MS_IRQ_START + 36)
#define INT_IRQ_MIIC_1            (GIC_SPI_MS_IRQ_START + 37)
#define INT_IRQ_MSPI_0            (GIC_SPI_MS_IRQ_START + 38)
#define INT_IRQ_MSPI_1            (GIC_SPI_MS_IRQ_START + 39)
#define INT_IRQ_BDMA_0            (GIC_SPI_MS_IRQ_START + 40)
#define INT_IRQ_BDMA_1            (GIC_SPI_MS_IRQ_START + 41)
#define INT_IRQ_BACH              (GIC_SPI_MS_IRQ_START + 42)
#define INT_IRQ_KEYPAD            (GIC_SPI_MS_IRQ_START + 43)
#define INT_IRQ_RTC               (GIC_SPI_MS_IRQ_START + 44)
#define INT_IRQ_SAR               (GIC_SPI_MS_IRQ_START + 45)
#define INT_IRQ_IMI               (GIC_SPI_MS_IRQ_START + 46)
#define INT_IRQ_FUART             (GIC_SPI_MS_IRQ_START + 47)
#define INT_IRQ_URDMA             (GIC_SPI_MS_IRQ_START + 48)
#define INT_IRQ_MIU               (GIC_SPI_MS_IRQ_START + 49)
#define INT_IRQ_GOP               (GIC_SPI_MS_IRQ_START + 50)
#define INT_IRQ_RIU_ERROR_RESP    (GIC_SPI_MS_IRQ_START + 51)
#define INT_IRQ_DUMMY_52          (GIC_SPI_MS_IRQ_START + 52)
#define INT_IRQ_DUMMY_53          (GIC_SPI_MS_IRQ_START + 53)
#define INT_IRQ_DUMMY_54          (GIC_SPI_MS_IRQ_START + 54)
#define INT_IRQ_DUMMY_55          (GIC_SPI_MS_IRQ_START + 55)
#define INT_IRQ_DUMMY_56          (GIC_SPI_MS_IRQ_START + 56)
#define INT_IRQ_DUMMY_57          (GIC_SPI_MS_IRQ_START + 57)
#define INT_IRQ_DUMMY_58          (GIC_SPI_MS_IRQ_START + 58)
#define INT_IRQ_DUMMY_59          (GIC_SPI_MS_IRQ_START + 59)
#define INT_IRQ_DUMMY_60          (GIC_SPI_MS_IRQ_START + 60)
#define INT_IRQ_DUMMY_61          (GIC_SPI_MS_IRQ_START + 61)
#define INT_IRQ_DUMMY_62          (GIC_SPI_MS_IRQ_START + 62)
#define INT_IRQ_DUMMY_63          (GIC_SPI_MS_IRQ_START + 63)
#define GIC_SPI_MS_IRQ_END        (GIC_SPI_MS_IRQ_START + 64)
#define GIC_SPI_MS_IRQ_NR         (GIC_SPI_MS_IRQ_END - GIC_SPI_MS_IRQ_START)

/* MS_NON_PM_FIQ 96-127 */
#define GIC_SPI_MS_FIQ_START       GIC_SPI_MS_IRQ_END
#define INT_FIQ_TIMER_0           (GIC_SPI_MS_FIQ_START +  0)
#define INT_FIQ_TIMER_1           (GIC_SPI_MS_FIQ_START +  1)
#define INT_FIQ_WDT               (GIC_SPI_MS_FIQ_START +  2)
#define INT_FIQ_IR                (GIC_SPI_MS_FIQ_START +  3)
#define INT_FIQ_IR_RC             (GIC_SPI_MS_FIQ_START +  4)
#define INT_FIQ_POWER_0_NG        (GIC_SPI_MS_FIQ_START +  5)
#define INT_FIQ_POWER_1_NG        (GIC_SPI_MS_FIQ_START +  6)
#define INT_FIQ_POWER_2_NG        (GIC_SPI_MS_FIQ_START +  7)
#define INT_FIQ_PM_XIU_TIMEOUT    (GIC_SPI_MS_FIQ_START +  8)
#define INT_FIQ_DUMMY_09          (GIC_SPI_MS_FIQ_START +  9)
#define INT_FIQ_DUMMY_10          (GIC_SPI_MS_FIQ_START + 10)
#define INT_FIQ_DUMMY_11          (GIC_SPI_MS_FIQ_START + 11)
#define INT_FIQ_TIMER_2           (GIC_SPI_MS_FIQ_START + 12)
#define INT_FIQ_DUMMY_13          (GIC_SPI_MS_FIQ_START + 13)
#define INT_FIQ_DUMMY_14          (GIC_SPI_MS_FIQ_START + 14)
#define INT_FIQ_DUMMY_15          (GIC_SPI_MS_FIQ_START + 15)
#define INT_FIQ_FIQ_FROM_PM       (GIC_SPI_MS_FIQ_START + 16)
#define INT_FIQ_MCU51_TO_ARM      (GIC_SPI_MS_FIQ_START + 17)
#define INT_FIQ_ARM_TO_MCU51      (GIC_SPI_MS_FIQ_START + 18)
#define INT_FIQ_DUMMY_19          (GIC_SPI_MS_FIQ_START + 19)
#define INT_FIQ_DUMMY_20          (GIC_SPI_MS_FIQ_START + 20)
#define INT_FIQ_LAN_ESD           (GIC_SPI_MS_FIQ_START + 21)
#define INT_FIQ_XIU_TIMEOUT       (GIC_SPI_MS_FIQ_START + 22)
#define INT_FIQ_SD_CDZ            (GIC_SPI_MS_FIQ_START + 23)
#define INT_FIQ_SAR_GPIO_0        (GIC_SPI_MS_FIQ_START + 24)
#define INT_FIQ_SAR_GPIO_1        (GIC_SPI_MS_FIQ_START + 25)
#define INT_FIQ_SAR_GPIO_2        (GIC_SPI_MS_FIQ_START + 26)
#define INT_FIQ_SAR_GPIO_3        (GIC_SPI_MS_FIQ_START + 27)
#define INT_FIQ_SPI0_GPIO_0       (GIC_SPI_MS_FIQ_START + 28)
#define INT_FIQ_SPI0_GPIO_1       (GIC_SPI_MS_FIQ_START + 29)
#define INT_FIQ_SPI0_GPIO_2       (GIC_SPI_MS_FIQ_START + 30)
#define INT_FIQ_SPI0_GPIO_3       (GIC_SPI_MS_FIQ_START + 31)
#define GIC_SPI_MS_FIQ_END        (GIC_SPI_MS_FIQ_START + 32)
#define GIC_SPI_MS_FIQ_NR         (GIC_SPI_MS_FIQ_END - GIC_SPI_MS_FIQ_START)

/* MS_PM_SLEEP_FIQ 0-31 */
#define PMSLEEP_FIQ_START          0
#define INT_PMSLEEP_IR            (PMSLEEP_FIQ_START +  0)
#define INT_PMSLEEP_DVI_CK_DET    (PMSLEEP_FIQ_START +  1)
#define INT_PMSLEEP_GPIO_0        (PMSLEEP_FIQ_START +  2)
#define INT_PMSLEEP_GPIO_1        (PMSLEEP_FIQ_START +  3)
#define INT_PMSLEEP_GPIO_2        (PMSLEEP_FIQ_START +  4)
#define INT_PMSLEEP_GPIO_3        (PMSLEEP_FIQ_START +  5)
#define INT_PMSLEEP_GPIO_4        (PMSLEEP_FIQ_START +  6)
#define INT_PMSLEEP_GPIO_5        (PMSLEEP_FIQ_START +  7)
#define INT_PMSLEEP_GPIO_6        (PMSLEEP_FIQ_START +  8)
#define INT_PMSLEEP_GPIO_7        (PMSLEEP_FIQ_START +  9)
#define INT_PMSLEEP_GPIO_8        (PMSLEEP_FIQ_START + 10)
#define INT_PMSLEEP_GPIO_9        (PMSLEEP_FIQ_START + 11)
#define INT_PMSLEEP_GPIO_10       (PMSLEEP_FIQ_START + 12)
#define INT_PMSLEEP_GPIO_11       (PMSLEEP_FIQ_START + 13)
#define INT_PMSLEEP_GPIO_12       (PMSLEEP_FIQ_START + 14)
#define INT_PMSLEEP_GPIO_13       (PMSLEEP_FIQ_START + 15)
#define INT_PMSLEEP_GPIO_14       (PMSLEEP_FIQ_START + 16)
#define INT_PMSLEEP_GPIO_15       (PMSLEEP_FIQ_START + 17)
#define INT_PMSLEEP_GPIO_16       (PMSLEEP_FIQ_START + 18)
#define INT_PMSLEEP_GPIO_17       (PMSLEEP_FIQ_START + 19)
#define INT_PMSLEEP_GPIO_18       (PMSLEEP_FIQ_START + 20)
#define INT_PMSLEEP_GPIO_19       (PMSLEEP_FIQ_START + 21)
#define INT_PMSLEEP_IRIN          (PMSLEEP_FIQ_START + 22)
#define INT_PMSLEEP_UART_RX       (PMSLEEP_FIQ_START + 23)
#define INT_PMSLEEP_GPIO_22       (PMSLEEP_FIQ_START + 24)
#define INT_PMSLEEP_GPIO_23       (PMSLEEP_FIQ_START + 25)
#define INT_PMSLEEP_SPI_CZ        (PMSLEEP_FIQ_START + 26)
#define INT_PMSLEEP_SPI_CK        (PMSLEEP_FIQ_START + 27)
#define INT_PMSLEEP_SPI_DI        (PMSLEEP_FIQ_START + 28)
#define INT_PMSLEEP_SPI_DO        (PMSLEEP_FIQ_START + 29)
#define INT_PMSLEEP_GPIO_28       (PMSLEEP_FIQ_START + 30)
#define INT_PMSLEEP_GPIO_29       (PMSLEEP_FIQ_START + 31)
#define PMSLEEP_FIQ_END           (PMSLEEP_FIQ_START + 32)
#define PMSLEEP_FIQ_NR            (PMSLEEP_FIQ_END - PMSLEEP_FIQ_START)

#endif // __ARCH_ARM_ASM_IRQS_H
