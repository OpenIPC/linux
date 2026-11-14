/*
 * This header provides constants for AK37E Soc
 * CLCOK IDs
 *
 */
#ifndef __AK37E_CLOCK_H__
#define __AK37E_CLOCK_H__

/*
* Source: OSCIN
*/
#define CPU_PLL_JCLK                   0
#define CPU_PLL_HCLK                   1
#define CPU_PLL_DPHYCLK                2
#define ASIC_PLL_CLK                   3
#define AUDIO_PLL_CLK                  4

/*
* Factor CLK IDs
* Source: ASIC
*/
#define ASIC_FACTOR_CSI0_SCLK          0
#define ASIC_FACTOR_CSI1_SCLK          1
#define ASIC_FACTOR_MAC0_OPCLK         2
#define ASIC_FACTOR_MAC1_OPCLK         3
#define ASIC_FACTOR_LCD_PCLK           4
#define ASIC_FACTOR_SPI0_CLK           5

/*
* Gate CLK IDs
* Source: ASIC
*/
#define ASIC_GCLK_BRIDGE_MODULE        0
#define ASIC_GCLK_MMC_0                1
#define ASIC_GCLK_MMC_1                2
#define ASIC_GCLK_SD_ADC               3
#define ASIC_GCLK_SD_DAC               4
#define ASIC_GCLK_SPI0                 5
#define ASIC_GCLK_SPI1                 6
#define ASIC_GCLK_UART0                7
#define ASIC_GCLK_UART1                8
#define ASIC_GCLK_L2BUF0               9
#define ASIC_GCLK_TWI0                 10
#define ASIC_GCLK_L2BUF1               11
#define ASIC_GCLK_GPIO                 12
#define ASIC_GCLK_MAC0                 13
#define ASIC_GCLK_PDM                  14
#define ASIC_GCLK_USB                  15
#define ASIC_GCLK_MAC1                 16
#define ASIC_GCLK_TWI1                 17
#define ASIC_GCLK_UART2                18
#define ASIC_GCLK_UART3                19
#define ASIC_GCLK_SPI2                 20
#define ASIC_GCLK_MMC_2                21
#define ASIC_GCLK_TWI2                 22
#define ASIC_GCLK_TWI3                 23
#define ASIC_GCLK_IMAGE_CAPTURE        24
#define ASIC_GCLK_JPEG_CODEC           25
#define ASIC_GCLK_VIDEO_DECODER        26
#define ASIC_GCLK_LCD_CONTROLLER       27
#define ASIC_GCLK_GUI                  28
#define ASIC_GCLK_DSI                  29

/*
* Factor CLK IDs
* Source: AUDIO
*/
#define AUDIO_FACTOR_SDADC_HSCLK       0
#define AUDIO_FACTOR_SDDAC_HSCLK       1
#define AUDIO_FACTOR_SDADC_CLK         2
#define AUDIO_FACTOR_SDDAC_CLK         3
#define AUDIO_FACTOR_PDM_HSCLK         4
#define AUDIO_FACTOR_PDM_I2SM_CLK      5
#define AUDIO_FACTOR_I2S0_MCLK         6
#define AUDIO_FACTOR_I2S0_B_LR_CLK     7

/*
* SAR ADC
*/
#define SAR_FACTOR_ADC_CLK             0

#endif //__AK37E_CLOCK_H__
