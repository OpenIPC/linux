/*!
*****************************************************************************
** \file        arch/arm/mach-gk7101/include/mach/eth.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#ifndef __MACH_ETH_H__
#define __MACH_ETH_H__

#include <mach/gpio.h>


/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#define ETH_INSTANCES   1

#define SUPPORT_GMII    0   // 10/100 bits phy

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define ETH_REG(x)                      (GK_VA_ETH_GMAC + (x))

#define ETH_MAC_CFG_REG                 ETH_REG(0x0000)

#define ETH_MAC_GMII_ADDR_REG           ETH_REG(0x0004)
#define ETH_MAC_GMII_DATA_REG           ETH_REG(0x0008)

#define ETH_MAC_FRAME_FILTER_REG        ETH_REG(0x000c)
#define ETH_MAC_HASH_HI_REG             ETH_REG(0x0010)
#define ETH_MAC_HASH_LO_REG             ETH_REG(0x0014)
#define ETH_MAC_FLOW_CTR_REG            ETH_REG(0x0018)
#define ETH_MAC_VLAN_TAG_REG            ETH_REG(0x001c)
#define ETH_MAC_VERSION_REG             ETH_REG(0x0058)

#define ETH_MAC_MAC0_HI_REG             ETH_REG(0x0040)
#define ETH_MAC_MAC0_LO_REG             ETH_REG(0x0044)
#define ETH_MAC_MAC1_HI_REG             ETH_REG(0x0048)
#define ETH_MAC_MAC1_LO_REG             ETH_REG(0x004c)
#define ETH_MAC_MAC2_HI_REG             ETH_REG(0x0050)
#define ETH_MAC_MAC2_LO_REG             ETH_REG(0x0054)


#define ETH_DMA_TX_POLL_DMD_REG         ETH_REG(0x1000)
#define ETH_DMA_RX_POLL_DMD_REG         ETH_REG(0x1004)
#define ETH_DMA_BUS_MODE_REG            ETH_REG(0x1008)
#define ETH_DMA_RX_DESC_LIST_REG        ETH_REG(0x100c)
#define ETH_DMA_TX_DESC_LIST_REG        ETH_REG(0x1010)
#define ETH_DMA_STATUS_REG              ETH_REG(0x1014)
#define ETH_DMA_MISS_FRAME_BOCNT_REG    ETH_REG(0x1018)

#define ETH_DMA_INTEN_REG               ETH_REG(0x101c)
#define ETH_DMA_OPMODE_REG              ETH_REG(0x1020)

#define ETH_DMA_HOST_TX_DESC_REG        ETH_REG(0x1048)
#define ETH_DMA_HOST_RX_DESC_REG        ETH_REG(0x104c)
#define ETH_DMA_HOST_TX_BUF_REG         ETH_REG(0x1040)
#define ETH_DMA_HOST_RX_BUF_REG         ETH_REG(0x1044)


//-------------------------------------------
#define ETH_MAC_CFG_OFFSET              (0x0000)
#define ETH_MAC_FRAME_FILTER_OFFSET     (0x000c)
#define ETH_MAC_HASH_HI_OFFSET          (0x0010)
#define ETH_MAC_HASH_LO_OFFSET          (0x0014)
#define ETH_MAC_GMII_ADDR_OFFSET        (0x0004)
#define ETH_MAC_GMII_DATA_OFFSET        (0x0008)
#define ETH_MAC_FLOW_CTR_OFFSET         (0x0018)
#define ETH_MAC_VLAN_TAG_OFFSET         (0x001c)
#define ETH_MAC_VERSION_OFFSET          (0x0058)
#define ETH_MAC_MAC0_HI_OFFSET          (0x0040)
#define ETH_MAC_MAC0_LO_OFFSET          (0x0044)
#define ETH_MAC_MAC1_HI_OFFSET          (0x0048)
#define ETH_MAC_MAC1_LO_OFFSET          (0x004c)
#define ETH_MAC_MAC2_HI_OFFSET          (0x0050)
#define ETH_MAC_MAC2_LO_OFFSET          (0x0054)

#define ETH_DMA_BUS_MODE_OFFSET         (0x1008)
#define ETH_DMA_TX_POLL_DMD_OFFSET      (0x1000)
#define ETH_DMA_RX_POLL_DMD_OFFSET      (0x1004)
#define ETH_DMA_RX_DESC_LIST_OFFSET     (0x100c)
#define ETH_DMA_TX_DESC_LIST_OFFSET     (0x1010)
#define ETH_DMA_STATUS_OFFSET           (0x1014)
#define ETH_DMA_OPMODE_OFFSET           (0x1020)
#define ETH_DMA_INTEN_OFFSET            (0x101c)
#define ETH_DMA_MISS_FRAME_BOCNT_OFFSET (0x1018)
#define ETH_DMA_HOST_TX_DESC_OFFSET     (0x1048)
#define ETH_DMA_HOST_RX_DESC_OFFSET     (0x104c)
#define ETH_DMA_HOST_TX_BUF_OFFSET      (0x1040)
#define ETH_DMA_HOST_RX_BUF_OFFSET      (0x1044)

/* ETH_MAC_CFG_REG */
#define ETH_MAC_CFG_WD                  0x00800000
#define ETH_MAC_CFG_JD                  0x00400000
#define ETH_MAC_CFG_BE                  0x00200000
#define ETH_MAC_CFG_JE                  0x00100000
#define ETH_MAC_CFG_IFG_96              0x000e0000
#define ETH_MAC_CFG_IFG_88              0x000c0000
#define ETH_MAC_CFG_IFG_80              0x000a0000
#define ETH_MAC_CFG_IFG_72              0x00080000
#define ETH_MAC_CFG_IFG_64              0x00060000
#define ETH_MAC_CFG_IFG_56              0x00040000
#define ETH_MAC_CFG_IFG_48              0x00020000
#define ETH_MAC_CFG_IFG_40              0x00000000
#define ETH_MAC_CFG_DCRS                0x00010000
#define ETH_MAC_CFG_PS                  0x00008000
#define ETH_MAC_CFG_FES                 0x00004000
#define ETH_MAC_CFG_DO                  0x00002000
#define ETH_MAC_CFG_LM                  0x00001000
#define ETH_MAC_CFG_DM                  0x00000800
#define ETH_MAC_CFG_IPC                 0x00000400
#define ETH_MAC_CFG_DR                  0x00000200
#define ETH_MAC_CFG_LUD                 0x00000100
#define ETH_MAC_CFG_ACS                 0x00000080
#define ETH_MAC_CFG_BL_1                0x00000060
#define ETH_MAC_CFG_BL_4                0x00000040
#define ETH_MAC_CFG_BL_8                0x00000020
#define ETH_MAC_CFG_BL_10               0x00000000
#define ETH_MAC_CFG_DC                  0x00000010
#define ETH_MAC_CFG_TE                  0x00000008  /* Transmitter Enable */
#define ETH_MAC_CFG_RE                  0x00000004  /* Receiver Enable  */

/* ETH_MAC_FRAME_FILTER_REG */
#define ETH_MAC_FRAME_FILTER_RA         0x80000000
#define ETH_MAC_FRAME_FILTER_SAF        0x00000200
#define ETH_MAC_FRAME_FILTER_SAIF       0x00000100
#define ETH_MAC_FRAME_FILTER_PCF_PASS   0x000000c0
#define ETH_MAC_FRAME_FILTER_PCF_FAIL   0x00000040
#define ETH_MAC_FRAME_FILTER_PCF_ALL    0x00000000
#define ETH_MAC_FRAME_FILTER_DBF        0x00000020
#define ETH_MAC_FRAME_FILTER_PM         0x00000010
#define ETH_MAC_FRAME_FILTER_DAIF       0x00000008
#define ETH_MAC_FRAME_FILTER_HMC        0x00000004
#define ETH_MAC_FRAME_FILTER_HUC        0x00000002
#define ETH_MAC_FRAME_FILTER_PR         0x00000001

/* ETH_MAC_GMII_ADDR_REG */
#define ETH_MAC_GMII_ADDR_PA(x)         (((x) & 0x1f) << 11)
#define ETH_MAC_GMII_ADDR_GR(x)         (((x) & 0x1f) << 6)
#define ETH_MAC_GMII_ADDR_CR_250_300MHZ 0x00000014
#define ETH_MAC_GMII_ADDR_CR_150_250MHZ 0x00000010
#define ETH_MAC_GMII_ADDR_CR_35_60MHZ   0x0000000c
#define ETH_MAC_GMII_ADDR_CR_20_35MHZ   0x00000008
#define ETH_MAC_GMII_ADDR_CR_100_150MHZ 0x00000004
#define ETH_MAC_GMII_ADDR_CR_60_100MHZ  0x00000000
#define ETH_MAC_GMII_ADDR_GW            0x00000002
#define ETH_MAC_GMII_ADDR_GB            0x00000001

/* ETH_MAC_FLOW_CTR_REG */
#define ETH_MAC_FLOW_CTR_PT(x)          (((x) & 0xffff) << 16)
#define ETH_MAC_FLOW_CTR_PLT_256        0x00000030
#define ETH_MAC_FLOW_CTR_PLT_144        0x00000020
#define ETH_MAC_FLOW_CTR_PLT_28         0x00000010
#define ETH_MAC_FLOW_CTR_PLT_4          0x00000000
#define ETH_MAC_FLOW_CTR_UP             0x00000008
#define ETH_MAC_FLOW_CTR_RFE            0x00000004
#define ETH_MAC_FLOW_CTR_TFE            0x00000002
#define ETH_MAC_FLOW_CTR_FCBBPA         0x00000001

/* ETH_MAC_VERSION_REG */
#define ETH_MAC_VERSION_USER(v)         (((x) & 0x0000ff00) >> 8)
#define ETH_MAC_VERSION_SYN(v)          ((x) & 0x000000ff)

/* ETH_DMA_BUS_MODE_REG */
#define ETH_DMA_BUS_MODE_FB             0x00010000
#define ETH_DMA_BUS_MODE_PR4            0x0000c000
#define ETH_DMA_BUS_MODE_PR3            0x00008000
#define ETH_DMA_BUS_MODE_PR2            0x00004000
#define ETH_DMA_BUS_MODE_PR1            0x00000000
#define ETH_DMA_BUS_MODE_PBL_32         0x00002000
#define ETH_DMA_BUS_MODE_PBL_16         0x00001000
#define ETH_DMA_BUS_MODE_PBL_8          0x00000800
#define ETH_DMA_BUS_MODE_PBL_4          0x00000400
#define ETH_DMA_BUS_MODE_PBL_2          0x00000200
#define ETH_DMA_BUS_MODE_PBL_1          0x00000100
#define ETH_DMA_BUS_MODE_DSL(len)       ((len & 0x1f) << 2)
#define ETH_DMA_BUS_MODE_DA_RX          0x00000002
#define ETH_DMA_BUS_MODE_DA_TX          0x00000000
#define ETH_DMA_BUS_MODE_SWR            0x00000001

/* ETH_DMA_STATUS_REG */
#define ETH_DMA_STATUS_GPI              0x10000000
#define ETH_DMA_STATUS_GMI              0x08000000
#define ETH_DMA_STATUS_GLI              0x04000000
#define ETH_DMA_STATUS_EB_MASK          0x03800000
#define ETH_DMA_STATUS_EB_TXDMA         0x02000000
#define ETH_DMA_STATUS_EB_RXDMA         0x00000000
#define ETH_DMA_STATUS_EB_RXFER         0x01000000
#define ETH_DMA_STATUS_EB_TXFER         0x00000000
#define ETH_DMA_STATUS_EB_DESC          0x00800000
#define ETH_DMA_STATUS_EB_DBUF          0x00000000
#define ETH_DMA_STATUS_TS_MASK          0x00700000
#define ETH_DMA_STATUS_TS_CTD           0x00700000
#define ETH_DMA_STATUS_TS_SUSP          0x00600000
#define ETH_DMA_STATUS_TS_READ          0x00300000
#define ETH_DMA_STATUS_TS_WAIT          0x00200000
#define ETH_DMA_STATUS_TS_FETCH         0x00100000
#define ETH_DMA_STATUS_TS_STOP          0x00000000
#define ETH_DMA_STATUS_RS_MASK          0x000e0000
#define ETH_DMA_STATUS_RS_RCV           0x000e0000
#define ETH_DMA_STATUS_RS_CRD           0x000a0000
#define ETH_DMA_STATUS_RS_SUSP          0x00080000
#define ETH_DMA_STATUS_RS_WAIT          0x00060000
#define ETH_DMA_STATUS_RS_FETCH         0x00020000
#define ETH_DMA_STATUS_RS_STOP          0x00000000
#define ETH_DMA_STATUS_NIS              0x00010000
#define ETH_DMA_STATUS_AIS              0x00008000
#define ETH_DMA_STATUS_ERI              0x00004000
#define ETH_DMA_STATUS_FBI              0x00002000
#define ETH_DMA_STATUS_ETI              0x00000400
#define ETH_DMA_STATUS_RWT              0x00000200
#define ETH_DMA_STATUS_RPS              0x00000100
#define ETH_DMA_STATUS_RU               0x00000080
#define ETH_DMA_STATUS_RI               0x00000040
#define ETH_DMA_STATUS_UNF              0x00000020
#define ETH_DMA_STATUS_OVF              0x00000010
#define ETH_DMA_STATUS_TJT              0x00000008
#define ETH_DMA_STATUS_TU               0x00000004
#define ETH_DMA_STATUS_TPS              0x00000002
#define ETH_DMA_STATUS_TI               0x00000001

/* ETH_DMA_OPMODE_REG */
#define ETH_DMA_OPMODE_DT               0x04000000
#define ETH_DMA_OPMODE_RSF              0x02000000
#define ETH_DMA_OPMODE_DFF              0x01000000
#define ETH_DMA_OPMODE_TSF              0x00200000
#define ETH_DMA_OPMODE_FTF              0x00100000
#define ETH_DMA_OPMODE_TTC_16           0x0001c000
#define ETH_DMA_OPMODE_TTC_24           0x00018000
#define ETH_DMA_OPMODE_TTC_32           0x00014000
#define ETH_DMA_OPMODE_TTC_40           0x00010000
#define ETH_DMA_OPMODE_TTC_256          0x0000c000
#define ETH_DMA_OPMODE_TTC_192          0x00008000
#define ETH_DMA_OPMODE_TTC_128          0x00004000
#define ETH_DMA_OPMODE_TTC_64           0x00000000
#define ETH_DMA_OPMODE_ST               0x00002000
#define ETH_DMA_OPMODE_RFD_4K           0x00001800
#define ETH_DMA_OPMODE_RFD_3K           0x00001000
#define ETH_DMA_OPMODE_RFD_2K           0x00000800
#define ETH_DMA_OPMODE_RFD_1K           0x00000000
#define ETH_DMA_OPMODE_RFA_4K           0x00000600
#define ETH_DMA_OPMODE_RFA_3K           0x00000400
#define ETH_DMA_OPMODE_RFA_2K           0x00000200
#define ETH_DMA_OPMODE_RFA_1K           0x00000000
#define ETH_DMA_OPMODE_EFC              0x00000100
#define ETH_DMA_OPMODE_FEF              0x00000080
#define ETH_DMA_OPMODE_FUF              0x00000040
#define ETH_DMA_OPMODE_RTC_128          0x00000018
#define ETH_DMA_OPMODE_RTC_96           0x00000010
#define ETH_DMA_OPMODE_RTC_32           0x00000008
#define ETH_DMA_OPMODE_RTC_64           0x00000000
#define ETH_DMA_OPMODE_OSF              0x00000004
#define ETH_DMA_OPMODE_SR               0x00000002

/* ETH_DMA_INTEN_REG */
#define ETH_DMA_INTEN_NIE               0x00010000
#define ETH_DMA_INTEN_AIE               0x00008000
#define ETH_DMA_INTEN_ERE               0x00004000
#define ETH_DMA_INTEN_FBE               0x00002000
#define ETH_DMA_INTEN_ETE               0x00000400
#define ETH_DMA_INTEN_RWE               0x00000200
#define ETH_DMA_INTEN_RSE               0x00000100
#define ETH_DMA_INTEN_RUE               0x00000080
#define ETH_DMA_INTEN_RIE               0x00000040
#define ETH_DMA_INTEN_UNE               0x00000020
#define ETH_DMA_INTEN_OVE               0x00000010
#define ETH_DMA_INTEN_TJE               0x00000008
#define ETH_DMA_INTEN_TUE               0x00000004
#define ETH_DMA_INTEN_TSE               0x00000002
#define ETH_DMA_INTEN_TIE               0x00000001

/* ETH_DMA_MISS_FRAME_BOCNT_REG */
#define ETH_DMA_MISS_FRAME_BOCNT_FIFO       0x10000000
#define ETH_DMA_MISS_FRAME_BOCNT_APP(v)     (((v) & 0x0ffe0000) >> 17)
#define ETH_DMA_MISS_FRAME_BOCNT_FRAME      0x00001000
#define ETH_DMA_MISS_FRAME_BOCNT_HOST(v)    ((v) & 0x0000ffff)

/* Receive Descriptor 0 (RDES0) */
#define ETH_RDES0_OWN                   0x80000000
#define ETH_RDES0_AFM                   0x40000000
#define ETH_RDES0_FL(v)                 (((v) & 0x3fff0000) >> 16)
#define ETH_RDES0_ES                    0x00008000
#define ETH_RDES0_DE                    0x00004000
#define ETH_RDES0_SAF                   0x00002000
#define ETH_RDES0_LE                    0x00001000
#define ETH_RDES0_OE                    0x00000800
#define ETH_RDES0_VLAN                  0x00000400
#define ETH_RDES0_FS                    0x00000200
#define ETH_RDES0_LS                    0x00000100
#define ETH_RDES0_IPC                   0x00000080
#define ETH_RDES0_LC                    0x00000040
#define ETH_RDES0_FT                    0x00000020
#define ETH_RDES0_RWT                   0x00000010
#define ETH_RDES0_RE                    0x00000008
#define ETH_RDES0_DBE                   0x00000004
#define ETH_RDES0_CE                    0x00000002
#define ETH_RDES0_RX                    0x00000001

#define ETH_RDES0_COE_MASK              0x000000a1
#define ETH_RDES0_COE_LENLT600          0x00000000  /* Bit(5:7:0)=>0 IEEE 802.3 type frame Length field is Lessthan 0x0600          */
#define ETH_RDES0_COE_UNSUPPORTED       0x00000001  /* Bit(5:7:0)=>1 Payload & Ip header checksum bypassed (unsuppported payload)       */
#define ETH_RDES0_COE_RESERVED          0x00000080  /* Bit(5:7:0)=>2 Reserved                               */
#define ETH_RDES0_COE_CHKBYPASS         0x00000081  /* Bit(5:7:0)=>3 Neither IPv4 nor IPV6. So checksum bypassed                */
#define ETH_RDES0_COE_NOCHKERROR        0x00000020  /* Bit(5:7:0)=>4 No IPv4/IPv6 Checksum error detected                   */
#define ETH_RDES0_COE_PLCHKERROR        0x00000021  /* Bit(5:7:0)=>5 Payload checksum error detected for Ipv4/Ipv6 frames           */
#define ETH_RDES0_COE_HDRCHKERROR       0x000000a0  /* Bit(5:7:0)=>6 Ip header checksum error detected for Ipv4 frames          */
#define ETH_RDES0_COE_HDRPLCHKERROR     0x000000a1  /* Bit(5:7:0)=>7 Payload & Ip header checksum error detected for Ipv4/Ipv6 frames   */

/* Receive Descriptor 1 (RDES1) */
#define ETH_RDES1_DIC                   0x80000000
#define ETH_RDES1_RER                   0x02000000
#define ETH_RDES1_RCH                   0x01000000
#define ETH_RDES1_RBS2(v)               (((v) & 0x003ff800) >> 11)  /* Receive Buffer 2 Size */
#define ETH_RDES1_RBS1(v)               ((v) & 0x000007ff)      /* Receive Buffer 1 Size */
#define ETH_RDES1_RBS2x(x)              (((x) << 11) & 0x003ff800)  /* Receive Buffer 2 Size */
#define ETH_RDES1_RBS1x(x)              ((x) & 0x000007ff)      /* Receive Buffer 1 Size */

/* Transmit Descriptor 0 (TDES0) */
#define ETH_TDES0_OWN                   0x80000000
#define ETH_TDES0_TTSS                  0x00020000
#define ETH_TDES0_IHE                   0x00010000
#define ETH_TDES0_ES                    0x00008000
#define ETH_TDES0_JT                    0x00004000
#define ETH_TDES0_FF                    0x00002000
#define ETH_TDES0_PCE                   0x00001000
#define ETH_TDES0_LCA                   0x00000800
#define ETH_TDES0_NC                    0x00000400
#define ETH_TDES0_LCO                   0x00000200
#define ETH_TDES0_EC                    0x00000100
#define ETH_TDES0_VF                    0x00000080
#define ETH_TDES0_CC(v)                 (((v) & 0x00000078) >> 3)
#define ETH_TDES0_ED                    0x00000004
#define ETH_TDES0_UF                    0x00000002
#define ETH_TDES0_DB                    0x00000001
#define ETH_TDES0_ES_MASK               (ETH_TDES0_UF | ETH_TDES0_ED | \
                                        ETH_TDES0_EC | ETH_TDES0_LCO | \
                                        ETH_TDES0_NC | ETH_TDES0_LCA | \
                                        ETH_TDES0_FF | ETH_TDES0_JT | \
                                        ETH_TDES0_ES)

/* Transmit Descriptor 1 (TDES1) */
#define ETH_TDES1_IC                    0x80000000
#define ETH_TDES1_LS                    0x40000000
#define ETH_TDES1_FS                    0x20000000
#define ETH_TDES1_CIC_TUI               0x10000000
#define ETH_TDES1_CIC_HDR               0x08000000
#define ETH_TDES1_DC                    0x04000000
#define ETH_TDES1_TER                   0x02000000
#define ETH_TDES1_TCH                   0x01000000
#define ETH_TDES1_DP                    0x00800000
#define ETH_TDES1_TBS2(v)               (((v) & 0x003ff800) >> 11)
#define ETH_TDES1_TBS1(v)               ((v) & 0x000007ff)
#define ETH_TDES1_TBS2x(x)              (((x) << 11) & 0x003ff800)
#define ETH_TDES1_TBS1x(x)              ((x) & 0x000007ff)

/* ==========================================================================*/
#define GKETH_MAC_SIZE                  (6)


#define EPHY_BASE                        (GK_VA_ETH_PHY)
#define EPHY_REG(x)                        (EPHY_BASE + (x))

#define REG_EPHY_CONTROL                EPHY_REG(0x000) /* read/write */
#define REG_EPHY_STATUS                 EPHY_REG(0x004) /* read */
#define REG_EPHY_ID1                    EPHY_REG(0x008) /* read */
#define REG_EPHY_ID2                    EPHY_REG(0x00C) /* read */
#define REG_EPHY_ANAR                   EPHY_REG(0x010) /* read/write */
#define REG_EPHY_ANLPAR                 EPHY_REG(0x014) /* read */
#define REG_EPHY_ANER                   EPHY_REG(0x018) /* read/write */
#define REG_EPHY_ANNPAR                 EPHY_REG(0x01C) /* read/write */
#define REG_EPHY_ANLPNP                 EPHY_REG(0x020) /* read */
#define REG_EPHY_MS_CONTROL             EPHY_REG(0x024) /* read/write */
#define REG_EPHY_MS_STATUS              EPHY_REG(0x028) /* read */
#define REG_EPHY_PSE_CONTROL            EPHY_REG(0x02C) /* read/write */
#define REG_EPHY_PSE_STATUS             EPHY_REG(0x030) /* read */
#define REG_EPHY_MMD_CONTROL            EPHY_REG(0x034) /* read/write */
#define REG_EPHY_MMD_CONTROL_ADDR       EPHY_REG(0x038) /* read/write */
#define REG_EPHY_AN_R_15                EPHY_REG(0x03C) /* read */
#define REG_EPHY_WAVE_SHAPING_34        EPHY_REG(0x040) /* read/write */
#define REG_EPHY_WAVE_SHAPING_56        EPHY_REG(0x044) /* read/write */
#define REG_EPHY_WAVE_SHAPING_78        EPHY_REG(0x048) /* read/write */
#define REG_EPHY_WAVE_SHAPING_9A        EPHY_REG(0x04C) /* read/write */
#define REG_EPHY_WAVE_SHAPING_BC        EPHY_REG(0x050) /* read/write */
#define REG_EPHY_WAVE_SHAPING_DE        EPHY_REG(0x054) /* read/write */
#define REG_EPHY_SPEED                  EPHY_REG(0x058) /* read/write */
#define REG_EPHY_LTP                    EPHY_REG(0x05C) /* read/write */
#define REG_EPHY_MCU                    EPHY_REG(0x060) /* read/write */
#define REG_EPHY_CODE_RAM               EPHY_REG(0x064) /* read/write */
#define REG_EPHY_CODE_RAM_W             EPHY_REG(0x068) /* read/write */
#define REG_EPHY_100M_LINK              EPHY_REG(0x088) /* read/write */
#define REG_EPHY_DEBUG                  EPHY_REG(0x0C8) /* read/write */
#define REG_EPHY_DEBUG_MODE             EPHY_REG(0x0E0) /* read/write */
#define REG_EPHY_ADC_GAIN_PGA           EPHY_REG(0x36C) /* read/write */
#define REG_EPHY_PLL_ADC_CTRL3          EPHY_REG(0x370) /* read/write */
#define REG_EPHY_RX_LPF                 EPHY_REG(0x374) /* read/write */
#define REG_EPHY_PLL_ADC_CTRL0          EPHY_REG(0x394) /* read/write */
#define REG_EPHY_PLL_ADC_CTRL1          EPHY_REG(0x398) /* read/write */
#define REG_EPHY_PLL_ADC_CTRL2          EPHY_REG(0x3A8) /* read/write */
#define REG_EPHY_ADC_DC                 EPHY_REG(0x3D4) /* read/write */
#define REG_EPHY_LDO                    EPHY_REG(0x3F8) /* read/write */
#define REG_EPHY_CLK_GATE               EPHY_REG(0x450) /* read */
#define REG_EPHY_CLK1                   EPHY_REG(0x460) /* read/write */
#define REG_EPHY_POWER                  EPHY_REG(0x474) /* read/write */
#define REG_EPHY_MDIIO                  EPHY_REG(0x540) /* read/write */
#define REG_EPHY_CLK0                   EPHY_REG(0x588) /* read/write */


/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct gk_eth_platform_info {
    u8                  mac_addr[GKETH_MAC_SIZE];
    u32                 napi_weight;
    u32                 watchdog_timeo;

    u32                 phy_id;
    u32                 phy_supported;
    u32                 mii_id;
    struct gk_gpio_io_info       phy_reset;
    u32                 mii_retry_limit;
    u32                 mii_retry_tmo;

    u32                 default_tx_ring_size;
    u32                 default_rx_ring_size;
    u32                 default_dma_bus_mode;
    u32                 default_dma_opmode;
    u32                 default_supported;

    int                 (*is_enabled)(void);
};

#define GK_ETH_PARAM_CALL(id, arg, perm) \
    module_param_cb(eth##id##_napi_weight, &param_ops_uint, &(arg.napi_weight), perm); \
    module_param_cb(eth##id##_watchdog_timeo, &param_ops_uint, &(arg.watchdog_timeo), perm); \
    module_param_cb(eth##id##_phy_id, &param_ops_uint, &(arg.phy_id), perm); \
    module_param_cb(eth##id##_mii_id, &param_ops_uint, &(arg.mii_id), perm); \
    module_param_cb(eth##id##_mii_retry_limit, &param_ops_uint, &(arg.mii_retry_limit), perm); \
    module_param_cb(eth##id##_mii_retry_tmo, &param_ops_uint, &(arg.mii_retry_tmo), perm);

/* ==========================================================================*/
extern struct platform_device           gk_eth0;

/* ==========================================================================*/
extern int gk_init_eth0(const u8 *mac_addr);
extern void gk_set_phy_reset_pin(u32 gpio_pin);
extern void gk_set_phy_speed_led(u32 gpio_type);


#endif /* __ASSEMBLER__ */
/* ==========================================================================*/


#endif /* __MACH_ETH_H__ */

