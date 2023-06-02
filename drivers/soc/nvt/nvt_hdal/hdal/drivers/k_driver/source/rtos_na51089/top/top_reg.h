/*
	Pinmux module internal header file

	Pinmux module internal header file

	@file		top_reg.h
	@ingroup
	@note		Nothing

	Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _PINMUX_INT_H
#define _PINMUX_INT_H

#include <rcw_macro.h>
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000

//
//0x00 TOP Control Register 0
//
#define TOP_REG0_OFS                        0x00
union TOP_REG0 {
	uint32_t reg;
	struct {
	unsigned int BOOT_SRC:4;				// Boot Source Selection
	unsigned int EJTAG_CH_SEL:1;			// EJTAG channel select (EJTAG_1 or EJTAG_2)
	unsigned int EJTAG_SEL:1;				// EJTAG select
	unsigned int MPLL_CLKSELL:1;			// PLL clock output mux
	unsigned int reserved0:1;
	unsigned int ETH_MODE_SEL:2;			// eth mode select
	unsigned int DEBUG_MODE_SEL:2;		// MIPS debug mode select
	unsigned int WDT_FAIL_RESET_EN:1;	// WDT fail reset function enable
	unsigned int reserved2:19;
	} bit;
};

//
//0x04 TOP Control Register 1
//
#define TOP_REG1_OFS                        0x04
union TOP_REG1 {
	uint32_t reg;
	struct {
	unsigned int reserved0:13;
	unsigned int SPI_EXIST:1;			// SPI exist enable control
	unsigned int SDIO_EXIST:1;		// SDIO exist enable control
	unsigned int reserved1:1;
	unsigned int EXTROM_EXIST:1;		// EXT ROM exist enable control
	unsigned int SDIO3_EN:1;			// SDIO3 enable/disable control
	unsigned int SDIO2_EXIST:2;		// SDIO2 exist enable control
	unsigned int EJTAG_EN:1;			// EJTAG enable/disable control
	unsigned int reserved2:10;
	unsigned int SDIO3_BUS_WIDTH:1;  // Select SDIO3 bus width
	} bit;
};

//
//0x08 TOP Control Register 2
//
#define TOP_REG2_OFS                        0x08
union TOP_REG2 {
	uint32_t reg;
	struct {
	unsigned int LCD_TYPE:4;			// Pinmux of LCD interface
	unsigned int reserved0:2;
	unsigned int PLCD_DE:1;			// Pinmux of DE (for parallel LCD)
	unsigned int CCIR_DATA_WIDTH:1;	// CCIR data width
	unsigned int CCIR_HVLD_VVLD:1;	// CCIR VVLD, HVLD select
	unsigned int CCIR_FIELD:1;		// CCIR FIELD select
	unsigned int TE_SEL:1;				// Memory Interface TE select
	unsigned int reserved1:21;
	} bit;
};

//
//0x0c TOP Control Register 3
//
#define TOP_REG3_OFS                        0x0C
union TOP_REG3 {
	uint32_t reg;
	struct {
	unsigned int SENSOR:3;			// Pinmux of primary sensor interface
	unsigned int reserved0:1;
	unsigned int SENSOR2:2;			// Pinmux of second sensor interface
	unsigned int SEN_CCIR_VSHS:2;	// Pinmux of CCIR VS/HS/FIELD/PXCLK
	unsigned int SEN_MCLK:2;			// Pinmux of sensor MCLK
	unsigned int SEN_VSHS:2;			// Pinmux of sensor VS/HS
	unsigned int SEN_MCLK2:2;			// Pinmux of sensor MCLK2
	unsigned int SEN2_VSHS:2;			// Pinmux of sensor2 VS/HS
	unsigned int SN3_MCLK:1;			// Pinmux of sensor SN3_MCLK
	unsigned int reserved1:1;
	unsigned int SP_CLK:2;			// Pinmux of Special Clock
	unsigned int SN2_MCLK_SRC:1;		// Pinmux of sensor SN2_MCLK_SRC (from CG)
	unsigned int SN2_XVSHS_SRC:1;	// Pinmux of sensor SN2_XVSHS_SRC (TG)
	unsigned int reserved2:10;
	} bit;
};

//
//0x10 TOP Control Register 4
//
#define TOP_REG4_OFS                        0x10
union TOP_REG4 {
	uint32_t reg;
	struct {
	unsigned int reserved0:32;
	} bit;
};

//
//0x14 TOP Control Register 5
//
#define TOP_REG5_OFS                        0x14
union TOP_REG5 {
	uint32_t reg;
	struct {
        unsigned int I2C:2;			// Pinmux of I2C interface
        unsigned int ETH_MDIO:2;		// Pinmux of ETH MDIO
        unsigned int I2C2:2;			// Pinmux of I2C2 interface
        unsigned int I2C3:2;			// Pinmux of I2C3 interface
        unsigned int SPI:2;			// SPI pinmux pad select
        unsigned int SPI2:2;			// SPI2 pinmux pad select
        unsigned int SPI3:2;			// SPI3 pinmux pad select
        unsigned int SPI3_RDY:2;		// SPI3 ready pinmux pad select
        unsigned int SPI_DAT:1;		// SPI data select
        unsigned int SPI2_DAT:1;		// SPI2 data select
        unsigned int SPI3_DAT:1;		// SPI3 data select
        unsigned int reserved1:1;
        unsigned int SIFCH0:1;		// Pinmux of SIF channel 0
        unsigned int reserved2:1;
        unsigned int SIFCH1:1;		// Pinmux of SIF channel 1
        unsigned int reserved3:1;
        unsigned int SIFCH2:2;		// Pinmux of SIF channel 2
        unsigned int SIFCH3:1;		// Pinmux of SIF channel 3
        unsigned int reserved4:1;
        unsigned int SDP:2;			// Pinmux of Serial Data Port
        unsigned int ETH:2;			// Pinmux of ETH
	} bit;
};

//
//0x18 TOP Control Register 6
//
#define TOP_REG6_OFS                        0x18
union TOP_REG6 {
	uint32_t reg;
	struct {
	unsigned int AUDIO:2;			// Pinmux of external AUDIO interface
	unsigned int AUDIO_MCLK:2;	// AUDIO MCLK pinmux pad select
	unsigned int reserved0:1;
	unsigned int DM_DATA0:1;		//Digital MIC data 0 pinmux select
	unsigned int DM_DATA1:1;		//Digital MIC data 1 pinmux select
	unsigned int REMOTE:2;		// Pinmux of REMOTE
	unsigned int RTC_CLK:1;           // Pinmux of RTC_CLK
	unsigned int SP2_CLK:2;           // Pinmux of Special Clock 2
	unsigned int reserved1:1;
	unsigned int ETH_LED:2;		// Pinmux of ETH LED
	unsigned int RMII_INPUT:1;	// Pinmux of ETH RMII Mode
	unsigned int PWM8:3;			// Pinmux of PWM8
	unsigned int PWM9:3;			// Pinmux of PWM9
	unsigned int PWM10:3;			// Pinmux of PWM10
	unsigned int PWM11:3;			// Pinmux of PWM11
	unsigned int EXT_PHYCLK:1;	// Pinmux of ETH_EXT_PHY_CLK
	unsigned int reserved2:1;
	unsigned int DIGITAL_MIC:2;	//Digital MIC pinmux select
	} bit;
};

//
//0x1C TOP Control Register 7
//
#define TOP_REG7_OFS                        0x1C
union TOP_REG7 {
	uint32_t reg;
	struct {
	unsigned int PWM0:3;             	// Pinmux of PWM0
	unsigned int PWM1:3;             	// Pinmux of PWM1
	unsigned int PWM2:3;             	// Pinmux of PWM2
	unsigned int PWM3:3;             	// Pinmux of PWM3
	unsigned int PWM4:3;             	// Pinmux of PWM4
	unsigned int PWM5:3;             	// Pinmux of PWM5
	unsigned int PWM6:3;             	// Pinmux of PWM6
	unsigned int PWM7:3;             	// Pinmux of PWM7
	unsigned int REMOTE_EXT:2;	// Pinmux of REMOTE EXT
	unsigned int PI_CNT:2;		// Pinmux of PI_CNT
	unsigned int PI_CNT2:2;		// Pinmux of PI_CNT2
	unsigned int PI_CNT3:2;		// Pinmux of PI_CNT3
	} bit;
};

//
//0x20 TOP Control Register 8
//
#define TOP_REG8_OFS                        0x20
union TOP_REG8 {
	uint32_t reg;
	struct {
	unsigned int RAM_SRC_SELECT:1;	// RAM SRC Select
	unsigned int reserved0:15;
	unsigned int FAST_CH_SELECT:1;	// Fast boot mode channel select
	unsigned int PD_ISO_EN:1;
	unsigned int reserved1:14;
	} bit;
};

//
//0x24 TOP Control Register 9
//
#define TOP_REG9_OFS                        0x24
union TOP_REG9 {
	uint32_t reg;
	struct {
	unsigned int UART:1;			// Pinmux of UART
	unsigned int reserved0:1;
	unsigned int UART2:2;			// Pinmux of UART2
	unsigned int UART3:3;			// Pinmux of UART3
	unsigned int reserved1:1;
	unsigned int UART2_CTSRTS:2;	// Pinmux of UART2 CTS/RTS
	unsigned int UART3_CTSRTS:2;	// Pinmux of UART3 CTS/RTS
	unsigned int reserved2:20;
	} bit;
};

//
//0x28 TOP Control Register 10
//
#define TOP_REG10_OFS                        0x28
union TOP_REG10 {
	uint32_t reg;
	struct {
	unsigned int reserved0:32;
	} bit;
};

//
// 0xA0 CGPIO Control Register
//
#define TOP_REGCGPIO0_OFS                    0xA0
union TOP_REGCGPIO0 {
	uint32_t reg;
	struct {
	unsigned int CGPIO_0:1;          // pinmux of CGPIO 0
	unsigned int CGPIO_1:1;          // pinmux of CGPIO 1
	unsigned int CGPIO_2:1;          // pinmux of CGPIO 2
	unsigned int CGPIO_3:1;          // pinmux of CGPIO 3
	unsigned int CGPIO_4:1;          // pinmux of CGPIO 4
	unsigned int CGPIO_5:1;          // pinmux of CGPIO 5
	unsigned int CGPIO_6:1;          // pinmux of CGPIO 6
	unsigned int CGPIO_7:1;          // pinmux of CGPIO 7
	unsigned int CGPIO_8:1;          // pinmux of CGPIO 8
	unsigned int CGPIO_9:1;          // pinmux of CGPIO 9
	unsigned int CGPIO_10:1;         // pinmux of CGPIO 10
	unsigned int CGPIO_11:1;         // pinmux of CGPIO 11
	unsigned int CGPIO_12:1;         // pinmux of CGPIO 12
	unsigned int CGPIO_13:1;         // pinmux of CGPIO 13
	unsigned int CGPIO_14:1;         // pinmux of CGPIO 14
	unsigned int CGPIO_15:1;         // pinmux of CGPIO 15
	unsigned int CGPIO_16:1;         // pinmux of CGPIO 16
	unsigned int CGPIO_17:1;         // pinmux of CGPIO 17
	unsigned int CGPIO_18:1;         // pinmux of CGPIO 18
	unsigned int CGPIO_19:1;         // pinmux of CGPIO 19
	unsigned int CGPIO_20:1;         // pinmux of CGPIO 20
	unsigned int CGPIO_21:1;         // pinmux of CGPIO 21
	unsigned int CGPIO_22:1;         // pinmux of CGPIO 22
	unsigned int reserved0:9;
	} bit;
};

//
// 0xA8 PGPIO Control Register
//
#define TOP_REGPGPIO0_OFS                    0xA8
union TOP_REGPGPIO0 {
	uint32_t reg;
	struct {
	unsigned int PGPIO_0:1;          // pinmux of PGPIO 0
	unsigned int PGPIO_1:1;          // pinmux of PGPIO 1
	unsigned int PGPIO_2:1;          // pinmux of PGPIO 2
	unsigned int PGPIO_3:1;          // pinmux of PGPIO 3
	unsigned int PGPIO_4:1;          // pinmux of PGPIO 4
	unsigned int PGPIO_5:1;          // pinmux of PGPIO 5
	unsigned int PGPIO_6:1;          // pinmux of PGPIO 6
	unsigned int PGPIO_7:1;          // pinmux of PGPIO 7
	unsigned int PGPIO_8:1;          // pinmux of PGPIO 8
	unsigned int PGPIO_9:1;          // pinmux of PGPIO 9
	unsigned int PGPIO_10:1;         // pinmux of PGPIO 10
	unsigned int PGPIO_11:1;         // pinmux of PGPIO 11
	unsigned int PGPIO_12:1;         // pinmux of PGPIO 12
	unsigned int PGPIO_13:1;         // pinmux of PGPIO 13
	unsigned int PGPIO_14:1;         // pinmux of PGPIO 14
	unsigned int PGPIO_15:1;         // pinmux of PGPIO 15
	unsigned int PGPIO_16:1;         // pinmux of PGPIO 16
	unsigned int PGPIO_17:1;         // pinmux of PGPIO 17
	unsigned int PGPIO_18:1;         // pinmux of PGPIO 18
	unsigned int PGPIO_19:1;         // pinmux of PGPIO 19
	unsigned int PGPIO_20:1;         // pinmux of PGPIO 20
	unsigned int PGPIO_21:1;         // pinmux of PGPIO 21
	unsigned int PGPIO_22:1;         // pinmux of PGPIO 22
	unsigned int PGPIO_23:1;         // pinmux of PGPIO 23
	unsigned int PGPIO_24:1;         // pinmux of PGPIO 24
	unsigned int PGPIO_25:1;         // pinmux of PGPIO 25
	unsigned int reserved0:6;
	} bit;
};

//
// 0xB0 SGPIO Control Register
//
#define TOP_REGSGPIO0_OFS                    0xB0
union TOP_REGSGPIO0 {
	uint32_t reg;
	struct {
	unsigned int SGPIO_0:1;          // pinmux of SGPIO 0
	unsigned int SGPIO_1:1;          // pinmux of SGPIO 1
	unsigned int SGPIO_2:1;          // pinmux of SGPIO 2
	unsigned int SGPIO_3:1;          // pinmux of SGPIO 3
	unsigned int SGPIO_4:1;          // pinmux of SGPIO 4
	unsigned int SGPIO_5:1;          // pinmux of SGPIO 5
	unsigned int SGPIO_6:1;          // pinmux of SGPIO 6
	unsigned int SGPIO_7:1;          // pinmux of SGPIO 7
	unsigned int SGPIO_8:1;          // pinmux of SGPIO 8
	unsigned int SGPIO_9:1;          // pinmux of SGPIO 9
	unsigned int SGPIO_10:1;         // pinmux of SGPIO 10
	unsigned int SGPIO_11:1;         // pinmux of SGPIO 11
	unsigned int SGPIO_12:1;         // pinmux of SGPIO 12
	unsigned int reserved0:19;
	} bit;
};

//
// 0xB8 LGPIO Control Register
//
#define TOP_REGLGPIO0_OFS                    0xB8
union TOP_REGLGPIO0 {
	uint32_t reg;
	struct {
	unsigned int LGPIO_0:1;          // pinmux of LGPIO 0
	unsigned int LGPIO_1:1;          // pinmux of LGPIO 1
	unsigned int LGPIO_2:1;          // pinmux of LGPIO 2
	unsigned int LGPIO_3:1;          // pinmux of LGPIO 3
	unsigned int LGPIO_4:1;          // pinmux of LGPIO 4
	unsigned int LGPIO_5:1;          // pinmux of LGPIO 5
	unsigned int LGPIO_6:1;          // pinmux of LGPIO 6
	unsigned int LGPIO_7:1;          // pinmux of LGPIO 7
	unsigned int LGPIO_8:1;          // pinmux of LGPIO 8
	unsigned int LGPIO_9:1;          // pinmux of LGPIO 9
	unsigned int reserved0:22;
	} bit;
};

//
// 0xD0 DGPIO Control Register
//
#define TOP_REGDGPIO0_OFS                    0xD0
union TOP_REGDGPIO0 {
	uint32_t reg;
	struct {
	unsigned int DGPIO_0:1;         // pinmux of DGPIO 0
	unsigned int DGPIO_1:1;         // pinmux of DGPIO 1
	unsigned int DGPIO_2:1;         // pinmux of DGPIO 2
	unsigned int DGPIO_3:1;         // pinmux of DGPIO 3
	unsigned int DGPIO_4:1;         // pinmux of DGPIO 4
	unsigned int DGPIO_5:1;         // pinmux of DGPIO 5
	unsigned int DGPIO_6:1;         // pinmux of DGPIO 6
	unsigned int DGPIO_7:1;         // pinmux of DGPIO 7
	unsigned int DGPIO_8:1;         // pinmux of DGPIO 8
	unsigned int DGPIO_9:1;         // pinmux of DGPIO 9
	unsigned int DGPIO_10:1;        // pinmux of DGPIO 10	
	unsigned int reserved0:21;
	} bit;
};

//
// 0xD8 HSIGPIO Control Register
//
#define TOP_REGHGPIO0_OFS                    0xD8
union TOP_REGHGPIO0 {
	uint32_t reg;
	struct {
	unsigned int HSIGPIO_0:1;          // pinmux of HSIGPIO 0
	unsigned int HSIGPIO_1:1;          // pinmux of HSIGPIO 1
	unsigned int HSIGPIO_2:1;          // pinmux of HSIGPIO 2
	unsigned int HSIGPIO_3:1;          // pinmux of HSIGPIO 3
	unsigned int HSIGPIO_4:1;          // pinmux of HSIGPIO 4
	unsigned int HSIGPIO_5:1;          // pinmux of HSIGPIO 5
	unsigned int HSIGPIO_6:1;          // pinmux of HSIGPIO 6
	unsigned int HSIGPIO_7:1;          // pinmux of HSIGPIO 7
	unsigned int HSIGPIO_8:1;          // pinmux of HSIGPIO 8
	unsigned int HSIGPIO_9:1;          // pinmux of HSIGPIO 9
	unsigned int HSIGPIO_10:1;         // pinmux of HSIGPIO 10
	unsigned int HSIGPIO_11:1;         // pinmux of HSIGPIO 11
	unsigned int reserved0:20;
	} bit;
};

//
// 0xE0 AGPIO Control Register
//
#define TOP_REGAGPIO0_OFS                    0xE0
union TOP_REGAGPIO0 {
	uint32_t reg;
	struct {
	unsigned int AGPIO_0:1;          // pinmux of AGPIO 0
	unsigned int AGPIO_1:1;          // pinmux of AGPIO 1
	unsigned int AGPIO_2:1;          // pinmux of AGPIO 2
	unsigned int reserved0:29;
	} bit;
};


//
// 0xE8 DSIGPIO Control Register
//
#define TOP_REGDSIGPIO0_OFS                    0xE8
union TOP_REGDSIGPIO0 {
	uint32_t reg;
	struct {
	unsigned int DSIGPIO_0:1;          // pinmux of DSIGPIO 0
	unsigned int DSIGPIO_1:1;          // pinmux of DSIGPIO 1
	unsigned int DSIGPIO_2:1;          // pinmux of DSIGPIO 2
	unsigned int DSIGPIO_3:1;          // pinmux of DSIGPIO 3
	unsigned int DSIGPIO_4:1;          // pinmux of DSIGPIO 4
	unsigned int DSIGPIO_5:1;          // pinmux of DSIGPIO 5
	unsigned int DSIGPIO_6:1;          // pinmux of DSIGPIO 6
	unsigned int DSIGPIO_7:1;          // pinmux of DSIGPIO 7
	unsigned int DSIGPIO_8:1;          // pinmux of DSIGPIO 8
	unsigned int DSIGPIO_9:1;          // pinmux of DSIGPIO 9
	unsigned int DSIGPIO_10:1;         // pinmux of DSIGPIO 10
	unsigned int reserved0:21;
	} bit;
};

//
//0xF0 Version Code Register
//
#define TOP_VERSION_REG_OFS                 0xF0
union TOP_VERSION_REG {
	uint32_t reg;
	struct {
	unsigned int reserved0:8;
	unsigned int ROM_CODE_VERSION:4; // ROM Code Version
	unsigned int ECO_VERSION:4;      // ECO Version
	unsigned int CHIP_ID:16;         // Chip ID
	} bit;
};

//
//0xF8 OCP bus debug port sel
//
#define TOP_REG_REG_OCP_DEBUG_OFS           0xF8
union TOP_REG_REG_OCP_DEBUG {
	uint32_t reg;
	struct {
	unsigned int OCP_DEBUG_SEL:4;     // OCP debug port
	unsigned int OCP2_DEBUG_SEL:4;    // OCP2 debug port
	unsigned int APBTG_DEBUG_SEL:4;   // APBTG debug port
	unsigned int reserved0:4;
	unsigned int DSP_DEBUG_SEL:6;     // DSP debug port
	unsigned int reserved1:10;
	} bit;
};

//
//0xFC Debug Port Register
//
#define TOP_REG_DEBUG_OFS                   0xFC
union TOP_REG_DEBUG {
	uint32_t reg;
	struct {
	unsigned int DEBUG_SEL:7;  // Debug Port Selection
	unsigned int reserved0:1;
	unsigned int DEBUG_OUTSEL:2;  // Debug Port output select
	unsigned int reserved1:22;
	} bit;
};
#endif
