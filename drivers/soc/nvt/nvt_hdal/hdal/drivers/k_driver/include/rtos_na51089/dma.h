/**
    Header file for DMA module.

    This file is the header file that define the API for DMA module.

    @file       DMA.h
    @ingroup    mIDrvDMA_DMA
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _DMA_H
#define _DMA_H

#include <stdint.h>
#include <kwrap/nvt_type.h>

/**
    @addtogroup mIDrvDMA_DMA
*/

/**
    DMA controller ID

*/
typedef enum {
	DMA_ID_1,                           ///< DMA Controller
	DMA_ID_2,                           ///< DMA Controller 2

	DMA_ID_COUNT,                       //< DMA controller count

	ENUM_DUMMY4WORD(DMA_ID)
} DMA_ID;

/**
    Check DRAM address is cacheable or not

    Check DRAM address is cacheable or not.

    @param[in] addr     DRAM address

    @return Cacheable or not
        - @b TRUE   : Cacheable
        - @b FALSE  : Non-cacheable
*/
#define dma_isCacheAddr(addr)           (((UINT32)(addr) >= 0x60000000UL) ? FALSE : TRUE)

/**
    Check (virtual) DRAM address is valid or not

    @param[in] addr     DRAM address

    @return valid or not
        - @b TRUE   : valid
        - @b FALSE  : Not valid
*/
#define dma_is_valid_addr(addr)         ((((UINT32)(addr))<0xC0000000UL)?       \
                                            ((addr)!=0?TRUE:FALSE)              \
                                            :FALSE)

/**
    Translate DRAM address to cacheable address.

    Translate DRAM address to cacheable address.

    @param[in] addr     DRAM address

    @return Cacheable DRAM address
*/
#define dma_getCacheAddr(addr)          ((((UINT32)(addr))>=0x60000000UL)?((UINT32)(addr)-0x60000000UL):(UINT32)(addr))


/**
    DRAM type encoding

    DRAM type encoding returned by dma_getDramType()
*/
typedef enum {
	DDR_TYPE_DDR2,          ///< DDR2 DRAM
	DDR_TYPE_DDR3,          ///< DDR3 DRAM
	ENUM_DUMMY4WORD(DMA_DDR_TYPE)
} DMA_DDR_TYPE;

extern DMA_DDR_TYPE     dma_getDramType(void);
extern UINT32           dma_getDramBaseAddr(DMA_ID id);

/*
    Get DRAM capacity

    Get DRAM capacity of DMA controller configuration

    @return DRAM capacity
*/
extern UINT32   dma_getDramCapacity(DMA_ID id);

//@}

#endif
