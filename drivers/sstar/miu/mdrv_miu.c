/*
 * mdrv_miu.c- Sigmastar
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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include "regMIU.h"
#include "irqs.h"
#include "MsTypes.h"
#include "mdrv_types.h"
#include "mdrv_miu.h"
#include "mhal_miu.h"
#include "registers.h"

//#include "mhal_bw.h"
#include "ms_platform.h"
//--------------------------------------------------------------------------------------------------
//  Local variable
//--------------------------------------------------------------------------------------------------

#ifndef CONFIG_MIU_PROTECT_SUPPORT_INT
static struct timer_list monitor_timer;
#else
static int MiuSelId[MIU_MAX_DEVICE] = {0};
static int g_protect_bugon_en       = 1;
#endif

static int bMiuProtect_is_initialized = 0;
U8         u8_MiuWhiteListNum         = 0;
U8         u8_MmuWhiteListNum         = 0;
#ifdef CONFIG_PM_SLEEP
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
static unsigned int miu_irq = 0;
#endif
#ifdef CONFIG_MMU_INTERRUPT_ENABLE
static unsigned int mmu_irq = 0;
#endif
#endif
//-------------------------------------------------------------------------------------------------
//  Local functions
//-------------------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function  \b Name: MDrv_MIU_Init
/// @brief \b Function  \b Description: parse occupied resource to software structure
/// @param None         \b IN :
/// @param None         \b OUT :
/// @param MS_BOOL      \b RET
/// @param None         \b GLOBAL :
////////////////////////////////////////////////////////////////////////////////
MS_BOOL MDrv_MIU_Init(void)
{
    MS_BOOL ret = FALSE;

    /* Parse the used client ID in hardware into software data structure */

    ret = HAL_MIU_ParseOccupiedResource();

    u8_MiuWhiteListNum = IDNUM_KERNELPROTECT;
    u8_MmuWhiteListNum = IDNUM_KERNELPROTECT;

    return ret;
}

MS_BOOL MDrv_MIU_GetProtectInfo(MS_U8 u8MiuSel, MIU_PortectInfo *pInfo)
{
    return HAL_MIU_GetHitProtectInfo(u8MiuSel, pInfo);
}

MS_BOOL MDrv_MIU_Get_IDEnables_Value(MS_U8 u8MiuSel, MS_U8 u8Blockx, MS_U8 u8ClientIndex)
{
    return HAL_MIU_GetProtectIdEnVal(u8MiuSel, u8Blockx, u8ClientIndex);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: MDrv_MIU_GetDefaultClientID_KernelProtect()
/// @brief \b Function \b Description:  Get default client id array pointer for protect kernel
/// @param <RET>           \b     : The pointer of Array of client IDs
////////////////////////////////////////////////////////////////////////////////
MS_U16 *MDrv_MIU_GetDefaultClientID_KernelProtect(void)
{
    return HAL_MIU_GetDefaultKernelProtectClientID();
}

MS_U16 *MDrv_MIU_GetClientID_KernelProtect(MS_U8 u8MiuSel)
{
    return HAL_MIU_GetKernelProtectClientID(u8MiuSel);
}

MS_U16 *MDrv_MMU_GetClientID_KernelProtect(MS_U8 u8MiuSel)
{
    return HAL_MMU_GetKernelProtectClientID(u8MiuSel);
}

MS_U64 MDrv_MIU_GetMmuBaseAddr(void)
{
#ifdef CONFIG_ARM_LPAE
    return LPAE_MMU_START_BASE;
#else
    return MMU_START_BASE;
#endif
}

#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
void MDrv_MIU_Protect_PanicOnOff(unsigned char u8On)
{
    g_protect_bugon_en = u8On;
}

static irqreturn_t MDrv_MIU_Protect_interrupt(s32 irq, void *dev_id)
{
    MS_U8           u8MiuSel = *(MS_U8 *)dev_id;
    MIU_PortectInfo pInfo;

    memset(&pInfo, 0, sizeof(pInfo));

    MDrv_MIU_GetProtectInfo(u8MiuSel, &pInfo);
    if (g_protect_bugon_en && pInfo.bHit)
        BUG();
    return IRQ_HANDLED;
}
#else
// create timer process
static void MDev_timer_callback(unsigned long value)
{
    MS_U8           u8MiuSel = 0;
    MIU_PortectInfo stProtInfo;

    for (u8MiuSel = 0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
    {
        stProtInfo.bHit = FALSE;

        MDrv_MIU_GetProtectInfo(u8MiuSel, &stProtInfo);
        if (stProtInfo.bHit)
        {
            panic("MIU %d Protect hit!\n", u8MiuSel);
        }
    }
    mod_timer(&monitor_timer, jiffies + 1 * HZ);
}
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: MDrv_MIU_Protect()
/// @brief \b Function \b Description:  Enable/Disable MIU Protection mode
/// @param u8Blockx        \b IN     : MIU Block to protect (MIU:0-15，MMU:16-31)
/// @param *pu8ProtectId   \b IN     : Allow specified client IDs to write
/// @param u32Start        \b IN     : Starting address(bus address)
/// @param u32End          \b IN     : End address(bus address)
/// @param bSetFlag        \b IN     : Disable or Enable MIU protection
///                                      - -Disable(0)
///                                      - -Enable(1)
/// @param <OUT>           \b None    :
/// @param <RET>           \b None    :
/// @param <GLOBAL>        \b None    :
////////////////////////////////////////////////////////////////////////////////
MS_BOOL MDrv_MIU_Protect(MS_U8 u8Blockx, MS_U16 *pu8ProtectId, ss_phys_addr_t u64BusStart, ss_phys_addr_t u64BusEnd,
                         MS_BOOL bSetFlag)
{
    MS_BOOL Result = FALSE;

    /* Case of former MIU protect */
    if (HAL_MIU_CheckGroup(u8Blockx))
    {
        if (TRUE != (Result = HAL_MIU_Protect(u8Blockx, pu8ProtectId, u64BusStart, u64BusEnd, bSetFlag)))
        {
            goto MDrv_MIU_Protect_Exit;
        }
    }

    if (!bMiuProtect_is_initialized)
    {
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
        struct device_node *dev_node       = NULL;
        MS_U8               u8MiuSel       = 0;
        int                 rc             = 0;
        int                 iIrqNum        = 0;
        char                compat_str[32] = {0};

        for (u8MiuSel = 0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
        {
            if (u8MiuSel == 0)
            {
                snprintf(compat_str, sizeof(compat_str) - 1, "sstar,miu");
            }
            else
            {
                snprintf(compat_str, sizeof(compat_str) - 1, "sstar,miu%d", u8MiuSel);
            }

            dev_node = of_find_compatible_node(NULL, NULL, compat_str);
            if (!dev_node)
            {
                printk("[MIU Protecrt] of_find_compatible_node Fail\r\n");
                Result = FALSE;
                goto MDrv_MIU_Protect_Exit;
            }

            iIrqNum = irq_of_parse_and_map(dev_node, 0);

            MiuSelId[u8MiuSel] = u8MiuSel;

            if (0
                != (rc = request_irq(iIrqNum, MDrv_MIU_Protect_interrupt, IRQF_TRIGGER_HIGH, "MIU_Protect",
                                     (void *)&MiuSelId[u8MiuSel])))
            {
                printk("[MIU Protecrt] request_irq [%d] Fail, ErrCode: %d\r\n", iIrqNum, rc);
                Result = FALSE;
                goto MDrv_MIU_Protect_Exit;
            }
#ifdef CONFIG_PM_SLEEP
            miu_irq = iIrqNum;
#endif
        }
#else
        init_timer(&monitor_timer);
        monitor_timer.function = MDev_timer_callback;
        monitor_timer.expires  = jiffies + HZ;
        add_timer(&monitor_timer);
#endif
        bMiuProtect_is_initialized++;
    }

MDrv_MIU_Protect_Exit:
    return Result;
}

MS_BOOL MDrv_MIU_Protect_Add_Ext_Feature(MS_U8 u8ModeSel, MS_U16 *pu8ProtectId, MS_U8 u8Blockx, MS_BOOL bSetFlag,
                                         MS_BOOL bIdFlag, MS_BOOL bInvertFlag)
{
    return HAL_MIU_Protect_Add_Ext_Feature(u8ModeSel, pu8ProtectId, u8Blockx, bSetFlag, bIdFlag, bInvertFlag);
}

MS_BOOL MDrv_MIU_SetAccessFromVpaOnly(MS_U8 u8Blockx, ss_phys_addr_t u64BusStart, ss_phys_addr_t u64BusEnd,
                                      MS_BOOL bSetFlag)
{
    if ((u8Blockx >= E_PROTECT_0) && (u8Blockx <= E_MIU_PROTECT_MAX))
    {
        return HAL_SetAccessFromVpaOnly(u8Blockx, u64BusStart, u64BusEnd, bSetFlag);
    }
    else
    {
        return false;
    }
}

unsigned int MDrv_MIU_ProtectDramSize(void)
{
    return HAL_MIU_ProtectDramSize();
}

int clientId_KernelProtectToName(unsigned short clientId, char *clientName)
{
#if defined CONFIG_MIU_SUPPORT_SYNOPSYS || defined CONFIG_MIU_RWCLIENT
    if ((char *)halWriteClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halWriteClientIDTName(clientId));
#else
    if ((char *)halClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halClientIDTName(clientId));
#endif
    return 0;
}
EXPORT_SYMBOL(clientId_KernelProtectToName);

#if defined CONFIG_MIU_SUPPORT_SYNOPSYS || defined CONFIG_MIU_RWCLIENT
int rcmd_clientId_KernelProtectToName(unsigned short clientId, char *clientName)
{
    if ((char *)halReadClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halReadClientIDTName(clientId));
    return 0;
}
EXPORT_SYMBOL(rcmd_clientId_KernelProtectToName);
#endif

int MDrv_MIU_Info(MIU_DramInfo *pDramInfo)
{
    return HAL_MIU_Info((MIU_DramInfo_Hal *)pDramInfo);
}

int MDrv_MIU_ClientIdToName(MS_U16 clientId, char *clientName)
{
#if defined CONFIG_MIU_SUPPORT_SYNOPSYS || defined CONFIG_MIU_RWCLIENT
    if ((char *)halWriteClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halWriteClientIDTName(clientId));
#else
    if ((char *)halClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halClientIDTName(clientId));
#endif
    return 0;
}
EXPORT_SYMBOL(MDrv_MIU_ClientIdToName);

int MDrv_MIU_RClientIdToName(MS_U16 clientId, char *clientName)
{
#if defined CONFIG_MIU_SUPPORT_SYNOPSYS || defined CONFIG_MIU_RWCLIENT
    if ((char *)halReadClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halReadClientIDTName(clientId));
#else
    if ((char *)halClientIDTName(clientId) != NULL)
        strcpy(clientName, (char *)halClientIDTName(clientId));
#endif
    return 0;
}
EXPORT_SYMBOL(MDrv_MIU_RClientIdToName);

void MDrv_MIU_SetBW(MS_U16 level)
{
    HAL_MIU_SetBW(level);
}

#ifdef CONFIG_MSTAR_MMAHEAP
EXPORT_SYMBOL(MDrv_MIU_Init);
EXPORT_SYMBOL(u8_MiuWhiteListNum);
EXPORT_SYMBOL(u8_MmuWhiteListNum);
EXPORT_SYMBOL(MDrv_MIU_GetDefaultClientID_KernelProtect);
EXPORT_SYMBOL(MDrv_MIU_GetClientID_KernelProtect);
EXPORT_SYMBOL(MDrv_MMU_GetClientID_KernelProtect);
EXPORT_SYMBOL(MDrv_MIU_SetBW);
EXPORT_SYMBOL(MDrv_MIU_Protect);
EXPORT_SYMBOL(MDrv_MIU_Protect_Add_Ext_Feature);
EXPORT_SYMBOL(MDrv_MIU_SetAccessFromVpaOnly);
#endif
EXPORT_SYMBOL(MDrv_MIU_Get_IDEnables_Value);
EXPORT_SYMBOL(MDrv_MIU_Info);
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
EXPORT_SYMBOL(MDrv_MIU_Protect_PanicOnOff);
#endif

#ifdef CONFIG_MIU_HW_MMU
static DEFINE_SPINLOCK(mmu_lock);

#ifdef CONFIG_MMU_INTERRUPT_ENABLE
static int        mmu_isr_init    = 0;
MDrv_MMU_Callback MDrv_MMU_Notify = NULL;
#endif

int MDrv_MMU_SetPageSize(unsigned char u8PgszMode)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_SetPageSize(u8PgszMode);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief \b Function \b Name: MDrv_MMU_SetRegion()
/// @brief \b Function \b Description:  Set the VPA and PA region
/// @param *u16Region           \b IN      : VPA Address
/// @param *u16ReplaceRegion    \b IN      : PA Address
/// @param <OUT>                \b None    :
/// @param <RET>                \b None    :
/// @param <GLOBAL>             \b None    :
////////////////////////////////////////////////////////////////////////////////
int MDrv_MMU_SetRegion(unsigned short u16Region, unsigned short u16ReplaceRegion)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_SetRegion(u16Region, u16ReplaceRegion);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

int MDrv_MMU_Map(unsigned short u16VirtAddrEntry, unsigned short u16PhyAddrEntry)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_Map(u16VirtAddrEntry, u16PhyAddrEntry);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

unsigned short MDrv_MMU_MapQuery(unsigned short u16VirtAddrEntry)
{
    unsigned long  flags;
    unsigned short entry;

    spin_lock_irqsave(&mmu_lock, flags);
    entry = HAL_MMU_MapQuery(u16VirtAddrEntry);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return entry;
}

int MDrv_MMU_UnMap(unsigned short u16VirtAddrEntry)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_UnMap(u16VirtAddrEntry);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

#ifdef CONFIG_MMU_INTERRUPT_ENABLE
static irqreturn_t MDrv_MMU_interrupt(s32 irq, void *dev_id)
{
    unsigned long  flags;
    unsigned int   status;
    unsigned short u16PhyAddrEntry, u16ClientId;
    unsigned char  u8IsWriteCmd = 0; // I6C no need

    spin_lock_irqsave(&mmu_lock, flags);
    status = HAL_MMU_Status(&u16PhyAddrEntry, &u16ClientId, &u8IsWriteCmd);

    if (MDrv_MMU_Notify != NULL)
    {
        MDrv_MMU_Notify(status, u16PhyAddrEntry, u16ClientId, u8IsWriteCmd);
    }
    else
    {
        printk("[%s] Status=0x%x, PhyAddrEntry=0x%x, ClientId=0x%x, IsWrite=%d\n", __FUNCTION__, status,
               u16PhyAddrEntry, u16ClientId, u8IsWriteCmd);
    }

    spin_unlock_irqrestore(&mmu_lock, flags);

    return IRQ_HANDLED;
}

void MDrv_MMU_CallbackFunc(MDrv_MMU_Callback pFuncPtr)
{
    unsigned long flags;

    spin_lock_irqsave(&mmu_lock, flags);
    MDrv_MMU_Notify = pFuncPtr;
    spin_unlock_irqrestore(&mmu_lock, flags);
}
EXPORT_SYMBOL(MDrv_MMU_CallbackFunc);
#endif
int MDrv_MMU_Enable(unsigned char u8Enable)
{
    unsigned long flags;
    int           ret;

#ifdef CONFIG_MMU_INTERRUPT_ENABLE
    if (!mmu_isr_init)
    {
        struct device_node *dev_node = NULL;
        int                 rc       = 0;
        int                 iIrqNum  = 0;
        dev_node                     = of_find_compatible_node(NULL, NULL, "sstar,mmu");
        if (!dev_node)
        {
            printk("[MMU] of_find_compatible_node Fail\r\n");
        }

        iIrqNum = irq_of_parse_and_map(dev_node, 0);

        if (0 != (rc = request_irq(iIrqNum, MDrv_MMU_interrupt, IRQF_TRIGGER_HIGH, "MMU", NULL)))
        {
            printk("[MMU] request_irq [%d] Fail, ErrCode: %d\r\n", iIrqNum, rc);
        }
#ifdef CONFIG_PM_SLEEP
        else
        {
            mmu_irq = iIrqNum;
        }
#endif
        mmu_isr_init = 1;
    }
#endif

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_Enable(u8Enable);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

int MDrv_MMU_AddClientId(unsigned short u16ClientId)
{
    unsigned long flags;
    int           ret;
    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_AddClientId(u16ClientId);
    spin_unlock_irqrestore(&mmu_lock, flags);
    return ret;
}

int MDrv_MMU_RemoveClientId(unsigned short u16ClientId)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_RemoveClientId(u16ClientId);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

int MDrv_MMU_Reset(void)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MMU_Reset();
    spin_unlock_irqrestore(&mmu_lock, flags);

    return ret;
}

#ifndef CONFIG_MMU_INTERRUPT_ENABLE
unsigned int MDrv_MMU_Status(unsigned short *u16PhyAddrEntry, unsigned short *u16ClientId, unsigned char *u8IsWriteCmd)
{
    unsigned long flags;
    unsigned int  status;

    spin_lock_irqsave(&mmu_lock, flags);
    status = HAL_MMU_Status(u16PhyAddrEntry, u16ClientId, u8IsWriteCmd);
    spin_unlock_irqrestore(&mmu_lock, flags);

    return status;
}
#endif
EXPORT_SYMBOL(MDrv_MMU_SetPageSize);
EXPORT_SYMBOL(MDrv_MMU_SetRegion);
EXPORT_SYMBOL(MDrv_MMU_Map);
EXPORT_SYMBOL(MDrv_MMU_MapQuery);
EXPORT_SYMBOL(MDrv_MMU_AddClientId);
EXPORT_SYMBOL(MDrv_MMU_RemoveClientId);
EXPORT_SYMBOL(MDrv_MMU_UnMap);
EXPORT_SYMBOL(MDrv_MMU_Enable);
EXPORT_SYMBOL(MDrv_MMU_Reset);
#ifndef CONFIG_MMU_INTERRUPT_ENABLE
EXPORT_SYMBOL(MDrv_MMU_Status);
#endif
#endif

unsigned char MDrv_MIU_ModuleReset(unsigned short u16ClientId, unsigned char wcmd)
{
    unsigned long flags;
    int           ret;

    spin_lock_irqsave(&mmu_lock, flags);
    ret = HAL_MIU_ModuleReset(u16ClientId, wcmd);
    spin_unlock_irqrestore(&mmu_lock, flags);
    return ret;
}
EXPORT_SYMBOL(MDrv_MIU_ModuleReset);

extern void mstar_create_MIU_node(void);
static int  mstar_miu_drv_probe(struct platform_device *pdev)
{
    mstar_create_MIU_node();

    if (!bMiuProtect_is_initialized)
    {
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
        struct device_node *dev_node       = NULL;
        MS_U8               u8MiuSel       = 0;
        int                 rc             = 0;
        int                 iIrqNum        = 0;
        char                compat_str[32] = {0};

        for (u8MiuSel = 0; u8MiuSel < MIU_MAX_DEVICE; u8MiuSel++)
        {
            if (u8MiuSel == 0)
            {
                snprintf(compat_str, sizeof(compat_str) - 1, "sstar,miu");
            }
            else
            {
                snprintf(compat_str, sizeof(compat_str) - 1, "sstar,miu%d", u8MiuSel);
            }

            dev_node = of_find_compatible_node(NULL, NULL, compat_str);
            if (!dev_node)
            {
                printk("[MIU Protecrt] of_find_compatible_node Fail\r\n");
                return 0;
            }

            iIrqNum = irq_of_parse_and_map(dev_node, 0);

            MiuSelId[u8MiuSel] = u8MiuSel;

            if (0
                != (rc = request_irq(iIrqNum, MDrv_MIU_Protect_interrupt, IRQF_TRIGGER_HIGH, "MIU_Protect",
                                     (void *)&MiuSelId[u8MiuSel])))
            {
                printk("[MIU Protecrt] request_irq [%d] Fail, ErrCode: %d\r\n", iIrqNum, rc);
                return 0;
            }
#ifdef CONFIG_PM_SLEEP
            miu_irq = iIrqNum;
#endif
        }
#else
        init_timer(&monitor_timer);
        monitor_timer.function = MDev_timer_callback;
        monitor_timer.expires  = jiffies + HZ;
        add_timer(&monitor_timer);
#endif
        bMiuProtect_is_initialized++;
    }

    return 0;
}

static int mstar_miu_drv_remove(struct platform_device *pdev)
{
    return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mstar_miu_drv_suspend(struct platform_device *dev, pm_message_t state)
{
    if (bMiuProtect_is_initialized)
    {
#ifdef CONFIG_MIU_PROTECT_SUPPORT_INT
        if (miu_irq)
        {
            disable_irq(miu_irq);
            free_irq(miu_irq, (void *)&MiuSelId[0]);
            miu_irq = 0;
        }
#else
        del_timer(&monitor_timer);
#endif
        bMiuProtect_is_initialized = 0;
    }

#ifdef CONFIG_MMU_INTERRUPT_ENABLE
    if (mmu_isr_init)
    {
        if (mmu_irq)
        {
            disable_irq(mmu_irq);
            free_irq(mmu_irq, NULL);
            mmu_irq = 0;
        }
        mmu_isr_init = 0;
    }
#endif
    return 0;
}

static int mstar_miu_drv_resume(struct platform_device *dev)
{
    return 0;
}
#endif
//#if defined (CONFIG_ARM64)
static struct of_device_id mstarmiu_of_device_ids[] = {
    {.compatible = "sstar,miu"},
    {},
};
//#endif

static struct platform_driver Sstar_miu_driver = {.probe  = mstar_miu_drv_probe,
                                                  .remove = mstar_miu_drv_remove,
#ifdef CONFIG_PM_SLEEP
                                                  .suspend = mstar_miu_drv_suspend,
                                                  .resume  = mstar_miu_drv_resume,
#endif
                                                  .driver = {
                                                      .name = "Sstar-miu",
                                                      //#if defined(CONFIG_ARM64)
                                                      .of_match_table = mstarmiu_of_device_ids,
                                                      //#endif
                                                      .owner = THIS_MODULE,
                                                  }};

static int __init mstar_miu_drv_init_module(void)
{
    int ret = 0;

    ret = platform_driver_register(&Sstar_miu_driver);

    if (ret)
    {
        printk("Register Sstar MIU Platform Driver Failed!");
    }
    return ret;
}

static void __exit mstar_miu_drv_exit_module(void)
{
    platform_driver_unregister(&Sstar_miu_driver);
}

module_init(mstar_miu_drv_init_module);
module_exit(mstar_miu_drv_exit_module);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("MIU driver");
MODULE_LICENSE("GPL");
