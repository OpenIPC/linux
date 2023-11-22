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
#include "registers.h"
#include "ms_platform.h"
#ifdef CONFIG_CAM_CLK
#include "camclk.h"
#include "drv_camclk_Api.h"
#endif
#include "mhal_miu_client.h"

#define _phy_to_miu_offset(MiuSel, Offset, PhysAddr)                              \
    if (PhysAddr < ARM_MIU1_BASE_ADDR)                                            \
    {                                                                             \
        MiuSel = E_MIU_0;                                                         \
        Offset = PhysAddr - ARM_MIU0_BASE_ADDR;                                   \
    }                                                                             \
    else if ((PhysAddr >= ARM_MIU1_BASE_ADDR) && (PhysAddr < ARM_MIU2_BASE_ADDR)) \
    {                                                                             \
        MiuSel = E_MIU_1;                                                         \
        Offset = PhysAddr - ARM_MIU1_BASE_ADDR;                                   \
    }                                                                             \
    else                                                                          \
    {                                                                             \
        MiuSel = E_MIU_2;                                                         \
        Offset = PhysAddr - ARM_MIU2_BASE_ADDR;                                   \
    }

#define MIU_HAL_ERR(fmt, args...) printk(KERN_ERR "miu hal error %s:%d" fmt, __FUNCTION__, __LINE__, ##args)

static MS_U16 clientId_KernelProtect[IDNUM_KERNELPROTECT] = {
    MIU_CLIENT_CPU_W,    MIU_CLIENT_MCU51_RW, MIU_CLIENT_USB20_H_RW, MIU_CLIENT_USB20_RW,
    MIU_CLIENT_EMAC_RW,  MIU_CLIENT_SD30_RW,  MIU_CLIENT_SDIO30_RW,  MIU_CLIENT_AESDMA_RW,
    MIU_CLIENT_URDMA_RW, MIU_CLIENT_BDMA_RW,  MIU_CLIENT_MOVDMA0_RW, 0,
};

#if defined(CONFIG_ARM) || defined(CONFIG_MIPS)
static MS_U32 m_u32MiuMapBase = 0xFD200000UL; // default set to arm 32bit platform
#elif defined(CONFIG_ARM64)
extern ptrdiff_t mstar_pm_base;
static ptrdiff_t m_u32MiuMapBase;
#endif

static MS_BOOL IDEnables[MIU_MAX_DEVICE][MIU_MAX_PROTECT_BLOCK][MIU_MAX_PROTECT_ID] = {
    {{0}, {0}, {0}, {0}}}; // ID enable for protect block 0~3
static MS_U16  IDList[MIU_MAX_DEVICE][MIU_MAX_PROTECT_ID]                              = {{0}}; // IDList for protection
static MS_BOOL MmuIDEnables[MIU_MAX_DEVICE][MMU_MAX_PROTECT_BLOCK][MIU_MAX_PROTECT_ID] = {
    {{0}, {0}, {0}, {0}}};                                               // ID enable for protect block 0~3
static MS_U16     MmuIDList[MIU_MAX_DEVICE][MIU_MAX_PROTECT_ID] = {{0}}; // IDList for protection
static MS_U8      m_u8PgszMode                                  = 0;
static MS_S16     s16RegionRec                                  = -1;
static MS_S16     s16RepRegionRec                               = -1;
struct miu_client miu0_clients[]                                = {
    {"OVERALL   ", 0x00, 0},
    {"GE        ", 0x01, 0},
    {"BDMA      ", 0x02, 0},
    {"MOVDMA    ", 0x03, 0},
    {"AESDMA    ", 0x04, 0},
    {"CMDQ0_R   ", 0x05, 0},
    {"CMDQ1_R   ", 0x06, 0},
    {"URDMA     ", 0x07, 0},
    {"CSI_TX_R  ", 0x08, 0},
    {"JPE_R     ", 0x09, 0},
    {"JPE_W     ", 0x0A, 0},
    {"JPD       ", 0x0B, 0},
    {"BACH      ", 0x0C, 0},
    {"BACH1     ", 0x0D, 0},
    {"IVE       ", 0x0E, 0},
    {"MCU51      ", 0x0F, 0},
    {"USB30     ", 0x10, 0},
    {"USB20     ", 0x11, 0},
    {"USB20_H   ", 0x12, 0},
    {"SD30      ", 0x13, 0},
    {"SDIO30    ", 0x14, 0},
    {"RSVD      ", 0x15, 1},
    {"SATA      ", 0x16, 0},
    {"EMAC      ", 0x17, 0},
    {"EMAC1     ", 0x18, 0},
    {"LDC       ", 0x19, 0},
    {"GOP       ", 0x1A, 0},
    {"GOP1_R    ", 0x1B, 0},
    {"GOP2_R     ", 0x1C, 0},
    {"GOP3_R    ", 0x1D, 0},
    {"GOP4_R    ", 0x1E, 0},
    {"GOP5_R    ", 0x1F, 0},
    {"VCODEC_R  ", 0x20, 0},
    {"VCODEC_W  ", 0x21, 0},
    {"ISP_ROT_R ", 0x22, 0},
    {"ISP_MOT0W ", 0x23, 0},
    {"ISP_MOT0R ", 0x24, 0},
    {"ISP_MLOAD ", 0x25, 0},
    {"GOP_DISP3 ", 0x26, 0},
    {"RSVD      ", 0x27, 1},
    {"RDMA_DIP  ", 0x28, 0},
    {"SC0_FRM_R ", 0x29, 0},
    {"SC0_FRM_W ", 0x2A, 0},
    {"SC1_FRM_W ", 0x2B, 0},
    {"SC2_FRM_W ", 0x2C, 0},
    {"SC3_FRM_RW", 0x2D, 0},
    {"SC4_FRM_RW", 0x2E, 0},
    {"SC5_FRM_RW", 0x2F, 0},
    {"RSVD      ", 0x30, 1},
    {"ISP_DMA_W ", 0x31, 0},
    {"ISP_DMA_R ", 0x32, 0},
    {"3DNR0_W   ", 0x33, 0},
    {"3DNR0_R   ", 0x34, 0},
    {"RSVD      ", 0x35, 1},
    {"PQ_VIP_DISP_W", 0x36, 1},
    {"DISP_CVBS_W", 0x37, 1},
    {"ISP_DMAG  ", 0x38, 0},
    {"GOP_DISP0 ", 0x39, 0},
    {"GOP_DISP1 ", 0x3A, 0},
    {"GOP_DISP2 ", 0x3B, 0},
    {"MOPROT0_Y ", 0x3C, 0},
    {"MOPROT0_C ", 0x3D, 0},
    {"MOPROT1_Y ", 0x3E, 0},
    {"MOPROT1_C ", 0x3F, 0},
    {"CPU_W     ", CPU_W_CLIENT_ID, 0},
    {"DLAHIWAY_R", DLA_HIWAY_R_ID, 0},
    {"DLAHIWAY_W", DLA_HIWAY_W_ID, 0},
    {"CPU_R     ", CPU_R_CLIENT_ID, 0},
    {"CLIENT_NR ", 0x80, 0, 0, 0, 0},
};

//-------------------------------------------------------------------------------------------------
//  MTLB HAL internal function
//-------------------------------------------------------------------------------------------------
const char *halClientIDTName(int id)
{
    int i = 0;
    for (i = 0; i < MIU0_CLIENT_NUM; i++)
    {
        if (miu0_clients[i].bw_client_id == id)
        {
            return miu0_clients[i].name;
        }
    }
    return NULL;
}

const char *halWriteClientIDTName(int id)
{
    return NULL;
}

const char *halReadClientIDTName(int id)
{
    return NULL;
}
static MS_U32 HAL_MIU_BA2PA(MS_U32 u32BusAddr)
{
    MS_PHYADDR u32PhyAddr = 0x0UL;

    // pa = ba - offset
    if ((u32BusAddr >= ARM_MIU0_BUS_BASE) && (u32BusAddr < ARM_MIU1_BUS_BASE))
        u32PhyAddr = u32BusAddr - ARM_MIU0_BUS_BASE + ARM_MIU0_BASE_ADDR;
    else if ((u32BusAddr >= ARM_MIU1_BUS_BASE) && (u32BusAddr < ARM_MIU2_BUS_BASE))
        u32PhyAddr = u32BusAddr - ARM_MIU1_BUS_BASE + ARM_MIU1_BASE_ADDR;
    else
        u32PhyAddr = u32BusAddr - ARM_MIU2_BUS_BASE + ARM_MIU2_BASE_ADDR;

    return u32PhyAddr;
}

static MS_S16 HAL_MIU_GetClientIndex(MS_U8 u8MiuSel, eMIUClientID eClientID)
{
    MS_U8 idx = 0;

    if (MIU_MAX_DEVICE <= u8MiuSel)
    {
        MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
        return (-1);
    }

    for (idx = 0; idx < MIU0_CLIENT_NUM; idx++)
    {
        if (eClientID == miu0_clients[idx].bw_client_id)
            return eClientID;
    }
    return (-1);
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

static void HAL_MIU_Write2BytesMask(MS_U32 u32RegOffset, MS_U16 u16Mask, MS_BOOL bEnable)
{
    MS_U16 u16Val = HAL_MIU_Read2Byte(u32RegOffset);

    u16Val = (bEnable) ? (u16Val | u16Mask) : (u16Val & ~u16Mask);
    HAL_MIU_Write2Byte(u32RegOffset, u16Val);
}

static void HAL_MIU_SetProtectIDReg(MS_U32 u32RegBase, MS_U8 u8MiuSel, MS_U16 u16ClientID)
{
    MS_S16 sVal   = HAL_MIU_GetClientIndex(u8MiuSel, (eMIUClientID)u16ClientID);
    MS_S16 sIDVal = 0;

    if (0 > sVal)
    {
        sVal = 0;
    }

    sIDVal = HAL_MIU_ReadByte(u32RegBase);
    sIDVal &= 0x80;
    sIDVal |= sVal;
    HAL_MIU_WriteByte(u32RegBase, sIDVal);
}

static MS_BOOL HAL_MIU_SetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                  MS_U32 u32RegProtectIdEn)
{
    MS_U32 u32index0, u32index1;
    MS_U16 u16ID       = 0;
    MS_U16 u16IdEnable = 0;
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
        if (u16ID == 0)
            continue;

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
                if (IDList[u8MiuSel][u32index1] == 0)
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

    u16IdEnable = 0;

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        if (IDEnables[u8MiuSel][u8Blockx][u32index0] == 1)
        {
            u16IdEnable |= (1 << u32index0);
        }
    }

    HAL_MIU_Write2Byte(u32RegProtectIdEn, u16IdEnable);

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, IDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MMU_SetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                  MS_U32 u32RegProtectIdEn)
{
    MS_U32 u32index0, u32index1;
    MS_U16 u16ID       = 0;
    MS_U16 u16IdEnable = 0;
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
        if (u16ID == 0)
            continue;

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
                if (MmuIDList[u8MiuSel][u32index1] == 0)
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

    u16IdEnable = 0;

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        if (MmuIDEnables[u8MiuSel][u8Blockx][u32index0] == 1)
        {
            u16IdEnable |= (1 << u32index0);
        }
    }

    /*Enable the ID register*/
    HAL_MIU_Write2Byte(u32RegProtectIdEn, u16IdEnable);

    /*Add protection client ID*/
    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, MmuIDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MIU_ResetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                    MS_U32 u32RegProtectIdEn)
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

    HAL_MIU_Write2Byte(u32RegProtectIdEn, u16IdEnable);

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
            IDList[u8MiuSel][u32index0] = 0;
        }
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, IDList[u8MiuSel][u32index0]);
    }

    return TRUE;
}

static MS_BOOL HAL_MMU_ResetGroupID(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32RegAddrID,
                                    MS_U32 u32RegProtectIdEn)
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

    HAL_MIU_Write2Byte(u32RegProtectIdEn, u16IdEnable);

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
            MmuIDList[u8MiuSel][u32index0] = 0;
        }
    }

    for (u32index0 = 0; u32index0 < MIU_MAX_PROTECT_ID; u32index0++)
    {
        HAL_MIU_SetProtectIDReg(u32RegAddrID + u32index0, u8MiuSel, MmuIDList[u8MiuSel][u32index0]);
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
    MS_U16 u16Ret        = 0;
    MS_U16 u16LoAddr     = 0;
    MS_U16 u16HiAddr     = 0;
    MS_U32 u32RegBase    = (u8MiuSel) ? MIU1_REG_BASE : MIU_REG_BASE;
    MS_U16 u16MmuRet     = 0;
    MS_U16 u16MmuLoAddr  = 0;
    MS_U16 u16MmuHiAddr  = 0;
    MS_U32 u32MmuRegBase = MIU_MMU_REG_BASE;
    MS_U32 u32EndAddr    = 0;
    char   clientName[40];
    if (!pInfo)
    {
        return FALSE;
    }

    u16Ret    = HAL_MIU_Read2Byte(u32RegBase + REG_MIU_PROTECT_STATUS);
    u16LoAddr = HAL_MIU_Read2Byte(u32RegBase + REG_MIU_PROTECT_LOADDR);
    u16HiAddr = HAL_MIU_Read2Byte(u32RegBase + REG_MIU_PROTECT_HIADDR);

    u16MmuRet    = HAL_MIU_Read2Byte(u32MmuRegBase + REG_MMU_PROTECT_STATUS);
    u16MmuLoAddr = HAL_MIU_Read2Byte(u32MmuRegBase + REG_MMU_PROTECT_LOADDR);
    u16MmuHiAddr = HAL_MIU_Read2Byte(u32MmuRegBase + REG_MMU_PROTECT_HIADDR);

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
        strcpy(clientName, (char *)halClientIDTName((MS_U8)(GET_MMU_HIT_CLIENT(u16MmuRet))));
        printk("MIU%u Block:%u Client:%s ID:%u-%u Hitted_Address(MMU):0x%x<->0x%x\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(u32MmuRegBase + REG_MMU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, TRUE);
        HAL_MIU_Write2BytesMask(u32MmuRegBase + REG_MMU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, FALSE);
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

        strcpy(clientName, (char *)halClientIDTName((MS_U8)(GET_HIT_CLIENT(u16Ret))));
        printk("MIU%u Block:%u Client:%s ID:%u-%u Hitted_Address(MIU):0x%x<->0x%x\n", u8MiuSel, pInfo->u8Block,
               clientName, pInfo->u8Group, pInfo->u8ClientID, pInfo->uAddress, u32EndAddr);

        // Clear log
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, TRUE);
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, FALSE);
    }

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
/// @param u8Blockx        \b IN     : MIU Block to protect (0 ~ 4)
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
MS_BOOL HAL_MIU_Protect(MS_U8 u8Blockx, MS_U16 *pu8ProtectId, MS_U32 u32BusStart, MS_U32 u32BusEnd, MS_BOOL bSetFlag)
{
    MS_U32 u32RegProtectId   = 0;
    MS_U32 u32RegBase        = 0;
    MS_U32 u32RegStartAddr   = 0;
    MS_U32 u32RegStartEnd    = 0;
    MS_U32 u32RegAddrMSB     = 0;
    MS_U32 u32RegProtectIdEn = 0;
    MS_U32 u32RegRWProtectEn = 0;
    MS_U32 u32StartOffset    = 0;
    MS_U32 u32EndOffset      = 0;
    MS_U8  u8MiuSel          = 0;
    MS_U16 u16Data           = 0;
    MS_U16 u16Data1          = 0;
    MS_U16 u16Data2          = 0;
    MS_U8  u8Data            = 0;
    MS_U32 u32Start = 0, u32End = 0;
    MS_U16 u16Region   = 0;
    MS_U8  u8MaxRegion = 0;
    MS_U32 u32DramSize = 0;
    u32DramSize        = HAL_MIU_ProtectDramSize() / 1024; // 1024Entry
    u32Start           = HAL_MIU_BA2PA(u32BusStart);
    u32End             = HAL_MIU_BA2PA(u32BusEnd);

    // Get MIU selection and offset
    _phy_to_miu_offset(u8MiuSel, u32EndOffset, u32End);
    _phy_to_miu_offset(u8MiuSel, u32StartOffset, u32Start);

    u32Start = u32StartOffset;
    u32End   = u32EndOffset;

    switch (m_u8PgszMode)
    {
        case E_MMU_PGSZ_128:
            u16Region   = MMU_ADDR_TO_REGION_128((unsigned int)u32Start);
            u8MaxRegion = u32DramSize / MMU_PAGE_SIZE_128;
            break;
        case E_MMU_PGSZ_256:
            u16Region   = MMU_ADDR_TO_REGION_256((unsigned int)u32Start);
            u8MaxRegion = u32DramSize / MMU_PAGE_SIZE_256;
            break;
        case E_MMU_PGSZ_512:
            u16Region   = MMU_ADDR_TO_REGION_512((unsigned int)u32Start);
            u8MaxRegion = u32DramSize / MMU_PAGE_SIZE_512;
            break;
        default:
            break;
    }

    // Parameter check
    if (u8Blockx > E_MMU_PROTECT_MAX)
    {
        MIU_HAL_ERR("Err: Out of the number of protect device\n");
        return FALSE;
    }
    else if (((u32Start & ((1 << MIU_PAGE_SHIFT) - 1)) != 0) || ((u32End & ((1 << MIU_PAGE_SHIFT) - 1)) != 0))
    {
        MIU_HAL_ERR("Err: u32Start：0x%x,u32End:0x%x,Protected address should be aligned to 8KB\n", u32Start, u32End);
        return FALSE;
    }
    else if (u32Start >= u32End)
    {
        MIU_HAL_ERR("Err: Start address is equal to or more than end address\n");
        return FALSE;
    }
    if (((s16RepRegionRec == -1) || ((u16Region >= 0) && (u16Region <= u8MaxRegion)))
        && ((u8Blockx >= E_PROTECT_0) && (u8Blockx <= E_MIU_PROTECT_MAX)))
    {
        if (u8MiuSel == E_MIU_0)
        {
            u32RegProtectId = MIU_PROTECT_ID0;
            u32RegBase      = MIU_REG_BASE;
            switch (u8Blockx)
            {
                case E_MIU_BLOCK_0:
                    u8Data            = 1 << 0;
                    u32RegRWProtectEn = MIU_PROTECT0_EN;
                    u32RegStartAddr   = MIU_PROTECT0_START;
                    u32RegStartEnd    = MIU_PROTECT0_END;
                    u32RegProtectIdEn = MIU_PROTECT0_ID_ENABLE;
                    u32RegAddrMSB     = MIU_PROTECT0_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFFF0UL);
                    break;
                case E_MIU_BLOCK_1:
                    u8Data            = 1 << 1;
                    u32RegRWProtectEn = MIU_PROTECT1_EN;
                    u32RegStartAddr   = MIU_PROTECT1_START;
                    u32RegStartEnd    = MIU_PROTECT1_END;
                    u32RegProtectIdEn = MIU_PROTECT1_ID_ENABLE;
                    u32RegAddrMSB     = MIU_PROTECT1_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFF0FUL);
                    break;
                case E_MIU_BLOCK_2:
                    u8Data            = 1 << 2;
                    u32RegRWProtectEn = MIU_PROTECT2_EN;
                    u32RegStartAddr   = MIU_PROTECT2_START;
                    u32RegStartEnd    = MIU_PROTECT2_END;
                    u32RegProtectIdEn = MIU_PROTECT2_ID_ENABLE;
                    u32RegAddrMSB     = MIU_PROTECT2_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xF0FFUL);
                    break;
                case E_MIU_BLOCK_3:
                    u8Data            = 1 << 3;
                    u32RegRWProtectEn = MIU_PROTECT3_EN;
                    u32RegStartAddr   = MIU_PROTECT3_START;
                    u32RegStartEnd    = MIU_PROTECT3_END;
                    u32RegProtectIdEn = MIU_PROTECT3_ID_ENABLE;
                    u32RegAddrMSB     = MIU_PROTECT3_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0x0FFFUL);
                    break;
                case E_MIU_BLOCK_4:
                    u8Data            = 1 << 0;
                    u32RegRWProtectEn = MIU_PROTECT4_EN;
                    u32RegStartAddr   = MIU_PROTECT4_START;
                    u32RegStartEnd    = MIU_PROTECT4_END;
                    u32RegProtectIdEn = MIU_PROTECT4_ID_ENABLE;
                    u32RegAddrMSB     = MIU_PROTECT4_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFFF0UL);
                    break;
                default:
                    MIU_HAL_ERR("%s not support block%u!\n", __FUNCTION__, u8Blockx);
                    return FALSE;
            }
        }
        else if (u8MiuSel == E_MIU_1)
        {
            u32RegProtectId = MIU1_PROTECT_ID0;
            u32RegBase      = MIU1_REG_BASE;

            switch (u8Blockx)
            {
                case E_MIU_BLOCK_0:
                    u8Data            = 1 << 0;
                    u32RegRWProtectEn = MIU1_PROTECT0_EN;
                    u32RegStartAddr   = MIU1_PROTECT0_START;
                    u32RegStartEnd    = MIU1_PROTECT0_END;
                    u32RegProtectIdEn = MIU1_PROTECT0_ID_ENABLE;
                    u32RegAddrMSB     = MIU1_PROTECT0_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFFF0UL);
                    break;
                case E_MIU_BLOCK_1:
                    u8Data            = 1 << 1;
                    u32RegRWProtectEn = MIU1_PROTECT1_EN;
                    u32RegStartAddr   = MIU1_PROTECT1_START;
                    u32RegStartEnd    = MIU1_PROTECT1_END;
                    u32RegProtectIdEn = MIU1_PROTECT1_ID_ENABLE;
                    u32RegAddrMSB     = MIU1_PROTECT1_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFF0FUL);
                    break;
                case E_MIU_BLOCK_2:
                    u8Data            = 1 << 2;
                    u32RegRWProtectEn = MIU1_PROTECT2_EN;
                    u32RegStartAddr   = MIU1_PROTECT2_START;
                    u32RegStartEnd    = MIU1_PROTECT2_END;
                    u32RegProtectIdEn = MIU1_PROTECT2_ID_ENABLE;
                    u32RegAddrMSB     = MIU1_PROTECT2_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xF0FFUL);
                    break;
                case E_MIU_BLOCK_3:
                    u8Data            = 1 << 3;
                    u32RegRWProtectEn = MIU1_PROTECT3_EN;
                    u32RegStartAddr   = MIU1_PROTECT3_START;
                    u32RegStartEnd    = MIU1_PROTECT3_END;
                    u32RegProtectIdEn = MIU1_PROTECT3_ID_ENABLE;
                    u32RegAddrMSB     = MIU1_PROTECT3_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0x0FFFUL);
                    break;
                case E_MIU_BLOCK_4:
                    u8Data            = 1 << 0;
                    u32RegRWProtectEn = MIU1_PROTECT4_EN;
                    u32RegStartAddr   = MIU1_PROTECT4_START;
                    u32RegStartEnd    = MIU1_PROTECT4_END;
                    u32RegProtectIdEn = MIU1_PROTECT4_ID_ENABLE;
                    u32RegAddrMSB     = MIU1_PROTECT4_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFFF0UL);
                    break;
                default:
                    MIU_HAL_ERR("%s not support block%u!\n", __FUNCTION__, u8Blockx);
                    return FALSE;
            }
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
            if (HAL_MIU_SetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn) == FALSE)
            {
                return FALSE;
            }

            // Set BIT29,30 of start/end address
            u16Data2 = u16Data2 | (MS_U16)((u32Start >> 29) << ((u8Blockx % 4) * 4)); // u16Data2 for start_ext addr
            u16Data1 = u16Data2 | (MS_U16)(((u32End - 1) >> 29) << ((u8Blockx % 4) * 4 + 2));
            HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

            // Start Address
            u16Data = (MS_U16)(u32Start >> MIU_PAGE_SHIFT); // 8k unit
            HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

            // End Address
            u16Data = (MS_U16)((u32End >> MIU_PAGE_SHIFT) - 1); // 8k unit;
            HAL_MIU_Write2Byte(u32RegStartEnd, u16Data);

            // Enable MIU write protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MIU_ResetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
        }

        // Clear log
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, TRUE);
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, FALSE);

        // Mask PWR IRQ
        HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_PWR_IRQ_MASK_OFFSET, REG_MIU_PROTECT_PWR_IRQ_MASK_BIT, TRUE);
    }
    else if (((s16RepRegionRec != -1) && (s16RegionRec == u16Region))
             && ((u8Blockx >= E_MMU_PROTECT_0) && (u8Blockx <= E_MMU_PROTECT_MAX)))
    {
        if (u8MiuSel == E_MIU_0)
        {
            u32RegProtectId = MMU_PROTECT_ID0; /*mmu protection client ID*/
            u32RegBase      = MIU_MMU_REG_BASE;
            u8Blockx        = u8Blockx - E_MMU_PROTECT_0;

            switch (u8Blockx)
            {
                case E_MMU_BLOCK_0:
                    u8Data            = (1 << 0) | (1 << 4);
                    u32RegRWProtectEn = MMU_PROTECT0_EN;        /*protection 0~3 write enable,protection 0~3 read
                                                                   enable,protection 0~3 function invert*/
                    u32RegStartAddr   = MMU_PROTECT0_START;     /*protect start address unit 8kbytes*/
                    u32RegStartEnd    = MMU_PROTECT0_END;       /*protect end address unit 8kbytes*/
                    u32RegProtectIdEn = MMU_PROTECT0_ID_ENABLE; /*protect0 ID enable*/
                    u32RegAddrMSB     = MMU_PROTECT0_MSB;       /*protect start and end address unit 8kbytes(Bit18~16)*/
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFF00UL);
                    break;
                case E_MMU_BLOCK_1:
                    u8Data            = (1 << 1) | (1 << 5);
                    u32RegRWProtectEn = MMU_PROTECT1_EN;
                    u32RegStartAddr   = MMU_PROTECT1_START;
                    u32RegStartEnd    = MMU_PROTECT1_END;
                    u32RegProtectIdEn = MMU_PROTECT1_ID_ENABLE;
                    u32RegAddrMSB     = MMU_PROTECT1_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0x00FFUL);
                    break;
                case E_MMU_BLOCK_2:
                    u8Data            = (1 << 2) | (1 << 6);
                    u32RegRWProtectEn = MMU_PROTECT2_EN;
                    u32RegStartAddr   = MMU_PROTECT2_START;
                    u32RegStartEnd    = MMU_PROTECT2_END;
                    u32RegProtectIdEn = MMU_PROTECT2_ID_ENABLE;
                    u32RegAddrMSB     = MMU_PROTECT2_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0xFF00UL);
                    break;
                case E_MMU_BLOCK_3:
                    u8Data            = (1 << 3) | (1 << 7);
                    u32RegRWProtectEn = MMU_PROTECT3_EN;
                    u32RegStartAddr   = MMU_PROTECT3_START;
                    u32RegStartEnd    = MMU_PROTECT3_END;
                    u32RegProtectIdEn = MMU_PROTECT3_ID_ENABLE;
                    u32RegAddrMSB     = MMU_PROTECT3_MSB;
                    u16Data1          = HAL_MIU_Read2Byte(u32RegAddrMSB);
                    u16Data2          = (u16Data1 & 0x00FFUL);
                    break;
                default:
                    MIU_HAL_ERR("%s not support block%u!\n", __FUNCTION__, u8Blockx);
                    return FALSE;
            }
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
            if (HAL_MMU_SetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn) == FALSE)
            {
                return FALSE;
            }

            // Set BIT29,30,31 of start/end address
            // >>29 = >>13(8k unit) >> 16(bit18~16)
            u16Data2 = u16Data2 | (MS_U16)((u32Start >> 29) << ((u8Blockx % 2) * 8)); // u16Data2 for start_ext addr
            u16Data1 = u16Data2 | (MS_U16)(((u32End - 1) >> 29) << ((u8Blockx % 2) * 8 + 4));
            HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

            // Start Address
            u16Data = (MS_U16)(u32Start >> MIU_PAGE_SHIFT); // 8k unit
            HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

            // End Address
            u16Data = (MS_U16)((u32End >> MIU_PAGE_SHIFT) - 1); // 8k unit;
            HAL_MIU_Write2Byte(u32RegStartEnd, u16Data);

            // Enable MMU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MMU_ResetGroupID(u8MiuSel, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
        }

        // Clear log
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MMU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, TRUE);
        HAL_MIU_Write2BytesMask(u32RegBase + REG_MMU_PROTECT_STATUS, REG_MMU_PROTECT_LOG_CLR, FALSE);
    }
    else
    {
        MIU_HAL_ERR(
            "%s u32Start：0x%x,u32End:0x%x.The address is neither a physical address nor a virtual address,Please "
            "check it!\n",
            __FUNCTION__, u32Start, u32End);
        return FALSE;
    }

    return TRUE;
}

MS_BOOL HAL_SetAccessFromVpaOnly(MS_U8 u8Blockx, MS_U32 u32BusStart, MS_U32 u32BusEnd, MS_BOOL bSetFlag)
{
    MS_U32 u32RegBase        = 0;
    MS_U32 u32RegStartAddr   = 0;
    MS_U32 u32RegStartEnd    = 0;
    MS_U32 u32RegAddrMSB     = 0;
    MS_U32 u32RegRWProtectEn = 0;
    MS_U32 u32StartOffset    = 0;
    MS_U32 u32EndOffset      = 0;
    MS_U8  u8MiuSel          = 0;
    MS_U16 u16Data           = 0;
    MS_U16 u16Data1          = 0;
    MS_U16 u16Data2          = 0;
    MS_U8  u8Data            = 0;
    MS_U32 u32Start = 0, u32End = 0;
    MS_U16 u16ChkFlgData       = 0;
    MS_U32 u32RegMmuProtChkFlg = 0;

    u32Start = HAL_MIU_BA2PA(u32BusStart);
    u32End   = HAL_MIU_BA2PA(u32BusEnd);

    // Get MIU selection and offset
    _phy_to_miu_offset(u8MiuSel, u32EndOffset, u32End);
    _phy_to_miu_offset(u8MiuSel, u32StartOffset, u32Start);

    u32Start = u32StartOffset;
    u32End   = u32EndOffset;

    // Parameter check
    if (u8Blockx > E_MIU_PROTECT_MAX)
    {
        MIU_HAL_ERR("Err: PA - Out of the number of PA protect device\n");
        return FALSE;
    }
    else if (((u32Start & ((1 << MIU_PAGE_SHIFT) - 1)) != 0) || ((u32End & ((1 << MIU_PAGE_SHIFT) - 1)) != 0))
    {
        MIU_HAL_ERR("Err: PA - Protected address should be aligned to 8KB\n");
        return FALSE;
    }
    else if (u32Start >= u32End)
    {
        MIU_HAL_ERR("Err: PA - Start address is equal to or more than end address\n");
        return FALSE;
    }

    if (u8MiuSel == E_MIU_0)
    {
        u32RegBase = MIU_REG_BASE;
        switch (u8Blockx)
        {
            case E_MIU_BLOCK_0:
                u8Data              = (1 << 0) | (1 << 4);  /*protection 0 write/read enable*/
                u16ChkFlgData       = 1 << 11;              /*protection 0 mmu flag check enable*/
                u32RegMmuProtChkFlg = MMU_PROTECT0_CHK_FLG; /*protection 0 mmu check flag control register*/
                u32RegRWProtectEn   = MIU_PROTECT0_EN;      /*protection 0 write/read/invert enable control register*/
                u32RegStartAddr     = MIU_PROTECT0_START;   /*protect start address unit 8kbytes*/
                u32RegStartEnd      = MIU_PROTECT0_END;     /*protect end address unit 8kbytes*/
                u32RegAddrMSB       = MIU_PROTECT0_MSB;     /*protect start and end address unit 8kbytes(Bit17~16)*/
                u16Data1            = HAL_MIU_Read2Byte(u32RegAddrMSB);
                u16Data2            = (u16Data1 & 0xFFF0UL);
                break;
            case E_MIU_BLOCK_1:
                u8Data              = (1 << 1) | (1 << 5);
                u16ChkFlgData       = 1 << 12;              /*protection 1 mmu flag check enable*/
                u32RegMmuProtChkFlg = MMU_PROTECT1_CHK_FLG; /*protection 1 mmu check flag control register*/
                u32RegRWProtectEn   = MIU_PROTECT1_EN;      /*protection 1 write/read/invert enable control register*/
                u32RegStartAddr     = MIU_PROTECT1_START;   /*protect start address unit 8kbytes*/
                u32RegStartEnd      = MIU_PROTECT1_END;     /*protect end address unit 8kbytes*/
                u32RegAddrMSB       = MIU_PROTECT1_MSB;     /*protect start and end address unit 8kbytes(Bit17~16)*/
                u16Data1            = HAL_MIU_Read2Byte(u32RegAddrMSB);
                u16Data2            = (u16Data1 & 0xFFF0UL);
                break;
            case E_MIU_BLOCK_2:
                u8Data              = (1 << 2) | (1 << 6);
                u16ChkFlgData       = 1 << 13;              /*protection 2 mmu flag check enable*/
                u32RegMmuProtChkFlg = MMU_PROTECT2_CHK_FLG; /*protection 2 mmu check flag control register*/
                u32RegRWProtectEn   = MIU_PROTECT2_EN;      /*protection 2 write/read/invert enable control register*/
                u32RegStartAddr     = MIU_PROTECT2_START;   /*protect start address unit 8kbytes*/
                u32RegStartEnd      = MIU_PROTECT2_END;     /*protect end address unit 8kbytes*/
                u32RegAddrMSB       = MIU_PROTECT2_MSB;     /*protect start and end address unit 8kbytes(Bit17~16)*/
                u16Data1            = HAL_MIU_Read2Byte(u32RegAddrMSB);
                u16Data2            = (u16Data1 & 0xFFF0UL);
                break;
            case E_MIU_BLOCK_3:
                u8Data              = (1 << 3) | (1 << 7);
                u16ChkFlgData       = 1 << 14;              /*protection 3 mmu flag check enable*/
                u32RegMmuProtChkFlg = MMU_PROTECT3_CHK_FLG; /*protection 3 mmu check flag control register*/
                u32RegRWProtectEn   = MIU_PROTECT3_EN;      /*protection 3 write/read/invert enable control register*/
                u32RegStartAddr     = MIU_PROTECT3_START;   /*protect start address unit 8kbytes*/
                u32RegStartEnd      = MIU_PROTECT3_END;     /*protect end address unit 8kbytes*/
                u32RegAddrMSB       = MIU_PROTECT3_MSB;     /*protect start and end address unit 8kbytes(Bit17~16)*/
                u16Data1            = HAL_MIU_Read2Byte(u32RegAddrMSB);
                u16Data2            = (u16Data1 & 0xFFF0UL);
                break;
            case E_MIU_BLOCK_4:
                u8Data              = (1 << 0) | (1 << 1);
                u16ChkFlgData       = 1 << 15;              /*protection 4 mmu flag check enable*/
                u32RegMmuProtChkFlg = MMU_PROTECT4_CHK_FLG; /*protection 4 mmu check flag control register*/
                u32RegRWProtectEn   = MIU_PROTECT4_EN;      /*protection 4 write/read/invert enable control register*/
                u32RegStartAddr     = MIU_PROTECT4_START;   /*protect start address unit 8kbytes*/
                u32RegStartEnd      = MIU_PROTECT4_END;     /*protect end address unit 8kbytes*/
                u32RegAddrMSB       = MIU_PROTECT4_MSB;     /*protect start and end address unit 8kbytes(Bit17~16)*/
                u16Data1            = HAL_MIU_Read2Byte(u32RegAddrMSB);
                u16Data2            = (u16Data1 & 0xFFF0UL);
                break;
            default:
                return FALSE;
        }
    }
    else
    {
        MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
        return FALSE;
    }

    // Disable MIU write/read protect
    HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);

    /* Clear mmu check flag check */
    HAL_MIU_Write2BytesMask(u32RegMmuProtChkFlg, u16ChkFlgData, DISABLE);

    if (bSetFlag)
    {
        /* Set mmu check flag check */
        HAL_MIU_Write2BytesMask(u32RegMmuProtChkFlg, u16ChkFlgData, ENABLE);

        // Set BIT29,30,31 of start/end address
        //>>29 = >>13(8k unit) >> 16(bit17~16)
        u16Data2 = u16Data2 | (MS_U16)((u32Start >> 29) << ((u8Blockx % 4) * 4)); // u16Data2 for start_ext addr
        u16Data1 = u16Data2 | (MS_U16)(((u32End - 1) >> 29) << ((u8Blockx % 4) * 4 + 2));
        HAL_MIU_Write2Byte(u32RegAddrMSB, u16Data1);

        // Start Address
        u16Data = (MS_U16)(u32Start >> MIU_PAGE_SHIFT); // 8k unit
        HAL_MIU_Write2Byte(u32RegStartAddr, u16Data);

        // End Address
        u16Data = (MS_U16)((u32End >> MIU_PAGE_SHIFT) - 1); // 8k unit;
        HAL_MIU_Write2Byte(u32RegStartEnd, u16Data);

        // Enable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
    }

    // Clear log
    HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, TRUE);
    HAL_MIU_Write2BytesMask(u32RegBase + REG_MIU_PROTECT_STATUS, REG_MIU_PROTECT_LOG_CLR, FALSE);

    // Mask PWR IRQ
    HAL_MIU_Write2BytesMask(REG_MIU_PROTECT_PWR_IRQ_MASK_OFFSET, REG_MIU_PROTECT_PWR_IRQ_MASK_BIT, TRUE);

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
    MS_U8  u8Data            = 0;
    MS_U8  u8InvertData      = 0;
    MS_U32 u32RegInvert      = 0;
    MS_U32 u32RegRWProtectEn = 0;
    MS_U32 u32RegProtectId   = 0;
    MS_U32 u32RegProtectIdEn = 0;

    if (u8ModeSel == 0)
    {
        if (u8Blockx >= E_MIU_BLOCK_NUM)
        {
            MIU_HAL_ERR("Err: MIU Blk Num out of range\n");
            return FALSE;
        }

        u32RegProtectId = MIU_PROTECT_ID0;
        switch (u8Blockx)
        {
            case E_MIU_BLOCK_0:
                u8Data            = (1 << 0) | (1 << 4);
                u8InvertData      = (1 << 0);
                u32RegInvert      = MIU_PROTECT0_INVERT;
                u32RegRWProtectEn = MIU_PROTECT0_EN;
                u32RegProtectIdEn = MIU_PROTECT0_ID_ENABLE;
                break;
            case E_MIU_BLOCK_1:
                u8Data            = (1 << 1) | (1 << 5);
                u8InvertData      = (1 << 1);
                u32RegInvert      = MIU_PROTECT1_INVERT;
                u32RegRWProtectEn = MIU_PROTECT1_EN;
                u32RegProtectIdEn = MIU_PROTECT1_ID_ENABLE;
                break;
            case E_MIU_BLOCK_2:
                u8Data            = (1 << 2) | (1 << 6);
                u8InvertData      = (1 << 2);
                u32RegInvert      = MIU_PROTECT2_INVERT;
                u32RegRWProtectEn = MIU_PROTECT2_EN;
                u32RegProtectIdEn = MIU_PROTECT2_ID_ENABLE;
                break;
            case E_MIU_BLOCK_3:
                u8Data            = (1 << 3) | (1 << 7);
                u8InvertData      = (1 << 3);
                u32RegInvert      = MIU_PROTECT3_INVERT;
                u32RegRWProtectEn = MIU_PROTECT3_EN;
                u32RegProtectIdEn = MIU_PROTECT3_ID_ENABLE;
                break;
            case E_MIU_BLOCK_4:
                u8Data            = (1 << 0) | (1 << 1);
                u8InvertData      = (1 << 2);
                u32RegInvert      = MIU_PROTECT4_INVERT;
                u32RegRWProtectEn = MIU_PROTECT4_EN;
                u32RegProtectIdEn = MIU_PROTECT4_ID_ENABLE;
                break;
            default:
                MIU_HAL_ERR("%s miu not support block%u!\n", __FUNCTION__, u8Blockx);
                return FALSE;
        }
        // Disable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);
        if (bSetFlag)
        {
            if (!bIdFlag)
            {
                // Reset Protect IDList
                HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
            }
            // Enable MIU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MIU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
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
        if (u8Blockx >= E_MMU_BLOCK_NUM)
        {
            MIU_HAL_ERR("Err: MMU Blk Num out of range\n");
            return FALSE;
        }

        u32RegProtectId = MMU_PROTECT_ID0;
        switch (u8Blockx)
        {
            case E_MMU_BLOCK_0:
                u8Data            = (1 << 0) | (1 << 4);
                u8InvertData      = (1 << 0);
                u32RegInvert      = MMU_PROTECT0_INVERT;
                u32RegRWProtectEn = MMU_PROTECT0_EN;
                u32RegProtectIdEn = MMU_PROTECT0_ID_ENABLE;
                break;
            case E_MMU_BLOCK_1:
                u8Data            = (1 << 1) | (1 << 5);
                u8InvertData      = (1 << 1);
                u32RegInvert      = MMU_PROTECT1_INVERT;
                u32RegRWProtectEn = MMU_PROTECT1_EN;
                u32RegProtectIdEn = MMU_PROTECT1_ID_ENABLE;
                break;
            case E_MMU_BLOCK_2:
                u8Data            = (1 << 2) | (1 << 6);
                u8InvertData      = (1 << 2);
                u32RegInvert      = MMU_PROTECT2_INVERT;
                u32RegRWProtectEn = MMU_PROTECT2_EN;
                u32RegProtectIdEn = MMU_PROTECT2_ID_ENABLE;
                break;
            case E_MMU_BLOCK_3:
                u8Data            = (1 << 3) | (1 << 7);
                u8InvertData      = (1 << 3);
                u32RegInvert      = MMU_PROTECT3_INVERT;
                u32RegRWProtectEn = MMU_PROTECT3_EN;
                u32RegProtectIdEn = MMU_PROTECT3_ID_ENABLE;
                break;
            default:
                MIU_HAL_ERR("%s mmu not support block%u!\n", __FUNCTION__, u8Blockx);
                return FALSE;
        }
        // Disable MIU write/read protect
        HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, DISABLE);
        if (bSetFlag)
        {
            if (!bIdFlag)
            {
                // Reset Protect IDList
                HAL_MMU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
            }
            // Enable MIU write/read protect
            HAL_MIU_WriteByteMask(u32RegRWProtectEn, u8Data, ENABLE);
        }
        else
        {
            // Reset Protect IDList
            HAL_MMU_ResetGroupID(0, u8Blockx, pu8ProtectId, u32RegProtectId, u32RegProtectIdEn);
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
    MS_U8  u8MiuSel          = 0;
    MS_U8  u8Blockx          = 0;
    MS_U8  u8ClientID        = 0;
    MS_U16 u16IdEnable       = 0;
    MS_U32 u32index          = 0;
    MS_U32 u32RegProtectId   = 0;
    MS_U32 u32RegProtectIdEn = 0;

    for (u8MiuSel = E_MIU_0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
    {
        for (u8Blockx = E_MIU_BLOCK_0; u8Blockx < E_MIU_BLOCK_NUM; u8Blockx++)
        {
            if (u8MiuSel == E_MIU_0)
            {
                u32RegProtectId = MIU_PROTECT_ID0;

                switch (u8Blockx)
                {
                    case E_MIU_BLOCK_0:
                        u32RegProtectIdEn = MIU_PROTECT0_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_1:
                        u32RegProtectIdEn = MIU_PROTECT1_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_2:
                        u32RegProtectIdEn = MIU_PROTECT2_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_3:
                        u32RegProtectIdEn = MIU_PROTECT3_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_4:
                        u32RegProtectIdEn = MIU_PROTECT4_ID_ENABLE;
                        break;
                    default:
                        return FALSE;
                }
            }
            else if (u8MiuSel == E_MIU_1)
            {
                u32RegProtectId = MIU1_PROTECT_ID0;

                switch (u8Blockx)
                {
                    case E_MIU_BLOCK_0:
                        u32RegProtectIdEn = MIU1_PROTECT0_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_1:
                        u32RegProtectIdEn = MIU1_PROTECT1_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_2:
                        u32RegProtectIdEn = MIU1_PROTECT2_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_3:
                        u32RegProtectIdEn = MIU1_PROTECT3_ID_ENABLE;
                        break;
                    case E_MIU_BLOCK_4:
                        u32RegProtectIdEn = MIU1_PROTECT4_ID_ENABLE;
                        break;
                    default:
                        return FALSE;
                }
            }
            else
            {
                MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
                return FALSE;
            }

            u16IdEnable = HAL_MIU_Read2Byte(u32RegProtectIdEn);

            for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
            {
                IDEnables[u8MiuSel][u8Blockx][u32index] = ((u16IdEnable >> u32index) & 0x1UL) ? 1 : 0;
            }
        }

        for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
        {
            u8ClientID                 = HAL_MIU_ReadByte(u32RegProtectId + u32index) & 0x7F;
            IDList[u8MiuSel][u32index] = u8ClientID;
        }
    }

    for (u8MiuSel = E_MIU_0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
    {
        for (u8Blockx = E_MMU_BLOCK_0; u8Blockx < E_MMU_BLOCK_NUM; u8Blockx++)
        {
            if (u8MiuSel == E_MIU_0)
            {
                u32RegProtectId = MMU_PROTECT_ID0;

                switch (u8Blockx)
                {
                    case E_MMU_BLOCK_0:
                        u32RegProtectIdEn = MMU_PROTECT0_ID_ENABLE;
                        break;
                    case E_MMU_BLOCK_1:
                        u32RegProtectIdEn = MMU_PROTECT1_ID_ENABLE;
                        break;
                    case E_MMU_BLOCK_2:
                        u32RegProtectIdEn = MMU_PROTECT2_ID_ENABLE;
                        break;
                    case E_MMU_BLOCK_3:
                        u32RegProtectIdEn = MMU_PROTECT3_ID_ENABLE;
                        break;
                    default:
                        return FALSE;
                }
            }
            else
            {
                MIU_HAL_ERR("%s not support MIU%u!\n", __FUNCTION__, u8MiuSel);
                return FALSE;
            }

            u16IdEnable = HAL_MIU_Read2Byte(u32RegProtectIdEn);

            for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
            {
                MmuIDEnables[u8MiuSel][u8Blockx][u32index] = ((u16IdEnable >> u32index) & 0x1UL) ? 1 : 0;
            }
        }

        for (u32index = 0; u32index < MIU_MAX_PROTECT_ID; u32index++)
        {
            u8ClientID                    = HAL_MIU_ReadByte(u32RegProtectId + u32index) & 0x7F;
            MmuIDList[u8MiuSel][u32index] = u8ClientID;
        }
    }

    return TRUE;
}

unsigned int HAL_MIU_ProtectDramSize(void)
{
    MS_U8 u8Val = HAL_MIU_ReadByte(MIU_PROTECT_DDR_SIZE);

    u8Val = (u8Val >> 4) & 0xF;

    if (0 == u8Val)
    {
        MIU_HAL_ERR("MIU protect DRAM size is undefined. Using 0x40000000 as default\n");
        return 0x40000000;
    }
    return (0x1 << (20 + u8Val));
}

int HAL_MIU_Info(MIU_DramInfo_Hal *pDramInfo)
{
    int          ret    = -1;
    unsigned int ddfset = 0;

    if (pDramInfo)
    {
        ddfset =
            (INREGMSK16(BASE_REG_ATOP_PA + REG_ID_19, 0x00FF) << 16) + INREGMSK16(BASE_REG_ATOP_PA + REG_ID_18, 0xFFFF);

        pDramInfo->size        = HAL_MIU_ProtectDramSize();
        pDramInfo->dram_freq   = ((432 * 4 * 4) << 19) / ddfset;
        pDramInfo->miupll_freq = 24 * INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x00FF)
                                 / ((INREGMSK16(BASE_REG_MIUPLL_PA + REG_ID_03, 0x0700) >> 8) + 2);
        pDramInfo->type = INREGMSK16(BASE_REG_MIU_PA + REG_ID_01, 0x0003);
        ;
        pDramInfo->data_rate = (1 << (INREGMSK16(BASE_REG_MIU_PA + REG_ID_01, 0x0300) >> 8));
        pDramInfo->bus_width = (16 << (INREGMSK16(BASE_REG_MIU_PA + REG_ID_01, 0x000C) >> 2));
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
        case E_MMU_PGSZ_128:
            u16CtrlRegVal &= REG_MMU_CTRL_PG_SIZE_128K;
            break;
        case E_MMU_PGSZ_256:
            u16CtrlRegVal |= REG_MMU_CTRL_PG_SIZE_256K;
            break;
        case E_MMU_PGSZ_512:
            u16CtrlRegVal |= REG_MMU_CTRL_PG_SIZE_512K;
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
        case E_MMU_PGSZ_128:
            u8RegShiftVal = 5; // 128KB:[31:27],region control 5bit
            break;
        case E_MMU_PGSZ_256:
            u8RegShiftVal = 4; // 256KB:[31:28],region control 4bit
            break;
        case E_MMU_PGSZ_512:
            u8RegShiftVal = 3; // 512KB:[31:29],region control 3bit
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

    u16CtrlRegVal = HAL_MIU_Read2Byte(REG_MMU_CTRL) & ~(BITS_RANGE(REG_MMU_CTRL_REGION_MASK))
                    & ~(BITS_RANGE(REG_MMU_CTRL_RP_REGION_MASK));
    u16CtrlRegVal |= (u16Region << 7);
    u16CtrlRegVal |= (u16ReplaceRegion << 12);

    HAL_MIU_Write2Byte(REG_MMU_CTRL, u16CtrlRegVal);

    return 0;
}

int HAL_MMU_Map(unsigned short u16VirtAddrEntry, unsigned short u16PhyAddrEntry)
{
    MS_U16 u16RegVal;

    if ((u16PhyAddrEntry >> 10) || (u16VirtAddrEntry >> 10))
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x, Virt:0x%x)\n", u16PhyAddrEntry, u16VirtAddrEntry);
        return -1;
    }

    // reg_mmu_wdata
    HAL_MIU_Write2Byte(REG_MMU_W_DATA, u16PhyAddrEntry);

    // reg_mmu_entry
    u16RegVal = REG_MMU_RW_ENTRY_MODE | u16VirtAddrEntry;
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16RegVal);

    // reg_mmu_access
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, BIT0);

    return 0;
}

unsigned short HAL_MMU_MapQuery(unsigned short u16VirtAddrEntry)
{
    MS_U16 u16RegVal;

    if (u16VirtAddrEntry >> 10)
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x)\n", u16VirtAddrEntry);
        return -1;
    }

    // reg_mmu_entry
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16VirtAddrEntry);

    // reg_mmu_access
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, BIT0);

    ndelay(100);

    // reg_mmu_rdata
    u16RegVal = HAL_MIU_Read2Byte(REG_MMU_R_DATA);

    return u16RegVal;
}

int HAL_MMU_UnMap(unsigned short u16PhyAddrEntry)
{
    MS_U16 u16RegVal;

    if (u16PhyAddrEntry >> 10)
    {
        MIU_HAL_ERR("Entry value over range(Phy:0x%x)\n", u16PhyAddrEntry);
        return -1;
    }

    // reg_mmu_wdata
    HAL_MIU_Write2Byte(REG_MMU_W_DATA, MMU_INVALID_ENTRY_VAL);

    // reg_mmu_entry
    u16RegVal = REG_MMU_RW_ENTRY_MODE | u16PhyAddrEntry;
    HAL_MIU_Write2Byte(REG_MMU_RW_ENTRY, u16RegVal);

    // reg_mmu_access
    HAL_MIU_Write2Byte(REG_MMU_ACCESS, BIT0);

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
        u16IrqRegVal &= ~(REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RD_MASK | REG_MMU_IRQ_WR_MASK);
    }
    else
    {
        u16CtrlRegVal &= ~REG_MMU_CTRL_ENABLE;
        // Disable IRQ
        // u16IrqRegVal &= ~(REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RD_MASK | REG_MMU_IRQ_WR_MASK);
        u16IrqRegVal |= (REG_MMU_IRQ_RW_MASK | REG_MMU_IRQ_RD_MASK | REG_MMU_IRQ_WR_MASK);
    }

    HAL_MIU_Write2Byte(REG_MMU_CTRL, u16CtrlRegVal);
    HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, u16IrqRegVal);

    return 0;
}

static void HAL_MIU_Write2Bytes_mask(MS_U32 u32RegOffset, MS_U16 u16Mask, MS_U16 u16Val)
{
    MS_U16 u16temp = HAL_MIU_Read2Byte(u32RegOffset);

    u16temp = (u16temp & ~u16Mask) | (u16Val & u16Mask);
    // u16Val = (bEnable) ? (u16Val | u16Mask) : (u16Val & ~u16Mask);
    HAL_MIU_Write2Byte(u32RegOffset, u16temp);
}

int HAL_MMU_Reset(void)
{
    MS_U16 u16RetryNum = 100;
    MS_U16 u16Reset    = (REG_MMU_CTRL_RESET | REG_MMU_CTRL_RESET_INIT_VAL);

    HAL_MIU_Write2Bytes_mask(REG_MMU_CTRL, REG_MMU_CTRL_ENABLE, 0x0); // disabled mmu_en
    HAL_MIU_Write2Byte(REG_MMU_CTRL, 0x0);                            // clear all
    HAL_MIU_Write2Bytes_mask(REG_MMU_CTRL, u16Reset, u16Reset);       // set reset

    do
    {
        if (HAL_MIU_Read2Byte(REG_MMU_CTRL) & REG_MMU_CTRL_INIT_DONE)
        {
            HAL_MIU_Write2Bytes_mask(REG_MMU_CTRL, u16Reset, 0x0); // disabled reset
            return 0;
        }

        u16RetryNum--;
    } while (u16RetryNum > 0);
    MIU_HAL_ERR("Reset timeout!\n");

    return -1;
}

unsigned int HAL_MMU_Status(unsigned short *u16PhyAddrEntry, unsigned short *u16ClientId, unsigned char *u8IsWriteCmd)
{
    MS_U16       u16IrqRegVal;
    unsigned int u32Status = 0;

    u16IrqRegVal = HAL_MIU_Read2Byte(REG_MMU_IRQ_CTRL);

    if (u16IrqRegVal & REG_MMU_IRQ_RW_FLAG)
    {
        *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_COLLISION_ENTRY);
        u16IrqRegVal |= REG_MMU_IRQ_RW_CLR;
        u32Status |= E_HAL_MMU_STATUS_RW_COLLISION;
    }

    if ((u16IrqRegVal & REG_MMU_IRQ_RD_FLAG) || (u16IrqRegVal & REG_MMU_IRQ_WR_FLAG))
    {
        *u16PhyAddrEntry = HAL_MIU_Read2Byte(REG_MMU_INVALID_ENTRY);
        *u16ClientId     = BITS_RANGE_VAL(HAL_MIU_Read2Byte(REG_MMU_INVALID_CLIENT_ID), REG_MMU_IRQ_INVALID_ID_MASK);
        *u8IsWriteCmd    = (u16IrqRegVal & REG_MMU_IRQ_INVALID_RW) ? 1 : 0;

        if (u16IrqRegVal & REG_MMU_IRQ_RD_FLAG)
        {
            u16IrqRegVal |= REG_MMU_IRQ_RD_CLR;
            u32Status |= E_HAL_MMU_STATUS_R_INVALID;
        }

        if (u16IrqRegVal & REG_MMU_IRQ_WR_FLAG)
        {
            u16IrqRegVal |= REG_MMU_IRQ_WR_CLR;
            u32Status |= E_HAL_MMU_STATUS_W_INVALID;
        }
    }

    // Clear IRQ
    if (u32Status != E_HAL_MMU_STATUS_NORMAL)
    {
        HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, u16IrqRegVal);
        HAL_MIU_Write2Byte(REG_MMU_IRQ_CTRL, 0x0);
    }

    return u32Status;
}

unsigned char HAL_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd)
{
    printk("Not support miu module reset\r\n");
    return 0;
}

int HAL_MMU_AddClientId(unsigned short u16ClientId)
{
    return 0;
}

int HAL_MMU_RemoveClientId(unsigned short u16ClientId)
{
    return 0;
}
