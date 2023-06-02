////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   EMAC.h
/// @author MStar Semiconductor Inc.
/// @brief  EMAC Driver Interface
///////////////////////////////////////////////////////////////////////////////////////////////////


// -----------------------------------------------------------------------------
// Linux EMAC.h define start
// -----------------------------------------------------------------------------

#ifndef __DRV_EMAC_H_
#define __DRV_EMAC_H_

#define EMAC_DBG(fmt, args...)              //{printk("Mstar_emac: "); printk(fmt, ##args);}
#define EMAC_ERR(fmt, args...)              {printk("Mstar_emac: "); printk(fmt, ##args);}
#define EMAC_INFO                           {printk("Line:%u\n", __LINE__);}
#define EMAC_TODO(fmt, args...)              {printk("[EMAC]%d TODO:", __LINE__); printk(fmt, ##args);}

#define MINOR_EMAC_NUM               1
#define MAJOR_EMAC_NUM               241
//-------------------------------------------------------------------------------------------------
//  Define Enable or Compiler Switches
//-------------------------------------------------------------------------------------------------
#define USE_TASK                            1            // 1:Yes, 0:No
#define EMAC_MTU                            (1524)




//--------------------------------------------------------------------------------------------------
//  Constant definition
//--------------------------------------------------------------------------------------------------
#if (!USE_TASK) // MEM_BASE_ADJUSTMENT ......................................
#define RAM_VA_BASE                         0xA0000000UL
#define RAM_PA_BASE                         0x00000000UL
#define RAM_VA_PA_OFFSET                    0x00000000UL
#define RX_BUFFER_BASE                      0x00000000UL         // ==0xA0000000 ~~ 0xA0004000 (Max: 16 KB)
#define RBQP_BASE                           RX_BUFFER_SIZE//0x00004000         // ==0xA0004000 ~~ 0xA0005FFF for MAX 1024 descriptors
#define TX_BUFFER_BASE                      (RX_BUFFER_SIZE+RBQP_SIZE)//0x00006000         // ==0xA0006000 ~~ ????????
#define TX_SKB_BASE                         TX_BUFFER_BASE+0x100UL//0x00006100
#define RX_FRAME_ADDR                       TX_SKB_BASE+0x600UL//0x00007000         // Software COPY&STORE one RX frame. Size is not defined.
#else // The memory allocation for TASK.
//--------------------------------------------------------------------------------------------------
//  Global variable
//--------------------------------------------------------------------------------------------------
phys_addr_t     RAM_VA_BASE;                      //= 0x00000000;     // After init, RAM_ADDR_BASE = EMAC_ABSO_MEM_BASE
phys_addr_t     RAM_PA_BASE;
phys_addr_t     RAM_VA_PA_OFFSET;
phys_addr_t     RBQP_BASE;                          //= RX_BUFFER_SIZE;//0x00004000;     // IMPORTANT: lowest 13 bits as zero.
#ifndef RX_ZERO_COPY
phys_addr_t     RX_BUFFER_BASE;                     //= 0x00000000;     // IMPORTANT: lowest 14 bits as zero.
//phys_addr_t     TX_BUFFER_BASE;                     //= (RX_BUFFER_SIZE+RBQP_SIZE);//0x00006000;
#endif
phys_addr_t     TX_SKB_BASE;                        //= (RX_BUFFER_SIZE+RBQP_SIZE+0x600);//0x00006100;

#endif //^MEM_BASE_ADJUSTMENT ...............................................

#define ETHERNET_TEST_NO_LINK               0x00000000UL
#define ETHERNET_TEST_AUTO_NEGOTIATION      0x00000001UL
#define ETHERNET_TEST_LINK_SUCCESS          0x00000002UL
#define ETHERNET_TEST_RESET_STATE           0x00000003UL
#define ETHERNET_TEST_SPEED_100M            0x00000004UL
#define ETHERNET_TEST_DUPLEX_FULL           0x00000008UL
#define ETHERNET_TEST_INIT_FAIL             0x00000010UL

#ifdef  RX_ZERO_COPY
//#define RX_RING_SIZE                       (RBQP_LENG*2)
#endif

u8 MY_DEV[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
u8 MY_MAC[6] = { 0x00UL, 0x30UL, 0x1BUL, 0xBAUL, 0x02UL, 0xDBUL };
u8 PC_MAC[6] = { 0x00UL, 0x1AUL, 0x4BUL, 0x5CUL, 0x39UL, 0xDFUL };
u8 ETH_PAUSE_FRAME_DA_MAC[6] = { 0x01UL, 0x80UL, 0xC2UL, 0x00UL, 0x00UL, 0x01UL };

#ifdef INT_JULIAN_D
   u32 xoffsetValue, xReceiveNum;
#endif
//-------------------------------------------------------------------------------------------------
//  Data structure
//-------------------------------------------------------------------------------------------------
struct rbf_t
{
  u32  addr;
  u32  size;
};

struct recv_desc_bufs
{
    struct rbf_t descriptors[MAX_RX_DESCR];       /* must be on sizeof (rbf_t) boundary */
    char recv_buf[RX_BUFFER_SIZE];                /* must be on MAX_RBUFF_SZ boundary */
};

struct tx_ring
{
    u8 used;
    struct sk_buff *skb;                /* holds skb until xmit interrupt completes */
    dma_addr_t skb_physaddr;            /* phys addr from pci_map_single */
};

struct skb_node
{
    u32 queuesize;
    struct sk_buff *skb;
    struct skb_node *next;
    struct skb_node *prev;
};

#define EP_FLAG_OPEND   0X00000001UL
#define EP_FLAG_SUSPENDING   0X00000002UL


struct EMAC_private
{
    struct net_device_stats stats;
    struct mii_if_info mii;             /* ethtool support */

    /* PHY */
    unsigned long phy_type;             /* type of PHY (PHY_ID) */
    spinlock_t *lock;                    /* lock for MDI interface */
    short phy_media;                    /* media interface type */

    /* Transmit */
#if defined(TX_SKB_PTR)
#if defined(TX_SW_QUEUE)
    struct tx_ring tx_swq[TX_SW_QUEUE_SIZE];
    unsigned int tx_rdidx;       		/* TX_SW_QUEUE read to hw index */
    unsigned int tx_wridx;       		/* TX_SW_QUEUE write index */
	unsigned int tx_clidx;       		/* TX_SW_QUEUE clear index */

    unsigned int tx_rdwrp;       		/* TX_SW_QUEUE read to hw index wrap*/
    unsigned int tx_wrwrp;       		/* TX_SW_QUEUE write index wrap*/
    unsigned int tx_clwrp;       		/* TX_SW_QUEUE clear index wrap */
    unsigned int tx_swq_full_cnt;       /* TX_SW_QUEUE full stopped count*/
#else
    struct tx_ring tx_swq[TX_RING_SIZE];
    //unsigned int tx_rdidx;
    unsigned int tx_wridx;
#endif
#endif
    unsigned int irqcnt;
    unsigned int tx_irqcnt;

#ifdef RX_ZERO_COPY
//	struct rx_ring rx_ring_list[RX_RING_SIZE];
	/* RX variables only touched in napi_poll.  No locking necessary. */
	//u64 *rx_ring;
	//dma_addr_t rx_ring_handle;
	//unsigned int rx_next;
	//unsigned int rx_next_fill;
	//unsigned int rx_current_fill;
	//struct sk_buff_head rx_list;
#endif


    struct sk_buff *skb;                /* holds skb until xmit interrupt completes */
    dma_addr_t skb_physaddr;            /* phys addr from pci_map_single */
    int skb_length;                     /* saved skb length for pci_unmap_single */
    unsigned char retx_count;       	/* resend count of tx */
    unsigned int txpkt;                 /* previous tx packet pointer */

    /* Receive */
    int rxBuffIndex;                    /* index into receive descriptor list */
    struct recv_desc_bufs *dlist;       /* descriptor list address */
    //struct recv_desc_bufs *dlist_phys;  /* descriptor list physical address */

    /* Suspend and resume */
    unsigned long ep_flag;

    struct net_device *dev;

    /* Bottom task */
#if defined ISR_BOTTOM_HALF
    struct work_struct rx_task;
    struct work_struct tx_task;
#endif
    /* Tx software queue */
    struct skb_node *tx_queue_head;
    struct skb_node *tx_queue_tail;
    unsigned int xReceiveFlag;

    struct mutex mutex;
    struct device *mstar_class_emac_device;
#ifdef MSTAR_EMAC_NAPI
    struct napi_struct  napi;
#endif
};

#define ROUND_SUP_4(x) (((x)+3)&~3)

struct eth_drv_sgX
{
    u32 buf;
    u32 len;
};

struct _BasicConfigEMAC
{
    u8 connected;          // 0:No, 1:Yes    <== (20070515) Wait for Julian's reply
    u8 speed;               // 10:10Mbps, 100:100Mbps
    // ETH_CTL Register:
    u8 wes;             // 0:Disable, 1:Enable (WR_ENABLE_STATISTICS_REGS)
    // ETH_CFG Register:
    u8 duplex;              // 1:Half-duplex, 2:Full-duplex
    u8 cam;                // 0:No CAM, 1:Yes
    u8 rcv_bcast;       // 0:No, 1:Yes
    u8 rlf;                // 0:No, 1:Yes receive long frame(1522)
    // MAC Address:
    u8 sa1[6];              // Specific Addr 1 (MAC Address)
    u8 sa2[6];              // Specific Addr 2
    u8 sa3[6];              // Specific Addr 3
    u8 sa4[6];              // Specific Addr 4
};
typedef struct _BasicConfigEMAC BasicConfigEMAC;

struct _UtilityVarsEMAC
{
    u32 cntChkINTCounter;
    u32 readIdxRBQP;        // Reset = 0x00000000
    u32 rxOneFrameAddr;     // Reset = 0x00000000 (Store the Addr of "ReadONE_RX_Frame")
    // Statistics Counters : (accumulated)
    u32 cntREG_ETH_FRA;
    u32 cntREG_ETH_SCOL;
    u32 cntREG_ETH_MCOL;
    u32 cntREG_ETH_OK;
    u32 cntREG_ETH_SEQE;
    u32 cntREG_ETH_ALE;
    u32 cntREG_ETH_DTE;
    u32 cntREG_ETH_LCOL;
    u32 cntREG_ETH_ECOL;
    u32 cntREG_ETH_TUE;
    u32 cntREG_ETH_CSE;
    u32 cntREG_ETH_RE;
    u32 cntREG_ETH_ROVR;
    u32 cntREG_ETH_SE;
    u32 cntREG_ETH_ELR;
    u32 cntREG_ETH_RJB;
    u32 cntREG_ETH_USF;
    u32 cntREG_ETH_SQEE;
    // Interrupt Counter :
    u32 cntHRESP;           // Reset = 0x0000
    u32 cntROVR;            // Reset = 0x0000
    u32 cntLINK;            // Reset = 0x0000
    u32 cntTIDLE;           // Reset = 0x0000
    u32 cntTCOM;            // Reset = 0x0000
    u32 cntTBRE;            // Reset = 0x0000
    u32 cntRTRY;            // Reset = 0x0000
    u32 cntTUND;            // Reset = 0x0000
    u32 cntTOVR;            // Reset = 0x0000
    u32 cntRBNA;            // Reset = 0x0000
    u32 cntRCOM;            // Reset = 0x0000
    u32 cntDONE;            // Reset = 0x0000
    // Flags:
    u8 flagMacTxPermit;    // 0:No,1:Permitted.  Initialize as "permitted"
    u8 flagISR_INT_RCOM;
    u8 flagISR_INT_RBNA;
    u8 flagISR_INT_DONE;
    u8 flagPowerOn;        // 0:Poweroff, 1:Poweron
    u8 initedEMAC;         // 0:Not initialized, 1:Initialized.
    u8 flagRBNA;
    // Misc Counter:
    u32 cntRxFrames;        // Reset = 0x00000000 (Counter of RX frames,no matter it's me or not)
    u32 cntReadONE_RX;      // Counter for ReadONE_RX_Frame
    u32 cntCase20070806;
    u32 cntChkToTransmit;
    // Misc Variables:
    u32 mainThreadTasks;    // (20071029_CHARLES) b0=Poweroff,b1=Poweron
};
typedef struct _UtilityVarsEMAC UtilityVarsEMAC;

BasicConfigEMAC ThisBCE;
UtilityVarsEMAC ThisUVE;

typedef volatile unsigned int EMAC_REG;

struct sk_buff *Tx_SkbAddr;


#ifdef TESTING
    extern void EMAC_TEST_All(void);
#endif

struct sk_buff *rx_skb[MAX_RX_DESCR];
u32 rx_abso_addr[MAX_RX_DESCR];
struct sk_buff * rx_skb_dummy;
u32 	rx_abso_addr_dummy;
#endif
// -----------------------------------------------------------------------------
// Linux EMAC.h End
// -----------------------------------------------------------------------------


