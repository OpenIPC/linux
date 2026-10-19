/*
 * This header provides constants for AK37E Soc
 * CLCOK IDs
 *
 */
#ifndef __AK3918AV130_CLOCK_H__
#define __AK3918AV130_CLOCK_H__

/*
* Fixed Clock IDs
* Source: OSCIN / PLL1
*/
#define PLL1_CLK                        0
#define PLL1_DPHY                       1
#define PLL1_JCLK                       2
#define PLL1_DDR2                       3
#define PLL1_HCLK                       4
#define PLL1_DCLK                       5
/*
* Fixed Clock IDs
* Source: OSCIN / PLL2
*/
#define PLL2_CLK                        0
#define PLL2_GCLK                       1
#define PLL2_DPHY_CFGCLK                2
/*
* Fixed Clock IDs
* Source: OSCIN / PLL3
*/
#define PLL3_CLK                        0


/*
* Factor CLK IDs of ENC NPU ISP CSI etc.
* Source: PLL1
*/
#define PLL1_FACTOR_ISP               0
#define PLL1_FACTOR_ENC               1
#define PLL1_FACTOR_CSI0_SCLK         2
#define PLL1_FACTOR_CSI1_SCLK         3
#define PLL1_FACTOR_PP                4

/*
* Factor CLK IDs
* Source: PLL2
*/
#define PLL2_FACTOR_DPHY_CFGCLK       0
#define PLL2_FACTOR_SPI0_CLK          1
#define PLL2_FACTOR_MAC_OPCLK         2
/*
* Factor CLK IDs of ENC NPU ISP etc.
* Source: PLL2
*/
#define PLL2_FACTOR_ENC               3 // PLL2_FACTOR_MAC_OPCLK +1
#define PLL2_FACTOR_NPU               4
#define PLL2_FACTOR_ISP               5
#define PLL2_FACTOR_CSI0_SCLK         6
#define PLL2_FACTOR_CSI1_SCLK         7


/*
* Factor CLK IDs
* Source: PLL3
*/
#define PLL3_FACTOR_SDADC_HSCLK       0
#define PLL3_FACTOR_SDDAC_HSCLK       1
#define PLL3_FACTOR_SDADC_CLK         2
#define PLL3_FACTOR_SDDAC_CLK         3
#define PLL3_FACTOR_I2S0_MCLK         4
#define PLL3_FACTOR_I2S0_B_LR_CLK     5

/*
* Factor CLK IDs of ENC NPU ISP etc.
* Source: PLL3
*/
#define PLL3_FACTOR_ISP               6 // PLL3_FACTOR_I2S0_B_LR_CLK +1
#define PLL3_FACTOR_NPU               7
#define PLL3_FACTOR_ENC               8
#define PLL3_FACTOR_CSI0_SCLK         9
#define PLL3_FACTOR_CSI1_SCLK         10


/*
* Factor CLK IDs
* Source: OSC24
*/
#define OSC24_FACTOR_SAR_ADC_CLK        0
#define OSC24_FACTOR_CSI0_SCLK          1
#define OSC24_FACTOR_CSI1_SCLK          2

#define MUX_ISP_CLK                     0
#define MUX_NPU_CLK                     1
#define MUX_ENC_CLK                     2
#define MUX_CSI0_CLK                    3
#define MUX_CSI1_CLK                    4

/*
* Gate CLK IDs
* Source: PLL2_FACTOR_GCLK
*/
#define GCLK_GATE                 0
#define GCLK_MMC_0                1
#define GCLK_MMC_1                2
#define GCLK_SD_ADC               3
#define GCLK_SD_DAC               4
#define GCLK_SPI0                 5
#define GCLK_SPI1                 6
#define GCLK_UART0                7
#define GCLK_UART1                8
#define GCLK_L2BUF0               9
#define GCLK_TWI0                10
#define GCLK_L2BUF1              11
#define GCLK_GPIO                12
#define GCLK_MAC                 13
#define GCLK_ENCRYPT             14
#define GCLK_USB                 15
#define GCLK_MIPI0               16
#define GCLK_TWI1                17
#define GCLK_UART2               18
#define GCLK_MIPI1               19
#define GCLK_SPI2                20
#define GCLK_MMC_2               21
#define GCLK_TWI2                22
#define GCLK_DRAMCTRL            24
#define GCLK_DDR_PHY             25

/*
* Reset  IDs 
*/
#define RESETS_PER_BANK         (32)
/*  0x08000020 BANK0. Only define glck reset id*/
#define RESET_GATE              GCLK_GATE     
#define RESET_MMC_0             GCLK_MMC_0    
#define RESET_MMC_1             GCLK_MMC_1    
#define RESET_SD_ADC            GCLK_SD_ADC   
#define RESET_SD_DAC            GCLK_SD_DAC   
#define RESET_SPI0              GCLK_SPI0     
#define RESET_SPI1              GCLK_SPI1     
#define RESET_UART0             GCLK_UART0    
#define RESET_UART1             GCLK_UART1    
#define RESET_L2BUF0            GCLK_L2BUF0   
#define RESET_TWI0              GCLK_TWI0     
#define RESET_L2BUF1            GCLK_L2BUF1   
#define RESET_GPIO              GCLK_GPIO     
#define RESET_MAC               GCLK_MAC      
#define RESET_ENCRYPT           GCLK_ENCRYPT  
#define RESET_USB               GCLK_USB      
#define RESET_MIPI0             GCLK_MIPI0    
#define RESET_TWI1              GCLK_TWI1     
#define RESET_UART2             GCLK_UART2    
#define RESET_MIPI1             GCLK_MIPI1    
#define RESET_SPI2              GCLK_SPI2     
#define RESET_MMC_2             GCLK_MMC_2    
#define RESET_TWI2              GCLK_TWI2     
#define RESET_DRAMCTRL          GCLK_DRAMCTRL 
#define RESET_DDR_PHY           GCLK_DDR_PHY 
/*  0x0800011c  BANK1 */
#define RESET_ISP               (RESETS_PER_BANK + 7)
#define RESET_NPU               (RESETS_PER_BANK + 15)
#define RESET_ENC               (RESETS_PER_BANK + 23)
#define RESET_PP                (RESETS_PER_BANK + 31)
#endif //__AK3918AV130_CLOCK_H__
