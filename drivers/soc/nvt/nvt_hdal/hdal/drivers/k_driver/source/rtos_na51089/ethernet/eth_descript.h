#ifndef _ETHERNET_DMA_DESCRIPTOR_H
#define _ETHERNET_DMA_DESCRIPTOR_H

#include "kwrap/type.h"


/*
    CSR clock range enum
*/
typedef enum {
	ETH_DMA_SAMP = 0x0F,           //< CSR Clock = APB/18

	ENUM_DUMMY4WORD(ETH_DMA_ENUM)
} ETH_DMA_ENUM;

/*
    Ethernet DMA rx queue node

    (CPU write, ethernet DMA read)
*/
typedef struct ETH_RX_DESC {
	// Receive Descriptor 0
	UINT32 uiBuf1AP;                //< Buffer 1 Address Pointer

	// Receive Descriptor 1
	UINT32 uiRsv1;                  //< Reserved

	// Receive Descriptor 2
	UINT32 uiBuf2AP;                //< Buffer 2 Address Pointer

	// Receive Descriptor 3
	UINT32 uiRsv2: 24;              //< Reserved
	UINT32 bBUF1V: 1;               //< Buffer 1 Address Valid
	UINT32 bBUF2V: 1;               //< Buffer 2 Address Valid
	UINT32 uiRsv3: 4;               //< Reserved
	UINT32 bIOC: 1;                 //< Interrupt On Completion
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer

	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_RX_DESC, *PETH_RX_DESC;

/*
    Ethernet DMA rx queue node

    (CPU read, ethernet DMA write)
*/
typedef struct ETH_RX_DESC_READ {
	// Receive Descriptor 0
	UINT32 uiOVT: 16;               //< Outer VLAN Tag
	UINT32 uiIVT: 16;               //< Inner VLAN Tag

	// Receive Descriptor 1
	UINT32 uiPT: 3;                 //< Payload Type
	UINT32 bIPHE: 1;                //< IP Header Error
	UINT32 bIPV4: 1;                //< IPv4 Header Present
	UINT32 bIPV6: 1;                //< IPv6 Header Present
	UINT32 bIPCB: 1;                //< IP Checksum Bypassed
	UINT32 bIPCE: 1;                //< IP Payload Error
	UINT32 uiPMT: 4;                //< PTP Message Type
	UINT32 bPFT: 1;                 //< PTP Packet Type
	UINT32 bPV: 1;                  //< PTP Version
	UINT32 bTSA: 1;                 //< Timestamp Available
	UINT32 bTD: 1;                  //< Timestamp Dropped
	UINT32 uiOPC: 16;               //< OAM Sub-Type Code

	// Receive Descriptor 2
	UINT32 uiHL: 10;                //< L3/L3 Header Length
	UINT32 bARPNR: 1;               //< ARP Reply Not Generated
	UINT32 uiRsv1: 3;               //< Reserved
	UINT32 bITS: 1;                 //< Inner VLAN Tag Filter Status
	UINT32 bOTS: 1;                 //< Outer VLAN Tag Filter Status
	UINT32 bSAF: 1;                 //< SA Address Filter Fail
	UINT32 bDAF: 1;                 //< DA Address Filter Fail
	UINT32 bHF: 1;                  //< Hash Filter Status
	UINT32 uiMADRM: 8;              //< MAC Address Match or Hash Value
	UINT32 bL3FM: 1;                //< Layer 3 Filter Match
	UINT32 bL4FM: 1;                //< Layer 4 Filter Match
	UINT32 uiL3L4FM: 3;             //< Layer 3 and Layer 4 Filter Number Matched

	// Receive Descriptor 3
	UINT32 uiPL: 15;                //< Packet Length
	UINT32 bES: 1;                  //< Error Summary
	UINT32 uiLT: 3;                 //< Length/Type Field
	UINT32 bDE: 1;                  //< Drible Bit Error
	UINT32 bRE: 1;                  //< Receive Error
	UINT32 bOE: 1;                  //< Overflow Error
	UINT32 bRWT: 1;                 //< Receive Watchdog Timeout
	UINT32 bGP: 1;                  //< Giant Packet
	UINT32 bCE: 1;                  //< CRC Error
	UINT32 bRS0V: 1;                //< Received Status RDES0 Valid
	UINT32 bRS1V: 1;                //< Received Status RDES1 Valid
	UINT32 bRS2V: 1;                //< Received Status RDES2 Valid
	UINT32 bLD: 1;                  //< Last Descriptor
	UINT32 bFD: 1;                  //< First Descriptor
	UINT32 bCTXT: 1;                //< Context Type
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer

	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_RX_DESC_READ, *PETH_RX_DESC_READ;

/*
    Ethernet DMA rx context

    (rx context descriptor)
*/
typedef struct ETH_RX_DESC_CONTEXT {
	// Receive Descriptor 0
	UINT32 uiRTSL;                  //< Receive Packet Timestamp Low

	// Receive Descriptor 1
	UINT32 uiRTSH;                  //< Receive Packet Timestamp High

	// Receive Descriptor 2
	UINT32 uiRsv1;                  //< Reserved

	// Receive Descriptor 3
	UINT32 uiRsv2: 30;              //< Reserved
	UINT32 bCTXT: 1;                //< Context Type
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer

	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_RX_DESC_CONTEXT, *PETH_RX_DESC_CONTEXT;

/*
    Ethernet DMA tx queue node

    (CPU write, ethernet DMA read)
*/
typedef struct ETH_TX_DESC {
	// Transmit Descriptor 0
	UINT32 uiBuf1AP;                //< Buffer 1 Address Pointer

	// Transmit Descriptor 1
	UINT32 uiBuf2AP;                //< Buffer 2 Address Pointer

	// Transmit Descriptor 2
	UINT32 uiHLB1L: 14;             //< Header Length or Buffer 1 Length
	UINT32 uiVTIR: 2;               //< (NOT Configured) VLAN Tag Insertion or Replacement
	UINT32 uiB2L: 14;               //< Buffer 2 Length
	UINT32 bTTSE_TMWD: 1;           //< Transmit Timestamp Enable (or External TSO Memory Write Disable)
	UINT32 bIOC: 1;                 //< Interrupt On Completion

	// Transmit Descriptor 3
	UINT32 uiFL_TPL: 15;            //< Packet Length (or TCP Payload Length)
	UINT32 uiTPL: 1;                //< TCP Payload Length[15]
	UINT32 uiCIC_TPL: 2;            //< Checksum Insertion Control (or TCP Payload Length[17..16])
	UINT32 bTSE: 1;                 //< TCP Segmentation Enable
	UINT32 uiTHL: 4;                //< TCP Header Length
	UINT32 uiSAIC: 2;               //< (NOT Configured) SA Insertion Control
	UINT32 bSAIC_MAC_REG_SEL: 1;    //< (NOT Configured) Select MAC Address Register 0 or 1 for SA
	UINT32 uiCPC: 2;                //< CRC Pad Control
	UINT32 bLD: 1;                  //< Last Descriptor
	UINT32 bFD: 1;                  //< First Descriptor
	UINT32 bCTXT: 1;                //< Context Type
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer

	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_TX_DESC, *PETH_TX_DESC;

/*
    Ethernet DMA tx queue node

    (CPU read, ethernet DMA write)
*/
typedef struct ETH_TX_DESC_READ {
	// Transmit Descriptor 0
	UINT32 uiTTSL;                  //< Transmit Packet Timestamp Low

	// Transmit Descriptor 1
	UINT32 uiTTSH;                  //< Transmit Packet Timestamp High

	// Transmit Descriptor 2
	UINT32 uiRsv1;

	// Transmit Descriptor 3
	UINT32 bIHE: 1;                 //< IP Header Error
	UINT32 bDB: 1;                  //< Deferred Bit
	UINT32 bUF: 1;                  //< Underflow Error
	UINT32 bED: 1;                  //< Excessive Deferral
	UINT32 uiCC: 4;                 //< Collision Count
	UINT32 bEC: 1;                  //< Excessive Collision
	UINT32 bLC: 1;                  //< Late Collision
	UINT32 bNC: 1;                  //< No Carrier
	UINT32 bLoC: 1;                 //< Loss of Carrier
	UINT32 bPCE: 1;                 //< Payload Checksum Error
	UINT32 bFF: 1;                  //< Packet Flush
	UINT32 bJT: 1;                  //< Jabber Timeout
	UINT32 bES: 1;                  //< Error Summary
	UINT32 bRsv2: 1;                //< Reserved
	UINT32 bTTSS: 1;                //< Tx Timestamp Status
	UINT32 uiRsv3: 10;              //< Reserved
	UINT32 bLD: 1;                  //< Last Descriptor
	UINT32 bFD: 1;                  //< First Descriptor
	UINT32 bCTXT: 1;                //< Context Type
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer
	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_TX_DESC_READ, *PETH_TX_DESC_READ;

/*
    Ethernet DMA tx context

    (tx context descriptor)
*/
typedef struct ETH_TX_DESC_CONTEXT {
	// Transmit Descriptor 0
	UINT32 uiTTSL;                  //< Transmit Packet Timestamp Low

	// Transmit Descriptor 1
	UINT32 uiTTSH;                  //< Transmit Packet Timestamp High

	// Transmit Descriptor 2
	UINT32 uiMSS: 14;               //< Maximum Segmentation Size
	UINT32 uiRsv1: 2;               //< Reserved
	UINT32 uiIVT: 16;               //< Inner VLAN Tag

	// Transmit Descriptor 3
	UINT32 uiVT: 16;                //< VLAN Tag
	UINT32 bVLTV: 1;                //< VLAN Tag Valid
	UINT32 bIVLTV: 1;               //< Inner VLAN Tag Valid
	UINT32 uiIVTIR: 2;              //< (NOT Configured) Inner VLAN Tag Insert or Replace
	UINT32 uiRsv2: 3;               //< Reserved
	UINT32 bCDE: 1;                 //< Context Descriptor Error
	UINT32 uiRsv3: 2;               //< Reserved
	UINT32 bTCMSSV: 1;              //< One-Step Timestamp Correction Input or MSS Valid
	UINT32 bOSTC: 1;                //< One-Step Timestamp Correction Enable
	UINT32 uiRsv4: 2;               //< Reserved
	UINT32 bCTXT: 1;                //< Context Type
	UINT32 bOWN: 1;                 //< Own Bit

	// User data 0
	UINT32 uiUsr1AP;                //< Buffer 1 Address Pointer

	// User data 1
	UINT32 uiUsr2AP;                //< Buffer 2 Address Pointer

	// Dummy
	UINT32 uiDummy;                 //< Dummy for desc word alig test
	UINT32 uiDummy2;                // align to 32 byte here
} ETH_TX_DESC_CONTEXT, *PETH_TX_DESC_CONTEXT;

/*
    Ethernet descriptor queue (include frame for descriptor)

    (assume we use ring structure. i.e. circular queue)
*/
typedef struct ETH_DESC_QUEUE {
	// configuration
	ETHMAC_DMA_DESCRIPT_MODE_ENUM queueType;    // queue type: ring or linked list
	UINT32 uiQueueBaseAddr;         //< Base address of queue
	UINT32 uiMaxDepth;              //< Max depth of this queue
	UINT32 uiFrameBaseAddr;         //< Base address of frame buffer for this queue

	// run-time state
	UINT32 uiCurrDepth;             //< Current depth
	UINT32 uiWritePos;              //< Next queue position to write
	UINT32 uiReadPos;               //< Next queue position to read
} ETH_DESC_QUEUE, *PETH_DESC_QUEUE;

//
// Exported functions
//
extern void ethDesc_initQueue(ETHMAC_DMA_DESCRIPT_MODE_ENUM txQueueMode,
							  UINT32 uiTxQueueBase,
							  UINT32 uiTxPacketBufBase,
							  ETHMAC_DMA_DESCRIPT_MODE_ENUM rxQueueMode,
							  UINT32 uiRxQueueBase,
							  UINT32 uiRxPacketBufBase);
extern BOOL ethDesc_isTxFull(void);
extern BOOL ethDesc_isTxEmpty(void);
extern ER   ethDesc_insertTxFrame(ETH_FRAME_HEAD *pFrameHead, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData);
extern UINT32 ethDesc_getLastTxDescAddr(void);
extern ER   ethDesc_removeTxFrame(void);
extern ER   ethDesc_retrieveRxFrame(UINT32 uiBufSize, UINT32 *puiLen, UINT8 *pData);

#endif


