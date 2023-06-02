/*
    @file       Ethernet_int.h

    @ingroup    mIDrvIO_Eth

    @brief      NT96510 Ethernet internal header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/

#ifndef _ETHERNET_INT_H
#define _ETHERNET_INT_H

//#include "DrvCommon.h"
#include "top.h"
#include "eth_gmac.h"
#include "cache.h"


//#define ETH_DEFAULT_MAC_ADDR        {0x00, 0x55, 0x7B, 0xB5, 0x7D, 0xF7}
#define ETH_DEFAULT_MAC_ADDR        {0x00, 0x55, 0x7B, 0xB5, 0x7D, 0xF8}

//#define ETH_TX_DESC_QUEUE_LENGTH    (512)
#define ETH_TX_DESC_QUEUE_LENGTH    (16)
#define ETH_RX_DESC_QUEUE_LENGTH    (16)

#define ETH_DESC_BUF_SIZE           (2048)      // buffer (pointed by descriptor) size
//#define ETH_DESC_BUF_SIZE           (1600)      // buffer (pointed by descriptor) size
//#define ETH_DESC_WORD_SIZE          (CPU_ICACHE_LINE_SIZE/sizeof(UINT32))   // Make each descriptor cache line alignment
#define ETH_DESC_WORD_SIZE          (32/sizeof(UINT32))   // Make each descriptor cache line alignment
//#define ETH_DESC_WORD_SIZE          (4+2)         // one descriptor is (4 words + 2)
//#define ETH_DESC_WORD_SIZE          (4+2+1)         // one descriptor is (4 words + 2 + 1)

/*
    Ethernet frame structure (in real ethernet packet)

*/
typedef struct ETH_FRAME_HEAD {
	UINT8 dstAddr[6];               //< Destination address
	UINT8 srcAddr[6];               //< Source address
//    UINT8 vlanTPIDMsb;
//    UINT8 vlanTPIDLsb;
//    UINT16 vlanTCI;
	UINT8 typeLenMsb;               //< type length (MSB)
	UINT8 typeLenLsb;               //< type length (LSB)
} ETH_FRAME_HEAD, *PETH_FRAME_HEAD;

/*
    Ethernet frame queue

    (assume we use ring structure. i.e. circular queue)
*/
typedef struct ETH_DMA_QUEUE {
	// configuration
	ETHMAC_DMA_DESCRIPT_MODE_ENUM queueType;    // queue type: ring or linked list
	UINT32 uiQueueBaseAddr;         //< Base address of queue
	UINT32 uiMaxDepth;              //< Max depth of this queue
	UINT32 uiFrameBaseAddr;         //< Base address of frame buffer for this queue

	// run-time state
	UINT32 uiCurrDepth;             //< Current depth
	UINT32 uiWritePos;              //< Next queue position to write
	UINT32 uiReadPos;               //< Next queue position to read
} ETH_DMA_QUEUE, *PETH_DMA_QUEUE;

#endif
//@}


