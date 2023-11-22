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
#include "ms_platform.h"

#ifndef __MDRV_MIU_H__
#define __MDRV_MIU_H__
#include <linux/types.h>
#include <linux/device.h>
#ifdef CONFIG_64BIT
typedef unsigned long long phy_addr;  // 4 bytes
typedef unsigned long long virt_addr; // 4 bytes
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
#define MMU_ADDR_TO_REGION_128(addr)  ((addr >> 29) & 0x7FUL)
#define MMU_ADDR_TO_REGION_256(addr)  ((addr >> 30) & 0x3FUL)
#define MMU_ADDR_TO_REGION_512(addr)  ((addr >> 31) & 0x1FUL)
#define MMU_ADDR_TO_REGION_1024(addr) ((addr >> 32) & 0x0FUL)

#define MMU_ADDR_TO_ENTRY_128(addr)  ((addr >> 17) & 0xFFFUL)
#define MMU_ADDR_TO_ENTRY_256(addr)  ((addr >> 18) & 0xFFFUL)
#define MMU_ADDR_TO_ENTRY_512(addr)  ((addr >> 19) & 0xFFFUL)
#define MMU_ADDR_TO_ENTRY_1024(addr) ((addr >> 20) & 0xFFFUL)

#define MMU_PAGE_SIZE_128  (0x20000UL)  // 128KB
#define MMU_PAGE_SIZE_256  (0x40000UL)  // 256KB
#define MMU_PAGE_SIZE_512  (0x80000UL)  // 512KB
#define MMU_PAGE_SIZE_1024 (0x100000UL) // 1MB

//-------------------------------------------------------------------------------------------------
//  Enumeration Define
//-------------------------------------------------------------------------------------------------

typedef enum
{
    // group 0
    MIU_CLIENTW_DUMMY_G0C0,
    MIU_CLIENTW_BDMA2,
    MIU_CLIENTW_BDMA3,
    MIU_CLIENTW_BDMA4,
    MIU_CLIENTW_BDMA5,
    MIU_CLIENTW_LS_ARB0,
    MIU_CLIENTW_LS_ARB1,
    MIU_CLIENTW_DUMMY_G0C7,
    MIU_CLIENTW_BDMA,
    MIU_CLIENTW_VENC1,
    MIU_CLIENTW_DUMMY_G0CA,
    MIU_CLIENTW_VENC1_SAD,
    MIU_CLIENTW_VDEC0,
    MIU_CLIENTW_DUMMY_G0CD,
    MIU_CLIENTW_DUMMY_G0CE,
    MIU_CLIENTW_DUMMY_G0CF,
    // group 1
    MIU_CLIENTW_PCIE0,
    MIU_CLIENTW_SATA1,
    MIU_CLIENTW_SATA2,
    MIU_CLIENTW_DUMMY_G1C3,
    MIU_CLIENTW_DUMMY_G1C4,
    MIU_CLIENTW_DUMMY_G1C5,
    MIU_CLIENTW_DUMMY_G1C6,
    MIU_CLIENTW_DUMMY_G1C7,
    MIU_CLIENTW_DUMMY_G1C8,
    MIU_CLIENTW_DUMMY_G1C9,
    MIU_CLIENTW_DUMMY_G1CA,
    MIU_CLIENTW_DUMMY_G1CB,
    MIU_CLIENTW_DUMMY_G1CC,
    MIU_CLIENTW_DUMMY_G1CD,
    MIU_CLIENTW_DUMMY_G1CE,
    MIU_CLIENTW_DUMMY_G1CF,
    // group 2
    MIU_CLIENTW_SC_WDMA0,
    MIU_CLIENTW_SC_WDMA1,
    MIU_CLIENTW_SC_WDMA2,
    MIU_CLIENTW_SC_WDMA3,
    MIU_CLIENTW_SC_WDMA4,
    MIU_CLIENTW_SC_WDMA5,
    MIU_CLIENTW_SC_WDMA6,
    MIU_CLIENTW_SC_WDMA7,
    MIU_CLIENTW_SC_WDMA8,
    MIU_CLIENTW_SD,
    MIU_CLIENTW_SDIO,
    MIU_CLIENTW_SDIO2,
    MIU_CLIENTW_SDIO3,
    MIU_CLIENTW_DIP_ROT,
    MIU_CLIENTW_DUMMY_G2CE,
    MIU_CLIENTW_DUMMY_G2CF,
    // group 3
    MIU_CLIENTW_AESDMA,
    MIU_CLIENTW_BACH,
    MIU_CLIENTW_BACH1,
    MIU_CLIENTW_BACH2,
    MIU_CLIENTW_BACH3,
    MIU_CLIENTW_BACH4,
    MIU_CLIENTW_DUMMY_G3C6,
    MIU_CLIENTW_DUMMY_G3C7,
    MIU_CLIENTW_DUMMY_G3C8,
    MIU_CLIENTW_JPE0,
    MIU_CLIENTW_JPE1,
    MIU_CLIENTW_LDC0,
    MIU_CLIENTW_LDC1,
    MIU_CLIENTW_USB20_UHC0,
    MIU_CLIENTW_USB20_UHC1,
    MIU_CLIENTW_IVE,
    // group 4
    MIU_CLIENTW_ISP0_MOT0,
    MIU_CLIENTW_ISP0_STA,
    MIU_CLIENTW_ISP0_DMA0,
    MIU_CLIENTW_ISP0_DMA1,
    MIU_CLIENTW_ISP0_DMA2,
    MIU_CLIENTW_ISP0_DMA3,
    MIU_CLIENTW_ISP0_DMA_EXT0,
    MIU_CLIENTW_ISP0_DMA_EXT1,
    MIU_CLIENTW_ISP0_3DNR,
    MIU_CLIENTW_ISP0_IMG,
    MIU_CLIENTW_ISP0_HDROUT,
    MIU_CLIENTW_ISP0_WDR,
    MIU_CLIENTW_ISP0_VIP,
    MIU_CLIENTW_ISP0_DMA_EXT2,
    MIU_CLIENTW_ITU0_WDMA,
    MIU_CLIENTW_DUMMY_G4CF,
    // group 5
    MIU_CLIENTW_ISP1_MOT0,
    MIU_CLIENTW_ISP1_STA,
    MIU_CLIENTW_ISP1_DMA0,
    MIU_CLIENTW_ISP1_DMA1,
    MIU_CLIENTW_ISP1_DMA2,
    MIU_CLIENTW_ISP1_DMA3,
    MIU_CLIENTW_ISP1_DMA_EXT0,
    MIU_CLIENTW_ISP1_DMA_EXT1,
    MIU_CLIENTW_ISP1_3DNR,
    MIU_CLIENTW_ISP1_IMG,
    MIU_CLIENTW_ISP1_HDROUT,
    MIU_CLIENTW_ISP1_WDR,
    MIU_CLIENTW_ISP1_VIP,
    MIU_CLIENTW_ISP1_DMA_EXT2,
    MIU_CLIENTW_ITU1_WDMA,
    MIU_CLIENTW_DUMMY_G5CF,
    // group 6
    MIU_CLIENTW_VENC0,
    MIU_CLIENTW_VENC0_SAD,
    MIU_CLIENTW_GMAC_TOE,
    MIU_CLIENTW_GE,
    MIU_CLIENTW_GE1,
    MIU_CLIENTW_DUMMY_G6C5,
    MIU_CLIENTW_JPD0,
    MIU_CLIENTW_JPD1,
    MIU_CLIENTW_JPD2,
    MIU_CLIENTW_JPD3,
    MIU_CLIENTW_PQ0_VIP_STAT,
    MIU_CLIENTW_PQ1_VIP_STAT,
    MIU_CLIENTW_DISP_CVBS,
    MIU_CLIENTW_GMAC_BDMA,
    MIU_CLIENTW_GMAC0,
    MIU_CLIENTW_GMAC1,
    // group 7
    MIU_CLIENTW_VDEC1,
    MIU_CLIENTW_USB30_DRD,
    MIU_CLIENTW_PCIE1,
    MIU_CLIENTW_DUMMY_G7C3,
    MIU_CLIENTW_DUMMY_G7C4,
    MIU_CLIENTW_DUMMY_G7C5,
    MIU_CLIENTW_DUMMY_G7C6,
    MIU_CLIENTW_DUMMY_G7C7,
    MIU_CLIENTW_DUMMY_G7C8,
    MIU_CLIENTW_DUMMY_G7C9,
    MIU_CLIENTW_DUMMY_G7CA,
    MIU_CLIENTW_DUMMY_G7CB,
    MIU_CLIENTW_DUMMY_G7CC,
    MIU_CLIENTW_DUMMY_G7CD,
    MIU_CLIENTW_DUMMY_G7CE,
    MIU_CLIENTW_DUMMY_G7CF,

    // HIGH WAY
    MIU_CLIENTW_CA55 = 0x80,
    MIU_CLIENTW_CA7  = 0x90,
    MIU_CLIENTW_DSP0 = 0xA0,
    MIU_CLIENTW_DSP1 = 0xB0,
    MIU_CLIENTW_DUMMY_G8C4,
    MIU_CLIENTW_DUMMY_G8C5,
    MIU_CLIENTW_IPU0 = 0xE0,
    MIU_CLIENTW_IPU1 = 0xF0,
    MIU_CLIENTW_DUMMY_G8C8,
    MIU_CLIENTW_DUMMY_G8C9,
    MIU_CLIENTW_DUMMY_G8CA,
    MIU_CLIENTW_DUMMY_G8CB,
    MIU_CLIENTW_DUMMY_G8CC,
    MIU_CLIENTW_DUMMY_G8CD,
    MIU_CLIENTW_DUMMY_G8CE,
    MIU_CLIENTW_DUMMY_G8CF,
} eMIUClientID_WRITE;

typedef enum
{
    // group 0
    MIU_CLIENTR_DUMMY_G0C0,
    MIU_CLIENTR_BDMA2,
    MIU_CLIENTR_BDMA3,
    MIU_CLIENTR_BDMA4,
    MIU_CLIENTR_BDMA5,
    MIU_CLIENTR_LS_ARB0,
    MIU_CLIENTR_LS_ARB1,
    MIU_CLIENTR_CMDQ_VEN,
    MIU_CLIENTR_BDMA,
    MIU_CLIENTR_VENC1,
    MIU_CLIENTR_VEN1_GOP,
    MIU_CLIENTR_VEN1_SAD,
    MIU_CLIENTR_VDEC0,
    MIU_CLIENTR_DUMMY_G0CD,
    MIU_CLIENTR_DUMMY_G0CE,
    MIU_CLIENTR_DUMMY_G0CF,
    // group 1
    MIU_CLIENTR_DUMMY_G1C0,
    MIU_CLIENTR_DUMMY_G1C1,
    MIU_CLIENTR_DUMMY_G1C2,
    MIU_CLIENTR_DUMMY_G1C3,
    MIU_CLIENTR_DUMMY_G1C4,
    MIU_CLIENTR_DUMMY_G1C5,
    MIU_CLIENTR_DUMMY_G1C6,
    MIU_CLIENTR_DUMMY_G1C7,
    MIU_CLIENTR_DUMMY_G1C8,
    MIU_CLIENTR_DUMMY_G1C9,
    MIU_CLIENTR_DUMMY_G1CA,
    MIU_CLIENTR_DUMMY_G1CB,
    MIU_CLIENTR_DUMMY_G1CC,
    MIU_CLIENTR_DUMMY_G1CD,
    MIU_CLIENTR_DUMMY_G1CE,
    MIU_CLIENTR_DUMMY_G1CF,
    // group 2
    MIU_CLIENTR_CDMQ,
    MIU_CLIENTR_CDMQ1,
    MIU_CLIENTR_CDMQ_DIG,
    MIU_CLIENTR_CDMQ_CMD,
    MIU_CLIENTR_CDMQ_CMD1,
    MIU_CLIENTR_CDMQ_ISP0,
    MIU_CLIENTR_CMDQ_ISP1,
    MIU_CLIENTR_RMAP,
    MIU_CLIENTR_CSI_TX,
    MIU_CLIENTR_SD,
    MIU_CLIENTR_SDIO,
    MIU_CLIENTR_SDIO2,
    MIU_CLIENTR_SDIO3,
    MIU_CLIENTR_DIP_ROT,
    MIU_CLIENTR_SC_GOP0,
    MIU_CLIENTR_SC_GOP1,
    // group 3
    MIU_CLIENTR_AESDMA,
    MIU_CLIENTR_BACH,
    MIU_CLIENTR_BACH1,
    MIU_CLIENTR_BACH2,
    MIU_CLIENTR_GOP_JPE0,
    MIU_CLIENTR_GOP_JPE1,
    MIU_CLIENTR_SC_RDMA0,
    MIU_CLIENTR_SC_RDMA1,
    MIU_CLIENTR_SC_RDMA2,
    MIU_CLIENTR_JPE0,
    MIU_CLIENTR_JPE1,
    MIU_CLIENTR_LDC0,
    MIU_CLIENTR_LDC1,
    MIU_CLIENTR_USB20_UHC0,
    MIU_CLIENTR_USB20_UHC1,
    MIU_CLIENTR_IVE,
    // group 4
    MIU_CLIENTR_ISP0_ROT,
    MIU_CLIENTR_ISP0_MOT,
    MIU_CLIENTR_ISP0_DMAG0,
    MIU_CLIENTR_ISP0_3DNR,
    MIU_CLIENTR_ISP0_MLOAD,
    MIU_CLIENTR_ISP0_DMAG1,
    MIU_CLIENTR_ISP0_DMAG_EXT0,
    MIU_CLIENTR_ISP0_DMAG_EXT1,
    MIU_CLIENTR_ISP0_WDR,
    MIU_CLIENTR_ISP0_DMAG_EXT2,
    MIU_CLIENTR_PCIE0,
    MIU_CLIENTR_SATA0,
    MIU_CLIENTR_SATA1,
    MIU_CLIENTR_DUMMY_G4CD,
    MIU_CLIENTR_DUMMY_G4CE,
    MIU_CLIENTR_DUMMY_G4CF,
    // group 5
    MIU_CLIENTR_ISP1_ROT,
    MIU_CLIENTR_ISP1_MOT,
    MIU_CLIENTR_ISP1_DMAG0,
    MIU_CLIENTR_ISP1_3DNR,
    MIU_CLIENTR_ISP1_MLOAD,
    MIU_CLIENTR_ISP1_DMAG1,
    MIU_CLIENTR_ISP1_DMAG_EXT0,
    MIU_CLIENTR_ISP1_DMAG_EXT1,
    MIU_CLIENTR_ISP1_WDR,
    MIU_CLIENTR_ISP1_DMAG_EXT2,
    MIU_CLIENTR_VDEC1,
    MIU_CLIENTR_USB30_DRD,
    MIU_CLIENTR_PCIE1,
    MIU_CLIENTR_DUMMY_G5CD,
    MIU_CLIENTR_DUMMY_G5CE,
    MIU_CLIENTR_DUMMY_G5CF,
    // group 6
    MIU_CLIENTR_MOPROT0_Y,
    MIU_CLIENTR_MOPROT0_C,
    MIU_CLIENTR_MOPROT1_Y,
    MIU_CLIENTR_MOPROT1_C,
    MIU_CLIENTR_MOPROT2_Y,
    MIU_CLIENTR_MOPROT2_C,
    MIU_CLIENTR_JPD0,
    MIU_CLIENTR_JPD1,
    MIU_CLIENTR_JPD2,
    MIU_CLIENTR_JPD3,
    MIU_CLIENTR_VENC0,
    MIU_CLIENTR_VEN0_GOP,
    MIU_CLIENTR_VEN0_SAD,
    MIU_CLIENTR_GMAC0,
    MIU_CLIENTR_GMAC_TOE,
    MIU_CLIENTR_GE,
    // group 7
    MIU_CLIENTR_GOP_DISP0,
    MIU_CLIENTR_GOP_DISP1,
    MIU_CLIENTR_GOP_DISP2,
    MIU_CLIENTR_GOP_DISP3,
    MIU_CLIENTR_GOP_DISP4,
    MIU_CLIENTR_GOP_DISP5,
    MIU_CLIENTR_GOP_DISP6,
    MIU_CLIENTR_GOP_DISP7,
    MIU_CLIENTR_GOP_DISP8,
    MIU_CLIENTR_GMAC1,
    MIU_CLIENTR_GE1,
    MIU_CLIENTR_GMAC_BDMA,
    MIU_CLIENTR_DUMMY_G7CC,
    MIU_CLIENTR_DUMMY_G7CD,
    MIU_CLIENTR_DUMMY_G7CE,
    MIU_CLIENTR_DUMMY_G7CF,
    // HIGH WAY
    MIU_CLIENTR_CA55 = 0x80,
    MIU_CLIENTR_CA7  = 0x90,
    MIU_CLIENTR_DSP0 = 0xA0,
    MIU_CLIENTR_DSP1 = 0xB0,
    MIU_CLIENTR_DUMMY_G8C4,
    MIU_CLIENTR_DUMMY_G8C5,
    MIU_CLIENTR_IPU0 = 0xE0,
    MIU_CLIENTR_IPU1 = 0xF0,
    MIU_CLIENTR_DUMMY_G8C8,
    MIU_CLIENTR_DUMMY_G8C9,
    MIU_CLIENTR_DUMMY_G8CA,
    MIU_CLIENTR_DUMMY_G8CB,
    MIU_CLIENTR_DUMMY_G8CC,
    MIU_CLIENTR_DUMMY_G8CD,
    MIU_CLIENTR_DUMMY_G8CE,
    MIU_CLIENTR_DUMMY_G8CF,
} eMIUClientID_READ;

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
    E_MMU_PGSZ_1024,
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
    unsigned char  bHit;
    unsigned char  u8Group;
    unsigned char  u8ClientID;
    unsigned char  u8Block;
    ss_phys_addr_t uAddress;
} MIU_PortectInfo;

typedef struct
{
    unsigned long long size;        // bytes
    unsigned int       dram_freq;   // MHz
    unsigned int       miupll_freq; // MHz
    unsigned char      type;        // 0:LPDDR4, 1:DDR4
    unsigned char      data_rate;   // 4:4x mode, 8:8x mode,
    unsigned char      bus_width;   // 32:32bit, 64:64bit
    unsigned char      ssc;         // 0:off, 1:on
} MIU_DramInfo;

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
unsigned char   MDrv_MIU_Protect(unsigned char u8Blockx, unsigned short *pu8ProtectId, ss_phys_addr_t u64BusStart,
                                 ss_phys_addr_t u64BusEnd, unsigned char bSetFlag);
unsigned char   MDrv_MIU_SetAccessFromVpaOnly(unsigned char u8Blockx, ss_phys_addr_t u64BusStart,
                                              ss_phys_addr_t u64BusEnd, unsigned char bSetFlag);
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
void          MDrv_MIU_Protect_PanicOnOff(unsigned char u8On);
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
#ifdef CONFIG_MIU_SUPPORT_SYNOPSYS
int rcmd_clientId_KernelProtectToName(unsigned short clientId, char *clientName);
#endif

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
