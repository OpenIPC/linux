/*
 * mdrv_miu.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef __MDRV_MIU_H__
#define __MDRV_MIU_H__
#include <linux/types.h>
#include <linux/device.h>
#ifdef CONFIG_64BIT
typedef unsigned long phy_addr;  // 4 bytes
typedef unsigned long virt_addr; // 4 bytes
#else
typedef unsigned long long phy_addr;  // 8 bytes
typedef unsigned long long virt_addr; // 8 bytes
#endif
/* Log color related */
#define ASCII_COLOR_RED    "\033[1;31m"
#define ASCII_COLOR_WHITE  "\033[1;37m"
#define ASCII_COLOR_YELLOW "\033[1;33m"
#define ASCII_COLOR_BLUE   "\033[1;36m"
#define ASCII_COLOR_GREEN  "\033[1;32m"
#define ASCII_COLOR_END    "\033[0m"

struct miu_device
{
    struct device dev;
    int           index;
    int           reg_dram_size; // register setting for dram size
};
#define MMU_ADDR_TO_REGION(addr)     ((addr >> 27) & 0xF)
#define MMU_ADDR_TO_REGION_128(addr) ((addr >> 27) & 0x1F)
#define MMU_ADDR_TO_REGION_256(addr) ((addr >> 28) & 0x0F)
#define MMU_ADDR_TO_REGION_512(addr) ((addr >> 29) & 0x07)

#define MMU_ADDR_TO_ENTRY(addr)     ((addr >> 17) & 0x3FF)
#define MMU_ADDR_TO_ENTRY_128(addr) ((addr >> 17) & 0x3FF)
#define MMU_ADDR_TO_ENTRY_256(addr) ((addr >> 18) & 0x3FF)
#define MMU_ADDR_TO_ENTRY_512(addr) ((addr >> 19) & 0x3FF)

#define MMU_PAGE_SIZE     (0x20000) // 128KB
#define MMU_PAGE_SIZE_128 (0x20000) // 128KB
#define MMU_PAGE_SIZE_256 (0x40000) // 256KB
#define MMU_PAGE_SIZE_512 (0x80000) // 512KB
typedef enum
{
    E_MIU_0 = 0,
    E_MIU_1,
    E_MIU_2,
    E_MIU_3,
    E_MIU_NUM,
} MIU_ID;

typedef enum
{
    E_MMU_PGSZ_128 = 0,
    E_MMU_PGSZ_256,
    E_MMU_PGSZ_512,
} MMU_PGSZ_MODE;

#ifdef CONFIG_MIU_HW_MMU
typedef enum
{
    E_MMU_STATUS_NORMAL       = 0,
    E_MMU_STATUS_RW_COLLISION = 0x1,
    E_MMU_STATUS_R_INVALID    = 0x2,
    E_MMU_STATUS_W_INVALID    = 0x4,
    E_MMU_STATUS_NUM,
} MMU_STATUS;
#endif

//-------------------------------------------------------------------------------------------------
//  Structure Define
//-------------------------------------------------------------------------------------------------

typedef struct
{
    unsigned char bHit;
    unsigned char u8Group;
    unsigned char u8ClientID;
    unsigned char u8Block;
    unsigned int  uAddress;
} MIU_PortectInfo;

typedef struct
{
    unsigned int  size;        // bytes
    unsigned int  dram_freq;   // MHz
    unsigned int  miupll_freq; // MHz
    unsigned char type;        // 2:DDR2, 3:DDR3
    unsigned char data_rate;   // 4:4x mode, 8:8x mode,
    unsigned char bus_width;   // 16:16bit, 32:32bit, 64:64bit
    unsigned char ssc;         // 0:off, 1:on
} MIU_DramInfo;
//-------------------------------------------------------------------------------------------------
//  Enumeration Define
//-------------------------------------------------------------------------------------------------

typedef enum
{
    // group 0
    MIU_CLIENT_NONE, // none can access
    MIU_CLIENT_GE_RW,
    MIU_CLIENT_BDMA_RW,
    MIU_CLIENT_MOVDMA0_RW,
    MIU_CLIENT_AESDMA_RW,
    MIU_CLIENT_CMDQ0_R,
    MIU_CLIENT_CMDQ1_R,
    MIU_CLIENT_URDMA_RW,
    MIU_CLIENT_CSI_TX_R,
    MIU_CLIENT_JPE_R,
    MIU_CLIENT_JPE_W,
    MIU_CLIENT_JPD_RW,
    MIU_CLIENT_BACH_RW,
    MIU_CLIENT_BACH1_RW,
    MIU_CLIENT_IVE_RW,
    MIU_CLIENT_MCU51_RW,
    // group 1
    MIU_CLIENT_USB30_RW,
    MIU_CLIENT_USB20_RW,
    MIU_CLIENT_USB20_H_RW,
    MIU_CLIENT_SD30_RW,
    MIU_CLIENT_SDIO30_RW,
    MIU_CLIENT_DUMMY_G1C5,
    MIU_CLIENT_SATA_RW,
    MIU_CLIENT_EMAC_RW,
    MIU_CLIENT_EMAC1_RW,
    MIU_CLIENT_LDCFeye_RW,
    MIU_CLIENT_GOP,
    MIU_CLIENT_GOP1_R,
    MIU_CLIENT_GOP2_R,
    MIU_CLIENT_GOP3_R,
    MIU_CLIENT_GOP4_R,
    MIU_CLIENT_GOP5_R,
    // group 2
    MIU_CLIENT_VCODEC_R,
    MIU_CLIENT_VCODEC_W,
    MIU_CLIENT_VEN_R = MIU_CLIENT_VCODEC_R,
    MIU_CLIENT_VEN_W = MIU_CLIENT_VCODEC_W,
    MIU_CLIENT_ISP_ROT_R,
    MIU_CLIENT_ISP_MOT0_W,
    MIU_CLIENT_ISP_MOT0_R,
    MIU_CLIENT_ISP_MLOAD_STA,
    MIU_CLIENT_GOP_DISP3_R,
    MIU_CLIENT_DUMMY_G2C7,
    MIU_CLIENT_RDMA_DIP_RW,
    MIU_CLIENT_SC0_FRAME_R,
    MIU_CLIENT_SC0_FRAME_W,
    MIU_CLIENT_SC1_FRAME_W,
    MIU_CLIENT_SC2_FRAME_W,
    MIU_CLIENT_SC3_FRAME_RW,
    MIU_CLIENT_SC4_FRAME_RW,
    MIU_CLIENT_SC5_FRAME_RW,
    // group 3
    MIU_CLIENT_DUMMY_G3C0,
    MIU_CLIENT_ISP_DMA_W,
    MIU_CLIENT_ISP_DMA_R,
    MIU_CLIENT_3DNR0_W,
    MIU_CLIENT_3DNR0_R,
    MIU_CLIENT_DUMMY_G3C5,
    MIU_CLIENT_PQ_VIP_DISP_W,
    MIU_CLIENT_DISP_CVBS_W,
    MIU_CLIENT_ISP_DMAG_RW,
    MIU_CLIENT_GOP_DISP0_R,
    MIU_CLIENT_GOP_DISP1_R,
    MIU_CLIENT_GOP_DISP2_R,
    MIU_CLIENT_MOPROT0_Y_R,
    MIU_CLIENT_MOPROT0_C_R,
    MIU_CLIENT_MOPROT1_Y_R,
    MIU_CLIENT_MOPROT1_C_R,

    // group 4
    MIU_CLIENT_DUMMY_G4C0,
    MIU_CLIENT_DUMMY_G4C1,
    MIU_CLIENT_DUMMY_G4C2,
    MIU_CLIENT_DUMMY_G4C3,
    MIU_CLIENT_DUMMY_G4C4,
    MIU_CLIENT_DUMMY_G4C5,
    MIU_CLIENT_DUMMY_G4C6,
    MIU_CLIENT_DUMMY_G4C7,
    MIU_CLIENT_DUMMY_G4C8,
    MIU_CLIENT_DUMMY_G4C9,
    MIU_CLIENT_DUMMY_G4CA,
    MIU_CLIENT_DUMMY_G4CB,
    MIU_CLIENT_DUMMY_G4CC,
    MIU_CLIENT_DUMMY_G4CD,
    MIU_CLIENT_DUMMY_G4CE,
    MIU_CLIENT_DUMMY_G4CF,
    // group 5
    MIU_CLIENT_DUMMY_G5C0,
    MIU_CLIENT_DUMMY_G5C1,
    MIU_CLIENT_DUMMY_G5C2,
    MIU_CLIENT_DUMMY_G5C3,
    MIU_CLIENT_DUMMY_G5C4,
    MIU_CLIENT_DUMMY_G5C5,
    MIU_CLIENT_DUMMY_G5C6,
    MIU_CLIENT_DUMMY_G5C7,
    MIU_CLIENT_DUMMY_G5C8,
    MIU_CLIENT_DUMMY_G5C9,
    MIU_CLIENT_DUMMY_G5CA,
    MIU_CLIENT_DUMMY_G5CB,
    MIU_CLIENT_DUMMY_G5CC,
    MIU_CLIENT_DUMMY_G5CD,
    MIU_CLIENT_DUMMY_G5CE,
    MIU_CLIENT_DUMMY_G5CF,
    // group 6
    MIU_CLIENT_DUMMY_G6C0,
    MIU_CLIENT_DUMMY_G6C1,
    MIU_CLIENT_DUMMY_G6C2,
    MIU_CLIENT_DUMMY_G6C3,
    MIU_CLIENT_DUMMY_G6C4,
    MIU_CLIENT_DUMMY_G6C5,
    MIU_CLIENT_DUMMY_G6C6,
    MIU_CLIENT_DUMMY_G6C7,
    MIU_CLIENT_DUMMY_G6C8,
    MIU_CLIENT_DUMMY_G6C9,
    MIU_CLIENT_DUMMY_G6CA,
    MIU_CLIENT_DUMMY_G6CB,
    MIU_CLIENT_DUMMY_G6CC,
    MIU_CLIENT_DUMMY_G6CD,
    MIU_CLIENT_DUMMY_G6CE,
    MIU_CLIENT_DUMMY_G6CF,
    // group 7
    MIU_CLIENT_CPU_W,       // CPU_W
    MIU_CLIENT_DLA_HIWAY_R, // DLA_HIWAY_R
    MIU_CLIENT_DLA_HIWAY_W, // DLA_HIWAY_W
    MIU_CLIENT_CPU_R,       // CPU_R
    MIU_CLIENT_DUMMY_G7C4,
    MIU_CLIENT_DUMMY_G7C5,
    MIU_CLIENT_DUMMY_G7C6,
    MIU_CLIENT_DUMMY_G7C7,
    MIU_CLIENT_DUMMY_G7C8,
    MIU_CLIENT_DUMMY_G7C9,
    MIU_CLIENT_DUMMY_G7CA,
    MIU_CLIENT_DUMMY_G7CB,
    MIU_CLIENT_DUMMY_G7CC,
    MIU_CLIENT_DUMMY_G7CD,
    MIU_CLIENT_DUMMY_G7CE,
    MIU_CLIENT_DUMMY_G7CF,
} eMIUClientID;

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

#define CONFIG_MIU_PROTECT_SUPPORT_INT
#define CONFIG_MIU_HW_MMU
#ifdef CONFIG_MIU_HW_MMU
#define CONFIG_MMU_INTERRUPT_ENABLE

// MDrv_MMU_Callback parameter: [IRQ Status] [Phyical Address Entry] [Client ID] [Is Write Command]
typedef void (*MDrv_MMU_Callback)(unsigned int, unsigned short, unsigned short, unsigned char);
#endif

unsigned char   MDrv_MIU_Init(void);
unsigned short *MDrv_MIU_GetDefaultClientID_KernelProtect(void);
unsigned short *MDrv_MIU_GetClientID_KernelProtect(unsigned char u8MiuSel);
unsigned short *MDrv_MMU_GetClientID_KernelProtect(unsigned char u8MiuSel);
unsigned char   MDrv_MIU_Protect(unsigned char u8Blockx, unsigned short *pu8ProtectId, phy_addr u64BusStart,
                                 phy_addr u64BusEnd, unsigned char bSetFlag);
unsigned char   MDrv_MIU_SetAccessFromVpaOnly(unsigned char u8Blockx, phy_addr u64BusStart, phy_addr u64BusEnd,
                                              unsigned char bSetFlag);
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
unsigned char MDrv_MIU_GetProtectInfo(unsigned char u8MiuDev, MIU_PortectInfo *pInfo);
#endif
unsigned char MDrv_MIU_Slits(unsigned char u8Blockx, phy_addr u64SlitsStart, phy_addr u65SlitsEnd,
                             unsigned char bSetFlag);
unsigned char MDrv_MIU_Get_IDEnables_Value(unsigned char u8MiuDev, unsigned char u8Blockx, unsigned char u8ClientIndex);
unsigned int  MDrv_MIU_ProtectDramSize(void);
int           MDrv_MIU_ClientIdToName(unsigned short clientId, char *clientName);
int           MDrv_MIU_RClientIdToName(unsigned short clientId, char *clientName);
int           MDrv_MIU_Info(MIU_DramInfo *pDramInfo);
phy_addr      MDrv_MIU_GetMmuBaseAddr(void);
int           clientId_KernelProtectToName(unsigned short clientId, char *clientName);

#ifdef CONFIG_MIU_HW_MMU
int            MDrv_MMU_SetPageSize(unsigned char u8PgszMode);
int            MDrv_MMU_SetRegion(unsigned short u16Region, unsigned short u16ReplaceRegion);
int            MDrv_MMU_Map(unsigned short u16VirtAddrEntry, unsigned short u16PhyAddrEntry);
unsigned short MDrv_MMU_MapQuery(unsigned short u16VirtAddrEntry);
int            MDrv_MMU_UnMap(unsigned short u16VirtAddrEntry);

#ifdef CONFIG_MMU_INTERRUPT_ENABLE
void MDrv_MMU_CallbackFunc(MDrv_MMU_Callback pFuncPtr);
#endif
int          MDrv_MMU_Enable(unsigned char u8Enable);
int          MDrv_MMU_Reset(void);
unsigned int MDrv_MMU_Status(unsigned short *u16PhyAddrEntry, unsigned short *u16ClientId, unsigned char *u8IsWriteCmd);
#endif
unsigned char MDrv_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd);

#endif // __MDRV_MIU_H__
