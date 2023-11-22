/*
 * mdr_sata_host_ahci_platform.c- Sigmastar
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/acpi.h>
#include <linux/pci_ids.h>
#include <linux/printk.h>
#include "ms_msys.h"
#include "ms_platform.h"
#include <cam_os_wrapper.h>

#include "ahci.h"
#include "mhal_sata_host.h"
#include "mhal_sata_host_ahci.h"
#include "mdrv_sata_host_ahci.h"

#define DRV_NAME    "ahci_sstar"
#define SW_OOB_MODE 0

static int n_ports = 1; // config sata ports //TBD

extern int g_hd_link0[4];

u32 port0_addr0 = 0;
u32 misc_addr0  = 0;

#define SSTAR_AHCI_SHT(drv_name) ATA_NCQ_SHT(drv_name), .shost_attrs = ahci_shost_attrs, .sdev_attrs = ahci_sdev_attrs

#define SATA_SSTAR_HOST_FLAGS (ATA_FLAG_SATA | ATA_FLAG_PIO_DMA | ATA_FLAG_ACPI_SATA | ATA_FLAG_AN | ATA_FLAG_NCQ)

struct _sstar_phy_adjust phy_adj0;

static unsigned int sstar_ahci_fill_sg(struct ata_queued_cmd *qc, void *cmd_tbl)
{
    struct scatterlist *sg;
    struct ahci_sg *    ahci_sg = cmd_tbl + AHCI_CMD_TBL_HDR_SZ;
    unsigned int        si;

    VPRINTK("ENTER\n");

    /*
     * Next, the S/G list.
     */
    for_each_sg(qc->sg, sg, qc->n_elem, si)
    {
        dma_addr_t addr   = sg_dma_address(sg);
        u32        sg_len = sg_dma_len(sg);

        // pr_err("%s , addr = 0x%llX ahci_sg[si].addr =0x%X , ahci_sg[si].addr_hi=0x%X\n", __func__, addr,
        //            cpu_to_le32(addr & 0xffffffff), cpu_to_le32((addr >> 16) >> 16));

#ifdef CONFIG_SS_SATA_AHCI_PLATFORM_HOST

#ifndef CONFIG_TEST_ALL_LINUX_MMU_MAP
        ahci_sg[si].addr    = (u32)(Chip_Phys_to_MIU(cpu_to_le32(addr & 0xffffffff)));
        ahci_sg[si].addr_hi = (u32)(Chip_Phys_to_MIU(cpu_to_le32((addr >> 16) >> 16)));
#else
        dma_addr_t miu_addr = Chip_Phys_to_MIU(addr);
        ahci_sg[si].addr    = (u32)cpu_to_le32(miu_addr & 0xffffffff);
        ahci_sg[si].addr_hi = (u32)(cpu_to_le32(miu_addr >> 16) >> 16);
#endif

#else
        ahci_sg[si].addr    = cpu_to_le32(addr & 0xffffffff);
        ahci_sg[si].addr_hi = cpu_to_le32((addr >> 16) >> 16);
#endif
        ahci_sg[si].flags_size = cpu_to_le32(sg_len - 1);

        // pr_err("%s , ahci_sg[si].addr =0x%X , ahci_sg[si].addr_hi=0x%X\n", __func__,ahci_sg[si].addr ,
        // ahci_sg[si].addr_hi );
    }

    return si;
}

static enum ata_completion_errors sstar_ahci_qc_prep(struct ata_queued_cmd *qc)
{
    struct ata_port *      ap       = qc->ap;
    struct ahci_port_priv *pp       = ap->private_data;
    int                    is_atapi = ata_is_atapi(qc->tf.protocol);
    void *                 cmd_tbl;
    u32                    opts;
    const u32              cmd_fis_len = 5; /* five dwords */
    unsigned int           n_elem;

    /*
     * Fill in command table information.  First, the header,
     * a SATA Register - Host to Device command FIS.
     */
    cmd_tbl = pp->cmd_tbl + qc->hw_tag * AHCI_CMD_TBL_SZ;

    ata_tf_to_fis(&qc->tf, qc->dev->link->pmp, 1, cmd_tbl);
    if (is_atapi)
    {
        memset(cmd_tbl + AHCI_CMD_TBL_CDB, 0, 32);
        memcpy(cmd_tbl + AHCI_CMD_TBL_CDB, qc->cdb, qc->dev->cdb_len);
    }

    n_elem = 0;
    if (qc->flags & ATA_QCFLAG_DMAMAP)
        n_elem = sstar_ahci_fill_sg(qc, cmd_tbl);

    /*
     * Fill in command slot information.
     */
    opts = cmd_fis_len | n_elem << 16 | (qc->dev->link->pmp << 12);
    if (qc->tf.flags & ATA_TFLAG_WRITE)
        opts |= AHCI_CMD_WRITE;
    if (is_atapi)
        opts |= AHCI_CMD_ATAPI | AHCI_CMD_PREFETCH;

    ahci_fill_cmd_slot(pp, qc->hw_tag, opts);

    return AC_ERR_OK;
}

static void sstar_ahci_host_stop(struct ata_host *host)
{
#ifdef CONFIG_SS_SATA_AHCI_PLATFORM_HOST
    struct device *            dev   = host->dev;
    struct ahci_platform_data *pdata = dev_get_platdata(dev);
#endif

    struct ahci_host_priv *hpriv = host->private_data;

#ifdef CONFIG_SS_SATA_AHCI_PLATFORM_HOST
    if (pdata && pdata->exit)
        pdata->exit(dev);
#endif

    ahci_platform_disable_resources(hpriv);
}

static int sstar_ahci_port_start(struct ata_port *ap)
{
    struct ahci_host_priv *hpriv = ap->host->private_data;
    struct device *        dev   = ap->host->dev;
    struct ahci_port_priv *pp;
    void *                 mem;
    dma_addr_t             mem_dma;
    size_t                 dma_sz, rx_fis_sz;

    pp = devm_kzalloc(dev, sizeof(*pp), GFP_KERNEL);
    if (!pp)
        return -ENOMEM;

    if (ap->host->n_ports > 1)
    {
        pp->irq_desc = devm_kzalloc(dev, 8, GFP_KERNEL);
        if (!pp->irq_desc)
        {
            devm_kfree(dev, pp);
            return -ENOMEM;
        }
        snprintf(pp->irq_desc, 8, "%s%d", dev_driver_string(dev), ap->port_no);
    }

    /* check FBS capability */
    if ((hpriv->cap & HOST_CAP_FBS) && sata_pmp_supported(ap))
    {
        void __iomem *port_mmio = ahci_port_base(ap);
        u32           cmd       = readl(port_mmio + PORT_CMD);
        if (cmd & PORT_CMD_FBSCP)
            pp->fbs_supported = true;
        else if (hpriv->flags & AHCI_HFLAG_YES_FBS)
        {
            dev_info(dev, "port %d can do FBS, forcing FBSCP\n", ap->port_no);
            pp->fbs_supported = true;
        }
        else
            dev_warn(dev, "port %d is not capable of FBS\n", ap->port_no);
    }

    if (pp->fbs_supported)
    {
        dma_sz    = AHCI_PORT_PRIV_FBS_DMA_SZ;
        rx_fis_sz = AHCI_RX_FIS_SZ * 16;
    }
    else
    {
        dma_sz    = AHCI_PORT_PRIV_DMA_SZ;
        rx_fis_sz = AHCI_RX_FIS_SZ;
    }

    //    of_dma_configure(dev,dev->of_node,true);
    //    dma_set_mask_and_coherent(dev,DMA_BIT_MASK(64));

    mem = dmam_alloc_coherent(dev, dma_sz, &mem_dma, GFP_KERNEL);
    if (!mem)
        return -ENOMEM;
    memset(mem, 0, dma_sz);

    /*
     * First item in chunk of DMA memory: 32-slot command table,
     * 32 bytes each in size
     */
    pp->cmd_slot = mem;
#ifdef CONFIG_SS_SATA_AHCI_PLATFORM_HOST
    // Translate physical address to bus address since SATA engine uses bus address.
    mem_dma = (dma_addr_t)Chip_Phys_to_MIU(mem_dma);
#endif
    pp->cmd_slot_dma = mem_dma;

    mem += AHCI_CMD_SLOT_SZ;
    mem_dma += AHCI_CMD_SLOT_SZ;

    /*
     * Second item: Received-FIS area
     */
    pp->rx_fis     = mem;
    pp->rx_fis_dma = mem_dma;

    mem += rx_fis_sz;
    mem_dma += rx_fis_sz;

    /*
     * Third item: data area for storing a single command
     * and its scatter-gather table
     */
    pp->cmd_tbl     = mem;
    pp->cmd_tbl_dma = mem_dma;

    // pr_err("%s , pp->cmd_tbl = 0x%p , pp->cmd_tbl_dma = 0x%llX \n",__func__ , pp->cmd_tbl , pp->cmd_tbl_dma);

    /*
     * Save off initial list of interrupts to be enabled.
     * This could be changed later
     */
    pp->intr_mask = DEF_PORT_IRQ;

    /*
     * Switch to per-port locking in case each port has its own MSI vector.
     */
    if (hpriv->flags & AHCI_HFLAG_MULTI_MSI)
    {
        spin_lock_init(&pp->lock);
        ap->lock = &pp->lock;
    }

    ap->private_data = pp;

    /* engage engines, captain */
    return ahci_port_resume(ap);
}

void sstar_ahci_sw_activity(struct ata_link *link)
{
    struct ata_port *      ap  = link->ap;
    struct ahci_port_priv *pp  = ap->private_data;
    struct ahci_em_priv *  emp = &pp->em_priv[link->pmp];

    if (!(link->flags & ATA_LFLAG_SW_ACTIVITY))
        return;

    emp->activity++;
    if (!timer_pending(&emp->timer))
        mod_timer(&emp->timer, jiffies + msecs_to_jiffies(10));
}

unsigned int sstar_ahci_qc_issue(struct ata_queued_cmd *qc)
{
    struct ata_port *      ap        = qc->ap;
    void __iomem *         port_mmio = ahci_port_base(ap);
    struct ahci_port_priv *pp        = ap->private_data;

    /* Keep track of the currently active link.  It will be used
     * in completion path to determine whether NCQ phase is in
     * progress.
     */
    pp->active_link = qc->dev->link;

#ifdef CONFIG_SS_SATA_AHCI_PLATFORM_HOST
    Chip_Flush_MIU_Pipe();
#endif

    if (ata_is_ncq(qc->tf.protocol))
        writel(1 << qc->hw_tag, port_mmio + PORT_SCR_ACT);

    if (pp->fbs_enabled && pp->fbs_last_dev != qc->dev->link->pmp)
    {
        u32 fbs = readl(port_mmio + PORT_FBS);
        fbs &= ~(PORT_FBS_DEV_MASK | PORT_FBS_DEC);
        fbs |= qc->dev->link->pmp << PORT_FBS_DEV_OFFSET;
        writel(fbs, port_mmio + PORT_FBS);
        pp->fbs_last_dev = qc->dev->link->pmp;
    }
#if 0
    pr_err("%s, 6+0, qc->tag = 0x%X, port_mmio + PORT_LST_ADDR_HI = 0x%08X, port_mmio + PORT_LST_ADDR = 0x%08X\n",
            __func__ , qc->tag, readl(port_mmio + PORT_LST_ADDR_HI) , readl(port_mmio + PORT_LST_ADDR));

    pr_err("%s, 6+0, port_mmio + PORT_FIS_ADDR_HI = 0x%08X, port_mmio + PORT_FIS_ADDR = 0x%08X\n",
                    __func__ , readl(port_mmio + PORT_FIS_ADDR_HI) , readl(port_mmio + PORT_FIS_ADDR));
#endif
    writel(1 << qc->hw_tag, port_mmio + PORT_CMD_ISSUE);

    sstar_ahci_sw_activity(qc->dev->link);

    return 0;
}
EXPORT_SYMBOL_GPL(sstar_ahci_qc_issue);

struct ata_port_operations sstar_ahci_platform_ops = {
    .inherits  = &ahci_ops,
    .qc_prep   = sstar_ahci_qc_prep,
    .qc_issue  = sstar_ahci_qc_issue,
    .host_stop = sstar_ahci_host_stop,
#ifdef CONFIG_PM
//  .port_suspend       = ahci_port_suspend,  this no need
//  .port_resume        = sstar_ahci_port_resume,
#endif
    .port_start = sstar_ahci_port_start,
};

static const struct ata_port_info ahci_port_info = {
    .flags     = SATA_SSTAR_HOST_FLAGS,
    .pio_mask  = ATA_PIO4,
    .udma_mask = ATA_UDMA6,
    .port_ops  = &sstar_ahci_platform_ops,
};

#ifdef CONFIG_ARM64
static struct scsi_host_template ahci_platform_sht = {
    SSTAR_AHCI_SHT(DRV_NAME),
    .can_queue    = 32,                   // SATA_SSTAR_QUEUE_DEPTH,
    .sg_tablesize = 128,                  // SATA_SSTAR_USED_PRD,
    .dma_boundary = 0xFFFFFFFFFFFFFFFFUL, // ATA_DMA_BOUNDARY,
};
#else
static struct scsi_host_template ahci_platform_sht = {
    SSTAR_AHCI_SHT(DRV_NAME),
    .can_queue = 32,              // SATA_SSTAR_QUEUE_DEPTH,
    .sg_tablesize = 128,          // SATA_SSTAR_USED_PRD,
    .dma_boundary = 0xFFFFFFFFUL, // ATA_DMA_BOUNDARY,
};
#endif

int __ss_sata_get_phy_mode(struct sstar_ahci_priv *priv)
{
    struct device_node *    dev_node;
    struct platform_device *pdev;
    int                     phy_mode               = 2;
    u32                     ssc_mode               = 0;
    u32                     tx_gen1_gen2_vpp[2]    = {0xFF, 0xFF};
    u32                     tx_gen3_de_emphasis[3] = {0xFF, 0xFF, 0xFF};

    // u32 rx_eq_cs[3]  = {0xFF, 0xFF, 0xFF};
    u32 rx_eq_ctle   = 0xFF;
    u32 rx_eq_rs1[3] = {0xFF, 0xFF, 0xFF};
    u32 rx_eq_rs2[3] = {0xFF, 0xFF, 0xFF};

    dev_node = of_find_compatible_node(NULL, NULL, SSTAR_SATA_DTS_NAME);

    if (!dev_node)
        return -ENODEV;

    pdev = of_find_device_by_node(dev_node);
    if (!pdev)
    {
        of_node_put(dev_node);
        return -ENODEV;
    }

    of_property_read_u32(dev_node, "phy_mode", &phy_mode);
    of_property_read_u32(dev_node, "tx_ssc_mode", &ssc_mode);

    printk("[SATA] phy_mode =%d , tx_ssc_mode = 0x%X\n", phy_mode, ssc_mode);

    of_property_read_u32_array(dev_node, "tx_gen1_gen2_Vpp", tx_gen1_gen2_vpp, 2);
    of_property_read_u32_array(dev_node, "tx_gen3_de_emphasis", tx_gen3_de_emphasis, 3);
    // of_property_read_u32_array(dev_node, "rx_eq_cs", rx_eq_cs, 3);
    of_property_read_u32(dev_node, "rx_eq_ctle", &rx_eq_ctle);
    of_property_read_u32_array(dev_node, "rx_eq_rs1", rx_eq_rs1, 3);
    of_property_read_u32_array(dev_node, "rx_eq_rs2", rx_eq_rs2, 3);

    if ((tx_gen1_gen2_vpp[0] != 0xFF) || (tx_gen1_gen2_vpp[1] != 0xFF))
    {
        printk("[SATA] tx_gen1_gen2_Vpp = 0x%X, 0x%X\n", tx_gen1_gen2_vpp[0], tx_gen1_gen2_vpp[1]);
    }

    if (tx_gen3_de_emphasis[0] != 0xFF)
    {
        printk("[SATA] tx_gen3_de_emphasis = 0x%X, 0x%X, 0x%X\n", tx_gen3_de_emphasis[0], tx_gen3_de_emphasis[1],
               tx_gen3_de_emphasis[2]);
    }

    /*
    if ((rx_eq_cs[0] != 0xFF) || (rx_eq_cs[1] != 0xFF) || (rx_eq_cs[2] != 0xFF))
    {
        printk("[SATA] rx_eq_cs = 0x%X, 0x%X, 0x%X\n", rx_eq_cs[0], rx_eq_cs[1], rx_eq_cs[2]);
    }
    */

    if (rx_eq_ctle != 0xFF)
    {
        printk("[SATA] rx_eq_ctle = 0x%X\n", rx_eq_ctle);
    }

    if ((rx_eq_rs1[0] != 0xFF) || (rx_eq_rs1[1] != 0xFF) || (rx_eq_rs1[2] != 0xFF))
    {
        printk("[SATA] rx_eq_rs1 = 0x%X, 0x%X, 0x%X\n", rx_eq_rs1[0], rx_eq_rs1[1], rx_eq_rs1[2]);
    }

    if ((rx_eq_rs2[0] != 0xFF) || (rx_eq_rs2[1] != 0xFF) || (rx_eq_rs2[2] != 0xFF))
    {
        printk("[SATA] rx_eq_rs2 = 0x%X, 0x%X, 0x%X\n", rx_eq_rs2[0], rx_eq_rs2[1], rx_eq_rs2[2]);
    }

    priv->phy_mode               = phy_mode;
    priv->ssc_mode               = ssc_mode;
    priv->tx_gen1_gen2_vpp[0]    = tx_gen1_gen2_vpp[0];
    priv->tx_gen1_gen2_vpp[1]    = tx_gen1_gen2_vpp[1];
    priv->tx_gen3_de_emphasis[0] = tx_gen3_de_emphasis[0];
    priv->tx_gen3_de_emphasis[1] = tx_gen3_de_emphasis[1];
    priv->tx_gen3_de_emphasis[2] = tx_gen3_de_emphasis[2];
    priv->rx_eq_ctle             = rx_eq_ctle;
    priv->rx_eq_rs1[0]           = rx_eq_rs1[0];
    priv->rx_eq_rs1[1]           = rx_eq_rs1[1];
    priv->rx_eq_rs1[2]           = rx_eq_rs1[2];
    priv->rx_eq_rs2[0]           = rx_eq_rs2[0];
    priv->rx_eq_rs2[1]           = rx_eq_rs2[1];
    priv->rx_eq_rs2[2]           = rx_eq_rs2[2];

    return phy_mode;
}

static ssize_t sata_link_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return 0;
}

static ssize_t sata_link_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str     = buf;
    int          i       = 0;
    const int    nGen    = 4;
    unsigned int nStrLen = PAGE_SIZE;

    str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "link : 0, G1, G2,G3 \n");

    for (i = 0; i < nGen; ++i)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "link0[%d] = %d ,", i, g_hd_link0[i]);
    }

    str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "\n");

    return str - buf;
}

DEVICE_ATTR(sata_dump, 0644, sata_link_show, sata_link_store);

static ssize_t sata_de_emphasis_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int nArg   = 0;
    int db     = 0;
    int enable = 0;
    int level  = 0;

    nArg = sscanf(buf, "%d %d %d", &enable, &db, &level);
    if (3 != nArg)
    {
        pr_err("[%s] invalid argument\n", __FUNCTION__);
        pr_err("usage:\n");
        pr_err("echo [enable] [db] [level]  > de_emphasis\n");
        pr_err("enable = 0 or 1\n");
        pr_err(
            "enable = 0 , db= [0,1,2] --> db value no active, refer to gen parameter, Gen1/Gen2/Gen3 work with their "
            "seperate level\n");
        pr_err("enable = 1 , db= 0 --> Gen3 run -1.5db\n");
        pr_err("enable = 1 , db= 1 --> Gen3 run -2.5db\n");
        pr_err("enable = 1 , db= 2 --> Gen3 run -3.5db\n");
        pr_err("level= 0 --> 0.4\n");
        pr_err("level= 1 --> 0.5\n");
        pr_err("level= 2 --> 0.6\n");
        pr_err("level= 3 --> 0.7\n");
        pr_err("level= 4 --> 0.8\n");
        pr_err("level= 5 --> 0.9 , with Gen3/Gen2 can apply, Gen1   can not apply\n");
        pr_err("level= 6 --> 1.0 , with Gen3 can apply       , Gen1/2 can not apply\n");
        pr_err("level= 7 --> 1.1 , with Gen3 can apply       , Gen1/2 can not apply\n");
        goto out;
    }

    if ((enable != 0) && (enable != 1))
    {
        pr_err("%s , not correct enable = %d\n", __func__, enable);
        goto out;
    }

    if ((db != DE_EMPHASIS_1p5_DB) && (db != DE_EMPHASIS_2p5_DB) && (db != DE_EMPHASIS_3p5_DB))
    {
        pr_err("%s , not correct db = %d\n", __func__, db);
        goto out;
    }

    if (!((level >= DE_EMPHASIS_LEVEL_0p4) && (level <= DE_EMPHASIS_LEVEL_1p1)))
    {
        pr_err("%s , not correct level = %d\n", __func__, level);
        goto out;
    }

    phy_adj0.dmhs.enable = enable;
    phy_adj0.dmhs.level  = level;
    phy_adj0.dmhs.db     = db;

    hal_sata_set_de_emphasis(0, &phy_adj0);
    phy_adj0.new_de_emphasis = 1;
out:
    return count;
}

static ssize_t sata_de_emphasis_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str     = buf;
    unsigned int nStrLen = PAGE_SIZE;

    if (phy_adj0.new_de_emphasis == 0)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "no set new de_emphasis , using default setting\n");

        goto out;
    }

    str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "Gen3 de_emphasis now is %s \n",
                         (phy_adj0.dmhs.enable == 1) ? ("enable") : ("disable"));

    if (phy_adj0.dmhs.enable == 1)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "current run on %s mode\n",
                             (phy_adj0.dmhs.db == 0) ? ("Gen3 -1.5db")
                                                     : ((phy_adj0.dmhs.db == 1) ? ("Gen3 -2.5db") : ("Gen3 -3.5db")));
        //        "Gen3 -1.5db"
        //        "Gen3 -2.5db"
        //        "Gen3 -3.5db"
    }
    else
    {
        str +=
            CamOsSnprintf(str, nStrLen - (int)(str - buf), "current run on %s mode\n",
                          (phy_adj0.test_gen == 3) ? ("Gen3 Vppdiff")
                                                   : ((phy_adj0.test_gen == 2) ? ("Gen2 Vppdiff") : ("Gen1 Vppdiff")));
    }

    if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p4)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.4\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p5)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.5\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p6)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.6\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p7)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.7\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p8)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.8\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_0p9)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 0.9\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_1p0)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 1.0\n");
    }
    else if (phy_adj0.dmhs.level == DE_EMPHASIS_LEVEL_1p1)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is 1.1\n");
    }
    else
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now level is unknow , %d \n", phy_adj0.dmhs.level);
    }

out:
    pr_err("usage:\n");
    pr_err("echo [enable] [db] [level]  > de_emphasis\n");
    pr_err("enable = 0 or 1\n");
    pr_err(
        "enable = 0 , db= [0,1,2] --> db value no active, refer to gen parameter, Gen1/Gen2/Gen3 work with their "
        "seperate level\n");
    pr_err("enable = 1 , db= 0 --> Gen3 run -1.5db\n");
    pr_err("enable = 1 , db= 1 --> Gen3 run -2.5db\n");
    pr_err("enable = 1 , db= 2 --> Gen3 run -3.5db\n");
    pr_err("level= 0 --> 0.4\n");
    pr_err("level= 1 --> 0.5\n");
    pr_err("level= 2 --> 0.6\n");
    pr_err("level= 3 --> 0.7\n");
    pr_err("level= 4 --> 0.8\n");
    pr_err("level= 5 --> 0.9 , with Gen3/Gen2 can apply, Gen1   can not apply\n");
    pr_err("level= 6 --> 1.0 , with Gen3 can apply       , Gen1/2 can not apply\n");
    pr_err("level= 7 --> 1.1 , with Gen3 can apply       , Gen1/2 can not apply\n");

    return str - buf;
}
DEVICE_ATTR(de_emphasis, 0644, sata_de_emphasis_show, sata_de_emphasis_store);

static ssize_t cts_pattern_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int nArg    = 0;
    int pattern = 0;

    nArg = sscanf(buf, "%d", &pattern);
    if (1 != nArg)
    {
        pr_err("[%s] invalid argument\n", __FUNCTION__);
        pr_err("usage:\n");
        pr_err("echo [pattern] > cts_pattern\n");
        pr_err("pattern= 0 --> HFTP\n");
        pr_err("pattern= 1 --> MFTP\n");
        pr_err("pattern= 2 --> LFTP\n");
        pr_err("pattern= 3 --> LBP\n");
        pr_err("pattern= 4 --> SSOP\n");

        goto out;
    }

    if (!((pattern >= 0) && (pattern <= 4)))
    {
        pr_err("%s , not correct pattern %d\n", __func__, pattern);
        goto out;
    }

    phy_adj0.cts_pattern     = pattern;
    phy_adj0.new_cts_pattern = 1;

    hal_sata_set_cts_pattern(0, &phy_adj0);

    pr_err("\n\nnow cts patern is \n");

    if (phy_adj0.cts_pattern == 0)
    {
        pr_err("HFTP\n");
    }
    else if (phy_adj0.cts_pattern == 1)
    {
        pr_err("MFTP\n");
    }
    else if (phy_adj0.cts_pattern == 2)
    {
        pr_err("LFTP\n");
    }
    else if (phy_adj0.cts_pattern == 3)
    {
        pr_err("LBP\n");
    }
    else
    {
        pr_err("SSOP\n");
    }
out:
    return count;
}

static ssize_t cts_pattern_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str     = buf;
    unsigned int nStrLen = PAGE_SIZE;

    if (phy_adj0.new_cts_pattern == 0)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "no set new pattern, using default setting\n");
        goto out;
    }

    if (phy_adj0.cts_pattern == 0)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now pattern is HFTP\n");
    }
    else if (phy_adj0.cts_pattern == 1)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now pattern is MFTP\n");
    }
    else if (phy_adj0.cts_pattern == 2)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now pattern is LFTP\n");
    }
    else if (phy_adj0.cts_pattern == 3)
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now pattern is LBP\n");
    }
    else
    {
        str += CamOsSnprintf(str, nStrLen - (int)(str - buf), "now pattern is SSOP\n");
    }

    str +=
        CamOsSnprintf(str, nStrLen - (int)(str - buf), "gen now is %s \n",
                      (phy_adj0.test_gen == 3) ? ("6.0Gbps") : ((phy_adj0.test_gen == 2) ? ("3.0Gbps") : ("1.5Gbps")));

out:
    pr_err("usage:\n");
    pr_err("echo [pattern] > cts_pattern\n");
    pr_err("pattern= 0 --> HFTP\n");
    pr_err("pattern= 1 --> MFTP\n");
    pr_err("pattern= 2 --> LFTP\n");
    pr_err("pattern= 3 --> LBP\n");
    pr_err("pattern= 4 --> SSOP\n");

    return str - buf;
}
DEVICE_ATTR(cts_pattern, 0644, cts_pattern_show, cts_pattern_store);

static ssize_t test_gen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int nArg     = 0;
    int test_gen = 0;

    nArg = sscanf(buf, "%d", &test_gen);
    if (1 != nArg)
    {
        pr_err("[%s] invalid argument\n", __FUNCTION__);
        pr_err("usage:\n");
        pr_err("echo [test_gen] > test_gen\n");
        pr_err("test gen= 0 --> pattern gen output 1.5Gbps\n");
        pr_err("test gen= 1 --> pattern gen output 3.0Gbps\n");
        pr_err("test gen= 2 --> pattern gen output 6.0Gbps\n");
        goto out;
    }

    if (test_gen != 0 && test_gen != 1 && test_gen != 2)
    {
        pr_err("%s , not correct test_gen %d\n", __func__, test_gen);
        goto out;
    }
    phy_adj0.test_gen = test_gen;
    hal_sata_set_testGen(0, phy_adj0.test_gen);
out:
    return count;
}

static ssize_t test_gen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *       str     = buf;
    unsigned int nStrLen = PAGE_SIZE;

    str +=
        CamOsSnprintf(str, nStrLen - (int)(str - buf), "test_gen now is %s \n",
                      (phy_adj0.test_gen == 2) ? ("6.0Gbps") : ((phy_adj0.test_gen == 1) ? ("3.0Gbps") : ("1.5Gbps")));

    pr_err("usage:\n");
    pr_err("echo [test_gen] > test_gen\n");
    pr_err("test gen= 0 --> pattern gen output 1.5Gbps\n");
    pr_err("test gen= 1 --> pattern gen output 3.0Gbps\n");
    pr_err("test gen= 2 --> pattern gen output 6.0Gbps\n");

    return str - buf;
}
DEVICE_ATTR(test_gen, 0644, test_gen_show, test_gen_store);

static SIMPLE_DEV_PM_OPS(ahci_sstar_pm_ops, sstar_ahci_suspend, sstar_ahci_resume);

static const struct of_device_id ahci_of_match[] = {
    //	{ .compatible = "generic-ahci", },
    {
        .compatible = "sstar,sata",
    },
    /* Keep the following compatibles for device tree compatibility */
    {},
};

MODULE_DEVICE_TABLE(of, ahci_of_match);

static int ahci_sstar_probe(struct platform_device *pdev)
{
    const struct of_device_id *of_id;
    struct device *            dev = &pdev->dev;
    struct sstar_ahci_priv *   priv;
    struct ahci_host_priv *    hpriv;
    struct resource *          res;
    // int                        phy_mode = 2;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
    {
        printk("%s , memory error\n", __func__);
        return -ENOMEM;
    }

    of_id = of_match_node(ahci_of_match, pdev->dev.of_node);
    if (!of_id)
    {
        printk("%s , dts error\n", __func__);
        return -ENODEV;
    }
    priv->dev = dev;

    memset(&phy_adj0, 0, sizeof(phy_adj0));

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ahci_port0");
    // pr_err("%s , res->start = 0x%llX , res->end = 0x%llX\n", __func__ , res->start, res->end );
    //    priv->res_ahci_port0 = devm_ioremap_resource(dev, res);

#ifdef CONFIG_ARM64
    priv->res_ahci_port0 = (void *)(res->start + MS_IO_OFFSET);
#else
    priv->res_ahci_port0 = (void *)((u32)(res->start + MS_IO_OFFSET));
#endif

    if (IS_ERR(priv->res_ahci_port0))
        return PTR_ERR(priv->res_ahci_port0);

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ahci_misc");
    // pr_err("%s , res->start = 0x%llX , res->end = 0x%llX\n", __func__ , res->start, res->end );
    //    priv->res_ahci_misc = devm_ioremap_resource(dev, res);

#ifdef CONFIG_ARM64
    priv->res_ahci_misc = (void *)(res->start + MS_IO_OFFSET);
#else
    priv->res_ahci_misc = (void *)((u32)(res->start + MS_IO_OFFSET));
#endif
    if (IS_ERR(priv->res_ahci_misc))
        return PTR_ERR(priv->res_ahci_misc);

    priv->tx_gen1_gen2_vpp[0] = priv->tx_gen1_gen2_vpp[1] = 0xFF;
    priv->tx_gen3_de_emphasis[0] = priv->tx_gen3_de_emphasis[1] = priv->tx_gen3_de_emphasis[2] = 0xFF;

    // priv->rx_eq_cs[0] = priv->rx_eq_cs[1] = priv->rx_eq_cs[2] = 0xFF;
    priv->rx_eq_ctle   = 0xFF;
    priv->rx_eq_rs1[0] = priv->rx_eq_rs1[1] = priv->rx_eq_rs1[2] = 0xFF;
    priv->rx_eq_rs2[0] = priv->rx_eq_rs2[1] = priv->rx_eq_rs2[2] = 0xFF;
    __ss_sata_get_phy_mode(priv);

    phy_adj0.test_gen = 0;

    // pr_info("[%s] , port_base =0x%08lX \n", __func__ , (unsigned long) priv->res_ahci_port0 );   //  0xFD346900
    // pr_info("[%s] , misc_base =0x%08lX \n", __func__ , (unsigned long) priv->res_ahci_misc );   // 0xFD287400
    ss_sata_init(priv, priv->phy_mode, n_ports);

    hpriv = ahci_platform_get_resources(pdev, 0); // linux 4.19 changed the parameters of ahci_platform_get_resources()
                                                  // , we only need use "0" to apply this new change .
    if (IS_ERR(hpriv))
        return PTR_ERR(hpriv);

    hpriv->plat_data = priv;
    hpriv->flags |= AHCI_HFLAG_YES_FBS; // hpriv->flags doest not match the  struct ata_port_info parameter . we need
                                        // set the flags paramter before ahci_platform_init_host();
    ss_sata_applySetting(0, hpriv);
    // pr_info("[%s] , line = %d \n", __func__, __LINE__);

    ss_sata_applyDmhs(0, priv, &phy_adj0);

    ret = ahci_platform_enable_resources(hpriv);
    if (ret)
        return ret;

    ret = ahci_platform_init_host(pdev, hpriv, &ahci_port_info, &ahci_platform_sht);
    if (ret)
    {
        goto disable_resources;
    }

    if (alloc_chrdev_region(&priv->dev_no, 0, 1, "sstar_sata") == 0)
    {
        int Major           = MAJOR(priv->dev_no);
        priv->attribute_dev = MKDEV(Major, 0);
        priv->sysfs_dev     = device_create(msys_get_sysfs_class(), NULL, priv->attribute_dev, NULL, "sstar_sata");
    } /*replace device_create_file -> CamDeviceCreateFile*/
    device_create_file(priv->sysfs_dev, &dev_attr_sata_dump);

    device_create_file(priv->sysfs_dev, &dev_attr_de_emphasis);
    device_create_file(priv->sysfs_dev, &dev_attr_cts_pattern);
    device_create_file(priv->sysfs_dev, &dev_attr_test_gen);

    pr_info("[%s] , SSTAR AHCI SATA registered \n", __func__);

    return 0;

disable_resources:
    ahci_platform_disable_resources(hpriv);
    return ret;
}

#ifdef CONFIG_ARM64
static const struct acpi_device_id ahci_acpi_match[] = {
    {ACPI_DEVICE_CLASS(PCI_CLASS_STORAGE_SATA_AHCI, 0xffffffffffffffff)},
    {},
};

#else
static const struct acpi_device_id ahci_acpi_match[] = {
    {ACPI_DEVICE_CLASS(PCI_CLASS_STORAGE_SATA_AHCI, 0xFFFFFFFF)},
    {},
};

#endif

MODULE_DEVICE_TABLE(acpi, ahci_acpi_match);

static int sstar_ahci_remove(struct platform_device *pdev)
{
    struct ata_host *       host  = dev_get_drvdata(&pdev->dev);
    struct ahci_host_priv * hpriv = host->private_data;
    struct sstar_ahci_priv *priv  = hpriv->plat_data;
    int                     ret;

    pr_warn("[%s] , remove\n", __func__);

    device_remove_file(priv->sysfs_dev, &dev_attr_sata_dump);
    device_remove_file(priv->sysfs_dev, &dev_attr_de_emphasis);
    device_remove_file(priv->sysfs_dev, &dev_attr_cts_pattern);
    device_remove_file(priv->sysfs_dev, &dev_attr_test_gen);
    device_destroy(msys_get_sysfs_class(), priv->attribute_dev);
    unregister_chrdev_region(priv->dev_no, 1);

    ret = ata_platform_remove_one(pdev);
    if (ret)
        return ret;

    pr_warn("[%s] , remove down\n", __func__);

    return 0;
}

static struct platform_driver ahci_driver = {
    .probe  = ahci_sstar_probe,
    .remove = sstar_ahci_remove,
    .driver =
        {
            .name             = DRV_NAME,
            .of_match_table   = ahci_of_match,
            .acpi_match_table = ahci_acpi_match,
            .pm               = &ahci_sstar_pm_ops,
        },
};
module_platform_driver(ahci_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sstar Semiconductor");
MODULE_DESCRIPTION("SSTAR AHCI SATA platform driver");
