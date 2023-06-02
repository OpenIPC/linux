/*
    @file       Eth_phy.h

    @ingroup    mIDrvIO_Eth

    @brief      NT96510 Ethernet phy (RTL8201E) header file

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

*/

#ifndef _ETHERNET_PHY_H
#define _ETHERNET_PHY_H

//#include "DrvCommon.h"
#include "top.h"
#include "ethernet_protected.h"
#include "nvt_eth.h"

#define ETH_PHY_PHYADDR         (0)     // depend on PHYAD[0]/PHYAD[1] bootstrap on RTL8201E
#define ETH_REVMII_LOCAL_ADDR   0x0
#define ETH_REVMII_REMOTE_ADDR  0x0

#if 0
#define REG16VALUE              UINT16

#define REG16DEF_BEGIN(name)      \
	typedef union                   \
	{                               \
		REG16VALUE   Reg;            \
		struct                      \
		{                           \

#define REG16DEF_BIT(field, bits) \
	unsigned field : bits;

#define REG16DEF_END(name)        \
	} Bit;                      \
	} T_##name;
#endif

enum Mii_GEN_STATUS {
    Mii_100BaseT4              = 0x8000, // T4 capable
    Mii_100BaseTX_Full         = 0x4000, // 100M full
    Mii_100BaseTX_Half         = 0x2000, // 100M half
    Mii_10BaseT_Full           = 0x1000, // 10M full
    Mii_10BaseT_Half           = 0x0800, // 10M half

    Mii_AutoNegCmplt           = 0x0020, /* Autonegotiation completed    5    RW */
    Mii_Link                   = 0x0004, /* Link status                  2    RW */
};

enum Mii_LNK_PART_ABl {
    Mii_LinkAbl_100BaseT4      = 0x200,   // 100Base-T4 capable
    Mii_LinkAbl_100BaseTX_FD   = 0x100,   // 100Base-Tx full duplex
    Mii_LinkAbl_100BaseTX_HD   = 0x080,   // 100Base-Tx (half duplex)
    Mii_LinkAbl_10BaseT_FD     = 0x040,   // 10Base-T full duplex
    Mii_LinkAbl_10BaseT_HD     = 0x020,   // 10Base-T (half duplex)
};

/*
    PHY link state enum
*/
typedef enum {
	ETHPHY_LINKST_DOWN = 0,             //< Link down
	ETHPHY_LINKST_UP = 1,               //< Link up

	ENUM_DUMMY4WORD(ETHPHY_LINKST_ENUM)
} ETHPHY_LINKST_ENUM;

/*
    PHY speed enum
*/
typedef enum {
	ETHPHY_SPD_10 = 0,                  //< 10 Mbps
	ETHPHY_SPD_100 = 1,                 //< 100 Mbps
	ETHPHY_SPD_1000 = 2,                //< 1000 Mbps

	ENUM_DUMMY4WORD(ETHPHY_SPD_ENUM)
} ETHPHY_SPD_ENUM;

/*
    PHY duplex enum
*/
typedef enum {
	ETHPHY_DUPLEX_HALF = 0,             //< Half duplex
	ETHPHY_DUPLEX_FULL = 1,             //< Full duplex

	ENUM_DUMMY4WORD(ETHPHY_DUPLEX_ENUM)
} ETHPHY_DUPLEX_ENUM;

/*
    External PHY context structure

*/
typedef struct {
	ETHPHY_LINKST_ENUM linkState;   //< link state
	ETHPHY_SPD_ENUM network_speed;  //< network speed: 10Mbps or 100Mbps
	ETHPHY_DUPLEX_ENUM duplexMode;  //< duplex mode
	NVTETH_LINK_CB link_cb;
} EXTPHY_CONTEXT, *PEXTPHY_CONTEXT;

/*
    Definitions for ID1/ID2 register
*/
//@{
#define EXTPHY_OUI_MSB_DEFAULT          (0x1C)
#define EXTPHY_OUI_LSB_DEFAULT          (0x32)
#define EXTPHY_OUI_REVISION_DEFAULT     (0x05)
#define EXTPHY_OUI_MODEL_DEFAULT        (0x01)
//@}



//
// External PHY register definitions
//

#define EXTPHY_CONTROL_REG_OFS          0x00
REGDEF_BEGIN(EXTPHY_CONTROL_REG)      // Basic Mode Control Register
REGDEF_BIT(, 6)
REGDEF_BIT(SPD_1000, 1)           /* 0: see SPD_SET(bit13), 1: 1000bps */
REGDEF_BIT(COL_TEST, 1)           /* Collision test */
REGDEF_BIT(DUPLEX_MODE, 1)        /* 0: half-duplex, 1: full-duplex */
REGDEF_BIT(RESTART_AN, 1)         /* Restart auto-negotiation */
REGDEF_BIT(, 1)
REGDEF_BIT(POWER_DOWN, 1)         /* Power down PHY. But MDIO still alive */
REGDEF_BIT(AN_EN, 1)              /* Auto-negotiation Enable */
REGDEF_BIT(SPD_SET, 1)            /* 0: 10Mbps, 1: 100Mbps */
REGDEF_BIT(LOOPBACK, 1)           /* Loopback enable */
REGDEF_BIT(RESET, 1)              /* Reset */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_CONTROL_REG)

#define EXTPHY_STATUS_REG_OFS           0x01
REGDEF_BEGIN(EXTPHY_STATUS_REG)       // Basic Mode Status Register
REGDEF_BIT(, 2)
REGDEF_BIT(LINK_STS, 1)           /* 0: no link, 1: link established */
REGDEF_BIT(AN_ABILITY, 1)         /* 0: not support AN, 1: support AN */
REGDEF_BIT(REMOTE_FAULT, 1)       /* Remote fault detected (100Base-FX) */
REGDEF_BIT(AN_COMPLETE, 1)        /* Auto-negotiation complete */
REGDEF_BIT(MF_PREAMBPLE_SUPPRESSION, 1)   /* Preamble suppression */
REGDEF_BIT(, 4)
REGDEF_BIT(TEN_BASE_T_HD, 1)      /* 10Base-T Half-duplex */
REGDEF_BIT(TEN_BASE_T_FD, 1)      /* 10Base-T Full-duplex */
REGDEF_BIT(HUND_BASE_T_HD, 1)     /* 100Base-TX Half-duplex */
REGDEF_BIT(HUND_BASE_T_FD, 1)     /* 100Base-TX Full-duplex */
REGDEF_BIT(HUND_BASE_T4, 1)       /* 100Base-T4 */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_STATUS_REG)

#define EXTPHY_ID1_REG_OFS              0x02
REGDEF_BEGIN(EXTPHY_ID1_REG)          // PHY Identifier Register 1
REGDEF_BIT(OUI_MSB, 16)           /* Should be 0x1C */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_ID1_REG)

#define EXTPHY_ID2_REG_OFS              0x03
REGDEF_BEGIN(EXTPHY_ID2_REG)          // PHY Identifier Register 2
REGDEF_BIT(REVISION, 4)           /* Should be 0x05 */
REGDEF_BIT(MODEL, 6)              /* Should be 0x01 */
REGDEF_BIT(OUI_LSB, 6)            /* Should be 0x32 */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_ID2_REG)

#define EXTPHY_AN_ADV_REG_OFS           0x04
REGDEF_BEGIN(EXTPHY_AN_ADV_REG)       // Auto-negotiation Advertisement Register
REGDEF_BIT(SELECTOR, 5)           /* Should be 0x01. Ohterwise INVALID */
REGDEF_BIT(L_10BASE_T, 1)         /* Local support 10Base-T */
REGDEF_BIT(L_10FD, 1)             /* Local support 10Base-T Full-Duplex */
REGDEF_BIT(L_100BASE_TX, 1)       /* Local support 100Base-TX */
REGDEF_BIT(L_100TXFD, 1)          /* Local support 100Base-TX Full-Duplex */
REGDEF_BIT(L_T4, 1)               /* Local support 100Base-T4 */
REGDEF_BIT(L_RXFC, 1)             /* Local support RX Flow Control */
REGDEF_BIT(, 2)
REGDEF_BIT(RF, 1)                 /* Advertise Remote Fault detection */
REGDEF_BIT(ACK, 1)                /* Ackowledgement reception of link partner */
REGDEF_BIT(NP, 1)                 /* Next Page bit */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_AN_ADV_REG)

#define EXTPHY_AN_LINK_PARTNER_OFS      0x05
REGDEF_BEGIN(EXTPHY_AN_LINK_PARTNER)  // Auto-negotiation Link Partner Ability Register
REGDEF_BIT(SELECTOR, 5)           /* Should be 0x01. Ohterwise INVALID */
REGDEF_BIT(P_10BASE_T, 1)         /* Partner support 10Base-T */
REGDEF_BIT(P_10FD, 1)             /* Partner support 10Base-T Full-Duplex */
REGDEF_BIT(P_100BASE_TX, 1)       /* Partner support 100Base-TX */
REGDEF_BIT(P_100TXFD, 1)          /* Partner support 100Base-TX Full-Duplex */
REGDEF_BIT(P_T4, 1)               /* Partner support 100Base-T4 */
REGDEF_BIT(P_RXFC, 1)             /* Partner support RX Flow Control */
REGDEF_BIT(P_TXFC, 1)             /* Partner support TX Flow Control */
REGDEF_BIT(, 1)
REGDEF_BIT(RF, 1)                 /* Link partner indicates Remote Fault */
REGDEF_BIT(ACK, 1)                /* Link partner ackowledges reception */
REGDEF_BIT(NP, 1)                 /* Next Page bit */

REGDEF_BIT(, 16)
REGDEF_END(EXTPHY_AN_LINK_PARTNER)

#define EXTPHY_1000_CONTROL_REG_OFS     0x09
REGDEF_BEGIN(EXTPHY_1000_CONTROL_REG) // Basic Mode Control Register
REGDEF_BIT(, 8)
REGDEF_BIT(HALF_1000, 1)          /* 0: Do not advertise, 1: Advertise */
REGDEF_BIT(FULL_1000, 1)          /* 0: Do not advertise, 1: Advertise */

REGDEF_BIT(, 22)
REGDEF_END(EXTPHY_1000_CONTROL_REG)

#define EXTPHY_1000_STATUS_REG_OFS      0x0A
REGDEF_BEGIN(EXTPHY_1000_STATUS_REG)  // Basic Mode Status Register
REGDEF_BIT(, 10)
REGDEF_BIT(HALF_1000, 1)          /* 1: Link Partner is capable of 1000Base-T half duplex */
REGDEF_BIT(FULL_1000, 1)          /* 1: Link Partner is capable of 1000Base-T full duplex */

REGDEF_BIT(, 20)
REGDEF_END(EXTPHY_1000_STATUS_REG)

//
// Function declarations
//
extern ER   ethPHY_open(void);
extern ER   ethPHY_close(void);

extern void ethPHY_reset(void);
extern BOOL ethPHY_verifyID(void);

extern ER   ethPHY_waitAutoNegotiation(void);
extern ER   ethPHY_getLinkCapability(PEXTPHY_CONTEXT pPhyContext);

extern ER   ethPHY_setLoopback(BOOL bEnable, BOOL bAutoNegotiation);
extern ER   ethPHY_setSpeed(ETHMAC_SPD_ENUM speed);
extern ER   ethPHY_setDuplex(ETHMAC_DUPLEX_ENUM duplex);
extern UINT32 ethPHY_getID(void);
extern void ethPHY_setTrim(void);
extern void ethInternalPHY_powerOn(void);

extern UINT32 ethPHY_getControl(void);
extern void eqos_mac_init(PEXTPHY_CONTEXT priv);

#endif
//@}




