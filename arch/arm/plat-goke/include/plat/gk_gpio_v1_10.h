/*!
*****************************************************************************
** \file        arch/arm/plat-goke/include/plat/gk_gpio_v1_10.h
**
** \version     $Id: gk_gpio_v1_10.h 10728 2016-10-21 02:05:33Z yulindeng $
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
#ifndef _GK_GPIO_V1_10_H_
#define _GK_GPIO_V1_10_H_



//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************
/************************/
/* GPIO pins definition */
/************************/
#define GPIO_SET_OUT_SEL(n)     ((n)&0x7F)
#define GPIO_SET_IN_SEL(n)      (((n)&0x7F)<<7)
#define GPIO_SET_OEN_SEL(n)     (((n)&0x7F)<<14)
#define GPIO_SET_OUT_INVERT(n)  (((n)&0x1)<<22)
#define GPIO_SET_OEN_INVERT(n)  (((n)&0x1)<<23)
#define GPIO_SET_IOCTRL(n)      (((n)&0x3F)<<24)
#define GPIO_SET_FUNC(n)        (((n)&0x3)<<30)

#define GPIO_GET_OUT_SEL(n)     (((n)&0x0000007F))
#define GPIO_GET_IN_SEL(n)      (((n)&0x00003F80)>>7)
#define GPIO_GET_OEN_SEL(n)     (((n)&0x001FC000)>>14)
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
#define IOCTRL_10MA             0x03    //!< 10mA



//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************
typedef enum
{
    /* ----------------------------------- GPIO output function define ----------------------------------- */
    //GPIO_SET_OUT_SEL(  0)
    //GPIO_SET_OUT_SEL(  1)
    //GPIO_SET_OUT_SEL(  2)
    GPIO_TYPE_OUTPUT_JTAG_TDO        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  3) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(  3),     //!<  3.Output type: pad_jtag_tdo
    //GPIO_TYPE_INOUT_I2C_CLK                                                                                                                                             GPIO_SET_OEN_SEL(  4) | GPIO_SET_IN_SEL(  4) | GPIO_SET_OUT_SEL(  4),     //!<  4.Input/Output type : pad_sensor_scl
    //GPIO_TYPE_INOUT_I2C_DATA                                                                                                                                            GPIO_SET_OEN_SEL(  5) | GPIO_SET_IN_SEL(  5) | GPIO_SET_OUT_SEL(  5),     //!<  5.Input/Output type : pad_sensor_sda
    GPIO_TYPE_OUTPUT_SENSOR_RESET    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(  6),     //!<  6.Output type: pad_sensor_reset
    GPIO_TYPE_OUTPUT_SENSOR_POWEREN  = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(  7),     //!<  7.Output type: pad_sensor_poweren
    GPIO_TYPE_OUTPUT_LCD_DATA0       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(  8),     //!<  8.Output type: pad_lcd_data0
    GPIO_TYPE_OUTPUT_LCD_DATA1       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(  9),     //!<  9.Output type: pad_lcd_data1
    GPIO_TYPE_OUTPUT_LCD_DATA2       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 10),     //!< 10.Output type: pad_lcd_data2
    GPIO_TYPE_OUTPUT_LCD_DATA3       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 11),     //!< 11.Output type: pad_lcd_data3
    GPIO_TYPE_OUTPUT_LCD_DATA4       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 12),     //!< 12.Output type: pad_lcd_data4
    GPIO_TYPE_OUTPUT_LCD_DATA5       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 13),     //!< 13.Output type: pad_lcd_data5
    GPIO_TYPE_OUTPUT_LCD_DATA6       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 14),     //!< 14.Output type: pad_lcd_data6
    GPIO_TYPE_OUTPUT_LCD_DATA7       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 15),     //!< 15.Output type: pad_lcd_data7
    GPIO_TYPE_OUTPUT_LCD_DATA8       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 16),     //!< 16.Output type: pad_lcd_data8
    GPIO_TYPE_OUTPUT_LCD_DATA9       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 17),     //!< 17.Output type: pad_lcd_data9
    GPIO_TYPE_OUTPUT_LCD_DATA10      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 18),     //!< 18.Output type: pad_lcd_data10
    GPIO_TYPE_OUTPUT_LCD_DATA11      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 19),     //!< 19.Output type: pad_lcd_data11
    GPIO_TYPE_OUTPUT_LCD_DATA12      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 20),     //!< 20.Output type: pad_lcd_data12
    GPIO_TYPE_OUTPUT_LCD_DATA13      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 21),     //!< 21.Output type: pad_lcd_data13
    GPIO_TYPE_OUTPUT_LCD_DATA14      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 22),     //!< 22.Output type: pad_lcd_data14
    GPIO_TYPE_OUTPUT_LCD_DATA15      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 23),     //!< 23.Output type: pad_lcd_data15
    GPIO_TYPE_OUTPUT_LCD_DATA16      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 24),     //!< 24.Output type: pad_lcd_data16
    GPIO_TYPE_OUTPUT_LCD_DATA17      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 25),     //!< 25.Output type: pad_lcd_data17
    GPIO_TYPE_OUTPUT_LCD_DATA18      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 26),     //!< 26.Output type: pad_lcd_data18
    GPIO_TYPE_OUTPUT_LCD_DATA19      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 27),     //!< 27.Output type: pad_lcd_data19
    GPIO_TYPE_OUTPUT_LCD_DATA20      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 28),     //!< 28.Output type: pad_lcd_data20
    GPIO_TYPE_OUTPUT_LCD_DATA21      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 29),     //!< 29.Output type: pad_lcd_data21
    GPIO_TYPE_OUTPUT_LCD_DATA22      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 30),     //!< 30.Output type: pad_lcd_data22
    GPIO_TYPE_OUTPUT_LCD_DATA23      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 31),     //!< 31.Output type: pad_lcd_data23
    GPIO_TYPE_OUTPUT_LCD_DEN         = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 32),     //!< 32.Output type: pad_lcd_den
    GPIO_TYPE_OUTPUT_LCD_VSYNC       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 33),     //!< 33.Output type: pad_lcd_vsync
    GPIO_TYPE_OUTPUT_LCD_HSYNC       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 34),     //!< 34.Output type: pad_lcd_hsync
    GPIO_TYPE_OUTPUT_LCD_CLOCK       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_10MA) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 35),     //!< 35.Output type: pad_lcd_clock
    //GPIO_TYPE_INOUT_I2C_CLK2                                                                                                                                            GPIO_SET_OEN_SEL( 36) | GPIO_SET_IN_SEL( 36) | GPIO_SET_OUT_SEL( 36),     //!< 36.Input/Output type : pad_lcd_scl
    //GPIO_TYPE_INOUT_I2C_DATA2                                                                                                                                           GPIO_SET_OEN_SEL( 37) | GPIO_SET_IN_SEL( 37) | GPIO_SET_OUT_SEL( 37),     //!< 37.Input/Output type : pad_lcd_sda
    GPIO_TYPE_OUTPUT_LCD_CS          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 38),     //!< 38.Output type: pad_lcd_cs
    GPIO_TYPE_OUTPUT_LCD_RESET       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 39),     //!< 39.Output type: pad_lcd_reset
    GPIO_TYPE_OUTPUT_LCD_BL_EN       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 40),     //!< 40.Output type: pad_lcd_bl_en
    //GPIO_TYPE_INPUT_USB_OTG_ID                                                                                                                                          GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 41) | GPIO_SET_OUT_SEL( 41),     //!< 41.Input type: pad_usb_otg_id
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                           GPIO_SET_OEN_SEL( 42) | GPIO_SET_IN_SEL( 42) | GPIO_SET_OUT_SEL( 42),     //!< 42.Input/Output type : pad_sd0_d0
    //GPIO_TYPE_INOUT_SD_DATA_1                                                                                                                                           GPIO_SET_OEN_SEL( 43) | GPIO_SET_IN_SEL( 43) | GPIO_SET_OUT_SEL( 43),     //!< 43.Input/Output type : pad_sd0_d1
    //GPIO_TYPE_INOUT_SD_DATA_2                                                                                                                                           GPIO_SET_OEN_SEL( 44) | GPIO_SET_IN_SEL( 44) | GPIO_SET_OUT_SEL( 44),     //!< 44.Input/Output type : pad_sd0_d2
    //GPIO_TYPE_INOUT_SD_DATA_3                                                                                                                                           GPIO_SET_OEN_SEL( 45) | GPIO_SET_IN_SEL( 45) | GPIO_SET_OUT_SEL( 45),     //!< 45.Input/Output type : pad_sd0_d3
    GPIO_TYPE_OUTPUT_SDIO_CLK        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_REPEAT   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 46) | GPIO_SET_IN_SEL( 46) | GPIO_SET_OUT_SEL( 46),     //!< 46.Output type: pad_sd0_clk
    //GPIO_TYPE_INOUT_SD_CMD                                                                                                                                              GPIO_SET_OEN_SEL( 47) | GPIO_SET_IN_SEL( 47) | GPIO_SET_OUT_SEL( 47),     //!< 47.Input/Output type : pad_sd0_cmd
    GPIO_TYPE_INPUT_SD_CD_N          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 48) | GPIO_SET_IN_SEL( 48) | GPIO_SET_OUT_SEL( 48),     //!< 48.Output type: pad_sd0_cd
    //GPIO_TYPE_INPUT_SPI0_SI                                                                                                                                              GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 49) | GPIO_SET_OUT_SEL( 49),     //!< 49.Input type: pad_spi_si
    GPIO_TYPE_OUTPUT_SPI0_SO         = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 50) | GPIO_SET_IN_SEL( 50) | GPIO_SET_OUT_SEL( 50),     //!< 50.Output type: pad_spi_so
    GPIO_TYPE_OUTPUT_SPI0_CS0        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 51) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 51),     //!< 51.Output type: pad_spi_cs
    GPIO_TYPE_OUTPUT_SPI0_SCLK       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 52) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 52),     //!< 52.Output type: pad_spi_sclk
    //GPIO_TYPE_INPUT_UART0_RX                                                                                                                                            GPIO_SET_OEN_SEL( 53) | GPIO_SET_IN_SEL( 53) | GPIO_SET_OUT_SEL( 53),     //!< 53.Input type: pad_uart_rx
    GPIO_TYPE_OUTPUT_UART0_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 54) | GPIO_SET_IN_SEL( 54) | GPIO_SET_OUT_SEL( 54),     //!< 54.Output type: pad_uart_tx
    GPIO_TYPE_OUTPUT_GPS_POWER_EN    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 55),     //!< 55.Output type: pad_gps_power_en
    GPIO_TYPE_OUTPUT_GPS_TXD         = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 56) | GPIO_SET_IN_SEL( 56) | GPIO_SET_OUT_SEL( 56),     //!< 56.Output type: pad_gps_tx
    //GPIO_TYPE_INPUT_GPS_RXD                                                                                                                                             GPIO_SET_OEN_SEL( 57) | GPIO_SET_IN_SEL( 57) | GPIO_SET_OUT_SEL( 57),     //!< 57.Input type: pad_gps_rx
    GPIO_TYPE_OUTPUT_RD_POWER_EN     = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL( 58) | GPIO_SET_OUT_SEL( 58),     //!< 58.Output type: pad_rd_power_en
    //GPIO_TYPE_INPUT_RD_RXD                                                                                                                                              GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 59) | GPIO_SET_OUT_SEL( 59),     //!< 59.Input type: pad_rd_rx
    //GPIO_TYPE_INOUT_I2C_CLK3                                                                                                                                            GPIO_SET_OEN_SEL( 60) | GPIO_SET_IN_SEL( 60) | GPIO_SET_OUT_SEL( 60),     //!< 60.Input/Output type : pad_g_sen_sck
    //GPIO_TYPE_INOUT_I2C_DATA3                                                                                                                                           GPIO_SET_OEN_SEL( 61) | GPIO_SET_IN_SEL( 61) | GPIO_SET_OUT_SEL( 61),     //!< 61.Input/Output type : pad_g_sen_sda
    GPIO_TYPE_OUTPUT_SF_CS0          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 62) | GPIO_SET_IN_SEL( 62) | GPIO_SET_OUT_SEL( 62),     //!< 62.Output type: pad_sf_cs
    GPIO_TYPE_OUTPUT_PWM             = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL( 63) | GPIO_SET_OUT_SEL( 63),     //!< 63.Output type: pad_pwm
    GPIO_TYPE_OUTPUT_I2S_MCLK        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 64) | GPIO_SET_IN_SEL( 64) | GPIO_SET_OUT_SEL( 64),     //!< 64.Output type: pad_i2s_mclk
    //GPIO_TYPE_INOUT_I2S_SCLK                                                                                                                                            GPIO_SET_OEN_SEL( 65) | GPIO_SET_IN_SEL( 65) | GPIO_SET_OUT_SEL( 65),     //!< 65.Input/Output type : pad_i2s_sclk
    //GPIO_TYPE_INOUT_I2S_WS                                                                                                                                              GPIO_SET_OEN_SEL( 66) | GPIO_SET_IN_SEL( 66) | GPIO_SET_OUT_SEL( 66),     //!< 66.Input/Output type : pad_i2s_ws
    //GPIO_TYPE_INPUT_I2S_SDATA0                                                                                                                                          GPIO_SET_OEN_SEL( 67) | GPIO_SET_IN_SEL( 67) | GPIO_SET_OUT_SEL( 67),     //!< 67.Input type: pad_i2s_sdata0
    GPIO_TYPE_OUTPUT_I2S_SDATA1      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 68) | GPIO_SET_IN_SEL( 68) | GPIO_SET_OUT_SEL( 68),     //!< 68.Output type: pad_i2s_data1
    GPIO_TYPE_OUTPUT_SPI0_CS1        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 74) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 69),     //!< 69.Output type: ss_1_n
    GPIO_TYPE_OUTPUT_SPI0_CS4        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 74) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 70),     //!< 70.Output type: ss_4_n
    GPIO_TYPE_OUTPUT_SPI0_CS5        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 74) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 71),     //!< 71.Output type: ss_5_n
    GPIO_TYPE_OUTPUT_SPI0_CS6        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 74) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 72),     //!< 72.Output type: ss_6_n
    GPIO_TYPE_OUTPUT_SPI0_CS7        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 74) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 73),     //!< 73.Output type: ss_7_n
    GPIO_TYPE_OUTPUT_SS_TXD2         = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 74),     //!< 74.Output type: txd2
    GPIO_TYPE_OUTPUT_SCLK2           = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 75),     //!< 75.Output type: sclk2_out
    GPIO_TYPE_OUTPUT_RTS2            = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 76),     //!< 76.Output type: rts_2_n
    GPIO_TYPE_OUTPUT_DTR2            = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 77),     //!< 77.Output type: dtr_2_n
    GPIO_TYPE_OUTPUT_UART1_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 78),     //!< 78.Output type: UART_TX_2
    GPIO_TYPE_OUTPUT_UART2_TX        = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 79),     //!< 79.Output type: UART_TX_3
    //GPIO_SET_OUT_SEL( 80)
    //GPIO_SET_OUT_SEL( 81)
    GPIO_TYPE_OUTPUT_EPHY_LED_0      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 82),     //!< 82.Output type: enet_phy_led0
    GPIO_TYPE_OUTPUT_EPHY_LED_1      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 83),     //!< 83.Output type: enet_phy_led1
    GPIO_TYPE_OUTPUT_EPHY_LED_2      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 84),     //!< 84.Output type: enet_phy_led2
    GPIO_TYPE_OUTPUT_EPHY_LED_3      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 85),     //!< 85.Output type: enet_phy_led3
    GPIO_TYPE_OUTPUT_EPHY_LED_4      = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 86),     //!< 86.Output type: enet_phy_led4
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_0  = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 87),     //!< 87.Output type: enet_phy_txd0
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_1  = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 88),     //!< 88.Output type: enet_phy_txd1
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_2  = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 89),     //!< 89.Output type: enet_phy_txd2
    GPIO_TYPE_OUTPUT_ENET_PHY_TXD_3  = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 90),     //!< 90.Output type: enet_phy_txd3
    GPIO_TYPE_OUTPUT_ENET_PHY_TXER   = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 91),     //!< 91.Output type: enet_phy_txer
    GPIO_TYPE_OUTPUT_ENET_PHY_TXEN   = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 92),     //!< 92.Output type: enet_phy_txen
    //GPIO_TYPE_INOUT_ETH_MDIO                                                                                                                                            GPIO_SET_OEN_SEL( 69) | GPIO_SET_IN_SEL( 87) | GPIO_SET_OUT_SEL( 93),     //!< 93.Output type: enet_gmii_mdo_o
    GPIO_TYPE_OUTPUT_ENET_GMII_MDC_O = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 94),     //!< 94.Output type: enet_gmii_mdc_o
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_0    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 95),     //!< 95.Output type: ahd_dac_dr0
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_1    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 96),     //!< 96.Output type: ahd_dac_dr1
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_2    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 97),     //!< 97.Output type: ahd_dac_dr2
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_3    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 98),     //!< 98.Output type: ahd_dac_dr3
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_4    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL( 99),     //!< 99.Output type: ahd_dac_dr4
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_5    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(100),     //!<100.Output type: ahd_dac_dr5
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_6    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(101),     //!<101.Output type: ahd_dac_dr6
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_7    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(102),     //!<102.Output type: ahd_dac_dr7
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_8    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(103),     //!<103.Output type: ahd_dac_dr8
    GPIO_TYPE_OUTPUT_AHB_DAC_DR_9    = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(104),     //!<104.Output type: ahd_dac_dr9
    GPIO_TYPE_OUTPUT_I80_RESET       = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(105),     //!<105.Output type: i80_reset
    GPIO_TYPE_OUTPUT_I80_CS          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(106),     //!<106.Output type: i80_cs
    GPIO_TYPE_OUTPUT_I80_RD          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(107),     //!<107.Output type: i80_rd
    GPIO_TYPE_OUTPUT_I80_WR          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(108),     //!<108.Output type: i80_wr
    GPIO_TYPE_OUTPUT_I80_RS          = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(109),     //!<109.Output type: i80_rs
    //GPIO_TYPE_INOUT_I80_D8                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 16) | GPIO_SET_OUT_SEL(110),     //!<110.Output type: i80_d8
    //GPIO_TYPE_INOUT_I80_D7                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 15) | GPIO_SET_OUT_SEL(111),     //!<111.Output type: i80_d7
    //GPIO_TYPE_INOUT_I80_D6                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 14) | GPIO_SET_OUT_SEL(112),     //!<112.Output type: i80_d6
    //GPIO_TYPE_INOUT_I80_D5                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 13) | GPIO_SET_OUT_SEL(113),     //!<113.Output type: i80_d5
    //GPIO_TYPE_INOUT_I80_D4                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 12) | GPIO_SET_OUT_SEL(114),     //!<114.Output type: i80_d4
    //GPIO_TYPE_INOUT_I80_D3                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 11) | GPIO_SET_OUT_SEL(115),     //!<115.Output type: i80_d3
    //GPIO_TYPE_INOUT_I80_D2                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 10) | GPIO_SET_OUT_SEL(116),     //!<116.Output type: i80_d2
    //GPIO_TYPE_INOUT_I80_D1                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  9) | GPIO_SET_OUT_SEL(117),     //!<117.Output type: i80_d1
    //GPIO_TYPE_INOUT_I80_D0                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  8) | GPIO_SET_OUT_SEL(118),     //!<118.Output type: i80_d0
    GPIO_TYPE_INPUT_UART3_TX         = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(119),     //!<119.Output type: UART_ORC_TX
    //GPIO_SET_OUT_SEL(120)
    //GPIO_SET_OUT_SEL(121)
    //GPIO_SET_OUT_SEL(122)
    //GPIO_SET_OUT_SEL(123)
    //GPIO_SET_OUT_SEL(124)
    //GPIO_SET_OUT_SEL(125)
    GPIO_TYPE_OUTPUT_0               = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(126),     //!< Output type: value = 0
    GPIO_TYPE_OUTPUT_1               = GPIO_SET_FUNC(GPIO_FUNC_OUT)   | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(127),     //!< Output type: value = 1

    /* ----------------------------------- GPIO input function define ----------------------------------- */
    GPIO_TYPE_INPUT_JTAG_TRSTN       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  0) | GPIO_SET_IN_SEL(  0) | GPIO_SET_OUT_SEL(  0),     //!< 0. Input type: pad_jtag_trstn
    GPIO_TYPE_INPUT_JTAG_TMS         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  1) | GPIO_SET_IN_SEL(  1) | GPIO_SET_OUT_SEL(  0),     //!< 1. Input type: pad_jtag_tms
    GPIO_TYPE_INPUT_JTAG_TDI         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  2) | GPIO_SET_IN_SEL(  2) | GPIO_SET_OUT_SEL(  0),     //!< 2. Input type: pad_jtag_tdi
    //GPIO_SET_IN_SEL(  3)
    //GPIO_TYPE_INOUT_I2C_CLK                                                                                                                                             GPIO_SET_OEN_SEL(  4) | GPIO_SET_IN_SEL(  4) | GPIO_SET_OUT_SEL(  4),     //!< 4. Input/Output type : pad_sensor_scl
    //GPIO_TYPE_INOUT_I2C_DATA                                                                                                                                            GPIO_SET_OEN_SEL(  5) | GPIO_SET_IN_SEL(  5) | GPIO_SET_OUT_SEL(  5),     //!< 5. Input/Output type : pad_sensor_sda
    //GPIO_SET_IN_SEL(  6)
    //GPIO_SET_IN_SEL(  7)
    //GPIO_TYPE_INOUT_I80_D0                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  8) | GPIO_SET_OUT_SEL(118),     //!<118.Output type: i80_d0
    //GPIO_TYPE_INOUT_I80_D1                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  9) | GPIO_SET_OUT_SEL(117),     //!<117.Output type: i80_d1
    //GPIO_TYPE_INOUT_I80_D2                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 10) | GPIO_SET_OUT_SEL(116),     //!<116.Output type: i80_d2
    //GPIO_TYPE_INOUT_I80_D3                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 11) | GPIO_SET_OUT_SEL(115),     //!<115.Output type: i80_d3
    //GPIO_TYPE_INOUT_I80_D4                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 12) | GPIO_SET_OUT_SEL(114),     //!<114.Output type: i80_d4
    //GPIO_TYPE_INOUT_I80_D5                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 13) | GPIO_SET_OUT_SEL(113),     //!<113.Output type: i80_d5
    //GPIO_TYPE_INOUT_I80_D6                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 14) | GPIO_SET_OUT_SEL(112),     //!<112.Output type: i80_d6
    //GPIO_TYPE_INOUT_I80_D7                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 15) | GPIO_SET_OUT_SEL(111),     //!<111.Output type: i80_d7
    //GPIO_TYPE_INOUT_I80_D8                                                                                                                                              GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 16) | GPIO_SET_OUT_SEL(110),     //!<110.Output type: i80_d8
    //GPIO_SET_IN_SEL( 17)
    //GPIO_SET_IN_SEL( 18)
    //GPIO_SET_IN_SEL( 19)
    //GPIO_SET_IN_SEL( 20)
    //GPIO_SET_IN_SEL( 21)
    //GPIO_SET_IN_SEL( 22)
    //GPIO_SET_IN_SEL( 23)
    //GPIO_SET_IN_SEL( 24)
    //GPIO_SET_IN_SEL( 25)
    //GPIO_SET_IN_SEL( 26)
    //GPIO_SET_IN_SEL( 27)
    //GPIO_SET_IN_SEL( 28)
    //GPIO_SET_IN_SEL( 29)
    //GPIO_SET_IN_SEL( 30)
    //GPIO_SET_IN_SEL( 31)
    //GPIO_SET_IN_SEL( 32)
    //GPIO_SET_IN_SEL( 33)
    //GPIO_SET_IN_SEL( 34)
    //GPIO_SET_IN_SEL( 35)
    //GPIO_TYPE_INOUT_I2C_CLK2                                                                                                                                            GPIO_SET_OEN_SEL( 36) | GPIO_SET_IN_SEL( 36) | GPIO_SET_OUT_SEL( 36),     //!< 36.Input/Output type : pad_lcd_scl
    //GPIO_TYPE_INOUT_I2C_DATA2                                                                                                                                           GPIO_SET_OEN_SEL( 37) | GPIO_SET_IN_SEL( 37) | GPIO_SET_OUT_SEL( 37),     //!< 37.Input/Output type : pad_lcd_sda
    //GPIO_SET_IN_SEL( 38)
    //GPIO_SET_IN_SEL( 39)
    //GPIO_SET_IN_SEL( 40)
    GPIO_TYPE_INPUT_USB_OTG_ID       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_REPEAT   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 41) | GPIO_SET_OUT_SEL( 41),     //!< 41.Input type: pad_usb_otg_id
    //GPIO_TYPE_INOUT_SD_DATA_0                                                                                                                                           GPIO_SET_OEN_SEL( 42) | GPIO_SET_IN_SEL( 42) | GPIO_SET_OUT_SEL( 42),     //!< 42.Input/Output type : pad_sd0_d0
    //GPIO_TYPE_INOUT_SD_DATA_1                                                                                                                                           GPIO_SET_OEN_SEL( 43) | GPIO_SET_IN_SEL( 43) | GPIO_SET_OUT_SEL( 43),     //!< 43.Input/Output type : pad_sd0_d1
    //GPIO_TYPE_INOUT_SD_DATA_2                                                                                                                                           GPIO_SET_OEN_SEL( 44) | GPIO_SET_IN_SEL( 44) | GPIO_SET_OUT_SEL( 44),     //!< 44.Input/Output type : pad_sd0_d2
    //GPIO_TYPE_INOUT_SD_DATA_3                                                                                                                                           GPIO_SET_OEN_SEL( 45) | GPIO_SET_IN_SEL( 45) | GPIO_SET_OUT_SEL( 45),     //!< 45.Input/Output type : pad_sd0_d3
    //GPIO_TYPE_OUTPUT_SDIO_CLK                                                                                                                                           GPIO_SET_OEN_SEL( 46) | GPIO_SET_IN_SEL( 46) | GPIO_SET_OUT_SEL( 46),     //!< 46.Output type: pad_sd0_clk
    //GPIO_TYPE_INOUT_SD_CMD                                                                                                                                              GPIO_SET_OEN_SEL( 47) | GPIO_SET_IN_SEL( 47) | GPIO_SET_OUT_SEL( 47),     //!< 47.Input/Output type : pad_sd0_cmd
    //GPIO_TYPE_INPUT_SD_CD_N                                                                                                                                             GPIO_SET_OEN_SEL( 48) | GPIO_SET_IN_SEL( 48) | GPIO_SET_OUT_SEL( 48),     //!< 48.Output type: pad_sd0_cd
    GPIO_TYPE_INPUT_SPI0_SI          = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 49) | GPIO_SET_OUT_SEL( 49),     //!< 49.Input type: pad_spi_si
    //PIO_TYPE_OUTPUT_SPI_SO                                                                                                                                              GPIO_SET_OEN_SEL( 50) | GPIO_SET_IN_SEL( 50) | GPIO_SET_OUT_SEL( 50),     //!< 50.Output type: pad_spi_so
    //GPIO_SET_IN_SEL( 51)
    //GPIO_SET_IN_SEL( 52)
    GPIO_TYPE_INPUT_UART0_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 53) | GPIO_SET_IN_SEL( 53) | GPIO_SET_OUT_SEL( 53),     //!< 53.Input type: pad_uart_rx
    //GPIO_TYPE_OUTPUT_UART0_TX                                                                                                                                           GPIO_SET_OEN_SEL( 54) | GPIO_SET_IN_SEL( 54) | GPIO_SET_OUT_SEL( 54),     //!< 54.Output type: pad_uart_tx
    //GPIO_SET_IN_SEL( 55)
    //GPIO_TYPE_OUTPUT_GPS_TXD                                                                                                                                            GPIO_SET_OEN_SEL( 56) | GPIO_SET_IN_SEL( 56) | GPIO_SET_OUT_SEL( 56),     //!< 56.Output type: pad_gps_tx
    GPIO_TYPE_INPUT_GPS_RXD          = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 57) | GPIO_SET_IN_SEL( 57) | GPIO_SET_OUT_SEL( 57),     //!< 57.Input type: pad_gps_rx
    //GPIO_TYPE_OUTPUT_RD_POWER_EN                                                                                                                                        GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL( 58) | GPIO_SET_OUT_SEL( 58),     //!< 58.Output type: pad_rd_power_en
    GPIO_TYPE_INPUT_RD_RXD           = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 59) | GPIO_SET_OUT_SEL( 59),     //!< 59.Input type: pad_rd_rx
    //GPIO_TYPE_INOUT_I2C_CLK3                                                                                                                                            GPIO_SET_OEN_SEL( 60) | GPIO_SET_IN_SEL( 60) | GPIO_SET_OUT_SEL( 60),     //!< 60.Input/Output type : pad_g_sen_sck
    //GPIO_TYPE_INOUT_I2C_DATA3                                                                                                                                           GPIO_SET_OEN_SEL( 61) | GPIO_SET_IN_SEL( 61) | GPIO_SET_OUT_SEL( 61),     //!< 61.Input/Output type : pad_g_sen_sda
    //GPIO_TYPE_OUTPUT_SF_CS0                                                                                                                                             GPIO_SET_OEN_SEL( 62) | GPIO_SET_IN_SEL( 62) | GPIO_SET_OUT_SEL( 62),     //!< 62.Output type: pad_sf_cs
    //GPIO_TYPE_OUTPUT_PWM                                                                                                                                                GPIO_SET_OEN_SEL(126) | GPIO_SET_IN_SEL( 63) | GPIO_SET_OUT_SEL( 63),     //!< 63.Output type: pad_pwm
    //GPIO_TYPE_OUTPUT_I2S_MCLK                                                                                                                                           GPIO_SET_OEN_SEL( 64) | GPIO_SET_IN_SEL( 64) | GPIO_SET_OUT_SEL( 64),     //!< 64.Output type: pad_i2s_mclk
    //GPIO_TYPE_INOUT_I2S_SCLK                                                                                                                                            GPIO_SET_OEN_SEL( 65) | GPIO_SET_IN_SEL( 65) | GPIO_SET_OUT_SEL( 65),     //!< 65.Input/Output type : pad_i2s_sclk
    //GPIO_TYPE_INOUT_I2S_WS                                                                                                                                              GPIO_SET_OEN_SEL( 66) | GPIO_SET_IN_SEL( 66) | GPIO_SET_OUT_SEL( 66),     //!< 66.Input/Output type : pad_i2s_ws
    //GPIO_TYPE_INPUT_I2S_SDATA0                                                                                                                                          GPIO_SET_OEN_SEL( 67) | GPIO_SET_IN_SEL( 67) | GPIO_SET_OUT_SEL( 67),     //!< 67.Input type: pad_i2s_sdata0
    //GPIO_TYPE_OUTPUT_I2S_SDATA1                                                                                                                                         GPIO_SET_OEN_SEL( 68) | GPIO_SET_IN_SEL( 68) | GPIO_SET_OUT_SEL( 68),     //!< 68.Output type: pad_i2s_data1
    GPIO_TYPE_INPUT_TIMER1_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 69) | GPIO_SET_OUT_SEL(126),     //!< 69.Input type: tm11_clk
    GPIO_TYPE_INPUT_TIMER2_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 70) | GPIO_SET_OUT_SEL(126),     //!< 70.Input type: tm12_clk
    GPIO_TYPE_INPUT_TIMER3_CLK       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 71) | GPIO_SET_OUT_SEL(126),     //!< 71.Input type: tm13_clk
    GPIO_TYPE_INPUT_SD_DATA4         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 70) | GPIO_SET_IN_SEL( 72) | GPIO_SET_OUT_SEL(126),     //!< Input type: sd_data4_in
    GPIO_TYPE_INPUT_SD_DATA5         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 71) | GPIO_SET_IN_SEL( 73) | GPIO_SET_OUT_SEL(126),     //!< Input type: sd_data5_in
    GPIO_TYPE_INPUT_SD_DATA6         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 72) | GPIO_SET_IN_SEL( 74) | GPIO_SET_OUT_SEL(126),     //!< Input type: sd_data6_in
    GPIO_TYPE_INPUT_SD_DATA7         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 73) | GPIO_SET_IN_SEL( 75) | GPIO_SET_OUT_SEL(126),     //!< Input type: sd_data7_in
    GPIO_TYPE_INPUT_ENET_CLK_RX      = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 76) | GPIO_SET_OUT_SEL(126),     //!< 76.Input type: gpio_clk_rx_i
    GPIO_TYPE_INPUT_ENET_CLK_TX      = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 77) | GPIO_SET_OUT_SEL(126),     //!< 77.Input type: gpio_clk_tx_i
    GPIO_TYPE_INPUT_GPIO_RCT_CLK     = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 78) | GPIO_SET_OUT_SEL(126),     //!< 78.Input type: gpio_rct_clk_au
    GPIO_TYPE_INPUT_ENET_PHY_RXD_0   = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 79) | GPIO_SET_OUT_SEL(126),     //!< 79.Input type: gpio_phy_rxd0
    GPIO_TYPE_INPUT_ENET_PHY_RXD_1   = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 80) | GPIO_SET_OUT_SEL(126),     //!< 80.Input type: gpio_phy_rxd1
    GPIO_TYPE_INPUT_ENET_PHY_RXD_2   = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_REPEAT   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 81) | GPIO_SET_OUT_SEL(126),     //!< 81.Input type: gpio_phy_rxd2
    GPIO_TYPE_INPUT_ENET_PHY_RXD_3   = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 82) | GPIO_SET_OUT_SEL(126),     //!< 82.Input type: gpio_phy_rxd3
    GPIO_TYPE_INPUT_ENET_PHY_COL     = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 83) | GPIO_SET_OUT_SEL(126),     //!< 83.Input type: gpio_phy_cil_i
    GPIO_TYPE_INPUT_ENET_PHY_CRS     = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 84) | GPIO_SET_OUT_SEL(126),     //!< 84.Input type: gpio_phy_crs_i
    GPIO_TYPE_INPUT_ENET_PHY_RXER    = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 85) | GPIO_SET_OUT_SEL(126),     //!< 85.Input type: gpio_phy_rxer
    GPIO_TYPE_INPUT_ENET_PHY_RXDV    = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 86) | GPIO_SET_OUT_SEL(126),     //!< 86.Input type: gpio_phy_rxdv
    //GPIO_TYPE_INOUT_ETH_MDIO                                                                                                                                            GPIO_SET_OEN_SEL( 69) | GPIO_SET_IN_SEL( 87) | GPIO_SET_OUT_SEL( 93),     //!< 93.Output type: enet_gmii_mdo_o
    GPIO_TYPE_INPUT_UART1_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 88) | GPIO_SET_OUT_SEL(126),     //!< 88.Input type: UART_RX_2
    GPIO_TYPE_INPUT_UART2_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 89) | GPIO_SET_OUT_SEL(126),     //!< 89.Input type: UART_RX_3
    GPIO_TYPE_INPUT_SS_RXD2          = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 90) | GPIO_SET_OUT_SEL(126),     //!< 90.Input type: usrt3rx
    GPIO_TYPE_INPUT_SD_WP_N          = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 91) | GPIO_SET_OUT_SEL(126),     //!< 91.Input type: sdwp_n
    GPIO_TYPE_INPUT_SD_WP3           = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 92) | GPIO_SET_OUT_SEL(126),     //!< 92.Input type: sdwp_n3
    GPIO_TYPE_INPUT_UART3_RX         = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL( 93) | GPIO_SET_OUT_SEL(126),     //!< 93.Input type: UART_ORC_RX
    //GPIO_SET_IN_SEL( 94)
    //GPIO_SET_IN_SEL( 95)
    //GPIO_SET_IN_SEL( 96)
    //GPIO_SET_IN_SEL( 97)
    //GPIO_SET_IN_SEL( 98)
    //GPIO_SET_IN_SEL( 99)
    //GPIO_SET_IN_SEL(100)
    //GPIO_SET_IN_SEL(101)
    //GPIO_SET_IN_SEL(102)
    //GPIO_SET_IN_SEL(103)
    //GPIO_SET_IN_SEL(104)
    //GPIO_SET_IN_SEL(105)
    //GPIO_SET_IN_SEL(106)
    //GPIO_SET_IN_SEL(107)
    //GPIO_SET_IN_SEL(108)
    //GPIO_SET_IN_SEL(109)
    //GPIO_SET_IN_SEL(110)
    //GPIO_SET_IN_SEL(111)
    //GPIO_SET_IN_SEL(112)
    //GPIO_SET_IN_SEL(113)
    //GPIO_SET_IN_SEL(114)
    //GPIO_SET_IN_SEL(115)
    //GPIO_SET_IN_SEL(116)
    //GPIO_SET_IN_SEL(117)
    //GPIO_SET_IN_SEL(118)
    //GPIO_SET_IN_SEL(119)
    //GPIO_SET_IN_SEL(120)
    //GPIO_SET_IN_SEL(121)
    //GPIO_SET_IN_SEL(122)
    //GPIO_SET_IN_SEL(123)
    //GPIO_SET_IN_SEL(124)
    //GPIO_SET_IN_SEL(125)
    GPIO_TYPE_INPUT                  = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(127),     //!< Input type: normal input
    GPIO_TYPE_INPUT_0                = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL(126) | GPIO_SET_OUT_SEL(127),     //!< Input type: normal input
    GPIO_TYPE_INPUT_1                = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(127) | GPIO_SET_IN_SEL(127) | GPIO_SET_OUT_SEL(127),     //!< Input type: normal input

    /* ----------------------------------- GPIO input&&output function define ----------------------------------- */
    GPIO_TYPE_INOUT_I2C_CLK          = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  4) | GPIO_SET_IN_SEL(  4) | GPIO_SET_OUT_SEL(  4),     //!<  4.Input/Output type : pad_sensor_scl
    GPIO_TYPE_INOUT_I2C_DATA         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(  5) | GPIO_SET_IN_SEL(  5) | GPIO_SET_OUT_SEL(  5),     //!<  5.Input/Output type : pad_sensor_sda
    GPIO_TYPE_INOUT_I80_D0           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  8) | GPIO_SET_OUT_SEL(118),     //!<118.Output type: i80_d0
    GPIO_TYPE_INOUT_I80_D1           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL(  9) | GPIO_SET_OUT_SEL(117),     //!<117.Output type: i80_d1
    GPIO_TYPE_INOUT_I80_D2           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 10) | GPIO_SET_OUT_SEL(116),     //!<116.Output type: i80_d2
    GPIO_TYPE_INOUT_I80_D3           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 11) | GPIO_SET_OUT_SEL(115),     //!<115.Output type: i80_d3
    GPIO_TYPE_INOUT_I80_D4           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 12) | GPIO_SET_OUT_SEL(114),     //!<114.Output type: i80_d4
    GPIO_TYPE_INOUT_I80_D5           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 13) | GPIO_SET_OUT_SEL(113),     //!<113.Output type: i80_d5
    GPIO_TYPE_INOUT_I80_D6           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 14) | GPIO_SET_OUT_SEL(112),     //!<112.Output type: i80_d6
    GPIO_TYPE_INOUT_I80_D7           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 15) | GPIO_SET_OUT_SEL(111),     //!<111.Output type: i80_d7
    GPIO_TYPE_INOUT_I80_D8           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 75) | GPIO_SET_IN_SEL( 16) | GPIO_SET_OUT_SEL(110),     //!<110.Output type: i80_d8
    GPIO_TYPE_INOUT_I2C_CLK2         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 36) | GPIO_SET_IN_SEL( 36) | GPIO_SET_OUT_SEL( 36),     //!< 36.Input/Output type : pad_lcd_scl
    GPIO_TYPE_INOUT_I2C_DATA2        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 37) | GPIO_SET_IN_SEL( 37) | GPIO_SET_OUT_SEL( 37),     //!< 37.Input/Output type : pad_lcd_sda
    GPIO_TYPE_INOUT_SD_DATA_0        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 42) | GPIO_SET_IN_SEL( 42) | GPIO_SET_OUT_SEL( 42),     //!< 42.Input/Output type : pad_sd0_d0
    GPIO_TYPE_INOUT_SD_DATA_1        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 43) | GPIO_SET_IN_SEL( 43) | GPIO_SET_OUT_SEL( 43),     //!< 43.Input/Output type : pad_sd0_d1
    GPIO_TYPE_INOUT_SD_DATA_2        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 44) | GPIO_SET_IN_SEL( 44) | GPIO_SET_OUT_SEL( 44),     //!< 44.Input/Output type : pad_sd0_d2
    GPIO_TYPE_INOUT_SD_DATA_3        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_8MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 45) | GPIO_SET_IN_SEL( 45) | GPIO_SET_OUT_SEL( 45),     //!< 45.Input/Output type : pad_sd0_d3
    GPIO_TYPE_INOUT_SD_CMD           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 47) | GPIO_SET_IN_SEL( 47) | GPIO_SET_OUT_SEL( 47),     //!< 47.Input/Output type : pad_sd0_cmd
    GPIO_TYPE_INOUT_I2C_CLK3         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 60) | GPIO_SET_IN_SEL( 60) | GPIO_SET_OUT_SEL( 60),     //!< 60.Input/Output type : pad_g_sen_sck
    GPIO_TYPE_INOUT_I2C_DATA3        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP  |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 61) | GPIO_SET_IN_SEL( 61) | GPIO_SET_OUT_SEL( 61),     //!< 61.Input/Output type : pad_g_sen_sda
    GPIO_TYPE_INOUT_I2S_SCLK         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 65) | GPIO_SET_IN_SEL( 65) | GPIO_SET_OUT_SEL( 65),     //!< 65.Input/Output type : pad_i2s_sclk
    GPIO_TYPE_INOUT_I2S_WS           = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 66) | GPIO_SET_IN_SEL( 66) | GPIO_SET_OUT_SEL( 66),     //!< 66.Input/Output type : pad_i2s_ws
    GPIO_TYPE_INPUT_I2S_SDATA0       = GPIO_SET_FUNC(GPIO_FUNC_IN)    | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA ) | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 67) | GPIO_SET_IN_SEL( 67) | GPIO_SET_OUT_SEL( 67),     //!< 67.Input type: pad_i2s_sdata0
    GPIO_TYPE_INOUT_ETH_MDIO         = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_NORMAL   |IOCTRL_4MA ) | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 69) | GPIO_SET_IN_SEL( 87) | GPIO_SET_OUT_SEL( 93),     //!< 93.Output type: enet_gmii_mdo_o

    GPIO_TYPE_UNDEFINED              = 0,
} GPIO_TYPE_E;
/*
GPIO_TYPE_OUTPUT_SPI1_SO         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 2) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 2),    //!< Output type: tssi_txd
GPIO_TYPE_OUTPUT_SPI1_CS0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 3),    //!< Output type: tssi_cs0_n
GPIO_TYPE_OUTPUT_SPI1_SCLK       = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 4),    //!< Output type: tssi_sclk_out
GPIO_TYPE_OUTPUT_UART2_RTS_N     = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 5),    //!< Output type: uart2_rts_n
GPIO_TYPE_OUTPUT_UART2_DTR_N     = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL( 6),    //!< Output type: uart2_dtr_n
GPIO_TYPE_OUTPUT_PWM3_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(10),    //!< Output type: pwm3_out
GPIO_TYPE_OUTPUT_PWM2_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(11),    //!< Output type: pwm2_out
GPIO_TYPE_OUTPUT_PWM1_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(12),    //!< Output type: pwm1_out
GPIO_TYPE_OUTPUT_PWM0_OUT        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(13),    //!< Output type: pwm0_out
GPIO_TYPE_OUTPUT_AOMCLK          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(32),    //!< Output type: i2s_au_clk
GPIO_TYPE_OUTPUT_AOBCLK          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(17) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(33),    //!< Output type: i2s_clk_o
GPIO_TYPE_OUTPUT_AOLRCLK         = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(18) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(34),    //!< Output type: i2s_ws_o
GPIO_TYPE_OUTPUT_AO_DATA0        = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(35),    //!< Output type: i2s_so
GPIO_TYPE_OUTPUT_SF_CS1          = GPIO_SET_FUNC(GPIO_FUNC_OUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 0) | GPIO_SET_IN_SEL( 0)   | GPIO_SET_OUT_SEL(37),    //!< Output type: sf_cs1_n
GPIO_TYPE_INPUT_SPI1_SI          = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+ 0) | GPIO_SET_OUT_SEL( 0),    //!< Input type: tssi_rxd
GPIO_TYPE_INPUT_I2S_CLK          = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+23) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_clk
GPIO_TYPE_INPUT_I2S_WS           = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+24) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_ws
GPIO_TYPE_INPUT_I2S_SI           = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+25) | GPIO_SET_OUT_SEL( 0),    //!< Input type: i2s_si
GPIO_TYPE_INPUT_CLK_AU           = GPIO_SET_FUNC(GPIO_FUNC_IN) | GPIO_SET_IOCTRL(IOCTRL_PULL_DOWN|IOCTRL_2MA)    | GPIO_SET_OEN_INVERT(0) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL( 1) | GPIO_SET_IN_SEL(2+26) | GPIO_SET_OUT_SEL( 0),    //!< Input type: clk_au
GPIO_TYPE_INOUT_SD_DATA_4        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(12) | GPIO_SET_IN_SEL(2+18) | GPIO_SET_OUT_SEL(26),    //!< Input/Output type: sd_data_out[4]
GPIO_TYPE_INOUT_SD_DATA_5        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(13) | GPIO_SET_IN_SEL(2+19) | GPIO_SET_OUT_SEL(27),    //!< Input/Output type: sd_data_out[5]
GPIO_TYPE_INOUT_SD_DATA_6        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(14) | GPIO_SET_IN_SEL(2+20) | GPIO_SET_OUT_SEL(28),    //!< Input/Output type: sd_data_out[6]
GPIO_TYPE_INOUT_SD_DATA_7        = GPIO_SET_FUNC(GPIO_FUNC_INOUT) | GPIO_SET_IOCTRL(IOCTRL_PULL_UP|IOCTRL_2MA)   | GPIO_SET_OEN_INVERT(1) | GPIO_SET_OUT_INVERT(0) | GPIO_SET_OEN_SEL(15) | GPIO_SET_IN_SEL(2+21) | GPIO_SET_OUT_SEL(29),    //!< Input/Output type: sd_data_out[7]
*/

//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************
typedef union { /* PLL_IOCTRL_GPIO */
    u32 all;
    struct {
        u32 io1                         : 6;
        u32                             : 2;
        u32 io2                         : 6;
        u32                             : 2;
        u32 io0                         : 6;
        u32                             : 2;
        u32 io3                         : 6;
        u32                             : 2;
    } bitc;
} GH_PLL_IOCTRL_GPIO_S;

typedef union { /* GPIO_OUTPUT_CFG */
    u32 all;
    struct {
        u32 out_sel                     : 7;
        u32 oen_sel                     : 7;
        u32 out_invert                  : 1;
        u32 oen_invert                  : 1;
        u32                             : 16;
    } bitc;
} GH_GPIO_OUTPUT_CFG_S;

typedef union { /* GPIO_INPUT_CFG */
    u32 all;
    struct {
        u32 in_sel                      : 7;
        u32                             : 25;
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
    GPIO_XREF_S gpio_chip[128];
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



#endif /* _GK_GPIO_V1_10_H_ */

