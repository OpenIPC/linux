////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ( "MStar Confidential Information" ) by the recipient.
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
#ifndef __DRV_EMAC__
#define __DRV_EMAC__

//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------
//#include <mstar/mstar_chip.h>




//-------------------------------------------------------------------------------------------------
//  Define Enable or Compiler Switches
//-------------------------------------------------------------------------------------------------
#define SOFTWARE_DESCRIPTOR
#define RX_CHECKSUM
#define INT_JULIAN_D
#define CHIP_FLUSH_READ
#define TX_QUEUE_4
//#define ISR_BOTTOM_HALF  //for RX
//#define TX_SOFTWARE_QUEUE //for TX
#define TX_SKB_PTR
//#define TX_SW_QUEUE
#define MSTAR_EMAC_NAPI
#define RX_ZERO_COPY
//#define LAN_ESD_CARRIER_INTERRUPT

#ifdef TX_SW_QUEUE
#define TX_SW_QUEUE_SIZE						(1024)  //effected size = TX_RING_SIZE - 1
#define TX_DESC_CLEARED						0
#define TX_DESC_WROTE						1
#define TX_DESC_READ						2
#define TX_FIFO_SIZE                        4 //HW FIFO size
#endif

#ifdef TX_QUEUE_4
#define TX_RING_SIZE						(4)  //effected size = TX_RING_SIZE - 1
#define EMAC_TX_COUNT       (14) //offset of TX counter in TSR
#else
#define TX_RING_SIZE						(2)  //effected size = TX_RING_SIZE - 1
#endif


#ifdef TX_QUEUE_4
#define EMAC_INT_MASK       (0xdff)
#else
#define EMAC_INT_MASK       (0xdff)
#endif

// Compiler Switches
#define REG_BIT_MAP
#define URANUS_ETHER_ADDR_CONFIGURABLE  /* MAC address can be changed? */
//--------------------------------------------------------------------------------------------------
//  Constant definition
//--------------------------------------------------------------------------------------------------
#define TRUE                                1
#define FALSE                               0

#define REG_BANK_EFUSE                      0x0020
#define REG_BANK_CLKGEN0                    0x1038
#define REG_BANK_SCGPCTRL                   0x1133
#define REG_BANK_CHIPTOP                    0x101E
#define REG_BANK_EMAC0                      0x1510 //0x1020
#define REG_BANK_EMAC1                      0x1511 //0x1021
#define REG_BACK_EMAC2                      0x1512 //0x1022
#define REG_BANK_EMAC3                      0x1513 //0x1023
#define REG_BANK_ALBANY0                    0x0031
#define REG_BANK_ALBANY1                    0x0032
#define REG_BANK_ALBANY2                    0x0033
#define REG_BANK_PMSLEEP                    0x000E


#define SOFTWARE_DESCRIPTOR_ENABLE          0x0001
#define CHECKSUM_ENABLE                     0x0FE
#define RX_CHECKSUM_ENABLE                  0x000E
#define CONFIG_EMAC_MOA                     1   // System Type
#define EMAC_SPEED_10                       10
#define EMAC_SPEED_100                      100

#define EMAC_ALLFF                          0xFFFFFFFF
#define EMAC_ABSO_MEM_BASE                  0xA0000000//EMAC_ABSO_MEM_BASE                  0xA0000000
#define INTERNEL_PHY_REG_BASE               (EMAC_RIU_REG_BASE+REG_BANK_ALBANY0*0x200)//0xFD243000
#define EMAC_RIU_REG_BASE                   0xFD000000
#define EMAC_ABSO_PHY_BASE                  0x80000000//EMAC_ABSO_MEM_BASE
#define EMAC_ABSO_MEM_SIZE                  0x30000//0x16000//0x180000//0x16000//(48 * 1024)     // More than: (32 + 16(0x3FFF)) KB
#define EMAC_MEM_SIZE_SQU                   4    // More than: (32 + 16(0x3FFF)) KB
#define EMAC_BUFFER_MEM_SIZE                0x0004000
#define EMAC_MAX_TX_QUEUE                   1000

// Base address here:
#define MIU0_BUS_BASE                       0x20000000
#define REG_ADDR_BASE                       (EMAC_RIU_REG_BASE+REG_BANK_EMAC0*0x200)//0xFD204000      // The register address base. Depends on system define.
#define RBQP_LENG                           0x80 // 0x40//                // ==?descriptors
#define MAX_RX_DESCR                        RBQP_LENG//32   /* max number of receive buffers */
#define SOFTWARE_DESC_LEN                   0x600
#ifdef SOFTWARE_DESCRIPTOR
#define RX_BUFFER_SEL                       0x0001          // 0x0=2KB,0x1=4KB,0x2=8KB,0x3=16KB, 0x09=1MB
#define RX_BUFFER_SIZE                      (RBQP_LENG*SOFTWARE_DESC_LEN)  //0x10000//0x20000//
#else
#define RX_BUFFER_SEL                       0x0003          // 0x0=2KB,0x1=4KB,0x2=8KB,0x3=16KB, 0x09=1MB
#define RX_BUFFER_SIZE                      (0x2000<<RX_BUFFER_SEL)//0x10000//0x20000//
#endif
#define RBQP_HW_BYTES    					0x0008
#define RBQP_SIZE                           (RBQP_HW_BYTES*RBQP_LENG)      // ==8bytes*?==?descriptors

#define MAX_INT_COUNTER                     100

#ifdef CONFIG_ETHERNET_ALBANY
    #define JULIAN_100_VAL                      (0x0000F011)
#else
    //#define JULIAN_100_VAL                      (0x0000F017) //for old board
    #define JULIAN_100_VAL                      (0x0000F015) //for GPHY
#endif


#define JULIAN_104_VAL                      (0x01010080UL)
#ifdef MSTAR_EMAC_NAPI
#define EMAC_NAPI_WEIGHT                    128
#endif
#define TX_CHECKSUM_ENABLE                  (0x00000470)
//PHY Register
#define PHY_REG_BASIC                       (0)
#define PHY_REG_STATUS                      (1)
//PHY setting
#define PHY_REGADDR_OFFSET                  (18)
#define PHY_ADDR_OFFSET                     (23)

//-------------------------------------------------------------------------------------------------
//  Bit Define
//-------------------------------------------------------------------------------------------------
#define EMAC_MIU_RW       (0x3 <<  10)   //EMAC power on clk

// -------- EMAC_CTL : (EMAC Offset: 0x0)  --------
#define EMAC_LB           ( 0x1 <<  0) // (EMAC) Loopback. Optional. When set, loopback signal is at high level.
#define EMAC_LBL          ( 0x1 <<  1) // (EMAC) Loopback local.
#define EMAC_RE           ( 0x1 <<  2) // (EMAC) Receive enable.
#define EMAC_TE           ( 0x1 <<  3) // (EMAC) Transmit enable.
#define EMAC_MPE          ( 0x1 <<  4) // (EMAC) Management port enable.
#define EMAC_CSR          ( 0x1 <<  5) // (EMAC) Clear statistics registers.
#define EMAC_ISR          ( 0x1 <<  6) // (EMAC) Increment statistics registers.
#define EMAC_WES          ( 0x1 <<  7) // (EMAC) Write enable for statistics registers.
#define EMAC_BP           ( 0x1 <<  8) // (EMAC) Back pressure.
// -------- EMAC_CFG : (EMAC Offset: 0x4) Network Configuration Register --------
#define EMAC_SPD          ( 0x1 <<  0) // (EMAC) Speed.
#define EMAC_FD           ( 0x1 <<  1) // (EMAC) Full duplex.
#define EMAC_BR           ( 0x1 <<  2) // (EMAC) Bit rate.
#define EMAC_CAF          ( 0x1 <<  4) // (EMAC) Copy all frames.
#define EMAC_NBC          ( 0x1 <<  5) // (EMAC) No broadcast.
#define EMAC_MTI          ( 0x1 <<  6) // (EMAC) Multicast hash enable
#define EMAC_UNI          ( 0x1 <<  7) // (EMAC) Unicast hash enable.
#define EMAC_RLF          ( 0x1 <<  8) // (EMAC) Receive Long Frame.
#define EMAC_EAE          ( 0x1 <<  9) // (EMAC) External address match enable.
#define EMAC_CLK          ( 0x3 << 10) // (EMAC)
#define EMAC_CLK_HCLK_8   ( 0x0 << 10) // (EMAC) HCLK divided by 8
#define EMAC_CLK_HCLK_16  ( 0x1 << 10) // (EMAC) HCLK divided by 16
#define EMAC_CLK_HCLK_32  ( 0x2 << 10) // (EMAC) HCLK divided by 32
#define EMAC_CLK_HCLK_64  ( 0x3 << 10) // (EMAC) HCLK divided by 64
#define EMAC_RT           ( 0x1 << 12) // (EMAC) Retry test
#define EMAC_CAMMEG       ( 0x1 << 13) // (EMAC)
// -------- EMAC_SR : (EMAC Offset: 0x8) Network Status Register --------
#define EMAC_MDIO         ( 0x1 <<  1) // (EMAC)
#define EMAC_IDLE         ( 0x1 <<  2) // (EMAC)
// -------- EMAC_TCR : (EMAC Offset: 0x10) Transmit Control Register --------
#define EMAC_LEN          ( 0x7FF <<  0) // (EMAC)
#define EMAC_NCRC         ( 0x1 << 15) // (EMAC)
// -------- EMAC_TSR : (EMAC Offset: 0x14) Transmit Control Register --------
#define EMAC_OVR          ( 0x1 <<  0) // (EMAC)
#define EMAC_COL          ( 0x1 <<  1) // (EMAC)
#define EMAC_RLE          ( 0x1 <<  2) // (EMAC)
//#define EMAC_TXIDLE     ( 0x1 <<  3) // (EMAC)
#define EMAC_IDLETSR      ( 0x1 <<  3) // (EMAC)
#define EMAC_BNQ          ( 0x1 <<  4) // (EMAC)
#define EMAC_COMP         ( 0x1 <<  5) // (EMAC)
#define EMAC_UND          ( 0x1 <<  6) // (EMAC)
#define EMAC_TBNQ         ( 0x1 <<  7) // (EMAC)
#define EMAC_FBNQ         ( 0x1 <<  8) // (EMAC)
#define EMAC_FIFO1IDLE    ( 0x1 <<  9) // (EMAC)
#define EMAC_FIFO2IDLE    ( 0x1 <<  10) // (EMAC)
#define EMAC_FIFO3IDLE    ( 0x1 <<  11) // (EMAC)
#define EMAC_FIFO4IDLE    ( 0x1 <<  12) // (EMAC)
// -------- EMAC_RSR : (EMAC Offset: 0x20) Receive Status Register --------
#define EMAC_DNA          ( 0x1 <<  0) // (EMAC)
#define EMAC_REC          ( 0x1 <<  1) // (EMAC)
#define EMAC_RSROVR       ( 0x1 <<  2) // (EMAC)
#define EMAC_BNA          ( 0x1 <<  3) // (EMAC)
// -------- EMAC_ISR : (EMAC Offset: 0x24) Interrupt Status Register --------
#define EMAC_INT_DONE     ( 0x1 <<  0) // (EMAC)
#define EMAC_INT_RCOM     ( 0x1 <<  1) // (EMAC)
#define EMAC_INT_RBNA     ( 0x1 <<  2) // (EMAC)
#define EMAC_INT_TOVR     ( 0x1 <<  3) // (EMAC)
#define EMAC_INT_TUND     ( 0x1 <<  4) // (EMAC)
#define EMAC_INT_RTRY     ( 0x1 <<  5) // (EMAC)
#define EMAC_INT_TBRE     ( 0x1 <<  6) // (EMAC)
#define EMAC_INT_TCOM     ( 0x1 <<  7) // (EMAC)
#define EMAC_INT_TIDLE    ( 0x1 <<  8) // (EMAC)
#define EMAC_INT_LINK     ( 0x1 <<  9) // (EMAC)
#define EMAC_INT_ROVR     ( 0x1 << 10) // (EMAC)
#define EMAC_INT_HRESP    ( 0x1 << 11) // (EMAC)
// -------- EMAC_IER : (EMAC Offset: 0x28) Interrupt Enable Register --------
// -------- EMAC_IDR : (EMAC Offset: 0x2c) Interrupt Disable Register --------
// -------- EMAC_IMR : (EMAC Offset: 0x30) Interrupt Mask Register --------
// -------- EMAC_MAN : (EMAC Offset: 0x34) PHY Maintenance Register --------
#define EMAC_DATA         ( 0xFFFF <<  0) // (EMAC)
#define EMAC_CODE         ( 0x3 << 16) // (EMAC)
#define EMAC_CODE_802_3   ( 0x2 << 16) // (EMAC) Write Operation
#define EMAC_REGA         ( 0x1F << 18) // (EMAC)
#define EMAC_PHYA         ( 0x1F << 23) // (EMAC)
#define EMAC_RW           ( 0x3 << 28) // (EMAC)
#define EMAC_RW_R         ( 0x2 << 28) // (EMAC) Read Operation
#define EMAC_RW_W         ( 0x1 << 28) // (EMAC) Write Operation
#define EMAC_HIGH         ( 0x1 << 30) // (EMAC)
#define EMAC_LOW          ( 0x1 << 31) // (EMAC)
// -------- EMAC_RBRP: (EMAC Offset: 0x38) Receive Buffer First full pointer--------
#define EMAC_WRAP_R       ( 0x1 << 14) // Wrap bit
// -------- EMAC_RBWP: (EMAC Offset: 0x3C) Receive Buffer Current pointer--------
#define EMAC_WRAP_W       ( 0x1 << 14) // Wrap bit
// ........................................................................ //
//URANUS PHY //
#define MII_URANUS_ID                   0x01111//Test value
//URANUS specific registers //
#define MII_USCR_REG                    16
#define MII_USCSR_REG                   17
#define MII_USINTR_REG                  21
/* ........................................................................ */
#define EMAC_DESC_DONE                  0x00000001  /* bit for if DMA is done */
#define EMAC_DESC_WRAP                  0x00000002  /* bit for wrap */
#define EMAC_BROADCAST                  0x80000000  /* broadcast address */
#define EMAC_MULTICAST                  0x40000000  /* multicast address */
#define EMAC_UNICAST                    0x20000000  /* unicast address */

#define EMAC_DESC_IPV4                  (0x1 << 11)
#define EMAC_DESC_UDP                   (0x1 << 18)
#define EMAC_DESC_TCP                   (0x1 << 19)
#define EMAC_DESC_IP_CSUM               (0x1 << 20)
#define EMAC_DESC_TCP_UDP_CSUM          (0x1 << 21)
// Constant: ----------------------------------------------------------------
// Register MAP:
#define REG_ETH_CTL                     0x00000000         // Network control register
#define REG_ETH_CFG                     0x00000004         // Network configuration register
#define REG_ETH_SR                      0x00000008         // Network status register
#define REG_ETH_TAR                     0x0000000C         // Transmit address register
#define REG_ETH_TCR                     0x00000010         // Transmit control register
#define REG_ETH_TSR                     0x00000014         // Transmit status register
#define REG_ETH_RBQP                    0x00000018         // Receive buffer queue pointer
#define REG_ETH_BUFF                    0x0000001C         // Receive Buffer Configuration
#define REG_ETH_RSR                     0x00000020         // Receive status register
#define REG_ETH_ISR                     0x00000024         // Interrupt status register
#define REG_ETH_IER                     0x00000028         // Interrupt enable register
#define REG_ETH_IDR                     0x0000002C         // Interrupt disable register
#define REG_ETH_IMR                     0x00000030         // Interrupt mask register
#define REG_ETH_MAN                     0x00000034         // PHY maintenance register
#define REG_ETH_BUFFRDPTR               0x00000038         // Receive First Full Pointer
#define REG_ETH_BUFFWRPTR               0x0000003C         // Receive Current pointer
#define REG_ETH_FRA                     0x00000040         // Frames transmitted OK
#define REG_ETH_SCOL                    0x00000044         // Single collision frames
#define REG_ETH_MCOL                    0x00000048         // Multiple collision frames
#define REG_ETH_OK                      0x0000004C         // Frames received OK
#define REG_ETH_SEQE                    0x00000050         // Frame check sequence errors
#define REG_ETH_ALE                     0x00000054         // Alignment errors
#define REG_ETH_DTE                     0x00000058         // Deferred transmission frames
#define REG_ETH_LCOL                    0x0000005C         // Late collisions
#define REG_ETH_ECOL                    0x00000060         // Excessive collisions
#define REG_ETH_TUE                     0x00000064         // Transmit under-run errors
#define REG_ETH_CSE                     0x00000068         // Carrier sense errors
#define REG_ETH_RE                      0x0000006C         // Receive resource error
#define REG_ETH_ROVR                    0x00000070         // Received overrun
#define REG_ETH_SE                      0x00000074         // Received symbols error
#define REG_ETH_ELR                     0x00000078         // Excessive length errors
#define REG_ETH_RJB                     0x0000007C         // Receive jabbers
#define REG_ETH_USF                     0x00000080         // Undersize frames
#define REG_ETH_SQEE                    0x00000084         // SQE test errors
#define REG_ETH_HSL                     0x00000090         // Hash register [31:0]
#define REG_ETH_HSH                     0x00000094         // Hash register [63:32]
#define REG_ETH_SA1L                    0x00000098         // Specific address 1 first 4 bytes
#define REG_ETH_SA1H                    0x0000009C         // Specific address 1 last  2 bytes
#define REG_ETH_SA2L                    0x000000A0         // Specific address 2 first 4 bytes
#define REG_ETH_SA2H                    0x000000A4         // Specific address 2 last  2 bytes
#define REG_ETH_SA3L                    0x000000A8         // Specific address 3 first 4 bytes
#define REG_ETH_SA3H                    0x000000AC         // Specific address 3 last  2 bytes
#define REG_ETH_SA4L                    0x000000B0         // Specific address 4 first 4 bytes
#define REG_ETH_SA4H                    0x000000B4         // Specific address 4 last  2 bytes
#define REG_TAG_TYPE                    0x000000B8         // tag type of the frame
#define REG_CAMA0_l                     0x00000200         // 16 LSB of CAM address  0
#define REG_CAMA0_h                     0x00000204         // 32 MSB of CAM address  0
#define REG_CAMA62_l                    0x000003F0         // 16 LSB of CAM address 62
#define REG_CAMA62_h                    0x000003F4         // 32 MSB of CAM address 62

#define REG_EMAC_JULIAN_0100            0x00000100
#define REG_EMAC_JULIAN_0104            0x00000104
#define REG_EMAC_JULIAN_0108            0x00000108
#define REG_EMAC_JULIAN_0138            0x00000138

u32 MHal_EMAC_ReadReg32( u32 xoffset );
u32 MHal_EMAC_ReadRam32( u32 uRamAddr, u32 xoffset);
u32 MHal_EMAC_NegotiationPHY(void);
u32 MHal_EMAC_CableConnection(void);
u32 MHal_EMAC_get_SA1H_addr(void);
u32 MHal_EMAC_get_SA1L_addr(void);
u32 MHal_EMAC_get_SA2H_addr(void);
u32 MHal_EMAC_get_SA2L_addr(void);
u32 MHal_EMAC_Read_CTL(void);
u32 MHal_EMAC_Read_CFG(void);
u32 MHal_EMAC_Read_RBQP(void);
u32 MHal_EMAC_Read_ISR(void);
u32 MHal_EMAC_Read_IDR(void);
u32 MHal_EMAC_Read_IER(void);
u32 MHal_EMAC_Read_IMR(void);
u32 MHal_EMAC_Read_RDPTR(void);
u32 MHal_EMAC_Read_BUFF(void);
u32 MHal_EMAC_Read_FRA(void);
u32 MHal_EMAC_Read_SCOL(void);
u32 MHal_EMAC_Read_MCOL(void);
u32 MHal_EMAC_Read_OK(void);
u32 MHal_EMAC_Read_SEQE(void);
u32 MHal_EMAC_Read_ALE(void);
u32 MHal_EMAC_Read_LCOL(void);
u32 MHal_EMAC_Read_ECOL(void);
u32 MHal_EMAC_Read_TUE(void);
u32 MHal_EMAC_Read_TCR( void );
u32 MHal_EMAC_Read_TSR(void);
u32 MHal_EMAC_Read_RSR( void );
u32 MHal_EMAC_Read_CSE(void);
u32 MHal_EMAC_Read_RE(void);
u32 MHal_EMAC_Read_ROVR(void);
u32 MHal_EMAC_Read_SE(void);
u32 MHal_EMAC_Read_ELR(void);
u32 MHal_EMAC_Read_RJB(void);
u32 MHal_EMAC_Read_USF(void);
u32 MHal_EMAC_Read_SQEE(void);
u32 MHal_EMAC_Read_JULIAN_0100(void);
u32 MHal_EMAC_Read_JULIAN_0104(void);
u32 MHal_EMAC_Read_JULIAN_0108(void);

void MHal_EMAC_WritReg32( u32 xoffset, u32 xval );
void MHal_EMAC_update_HSH(u32 mc0, u32 mc1);
void MHal_EMAC_Write_CTL(u32 xval);
void MHal_EMAC_Write_CFG(u32 xval);
void MHal_EMAC_Write_RBQP(u32 u32des);
void MHal_EMAC_Write_BUFF(u32 xval);
void MHal_EMAC_Write_MAN(u32 xval);
void MHal_EMAC_Write_TAR(u32 xval);
void MHal_EMAC_Write_TCR(u32 xval);
void MHal_EMAC_Write_TSR( u32 xval );
void MHal_EMAC_Write_RSR( u32 xval );
void MHal_EMAC_Write_RDPTR(u32 xval);
void MHal_EMAC_Write_WRPTR(u32 xval);
void MHal_EMAC_Write_ISR( u32 xval );
void MHal_EMAC_Write_IER(u32 xval);
void MHal_EMAC_Write_IDR(u32 xval);
void MHal_EMAC_Write_IMR(u32 xval);
void MHal_EMAC_Write_SA1H(u32 xval);
void MHal_EMAC_Write_SA1L(u32 xval);
void MHal_EMAC_Write_SA2H(u32 xval);
void MHal_EMAC_Write_SA2L(u32 xval);
void MHal_EMAC_Write_JULIAN_0100(u32 xval);
void MHal_EMAC_Write_JULIAN_0104(u32 xval);
void MHal_EMAC_Write_JULIAN_0108(u32 xval);
void MHal_EMAC_Set_Tx_JULIAN_T(u32 xval);
u32 MHal_EMAC_Get_Tx_FIFO_Threshold(void);
void MHal_EMAC_Set_Rx_FIFO_Enlarge(u32 xval);
u32 MHal_EMAC_Get_Rx_FIFO_Enlarge(void);
void MHal_EMAC_Set_Miu_Priority(u32 xval);
u32 MHal_EMAC_Get_Miu_Priority(void);
void MHal_EMAC_Set_Tx_Hang_Fix_ECO(u32 xval);
void MHal_EMAC_Set_MIU_Out_Of_Range_Fix(u32 xval);
void MHal_EMAC_Set_Rx_Tx_Burst16_Mode(u32 xval);
void MHal_EMAC_Set_Tx_Rx_Req_Priority_Switch(u32 xval);
void MHal_EMAC_Set_Rx_Byte_Align_Offset(u32 xval);
void MHal_EMAC_Write_Protect(u32 start_addr, u32 length);
void MHal_EMAC_HW_init(void);
void MHal_EMAC_Power_On_Clk(struct device);
void MHal_EMAC_Power_Off_Clk(struct device);
void MHal_EMAC_timer_callback(unsigned long value);
void MHal_EMAC_WritRam32(u32 uRamAddr, u32 xoffset,u32 xval);
void MHal_EMAC_enable_mdi(void);
void MHal_EMAC_disable_mdi(void);
void MHal_EMAC_write_phy (unsigned char phy_addr, unsigned char address, u32 value);
void MHal_EMAC_read_phy(unsigned char phy_addr, unsigned char address,u32 *value);
void MHal_EMAC_enable_phyirq (void);
void MHal_EMAC_disable_phyirq (void);
void MHal_EMAC_update_speed_duplex(u32 uspeed, u32 uduplex);
void MHal_EMAC_Write_SA1_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5);
void MHal_EMAC_Write_SA2_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5);
void MHal_EMAC_Write_SA3_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5);
void MHal_EMAC_Write_SA4_MAC_Address(u8 m0,u8 m1,u8 m2,u8 m3,u8 m4,u8 m5);
void MHal_EMAC_Set_Reverse_LED(u32 xval);
u8 MHal_EMAC_Get_Reverse_LED(void);

#endif
// -----------------------------------------------------------------------------
// Linux EMAC.c End
// -----------------------------------------------------------------------------


