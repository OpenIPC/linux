#ifndef _ETHERNET_REG_H
#define _ETHERNET_REG_H

//#include "DrvCommon.h"
#include "rcw_macro.h"
#include "kwrap/type.h"

/*
    GMII Busy enum
*/
typedef enum {
	ETH_GMII_GB_READY = 0,              //< GMII MDIO ready
	ETH_GMII_GB_BUSY = 1,               //< GMII MDIO busy

	ENUM_DUMMY4WORD(ETH_GMII_GB_ENUM)
} ETH_GMII_GB_ENUM;

/*
    GMII Write/Read enum
*/
typedef enum {
	ETH_GMII_GOC_WRITE = 1,              //< GMII MDIO WRITE operation
	ETH_GMII_GOC_READ = 3,               //< GMII MDIO READ operation

	ENUM_DUMMY4WORD(ETH_GMII_GOC_ENUM)
} ETH_GMII_GOC_ENUM;

/*
    CSR clock range enum
*/
typedef enum {
	ETH_CR_ENUM_DIV42 = 0x00,           //< CSR Clock = APB/42
	ETH_CR_ENUM_DIV62 = 0x01,           //< CSR Clock = APB/62
	ETH_CR_ENUM_DIV16 = 0x02,           //< CSR Clock = APB/16
	ETH_CR_ENUM_DIV26 = 0x03,           //< CSR Clock = APB/26
	ETH_CR_ENUM_DIV102 = 0x04,          //< CSR Clock = APB/102
	ETH_CR_ENUM_DIV124 = 0x05,          //< CSR Clock = APB/124
	ETH_CR_ENUM_DIV4  = 0x08,           //< CSR Clock = APB/4
	ETH_CR_ENUM_DIV6  = 0x09,           //< CSR Clock = APB/6
	ETH_CR_ENUM_DIV8  = 0x0A,           //< CSR Clock = APB/8
	ETH_CR_ENUM_DIV10 = 0x0B,           //< CSR Clock = APB/10
	ETH_CR_ENUM_DIV12 = 0x0C,           //< CSR Clock = APB/12
	ETH_CR_ENUM_DIV14 = 0x0D,           //< CSR Clock = APB/14
	ETH_CR_ENUM_DIV16_2 = 0x0E,         //< CSR Clock = APB/16 (2nd path)
	ETH_CR_ENUM_DIV18 = 0x0F,           //< CSR Clock = APB/18

	ENUM_DUMMY4WORD(ETH_CR_ENUM)
} ETH_CR_ENUM;

/*
    PHY internal select enum
*/
typedef enum {
	ETH_PHYSEL_ENUM_GMII   = 1,         //< GMII/MII
	ETH_PHYSEL_ENUM_RGMII  = 2,         //< RGMII
	ETH_PHYSEL_ENUM_RMII   = 4,         //< RMII
	ETH_PHYSEL_ENUM_REVMII = 7,         //< RevMII

	ENUM_DUMMY4WORD(ETH_PHYSEL_ENUM)
} ETH_PHYSEL_ENUM;

#define ETH_MAC_CONFIG_REG_OFS      0x00
REGDEF_BEGIN(ETH_MAC_CONFIG_REG)        /* MAC Configuration Register */
REGDEF_BIT(RE, 1)                   /* RE: Receiver enable */
REGDEF_BIT(TE, 1)                   /* TE: Transmitter enable */
REGDEF_BIT(PRELEN, 2)               /* PRELEN: Preamble Length or Transmit Packets */
REGDEF_BIT(DC, 1)                   /* DC: Deferral check */
REGDEF_BIT(BL, 2)                   /* BL: Back-off limit */
REGDEF_BIT(, 1)
REGDEF_BIT(DR, 1)                   /* DR: Disable Retry */
REGDEF_BIT(DCRS, 1)                 /* DCRS: Disable CaRrier Sense during transmission */
REGDEF_BIT(DO, 1)                   /* DO: Disable (receive) Own */
REGDEF_BIT(ECRSFD, 1)               /* ECRSFD: Enable Carrier Sense Before Transmission in Full-Duplex mode */
REGDEF_BIT(LM, 1)                   /* LM: Loopback mode */
REGDEF_BIT(DM, 1)                   /* DM: Duplex mode */
REGDEF_BIT(FES, 1)                  /* FES: Fast Ethernet Speed */
REGDEF_BIT(PS, 1)                   /* PS: Port Select */
REGDEF_BIT(JE, 1)                   /* JE: Jumbo frame Enable */
REGDEF_BIT(JD, 1)                   /* JD: Jabber Disable */
REGDEF_BIT(BE, 1)                   /* BE: frame Burst Enable in 1Gbps half-duplex */
REGDEF_BIT(WD, 1)                   /* WD: Watchdog Disable */
REGDEF_BIT(ACS, 1)                  /* ACS: Automatic PAD/CRC stripping */
REGDEF_BIT(CST, 1)                  /* CST: Crc Stripping for Type frames */
REGDEF_BIT(S2KP, 1)                 /* S2KP: IEEE802.3as Support for 2K Packets */
REGDEF_BIT(GPSLCE, 1)               /* GPSLCE: Giant Packet Size Limit Control Enable */
REGDEF_BIT(IPG, 3)                  /* IPG: Inter Packet Gap */
REGDEF_BIT(IPC, 1)                  /* IPC: (IP) Checksum offload */
REGDEF_BIT(SARC, 3)                 /* SARC: Source Address Insertion or Replacement Control */
REGDEF_BIT(ARPEN, 1)                /* (NOT Configured) ARPEN: ARP Offload Enable */
REGDEF_END(ETH_MAC_CONFIG_REG)          /* MAC Configuration Register */

#define ETH_MAC_EXT_CONFIG_REG_OFS      0x04
REGDEF_BEGIN(ETH_MAC_EXT_CONFIG_REG)    /* MAC Ext Configuration Register */
REGDEF_BIT(GPSL, 14)                /* GPSL: Giant Packet Size Limit */
REGDEF_BIT(, 2)
REGDEF_BIT(DCRCC, 1)                /* DCRCC: Disable CRC Checking for Received Packets */
REGDEF_BIT(SPEN, 1)                 /* SPEN: Slow Protocol Detetion Enable */
REGDEF_BIT(USP, 1)                  /* USP: Unicast Slow Protocol Packet Detect */
REGDEF_BIT(, 1)
REGDEF_BIT(HDSMS, 3)                /* HDSMS: Maximum Size for Splitting the Header Data */
REGDEF_BIT(, 1)
REGDEF_BIT(EIPGEN, 1)               /* EIPGEN: Extended Inter-Packet Gap Enable */
REGDEF_BIT(EIPG, 5)                 /* EIPG: Extended Inter-Packet Gap */
REGDEF_BIT(, 2)
REGDEF_END(ETH_MAC_EXT_CONFIG_REG)      /* MAC Ext Configuration Register end */


#define ETH_MAC_PACKET_FILTER_REG_OFS   0x08
REGDEF_BEGIN(ETH_MAC_PACKET_FILTER_REG) /* MAC Packet Filter Register */
REGDEF_BIT(PR, 1)                   /* PR: Promiscuous Mode */
REGDEF_BIT(HUC, 1)                  /* HUC: Hash UniCast */
REGDEF_BIT(HMC, 1)                  /* HMC: Hash MultiCast */
REGDEF_BIT(DAIF, 1)                 /* DAIF: DA Inserse Filtering */
REGDEF_BIT(PM, 1)                   /* PM: Pass all Multicast */
REGDEF_BIT(DBF, 1)                  /* DBF: Disable Broadcast Frames */
REGDEF_BIT(PCF, 2)                  /* PCF: Pass Control Frames */
REGDEF_BIT(SAIF, 1)                 /* SAIF: SA Inserse Filtering */
REGDEF_BIT(SAF, 1)                  /* SAF: Source Address Filter enable */
REGDEF_BIT(HPF, 1)                  /* HPF: Hash or Perfect Filter */
REGDEF_BIT(, 5)
REGDEF_BIT(VTFE, 1)                 /* VTFE: VLAN Tag Filter Enable */
REGDEF_BIT(, 3)
REGDEF_BIT(IPFE, 1)                 /* IPFE: Layer3/4 Filter Enable */
REGDEF_BIT(DNTU, 1)                 /* DNTU: Drop Non-TCP/UDP Enable */
REGDEF_BIT(, 9)
REGDEF_BIT(RA, 1)                   /* RA: Receive All */
REGDEF_END(ETH_MAC_PACKET_FILTER_REG)   /* MAC Packet Filter Register end */

#define ETH_WDT_TIMEOUT_REG_OFS     0x0C
REGDEF_BEGIN(ETH_WDT_TIMEOUT_REG)       /* MAC Watchdog Timeout Register */
REGDEF_BIT(WTO, 4)                  /* Watchdog Timeout */
REGDEF_BIT(, 4)
REGDEF_BIT(PWE, 1)                  /* Programmable Watchdog Enable */
REGDEF_BIT(, 23)
REGDEF_END(ETH_WDT_TIMEOUT_REG)         /* MAC Watchdog Timeout Register end */

#define ETH_HASH_TBL_REG0_OFS       0x10
REGDEF_BEGIN(ETH_HASH_TBL_REG0)         /* Hash Table High Register 0 */
REGDEF_BIT(HT31T0, 32)              /* Hash Table First 32 bits */
REGDEF_END(ETH_HASH_TBL_REG0)           /* Hash Table High Register 0 end */

#define ETH_HASH_TBL_REG1_OFS       0x14
REGDEF_BEGIN(ETH_HASH_TBL_REG1)         /* Hash Table High Register 1 */
REGDEF_BIT(HT63T32, 32)              /* Hash Table First 32 bits */
REGDEF_END(ETH_HASH_TBL_REG1)           /* Hash Table High Register 1 end */

#define ETH_VLAN_TAG_REG_OFS        0x50
REGDEF_BEGIN(ETH_VLAN_TAG_REG)          /* VLAN Tag Register */
REGDEF_BIT(VL, 16)                  /* VL: VLAN tag identifier */
REGDEF_BIT(ETV, 1)                  /* ETV: Enable 12bit VLAN Tag Comparison */
REGDEF_BIT(VTIM, 1)                 /* VTIM: VLAN Tag Inverse Match Enable */
REGDEF_BIT(ESVL, 1)                 /* ESVL: Enable S-VLAN */
REGDEF_BIT(ERSVLM, 1)               /* ERSVLM: Enable Receive S-VLAN Match */
REGDEF_BIT(DOVLTC, 1)               /* DOVLTC: Disable VLAN Type Check */
REGDEF_BIT(ELVS, 2)                 /* ELVS: Enable VLAN Tag Stripping on Receive */
REGDEF_BIT(, 1)
REGDEF_BIT(EVLRXS, 1)               /* EVLRXS: Enable VLAN Tag in Rx status */
REGDEF_BIT(VTHM, 1)                 /* VTHM: VLAN Tag Hash Table Match Enable */
REGDEF_BIT(EDVLP, 1)                /* EDVLP: Enable Double VLAN Processing */
REGDEF_BIT(ERIVLT, 1)               /* ERIVLT: Enable Inner VLAN Tag */
REGDEF_BIT(EIVLS, 2)                /* EIVLS: Enable Inner VLAN Tag Stripping on Receive */
REGDEF_BIT(, 1)
REGDEF_BIT(EIVLRXS, 1)              /* EIVLRXS: Enable Inner VLAN Tag in Rx status */
REGDEF_END(ETH_VLAN_TAG_REG)            /* VLAN Tag Register end  */

#define ETH_VLAN_INCL_REG_OFS       0x60
REGDEF_BEGIN(ETH_VLAN_INCL_REG)         /* VLAN Incl Register */
REGDEF_BIT(VTL, 16)                 /* VTL: VLAN tag for Transmit Packets */
REGDEF_BIT(VLC, 2)                  /* VLC: VLAN tag control in Transmit Packets */
REGDEF_BIT(VLP, 1)                  /* VLP: VLAN Priority Control */
REGDEF_BIT(CSVL, 1)                 /* CSVL: C-VLAN or S-VLAN */
REGDEF_BIT(VLTI, 1)                 /* VLTI: VLAN Tag Input */
REGDEF_BIT(CBTI, 1)                 /* CBTI: Channel Based Tag insertion */
REGDEF_BIT(, 2)
REGDEF_BIT(ADDR, 1)                 /* ADDR: Address */
REGDEF_BIT(, 5)
REGDEF_BIT(RDWR, 1)                 /* RDWR: Read write control */
REGDEF_BIT(BUSY, 1)                 /* BUSY: Busy */
REGDEF_END(ETH_VLAN_INCL_REG)           /* VLAN Incl Register end  */

#define ETH_Q0_TX_FLOW_CTRL_REG_OFS 0x70
REGDEF_BEGIN(ETH_Q0_TX_FLOW_CTRL_REG)   /* Tx Flow Control Register */
REGDEF_BIT(FCB_BPA, 1)              /* Flow Control Busy/Backpressure Active */
REGDEF_BIT(TFE, 1)                  /* TFE: Transmit Flow control Enable */
REGDEF_BIT(, 2)
REGDEF_BIT(PLT, 3)                  /* PLT: Pause Low Threshold */
REGDEF_BIT(DZPQ, 1)                 /* DZPQ: Disable Zero-Quanta Pause */
REGDEF_BIT(, 8)
REGDEF_BIT(PT, 16)                  /* PT: Pause Time */
REGDEF_END(ETH_Q0_TX_FLOW_CTRL_REG)     /* Tx Flow Control Register end */

#define ETH_Q0_RX_FLOW_CTRL_REG_OFS 0x90
REGDEF_BEGIN(ETH_Q0_RX_FLOW_CTRL_REG)   /* Rx Flow Control Register */
REGDEF_BIT(RFE, 1)                  /* RFE: Receive Flow control Enable */
REGDEF_BIT(UP, 1)                   /* UP: Unicast Pause Frame Detect */
REGDEF_BIT(, 30)
REGDEF_END(ETH_Q0_RX_FLOW_CTRL_REG)     /* Rx Flow Control Register end */

#define ETH_INT_STS_REG_OFS         0xB0
REGDEF_BEGIN(ETH_INT_STS_REG)           /* Ethernet Interrupt Status Register */
REGDEF_BIT(RGSMIIS, 1)              /* RGMII/SMII interrupt status */
REGDEF_BIT(PCSLCHGIS, 1)            /* (NOT Configured) PCS Link status changed */
REGDEF_BIT(PCSANCIS, 1)             /* (NOT Configured) PCS Auto-Negotiation Complete */
REGDEF_BIT(PHYIS, 1)                /* PHY interrupt */
REGDEF_BIT(PMTIS, 1)                /* PMT interrupt status */
REGDEF_BIT(LPIIS, 1)                /* LPI interrupt status */
REGDEF_BIT(, 2)
REGDEF_BIT(MMCIS, 1)                /* MMC Interrupt status */
REGDEF_BIT(MMCRXIS, 1)              /* MMC Receive Interrupt status */
REGDEF_BIT(MMCTXIS, 1)              /* MMC Transmit Interrupt status */
REGDEF_BIT(MMCRXIPIS, 1)            /* MMC Receive Checksum Offload status */
REGDEF_BIT(TSIS, 1)                 /* Timestamp interrupt status */
REGDEF_BIT(TXSTSIS, 1)              /* Transmit status interrupt */
REGDEF_BIT(RXSTSIS, 1)              /* Receive status interrupt */
REGDEF_BIT(GPIIS, 1)                /* (NOT Configured) GPI Interrupt Status */
REGDEF_BIT(, 2)                		/* (NOT Configured) GPI Interrupt Status */
REGDEF_BIT(MDIOIS, 1)               /* (NOT Configured) GPI Interrupt Status */
REGDEF_BIT(, 13)
REGDEF_END(ETH_INT_STS_REG)             /* Ethernet Interrupt Status Register end  */

#define ETH_INT_EN_REG_OFS          0xB4
REGDEF_BEGIN(ETH_INT_EN_REG)            /* Ethernet Interrupt Status Register */
REGDEF_BIT(RGSMIIIE, 1)             /* RGMII/SMII interrupt enable */
REGDEF_BIT(PCSLCHGIE, 1)            /* (NOT Configured) PCS Link status changed interrupt enable */
REGDEF_BIT(PCSANCIE, 1)             /* (NOT Configured) PCS Auto-Negotiation Complete interrupt enable */
REGDEF_BIT(PHYIE, 1)                /* PHY interrupt enable */
REGDEF_BIT(PMTIE, 1)                /* PMT interrupt enable */
REGDEF_BIT(LPIIE, 1)                /* LPI interrupt enable */
REGDEF_BIT(, 6)
REGDEF_BIT(TSIE, 1)                 /* Timestamp interrupt enable */
REGDEF_BIT(TXSTSIE, 1)              /* Transmit status interrupt enable */
REGDEF_BIT(RXSTSIE, 1)              /* Receive status interrupt enable */
REGDEF_BIT(, 3)
REGDEF_BIT(MDIOIE, 1)               /* (NOT Configured) GPI Interrupt Status */
REGDEF_BIT(, 13)
REGDEF_END(ETH_INT_EN_REG)              /* Ethernet Interrupt Status Register end  */

#define ETH_RX_TX_STS_REG_OFS       0xB8
REGDEF_BEGIN(ETH_RX_TX_STS_REG)         /* MAC Rx Tx Status Register */
REGDEF_BIT(TJT, 1)                  /* Transmit Jabber Timeout */
REGDEF_BIT(NCARR, 1)                /* No Carrier */
REGDEF_BIT(LCARR, 1)                /* Loss Carrier */
REGDEF_BIT(EXDEF, 1)                /* Excessive Deferral */
REGDEF_BIT(LCOL, 1)                 /* Late Collision */
REGDEF_BIT(EXCOL, 1)                /* Excessive Collision */
REGDEF_BIT(, 2)
REGDEF_BIT(RWT, 1)                  /* Receive Watchdog Timeout */
REGDEF_BIT(, 23)
REGDEF_END(ETH_RX_TX_STS_REG)           /* MAC Rx Tx Status Register end */

#define ETH_PMT_CTRL_STS_REG_OFS    0xC0
REGDEF_BEGIN(ETH_PMT_CTRL_STS_REG)      /* PMT Control Status Register */
REGDEF_BIT(PWRDWN, 1)               /* Power down mode */
REGDEF_BIT(MGKPKTEN, 1)             /* Magic packet enable */
REGDEF_BIT(RWKPKTEN, 1)             /* Remote wake-up packet enable */
REGDEF_BIT(, 2)
REGDEF_BIT(MGKPRCVD, 1)             /* Magic packet received */
REGDEF_BIT(RWKPRCVD, 1)             /* Remote wake-up packet received */
REGDEF_BIT(, 17)
REGDEF_BIT(RWKPTR, 5)               /* Remote wake-up FIFO pointer */
REGDEF_BIT(, 2)
REGDEF_BIT(RWKFILTRST, 1)           /* Remote wake-up packet filter register pointer reset */
REGDEF_END(ETH_PMT_CTRL_STS_REG)        /* PMT Control Status Register end */

#define ETH_PMT_FLITER_REG_OFS      0xC4
REGDEF_BEGIN(ETH_PMT_FLITER_REG)        /* PMT Filter Register */
REGDEF_BIT(, 32)
REGDEF_END(ETH_PMT_FLITER_REG)          /* PMT Filter Register end */

#define ETH_LPI_CTRL_STS_REG_OFS    0xD0
REGDEF_BEGIN(ETH_LPI_CTRL_STS_REG)      /* LPI Control Status Register */
REGDEF_BIT(TLPIEN, 1)               /* Transmit LPI Entry */
REGDEF_BIT(TLPIEX, 1)               /* Transmit LPI Exit */
REGDEF_BIT(RLPIEN, 1)               /* Receive LPI Entry */
REGDEF_BIT(RLPIEX, 1)               /* Receive LPI Exit */
REGDEF_BIT(, 4)
REGDEF_BIT(TLPIST, 1)               /* Transmit LPI State */
REGDEF_BIT(RLPIST, 1)               /* Receive LPI State */
REGDEF_BIT(, 6)
REGDEF_BIT(LPIEN, 1)                /* LPI enable */
REGDEF_BIT(PLS, 1)                  /* PHY Link Status */
REGDEF_BIT(PLSEN, 1)                /* PHY Link Status Enable */
REGDEF_BIT(LPITXA, 1)               /* LPI Tx Automate */
REGDEF_BIT(LPIATE, 1)               /* LPI Timer Enable */
REGDEF_BIT(LPITCSE, 1)              /* LPI Tx Clock Stop Enable*/
REGDEF_BIT(, 10)
REGDEF_END(ETH_LPI_CTRL_STS_REG)        /* LPI Control Status Register end */

#define ETH_LPI_TIMERS_CTRL_REG_OFS 0xD4
REGDEF_BEGIN(ETH_LPI_TIMERS_CTRL_REG)   /* LPI Timers Control Register */
REGDEF_BIT(TWT, 16)                 /* LPI TW Timer */
REGDEF_BIT(LST, 10)                 /* LPI LS Timer */
REGDEF_BIT(, 6)
REGDEF_END(ETH_LPI_TIMERS_CTRL_REG)     /* LPI Timers Control Register end */

#define ETH_LPI_ENTRY_TIMER_REG_OFS 0xD8
REGDEF_BEGIN(ETH_LPI_ENTRY_TIMER_REG)   /* LPI Entry Timer Register */
REGDEF_BIT(, 3)
REGDEF_BIT(LPIET, 17)               /* LPI Entry Timer */
REGDEF_BIT(, 12)
REGDEF_END(ETH_LPI_ENTRY_TIMER_REG)     /* LPI Entry Timer Register end */

#define ETH_1US_TIC_CNTR_REG_OFS    0xDC
REGDEF_BEGIN(ETH_1US_TIC_CNTR_REG)      /* MAC 1US Tic Counter Register */
REGDEF_BIT(TIC_1US_CNTR, 12)        /* 1US TIC Counter */
REGDEF_BIT(, 20)
REGDEF_END(ETH_1US_TIC_CNTR_REG)        /* MAC 1US Tic Counter Register end */

#define ETH_PHYIF_CTRL_STS_REG_OFS  0xF8
REGDEF_BEGIN(ETH_PHYIF_CTRL_STS_REG)    /* MAC PHYIF Control Status Register */
REGDEF_BIT(TC, 1)                   /* Transmit Configuration in RGMII, SGMII, or SMII */
REGDEF_BIT(LUD, 1)                  /* Link Up or Down */
REGDEF_BIT(SFTERR, 1)               /* (NOT Configured) SMII Force Transmit Error */
REGDEF_BIT(, 1)
REGDEF_BIT(SMIDRXS, 1)              /* (NOT Configured) Delay SMII Rx Data Sampling with respect to the SMII SYNC Signal */
REGDEF_BIT(, 11)
REGDEF_BIT(LNKMOD, 1)               /* Link Mode */
REGDEF_BIT(LNKSPEED, 1)             /* Link Speed */
REGDEF_BIT(LNKSTS, 1)               /* Link Status */
REGDEF_BIT(JABTO, 1)                /* Jabber Timeout */
REGDEF_BIT(FALSCARDET, 1)           /* False Carrier Detected */
REGDEF_BIT(, 10)
REGDEF_END(ETH_PHYIF_CTRL_STS_REG)      /* MAC PHYIF Control Status Register end */

#define ETH_VERSION_REG_OFS         0x110
REGDEF_BEGIN(ETH_VERSION_REG)           /* Version Register */
REGDEF_BIT(SNPSVER, 8)              /* Synopsys version: 0x42 */
REGDEF_BIT(USERVER, 8)              /* User-defined version: 0x10 */
REGDEF_BIT(, 16)
REGDEF_END(ETH_VERSION_REG)             /* Version Register end  */

#define ETH_DEBUG_REG_OFS           0x114
REGDEF_BEGIN(ETH_DEBUG_REG)             /* Debug Register */
REGDEF_BIT(RPESTS, 1)
REGDEF_BIT(RFCSTS, 2)
REGDEF_BIT(, 13)
REGDEF_BIT(TPESTS, 1)
REGDEF_BIT(TFCSTS, 2)
REGDEF_BIT(, 13)
REGDEF_END(ETH_DEBUG_REG)               /* Debug Register end  */

#define ETH_HW_FEATURE0_REG_OFS     0x11C
REGDEF_BEGIN(ETH_HW_FEATURE0_REG)       /* HW Feature0 Register */
REGDEF_BIT(MIISEL, 1)               /* 10/100 Mbps support */
REGDEF_BIT(GMIISEL, 1)              /* 1000 Mbps support */
REGDEF_BIT(HDSEL, 1)                /* Half-duplex support */
REGDEF_BIT(PCSSEL, 1)               /* PCS Registers support (TBI/SGMII/RTBI) */
REGDEF_BIT(VLHASH, 1)               /* VLAN Hash Filter selected */
REGDEF_BIT(SMASEL, 1)               /* SMA (MDIO) Interface */
REGDEF_BIT(RWKSEL, 1)               /* PMT Remote Wake-up Packet Enable */
REGDEF_BIT(MGKSEL, 1)               /* PMT Magic Packet Enable */
REGDEF_BIT(MMCSEL, 1)               /* RMON Module Enable */
REGDEF_BIT(ARPOFFSEL, 1)            /* ARP Offload Enable */
REGDEF_BIT(, 2)
REGDEF_BIT(TSSEL, 1)                /* IEEE1588-2008 Timestamp Enable */
REGDEF_BIT(EEESEL, 1)               /* Energy Efficient Ethernet Enable */
REGDEF_BIT(TXCOESEL, 1)             /* Transmit Checksum Offload Enable */
REGDEF_BIT(, 1)
REGDEF_BIT(RXCOESEL, 1)             /* Receive Checksum Offload Enable */
REGDEF_BIT(, 1)
REGDEF_BIT(ADDMACADRSEL, 5)         /* MAC Address 1~31 selected */
REGDEF_BIT(MACADR32SEL, 1)          /* MAC Address 32~63 selected */
REGDEF_BIT(MACADR64SEL, 1)          /* MAC Address 64~127 selected */
REGDEF_BIT(TSSTSSEL, 2)             /* Timestamp System Time Source */
REGDEF_BIT(SAVLANINS, 1)            /* Source Address or VLAN Insertion Enable */
REGDEF_BIT(ACTPHYSEL, 3)            /* Active PHY Selected */
REGDEF_BIT(, 1)
REGDEF_END(ETH_HW_FEATURE0_REG)         /* HW Feature0 Register end */

#define ETH_HW_FEATURE1_REG_OFS     0x120
REGDEF_BEGIN(ETH_HW_FEATURE1_REG)       /* HW Feature1 Register */
REGDEF_BIT(RXFIFOSIZE, 5)           /* MTL Receive FIFO Size */
REGDEF_BIT(SPRAM, 1)                /* Single Port RAM Enable */
REGDEF_BIT(TXFIFOSIZE, 5)           /* MTL Transmit FIFO Size */
REGDEF_BIT(OSTEN, 1)                /* One-Step Timestamping Enable */
REGDEF_BIT(PTOEN, 1)                /* PTP Offload Enable */
REGDEF_BIT(ADVTHWORD, 1)            /* IEEE1588 High Word Register Enable */
REGDEF_BIT(ADDR64, 2)               /* Address Width */
REGDEF_BIT(DCBEN, 1)                /* Data Center Bridging Feature Enable */
REGDEF_BIT(SPHEN, 1)                /* Split Header Feature Enable */
REGDEF_BIT(TSOEN, 1)                /* TCP Segmentation Offload Enable */
REGDEF_BIT(DBGMEMA, 1)              /* DMA Debug Register Enable */
REGDEF_BIT(AVSEL, 1)                /* AV Feature Enable */
REGDEF_BIT(RAVSEL, 1)               /* Rx Side Only AV Feature Enable */
REGDEF_BIT(, 1)
REGDEF_BIT(POUOST, 1)               /* One Step for PTP over UDP/IP Feature Enable */
REGDEF_BIT(HASHTBLSZ, 2)            /* Hash Table Size */
REGDEF_BIT(, 1)
REGDEF_BIT(L3L4FNUM, 4)             /* Total number of L3 or L4 Filters */
REGDEF_BIT(, 1)
REGDEF_END(ETH_HW_FEATURE1_REG)         /* HW Feature1 Register end */

#define ETH_HW_FEATURE2_REG_OFS     0x124
REGDEF_BEGIN(ETH_HW_FEATURE2_REG)       /* HW Feature2 Register */
REGDEF_BIT(RXQCNT, 4)               /* Number of MTL Receive Queues */
REGDEF_BIT(, 2)
REGDEF_BIT(TXQCNT, 4)               /* Number of MTL Transmit Queues */
REGDEF_BIT(, 2)
REGDEF_BIT(RXCHCNT, 4)              /* Number of MTL Receive Channels */
REGDEF_BIT(, 2)
REGDEF_BIT(TXCHCNT, 4)              /* Number of MTL Transmit Channels */
REGDEF_BIT(, 2)
REGDEF_BIT(PPSOUTNUM, 3)            /* Number of PPS Outputs */
REGDEF_BIT(, 1)
REGDEF_BIT(AUXNAPNUM, 3)            /* Number of Auxiliary Snapshot Inputs */
REGDEF_BIT(, 1)
REGDEF_END(ETH_HW_FEATURE2_REG)         /* HW Feature2 Register end */

#define ETH_HW_FEATURE3_REG_OFS     0x128
REGDEF_BEGIN(ETH_HW_FEATURE3_REG)       /* HW Feature3 Register */
REGDEF_BIT(NRVF, 3)                 /* Number of Extended VLAN Tag Filters Enabled */
REGDEF_BIT(, 1)
REGDEF_BIT(CBTISEL, 1)              /* Queue/Channel Based VLAN tag insertion on Tx Enable */
REGDEF_BIT(, 27)
REGDEF_END(ETH_HW_FEATURE3_REG)         /* HW Feature3 Register end */

#define ETH_MDIO_ADDR_REG_OFS       0x200
REGDEF_BEGIN(ETH_MDIO_ADDR_REG)         /* MDIO Address Register */
REGDEF_BIT(GB, 1)                   /* GB: GMII Busy */
REGDEF_BIT(C45E, 1)                 /* C45E: Clause 45 PHY Enable */
REGDEF_BIT(GOC, 2)                  /* GOC: GMII Operation Command */
REGDEF_BIT(SKAP, 1)                 /* SKAP: Skip Address Packet */
REGDEF_BIT(, 3)
REGDEF_BIT(CR, 4)                   /* CR: csr Clock Range */
REGDEF_BIT(NTC, 3)                  /* NTC: Number of Trailing Clocks */
REGDEF_BIT(, 1)
REGDEF_BIT(RDA, 5)                  /* RDA: Register/Device Address */
REGDEF_BIT(PA, 5)                   /* PA: Physical layer Address */
REGDEF_BIT(BTB, 1)                  /* BTB: Back to Back transactions */
REGDEF_BIT(PSE, 1)                  /* PSE: Preamble Supression Enable */
REGDEF_BIT(, 4)
REGDEF_END(ETH_MDIO_ADDR_REG)           /* MDIO Address Register end  */

#define ETH_MDIO_DATA_REG_OFS       0x204
REGDEF_BEGIN(ETH_MDIO_DATA_REG)         /* MDIO Data Register */
REGDEF_BIT(GD, 16)                  /* GD: GMII Data */
REGDEF_BIT(RA, 16)                  /* RA: Register Address */
REGDEF_END(ETH_MDIO_DATA_REG)           /* MDIO Data Register end  */

#define ETH_CSR_SW_CTRL_REG_OFS     0x230
REGDEF_BEGIN(ETH_CSR_SW_CTRL_REG)       /* CSR SW Control Register */
REGDEF_BIT(RCWE, 1)                 /* RCWE: Register Clear on Write 1 Enable */
REGDEF_BIT(, 31)
REGDEF_END(ETH_CSR_SW_CTRL_REG)         /* CSR SW Control Register end  */

#define ETH_MAC_ADDR0_HIGH_REG_OFS  0x300
REGDEF_BEGIN(ETH_MAC_ADDR0_HIGH_REG)    /* MAC Address 0 High Register */
REGDEF_BIT(ADDRHI, 16)              /* bit[47..32] of MAC addr */
REGDEF_BIT(rsv, 15)
REGDEF_BIT(AE, 1)                   /* AE: always 1 */
REGDEF_END(ETH_MAC_ADDR0_HIGH_REG)      /* MAC Address 0 High Register end */

#define ETH_MAC_ADDR0_LOW_REG_OFS   0x304
REGDEF_BEGIN(ETH_MAC_ADDR0_LOW_REG)     /* MAC Address 0 High Register */
REGDEF_BIT(ADDRLO, 32)              /* bit[31..0] of MAC addr */
REGDEF_END(ETH_MAC_ADDR0_LOW_REG)       /* MAC Address 0 High Register end */

#define ETH_MAC_ADDR1_HIGH_REG_OFS  0x308
#define ETH_MAC_ADDR2_HIGH_REG_OFS  0x310
#define ETH_MAC_ADDR3_HIGH_REG_OFS  0x318
#define ETH_MAC_ADDR4_HIGH_REG_OFS  0x320
#define ETH_MAC_ADDR5_HIGH_REG_OFS  0x328
#define ETH_MAC_ADDR6_HIGH_REG_OFS  0x330
#define ETH_MAC_ADDR7_HIGH_REG_OFS  0x338
#define ETH_MAC_ADDR8_HIGH_REG_OFS  0x340
#define ETH_MAC_ADDR9_HIGH_REG_OFS  0x348
#define ETH_MAC_ADDR10_HIGH_REG_OFS 0x350
#define ETH_MAC_ADDR11_HIGH_REG_OFS 0x358
#define ETH_MAC_ADDR12_HIGH_REG_OFS 0x360
#define ETH_MAC_ADDR13_HIGH_REG_OFS 0x368
#define ETH_MAC_ADDR14_HIGH_REG_OFS 0x370
#define ETH_MAC_ADDR15_HIGH_REG_OFS 0x378
REGDEF_BEGIN(ETH_MAC_ADDR1_HIGH_REG)    /* MAC Address 1 High Register */
REGDEF_BIT(ADDRHI, 16)              /* bit[47..32] of MAC addr */
REGDEF_BIT(rsv, 8)
REGDEF_BIT(MBC, 6)                  /* Mask Byte Control */
REGDEF_BIT(SA, 1)                   /* Source Address */
REGDEF_BIT(AE, 1)                   /* Address Enable */
REGDEF_END(ETH_MAC_ADDR1_HIGH_REG)      /* MAC Address 1 High Register end */

#define ETH_MAC_ADDR1_LOW_REG_OFS   0x30C
#define ETH_MAC_ADDR2_LOW_REG_OFS   0x314
#define ETH_MAC_ADDR3_LOW_REG_OFS   0x31C
#define ETH_MAC_ADDR4_LOW_REG_OFS   0x324
#define ETH_MAC_ADDR5_LOW_REG_OFS   0x32C
#define ETH_MAC_ADDR6_LOW_REG_OFS   0x334
#define ETH_MAC_ADDR7_LOW_REG_OFS   0x33C
#define ETH_MAC_ADDR8_LOW_REG_OFS   0x344
#define ETH_MAC_ADDR9_LOW_REG_OFS   0x34C
#define ETH_MAC_ADDR10_LOW_REG_OFS  0x354
#define ETH_MAC_ADDR11_LOW_REG_OFS  0x35C
#define ETH_MAC_ADDR12_LOW_REG_OFS  0x364
#define ETH_MAC_ADDR13_LOW_REG_OFS  0x36C
#define ETH_MAC_ADDR14_LOW_REG_OFS  0x374
#define ETH_MAC_ADDR15_LOW_REG_OFS  0x37C
REGDEF_BEGIN(ETH_MAC_ADDR1_LOW_REG)     /* MAC Address 1 High Register */
REGDEF_BIT(ADDRLO, 32)              /* bit[31..0] of MAC addr */
REGDEF_END(ETH_MAC_ADDR1_LOW_REG)       /* MAC Address 1 High Register end */

#define ETH_MMC_RX_INT_MSK_REG_OFS      0x70C
#define ETH_MMC_TX_INT_MSK_REG_OFS      0x710
#define ETH_MMC_RX_IPC_INT_MSK_REG_OFS  0x800

#define ETH_TIMESTAMP_CTRL_REG_OFS  0xB00
REGDEF_BEGIN(ETH_TIMESTAMP_CTRL_REG)    /* Timestamp Control Register */
REGDEF_BIT(TSENA, 1)                /* TSENA: Enable Timestamp */
REGDEF_BIT(TSCFUPDT, 1)             /* TSCFUPDT: Fine or Coarse Timestamp Update */
REGDEF_BIT(TSINIT, 1)               /* TSINIT: Initialize Timestamp */
REGDEF_BIT(TSUPDT, 1)               /* TSUPDT: Update Timestamp */
REGDEF_BIT(TSTRIG, 1)               /* TSTRIG: Enable Timestamp Interrupt Trigger */
REGDEF_BIT(TSADDREG, 1)             /* TSADDREG: Update Addend Register */
REGDEF_BIT(, 2)
REGDEF_BIT(TSENALL, 1)              /* TSENALL: Enable Timestamp for All Packets */
REGDEF_BIT(TSCTRLSSR, 1)            /* TSCTRLSSR: Timestamp Digital or Binary Rollover Control */
REGDEF_BIT(TSVER2ENA, 1)            /* TSVER2ENA: Enable PTP Packet Processing for version 2 format */
REGDEF_BIT(TSIPENA, 1)              /* TSIPENA: Enable Processing of PTP over Ethernet Packets */
REGDEF_BIT(TSIPV6ENA, 1)            /* TSIPV6ENA: Enable Processing of PTP Sent over IPv6-UDP */
REGDEF_BIT(TSIPV4ENA, 1)            /* TSIPV4ENA: Enable Processing of PTP Sent over IPv4-UDP */
REGDEF_BIT(TSEVNTENA, 1)            /* TSEVNTENA: Enable Timestamp Snapshot for Event Messages */
REGDEF_BIT(TSMSTRENA, 1)            /* TSMSTRENA: Enable Snapshot for Messages Relevant to Master */
REGDEF_BIT(SNAPTYPSEL, 2)           /* SNAPTYPSEL: Select PTP packets for Taking Snapshots */
REGDEF_BIT(TSENMACADDR, 1)          /* TSENMACADDR: Enable MAC Address for PTP Packet Filtering */
REGDEF_BIT(CSC, 1)                  /* CSC: Enable checksum correction during OST for PTP over UDP/IPv4 packets */
REGDEF_BIT(ESTI, 1)                 /* (NOT Configured) ESTI: External System Time Input */
REGDEF_BIT(, 3)
REGDEF_BIT(TXTSSTSM, 1)             /* TXTSSTSM: Transmit Timestamp Status Mode */
REGDEF_BIT(, 3)
REGDEF_BIT(AV8021ASMEN, 1)          /* TXTSSTSM: AV 802.1AS Mode Enable */
REGDEF_BIT(, 3)
REGDEF_END(ETH_TIMESTAMP_CTRL_REG)      /* Timestamp Control Register end */

#define ETH_SUB_SECOND_REG_OFS      0xB04
REGDEF_BEGIN(ETH_SUB_SECOND_REG)        /* MAC Sub Second Register */
REGDEF_BIT(, 8)
REGDEF_BIT(SNSINC, 8)               /* (NOT Configured) SNSINC: Sub-nanosecond Increment Value */
REGDEF_BIT(SSINC, 8)                /* SSINC: Sub-second Increment Value */
REGDEF_BIT(, 8)
REGDEF_END(ETH_SUB_SECOND_REG)          /* MAC Sub Second Register end */

#define ETH_SYSTEM_TIME_REG_OFS     0xB08
REGDEF_BEGIN(ETH_SYSTEM_TIME_REG)       /* MAC System Time Register */
REGDEF_BIT(TSS, 32)                 /* TSS: Timestamp Second */
REGDEF_END(ETH_SYSTEM_TIME_REG)         /* MAC System Time Register end */

#define ETH_SYSTEM_TIME_NS_REG_OFS  0xB0C
REGDEF_BEGIN(ETH_SYSTEM_TIME_NS_REG)    /* MAC System Time Nanosecond Register */
REGDEF_BIT(TSSS, 31)                /* TSSS: Timestamp Sub Second */
REGDEF_BIT(, 1)
REGDEF_END(ETH_SYSTEM_TIME_NS_REG)      /* MAC System Time Nanosecond Register end */

#define ETH_SYSTEM_TIME_SEC_UPDT_REG_OFS    0xB10
REGDEF_BEGIN(ETH_SYSTEM_TIME_SEC_UPDT_REG)  /* MAC System Time Seconds Update Register */
REGDEF_BIT(TSS, 32)                     /* TSS: Timestamp Second */
REGDEF_END(ETH_SYSTEM_TIME_SEC_UPDT_REG)    /* MAC System Time Seconds Update Register end */

#define ETH_SYSTEM_TIME_NS_UPDT_REG_OFS     0xB14
REGDEF_BEGIN(ETH_SYSTEM_TIME_NS_UPDT_REG)   /* MAC System Time Nanoseconds Update Register */
REGDEF_BIT(TSSS, 31)                    /* TSSS: Timestamp Sub Second */
REGDEF_BIT(ADDSUB, 1)                   /* ADDSUB: Add or Subtract Time */
REGDEF_END(ETH_SYSTEM_TIME_NS_UPDT_REG)     /* MAC System Time Nanoseconds Update Register end */

#define ETH_TIMESTAMP_ADDEND_REG_OFS        0xB18
REGDEF_BEGIN(ETH_TIMESTAMP_ADDEND_REG)      /* MAC Timestamp Addend Register */
REGDEF_BIT(TSAR, 32)                    /* TSAR: Timestamp Addend Register */
REGDEF_END(ETH_TIMESTAMP_ADDEND_REG)        /* MAC Timestamp Addend Register end */

#define ETH_TIMESTAMP_STATUS_REG_OFS        0xB20
REGDEF_BEGIN(ETH_TIMESTAMP_STATUS_REG)      /* MAC Timestamp Status Register */
REGDEF_BIT(TSSOVF, 1)                   /* TSSOVF: Timestamp Seconds Overflow */
REGDEF_BIT(TSTARGT0, 1)                 /* TSTARGT0: Timestamp Target Time Reached */
REGDEF_BIT(AUXTSTRIG, 1)                /* (NOT Configured) AUXTSTRIG: Auxiliary Timestamp Trigger Snapshot */
REGDEF_BIT(TSTRGTERR0, 1)               /* TSTRGTERR0: Timestamp Target Time Error */
REGDEF_BIT(TSTARGT1, 1)                 /* TSTARGT0: Timestamp Target Time Reached for Target Time PPS1 */
REGDEF_BIT(TSTRGTERR1, 1)               /* TSTRGTERR1: Timestamp Target Time Error */
REGDEF_BIT(TSTARGT2, 1)                 /* TSTARGT2: Timestamp Target Time Reached for Target Time PPS2 */
REGDEF_BIT(TSTRGTERR2, 1)               /* TSTRGTERR2: Timestamp Target Time Error */
REGDEF_BIT(TSTARGT3, 1)                 /* TSTARGT3: Timestamp Target Time Reached for Target Time PPS3 */
REGDEF_BIT(TSTRGTERR3, 1)               /* TSTRGTERR3: Timestamp Target Time Error */
REGDEF_BIT(, 5)
REGDEF_BIT(TXTSSIS, 1)                  /* TXTSSIS: Tx Timestamp Status Interrupt Status */
REGDEF_BIT(ATSSTN, 4)                   /* (NOT Configured) ATSSTN: Auxiliary Timestamp Snapshot Trigger Identifier */
REGDEF_BIT(, 4)
REGDEF_BIT(ATSSTM, 1)                   /* (NOT Configured) ATSSTM: Auxiliary Timestamp Snapshot Trigger Missed */
REGDEF_BIT(ATSNS, 1)                    /* (NOT Configured) ATSNS: Number of Auxiliary Timestamp Snapshots */
REGDEF_BIT(, 2)
REGDEF_END(ETH_TIMESTAMP_STATUS_REG)        /* MAC Timestamp Status Register end */

#define ETH_TIMESTAMP_STATUS_NS_REG_OFS     0xB30
REGDEF_BEGIN(ETH_TIMESTAMP_STATUS_NS_REG)   /* MAC Timestamp Status Nanoseconds Register */
REGDEF_BIT(TXTSSLO, 31)                 /* TXTSSLO: Transmit Timestamp Status Low */
REGDEF_BIT(TXTSSMIS, 1)                 /* TXTSSMIS: Transmit Timestamp Status Missed */
REGDEF_END(ETH_TIMESTAMP_STATUS_NS_REG)     /* MAC Timestamp Status Nanoseconds Register end */

#define ETH_TIMESTAMP_STATUS_SECONDS_REG_OFS    0xB34
REGDEF_BEGIN(ETH_TIMESTAMP_STATUS_SECONDS_REG)  /* MAC Timestamp Status Seconds Register */
REGDEF_BIT(TXTSSHI, 32)                     /* TXTSSHI: Transmit Timestamp Status High */
REGDEF_END(ETH_TIMESTAMP_STATUS_SECONDS_REG)    /* MAC Timestamp Status Seconds Register end */

#define ETH_TIMESTAMP_INGRESS_CORR_NS_REG_OFS   0xB58
REGDEF_BEGIN(ETH_TIMESTAMP_INGRESS_CORR_NS_REG) /* MAC Timestamp Ingress Corr Nanoseconds Register */
REGDEF_BIT(TSIC, 32)                        /* TSIC: Timestamp Ingress Correction */
REGDEF_END(ETH_TIMESTAMP_INGRESS_CORR_NS_REG)   /* MAC Timestamp Ingress Corr Nanoseconds Register end */

#define ETH_TIMESTAMP_EGRESS_CORR_NS_REG_OFS    0xB5C
REGDEF_BEGIN(ETH_TIMESTAMP_EGRESS_CORR_NS_REG)  /* MAC Timestamp Egress Corr Nanoseconds Register */
REGDEF_BIT(TSEC, 32)                        /* TSEC: Timestamp Egress Correction */
REGDEF_END(ETH_TIMESTAMP_EGRESS_CORR_NS_REG)    /* MAC Timestamp Egress Corr Nanoseconds Register end */

#define ETH_MTL_OPERATION_MODE_REG_OFS      0xC00
REGDEF_BEGIN(ETH_MTL_OPERATION_MODE_REG)    /* MTL Operation Mode Register */
REGDEF_BIT(, 1)
REGDEF_BIT(DTXSTS, 1)                   /* DTXSTS: Drop Transmit Status */
REGDEF_BIT(, 6)
REGDEF_BIT(CNTPRST, 1)                  /* CNTPRST: Counters Preset */
REGDEF_BIT(CNTCLR, 1)                   /* CNTCLR: Counters Reset */
REGDEF_BIT(, 22)
REGDEF_END(ETH_MTL_OPERATION_MODE_REG)      /* MTL Operation Mode Register end */

#define ETH_MTL_INT_STS_REG_OFS             0xC20
REGDEF_BEGIN(ETH_MTL_INT_STS_REG)           /* MTL Interrupt Status Register */
REGDEF_BIT(Q0IS, 1)                     /* Q0IS: Queue 0 Interrupt Status */
REGDEF_BIT(, 31)
REGDEF_END(ETH_MTL_INT_STS_REG)             /* MTL Interrupt Status Register end */

#define ETH_MTL_TxQ0_OP_MODE_REG_OFS        0xD00
REGDEF_BEGIN(ETH_MTL_TxQ0_OP_MODE_REG)      /* MTL TxQ0 Operation Mode Register */
REGDEF_BIT(FTQ, 1)                      /* Flush Transmit Queue */
REGDEF_BIT(TSF, 1)                      /* Transmit Store and Forward */
REGDEF_BIT(TXQEN, 2)                    /* Transmit Queue Enable */
REGDEF_BIT(TTC, 3)                      /* Transmit Threshold Control */
REGDEF_BIT(, 9)
REGDEF_BIT(TQS, 4)                      /* Transmit Queue Size */
REGDEF_BIT(, 12)
REGDEF_END(ETH_MTL_TxQ0_OP_MODE_REG)        /* MTL TxQ0 Operation Mode Register end */

#define ETH_MTL_TxQ0_UNDERFLOW_REG_OFS      0xD04
REGDEF_BEGIN(ETH_MTL_TxQ0_UNDERFLOW_REG)    /* MTL TxQ0 Underflow Register */
REGDEF_BIT(UFFRMCNT, 11)                /* Underflow Packet Counter */
REGDEF_BIT(UFCNTOVF, 1)                 /* Overflow Bit for Underflow Packet Counter */
REGDEF_BIT(, 20)
REGDEF_END(ETH_MTL_TxQ0_UNDERFLOW_REG)      /* MTL TxQ0 Underflow Register end */

#define ETH_MTL_TxQ0_DBG_REG_OFS            0xD08
REGDEF_BEGIN(ETH_MTL_TxQ0_DBG_REG)          /* MTL TxQ0 Debug Register */
REGDEF_BIT(TXQPAUSED, 1)                /* Transmit Queue in Pause */
REGDEF_BIT(TRCSTS, 2)                   /* MTL Tx Queue Read Controller Status */
REGDEF_BIT(TWCSTS, 1)                   /* MTL Tx Queue Write Controller Status */
REGDEF_BIT(TXQSTS, 1)                   /* MTL Tx Queue Not Empty Status */
REGDEF_BIT(TXSTSFSTS, 1)                /* MTL Tx Status FIFO Full Status */
REGDEF_BIT(, 10)
REGDEF_BIT(PTXQ, 3)                     /* Number of Packets in the Transmit Queue */
REGDEF_BIT(, 1)
REGDEF_BIT(STXSTSF, 3)                  /* Number of Status Words in Tx Status FIFO of Queue */
REGDEF_BIT(, 9)
REGDEF_END(ETH_MTL_TxQ0_DBG_REG)            /* MTL TxQ0 Debug Register end */

#define ETH_MTL_Q0_INT_CTRL_REG_OFS         0xD2C
REGDEF_BEGIN(ETH_MTL_Q0_INT_CTRL_REG)       /* MTL Q0 Interrupt Control Status Register */
REGDEF_BIT(TXUNFIS, 1)                  /* Transmit Queue Underflow Interrupt Status */
REGDEF_BIT(ABPSIS, 1)                   /* (NOT Configured) Average Bits Per Slot Interrupt Status */
REGDEF_BIT(, 6)
REGDEF_BIT(TXUIE, 1)                    /* Transmit Queue Underflow Interrupt Enable */
REGDEF_BIT(ABPSIE, 1)                   /* (NOT Configured) Average Bits Per Slot Interrupt Enable */
REGDEF_BIT(, 6)
REGDEF_BIT(RXOVFIS, 1)                  /* Receive Queue Overflow Interrupt Status */
REGDEF_BIT(, 7)
REGDEF_BIT(RXOIE, 1)                    /* Receive Queue Overflow Interrupt Enable */
REGDEF_BIT(, 7)
REGDEF_END(ETH_MTL_Q0_INT_CTRL_REG)         /* MTL Q0 Interrupt Control Status end */

#define ETH_MTL_RxQ0_OP_MODE_REG_OFS        0xD30
REGDEF_BEGIN(ETH_MTL_RxQ0_OP_MODE_REG)      /* MTL RxQ0 Operation Mode Register */
REGDEF_BIT(RTC, 2)                      /* Receive Queue Threshold Control */
REGDEF_BIT(, 1)
REGDEF_BIT(FUP, 1)                      /* Forward Undersized Good Packets */
REGDEF_BIT(FEP, 1)                      /* Forward Error Packets */
REGDEF_BIT(RSF, 1)                      /* Receive Queue Store and Forward */
REGDEF_BIT(DIS_TCP_EF, 1)               /* Disable Dropping of TCP/IP Checksum Error Packets */
REGDEF_BIT(EHFC, 1)                     /* Enable Hardware Flow Control */
REGDEF_BIT(RFA, 3)                      /* Threshold for Activating Flow Control */
REGDEF_BIT(, 3)
REGDEF_BIT(RFD, 3)                      /* Threshold for Deactivating Flow Control */
REGDEF_BIT(, 3)
REGDEF_BIT(RQS, 4)                      /* Receive Queue Size */
REGDEF_BIT(, 8)
REGDEF_END(ETH_MTL_RxQ0_OP_MODE_REG)        /* MTL RxQ0 Operation Mode end */

#define ETH_DMA_MODE_REG_OFS        0x1000
REGDEF_BEGIN(ETH_DMA_MODE_REG)          /* DMA Mode Register */
REGDEF_BIT(SWR, 1)                  /* Software reset */
REGDEF_BIT(DA, 1)                   /* DMA Arbitration scheme */
REGDEF_BIT(TAA, 3)                  /* (NOT Configured) Transmit Arbitration Algorithm */
REGDEF_BIT(, 6)
REGDEF_BIT(TXPR, 1)                 /* (NOT Configured) Transmit Priority */
REGDEF_BIT(PR, 3)                   /* (NOT Configured) Priority Ratio */
REGDEF_BIT(, 1)
REGDEF_BIT(INTM, 2)                 /* Interrupt Mode */
REGDEF_BIT(, 14)
REGDEF_END(ETH_DMA_MODE_REG)            /* DMA Mode Register end */

#define ETH_DMA_SYS_BUS_REG_OFS     0x1004
REGDEF_BEGIN(ETH_DMA_SYS_BUS_REG)       /* DMA SysBus Register */
REGDEF_BIT(FB, 1)                   /* Fixed Burst */
REGDEF_BIT(BLEN4, 1)                /* AXI Burst Length 4 */
REGDEF_BIT(BLEN8, 1)                /* AXI Burst Length 8 */
REGDEF_BIT(BLEN16, 1)               /* AXI Burst Length 16 */
REGDEF_BIT(BLEN32, 1)               /* AXI Burst Length 32 */
REGDEF_BIT(BLEN64, 1)               /* AXI Burst Length 64 */
REGDEF_BIT(BLEN128, 1)              /* (NOT Configured) AXI Burst Length 128 */
REGDEF_BIT(BLEN256, 1)              /* (NOT Configured) AXI Burst Length 256 */
REGDEF_BIT(, 2)
REGDEF_BIT(AALE, 1)                 /* Automatic AXI LPI Enable */
REGDEF_BIT(EAME, 1)                 /* (NOT Configured) Enhanced Address Mode Enable */
REGDEF_BIT(AAL, 1)                  /* Address-Aligned Beats */
REGDEF_BIT(ONEKBBE, 1)              /* 1KB Boundary Enable */
REGDEF_BIT(MB, 1)                   /* (NOT Configured) Mixed Burst */
REGDEF_BIT(RB, 1)                   /* (NOT Configured) Bebuild INCRx Burst */
REGDEF_BIT(RD_OSR_LMT, 2)           /* Read Outstanding Request Limit */
REGDEF_BIT(, 6)
REGDEF_BIT(WR_OSR_LMT, 2)           /* Write Outstanding Request Limit */
REGDEF_BIT(, 4)
REGDEF_BIT(LPI_XIT_PKT, 1)          /* (NOT Configured) Unlock on Magic Packet or Remote Wake-Up Packet */
REGDEF_BIT(EN_LPI, 1)               /* Enable Low Power Interface (LPI) */
REGDEF_END(ETH_DMA_SYS_BUS_REG)         /* DMA SysBus Register end */

#define ETH_DMA_INT_STS_REG_OFS     0x1008
REGDEF_BEGIN(ETH_DMA_INT_STS_REG)       /* DMA Interrupt Status Register */
REGDEF_BIT(DC0IS, 1)                /* DMA Channel 0 Interrupt Status */
REGDEF_BIT(, 15)
REGDEF_BIT(MTLIS, 1)                /* MTL Interrupt Status */
REGDEF_BIT(MACIS, 1)                /* MAC Interrupt Status */
REGDEF_BIT(, 14)
REGDEF_END(ETH_DMA_INT_STS_REG)         /* DMA Interrupt Status Register end */

#define ETH_DMA_DBG_STS0_REG_OFS    0x100C
REGDEF_BEGIN(ETH_DMA_DBG_STS0_REG)      /* DMA Debug Status0 Register */
REGDEF_BIT(AXWHSTS, 1)              /* AXI Master Write Channel Status */
REGDEF_BIT(AXRHSTS, 1)              /* AXI Master Read Channel Status */
REGDEF_BIT(, 6)
REGDEF_BIT(RPS0, 4)                 /* DMA Channel 0 Receive process State */
REGDEF_BIT(TPS0, 4)                 /* DMA Channel 0 Transmit process State */
REGDEF_BIT(, 16)
REGDEF_END(ETH_DMA_DBG_STS0_REG)        /* DMA Debug Status0 Register end */

#define ETH_AXI_LPI_ENTRY_INTERVAL_REG_OFS      0x1040
REGDEF_BEGIN(ETH_AXI_LPI_ENTRY_INTERVAL_REG)    /* AXI LPI Entry Interval Register */
REGDEF_BIT(LPIEI, 4)                        /* LPI Entry Interval */
REGDEF_BIT(, 28)
REGDEF_END(ETH_AXI_LPI_ENTRY_INTERVAL_REG)      /* AXI LPI Entry Interval Register end */

#define ETH_DMA_CH0_CTRL_REG_OFS    0x1100
REGDEF_BEGIN(ETH_DMA_CH0_CTRL_REG)      /* DMA Debug Status0 Register */
REGDEF_BIT(MSS, 14)                 /* Maximum Segment Size (unit: byte) */
REGDEF_BIT(, 2)
REGDEF_BIT(PBLx8, 1)                /* 8xPBL mode */
REGDEF_BIT(, 1)
REGDEF_BIT(DSL, 3)                  /* Descriptor Skip Length (unit: 4 word) */
REGDEF_BIT(, 3)
REGDEF_BIT(SPH, 1)                  /* Split Headers */
REGDEF_BIT(, 7)
REGDEF_END(ETH_DMA_CH0_CTRL_REG)        /* DMA Debug Status0 Register end */

#define ETH_DMA_CH0_TX_CTRL_REG_OFS 0x1104
REGDEF_BEGIN(ETH_DMA_CH0_TX_CTRL_REG)   /* DMA CH0 Tx Control Register */
REGDEF_BIT(ST, 1)                   /* Start/stop Transmission command */
REGDEF_BIT(TCW, 3)                  /* (NOT Configured) Transmit Channel Weight */
REGDEF_BIT(OSF, 1)                  /* Operate on Second Packet */
REGDEF_BIT(, 7)
REGDEF_BIT(TSE, 1)                  /* TCP Segmentation Enabled */
REGDEF_BIT(UFO, 2)                   /* UFO Mode Sel */
REGDEF_BIT(IPBL, 1)                 /* Ignore PBL Requirement */
REGDEF_BIT(TxPBL, 6)                /* Transmit Programmable Burst Length */
REGDEF_BIT(, 2)
REGDEF_BIT(TQOS, 4)                 /* (NOT Configured) Transmit QOS */
REGDEF_BIT(, 4)
REGDEF_END(ETH_DMA_CH0_TX_CTRL_REG)     /* DMA CH0 Tx Control Register end */

#define ETH_DMA_CH0_RX_CTRL_REG_OFS 0x1108
REGDEF_BEGIN(ETH_DMA_CH0_RX_CTRL_REG)   /* DMA CH0 Rx Control Register */
REGDEF_BIT(SR, 1)                   /* Stop/run Receive */
REGDEF_BIT(, 3)
REGDEF_BIT(RBSZ, 11)                 /* Receive Buffer Size (unit: 4word) */
REGDEF_BIT(, 1)
REGDEF_BIT(RxPBL, 6)                /* Receive Programmable Burst Length */
REGDEF_BIT(, 2)
REGDEF_BIT(RQOS, 4)                 /* (NOT Configured) Rx QOS */
REGDEF_BIT(, 3)
REGDEF_BIT(RPF, 1)                  /* DMA Rx Channel0 Packet Flush */
REGDEF_END(ETH_DMA_CH0_RX_CTRL_REG)     /* DMA CH0 Rx Control Register end */

#define ETH_DMA_CH0_TxDesc_LIST_ADDR_REG_OFS    0x1114
REGDEF_BEGIN(ETH_DMA_CH0_TxDesc_LIST_ADDR_REG)  /* DMA CH0 TxDesc List Address Register */
REGDEF_BIT(, 4)
REGDEF_BIT(TDESLA, 28)
REGDEF_END(ETH_DMA_CH0_TxDesc_LIST_ADDR_REG)    /* DMA CH0 TxDesc List Address Register end */

#define ETH_DMA_CH0_RxDesc_LIST_ADDR_REG_OFS    0x111C
REGDEF_BEGIN(ETH_DMA_CH0_RxDesc_LIST_ADDR_REG)  /* DMA CH0 RxDesc List Address Register */
REGDEF_BIT(, 4)
REGDEF_BIT(RDESLA, 28)
REGDEF_END(ETH_DMA_CH0_RxDesc_LIST_ADDR_REG)    /* DMA CH0 RxDesc List Address Register end */

#define ETH_DMA_CH0_TxDesc_TAIL_PNTR_REG_OFS    0x1120
REGDEF_BEGIN(ETH_DMA_CH0_TxDesc_TAIL_PNTR_REG)  /* DMA CH0 TxDesc List Address Register */
REGDEF_BIT(, 4)
REGDEF_BIT(TDTP, 28)
REGDEF_END(ETH_DMA_CH0_TxDesc_TAIL_PNTR_REG)    /* DMA CH0 TxDesc List Address Register end */

#define ETH_DMA_CH0_RxDesc_TAIL_PNTR_REG_OFS    0x1128
REGDEF_BEGIN(ETH_DMA_CH0_RxDesc_TAIL_PNTR_REG)  /* DMA CH0 RxDesc List Address Register */
REGDEF_BIT(, 4)
REGDEF_BIT(RDTP, 28)
REGDEF_END(ETH_DMA_CH0_RxDesc_TAIL_PNTR_REG)    /* DMA CH0 RxDesc List Address Register end */

#define ETH_DMA_CH0_TxDesc_RING_LEN_REG_OFS     0x112C
REGDEF_BEGIN(ETH_DMA_CH0_TxDesc_RING_LEN_REG)   /* DMA CH0 TxDesc Ring Length Register */
REGDEF_BIT(TDRL, 10)
REGDEF_BIT(, 22)
REGDEF_END(ETH_DMA_CH0_TxDesc_RING_LEN_REG)     /* DMA CH0 TxDesc Ring Length Register end */

#define ETH_DMA_CH0_RxDesc_RING_LEN_REG_OFS     0x1130
REGDEF_BEGIN(ETH_DMA_CH0_RxDesc_RING_LEN_REG)   /* DMA CH0 RxDesc Ring Length Register */
REGDEF_BIT(RDRL, 10)
REGDEF_BIT(, 22)
REGDEF_END(ETH_DMA_CH0_RxDesc_RING_LEN_REG)     /* DMA CH0 RxDesc Ring Length Register end */

#define ETH_DMA_CH0_INTEN_REG_OFS   0x1134
REGDEF_BEGIN(ETH_DMA_CH0_INTEN_REG)     /* DMA CH0 Interrupt Enable Register */
REGDEF_BIT(TIE, 1)                  /* Transmit Interrupt Enable */
REGDEF_BIT(TXSE, 1)                 /* Transmit Stopped Enable */
REGDEF_BIT(TBUE, 1)                 /* Transmit buffer Unavailable Enable */
REGDEF_BIT(, 3)
REGDEF_BIT(RIE, 1)                  /* Receive Interrupt Enable */
REGDEF_BIT(RBUE, 1)                 /* Receive buffer Unavailable Enable */
REGDEF_BIT(RSE, 1)                  /* Receive Stopped Enable */
REGDEF_BIT(RWTE, 1)                 /* Receive Watchdog timeout Enable */
REGDEF_BIT(ETIE, 1)                 /* Early Transmit interrupt Enable */
REGDEF_BIT(ERIE, 1)                 /* Early Receive interrupt Enable */
REGDEF_BIT(FBEE, 1)                 /* Fatal Bus error Enable */
REGDEF_BIT(CDEE, 1)                 /* Context Descriptor error Enable */
REGDEF_BIT(AIE, 1)                  /* Abnormal Interrupt summary Enable */
REGDEF_BIT(NIE, 1)                  /* Normal Interrupt summary Enable */
REGDEF_BIT(, 16)
REGDEF_END(ETH_DMA_CH0_INTEN_REG)       /* DMA CH0Interrupt Enable Register end */

#define ETH_DMA_CH0_RX_INT_WATCHDOG_TIMER_REG_OFS   0x1138
REGDEF_BEGIN(ETH_DMA_CH0_RX_INT_WATCHDOG_TIMER_REG) /* DMA CH0 Rx Interrupt Watchdog Timer Register */
REGDEF_BIT(RWT, 8)                              /* RI watchdog timer count */
REGDEF_BIT(, 8)
REGDEF_BIT(RWTU, 2)                             /* Receive Interrupt Watchdog Timer Count Units */
REGDEF_BIT(, 14)
REGDEF_END(ETH_DMA_CH0_RX_INT_WATCHDOG_TIMER_REG)   /* DMA CH0 Rx Interrupt Watchdog Timer Register end */

#define ETH_DMA_CH0_CURR_APP_TXDESC_REG_OFS         0x1144
REGDEF_BEGIN(ETH_DMA_CH0_CURR_APP_TXDESC_REG)       /* DMA CH0 Current App TxDesc Register */
REGDEF_BIT(CURTDESAPTR, 32)
REGDEF_END(ETH_DMA_CH0_CURR_APP_TXDESC_REG)         /* DMA CH0 Current App TxDesc Register end */

#define ETH_DMA_CH0_CURR_APP_RXDESC_REG_OFS         0x114C
REGDEF_BEGIN(ETH_DMA_CH0_CURR_APP_RXDESC_REG)       /* DMA CH0 Current App RxDesc Register */
REGDEF_BIT(CURRDESAPTR, 32)
REGDEF_END(ETH_DMA_CH0_CURR_APP_RXDESC_REG)         /* DMA CH0 Current App RxDesc Register end */

#define ETH_DMA_CH0_CURR_APP_TXBUF_REG_OFS          0x1154
REGDEF_BEGIN(ETH_DMA_CH0_CURR_APP_TXBUF_REG)        /* DMA CH0 Current App TxBuffer Register */
REGDEF_BIT(CURTBUFAPTR, 32)
REGDEF_END(ETH_DMA_CH0_CURR_APP_TXBUF_REG)          /* DMA CH0 Current App TxBuffer Register end */

#define ETH_DMA_CH0_CURR_APP_RXBUF_REG_OFS          0x115C
REGDEF_BEGIN(ETH_DMA_CH0_CURR_APP_RXBUF_REG)        /* DMA CH0 Current App RxBuffer Register */
REGDEF_BIT(CURRBUFAPTR, 32)
REGDEF_END(ETH_DMA_CH0_CURR_APP_RXBUF_REG)          /* DMA CH0 Current App RxBuffer Register end */

#define ETH_DMA_CH0_STS_REG_OFS     0x1160
REGDEF_BEGIN(ETH_DMA_CH0_STS_REG)       /* DMA CH0 Status Register */
REGDEF_BIT(TI, 1)                   /* Transmit Interrupt */
REGDEF_BIT(TPS, 1)                  /* Transmit Process Stopped */
REGDEF_BIT(TBU, 1)                  /* Transmit buffer Unavailable */
REGDEF_BIT(, 3)
REGDEF_BIT(RI, 1)                   /* Receive Interrupt */
REGDEF_BIT(RBU, 1)                  /* Receive buffer Unavailable */
REGDEF_BIT(RPS, 1)                  /* Receive Process Stopped */
REGDEF_BIT(RWT, 1)                  /* Receive Watchdog timeout */
REGDEF_BIT(ETI, 1)                  /* Early Transmit interrupt */
REGDEF_BIT(ERI, 1)                  /* Early Receive interrupt */
REGDEF_BIT(FBE, 1)                  /* Fatal Bus error */
REGDEF_BIT(CDE, 1)                  /* Context Descriptor error */
REGDEF_BIT(AIS, 1)                  /* Abnormal Interrupt summary */
REGDEF_BIT(NIS, 1)                  /* Normal Interrupt summary */
REGDEF_BIT(TEB, 3)                  /* Tx DMA Error Bits */
REGDEF_BIT(REB, 3)                  /* Rx DMA Error Bits */
REGDEF_BIT(, 10)
REGDEF_END(ETH_DMA_CH0_STS_REG)         /* DMA CH0 Status Register end */

#define ETH_DMA_CH0_MISSED_FRAME_REG_OFS    0x1020
REGDEF_BEGIN(ETH_DMA_CH0_MISSED_FRAME_REG)  /* DMA CH0 Miss Frame Cnt Register */
REGDEF_BIT(MFC, 11)                     /* Dropped Packet Counters */
REGDEF_BIT(, 4)
REGDEF_BIT(MFCO, 1)                     /* Overflow status of the MFC counter */
REGDEF_BIT(, 16)
REGDEF_END(ETH_DMA_CH0_MISSED_FRAME_REG)    /* DMA CH0 Miss Frame Cnt Register end */

////////////////////////////////////////////////////


// Following register are added by iVot-IM
#define ETH_IM_DEBUG_CTRL_REG_OFS       0x3000
REGDEF_BEGIN(ETH_IM_DEBUG_CTRL_REG)     /* Debug Control Register */
REGDEF_BIT(, 16)
REGDEF_BIT(LOCAL_ADDR, 5)           /* RevMII local Addr  */
REGDEF_BIT(REMOTE_ADDR, 5)          /* RevMII remote Addr  */
REGDEF_BIT(, 6)
REGDEF_END(ETH_IM_DEBUG_CTRL_REG)       /* Debug Control Register */

#define ETH_IM_CTRL_EN_REG_OFS           0x3004
REGDEF_BEGIN(ETH_IM_CTRL_EN_REG)        /* Control Register */
REGDEF_BIT(, 4)
REGDEF_BIT(ETH_PHY_SEL, 2)          /* External PHY SEL  */
REGDEF_BIT(, 3)
REGDEF_BIT(SBD_FC_EN, 1)            /* Sideband flow control to for triggering flow control  */
REGDEF_BIT(, 21)
REGDEF_BIT(RMII_OUTPUT_PHASE, 1)        /* Sideband flow control to for triggering flow control  */
REGDEF_END(ETH_IM_CTRL_EN_REG)          /* Control Register */

#define ETH_IM_INT_STS_REG_OFS          0x3008
REGDEF_BEGIN(ETH_IM_INT_STS_REG)        /* Interrupt Status Register */
REGDEF_BIT(SBD_STS, 1)              /* sbd_intr_o/sbd_perch_tx_intr_o/sbd_perch_rx_intr_o */
REGDEF_BIT(SBD_TX_STS, 1)           /* sbd_perch_tx_intr_o */
REGDEF_BIT(SBD_RX_STS, 1)           /* sbd_perch_rx_intr_o */
REGDEF_BIT(PMT_STS, 1)              /* pmt_intr_o */
REGDEF_BIT(LPI_STS, 1)              /* lpi_intr_o */
REGDEF_BIT(GMIIPHY_STS, 1)          /* gmii_phy_intr_o */
REGDEF_BIT(, 2)
REGDEF_BIT(AHBC_IDLE, 1)            /* AHBC Bridge idle/busy state */
REGDEF_BIT(, 23)
REGDEF_END(ETH_IM_INT_STS_REG)          /* Interrupt Status Register */

#define ETH_IM_PHY_LED_REG_OFS          0x300C
REGDEF_BEGIN(ETH_IM_PHY_LED_REG)        /* PHY LED signal Register */
REGDEF_BIT(LED_SEL, 4)              /* LED signal source */
REGDEF_BIT(LED2_SEL, 4)             /* LED2 signal source */
REGDEF_BIT(, 24)
REGDEF_END(ETH_IM_PHY_LED_REG)          /* PHY LED signal Register */

#define ETH_IM_IO_CTRL_REG_OFS           0x3014
REGDEF_BEGIN(ETH_IM_IO_CTRL_REG)        /* IO Control Register */
REGDEF_BIT(EXT_RMII_REF_EN, 1)
REGDEF_BIT(RMII_REF_RX_INV, 1)
REGDEF_BIT(INT_RMII_REF_LB, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(RMII_REF_CLK_O_EN, 1)
REGDEF_BIT(RMII_REF_CLK_I_EN, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(INT_RMII_REF_LB_DLY, 5)
REGDEF_BIT(, 3)
REGDEF_BIT(RGMII_TXCLK_DELAY, 4)
REGDEF_BIT(RX_DLY, 4)
REGDEF_BIT(TX_DLY, 5)
REGDEF_BIT(TXC_SRC, 1)
REGDEF_BIT(TXD_SRC, 2)
REGDEF_END(ETH_IM_IO_CTRL_REG)          /* IO Control Register */

#define ETH_IM_PHY_TX_ROUT_REG_OFS      0x3B74
REGDEF_BEGIN(ETH_IM_PHY_TX_ROUT_REG)
REGDEF_BIT(, 3)
REGDEF_BIT(TX_ROUT, 3)
REGDEF_BIT(, 26)
REGDEF_END(ETH_IM_PHY_TX_ROUT_REG)

#define ETH_IM_PHY_TX_DAC_REG_OFS       0x3B78
REGDEF_BEGIN(ETH_IM_PHY_TX_DAC_REG)
REGDEF_BIT(TX_DAC, 5)
REGDEF_BIT(, 27)
REGDEF_END(ETH_IM_PHY_TX_DAC_REG)

#define ETH_IM_PHY_TX_SEL_REG_OFS       0x3B68
REGDEF_BEGIN(ETH_IM_PHY_TX_SEL_REG)
REGDEF_BIT(, 2)
REGDEF_BIT(SEL_TX, 3)
REGDEF_BIT(SEL_RX, 3)
REGDEF_BIT(, 24)
REGDEF_END(ETH_IM_PHY_TX_SEL_REG)

#endif

