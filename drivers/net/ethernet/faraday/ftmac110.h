#ifndef FTMAC110_H
#define FTMAC110_H

// --------------------------------------------------------------------
//        FTMAC110 MAC Registers
// --------------------------------------------------------------------

#define ISR_REG             0x00        // interrups status register
#define IMR_REG             0x04        // interrupt maks register
#define MAC_MADR_REG        0x08        // MAC address (Most significant)
#define MAC_LADR_REG        0x0c        // MAC address (Least significant)
#define MAHT0_REG           0x10        // Multicast Address Hash Table 0 register
#define MAHT1_REG           0x14        // Multicast Address Hash Table 1 register
#define TXPD_REG            0x18        // Transmit Poll Demand register
#define RXPD_REG            0x1c        // Receive Poll Demand register
#define TXR_BADR_REG        0x20        // Transmit Ring Base Address register
#define RXR_BADR_REG        0x24        // Receive Ring Base Address register
#define ITC_REG             0x28        // interrupt timer control register
#define APTC_REG            0x2C        // Automatic Polling Timer control register
#define DBLAC_REG           0x30        // DMA Burst Length and Arbitration control register

#define MACCR_REG           0x88        // MAC control register
#define MACSR_REG           0x8C        // MAC status register
#define PHYCR_REG           0x90        // PHY control register
#define PHYDATA_REG         0x94        // PHY Write Data register
#define FCR_REG             0x98        // Flow Control register
#define BPR_REG             0x9c        // back pressure register

//Interrupt status register(ISR), Interrupt mask register(IMR) bit setting
#define PHYSTS_CHG_bit		(1UL<<9)
#define AHB_ERR_bit		(1UL<<8)
#define RPKT_LOST_bit		(1UL<<7)
#define RPKT_SAV_bit		(1UL<<6)
#define XPKT_LOST_bit		(1UL<<5)
#define XPKT_OK_bit		(1UL<<4)
#define NOTXBUF_bit		(1UL<<3)
#define XPKT_FINISH_bit		(1UL<<2)
#define NORXBUF_bit		(1UL<<1)
#define RPKT_FINISH_bit		(1UL<<0)

/* MACC control bits */
#define Speed_100_bit		(1UL<<18)
#define RX_BROADPKT_bit		(1UL<<17)       // Receiving broadcast packet
#define RX_MULTIPKT_bit		(1UL<<16)       // receiving multicast packet
#define FULLDUP_bit		(1UL<<15)       // full duplex
#define CRC_APD_bit		(1UL<<14)       // append crc to transmit packet
#define RCV_ALL_bit		(1UL<<12)       // not check incoming packet's destination address
#define RX_FTL_bit		(1UL<<11)       // Store incoming packet even its length is les than 64 byte
#define RX_RUNT_bit		(1UL<<10)       // Store incoming packet even its length is les than 64 byte
#define HT_MULTI_EN_bit		(1UL<<9)
#define RCV_EN_bit		(1UL<<8)        // receiver enable
#define ENRX_IN_HALFTX_bit	(1UL<<6)        // rx in half tx
#define XMT_EN_bit		(1UL<<5)        // transmitter enable
#define CRC_DIS_bit		(1UL<<4)
#define LOOP_EN_bit		(1UL<<3)        // Internal loop-back
#define SW_RST_bit		(1UL<<2)
#define RDMA_EN_bit		(1UL<<1)        // enable DMA receiving channel
#define XDMA_EN_bit		(1UL<<0)        // enable DMA transmitting channel

// --------------------------------------------------------------------
//        MII PHY Registers
// --------------------------------------------------------------------

//
// Bits related to the MII interface
//
#define MIIREG_READ		(1 << 26)
#define MIIREG_WRITE		(1 << 27)
#define MIIREG_PHYREG_SHIFT	21
#define MIIREG_PHYADDR_SHIFT	16

// --------------------------------------------------------------------
//        Receive Ring descriptor structure
// --------------------------------------------------------------------
typedef struct {
    // RXDES0
    uint32_t len:11;            // BIT:  0 - 10
    uint32_t rsvd1:5;           // BIT: 11 - 15
    uint32_t mcast:1;           // BIT: 16
    uint32_t bcast:1;           // BIT: 17
    uint32_t error:5;
    uint32_t rsvd2:5;           // BIT: 23 - 27
    uint32_t lrs:1;             // BIT: 28
    uint32_t frs:1;             // BIT: 29
    uint32_t rsvd3:1;           // BIT: 30
    uint32_t owner:1;           // BIT: 31 - 1:Hardware, 0: Software

    // RXDES1
    uint32_t bufsz:11;          // BIT:  0 - 10
    uint32_t rsvd4:20;          // BIT: 11 - 30
    uint32_t end:1;             // BIT: 31

    // RXDES2
    uint32_t buf;

    // RXDES3
    void *skb;
} ftmac110_rxdesc_t;

typedef struct {
    // TXDES0
    uint32_t error:2;
    uint32_t rsvd1:29;
    uint32_t owner:1;           // BIT: 31 - 1:Hardware, 0: Software

    // TXDES1
    uint32_t len:11;            // BIT:  0 - 10
    uint32_t rsvd2:16;
    uint32_t lts:1;             // BIT: 27
    uint32_t fts:1;             // BIT: 28
    uint32_t tx2fic:1;          // BIT: 29
    uint32_t txic:1;            // BIT: 30
    uint32_t end:1;             // BIT: 31

    // TXDES2
    uint32_t buf;

    // TXDES3
    void *skb;

} ftmac110_txdesc_t;

#endif /* FTMAC_H */
