/*
 * pcie_bench.c - Sigmastar
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

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h> /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/pci_regs.h>
#include <linux/crc32.h>
#include <linux/random.h>

#include "ms_msys.h"
#include "ms_platform.h"
#include "registers.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SIGMASTAR");
MODULE_DESCRIPTION("PCIe LOOPBACK UT");

#define PCIE_UT_VERSION "v0.1.5"

//#define PCIE_LPBK_DBG
#ifdef PCIE_LPBK_DBG
#define FUNC_ENTRY_DBG(dev) dev_err(dev, "%s(%d) ++\r\n", __func__, __LINE__)
#define FUNC_EXIT_DBG(dev)  dev_err(dev, "%s(%d) --\r\n", __func__, __LINE__)
#else
#define FUNC_ENTRY_DBG(dev)
#define FUNC_EXIT_DBG(dev)
#endif

#define CLKGEN_BASE    0x1F207000 // bank 1038
#define CLKGEN2_BASE   0x1F207E00 // bank 103F
#define PCIE0_PHY_BASE 0x1F2C7400 // bank 163A
#define PCIE1_PHY_BASE 0x1F2C7600 // bank 163B
#define PCIE0_MAC_BASE 0x1F2C7800 // bank 163C
#define PCIE1_MAC_BASE 0x1F2C7A00 // bank 163D
#define PCIE0_X32_BASE 0x1F350000 // bank 1A80
#define PCIE1_X32_BASE 0x1F351000 // bank 1A88
#define PCIE0_CFG_BASE 0x1A000000
#define PCIE1_CFG_BASE 0x1B000000

// X32 port logic
#define PCIE_LINK_WIDTH_SPEED_CONTROL 0x80C
#define PORT_LOGIC_SPEED_CHANGE       (0x1 << 17)
#define PORT_LOGIC_AUTO_LANE_FLIP     (0x1 << 16)
#define PORT_LOGIC_LINK_WIDTH_2_LANES (0x2 << 8)
#define PORT_LOGIC_FAST_TRAIN_SEQ(s)  ((s)&0xFF)

// iATU unroll registers
#define PCIE_ATU_UNR_REGION_CTRL1 0x00
#define PCIE_ATU_UNR_REGION_CTRL2 0x04
#define PCIE_ATU_UNR_LOWER_BASE   0x08
#define PCIE_ATU_UNR_UPPER_BASE   0x0C
#define PCIE_ATU_UNR_LIMIT        0x10
#define PCIE_ATU_UNR_LOWER_TARGET 0x14
#define PCIE_ATU_UNR_UPPER_TARGET 0x18
/* Parameters for the waiting for iATU enabled routine */
#define LINK_WAIT_MAX_IATU_RETRIES 5
#define LINK_WAIT_IATU             9

#define PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(region) ((region) << 9)
#define PCIE_GET_ATU_INB_UNR_REG_OFFSET(region)  (((region) << 9) | BIT(8))

#define PCIE_ATU_ENABLE (0x1 << 31)

//
#define PCIE_TLP_TYPE_MEM  (0x0 << 0)
#define PCIE_TLP_TYPE_IO   (0x2 << 0)
#define PCIE_TLP_TYPE_CFG0 (0x4 << 0)
#define PCIE_TLP_TYPE_CFG1 (0x5 << 0)

// DMA related
#define DMA_LL_MAX_NUM  16
#define DMA_XFER_SIZE   0x200000
#define MEM_VERIFY_SIZE 0x20000 // 128KB
#define DMA_VERIFY_SIZE 0x400   // 1KB

#define PORT_LOGIC_LTSSM_STATE_MASK 0x1f
#define PORT_LOGIC_LTSSM_STATE_L0   0x11
/* Parameters for the waiting for link up routine */
#define LINK_WAIT_MAX_RETRIES (200)
#define LINK_WAIT_MSLEEP      (100)

/*
 * Linked List Element
 */
struct ll_element
{
    u32 cb : 1;  // Cycle Bit
    u32 tcb : 1; // Toggle Cycle Bit
    u32 llp : 1; // Load Link Pointer
    u32 lie : 1; // Local Interrupt Enable
    u32 rie : 1; // Remote Interrupt Enable
    u32 resv_5 : 27;
    u32 xfer_size; // transfer size
    u32 sar_l;     // source address low 32-bit
    u32 sar_h;     // source address high 32-bit
    u32 dar_l;     // destination address low 32-bit
    u32 dar_h;     // destination address high 32-bit
};

/*
 * handler
 */
struct pcie_handle
{
    struct device *dev;
    dev_t          devno;
    void __iomem * dbi0_base;
    void __iomem * dbi1_base;
    void __iomem * cfg0_base;
    void __iomem * cfg1_base;
    void *         phy0_riu;    // bank 163A
    void *         phy1_riu;    // bank 163B
    void *         mac0_riu;    // bank 163C
    void *         mac1_riu;    // bank 163D
    void *         clkgen_riu;  // bank 1038
    void *         clkgen2_riu; // bank 103F
    // DMA parts
    u32                desclist_sz;
    struct ll_element *desclist;
    dma_addr_t         desclist_dma;
    void *             lmem_base;    // local memory base VA
    void *             lmem;         // local memory VA, physical in 1MB aligned
    void *             rmem_base;    // remote memory base VA
    void *             rmem;         // remote memory VA, physical in 1MB aligned
    dma_addr_t         lmem_basedma; // local memory base PA
    dma_addr_t         lmem_dma;     // local memory PA in 1MB aligned
    dma_addr_t         rmem_basedma; // remote memory base PA
    dma_addr_t         rmem_dma;     // remote memory PA in 1MB aligned
};

#define PCIE_DEVNAME "pcie_bench"

static struct pcie_handle pcie_ctrl;
static u64                sys_dma_mask = 0xffffffffUL;

int __pcie_read(void __iomem *addr, int size, u32 *val)
{
    if ((uintptr_t)addr & (size - 1))
    {
        *val = 0;
        return -1;
    }

    if (size == 4)
    {
        *val = readl(addr);
    }
    else if (size == 2)
    {
        *val = readw(addr);
    }
    else if (size == 1)
    {
        *val = readb(addr);
    }
    else
    {
        *val = 0;
        return -1;
    }

    return 0;
}

int __pcie_write(void __iomem *addr, int size, u32 val)
{
    if ((uintptr_t)addr & (size - 1))
        return -1;

    if (size == 4)
        writel(val, addr);
    else if (size == 2)
        writew(val, addr);
    else if (size == 1)
        writeb(val, addr);
    else
        return -1;

    return 0;
}

u32 __dw_pcie_read_dbi(struct device *dev, void __iomem *base, u32 reg, size_t size)
{
    int ret;
    u32 val;

    ret = __pcie_read(base + reg, size, &val);
    if (ret)
        dev_err(dev, "Read DBI address failed\n");

    return val;
}

void __dw_pcie_write_dbi(struct device *dev, void __iomem *base, u32 reg, size_t size, u32 val)
{
    int ret;

    ret = __pcie_write(base + reg, size, val);
    if (ret)
        dev_err(dev, "Write DBI address failed\n");
}

#if (1)
static void __dw_pcie_writel_ob_unroll(struct pcie_handle *ctrl, u32 index, u32 reg, u32 val)
{
    u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

    OUTREG16(ctrl->mac0_riu + 0x3D * 4, 0x0000); // reg_dbi_addr_20to12
    OUTREG16(ctrl->mac0_riu + 0x3E * 4, 0x0C00); // reg_dbi_addr_32to21, bit[32:31] = 2'b11
    reg = offset + reg;
    if (reg > 0x1000)
    {                                                              // iATU address is over 4KB
        OUTREG16(ctrl->mac0_riu + 0x3D * 4, (reg >> 12) & 0x01FF); // reg_dbi_addr_20to12
        reg = reg & 0xFFF;
    }
    // write iATU register
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, reg, 4, val);

    OUTREG16(ctrl->mac0_riu + 0x3D * 4, 0x0000); // reg_dbi_addr_20to12
    OUTREG16(ctrl->mac0_riu + 0x3E * 4, 0x0000); // reg_dbi_addr_32to21
}

static u32 __dw_pcie_readl_ob_unroll(struct pcie_handle *ctrl, u32 index, u32 reg)
{
    u32 data;
    u32 offset = PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(index);

    OUTREG16(ctrl->mac0_riu + 0x3D * 4, 0x0000); // reg_dbi_addr_20to12
    OUTREG16(ctrl->mac0_riu + 0x3E * 4, 0x0C00); // reg_dbi_addr_32to21, bit[32:31] = 2'b11
    reg = offset + reg;
    if (reg > 0x1000)
    {                                                              // iATU address is over 4KB
        OUTREG16(ctrl->mac0_riu + 0x3D * 4, (reg >> 12) & 0x01FF); // reg_dbi_addr_20to12
        reg = reg & 0xFFF;
    }
    // read iATU register
    data = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, reg, 4);

    OUTREG16(ctrl->mac0_riu + 0x3D * 4, 0x0000); // reg_dbi_addr_20to12
    OUTREG16(ctrl->mac0_riu + 0x3E * 4, 0x0000); // reg_dbi_addr_32to21
    return data;
}

static void __dw_pcie_prog_ob_atu_unroll(struct pcie_handle *ctrl, int index, int type, u64 cpu_addr, u64 pci_addr,
                                         u32 size)
{
    u32 retries, val;

    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_LOWER_BASE, lower_32_bits(cpu_addr));
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_UPPER_BASE, upper_32_bits(cpu_addr));
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_LIMIT, lower_32_bits(cpu_addr + size - 1));
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_LOWER_TARGET, lower_32_bits(pci_addr));
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_UPPER_TARGET, upper_32_bits(pci_addr));
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_REGION_CTRL1, type);
    __dw_pcie_writel_ob_unroll(ctrl, index, PCIE_ATU_UNR_REGION_CTRL2, PCIE_ATU_ENABLE);

    /*
     * Make sure ATU enable takes effect before any subsequent config
     * and I/O accesses.
     */
    for (retries = 0; retries < LINK_WAIT_MAX_IATU_RETRIES; retries++)
    {
        val = __dw_pcie_readl_ob_unroll(ctrl, index, PCIE_ATU_UNR_REGION_CTRL2);
        if (val & PCIE_ATU_ENABLE)
            return;
        mdelay(LINK_WAIT_IATU);
    }
    dev_err(ctrl->dev, "Outbound iATU is not being enabled\n");
}
#endif
/* setup handle and device */
static int _init(struct pcie_handle *ctrl)
{
    int            ret = 0;
    struct device *dev;

    memset(ctrl, 0, sizeof(struct pcie_handle));

    // alloc character device number
    ret = alloc_chrdev_region(&ctrl->devno, 0, 254, PCIE_DEVNAME);
    if (ret)
    {
        printk("alloc_chrdev_region failed!\n");
        return ret;
    }
    // printk("dev major: %d, minor: %d\n", MAJOR(ctrl->devno), MINOR(ctrl->devno));
    dev       = device_create(msys_get_sysfs_class(), NULL, ctrl->devno, NULL, "pcie_lpbk");
    ctrl->dev = dev;

    FUNC_ENTRY_DBG(dev);

    ctrl->dev->dma_mask          = &sys_dma_mask;
    ctrl->dev->coherent_dma_mask = sys_dma_mask;
#ifdef CONFIG_ARM64
    // Because this driver use character dirver mode not plateform-bus-driver mode.
    // So we need to configure the dma to register swiotlb ops.
    of_dma_configure(dev, pcie_ctrl.dev->of_node, true);

    // Set the mask of DMA to support DRAM bigger than 4GB
    ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
    if (ret)
    {
        dev_err(dev, "no suitable DMA available\n");
        return ret;
    }
#endif
    // riu base
    ctrl->clkgen_riu  = (void *)CLKGEN_BASE;
    ctrl->clkgen2_riu = (void *)CLKGEN2_BASE;
    ctrl->phy0_riu    = (void *)PCIE0_PHY_BASE;
    ctrl->mac0_riu    = (void *)PCIE0_MAC_BASE;
    ctrl->phy1_riu    = (void *)PCIE1_PHY_BASE;
    ctrl->mac1_riu    = (void *)PCIE1_MAC_BASE;
    // remap pcie x32 register base, 4KB
    ctrl->dbi0_base = devm_ioremap(dev, PCIE0_X32_BASE, 0x1000);
    ctrl->dbi1_base = devm_ioremap(dev, PCIE1_X32_BASE, 0x1000);
    // cpu base for pcie0 to pcie1, 8MB
    ctrl->cfg0_base = devm_ioremap(dev, PCIE0_CFG_BASE, 0x800000);
    ctrl->cfg1_base = devm_ioremap(dev, PCIE1_CFG_BASE, 0x800000);
    /*
    printk("clkgen  base %px\r\n", ctrl->clkgen_riu);
    printk("clkgen2 base %px\r\n", ctrl->clkgen2_riu);
    printk("mac0    base %px\r\n", ctrl->mac0_riu);
    printk("mac1    base %px\r\n", ctrl->mac1_riu);
    printk("phy0    base %px\r\n", ctrl->phy0_riu);
    printk("phy1    base %px\r\n", ctrl->phy1_riu);
    printk("dbi0    base %px\r\n", ctrl->dbi0_base);
    printk("dbi1    base %px\r\n", ctrl->dbi1_base);
    printk("cfg0    base %px\r\n", ctrl->cfg0_base);
    printk("cfg1    base %px\r\n", ctrl->cfg1_base);
    */
    // alloc descritor list
    // due to desclist_next locate at offset 0x200,so reserve size need to more than 0x200+0x18
    ctrl->desclist_sz = DMA_LL_MAX_NUM * sizeof(struct ll_element) * 2;
    ctrl->desclist    = dma_alloc_coherent(ctrl->dev, ctrl->desclist_sz, &ctrl->desclist_dma, GFP_KERNEL);
    if (!ctrl->desclist)
    {
        dev_err(ctrl->dev, "unable to alloc desc list\r\n");
        return -1;
    }
    // alloc local memory
    ctrl->lmem_base = dma_alloc_coherent(ctrl->dev, DMA_XFER_SIZE, &ctrl->lmem_basedma, GFP_KERNEL);
    if (!ctrl->lmem_base)
    {
        dev_err(ctrl->dev, "unable to alloc local memory\r\n");
        return -2;
    }
    // alloc remote memory
    ctrl->rmem_base = dma_alloc_coherent(ctrl->dev, DMA_XFER_SIZE, &ctrl->rmem_basedma, GFP_KERNEL);
    if (!ctrl->rmem_base)
    {
        dev_err(ctrl->dev, "unable to alloc remote memory\r\n");
        return -3;
    }
    /*
#ifdef CONFIG_ARM64
    printk("desc list pa: %llx, va: %px\r\n", ctrl->desclist_dma, (void *)ctrl->desclist);
    printk("local mem pa: %llx, va: %px\r\n", ctrl->lmem_basedma, ctrl->lmem_base);
    printk("desti mem pa: %llx, va: %px\r\n", ctrl->rmem_basedma, ctrl->rmem_base);
#else
    printk("desc list pa: %x, va: %px\r\n", ctrl->desclist_dma, (void *)ctrl->desclist);
    printk("local mem pa: %x, va: %px\r\n", ctrl->lmem_basedma, ctrl->lmem_base);
    printk("desti mem pa: %x, va: %px\r\n", ctrl->rmem_basedma, ctrl->rmem_base);
#endif
    */
    FUNC_EXIT_DBG(dev);
    return 0;
}

static void _deinit(struct pcie_handle *ctrl)
{
    struct device *dev = pcie_ctrl.dev;

    FUNC_ENTRY_DBG(dev);

    if (ctrl->rmem_basedma && ctrl->rmem_base)
        dma_free_coherent(ctrl->dev, DMA_XFER_SIZE, ctrl->rmem_base, ctrl->rmem_basedma);

    if (ctrl->lmem_basedma && ctrl->lmem_base)
        dma_free_coherent(ctrl->dev, DMA_XFER_SIZE, ctrl->lmem_base, ctrl->lmem_basedma);

    if (ctrl->desclist_dma && ctrl->desclist)
        dma_free_coherent(ctrl->dev, ctrl->desclist_sz, (void *)ctrl->desclist, ctrl->desclist_dma);

    if (ctrl->dbi0_base)
        devm_iounmap(dev, ctrl->dbi0_base);
    if (ctrl->dbi1_base)
        devm_iounmap(dev, ctrl->dbi1_base);
    if (ctrl->cfg0_base)
        devm_iounmap(dev, ctrl->cfg0_base);
    if (ctrl->cfg1_base)
        devm_iounmap(dev, ctrl->cfg1_base);

    FUNC_EXIT_DBG(ctrl->dev);
}

/* dump configure space of pcie0 & pcie1 */
#if 0 // def PCIE_LPBK_DBG
static void _dump_cfg_space(struct pcie_handle *ctrl)
{
	u32 i;
	
	FUNC_ENTRY_DBG(ctrl->dev);
	printk("=============== PCIE0 Header ==============\r\n");
	for(i = 0; i < 0x40; i+=16) {
		printk("x%03X: %08X %08X %08X %08X\r\n", i,
												readl(ctrl->dbi0_base+i),
												readl(ctrl->dbi0_base+i+4),
												readl(ctrl->dbi0_base+i+8),
												readl(ctrl->dbi0_base+i+12));
	}
	printk("============= PCIE0 Capability ============\r\n");
	for(i = 0x40; i < 0x400; i+=16) {
		printk("x%03X: %08X %08X %08X %08X\r\n", i,
												readl(ctrl->dbi0_base+i),
												readl(ctrl->dbi0_base+i+4),
												readl(ctrl->dbi0_base+i+8),
												readl(ctrl->dbi0_base+i+12));
	}
	printk("=============== PCIE1 Header ==============\r\n");
	for(i = 0; i < 0x40; i+=16) {
		printk("x%03X: %08X %08X %08X %08X\r\n", i,
												readl(ctrl->dbi1_base+i),
												readl(ctrl->dbi1_base+i+4),
												readl(ctrl->dbi1_base+i+8),
												readl(ctrl->dbi1_base+i+12));
	}
	printk("============= PCIE1 Capability ============\r\n");
	for(i = 0x40; i < 0x400; i+=16) {
		printk("x%03X: %08X %08X %08X %08X\r\n", i,
												readl(ctrl->dbi1_base+i),
												readl(ctrl->dbi1_base+i+4),
												readl(ctrl->dbi1_base+i+8),
												readl(ctrl->dbi1_base+i+12));
	}
	FUNC_EXIT_DBG(ctrl->dev);
}
#else
#define _dump_cfg_space(c) \
    do                     \
    {                      \
    } while (0);
#endif

#if (1)
// ref 100M_Clock reg-setting
static void sstar_pcieif_internalclk_en(struct pcie_handle *ctrl)
{
    // u16 u16regdat = 0;
    FUNC_ENTRY_DBG(ctrl->dev);

    // upll1 for pcie//
    //  xtal_atop wriu 0x00111b12 0x00
    OUTREGMSK16(BASE_REG_XTAL_ATOP_PA + REG_ID_09, 0, 0x00FF);

    //+++wriu 0x00103003 0x00
    OUTREGMSK16(BASE_REG_MPLL_PA + REG_ID_01, 0, 0xFF00);

    if (1) //(id == 0)
    {
        // wriu 0x00141f46 0x32
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_23, 0x32, 0x00FF);

        // wriu 0x00141f4e 0xbc
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_27, 0xbc, 0x00FF);

        // wriu 0x00141f00 0x00
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_00, 0x00, 0x00FF);

        // wriu 0x00141f40 0x20
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_20, 0x20, 0x00FF);

        // wriu 0x00141f4a 0xaf
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_25, 0xaf, 0x00FF);

        // wriu 0x00141f4c 0x80
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_26, 0x80, 0x00FF);

        //+++wriu 0x00141f4d 0x81
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_26, 0x8100, 0xFF00);

        //+++wriu 0x00141f43 0x01
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_21, 0x0100, 0xFF00);

        // SATA
        OUTREG16(BASE_REG_SATA_MAC_PA + REG_ID_00, 0);
        OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6E, 0);
        OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6C, 0);
        OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_46, 0);

        CLRREG16(BASE_REG_SATA_MAC2_PA + REG_ID_00, BIT(12));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_14, BIT(0));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_14, BIT(12));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_15, BIT(0) | BIT(1));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_16, BIT(10) | BIT(11));

        // u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
        // printk("chk1=0x%x \r\n", u16regdat);
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_31, BIT(1) | BIT(2));
        // u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
        // printk("chk2=0x%x \r\n", u16regdat);
    }

    if (1) //(id == 1)
    {
        // PCIE1
        OUTREG16(ctrl->phy1_riu + (0x02 << 2), 0x0000); // Select ref_pad_clk_p/m

        // aupll for pcie//
        // wriu 0x00111b12 0x00
        //+++wriu 0x00103003 0x00
        // wriu 0x00141d4e 0xbc
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_27, 0xbc, 0x00FF);

        // wriu 0x00141d46 0x19
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_23, 0x19, 0x00FF);

        /// wriu 0x00141d00 0x00
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_00, 0x00, 0x00FF);

        // wriu 0x00141d40 0x20
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_20, 0x20, 0x00FF);

        // wriu 0x00141d4a 0xa7
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_25, 0xa7, 0x00FF);

        // wriu 0x00141d4c 0x80
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_26, 0x80, 0x00FF);

        //+++wriu 0x00141d4d 0x81
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_26, 0x8100, 0xFF00);
    }
    FUNC_EXIT_DBG(ctrl->dev);
}

#if (0)
{
    u16 u16regdat = 0;

    FUNC_ENTRY_DBG(ctrl->dev);

    // upll1 for pcie//
    //  xtal_atop wriu 0x00111b12 0x00
    u16regdat = INREG16(0x1f000000 + (0x111b * 0x200) + 0x12 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x111b * 0x200) + 0x12 * 2, u16regdat);

    //+++wriu 0x00103003 0x00
    u16regdat = INREG16(0x1f000000 + (0x1030 * 0x200) + 0x02 * 2);
    u16regdat &= 0xFF;
    OUTREG16(0x1f000000 + (0x1030 * 0x200) + 0x02 * 2, u16regdat);

    // wriu 0x00141f46 0x32
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x46 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x46 * 2, u16regdat | 0x32);
    // wriu 0x00141f4e 0xbc
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x4e * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x4e * 2, u16regdat | 0xbc);

    // wriu 0x00141f00 0x00
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x00 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x00 * 2, u16regdat);

    // wriu 0x00141f40 0x20
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x40 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x40 * 2, u16regdat | 0x20);

    // wriu 0x00141f4a 0xaf
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x4a * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x4a * 2, u16regdat | 0xaf);

    // wriu 0x00141f4c 0x80
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x4c * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x4c * 2, u16regdat | 0x80);

    //+++wriu 0x00141f4d 0x81
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x4c * 2);
    u16regdat &= 0xFF;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x4c * 2, u16regdat | 0x8100);

    //+++wriu 0x00141f43 0x01
    u16regdat = INREG16(0x1f000000 + (0x141f * 0x200) + 0x42 * 2);
    u16regdat &= 0xFF;
    OUTREG16(0x1f000000 + (0x141f * 0x200) + 0x42 * 2, u16regdat | 0x0100);

    // SATA
    OUTREG16(0x1f000000 + (0x143a * 0x200) + 0 * 4, 0);
    OUTREG16(0x1f000000 + (0x1038 * 0x200) + 0x6e * 4, 0);
    OUTREG16(0x1f000000 + (0x1038 * 0x200) + 0x6c * 4, 0);
    OUTREG16(0x1f000000 + (0x1038 * 0x200) + 0x46 * 4, 0);

    u16regdat = INREG16(0x1f000000 + (0x1441 * 0x200) + 0x00 * 4);
    u16regdat &= ~(BIT(12));
    OUTREG16(0x1f000000 + (0x1441 * 0x200) + 0x00 * 4, u16regdat);

    u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x14 * 4);
    u16regdat |= BIT(0);
    OUTREG16(0x1f000000 + (0x143d * 0x200) + 0x14 * 4, u16regdat);

    u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x14 * 4);
    u16regdat |= BIT(12);
    OUTREG16(0x1f000000 + (0x143d * 0x200) + 0x14 * 4, u16regdat);

    u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x15 * 4);
    u16regdat |= BIT(0) | BIT(1);
    OUTREG16(0x1f000000 + (0x143d * 0x200) + 0x15 * 4, u16regdat);

    u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x16 * 4);
    u16regdat |= BIT(10) | BIT(11);
    OUTREG16(0x1f000000 + (0x143d * 0x200) + 0x16 * 4, u16regdat);

    u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
    // printk("1regdat=0x%x \r\n", u16regdat);
    u16regdat |= BIT(1) | BIT(2);
    // printk("2regdat=0x%x \r\n", u16regdat);
    OUTREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4, u16regdat);
    // u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
    // printk("3regdat=0x%x \r\n", u16regdat);

    // PCIE1

    // aupll for pcie//
    // wriu 0x00111b12 0x00
    //+++wriu 0x00103003 0x00
    // wriu 0x00141d4e 0xbc
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x4e * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x4e * 2, u16regdat | 0xbc);

    // wriu 0x00141d46 0x19
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x46 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x46 * 2, u16regdat | 0x19);

    /// wriu 0x00141d00 0x00
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x00 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x00 * 2, u16regdat);

    // wriu 0x00141d40 0x20
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x40 * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x40 * 2, u16regdat | 0x20);

    // wriu 0x00141d4a 0xa7
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x4a * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x4a * 2, u16regdat | 0xa7);

    // wriu 0x00141d4c 0x80
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x4c * 2);
    u16regdat &= 0xFF00;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x4c * 2, u16regdat | 0x80);

    //+++wriu 0x00141d4d 0x81
    u16regdat = INREG16(0x1f000000 + (0x141d * 0x200) + 0x4c * 2);
    u16regdat &= 0xFF;
    OUTREG16(0x1f000000 + (0x141d * 0x200) + 0x4c * 2, u16regdat | 0x8100);

    FUNC_EXIT_DBG(ctrl->dev);
}

#endif

static int _self_macphy_port0_loopback(struct pcie_handle *ctrl)
{
    u32 data    = 0;
    u32 dataII  = 0;
    u32 TESTNUM = 0;

    FUNC_ENTRY_DBG(ctrl->dev);

    OUTREG16(ctrl->phy0_riu + 0x0A * 2, 1); // 2021/11/08 reg_phy_vreg_bypass setting to 1.8V

    // enable clocks
    OUTREG16(ctrl->clkgen_riu + 0xA0 * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x8A * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x96 * 2, 0x0000);

    OUTREG16(ctrl->mac0_riu + 0x04 * 2, 0x0004); // pcie0 reg_pcie_device_type RC mode
    OUTREG16(ctrl->mac1_riu + 0x04 * 2, 0x0000); // pcie1 reg_pcie_device_type EP mode

    // set pcie0 slv_read_bypass as 1, that is, waiting for PCIe1 CLP_D
    OUTREG16(ctrl->phy0_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass
    // set pcie1 slv_read_bypass as 1, that is, waiting for PCIe0 CLP_D
    OUTREG16(ctrl->phy1_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass

    // reg_slv_wstrb_mask = h'f to allow 32-bit write
    OUTREG16(ctrl->mac0_riu + 0x56 * 2, 0xFFFF);
    OUTREG16(ctrl->mac1_riu + 0x56 * 2, 0xFFFF);

    // Enable PCIe1 LTSSM by writing RIU offset 0x60 bit[4] as 1
    OUTREG16(ctrl->mac1_riu + 0xc0 * 2, 0x0010);                            // wriu -w 0x163dc0 0x0010
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, 0x0004);                            // wriu -w 0x163c50 0x0004
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x50 * 2, 4, 0x10002);  // wriu -w 0x1a8050 0x10002
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x406 * 2, 4, 0x1022c); // wriu -w 0x1a8406 0x1022c
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x406 * 2, 4, 0x3022c); // wriu -w 0x1a8c06 0x3022c
    OUTREG16(ctrl->mac0_riu + 0xc0 * 2, 0x0010);                            // wriu -w 0x163cc0 0x0010

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x3c * 2, 4, 0x102830);   // wriu -w 0x1a883c 0x102830 //start here
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x3c * 2, 4, 0x102830);   // wriu -w 0x1a803c 0x102830
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0a * 2, 4, 0x0000);     // wriu -w 0x1a880a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x08 * 2, 4, 0xc6f00004); // wriu -w 0x1a8808 0xc6f00004
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0c * 2, 4, 0x2ae00000); // wriu -w 0x1a880c 0x2ae00000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x18 * 2, 4, 0x2f310001); // wriu -w 0x1a8818 0x2f310001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x10 * 2, 4, 0x2101);     // wriu -w 0x1a8810 0x2101

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0e * 2, 4, 0x0101);     // wriu -w 0x1a800e 0x0101
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x0000);     // wriu -w 0x1a8018 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0e * 2, 4, 0x2020);     // wriu -w 0x1a800e 0x2020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x12 * 2, 4, 0x10001);    // wriu -w 0x1a8012 0x10001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x14 * 2, 4, 0x0000);     // wriu -w 0x1a8014 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x16 * 2, 4, 0x0000);     // wriu -w 0x1a8016 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x12 * 2, 4, 0xc6f0c6f0); // wriu -w 0x1a8012 0xc6f0c6f0
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x10 * 2, 4, 0x2f302ae0); // wriu -w 0x1a8010 0x2f302ae0
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x02 * 2, 4, 0x110007);   // wriu -w 0x1a8002 0x110007

    ////PCIe Test Setting:
    ////PCIe0 LINK_CONTROL2_LINK_STATUS2 Target Link Speed as 2
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x50 * 2, 4, 0x10002); // wriu -w 0x1a8050 0x10002
    ////PCIe0 LINK_CONTROL2_LINK_STATUS2 Target Link Speed as 1
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x50 * 2, 4, 0x10001); // wriu -w 0x1a8050 0x10001

    ////PCIe0 Enables PIPE_LOOPBACK_CONTROL PIPE_LOOPBACK bit
    // enable for mode1[mac] , disable for mode2[phy]
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x45c * 2, 4, 0x80000003); //wriu -w 0x1a845c 0x80000003

    ////PCIe0 Enables PORT_LINK_CTRL LOOPBACK_ENABLE bit
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x388 * 2, 4, 0x430124); // wriu -w 0x1a8388 0x430124

    // polling LPBK_ACTIVE
    while (1)
    {
        data   = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x728, 4);
        dataII = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x72c, 4);
        data   = data & 0x3F;
        // printk("%x,%x", data, dataII);
        mdelay(2);
        if (data == 0x1B)
        {
            // printk("TESTNUM:%d\r\n", TESTNUM);
            break;
        }
        TESTNUM++;
        if (TESTNUM == 1000)
        {
            // printk("TESTNUM:%d\r\n", TESTNUM);
            return -1;
        }
    }

    // PCIe0 LTSSM enters RCVRY_LOCK state !!!
    ////PCIe0 writes One to clear AER/Unorrectable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x82 * 2, 4, 0x100000); // wriu -w 0x1a8082 0x100000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x82 * 2, 4, 0x0020);   // wriu -w 0x1a8082 0x0020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x82 * 2, 4, 0x0010);   // wriu -w 0x1a8082 0x0010
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x82 * 2, 4, 0x2000);   // wriu -w 0x1a8082 0x2000
    ////PCIe0 writes One to clear Correctable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x88 * 2, 4, 0x0001); // wriu -w 0x1a8088 0x0001
    // PCIe1 LTSSM enters RCVRY_LOCK state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x88 * 2, 4, 0x0080); // wriu -w 0x1a8088 0x0080
    // PCIe0 LTSSM enters RCVRY_RCVRCFG state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x88 * 2, 4, 0x0040); // wriu -w 0x1a8088 0x0040
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0e * 2, 4, 0x0101); // wriu -w 0x1a800e 0x0101
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    // PCIe1 LTSSM enters RCVRY_RCVRCFG state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0e * 2, 4, 0x2020);     // wriu -w 0x1a800e 0x2020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x12 * 2, 4, 0xfd11fd11); // wriu -w 0x1a8012 0xfd11fd11
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x14 * 2, 4, 0x03b9);     // wriu -w 0x1a8014 0x03b9
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x16 * 2, 4, 0x03b9);     // wriu -w 0x1a8016 0x03b9
    // PCIe0 LTSSM enters RCVRY_IDLE state !!!
    // PCIe0 LTSSM enters LPBK_ENTRY state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x10 * 2, 4, 0x78e05d20); // wriu -w 0x1a8010 0x78e05d20
    // PCIe0 LTSSM enters LPBK_ACTIVE state !!!
    // PCIe1 LTSSM enters LPBK_ENTRY state !!!
    // PCIe1 LTSSM enters LPBK_ACTIVE state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x02 * 2, 4, 0x110007); // wriu -w 0x1a8002 0x110007
    ////PCIe0 writes One to clear AER/Unorrectable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x82 * 2, 4, 0x100000); // wriu -w 0x1a8082 0x100000

    FUNC_EXIT_DBG(ctrl->dev);
    return 0;
}
#endif

#if (1)
static int _self_macphy_port1_loopback(struct pcie_handle *ctrl)
{
    u32 data    = 0;
    u32 dataII  = 0;
    u32 TESTNUM = 0;

    FUNC_ENTRY_DBG(ctrl->dev);

    OUTREG16(ctrl->phy0_riu + 0x0A * 2, 1); // 2021/11/08 reg_phy_vreg_bypass setting to 1.8V
    OUTREG16(ctrl->phy1_riu + 0x0A * 2, 1); // 2021/11/08 reg_phy_vreg_bypass setting to 1.8V

    // enable clocks
    OUTREG16(ctrl->clkgen_riu + 0xA0 * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x8A * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x96 * 2, 0x0000);

    OUTREG16(ctrl->mac1_riu + 0x04 * 2, 0x0004); // pcie0 reg_pcie_device_type RC mode
    OUTREG16(ctrl->mac0_riu + 0x04 * 2, 0x0000); // pcie1 reg_pcie_device_type EP mode

    // set pcie0 slv_read_bypass as 1, that is, waiting for PCIe1 CLP_D
    OUTREG16(ctrl->phy0_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass
    // set pcie1 slv_read_bypass as 1, that is, waiting for PCIe0 CLP_D
    OUTREG16(ctrl->phy1_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass

    // reg_slv_wstrb_mask = h'f to allow 32-bit write
    OUTREG16(ctrl->mac0_riu + 0x56 * 2, 0xFFFF);
    OUTREG16(ctrl->mac1_riu + 0x56 * 2, 0xFFFF);

    // Enable PCIe1 LTSSM by writing RIU offset 0x60 bit[4] as 1
    OUTREG16(ctrl->mac0_riu + 0xc0 * 2, 0x0010);                            // wriu -w 0x163dc0 0x0010
    OUTREG16(ctrl->mac1_riu + 0x50 * 2, 0x0004);                            // wriu -w 0x163c50 0x0004
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x50 * 2, 4, 0x10002);  // wriu -w 0x1a8050 0x10002
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x406 * 2, 4, 0x1022c); // wriu -w 0x1a8406 0x1022c
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x406 * 2, 4, 0x3022c); // wriu -w 0x1a8c06 0x3022c
    OUTREG16(ctrl->mac1_riu + 0xc0 * 2, 0x0010);                            // wriu -w 0x163cc0 0x0010

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x3c * 2, 4, 0x102830);   // wriu -w 0x1a883c 0x102830 //start here
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x3c * 2, 4, 0x102830);   // wriu -w 0x1a803c 0x102830
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0a * 2, 4, 0x0000);     // wriu -w 0x1a880a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x08 * 2, 4, 0xc6f00004); // wriu -w 0x1a8808 0xc6f00004
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0c * 2, 4, 0x2ae00000); // wriu -w 0x1a880c 0x2ae00000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x2f310001); // wriu -w 0x1a8818 0x2f310001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x10 * 2, 4, 0x2101);     // wriu -w 0x1a8810 0x2101

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0e * 2, 4, 0x0101);     // wriu -w 0x1a800e 0x0101
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x18 * 2, 4, 0x0000);     // wriu -w 0x1a8018 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0e * 2, 4, 0x2020);     // wriu -w 0x1a800e 0x2020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x12 * 2, 4, 0x10001);    // wriu -w 0x1a8012 0x10001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x14 * 2, 4, 0x0000);     // wriu -w 0x1a8014 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x16 * 2, 4, 0x0000);     // wriu -w 0x1a8016 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x12 * 2, 4, 0xc6f0c6f0); // wriu -w 0x1a8012 0xc6f0c6f0
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x10 * 2, 4, 0x2f302ae0); // wriu -w 0x1a8010 0x2f302ae0
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x02 * 2, 4, 0x110007);   // wriu -w 0x1a8002 0x110007

    ////PCIe Test Setting:
    ////PCIe0 LINK_CONTROL2_LINK_STATUS2 Target Link Speed as 2
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x50 * 2, 4, 0x10002); // wriu -w 0x1a8050 0x10002
    ////PCIe0 LINK_CONTROL2_LINK_STATUS2 Target Link Speed as 1
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x50 * 2, 4, 0x10001); // wriu -w 0x1a8050 0x10001

    ////PCIe0 Enables PIPE_LOOPBACK_CONTROL PIPE_LOOPBACK bit
    // enable for mode1[mac] , disable for mode2[phy]
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x45c * 2, 4, 0x80000003); // wriu -w 0x1a845c 0x80000003

    ////PCIe0 Enables PORT_LINK_CTRL LOOPBACK_ENABLE bit
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x388 * 2, 4, 0x430124); // wriu -w 0x1a8388 0x430124

    // polling LPBK_ACTIVE
    while (1)
    {
        data   = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi1_base, 0x728, 4);
        dataII = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi1_base, 0x72c, 4);
        data   = data & 0x3F;
        // printk("%x,%x", data, dataII);
        mdelay(2);
        if (data == 0x1B)
        {
            // printk("TESTNUM:%d\r\n", TESTNUM);
            break;
        }
        TESTNUM++;
        if (TESTNUM == 1000)
        {
            // printk("TESTNUM:%d\r\n", TESTNUM);
            return -1;
        }
    }

    // PCIe0 LTSSM enters RCVRY_LOCK state !!!
    ////PCIe0 writes One to clear AER/Unorrectable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x82 * 2, 4, 0x100000); // wriu -w 0x1a8082 0x100000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x82 * 2, 4, 0x0020);   // wriu -w 0x1a8082 0x0020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x82 * 2, 4, 0x0010);   // wriu -w 0x1a8082 0x0010
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x82 * 2, 4, 0x2000);   // wriu -w 0x1a8082 0x2000
    ////PCIe0 writes One to clear Correctable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x88 * 2, 4, 0x0001); // wriu -w 0x1a8088 0x0001
    // PCIe1 LTSSM enters RCVRY_LOCK state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x88 * 2, 4, 0x0080); // wriu -w 0x1a8088 0x0080
    // PCIe0 LTSSM enters RCVRY_RCVRCFG state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x88 * 2, 4, 0x0040); // wriu -w 0x1a8088 0x0040
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0e * 2, 4, 0x0101); // wriu -w 0x1a800e 0x0101
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    // PCIe1 LTSSM enters RCVRY_RCVRCFG state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0e * 2, 4, 0x2020);     // wriu -w 0x1a800e 0x2020
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x12 * 2, 4, 0xfd11fd11); // wriu -w 0x1a8012 0xfd11fd11
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x14 * 2, 4, 0x03b9);     // wriu -w 0x1a8014 0x03b9
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x16 * 2, 4, 0x03b9);     // wriu -w 0x1a8016 0x03b9
    // PCIe0 LTSSM enters RCVRY_IDLE state !!!
    // PCIe0 LTSSM enters LPBK_ENTRY state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x10 * 2, 4, 0x78e05d20); // wriu -w 0x1a8010 0x78e05d20
    // PCIe0 LTSSM enters LPBK_ACTIVE state !!!
    // PCIe1 LTSSM enters LPBK_ENTRY state !!!
    // PCIe1 LTSSM enters LPBK_ACTIVE state !!!
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x02 * 2, 4, 0x110007); // wriu -w 0x1a8002 0x110007
    ////PCIe0 writes One to clear AER/Unorrectable Error Status Report bits
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x82 * 2, 4, 0x100000); // wriu -w 0x1a8082 0x100000

    FUNC_EXIT_DBG(ctrl->dev);
    return 0;
}
#endif

/* local & remote memory initial */
void _mem_setup(struct pcie_handle *ctrl)
{
    // u32 bar0, bar1, membase;
    ss_miu_addr_t src_miu, dst_miu;

    FUNC_ENTRY_DBG(ctrl->dev);

    // Initial src / dst buffer content
    memset(ctrl->lmem_base, 0x55, DMA_XFER_SIZE);
    memset(ctrl->rmem_base, 0xAA, DMA_XFER_SIZE);

    // Let src / dst PA to be 1MB aligned
    ctrl->lmem_dma = (ctrl->lmem_basedma + 0x100000 - 1) & ~(0xFFFFF);
    ctrl->rmem_dma = (ctrl->rmem_basedma + 0x100000 - 1) & ~(0xFFFFF);
    src_miu        = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma);
    dst_miu        = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    ctrl->lmem     = ctrl->lmem_base + (ctrl->lmem_dma - ctrl->lmem_basedma);
    ctrl->rmem     = ctrl->rmem_base + (ctrl->rmem_dma - ctrl->rmem_basedma);
    /*
#ifdef CONFIG_ARM64
    printk("src va base: %px, pa base: %llx\r\n", ctrl->lmem_base, ctrl->lmem_basedma);
    printk("src va: %px, pa: %llx, miu: %llx\r\n", ctrl->lmem, ctrl->lmem_dma, src_miu);
    printk("dst va base: %px, pa base: %llx\r\n", ctrl->rmem_base, ctrl->rmem_basedma);
    printk("dst va: %px, pa: %llx, miu: %llx\r\n", ctrl->rmem, ctrl->rmem_dma, dst_miu);
#else
    printk("src va base: %px, pa base: %x\r\n", ctrl->lmem_base, ctrl->lmem_basedma);
    printk("src va: %px, pa: %x, miu: %llx\r\n", ctrl->lmem, ctrl->lmem_dma, src_miu);
    printk("dst va base: %px, pa base: %x\r\n", ctrl->rmem_base, ctrl->rmem_basedma);
    printk("dst va: %px, pa: %x, miu: %llx\r\n", ctrl->rmem, ctrl->rmem_dma, dst_miu);
#endif
    */
    FUNC_EXIT_DBG(ctrl->dev);
}

#if (1)
static int _dma_verify_port0(struct pcie_handle *ctrl)
{
    u64                base, target;
    dma_addr_t         desc_orgin, desc_next;
    u32 *              pPatern;
    struct ll_element *desclist_next;
    struct ll_element *desclist_origin;
    // u32                i;
    u32 *addr;

    FUNC_ENTRY_DBG(ctrl->dev);

    _mem_setup(ctrl);

    // test 1: base is an invalid addr, use OB iATU index0 translate to valid addr
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    base   = target + DMA_XFER_SIZE;
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, base, target, DMA_XFER_SIZE);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    // OUTREG16(ctrl->mac0_riu + 0x58*2, (base >> 24) & 0xFFFF);   // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    // OUTREG16(ctrl->mac0_riu + 0x5A*2, (base >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    // OUTREG16(ctrl->mac0_riu + 0x5C*2, (base >> 56) & 0xFF);   // pcie0 reg_slv_addr_bank_63to56

    pPatern  = ctrl->lmem;
    *pPatern = 0xabcdef01;
    pPatern += 0x3F;
    *pPatern = 0xabcdef10;

    pPatern++; // 0x40
    *pPatern = 0xabcdef02;
    pPatern += 0x3F;
    *pPatern = 0xabcdef20;

    pPatern++; // 0x80
    *pPatern = 0xabcdef03;
    pPatern += 0x3F;
    *pPatern = 0xabcdef30;

    pPatern++; // 0xC0
    *pPatern = 0xabcdef04;
    pPatern += 0x3F;
    *pPatern = 0xabcdef40;

    desc_orgin      = ctrl->desclist_dma;
    desc_next       = (ctrl->desclist_dma) + 0x200;
    desclist_origin = (struct ll_element *)((void *)(ctrl->desclist));
    desclist_next   = (struct ll_element *)(((void *)(ctrl->desclist)) + 0x200);
    // printk("desclist_next=0x%px \r\n", (void *)desclist_next);

    // data element 0-0
    // printk("ctrl->desclist[W0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 1;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU(((ctrl->lmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->dar_h     = 0;

    // link element 0
    (ctrl->desclist)++;
    // printk("ctrl->desclist[W1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((desc_orgin & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    // printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    // printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_next;
    // data element 0-0
    // printk("ctrl->desclist[R0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF)) + 0x0600; // result is here
    ctrl->desclist->dar_h     = 0;

    // link element 0
    (ctrl->desclist)++;
    // printk("ctrl->desclist[R1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((desc_next & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    // printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    // printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_origin; // recover the pointer for later deinit usage

    Chip_Flush_MIU_Pipe();

    // Set PCIe0 dbi_addr[32:12] as h180380 for configure DMA register
    OUTREG16(ctrl->mac0_riu + 0x7a * 2, 0x0180);
    OUTREG16(ctrl->mac0_riu + 0x7c * 2, 0x0c01);

    // perform DMA write operation
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x96 * 2, 4,
        0x0001); // wriu -w 0x1a8096 0x0001 //This register controls power enable/disable for write channel 1.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x06 * 2, 4,
                        0x0001); // wriu -w 0x1a8006 0x0001 //This register indicates the status of DMA write engine and
                                 // the status of DMA write engine handshake feature (per-channel).
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x30 * 2, 4,
                        0x8781fff8); // wriu -w 0x1a8030 0x8781fff8 //This register holds the lower 32 bits of the Done
                                     // IMWr TLP address.
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x32 * 2, 4,
        0x0000); // wriu -w 0x1a8032 0x0000 //This register holds the higher 32 bits of the Done IMWr TLP address.
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x34 * 2, 4,
        0x8781fffc); // wriu -w 0x1a8034 0x8781fffc //This register holds the lower 32 bits of Abort IMWr TLP.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x36 * 2, 4,
                        0x0000); // wriu -w 0x1a8036 0x0000 //This register holds the higher 32 bits of Abort IMWr TLP.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x38 * 2, 4,
                        0xff700000); // wriu -w 0x1a8038 0xff700000 //This register holds the Channel 1 and 0 IMWr Done
                                     // or Abort TLP Data.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0C * 2, 4, 0x87e1); // wriu -w 0x1a800c 0x87e1
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x54 * 2, 4,
        0x0000); // wriu -w 0x1a8054 0x0000 //This register holds the Abort and Done DMA read interrupt mask.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x62 * 2, 4, 0xff00ff); // wriu -w 0x1a8062 0xff00ff
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x2a * 2, 4,
        0x0000); // wriu -w 0x1a802a 0x0000 //This register holds the Abort and Done DMA write interrupt mask.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x48 * 2, 4, 0xff00ff); // wriu -w 0x1a8048 0xff00ff

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0200 * 2, 4, 0x8000318);  // wriu -w 0x1a8200 0x8000318
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0202 * 2, 4, 0xc0000000); // wriu -w 0x1a8202 0xc0000000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0204 * 2, 4, 0x0800);     // wriu -w 0x1a8204 0x0800
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0206 * 2, 4, 0x0000);     // wriu -w 0x1a8206 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0208 * 2, 4, 0x0000);     // wriu -w 0x1a8208 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x020a * 2, 4, 0x0000);     // wriu -w 0x1a820a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x020c * 2, 4, 0x0000);     // wriu -w 0x1a820c 0x0000
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi0_base, 0x020e * 2, 4,
        Chip_Phys_to_MIU((desc_orgin & 0xFFFFFFFF))); // wriu -w 0x1a820e 0xffffff70 //This register holds the lower 32
                                                      // bits of the DMA write linked list pointer.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0210 * 2, 4,
                        0x0000); // wriu -w 0x1a8210 0x0000 //This register holds the higher 32 bits of the DMA write
                                 // linked list pointer.

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0xb4 * 2, 4, 0x0001);     // wriu -w 0x1a80b4 0x0001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x16 * 2, 4, 0x0001);     // wriu -w 0x1a8016 0x0001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x66 * 2, 4, 0x8781fff8); // wriu -w 0x1a8066 0x8781fff8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x68 * 2, 4, 0x0000);     // wriu -w 0x1a8068 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x6a * 2, 4, 0x8781fffc); // wriu -w 0x1a806a 0x8781fffc
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x6c * 2, 4, 0x0000);     // wriu -w 0x1a806c 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x6e * 2, 4, 0xffb8);     // wriu -w 0x1a806e 0xffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x1c * 2, 4, 0x843f);     // wriu -w 0x1a801c 0x843f
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x54 * 2, 4, 0x0000);     // wriu -w 0x1a8054 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x62 * 2, 4, 0xff00ff);   // wriu -w 0x1a8062 0xff00ff
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x2a * 2, 4, 0x0000);     // wriu -w 0x1a802a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x48 * 2, 4, 0xff00ff);   // wriu -w 0x1a8048 0xff00ff

    // perform DMA read operation
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0180 * 2, 4, 0x400318); // wriu -w 0x1a8180 0x400318
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0182 * 2, 4, 0x0000);   // wriu -w 0x1a8182 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0184 * 2, 4, 0x0800);   // wriu -w 0x1a8184 0x0800
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0186 * 2, 4, 0x0000);   // wriu -w 0x1a8186 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0188 * 2, 4, 0x0000);   // wriu -w 0x1a8188 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x018a * 2, 4, 0x0000);   // wriu -w 0x1a818a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x018c * 2, 4, 0x0000);   // wriu -w 0x1a818c 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x018e * 2, 4,
                        Chip_Phys_to_MIU((desc_next & 0xFFFFFFFF)));        // wriu -w 0x1a818e 0xffffffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0190 * 2, 4, 0x0000); // wriu -w 0x1a8190 0x0000

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x08 * 2, 4, 0x0001); // wriu -w 0x1a8008 0x0001
    // printk(" DMA Transfer.");
    // printk(" DMA Transfer..");
    // printk(" DMA Transfer... \r\n");
    mdelay(50);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    mdelay(50);
    // Waiting edma[1] interrupt !!!
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x2c * 2,    4,     0x0002);//wriu -w 0x1a802c 0x0002

    // printout result
    addr = ((ctrl->rmem) + 0x600); // result is here
    /*
    printk("addr:0x%lx \r\n", (unsigned long)addr);
    for (i = 0; i < 0x80; i += 4)
    {
        printk("%px: %08x %08x %08x %08x\r\n", addr + i, addr[i], addr[i + 1], addr[i + 2], addr[i + 3]);
    }
    */

    if (addr[0] != 0xabcdef01)
        return -1;
    if (addr[0x3f] != 0xabcdef10)
        return -1;
    if (addr[0x40] != 0xabcdef02)
        return -1;
    if (addr[0x7f] != 0xabcdef20)
        return -1;
    if (addr[0x80] != 0xabcdef03)
        return -1;
    if (addr[0xbf] != 0xabcdef30)
        return -1;
    if (addr[0xc0] != 0xabcdef04)
        return -1;
    if (addr[0xff] != 0xabcdef40)
        return -1;

    FUNC_EXIT_DBG(ctrl->dev);

    // invalidate for later port1 verification
    memset(ctrl->lmem_base, 0x55, DMA_XFER_SIZE);
    memset(ctrl->rmem_base, 0xAA, DMA_XFER_SIZE);
    return 0;
}
#endif

#if (1)
static int _dma_verify_port1(struct pcie_handle *ctrl)
{
    u64                base, target;
    dma_addr_t         desc_orgin, desc_next;
    u32 *              pPatern;
    struct ll_element *desclist_next;
    struct ll_element *desclist_origin;
    // u32                i;
    u32 *addr;

    FUNC_ENTRY_DBG(ctrl->dev);

    //_mem_setup(ctrl);

    // test 1: base is an invalid addr, use OB iATU index0 translate to valid addr
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    base   = target + DMA_XFER_SIZE;
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, base, target, DMA_XFER_SIZE);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac1_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    // OUTREG16(ctrl->mac0_riu + 0x58*2, (base >> 24) & 0xFFFF);   // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    // OUTREG16(ctrl->mac0_riu + 0x5A*2, (base >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    // OUTREG16(ctrl->mac0_riu + 0x5C*2, (base >> 56) & 0xFF);   // pcie0 reg_slv_addr_bank_63to56

    pPatern  = ctrl->lmem;
    *pPatern = 0xabcdef01;
    pPatern += 0x3F;
    *pPatern = 0xabcdef10;

    pPatern++; // 0x40
    *pPatern = 0xabcdef02;
    pPatern += 0x3F;
    *pPatern = 0xabcdef20;

    pPatern++; // 0x80
    *pPatern = 0xabcdef03;
    pPatern += 0x3F;
    *pPatern = 0xabcdef30;

    pPatern++; // 0xC0
    *pPatern = 0xabcdef04;
    pPatern += 0x3F;
    *pPatern = 0xabcdef40;

    desc_orgin      = ctrl->desclist_dma;
    desc_next       = (ctrl->desclist_dma) + 0x200;
    desclist_origin = (struct ll_element *)((void *)(ctrl->desclist));
    desclist_next   = (struct ll_element *)(((void *)(ctrl->desclist)) + 0x200);
    // printk("desclist_next=0x%px \r\n", (void *)desclist_next);

    // data element 0-0
    // printk("ctrl->desclist[W0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 1;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU(((ctrl->lmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->dar_h     = 0;

    // link element 0
    (ctrl->desclist)++;
    // printk("ctrl->desclist[W1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((desc_orgin & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    // printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    // printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_next;
    // data element 0-0
    // printk("ctrl->desclist[R0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = Chip_Phys_to_MIU(((ctrl->rmem_dma) & 0xFFFFFFFF)) + 0x0600; // result is here
    ctrl->desclist->dar_h     = 0;

    // link element 0
    (ctrl->desclist)++;
    // printk("ctrl->desclist[R1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((desc_next & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = 0;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    // printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    // printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_origin; // recover the pointer for later deinit usage

    Chip_Flush_MIU_Pipe();

    // Set PCIe0 dbi_addr[32:12] as h180380 for configure DMA register
    OUTREG16(ctrl->mac1_riu + 0x7a * 2, 0x0180);
    OUTREG16(ctrl->mac1_riu + 0x7c * 2, 0x0c01);

    // perform DMA write operation
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x96 * 2, 4,
        0x0001); // wriu -w 0x1a8096 0x0001 //This register controls power enable/disable for write channel 1.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x06 * 2, 4,
                        0x0001); // wriu -w 0x1a8006 0x0001 //This register indicates the status of DMA write engine and
                                 // the status of DMA write engine handshake feature (per-channel).
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x30 * 2, 4,
                        0x8781fff8); // wriu -w 0x1a8030 0x8781fff8 //This register holds the lower 32 bits of the Done
                                     // IMWr TLP address.
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x32 * 2, 4,
        0x0000); // wriu -w 0x1a8032 0x0000 //This register holds the higher 32 bits of the Done IMWr TLP address.
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x34 * 2, 4,
        0x8781fffc); // wriu -w 0x1a8034 0x8781fffc //This register holds the lower 32 bits of Abort IMWr TLP.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x36 * 2, 4,
                        0x0000); // wriu -w 0x1a8036 0x0000 //This register holds the higher 32 bits of Abort IMWr TLP.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x38 * 2, 4,
                        0xff700000); // wriu -w 0x1a8038 0xff700000 //This register holds the Channel 1 and 0 IMWr Done
                                     // or Abort TLP Data.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0C * 2, 4, 0x87e1); // wriu -w 0x1a800c 0x87e1
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x54 * 2, 4,
        0x0000); // wriu -w 0x1a8054 0x0000 //This register holds the Abort and Done DMA read interrupt mask.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x62 * 2, 4, 0xff00ff); // wriu -w 0x1a8062 0xff00ff
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x2a * 2, 4,
        0x0000); // wriu -w 0x1a802a 0x0000 //This register holds the Abort and Done DMA write interrupt mask.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x48 * 2, 4, 0xff00ff); // wriu -w 0x1a8048 0xff00ff

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0200 * 2, 4, 0x8000318);  // wriu -w 0x1a8200 0x8000318
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0202 * 2, 4, 0xc0000000); // wriu -w 0x1a8202 0xc0000000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0204 * 2, 4, 0x0800);     // wriu -w 0x1a8204 0x0800
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0206 * 2, 4, 0x0000);     // wriu -w 0x1a8206 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0208 * 2, 4, 0x0000);     // wriu -w 0x1a8208 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x020a * 2, 4, 0x0000);     // wriu -w 0x1a820a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x020c * 2, 4, 0x0000);     // wriu -w 0x1a820c 0x0000
    __dw_pcie_write_dbi(
        ctrl->dev, ctrl->dbi1_base, 0x020e * 2, 4,
        Chip_Phys_to_MIU((desc_orgin & 0xFFFFFFFF))); // wriu -w 0x1a820e 0xffffff70 //This register holds the lower 32
                                                      // bits of the DMA write linked list pointer.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0210 * 2, 4,
                        0x0000); // wriu -w 0x1a8210 0x0000 //This register holds the higher 32 bits of the DMA write
                                 // linked list pointer.

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0xb4 * 2, 4, 0x0001);     // wriu -w 0x1a80b4 0x0001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x16 * 2, 4, 0x0001);     // wriu -w 0x1a8016 0x0001
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x66 * 2, 4, 0x8781fff8); // wriu -w 0x1a8066 0x8781fff8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x68 * 2, 4, 0x0000);     // wriu -w 0x1a8068 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x6a * 2, 4, 0x8781fffc); // wriu -w 0x1a806a 0x8781fffc
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x6c * 2, 4, 0x0000);     // wriu -w 0x1a806c 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x6e * 2, 4, 0xffb8);     // wriu -w 0x1a806e 0xffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x1c * 2, 4, 0x843f);     // wriu -w 0x1a801c 0x843f
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x54 * 2, 4, 0x0000);     // wriu -w 0x1a8054 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x62 * 2, 4, 0xff00ff);   // wriu -w 0x1a8062 0xff00ff
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x2a * 2, 4, 0x0000);     // wriu -w 0x1a802a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x48 * 2, 4, 0xff00ff);   // wriu -w 0x1a8048 0xff00ff

    // perform DMA read operation
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0180 * 2, 4, 0x400318); // wriu -w 0x1a8180 0x400318
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0182 * 2, 4, 0x0000);   // wriu -w 0x1a8182 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0184 * 2, 4, 0x0800);   // wriu -w 0x1a8184 0x0800
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0186 * 2, 4, 0x0000);   // wriu -w 0x1a8186 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0188 * 2, 4, 0x0000);   // wriu -w 0x1a8188 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x018a * 2, 4, 0x0000);   // wriu -w 0x1a818a 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x018c * 2, 4, 0x0000);   // wriu -w 0x1a818c 0x0000
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x018e * 2, 4,
                        Chip_Phys_to_MIU((desc_next & 0xFFFFFFFF)));        // wriu -w 0x1a818e 0xffffffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0190 * 2, 4, 0x0000); // wriu -w 0x1a8190 0x0000

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x08 * 2, 4, 0x0001); // wriu -w 0x1a8008 0x0001
    // printk(" DMA Transfer.");
    // printk(" DMA Transfer..");
    // printk(" DMA Transfer... \r\n");
    mdelay(50);

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    mdelay(50);
    // Waiting edma[1] interrupt !!!
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x2c * 2,    4,     0x0002);//wriu -w 0x1a802c 0x0002

    // printout result
    addr = ((ctrl->rmem) + 0x600); // result is here
    /*
    printk("addr:0x%lx \r\n", (unsigned long)addr);
    for (i = 0; i < 0x80; i += 4)
    {
        printk("%px: %08x %08x %08x %08x\r\n", addr + i, addr[i], addr[i + 1], addr[i + 2], addr[i + 3]);
    }
    */

    if (addr[0] != 0xabcdef01)
        return -1;
    if (addr[0x3f] != 0xabcdef10)
        return -1;
    if (addr[0x40] != 0xabcdef02)
        return -1;
    if (addr[0x7f] != 0xabcdef20)
        return -1;

    if (addr[0x80] != 0xabcdef03)
        return -1;
    if (addr[0xbf] != 0xabcdef30)
        return -1;
    if (addr[0xc0] != 0xabcdef04)
        return -1;
    if (addr[0xff] != 0xabcdef40)
        return -1;

    FUNC_EXIT_DBG(ctrl->dev);
    return 0;
}
#endif

//------------------------------------------------------------------------------
//  Function    : LoopBack_Probe
//  Description :
//------------------------------------------------------------------------------
static int loopback_probe(void)
{
    int ret = 0;

    ret = _init(&pcie_ctrl);
    if (ret)
    {
        printk("pcie setup failed %d\r\n", ret);
        _deinit(&pcie_ctrl);
        return ret;
    }

#if (1) // using SelfLoopBack module card only
    sstar_pcieif_internalclk_en(&pcie_ctrl);
    ret = _self_macphy_port0_loopback(&pcie_ctrl);
    if (ret)
        printk("PCIE0_Link_FAIL\r\n");
    else
        printk("PCIE0_Link_UP\r\n");

    // DMA verification
    ret = _dma_verify_port0(&pcie_ctrl);
    if (ret)
        printk("PCIE0_DMA_FAIL\r\n");
    else
        printk("PCIE0_DMA_PASS\r\n");
#endif

#if (1) // using SelfLoopBack module card only
    ret = _self_macphy_port1_loopback(&pcie_ctrl);
    if (ret)
        printk("PCIE1_Link_FAIL\r\n");
    else
        printk("PCIE1_Link_UP\r\n");

    // DMA verification
    ret = _dma_verify_port1(&pcie_ctrl);
    if (ret)
        printk("PCIE1_DMA_FAIL\r\n");
    else
        printk("PCIE1_DMA_PASS\r\n");
#endif

    _deinit(&pcie_ctrl);
    return ret;
}

//------------------------------------------------------------------------------
//  Function    : loopback_remove
//  Description :
//------------------------------------------------------------------------------
static int loopback_remove(void)
{
    return 0;
}

static int __init pcie_bench_init(void)
{
    return loopback_probe();
}

static void __exit pcie_bench_exit(void)
{
    loopback_remove();
}

module_init(pcie_bench_init);
module_exit(pcie_bench_exit);
