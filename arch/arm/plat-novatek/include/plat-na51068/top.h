/**
    TOP controller header

    Sets the pinmux of each module.

    @file       top.h
    @ingroup    mIDrvSys_TOP
    @note       Refer NA51068 data sheet for PIN/PAD naming

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _TOP_H
#define _TOP_H


#if defined(__FREERTOS)
/* rtos */
#include <kwrap/nvt_type.h>
#include <stdint.h>
#else
/* linux kernel */
#include <mach/nvt_type.h>
#endif


/**
    @addtogroup mIDrvSys_TOP
*/
//@{


/*
	Follow the naming rule for pinctrl tool parsing:
	1. The name of enum variable should be "PIN_xxx_CFG", such as PIN_UART_CFG
	2. The name of enum should be prefixed with "PIN_xxx_CFG", such as PIN_UART_CFG_CH0_1ST_PINMUX
	3. The value of enum should be less than 0x80000000, such as PIN_UART_CFG_CH0_1ST_PINMUX = 0x01
*/

typedef enum {
	PINMUX_DEBUGPORT_CKG =          0x0000,     ///< CKGen

	// TBD///////////////////////


	PINMUX_DEBUGPORT_GROUP_NONE =   0x0000,     ///< No debug port is output
	PINMUX_DEBUGPORT_GROUP1 =       0x0100,     ///< Output debug port to MC[18..0]
	PINMUX_DEBUGPORT_GROUP2 =       0x0200,     ///< Output debug port to LCD[18..0]

	ENUM_DUMMY4WORD(PINMUX_DEBUGPORT)
} PINMUX_DEBUGPORT;


typedef enum {
	PIN_UART_CFG_NONE,

	PIN_UART_CFG_CH0_1ST_PINMUX = 0x01, ///< Enable UART0_1. @ X_UART0_SOUT / X_UART0_SIN
	PIN_UART_CFG_CH0_CTSRTS 	= 0x02, ///< Enalbe UART0_1. @ X_UART0_CTS  / X_UART0_RTS

	PIN_UART_CFG_CH1_1ST_PINMUX = 0x10, ///< Enable UART1_1. @ X_UART1_SOUT / X_UART1_SIN
	PIN_UART_CFG_CH1_2ND_PINMUX = 0x20, ///< Enable UART1_2. @ X_I2C1_SCL   / X_I2C1_SDA
	PIN_UART_CFG_CH1_3RD_PINMUX = 0x40, ///< Enable UART1_3. @ X_GPIO_0     / X_GPIO_1

	PIN_UART_CFG_CH2_1ST_PINMUX = 0x100,///< Enable UART2_1. @ X_UART2_SOUT / X_UART2_SIN

	PIN_UART_CFG_CH3_1ST_PINMUX =0x1000,///< Enable UART3_1. @ X_CPU_TCK    / X_CPU_TDO
	PIN_UART_CFG_CH3_2ND_PINMUX =0x2000,///< Enable UART3_2. @ X_I2C0_SCL   / X_I2C0_SDA
	PIN_UART_CFG_CH3_3RD_PINMUX =0x4000,///< Enable UART3_3. @ X_I2C2_SCL   / X_I2C2_SDA
	PIN_UART_CFG_CH3_4TH_PINMUX =0x8000,///< Enable UART3_4. @ X_I2S1_SCLK  / X_I2S1_FS
	PIN_UART_CFG_CH3_5TH_PINMUX=0x10000,///< Enable UART3_5. @ X_GPIO_2     / X_GPIO_3
	PIN_UART_CFG_CH3_6TH_PINMUX=0x20000,///< Enable UART3_6. @ X_GPIO_7     / X_GPIO_8

	PIN_UART_CFG_CEVA_1ST_PINMUX=0x100000,///< Enable UART_DSP_1. @ X_UART1_SOUT / X_UART1_SIN

	ENUM_DUMMY4WORD(PIN_UART_CFG)
} PIN_UART_CFG;


typedef enum {
	PIN_I2C_CFG_NONE,

	PIN_I2C_CFG_CH0_1ST_PINMUX	= 0x01, 	///< Enable I2C0_1. @ X_I2C0_SCL / X_I2C0_SDA
	PIN_I2C_CFG_CH0_2ND_PINMUX	= 0x02, 	///< Enable I2C0_2. @ X_GPIO_0 / X_GPIO_1

	PIN_I2C_CFG_CH1_1ST_PINMUX	= 0x010, 	///< Enable I2C1_1. @ X_I2C1_SCL / X_I2C1_SDA
	PIN_I2C_CFG_CH1_2ND_PINMUX 	= 0x020, 	///< Enable I2C1_2. @ X_GPIO_2 / X_GPIO_3
	PIN_I2C_CFG_CH1_3RD_PINMUX 	= 0x040, 	///< Enable I2C1_3. @ X_GPIO_5 / X_GPIO_6
	PIN_I2C_CFG_CH1_4TH_PINMUX 	= 0x080, 	///< Enable I2C1_4. @ X_UART1_SOUT / X_UART1_SIN
	PIN_I2C_CFG_CH1_5TH_PINMUX 	= 0x100, 	///< Enable I2C1_5. @ X_CPU_TDI / X_CPU_TMS

	PIN_I2C_CFG_CH2_1ST_PINMUX	= 0x1000, 	///< Enable I2C2_1. @ X_I2C2_SCL / X_I2C2_SDA
	PIN_I2C_CFG_CH2_2ND_PINMUX	= 0x2000, 	///< Enable I2C2_2. @ X_I2S1_SCLK / X_I2S1_FS

	PIN_I2C_CFG_HDMI_1ST_PINMUX	= 0x10000,	///< Enable I2C_HDMITX. @ X_HDMI0_I2C_SCL / X_HDMI0_I2C_SDA

	ENUM_DUMMY4WORD(PIN_I2C_CFG)
} PIN_I2C_CFG;


typedef enum {
	PIN_SDIO_CFG_NONE,

	PIN_SDIO_CFG_1ST_PINMUX = 0x10,	///< 1st pinmux location; (X_SD0_DAT[3:0] / X_SD0_CD / X_SD0_CMD_RSP / X_SD0_CLK)@
									///< X_EXT1_CLK / X_I2C1_SDA / X_I2C1_SCL / X_I2S1_TXD / X_I2S1_SCLK / X_I2S1_FS / X_I2S1_RXD
	PIN_SDIO_CFG_2ND_PINMUX = 0x20,	///< 2nd pinmux location; (X_SD1_DAT[3:0] / X_SD1_CD / X_SD1_CMD_RSP / X_SD1_CLK)@
									///< X_I2C2_SDA / X_I2C2_SCL / X_GPIO_9 / X_GPIO_8 / X_GPIO_5 / X_GPIO_6 / X_GPIO_7

	ENUM_DUMMY4WORD(PIN_SDIO_CFG)
} PIN_SDIO_CFG;


typedef enum {
	PIN_SPI_CFG_NONE,

	PIN_SPI_CFG_CH0_1ST_PINMUX = 0x01,   ///< Enable SPI. @ X_SPI_SCK / X_SPI_CSn[0] / X_SPI_TX / X_SPI_RX / X_SPI_WPn / X_SPI_HOLDn / X_SPI_CSn[1]
	PIN_SPI_CFG_CH0_2ND_PINMUX = 0x02,   ///< Enable SPI. @ X_SPI_SCK / X_SPI_CSn[0] / X_SPI_SIO0 / X_SPI_SIO1 / X_SPI_SIO2 / X_SPI_SIO3 / X_SPI_CSn[1]
	PIN_SPI_CFG_CH0_3RD_PINMUX = 0x04,   ///< Enable SPI. @ X_SPI_SCK / X_SPI_CSn[0] / X_SPI_TX / X_SPI_RX

	ENUM_DUMMY4WORD(PIN_SPI_CFG)
} PIN_SPI_CFG;


typedef enum {
	PIN_EXTCLK_CFG_NONE,

	PIN_EXTCLK_CFG_CH0_1ST_PINMUX = 0x01,   ///< Enable EXT0CLK. @ X_EXT0_CLK
	PIN_EXTCLK_CFG_CH0_2ND_PINMUX = 0x02,   ///< Enable EXT0CLK. @ X_EXT1_CLK
	PIN_EXTCLK_CFG_CH0_3RD_PINMUX = 0x04,   ///< Enable EXT0CLK. @ X_CAP2_CLK

	PIN_EXTCLK_CFG_CH1_1ST_PINMUX = 0x10,   ///< Enable EXT1CLK. @ X_EXT1_CLK
	PIN_EXTCLK_CFG_CH1_2ND_PINMUX = 0x20,   ///< Enable EXT1CLK. @ X_EXT0_CLK
	PIN_EXTCLK_CFG_CH1_3RD_PINMUX = 0x40,   ///< Enable EXT1CLK. @ X_CAP1_CLK


	ENUM_DUMMY4WORD(PIN_EXTCLK_CFG)
} PIN_EXTCLK_CFG;


typedef enum {
	PIN_SSP_CFG_NONE,

	PIN_SSP_CFG_CH0_1ST_PINMUX		= 0x01,   	///< Enable I2S0. @ X_I2S0_SCLK / X_I2S0_FS / X_I2S0_RXD / X_I2S0_TXD
	PIN_SSP_CFG_CH0_2ND_PINMUX 		= 0x02,   	///< Enable I2S0. @ X_I2S1_SCLK / X_I2S1_FS / X_I2S1_RXD / X_I2S1_TXD

	PIN_SSP_CFG_CH1_1ST_PINMUX 		= 0x10,   	///< Enable I2S1. @ X_I2S1_SCLK / X_I2S1_FS / X_I2S1_RXD / X_I2S1_TXD

	PIN_SSP_CFG_CH2_1ST_PINMUX 		= 0x100,  	///< Enable I2S2. @ X_CPU_TDI / X_CPU_TMS / X_CPU_TCK / X_CPU_TDO
	PIN_SSP_CFG_CH2_2ND_PINMUX 		= 0x200,  	///< Enable I2S2. @ X_GPIO_6 / X_GPIO_7 / X_GPIO_8 / X_GPIO_9
	PIN_SSP_CFG_CH2_3RD_PINMUX 		= 0x400,  	///< Enable I2S2. @ X_I2C0_SCL / X_I2C0_SDA / X_I2S0_TXD
	PIN_SSP_CFG_CH2_4TH_PINMUX 		= 0x800,  	///< Enable I2S2. @ X_I2C1_SCL / X_I2C1_SDA / X_I2S1_TXD

	PIN_SSP_CFG_MCLK0_1ST_PINMUX	= 0x1000,	///< Enable I2S0_MCLK. @ X_EXT0_CLK

	PIN_SSP_CFG_MCLK1_1ST_PINMUX	= 0x10000,	///< Enable I2S1_MCLK. @ X_EXT1_CLK

	PIN_SSP_CFG_MCLK2_1ST_PINMUX	= 0x100000,	///< Enable I2S2_MCLK. @ X_GPIO_5
	PIN_SSP_CFG_MCLK2_2ND_PINMUX	= 0x200000, ///< Enable I2S2_MCLK. @ X_CPU_NTRST

	ENUM_DUMMY4WORD(PIN_SSP_CFG)
} PIN_SSP_CFG;


typedef enum {
	PIN_LCD_CFG_NONE,

	PIN_LCD_CFG_LCD310_RGB888_1ST_PINMUX = 0x001,   ///< Enable LCD310 RGB888. @ VCAP/i2c/i2s interface

	PIN_LCD_CFG_LCD310_BT1120_1ST_PINMUX = 0x010,   ///< Enable LCD310 BT1120. @ ethernet interface
	PIN_LCD_CFG_LCD310_BT1120_2ND_PINMUX = 0x020,   ///< Enable LCD310 BT1120. @ VCAP interface with X_CAP0_CLK as clock
	PIN_LCD_CFG_LCD210_BT1120_1ST_PINMUX = 0x040,   ///< Enable LCD210 BT1120. @ ethernet interface
	PIN_LCD_CFG_LCD210_BT1120_2ND_PINMUX = 0x080,   ///< Enable LCD210 BT1120. @ VCAP interface with X_CAP0_CLK as clock

	ENUM_DUMMY4WORD(PIN_LCD_CFG)
} PIN_LCD_CFG;


typedef enum {
	PIN_REMOTE_CFG_NONE,

	PIN_REMOTE_CFG_1ST_PINMUX  = 0x001,   ///< Enable Remote/IrDA. @ X_CPU_TDI
	PIN_REMOTE_CFG_2ND_PINMUX  = 0x002,   ///< Enable Remote/IrDA. @ X_CPU_TMS
	PIN_REMOTE_CFG_3RD_PINMUX  = 0x004,   ///< Enable Remote/IrDA. @ X_CPU_TCK
	PIN_REMOTE_CFG_4TH_PINMUX  = 0x008,   ///< Enable Remote/IrDA. @ X_CPU_TDO
	PIN_REMOTE_CFG_5TH_PINMUX  = 0x010,   ///< Enable Remote/IrDA. @ X_UART1_SOUT
	PIN_REMOTE_CFG_6TH_PINMUX  = 0x020,   ///< Enable Remote/IrDA. @ X_UART1_SIN
	PIN_REMOTE_CFG_7TH_PINMUX  = 0x040,   ///< Enable Remote/IrDA. @ X_GPIO_0
	PIN_REMOTE_CFG_8TH_PINMUX  = 0x080,   ///< Enable Remote/IrDA. @ X_GPIO_1
	PIN_REMOTE_CFG_9TH_PINMUX  = 0x100,   ///< Enable Remote/IrDA. @ X_GPIO_2
	PIN_REMOTE_CFG_10TH_PINMUX = 0x200,   ///< Enable Remote/IrDA. @ X_GPIO_3
	PIN_REMOTE_CFG_11TH_PINMUX = 0x400,   ///< Enable Remote/IrDA. @ X_GPIO_4

	ENUM_DUMMY4WORD(PIN_REMOTE_CFG)
} PIN_REMOTE_CFG;


typedef enum {
	PIN_VCAP_CFG_NONE,

	PIN_VCAP_CFG_CH0_1ST_PINMUX  = 0x001,   ///< Enable VCAP0. @ CAP0 CLK at X_CAP0_CLK
	PIN_VCAP_CFG_CH0_2ND_PINMUX  = 0x002,   ///< Enable VCAP0. @ CAP0 CLK at X_EXT1_CLK
	PIN_VCAP_CFG_TSI0_1ST_PINMUX = 0x004,   ///< Enable TSI0.  @ VCAP0 interface, CLK at X_VCAP0_CLK

	PIN_VCAP_CFG_CH1_1ST_PINMUX  = 0x010,   ///< Enable VCAP1. @ CAP1 CLK at X_CAP1_CLK
	PIN_VCAP_CFG_TSI1_1ST_PINMUX = 0x020,   ///< Enable TSI1.  @ VCAP1 interface

	PIN_VCAP_CFG_CH2_1ST_PINMUX  = 0x100,   ///< Enable VCAP2. @ CAP2 CLK at X_CAP2_CLK
	PIN_VCAP_CFG_CH2_2ND_PINMUX  = 0x200,   ///< Enable VCAP2. @ CAP2 CLK at X_CAP1_CLK
	PIN_VCAP_CFG_TSI2_1ST_PINMUX = 0x400,   ///< Enable TSI2.  @ VCAP2 interface

	PIN_VCAP_CFG_CH3_1ST_PINMUX  = 0x1000,  ///< Enable VCAP3. @ CAP3 CLK at X_CAP3_CLK
	PIN_VCAP_CFG_CH3_2ND_PINMUX  = 0x2000,  ///< Enable VCAP3. @ CAP3 CLK at X_I2C1_SCL
	PIN_VCAP_CFG_CH3_3RD_PINMUX  = 0x4000,  ///< Enable VCAP3. @ CAP3 CLK at X_CAP2_CLK
	PIN_VCAP_CFG_TSI3_1ST_PINMUX = 0x8000,  ///< Enable TSI3.  @ VCAP3 interface

	PIN_VCAP_CFG_BT1120_0_1ST_PINMUX = 0x10000,	 ///< Enable VCAP BT1120_0 @ CAP0_CLK + CAP0_D[7:0] + CAP1_D[7:0]
	PIN_VCAP_CFG_BT1120_0_2ND_PINMUX = 0x20000,  ///< Enable VCAP BT1120_0 @ CAP1_CLK + CAP0_D[7:0] + CAP1_D[7:0]
	PIN_VCAP_CFG_BT1120_0_3RD_PINMUX = 0x40000,  ///< Enable VCAP BT1120_0 @ EXT1CLK-as-CAP0CLK  + CAP0_D[7:0] + CAP1_D[7:0]

	PIN_VCAP_CFG_BT1120_1_1ST_PINMUX = 0x100000, ///< Enable VCAP BT1120_1 @ CAP2_CLK 				+ CAP2_D[7:0] + CAP3_D[7:0]
	PIN_VCAP_CFG_BT1120_1_2ND_PINMUX = 0x200000, ///< Enable VCAP BT1120_1 @ CAP3_CLK 				+ CAP2_D[7:0] + CAP3_D[7:0]
	PIN_VCAP_CFG_BT1120_1_3RD_PINMUX = 0x400000, ///< Enable VCAP BT1120_1 @ CAP1_CLK-as-CAP2CLK 	+ CAP2_D[7:0] + CAP3_D[7:0]
	PIN_VCAP_CFG_BT1120_1_4TH_PINMUX = 0x800000, ///< Enable VCAP BT1120_1 @ X_I2C1_SCL-as-CAP3CLK 	+ CAP2_D[7:0] + CAP3_D[7:0]
	PIN_VCAP_CFG_BT1120_1_5TH_PINMUX = 0x1000000,///< Enable VCAP BT1120_1 @ CAP2_CLK-as-CAP3CLK 	+ CAP2_D[7:0] + CAP3_D[7:0]

	ENUM_DUMMY4WORD(PIN_VCAP_CFG)
}PIN_VCAP_CFG;


typedef enum {
	PIN_ETH_CFG_NONE,

	PIN_ETH_CFG_RGMII_1ST_PINMUX  = 0x001,   ///< Enable RGMII. @ ETH interface
	PIN_ETH_CFG_RGMII_2ND_PINMUX  = 0x002,   ///< Enable RGMII. @ VCAP interface

	PIN_ETH_CFG_RMII_1ST_PINMUX   = 0x010,   ///< Enable RMII.  @ ETH interface
	PIN_ETH_CFG_RMII_2ND_PINMUX   = 0x020,   ///< Enable RMII.  @ VCAP interface

	PIN_ETH_CFG_PHY_ROUTE_ETH0    = 0x100,   ///< Select Embedded PHY route to ETH0 GMAC.
	PIN_ETH_CFG_PHY_ROUTE_ETH1    = 0x200,   ///< Select Embedded PHY route to ETH1 GMAC.

	PIN_ETH_CFG_EXTPHYCLK_1ST_PINMUX  = 0x1000,   ///< Output 25 MHz PHY CLK when RGMII/RMII @ ETH interface
	PIN_ETH_CFG_EXTPHYCLK_2ND_PINMUX  = 0x2000,   ///< Output 25 MHz PHY CLK when RGMII/RMII @ VCAP interface

	ENUM_DUMMY4WORD(PIN_ETH_CFG)
}PIN_ETH_CFG;


typedef enum {
	PIN_MISC_CFG_NONE,

	PIN_MISC_CFG_CPU_ICE 			= 0x00000001,///< Enable CPU ICE @ CPU ICE interface
	PIN_MISC_CFG_VGA_HS  			= 0x00000002,///< Enable VGA_HS
	PIN_MISC_CFG_VGA_VS  			= 0x00000004,///< Enable VGA_VS
	PIN_MISC_CFG_BMC                = 0x00000008,///< Enable BMC @ X_GPIO 5/6/7/8
	PIN_MISC_CFG_RTC_CAL_OUT		= 0x00000010,///< Enable X_RTC_CAL_OUT
	PIN_MISC_CFG_DAC_RAMP_TP		= 0x00000020,///< Enable DAC_RAMP_TP
	PIN_MISC_CFG_HDMI_HOTPLUG		= 0x00000040,///< Enable X_HDMI0_HPD


	PIN_MISC_CFG_DSP_ICE_1ST_PINMUX = 0x00010000,///< Enable DSP ICE @ CPU ICE interface
	PIN_MISC_CFG_DSP_ICE_2ND_PINMUX = 0x00020000,///< Enable DSP ICE @ I2S0 interface

	PIN_MISC_CFG_ETH_LED_1ST_PINMUX = 0x00100000,///< Enable ETH activity LED @ CPU ICE interface
	PIN_MISC_CFG_ETH_LED_2ND_PINMUX = 0x00200000,///< Enable ETH activity LED @ X_GPIO 8/9

	PIN_MISC_CFG_SATA_LED_1ST_PINMUX= 0x01000000,///< Enable SATA activity LED @ CPU ICE interface
	PIN_MISC_CFG_SATA_LED_2ND_PINMUX= 0x02000000,///< Enable SATA activity LED @ X_GPIO 3/4

	ENUM_DUMMY4WORD(PIN_MISC_CFG)
}PIN_MISC_CFG;


typedef enum {
	PIN_PWM_CFG_NONE,

	PIN_PWM_CFG_CH0_1ST_PINMUX = 0x01, ///< Enable PWM0_1. @ X_PWM0_OUT

	PIN_PWM_CFG_CH1_1ST_PINMUX = 0x10, ///< Enable PWM1_1. @ X_PWM1_OUT

	PIN_PWM_CFG_CH2_1ST_PINMUX = 0x100,///< Enable PWM2_1. @ X_I2S1_TXD

	ENUM_DUMMY4WORD(PIN_PWM_CFG)
} PIN_PWM_CFG;


typedef enum {
	PIN_VCAPINT_CFG_NONE,

	PIN_VCAPINT_CFG_CAP0_CFG_0    = 0x1,           ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP0_CFG_2    = 0x2,           ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP0_CFG_3    = 0x4,           ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP0_CFG_4    = 0x8,           ///< VCAP internal mux select

	PIN_VCAPINT_CFG_CAP1_CFG_0    = 0x100,         ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP1_CFG_2    = 0x200,         ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP1_CFG_3    = 0x400,         ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP1_CFG_4    = 0x800,         ///< VCAP internal mux select

	PIN_VCAPINT_CFG_CAP2_CFG_0    = 0x10000,       ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP2_CFG_1    = 0x20000,       ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP2_CFG_2    = 0x40000,       ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP2_CFG_3    = 0x80000,       ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP2_CFG_4    = 0x100000,      ///< VCAP internal mux select

	PIN_VCAPINT_CFG_CAP3_CFG_0    = 0x1000000,     ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP3_CFG_1    = 0x2000000,     ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP3_CFG_2    = 0x4000000,     ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP3_CFG_3    = 0x8000000,     ///< VCAP internal mux select
	PIN_VCAPINT_CFG_CAP3_CFG_4    = 0x10000000,    ///< VCAP internal mux select

	ENUM_DUMMY4WORD(PIN_VCAPINT_CFG)
}PIN_VCAPINT_CFG;


/**
    Function group

    @note For pinmux_init()
*/
typedef enum {
	PIN_FUNC_UART,      ///< UART. Configuration refers to PIN_UART_CFG.
	PIN_FUNC_I2C,       ///< I2C. Configuration refers to PIN_I2C_CFG.
	PIN_FUNC_SDIO,      ///< SDIO. Configuration refers to PIN_SDIO_CFG.
	PIN_FUNC_SPI,       ///< SPI. Configuration refers to PIN_SPI_CFG.
	PIN_FUNC_EXTCLK,    ///< EXTCLK. Configuration refers to PIN_EXTCLK_CFG
	PIN_FUNC_SSP,       ///< AUDIO/SSP. Configuration refers to PIN_SSP_CFG.
	PIN_FUNC_LCD,       ///< LCD interface. Configuration refers to PIN_LCD_CFG.
	PIN_FUNC_REMOTE,    ///< REMOTE. Configuration refers to PIN_REMOTE_CFG.
	PIN_FUNC_VCAP,      ///< VCAP. Configuration refers to PIN_VCAP_CFG
	PIN_FUNC_ETH,       ///< ETH. Configuration refers to PIN_ETH_CFG
	PIN_FUNC_MISC,      ///< MISC. Configuration refers to PIN_MISC_CFG
	PIN_FUNC_PWM,       ///< PWM. Configuration refers to PIN_PWM_CFG.
	PIN_FUNC_VCAPINT,   ///< VCAP INTERNAL MUX. Configuration refers to PIN_VCAPINT_CFG
	PIN_FUNC_MAX,

	ENUM_DUMMY4WORD(PIN_FUNC)
} PIN_FUNC;


/**
    Pinmux group

    @note For pinmux_init()
*/
typedef struct {
	PIN_FUNC    pin_function;    ///< PIN Function group
	UINT32      config;          ///< Configuration for pinFunction
} PIN_GROUP_CONFIG;


#if defined(__FREERTOS)
/* rtos */
struct nvt_pinctrl_info {
	PIN_GROUP_CONFIG top_pinmux[PIN_FUNC_MAX];
};
ER pinmux_init(struct nvt_pinctrl_info *info);
extern ER pinmux_config_uart(uint32_t config);
extern ER pinmux_config_i2c(uint32_t config);
extern ER pinmux_config_sdio(uint32_t config);
extern ER pinmux_config_spi(uint32_t config);
extern ER pinmux_config_extclk(uint32_t config);
extern ER pinmux_config_ssp(uint32_t config);
extern ER pinmux_config_lcd(uint32_t config);
extern ER pinmux_config_remote(uint32_t config);
extern ER pinmux_config_vcap(uint32_t config);
extern ER pinmux_config_eth(uint32_t config);
extern ER pinmux_config_misc(uint32_t config);
extern ER pinmux_config_pwm(uint32_t config);
extern ER pinmux_config_vcap_interal(uint32_t config);
#else
/* linux kernel */
int nvt_pinmux_update(PIN_GROUP_CONFIG *pinmux_config, int count);
int nvt_pinmux_capture(PIN_GROUP_CONFIG *pinmux_config, int count);
#endif

extern uint32_t pinmux_get_config(PIN_FUNC pinfunc);


//@}    //addtogroup mIHALSysCfg


#endif
