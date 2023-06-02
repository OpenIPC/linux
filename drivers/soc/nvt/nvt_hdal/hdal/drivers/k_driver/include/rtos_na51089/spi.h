/**
    Header file of SPI module driver

    This file is the header file of SPI module

    @file       SPI.h
    @ingroup    mIDrvIO_SPI
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _SPI_H
#define _SPI_H

#include <kwrap/nvt_type.h>

/**
    @addtogroup mIDrvIO_SPI
*/
//@{

/**
    SPI ID
*/
typedef enum {
	SPI_ID_1,                   ///< SPI 1 (PIO, DMA)
	SPI_ID_2,                   ///< SPI 2 (PIO, DMA)
	SPI_ID_3,                   ///< SPI 3 (PIO, Gyro)
	SPI_ID_4,                   ///< SPI 4 (PIO, DMA)
	SPI_ID_5,                   ///< SPI 5 (PIO, DMA)

	SPI_ID_COUNT,               ///< SPI ID count

	ENUM_DUMMY4WORD(SPI_ID)
} SPI_ID;

/**
    SPI IO CONTROL ID
*/
typedef enum {
	ENUM_DUMMY4WORD(SPI_IOCTRL)
} SPI_IOCTRL;

/**
    SPI Configuration Identifier

    @note For spi_setConfig()
*/
typedef enum {
	SPI_CONFIG_ID_BUSMODE,          ///< SPI Bus Mode. Context can be:
	///< - @b SPI_MODE_0 (default)
	///< - @b SPI_MODE_1
	///< - @b SPI_MODE_2
	///< - @b SPI_MODE_3
	SPI_CONFIG_ID_FREQ,             ///< SPI Bus Frequency. Unit: Hz (default: 24000000)
	SPI_CONFIG_ID_MSB_LSB,          ///< SPI MSB/LSB select. Context can be:
	///< - @b SPI_MSB: MSB (bit) first
	///< - @b SPI_LSB: LSB (bit) first (default)
	SPI_CONFIG_ID_WIDE_BUS_ORDER,   ///< Bit order in wide bus mode. Context can be:
	///< - @b SPI_WIDE_BUS_ORDER_NORMAL: normal order (default)
	///< - @b SPI_WIDE_BUS_ORDER_INVERT: inverted order
	SPI_CONFIG_ID_CS_ACT_LEVEL,     ///< CS active level select. Context can be:
	///< - @b SPI_CS_ACT_LEVEL_LOW: low active (default)
	///< - @b SPI_CS_ACT_LEVEL_HIGH: high active
	SPI_CONFIG_ID_CS_CK_DLY,        ///< CS edge to Clock edge delay. unit: us
	SPI_CONFIG_ID_PKT_DLY,          ///< Delay between each SPI packet. Default 0 us. unit: us.
	SPI_CONFIG_ID_RDY_POLARITY,     ///< Polarity of SPI_RDY (only SPI_ID_4 support SPI_RDY PIN)
	///< - @b SPI_RDY_ACT_LEVEL_LOW: low ready (default)
	///< - @b SPI_RDY_ACT_LEVEL_HIGH: high ready
	SPI_CONFIG_ID_DO_HZ_EN,         ///< SPI DO output Hi-Z control
	///< - @b TRUE: SPI DO be hi-Z when no data is needed to output
	///< - @b FALSE: SPI DO will be driven by SPI controller event no data is needed to output (default)
	SPI_CONFIG_ID_AUTOPINMUX,       ///< SPI auto pinmux control (NOT support)
	///< - @b TRUE: SPI will enable pinmux (according to pinmux_init()) when spi_open(), disable pinmux when spi_close()
	///< - @b FALSE: SPI driver will not alter pinmux(default)
	SPI_CONFIG_ID_VD_SRC,           ///< SPI Gyro VD source select
	///< - @b SPI_VD_SRC_SIE1: SIE1 (default)
	///< - @b SPI_VD_SRC_SIE2: SIE2
	SPI_CONFIG_ID_LATCH_CLK_SHIFT,  ///< Shift latch clock.
	///< Data is from SPI_CONFIG_ID_LATCH_CLK_EDGE
	///< - @b SPI_LATCH_CLK_0T (default)
	///< - @b SPI_LATCH_CLK_1T
	///< - @b SPI_LATCH_CLK_2T
	SPI_CONFIG_ID_LATCH_CLK_EDGE,   ///< Latch clock edge
	///< Latch data from SPI_CONFIG_ID_LATCH_DATA_SRC
	///< - @b SPI_LATCH_EDGE_RISING (default)
	///< - @b SPI_LATCH_EDGE_FALLING
	SPI_CONFIG_ID_LATCH_DATA_SRC,   ///< Latch Data source
	///< - @b SPI_LATCH_DATA_PAD (default)
	///< - @b SPI_LATCH_DATA_DLY (data from SPI_CONFIG_ID_DELAY_CLK_EDGE)
	SPI_CONFIG_ID_DELAY_CLK_EDGE,   ///< Delay chain clock edge select
	///< (Clock is from SPI_CONFIG_ID_DELAY_CHAIN_SEL)
	///< - @b SPI_DLY_CLK_EDGE_RISING (default)
	///< - @b SPI_DLY_CLK_EDGE_FALLING
	SPI_CONFIG_ID_DELAY_CHAIN_SEL,  ///< Delay chain select (unit: delay cell)
	///< (Clock is from SPI_CONFIG_ID_DELAY_CLK_POLARITY)
	///< (Default value: 0)
	///< (range: 0 ~ 63)
	SPI_CONFIG_ID_DELAY_CLK_POLARITY,///< Delay clock polarity
	///< (Clock is from SPI_CONFIG_ID_DELAY_CLK_SRC)
	///< - @b SPI_DLY_CLK_POLARITY_NORMAL (default)
	///< - @b SPI_DLY_CLK_POLARITY_INVERT
	SPI_CONFIG_ID_DELAY_CLK_SRC,    ///< Delay chain clock source
	///< - @b SPI_DLY_CLK_SRC_INTERNAL (default)
	///< - @b SPI_DLY_CLK_SRC_PAD
	SPI_CONFIG_ID_GYRO_SYNC_END_OFFSET, ///< Adjust offset of SPI_GYRO_INT_SYNC_END
	///< (unit: gyro gransfer)
	///< (range: 0 ~ SPI_GYRO_INFO.uiTransferCount - 1, where SPI_GYRO_INFO is your object for spi_startGyro())
	///< - @b 0: Issue SPI_GYRO_INT_SYNC_END at last gyro gransfer (default)
	///< - @b 1: Issue SPI_GYRO_INT_SYNC_END at second-last gyro gransfer
	///< - @b 2: Issue SPI_GYRO_INT_SYNC_END at third-last gyro gransfer
	///< - etc...


	////////// Engineering Usage ///////////////////
	SPI_CONFIG_ID_ENG_PKT_COUNT,    //< engineer mode: packet count for spi_writeSingle(), spi_readSingle(), spi_writeReadSingle().
	//< Context can be:
	//< - @b 1: 1 packet
	//< - @b 2: 2 packet
	//< - @b 4: 4 packet
	SPI_CONFIG_ID_ENG_MSB_LSB,      //< engineer mode: MSB/LSB. Context can be:
	//< - @b SPI_MSB: MSB (bit) first
	//< - @b SPI_LSB: LSB (bit) first
	SPI_CONFIG_ID_ENG_DMA_ABORT,    //< engineer mode: DMA abort. Context can be:
	//< - @b FALSE: no DMA abort after DMA is triggered
	//< - @b TRUE: test DMA abort after DMA is triggered
	SPI_CONFIG_ID_ENG_GYRO_UNIT,    //< engineer mode: gyro delay unit. Context can be:
	//< - @b FALSE: unit is us (default value)
	//< - @b TRUE: unit is SPI CLK
	SPI_CONFIG_ID_ENG_GYRO_INTMSK,  //< engineer mode: gyro interrupt mask

	ENUM_DUMMY4WORD(SPI_CONFIG_ID)
} SPI_CONFIG_ID;

/**
    SPI mode

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_BUSMODE, )
*/
typedef enum {
	SPI_MODE_0,                 ///< MODE 0
	SPI_MODE_1,                 ///< MODE 1
	SPI_MODE_2,                 ///< MODE 2
	SPI_MODE_3,                 ///< MODE 3

	ENUM_DUMMY4WORD(SPI_MODE)
} SPI_MODE;

/**
    SPI LSB/MSB select

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_MSB_LSB, )
*/
typedef enum {
	SPI_MSB,                    ///< MSb first
	SPI_LSB,                    ///< LSb first

	ENUM_DUMMY4WORD(SPI_LSB_MSB)
} SPI_LSB_MSB;

/**
    SPI CS active level

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_CS_ACT_LEVEL, )
*/
typedef enum {
	SPI_CS_ACT_LEVEL_LOW,       ///< CS is low active
	SPI_CS_ACT_LEVEL_HIGH,      ///< CS is high active

	ENUM_DUMMY4WORD(SPI_CS_ACT_LEVEL)
} SPI_CS_ACT_LEVEL;

/**
    SPI RDY active level

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_RDY_POLARITY, )
*/
typedef enum {
	SPI_RDY_ACT_LEVEL_LOW,      ///< SPI_RDY is low ready (i.e. high busy)
	SPI_RDY_ACT_LEVEL_HIGH,     ///< SPI_RDY is high ready (i.e. low busy)

	ENUM_DUMMY4WORD(SPI_RDY_ACT_LEVEL)
} SPI_RDY_ACT_LEVEL;

/*
    SPI bus width

    @note For spi_setBusWidth()
*/
typedef enum {
	SPI_BUS_WIDTH_1_BIT,        //< Bus width is 1 bit (full duplex)
	SPI_BUS_WIDTH_2_BITS,       //< Bus width is 2 bits (half duplex)
//    SPI_BUS_WIDTH_4_BITS,       //< Bus width is 4 bits (half duplex)

	SPI_BUS_WIDTH_HD_1BIT,      //< Bus width is half duplex 1 bit (only require CS, CLK, DIO)

	ENUM_DUMMY4WORD(SPI_BUS_WIDTH)
} SPI_BUS_WIDTH;

/**
    SPI wide bus order

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_WIDE_BUS_ORDER, )
*/
typedef enum {
	SPI_WIDE_BUS_ORDER_NORMAL,  ///< Normal order. Transmit sequence is SPI_IO3, SPI_IO2, SPI_IO1, SPI_IO0.
	SPI_WIDE_BUS_ORDER_INVERT,  ///< Invert order. Transmit sequence is SPI_IO0, SPI_IO1, SPI_IO2, SPI_IO3.

	ENUM_DUMMY4WORD(SPI_WIDE_BUS_ORDER)
} SPI_WIDE_BUS_ORDER;

/**
    SPI transfer length

    @note For spi_setTransferLen()
*/
typedef enum {
	SPI_TRANSFER_LEN_1BYTE,     ///< 1 byte
	SPI_TRANSFER_LEN_2BYTES,    ///< 2 bytes

	ENUM_DUMMY4WORD(SPI_TRANSFER_LEN)
} SPI_TRANSFER_LEN;

/**
    SPI latch clock

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_LATCH_CLK_SHIFT, )
*/
typedef enum {
	SPI_LATCH_CLK_0T,           ///< Latch at 0T
	SPI_LATCH_CLK_1T,           ///< Latch at 1T
	SPI_LATCH_CLK_2T,           ///< Latch at 2T

	ENUM_DUMMY4WORD(SPI_LATCH_CLK)
} SPI_LATCH_CLK;

/**
    SPI latch edge

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_LATCH_CLK_EDGE, )
*/
typedef enum {
	SPI_LATCH_EDGE_RISING,      ///< Latch at rising edge
	SPI_LATCH_EDGE_FALLING,     ///< Latch at falling edge

	ENUM_DUMMY4WORD(SPI_LATCH_EDGE)
} SPI_LATCH_EDGE;

/**
    SPI latch data source

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_LATCH_DATA_SRC, )
*/
typedef enum {
	SPI_LATCH_DATA_PAD,         ///< Latch data from data PAD
	SPI_LATCH_DATA_DLY,         ///< Latch data from result of delay chain

	ENUM_DUMMY4WORD(SPI_LATCH_DATA)
} SPI_LATCH_DATA;

/**
    SPI delay clock edge

    Select to use rising or falling edge of clock output by delay chain

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_DELAY_CLK_EDGE, )
*/
typedef enum {
	SPI_DLY_CLK_EDGE_RISING,    ///< Sample data at rising edge
	SPI_DLY_CLK_EDGE_FALLING,   ///< Sample data at falling edge

	ENUM_DUMMY4WORD(SPI_DLY_CLK_EDGE)
} SPI_DLY_CLK_EDGE;

/**
    SPI delay clock polarity

    Select to select polarity (normal/invert) of delay chain input clock

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_DELAY_CLK_POLARITY, )
*/
typedef enum {
	SPI_DLY_CLK_POLARITY_NORMAL,    ///< Normal
	SPI_DLY_CLK_POLARITY_INVERT,    ///< Invert

	ENUM_DUMMY4WORD(SPI_DLY_CLK_POLARITY)
} SPI_DLY_CLK_POLARITY;

/**
    SPI delay clock source

    Select clock to be delayed by delay chain

    @note For spi_setConfig(SPI_ID, SPI_CONFIG_ID_DELAY_CLK_SRC, )
*/
typedef enum {
	SPI_DLY_CLK_SRC_INTERNAL,   ///< Delay internal SPI clock
	SPI_DLY_CLK_SRC_PAD,        ///< Delay SPI clock from SPI_CLK PAD

	ENUM_DUMMY4WORD(SPI_DLY_CLK_SRC)
} SPI_DLY_CLK_SRC;

/**
    SPI gyro mode

    @note For gyroMode of SPI_GYRO_INFO
*/
typedef enum {
	SPI_GYRO_MODE_SIE_SYNC,             ///< SIE sync mode
	SPI_GYRO_MODE_ONE_SHOT,             ///< one shot mode
	SPI_GYRO_MODE_FREE_RUN,             ///< free run mode

	SPI_GYRO_MODE_SIE_SYNC_WITH_RDY,    ///< SIE sync mode with ready PIN from device

	ENUM_DUMMY4WORD(SPI_GYRO_MODE)
} SPI_GYRO_MODE;

/**
    SPI GYRO interrupt

    @note For uiGyroSts of SPI_GYRO_CB
*/
typedef enum {
	SPI_GYRO_INT_SYNC_END           = 0x00000001,   ///< End of one SIE sync. All data in this sync are completed.
	SPI_GYRO_INT_OVERRUN            = 0x00000002,   ///< HW FIFO overrun. FW should do error handling, ex: re-start gyro polling
	SPI_GYRO_INT_SEQ_ERR            = 0x00000004,   ///< Sequence error. Next SIE sync is too early.
	SPI_GYRO_INT_TRS_TIMEOUT        = 0x00000008,   ///< A Gyro Transfer timeout. (Maybe SPI_RDY hold by device too long)
	SPI_GYRO_INT_CHANGE_END         = 0x00000010,   ///< End of SIE VD after spi_changeGyro() is invoked. (Next VD will be new gyro setting)
	SPI_GYRO_INT_LAST_TRS           = 0x00000020,   ///< End of last transfer in a VD period

	SPI_GYRO_INT_QUEUE_THRESHOLD    = 0x00000100,   ///< SW Queue exceed threshold. Upper layer should sink Gyro data ASAP.
	SPI_GYRO_INT_QUEUE_OVERRUN      = 0x00000200,   ///< Sw Queue overrun. FW should do error handling.

	ENUM_DUMMY4WORD(SPI_GYRO_INT)
} SPI_GYRO_INT;

/**
    SPI VD source

    @note For SPI_CONFIG_ID_VD_SRC
*/
typedef enum {
	SPI_VD_SRC_SIE1,                    ///< VD source is SIE1
	SPI_VD_SRC_SIE2,                    ///< VD source is SIE2
	SPI_VD_SRC_SIE4,                    ///< VD source is SIE4
	SPI_VD_SRC_SIE5,                    ///< VD source is SIE5

	ENUM_DUMMY4WORD(SPI_VD_SRC)
} SPI_VD_SRC;


/*
    SPI initialization infomation

    (OBSOLETE structure)
*/
typedef struct {
	BOOL        bCSActiveLow;   ///< CS polarity
	///< - TRUE: CS is active LOW
	///< - FALSE: CS is active HIGH
	BOOL        bMasterMode;    ///< Master mode
	///< - TRUE: master mode
	UINT32      uiFreq;         ///< Clock frequency (unit: Hz)
	SPI_MODE    spiMODE;        ///< SPI MODE select
	///< - SPI_MODE_0: mode 0
	///< - SPI_MODE_1: mode 1
	///< - SPI_MODE_2: mode 2
	///< - SPI_MODE_3: mode 3
	BOOL        bLSB;           ///< LSB mode select (bit order)
	///< - TRUE: LSB mode
	///< - FALSE: MSB mode
	SPI_WIDE_BUS_ORDER wideBusOrder;    ///< Bus order is wide bus mode (SPI_BUS_WIDTH_2_BITS)
	///< - SPI_WIDE_BUS_ORDER_NORMAL: normal order
	///< - SPI_WIDE_BUS_ORDER_INVERT: inverted order
	UINT32      uiCsCkDelay;    ///< Delay time between CS edge and first clock edge
	UINT32      uiPktDelay;     ///< Delay time between each SPI packet
} SPI_INIT_INFO, *PSPI_INIT_INFO;

/**
    GYRO Buffer Queue

    @note For spi_getGyroData()
*/
typedef struct {
	UINT32  uiFrameID;          ///< record frame ID of this buffer
	UINT32  uiDataID;           ///< record Gyro data ID. Valid value: 0~31
	UINT32  vRecvWord[2];       ///< stores data received in one Gyro data transfer
} GYRO_BUF_QUEUE, *PGYRO_BUF_QUEUE;

/**
    @name SPI gyro mode call back prototype

    @note For pEventHandler of SPI_GYRO_INFO

    @param[in] gyroSts      gyro status. Can be:
                            - @b SPI_GYRO_INT_SYNC_END
                            - @b SPI_GYRO_INT_OVERRUN
                            - @b SPI_GYRO_INT_SEQ_ERR
                            - @b SPI_GYRO_INT_TRS_TIMEOUT

    @return void
*/
//@{
typedef void (*SPI_GYRO_CB)(SPI_GYRO_INT gyroSts);
//@}

/**
    SPI GYRO control infomation

    @note For spi_startGyro()
*/
typedef struct {
	SPI_GYRO_MODE   gyroMode;   ///< Gyro trigger mode

	UINT32 uiOpInterval;        ///< Delay between 2 OP (unit: us)
	UINT32 uiTransferInterval;  ///< Delay between 2 transfer (unit: us)
	UINT32 uiTransferCount;     ///< total transfer in single run. valid value: 1~32

	UINT32 uiTransferLen;       ///< length of Transer: unit: OP. valid value: 1~4
	UINT32 uiOp0Length;         ///< length of OP0. unit: byte. valid value: 1~8
	UINT32 uiOp1Length;         ///< length of OP1. unit: byte. valid value: 1~8
	UINT32 uiOp2Length;         ///< length of OP2. unit: byte. valid value: 1~8
	UINT32 uiOp3Length;         ///< length of OP3. unit: byte. valid value: 1~8

	UINT8 vOp0OutData[8];       ///< stores data to be output in OP0
	UINT8 vOp1OutData[8];       ///< stores data to be output in OP1
	UINT8 vOp2OutData[8];       ///< stores data to be output in OP2
	UINT8 vOp3OutData[8];       ///< stores data to be output in OP3

	void (*pEventHandler)(SPI_GYRO_INT gyroSts);    ///< EventHandler function pointer, set to NULL if you don't want to handle audio event
} SPI_GYRO_INFO, *PSPI_GYRO_INFO;


extern ER   spi_open(SPI_ID spiID);
extern ER   spi_close(SPI_ID spiID);
extern BOOL spi_isOpened(SPI_ID spiID);

extern ER   spi_setTransferLen(SPI_ID spiID, SPI_TRANSFER_LEN length);
extern void spi_setCSActive(SPI_ID spiID, BOOL bCSActive);
extern ER   spi_writeReadData(SPI_ID spiID, UINT32 uiWordCount, UINT32 *pTxBuf, UINT32 *pRxBuf, BOOL bDmaMode);
extern ER   spi_waitDataDone(SPI_ID spiID);
extern ER   spi_writeSingle(SPI_ID spiID, UINT32 uiTxWord);
extern ER   spi_writePacket(SPI_ID spiID, UINT32 uiPktCnt, UINT32 uiTxWord);
extern ER   spi_readSingle(SPI_ID spiID, UINT32 *pRxWord);
extern ER   spi_writeReadSingle(SPI_ID spiID, UINT32 uiTxWord, UINT32 *pRxWord);

extern ER   spi_startGyro(SPI_ID spiID, SPI_GYRO_INFO *pGyroInfo);
extern ER   spi_changeGyro(SPI_ID spiID, SPI_GYRO_INFO *pGyroInfo, UINT32 uiSyncEndOffset);
extern ER   spi_stopGyro(SPI_ID spiID);
extern UINT32 spi_getGyroQueueCount(SPI_ID spiID);
extern ER   spi_getGyroData(SPI_ID spiID, PGYRO_BUF_QUEUE pGyroData);

extern ER   spi_setConfig(SPI_ID spiID, SPI_CONFIG_ID configID, UINT32 configContext);
extern ER   spi_enBitMatch(SPI_ID spiID, UINT32 uiCmd, UINT32 uiBitPosition, BOOL bWaitValue);
extern ER   spi_waitBitMatch(SPI_ID spiID, UINT32 uiTimeoutMs);
extern ER   spi_setBusWidth(SPI_ID spiID, SPI_BUS_WIDTH busWidth);

extern void spi_platform_init(void);
extern void spi_platform_uninit(void);
//@}

#endif
