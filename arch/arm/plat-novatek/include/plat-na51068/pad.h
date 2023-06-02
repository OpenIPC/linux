/**
    PAD controller header

    PAD controller header

    @file       pad.h
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _PAD_H
#define _PAD_H

#if defined(__FREERTOS)
/* rtos */
#include <kwrap/nvt_type.h>
#else
/* linux kernel */
#include <mach/nvt_type.h>
#endif


/**
    @addtogroup mIDrvSys_PAD
*/
//@{
typedef enum
{
	// 0x00 Pad Control PUPD Register 0
	// 0x80 Pad Control Driving/Sink Register 0
	PAD_X_CPU_NTRST = 0,
	PAD_X_CPU_TDI,
	PAD_X_CPU_TMS,
	PAD_X_CPU_TCK,
	PAD_X_CPU_TDO,
	PAD_X_SPI_SCK,
	PAD_X_SPI_CSN0,
	PAD_X_SPI_TX,
	PAD_X_SPI_RX,
	PAD_X_SPI_WPN,
	PAD_X_SPI_HOLDN,
	PAD_X_SPI_CSN1,
	PAD_X_SPI_RST,
	PAD_X_UART0_SOUT,
	PAD_X_UART0_SIN,
	PAD_X_UART0_RTS,

	// 0x04 Pad Control PUPD Register 1
	// 0x84 Pad Control Driving/Sink Register 1
	PAD_X_UART0_CTS,
	PAD_X_UART1_SOUT,
	PAD_X_UART1_SIN,
	PAD_X_UART2_SOUT,
	PAD_X_UART2_SIN,
	PAD_X_I2C0_SCL,
	PAD_X_I2C0_SDA,
	PAD_X_I2C1_SCL,
	PAD_X_I2C1_SDA,
	PAD_X_I2C2_SCL,
	PAD_X_I2C2_SDA,
	PAD_X_I2S0_SCLK,
	PAD_X_I2S0_FS,
	PAD_X_I2S0_RXD,
	PAD_X_I2S0_TXD,
	PAD_X_I2S1_SCLK,

	// 0x08 Pad Control PUPD Register 2
	// 0x88 Pad Control Driving/Sink Register 2
	PAD_X_I2S1_FS,
	PAD_X_I2S1_RXD,
	PAD_X_I2S1_TXD,
	PAD_X_EXT0_CLK,
	PAD_X_EXT1_CLK,
	PAD_X_RESET_OUT,
	PAD_X_PWM0_OUT,
	PAD_X_PWM1_OUT,

	// 0x10 Pad Control PUPD Register 3
	// 0x90 Pad Control Driving/Sink Register 3
	PAD_X_GPIO_0 = 64,
	PAD_X_GPIO_1,
	PAD_X_GPIO_2,
	PAD_X_GPIO_3,
	PAD_X_GPIO_4,
	PAD_X_GPIO_5,
	PAD_X_GPIO_6,
	PAD_X_GPIO_7,
	PAD_X_GPIO_8,
	PAD_X_GPIO_9,

	// 0x20 Pad Control PUPD Register 4
	// 0xA0 Pad Control Driving/Sink Register 4
	PAD_X_CAP0_CLK = 128,
	PAD_X_CAP0_DATA0,
	PAD_X_CAP0_DATA1,
	PAD_X_CAP0_DATA2,
	PAD_X_CAP0_DATA3,
	PAD_X_CAP0_DATA4,
	PAD_X_CAP0_DATA5,
	PAD_X_CAP0_DATA6,
	PAD_X_CAP0_DATA7,

	// 0x24 Pad Control PUPD Register 5
	// 0xA4 Pad Control Driving/Sink Register 5
	PAD_X_CAP1_CLK = 144,
	PAD_X_CAP1_DATA0,
	PAD_X_CAP1_DATA1,
	PAD_X_CAP1_DATA2,
	PAD_X_CAP1_DATA3,
	PAD_X_CAP1_DATA4,
	PAD_X_CAP1_DATA5,
	PAD_X_CAP1_DATA6,
	PAD_X_CAP1_DATA7,

	// 0x28 Pad Control PUPD Register 6
	// 0xA8 Pad Control Driving/Sink Register 6
	PAD_X_CAP2_CLK = 160,
	PAD_X_CAP2_DATA0,
	PAD_X_CAP2_DATA1,
	PAD_X_CAP2_DATA2,
	PAD_X_CAP2_DATA3,
	PAD_X_CAP2_DATA4,
	PAD_X_CAP2_DATA5,
	PAD_X_CAP2_DATA6,
	PAD_X_CAP2_DATA7,

	// 0x2C Pad Control PUPD Register 7
	// 0xAC Pad Control Driving/Sink Register 7
	PAD_X_CAP3_CLK = 176,
	PAD_X_CAP3_DATA0,
	PAD_X_CAP3_DATA1,
	PAD_X_CAP3_DATA2,
	PAD_X_CAP3_DATA3,
	PAD_X_CAP3_DATA4,
	PAD_X_CAP3_DATA5,
	PAD_X_CAP3_DATA6,
	PAD_X_CAP3_DATA7,

	// 0x40 Pad Control PUPD Register 8
	// 0xC0 Pad Control Driving/Sink Register 8
	PAD_X_EPHY0_REFCLK = 256,
	PAD_X_EPHY0_RESET,
	PAD_X_RGMII0_RX_CLK,
	PAD_X_RGMII0_RX_CTL,
	PAD_X_RGMII0_RXD0,
	PAD_X_RGMII0_RXD1,
	PAD_X_RGMII0_RXD2,
	PAD_X_RGMII0_RXD3,
	PAD_X_RGMII0_TX_CLK,
	PAD_X_RGMII0_TX_CTL,
	PAD_X_RGMII0_TXD0,
	PAD_X_RGMII0_TXD1,
	PAD_X_RGMII0_TXD2,
	PAD_X_RGMII0_TXD3,
	PAD_X_RGMII0_MDC,
	PAD_X_RGMII0_MDIO,

	// 0x4C Pad Control PUPD Register 9
	// 0xCC Pad Control Driving/Sink Register 9
	PAD_X_VGA_HS = 304,
	PAD_X_VGA_VS,
	PAD_X_HDMI0_I2C_SCL,
	PAD_X_HDMI0_I2C_SDA,
	PAD_X_HDMI0_HPD,

	PAD_PIN_MAX,
	ENUM_DUMMY4WORD(PAD)
} PAD_PIN;

/**
    Pad type select

    Pad type select

    Pad type value for pad_set_pull_updown(), pad_get_pull_updown().
*/
typedef enum {
	PAD_NONE                =    0x00,       ///< none of pull up/down
	PAD_PULLDOWN            =    0x01,       ///< pull down
	PAD_PULLUP              =    0x02,       ///< pull up
	PAD_KEEPER              =    0x03,       ///< keeper

	ENUM_DUMMY4WORD(PAD_PULL)
} PAD_PULL;

/**
    Pad driving select

    Pad driving select

    Pad driving value for pad_set_drivingsink(), pad_get_drivingsink().
*/
typedef enum {
	PAD_DRIVINGSINK_4MA     =    0x00,       ///< Pad driver/sink 4mA
	PAD_DRIVINGSINK_8MA     =    0x01,       ///< Pad driver/sink 8mA
	PAD_DRIVINGSINK_12MA    =    0x02,       ///< Pad driver/sink 12mA
	PAD_DRIVINGSINK_16MA    =    0x03,       ///< Pad driver/sink 16mA

	ENUM_DUMMY4WORD(PAD_DRIVINGSINK)
} PAD_DRIVINGSINK;

extern ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype);
extern ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype);
extern ER pad_set_drivingsink(PAD_PIN pin, PAD_DRIVINGSINK driving);
extern ER pad_get_drivingsink(PAD_PIN pin, PAD_DRIVINGSINK *driving);

//@}

#endif
