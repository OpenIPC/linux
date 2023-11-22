/*
 * mdrv_sysfs_ut.c- Sigmastar
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
//#include <asm/uaccess.h> /* for get_fs*/
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h> /* for dma_alloc_coherent */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <asm/cacheflush.h>
#include "registers.h"
#include "mdrv_miu.h"
#include "mdrv_msys_io_st.h"
#include "cam_os_wrapper.h"
#include "MsTypes.h"
#include "mdrv_types.h"
#include "ms_msys.h"
#include "mhal_miu.h"
#include "ms_platform.h"

static struct mmu_parameter
{
    void *         pVirtPtr;          // VA
    void *         pMiuPtr;           // MIUPA
    void *         poutMiuPtr;        // cross region
    void *         pVirtPagePtr[6];   // VA >> pagesize
    void *         pMiuPagePtr[6];    // MIUPA >> pagesize
    void *         pOutMiuPagePtr[6]; // MIUPA >> pagesize cross region
    unsigned short nRegion;
    unsigned short nReplaceRegion;
    unsigned long  pPhysPtr;            // PA
    unsigned short nMiuAddrEntry[6];    // entry of region
    unsigned short nVirtAddrEntry[6];   // entry of region
    unsigned short nOutMiuAddrEntry[6]; // entry of region
    unsigned int   u32PageSize;
    unsigned int   pattern;
    unsigned int   reset;
} ut_parameter = {NULL};

static unsigned int      u32IrqStatus;
static void *            pFillThreadPhyPtr;
static unsigned int      nFillThreadBufSize;
static struct miu_device miuut_dev;
extern void              CamOsPrintf(const char *szFmt, ...);

#if defined(CONFIG_ARCH_INFINITY7)
MS_U64 miu_base_addr = 0x1000000000;
MS_U64 mmu_base      = 0x100000000;
#endif

#if defined(CONFIG_ARCH_INFINITY6C)
// miu_base_addr
MS_U32 miu_base_addr = 0x20000000;
MS_U32 mmu_base      = 0x0;

static MS_U16 MIUUT_KernelProtect[IDNUM_KERNELPROTECT] = {
    MIU_CLIENTR_CA35,
    0,
};

void searchPageSize(int SliceMode)
{
    switch (SliceMode)
    {
        case E_MMU_PGSZ_32:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE;
            ut_parameter.pattern     = 0xAAAA;
            break;
        case E_MMU_PGSZ_64:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_64;
            ut_parameter.pattern     = 0xBBBB;
            break;
        case E_MMU_PGSZ_128:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_128;
            ut_parameter.pattern     = 0xCCCC;
            break;
        default:
            CamOsPrintf("[%s] ERR SliceMode = %d \n", __FUNCTION__, SliceMode);
            break;
    }
}
#elif defined(CONFIG_ARCH_PIONEER3)
// miu_base_addr
MS_U32 miu_base_addr = 0x20000000;
MS_U32 mmu_base      = 0x0;

static MS_U16 MIUUT_KernelProtect[IDNUM_KERNELPROTECT] = {
    MIU_CLIENT_MIPS_RW,
    0,
};

void searchPageSize(int SliceMode)
{
    switch (SliceMode)
    {
        case E_MMU_PGSZ_128:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_128;
            ut_parameter.pattern     = 0xAAAA;
            break;
        case E_MMU_PGSZ_256:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_256;
            ut_parameter.pattern     = 0xBBBB;
            break;
        case E_MMU_PGSZ_512:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_512;
            ut_parameter.pattern     = 0xCCCC;
            break;
        default:
            CamOsPrintf("[%s] ERR SliceMode = %d \n", __FUNCTION__, SliceMode);
            break;
    }
}
#elif defined(CONFIG_ARCH_INFINITY6E)
// miu_base_addr
MS_U32 miu_base_addr = 0x20000000;
MS_U32 mmu_base      = 0x0;

static MS_U16 MIUUT_KernelProtect[IDNUM_KERNELPROTECT] = {
    MIU_CLIENT_MIPS_RW,
    0,
};

void searchPageSize(int SliceMode)
{
    switch (SliceMode)
    {
        case E_MMU_PGSZ_128:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_128;
            ut_parameter.pattern     = 0xAAAA;
            break;
        case E_MMU_PGSZ_256:
            ut_parameter.u32PageSize = MMU_PAGE_SIZE_256;
            ut_parameter.pattern     = 0xBBBB;
            break;
        default:
            CamOsPrintf("[%s] ERR SliceMode = %d \n", __FUNCTION__, SliceMode);
            break;
    }
}
#endif

static struct miuut_alloc_dmem
{
    dma_addr_t  ut_phy_addr;
    const char *dmem_name;
    u8 *        ut_vir_addr;
} ALLOC_DMEM = {0, "MIU_UT", 0};

#define MMU_TEST_MEM_SIZE (3 * 1024 * 1024)

static void *alloc_dmem(const char *name, unsigned int size, dma_addr_t *addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name, name, strlen(name) + 1);
    dmem.length = size;
    if (0 != msys_request_dmem(&dmem))
    {
        return NULL;
    }
    *addr = dmem.phys;
    return (void *)((uintptr_t)dmem.kvirt);
}
void free_dmem(const char *name, unsigned int size, void *virt, dma_addr_t addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name, name, strlen(name) + 1);
    dmem.length = size;
    //    dmem.kvirt  = (unsigned long long)((uintptr_t)virt);
    dmem.kvirt = (void *)((uintptr_t)virt);
    dmem.phys  = (unsigned long long)((uintptr_t)addr);
    msys_release_dmem(&dmem);
}

static void MmuEntryAccessTest(void)
{
    unsigned short i, nMapVal;

    for (i = 0; i < (1 << MMU_ENTRY); i++)
    {
        nMapVal = MDrv_MMU_MapQuery(i);
        if (nMapVal != MMU_INVALID_ENTRY_VAL)
        {
            CamOsPrintf("\033[31m[%s] Entry 0x%x initial value != 0x%x\n\033[0m", __FUNCTION__, nMapVal,
                        MMU_INVALID_ENTRY_VAL);
            return;
        }
    }

    nMapVal = (1 << MMU_ENTRY) - 1;
    for (i = 0; i < (1 << MMU_ENTRY); i++)
        MDrv_MMU_Map(i, nMapVal--);

    nMapVal = (1 << MMU_ENTRY) - 1;
    for (i = 0; i < (1 << MMU_ENTRY); i++)
    {
        if (nMapVal != MDrv_MMU_MapQuery(i))
        {
            CamOsPrintf("\033[31m[%s] Entry 0x%x value != 0x%x(0x%x)\n\033[0m", __FUNCTION__, i, nMapVal,
                        MDrv_MMU_MapQuery(i));
            return;
        }

        nMapVal--;
    }

    CamOsPrintf("[%s] Test pass!\n", __FUNCTION__);
}
static int _MmuBdmaFillTest(void *nDstPhyPtr, void *nVerifyVirtPtr, unsigned int nBufSize, U32 pattern)
{
    MSYS_DMA_FILL stDmaCfg;
    int           i, count;

    stDmaCfg.phyaddr = (unsigned long)nDstPhyPtr;
    stDmaCfg.length  = nBufSize;
    stDmaCfg.pattern = pattern;
    printk("Bdma PA %x VA %x Pattern %x \r\n", (unsigned int)nDstPhyPtr, (unsigned int)(nVerifyVirtPtr), pattern);

    msys_dma_fill(&stDmaCfg);

    count = nBufSize / sizeof(unsigned int);
    for (i = 0; i < count; i++)
    {
        if (pattern != *((unsigned int *)(nVerifyVirtPtr) + i))
        {
            CamOsPrintf("[%s] compare faild [%d]=0x%x  \r\n", __FUNCTION__, i, *((unsigned int *)(nVerifyVirtPtr + i)));
            break;
        }
    }

    if (i != count)
    {
        return -1;
    }

    return 0;
}

int addrToRegion(int addr, int pageSize)
{
    int i        = 0;
    int pgOffset = 0;

    for (i = 0; i <= 35; i++)
    {
        if ((pageSize >> i) & 0x1)
        {
            pgOffset = i;
            break;
        }
    }
    return (addr >> (MMU_ENTRY + pgOffset));
}

int addrToEntry(int addr, int pageSize)
{
    int i        = 0;
    int pgOffset = 0;

    for (i = 0; i <= 35; i++)
    {
        if ((pageSize >> i) & 0x1)
        {
            pgOffset = i;
            break;
        }
    }
    return ((addr >> pgOffset) & ((1 << MMU_ENTRY) - 1));
}

void *add_align(void *addr, int pageSize)
{
    return (void *)(((unsigned int)addr & (0xFFFF0000 - pageSize + 0x10000)) + pageSize);
}

void addConverter(int SliceMode, int region)
{
    searchPageSize(SliceMode);

    if (region)
        ut_parameter.poutMiuPtr =
            (void *)(unsigned int)ut_parameter.pMiuPtr
            + (ut_parameter.u32PageSize * (1 << MMU_ENTRY)); // 128k x 512entry = 0x4000000  <region 1>
    else
        ut_parameter.poutMiuPtr = (void *)(unsigned int)ut_parameter.pMiuPtr;

    ut_parameter.nRegion        = addrToRegion((unsigned int)ut_parameter.poutMiuPtr, ut_parameter.u32PageSize);
    ut_parameter.nReplaceRegion = addrToRegion((unsigned int)ut_parameter.pMiuPtr, ut_parameter.u32PageSize);

    MDrv_MMU_SetRegion(ut_parameter.nRegion, ut_parameter.nReplaceRegion); //(vpa,pa);
    ut_parameter.pVirtPagePtr[0]   = add_align(ut_parameter.pVirtPtr, ut_parameter.u32PageSize);
    ut_parameter.pOutMiuPagePtr[0] = add_align(ut_parameter.poutMiuPtr, ut_parameter.u32PageSize);
    ut_parameter.pMiuPagePtr[0]    = add_align(ut_parameter.pMiuPtr, ut_parameter.u32PageSize);

    ut_parameter.nMiuAddrEntry[0] = addrToEntry((unsigned int)ut_parameter.pMiuPagePtr[0], ut_parameter.u32PageSize);
    ut_parameter.nOutMiuAddrEntry[0] =
        addrToEntry((unsigned int)ut_parameter.pOutMiuPagePtr[0], ut_parameter.u32PageSize);

    CamOsPrintf("[%s] nRegion = 0x%X , nReplaceRegion = 0x%X , poutMiuPtr = 0x%X Entry=0x%x \n", __FUNCTION__,
                ut_parameter.nRegion, ut_parameter.nReplaceRegion, (unsigned int)ut_parameter.poutMiuPtr,
                ut_parameter.nOutMiuAddrEntry[0]);
}

static void MmuBdmaFillThread(void *pUserData)
{
    MSYS_DMA_FILL stDmaCfg;

    stDmaCfg.phyaddr = (unsigned long)pFillThreadPhyPtr;
    stDmaCfg.length  = nFillThreadBufSize;
    stDmaCfg.pattern = 0xABCD1234;

    while (CAM_OS_OK != CamOsThreadShouldStop())
    {
        msys_dma_fill(&stDmaCfg);
        msleep(1);
    }

    return;
}

static int _MmuMdmaCopyTest(void *nSrcPhyPtr, void *nDstPhyPtr, void *nSrcVirtPtr, void *nDstVirtPtr,
                            unsigned int nBufSize)
{
    MSYS_DMA_BLIT stDmaCfg;
    int           i, count;

    if (0xABCD1234 != *((unsigned int *)nSrcVirtPtr))
    {
        CamOsPrintf("[%s] Need to fill pattern first!\n", __FUNCTION__);
        // return -1;
    }

    stDmaCfg.phyaddr_src  = (unsigned long)nSrcPhyPtr;
    stDmaCfg.phyaddr_dst  = (unsigned long)nDstPhyPtr;
    stDmaCfg.lineofst_src = 0;
    stDmaCfg.lineofst_dst = 0;
    stDmaCfg.width_src    = 0;
    stDmaCfg.width_dst    = 0;
    stDmaCfg.length       = nBufSize;

    msys_dma_blit(&stDmaCfg);

    count = nBufSize / sizeof(unsigned int);
    for (i = 0; i < count; i++)
    {
        if (*((unsigned int *)(nSrcVirtPtr) + i) != *((unsigned int *)(nDstVirtPtr) + i))
        {
            CamOsPrintf("[%s] compare faild [%d]=0x%x  \r\n", __FUNCTION__, i, *((unsigned int *)(nDstVirtPtr + i)));
            break;
        }
    }

    if (i != count)
    {
        return -1;
    }

    return 0;
}

void _MmuInterruptCallback(unsigned int u32Sstatus, unsigned short u16PhyAddrEntry, unsigned short u16ClientId,
                           unsigned char u8IsWriteCmd)
{
    u32IrqStatus |= u32Sstatus;

    CamOsPrintf("[%s] int status 0x%x Status=0x%x, PhyAddrEntry=0x%x, ClientId=0x%x ,  IsWrite=%d\n", __FUNCTION__,
                u32Sstatus, u32IrqStatus, u16PhyAddrEntry, u16ClientId, u8IsWriteCmd);
}

static void MmuInterruptTest(void)
{
    int i, nTestRound = 0;

    CamOsThread tBdmaFillThread;

    CamOsPrintf("[%s] pMiuPtr= 0x%X  \n", __FUNCTION__, (unsigned int)ut_parameter.pMiuPtr);
    addConverter(0, 0);
    MDrv_MMU_SetPageSize(0);

    u32IrqStatus = 0;
    CamOsPrintf("[%s] pVirtPagePtr = 0x%X , pMiuPagePtr= 0x%X ,MiuAddrEntry  0x%X\n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPagePtr[0], (unsigned int)ut_parameter.pOutMiuPagePtr[0],
                ut_parameter.nOutMiuAddrEntry[0]);

    for (i = 1; i < 6; i++)
    {
        ut_parameter.pVirtPagePtr[i]   = ut_parameter.pVirtPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.pOutMiuPagePtr[i] = ut_parameter.pOutMiuPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.nOutMiuAddrEntry[i] =
            addrToEntry((unsigned int)ut_parameter.pOutMiuPagePtr[i], ut_parameter.u32PageSize);

        CamOsPrintf("[%s] before swap pVirtPagePtr[%d] = 0x%X , pOutMiuPagePtr[%d] = 0x%X \n", __FUNCTION__, i,
                    (unsigned int)ut_parameter.pVirtPagePtr[i], i, (unsigned int)ut_parameter.pOutMiuPagePtr[i]);

        *((unsigned int *)ut_parameter.pVirtPagePtr[i]) = (unsigned int)ut_parameter.pVirtPagePtr[i];
        CamOsPrintf("before Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                    *((unsigned int *)ut_parameter.pVirtPagePtr[i]));
    }

    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nOutMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nOutMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nOutMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nOutMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nOutMiuAddrEntry[4];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nOutMiuAddrEntry[5];

    for (i = 0; i < (1 << MMU_ENTRY); i++) // mapping kernel code. except mmu_ut
    {
        if (!(((i >= ut_parameter.nMiuAddrEntry[0]) && (i <= ut_parameter.nMiuAddrEntry[5]))))
        {
            MDrv_MMU_Map(i, i);
        }
        else
        {
            printk("0xi %x\r\n", i);
        }
    }

    for (i = 0; i < 6; i++)
    {
        printk("Mapping VA 0x%x <> PA 0x%x \r\n", ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
        MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]); //(VA,PA)
    }

    MDrv_MMU_CallbackFunc(_MmuInterruptCallback);
    MDrv_MMU_Enable(1);

    // Make read/write collision
    pFillThreadPhyPtr  = ut_parameter.pOutMiuPagePtr[0];
    nFillThreadBufSize = MMU_PAGE_SIZE * 6;
    CamOsThreadCreate(&tBdmaFillThread, NULL, (void *)MmuBdmaFillThread, NULL);

    nTestRound = 10000;
    while (nTestRound > 0)
    {
        if (nTestRound & 0x1)
        {
            for (i = 0; i < 6; i++)
            {
                ut_parameter.nVirtAddrEntry[i] = ut_parameter.nOutMiuAddrEntry[5 - i];
            }
        }
        else
        {
            for (i = 0; i < 6; i++)
            {
                ut_parameter.nVirtAddrEntry[i] = ut_parameter.nOutMiuAddrEntry[i];
            }
        }

        for (i = 0; i < 6; i++)
        {
            MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
        }

        nTestRound--;
    }

    CamOsThreadStop(tBdmaFillThread);

    if ((u32IrqStatus != (E_HAL_MMU_STATUS_RW_COLLISION)))
    {
        CamOsPrintf("\033[31m[%s][%s] COLLISION Test fail(irq status = 0x%x)!\n\033[0m", __FUNCTION__, u32IrqStatus);
        goto MMU_INTERRUPT_TEST_EXIT;
    }
    else
    {
        CamOsPrintf("\033[32m[%s] Test pass! IRQ COLLISION status %d\n\033[0m", __FUNCTION__, u32IrqStatus);
    }
    // clear isr
    u32IrqStatus = 0;

#if 1
    // Read/Write invalid entry
    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nOutMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nOutMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nOutMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nOutMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nOutMiuAddrEntry[4];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nOutMiuAddrEntry[5];

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }

    _MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pVirtPagePtr[0], MMU_PAGE_SIZE, 0x1234);
    MDrv_MMU_UnMap(ut_parameter.nOutMiuAddrEntry[0]);
    _MmuMdmaCopyTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0],
                     ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE);

    MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[0], ut_parameter.nVirtAddrEntry[0]);
    MDrv_MMU_UnMap(ut_parameter.nOutMiuAddrEntry[1]);
    _MmuMdmaCopyTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0],
                     ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE);
#endif
    if ((u32IrqStatus != (E_HAL_MMU_STATUS_RW_INVALID)))
    {
        CamOsPrintf("\033[31m[%s][%s] Test fail(irq status = 0x%x)!\n\033[0m", __FUNCTION__, u32IrqStatus);
        goto MMU_INTERRUPT_TEST_EXIT;
    }
    else
    {
        CamOsPrintf("\033[32m[%s] Test pass! IRQ RW_INVALID status %d\n\033[0m", __FUNCTION__, u32IrqStatus);
    }
MMU_INTERRUPT_TEST_EXIT:
    return;
}

static void MmuInterruptTest_02(unsigned char u8SliceMode)
{
    int         i, nTestRound;
    CamOsThread tBdmaFillThread;
    u32IrqStatus = 0;

    addConverter(u8SliceMode, 1);
    MDrv_MMU_SetPageSize(u8SliceMode);

    for (i = 1; i < 6; i++)
    {
        ut_parameter.pVirtPagePtr[i]   = ut_parameter.pVirtPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.pOutMiuPagePtr[i] = ut_parameter.pOutMiuPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.nOutMiuAddrEntry[i] =
            addrToEntry((unsigned int)ut_parameter.pOutMiuPagePtr[i], ut_parameter.u32PageSize);

        CamOsPrintf("[%s] before swap pVirtPagePtr[%d] = 0x%X , pOutMiuPagePtr[%d] = 0x%X \n", __FUNCTION__, i,
                    (unsigned int)ut_parameter.pVirtPagePtr[i], i, (unsigned int)ut_parameter.pOutMiuPagePtr[i]);

        *((unsigned int *)ut_parameter.pVirtPagePtr[i]) = (unsigned int)ut_parameter.pVirtPagePtr[i];
        CamOsPrintf("before Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                    *((unsigned int *)ut_parameter.pVirtPagePtr[i]));
    }

    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nOutMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nOutMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nOutMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nOutMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nOutMiuAddrEntry[4];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nOutMiuAddrEntry[5];

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }

    MDrv_MMU_CallbackFunc(_MmuInterruptCallback);
    MDrv_MMU_Enable(1);

    // Make read/write collision
    pFillThreadPhyPtr  = ut_parameter.pOutMiuPagePtr[0];
    nFillThreadBufSize = MMU_PAGE_SIZE * 6;
    CamOsThreadCreate(&tBdmaFillThread, NULL, (void *)MmuBdmaFillThread, NULL);

    nTestRound = 100000; // if u32IrqStatus does not have the E_HAL_MMU_STATUS_RW_COLLISION , please modify this counter
                         // for verify .  collision is relating with timing & cpu/miu clock
    while (nTestRound > 0)
    {
        if (nTestRound & 0x1)
        {
            for (i = 0; i < 6; i++)
                ut_parameter.nVirtAddrEntry[i] = ut_parameter.nOutMiuAddrEntry[5 - i];
        }
        else
        {
            for (i = 0; i < 6; i++)
                ut_parameter.nVirtAddrEntry[i] = ut_parameter.nOutMiuAddrEntry[i];
        }

        for (i = 0; i < 6; i++)
        {
            MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
        }

        nTestRound--;
    }

    CamOsThreadStop(tBdmaFillThread);

    // Read/Write invalid entry
    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nOutMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nOutMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nOutMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nOutMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nOutMiuAddrEntry[4];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nOutMiuAddrEntry[5];

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }

    _MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pVirtPagePtr[0], MMU_PAGE_SIZE, 0x5566);
    MDrv_MMU_UnMap(ut_parameter.nOutMiuAddrEntry[0]);
    _MmuMdmaCopyTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0],
                     ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE);

    MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[0], ut_parameter.nVirtAddrEntry[0]);
    MDrv_MMU_UnMap(ut_parameter.nOutMiuAddrEntry[1]);
    _MmuMdmaCopyTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0],
                     ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE);

    if ((u32IrqStatus != (E_HAL_MMU_STATUS_RW_COLLISION | E_HAL_MMU_STATUS_RW_INVALID)))
    {
        CamOsPrintf("\033[31m[%s][%s] Test fail(irq status = 0x%x)!\n\033[0m", __FUNCTION__, u32IrqStatus);
    }
    else
    {
        CamOsPrintf("\033[32m[%s] Test pass! IRQ RW_INVALID status %d\n\033[0m", __FUNCTION__, u32IrqStatus);
    }
}

static void MmuMappingTest_03(unsigned char u8SliceMode)
{
    int i;

    MDrv_MMU_SetPageSize(u8SliceMode);

    addConverter(u8SliceMode, 1);

    i = 0;
    CamOsPrintf("[%s] before swap pVirtPagePtr[%d] = 0x%X , pOutMiuPagePtr[%d] = 0x%X \n", __FUNCTION__, i,
                (unsigned int)ut_parameter.pVirtPagePtr[i], i, (unsigned int)ut_parameter.pOutMiuPagePtr[i]);
    *((unsigned int *)ut_parameter.pVirtPagePtr[i]) = (unsigned int)ut_parameter.pVirtPagePtr[i];
    CamOsPrintf("before Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                *((unsigned int *)ut_parameter.pVirtPagePtr[i]));

    for (i = 1; i < 6; i++)
    {
        ut_parameter.pVirtPagePtr[i]   = ut_parameter.pVirtPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.pMiuPagePtr[i]    = ut_parameter.pMiuPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.pOutMiuPagePtr[i] = ut_parameter.pOutMiuPagePtr[i - 1] + ut_parameter.u32PageSize;
        ut_parameter.nOutMiuAddrEntry[i] =
            addrToEntry((unsigned int)ut_parameter.pOutMiuPagePtr[i], ut_parameter.u32PageSize);

        CamOsPrintf("[%s] Before swap pVirtPagePtr[%d] = 0x%X , pMiuPagePtr[%d] = 0x%X pOutMiuPagePtr[%d] = 0x%X",
                    __FUNCTION__, i, (unsigned int)ut_parameter.pVirtPagePtr[i], i,
                    (unsigned int)ut_parameter.pMiuPagePtr[i], i, (unsigned int)ut_parameter.pOutMiuPagePtr[i]);

        *((unsigned int *)ut_parameter.pVirtPagePtr[i]) = (unsigned int)ut_parameter.pVirtPagePtr[i];
        CamOsPrintf("before Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                    *((unsigned int *)ut_parameter.pVirtPagePtr[i]));
    }

    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nOutMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nOutMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nOutMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nOutMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nOutMiuAddrEntry[5];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nOutMiuAddrEntry[4];

    CamOsPrintf("[%s] PageSize:0x%x\n", __FUNCTION__, ut_parameter.u32PageSize);
    CamOsPrintf("[%s] First round...\n", __FUNCTION__);

    for (i = 0; i < (1 << MMU_ENTRY); i++) // mapping kernel code. except mmu_ut
    {
        if (!(((i >= ut_parameter.nOutMiuAddrEntry[0]) && (i <= ut_parameter.nOutMiuAddrEntry[5]))))
        {
            MDrv_MMU_Map(i, i);
        }
    }

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nOutMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }

    MDrv_MMU_Enable(1);

    if (_MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pVirtPagePtr[1], ut_parameter.u32PageSize,
                         ut_parameter.pattern)
        < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail! (128KB skip this error)\n\033[0m", __FUNCTION__);
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }
    CamOsPrintf("[%s] Second round...\n", __FUNCTION__);

    if (_MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0], ut_parameter.u32PageSize, 0xCCDD)
        < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail! (128KB skip this error)\n\033[0m", __FUNCTION__);
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }

    for (i = 0; i < 6; i++)
    {
        CamOsPrintf("Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                    *((unsigned int *)ut_parameter.pVirtPagePtr[i]));
    }

    if (ut_parameter.reset != 0xbabe)
    {
        HAL_MMU_Reset();
        /*clear mmu entry*/
        *(unsigned short volatile *)0xFD202744 = 0x0; // 0x0x1013 0x51
        *(unsigned short volatile *)0xFD202748 = 0x0; // 0x0x1013 0x52
        CamOsPrintf("Unmap MMU\r\n");
    }

    CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
}

static void MmuMappingTest_02(void)
{
    int i;

    MDrv_MMU_SetPageSize(0);
    addConverter(0, 1);

    CamOsPrintf("[%s] pVirtPtr = 0x%X , pPhysPtr= 0x%X, pMiuPtr= 0x%X  \n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPtr, (unsigned int)ut_parameter.pPhysPtr,
                (unsigned int)ut_parameter.pMiuPtr);

    CamOsPrintf("[%s] nRegion = 0x%X , nReplaceRegion = 0x%X , poutMiuPtr = 0x%X \n", __FUNCTION__,
                ut_parameter.nRegion, ut_parameter.nReplaceRegion, (unsigned int)ut_parameter.poutMiuPtr);

    for (i = 1; i < 6; i++)
    {
        ut_parameter.pVirtPagePtr[i]   = ut_parameter.pVirtPagePtr[i - 1] + MMU_PAGE_SIZE;
        ut_parameter.pMiuPagePtr[i]    = ut_parameter.pMiuPagePtr[i - 1] + MMU_PAGE_SIZE;
        ut_parameter.pOutMiuPagePtr[i] = ut_parameter.pOutMiuPagePtr[i - 1] + MMU_PAGE_SIZE;

        ut_parameter.nMiuAddrEntry[i] = MMU_ADDR_TO_ENTRY((unsigned int)ut_parameter.pMiuPagePtr[i]);
        CamOsPrintf("[%s] Before swap pVirtPagePtr[%d] = 0x%X , pMiuPagePtr[%d] = 0x%X pOutMiuPagePtr[%d] = 0x%X",
                    __FUNCTION__, i, (unsigned int)ut_parameter.pVirtPagePtr[i], i,
                    (unsigned int)ut_parameter.pMiuPagePtr[i], i, (unsigned int)ut_parameter.pOutMiuPagePtr[i]);

        *((unsigned int *)ut_parameter.pVirtPagePtr[i]) = (unsigned int)ut_parameter.pVirtPagePtr[i];

        CamOsPrintf(" Mem 0x%X = 0x%X\n", (unsigned int)ut_parameter.pVirtPagePtr[i],
                    *((unsigned int *)ut_parameter.pVirtPagePtr[i]));
    }

    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nMiuAddrEntry[0];

    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nMiuAddrEntry[2];

    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nMiuAddrEntry[5];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nMiuAddrEntry[4];

    CamOsPrintf("[%s] First round...\n", __FUNCTION__);

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }

    CamOsPrintf("[%s] MDrv_MMU_Enable..\n", __FUNCTION__);

    MDrv_MMU_Enable(1);

    if (_MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[0], ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE, 0xCCDD) < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        return;
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }

    CamOsPrintf("[%s] Second round...\n", __FUNCTION__);

    if (_MmuBdmaFillTest(ut_parameter.pOutMiuPagePtr[1], ut_parameter.pVirtPagePtr[0], MMU_PAGE_SIZE, 0xAABB) < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }

    HAL_MMU_Reset();

    if (0xAABB != *(unsigned int *)ut_parameter.pVirtPagePtr[0])
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        CamOsPrintf("fail value=%x\r\n", *(unsigned int *)ut_parameter.pVirtPagePtr[0]);
    }
    if (0xCCDD != *(unsigned int *)ut_parameter.pVirtPagePtr[1])
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        CamOsPrintf("fail value=%x\r\n", *(unsigned int *)ut_parameter.pVirtPagePtr[1]);
    }

    CamOsPrintf("\033[32m[%s] Test pass!\n\033[0m", __FUNCTION__);
}

static void MmuMappingTest(void)
{
    int i;
    MDrv_MMU_SetPageSize(0);

    CamOsPrintf("[%s] pVirtPtr = 0x%X , pPhysPtr= 0x%X, pMiuPtr= 0x%X  \n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPtr, (unsigned int)ut_parameter.pPhysPtr,
                (unsigned int)ut_parameter.pMiuPtr);
    addConverter(0, 0);

    CamOsPrintf("[%s] pVirtPagePtr = 0x%X , pMiuPagePtr= 0x%X ,MiuAddrEntry  0x%X\n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPagePtr[0], (unsigned int)ut_parameter.pMiuPagePtr[0],
                ut_parameter.nMiuAddrEntry[0]);

    for (i = 1; i < 6; i++)
    {
        ut_parameter.pVirtPagePtr[i]  = ut_parameter.pVirtPagePtr[i - 1] + MMU_PAGE_SIZE;
        ut_parameter.pMiuPagePtr[i]   = ut_parameter.pMiuPagePtr[i - 1] + MMU_PAGE_SIZE;
        ut_parameter.nMiuAddrEntry[i] = MMU_ADDR_TO_ENTRY((unsigned int)ut_parameter.pMiuPagePtr[i]);
        CamOsPrintf("[%s] pVirtPagePtr = 0x%X , pMiuPagePtr= 0x%X , MiuAddrEntry  0x%X\n", __FUNCTION__,
                    (unsigned int)ut_parameter.pVirtPagePtr[i], (unsigned int)ut_parameter.pMiuPagePtr[i],
                    ut_parameter.nMiuAddrEntry[i]);
    }

    ut_parameter.nVirtAddrEntry[0] = ut_parameter.nMiuAddrEntry[1];
    ut_parameter.nVirtAddrEntry[1] = ut_parameter.nMiuAddrEntry[0];
    ut_parameter.nVirtAddrEntry[2] = ut_parameter.nMiuAddrEntry[3];
    ut_parameter.nVirtAddrEntry[3] = ut_parameter.nMiuAddrEntry[2];
    ut_parameter.nVirtAddrEntry[4] = ut_parameter.nMiuAddrEntry[5];
    ut_parameter.nVirtAddrEntry[5] = ut_parameter.nMiuAddrEntry[4];

    // First round: add clients are set to MMU
    CamOsPrintf("First round..nVirtAddrEntry0 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[0],
                (unsigned int)ut_parameter.pVirtPagePtr[0]);
    CamOsPrintf("First round..nVirtAddrEntry1 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[1],
                (unsigned int)ut_parameter.pVirtPagePtr[1]);
    CamOsPrintf("First round..nVirtAddrEntry2 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[2],
                (unsigned int)ut_parameter.pVirtPagePtr[2]);
    CamOsPrintf("First round..nVirtAddrEntry3 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[3],
                (unsigned int)ut_parameter.pVirtPagePtr[3]);
    CamOsPrintf("First round..nVirtAddrEntry4 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[4],
                (unsigned int)ut_parameter.pVirtPagePtr[4]);
    CamOsPrintf("First round..nVirtAddrEntry5 0x%X 0x%X\n", ut_parameter.nVirtAddrEntry[5],
                (unsigned int)ut_parameter.pVirtPagePtr[5]);

    for (i = 0; i < (1 << MMU_ENTRY); i++) // mapping kernel code. except mmu_ut
    {
        if (!(((i >= ut_parameter.nMiuAddrEntry[0]) && (i <= ut_parameter.nMiuAddrEntry[5]))))
        {
            MDrv_MMU_Map(i, i);
        }
        else
        {
            //            printk("0xi %x\r\n", i);
        }
    }

    for (i = 0; i < 6; i++)
    {
        MDrv_MMU_Map(ut_parameter.nMiuAddrEntry[i], ut_parameter.nVirtAddrEntry[i]);
    }
    MDrv_MMU_Enable(1);

    if (_MmuBdmaFillTest(ut_parameter.pMiuPagePtr[0], ut_parameter.pVirtPagePtr[0], MMU_PAGE_SIZE, 0xCCDD) < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }

    CamOsPrintf("[%s] Second round...\n", __FUNCTION__);

    if (_MmuBdmaFillTest(ut_parameter.pMiuPagePtr[1], ut_parameter.pVirtPagePtr[1], MMU_PAGE_SIZE, 0xAABB) < 0)
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        return;
    }
    else
    {
        CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
    }
    MDrv_MMU_Reset();

    /*clear mmu entry*/
    *(unsigned short volatile *)0xFD202744 = 0x0; // 0x0x1013 0x51
    *(unsigned short volatile *)0xFD202748 = 0x0; // 0x0x1013 0x52

    if (0xAABB != *(unsigned int *)ut_parameter.pVirtPagePtr[0])
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        CamOsPrintf("fail value=%x\r\n", *(unsigned int *)ut_parameter.pVirtPagePtr[0]);
    }
    if (0xCCDD != *(unsigned int *)ut_parameter.pVirtPagePtr[1])
    {
        CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        CamOsPrintf("fail value=%x\r\n", *(unsigned int *)ut_parameter.pVirtPagePtr[1]);
    }

    CamOsPrintf("[%s] Test pass!\n", __FUNCTION__);
}
extern MS_BOOL MDrv_MIU_Protect_Add_Ext_Feature(MS_U8 u8ModeSel, MS_U16 *pu8ProtectId, MS_U8 u8Blockx, MS_BOOL bSetFlag,
                                                MS_BOOL bIdFlag, MS_BOOL bInvertFlag);

static void MiuProtectionWhiteList(unsigned char u8SliceMode, int mmu)
{
    U16 *pu8ProtectId     = MIUUT_KernelProtect;
    U8   u8Blockx         = 0;
    U32  u32BusStart[5]   = {0};
    U32  u32BusEnd[5]     = {0};
    U32  u32BusStartPA[5] = {0};
    U32  u32BusEndPA[5]   = {0};
    BOOL bSetFlag         = 1;
    BOOL bIdFlag          = 1;
    BOOL bInvertFlag      = 0;
    int  count            = 4;
    int  block            = u8Blockx;

    if (mmu)
    {
        count = 1;
    }

    CamOsPrintf("[%s] pVirtPtr = 0x%X , pPhysPtr= 0x%X, pMiuPtr= 0x%X  \n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPtr, (unsigned int)ut_parameter.pPhysPtr,
                (unsigned int)ut_parameter.pMiuPtr);

    MDrv_MMU_SetPageSize(u8SliceMode);

    for (u8Blockx = 0; u8Blockx < count; u8Blockx++)
    {
        if (mmu == 0)
        {
            u32BusStart[u8Blockx] = (unsigned int)ut_parameter.pPhysPtr + u8Blockx * ((u8SliceMode + 1) * 0x20000);
            u32BusEnd[u8Blockx] = (unsigned int)ut_parameter.pPhysPtr + (u8Blockx + 1) * ((u8SliceMode + 1) * 0x20000);
        }
        else
        {
            u32BusStart[u8Blockx] = (unsigned int)ut_parameter.pOutMiuPagePtr[u8Blockx] + 0X20000000;
            u32BusEnd[u8Blockx] =
                (unsigned int)ut_parameter.pOutMiuPagePtr[u8Blockx] + ((u8SliceMode + 1) * 0x20000) + 0X20000000;
            block = u8Blockx + E_MMU_PROTECT_0;

            u32BusStartPA[u8Blockx] = (unsigned int)ut_parameter.pMiuPagePtr[u8Blockx] + 0X20000000;
            u32BusEndPA[u8Blockx] =
                (unsigned int)ut_parameter.pMiuPagePtr[u8Blockx] + ((u8SliceMode + 1) * 0x20000) + 0X20000000;
        }

        printk("start %x end %x\r\n", u32BusStart[u8Blockx], u32BusEnd[u8Blockx]);
        printk("E_MMU_PROTECT_0+u8Blockx %d\r\n", E_MMU_PROTECT_0 + u8Blockx);

        if (mmu)
        {
            if (MDrv_MIU_SetAccessFromVpaOnly(block - 16, ((unsigned int)u32BusStartPA[u8Blockx]),
                                              ((unsigned int)u32BusEndPA[u8Blockx]), bSetFlag))
                CamOsPrintf("\033[32m[%s] u8Blockx:%d SetAccessFromVpaOnly successfully!\n\033[0m", __FUNCTION__,
                            u8Blockx);
        }
        if (MDrv_MIU_Protect(block, pu8ProtectId, u32BusStart[u8Blockx], u32BusEnd[u8Blockx], bSetFlag))
        { /*disable read protect*/
            if (MDrv_MIU_Protect_Add_Ext_Feature(mmu, pu8ProtectId, block, bSetFlag, bIdFlag, bInvertFlag))
                CamOsPrintf("[%s] u8Blockx:%d Set MIU Protect Successfully!\n", __FUNCTION__,
                            E_MMU_PROTECT_0 + u8Blockx);
        }
        else
            CamOsPrintf("[%s] u8Blockx:%d Set MIU Protect failed!\n", __FUNCTION__, E_MMU_PROTECT_0 + u8Blockx);

        if (_MmuBdmaFillTest((void *)((u32BusStart[u8Blockx] - miu_base_addr)),
                             (void *)(unsigned int)ut_parameter.pVirtPtr, 0x40, 0xCCDD)
            < 0)
        {
            CamOsPrintf("\033[31m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
        }
        else
        {
            CamOsPrintf("\033[32m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        }
        *((unsigned int *)(ut_parameter.pVirtPtr)) = 0xABCD;
        CamOsPrintf("\033[32m[%s] CPU write %x = 0xABCD \n\033[0m", __FUNCTION__,
                    (void *)(unsigned int)ut_parameter.pVirtPtr);
    }
}

static void MiuProtectionBlackList(unsigned char u8SliceMode, int mmu)
{
    U16 *pu8ProtectId   = MIUUT_KernelProtect;
    U8   u8Blockx       = 0;
    U32  u32BusStart[5] = {0};
    U32  u32BusEnd[5]   = {0};
    BOOL bSetFlag       = 1;
    BOOL bIdFlag        = 1;
    BOOL bInvertFlag    = 1;
    CamOsPrintf("[%s] pVirtPtr = 0x%X , pPhysPtr= 0x%X, pMiuPtr= 0x%X  \n", __FUNCTION__,
                (unsigned int)ut_parameter.pVirtPtr, (unsigned int)ut_parameter.pPhysPtr,
                (unsigned int)ut_parameter.pMiuPtr);

    MDrv_MMU_SetPageSize(u8SliceMode);

    for (u8Blockx = 0; u8Blockx < E_MIU_BLOCK_NUM; u8Blockx++)
    {
        u32BusStart[u8Blockx] = (unsigned int)ut_parameter.pPhysPtr + u8Blockx * ((u8SliceMode + 1) * 0x2000);
        u32BusEnd[u8Blockx]   = (unsigned int)ut_parameter.pPhysPtr + (u8Blockx + 1) * ((u8SliceMode + 1) * 0x2000);

        if (MDrv_MIU_Protect(u8Blockx, pu8ProtectId, u32BusStart[u8Blockx], u32BusEnd[u8Blockx], bSetFlag))
        {
            if (MDrv_MIU_Protect_Add_Ext_Feature(0, pu8ProtectId, u8Blockx, bSetFlag, bIdFlag, bInvertFlag))
                CamOsPrintf("[%s] u8Blockx:%d Set MIU Protect Successfully!\n", __FUNCTION__, u8Blockx);
        }
        else
        {
            CamOsPrintf("[%s] u8Blockx:%d Set MIU Protect failed!\n", __FUNCTION__, u8Blockx);
        }
        if (_MmuBdmaFillTest((void *)(u32BusStart[u8Blockx] - miu_base_addr),
                             (void *)(unsigned int)ut_parameter.pVirtPtr + (u8Blockx * ((u8SliceMode + 1) * 0x2000)),
                             0x40, 0xCCDD)
            < 0)
        {
            CamOsPrintf("\033[31m[%s] BDMA test fail!\n\033[0m", __FUNCTION__);
        }
        else
        {
            CamOsPrintf("\033[32m[%s] BDMA test pass!\n\033[0m", __FUNCTION__);
        }

        *((unsigned int *)(ut_parameter.pVirtPtr)) = 0xABCD;
        CamOsPrintf("\033[31m[%s] CPU write %x = 0xABCD \n\033[0m", __FUNCTION__,
                    (void *)(unsigned int)ut_parameter.pVirtPtr);
    }
}

static ssize_t miu_UT_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    int   n;

    n = sprintf(str, "[%s][%d]: OK!\n", __FUNCTION__, __LINE__);
    return n;
}

static ssize_t miu_UT(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned int testcase, slice_mode;

    sscanf(buf, "%d %d", &testcase, &slice_mode);
    printk("[MIU_UT] %d\r\n", testcase);

    if (slice_mode >= 4)
    {
        slice_mode = 0;
    }

    if (!(ALLOC_DMEM.ut_vir_addr = alloc_dmem(ALLOC_DMEM.dmem_name, MMU_TEST_MEM_SIZE, &ALLOC_DMEM.ut_phy_addr)))
    {
        printk("[input]unable to allocate aesdma memory\n");
    }
    memset(ALLOC_DMEM.ut_vir_addr, 0, MMU_TEST_MEM_SIZE);

    ut_parameter.pPhysPtr = ALLOC_DMEM.ut_phy_addr;
    ut_parameter.pVirtPtr = (void *)((uintptr_t)ALLOC_DMEM.ut_vir_addr);
    ut_parameter.pMiuPtr  = (void *)ut_parameter.pPhysPtr - miu_base_addr;

    MDrv_MMU_Reset();
    msleep(10); // wait for MMU initial ready

    switch (testcase)
    {
        case 1: // Reset and entry access test
            MmuEntryAccessTest();
            break;
        case 2: // Multi HW IP access MMU test
            MmuMappingTest();
            break;
        case 3: // Multi HW IP access MMU test
            MmuMappingTest_02();
            break;
        case 4: // 3 mode pagesize MmuMapping Test
            switch (slice_mode)
            {
                case 1:
                    MmuMappingTest_03(0);
                    break;
                case 2:
                    MmuMappingTest_03(1);
                    break;
                case 3:
                    MmuMappingTest_03(2);
                    break;
                default:
                    CamOsPrintf("error slice mode,it should be 0~2\r\n");
                    break;
            }
            break;
        case 5: // Interrupt test
            /* clear entry*/
            MmuInterruptTest();
            break;
        case 6: // 3 mode pagesize MmuInterrupt Test
            switch (slice_mode)
            {
                case 1:
                    MmuInterruptTest_02(0);
                    break;
                case 2:
                    MmuInterruptTest_02(1);
                    break;
                case 3:
                    MmuInterruptTest_02(2);
                    break;
                default:
                    CamOsPrintf("error slice mode,it should be 0~2\r\n");
                    break;
            }
            break;
        case 7: // 3 mode pagesize MiuProtectionWhiteList Test
            switch (slice_mode)
            {
                case 1:
                    MiuProtectionWhiteList(0, 0);
                    break;
                case 2:
                    MiuProtectionWhiteList(1, 0);
                    break;
                case 3:
                    MiuProtectionWhiteList(2, 0);
                    break;
                default:
                    CamOsPrintf("error slice mode,it should be 0~2\r\n");
                    break;
            }
            break;
        case 8: // 3 mode pagesize MiuProtectionBlackList Test
            switch (slice_mode)
            {
                case 1:
                    MiuProtectionBlackList(0, 0);
                    break;
                case 2:
                    MiuProtectionBlackList(1, 0);
                    break;
                case 3:
                    MiuProtectionBlackList(2, 0);
                    break;
                default:
                    CamOsPrintf("error slice mode,it should be 0~2\r\n");
                    break;
            }
            break;
        case 9: // 3 mode pagesize MmuProtectionWhiteList Test
            switch (slice_mode)
            {
                case 1:
                    ut_parameter.reset = 0xbabe;
                    MmuMappingTest_03(0);
                    // Add MMU_+_BASE
                    ut_parameter.poutMiuPtr =
                        (void *)(unsigned int)ut_parameter.pMiuPtr
                        + ut_parameter.u32PageSize * (1 << MMU_ENTRY); // pg_size x 512entry   <region 1>
                    MiuProtectionWhiteList(0, 1);
                    ut_parameter.reset = 0x0;
                    break;
                case 2:
                    ut_parameter.reset = 0xbabe;
                    MmuMappingTest_03(1);
                    // Add MMU_+_BASE
                    ut_parameter.poutMiuPtr =
                        (void *)(unsigned int)ut_parameter.pMiuPtr
                        + ut_parameter.u32PageSize * (1 << MMU_ENTRY); // pg_size x 512entry   <region 1>
                    MiuProtectionWhiteList(1, 1);
                    ut_parameter.reset = 0x0;
                    break;
                case 3:
                    ut_parameter.reset = 0xbabe;
                    MmuMappingTest_03(2);
                    // Add MMU_+_BASE
                    ut_parameter.poutMiuPtr =
                        (void *)(unsigned int)ut_parameter.pMiuPtr
                        + ut_parameter.u32PageSize * (1 << MMU_ENTRY); // pg_size x 512entry   <region 1>
                    MiuProtectionWhiteList(2, 1);
                    ut_parameter.reset = 0x0;
                    break;
                default:
                    CamOsPrintf("error slice mode,it should be 0~2\r\n");
                    break;
            }
            break;

        default:
            CamOsPrintf("unknown command\r\n");
            break;
    }
    free_dmem(ALLOC_DMEM.dmem_name, MMU_TEST_MEM_SIZE, ALLOC_DMEM.ut_vir_addr, ALLOC_DMEM.ut_phy_addr);

    return n;
}
DEVICE_ATTR(MIU_UT, 0644, miu_UT_show, miu_UT);

void create_miu_ut_node(struct bus_type *miu_subsys)
{
    int ret;
    miuut_dev.index         = 0;
    miuut_dev.dev.kobj.name = "miu_ut";
    miuut_dev.dev.bus       = miu_subsys;

    ret = device_register(&miuut_dev.dev);
    if (ret)
    {
        printk(KERN_ERR "Failed to register %s device!! %d\n", miuut_dev.dev.kobj.name, ret);
        return;
    }
    device_create_file(&miuut_dev.dev, &dev_attr_MIU_UT);
}
