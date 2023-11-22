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
#define MMU_ADDR_TO_REGION(addr)     ((addr >> 29) & 0x7F)
#define MMU_ADDR_TO_REGION_64(addr)  ((addr >> 29) & 0x7F)
#define MMU_ADDR_TO_REGION_128(addr) ((addr >> 30) & 0x3F)

#define MMU_ADDR_TO_ENTRY(addr)     ((addr >> 16) & 0x1FFF)
#define MMU_ADDR_TO_ENTRY_64(addr)  ((addr >> 16) & 0x1FFF)
#define MMU_ADDR_TO_ENTRY_128(addr) ((addr >> 17) & 0x1FFF)

#define MMU_PAGE_SIZE     (0x10000) // 64KB
#define MMU_PAGE_SIZE_64  (0x10000) // 64KB
#define MMU_PAGE_SIZE_128 (0x20000) // 128KB

//-------------------------------------------------------------------------------------------------
//  Enumeration Define
//-------------------------------------------------------------------------------------------------

typedef enum
{
    // group 0
    MIU_CLIENTW_DUMMY_G0C0,
    MIU_CLIENTW_SC_WDMA1,
    MIU_CLIENTW_SC_WDMA2,
    MIU_CLIENTW_BACH,
    MIU_CLIENTW_BACH2,
    MIU_CLIENTW_BDMA,
    MIU_CLIENTW_BDMA2,
    MIU_CLIENTW_JPE0,
    MIU_CLIENTW_LS_ARB1,
    MIU_CLIENTW_SD,
    MIU_CLIENTW_FCIE,
    MIU_CLIENTW_USB20_UHC0,
    MIU_CLIENTW_DUMMY_G0CC,
    MIU_CLIENTW_SC_WDMA0,
    MIU_CLIENTW_DUMMY_G0CE,
    MIU_CLIENTW_DUMMY_G0CF,
    // group 1
    MIU_CLIENTW_SC_WDMA3,
    MIU_CLIENTW_SC_WDMA4,
    MIU_CLIENTW_SC_WDMA5,
    MIU_CLIENTW_SC_WDMA6,
    MIU_CLIENTW_AESDMA,
    MIU_CLIENTW_BACH1,
    MIU_CLIENTW_LS_ARB0,
    MIU_CLIENTW_DUMMY_G1C7,
    MIU_CLIENTW_BDMA_XOR,
    MIU_CLIENTW_DUMMY_G1C9,
    MIU_CLIENTW_JPE1,
    MIU_CLIENTW_LDC0,
    MIU_CLIENTW_IVE,
    MIU_CLIENTW_DUMMY_G1CD,
    MIU_CLIENTW_USB20_UHC1,
    MIU_CLIENTW_DUMMY_G1CF,
    // group 2
    MIU_CLIENTW_DUMMY_G2C0,
    MIU_CLIENTW_DUMMY_G2C1,
    MIU_CLIENTW_DUMMY_G2C2,
    MIU_CLIENTW_DUMMY_G2C3,
    MIU_CLIENTW_DUMMY_G2C4,
    MIU_CLIENTW_DUMMY_G2C5,
    MIU_CLIENTW_DUMMY_G2C6,
    MIU_CLIENTW_DUMMY_G2C7,
    MIU_CLIENTW_DUMMY_G2C8,
    MIU_CLIENTW_DUMMY_G2C9,
    MIU_CLIENTW_DUMMY_G2CA,
    MIU_CLIENTW_DUMMY_G2CB,
    MIU_CLIENTW_DUMMY_G2CC,
    MIU_CLIENTW_DUMMY_G2CD,
    MIU_CLIENTW_DUMMY_G2CE,
    MIU_CLIENTW_DUMMY_G2CF,
    // group 3
    MIU_CLIENTW_GMAC0,
    MIU_CLIENTW_GMAC1,
    MIU_CLIENTW_GMAC_TOE0,
    MIU_CLIENTW_GMAC_TOE1,
    MIU_CLIENTW_GMAC_TOE2,
    MIU_CLIENTW_GMAC_TOE3,
    MIU_CLIENTW_GMAC_TOE4,
    MIU_CLIENTW_USB30_DRD,
    MIU_CLIENTW_VDEC1,
    MIU_CLIENTW_VDEC0,
    MIU_CLIENTW_GMAC_TOE5,
    MIU_CLIENTW_GMAC_TOE6,
    MIU_CLIENTW_GMAC_TOE7,
    MIU_CLIENTW_SGDMA,
    MIU_CLIENTW_BDMA_NET,
    MIU_CLIENTW_DUMMY_G3CF,
    // group 4
    MIU_CLIENTW_DUMMY_G4C0,
    MIU_CLIENTW_DUMMY_G4C1,
    MIU_CLIENTW_DUMMY_G4C2,
    MIU_CLIENTW_DUMMY_G4C3,
    MIU_CLIENTW_DUMMY_G4C4,
    MIU_CLIENTW_DUMMY_G4C5,
    MIU_CLIENTW_JPDW0,
    MIU_CLIENTW_JPDW1,
    MIU_CLIENTW_DUMMY_G4C8,
    MIU_CLIENTW_DUMMY_G4C9,
    MIU_CLIENTW_PQ0_VIP_STAT,
    MIU_CLIENTW_VENC0,
    MIU_CLIENTW_DISP_CVBS,
    MIU_CLIENTW_VENC0_SAD1,
    MIU_CLIENTW_GE,
    MIU_CLIENTW_DUMMY_G4CF,
    // group 5
    MIU_CLIENTW_ISP_3DNR0,
    MIU_CLIENTW_ISP_IMG,
    MIU_CLIENTW_ISP_DMA0,
    MIU_CLIENTW_ISP_DMA1,
    MIU_CLIENTW_ISP_DMA2,
    MIU_CLIENTW_ISP_DMA3,
    MIU_CLIENTW_DUMMY_G5C6,
    MIU_CLIENTW_ISP_WDR,
    MIU_CLIENTW_DUMMY_G5C8,
    MIU_CLIENTW_DUMMY_G5C9,
    MIU_CLIENTW_DUMMY_G5CA,
    MIU_CLIENTW_DUMMY_G5CB,
    MIU_CLIENTW_DUMMY_G5CC,
    MIU_CLIENTW_SATA1,
    MIU_CLIENTW_SATA2,
    MIU_CLIENTW_DUMMY_G5CF,

    MIU_CLIENTW_BIST256 = 0xF0,
    // HIGH WAY
    MIU_CLIENTW_IPU  = 0x100,
    MIU_CLIENTW_CA55 = 0x101,
} eMIUClientID_WCMD;

typedef enum
{
    // group 0
    MIU_CLIENTR_DUMMY_G0C0,
    MIU_CLIENTR_CMDQ1,
    MIU_CLIENTR_JPE0_GOP,
    MIU_CLIENTR_BACH,
    MIU_CLIENTR_CMDQ,
    MIU_CLIENTR_BDMA,
    MIU_CLIENTR_BDMA2,
    MIU_CLIENTR_JPE0,
    MIU_CLIENTR_LS_ARB1,
    MIU_CLIENTR_SD,
    MIU_CLIENTR_FCIE,
    MIU_CLIENTR_USB20_UHC0,
    MIU_CLIENTR_DUMMY_G0CC,
    MIU_CLIENTR_DUMMY_G0CD,
    MIU_CLIENTR_SC_GOP,
    MIU_CLIENTR_DUMMY_G0CF,
    // group 1
    MIU_CLIENTR_CMDQ_CMD,
    MIU_CLIENTR_CMDQ_CMD1,
    MIU_CLIENTR_CMDQ_ISP0,
    MIU_CLIENTR_RMAP,
    MIU_CLIENTR_AESDMA,
    MIU_CLIENTR_BACH1,
    MIU_CLIENTR_LS_ARB0,
    MIU_CLIENTR_SC_RDMA0,
    MIU_CLIENTR_BDMA_XOR,
    MIU_CLIENTR_JPE1_GOP,
    MIU_CLIENTR_JPE1,
    MIU_CLIENTR_LDC0,
    MIU_CLIENTR_IVE,
    MIU_CLIENTR_DUMMY_G1CD,
    MIU_CLIENTR_USB20_UHC1,
    MIU_CLIENTR_DUMMY_G1CF,
    // group 2
    MIU_CLIENTR_DUMMY_G2C0,
    MIU_CLIENTR_DUMMY_G2C1,
    MIU_CLIENTR_DUMMY_G2C2,
    MIU_CLIENTR_DUMMY_G2C3,
    MIU_CLIENTR_DUMMY_G2C4,
    MIU_CLIENTR_DUMMY_G2C5,
    MIU_CLIENTR_DUMMY_G2C6,
    MIU_CLIENTR_DUMMY_G2C7,
    MIU_CLIENTR_DUMMY_G2C8,
    MIU_CLIENTR_DUMMY_G2C9,
    MIU_CLIENTR_DUMMY_G2CA,
    MIU_CLIENTR_DUMMY_G2CB,
    MIU_CLIENTR_DUMMY_G2CC,
    MIU_CLIENTR_DUMMY_G2CD,
    MIU_CLIENTR_DUMMY_G2CE,
    MIU_CLIENTR_DUMMY_G2CF,
    // group 3
    MIU_CLIENTR_GMAC0,
    MIU_CLIENTR_GMAC1,
    MIU_CLIENTR_GMAC_TOE0,
    MIU_CLIENTR_GMAC_TOE1,
    MIU_CLIENTR_GMAC_TOE2,
    MIU_CLIENTR_GMAC_TOE3,
    MIU_CLIENTR_GMAC_TOE4,
    MIU_CLIENTR_USB30_DRD,
    MIU_CLIENTR_VDEC1,
    MIU_CLIENTR_VDEC0,
    MIU_CLIENTR_GMAC_TOE5,
    MIU_CLIENTR_GMAC_TOE6,
    MIU_CLIENTR_GMAC_TOE7,
    MIU_CLIENTR_CMDQ_TOE,
    MIU_CLIENTR_BDMA_NET,
    MIU_CLIENTR_DUMMY_G3CF,
    // group 4
    MIU_CLIENTR_MOP_ROT0_Y,
    MIU_CLIENTR_MOP_ROT0_C,
    MIU_CLIENTR_MOP_ROT1_Y,
    MIU_CLIENTR_MOP_ROT1_C,
    MIU_CLIENTR_DISP0_GOP,
    MIU_CLIENTR_DISP1_GOP,
    MIU_CLIENTR_JPDR0,
    MIU_CLIENTR_DISP2_GOP,
    MIU_CLIENTR_DISP3_GOP,
    MIU_CLIENTR_DISP4_GOP,
    MIU_CLIENTR_DISP5_GOP,
    MIU_CLIENTR_VENC0,
    MIU_CLIENTR_JPDR1,
    MIU_CLIENTR_VENC0_SAD1,
    MIU_CLIENTR_GE,
    MIU_CLIENTR_VENC0_GOP,
    // group 5
    MIU_CLIENTR_ISP_3DNR0,
    MIU_CLIENTR_ISP_WDR,
    MIU_CLIENTR_ISP_DMAG,
    MIU_CLIENTR_ISP_MLOAD,
    MIU_CLIENTR_DUMMY_G5C4,
    MIU_CLIENTR_DUMMY_G5C5,
    MIU_CLIENTR_DUMMY_G5C6,
    MIU_CLIENTR_DUMMY_G5C7,
    MIU_CLIENTR_DUMMY_G5C8,
    MIU_CLIENTR_DUMMY_G5C9,
    MIU_CLIENTR_DUMMY_G5CA,
    MIU_CLIENTR_DUMMY_G5CB,
    MIU_CLIENTR_DUMMY_G5CC,
    MIU_CLIENTR_SATA1,
    MIU_CLIENTR_SATA2,
    MIU_CLIENTR_DUMMY_G5CF,

    MIU_CLIENTR_BIST256 = 0xF0,
    // HIGH WAY
    MIU_CLIENTR_IPU  = 0x100,
    MIU_CLIENTR_CA55 = 0x101,
} eMIUClientID_RCMD;

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
    E_MMU_PGSZ_64 = 0,
    E_MMU_PGSZ_128,
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
    unsigned char      type;        // 0:DDR3, 1:DDR4
    unsigned char      data_rate;   // 4:4x mode, 8:8x mode,
    unsigned char      bus_width;   // 16:16bit, 32:32bit, 64:64bit
    unsigned char      ssc;         // 0:off, 1:on
} MIU_DramInfo;
//-------------------------------------------------------------------------------------------------
//  Enumeration Define
//-------------------------------------------------------------------------------------------------

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
