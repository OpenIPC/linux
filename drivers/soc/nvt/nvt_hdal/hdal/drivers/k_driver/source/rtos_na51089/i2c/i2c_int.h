/*
    I2C module internal header file

    I2C module internal header file.

    @file       i2c_int.h
    @ingroup    mIDrvIO_I2C
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _I2C_INT_H
#define _I2C_INT_H

#include "kdrv_i2c.h"
#include "i2c.h"


#define FLGPTN_I2C  FLGPTN_BIT(0)
#define FLGPTN_I2C2 FLGPTN_BIT(1)
#define FLGPTN_I2C3 FLGPTN_BIT(2)

/**
    @addtogroup mIDrvIO_I2C
*/
//@{

// In NT96650's proposal, I2C controller should output / input all possible
// pinmux by default. But NT96650A's input can be 1st pinmux or 2nd pinmux only.
// If both pinmux are assigned to I2C, only 1st pinmux's input will be passed
// to I2C controller. Thus, FW have to change pinmux dynamically.
// ENABLE   : FW change pinmux dynamically
// DISABLE  : FW won't change pinmux (Configured by project)
#define I2C_HANDLE_PINMUX       DISABLE

// I2C_TOTAL_SESSION must >= 1 and <= 32
STATIC_ASSERT((I2C_TOTAL_SESSION >= 1) &&(I2C_TOTAL_SESSION <= 32));

// Session allocated mask
#define I2C_SES_ALLOCATED_MASK  ((I2C_TOTAL_SESSION) == 32 ? 0xFFFFFFFF : (((UINT32)1 << (I2C_TOTAL_SESSION)) - 1))
#define I2C_SES_NOT_ALLOCATED   0x00000000      // Session is not allocted
#define I2C_SES_ALLOCATED       0x00000001      // Session is allocated

#define I2C_SES_NOT_LOCKED      0x00000000      // Session is not locked
#define I2C_SES_LOCKED          0x00000001      // Session is locked

#define I2C_SES_NOT_MODIFIED    0x00000000      // Session is not modified
#define I2C_SES_MODIFIED        0x00000001      // Session is modified

// I2C source clock
#if defined(_NVT_FPGA_)
// (OSC * 2) * 48 / 480 --> OSC / 5
#define I2C_SOURCE_CLOCK        (_FPGA_PLL_OSC_ / 5)
#else
#define I2C_SOURCE_CLOCK        48000000        // 48 MHz
#endif

// APB clock can be 48, 60, 80, 120 MHz. But we assume it's fixed at 80 MHz.
#if defined(_NVT_FPGA_) && defined(_NVT_EMULATION_)
// (OSC * 2) * 80 / 480 --> OSC / 3
#define I2C_APB_CLOCK           (_FPGA_PLL_OSC_ / 3)
#else
#define I2C_APB_CLOCK           80000000        // 80 MHz
#endif

// Timing & Bus clock register
#define I2C_CLKLOW_MIN          0x6
#define I2C_CLKLOW_MAX          0x3FF
#define I2C_CLKHIGH_MIN         0x2
#define I2C_CLKHIGH_MAX         0x3FF
#define I2C_TSR_MIN             0x1
#define I2C_TSR_MAX             0x3FF
#define I2C_GSR_MAX             0xF

// Timing & Bus clock register default value
#if defined(_NVT_FPGA_) && defined(_NVT_EMULATION_)
#define I2C_DEFAULT_GSR         0
#define I2C_DEFAULT_CLKLOW      1   // (400 KHz) *** not confirm yet ***
#define I2C_DEFAULT_CLKHIGH     2   // (400 KHz) *** not confirm yet ***
#define I2C_DEFAULT_TSR         1   // (400 KHz)
#else
#define I2C_DEFAULT_GSR         2
#define I2C_DEFAULT_CLKLOW      63  // (400 KHz) *** not confirm yet ***
#define I2C_DEFAULT_CLKHIGH     55  // (400 KHz) *** not confirm yet ***
#define I2C_DEFAULT_TSR         26  // (400 KHz)
#endif
#define I2C_DEFAULT_SCL_TIMEOUT 0x3FF

// Polling timeout
//#if defined(_NVT_FPGA_) && defined(_NVT_EMULATION_)
#if I2C_EMU_TEST
#define I2C_POLLING_TIMEOUT     0x01000000
#else
#define I2C_POLLING_TIMEOUT     0x001A0000
#endif

// I2C Control Register
#define I2C_CTRL_INTEN_MASK     0xBFF0
//#define I2C_CTRL_INTEN_MASK     0x3FF0
#define I2C_DMA_DIR_READ        0
#define I2C_DMA_DIR_WRITE       1

// I2C Status Register
// Bit 1 --> NACK status for slave mode
#define I2C_STATUS_NACK         0x02

// I2C SCL timeout value in ns (Not register value)
#define I2C_SCL_TIMEOUT_MIN     0x14D556        // 1,365,333 ns
#define I2C_SCL_TIMEOUT_MAX     0x53408000      // 1,396,736,000 ns

// I2C bus free time timeout in ns (Not register value)
#define I2C_BUSFREE_TIME_MIN    0x15            // 20.8 ns
#define I2C_BUSFREE_TIME_MAX    0x14CCCCD       // 21,810,360 + 20.8ns


#define I2C_BUSFREE_REG_MASK    0xFFFFF

// I2C SAR register
#define I2C_SAR_REG_MASK        0x3FF
#define I2C_SAR_7BIT_MASK       0x7F

// I2C Config (PIO)
#define I2C_ACCESS_PIO          0
#define I2C_ACCESS_DMA          1
#define I2C_NACK_GEN_SHIFT      4
#define I2C_START_GEN_SHIFT     8
#define I2C_STOP_GEN_SHIFT      12

// I2C Data
#define I2C_DATA_MASK           0xFF
#define I2C_DATA_SHIFT          8

// I2C DMA
#define I2C_DMA_SIZE_MIN        0x4
#define I2C_DMA_SIZE_MAX        0xFFFC
#define I2C_DMA_DB_MIN          0x2
#define I2C_DMA_DB_MAX          0xFF

// I2C VD Delay
#define I2C_VD_DELAY_CLOCK_MIN  0x0
#define I2C_VD_DELAY_CLOCK_MAX  0xFFFFFF

#define I2C_VD_DELAY_NS_MIN     0
#define I2C_VD_DELAY_NS_MAX     100000000 // 100ms

/*
    I2C configuration

    I2C configuration.
*/
typedef struct {
	I2C_MODE        Mode;
	I2C_SAR_MODE    SARMode;
#if (I2C_HANDLE_PINMUX == ENABLE)
	I2C_PINMUX      PinMux;
#endif
	UINT32          uiClkLowCnt;
	UINT32          uiClkHighCnt;
	UINT32          uiTSR;
	UINT32          uiGSR;
	UINT32          uiSCLTimeout;
	UINT32          uiBusFree;
	UINT32          uiSAR;
	BOOL            bHandleNACK;
	BOOL            bGeneralCall;
	BOOL            uiSCLTimeoutCheckEn;
	UINT32          uiVDSrc;
	BOOL            bVDsync;
	BOOL            bRCWrite;
	UINT32          uiSize2;
	UINT32          uiSize3;
	UINT32          uiDB1;
	UINT32          uiDB2;
	UINT32          uiDB3;
	UINT32          uiVDNumber;
	UINT32          uiVDDelay;
	UINT32          uiVDIntval;
	UINT32          uiVDIntval2;
	BOOL            bDescMode;
	UINT32			timeout_ms;
} I2C_CONFIG;

// Register default value
#define I2C_CTRL_REG_DEFAULT        0x00000002
#define I2C_STS_REG_DEFAULT         0x10000000
#define I2C_CONFIG_REG_DEFAULT      0x00000000
#define I2C_BUSCLK_REG_DEFAULT      0x00000000
#define I2C_DATA_REG_DEFAULT        0x00000000
#define I2C_SAR_REG_DEFAULT         0x00000000
#define I2C_TIMING_REG_DEFAULT      0x03FF1001
#define I2C_BUS_REG_DEFAULT         0x00000003
#define I2C_DMASIZE_REG_DEFAULT     0x00000000
#define I2C_DMAADDR_REG_DEFAULT     0x00000000
#define I2C_BUSFREE_REG_DEFAULT     0x000000E2
#define I2C_VD_SIZE_REG_DEFAULT     0x00000000
#define I2C_VD_CONTROL_REG_DEFAULT  0x01000000
#define I2C_VD_DELAY_REG_DEFAULT    0x00000000
#define I2C_DESC_INFO_DEFAULT       0x00000000

/*
    I2C register default value

    I2C register default value.
*/
typedef struct {
	UINT32  uiOffset;
	UINT32  uiValue;
	CHAR    *pName;
} I2C_REG_DEFAULT;
//@}

#endif
