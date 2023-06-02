/*!
*****************************************************************************
** \file        arch/arm/plat-goke/include/plat/gk_gpio_v1_00.h
**
** \version     $Id: gk_gpio_v1_00.h 9208 2016-07-21 12:51:42Z yulindeng $
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2016 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#ifndef _GK_GPIO_V1_20_H_
#define _GK_GPIO_V1_20_H_



//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
/************************/
/* GPIO pins definition */
/************************/
#define GPIO_SET_OUT_SEL(n)     ((n)&0x7F)
#define GPIO_SET_IN_SEL(n)      (((n)&0x3F)<<8)
#define GPIO_SET_OEN_SEL(n)     (((n)&0x3F)<<16)
#define GPIO_SET_OUT_INVERT(n)  (((n)&0x1)<<22)
#define GPIO_SET_OEN_INVERT(n)  (((n)&0x1)<<23)
#define GPIO_SET_IOCTRL(n)      (((n)&0x3F)<<24)
#define GPIO_SET_FUNC(n)        (((n)&0x3)<<30)

#define GPIO_GET_OUT_SEL(n)     (((n)&0x0000007F))
#define GPIO_GET_IN_SEL(n)      (((n)&0x00003F00)>>8)
#define GPIO_GET_OEN_SEL(n)     (((n)&0x003F0000)>>16)
#define GPIO_GET_OUT_INVERT(n)  (((n)&0x00400000)>>22)
#define GPIO_GET_OEN_INVERT(n)  (((n)&0x00800000)>>23)
#define GPIO_GET_IOCTRL(n)      (((n)&0x3F000000)>>24)
#define GPIO_GET_FUNC(n)        (((n)&0xC0000000)>>30)

#define IOCTRL_NORMAL           0x00    //!< Hi-z
#define IOCTRL_PULL_UP          0x10    //!< PULL_UP
#define IOCTRL_PULL_DOWN        0x20    //!< PULL_DOWN
#define IOCTRL_REPEAT           0x30    //!< REPEAT

#define IOCTRL_2MA              0x00    //!< 2mA
#define IOCTRL_4MA              0x01    //!< 4mA
#define IOCTRL_8MA              0x02    //!< 8mA
#define IOCTRL_12MA             0x03    //!< 12mA



//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
typedef enum
{
    /* ----------------------------------- GPIO output function define ----------------------------------- */
    GPIO_TYPE_OUTPUT_0               = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 0),    //!< Output type: value = 0
    GPIO_TYPE_OUTPUT_1               = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 1),    //!< Output type: value = 1
    GPIO_TYPE_OUTPUT_SPI1_SO         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 2) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 2),    //!< Output type: tssi_txd
    GPIO_TYPE_OUTPUT_SPI1_CS0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 3),    //!< Output type: tssi_cs0_n
    GPIO_TYPE_OUTPUT_SPI1_SCLK       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 4),    //!< Output type: tssi_sclk_out
    GPIO_TYPE_OUTPUT_UART2_RTS_N     = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 5),    //!< Output type: uart2_rts_n
    GPIO_TYPE_OUTPUT_UART2_DTR_N     = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 6),    //!< Output type: uart2_dtr_n
    GPIO_TYPE_OUTPUT_UART2_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 7),    //!< Output type: uart2_tx
    GPIO_TYPE_OUTPUT_UART1_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 8),    //!< Output type: uart1_tx
    GPIO_TYPE_OUTPUT_UART0_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 9),    //!< Output type: uart0_tx
    GPIO_TYPE_OUTPUT_PWM3_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(10),    //!< Output type: pwm3_out
    GPIO_TYPE_OUTPUT_PWM2_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(11),    //!< Output type: pwm2_out
    GPIO_TYPE_OUTPUT_PWM1_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(12),    //!< Output type: pwm1_out
    GPIO_TYPE_OUTPUT_PWM0_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(13),    //!< Output type: pwm0_out
    GPIO_TYPE_OUTPUT_SPI0_SO         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 7) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(14),    //!< Output type: ssi_txd
    GPIO_TYPE_OUTPUT_SPI0_CS7        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(15),    //!< Output type: ssi_cs7_n
    GPIO_TYPE_OUTPUT_SPI0_CS6        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(16),    //!< Output type: ssi_cs6_n
    GPIO_TYPE_OUTPUT_SPI0_CS5        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(17),    //!< Output type: ssi_cs5_n
    GPIO_TYPE_OUTPUT_SPI0_CS4        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(18),    //!< Output type: ssi_cs4_n
    GPIO_TYPE_OUTPUT_SPI0_CS1        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(19),    //!< Output type: ssi_cs1_n
    GPIO_TYPE_OUTPUT_SPI0_CS0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(20),    //!< Output type: ssi_cs0_n
    GPIO_TYPE_OUTPUT_SPI0_SCLK       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(21),    //!< Output type: ssi_sclk_out
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL( 8)                           GPIO_SET_OUT_SEL(22)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL( 9)                           GPIO_SET_OUT_SEL(23)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(10)                           GPIO_SET_OUT_SEL(24)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(11)                           GPIO_SET_OUT_SEL(25)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(12)                           GPIO_SET_OUT_SEL(26)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(13)                           GPIO_SET_OUT_SEL(27)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(14)                           GPIO_SET_OUT_SEL(28)
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                          GPIO_SET_OEN_SEL(15)                           GPIO_SET_OUT_SEL(29)
    //GPIO_TYPE_INOUT_SDIO_CMD                                                                                                                                           GPIO_SET_OEN_SEL(16)                           GPIO_SET_OUT_SEL(30)
    GPIO_TYPE_OUTPUT_SDIO_CLK        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(31),    //!< Output type: sd_clk_sdcard
    GPIO_TYPE_OUTPUT_AOMCLK          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(32),    //!< Output type: i2s_au_clk
    GPIO_TYPE_OUTPUT_AOBCLK          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(17) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(33),    //!< Output type: i2s_clk_o
    GPIO_TYPE_OUTPUT_AOLRCLK         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(18) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(34),    //!< Output type: i2s_ws_o
    GPIO_TYPE_OUTPUT_AO_DATA0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(35),    //!< Output type: i2s_so
    GPIO_TYPE_OUTPUT_SF_CS0          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(36),    //!< Output type: sf_cs0_n
    GPIO_TYPE_OUTPUT_SF_CS1          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(37),    //!< Output type: sf_cs1_n
    GPIO_TYPE_OUTPUT_EPHY_LED_0      = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(38),    //!< Output type: ephy_led[0] hcd ok
    GPIO_TYPE_OUTPUT_EPHY_LED_1      = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(39),    //!< Output type: ephy_led[1] duplex
    GPIO_TYPE_OUTPUT_EPHY_LED_2      = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(40),    //!< Output type: ephy_led[2] 10M CRS out
    GPIO_TYPE_OUTPUT_EPHY_LED_3      = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(41),    //!< Output type: ephy_led[3] 100M CRS out
    GPIO_TYPE_OUTPUT_EPHY_LED_4      = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(42),    //!< Output type: ephy_led[4] clo gs
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_0  = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(43),    //!< Output type: enet_phy_txd[0]
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_1  = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(44),    //!< Output type: enet_phy_txd[1]
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_2  = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(45),    //!< Output type: enet_phy_txd[2]
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_3  = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(46),    //!< Output type: enet_phy_txd[3]
    GPIO_TYPE_OUTPUT_ENET_PHY_TXER   = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(47),    //!< Output type: enet_phy_txer
    GPIO_TYPE_OUTPUT_ENET_PHY_TXEN   = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(48),    //!< Output type: enet_phy_txen
    // GPIO_TYPE_INOUT_ETH_MDIO                                                                                                                                                                                         GPIO_SET_OUT_SEL(49)
    GPIO_TYPE_OUTPUT_ENET_GMII_MDC_O = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(50),    //!< Output type: enet_gmii_mdc_o
    //GPIO_TYPE_INOUT_SD2_DATA_0                                                                                                                                         GPIO_SET_OEN_SEL(20)                           GPIO_SET_OUT_SEL(51)
    //GPIO_TYPE_INOUT_SD2_DATA_1                                                                                                                                         GPIO_SET_OEN_SEL(21)                           GPIO_SET_OUT_SEL(52)
    //GPIO_TYPE_INOUT_SD2_DATA_2                                                                                                                                         GPIO_SET_OEN_SEL(22)                           GPIO_SET_OUT_SEL(53)
    //GPIO_TYPE_INOUT_SD2_DATA_3                                                                                                                                         GPIO_SET_OEN_SEL(23)                           GPIO_SET_OUT_SEL(54)
    //GPIO_TYPE_INOUT_SDIO_CMD                                                                                                                                           GPIO_SET_OEN_SEL(24)                           GPIO_SET_OUT_SEL(55)
    GPIO_TYPE_OUTPUT_SDIO2_CLK       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(56),    //!< Output type: sd2_clk_sdcard
    GPIO_TYPE_OUTPUT_JTAGE_TDO       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(57),    //!< Output type: jtag_tdout
    GPIO_TYPE_OUTPUT_VD_VSYNC        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(58),    //!< Output type: lcd_vsync
    GPIO_TYPE_OUTPUT_VD_HSYNC        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(59),    //!< Output type: lcd_hsync
    GPIO_TYPE_OUTPUT_VD_CLOCK        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(60),    //!< Output type: lcd_dclk
    GPIO_TYPE_OUTPUT_VD_HVLD         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(61),    //!< Output type: lcd_hvld
    GPIO_TYPE_OUTPUT_VD_DATA0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(62),    //!< Output type: lcd_data0
    GPIO_TYPE_OUTPUT_VD_DATA1        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(63),    //!< Output type: lcd_data1
    GPIO_TYPE_OUTPUT_VD_DATA2        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(64),    //!< Output type: lcd_data2
    GPIO_TYPE_OUTPUT_VD_DATA3        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(65),    //!< Output type: lcd_data3
    GPIO_TYPE_OUTPUT_VD_DATA4        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(66),    //!< Output type: lcd_data4
    GPIO_TYPE_OUTPUT_VD_DATA5        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(67),    //!< Output type: lcd_data5
    GPIO_TYPE_OUTPUT_VD_DATA6        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(68),    //!< Output type: lcd_data6
    GPIO_TYPE_OUTPUT_VD_DATA7        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(69),    //!< Output type: lcd_data7
    GPIO_TYPE_OUTPUT_VD_DATA8        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(70),    //!< Output type: lcd_data8
    GPIO_TYPE_OUTPUT_VD_DATA9        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(71),    //!< Output type: lcd_data9
    GPIO_TYPE_OUTPUT_VD_DATA10       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(72),    //!< Output type: lcd_data10
    GPIO_TYPE_OUTPUT_VD_DATA11       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(73),    //!< Output type: lcd_data11
    GPIO_TYPE_OUTPUT_VD_DATA12       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(74),    //!< Output type: lcd_data12
    GPIO_TYPE_OUTPUT_VD_DATA13       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(75),    //!< Output type: lcd_data13
    GPIO_TYPE_OUTPUT_VD_DATA14       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(76),    //!< Output type: lcd_data14
    GPIO_TYPE_OUTPUT_VD_DATA15       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(77),    //!< Output type: lcd_data15
    GPIO_TYPE_OUTPUT_VD_DATA16       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(78),    //!< Output type: lcd_data16
    GPIO_TYPE_OUTPUT_VD_DATA17       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(79),    //!< Output type: lcd_data17
    GPIO_TYPE_OUTPUT_VD_DATA18       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(80),    //!< Output type: lcd_data18
    GPIO_TYPE_OUTPUT_VD_DATA19       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(81),    //!< Output type: lcd_data19
    GPIO_TYPE_OUTPUT_VD_DATA20       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(82),    //!< Output type: lcd_data20
    GPIO_TYPE_OUTPUT_VD_DATA21       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(83),    //!< Output type: lcd_data21
    GPIO_TYPE_OUTPUT_VD_DATA22       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(84),    //!< Output type: lcd_data22
    GPIO_TYPE_OUTPUT_VD_DATA23       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(85),    //!< Output type: lcd_data23
    GPIO_TYPE_OUTPUT_RCT_CLK_OUT2    = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(86),    //!< Output type: rct_clk_out2
    GPIO_TYPE_OUTPUT_RCT_CLK_OUT1    = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(87),    //!< Output type: rct_clk_out1
    GPIO_TYPE_OUTPUT_I80_LCD_RST     = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(88),    //!< Output type: i80_lcd_rst
    GPIO_TYPE_OUTPUT_I80_RDN         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(89),    //!< Output type: i80_rdn
    GPIO_TYPE_OUTPUT_I80_WRN         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(90),    //!< Output type: i80_wrn
    GPIO_TYPE_OUTPUT_I80_DCX         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(91),    //!< Output type: i80_dcx
    GPIO_TYPE_OUTPUT_I80_CSN         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(92),    //!< Output type: i80_csn
	GPIO_TYPE_OUTPUT_I80_DATA0       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(93),    //!< Output type: i80_data_tx0
	GPIO_TYPE_OUTPUT_I80_DATA1       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(94),    //!< Output type: i80_data_tx1
	GPIO_TYPE_OUTPUT_I80_DATA2       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(95),    //!< Output type: i80_data_tx2
	GPIO_TYPE_OUTPUT_I80_DATA3       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(96),    //!< Output type: i80_data_tx3
	GPIO_TYPE_OUTPUT_I80_DATA4       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(97),    //!< Output type: i80_data_tx4
	GPIO_TYPE_OUTPUT_I80_DATA5       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(98),    //!< Output type: i80_data_tx5
	GPIO_TYPE_OUTPUT_I80_DATA6       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(99),    //!< Output type: i80_data_tx6
	GPIO_TYPE_OUTPUT_I80_DATA7       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(100),   //!< Output type: i80_data_tx7
	GPIO_TYPE_OUTPUT_I80_DATA8       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(101),   //!< Output type: i80_data_tx8
    GPIO_TYPE_OUTPUT_PWM7_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(102),   //!< Output type: pwm7_out
    GPIO_TYPE_OUTPUT_PWM6_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(103),   //!< Output type: pwm6_out
    GPIO_TYPE_OUTPUT_PWM5_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(104),   //!< Output type: pwm5_out
    GPIO_TYPE_OUTPUT_PWM4_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(105),   //!< Output type: pwm4_out

    /* ----------------------------------- GPIO input function define ----------------------------------- */

    GPIO_TYPE_INPUT                  = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_NORMAL|IOCTRL_2MA)      | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(   0) | GPIO_SET_OUT_SEL( 0),    //!< Input type: normal input
    GPIO_TYPE_INPUT_0                = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(   0) | GPIO_SET_OUT_SEL( 0),    //!< Input type: normal input
    GPIO_TYPE_INPUT_1                = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(   1) | GPIO_SET_OUT_SEL( 0),    //!< Input type: normal input
    GPIO_TYPE_INPUT_SPI1_SI          = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 0) | GPIO_SET_OUT_SEL( 0),    //!< Input type: tssi_rxd
    //GPIO_TYPE_INOUT_I2C_DATA                                                                                                                                                                  GPIO_SET_IN_SEL(2+ 1)
    //GPIO_TYPE_INOUT_I2C_CLK                                                                                                                                                                   GPIO_SET_IN_SEL(2+ 2)
    //GPIO_TYPE_INOUT_I2C_DATA2                                                                                                                                                                 GPIO_SET_IN_SEL(2+ 3)
    //GPIO_TYPE_INOUT_I2C_CLK2                                                                                                                                                                  GPIO_SET_IN_SEL(2+ 4)
    GPIO_TYPE_INPUT_UART2_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 5) | GPIO_SET_OUT_SEL( 0),    //!< Input type: uart2_rx
    GPIO_TYPE_INPUT_UART1_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 6) | GPIO_SET_OUT_SEL( 0),    //!< Input type: uart1_rx
    GPIO_TYPE_INPUT_UART0_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 7) | GPIO_SET_OUT_SEL( 0),    //!< Input type: uart0_rx
    GPIO_TYPE_INPUT_TIMER1_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 8) | GPIO_SET_OUT_SEL( 0),    //!< Input type: timer1_clk
    GPIO_TYPE_INPUT_TIMER2_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 9) | GPIO_SET_OUT_SEL( 0),    //!< Input type: timer2_clk
    GPIO_TYPE_INPUT_TIMER3_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+10) | GPIO_SET_OUT_SEL( 0),    //!< Input type: timer3_clk
    GPIO_TYPE_INPUT_SPI0_SI          = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+11) | GPIO_SET_OUT_SEL( 0),    //!< Input type: ssi_rxd
    GPIO_TYPE_INPUT_SD_WP_N          = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+12) | GPIO_SET_OUT_SEL( 0),    //!< Input type: sd_wp_n
    GPIO_TYPE_INPUT_SD_CD_N          = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+13) | GPIO_SET_OUT_SEL( 0),    //!< Input type: sd_cd_n
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                                                 GPIO_SET_IN_SEL(2+14)
    //GPIO_TYPE_INOUT_SD_DATA_1                                                                                                                                                                 GPIO_SET_IN_SEL(2+15)
    //GPIO_TYPE_INOUT_SD_DATA_2                                                                                                                                                                 GPIO_SET_IN_SEL(2+16)
    //GPIO_TYPE_INOUT_SD_DATA_3                                                                                                                                                                 GPIO_SET_IN_SEL(2+17)
    //GPIO_TYPE_INOUT_SD_DATA_4                                                                                                                                                                 GPIO_SET_IN_SEL(2+18)
    //GPIO_TYPE_INOUT_SD_DATA_5                                                                                                                                                                 GPIO_SET_IN_SEL(2+19)
    //GPIO_TYPE_INOUT_SD_DATA_6                                                                                                                                                                 GPIO_SET_IN_SEL(2+20)
    //GPIO_TYPE_INOUT_SD_DATA_7                                                                                                                                                                 GPIO_SET_IN_SEL(2+21)
    //GPIO_TYPE_INOUT_SDIO_CMD                                                                                                                                                                  GPIO_SET_IN_SEL(2+22)
    GPIO_TYPE_INPUT_I2S_CLK          = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+23) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_clk
    GPIO_TYPE_INPUT_I2S_WS           = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+24) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_ws
    GPIO_TYPE_INPUT_I2S_SI           = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+25) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_si
    GPIO_TYPE_INPUT_CLK_AU           = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+26) | GPIO_SET_OUT_SEL( 0),    //!< Input type: clk_au
    GPIO_TYPE_INPUT_ENET_PHY_RXD_0   = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+27) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxd[0]
    GPIO_TYPE_INPUT_ENET_PHY_RXD_1   = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+28) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxd[1]
    GPIO_TYPE_INPUT_ENET_PHY_RXD_2   = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+29) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxd[2]
    GPIO_TYPE_INPUT_ENET_PHY_RXD_3   = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+30) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxd[3]
    GPIO_TYPE_INPUT_ENET_PHY_COL     = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+31) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_col
    GPIO_TYPE_INPUT_ENET_PHY_CRS     = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+32) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_crs
    GPIO_TYPE_INPUT_ENET_PHY_RXER    = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+33) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxer
    GPIO_TYPE_INPUT_ENET_PHY_RXDV    = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+34) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_phy_rxdv
    // GPIO_TYPE_INOUT_ETH_MDIO                                                                                                                                                               GPIO_SET_IN_SEL(2+35)
    GPIO_TYPE_INPUT_ENET_CLK_RX      = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+36) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_clk_rx
    GPIO_TYPE_INPUT_ENET_CLK_TX      = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+37) | GPIO_SET_OUT_SEL( 0),    //!< Input type: enet_clk_tx
    GPIO_TYPE_INPUT_SD2_WP_N         = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+38) | GPIO_SET_OUT_SEL( 0),    //!< Input type: sd2_wp_n
    GPIO_TYPE_INPUT_SD2_CD_N         = GPIO_SET_FUNC(GPIO_FUNC_IN)  | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)     | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+39) | GPIO_SET_OUT_SEL( 0),    //!< Input type: sd2_cd_n
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                                     GPIO_SET_IN_SEL(2+40)
    //GPIO_TYPE_INOUT_SD_DATA_1                                                                                                                                                     GPIO_SET_IN_SEL(2+41)
    //GPIO_TYPE_INOUT_SD_DATA_2                                                                                                                                                     GPIO_SET_IN_SEL(2+42)
    //GPIO_TYPE_INOUT_SD_DATA_3                                                                                                                                                     GPIO_SET_IN_SEL(2+43)
    //GPIO_TYPE_INOUT_SDIO_CMD                                                                                                                                                      GPIO_SET_IN_SEL(2+44)
	GPIO_TYPE_INPUT_I80_DATA0        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+45) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx0
	GPIO_TYPE_INPUT_I80_DATA1        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+46) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx1
	GPIO_TYPE_INPUT_I80_DATA2        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+47) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx2
	GPIO_TYPE_INPUT_I80_DATA3        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+48) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx3
	GPIO_TYPE_INPUT_I80_DATA4        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+49) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx4
	GPIO_TYPE_INPUT_I80_DATA5        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+50) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx5
	GPIO_TYPE_INPUT_I80_DATA6        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+51) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx6
	GPIO_TYPE_INPUT_I80_DATA7        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+52) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx7
	GPIO_TYPE_INPUT_I80_DATA8        = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+53) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i80_data_rx8
    GPIO_TYPE_INPUT_JTAG_TRSTN       = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+56) | GPIO_SET_OUT_SEL( 0),    //!< Input type: jtag_trstn
    GPIO_TYPE_INPUT_JTAG_TCK         = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+57) | GPIO_SET_OUT_SEL( 0),    //!< Input type: jtag_tck
    GPIO_TYPE_INPUT_JTAG_TMS         = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+58) | GPIO_SET_OUT_SEL( 0),    //!< Input type: jtag_tms
    GPIO_TYPE_INPUT_JTAG_TDI         = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+59) | GPIO_SET_OUT_SEL( 0),    //!< Input type: jtag_tdi

    /* ----------------------------------- GPIO input&&output function define ----------------------------------- */
    GPIO_TYPE_INOUT_I2C_DATA         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 3) | GPIO_SET_IN_SEL(2+ 1) | GPIO_SET_OUT_SEL( 0),    //!< Input/Output type: i2c_sda
    GPIO_TYPE_INOUT_I2C_CLK          = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 4) | GPIO_SET_IN_SEL(2+ 2) | GPIO_SET_OUT_SEL( 0),    //!< Input/Output type: i2c_scl
    GPIO_TYPE_INOUT_I2C_DATA2        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 5) | GPIO_SET_IN_SEL(2+ 3) | GPIO_SET_OUT_SEL( 0),    //!< Input/Output type: i2c_sda2
    GPIO_TYPE_INOUT_I2C_CLK2         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 6) | GPIO_SET_IN_SEL(2+ 4) | GPIO_SET_OUT_SEL( 0),    //!< Input/Output type: i2c_scl2

    GPIO_TYPE_INOUT_ETH_MDIO         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(19) | GPIO_SET_IN_SEL(2+35) | GPIO_SET_OUT_SEL(49),    //!< Input/Output type: enet_gmii_mdi/enet_gmii_mod_o

    GPIO_TYPE_INOUT_SD_DATA_0        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 8) | GPIO_SET_IN_SEL(2+14) | GPIO_SET_OUT_SEL(22),    //!< Input/Output type: sd_data_out[0]
    GPIO_TYPE_INOUT_SD_DATA_1        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 9) | GPIO_SET_IN_SEL(2+15) | GPIO_SET_OUT_SEL(23),    //!< Input/Output type: sd_data_out[1]
    GPIO_TYPE_INOUT_SD_DATA_2        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(10) | GPIO_SET_IN_SEL(2+16) | GPIO_SET_OUT_SEL(24),    //!< Input/Output type: sd_data_out[2]
    GPIO_TYPE_INOUT_SD_DATA_3        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(11) | GPIO_SET_IN_SEL(2+17) | GPIO_SET_OUT_SEL(25),    //!< Input/Output type: sd_data_out[3]
    GPIO_TYPE_INOUT_SD_DATA_4        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(12) | GPIO_SET_IN_SEL(2+18) | GPIO_SET_OUT_SEL(26),    //!< Input/Output type: sd_data_out[4]
    GPIO_TYPE_INOUT_SD_DATA_5        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(13) | GPIO_SET_IN_SEL(2+19) | GPIO_SET_OUT_SEL(27),    //!< Input/Output type: sd_data_out[5]
    GPIO_TYPE_INOUT_SD_DATA_6        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(14) | GPIO_SET_IN_SEL(2+20) | GPIO_SET_OUT_SEL(28),    //!< Input/Output type: sd_data_out[6]
    GPIO_TYPE_INOUT_SD_DATA_7        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(15) | GPIO_SET_IN_SEL(2+21) | GPIO_SET_OUT_SEL(29),    //!< Input/Output type: sd_data_out[7]
    GPIO_TYPE_INOUT_SD_CMD           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(16) | GPIO_SET_IN_SEL(2+22) | GPIO_SET_OUT_SEL(30),    //!< Input/Output : sd_cmd

    GPIO_TYPE_INOUT_SD2_DATA_0       = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(20) | GPIO_SET_IN_SEL(2+40) | GPIO_SET_OUT_SEL(51),    //!< Input/Output type: sd2_data_out[0]
    GPIO_TYPE_INOUT_SD2_DATA_1       = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(21) | GPIO_SET_IN_SEL(2+41) | GPIO_SET_OUT_SEL(52),    //!< Input/Output type: sd2_data_out[1]
    GPIO_TYPE_INOUT_SD2_DATA_2       = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(22) | GPIO_SET_IN_SEL(2+42) | GPIO_SET_OUT_SEL(53),    //!< Input/Output type: sd2_data_out[2]
    GPIO_TYPE_INOUT_SD2_DATA_3       = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(23) | GPIO_SET_IN_SEL(2+43) | GPIO_SET_OUT_SEL(54),    //!< Input/Output type: sd2_data_out[3]
    GPIO_TYPE_INOUT_SD2_CMD          = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(24) | GPIO_SET_IN_SEL(2+44) | GPIO_SET_OUT_SEL(55),    //!< Input/Output : sd2_cmd

	GPIO_TYPE_INOUT_I80_DATA0        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+45) | GPIO_SET_OUT_SEL(93),    //!< Input/Output type: i80_data_rx0
	GPIO_TYPE_INOUT_I80_DATA1        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+46) | GPIO_SET_OUT_SEL(94),    //!< Input/Output type: i80_data_rx1
	GPIO_TYPE_INOUT_I80_DATA2        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+47) | GPIO_SET_OUT_SEL(95),    //!< Input/Output type: i80_data_rx2
	GPIO_TYPE_INOUT_I80_DATA3        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+48) | GPIO_SET_OUT_SEL(96),    //!< Input/Output type: i80_data_rx3
	GPIO_TYPE_INOUT_I80_DATA4        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+49) | GPIO_SET_OUT_SEL(97),    //!< Input/Output type: i80_data_rx4
	GPIO_TYPE_INOUT_I80_DATA5        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+50) | GPIO_SET_OUT_SEL(98),    //!< Input/Output type: i80_data_rx5
	GPIO_TYPE_INOUT_I80_DATA6        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+51) | GPIO_SET_OUT_SEL(99),    //!< Input/Output type: i80_data_rx6
	GPIO_TYPE_INOUT_I80_DATA7        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+52) | GPIO_SET_OUT_SEL(100),   //!< Input/Output type: i80_data_rx7
	GPIO_TYPE_INOUT_I80_DATA8        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(25) | GPIO_SET_IN_SEL(2+53) | GPIO_SET_OUT_SEL(101),   //!< Input/Output type: i80_data_rx8

    GPIO_TYPE_UNDEFINED              = 0,
} GPIO_TYPE_E;



//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************
typedef union { /* PLL_IOCTRL_GPIO */
    u32 all;
    struct {
        u32 io0                         : 6;
        u32                             : 2;
        u32 io1                         : 6;
        u32                             : 2;
        u32 io2                         : 6;
        u32                             : 2;
        u32 io3                         : 6;
        u32                             : 2;
    } bitc;
} GH_PLL_IOCTRL_GPIO_S;

typedef union { /* GPIO_OUTPUT_CFG */
    u32 all;
    struct {
        u32 out_sel                     : 7;
        u32                             : 1;
        u32 oen_sel                     : 6;
        u32 out_invert                  : 1;
        u32 oen_invert                  : 1;
        u32                             : 16;
    } bitc;
} GH_GPIO_OUTPUT_CFG_S;

typedef union { /* GPIO_INPUT_CFG */
    u32 all;
    struct {
        u32 in_sel                      : 6;
        u32                             : 26;
    } bitc;
} GH_GPIO_INPUT_CFG_S;

typedef union { /* GPIO_INT_EN */
    u32 all;
    struct {
        u32 int_en                      : 1;
        u32                             : 31;
    } bitc;
} GH_GPIO_INT_EN_S;

typedef struct gpio_cfg
{
    u32 gpio_count;
    GPIO_XREF_S gpio_chip[64];
    u32 extphy_gpio_count;
    GPIO_XREF_S ext_phy_gpio[16];
    u32 intphy_gpio_count;
    GPIO_XREF_S int_phy_gpio[16];

    u32 ir_led_ctl;
    u32 ir_cut1;
    u32 ir_cut2;

    u32 sensor_reset;

    u32 phy_reset;
    u32 phy_speed_led;

    u32 spi0_en0;
    u32 spi1_en0;

    u32 pwm0;
    u32 pwm1;
    u32 pwm2;
    u32 pwm3;

    u32 usb_host;

    u32 sd_detect;
    u32 sd_power;
    u32 sd1_detect;
    u32 sd1_power;

    //board info
    u32 soc_type;
    s8  board_type[32];
    u32 board_version;
    u32 reserve1;

    //extra device info
    u32 ext_phy_clk;
    u32 reserve2;

    // Add by Steven Yu:for pmu
    u32 pmu_ctl;

}gpio_cfg_t;



//*****************************************************************************
//*****************************************************************************
//** Global Data
//*****************************************************************************
//*****************************************************************************
extern gpio_cfg_t gk_all_gpio_cfg;
extern u8 cmdline_phytype;



//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif



#endif /* _GK_GPIO_V1_20_H_ */

