/*
 * mhal_miu.c- Sigmastar
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
#include <linux/printk.h>
#include <linux/delay.h>
#include "MsTypes.h"
#include "mdrv_types.h"
#include "mdrv_miu.h"
#include "regMIU.h"
#include "mhal_miu.h"
#include "mstar_chip.h"
#include "registers.h"
#include "ms_platform.h"
#ifdef CONFIG_CAM_CLK
#include "camclk.h"
#include "drv_camclk_Api.h"
#endif
#include "mhal_miu_client.h"

//-------------------------------------------------------------------------------------------------
//  Macro Define
//-------------------------------------------------------------------------------------------------
#define _phy_to_miu_offset(MiuSel, Offset, PhysAddr) \
    {                                                \
        MiuSel = E_MIU_0;                            \
        Offset = PhysAddr - ARM_MIU0_BASE_ADDR;      \
    }

#define MIU_HAL_ERR(fmt, args...) printk(KERN_ERR "miu hal error %s:%d" fmt, __FUNCTION__, __LINE__, ##args)

//-------------------------------------------------------------------------------------------------
//  Local Variable
//-------------------------------------------------------------------------------------------------
// maximum clientID to 32
static MS_U16 clientId_KernelProtect[IDNUM_KERNELPROTECT] = {
    MIU_CLIENTW_CA55,      MIU_CLIENTW_LS_ARB0,    MIU_CLIENTW_LS_ARB1,
    MIU_CLIENTW_USB30_DRD, MIU_CLIENTW_USB20_UHC0, MIU_CLIENTW_USB20_UHC1,
    MIU_CLIENTW_SD,        MIU_CLIENTW_AESDMA,     MIU_CLIENTW_BDMA,
    MIU_CLIENTW_BDMA2,     MIU_CLIENTW_BDMA_XOR,   MIU_CLIENTW_GMAC0,
    MIU_CLIENTW_GMAC1,     MIU_CLIENTW_GMAC_TOE0,  MIU_CLIENTW_GMAC_TOE1,
    MIU_CLIENTW_GMAC_TOE2, MIU_CLIENTW_GMAC_TOE3,  MIU_CLIENTW_GMAC_TOE4,
    MIU_CLIENTW_GMAC_TOE5, MIU_CLIENTW_GMAC_TOE6,  MIU_CLIENTW_GMAC_TOE7,
    MIU_CLIENTW_BDMA_NET,  MIU_CLIENTW_BIST256,    MIU_CLIENTW_FCIE,
    MIU_CLIENTW_SATA1,     MIU_CLIENTW_SATA2,      0,
};

#if defined(CONFIG_ARM) || defined(CONFIG_MIPS)
static MS_U32 m_u32MiuMapBase = 0xFD200000UL; // default set to arm 32bit platform
#elif defined(CONFIG_ARM64)
extern ptrdiff_t mstar_pm_base;
static ptrdiff_t m_u32MiuMapBase;
#endif

static MS_BOOL IDEnables[MIU_MAX_DEVICE][MIU_MAX_PROTECT_BLOCK][MIU_MAX_PROTECT_ID] = {
    {{0}, {0}, {0}, {0}}};                                            // ID enable for MIU protect block 0~3
static MS_U16  IDList[MIU_MAX_DEVICE][MIU_MAX_PROTECT_ID] = {{0x00}}; // IDList for MIU protection
static MS_BOOL MmuIDEnables[MIU_MAX_DEVICE][MMU_MAX_PROTECT_BLOCK][MIU_MAX_PROTECT_ID] = {
    {{0}, {0}, {0}, {0}}};                                              // ID enable for MMU protect block 0~3
static MS_U16 MmuIDList[MIU_MAX_DEVICE][MIU_MAX_PROTECT_ID] = {{0x00}}; // IDList for MMU protection
static MS_U8  m_u8PgszMode                                  = 0;
static MS_S16 s16RegionRec                                  = -1;
static MS_S16 s16RepRegionRec                               = -1;

struct miu_client miu0_wr_clients[] = {
    {"OVERALL_W      ", 0x00, 0},     {"SC_WDMA1_W     ", 0x01, 0},  {"SC_WDMA2_W     ", 0x02, 0},
    {"BACH_W         ", 0x03, 0},     {"BACH2_W        ", 0x04, 0},  {"BDMA_W         ", 0x05, 0},
    {"BDMA2_W        ", 0x06, 0},     {"JPE0_W         ", 0x07, 0},  {"LOW_SPEED1_W   ", 0x08, 0},
    {"SD_W           ", 0x09, 0},     {"FCIE_W         ", 0x0A, 0},  {"USB20_UHC0_W   ", 0x0B, 0},
    {"RSVD           ", 0x0C, 1},     {"SC_WDMA0_W     ", 0x0D, 0},  {"RSVD           ", 0x0E, 1},
    {"BIST_W         ", 0x0F, 0},     {"SC_WDMA3_W     ", 0x10, 0},  {"SC_WDMA4_W     ", 0x11, 0},
    {"SC_WDMA5_W     ", 0x12, 0},     {"SC_WDMA6_W     ", 0x13, 0},  {"AESDMA_W       ", 0x14, 0},
    {"BACH1_W        ", 0x15, 0},     {"LOW_SPEED0_W   ", 0x16, 0},  {"RSVD           ", 0x17, 1},
    {"BDMA_XOR_w     ", 0x18, 0},     {"RSVD           ", 0x19, 1},  {"JPE1_W         ", 0x1A, 0},
    {"LDC0_W         ", 0x1B, 0},     {"IVE_W          ", 0x1C, 0},  {"RSVD           ", 0x1D, 1},
    {"USB20_UHC1_W   ", 0x1E, 0},     {"BIST_W         ", 0x1F, 0},  {"RSVD           ", 0x20, 1},
    {"RSVD           ", 0x21, 1},     {"RSVD           ", 0x22, 1},  {"RSVD           ", 0x23, 1},
    {"RSVD           ", 0x24, 1},     {"RSVD           ", 0x25, 1},  {"RSVD           ", 0x26, 1},
    {"RSVD           ", 0x27, 1},     {"RSVD           ", 0x28, 1},  {"RSVD           ", 0x29, 1},
    {"RSVD           ", 0x2A, 1},     {"RSVD           ", 0x2B, 1},  {"RSVD           ", 0x2C, 1},
    {"RSVD           ", 0x2D, 1},     {"RSVD           ", 0x2E, 1},  {"RSVD           ", 0x2F, 1},
    {"GMAC0_W        ", 0x30, 0},     {"GMAC1_W        ", 0x31, 0},  {"GMAC_TOE0_W    ", 0x32, 0},
    {"GMAC_TOE1_W    ", 0x33, 0},     {"GMAC_TOE2_W    ", 0x34, 0},  {"GMAC_TOE3_W    ", 0x35, 0},
    {"GMAC_TOE4_W    ", 0x36, 0},     {"USB30_DRD_W    ", 0x37, 0},  {"VDEC1_W        ", 0x38, 0},
    {"VDEC0_W        ", 0x39, 0},     {"GMAC_TOE5_W    ", 0x3A, 0},  {"GMAC_TOE6_W    ", 0x3B, 0},
    {"GMAC_TOE7_W    ", 0x3C, 0},     {"SGDMA_W        ", 0x3D, 0},  {"BDMA_NET_W     ", 0x3E, 0},
    {"BIST_W         ", 0x3F, 0},     {"RSVD           ", 0x40, 1},  {"RSVD           ", 0x41, 1},
    {"RSVD           ", 0x42, 1},     {"RSVD           ", 0x43, 1},  {"RSVD           ", 0x44, 1},
    {"RSVD           ", 0x45, 1},     {"JPDW0_W        ", 0x46, 0},  {"JPDW1_W        ", 0x47, 0},
    {"RSVD           ", 0x48, 1},     {"RSVD           ", 0x49, 1},  {"PQ0_VIP_STAT_W ", 0x4A, 0},
    {"VENC0_W        ", 0x4B, 0},     {"DISP_CVBS_W    ", 0x4C, 0},  {"VENC0_SAD1_W   ", 0x4D, 0},
    {"GE_W           ", 0x4E, 0},     {"BIST_W         ", 0x4F, 0},  {"ISP_3DNR0_W    ", 0x50, 0},
    {"ISP_IMG_W      ", 0x51, 0},     {"ISP_DMA0_W     ", 0x52, 0},  {"ISP_DMA1_W     ", 0x53, 0},
    {"ISP_DMA2_W     ", 0x54, 0},     {"ISP_DMA3_W     ", 0x55, 0},  {"RSVD           ", 0x56, 1},
    {"ISP_WDR_W      ", 0x57, 0},     {"RSVD           ", 0x58, 1},  {"RSVD           ", 0x59, 1},
    {"RSVD           ", 0x5A, 1},     {"RSVD           ", 0x5B, 1},  {"RSVD           ", 0x5C, 1},
    {"SATA1_W        ", 0x5D, 0},     {"SATA2_W        ", 0x5E, 0},  {"BIST_W         ", 0x5F, 0},
    {"BIST256_W      ", 0xF0, 0},     {"IPU_W          ", 0x100, 0}, {"CPU_CA55_W     ", 0x101, 0},
    {"CLIENT_NR ", 0x62, 0, 0, 0, 0},
};

struct miu_client miu0_rd_clients[] = {
    {"OVERALL_R      ", 0x00, 0},     {"CMDQ1_R        ", 0x01, 0},  {"JPE0_GOP_R     ", 0x02, 0},
    {"BACH_R         ", 0x03, 0},     {"CMDQ_R         ", 0x04, 0},  {"BDMA_R         ", 0x05, 0},
    {"BDMA2_R        ", 0x06, 0},     {"JPE0_R         ", 0x07, 0},  {"LOW_SPEED1_R   ", 0x08, 0},
    {"SD_R           ", 0x09, 0},     {"FCIE_R         ", 0x0A, 0},  {"USB20_UHC0_R   ", 0x0B, 0},
    {"RSVD           ", 0x0C, 1},     {"RSVD           ", 0x0D, 1},  {"SC_GOP_R       ", 0x0E, 0},
    {"BIST_R         ", 0x0F, 0},     {"CMDQ_CMD_R     ", 0x10, 0},  {"CMDQ_CMD1_R    ", 0x11, 0},
    {"CMDQ_ISP0_R    ", 0x12, 0},     {"RMAP_R         ", 0x13, 0},  {"AESDMA_R       ", 0x14, 0},
    {"BACH1_R        ", 0x15, 0},     {"LOW_SPEED0_R   ", 0x16, 0},  {"SC_RDMA0_R     ", 0x17, 0},
    {"BDMA_XOR_R     ", 0x18, 0},     {"JPE1_GOP_R     ", 0x19, 0},  {"JPE1_R         ", 0x1A, 0},
    {"LDC0_R         ", 0x1B, 0},     {"IVE_R          ", 0x1C, 0},  {"RSVD           ", 0x1D, 1},
    {"USB20_UHC1_R   ", 0x1E, 0},     {"BIST_R         ", 0x1F, 0},  {"RSVD           ", 0x20, 1},
    {"RSVD           ", 0x21, 1},     {"RSVD           ", 0x22, 1},  {"RSVD           ", 0x23, 1},
    {"RSVD           ", 0x24, 1},     {"RSVD           ", 0x25, 1},  {"RSVD           ", 0x26, 1},
    {"RSVD           ", 0x27, 1},     {"RSVD           ", 0x28, 1},  {"RSVD           ", 0x29, 1},
    {"RSVD           ", 0x2A, 1},     {"RSVD           ", 0x2B, 1},  {"RSVD           ", 0x2C, 1},
    {"RSVD           ", 0x2D, 1},     {"RSVD           ", 0x2E, 1},  {"RSVD           ", 0x2F, 1},
    {"GMAC0_R        ", 0x30, 0},     {"GMAC1_R        ", 0x31, 0},  {"GMAC_TOE0_R    ", 0x32, 0},
    {"GMAC_TOE1_R    ", 0x33, 0},     {"GMAC_TOE2_R    ", 0x34, 0},  {"GMAC_TOE3_R    ", 0x35, 0},
    {"GMAC_TOE4_R    ", 0x36, 0},     {"USB30_DRD_R    ", 0x37, 0},  {"VDEC1_R        ", 0x38, 0},
    {"VDEC0_R        ", 0x39, 0},     {"GMAC_TOE5_R    ", 0x3A, 0},  {"GMAC_TOE6_R    ", 0x3B, 0},
    {"GMAC_TOE7_R    ", 0x3C, 0},     {"CMDQ_TOE_R     ", 0x3D, 0},  {"BDMA_NET_R     ", 0x3E, 0},
    {"BIST_R         ", 0x3F, 0},     {"MOP_ROT0_Y_R   ", 0x40, 0},  {"MOP_ROT0_C_R   ", 0x41, 0},
    {"MOP_ROT1_Y_R   ", 0x42, 0},     {"MOP_ROT1_C_R   ", 0x43, 0},  {"DISP0_GOP_R    ", 0x44, 0},
    {"DISP1_GOP_R    ", 0x45, 0},     {"JPDR0_R        ", 0x46, 0},  {"DISP2_GOP_R    ", 0x47, 0},
    {"DISP3_GOP_R    ", 0x48, 0},     {"DISP4_GOP_R    ", 0x49, 0},  {"DISP5_GOP_R    ", 0x4A, 0},
    {"VENC0_R        ", 0x4B, 0},     {"JPDR1_R        ", 0x4C, 0},  {"VENC0_SAD1_R   ", 0x4D, 0},
    {"GE_R           ", 0x4E, 0},     {"VENC0_GOP_R    ", 0x4F, 0},  {"ISP_3DNR0_R    ", 0x50, 0},
    {"ISP_WDR_R      ", 0x51, 0},     {"ISP_DMAG_R     ", 0x52, 0},  {"ISP_MLOAD_R    ", 0x53, 0},
    {"RSVD           ", 0x54, 1},     {"RSVD           ", 0x55, 1},  {"RSVD           ", 0x56, 1},
    {"RSVD           ", 0x57, 1},     {"RSVD           ", 0x58, 1},  {"RSVD           ", 0x59, 1},
    {"RSVD           ", 0x5A, 1},     {"RSVD           ", 0x5B, 1},  {"RSVD           ", 0x5C, 1},
    {"SATA1_R        ", 0x5D, 0},     {"SATA2_R        ", 0x5E, 0},  {"BIST_R         ", 0x5F, 0},
    {"BIST256_R      ", 0xF0, 0},     {"IPU_R          ", 0x100, 0}, {"CPU_CA55_R     ", 0x101, 0},
    {"CLIENT_NR ", 0x62, 0, 0, 0, 0},
};

//-------------------------------------------------------------------------------------------------
//  MTLB HAL internal function
//-------------------------------------------------------------------------------------------------
const char *halWriteClientIDTName(int id)
{
    int i = 0;

    for (i = 0; i < MIU0_CLIENT_WR_NUM; i++)
    {
        if (miu0_wr_clients[i].bw_client_id == id)
        {
            return miu0_wr_clients[i].name;
        }
    }

    printk("%s: wrong client id: %X\r\n", __FUNCTION__, id);
    return NULL;
}

const char *halReadClientIDTName(int id)
{
    int i = 0;
    for (i = 0; i < MIU0_CLIENT_RD_NUM; i++)
    {
        if (miu0_rd_clients[i].bw_client_id == id)
        {
            return miu0_rd_clients[i].name;
        }
    }

    printk("%s: wrong client id: %X\r\n", __FUNCTION__, id);
    return NULL;
}

static MS_U64 HAL_MIU_BA2PA(MS_U64 u32BusAddr)
{
    MS_PHYADDR u32PhyAddr = 0x0UL;

#if 1
    u32PhyAddr = u32BusAddr - ARM_MIU0_BUS_BASE + ARM_MIU0_BASE_ADDR;
#else
    // pa = ba - offset
    if ((u32BusAddr >= ARM_MIU0_BUS_BASE) && (u32BusAddr < ARM_MIU1_BUS_BASE))
        u32PhyAddr = u32BusAddr - ARM_MIU0_BUS_BASE + ARM_MIU0_BASE_ADDR;
    else if ((u32BusAddr >= ARM_MIU1_BUS_BASE) && (u32BusAddr < ARM_MIU2_BUS_BASE))
        u32PhyAddr = u32BusAddr - ARM_MIU1_BUS_BASE + ARM_MIU1_BASE_ADDR;
    else
        u32PhyAddr = u32BusAddr - ARM_MIU2_BUS_BASE + ARM_MIU2_BASE_ADDR;
#endif
    return u32PhyAddr;
}

static MS_U8 HAL_MIU_ReadByte(MS_U32 u32RegProtectId)
{
#if defined(CONFIG_ARM64)
    m_u32MiuMapBase = (mstar_pm_base + 0x00200000UL);
#endif
    return ((volatile MS_U8 *)(m_u32MiuMapBase))[(u32RegProtectId << 1) - (u32RegProtectId & 1)];
}

static MS_U16 HAL_MIU_Read2Byte(MS_U32 u32RegProtectId)
{
#if defined(CONFIG_ARM64)
    m_u32MiuMapBase = (mstar_pm_base + 0x00200000UL);
#endif
    return ((volatile MS_U16 *)(m_u32MiuMapBase))[u32RegProtectId];
}

static MS_BOOL HAL_MIU_WriteByte(MS_U32 u32RegProtectId, MS_U8 u8Val)
{
    if (!u32RegProtectId)
    {
        MIU_HAL_ERR("%s reg error!\n", __FUNCTION__);
        return FALSE;
    }

#if defined(CONFIG_ARM64)
    m_u32MiuMapBase = (mstar_pm_base + 0x00200000UL);
#endif
    ((volatile MS_U8 *)(m_u32MiuMapBase))[(u32RegProtectId << 1) - (u32RegProtectId & 1)] = u8Val;

    return TRUE;
}

static MS_BOOL HAL_MIU_Write2Byte(MS_U32 u32RegProtectId, MS_U16 u16Val)
{
    if (!u32RegProtectId)
    {
        MIU_HAL_ERR("%s reg error!\n", __FUNCTION__);
        return FALSE;
    }

#if defined(CONFIG_ARM64)
    m_u32MiuMapBase = (mstar_pm_base + 0x00200000UL);
#endif
    ((volatile MS_U16 *)(m_u32MiuMapBase))[u32RegProtectId] = u16Val;

    return TRUE;
}

static void HAL_MIU_WriteByteMask(MS_U32 u32RegOffset, MS_U8 u8Mask, MS_BOOL bEnable)
{
    MS_U8 u8Val = HAL_MIU_ReadByte(u32RegOffset);

    u8Val = (bEnable) ? (u8Val | u8Mask) : (u8Val & ~u8Mask);
    HAL_MIU_WriteByte(u32RegOffset, u8Val);
}

static void HAL_MIU_Write2BytesMask(MS_U32 u32RegOffset, MS_U16 u16Mask, MS_U16 u16Val)
{
    MS_U16 u16temp = HAL_MIU_Read2Byte(u32RegOffset);

    u16temp = (u16temp & ~u16Mask) | (u16Val & u16Mask);

    // u16Val = (bEnable) ? (u16Val | u16Mask) : (u16Val & ~u16Mask);
    HAL_MIU_Write2Byte(u32RegOffset, u16temp);
}

static MS_BOOL HAL_MIU_SetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                  MS_U32 u32RegProtectIdEnLSB, MS_U32 u32RegProtectIdEnMSB)
{
    MS_U32 u32index0, u32index1;
    MS_U16 u16ID       = 0;
    MS_U32 u32IdEnable = 0;
    MS_U16 u16CheckID;
    MS_U8  u8isfound0, u8isfound1;

    // Reset IDEnables for protect u8Blockx
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        IDEnables[u8MiuSel][u8Blockx][u32index0] = 0;
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        u16ID = pu8ProtectId[u32index0];

        // Unused ID
        if (u16ID == 0x00)
        {
            continue;
        }
        else
        {
            u16CheckID = 0;
            // check insert ID is usable
            for (u32index1 = 0; u32index1 < MIU0_CLIENT_WR_NUM; u32index1++)
            {
                if (u16ID == miu0_wr_clients[u32index1].bw_client_id)
                {
                    u16CheckID = 1;
                    break;
                }
            }
            if (!u16CheckID)
                continue;
        }

        u8isfound0 = FALSE;

        for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_ID; u32index1++)
        {
            if (IDList[u8MiuSel][u32index1] == u16ID)
            {
                // ID reused former setting
                IDEnables[u8MiuSel][u8Blockx][u32index1] = 1;
                u8isfound0                               = TRUE;
                break;
            }
        }

        // Need to create new ID in IDList
        if (u8isfound0 != TRUE)
        {
            u8isfound1 = FALSE;

            for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_ID; u32index1++)
            {
                if (IDList[u8MiuSel][u32index1] == 0x00)
                {
                    IDList[u8MiuSel][u32index1]              = u16ID;
                    IDEnables[u8MiuSel][u8Blockx][u32index1] = 1;
                    u8isfound1                               = TRUE;
                    break;
                }
            }

            // ID overflow
            if (u8isfound1 == FALSE)
            {
                return FALSE;
            }
        }
    }

    u32IdEnable = 0;

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        if (IDEnables[u8MiuSel][u8Blockx][u32index0] == 1)
        {
            u32IdEnable |= (1 << u32index0);
        }
    }
    HAL_MIU_Write2Byte(u32RegProtectIdEnLSB, u32IdEnable & 0xFFFF);
    HAL_MIU_Write2Byte(u32RegProtectIdEnMSB, u32IdEnable >> 16);

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_Write2Byte(u32RegAddrID + (u32index0 << 1), IDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MMU_SetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                  MS_U32 u32RegProtectIdEnLSB, MS_U32 u32RegProtectIdEnMSB)
{
    MS_U32 u32index0, u32index1;
    MS_U16 u16ID       = 0;
    MS_U32 u32IdEnable = 0;
    MS_U16 u16CheckID;
    MS_U8  u8isfound0, u8isfound1;

    // Reset IDEnables for protect u8Blockx
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        MmuIDEnables[u8MiuSel][u8Blockx][u32index0] = 0;
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        u16ID = pu8ProtectId[u32index0];
        // Unused ID
        if (u16ID == 0x00)
        {
            continue;
        }
        else
        {
            u16CheckID = 0;
            // check insert ID is usable
            for (u32index1 = 0; u32index1 < MIU0_CLIENT_WR_NUM; u32index1++)
            {
                if (u16ID == miu0_wr_clients[u32index1].bw_client_id)
                {
                    u16CheckID = 1;
                    break;
                }
            }
            if (!u16CheckID)
                continue;
        }

        u8isfound0 = FALSE;

        for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_ID; u32index1++)
        {
            if (MmuIDList[u8MiuSel][u32index1] == u16ID)
            {
                // ID reused former setting
                MmuIDEnables[u8MiuSel][u8Blockx][u32index1] = 1;
                u8isfound0                                  = TRUE;
                break;
            }
        }

        // Need to create new ID in IDList
        if (u8isfound0 != TRUE)
        {
            u8isfound1 = FALSE;

            for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_ID; u32index1++)
            {
                if (MmuIDList[u8MiuSel][u32index1] == 0x00)
                {
                    MmuIDList[u8MiuSel][u32index1]              = u16ID;
                    MmuIDEnables[u8MiuSel][u8Blockx][u32index1] = 1;
                    u8isfound1                                  = TRUE;
                    break;
                }
            }

            // ID overflow
            if (u8isfound1 == FALSE)
            {
                return FALSE;
            }
        }
    }

    u32IdEnable = 0;

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        if (MmuIDEnables[u8MiuSel][u8Blockx][u32index0] == 1)
        {
            u32IdEnable |= (1 << u32index0);
        }
    }

    /*Enable the ID register*/
    HAL_MIU_Write2Byte(u32RegProtectIdEnLSB, u32IdEnable & 0xFFFF);
    HAL_MIU_Write2Byte(u32RegProtectIdEnMSB, u32IdEnable >> 16);

    /*Add protection client ID*/
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_Write2Byte(u32RegAddrID + (u32index0 << 1), MmuIDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MIU_ResetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                    MS_U32 u32RegProtectIdEnLSB, MS_U32 u32RegProtectIdEnMSB)
{
    MS_U32 u32index0, u32index1;
    MS_U8  u8isIDNoUse = 0;
    MS_U16 u16IdEnable = 0;

    // Reset IDEnables for protect u8Blockx
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        IDEnables[u8MiuSel][u8Blockx][u32index0] = 0;
    }

    u16IdEnable = 0x0UL;

    HAL_MIU_Write2Byte(u32RegProtectIdEnLSB, u16IdEnable);
    HAL_MIU_Write2Byte(u32RegProtectIdEnMSB, u16IdEnable);

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        u8isIDNoUse = FALSE;

        for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_BLOCK; u32index1++)
        {
            if (IDEnables[u8MiuSel][u32index1][u32index0] == 1)
            {
                // Protect ID is still be used
                u8isIDNoUse = FALSE;
                break;
            }
            u8isIDNoUse = TRUE;
        }

        if (u8isIDNoUse == TRUE)
        {
            IDList[u8MiuSel][u32index0] = 0x00;
        }
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_Write2Byte(u32RegAddrID + (u32index0 << 1), IDList[u8MiuSel][u32index0]);
        // HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, IDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MMU_ResetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                    MS_U32 u32RegProtectIdEnLSB, MS_U32 u32RegProtectIdEnMSB)
{
    MS_U32 u32index0, u32index1;
    MS_U8  u8isIDNoUse = 0;
    MS_U16 u16IdEnable = 0;

    // Reset IDEnables for protect u8Blockx
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        MmuIDEnables[u8MiuSel][u8Blockx][u32index0] = 0;
    }

    u16IdEnable = 0x0UL;

    HAL_MIU_Write2Byte(u32RegProtectIdEnLSB, u16IdEnable);
    HAL_MIU_Write2Byte(u32RegProtectIdEnMSB, u16IdEnable);

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        u8isIDNoUse = FALSE;

        for (u32index1 = 0; u32index1 < MIU_MAX_PROTECT_BLOCK; u32index1++)
        {
            if (MmuIDEnables[u8MiuSel][u32index1][u32index0] == 1)
            {
                // Protect ID is still be used
                u8isIDNoUse = FALSE;
                break;
            }
            u8isIDNoUse = TRUE;
        }

        if (u8isIDNoUse == TRUE)
        {
            MmuIDList[u8MiuSel][u32index0] = 0x00;
        }
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_Write2Byte(u32RegAddrID + (u32index0 << 1), MmuIDList[u8MiuSel][u32index0]);
        // HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, MmuIDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

#define GET_HIT_BLOCK(regval)      BITS_RANGE_VAL(regval, REG_MIU_PROTECT_HIT_NO)
#define GET_HIT_CLIENT(regval)     BITS_RANGE_VAL(regval, REG_MIU_PROTECT_HIT_ID)
#define GET_MMU_HIT_BLOCK(regval)  BITS_RANGE_VAL(regval, REG_MMU_PROTECT_HIT_NO)
#define GET_MMU_HIT_CLIENT(regval) BITS_RANGE_VAL(regval, REG_MMU_PROTECT_HIT_ID)

MS_BOOL HAL_MIU_CheckGroup(int group)
{
    if ((group >= E_PROTECT_0) && (group <= E_MMU_PROTECT_MAX))
        return TRUE;
    else
        return FALSE;
}

MS_BOOL HAL_MIU_GetHitProtectInfo(MS_U8 u8MiuSel, MIU_PortectInfo *pInfo)
{
    MS_U16 u16Ret    = 0;
    MS_U16 u16LoAddr = 0;
    MS_U16 u16HiAddr = 0;
    MS_U16 u16RetRD  = 0;
    // MS_U32 u32RegBase = (u8MiuSel) ? MIU1_REG_BASE : MIU_REG_BASE;
    MS_U16 u16MmuRet    = 0;
    MS_U16 u16MmuLoAddr = 0;
    MS_U16 u16MmuHiAddr = 0;
    MS_U16 u16MmuRetRD  = 0;
    // MS_U32 u32MmuRegBase = MIU_MMU_REG_BASE;
    MS_U64 u32EndAddr = 0;
    char   clientName[40];
    if (!pInfo)
    {
        return FALSE;
    }

#if 0
    printk("%s %d  miu wcmd=%X \r\n", __FUNCTION__, __LINE__, HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_STATUS));
    printk("%s %d  mmu wcmd=%X \r\n", __FUNCTION__, __LINE__, HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_STATUS));
#endif
    if (HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_STATUS) & REG_MIU_PROTECT_HIT_FALG)
    {
        u16Ret    = HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_STATUS);
        u16LoAddr = HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_LOADDR);
        u16HiAddr = HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_HIADDR);
    }
    else if (HAL_MIU_Read2Byte(REG_MIU_RCMD_PROTECT_STATUS) & REG_MIU_PROTECT_HIT_FALG)
    {
        u16RetRD  = HAL_MIU_Read2Byte(REG_MIU_RCMD_PROTECT_STATUS);
        u16LoAddr = HAL_MIU_Read2Byte(REG_MIU_RCMD_PROTECT_LOADDR);
        u16HiAddr = HAL_MIU_Read2Byte(REG_MIU_RCMD_PROTECT_HIADDR);
    }

    if (HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_STATUS) & REG_MMU_PROTECT_HIT_FALG)
    {
        u16MmuRet    = HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_STATUS);
        u16MmuLoAddr = HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_LOADDR);
        u16MmuHiAddr = HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_HIADDR);
    }
    else if (HAL_MIU_Read2Byte(REG_MMU_RCMD_PROTECT_STATUS) & REG_MMU_PROTECT_HIT_FALG)
    {
        u16MmuRetRD  = HAL_MIU_Read2Byte(REG_MMU_RCMD_PROTECT_STATUS);
        u16MmuLoAddr = HAL_MIU_Read2Byte(REG_MMU_RCMD_PROTECT_LOADDR);
        u16MmuHiAddr = HAL_MIU_Read2Byte(REG_MMU_RCMD_PROTECT_HIADDR);
    }

    if (REG_MMU_PROTECT_HIT_FALG & u16MmuRet) /*indicate protection area been accessed*/
    {
        pInfo->bHit    = TRUE;
        pInfo->u8Block = (MS_U8)GET_MMU_HIT_BLOCK(u16MmuRet);         /*record times of hitting protection area*/
        pInfo->u8Group = (MS_U8)(GET_MMU_HIT_CLIENT(u16MmuRet) >> 4); /* calculate group id*/
        pInfo->u8ClientID =
            (MS_U8)(GET_MMU_HIT_CLIENT(u16MmuRet) & 0x0F); /*record id which attemp to write protection area*/
        pInfo->uAddress = (MS_U32)((u16MmuHiAddr << 16) | u16MmuLoAddr);
        pInfo->uAddress = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT; /*address alignment*/

        u32EndAddr = (pInfo->uAddress + MIU_PROTECT_ADDRESS_UNIT - 1);
        strcpy(clientName, (char *)halWriteClientIDTName((MS_U16)(GET_HIT_CLIENT(u16MmuRet))));
        printk("MIU%u Block:%u WR Client:%s ID:%u-%u Hitted_Address(MMU):0x%llx<->0x%llx\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, REG_MMU_PROTECT_LOG_CLR);
        HAL_MIU_Read2Byte(REG_MIU_PROTECT_STATUS);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, 0x00);
    }
    else if (REG_MMU_PROTECT_HIT_FALG & u16MmuRetRD) /*indicate protection area been accessed*/
    {
        pInfo->bHit    = TRUE;
        pInfo->u8Block = (MS_U8)GET_MMU_HIT_BLOCK(u16MmuRetRD);         /*record times of hitting protection area*/
        pInfo->u8Group = (MS_U8)(GET_MMU_HIT_CLIENT(u16MmuRetRD) >> 4); /* calculate group id*/
        pInfo->u8ClientID =
            (MS_U8)(GET_MMU_HIT_CLIENT(u16MmuRetRD) & 0x0F); /*record id which attemp to write protection area*/
        pInfo->uAddress = (MS_U32)((u16MmuHiAddr << 16) | u16MmuLoAddr);
        pInfo->uAddress = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT; /*address alignment*/

        u32EndAddr = (pInfo->uAddress + MIU_PROTECT_ADDRESS_UNIT - 1);
        strcpy(clientName, (char *)halReadClientIDTName((MS_U16)(GET_HIT_CLIENT(u16MmuRetRD))));
        printk("MIU%u Block:%u RD Client:%s ID:%u-%u Hitted_Address(MMU):0x%llx<->0x%llx\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, REG_MMU_PROTECT_LOG_CLR);
        HAL_MIU_Read2Byte(REG_MIU_PROTECT_STATUS);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, 0x00);
    }

    if (REG_MIU_PROTECT_HIT_FALG & u16Ret)
    {
        pInfo->bHit       = TRUE;
        pInfo->u8Block    = (MS_U8)GET_HIT_BLOCK(u16Ret);
        pInfo->u8Group    = (MS_U8)(GET_HIT_CLIENT(u16Ret) >> 4);
        pInfo->u8ClientID = (MS_U8)(GET_HIT_CLIENT(u16Ret) & 0x0F);
        pInfo->uAddress   = (MS_U32)((u16HiAddr << 16) | u16LoAddr);
        pInfo->uAddress   = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT;

        u32EndAddr = (pInfo->uAddress + MIU_PROTECT_ADDRESS_UNIT - 1);
        strcpy(clientName, (char *)halWriteClientIDTName((MS_U16)(GET_HIT_CLIENT(u16Ret))));
        printk("MIU%u Block:%u WR Client:%s ID:%u-%u Hitted_Address(MIU):0x%llx<->0x%llx\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, REG_MIU_PROTECT_LOG_CLR);
        HAL_MIU_Read2Byte(REG_MIU_PROTECT_STATUS);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, 0x00);
    }
    else if (REG_MIU_PROTECT_HIT_FALG & u16RetRD)
    {
        pInfo->bHit       = TRUE;
        pInfo->u8Block    = (MS_U8)GET_HIT_BLOCK(u16RetRD);
        pInfo->u8Group    = (MS_U8)(GET_HIT_CLIENT(u16RetRD) >> 4);
        pInfo->u8ClientID = (MS_U8)(GET_HIT_CLIENT(u16RetRD) & 0x0F);
        pInfo->uAddress   = (MS_U32)((u16HiAddr << 16) | u16LoAddr);
        pInfo->uAddress   = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT;

        u32EndAddr = (pInfo->uAddress + MIU_PROTECT_ADDRESS_UNIT - 1);
        strcpy(clientName, (char *)halReadClientIDTName((MS_U16)(GET_HIT_CLIENT(u16RetRD))));
        printk("MIU%u Block:%u WR Client:%s ID:%u-%u Hitted_Address(MIU):0x%llx<->0x%llx\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, REG_MIU_PROTECT_LOG_CLR);
        HAL_MIU_Read2Byte(REG_MIU_PROTECT_STATUS);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, 0x00);

        if ((MIU_CLIENTR_VDEC1 == GET_HIT_CLIENT(u16RetRD)) || (MIU_CLIENTR_VDEC0 == GET_HIT_CLIENT(u16RetRD)))
            pInfo->bHit = FALSE;
    }

#if 0
    // Clear log
    HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, REG_MMU_PROTECT_LOG_CLR);
    HAL_MIU_Read2Byte(REG_MIU_PROTECT_STATUS);
    HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, 0x00);


    u16LoAddr   = HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_LOADDR);
    u16HiAddr   = HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_HIADDR);
    pInfo->uAddress     = (MS_U32)((u16HiAddr << 16) | u16LoAddr);
    pInfo->uAddress     = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT;
    printk("%s %d  miu wcmd=%X address = %X\r\n", __FUNCTION__, __LINE__, HAL_MIU_Read2Byte(REG_MIU_WCMD_PROTECT_STATUS),  (unsigned int)pInfo->uAddress);


    u16MmuLoAddr = HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_LOADDR);
    u16MmuHiAddr = HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_HIADDR);
    pInfo->uAddress = (MS_U32)((u16MmuHiAddr << 16) | u16MmuLoAddr);
    pInfo->uAddress = pInfo->uAddress * MIU_PROTECT_ADDRESS_UNIT; /*address alignment*/
    printk("%s %d  mmu wcmd=%X address = %X\r\n", __FUNCTION__, __LINE__, HAL_MIU_Read2Byte(REG_MMU_WCMD_PROTECT_STATUS), (unsigned int)pInfo->uAddress);
#endif
    return TRUE;
}

MS_BOOL HAL_MIU_GetProtectIdEnVal(MS_U8 u8MiuSel, MS_U8 u8BlockId, MS_U8 u8ProtectIdIndex)
{
    if (u8BlockId > E_MIU_PROTECT_MAX)
    {
        u8BlockId -= E_MMU_PROTECT_0;
        return MmuIDEnables[u8MiuSel][u8BlockId][u8ProtectIdIndex];
    }
    else
    {
        return IDEnables[u8MiuSel][u8BlockId][u8ProtectIdIndex];
    }
}

MS_U16 *HAL_MIU_GetDefaultKernelProtectClientID(void)
{
    if (IDNUM_KERNELPROTECT > 0)
    {
        return (MS_U16 *)&clientId_KernelProtect[0];
    }
    return NULL;
}

MS_U16 *HAL_MIU_GetKernelProtectClientID(MS_U8 u8MiuSel)
{
    if (IDNUM_KERNELPROTECT > 0)
    {
        return (MS_U16 *)&IDList[u8MiuSel][0];
    }
    return NULL;
}

MS_U16 *HAL_MMU_GetKernelProtectClientID(MS_U8 u8MiuSel)
{
    if (IDNUM_KERNELPROTECT > 0)
    {
        return (MS_U16 *)&MmuIDList[u8MiuSel][0];
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: HAL_MIU_Protect()
/// @brief \b Function \b Description: Enable/Disable MIU Protection mode
/// @param u8Blockx        \b IN     : MIU Block to protect (0 ~ 31)
/// @param *pu8ProtectId   \b IN     : Allow specified client IDList to write
/// @param u32Start        \b IN     : Starting bus address
/// @param u32End          \b IN     : End bus address
/// @param bSetFlag        \b IN     : Disable or Enable MIU protection
///                                      - -Disable(0)
///                                      - -Enable(1)
/// @param <OUT>           \b None    :
/// @param <RET>           \b None    :
/// @param <GLOBAL>        \b None    :
////////////////////////////////////////////////////////////////////////////////
MS_BOOL HAL_MIU_Protect(MS_U8 u8Blockx, MS_U16 *pu8ProtectId, ss_phys_addr_t u64BusStart, ss_phys_addr_t u64BusEnd,
                        MS_BOOL bSetFlag)
{
    MS_U32 u32RegProtectId = 0;
    // MS_U32 u32RegBase = 0;
    MS_U32 u32RegStartAddr      = 0;
    MS_U32 u32RegEndAddr        = 0;
    MS_U32 u32RegAddrMSB        = 0;
    MS_U32 u32RegProtectIdEnLSB = 0;
    MS_U32 u32RegProtectIdEnMSB = 0;
    MS_U32 u32RegRWProtectEn    = 0;
    MS_U64 u64StartOffset       = 0;
    MS_U64 u64EndOffset         = 0;
    MS_U8  u8MiuSel             = 0;
    MS_U16 u16Data              = 0;
    MS_U16 u16Data1             = 0;
    MS_U16 u16Data2             = 0;
    MS_U8  u8Data               = 0;
    MS_U64 u64Start = 0, u64End = 0;
    MS_U16 u16Region   = 0;
    MS_U8  u8MaxRegion = 0;
    MS_U64 u32DramSize = 0;

    // printk("%s %d start=%llX, %llX u8Blockx=%X\r\n", __FUNCTION__, __LINE__, u64BusStart, u64BusEnd, u8Blockx);
    u32DramSize = HAL_MIU_ProtectDramSize() / (1 << MIU_MMU_ENTRY_BITS); // 8192 Entry
    u64Start    = HAL_MIU_BA2PA(u64BusStart);
    u64End      = HAL_MIU_BA2PA(u64BusEnd);

    // printk("%s %d start=%llX, %llX\r\n", __FUNCTION__, __LINE__, u64Start, u64End);
    //  Get MIU selection and offset
    _phy_to_miu_offset(u8MiuSel, u64EndOffset, u64End);
    _phy_to_miu_offset(u8MiuSel, u64StartOffset, u64Start);

    u64Start = u64StartOffset;
    u64End   = u64EndOffset;

    // printk("%s %d start=%llX, %llX\r\n", __FUNCTION__, __LINE__, u64Start, u64End);

    switch (m_u8PgszMode)
    {
        case E_MMU_PGSZ_64:
            u16Region   = MMU_ADDR_TO_REGION_64((unsigned long long)u64Start);
            u8MaxRegion = u32DramSize / MMU_PAGE_SIZE_64;
            break;
        case E_MMU_PGSZ_128:
            u16Region   = MMU_ADDR_TO_REGION_128((unsigned long long)u64Start);
            u8MaxRegion = u32DramSize / MMU_PAGE_SIZE_128;
            break;
        default:
            MIU_HAL_ERR("Err: not support page size (%d)\n", m_u8PgszMode);
            break;
    }

    // Parameter check
    if (u8Blockx > E_MMU_PROTECT_MAX)
    {
        MIU_HAL_ERR("Err: Out of the number of protect device\n");
        return FALSE;
    }
    else if (((u64Start & ((1 << MIU_PAGE_SHIFT) - 1)) != 0) || ((u64End & ((1 << MIU_PAGE_SHIFT) - 1)) != 0))
    {
        MIU_HAL_ERR("Err: u32Start:0x%llx,u32End:0x%llx,Protected address should be aligned to 8KB\n", u64Start,
                    u64End);
        return FALSE;
    }
    else if (u64Start >= u64End)
    {
        MIU_HAL_ERR("Err: Start address is equal to or more than end address\n");
        return FALSE;
    }

    if (((s16RepRegionRec == -1) || ((u16Region >= 0) && (u16Region <= u8MaxRegion)))
        && ((u8Blockx >= E_PROTECT_0) && (u8Blockx <= E_MIU_PROTECT_MAX)))
    {
        if (u8MiuSel == E_MIU_0)
        {
            // reg_protect_id00
            u32RegProtectId = MIU_PROTECT_WID00;
            // u32RegBase = MIU_REG_BASE;

            // only set protect write enable
            u8Data               = 1 << (u8Blockx % 4);
            u32RegRWProtectEn    = MIU_PROTECT00_03_EN + ((u8Blockx / 4) << 1);
            u32RegStartAddr      = MIU_PROTECT00_START + (u8Blockx << 2);
            u32RegEndAddr        = MIU_PROTECT00_END + (u8Blockx << 2);
            u32RegProtectIdEnLSB = MIU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
            u32RegProtectIdEnMSB = MIU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);
            u32RegAddrMSB        = MIU_PROTECT00_MSB + (u8Blockx << 1);
        }
        else
        {
            MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
            return FALSE;
        }

        // Disable MIU write protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);

        if (bSetFlag)
        {
            // Set Protect IDList
            if (HAL_MIU_SetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                   u32RegProtectIdEnMSB)
                == FALSE)
            {
                return FALSE;
            }

            // Set BIT29~35 of start/end address
            u16Data2 = (MS_U16)(u64Start >> (MIU_PAGE_SHIFT + 16)); // u16Data2 for start_ext addr
            u16Data1 = u16Data2 | (MS_U16)(((u64End - 1) >> (MIU_PAGE_SHIFT + 16)) << 8);
            HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

            // Start Address
            u16Data = (MS_U16)(u64Start >> MIU_PAGE_SHIFT); // 4k unit
            HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

            // End Address
            u16Data = (MS_U16)((u64End >> MIU_PAGE_SHIFT) - 1); // 4k unit;
            HAL_MIU_Write2Byte(u32RegEndAddr, u16Data);

            // Enable MIU write protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
#if 0
            // dbg message
            printk("\r\n======= Set BlockID %d =========\r\n", u8Blockx);
            printk("Protection client ID reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectId >> 8) | 0x1000, (u32RegProtectId & 0xFF) >> 1);
            printk("Protection write enable reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegRWProtectEn >> 8) | 0x1000, (u32RegRWProtectEn & 0xFF) >>1 );
            printk("Protection write enable bit: 0x%X\r\n", u8Data);
            printk("Protection start addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegStartAddr >> 8) | 0x1000, (u32RegStartAddr & 0xFF) >> 1);
            printk("Protection end addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegEndAddr >> 8) | 0x1000, (u32RegEndAddr & 0xFF) >> 1);
            printk("Protection MSB addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegAddrMSB >> 8) | 0x1000, (u32RegAddrMSB & 0xFF) >> 1);
            printk("Protect client enable for LSB: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectIdEnLSB >> 8) | 0x1000, (u32RegProtectIdEnLSB & 0xFF) >> 1);
            printk("Protect client enable for MSB: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectIdEnMSB >> 8) | 0x1000, (u32RegProtectIdEnMSB & 0xFF) >> 1);
            printk("================================\r\n");
#endif
            // Reset Protect IDList
            HAL_MIU_ResetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                 u32RegProtectIdEnMSB);
        }

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, REG_MIU_PROTECT_LOG_CLR);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, 0x00);
    }
    else if (((s16RepRegionRec != -1) && (s16RegionRec == u16Region))
             && ((u8Blockx >= E_MMU_PROTECT_0) && (u8Blockx <= E_MMU_PROTECT_MAX)))
    {
        if (u8MiuSel == E_MIU_0)
        {
            u32RegProtectId = MMU_PROTECT_WID00; /*mmu protection client ID*/
            // u32RegBase = MIU_MMU_REG_BASE;
            u8Blockx = u8Blockx - E_MMU_PROTECT_0;

            u8Data               = 1 << (u8Blockx % 4);
            u32RegRWProtectEn    = MMU_PROTECT00_03_EN + ((u8Blockx / 4) << 1);
            u32RegStartAddr      = MMU_PROTECT00_START + (u8Blockx << 2);
            u32RegEndAddr        = MMU_PROTECT00_END + (u8Blockx << 2);
            u32RegProtectIdEnLSB = MMU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
            u32RegProtectIdEnMSB = MMU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);
            u32RegAddrMSB        = MMU_PROTECT00_MSB + (u8Blockx << 1);
        }
        else
        {
            MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
            return FALSE;
        }

        // Disable MMU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);
        if (bSetFlag)
        {
            // Set Protect IDList
            if (HAL_MMU_SetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                   u32RegProtectIdEnMSB)
                == FALSE)
            {
                return FALSE;
            }

            // Set BIT29~35 of start/end address
            u16Data2 = (MS_U16)(u64Start >> (MIU_PAGE_SHIFT + 16)); // u16Data2 for start_ext addr
            u16Data1 = u16Data2 | (MS_U16)(((u64End - 1) >> (MIU_PAGE_SHIFT + 16)) << 8);
            HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

            // Start Address
            u16Data = (MS_U16)(u64Start >> MIU_PAGE_SHIFT); // 4k unit
            HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

            // End Address
            u16Data = (MS_U16)((u64End >> MIU_PAGE_SHIFT) - 1); // 4k unit;
            HAL_MIU_Write2Byte(u32RegEndAddr, u16Data);

            // Enable MMU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
#if 0
            // dbg message
            printk("\r\n======= Set MMU BlockID %d =========\r\n", u8Blockx);
            printk("Protection client ID reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectId >> 8) | 0x1000, (u32RegProtectId & 0xFF) >> 1);
            printk("Protection write enable reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegRWProtectEn >> 8) | 0x1000, (u32RegRWProtectEn & 0xFF) >>1 );
            printk("Protection write enable bit: 0x%X\r\n", u8Data);
            printk("Protection start addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegStartAddr >> 8) | 0x1000, (u32RegStartAddr & 0xFF) >> 1);
            printk("Protection end addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegEndAddr >> 8) | 0x1000, (u32RegEndAddr & 0xFF) >> 1);
            printk("Protection MSB addr reg: bank: 0x%X, offset: 0x%X\r\n", (u32RegAddrMSB >> 8) | 0x1000, (u32RegAddrMSB & 0xFF) >> 1);
            printk("Protect client enable for LSB: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectIdEnLSB >> 8) | 0x1000, (u32RegProtectIdEnLSB & 0xFF) >> 1);
            printk("Protect client enable for MSB: bank: 0x%X, offset: 0x%X\r\n", (u32RegProtectIdEnMSB >> 8) | 0x1000, (u32RegProtectIdEnMSB & 0xFF) >> 1);
            printk("================================\r\n");
#endif

            // Reset Protect IDList
            HAL_MMU_ResetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                 u32RegProtectIdEnMSB);
        }

        // Clear log
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, REG_MMU_PROTECT_LOG_CLR);
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, 0x00);
    }
    else
    {
        MIU_HAL_ERR(
            "%s u32Start:0x%llx,u32End:0x%llx.The address is neither a physical address nor a virtual address,Please "
            "check it!\n",
            __FUNCTION__, u64Start, u64End);
        return FALSE;
    }

    return TRUE;
}

MS_BOOL HAL_SetAccessFromVpaOnly(MS_U8 u8Blockx, ss_phys_addr_t u64BusStart, ss_phys_addr_t u64BusEnd, MS_BOOL bSetFlag)
{
    MS_U32 u32RegStartAddr   = 0;
    MS_U32 u32RegEndAddr     = 0;
    MS_U32 u32RegAddrMSB     = 0;
    MS_U32 u32RegRWProtectEn = 0;
    MS_U64 u32StartOffset    = 0;
    MS_U64 u32EndOffset      = 0;
    MS_U8  u8MiuSel          = 0;
    MS_U16 u16Data           = 0;
    MS_U16 u16Data1          = 0;
    MS_U16 u16Data2          = 0;
    MS_U8  u8Data            = 0;
    MS_U64 u64Start = 0, u64End = 0;
    MS_U16 u16ChkFlgData       = 0;
    MS_U32 u32RegMmuProtChkFlg = 0;

    u64Start = HAL_MIU_BA2PA(u64BusStart);
    u64End   = HAL_MIU_BA2PA(u64BusEnd);
    // Get MIU selection and offset
    _phy_to_miu_offset(u8MiuSel, u32EndOffset, u64End);
    _phy_to_miu_offset(u8MiuSel, u32StartOffset, u64Start);

    u64Start = u32StartOffset;
    u64End   = u32EndOffset;
    // Parameter check
    if (u8Blockx > E_MIU_PROTECT_MAX)
    {
        MIU_HAL_ERR("Err: PA - Out of the number of PA protect device\n");
        return FALSE;
    }
    else if (((u64Start & ((1 << MIU_PAGE_SHIFT) - 1)) != 0) || ((u64End & ((1 << MIU_PAGE_SHIFT) - 1)) != 0))
    {
        MIU_HAL_ERR("Err: PA - Protected address should be aligned to 8KB\n");
        return FALSE;
    }
    else if (u64Start >= u64End)
    {
        MIU_HAL_ERR("Err: PA - Start address is equal to or more than end address\n");
        return FALSE;
    }

    if (u8MiuSel == E_MIU_0)
    {
        u8Data            = 1 << (u8Blockx % 4);
        u32RegRWProtectEn = MIU_PROTECT00_03_EN + ((u8Blockx / 4) << 1);

        u16ChkFlgData = 1 << (u8Blockx & 0x0F); /*mmu flag check enable*/
        if (u8Blockx > 0x0F)
        {
            u32RegMmuProtChkFlg = MMU_PROTECT_CHK_FLG_MSB; /*mmu check flag control register*/
        }
        else
        {
            u32RegMmuProtChkFlg = MMU_PROTECT_CHK_FLG_LSB; /*mmu check flag control register*/
        }

        u32RegStartAddr = MIU_PROTECT00_START + (u8Blockx << 2);
        u32RegEndAddr   = MIU_PROTECT00_END + (u8Blockx << 2);
        u32RegAddrMSB   = MIU_PROTECT00_MSB + (u8Blockx << 1);
    }
    else
    {
        MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
        return FALSE;
    }

    // Disable MIU write/read protect
    HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);

    /* Clear mmu check flag check */
    HAL_MIU_Write2BytesMask(u32RegMmuProtChkFlg, u16ChkFlgData, 0x00);

    if (bSetFlag)
    {
        /* Set mmu check flag check */
        HAL_MIU_Write2BytesMask(u32RegMmuProtChkFlg, u16ChkFlgData, u16ChkFlgData);

        // Set BIT29~35 of start/end address
        u16Data2 = (MS_U16)(u64Start >> (MIU_PAGE_SHIFT + 16)); // u16Data2 for start_ext addr
        u16Data1 = u16Data2 | (MS_U16)(((u64End - 1) >> (MIU_PAGE_SHIFT + 16)) << 8);
        HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

        // Start Address
        u16Data = (MS_U16)(u64Start >> MIU_PAGE_SHIFT); // 4k unit
        HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

        // End Address
        u16Data = (MS_U16)((u64End >> MIU_PAGE_SHIFT) - 1); // 4k unit;
        HAL_MIU_Write2Byte(u32RegEndAddr, u16Data);

        // Enable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
    }

    // Clear log
    HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, REG_MIU_PROTECT_LOG_CLR);
    HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, 0x00);

    // Mask PWR IRQ
    // HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_PWR_IRQ_MASK_OFFSET, REG_MIU_PROTECT_PWR_IRQ_MASK_BIT, TRUE);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: HAL_MIU_Protect_Add_Ext_Feature()
/// @brief \b Function \b Description: Add extral features for miu protection to test
/// @param u8MiuSel        \b IN     : u8ModeSel(0-1) 0:MIU    1:MMU
/// @param *pu8ProtectId   \b IN     : Allow specified client IDList to write
/// @param u8Blockx        \b IN     : MIU Block to protect (0 ~ 4)
/// @param bSetFlag        \b IN     : Disable or Enable MIU protection
///                                        - -Disable(0)
///                                        - -Enable(1)
/// @param bIdFlag         \b IN     : Disable or Enable MIU protec id
/// @param bInvertFlag     \b IN     : MIU protection function invert
/// @param <OUT>           \b None   :
/// @param <RET>           \b None   :
/// @param <GLOBAL>        \b None   :
////////////////////////////////////////////////////////////////////////////////

MS_BOOL HAL_MIU_Protect_Add_Ext_Feature(MS_U8 u8ModeSel, MS_U16 *pu8ProtectId, MS_U8 u8Blockx, MS_BOOL bSetFlag,
                                        MS_BOOL bIdFlag, MS_BOOL bInvertFlag)
{
    MS_U8  u8Data               = 0;
    MS_U8  u8InvertData         = 0;
    MS_U32 u32RegInvert         = 0;
    MS_U32 u32RegRWProtectEn    = 0;
    MS_U32 u32RegProtectId      = 0;
    MS_U32 u32RegProtectIdEnLSB = 0;
    MS_U32 u32RegProtectIdEnMSB = 0;

    if (u8ModeSel == 0)
    {
        if (u8Blockx >= E_MIU_PROTECT_MAX)
        {
            MIU_HAL_ERR("Err: MIU Blk Num out of range\n");
            return FALSE;
        }

        u32RegProtectId = MIU_PROTECT_WID00;

        u8Data            = 1 << (u8Blockx % 4);
        u32RegRWProtectEn = MIU_PROTECT00_03_EN + ((u8Blockx / 4) << 1);

        u8InvertData = 1 << (u8Blockx % 4);
        u32RegInvert = MIU_PROTECT00_03_INVERT + ((u8Blockx / 4) << 1);

        u32RegProtectIdEnLSB = MIU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
        u32RegProtectIdEnMSB = MIU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);

        // Disable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);
        if (bSetFlag)
        {
            if (!bIdFlag)
            {
                // Reset Protect IDList
                HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                     u32RegProtectIdEnMSB);
            }
            // Enable MIU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                 u32RegProtectIdEnMSB);
        }

        HAL_MIU_WriteByteMask(u32RegInvert, u8InvertData, DISABLE);
        if (bInvertFlag)
        {
            // Enable MIU invert function
            HAL_MIU_WriteByteMask(u32RegInvert, u8InvertData, ENABLE);
        }
    }
    else if (u8ModeSel == 1)
    {
        u8Blockx = u8Blockx - E_MMU_PROTECT_0;
        if (u8Blockx >= E_MIU_PROTECT_MAX)
        {
            MIU_HAL_ERR("Err: MMU Blk Num out of range\n");
            return FALSE;
        }

        u32RegProtectId   = MMU_PROTECT_WID00;
        u8Data            = 1 << (u8Blockx % 4);
        u32RegRWProtectEn = MMU_PROTECT00_03_EN + ((u8Blockx / 4) << 1);

        u8InvertData = 1 << (u8Blockx % 4);
        u32RegInvert = MMU_PROTECT00_03_INVERT + ((u8Blockx / 4) << 1);

        u32RegProtectIdEnLSB = MMU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
        u32RegProtectIdEnMSB = MMU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);

        // Disable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);
        if (bSetFlag)
        {
            if (!bIdFlag)
            {
                // Reset Protect IDList
                HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                     u32RegProtectIdEnMSB);
            }
            // Enable MIU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEnLSB,
                                 u32RegProtectIdEnMSB);
        }

        HAL_MIU_WriteByteMask(u32RegInvert, u8InvertData, DISABLE);
        if (bInvertFlag)
        {
            // Enable MIU invert function
            HAL_MIU_WriteByteMask(u32RegInvert, u8InvertData, ENABLE);
        }
    }
    else
    {
        MIU_HAL_ERR("%s not support Mode:%d!\n", __FUNCTION__, u8ModeSel);
        return FALSE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function  \b Name: HAL_MIU_ParseOccupiedResource
/// @brief \b Function  \b Description: Parse occupied resource to software structure
/// @return             \b 0: Fail 1: OK
////////////////////////////////////////////////////////////////////////////////
MS_BOOL HAL_MIU_ParseOccupiedResource(void)
{
    MS_U8  u8MiuSel             = 0;
    MS_U8  u8Blockx             = 0;
    MS_U8  u8ClientID           = 0;
    MS_U32 u32IdEnable          = 0;
    MS_U32 u32index             = 0;
    MS_U32 u32RegProtectId      = 0;
    MS_U32 u32RegProtectIdEnLSB = 0;
    MS_U32 u32RegProtectIdEnMSB = 0;

    for (u8MiuSel = E_MIU_0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
    {
        for (u8Blockx = 0; u8Blockx < MIU_MAX_PROTECT_BLOCK; u8Blockx++)
        {
            if (u8MiuSel == E_MIU_0)
            {
                u32RegProtectId      = MIU_PROTECT_WID00;
                u32RegProtectIdEnLSB = MIU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
                u32RegProtectIdEnMSB = MIU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);
            }
            else
            {
                MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
                return FALSE;
            }

            u32IdEnable = HAL_MIU_Read2Byte(u32RegProtectIdEnLSB) | (HAL_MIU_Read2Byte(u32RegProtectIdEnMSB) << 16);

            for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
            {
                IDEnables[u8MiuSel][u8Blockx][u32index] = ((u32IdEnable >> u32index) & 0x1UL) ? 1 : 0;
            }
        }

        for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
        {
            u8ClientID                 = HAL_MIU_ReadByte(u32RegProtectId + u32index) & 0xFF;
            IDList[u8MiuSel][u32index] = u8ClientID;
            // IDList[u8MiuSel][u32index] = clientTbl[u8MiuSel][u8ClientID];
        }
    }

    for (u8MiuSel = E_MIU_0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
    {
        for (u8Blockx = 0; u8Blockx < MMU_MAX_PROTECT_BLOCK; u8Blockx++)
        {
            if (u8MiuSel == E_MIU_0)
            {
                u32RegProtectId      = MMU_PROTECT_WID00;
                u32RegProtectIdEnLSB = MMU_PROTECT00_LSB_WID_ENABLE + (u8Blockx << 2);
                u32RegProtectIdEnMSB = MMU_PROTECT00_MSB_WID_ENABLE + (u8Blockx << 2);
            }
            else
            {
                MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
                return FALSE;
            }

            u32IdEnable = HAL_MIU_Read2Byte(u32RegProtectIdEnLSB) | (HAL_MIU_Read2Byte(u32RegProtectIdEnMSB) << 16);

            for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
            {
                MmuIDEnables[u8MiuSel][u8Blockx][u32index] = ((u32IdEnable >> u32index) & 0x1UL) ? 1 : 0;
            }
        }

        for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
        {
            u8ClientID                    = HAL_MIU_ReadByte(u32RegProtectId + u32index) & 0xFF;
            MmuIDList[u8MiuSel][u32index] = u8ClientID;
            // MmuIDList[u8MiuSel][u32index] = clientTbl[u8MiuSel][u8ClientID];
        }
    }

    return TRUE;
}

U64 HAL_MIU_ProtectDramSize(void)
{
    MS_U8 u8Val = HAL_MIU_ReadByte(MIU_PROTECT_DDR_SIZE);

    u8Val = u8Val & 0xF;

    if (0 == u8Val)
    {
        u8Val = 0x10; // 64GB
    }

    return ((unsigned long long)(0x1) << (20 + u8Val));
}

int HAL_MIU_Info(MIU_DramInfo_Hal *pDramInfo)
{
    int ret = -1;

    unsigned int ddr_region = 0;
    unsigned int ddfset     = 0;

    if (pDramInfo)
    {
        ddfset = (INREGMSK16(BASE_REG_MIU_ATOP + REG_ID_19, 0x00FF) << 16)
                 + INREGMSK16(BASE_REG_MIU_ATOP + REG_ID_18, 0xFFFF);
        ddr_region = INREG16(BASE_REG_MIU_ATOP + REG_ID_1C) & 0x03;

        pDramInfo->size        = HAL_MIU_ProtectDramSize();
        pDramInfo->dram_freq   = ((unsigned long long)432 << (21 + ddr_region)) / ddfset;
        pDramInfo->miupll_freq = 24 * INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x00FF)
                                 / ((INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x0700) >> 8) + 2);
        pDramInfo->type = INREGMSK16(BASE_REG_MIU_ATOP + REG_ID_00, 0x0003);
        ;
        pDramInfo->data_rate = 4;
        pDramInfo->bus_width = 32;
        pDramInfo->ssc       = ((INREGMSK16(BASE_REG_ATOP_PA + REG_ID_14, 0xC000) == 0x8000) ? 0 : 1);

        ret = 0;
    }

    return ret;
}

int HAL_MMU_SetPageSize(unsigned char u8PgszMode)
{
    MS_U16 u16CtrlRegVal;

    m_u8PgszMode = u8PgszMode;

    u16CtrlRegVal = HAL_MIU_Read2Byte(REG_MMU_CTRL);
    u16CtrlRegVal &= ~(BITS_RANGE(REG_MMU_CTRL_PG_SIZE)); /*clear bit1 and bit2*/
    switch (m_u8PgszMode)
    {
        case E_MMU_PGSZ_64:
            u16CtrlRegVal &= REG_MMU_CTRL_PG_SIZE_64K;
            break;
        case E_MMU_PGSZ_128:
            u16CtrlRegVal |= REG_MMU_CTRL_PG_SIZE_128K;
            break;
        default:
            break;
    }

    HAL_MIU_Write2Byte(REG_MMU_CTRL, u16CtrlRegVal);

    return 0;
}

int HAL_MMU_SetRegion(unsigned short u16Region, unsigned short u16ReplaceRegion)
{
    MS_U16 u16CtrlRegVal;
    MS_U8  u8RegShiftVal = 0;

    s16RegionRec    = u16Region;
    s16RepRegionRec = u16ReplaceRegion;

    switch (m_u8PgszMode)
    {
        case E_MMU_PGSZ_64:
            u8RegShiftVal = 7; // 128KB:[35:29],region control 5bit
            break;
        case E_MMU_PGSZ_128:
            u8RegShiftVal = 6; // 128KB:[35:30],region control 5bit
            break;
        default:
            break;
    }

    if (u16Region >> u8RegShiftVal)
    {
        MIU_HAL_ERR("Region value over range(0x%x)\n", u16Region);
        return -1;
    }

    if (u16ReplaceRegion >> u8RegShiftVal)
    {
        MIU_HAL_ERR("Replace Region value over range(0x%x)\n", u16ReplaceRegion);
        return -1;
    }

    u16CtrlRegVal = u16Region | (u16ReplaceRegion << 8);
    HAL_MIU_Write2Byte(REG_MMU_VPA_REPLACE, u16CtrlRegVal);
    return 0;
}

int HAL_MMU_Map(unsigned short u16VirtAddrEntry, unsigned short u16PhyAddrEntry)
{
    MS_U16 u16RegVal;

    if ((u16PhyAddrEntry >> MIU_MMU_ENTRY_BITS) || (u16VirtAddrEntry >> MIU_MMU_ENTRY_BITS))
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x, Virt:0x%x)\n", u16PhyAddrEntry, u16VirtAddrEntry);
        return -1;
    }

    // reg_mmu_wdata
    // HAL_MIU_Write2Byte(REG_MMU_W_DATA, u16PhyAddrEntry & REG_MMU_W_DATA_MASK);
    HAL_MIU_Write2BytesMask(REG_MMU_W_DATA, REG_MMU_W_DATA_MASK, u16PhyAddrEntry & REG_MMU_W_DATA_MASK);

    // reg_mmu_entry
    u16RegVal = REG_MMU_RW_ENTRY_MODE | u16VirtAddrEntry;
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16RegVal);

    // reg_mmu_access
    // HAL_MIU_Write2Byte(REG_MMU_ACCESS, 0x0000);
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, REG_MMU_ACCESS_TRIGGER);

    return 0;
}

unsigned short HAL_MMU_MapQuery(unsigned short u16VirtAddrEntry)
{
    MS_U16 u16RegVal;

    if (u16VirtAddrEntry >> MIU_MMU_ENTRY_BITS)
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x)\n", u16VirtAddrEntry);
        return -1;
    }

    // reg_mmu_entry
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16VirtAddrEntry);

    // reg_mmu_access
    // HAL_MIU_Write2Byte(REG_MMU_ACCESS, 0x0000);
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, REG_MMU_ACCESS_TRIGGER);

    ndelay(100);

    // reg_mmu_rdata
    u16RegVal = HAL_MIU_Read2Byte(REG_MMU_R_DATA);

    return u16RegVal;
}

int HAL_MMU_UnMap(unsigned short u16PhyAddrEntry)
{
    MS_U16 u16RegVal;

    if (u16PhyAddrEntry >> MIU_MMU_ENTRY_BITS)
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x)\n", u16PhyAddrEntry);
        return -1;
    }

    // reg_mmu_wdata
    // HAL_MIU_Write2Byte(REG_MMU_W_DATA, MMU_INVALID_ENTRY_VAL);
    HAL_MIU_Write2BytesMask(REG_MMU_W_DATA, REG_MMU_W_DATA_MASK, MMU_INVALID_ENTRY_VAL);

    // reg_mmu_entry
    u16RegVal = REG_MMU_RW_ENTRY_MODE | u16PhyAddrEntry;
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16RegVal);

    // reg_mmu_access
    // HAL_MIU_Write2Byte(REG_MMU_ACCESS, 0x0000);
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, REG_MMU_ACCESS_TRIGGER);

    return 0;
}

int HAL_MMU_Enable(unsigned char u8Enable)
{
    MS_U16 u16CtrlRegVal;
    MS_U16 u16IrqRegVal;

    u16CtrlRegVal = HAL_MIU_Read2Byte(REG_MMU_CTRL);
    u16IrqRegVal  = HAL_MIU_Read2Byte(REG_MMU_IRQ_CTRL);

    if (u8Enable)
    {
        u16CtrlRegVal |= REG_MMU_CTRL_ENABLE;
        // Enable IRQ
        // u16IrqRegVal |= (REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RD_MASK | REG_MMU_IRQ_WR_MASK);
        u16IrqRegVal &= ~(REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RCMD_MASK | REG_MMU_IRQ_WCMD_MASK);
    }
    else
    {
        u16CtrlRegVal &= ~REG_MMU_CTRL_ENABLE;
        // Disable IRQ
        // u16IrqRegVal &= ~(REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RD_MASK | REG_MMU_IRQ_WR_MASK);
        u16IrqRegVal |= (REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RCMD_MASK | REG_MMU_IRQ_RCMD_MASK);
    }

    HAL_MIU_Write2Byte(REG_MMU_CTRL, u16CtrlRegVal);
    HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, u16IrqRegVal);

    return 0;
}

int HAL_MMU_Reset(void)
{
    MS_U16 u16RetryNum = 200;
    MS_U16 u16Reset    = (REG_MMU_CTRL_RESET | REG_MMU_CTRL_RESET_INIT_VAL);

    HAL_MIU_Write2BytesMask(REG_MMU_CTRL, REG_MMU_CTRL_ENABLE, ~REG_MMU_CTRL_ENABLE); // disabled mmu_en
    HAL_MIU_Write2Byte(REG_MMU_CTRL, 0x0);                                            // clear all
    HAL_MIU_Write2BytesMask(REG_MMU_CTRL, u16Reset, u16Reset);                        // set reset

    do
    {
        if (HAL_MIU_Read2Byte(REG_MMU_CTRL) & REG_MMU_CTRL_INIT_DONE)
        {
            HAL_MIU_Write2BytesMask(REG_MMU_CTRL, u16Reset, ~u16Reset); // disabled reset
            return 0;
        }

        u16RetryNum--;
    } while (u16RetryNum > 0);

    MIU_HAL_ERR("Reset timeout!\n");

    return -1;
}

unsigned int HAL_MMU_Status(unsigned short *u16PhyAddrEntry, unsigned short *u16ClientId, unsigned char *u8IsWriteCmd)
{
    MS_U16       u16IrqRegFlag;
    MS_U16       u16IrqRegCtrl;
    unsigned int u32Status = E_HAL_MMU_STATUS_NORMAL;

    u16IrqRegCtrl = HAL_MIU_Read2Byte(REG_MMU_IRQ_CTRL);
    u16IrqRegFlag = HAL_MIU_Read2Byte(REG_MMU_IRQ_FLAG);

    if (u16IrqRegFlag & REG_MMU_IRQ_RW_FLAG)
    {
        if (u16IrqRegFlag & REG_MMU_IRQ_RCMD_RW_FLAG)
        {
            *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_RCMD_COLLISION_ENTRY);
            u16IrqRegCtrl |= REG_MMU_IRQ_RCMD_RW_CLR;
        }
        else if (u16IrqRegFlag & REG_MMU_IRQ_WCMD_RW_FLAG)
        {
            *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_WCMD_COLLISION_ENTRY);
            u16IrqRegCtrl |= REG_MMU_IRQ_WCMD_RW_CLR;
        }
    }

    if ((u16IrqRegFlag & REG_MMU_IRQ_RCMD_FLAG) || (u16IrqRegFlag & REG_MMU_IRQ_WCMD_FLAG))
    {
        // read valid entry
        if (u16IrqRegFlag & REG_MMU_IRQ_RCMD_FLAG)
        {
            *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_RCMD_INVALID_ENTRY);
            *u16ClientId     = HAL_MIU_Read2Byte(REG_MMU_RCMD_INVALID_CLIENT_ID) & 0x01FF;
            u16IrqRegCtrl |= REG_MMU_IRQ_RCMD_CLR;
        }
        // write valid entry
        else if (u16IrqRegFlag & REG_MMU_IRQ_WCMD_FLAG)
        {
            *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_WCMD_INVALID_ENTRY);
            *u16ClientId     = HAL_MIU_Read2Byte(REG_MMU_WCMD_INVALID_CLIENT_ID) & 0x01FF;
            u16IrqRegCtrl |= REG_MMU_IRQ_WCMD_CLR;
        }

        if (u16IrqRegFlag & REG_MMU_IRQ_RCMD_FLAG)
        {
            u32Status |= E_HAL_MMU_STATUS_R_INVALID;
            *u8IsWriteCmd = 0;
        }

        if (u16IrqRegFlag & REG_MMU_IRQ_WCMD_FLAG)
        {
            u32Status |= E_HAL_MMU_STATUS_W_INVALID;
            *u8IsWriteCmd = 1;
        }
    }

    // Clear IRQ
    if (u32Status != E_HAL_MMU_STATUS_NORMAL)
    {
        HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, u16IrqRegCtrl);
        HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, 0x0);
    }

    return u32Status;
}

unsigned char HAL_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd)
{
    unsigned short grp_idx, grp_ch;
    unsigned int   effi_clean_reg, effi_status_reg;
    unsigned short u16RetryNum = 200;

    grp_idx = u16ClientId >> 4;
    grp_ch  = u16ClientId & 0x0F;

    if (wcmd)
    {
        switch (grp_idx)
        {
            case 0:
                effi_clean_reg  = MIU_EFFI_SC0_BASE + ((0x10 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_SC0_BASE + (0x30 << 1);
                break;
            case 1:
                effi_clean_reg  = MIU_EFFI_SC1_BASE + ((0x10 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_SC1_BASE + (0x30 << 1);
                break;
            case 3:
                effi_clean_reg  = MIU_EFFI_NET0_BASE + ((0x10 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_NET0_BASE + (0x30 << 1);
                break;
            case 4:
                effi_clean_reg  = MIU_EFFI_DISP0_BASE + ((0x10 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_DISP0_BASE + (0x30 << 1);
                break;
            case 5:
                effi_clean_reg  = MIU_EFFI_ISP0_BASE + ((0x10 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_ISP0_BASE + (0x30 << 1);
                break;
            default:
                printk(KERN_ERR "%s: Client ID %X is not support\r\n", __FUNCTION__, u16ClientId);
                return FALSE;
                break;
        }
    }
    else
    {
        switch (grp_idx)
        {
            case 0:
                effi_clean_reg  = MIU_EFFI_SC0_BASE + ((0x20 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_SC0_BASE + (0x31 << 1);
                break;
            case 1:
                effi_clean_reg  = MIU_EFFI_SC1_BASE + ((0x20 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_SC1_BASE + (0x31 << 1);
                break;
            case 3:
                effi_clean_reg  = MIU_EFFI_NET0_BASE + ((0x20 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_NET0_BASE + (0x31 << 1);
                break;
            case 4:
                effi_clean_reg  = MIU_EFFI_DISP0_BASE + ((0x20 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_DISP0_BASE + (0x31 << 1);
                break;
            case 5:
                effi_clean_reg  = MIU_EFFI_ISP0_BASE + ((0x20 + grp_ch) << 1);
                effi_status_reg = MIU_EFFI_ISP0_BASE + (0x31 << 1);
                break;
            default:
                printk(KERN_ERR "%s: Client ID %X is not support\r\n", __FUNCTION__, u16ClientId);
                return FALSE;
                break;
        }
    }

    // trun on clean-on
    HAL_MIU_Write2Byte(effi_clean_reg, BIT0);

    // wait clean-on status
    while (HAL_MIU_Read2Byte(effi_status_reg) != 0)
    {
        u16RetryNum--;
        if (!u16RetryNum)
        {
            printk(KERN_ERR "%s: Client ID %X wait reset status fail\r\n", __FUNCTION__, u16ClientId);
            HAL_MIU_Write2Byte(effi_clean_reg, 0x0000);
            return FALSE;
        }
    }

    // trun off clean-on
    HAL_MIU_Write2Byte(effi_clean_reg, 0x0000);
    return TRUE;
}

int HAL_MMU_AddClientId(unsigned short u16ClientId)
{
    return 0;
}

int HAL_MMU_RemoveClientId(unsigned short u16ClientId)
{
    return 0;
}
