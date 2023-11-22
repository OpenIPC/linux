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
#define MMU_ADDR_TO_REGION(addr)     ((addr >> 27) & 0x1F) // 32
#define MMU_ADDR_TO_REGION_64(addr)  ((addr >> 28) & 0xF)
#define MMU_ADDR_TO_REGION_128(addr) ((addr >> 29) & 0x7)

#define MMU_ADDR_TO_ENTRY(addr)     ((addr >> 15) & 0xFFF) // 32
#define MMU_ADDR_TO_ENTRY_64(addr)  ((addr >> 16) & 0xFFF)
#define MMU_ADDR_TO_ENTRY_128(addr) ((addr >> 17) & 0xFFF) // 4096 entry

#define MMU_PAGE_SIZE     (0x8000)  // 32KB
#define MMU_PAGE_SIZE_64  (0x10000) // 64KB
#define MMU_PAGE_SIZE_128 (0x20000) // 128KB

//-------------------------------------------------------------------------------------------------
//  Enumeration Define
//-------------------------------------------------------------------------------------------------

typedef enum
{
    // group 0
    MIU_CLIENTW_DUMMY_G0C0,
    MIU_CLIENTW_DUMMY_G0C1,
    MIU_CLIENTW_DUMMY_G0C2,
    MIU_CLIENTW_BACH,
    MIU_CLIENTW_DUMMY_G0C4,
    MIU_CLIENTW_BDMA,
    MIU_CLIENTW_BDMA2,
    MIU_CLIENTW_JPE0,
    MIU_CLIENTW_LS_ARB1,
    MIU_CLIENTW_SD,
    MIU_CLIENTW_FCIE,
    MIU_CLIENTW_USB20_UHC0,
    MIU_CLIENTW_VENC0,
    MIU_CLIENTW_LS_ARB0,
    MIU_CLIENTW_DUMMY_G0CE,
    MIU_CLIENTW_G0BIST_G0CF,
    // group 1
    MIU_CLIENTW_SC_WDMA3,
    MIU_CLIENTW_VENC0_1,
    MIU_CLIENTW_DUMMY_G1C2,
    MIU_CLIENTW_DUMMY_G1C3,
    MIU_CLIENTW_AESDMA,
    MIU_CLIENTW_DUMMY_G1C5,
    MIU_CLIENTW_SC_WDMA0,
    MIU_CLIENTW_DUMMY_G1C7,
    MIU_CLIENTW_DUMMY_G1C8,
    MIU_CLIENTW_SC_WDMA1,
    MIU_CLIENTW_SC_WDMA2,
    MIU_CLIENTW_DUMMY_G1CB,
    MIU_CLIENTW_IVE,
    MIU_CLIENTW_EMAC,
    MIU_CLIENTW_DUMMY_G1CE,
    MIU_CLIENTW_G1BIST_G1CF,
    // group 2
    MIU_CLIENTW_ISP_3DNR,
    MIU_CLIENTW_ISP_WDR,
    MIU_CLIENTW_ISP_DMAG,
    MIU_CLIENTW_DUMMY_G2C3,
    MIU_CLIENTW_ISP_IIR,
    MIU_CLIENTW_ISP_ROT,
    MIU_CLIENTW_DUMMY_G2C6,
    MIU_CLIENTW_DUMMY_G2C7,
    MIU_CLIENTW_ISP_IMG,
    MIU_CLIENTW_ISM_DMAG,
    MIU_CLIENTW_ISP_STA,
    MIU_CLIENTW_DUMMY_G2CB,
    MIU_CLIENTW_DUMMY_G2CC,
    MIU_CLIENTW_DUMMY_G2CD,
    MIU_CLIENTW_DUMMY_G2CE,
    MIU_CLIENTW_G2BIST_G2CF,

    // HIGH WAY
    MIU_CLIENTW_CA35    = 0x70,
    MIU_CLIENTW_IPU     = 0x71,
    MIU_CLIENTW_TOPBIST = 0x3F,
} eMIUClientID_WCMD;

typedef enum
{
    // group 0
    MIU_CLIENTR_DUMMY_G0C0,
    MIU_CLIENTR_CMDQ1,
    MIU_CLIENTR_DUMMY_G0C2,
    MIU_CLIENTR_BACH,
    MIU_CLIENTR_CMDQ,
    MIU_CLIENTR_BDMA,
    MIU_CLIENTR_BDMA2,
    MIU_CLIENTR_JPE0,
    MIU_CLIENTR_LS_ARB1,
    MIU_CLIENTR_SD,
    MIU_CLIENTR_FCIE,
    MIU_CLIENTR_USB20_UHC0,
    MIU_CLIENTR_VENC0,
    MIU_CLIENTR_LOW_SPEED0,
    MIU_CLIENTR_DUMMY_G0CE,
    MIU_CLIENTR_DUMMY_G0CF,
    // group 1
    MIU_CLIENTR_DUMMY_G1C0,
    MIU_CLIENTR_VENC1_1,
    MIU_CLIENTR_VENC1_2,
    MIU_CLIENTR_CMDQ_ISP0,
    MIU_CLIENTR_AESDMA,
    MIU_CLIENTR_DUMMY_G1C5,
    MIU_CLIENTR_DUMMY_G1C6,
    MIU_CLIENTR_SC_RDMA0,
    MIU_CLIENTR_SC_RDMA1,
    MIU_CLIENTR_DUMMY_G1C9,
    MIU_CLIENTR_DUMMY_G1CA,
    MIU_CLIENTR_JPE0_GOP,
    MIU_CLIENTR_IVE,
    MIU_CLIENTR_EMAC,
    MIU_CLIENTR_GOP_SC,
    MIU_CLIENTR_DUMMY_G1CF,

    // HIGH WAY
    MIU_CLIENTR_CA35    = 0x70,
    MIU_CLIENTR_IPU     = 0x71,
    MIU_CLIENTR_TOPBIST = 0x3F,

} eMIUClientID_RCMD;

#define MMU_ENTRY (0xC) // 4096

typedef enum
{
    E_LEVEL_0 = 0, // off
    E_LEVEL_1 = 1, // 0x1
    E_LEVEL_2 = 2, // 0x10
    E_LEVEL_3 = 3, // 0x20
    E_LEVEL_4 = 4, // 0x40
    E_LEVEL_5 = 5, // 0x80
    E_LEVEL_6 = 6, // 0xF0
    E_LEVEL_7,     // 0xFF
} MIU_MASK_PERIOD;

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
    E_MMU_PGSZ_32 = 0,
    E_MMU_PGSZ_64 = 1,
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
    unsigned char  u8RW;
    ss_phys_addr_t uAddress;
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
void            MDrv_MIU_SetBW(unsigned short level);

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
#if defined CONFIG_MIU_SUPPORT_SYNOPSYS || defined CONFIG_MIU_RWCLIENT
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

#endif // __MDRV_MIU_H__
