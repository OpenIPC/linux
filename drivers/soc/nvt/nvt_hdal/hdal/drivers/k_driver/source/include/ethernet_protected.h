/*
    Protected header for Ethernet module

    Ethernet Configuration module protected header file.

    @file       Ethernet_protected.h
    @ingroup    mIDrvIO_Eth
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _ETHERNET_PROTECTED_H
#define _ETHERNET_PROTECTED_H

#include <kwrap/nvt_type.h>
#include "netif/nvt_eth.h"

//#define ETH_EXTERNAL_PHY  // External PHY
//#define ETH_EXTERNAL_CLK  // Input Mode
//#define ETH_RGMII
//#define RGMII_RX_DELAY_CHAIN


/*
    @addtogroup mIDrvIO_Eth
*/
//@{

/**
    @name Ethernet general call back prototype

    @param[in] UINT32 buffer address
    @param[in] UINT32 buffer length

    @return UINT32
*/
typedef UINT32(*ETH_CALLBACK_HDL)(UINT32, UINT32);

/*
    MAC Address ID

    @note ethernet IP has has multiple MAC address, total 32 MAC address
*/
typedef enum {
	ETH_MAC_ID_0,               //< MAC address 0
	ETH_MAC_ID_1,               //< MAC address 1
	ETH_MAC_ID_2,               //< MAC address 2
	ETH_MAC_ID_3,               //< MAC address 3
	ETH_MAC_ID_4,               //< MAC address 4
	ETH_MAC_ID_5,               //< MAC address 5
	ETH_MAC_ID_6,               //< MAC address 6
	ETH_MAC_ID_7,               //< MAC address 7
	ETH_MAC_ID_8,               //< MAC address 8
	ETH_MAC_ID_9,               //< MAC address 9
	ETH_MAC_ID_10,              //< MAC address 10
	ETH_MAC_ID_11,              //< MAC address 11
	ETH_MAC_ID_12,              //< MAC address 12
	ETH_MAC_ID_13,              //< MAC address 13
	ETH_MAC_ID_14,              //< MAC address 14
	ETH_MAC_ID_15,              //< MAC address 15

	ETH_MAC_ID_COUNT,           //< MAC address count

	ENUM_DUMMY4WORD(ETH_MAC_ID)
} ETH_MAC_ID;

/*
    Ethernet loopback mode

*/
/*
typedef enum {
	ETH_LOOPBACK_MODE_DISABLE,      //< Normal (disable loopback)
	ETH_LOOPBACK_MODE_MAC,          //< Loopback at MAC layer
	ETH_LOOPBACK_MODE_PHY,          //< Loopback at PHY layer
	ETH_LOOPBACK_MODE_EXTERNAL,     //< Loopback at External layer

	ENUM_DUMMY4WORD(ETH_LOOPBACK_MODE_ENUM)
} ETH_LOOPBACK_MODE_ENUM;
*/
/*
    Ethernet duplex mode

*/
typedef enum {
	ETH_DUPLEX_HALF,                //< Half duplex
	ETH_DUPLEX_FULL,                //< Full duplex

	ENUM_DUMMY4WORD(ETH_DUPLEX_ENUM)
} ETH_DUPLEX_ENUM;

/*
    Ethernet Configuration ID

*/
/*
typedef enum {
	ETH_CONFIG_ID_SET_MEM_REGION,   //< Install buffer address.
	//< Context should be DRAM address
	ETH_CONFIG_ID_MAC_ADDR,         //< local MAC address
	//< Context should be a unsigned char array
	//< length should be 6 bytes
	ETH_CONFIG_ID_LOOPBACK,         //< loopback option
	//< Context can be:
	//< - @b ETH_LOOPBACK_MODE_DISABLE
	//< - @b ETH_LOOPBACK_MODE_MAC
	//< - @b ETH_LOOPBACK_MODE_PHY
	ETH_CONFIG_ID_WRITE_DETECT_TEST,//< Write detect test option
	//< Context can be: TRUE or FALSE
	ETH_CONFIG_ID_SPEED,            //< Force MAC/PHY speed
	//< Context can be:
	//< - @b 10
	//< - @b 100
	//< - @b 1000
	ETH_CONFIG_ID_DUPLEX,           //< Force MAC/PHY duplex
	//< Context can be:
	//< - @b ETH_DUPLEX_HALF
	//< - @b ETH_DUPLEX_FULL


	ETH_CONFIG_ID_COUNT,            //< MAC address count

	ETH_CONFIG_SET_LPI,             //< Enable/Disable LPI
	ETH_CONFIG_SET_PMT,             //< Enable/Disable PMT
	ETH_CONFIG_SET_MSS,             //< Set MSS

	ENUM_DUMMY4WORD(ETH_CONFIG_ID)
} ETH_CONFIG_ID;
*/
/*
    Ethernet payload type

*/
/*
typedef enum {
	ETH_PAYLOAD_RAW,                //< Raw data
	ETH_PAYLOAD_IP,                 //< IP(v4) packet
	ETH_PAYLOAD_ARP,                //< ARP packet
	ETH_PAYLOAD_TCP,                //< TCP packet (IP/TCP)
	ETH_PAYLOAD_UDP,                //< UDP packet (IP/UDP)

	ENUM_DUMMY4WORD(ETH_PAYLOAD_ENUM)
} ETH_PAYLOAD_ENUM;
*/
/*
    Ethernet call back list
*/
typedef enum {
	ETH_CALLBACK_RX_DONE,           //< rx done

	ENUM_DUMMY4WORD(ETH_CALLBACK)
} ETH_CALLBACK;
/*
#define htons __builtin_bswap16
#define ntohs __builtin_bswap16
#define htonl __builtin_bswap32
#define ntohl __builtin_bswap32
*/
#define swap_uint32(x) ((((x) & 0x000000FF) << 24) | \
						(((x) & 0x0000FF00) << 8)   | \
						(((x) & 0x00FF0000) >> 8)   | \
						(((x) & 0xFF000000) >> 24))

#define swap_uint16(x) ((((x) & 0x00FF) << 8) | \
						(( (x) & 0xFF00) >>8 ))

extern void eth_init(void);
extern ER   eth_open(void);
extern ER   eth_close(void);
extern ER   eth_setConfig(ETH_CONFIG_ID configID, UINT32 uiConfig);
extern ER   eth_getConfig(ETH_CONFIG_ID configID, UINT32 *pConfigContext);
extern ER   eth_setCallBack(ETH_CALLBACK callBackID, ETH_CALLBACK_HDL pCallBack);
extern UINT32 eth_getPhyID(void);
extern ER   eth_send(UINT8 *pDstAddr, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData);
extern ER   eth_waitRcv(UINT32 uiBufSize, UINT32 *puiLen, UINT8 *pData);

extern ER   eth_send_prepare(UINT8 *pDstAddr, ETH_PAYLOAD_ENUM type, UINT32 uiLen, UINT8 *pData);
extern ER   eth_sendandwait(void);

extern void eth_clearWGisr(void);

//@}

#endif



