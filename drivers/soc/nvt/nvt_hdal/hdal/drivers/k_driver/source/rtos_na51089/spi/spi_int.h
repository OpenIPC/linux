/*
    @file       spi_int.h

    @ingroup    mIDrvIO_SPI

    @brief      NT96660 SPI internal header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.

*/

#ifndef _SPI_INT_H
#define _SPI_INT_H

//#include "DrvCommon.h"
#include <kwrap/nvt_type.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <io_address.h>
#include "top.h"
#include "spi_reg.h"
#include "cache_protected.h"

#define DRV_SUPPORT_IST             (ENABLE)
#define _EMULATION_ON_CPU2_	        (0)
#ifdef _NVT_EMULATION_
#define _EMULATION_                 (ENABLE)
#else
#define _EMULATION_                 (DISABLE)
#endif

#ifdef _NVT_FPGA_
#define _FPGA_EMULATION_            (ENABLE)
#else
#define _FPGA_EMULATION_            (DISABLE)
#endif

#define SPI_DESIGN_WORKAROUND   (0)

#define SPI_DMA_DIR_SPI2RAM     (0)
#define SPI_DMA_DIR_RAM2SPI     (1)

#define SPI_MAX_TRANSFER_CNT    (0xFFFFFF)  ///< max transfer count for spi_writeReadData()

#define SPI_SETREG(ofs,value)  OUTW(IOADDR_SPI_REG_BASE+(ofs),(value))
#define SPI_GETREG(ofs)        INW(IOADDR_SPI_REG_BASE+(ofs))

#define SPI2_SETREG(ofs,value) OUTW(IOADDR_SPI2_REG_BASE+(ofs),(value))
#define SPI2_GETREG(ofs)       INW(IOADDR_SPI2_REG_BASE+(ofs))

#define SPI3_SETREG(ofs,value) OUTW(IOADDR_SPI3_REG_BASE+(ofs),(value))
#define SPI3_GETREG(ofs)       INW(IOADDR_SPI3_REG_BASE+(ofs))

#define SPI4_SETREG(ofs,value) OUTW(IOADDR_SPI4_REG_BASE+(ofs),(value))
#define SPI4_GETREG(ofs)       INW(IOADDR_SPI4_REG_BASE+(ofs))

#define SPI5_SETREG(ofs,value) OUTW(IOADDR_SPI5_REG_BASE+(ofs),(value))
#define SPI5_GETREG(ofs)       INW(IOADDR_SPI5_REG_BASE+(ofs))


#define SPI_GYRO_CTRL_COUNT     (1)
// Software Queue depth is 3 SIE Sync. Max 32 transfer per SIE Sync., Max 4 OP per transfer
#define SPI_GYRO_QUEUE_DEPTH    (SPI_GYRO_FIFO_DEPTH*SPI_GYRO_TRSLEN_MAX*3+1)

// Not every SPI controller support gyro sensor polling
// Use a hash function to convert SPI ID to internal gyro resource
#define SPI_GYRO_ID_HASH(id)    (0)

/*
    SPI Capability
*/
typedef enum {
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
typedef enum {
	SPI_STATE_IDLE,
	SPI_STATE_PIO,
	SPI_STATE_DMA,
	SPI_STATE_FLASH,
	SPI_STATE_FLASH_WAIT,
	SPI_STATE_GYRO,

	ENUM_DUMMY4WORD(SPI_STATE)
} SPI_STATE;

/*
    SPI Gyro State
*/
typedef enum {
	SPI_GYRO_STATE_IDLE,
	SPI_GYRO_STATE_RUN,
	SPI_GYRO_STATE_CHANGE_ISSUED,

	ENUM_DUMMY4WORD(SPI_GYRO_STATE)
} SPI_GYRO_STATE;

/*
    SPI register default value

    SPI register default value.
*/
typedef struct {
	UINT32  uiOffset;
	UINT32  uiValue;
	CHAR    *pName;
} SPI_REG_DEFAULT;

extern UINT32 pinmux_getPinmux(PIN_FUNC id);

#endif
//@}

