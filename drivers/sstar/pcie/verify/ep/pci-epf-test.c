/*
 * pci-epf-test.c - Sigmastar
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

#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci_ids.h>
#include <linux/random.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci_regs.h>
#include "pcie-designware.h"
#include "pcie-sstar.h"
#include "drv_pcieif.h"
#include "ms_platform.h"
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
#include "drv_edma.h"
#endif
#define IRQ_TYPE_LEGACY 0
#define IRQ_TYPE_MSI    1
#define IRQ_TYPE_MSIX   2

#define COMMAND_RAISE_LEGACY_IRQ BIT(0)
#define COMMAND_RAISE_MSI_IRQ    BIT(1)
#define COMMAND_RAISE_MSIX_IRQ   BIT(2)
#define COMMAND_READ             BIT(3)
#define COMMAND_WRITE            BIT(4)
#define COMMAND_COPY             BIT(5)
#define COMMAND_DUMP_BAR         BIT(6)

#define STATUS_READ_SUCCESS     BIT(0)
#define STATUS_READ_FAIL        BIT(1)
#define STATUS_WRITE_SUCCESS    BIT(2)
#define STATUS_WRITE_FAIL       BIT(3)
#define STATUS_COPY_SUCCESS     BIT(4)
#define STATUS_COPY_FAIL        BIT(5)
#define STATUS_IRQ_RAISED       BIT(6)
#define STATUS_SRC_ADDR_INVALID BIT(7)
#define STATUS_DST_ADDR_INVALID BIT(8)

#define FLAG_USE_DMA BIT(0)

#define TIMER_RESOLUTION 1

#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
static int dma_id = 1;
module_param(dma_id, int, 0444);
MODULE_PARM_DESC(dma_id, "DMA engine id, 0 for PCIe controller 0; 1 for controller 1");

#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
static char *epc_name = "1f351000.pcie";
module_param(epc_name, charp, 0444);
MODULE_PARM_DESC(epc_name, "EP controller name, e.g 1f350000.pcie, 1f351000.pcie");
#endif
#endif

static struct workqueue_struct *kpcitest_workqueue;

struct pci_epf_test
{
    void *                         reg[PCI_STD_NUM_BARS];
    struct pci_epf *               epf;
    enum pci_barno                 test_reg_bar;
    size_t                         msix_table_offset;
    bool                           cmd_notifier;
    struct delayed_work            cmd_handler;
    struct pci_epf_bar             epf_bar[PCI_STD_NUM_BARS];
    struct completion              transfer_complete;
    const struct pci_epc_features *epc_features;
};

struct pci_epf_test_reg
{
    u32 magic;
    u32 command;
    u32 status;
    u64 src_addr;
    u64 dst_addr;
    u32 size;
    u32 checksum;
    u32 irq_type;
    u32 irq_number;
    u32 flags;
} __packed;

static struct pci_epf_header test_header = {
#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
    .vendorid = PCI_VENDOR_ID_TI,
    .deviceid = PCI_DEVICE_ID_TI_DRA74x,
#else
    .vendorid = PCI_ANY_ID,
    .deviceid = PCI_ANY_ID,
#endif
    .baseclass_code = PCI_CLASS_OTHERS,
    .interrupt_pin  = PCI_INTERRUPT_INTA,
};

// static size_t bar_size[] = { 512, 512, 1024, 16384, 131072, 1048576 };
/* bar0 is fixed-size as 1MB, bar1 change to 64KB for test*/
static size_t bar_size[] = {1048576, 65536, 1024, 16384, 131072, 1048576};

/**
 * pci_epf_free_space() - free the allocated PCI EPF register space
 * @addr: the virtual address of the PCI EPF register space
 * @bar: the BAR number corresponding to the register space
 *
 * Invoke to free the allocated PCI EPF register space.
 */
static void _free_space(struct pci_epf *epf, void *addr, enum pci_barno bar)
{
    struct device *      dev      = epf->epc->dev.parent;
    struct pci_epf_test *epf_test = epf_get_drvdata(epf);

    if (!addr)
        return;

    dma_free_coherent(dev, epf_test->epf_bar[bar].size, epf_test->epf_bar[bar].addr, epf_test->epf_bar[bar].phys_addr);

    epf_test->epf_bar[bar].phys_addr = 0;
    epf_test->epf_bar[bar].addr      = NULL;
    epf_test->epf_bar[bar].size      = 0;
    epf_test->epf_bar[bar].barno     = 0;
    epf_test->epf_bar[bar].flags     = 0;

    epf->bar[bar].phys_addr = 0;
    epf->bar[bar].addr      = NULL;
    epf->bar[bar].size      = 0;
    epf->bar[bar].barno     = 0;
    epf->bar[bar].flags     = 0;
}

/**
 * pci_epf_alloc_space() - allocate memory for the PCI EPF register space
 * @size: the size of the memory that has to be allocated
 * @bar: the BAR number corresponding to the allocated register space
 *
 * Invoke to allocate memory for the PCI EPF register space.
 */
void *_alloc_space(struct pci_epf *epf, size_t size, enum pci_barno bar, size_t align)
{
    void *               space;
    struct device *      dev      = epf->epc->dev.parent;
    struct pci_epf_test *epf_test = epf_get_drvdata(epf);
    dma_addr_t           phys_addr;

    if (size < 128)
        size = 128;

    if (align)
        size = ALIGN(size, align);
    else
        size = roundup_pow_of_two(size);

    /* giving twice size to make sure the target addr of Inbound ATU is BAR size aligned */
    space = dma_alloc_coherent(dev, size << 1, &phys_addr, GFP_KERNEL);
    if (!space)
    {
        dev_err(dev, "failed to allocate mem space\n");
        return NULL;
    }
    /* keep the original physical addr, virtual addr, and size */
    epf_test->epf_bar[bar].phys_addr = phys_addr;
    epf_test->epf_bar[bar].addr      = space;
    epf_test->epf_bar[bar].size      = size << 1;
    epf_test->epf_bar[bar].barno     = bar;
    epf_test->epf_bar[bar].flags     = PCI_BASE_ADDRESS_SPACE_MEMORY;

    epf->bar[bar].phys_addr = ALIGN(phys_addr, size);
    space                   = space + (epf->bar[bar].phys_addr - epf_test->epf_bar[bar].phys_addr);
    epf->bar[bar].addr      = space;
    epf->bar[bar].size      = size;
    epf->bar[bar].barno     = bar;
    epf->bar[bar].flags     = PCI_BASE_ADDRESS_SPACE_MEMORY;

    dev_err(dev, "-BAR[%d]:\r\n\tPA_base %llx, VA_base %px\r\n\tPA_align %llx, VA_align %px\r\n", bar,
            epf_test->epf_bar[bar].phys_addr, epf_test->epf_bar[bar].addr, epf->bar[bar].phys_addr, epf->bar[bar].addr);
    return space;
}

#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
static void pci_epf_test_dma_callback(eDmaRequest *req)
{
    struct pci_epf_test *epf_test = req->pCbData[0];

    drvEDMA_ReleaseRequest(req);
    complete(&epf_test->transfer_complete);
}

static int pci_epf_test_data_transfer(struct pci_epf_test *epf_test, dma_addr_t dma_dst, dma_addr_t dma_src, size_t len,
                                      eDmaDir dir)
{
    struct device *dev = &epf_test->epf->dev;
    eDmaRequest *  req;
    int            ret;

    req = drvEDMA_AllocRequest(dir, 1);
    if (!req)
    {
        dev_err(dev, "Failed to allocate DMA request\n");
        return -EINVAL;
    }
    req->burst[0].sar = dma_src;
    req->burst[0].dar = dma_dst;
    req->burst[0].sz  = len;
    req->fpCallback   = pci_epf_test_dma_callback;
    req->pCbData[0]   = epf_test;

    reinit_completion(&epf_test->transfer_complete);
    drvEDMA_SubmitRequest(dma_id, req);

    ret = wait_for_completion_interruptible(&epf_test->transfer_complete);
    if (ret < 0)
    {
        dev_err(dev, "DMA wait_for_completion_timeout\n");
        return -ETIMEDOUT;
    }

    return 0;
}
#endif

static void pci_epf_test_print_rate(const char *ops, u64 size, struct timespec64 *start, struct timespec64 *end,
                                    bool dma)
{
    struct timespec64 ts;
    u64               rate, ns;

    ts = timespec64_sub(*end, *start);

    /* convert both size (stored in 'rate') and time in terms of 'ns' */
    ns   = timespec64_to_ns(&ts);
    rate = size * NSEC_PER_SEC;

    /* Divide both size (stored in 'rate') and ns by a common factor */
    while (ns > UINT_MAX)
    {
        rate >>= 1;
        ns >>= 1;
    }

    if (!ns)
        return;

    /* calculate the rate */
    do_div(rate, (uint32_t)ns);

    pr_info(
        "\n%s => Size: %llu bytes\t DMA: %s\t Time: %llu.%09u seconds\t"
        "Rate: %llu KB/s\n",
        ops, size, dma ? "YES" : "NO", (u64)ts.tv_sec, (u32)ts.tv_nsec, rate / 1024);
}

static void pci_epf_test_dump_bar(struct pci_epf *epf, enum pci_barno bar, u32 size)
{
    u32  i    = 0;
    u32 *addr = epf->bar[bar].addr;

    pr_info("======== DUMP BAR %d, size %d ======================\r\n", bar, size);
    for (i = 0; i < size; i += 4)
    {
        pr_info("%px: %08x %08x %08x %08x\r\n", addr + i, addr[i], addr[i + 1], addr[i + 2], addr[i + 3]);
    }
}

static int pci_epf_test_copy(struct pci_epf_test *epf_test)
{
    int                      ret;
    void __iomem *           src_addr;
    void __iomem *           dst_addr;
    phys_addr_t              src_phys_addr;
    phys_addr_t              dst_phys_addr;
    struct timespec64        start, end;
    struct pci_epf *         epf          = epf_test->epf;
    struct device *          dev          = &epf->dev;
    struct pci_epc *         epc          = epf->epc;
    enum pci_barno           test_reg_bar = epf_test->test_reg_bar;
    struct pci_epf_test_reg *reg          = epf_test->reg[test_reg_bar];

    src_addr = pci_epc_mem_alloc_addr(epc, &src_phys_addr, reg->size);
    if (!src_addr)
    {
        dev_err(dev, "Failed to allocate source address\n");
        reg->status = STATUS_SRC_ADDR_INVALID;
        ret         = -ENOMEM;
        goto err;
    }

    ret = pci_epc_map_addr(epc, epf->func_no, src_phys_addr, reg->src_addr, reg->size);
    if (ret)
    {
        dev_err(dev, "Failed to map source address\n");
        reg->status = STATUS_SRC_ADDR_INVALID;
        goto err_src_addr;
    }

    dst_addr = pci_epc_mem_alloc_addr(epc, &dst_phys_addr, reg->size);
    if (!dst_addr)
    {
        dev_err(dev, "Failed to allocate destination address\n");
        reg->status = STATUS_DST_ADDR_INVALID;
        ret         = -ENOMEM;
        goto err_src_map_addr;
    }

    ret = pci_epc_map_addr(epc, epf->func_no, dst_phys_addr, reg->dst_addr, reg->size);
    if (ret)
    {
        dev_err(dev, "Failed to map destination address\n");
        reg->status = STATUS_DST_ADDR_INVALID;
        goto err_dst_addr;
    }

    ktime_get_ts64(&start);
    memcpy(dst_addr, src_addr, reg->size);
    ktime_get_ts64(&end);
    pci_epf_test_print_rate("COPY", reg->size, &start, &end, false);

    pci_epc_unmap_addr(epc, epf->func_no, dst_phys_addr);

err_dst_addr:
    pci_epc_mem_free_addr(epc, dst_phys_addr, dst_addr, reg->size);

err_src_map_addr:
    pci_epc_unmap_addr(epc, epf->func_no, src_phys_addr);

err_src_addr:
    pci_epc_mem_free_addr(epc, src_phys_addr, src_addr, reg->size);

err:
    return ret;
}

static int pci_epf_test_read(struct pci_epf_test *epf_test)
{
    int                      ret;
    void __iomem *           src_addr;
    void *                   buf;
    u32                      crc32;
    bool                     use_dma;
    phys_addr_t              phys_addr;
    struct timespec64        start, end;
    struct pci_epf *         epf          = epf_test->epf;
    struct device *          dev          = &epf->dev;
    struct pci_epc *         epc          = epf->epc;
    enum pci_barno           test_reg_bar = epf_test->test_reg_bar;
    struct pci_epf_test_reg *reg          = epf_test->reg[test_reg_bar];

    buf     = kzalloc(reg->size, GFP_KERNEL);
    use_dma = !!(reg->flags & FLAG_USE_DMA);
    if (!buf)
    {
        ret = -ENOMEM;
        goto err_map_addr;
    }
    dev_err(dev, "epf: buf %px\n", buf);

    // use_dma = !!(reg->flags & FLAG_USE_DMA);
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (use_dma)
    {
        phys_addr_t dst_phys_addr, dst_miu_addr;
        dst_phys_addr = dma_map_single(epc->dev.parent, buf, reg->size, DMA_FROM_DEVICE);
        dev_err(dev, "epf: dst phys %llx\n", dst_phys_addr);
        dst_miu_addr = (phys_addr_t)Chip_Phys_to_MIU((ss_phys_addr_t)dst_phys_addr);
        dev_err(dev, "epf: src miu %llx\n", reg->src_addr);
        dev_err(dev, "epf: dst miu %llx\n", dst_miu_addr);
        ktime_get_ts64(&start);
        ret = pci_epf_test_data_transfer(epf_test, dst_miu_addr, reg->src_addr, reg->size, EDMA_READ_DIR);
        if (ret)
            dev_err(dev, "Data transfer failed\n");
        ktime_get_ts64(&end);

        dma_unmap_single(epc->dev.parent, dst_phys_addr, reg->size, DMA_FROM_DEVICE);
    }
    else
#endif
    {
        src_addr = pci_epc_mem_alloc_addr(epc, &phys_addr, reg->size);
        if (!src_addr)
        {
            dev_err(dev, "Failed to allocate address\n");
            reg->status = STATUS_SRC_ADDR_INVALID;
            ret         = -ENOMEM;
            goto err;
        }

        ret = pci_epc_map_addr(epc, epf->func_no, phys_addr, reg->src_addr, reg->size);
        if (ret)
        {
            dev_err(dev, "Failed to map address\n");
            reg->status = STATUS_SRC_ADDR_INVALID;
            goto err_addr;
        }
        dev_err(dev, "epf: src phys %llx, virt %px\n", phys_addr, src_addr);

        ktime_get_ts64(&start);
        memcpy_fromio(buf, src_addr, reg->size);
        ktime_get_ts64(&end);
    }

    pci_epf_test_print_rate("READ", reg->size, &start, &end, use_dma);

    crc32 = crc32_le(~0, buf, reg->size);
    dev_err(dev, "crc32 %x, %x\n", crc32, reg->checksum);
    if (crc32 != reg->checksum)
        ret = -EIO;

    kfree(buf);

err_map_addr:
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (!use_dma)
#endif
        pci_epc_unmap_addr(epc, epf->func_no, phys_addr);

err_addr:
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (!use_dma)
#endif
        pci_epc_mem_free_addr(epc, phys_addr, src_addr, reg->size);

err:
    return ret;
}

static int pci_epf_test_write(struct pci_epf_test *epf_test)
{
    int                      ret;
    void __iomem *           dst_addr;
    void *                   buf;
    bool                     use_dma;
    phys_addr_t              phys_addr;
    struct timespec64        start, end;
    struct pci_epf *         epf          = epf_test->epf;
    struct device *          dev          = &epf->dev;
    struct pci_epc *         epc          = epf->epc;
    enum pci_barno           test_reg_bar = epf_test->test_reg_bar;
    struct pci_epf_test_reg *reg          = epf_test->reg[test_reg_bar];

    use_dma = !!(reg->flags & FLAG_USE_DMA);

    buf = kzalloc(reg->size, GFP_KERNEL);
    if (!buf)
    {
        ret = -ENOMEM;
        goto err_map_addr;
    }
    dev_err(dev, "epf: buf %px\n", buf);

    get_random_bytes(buf, reg->size);
    reg->checksum = crc32_le(~0, buf, reg->size);

    // use_dma = !!(reg->flags & FLAG_USE_DMA);
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (use_dma)
    {
        phys_addr_t src_phys_addr, src_miu_addr;
        src_phys_addr = dma_map_single(epc->dev.parent, buf, reg->size, DMA_TO_DEVICE);
        dev_err(dev, "epf: src phys %llx\n", src_phys_addr);
        src_miu_addr = (phys_addr_t)Chip_Phys_to_MIU((ss_phys_addr_t)src_phys_addr);
        dev_err(dev, "epf: src miu %llx\n", src_miu_addr);
        dev_err(dev, "epf: dst miu %llx\n", reg->dst_addr);
        ktime_get_ts64(&start);
        ret = pci_epf_test_data_transfer(epf_test, reg->dst_addr, src_miu_addr, reg->size, EDMA_WRITE_DIR);
        if (ret)
            dev_err(dev, "Data transfer failed\n");
        ktime_get_ts64(&end);

        dma_unmap_single(epc->dev.parent, src_phys_addr, reg->size, DMA_TO_DEVICE);
    }
    else
#endif
    {
        dst_addr = pci_epc_mem_alloc_addr(epc, &phys_addr, reg->size);
        if (!dst_addr)
        {
            dev_err(dev, "Failed to allocate address\n");
            reg->status = STATUS_DST_ADDR_INVALID;
            ret         = -ENOMEM;
            goto err;
        }
        dev_err(dev, "epf: dst phys %llx, virt %px\n", phys_addr, dst_addr);

        ret = pci_epc_map_addr(epc, epf->func_no, phys_addr, reg->dst_addr, reg->size);
        if (ret)
        {
            dev_err(dev, "Failed to map address\n");
            reg->status = STATUS_DST_ADDR_INVALID;
            goto err_addr;
        }

        ktime_get_ts64(&start);
        memcpy_toio(dst_addr, buf, reg->size);
        ktime_get_ts64(&end);
    }

    pci_epf_test_print_rate("WRITE", reg->size, &start, &end, use_dma);

    /*
     * wait 1ms inorder for the write to complete. Without this delay L3
     * error in observed in the host system.
     */
    usleep_range(1000, 2000);
    dev_err(dev, "epf: crc32 %x\n", reg->checksum);

    kfree(buf);

err_map_addr:
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (!use_dma)
#endif
        pci_epc_unmap_addr(epc, epf->func_no, phys_addr);

err_addr:
#if defined(CONFIG_SSTAR_EDMA) || defined(CONFIG_SSTAR_EDMA_MODULE)
    if (!use_dma)
#endif
        pci_epc_mem_free_addr(epc, phys_addr, dst_addr, reg->size);

err:
    return ret;
}

static void pci_epf_test_raise_irq(struct pci_epf_test *epf_test, u8 irq_type, u16 irq)
{
    struct pci_epf *         epf          = epf_test->epf;
    struct device *          dev          = &epf->dev;
    struct pci_epc *         epc          = epf->epc;
    enum pci_barno           test_reg_bar = epf_test->test_reg_bar;
    struct pci_epf_test_reg *reg          = epf_test->reg[test_reg_bar];

    reg->status |= STATUS_IRQ_RAISED;

    switch (irq_type)
    {
        case IRQ_TYPE_LEGACY:
            pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_LEGACY, 0);
            break;
        case IRQ_TYPE_MSI:
            pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSI, irq);
            break;
        case IRQ_TYPE_MSIX:
            pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSIX, irq);
            break;
        default:
            dev_err(dev, "Failed to raise IRQ, unknown type\n");
            break;
    }
}

static void pci_epf_test_cmd_handler(struct work_struct *work)
{
    int                      ret;
    int                      count;
    u32                      command;
    struct pci_epf_test *    epf_test     = container_of(work, struct pci_epf_test, cmd_handler.work);
    struct pci_epf *         epf          = epf_test->epf;
    struct device *          dev          = &epf->dev;
    struct pci_epc *         epc          = epf->epc;
    enum pci_barno           test_reg_bar = epf_test->test_reg_bar;
    struct pci_epf_test_reg *reg          = epf_test->reg[test_reg_bar];

    command = reg->command;
    if (!command)
        goto reset_handler;

    reg->command = 0;
    reg->status  = 0;

    if (command)
    {
        dev_err(dev, "cmd %x\r\n", command);
    }

    if (reg->irq_type > IRQ_TYPE_MSIX)
    {
        dev_err(dev, "Failed to detect IRQ type\n");
        goto reset_handler;
    }

    if (command & COMMAND_RAISE_LEGACY_IRQ)
    {
        dev_err(dev, "LEGACY_IRQ\n");
        reg->status = STATUS_IRQ_RAISED;
        pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_LEGACY, 0);
        goto reset_handler;
    }

    if (command & COMMAND_WRITE)
    {
        dev_err(dev, "WRITE\n");
        ret = pci_epf_test_write(epf_test);
        if (ret)
            reg->status |= STATUS_WRITE_FAIL;
        else
            reg->status |= STATUS_WRITE_SUCCESS;
        dev_err(dev, "epf: raise irq %x num %x\n", reg->irq_type, reg->irq_number);
        pci_epf_test_raise_irq(epf_test, reg->irq_type, reg->irq_number);
        goto reset_handler;
    }

    if (command & COMMAND_READ)
    {
        dev_err(dev, "READ\n");
        ret = pci_epf_test_read(epf_test);
        if (!ret)
            reg->status |= STATUS_READ_SUCCESS;
        else
            reg->status |= STATUS_READ_FAIL;
        dev_err(dev, "epf: raise irq %x num %x\n", reg->irq_type, reg->irq_number);
        pci_epf_test_raise_irq(epf_test, reg->irq_type, reg->irq_number);
        goto reset_handler;
    }

    if (command & COMMAND_COPY)
    {
        dev_err(dev, "COPY\n");
        ret = pci_epf_test_copy(epf_test);
        if (!ret)
            reg->status |= STATUS_COPY_SUCCESS;
        else
            reg->status |= STATUS_COPY_FAIL;
        pci_epf_test_raise_irq(epf_test, reg->irq_type, reg->irq_number);
        goto reset_handler;
    }

    if (command & COMMAND_RAISE_MSI_IRQ)
    {
        dev_err(dev, "MSI\n");
        count = pci_epc_get_msi(epc, epf->func_no);
        if (reg->irq_number > count || count <= 0)
            goto reset_handler;
        reg->status = STATUS_IRQ_RAISED;
        pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSI, reg->irq_number);
        goto reset_handler;
    }

    if (command & COMMAND_RAISE_MSIX_IRQ)
    {
        dev_err(dev, "MSIX\n");
        count = pci_epc_get_msix(epc, epf->func_no);
        if (reg->irq_number > count || count <= 0)
            goto reset_handler;
        reg->status = STATUS_IRQ_RAISED;
        pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSIX, reg->irq_number);
        goto reset_handler;
    }

    if (command & COMMAND_DUMP_BAR)
    {
        pci_epf_test_dump_bar(epf, reg->src_addr, reg->size);
        goto reset_handler;
    }

reset_handler:
    if (!epf_test->cmd_notifier)
        queue_delayed_work(kpcitest_workqueue, &epf_test->cmd_handler, msecs_to_jiffies(1));
}

static irqreturn_t pci_epf_test_irq_handler(int irq, void *arg)
{
    struct pci_epf *     epf      = (struct pci_epf *)arg;
    struct pci_epf_test *epf_test = epf_get_drvdata(epf);

    if (sstar_pcieif_memwr_hit_intr_sta(epf->epc->portid))
    {
        // pr_err("wr_addr_match irq\r\n");
        sstar_pcieif_clr_memwr_hit_intr_sta(epf->epc->portid);
        queue_work(kpcitest_workqueue, &epf_test->cmd_handler.work);
    }

    return IRQ_HANDLED;
}

static void pci_epf_test_unbind(struct pci_epf *epf)
{
    struct pci_epf_test *epf_test = epf_get_drvdata(epf);
    struct pci_epc *     epc      = epf->epc;
    struct pci_epf_bar * epf_bar;
    int                  bar;

    cancel_delayed_work(&epf_test->cmd_handler);
    pci_epc_stop(epc);
    for (bar = BAR_0; bar <= BAR_1; bar++)
    {
        epf_bar = &epf->bar[bar];

        if (epf_test->reg[bar])
        {
            pci_epc_clear_bar(epc, epf->func_no, epf_bar);
            _free_space(epf, epf_test->reg[bar], bar);
        }
    }
}

static int pci_epf_test_set_bar(struct pci_epf *epf)
{
    int                            bar, add;
    int                            ret;
    struct pci_epf_bar *           epf_bar;
    struct pci_epc *               epc          = epf->epc;
    struct device *                dev          = &epf->dev;
    struct pci_epf_test *          epf_test     = epf_get_drvdata(epf);
    enum pci_barno                 test_reg_bar = epf_test->test_reg_bar;
    const struct pci_epc_features *epc_features;

    epc_features = epf_test->epc_features;

    for (bar = BAR_0; bar <= BAR_1; bar += add)
    {
        epf_bar = &epf->bar[bar];
        /*
         * pci_epc_set_bar() sets PCI_BASE_ADDRESS_MEM_TYPE_64
         * if the specific implementation required a 64-bit BAR,
         * even if we only requested a 32-bit BAR.
         */
        add = (epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64) ? 2 : 1;

        if (!!(epc_features->reserved_bar & (1 << bar)))
            continue;

        pr_err("set BAR%d size x%lx\n", bar, epf_bar->size);
        ret = pci_epc_set_bar(epc, epf->func_no, epf_bar);
        if (ret)
        {
            _free_space(epf, epf_test->reg[bar], bar);
            dev_err(dev, "Failed to set BAR%d\n", bar);
            if (bar == test_reg_bar)
                return ret;
        }
    }

    return 0;
}

static int pci_epf_test_core_init(struct pci_epf *epf)
{
    struct pci_epf_test *          epf_test = epf_get_drvdata(epf);
    struct pci_epf_header *        header   = epf->header;
    const struct pci_epc_features *epc_features;
    struct pci_epc *               epc = epf->epc;
    struct device *                dev = &epf->dev;
    struct dw_pcie_ep *            ep;
    bool                           msix_capable = false;
    bool                           msi_capable  = true;
    int                            ret;

    epc_features = pci_epc_get_features(epc, epf->func_no);
    if (epc_features)
    {
        msix_capable = epc_features->msix_capable;
        msi_capable  = epc_features->msi_capable;
    }

    ret = pci_epc_write_header(epc, epf->func_no, header);
    if (ret)
    {
        dev_err(dev, "Configuration header write failed\n");
        return ret;
    }

    ret = pci_epf_test_set_bar(epf);
    if (ret)
        return ret;

    if (msi_capable)
    {
        ret = pci_epc_set_msi(epc, epf->func_no, epf->msi_interrupts);
        if (ret)
        {
            dev_err(dev, "MSI configuration failed\n");
            return ret;
        }
    }

    if (msix_capable)
    {
        ret = pci_epc_set_msix(epc, epf->func_no, epf->msix_interrupts, epf_test->test_reg_bar,
                               epf_test->msix_table_offset);
        if (ret)
        {
            dev_err(dev, "MSI-X configuration failed\n");
            return ret;
        }
    }

    ep = (struct dw_pcie_ep *)epc_get_drvdata(epc);
    if (ep->irq > 0)
    {
        ss_miu_addr_t test_reg_cmd = epf_test->epf->bar[epf_test->test_reg_bar].phys_addr;

        /* trigger interrupt when RC write the command */
        test_reg_cmd += offsetof(struct pci_epf_test_reg, command);
        test_reg_cmd = Chip_Phys_to_MIU(test_reg_cmd);
        sstar_pcieif_enable_memwr_hit_intr(epc->portid, test_reg_cmd);
        dev_err(dev, "wr_match_addr 0x%llx, irq %d", test_reg_cmd, ep->irq);
        ret = devm_request_irq(&epc->dev, ep->irq, pci_epf_test_irq_handler, IRQF_SHARED, "pci_epf_test", epf);
        if (ret)
        {
            dev_err(dev, "failed to request irq, ret = %d\n", ret);
            return ret;
        }
        epf_test->cmd_notifier = true;
    }

    return 0;
}

static int pci_epf_test_notifier(struct notifier_block *nb, unsigned long val, void *data)
{
    struct pci_epf *     epf      = container_of(nb, struct pci_epf, nb);
    struct pci_epf_test *epf_test = epf_get_drvdata(epf);
    int                  ret;

    switch (val)
    {
        case CORE_INIT:
            ret = pci_epf_test_core_init(epf);
            if (ret)
                return NOTIFY_BAD;
            break;

        case LINK_UP:
            queue_delayed_work(kpcitest_workqueue, &epf_test->cmd_handler, msecs_to_jiffies(1));
            break;

        default:
            dev_err(&epf->dev, "Invalid EPF test notifier event\n");
            return NOTIFY_BAD;
    }

    return NOTIFY_OK;
}

static int pci_epf_test_alloc_space(struct pci_epf *epf)
{
    struct pci_epf_test *          epf_test = epf_get_drvdata(epf);
    struct device *                dev      = &epf->dev;
    struct pci_epf_bar *           epf_bar;
    size_t                         msix_table_size = 0;
    size_t                         test_reg_bar_size;
    size_t                         pba_size = 0;
    bool                           msix_capable;
    void *                         base;
    int                            bar, add;
    enum pci_barno                 test_reg_bar = epf_test->test_reg_bar;
    const struct pci_epc_features *epc_features;
    size_t                         test_reg_size;

    epc_features = epf_test->epc_features;

    test_reg_bar_size = ALIGN(sizeof(struct pci_epf_test_reg), 128);

    msix_capable = epc_features->msix_capable;
    if (msix_capable)
    {
        msix_table_size             = PCI_MSIX_ENTRY_SIZE * epf->msix_interrupts;
        epf_test->msix_table_offset = test_reg_bar_size;
        /* Align to QWORD or 8 Bytes */
        pba_size = ALIGN(DIV_ROUND_UP(epf->msix_interrupts, 8), 8);
    }
    test_reg_size = test_reg_bar_size + msix_table_size + pba_size;

    if (epc_features->bar_fixed_size[test_reg_bar])
    {
        if (test_reg_size > bar_size[test_reg_bar])
            return -ENOMEM;
        test_reg_size = bar_size[test_reg_bar];
    }

    base = _alloc_space(epf, test_reg_size, test_reg_bar, epc_features->align);
    if (!base)
    {
        dev_err(dev, "Failed to allocated register space\n");
        return -ENOMEM;
    }
    epf_test->reg[test_reg_bar] = base;

    for (bar = BAR_0; bar <= BAR_1; bar += add)
    {
        epf_bar = &epf->bar[bar];
        add     = (epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64) ? 2 : 1;

        if (bar == test_reg_bar)
            continue;

        if (!!(epc_features->reserved_bar & (1 << bar)))
            continue;

        base = _alloc_space(epf, bar_size[bar], bar, epc_features->align);
        if (!base)
            dev_err(dev, "Failed to allocate space for BAR%d\n", bar);
        epf_test->reg[bar] = base;
    }
    return 0;
}

static void pci_epf_configure_bar(struct pci_epf *epf, const struct pci_epc_features *epc_features)
{
    struct pci_epf_bar *epf_bar;
    bool                bar_fixed_64bit;
    int                 i;

    for (i = 0; i < PCI_STD_NUM_BARS; i++)
    {
        epf_bar         = &epf->bar[i];
        bar_fixed_64bit = !!(epc_features->bar_fixed_64bit & (1 << i));
        if (bar_fixed_64bit)
            epf_bar->flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
        if (epc_features->bar_fixed_size[i])
            bar_size[i] = epc_features->bar_fixed_size[i];
    }
}

static int pci_epf_test_bind(struct pci_epf *epf)
{
    int                            ret;
    struct pci_epf_test *          epf_test = epf_get_drvdata(epf);
    const struct pci_epc_features *epc_features;
    enum pci_barno                 test_reg_bar       = BAR_0;
    struct pci_epc *               epc                = epf->epc;
    bool                           linkup_notifier    = false;
    bool                           core_init_notifier = false;

    if (WARN_ON_ONCE(!epc))
        return -EINVAL;

    epc_features = pci_epc_get_features(epc, epf->func_no);
    if (epc_features)
    {
        linkup_notifier    = epc_features->linkup_notifier;
        core_init_notifier = epc_features->core_init_notifier;
        test_reg_bar       = pci_epc_get_first_free_bar(epc_features);
        pci_epf_configure_bar(epf, epc_features);
    }

    epf_test->test_reg_bar = test_reg_bar;
    epf_test->epc_features = epc_features;

    ret = pci_epf_test_alloc_space(epf);
    if (ret)
        return ret;

    if (!core_init_notifier)
    {
        ret = pci_epf_test_core_init(epf);
        if (ret)
            return ret;
    }

    if (linkup_notifier)
    {
        epf->nb.notifier_call = pci_epf_test_notifier;
        pci_epc_register_notifier(epc, &epf->nb);
    }
    else
    {
        queue_work(kpcitest_workqueue, &epf_test->cmd_handler.work);
    }

    return 0;
}

static const struct pci_epf_device_id pci_epf_test_ids[] = {
    {
        .name = "pci_epf_test",
    },
    {},
};

static int pci_epf_test_probe(struct pci_epf *epf)
{
    struct pci_epf_test *epf_test;
    struct device *      dev          = &epf->dev;
    bool                 cmd_notifier = false;
    int                  ret          = 0;
#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
    struct pci_epc *epc;
#endif

    epf_test = devm_kzalloc(dev, sizeof(*epf_test), GFP_KERNEL);
    if (!epf_test)
        return -ENOMEM;

    epf->header = &test_header;
#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
    epf->msi_interrupts  = 16;
    epf->msix_interrupts = 8;
    epf->func_no         = 0;
#endif
    epf_test->epf          = epf;
    epf_test->cmd_notifier = cmd_notifier;
    init_completion(&epf_test->transfer_complete);

    INIT_DELAYED_WORK(&epf_test->cmd_handler, pci_epf_test_cmd_handler);

    epf_set_drvdata(epf, epf_test);

#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
    epc = pci_epc_get(epc_name);
    if (IS_ERR(epc))
    {
        dev_err(dev, "failed to get epc %s\r\n", epc_name);
        return -EINVAL;
    }

    ret = pci_epc_add_epf(epc, epf);
    if (ret)
    {
        dev_err(dev, "epc failed to add epf\r\n");
        goto err_epf;
    }

    ret = pci_epf_bind(epf);
    if (ret)
    {
        dev_err(dev, "epf failed to bind\r\n");
        pci_epc_remove_epf(epc, epf);
        goto err_epf;
    }

    ret = pci_epc_start(epc);
    if (ret)
    {
        dev_err(dev, "failed to start epc\r\n");
        pci_epf_unbind(epf);
        pci_epc_remove_epf(epc, epf);
        goto err_epf;
    }
    return 0;

err_epf:
    pci_epc_put(epc);
#endif

    return ret;
}

static struct pci_epf_ops ops = {
    .unbind = pci_epf_test_unbind,
    .bind   = pci_epf_test_bind,
};

static struct pci_epf_driver test_driver = {
    .driver.name = "pci_epf_test",
    .probe       = pci_epf_test_probe,
    .id_table    = pci_epf_test_ids,
    .ops         = &ops,
    .owner       = THIS_MODULE,
};

static int __init pci_epf_test_init(void)
{
    int ret;

    kpcitest_workqueue = alloc_workqueue("kpcitest", WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
    if (!kpcitest_workqueue)
    {
        pr_err("Failed to allocate the kpcitest work queue\n");
        return -ENOMEM;
    }
    ret = pci_epf_register_driver(&test_driver);
    if (ret)
    {
        pr_err("Failed to register pci epf test driver --> %d\n", ret);
        return ret;
    }
#ifndef CONFIG_PCI_ENDPOINT_CONFIGFS
    if (IS_ERR(pci_epf_create("pci_epf_test.0")))
    {
        pr_err("failed to create epf device\r\n");
        return -EINVAL;
    }
#endif

    return 0;
}
module_init(pci_epf_test_init);

static void __exit pci_epf_test_exit(void)
{
    pci_epf_unregister_driver(&test_driver);
}
module_exit(pci_epf_test_exit);

MODULE_DESCRIPTION("PCI EPF TEST DRIVER");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
