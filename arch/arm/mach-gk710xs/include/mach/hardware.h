/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/hardware.h
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


#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/pgtable.h>
#include <asm/sizes.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE

/*-------------------------------------------------------------*/

#define CONFIG_GK_PPM_SIZE          0x200000

#define DEFAULT_MEM_START               (0xc0000000)


#define AHB_SIZE                        (0x01000000)
#define APB_SIZE                        (0x01000000)

#define NOLINUX_MEM_V_START             (0xe0000000)
#define NOLINUX_MEM_V_SIZE              (0x00100000)


//-------------------dummy-----ahb-----------
#define GK_PA_AHB_BASE              (0x90000000)
#define GK_VA_AHB_BASE              (0xf2000000)

#define GK_ADDR_SDC_OFFSET          0x00000   //0x02000  //for dummy
#define GK_ADDR_SDC1_OFFSET         0x10000   //0x02000  //for dummy
#define GK_ADDR_EFUSE_OFFSET        0x01000
#define GK_ADDR_DDRC_OFFSET         0x02000
#define GK_ADDR_DMAC_OFFSET         0x03000
#define GK_ADDR_VO_OFFSET           0x04000
#define GK_ADDR_CRYPT_OFFSET        0x05000
#define GK_ADDR_USB_OFFSET          0x06000
#define GK_ADDR_VIC1_OFFSET         0x08000
#define GK_ADDR_VIC2_OFFSET         0x09000
#define GK_ADDR_ETH_GMAC_OFFSET     0x0E000
#define GK_ADDR_ETH_DMA_OFFSET      0x0F000
#define GK_ADDR_I2S_OFFSET          0x0A000
#define GK_ADDR_SFLASH_OFFSET       0x16000

#define GK_ADDR_ADC_OFFSET          0x20A00
#define GK_ADDR_AHB_GREG_OFFSET     0x20E00
#define GK_ADDR_USB_PHY_OFFSET      0x21000
#define GK_ADDR_ENET_PHY_OFFSET     0x22000
#define GK_ADDR_VEDIO_DAC_OFFSET    0x22600


#define GK_ADDR_PMU_OFFSET          0x80000
#define GK_ADDR_PMU_RTC_OFFSET      0x80000
#define GK_ADDR_PMU_C51_CODE_OFFSET 0xC0000


#define GK_ADDR_AUDIO_CODEC_ANALOG_OFFSET   0x21C00
#define GK_ADDR_AUDIO_CODEC_DIGITAL_OFFSET  0x20C00


#define GK_PA_SDC                   (GK_PA_AHB_BASE + GK_ADDR_SDC_OFFSET)
#define GK_PA_SDC1                  (GK_PA_AHB_BASE + GK_ADDR_SDC1_OFFSET)
#define GK_PA_I2S                   (GK_PA_AHB_BASE + GK_ADDR_I2S_OFFSET)
#define GK_PA_AHB_GREG              (GK_PA_AHB_BASE + GK_ADDR_AHB_GREG_OFFSET)
#define GK_PA_EFUSE                 (GK_PA_AHB_BASE + GK_ADDR_EFUSE_OFFSET)
#define GK_PA_SFLASH                (GK_PA_AHB_BASE + GK_ADDR_SFLASH_OFFSET)
#define GK_PA_ADC                   (GK_PA_AHB_BASE + GK_ADDR_ADC_OFFSET)
#define GK_PA_VO                    (GK_PA_AHB_BASE + GK_ADDR_VO_OFFSET)
#define GK_PA_AUDIO_CODEC_DIGITAL   (GK_PA_AHB_BASE + GK_ADDR_AUDIO_CODEC_DIGITAL_OFFSET)
#define GK_PA_AUDIO_CODEC_ANALOG    (GK_PA_AHB_BASE + GK_ADDR_AUDIO_CODEC_ANALOG_OFFSET)
#define GK_PA_DDRC                  (GK_PA_AHB_BASE + GK_ADDR_DDRC_OFFSET)
#define GK_PA_VIC1                  (GK_PA_AHB_BASE + GK_ADDR_VIC1_OFFSET)
#define GK_PA_VIC2                  (GK_PA_AHB_BASE + GK_ADDR_VIC2_OFFSET)
#define GK_PA_ETH_GMAC              (GK_PA_AHB_BASE + GK_ADDR_ETH_GMAC_OFFSET)
#define GK_PA_ETH_DMA               (GK_PA_AHB_BASE + GK_ADDR_ETH_DMA_OFFSET)
#define GK_PA_USB_PHY               (GK_PA_AHB_BASE + GK_ADDR_USB_PHY_OFFSET)
#define GK_PA_ETH_PHY               (GK_PA_AHB_BASE + GK_ADDR_ENET_PHY_OFFSET)
#define GK_PA_PMU                   (GK_PA_AHB_BASE + GK_ADDR_PMU_OFFSET)
#define GK_PA_PMU_RTC               (GK_PA_AHB_BASE + GK_ADDR_PMU_RTC_OFFSET)
#define GK_PA_PMU_C51_CODE          (GK_PA_AHB_BASE + GK_ADDR_PMU_C51_CODE_OFFSET)
#define GK_PA_VEDIO_DAC             (GK_PA_AHB_BASE + GK_ADDR_VEDIO_DAC_OFFSET)
#define GK_PA_CRYPT_UNIT            (GK_PA_AHB_BASE + GK_ADDR_CRYPT_OFFSET)

#define GK_VA_SDC                   (GK_VA_AHB_BASE + GK_ADDR_SDC_OFFSET)
#define GK_VA_SDC1                  (GK_VA_AHB_BASE + GK_ADDR_SDC1_OFFSET)
#define GK_VA_I2S                   (GK_VA_AHB_BASE + GK_ADDR_I2S_OFFSET)
#define GK_VA_AHB_GREG              (GK_VA_AHB_BASE + GK_ADDR_AHB_GREG_OFFSET)
#define GK_VA_EFUSE                 (GK_VA_AHB_BASE + GK_ADDR_EFUSE_OFFSET)
#define GK_VA_ADC                   (GK_VA_AHB_BASE + GK_ADDR_ADC_OFFSET)
#define GK_VA_DDRC                  (GK_VA_AHB_BASE + GK_ADDR_DDRC_OFFSET)
#define GK_VA_VIC1                  (GK_VA_AHB_BASE + GK_ADDR_VIC1_OFFSET)
#define GK_VA_VIC2                  (GK_VA_AHB_BASE + GK_ADDR_VIC2_OFFSET)
#define GK_VA_ETH_GMAC              (GK_VA_AHB_BASE + GK_ADDR_ETH_GMAC_OFFSET)
#define GK_VA_ETH_DMA               (GK_VA_AHB_BASE + GK_ADDR_ETH_DMA_OFFSET)
#define GK_VA_USB_PHY               (GK_VA_AHB_BASE + GK_ADDR_USB_PHY_OFFSET)
#define GK_VA_ETH_PHY               (GK_VA_AHB_BASE + GK_ADDR_ENET_PHY_OFFSET)
#define GK_VA_PMU                   (GK_VA_AHB_BASE + GK_ADDR_PMU_OFFSET)
#define GK_VA_PMU_RTC               (GK_VA_AHB_BASE + GK_ADDR_PMU_RTC_OFFSET)
#define GK_VA_SFLASH                (GK_VA_AHB_BASE + GK_ADDR_SFLASH_OFFSET)
#define GK_VA_VO                    (GK_VA_AHB_BASE + GK_ADDR_VO_OFFSET)
#define GK_VA_AUDIO_CODEC_DIGITAL   (GK_VA_AHB_BASE + GK_ADDR_AUDIO_CODEC_DIGITAL_OFFSET)
#define GK_VA_AUDIO_CODEC_ANALOG    (GK_VA_AHB_BASE + GK_ADDR_AUDIO_CODEC_ANALOG_OFFSET)
#define GK_VA_PMU_C51_CODE          (GK_VA_AHB_BASE + GK_ADDR_PMU_C51_CODE_OFFSET)
#define GK_VA_VEDIO_DAC             (GK_VA_AHB_BASE + GK_ADDR_VEDIO_DAC_OFFSET)  /*cvbs*/
#define GK_VA_CRYPT_UNIT            (GK_VA_AHB_BASE + GK_ADDR_CRYPT_OFFSET)

#define GK_PA_DMAC                  (GK_PA_AHB_BASE + GK_ADDR_DMAC_OFFSET)
#define GK_PA_USB                   (GK_PA_AHB_BASE + GK_ADDR_USB_OFFSET)
#define GK_VA_DMAC                  (GK_VA_AHB_BASE + GK_ADDR_DMAC_OFFSET)
#define GK_VA_USB                   (GK_VA_AHB_BASE + GK_ADDR_USB_OFFSET)


//-------------------dummy-----apb-----------
#define GK_PA_APB_BASE          (0xA0000000)
#define GK_VA_APB_BASE          (0xf3000000)

#define GK_ADDR_TIMER_OFFSET    0x00000
#define GK_ADDR_IDC_OFFSET      0x03000
#define GK_ADDR_IDC2_OFFSET     0x04000
#define GK_ADDR_UART0_OFFSET    0x05000
#define GK_ADDR_WDT_OFFSET      0x06000
#define GK_ADDR_GPIO0_OFFSET    0x09000
#define GK_ADDR_ADC2_OFFSET     0x07000

#define GK_ADDR_PWM0_OFFSET     0x0a000
#define GK_ADDR_PWM1_OFFSET     0x0b000
#define GK_ADDR_PWM2_OFFSET     0x0c000
#define GK_ADDR_PWM3_OFFSET     0x0d000

#define GK_ADDR_UART2_OFFSET    0x1E000
#define GK_ADDR_UART1_OFFSET    0x1F000

#define GK_ADDR_SSI1_OFFSET     0x08000
#define GK_ADDR_SSI2_OFFSET     0x02000
#define GK_ADDR_TSSI_OFFSET     0x01000

#define GK_ADDR_MEM_OFFSET      0x150000
#define GK_ADDR_RCT_OFFSET      0x170000



#define GK_PA_TIMER             (GK_PA_APB_BASE + GK_ADDR_TIMER_OFFSET)
#define GK_PA_IDC               (GK_PA_APB_BASE + GK_ADDR_IDC_OFFSET)
#define GK_PA_IDC2              (GK_PA_APB_BASE + GK_ADDR_IDC2_OFFSET)
#define GK_PA_UART0             (GK_PA_APB_BASE + GK_ADDR_UART0_OFFSET)
#define GK_PA_WDT               (GK_PA_APB_BASE + GK_ADDR_WDT_OFFSET)
#define GK_PA_GPIO0             (GK_PA_APB_BASE + GK_ADDR_GPIO0_OFFSET)
#define GK_PA_PWM0              (GK_PA_APB_BASE + GK_ADDR_PWM0_OFFSET)
#define GK_PA_PWM1              (GK_PA_APB_BASE + GK_ADDR_PWM1_OFFSET)
#define GK_PA_PWM2              (GK_PA_APB_BASE + GK_ADDR_PWM2_OFFSET)
#define GK_PA_PWM3              (GK_PA_APB_BASE + GK_ADDR_PWM3_OFFSET)
#define GK_PA_UART2             (GK_PA_APB_BASE + GK_ADDR_UART2_OFFSET)
#define GK_PA_UART1             (GK_PA_APB_BASE + GK_ADDR_UART1_OFFSET)
#define GK_PA_ADC2              (GK_PA_APB_BASE + GK_ADDR_ADC2_OFFSET)

#define GK_PA_SSI1              (GK_PA_APB_BASE + GK_ADDR_SSI1_OFFSET)
#define GK_PA_SSI2              (GK_PA_APB_BASE + GK_ADDR_SSI2_OFFSET)
#define GK_PA_TSSI              (GK_PA_APB_BASE + GK_ADDR_TSSI_OFFSET)
#define GK_PA_RCT               (GK_PA_APB_BASE + GK_ADDR_RCT_OFFSET)
#define GK_PA_MEM               (GK_PA_APB_BASE + GK_ADDR_MEM_OFFSET)



#define GK_VA_TIMER             (GK_VA_APB_BASE + GK_ADDR_TIMER_OFFSET)
#define GK_VA_IDC               (GK_VA_APB_BASE + GK_ADDR_IDC_OFFSET)
#define GK_VA_IDC2              (GK_VA_APB_BASE + GK_ADDR_IDC2_OFFSET)
#define GK_VA_UART0             (GK_VA_APB_BASE + GK_ADDR_UART0_OFFSET)
#define GK_VA_WDT               (GK_VA_APB_BASE + GK_ADDR_WDT_OFFSET)
#define GK_VA_GPIO0             (GK_VA_APB_BASE + GK_ADDR_GPIO0_OFFSET)
#define GK_VA_PWM0              (GK_VA_APB_BASE + GK_ADDR_PWM0_OFFSET)
#define GK_VA_PWM1              (GK_VA_APB_BASE + GK_ADDR_PWM1_OFFSET)
#define GK_VA_PWM2              (GK_VA_APB_BASE + GK_ADDR_PWM2_OFFSET)
#define GK_VA_PWM3              (GK_VA_APB_BASE + GK_ADDR_PWM3_OFFSET)
#define GK_VA_ADC2              (GK_VA_APB_BASE + GK_ADDR_ADC2_OFFSET)

#define GK_VA_UART2             (GK_VA_APB_BASE + GK_ADDR_UART2_OFFSET)
#define GK_VA_UART1             (GK_VA_APB_BASE + GK_ADDR_UART1_OFFSET)

#define GK_VA_SSI1              (GK_VA_APB_BASE + GK_ADDR_SSI1_OFFSET)
#define GK_VA_SSI2              (GK_VA_APB_BASE + GK_ADDR_SSI2_OFFSET)
#define GK_VA_TSSI              (GK_VA_APB_BASE + GK_ADDR_TSSI_OFFSET)
#define GK_VA_RCT               (GK_VA_APB_BASE + GK_ADDR_RCT_OFFSET)
#define GK_VA_MEM               (GK_VA_APB_BASE + GK_ADDR_MEM_OFFSET)


#define GRAPHICS_DMA_OFFSET         0x15000
#define GRAPHICS_DMA_BASE           (GK_VA_AHB_BASE + GRAPHICS_DMA_OFFSET)
#define GRAPHICS_DMA_REG(x)         (GRAPHICS_DMA_BASE + (x))



#define VO_CLK_ONCHIP_PLL_27MHZ     0x0        /* Default setting */
#define VO_CLK_ONCHIP_PLL_SP_CLK    0x1
#define VO_CLK_ONCHIP_PLL_CLK_SI    0x2
#define VO_CLK_EXTERNAL             0x3
#define VO_CLK_ONCHIP_PLL_DSP_SCLK  0x4

#define TIMER_OFFSET                0xb000


#define CONSISTENT_DMA_SIZE         SZ_8M

/* ==========================================================================*/

//end

#endif  /* __ASM_ARCH_HARDWARE_H */


