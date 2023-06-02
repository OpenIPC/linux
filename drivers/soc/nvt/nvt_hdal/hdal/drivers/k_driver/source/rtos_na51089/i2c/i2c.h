/**
    Header file for I2C/I2C2 module

    This file is the header file that defines the API and data type for I2C/I2C2 module.

    @file       i2c.h
    @ingroup    mIDrvIO_I2C
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _I2C_H
#define _I2C_H

// TODO
#define DESC_POLL_RST 0
// TODO END

#include "kdrv_i2c.h"
#include "comm/driver.h"
#include <stdlib.h>
#include <stdio.h>

#include "rcw_macro.h"
#include "io_address.h"
#include "interrupt.h"
#include "pad.h"
#include "top.h"
#include "gpio.h"
#include "pll.h"
#include "pll_protected.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/debug.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/type.h"
#include "kwrap/cpu.h"
#include "kwrap/util.h"

#include "string.h"

#include "dma.h"
#include "dma_protected.h"


#define I2C_EMU_TEST ENABLE

#define I2C_UPD_PAD_RDY DISABLE
#define I2C_GET_IC_VER_RDY DISABLE


/**
    @addtogroup mIDrvIO_I2C
*/
//@{

/**
    I2C session ID

    When I2C operates at master mode, I2C master can be transmit to multiple i2c-devices on the same I2C
    physical bus. This session id means one i2c-device on the bus. When the application user writes the
    driver for one i2c-device, the user should use i2c_open() to get the session id for your device.
    And then use this session id for the furthur i2c-device configurations/control.
    when the i2c_lock(I2C_SESSION) by the dedicated i2c driver session, the i2c configurations for that
    session would taking effect, such as I2C-BUS-Speed/Pinmux/... ,etc. So the user must notice that the
    i2c_setConfig(I2C_SESSION) configurations must be completed before i2c_lock(I2C_SESSION).
*/
typedef enum {
	I2C_SES0,                           ///< I2C session 0
	I2C_SES1,                           ///< I2C session 1
	I2C_SES2,                           ///< I2C session 2
	I2C_SES3,                           ///< I2C session 3
	I2C_SES4,                           ///< I2C session 4
	I2C_SES5,                           ///< I2C session 5

	I2C_TOTAL_SESSION,                  ///< I2C total session number
	ENUM_DUMMY4WORD(I2C_SESSION)
} I2C_SESSION, *PI2C_SESSION;

/**
    I2C Mode

    I2C operating mode. This is used in i2c_setConfig(I2C_SESSION, I2C_CONFIG_ID_MODE)
    to set the i2c controller as master or slave mode.
*/
typedef enum {
	I2C_MODE_SLAVE,                     ///< Slave mode
	I2C_MODE_MASTER,                    ///< Master mode (Controller will output SCL)

	ENUM_DUMMY4WORD(I2C_MODE)
} I2C_MODE, *PI2C_MODE;

/**
    I2C pinmux

    I2C pinmux. In the NT96680, one I2C controller may has two to four different pinout locations.
    This definition is used in i2c_setConfig(I2C_SESSION, I2C_CONFIG_ID_PINMUX) to assign the pinout location
    for dedicated I2C_SESSION.
*/
typedef enum {
	I2C_PINMUX_1ST,                     ///< 1st pinmux (I2C_1_XXX or I2C2_1_XXX)
	I2C_PINMUX_2ND,                     ///< 2nd pinmux (I2C_2_XXX or I2C2_2_XXX)
	I2C_PINMUX_3RD,                     ///< 3rd pinmux
	I2C_PINMUX_4TH,                     ///< 4th pinmux

	ENUM_DUMMY4WORD(I2C_PINMUX)
} I2C_PINMUX, *PI2C_PINMUX;

/**
    I2C slave addressing mode

    When I2C controller operates at slave mode,
    thie field is used in i2c_setConfig(I2C_SESSION, I2C_CONFIG_ID_SAR_MODE) to select the addressing mode is
    7-bits or 10-bits.
*/
typedef enum {
	I2C_SAR_MODE_7BITS,                 ///< 7 bits slave addressing mode
	I2C_SAR_MODE_10BITS,                ///< 10 bits slave addressing mode

	ENUM_DUMMY4WORD(I2C_SAR_MODE)
} I2C_SAR_MODE, *PI2C_SAR_MODE;

/**
    I2C slave address length

    When I2C controller operates at slave mode, this is used in i2c_waitSARMatch() to wait for the
    slave address matched event.
*/
typedef enum {
	I2C_SAR_LEN_1BYTE = 1,              ///< 1 byte  Slave Address
	I2C_SAR_LEN_2BYTES,                 ///< 2 bytes Slave Address

	ENUM_DUMMY4WORD(I2C_SAR_LEN)
} I2C_SAR_LEN, *PI2C_SAR_LEN;

/**
    I2C master mode bus clock

    I2C master mode pre-defined bus clock. This is used in i2c_setConfig(I2C_SESSION, I2C_CONFIG_ID_BUSCLOCK).
*/
typedef enum {
	I2C_BUS_CLOCK_50KHZ     =   50000,  ///< 50 KHz
	I2C_BUS_CLOCK_100KHZ    =  100000,  ///< 100 KHz
	I2C_BUS_CLOCK_200KHZ    =  200000,  ///< 200 KHz
	I2C_BUS_CLOCK_400KHZ    =  400000,  ///< 400 KHz
	I2C_BUS_CLOCK_1MHZ      = 1000000,  ///< 1 MHz

	ENUM_DUMMY4WORD(I2C_BUS_CLOCK)
} I2C_BUS_CLOCK, *PI2C_BUS_CLOCK;

/**
    I2C transmit/receive byte counter for PIO mode

    I2C transmit/receive byte counter for PIO mode.
*/
typedef enum {
	I2C_BYTE_CNT_1 = 1,
	I2C_BYTE_CNT_2,
	I2C_BYTE_CNT_3,
	I2C_BYTE_CNT_4,
	I2C_BYTE_CNT_5,
	I2C_BYTE_CNT_6,
	I2C_BYTE_CNT_7,
	I2C_BYTE_CNT_8,

	ENUM_DUMMY4WORD(I2C_BYTE_CNT)
} I2C_BYTE_CNT, *PI2C_BYTE_CNT;

/**
    I2C transmit/receive byte parameter for PIO mode

    I2C transmit/receive byte parameter for PIO mode.
*/
typedef enum {
	I2C_BYTE_PARAM_NONE     = 0x00000000,   ///< Dummy, for slave transmit or master transmit without START or STOP
	I2C_BYTE_PARAM_ACK      = 0x00000000,   ///< Receive mode only, response ACK after data is received
	I2C_BYTE_PARAM_NACK     = 0x00000001,   ///< Receive mode only, response NACK after data is received
	I2C_BYTE_PARAM_START    = 0x00000004,   ///< Master mode only, generate START condition before transmit data
	I2C_BYTE_PARAM_STOP     = 0x00000008,   ///< Master mode only, generate STOP condition after data is transmitted or received
	///< @note  Can't generate STOP and START at the same byte

	ENUM_DUMMY4WORD(I2C_BYTE_PARAM)
} I2C_BYTE_PARAM, *PI2C_BYTE_PARAM;


/**
    SIE VD source

    SIE VD source  for SIF_CONFIG_ID_BURST_SRC.
*/
typedef enum {
	I2C_VD_SIE1,            ///< VD source from SIE channel 1
	I2C_VD_SIE2,            ///< VD source from SIE channel 2, 520/560 max
	I2C_VD_SIE3,            ///< VD source from SIE channel 3
	I2C_VD_SIE4,            ///< VD source from SIE channel 4
	I2C_VD_SIE5,            ///< VD source from SIE channel 5, 528 max
	ENUM_DUMMY4WORD(I2C_VD_SOURCE)
} I2C_VD_SOURCE;


/**
    I2C VD Sync mode

    The Number of DMA transaction will synchronize with the following video frame start bit (VD signal)
*/
typedef enum {
	I2C_VD_NUMBER_NONE = 0,   ///< Reserved
	I2C_VD_NUMBER_1,          ///< synchronize with the following 1 VD signal
	I2C_VD_NUMBER_2,          ///< synchronize with the following 2 VD signal
	I2C_VD_NUMBER_3,          ///< synchronize with the following 3 VD signal

	ENUM_DUMMY4WORD(I2C_VD_NUMBER)
} I2C_VD_NUMBER, *PI2C_VD_NUMBER;


/**
    I2C VD Sync Interval

    The interval of DMA VD sync transaction between DMA_TRANS_G1/2 and DMA_TRANS_G2/3 (in VD signal unit).
*/
typedef enum {
	I2C_VD_INTVAL_NONE = 0,
	I2C_VD_INTVAL_1,
	I2C_VD_INTVAL_2,
	I2C_VD_INTVAL_3,
	I2C_VD_INTVAL_4,
	I2C_VD_INTVAL_5,
	I2C_VD_INTVAL_6,
	I2C_VD_INTVAL_7,

	ENUM_DUMMY4WORD(I2C_VD_INTVAL)
} I2C_VD_INTVAL, *PI2C_VD_INTVAL;

/**
    I2C transmit/receive byte data and parameter for PIO mode

    I2C transmit/receive byte data and parameter for PIO mode.
*/
typedef struct {
	UINT32          uiValue;                ///< 8-bits data to transmit or receive
	I2C_BYTE_PARAM  Param;                  ///< Parameter to transmit or receive data
} I2C_BYTE, *PI2C_BYTE;

/**
    I2C transmit / receive parameter for PIO mode

    I2C transmit / receive parameter for PIO mode.
*/
typedef struct {
	DRV_VER_INFO    VersionInfo;            ///< Driver version
	I2C_BYTE_CNT    ByteCount;              ///< How many bytes will be transmitted / received
	I2C_BYTE       *pByte;                  ///< Byte data and parameter
} I2C_DATA, *PI2C_DATA;

/**
    I2C Configuration ID

    I2C Configuration ID for i2c_setConfig() and i2c_getConfig()
*/
typedef enum {
	I2C_CONFIG_ID_MODE,             ///< Mast or Slave mode, This is channel independent setting. Please config once after power on.
	///< - @b I2C_MODE_SLAVE    : Slave mode
	///< - @b I2C_MODE_MASTER   : Master mode (Default value)

	I2C_CONFIG_ID_BUSCLOCK,         ///< Master mode bus clock. Please use pre-defined bus clock if possible.
	///< - @b I2C_BUS_CLOCK_50KHZ
	///< - @b I2C_BUS_CLOCK_100KHZ
	///< - @b I2C_BUS_CLOCK_200KHZ
	///< - @b I2C_BUS_CLOCK_400KHZ
	///< - @b I2C_BUS_CLOCK_1MHZ
	///< @note  Valid value: 50,000 ~ 1,000,000 , unit: Hz.
	///<        Real bus clock might lower than your setting.
	///<        Please call i2c_getConfig(I2C_CONFIG_ID_BUSCLOCK) to confirm.

	I2C_CONFIG_ID_SCL_TIMEOUT,      ///< SCL timeout value. When SCL is kept low over this value, an event will be issued.
	///< @note  Valid value: 819,200 ~ 838,041,600 , unit: ns. Default: 838,041,600 ns.
	///<        Real timeout might greater than your setting.
	///<        Please call i2c_getConfig(I2C_CONFIG_ID_SCL_TIMEOUT) to confirm.

	I2C_CONFIG_ID_BUSFREE_TIME,     ///< Bus free time between STOP and START condition.
	///< @note  Valid value: 20 ~ 10,645 , unit: ns. Default: 4708 ns.
	///<        Real timeout might greater than your setting.
	///<        Please call i2c_getConfig(I2C_CONFIG_ID_BUSFREE_TIME) to confirm.

	I2C_CONFIG_ID_PINMUX,           ///< Select pinmux
	///< - @b I2C_PINMUX_1ST (Default value)
	///< - @b I2C_PINMUX_2ND
	///< - @b I2C_PINMUX_3RD
	///< - @b I2C_PINMUX_4TH

	I2C_CONFIG_ID_HANDLE_NACK,      ///< Handle NACK from slave
	///< - @b TRUE  : Send STOP and back to idle (Default value)
	///< - @b FALSE : Do nothing

	I2C_CONFIG_ID_SAR,              ///< Slave mode address
	///< @note  Please refer to I2C specification for valid slave address

	I2C_CONFIG_ID_SAR_MODE,         ///< I2C slave addressing mode
	///< - @b I2C_SAR_MODE_7BITS    : 7 bits addressing mode (Default value)
	///< - @b I2C_SAR_MODE_10BITS   : 10 bits addressing mode

	I2C_CONFIG_ID_GC,               ///< Response general call at slave mode
	///< - @b TRUE  : Response general call
	///< - @b FALSE : Don't response general call (Default value)

	I2C_CONFIG_ID_CLKCNT_L = 0x1000,// Master mode bus clock low counter
	// Formula: SCL = 48MHz / (CLKCNT_L + CLKCNT_H + GSR + "Clock Rising Time")
	// @note    Reserved for engineering

	I2C_CONFIG_ID_CLKCNT_H,         // Master mode bus clock high counter
	// Formula: SCL = 48MHz / (CLKCNT_L + CLKCNT_H + GSR + "Clock Rising Time")
	// @note    Reserved for engineering

	I2C_CONFIG_ID_TSR,              // Drive SDA after (TSR + 4) clock cycles when SCL goes low
	// @note    Reserved for engineering

	I2C_CONFIG_ID_GSR,              // Suppress glitch <= GSR clock cycles for SDA and SCL
	// @note    Reserved for engineering

	I2C_CONFIG_ID_SCLTIMEOUT_CHECK, ///< Enable/Disable the SCL Timeout Check. Default is enabled.

	I2C_CONFIG_ID_VD,               // I2C VD sync mode
	///< - @b TRUE  : Response VD sync
	///< - @b FALSE : Don't response VD sync (Default value)

	I2C_CONFIG_ID_DMA_RCWrite,      // I2C DMA random command write mode
	///< - @b TRUE  : Enable DMA random command write mode
	///< - @b FALSE : Disable DMA random command write mode (Default value)

	I2C_CONFIG_ID_DMA_TRANS_SIZE2,  // I2C DMA random command write mode for the size of Transfer Group2

	I2C_CONFIG_ID_DMA_TRANS_SIZE3,  // I2C DMA random command write mode for size of Transfer Group3

	I2C_CONFIG_ID_DMA_TRANS_DB1,    // I2C DMA random command write mode for Data Byte 1

	I2C_CONFIG_ID_DMA_TRANS_DB2,    // I2C DMA random command write mode for Data Byte 2

	I2C_CONFIG_ID_DMA_TRANS_DB3,    // I2C DMA random command write mode for Data Byte 3

	I2C_CONFIG_ID_DMA_VD_SRC,       ///< Input Soure of SIE VD channel
	///< - @note [CHIP_NA51055/520] Valid value: 0x0~0x1. Default 0x0
	///< - @note [other(CHIP_NA51084/528)] Valid value: 0x0~0x4. Default 0x0

	I2C_CONFIG_ID_DMA_VD_NUMBER,    // I2C VD sync mode
	// @note    Number of DMA transaction will synchronize with the following video frame start bit (VD signal).
	///< - @b I2C_VD_NUMBER_NONE    : Reserved.
	///< - @b I2C_VD_NUMBER_1       : synchronize with the following 1 VD signal
	///< - @b I2C_VD_NUMBER_2       : synchronize with the following 2 VD signal
	///< - @b I2C_VD_NUMBER_3       : synchronize with the following 3 VD signal

	I2C_CONFIG_ID_DMA_VD_DELAY,     // I2C VD sync Delay

	I2C_CONFIG_ID_DMA_VD_INTVAL,    // I2C VD sync interval between DMA_TRANS_G1 and DMA_TRANS_G2

	I2C_CONFIG_ID_DMA_VD_INTVAL2,   // I2C VD sync interval between DMA_TRANS_G2 and DMA_TRANS_G3

	I2C_CONFIG_ID_DMA_DESC,         // I2C DMA Desc mode
	I2C_CONFIG_ID_DMA_DESC_CNT,     // [get_only] I2C DMA Desc only (show how many command of DMA descriptor already transferred)
	I2C_CONFIG_ID_DMA_DESC_POLL1,   // [get_only] I2C DMA Desc only (show poll data 1)
	I2C_CONFIG_ID_DMA_DESC_POLL2,   // [get_only] I2C DMA Desc only (show poll data 2)

	I2C_CONFIG_ID_TIMEOUT_MS,		// i2c wait flag timeout


	ENUM_DUMMY4WORD(I2C_CONFIG_ID)
} I2C_CONFIG_ID, *PI2C_CONFIG_ID;

/**
    I2C Status

    I2C status for transmit and receive. Only ONE of these base status could be returned.
*/
typedef enum {
	I2C_STS_OK,                     ///< (Base status) Status OK
	I2C_STS_INVALID_PARAM,          ///< (Base status) Invalid parameter
	I2C_STS_NACK,                   ///< (Base status) Receive NACK
	I2C_STS_BUS_NOT_AVAILABLE,      ///< (Base status) Bus is not avaliable
	I2C_STS_AL,                     ///< (Base status) Arbitration lost
	I2C_STS_SAM,                    ///< (Base status) Slave address matched, for i2c_waitSARMatch()
	I2C_STS_GC,                     ///< (Base status) Receive general call, for i2c_waitSARMatch()
	I2C_STS_DESCERR,                ///< (Base status) Desc poll timeout

	I2C_STS_STOP = 0x80000000,      ///< (Extra status) Detect STOP at slave mode
	///< You might get (I2C_STS_NACK | I2C_STS_STOP) or (I2C_STS_NACK | I2C_STS_STOP)

	ENUM_DUMMY4WORD(I2C_STS)
} I2C_STS, *PI2C_STS;

/**
    I2C R/W bit

    I2C R/W bit, could be read or write
*/
typedef enum {
	I2C_RWBIT_WRITE,                ///< Write, indicate master-transmit or slave-receive mode
	I2C_RWBIT_READ,                 ///< Read, indicate master-receive or slave-transmit mode

	ENUM_DUMMY4WORD(I2C_RWBIT)
} I2C_RWBIT;

// I2C macro to convert SAR to data
// Convert 7-bits slave address to data
#define i2c_conv7bSARToData(S)      ((S) << 1)
// Convert 10-bits slave address to 1st byte data
#define i2c_conv10bSARTo1stData(S)  ((((S) & 0x300) >> 7) | 0xF0)
// Convert 10-bits slave address to 2nd byte data
#define i2c_conv10bSARTo2ndData(S)  ((S) & 0x0FF)

// Get base status from I2C_STS
#define i2c_getBaseStatus(S)        ((S) & ~0xF0000000)
// Get extra status from I2C_STS
#define i2c_getExtraStatus(S)       ((S) & 0xF0000000)

/**
	SESSION table
*/
typedef struct {
	const char      *name;
	I2C_SESSION     session;
} I2C_SESSION_TBL;

/**
    I2C Module ID

    This is used by the I2C Get Object API i2c_getDrvObject() to get the specific I2C module control object.
*/
#define I2C_FW_MAX_SUPT_ID 3 // max number for [520/560 : 3 controller, 528: 5 controller]
typedef enum {
	I2C_ID_I2C,		///< I2C  Controller ID
	I2C_ID_I2C2,	///< I2C2 Controller ID
	I2C_ID_I2C3,	///< I2C3 Controller ID

	I2C_ID_MAX,		///< for 520 backward compatible

	I2C_ID_I2C4 = I2C_ID_MAX,      	///< I2C4 Controller ID
	I2C_ID_I2C5,      				///< I2C5 Controller ID

	ENUM_DUMMY4WORD(I2C_ID)
} I2C_ID;


/**
    I2C Driver Object Definotions

    This is used by the I2C Get Object API i2c_getDrvObject() to get the specific I2C module control object.
*/
typedef struct {
	ER(*open)(PI2C_SESSION pSession);
	ER(*close)(I2C_SESSION Session);
	BOOL (*isOpened)(I2C_SESSION Session);

	ER(*lock)(I2C_SESSION Session);
	ER(*unlock)(I2C_SESSION Session);
	BOOL (*islocked)(I2C_SESSION Session);

	void (*setConfig)(I2C_SESSION Session, I2C_CONFIG_ID ConfigID, UINT32 uiConfig);
	UINT32(*getConfig)(I2C_SESSION Session, I2C_CONFIG_ID ConfigID);

	I2C_STS(*transmit)(PI2C_DATA pData);
	I2C_STS(*receive)(PI2C_DATA pData);

	I2C_RWBIT(*getRWBit)(void);

	BOOL (*compareReg)(void);
	void (*getClockPeriod)(I2C_SESSION Session, double *dHigh, double *dLow);
	void (*setVDDelay)(I2C_SESSION Session, UINT32 uiDelay);

	I2C_STS(*transmitDMA)(UINT32 uiAddr, UINT32 uiSize);
	I2C_STS(*receiveDMA)(UINT32 uiAddr, UINT32 uiSize);

	I2C_STS(*waitSARMatch)(I2C_SAR_LEN SARLen);

	I2C_STS(*transmitDMA_En)(UINT32 uiAddr, UINT32 uiSize);
	I2C_STS(*transmitDMA_Wait)(UINT32 uiAddr, UINT32 uiSize);

	I2C_STS(*clearBus_En)(void);
} I2C_OBJ, *PI2C_OBJ;

//
//  I2C Object APIs
//
extern PI2C_OBJ i2c_getDrvObject(I2C_ID I2cID);
extern UINT32 i2c_get_module_num(void);

//
// I2C public APIs
//
extern ER           i2c_open(PI2C_SESSION pSession);
extern ER           i2c_close(I2C_SESSION Session);
extern BOOL         i2c_isOpened(I2C_SESSION Session);

extern ER           i2c_lock(I2C_SESSION Session);
extern ER           i2c_unlock(I2C_SESSION Session);
extern BOOL         i2c_islocked(I2C_SESSION Session);

extern void         i2c_setConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID, UINT32 uiConfig);
extern UINT32       i2c_getConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID);

extern I2C_STS      i2c_transmit(PI2C_DATA pData);
extern I2C_STS      i2c_receive(PI2C_DATA pData);

extern I2C_RWBIT    i2c_getRWBit(void);

extern I2C_STS      i2c_transmitDMA(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c_receiveDMA(UINT32 uiAddr, UINT32 uiSize);

extern I2C_STS      i2c_transmitDMA_En(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c_transmitDMA_Wait(UINT32 uiAddr, UINT32 uiSize);

extern I2C_STS      i2c_waitSARMatch(I2C_SAR_LEN SARLen);
extern I2C_STS      i2c_clearBus_En(void);

extern BOOL         i2cTest_compareRegDefaultValue(void);
extern void         i2cTest_getClockPeriod(I2C_SESSION Session, double *dHigh, double *dLow);
extern void         i2cTest_setVDDelay(I2C_SESSION Session, UINT32 uiDelay);

//
// I2C2 public APIs
//
extern ER           i2c2_open(PI2C_SESSION pSession);
extern ER           i2c2_close(I2C_SESSION Session);
extern BOOL         i2c2_isOpened(I2C_SESSION Session);

extern ER           i2c2_lock(I2C_SESSION Session);
extern ER           i2c2_unlock(I2C_SESSION Session);
extern BOOL         i2c2_islocked(I2C_SESSION Session);

extern void         i2c2_setConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID, UINT32 uiConfig);
extern UINT32       i2c2_getConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID);

extern I2C_STS      i2c2_transmit(PI2C_DATA pData);
extern I2C_STS      i2c2_receive(PI2C_DATA pData);

extern I2C_RWBIT    i2c2_getRWBit(void);

extern I2C_STS      i2c2_transmitDMA(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c2_receiveDMA(UINT32 uiAddr, UINT32 uiSize);

extern I2C_STS      i2c2_waitSARMatch(I2C_SAR_LEN SARLen);

extern I2C_STS      i2c2_transmitDMA_En(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c2_transmitDMA_Wait(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c2_clearBus_En(void);

extern BOOL         i2c2Test_compareRegDefaultValue(void);
extern void         i2c2Test_getClockPeriod(I2C_SESSION Session, double *dHigh, double *dLow);
extern void         i2c2Test_setVDDelay(I2C_SESSION Session, UINT32 uiDelay);

//
//  I2C3 public APIs
//
extern ER           i2c3_open(PI2C_SESSION pSession);
extern ER           i2c3_close(I2C_SESSION Session);
extern BOOL         i2c3_isOpened(I2C_SESSION Session);

extern ER           i2c3_lock(I2C_SESSION Session);
extern ER           i2c3_unlock(I2C_SESSION Session);
extern BOOL         i2c3_islocked(I2C_SESSION Session);

extern void         i2c3_setConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID, UINT32 uiConfig);
extern UINT32       i2c3_getConfig(I2C_SESSION Session, I2C_CONFIG_ID ConfigID);

extern I2C_STS      i2c3_transmit(PI2C_DATA pData);
extern I2C_STS      i2c3_receive(PI2C_DATA pData);

extern I2C_RWBIT    i2c3_getRWBit(void);

extern I2C_STS      i2c3_transmitDMA(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c3_receiveDMA(UINT32 uiAddr, UINT32 uiSize);

extern I2C_STS      i2c3_waitSARMatch(I2C_SAR_LEN SARLen);

extern I2C_STS      i2c3_transmitDMA_En(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c3_transmitDMA_Wait(UINT32 uiAddr, UINT32 uiSize);
extern I2C_STS      i2c3_clearBus_En(void);

extern BOOL         i2c3Test_compareRegDefaultValue(void);
extern void         i2c3Test_getClockPeriod(I2C_SESSION Session, double *dHigh, double *dLow);
extern void         i2c3Test_setVDDelay(I2C_SESSION Session, UINT32 uiDelay);


//@}

#endif
