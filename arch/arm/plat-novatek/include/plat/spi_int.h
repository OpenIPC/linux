/*
	SPI Internal header
	@file       spi_int.h
	@ingroup    mIDrvIO_SPI
	@brief      SPI internal header file
	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _SPI_INT_H
#define _SPI_INT_H

#include "spi_reg.h"

#define SPI_DESIGN_WORKAROUND   (1)

#define SPI_DMA_DIR_SPI2RAM     (0)
#define SPI_DMA_DIR_RAM2SPI     (1)

#define SPI_MAX_TRANSFER_CNT    (0xFFFFFF)  ///< max transfer count for spi_writeReadData()

#define SPI_GYRO_CTRL_COUNT     (1)
/*Software Queue depth is 3 SIE Sync. Max 32 transfer per SIE Sync., Max 4 OP per transfer*/
#define SPI_GYRO_QUEUE_DEPTH    (SPI_GYRO_FIFO_DEPTH*SPI_GYRO_TRSLEN_MAX*3+1)

/* Not every SPI controller support gyro sensor polling
Use a hash function to convert SPI ID to internal gyro resource */
#define SPI_GYRO_ID_HASH(id)    (0)

#define SPI_FLASH_BASE_ADDR 0xC0230000
/*
    SPI Capability
*/
typedef enum
{
	SPI_CAPABILITY_PIO = 0x0,           //< Support PIO mode
	SPI_CAPABILITY_DMA = 0x1,           //< Support DMA mode
	SPI_CAPABILITY_GYRO = 0x02,         //< Support Gyro polling mode

	SPI_CAPABILITY_1BIT = 0x000,        //< Support 1 bits full duplex mode
	SPI_CAPABILITY_2BITS = 0x100,       //< Support 2 bits half duplex mode
	SPI_CAPABILITY_4BITS = 0x200,       //< Support 4 bits half duplex mode

	ENUM_DUMMY4WORD(SPI_CAPABILITY)
} SPI_CAPABILITY;

/*
    SPI State
*/
typedef enum
{
	SPI_STATE_IDLE,
	SPI_STATE_PIO,
	SPI_STATE_DMA,
	SPI_STATE_FLASH,
	SPI_STATE_FLASH_WAIT,
	SPI_STATE_GYRO,

	ENUM_DUMMY4WORD(SPI_STATE)
} SPI_STATE;
#endif
//@}

