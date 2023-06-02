/*
    @file       Eth_gmac.h

    @ingroup    mIDrvIO_Eth

    @brief      NT96510 Ethernet GMAC header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/

#ifndef _ETHERNET_GMAC_H
#define _ETHERNET_GMAC_H

//#include "DrvCommon.h"
#include "top.h"
#include "ethernet_protected.h"

#define ETHMAC_SYSBUS_WIDTH_SIZE    8//16  // unit: byte
#define ETHMAC_SYSBUS_WIDTH_MASK    0x7

/*
    MAC duplex enum
*/
typedef enum {
	ETHMAC_DUPLEX_HALF = 0,             //< Half duplex
	ETHMAC_DUPLEX_FULL = 1,             //< Full duplex

	ENUM_DUMMY4WORD(ETHMAC_DUPLEX_ENUM)
} ETHMAC_DUPLEX_ENUM;

/*
    MAC speed enum
*/
typedef enum {
	ETHMAC_SPD_10 = 0,                  //< 10 Mbps
	ETHMAC_SPD_100 = 1,                 //< 100 Mbps
	ETHMAC_SPD_1000 = 2,                //< 1000 Mbps (Not support in NT96660)

	ENUM_DUMMY4WORD(ETHMAC_SPD_ENUM)
} ETHMAC_SPD_ENUM;

/*
    MAC backoff limit enum
*/
typedef enum {
	ETHMAC_BACKOFF_LIMIT_1023 = 0,      //< delay 0~1023 slot time (spec value)
	ETHMAC_BACKOFF_LIMIT_255 = 1,       //< delay 0~255 slot time
	ETHMAC_BACKOFF_LIMIT_15 = 2,        //< delay 0~15 slot time
	ETHMAC_BACKOFF_LIMIT_1 = 3,         //< delay 0~1 slot time

	ENUM_DUMMY4WORD(ETHMAC_BACKOFF_LIMIT_ENUM)
} ETHMAC_BACKOFF_LIMIT_ENUM;

/*
    MAC control rx frame filtering
*/
typedef enum {
	ETHMAC_RX_FRAME_FILTER_DISABLE = 0,     //< disable rx frame filter. i.e. receive all frame
	ETHMAC_RX_FRAME_FILTER_PROMISCUOUS = 1, //< enable rx frame filter in promiscuous mode
	ETHMAC_RX_FRAME_FILTER_NORMAL = 2,      //< enable rx frame filter in normal mode

	ENUM_DUMMY4WORD(ETHMAC_RX_FRAME_FILTER_ENUM)
} ETHMAC_RX_FRAME_FILTER_ENUM;

/*
    MAC control frame processing
*/
typedef enum {
	ETHMAC_CTRL_FRAME_FILTER_ALL = 0,   //< filter all control frame
	ETHMAC_CTRL_FRAME_FILTER_PAUSE = 1, //< filter pause, pass others
	ETHMAC_CTRL_FRAME_RECV_ALL = 2,     //< receive all control frame
	ETHMAC_CTRL_FRAME_RECV_ADDR = 3,    //< receive control frame passed address filter

	ENUM_DUMMY4WORD(ETHMAC_CTRL_FRAME_ENUM)
} ETHMAC_CTRL_FRAME_ENUM;

/*
    MAC multicast filter mode
*/
typedef enum {
	ETHMAC_MULTICAST_FILTER_MODE_NORMAL = 0,    //< normal filter (accept all multicast frame)
	ETHMAC_MULTICAST_FILTER_MODE_PERFECT = 1,   //< perfect matching with destination address registers
	ETHMAC_MULTICAST_FILTER_MODE_HASH = 2,      //< filter destination address with hash table

	ENUM_DUMMY4WORD(ETHMAC_MULTICAST_FILTER_MODE_ENUM)
} ETHMAC_MULTICAST_FILTER_MODE_ENUM;

/*
    MAC destination address filter mode
*/
typedef enum {
	ETHMAC_DST_ADDR_FILTER_MODE_NORMAL = 0,     //< normal filter
	ETHMAC_DST_ADDR_FILTER_MODE_INVERSE = 1,    //< inverse filter (result)

	ENUM_DUMMY4WORD(ETHMAC_DST_ADDR_FILTER_MODE_ENUM)
} ETHMAC_DST_ADDR_FILTER_MODE_ENUM;

/*
    MAC DMA descriptor mode
*/
typedef enum {
	ETHMAC_DMA_DESCRIPT_MODE_RING = 0,          //< ring structure
	ETHMAC_DMA_DESCRIPT_MODE_CHAIN = 1,         //< (OBSOLETE) chain structure

	ENUM_DUMMY4WORD(ETHMAC_DMA_DESCRIPT_MODE_ENUM)
} ETHMAC_DMA_DESCRIPT_MODE_ENUM;

/*
    MAC DMA Burst Length
*/
typedef enum {
	ETHMAC_DMA_BURST_1 = 1,                     //< burst length 1
	ETHMAC_DMA_BURST_2 = 2,                     //< burst length 2
	ETHMAC_DMA_BURST_4 = 4,                     //< burst length 4
	ETHMAC_DMA_BURST_8 = 8,                     //< burst length 8
	ETHMAC_DMA_BURST_16 = 16,                   //< burst length 16
	ETHMAC_DMA_BURST_32 = 32,                   //< burst length 32
	ETHMAC_DMA_BURST_64 = 64,                   //< burst length 64
	ETHMAC_DMA_BURST_128 = 128,                 //< burst length 128
	ETHMAC_DMA_BURST_256 = 256,                 //< burst length 256

	ENUM_DUMMY4WORD(ETHMAC_DMA_BURST_ENUM)
} ETHMAC_DMA_BURST_ENUM;

/*
    MAC DMA Rx Mode
*/
typedef enum {
	ETHMAC_DMA_RXMODE_CUT_THROUGH = 0,          //< cut through mode (threshold mode)
	ETHMAC_DMA_RXMODE_STORE_FORWARD = 1,        //< store and forward mode

	ENUM_DUMMY4WORD(ETHMAC_DMA_RXMODE_ENUM)
} ETHMAC_DMA_RXMODE_ENUM;

/*
    MAC DMA Tx Mode
*/
typedef enum {
	ETHMAC_DMA_TXMODE_THRESHOLD = 0,            //< threshold mode
	ETHMAC_DMA_TXMODE_STORE_FORWARD = 1,        //< store and forward mode

	ENUM_DUMMY4WORD(ETHMAC_DMA_TXMODE_ENUM)
} ETHMAC_DMA_TXMODE_ENUM;

/*
    MAC DMA Int Mode
*/
typedef enum {
	ETHMAC_DMA_INTMODE_EDGE = 0,            //< edge mode
	ETHMAC_DMA_INTMODE_LEVEL = 1,        	//< level mode

	ENUM_DUMMY4WORD(ETHMAC_DMA_INTMODE_ENUM)
} ETHMAC_DMA_INTMODE_ENUM;


extern void ethGMAC_reset(void);
extern void ethGMAC_setMacAddress(ETH_MAC_ID id, UINT8 *pMacAddr);
extern void ethGMAC_initMDCclock(void);

extern ER   ethGMAC_writeExtPhy(UINT32 uiPhyAddr, UINT32 uiRegAddr, UINT32 uiData);
extern ER   ethGMAC_readExtPhy(UINT32 uiPhyAddr, UINT32 uiRegAddr, UINT32 *puiData);

//extern void ethGMAC_setMacWatchdog(BOOL bEnable);
//extern void ethGMAC_setMacJabber(BOOL bEnable);
extern void ethGMAC_setFrameBurst(BOOL bEnable);
extern void ethGMAC_setJumboFrame(BOOL bEnable);
//extern void ethGMAC_setReceiveOwn(BOOL bEnable);
extern void ethGMAC_setLoopback(BOOL bEnable);
extern void ethGMAC_setDuplexMode(ETHMAC_DUPLEX_ENUM duplexMode);
//extern void ethGMAC_setRetryAfterCollision(BOOL bEnable);
extern void ethGMAC_setCrcStrip(BOOL bEnable);
//extern void ethGMAC_setupBackoffLimit(ETHMAC_BACKOFF_LIMIT_ENUM backOffLimit);
//extern void ethGMAC_setDeferralCheck(BOOL bEnable);
extern void ethGMAC_setRecvChecksumOffload(BOOL bEnable);

extern void ethGMAC_setTxEn(BOOL bEnable);
extern void ethGMAC_setRxEn(BOOL bEnable);

extern void ethGMAC_setSpeed(ETHMAC_SPD_ENUM speed);


extern void ethGMAC_setRxFrameFilter(ETHMAC_RX_FRAME_FILTER_ENUM filterMode);
extern void ethGMAC_setRxCtrlFrameFilter(ETHMAC_CTRL_FRAME_ENUM ctrlFrameMode);
extern void ethGMAC_setRxBroadcast(BOOL bEnable);
extern void ethGMAC_setRxSrcAddrFilter(BOOL bEnable);
extern void ethGMAC_setRxDstAddrFilterMode(ETHMAC_DST_ADDR_FILTER_MODE_ENUM filterMode);
extern void ethGMAC_setRxMulticastFilter(ETHMAC_MULTICAST_FILTER_MODE_ENUM filerMode);
extern void ethGMAC_setRxUnicastHash(BOOL bEnable);

extern void ethGMAC_setUnicastPauseFrame(BOOL bEnable);
extern void ethGMAC_setRxFlowControl(BOOL bEnable);
extern void ethGMAC_setTxFlowControl(BOOL bEnable);

extern void ethGMAC_disableAllMMCIntEn(void);

extern void ethGMAC_setDmaAAL(BOOL en);
extern void ethGMAC_setFixedBurst(BOOL en);
extern void ethGMAC_setRBurst(BOOL en);
extern void ethGMAC_setDmaTxDescAddr(UINT32 uiAddr);
extern void ethGMAC_setDmaRxDescAddr(UINT32 uiAddr);
extern void ethGMAC_setDmaBurst(ETHMAC_DMA_BURST_ENUM burst);
extern void ethGMAC_setDmaMSS(UINT32 uiMSS);
extern void ethGMAC_setDmaDescSkipLen(UINT32 uiWordCount);
extern void ethGMAC_setDmaRxDescCount(UINT32 uiCount);
extern void ethGMAC_setDmaTxDescCount(UINT32 uiCount);
extern void ethGMAC_setDmaRxThreshold(UINT32 uiByteCount);
extern void ethGMAC_setDmaRxMode(ETHMAC_DMA_RXMODE_ENUM rxMode);
extern void ethGMAC_setDmaTxMode(ETHMAC_DMA_TXMODE_ENUM txMode);
extern void ethGMAC_setDmaRxEn(BOOL bEnable);
extern void ethGMAC_setDmaRxBufSize(UINT32 uiSize);
extern void ethGMAC_setDmaTsoEn(BOOL bEnable);
extern void ethGMAC_setDmaTxEn(BOOL bEnable);

extern void ethGMAC_initMacIntEn(void);
extern void ethGMAC_initDmaIntEn(void);

extern void ethGMAC_resumeDmaTx(UINT32 uiDescAddr);
extern void ethGMAC_updateDmaRxTailPtr(UINT32 uiDescAddr);

extern void ethGMAC_externalPhyEn(void);
extern void ethGMAC_rmiiRefClkOutputEn(void);
extern void ethGMAC_rmiiRefClkInputEn(void);
extern void ethGMAC_setIntMode(ETHMAC_DMA_INTMODE_ENUM mode);

#endif
//@}



