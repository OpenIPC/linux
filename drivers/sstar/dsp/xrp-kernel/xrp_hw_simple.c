/*
 * xrp_hw_simple.c- Sigmastar
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

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include "xrp_kernel_defs.h"
#include "xrp_hw.h"
#include "xrp_hw_simple_dsp_interface.h"
#include "cam_sysfs.h"

#include "cam_os_wrapper.h"
#include "reg.h"
#include "hal_dsp.h"
#include "hal_dsp_dbg.h"
#include "dsp_ut.h"

#define DRIVER_NAME "xrp-hw-simple"

#ifndef DEBUG
#define DEBUG
#endif

#define XRP_REG_RESET    (0x04)
#define XRP_REG_RUNSTALL (0x08)

enum xrp_irq_mode
{
    XRP_IRQ_NONE,
    XRP_IRQ_LEVEL,
    XRP_IRQ_EDGE,
    XRP_IRQ_EDGE_SW,
    XRP_IRQ_MAX,
};

struct xrp_hw_simple
{
    struct xvp *  xrp;
    phys_addr_t   regs_phys;
    void __iomem *regs;
    u8            core_id;
    /* how IRQ is used to notify the device of incoming data */
    enum xrp_irq_mode device_irq_mode;
    /*
     * offset of device IRQ register in MMIO region (device side)
     * bit number
     * device IRQ#
     */
    u32 device_irq[3];
    /* offset of devuce IRQ register in MMIO region (host side) */
    u32 device_irq_host_offset;
    /* how IRQ is used to notify the host of incoming data */
    enum xrp_irq_mode host_irq_mode;
    /*
     * offset of IRQ register (device side)
     * bit number
     */
    u32 host_irq[2];
};

static inline void reg_write32(struct xrp_hw_simple *hw, unsigned addr, u32 v)
{
    if (hw->regs)
        __raw_writel(v, hw->regs + addr);
}

static inline u32 reg_read32(struct xrp_hw_simple *hw, unsigned addr)
{
    if (hw->regs)
        return __raw_readl(hw->regs + addr);
    else
        return 0;
}

static void *get_hw_sync_data(void *hw_arg, size_t *sz)
{
    static const u32 irq_mode[] = {
        [XRP_IRQ_NONE]    = XRP_DSP_SYNC_IRQ_MODE_NONE,
        [XRP_IRQ_LEVEL]   = XRP_DSP_SYNC_IRQ_MODE_LEVEL,
        [XRP_IRQ_EDGE]    = XRP_DSP_SYNC_IRQ_MODE_EDGE,
        [XRP_IRQ_EDGE_SW] = XRP_DSP_SYNC_IRQ_MODE_EDGE,
    };
    struct xrp_hw_simple *          hw           = hw_arg;
    struct xrp_hw_simple_sync_data *hw_sync_data = kmalloc(sizeof(*hw_sync_data), GFP_KERNEL);

    if (!hw_sync_data)
        return NULL;

    *hw_sync_data = (struct xrp_hw_simple_sync_data){
        .device_mmio_base  = hw->regs_phys,
        .host_irq_mode     = hw->host_irq_mode,
        .host_irq_offset   = hw->host_irq[0],
        .host_irq_bit      = hw->host_irq[1],
        .device_irq_mode   = irq_mode[hw->device_irq_mode],
        .device_irq_offset = hw->device_irq[0],
        .device_irq_bit    = hw->device_irq[1],
        .device_irq        = hw->device_irq[2],
    };
    *sz = sizeof(*hw_sync_data);
    return hw_sync_data;
}
#if defined(SSTAR_DSP)
static int enable(void *hw_arg)
{
    struct xrp_hw_simple *hw = hw_arg;
    if (HAL_DSP_OK == HalDspInit(hw->core_id))
    {
        return 0;
    }
    return -1;
}
static void reset(void *hw_arg)
{
    HalDspAddressMap_t    addrMap;
    struct xrp_hw_simple *hw = hw_arg;
    __u32                 v  = 0;

#define PHY_ADDR_MIU_BASE 0x20000000
    /// core
    // rom
    addrMap.eSet = HAL_DSP_ADDRESS_SET0;
    if (hw->core_id == 0)
    {
        addrMap.offset = (0X30000000 - PHY_ADDR_MIU_BASE - 0x10000000);
    }
    else if (hw->core_id == 1)
    {
        addrMap.offset = (0X32000000 - PHY_ADDR_MIU_BASE - 0x10000000);
    }
    else if (hw->core_id == 2)
    {
        addrMap.offset = (0X34000000 - PHY_ADDR_MIU_BASE - 0x10000000);
    }
    else
    {
        addrMap.offset = (0X36000000 - PHY_ADDR_MIU_BASE - 0x10000000);
    }
    addrMap.u32BaseAddress = 0x10000000;
    addrMap.u32Len         = 0x02000000;
    HalDspSetAddressMap(hw->core_id, &addrMap);

    // ram
    addrMap.eSet = HAL_DSP_ADDRESS_SET1;
    if (hw->core_id == 0)
    {
        addrMap.offset = (0x40000000 - PHY_ADDR_MIU_BASE - 0x20000000);
    }
    else if (hw->core_id == 1)
    {
        addrMap.offset = (0x42000000 - PHY_ADDR_MIU_BASE - 0x20000000);
    }
    else if (hw->core_id == 2)
    {
        addrMap.offset = (0x44000000 - PHY_ADDR_MIU_BASE - 0x20000000);
    }
    else
    {
        addrMap.offset = (0x46000000 - PHY_ADDR_MIU_BASE - 0x20000000);
    }
    addrMap.u32BaseAddress = 0x20000000;
    addrMap.u32Len         = 0x02000000;
    HalDspSetAddressMap(hw->core_id, &addrMap);
    // other
    addrMap.eSet   = HAL_DSP_ADDRESS_SET2;
    addrMap.offset = (0);
    HalDspSetAddressMap(hw->core_id, &addrMap);

    HalDspSetPRID(hw->core_id, 0X272);
    HalDspSetResetVector(hw->core_id, true, 0x10040000);

    HalDspSetInterruptMask(hw->core_id, HAL_DSP_INT_MASK_CORE, BIT(5));
    HalDspSetInterruptMask(hw->core_id, HAL_DSP_INT_MASK_CMDQ0, 0x0);
    HalDspSetInterruptMask(hw->core_id, HAL_DSP_INT_MASK_CMDQ1, 0x0);
    HalDspSetCoreSightDbgMode(hw->core_id, HAL_DSP_DBG_PDBGEN | HAL_DSP_DBG_DBGEN | HAL_DSP_DBG_NIDEN
                                               | HAL_DSP_DBG_SPIDEN | HAL_DSP_DBG_SPNIDEN);
    //*(__u16*)(0xfd200800) = 0x0902;
    HalDspSetXIUClkEn(hw->core_id, true);
    HalDspSetAPBSlvClkEn(hw->core_id, true);
    v = 1;
    // HalDspAPBWrite(hw->core_id, 0x1000, v, 0xFFFFFFFF);
    // v = 0xF0000000;
    // HalDspAPBWrite(hw->core_id, 0x1100, v, 0xFFFFFFFF);

    HalDspDbgEnablePMG(hw->core_id, true);
    HalDspDbgSetPMGCtrl(hw->core_id, HAL_DSP_DBG_PM0, true, true, 0, 0, 0xFF);
    HalDspDbgSetPMGCtrl(hw->core_id, HAL_DSP_DBG_PM1, true, true, 0, 1, 0xFF);
    HalDspDbgSetPMGCtrl(hw->core_id, HAL_DSP_DBG_PM2, true, true, 0, 2, 0xFF);
    HalDspDbgSetPMGCtrl(hw->core_id, HAL_DSP_DBG_PM3, true, true, 0, 3, 0xFF);
    HalDspDbgSetPMGCtrl(hw->core_id, HAL_DSP_DBG_PM4, true, true, 0, 4, 0xFF);

    HalDspReset(hw->core_id, HAL_DSP_CORE_RESET_ALL);
    CamOsUsSleep(1);
    HalDspReset(hw->core_id, HAL_DSP_RESET_DONE);

    {
        HalDspIRQStat_t stIRQStat;
        int             i = 0;
        memset(&stIRQStat, 0, sizeof(HalDspIRQStat_t));
        stIRQStat.bMskIrq = 1;
        for (; i < 32; i++)
            HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, i);
        stIRQStat.bClearIrq      = 0;
        stIRQStat.bEdgeEn        = 1;
        stIRQStat.bForceValid    = 0;
        stIRQStat.bMskIrq        = 0;
        stIRQStat.bNegPos        = 0;
        stIRQStat.bStatBeforeMsk = 0;
        HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, 4);
        HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, 6);
        HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, 7);
        memset(&stIRQStat, 0, sizeof(HalDspIRQStat_t));
        HalDspGetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, 4);
        pr_err("%s %d  %d %d %d %d %d %d\n", __func__, __LINE__, stIRQStat.bClearIrq, stIRQStat.bEdgeEn,
               stIRQStat.bForceValid, stIRQStat.bMskIrq, stIRQStat.bNegPos, stIRQStat.bStatBeforeMsk);
    }
}

static void halt(void *hw_arg)
{
    struct xrp_hw_simple *hw = hw_arg;
    // reg_write32(hw_arg, XRP_REG_RUNSTALL, 1);
    HalDspRunStallReset(hw->core_id, true);
}

static void release(void *hw_arg)
{
    struct xrp_hw_simple *hw = hw_arg;
    // reg_write32(hw_arg, XRP_REG_RUNSTALL, 0);
    HalDspRunStallReset(hw->core_id, false);
}
static void send_irq(void *hw_arg)
{
    // HalDspSetInterruptMask(hw->core_id,HAL_DSP_INT_MASK_CPU,BIT(1)|BIT(2));
    // HalDspGPIInterruptTrigger(hw->core_id,BIT(1));
}
static void ack_irq(void *hw_arg)
{
    // HalDspSetInterruptMask(hw->core_id,HAL_DSP_INT_MASK_CPU,BIT(1)|BIT(2));
    // HalDspGPIInterruptTrigger(hw->core_id,BIT(2));
}
// static void send_irq(void *hw_arg)
// {
//     struct xrp_hw_simple *hw = hw_arg;

//     switch (hw->device_irq_mode)
//     {
//         case XRP_IRQ_EDGE_SW:
//             reg_write32(hw, hw->device_irq_host_offset,
//                         BIT(hw->device_irq[1]));
//             while ((reg_read32(hw, hw->device_irq_host_offset) &
//                     BIT(hw->device_irq[1])))
//                 mb();
//             break;
//         case XRP_IRQ_EDGE:
//             reg_write32(hw, hw->device_irq_host_offset, 0);
//         /* fallthrough */
//         case XRP_IRQ_LEVEL:
//             wmb();
//             reg_write32(hw, hw->device_irq_host_offset,
//                         BIT(hw->device_irq[1]));
//             break;
//         default:
//             break;
//     }
// }

// static void ack_irq(void *hw_arg)
// {
//     struct xrp_hw_simple *hw = hw_arg;

//     if (hw->host_irq_mode == XRP_IRQ_LEVEL)
//         reg_write32(hw, hw->host_irq[0], 0);
// }

typedef struct dfsstamp_s
{
    u8  idx;
    u64 ts[2];
    u32 irq6;
    u32 irq7;
} dfsstamp;
static dfsstamp dfsTs[4] = {0};
// static int turn = 0;
// #define DATA_ADDRESS 0xCA000010
// static volatile int *precord = (int *)(DATA_ADDRESS);
irqreturn_t irq_handler(int irq, void *dev_id)
{
    HalDspIRQStat_t       stIRQStat;
    struct xrp_hw_simple *hw = dev_id;

    if (0)
    {
        irqreturn_t ret = xrp_irq_handler(irq, hw->xrp);

        if (ret == IRQ_HANDLED)
            ack_irq(hw);

        return ret;
    }
    // dfsTs[hw->core_id].idx++;

    HalDspGetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_BEFOREMSK, 6);
    if (stIRQStat.bStatBeforeMsk)
    {
        stIRQStat.bClearIrq = 1;
        HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_CLEAR, 6);
        dfsTs[hw->core_id].irq6++;
    }
    HalDspGetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_BEFOREMSK, 7);
    if (stIRQStat.bStatBeforeMsk)
    {
        stIRQStat.bClearIrq = 1;
        HalDspSetIRQStat(hw->core_id, &stIRQStat, HAL_DSP_IRQ_STAT_CLEAR, 7);
        dfsTs[hw->core_id].irq7++;
    }

    return IRQ_HANDLED;
}

#else
static void reset(void *hw_arg)
{
    reg_write32(hw_arg, XRP_REG_RESET, 1);
    udelay(1);
    reg_write32(hw_arg, XRP_REG_RESET, 0);
}

static void halt(void *hw_arg)
{
    reg_write32(hw_arg, XRP_REG_RUNSTALL, 1);
}

static void release(void *hw_arg)
{
    reg_write32(hw_arg, XRP_REG_RUNSTALL, 0);
}

static void send_irq(void *hw_arg)
{
    struct xrp_hw_simple *hw = hw_arg;

    switch (hw->device_irq_mode)
    {
        case XRP_IRQ_EDGE_SW:
            reg_write32(hw, hw->device_irq_host_offset, BIT(hw->device_irq[1]));
            while ((reg_read32(hw, hw->device_irq_host_offset) & BIT(hw->device_irq[1])))
                mb();
            break;
        case XRP_IRQ_EDGE:
            reg_write32(hw, hw->device_irq_host_offset, 0);
        /* fallthrough */
        case XRP_IRQ_LEVEL:
            wmb();
            reg_write32(hw, hw->device_irq_host_offset, BIT(hw->device_irq[1]));
            break;
        default:
            break;
    }
}

static void ack_irq(void *hw_arg)
{
    struct xrp_hw_simple *hw = hw_arg;

    if (hw->host_irq_mode == XRP_IRQ_LEVEL)
        reg_write32(hw, hw->host_irq[0], 0);
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
    struct xrp_hw_simple *hw  = dev_id;
    irqreturn_t           ret = xrp_irq_handler(irq, hw->xrp);

    if (ret == IRQ_HANDLED)
        ack_irq(hw);

    return ret;
}
#endif

#if defined(__XTENSA__)
static bool cacheable(void *hw_arg, unsigned long pfn, unsigned long n_pages)
{
    return true;
}

static void dma_sync_for_device(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags)
{
    switch (flags)
    {
        case XRP_FLAG_READ:
            __flush_dcache_range((unsigned long)vaddr, sz);
            break;

        case XRP_FLAG_READ_WRITE:
            __flush_dcache_range((unsigned long)vaddr, sz);
            __invalidate_dcache_range((unsigned long)vaddr, sz);
            break;

        case XRP_FLAG_WRITE:
            __invalidate_dcache_range((unsigned long)vaddr, sz);
            break;
    }
}

static void dma_sync_for_cpu(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags)
{
    switch (flags)
    {
        case XRP_FLAG_READ_WRITE:
        case XRP_FLAG_WRITE:
            __invalidate_dcache_range((unsigned long)vaddr, sz);
            break;
    }
}

#elif defined(__arm__)
static bool cacheable(void *hw_arg, unsigned long pfn, unsigned long n_pages)
{
    return true;
}

static void dma_sync_for_device(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags)
{
    switch (flags)
    {
        case XRP_FLAG_READ:
            __cpuc_flush_dcache_area(vaddr, sz);
            outer_clean_range(paddr, paddr + sz);
            break;

        case XRP_FLAG_WRITE:
            __cpuc_flush_dcache_area(vaddr, sz);
            outer_inv_range(paddr, paddr + sz);
            break;

        case XRP_FLAG_READ_WRITE:
            __cpuc_flush_dcache_area(vaddr, sz);
            outer_flush_range(paddr, paddr + sz);
            break;
    }
}

static void dma_sync_for_cpu(void *hw_arg, void *vaddr, phys_addr_t paddr, unsigned long sz, unsigned flags)
{
    switch (flags)
    {
        case XRP_FLAG_WRITE:
        case XRP_FLAG_READ_WRITE:
            __cpuc_flush_dcache_area(vaddr, sz);
            outer_inv_range(paddr, paddr + sz);
            break;
    }
}
#endif

static const struct xrp_hw_ops hw_ops = {
    .halt    = halt,
    .release = release,
    .reset   = reset,

    .get_hw_sync_data = get_hw_sync_data,

    .send_irq = send_irq,
    .enable   = enable,
#if defined(__XTENSA__) || defined(__arm__)
    .cacheable           = cacheable,
    .dma_sync_for_device = dma_sync_for_device,
    .dma_sync_for_cpu    = dma_sync_for_cpu,
#endif
};
static long init_hw(struct platform_device *pdev, struct xrp_hw_simple *hw, int mem_idx,
                    enum xrp_init_flags *init_flags)
{
    // struct resource *mem;
    int  irq = -1;
    long ret;
    s32  dsp_id = -1;
    // mem = CamPlatformGetResource(pdev, IORESOURCE_MEM, mem_idx);
    // if (!mem)
    // {
    //     ret = -ENODEV;
    //     goto err;
    // }
    // hw->regs_phys = mem->start;
    // hw->regs = devm_ioremap_resource(&pdev->dev, mem);
    // pr_err("%s: regs = %pap/%p\n",
    //        __func__, &mem->start, hw->regs);
    ret = of_property_read_u32(pdev->dev.of_node, "dsp-id", &dsp_id);
    if (ret < 0)
    {
        pr_err("%s %d device_property_read_u8 dsp-id fail\n", __func__, __LINE__);
    }
    hw->core_id = dsp_id;
    ret         = HalDspGetDspIrqNumById(hw->core_id, &irq);
    if (HAL_DSP_OK != ret)
    {
        pr_err("%s %d HalDspGetDspIrqNumById fail\n", __func__, __LINE__);
    }
    else
    {
        pr_err("%s %d HalDspGetDspIrqNumById %d core %d\n", __func__, __LINE__, irq, hw->core_id);
    }
    if (irq >= 0)
    {
        dev_dbg(&pdev->dev, "%s: host IRQ = %d, ", __func__, irq);

        ret = 0; // devm_request_irq(&pdev->dev, irq, irq_handler, IRQF_SHARED | IRQF_ONESHOT, pdev->name, hw);
        if (ret < 0)
        {
            dev_err(&pdev->dev, "request_irq %d failed\n", irq);
            goto err;
        }
        *init_flags |= XRP_INIT_USE_HOST_IRQ;
    }
    else
    {
        dev_info(&pdev->dev, "using polling mode on the host side\n");
    }
    ret = 0;
err:
    return ret;
}
/*
static long init_hw(struct platform_device *pdev, struct xrp_hw_simple *hw,
                    int mem_idx, enum xrp_init_flags *init_flags)
{
    struct resource *mem;
    int irq;
    long ret;

    mem = CamPlatformGetResource(pdev, IORESOURCE_MEM, mem_idx);
    if (!mem)
    {
        ret = -ENODEV;
        goto err;
    }
    hw->regs_phys = mem->start;
    hw->regs = devm_ioremap_resource(&pdev->dev, mem);
    pr_err("%s: regs = %pap/%p\n",
             __func__, &mem->start, hw->regs);

    ret = CamOfPropertyReadVariableU32Array(pdev->dev.of_node,
                                            "device-irq",
                                            hw->device_irq, 0,
                                            ARRAY_SIZE(hw->device_irq));
    if (ret == 0)
    {
        u32 device_irq_host_offset;

        ret = CamofPropertyReadU32(pdev->dev.of_node,
                                   "device-irq-host-offset",
                                   &device_irq_host_offset);
        if (ret == 0)
        {
            hw->device_irq_host_offset = device_irq_host_offset;
        }
        else
        {
            hw->device_irq_host_offset = hw->device_irq[0];
            ret = 0;
        }
    }
    if (ret == 0)
    {
        u32 device_irq_mode;

        ret = CamofPropertyReadU32(pdev->dev.of_node,
                                   "device-irq-mode",
                                   &device_irq_mode);
        if (device_irq_mode < XRP_IRQ_MAX)
            hw->device_irq_mode = device_irq_mode;
        else
            ret = -ENOENT;
    }
    if (ret == 0)
    {
        dev_dbg(&pdev->dev,
                "%s: device IRQ MMIO host offset = 0x%08x, offset = 0x%08x, bit = %d, device IRQ = %d, IRQ mode = %d",
                __func__, hw->device_irq_host_offset,
                hw->device_irq[0], hw->device_irq[1],
                hw->device_irq[2], hw->device_irq_mode);
    }
    else
    {
        dev_info(&pdev->dev,
                 "using polling mode on the device side\n");
    }

    ret = CamOfPropertyReadVariableU32Array(pdev->dev.of_node, "host-irq",
                                            hw->host_irq, 0,
                                            ARRAY_SIZE(hw->host_irq));
    if (ret == 0)
    {
        u32 host_irq_mode;

        ret = CamofPropertyReadU32(pdev->dev.of_node,
                                   "host-irq-mode",
                                   &host_irq_mode);
        if (host_irq_mode < XRP_IRQ_MAX)
            hw->host_irq_mode = host_irq_mode;
        else
            ret = -ENOENT;
    }

    if (ret == 0 && hw->host_irq_mode != XRP_IRQ_NONE)
    {
        struct resource *r = CamPlatformGetResource(pdev, IORESOURCE_IRQ, 0);
        irq = r ? r->start : -ENXIO;
    }
    else
        irq = -1;

    if (irq >= 0)
    {
        dev_dbg(&pdev->dev, "%s: host IRQ = %d, ",
                __func__, irq);

        ret = devm_request_irq(&pdev->dev, irq, irq_handler,
                               IRQF_SHARED, pdev->name, hw);
        if (ret < 0)
        {
            dev_err(&pdev->dev, "request_irq %d failed\n", irq);
            goto err;
        }
        *init_flags |= XRP_INIT_USE_HOST_IRQ;
    }
    else
    {
        dev_info(&pdev->dev, "using polling mode on the host side\n");
    }
    ret = 0;
err:
    return ret;
}
*/
static long init(struct platform_device *pdev, struct xrp_hw_simple *hw)
{
    long                ret;
    enum xrp_init_flags init_flags = 0;

    ret = init_hw(pdev, hw, 0, &init_flags);
    if (ret < 0)
        return ret;

    return xrp_init(pdev, init_flags, &hw_ops, hw);
}

static long init_v1(struct platform_device *pdev, struct xrp_hw_simple *hw)
{
    long                ret;
    enum xrp_init_flags init_flags = 0;

    ret = init_hw(pdev, hw, 1, &init_flags);
    if (ret < 0)
        return ret;

    return xrp_init_v1(pdev, init_flags, &hw_ops, hw);
}

static long init_cma(struct platform_device *pdev, struct xrp_hw_simple *hw)
{
    long                ret;
    enum xrp_init_flags init_flags = 0;

    ret = init_hw(pdev, hw, 0, &init_flags);
    if (ret < 0)
        return ret;

    return xrp_init_cma(pdev, init_flags, &hw_ops, hw);
}

#ifdef CONFIG_OF
static const struct of_device_id xrp_hw_simple_match[] = {
    {
        .compatible = "cdns,xrp-hw-simple",
        .data       = init,
    },
    {
        .compatible = "cdns,xrp-hw-simple,v1",
        .data       = init_v1,
    },
    {
        .compatible = "cdns,xrp-hw-simple,cma",
        .data       = init_cma,
    },
    {},
};
MODULE_DEVICE_TABLE(of, xrp_hw_simple_match);
#endif

static int xrp_hw_simple_probe(struct platform_device *pdev)
{
    struct xrp_hw_simple *     hw = CamDevmKmalloc(&pdev->dev, sizeof(*hw), GFP_KERNEL);
    const struct of_device_id *match;
    long (*init)(struct platform_device * pdev, struct xrp_hw_simple * hw);
    long ret;

    if (!hw)
        return -ENOMEM;

    match = of_match_device(of_match_ptr(xrp_hw_simple_match), &pdev->dev);
    if (!match)
        return -ENODEV;

    init = match->data;
    ret  = init(pdev, hw);
    if (IS_ERR_VALUE(ret))
    {
        xrp_deinit(pdev);
        return ret;
    }
    else
    {
        hw->xrp = ERR_PTR(ret);
        return 0;
    }
}

static int xrp_hw_simple_remove(struct platform_device *pdev)
{
    return xrp_deinit(pdev);
}

static const struct dev_pm_ops xrp_hw_simple_pm_ops = {
    SET_RUNTIME_PM_OPS(xrp_runtime_suspend, xrp_runtime_resume, NULL)};

static struct platform_driver xrp_hw_simple_driver = {
    .probe  = xrp_hw_simple_probe,
    .remove = xrp_hw_simple_remove,
    .driver =
        {
            .name           = DRIVER_NAME,
            .of_match_table = of_match_ptr(xrp_hw_simple_match),
            .pm             = &xrp_hw_simple_pm_ops,
        },
};

static int __init __driver_sstar_xrp_init(void)
{
    dsp_ut_init();
    return CamPlatformDriverRegister(&xrp_hw_simple_driver);
}
module_init(__driver_sstar_xrp_init);
static void __exit __driver_sstar_xrp_exit(void)
{
    dsp_ut_exit();
    CamPlatformDriverUnregister(&xrp_hw_simple_driver);
}
module_exit(__driver_sstar_xrp_exit);
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("XRP: low level device driver for Xtensa Remote Processing");
MODULE_LICENSE("Dual MIT/GPL");
