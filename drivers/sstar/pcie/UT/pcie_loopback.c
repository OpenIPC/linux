/*
 * pcie_loopback.c - Sigmastar
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SIGMASTAR");
MODULE_DESCRIPTION("PCIe LOOPBACK UT");

#define PCIE_UT_VERSION "v0.1.5"

#define PCIE_LPBK_DBG
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

#define PCIE_DEVNAME "pcie_loopback"

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
    printk("dev major: %d, minor: %d\n", MAJOR(ctrl->devno), MINOR(ctrl->devno));
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

    // alloc descritor list
    ctrl->desclist_sz = DMA_LL_MAX_NUM * sizeof(struct ll_element);
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
#ifdef CONFIG_ARM64
    printk("desc list pa: %llx, va: %px\r\n", ctrl->desclist_dma, (void *)ctrl->desclist);
    printk("local mem pa: %llx, va: %px\r\n", ctrl->lmem_basedma, ctrl->lmem_base);
    printk("desti mem pa: %llx, va: %px\r\n", ctrl->rmem_basedma, ctrl->rmem_base);
#else
    printk("desc list pa: %x, va: %px\r\n", ctrl->desclist_dma, (void *)ctrl->desclist);
    printk("local mem pa: %x, va: %px\r\n", ctrl->lmem_basedma, ctrl->lmem_base);
    printk("desti mem pa: %x, va: %px\r\n", ctrl->rmem_basedma, ctrl->rmem_base);
#endif

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

/* pre configure pcie0 as rc, pcie1 as ep */
static void _pre_config_pcie(struct pcie_handle *ctrl)
{
    FUNC_ENTRY_DBG(ctrl->dev);

    // enable clocks
    OUTREG16(ctrl->clkgen_riu + 0xA0 * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x8A * 2, 0x0000);
    OUTREG16(ctrl->clkgen2_riu + 0x96 * 2, 0x0000);

    OUTREG16(ctrl->mac0_riu + 0x04 * 2, 0x0004); // pcie0 reg_pcie_device_type RC mode
    OUTREG16(ctrl->mac1_riu + 0x04 * 2, 0x0000); // pcie1 reg_pcie_device_type EP mode

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x50 * 2, 4, 0x10002); // FIXME: LINK_CONTROL2_LINK_STATUS2_REG???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCIE_LINK_WIDTH_SPEED_CONTROL, 4, // GEN2_CTRL_OFF
                        PORT_LOGIC_AUTO_LANE_FLIP | PORT_LOGIC_LINK_WIDTH_2_LANES | PORT_LOGIC_FAST_TRAIN_SEQ(0x2C));
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCIE_LINK_WIDTH_SPEED_CONTROL, 4,
                        PORT_LOGIC_SPEED_CHANGE | PORT_LOGIC_AUTO_LANE_FLIP | PORT_LOGIC_LINK_WIDTH_2_LANES
                            | PORT_LOGIC_FAST_TRAIN_SEQ(0x2C));

    FUNC_EXIT_DBG(ctrl->dev);
}

/* post configure pcie */
static void _post_config_pcie(struct pcie_handle *ctrl)
{
    FUNC_ENTRY_DBG(ctrl->dev);

    // DEVICE_CONTROL_DEVICE_STATUS:PCIE_CAP_MAX_PAYLOAD_SIZE_CS: MAX_256B_SIZE
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x78, 4, 0x102830);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x78, 4, 0x102830);

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_1, 4, 0x02dc);       // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_0, 4, 0x29300004);   // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_2, 4, 0x66300000);   // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_ROM_ADDRESS, 4, 0x87810001);      // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_4, 4, 0xd401);       // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_IO_BASE, 4, 0x0101);              // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_IO_BASE_UPPER16, 4, 0x0000);      // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_IO_BASE, 4, 0xd0d0);              // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_PREF_MEMORY_BASE, 4, 0x29312931); // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_PREF_BASE_UPPER32, 4, 0x02dc);    // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_PREF_LIMIT_UPPER32, 4, 0x02dc);   // FIXME: ???
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_MEMORY_BASE, 4, 0x87806630);      // FIXME: ???

    /*__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, PCI_COMMAND,           4, PCI_COMMAND_IO|
                                                                            PCI_COMMAND_MEMORY|
                                                                            PCI_COMMAND_MASTER);*/
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_COMMAND, 4,
                        PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

    // set pcie0 slv_read_bypass as 1, that is, waiting for PCIe1 CLP_D
    OUTREG16(ctrl->phy0_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass
    // set pcie1 slv_read_bypass as 1, that is, waiting for PCIe0 CLP_D
    OUTREG16(ctrl->phy1_riu + 0x4E * 2, 0x0100); // pcie0 slv_read_bypass

    // reg_slv_wstrb_mask = h'f to allow 32-bit write
    OUTREG16(ctrl->mac0_riu + 0x56 * 2, 0xFFFF);
    OUTREG16(ctrl->mac1_riu + 0x56 * 2, 0xFFFF);

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x81c, 4,
                        0x007F); // TARGET_MAP_PF need to apply for each bar if match,default is 0x7e
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x81c, 4,
                        0x007F); // TARGET_MAP_PF need to apply for each bar if match,default is 0x7e

    FUNC_EXIT_DBG(ctrl->dev);
}

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

    FUNC_EXIT_DBG(ctrl->dev);
}

void __cr_data_in(void *base, u16 val)
{
    OUTREG16(base + 0x48 * 2, 0x0000); // reg_cr_write = 0
    OUTREG16(base + 0x48 * 2, 0x0000); // reg_cr_write = 0
    OUTREG16(base + 0x48 * 2, 0x0000); // reg_cr_write = 0
    OUTREG16(base + 0x44 * 2, val);    // reg_cr_data_in
    OUTREG16(base + 0x48 * 2, 0x0001); // reg_cr_write = 1
    OUTREG16(base + 0x48 * 2, 0x0001); // reg_cr_write = 1
    OUTREG16(base + 0x48 * 2, 0x0001); // reg_cr_write = 1
}

/* start linking up pcie0 & pcie1 MAC to MAC */
static void _link_up(struct pcie_handle *ctrl)
{
    FUNC_ENTRY_DBG(ctrl->dev);

    OUTREG16(ctrl->mac0_riu + 0xC0 * 2, 0x0010); // pcie0 reg_app_ltssm_enable
    OUTREG16(ctrl->mac1_riu + 0xC0 * 2, 0x0010); // pcie1 reg_app_ltssm_enable

    __cr_data_in(ctrl->phy0_riu, 0x0002);
    // Waiting for pcie0 smlh_ltssm_state changed to DETECT_ACT (0x01) state
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy0_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x0002);
    // Waiting for pcie1 smlh_ltssm_state changed to DETECT_ACT (0x01) state
    while ((INREG16(ctrl->phy1_riu + 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy1_riu, 0x0003);
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy0_riu, 0x006F);
    __cr_data_in(ctrl->phy0_riu, 0x406F);
    __cr_data_in(ctrl->phy0_riu, 0x006F);
    // Waiting for pcie0 smlh_ltssm_state changed to POLL_ACTIVE (0x02) state
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x02)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy1_riu, 0x006F);
    __cr_data_in(ctrl->phy1_riu, 0x406F);
    __cr_data_in(ctrl->phy1_riu, 0x006F);
    // Waiting for pcie1 smlh_ltssm_state changed to POLL_ACTIVE (0x02) state
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x02)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy0_riu, 0x0003);
    __cr_data_in(ctrl->phy0_riu, 0x4003);
    __cr_data_in(ctrl->phy0_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x4003);
    __cr_data_in(ctrl->phy1_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x0303);
    __cr_data_in(ctrl->phy0_riu, 0x0303);
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x0E)
        ; // reg_cr_data_out
    while ((INREG16(ctrl->phy1_riu + 0x4A * 2) & 0x3F) != 0x0E)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy0_riu, 0x0393);
    __cr_data_in(ctrl->phy0_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x0393);
    __cr_data_in(ctrl->phy1_riu, 0x0003);
    __cr_data_in(ctrl->phy0_riu, 0x4003);
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x0D)
        ; // reg_cr_data_out
    __cr_data_in(ctrl->phy0_riu, 0x0003);
    __cr_data_in(ctrl->phy1_riu, 0x4003);
    while ((INREG16(ctrl->phy1_riu + 0x4A * 2) & 0x3F) != 0x0D)
        ; // reg_cr_data_out

    __cr_data_in(ctrl->phy1_riu, 0x0303);
    __cr_data_in(ctrl->phy0_riu, 0x0303);
    while ((INREG16(ctrl->phy0_riu + 0x4A * 2) & 0x3F) != 0x11)
        ; // reg_cr_data_out
    while ((INREG16(ctrl->phy1_riu + 0x4A * 2) & 0x3F) != 0x11)
        ; // reg_cr_data_out

    FUNC_EXIT_DBG(ctrl->dev);
}

/* CFG verification */
#if (1)
static void _cfg_verify(struct pcie_handle *ctrl)
{
    u32 data[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

    FUNC_ENTRY_DBG(ctrl->dev);

    // Set CfgRd TLP address [31:24] as 0x01 => bus:1
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, 0x0001); // pcie0 reg_slv_addr_bank_39to24

    // pcie0 reg_slv_armisc_info[4:0] TLP's type = 5'b00010, Type 0 configuration
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_CFG0);
    __pcie_read(ctrl->cfg0_base + 0x0, 4, &data[0]);
    __pcie_read(ctrl->cfg0_base + 0x4, 4, &data[1]);
    __pcie_read(ctrl->cfg0_base + 0x8, 4, &data[2]);
    __pcie_read(ctrl->cfg0_base + 0xC, 4, &data[3]);
    printk("read back by CfgRd0: x%X x%X x%X x%X\r\n", data[0], data[1], data[2], data[3]);

    //
    // perform Cfg write operation
    //
    // pcie0 reg_slv_awmisc_info[4:0] TLP's type = 5'b00010, Type 0 configuration
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_CFG0);
    // Issue CfgWr0 to addr 0 of EP configuration space
    printk("CfgWr0: set EP status and command register\r\n");
    __pcie_write(ctrl->cfg0_base + PCI_COMMAND, 4, 0x00000007);
    // Check result by PCIe1 DBI
    data[0] = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi1_base, PCI_COMMAND, 4);
    printk("read back by pcie1 dbi: x%X\r\n", data[0]);
    WARN((data[0] & 0x7) != 0x7, "CfgWr0 FAILED!\r\n");
    //__pcie_write(ctrl->cfg0_base + PCI_COMMAND, 4, 0x00000000); // clear by wire
    //
    // perform Cfg read operation
    //
    // pcie0 reg_slv_armisc_info[4:0] TLP's type = 5'b00010, Type 0 configuration
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_CFG0);
    // Check result by CfgRd0 to addr 4
    __pcie_read(ctrl->cfg0_base + PCI_COMMAND, 4, &data[0]);
    printk("read back by CfgRd0: x%X\r\n", data[0]);
    // WARN(data[0] & 0x7, "CfgRd0 FAILED!\r\n");

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
/* MEM verification */
static void _mem_verify_byEP(struct pcie_handle *ctrl)
{
    u32           i, src_crc32, dst_crc32;
    ss_miu_addr_t dst_miu;
    u32           test_sz = MEM_VERIFY_SIZE; // 128KB
    printk("_mem_verify_byEP 2021/06/25 2218 skip write for reg dump \r\n");
    FUNC_ENTRY_DBG(ctrl->dev);

    _mem_setup(ctrl);
    dst_miu = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    dst_miu = 0x110000;

    printk("Gen random data & calc crc on local memory\r\n");
    get_random_bytes(ctrl->lmem, test_sz);
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);

    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (dst_miu >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (dst_miu >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (dst_miu >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x81c, 4,
                        0x007F); // TARGET_MAP_PF need to apply for each bar if match,default is 0x3e

#if (1) // skip write for reg dump
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    printk("MemWr...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_write(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + i, 4, *(u32 *)(ctrl->lmem + i));
    }
    printk("Calc crc on remote memory\r\n");
    dst_crc32 = crc32_le(~0, ctrl->rmem, test_sz);
    printk(" = dst crc x%X\r\n", dst_crc32);

    WARN(src_crc32 != dst_crc32, "MemWr FAILED!\r\n");

    printk("Clear local memory\r\n");
    memset(ctrl->lmem, 0, test_sz);
    src_crc32 = 0;

    // pcie0 (RC) issues MemRd request to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_MEM);
    printk("MemRd...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_read(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + i, 4, (u32 *)(ctrl->lmem + i));
    }

    printk("Calc crc on local memory\r\n");
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);
    WARN(src_crc32 != dst_crc32, "MemRd FAILED!\r\n");
#endif
    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (1)
/* MEM verification */
static void _mem_verify(struct pcie_handle *ctrl)
{
    u64           base, target;
    u32           i, src_crc32, dst_crc32;
    ss_miu_addr_t dst_miu;
    u32           test_sz = MEM_VERIFY_SIZE; // 128KB

    FUNC_ENTRY_DBG(ctrl->dev);

    _mem_setup(ctrl);
    dst_miu = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);

    printk("Gen random data & calc crc on local memory\r\n");
    get_random_bytes(ctrl->lmem, test_sz);
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);

    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (dst_miu >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (dst_miu >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (dst_miu >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56

    // IATU setting
    base   = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    target = base + test_sz;
    printk("base:0x%lx , target:0x%lx  \r\n", (unsigned long)base, (unsigned long)target);
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, base, target, test_sz);

    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x81c, 4, 0x007F );//TARGET_MAP_PF need to apply for each bar if
    // match,default is 0x3e

    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    printk("MemWr...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_write(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + i, 4, *(u32 *)(ctrl->lmem + i));
    }
    printk("Calc crc on remote memory + test_sz\r\n");
    dst_crc32 = crc32_le(~0, (ctrl->rmem) + test_sz, test_sz);
    printk(" = dst crc x%X\r\n", dst_crc32);
    WARN(src_crc32 != dst_crc32, "MemWr FAILED!\r\n");

    printk("Clear local memory\r\n");
    memset(ctrl->lmem, 0, test_sz);
    src_crc32 = 0;

    // pcie0 (RC) issues MemRd request to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_MEM);
    printk("MemRd...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_read(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + test_sz + i, 4, (u32 *)(ctrl->lmem + i));
    }

    printk("Calc crc on local memory\r\n");
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);
    WARN(src_crc32 != dst_crc32, "MemRd FAILED!\r\n");

    memset(ctrl->lmem, 0, test_sz);
    memset(ctrl->rmem, 0, test_sz + test_sz);

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
/* MEM_EP verification */
static void _mem_ep_verify(struct pcie_handle *ctrl)
{
    u32           i, src_crc32, dst_crc32;
    ss_miu_addr_t dst_miu;
    u32           test_sz = MEM_VERIFY_SIZE; // 128KB
    printk("_mem_ep_verify 2021/06/18 1456 \r\n");
    FUNC_ENTRY_DBG(ctrl->dev);

    _mem_setup(ctrl);
    dst_miu = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);

    printk("Gen random data & calc crc on local memory\r\n");
    get_random_bytes(ctrl->lmem, test_sz);
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);

    // pcie1 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac1_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac1_riu + 0x58 * 2, (dst_miu >> 24) & 0xFFFF); // pcie1 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac1_riu + 0x5A * 2, (dst_miu >> 40) & 0xFFFF); // pcie1 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac1_riu + 0x5C * 2, (dst_miu >> 56) & 0xFF); // pcie1 reg_slv_addr_bank_63to56

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x81c, 4,
                        0x007F); // TARGET_MAP_PF need to apply for each bar if match,default is 0x3e

    // pcie1 (EP) issues MemWr requests to pcie0 (RC)
    printk("MemWr...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_write(ctrl->cfg1_base + (dst_miu & 0xFFFFFF) + i, 4, *(u32 *)(ctrl->lmem + i));
    }
    printk("Calc crc on remote memory\r\n");
    dst_crc32 = crc32_le(~0, ctrl->rmem, test_sz);
    printk(" = dst crc x%X\r\n", dst_crc32);
    WARN(src_crc32 != dst_crc32, "MemWr FAILED!\r\n");

    printk("Clear local memory\r\n");
    memset(ctrl->lmem, 0, test_sz);
    src_crc32 = 0;

    // pcie1 (EP) issues MemRd request to pcie0 (RC)
    // pcie1 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac1_riu + 0x50 * 2, PCIE_TLP_TYPE_MEM);
    printk("MemRd...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_read(ctrl->cfg1_base + (dst_miu & 0xFFFFFF) + i, 4, (u32 *)(ctrl->lmem + i));
    }

    printk("Calc crc on local memory\r\n");
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);
    WARN(src_crc32 != dst_crc32, "MemRd FAILED!\r\n");

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
/* iATU verification */
static void _iatu_verify(struct pcie_handle *ctrl)
{
    u64 base, target;

    FUNC_ENTRY_DBG(ctrl->dev);

    _mem_setup(ctrl);

    // test 1: base is an invalid addr, use OB iATU index0 translate to valid addr
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    base   = target + DMA_XFER_SIZE;
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, base, target, DMA_XFER_SIZE);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (base >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (base >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (base >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56
    printk("1. MemWr CAFE (%llx -> %llx)\r\n", base, target);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x0, 4, 0xCCCCCCCC);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x4, 4, 0xAAAAAAAA);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x8, 4, 0xFFFFFFFF);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0xC, 4, 0xEEEEEEEE);
    if ((*(u32 *)(ctrl->rmem + 0x0) != 0xCCCCCCCC) || (*(u32 *)(ctrl->rmem + 0x4) != 0xAAAAAAAA)
        || (*(u32 *)(ctrl->rmem + 0x8) != 0xFFFFFFFF) || (*(u32 *)(ctrl->rmem + 0xC) != 0xEEEEEEEE))
    {
        printk("Remote content %X %X %X %X\r\n", *(u32 *)(ctrl->rmem + 0x0), *(u32 *)(ctrl->rmem + 0x4),
               *(u32 *)(ctrl->rmem + 0x8), *(u32 *)(ctrl->rmem + 0xC));
        WARN(true, "OB iATU_0 MemWr FAILED!\r\n");
    }
    // test 2: base is local addr, use OB iATU index1 translate to remote addr
    base   = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma);
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 1, PCIE_TLP_TYPE_MEM, base, target, DMA_XFER_SIZE);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (base >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (base >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (base >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56
    printk("2. MemWr BEEF (%llx -> %llx)\r\n", base, target);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x0, 4, 0xBBBBBBBB);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x4, 4, 0xEEEEEEEE);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0x8, 4, 0xEEEEEEEE);
    __pcie_write(ctrl->cfg0_base + (base & 0xFFFFFF) + 0xC, 4, 0xFFFFFFFF);
    printk("Local content %X %X %X %X\r\n", *(u32 *)(ctrl->lmem + 0x0), *(u32 *)(ctrl->lmem + 0x4),
           *(u32 *)(ctrl->lmem + 0x8), *(u32 *)(ctrl->lmem + 0xC));
    if ((*(u32 *)(ctrl->rmem + 0x0) != 0xBBBBBBBB) || (*(u32 *)(ctrl->rmem + 0x4) != 0xEEEEEEEE)
        || (*(u32 *)(ctrl->rmem + 0x8) != 0xEEEEEEEE) || (*(u32 *)(ctrl->rmem + 0xC) != 0xFFFFFFFF))
    {
        printk("Remote content %X %X %X %X\r\n", *(u32 *)(ctrl->rmem + 0x0), *(u32 *)(ctrl->rmem + 0x4),
               *(u32 *)(ctrl->rmem + 0x8), *(u32 *)(ctrl->rmem + 0xC));
        WARN(true, "OB iATU_1 MemWr FAILED!\r\n");
    }

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
/* IO verification */  only operate at x32 addsres_3DW only not applicable for x64_4DW
static void _io_verify(struct pcie_handle *ctrl)
{
    u64           base, target;
    u32           i, src_crc32, dst_crc32;
    ss_miu_addr_t dst_miu;
    u32           test_sz = MEM_VERIFY_SIZE; // 128KB

    FUNC_ENTRY_DBG(ctrl->dev);

    //_mem_setup(ctrl);
    dst_miu = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);

    printk("Gen random data & calc crc on local memory\r\n");
    get_random_bytes(ctrl->lmem, test_sz);
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);

    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_IO);
    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (dst_miu >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (dst_miu >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (dst_miu >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56

    // IATU setting
    base   = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    target = base + test_sz;
    printk("base:0x%lx , target:0x%lx  \r\n", (unsigned long)base, (unsigned long)target);
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_IO, base, target, test_sz);

    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x81c, 4, 0x007F );//TARGET_MAP_PF need to apply for each bar if
    // match,default is 0x3e

    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    printk("IOWr...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_write(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + i, 4, *(u32 *)(ctrl->lmem + i));
    }
    printk("Calc crc on remote memory + test_sz\r\n");
    dst_crc32 = crc32_le(~0, (ctrl->rmem) + test_sz, test_sz);
    printk(" = dst crc x%X\r\n", dst_crc32);
    WARN(src_crc32 != dst_crc32, "IOWr FAILED!\r\n");

    printk("IOWr   return here   \r\n");
    return;

    printk("Clear local memory\r\n");
    memset(ctrl->lmem, 0, test_sz);
    src_crc32 = 0;

    // pcie0 (RC) issues MemRd request to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_IO);
    printk("IORd...\r\n");
    for (i = 0; i < test_sz; i += 4)
    {
        __pcie_read(ctrl->cfg0_base + (dst_miu & 0xFFFFFF) + test_sz + i, 4, (u32 *)(ctrl->lmem + i));
    }

    printk("Calc crc on local memory\r\n");
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);
    WARN(src_crc32 != dst_crc32, "IORd FAILED!\r\n");

    memset(ctrl->lmem, 0, test_sz);
    memset(ctrl->rmem, 0, test_sz + test_sz);

    FUNC_EXIT_DBG(ctrl->dev);
}

#endif

#if (1)
/* _dma_iatu_verify verification */
static void _dma_iatu_bar_verify(struct pcie_handle *ctrl)
{
    u64        base, target;
    dma_addr_t desc_orgin, desc_next;
    // u32 * pPatern;
    struct ll_element *desclist_next;
    u32                src_crc32, dst_crc32;
    u32                test_sz = DMA_VERIFY_SIZE;

    FUNC_ENTRY_DBG(ctrl->dev);

    //_mem_setup(ctrl); already execute at mem_compare

    // test 1: base is an invalid addr, use OB iATU index0 translate to valid addr
    // base   = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma);
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    base   = target + DMA_XFER_SIZE;
    printk("base:0x%lx , target:0x%lx  \r\n", (unsigned long)base, (unsigned long)target);

    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, base, target, test_sz);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_MEM);
    // pcie0 reg_slv_awmisc_info TLP's type = 5'b00000, MemRd/MemWr
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_MEM);

    // Set TLP address [39:24]
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, (target >> 24) & 0xFFFF); // pcie0 reg_slv_addr_bank_39to24
    // Set TLP address [55:40]
    OUTREG16(ctrl->mac0_riu + 0x5A * 2, (target >> 40) & 0xFFFF); // pcie0 reg_slv_addr_bank_55to40
    // Set TLP address [63:56]
    OUTREG16(ctrl->mac0_riu + 0x5C * 2, (target >> 56) & 0xFF); // pcie0 reg_slv_addr_bank_63to56

    // BAR setting
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_1, 4, target >> 32);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_0, 4, (target & 0xFFFFFFFF) | 0x04);

    printk("Gen random data & calc crc on local memory\r\n");
    get_random_bytes(ctrl->lmem, test_sz);
    src_crc32 = crc32_le(~0, ctrl->lmem, test_sz);
    printk(" = src crc x%X\r\n", src_crc32);

    desc_orgin    = ctrl->desclist_dma;
    desc_next     = (ctrl->desclist_dma) + 0x200;
    desclist_next = (struct ll_element *)(((void *)(ctrl->desclist)) + 0x200);
    printk("desclist_next=0x%px \r\n", (void *)desclist_next);

    // data element 0-0
    printk("ctrl->desclist[W0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 1;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = test_sz;

    ctrl->desclist->sar_l = (Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma)) & 0xFFFFFFFF;
    ctrl->desclist->sar_h = (Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma)) >> 32;
    ctrl->desclist->dar_l = base & 0xFFFFFFFF;
    ctrl->desclist->dar_h = base >> 32;

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[W1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((ss_phys_addr_t)(desc_orgin & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = (Chip_Phys_to_MIU((ss_phys_addr_t)desc_orgin)) >> 32;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_next;
    // data element 0-0
    printk("ctrl->desclist[R0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = test_sz;
    ctrl->desclist->sar_l     = (target & 0xFFFFFFFF);
    ctrl->desclist->sar_h     = target >> 32;
    ctrl->desclist->dar_l     = ((Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma)) & 0xFFFFFFFF) + 0x0600;
    ctrl->desclist->dar_h     = (Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->lmem_dma)) >> 32;

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[R1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = Chip_Phys_to_MIU((ss_phys_addr_t)(desc_next & 0xFFFFFFFF));
    ctrl->desclist->sar_h     = (Chip_Phys_to_MIU((ss_phys_addr_t)desc_next)) >> 32;
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    // restore desclist address,so the release process could be done
    (ctrl->desclist)--;
    ctrl->desclist = (struct ll_element *)(((void *)(ctrl->desclist)) - 0x200);
    Chip_Flush_MIU_Pipe();

    // Set PCIe0 dbi_addr[32:12] as h180380
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
        Chip_Phys_to_MIU(
            (ss_phys_addr_t)(desc_orgin & 0xFFFFFFFF))); // wriu -w 0x1a820e 0xffffff70 //This register holds the lower
                                                         // 32 bits of the DMA write linked list pointer.
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0210 * 2, 4,
                        (Chip_Phys_to_MIU((ss_phys_addr_t)desc_orgin))
                            >> 32); // wriu -w 0x1a8210 0x0000 //This register holds the higher 32 bits of the DMA write
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
                        Chip_Phys_to_MIU((ss_phys_addr_t)(desc_next & 0xFFFFFFFF))); // wriu -w 0x1a818e 0xffffffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0190 * 2, 4,
                        (Chip_Phys_to_MIU((ss_phys_addr_t)desc_next)) >> 32); // wriu -w 0x1a8190 0x0000

    // PCIe1 MAC RIU registers needs to set 0x7D, 0x7E, 0x7F for MIU write address comparison interrupt.
    // PCIe0 slv_rdata_irq asserted when PCIe0 CfgRd PCIe1 successfully
    // ctrl->desclist->dar_l =  (((ctrl->remotemem_dma) & 0xFFFFFFFF) & (~0x20000000) ) ;
    OUTREG16(ctrl->mac1_riu + 0xFA * 2, (target & 0xFFFF) + 0x0100);  // reg_pcie_mstrwr_addr15to0_match
    OUTREG16(ctrl->mac1_riu + 0xFC * 2, (target & 0xFFFF0000) >> 16); // reg_pcie_mstrwr_addr31to16_match
    OUTREG16(ctrl->mac1_riu + 0xFE * 2,
             (Chip_Phys_to_MIU((ss_phys_addr_t)target)) >> 32); // reg_pcie_mstrwr_addr35to32_match

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x08 * 2, 4, 0x0001); // wriu -w 0x1a8008 0x0001
    // printk(" ((ctrl->remotemem_dma) & 0xFFFF) + 0x0100 = 0x%x  \r\n", (unsigned int)(base & 0xFFFF) + 0x0100  );
    // printk(" (((ctrl->remotemem_dma) & 0xFFFF0000) & (~0x20000000))>>16 = 0x%x \r\n", (unsigned int)(((base &
    // 0xFFFF0000) & (~0x20000000))>>16)    ); printk(" (((ctrl->remotemem_dma) & 0xFFFF0000) & (~0x20000000))>>32 =
    // 0x%x \r\n", (unsigned int)(((base & 0xFFFF0000) & (~0x20000000))>>32)    );
    printk(" waste time 1 \r\n");
    printk(" waste time 2 \r\n");
    printk(" waste time 3 \r\n");
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    // Waiting edma[1] interrupt !!!
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x2c * 2,    4,     0x0002);//wriu -w 0x1a802c 0x0002

    printk("Calc crc on soruce memory + 0x0600 \r\n");
    dst_crc32 = crc32_le(~0, (ctrl->lmem) + 0x0600, test_sz);
    printk(" = dst crc x%X\r\n", dst_crc32);
    WARN(src_crc32 != dst_crc32, "_dma_iatu_bar_verify FAILED!\r\n");

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
static void _dma_verify(struct pcie_handle *ctrl)
{
    u64                base, target;
    dma_addr_t         desc_orgin, desc_next;
    u32 *              pPatern;
    struct ll_element *desclist_next;

    FUNC_ENTRY_DBG(ctrl->dev);

    //_mem_setup(ctrl);

    // test 1: base is an invalid addr, use OB iATU index0 translate to valid addr
    target = Chip_Phys_to_MIU((ss_phys_addr_t)ctrl->rmem_dma);
    base   = target + DMA_XFER_SIZE;
    // program outbound atu
    __dw_pcie_prog_ob_atu_unroll(ctrl, 0, PCIE_TLP_TYPE_MEM, target, target, DMA_XFER_SIZE);
    // pcie0 (RC) issues MemWr requests to pcie1 (EP)
    // pcie0 reg_slv_armisc_info TLP's type = 5'b00000, MemRd/MemWr
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

    desc_orgin    = ctrl->desclist_dma;
    desc_next     = (ctrl->desclist_dma) + 0x200;
    desclist_next = (struct ll_element *)(((void *)(ctrl->desclist)) + 0x200);
    printk("desclist_next=0x%px \r\n", (void *)desclist_next);

    // data element 0-0
    printk("ctrl->desclist[W0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 1;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = (((ctrl->lmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = ((ctrl->lmem_dma) >> 32);
    ctrl->desclist->dar_l     = (((ctrl->rmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->dar_h     = ((ctrl->rmem_dma) >> 32);

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[W1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = ((desc_orgin & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = (desc_orgin >> 32);
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_next;
    // data element 0-0
    printk("ctrl->desclist[R0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = (((ctrl->rmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = ((ctrl->rmem_dma) >> 32);
    ctrl->desclist->dar_l     = ((((ctrl->lmem_dma) & 0xFFFFFFFF) & (~0x20000000)) + 0x0600);
    ctrl->desclist->dar_h     = ((ctrl->lmem_dma) >> 32);

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[R1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = ((desc_next & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = (desc_next >> 32);
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

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
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x020e * 2, 4,
                        desc_orgin & (~0x20000000)); // wriu -w 0x1a820e 0xffffff70 //This register holds the lower 32
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
                        desc_next & (~0x20000000));                         // wriu -w 0x1a818e 0xffffffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x0190 * 2, 4, 0x0000); // wriu -w 0x1a8190 0x0000

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x08 * 2, 4, 0x0001); // wriu -w 0x1a8008 0x0001
    printk(" wast time here \r\n");
    printk(" wast time here \r\n");
    printk(" wast time here \r\n");
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    // Waiting edma[1] interrupt !!!
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x2c * 2,    4,     0x0002);//wriu -w 0x1a802c 0x0002

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

#if (0)
/* _dma_ep_fire verification */
static void _dma_ep_fire(struct pcie_handle *ctrl)
{
    dma_addr_t         desc_orgin, desc_next;
    u32 *              pPatern;
    struct ll_element *desclist_next;

    FUNC_ENTRY_DBG(ctrl->dev);
    printk("Verify DMA_EP test item \r\n");
    _mem_setup(ctrl);

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

    desc_orgin    = ctrl->desclist_dma;
    desc_next     = (ctrl->desclist_dma) + 0x200;
    desclist_next = (struct ll_element *)(((void *)(ctrl->desclist)) + 0x200);
    printk("desclist_next=0x%px \r\n", (void *)desclist_next);

    // data element 0-0
    printk("ctrl->desclist[W0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = (((ctrl->lmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = ((ctrl->lmem_dma) >> 32);
    ctrl->desclist->dar_l     = (((ctrl->rmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->dar_h     = ((ctrl->rmem_dma) >> 32);

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[W1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = ((desc_orgin & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = (desc_orgin >> 32);
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;

    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    ctrl->desclist = desclist_next;
    // data element 0-0
    printk("ctrl->desclist[R0]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 1;
    ctrl->desclist->tcb       = 0;
    ctrl->desclist->llp       = 0;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0x400;
    ctrl->desclist->sar_l     = (((ctrl->rmem_dma) & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = ((ctrl->rmem_dma) >> 32);
    ctrl->desclist->dar_l     = ((((ctrl->lmem_dma) & 0xFFFFFFFF) & (~0x20000000)) + 0x0600);
    ctrl->desclist->dar_h     = ((ctrl->lmem_dma) >> 32);

    // link element 0
    (ctrl->desclist)++;
    printk("ctrl->desclist[R1]=0x%px \r\n", (void *)ctrl->desclist);
    ctrl->desclist->cb        = 0;
    ctrl->desclist->tcb       = 1;
    ctrl->desclist->llp       = 1;
    ctrl->desclist->lie       = 0;
    ctrl->desclist->rie       = 0;
    ctrl->desclist->resv_5    = 0;
    ctrl->desclist->xfer_size = 0;
    ctrl->desclist->sar_l     = ((desc_next & 0xFFFFFFFF) & (~0x20000000));
    ctrl->desclist->sar_h     = (desc_next >> 32);
    ctrl->desclist->dar_l     = 0;
    ctrl->desclist->dar_h     = 0;
    printk("ctrl->desclist->sar_l=0x%x \r\n", ctrl->desclist->sar_l);
    printk("ctrl->desclist->sar_h=0x%x \r\n", ctrl->desclist->sar_h);

    Chip_Flush_MIU_Pipe();

    // Switch to PCIE0 DMA Register Bank
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
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x020e * 2, 4,
                        desc_orgin & (~0x20000000)); // wriu -w 0x1a820e 0xffffff70 //This register holds the lower 32
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
                        desc_next & (~0x20000000));                         // wriu -w 0x1a818e 0xffffffb8
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x0190 * 2, 4, 0x0000); // wriu -w 0x1a8190 0x0000

    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x08 * 2, 4, 0x0001); // wriu -w 0x1a8008 0x0001
    printk(" wast time here \r\n");
    printk(" wast time here \r\n");
    printk(" wast time here \r\n");
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x18 * 2, 4, 0x0000); // wriu -w 0x1a8018 0x0000
    // Waiting edma[1] interrupt !!!
    //__dw_pcie_write_dbi(ctrl->dev, ctrl->dbi1_base, 0x2c * 2,    4,     0x0002);//wriu -w 0x1a802c 0x0002

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

/* EP_Driver verification */
#if (0)
static void _ep_driver_verify(struct pcie_handle *ctrl)
{
    u32 u32Ddata = 0;

    FUNC_ENTRY_DBG(ctrl->dev);

    // RC CfgR the VendorID,DeviceID from EP
    //  Set CfgRd TLP address [31:24] as 0x01 => bus:1
    OUTREG16(ctrl->mac0_riu + 0x58 * 2, 0x0001); // pcie0 reg_slv_addr_bank_39to24

    // pcie0 reg_slv_armisc_info[4:0] TLP's type = 5'b00100, Type 0 configuration
    OUTREG16(ctrl->mac0_riu + 0x50 * 2, PCIE_TLP_TYPE_CFG0);
    __pcie_read(ctrl->cfg0_base + 0x0, 4, &u32Ddata);
    printk("RC CfgR the VendorID,DeviceID from EP: 0x%X \r\n", u32Ddata);

    // pcie0 reg_slv_awmisc_info[4:0] TLP's type = 5'b00100, Type 0 configuration
    OUTREG16(ctrl->mac0_riu + 0x40 * 2, PCIE_TLP_TYPE_CFG0);
    // Issue CfgWr0 to addr 0 of EP configuration space
    printk("CfgWr0: set EP BAR0 register = 0x100000 \r\n");
    __pcie_write(ctrl->cfg0_base + PCI_BASE_ADDRESS_0, 4, 0x000000);
    __pcie_write(ctrl->cfg0_base + PCI_BASE_ADDRESS_1, 4, 0x100000);
    // Check result by PCIe1 DBI
    u32Ddata = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_0, 4);
    printk("read back PCI_BASE_ADDRESS_0 by pcie1 dbi: 0x%X\r\n", u32Ddata);
    u32Ddata = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi1_base, PCI_BASE_ADDRESS_1, 4);
    printk("read back by PCI_BASE_ADDRESS_1 pcie1 dbi: 0x%X\r\n", u32Ddata);

    // memory verify
    _mem_verify_byEP(ctrl);

    FUNC_EXIT_DBG(ctrl->dev);
}
#endif

static void _hotplug_verify(struct pcie_handle *ctrl)
{
    u32 u32RegData = 0;
    FUNC_ENTRY_DBG(ctrl->dev);

    OUTREG16(ctrl->mac0_riu + 0x28 * 2, 0xBFFF); // reg_pcie_mask_irq2
    OUTREG16(ctrl->mac0_riu + 0x30 * 2, 0xE000); // reg_pcie_int2_edge

    u32RegData = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x08bc, 4);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x08bc, 4, u32RegData | BIT(0));
    printk("DBI_RO_WR_Enable:write[0x%x]  \r\n", (unsigned int)(u32RegData | BIT(0)));

    u32RegData = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x84, 4);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x84, 4, u32RegData | BIT(0));
    printk("PCIE_CAP_Attention_Indicator_Button:write[0x%x]  \r\n", (unsigned int)(u32RegData | BIT(0)));

    u32RegData = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x88, 4);
    __dw_pcie_write_dbi(ctrl->dev, ctrl->dbi0_base, 0x88, 4, u32RegData | BIT(0) | BIT(5));
    printk("PCIE_CAP_Attention_Button_Pressed_Enable:write[0x%x]  \r\n", (unsigned int)(u32RegData | BIT(0) | BIT(5)));

    OUTREG16(ctrl->mac0_riu + 0xA4 * 2, 1); // reg_atten_button_pressed

    u32RegData = __dw_pcie_read_dbi(ctrl->dev, ctrl->dbi0_base, 0x88, 4);
    printk("SLOT CTL SLOT STATUS=[0x%x]  \r\n", u32RegData);

    if ((INREG16(ctrl->mac0_riu + 0x32 * 2) & 0x4000) == 0x4000)
        printk("Hotplug_Interrupt_Status:OK \r\n");
    else
        printk("Hotplug_Interrupt_Status:NG \r\n");

    FUNC_EXIT_DBG(ctrl->dev);
}

//------------------------------------------------------------------------------
//  Function    : LoopBack_Probe
//  Description :
//------------------------------------------------------------------------------
static int loopback_probe(void)
{
    int ret = 0;

    printk("loopback_probe %s\r\n", PCIE_UT_VERSION);
    printk("Besure to disable PCIE driver @ menuconfig first or the verification will be fail \r\n");

    ret = _init(&pcie_ctrl);
    if (ret)
    {
        printk("pcie setup failed %d\r\n", ret);
        _deinit(&pcie_ctrl);
        return ret;
    }
    _pre_config_pcie(&pcie_ctrl);
    _dump_cfg_space(&pcie_ctrl);
    _link_up(&pcie_ctrl);
    _post_config_pcie(&pcie_ctrl);
    // CfgRd, CfgWr verification
    _cfg_verify(&pcie_ctrl);

    // MemRd, MemWr verification
    _mem_verify(&pcie_ctrl);
    //_mem_ep_verify(&pcie_ctrl);

    // iATU verification
    //_iatu_verify(&pcie_ctrl);

    // IO verification
    //_io_verify(&pcie_ctrl);

    // DMA verification
    //_dma_verify(&pcie_ctrl);

    // DMA_IATU_BAR verification
    _dma_iatu_bar_verify(&pcie_ctrl);
    //_dma_iatu_bar_verify_0617(&pcie_ctrl);
    //_dma_ep_fire(&pcie_ctrl);

    // EP_Driver verification
    //_ep_driver_verify(&pcie_ctrl);

    _hotplug_verify(&pcie_ctrl);

    _deinit(&pcie_ctrl);
    return 0;
}

//------------------------------------------------------------------------------
//  Function    : loopback_remove
//  Description :
//------------------------------------------------------------------------------
static int loopback_remove(void)
{
    return 0;
}

static int __init pcie_loopback_init(void)
{
    return loopback_probe();
}

static void __exit pcie_loopback_exit(void)
{
    loopback_remove();
}

module_init(pcie_loopback_init);
module_exit(pcie_loopback_exit);
