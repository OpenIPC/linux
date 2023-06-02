/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include "ahci.h"
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <mach/platform/board.h>
#ifdef CONFIG_PLATFORM_GM8210
#include <mach/fmem.h>
#include <mach/ftpmu010_pcie.h>
#endif
#ifdef CONFIG_PLATFORM_GM8287
#include <mach/ftpmu010.h>
#endif


#define DRV_NAME	"FTSATA100_AHCI"
#define DRV_VERSION	"0.15"

#define SATA_PHY_CLK    30000000

#define ENABLE_INTERNAL_RESISTER    // only valid on GM8287 series
#define ENABLE_NCQ_CAP              // turn on NCQ capibility

#define SATAPHY_CFG_ISATA       0x11
#define SATAPHY_CFG_ESATA       0x22
#define FTSATA100_SET_PHY_SATA0 0x33
#define FTSATA100_SET_PHY_SATA1 0x44
#define FTSATA100_SET_PHY_SATA2 0x55
#define FTSATA100_SET_PHY_SATA3 0x66

#define MAX_SATAC_NUM	4
#define MAX_NAME_SZ		10

#ifdef CONFIG_PLATFORM_GM8210
static void __iomem *h2x_sata0_va_base;
static void __iomem *h2x_sata1_va_base;
static void __iomem *h2x_sata2_va_base;
static void __iomem *h2x_sata3_va_base;
static void __iomem *pcie_va_base;
static void __iomem *i2c0_sataphy_va_base;
static void __iomem *i2c1_sataphy_va_base;
static void __iomem *i2c2_sataphy_va_base;
static void __iomem *i2c3_sataphy_va_base;
#endif
#ifdef CONFIG_PLATFORM_GM8287
static void __iomem *i2c3_sataphy_va_base;
static void __iomem *i2c4_sataphy_va_base;
#endif

static u8 i2c_sladdr_r, i2c_sladdr_w;
static u32 plat_id;

struct proc_dir_entry *p_root = NULL;
static struct proc_dir_entry *p_host[MAX_SATAC_NUM];
static struct proc_dir_entry *p_phy_i2c_reg[MAX_SATAC_NUM];

#ifdef CONFIG_PLATFORM_GM8210
static fmem_pci_id_t pci_id;
static fmem_cpu_id_t cpu_id;

#ifdef CONFIG_EP_SATA_ENABLE
static void __iomem *slave_8210_pmu_va_base;
struct s_ftsata100_host {
    spinlock_t      lock;
    //struct device   *dev;
    void __iomem    *mmio;
    //unsigned int    n_ports;
    //void            *private_data;
    //unsigned long   flags;
    unsigned int    id;
} ftsata100_host[MAX_SATAC_NUM];
#endif
#endif

static struct s_host_port {
	unsigned int	id;
	unsigned char	name[MAX_NAME_SZ];
	void __iomem	*mmio_base;
} host_port[MAX_SATAC_NUM];

enum {
	PLAT_8312_ID    = 0x831200,
    PLAT_8287_ID    = 0x828710,
	PLAT_END_ID     = 0xffffff
};

enum {
	SPEED_GEN1	= 1,
	SPEED_GEN2	= 2,
	SPEED_GNE3	= 3
};

enum ahci_type {
	AHCI,		/* standard platform ahci */
	IMX53_AHCI,	/* ahci on i.mx53 */
	FTSATA100_AHCI,  /* ahci on FTSATA100 */
};

static struct platform_device_id ahci_devtype[] = {
	{
		.name = "ahci",
		.driver_data = AHCI,
	}, {
		.name = "imx53-ahci",
		.driver_data = IMX53_AHCI,
	}, {
		.name = "ftsata100",
		.driver_data = FTSATA100_AHCI,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, ahci_devtype);

static const struct ata_port_info ahci_port_info[] = {
	/* by features */
	[AHCI] = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_ops,
	},
	[IMX53_AHCI] = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_pmp_retry_srst_ops,
	},
	[FTSATA100_AHCI] = {
#ifdef ENABLE_NCQ_CAP
		.private_data	= (void *)(AHCI_HFLAG_YES_NCQ),
#else
		.private_data   = (void *)(AHCI_HFLAG_NO_NCQ),
#endif
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_pmp_retry_srst_ops,
	},
};

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT("ahci_platform"),
};

void ftsata100_dump_reg(struct ata_port *ap)
{
	struct ahci_host_priv *hpriv = ap->host->private_data;
	void __iomem *mmio = hpriv->mmio;
	struct ahci_port_priv *pp = ap->private_data;
	struct ahci_sg *ftsata100_sg;
	u32 *pfis, i, rec_n_sg;

	printk(KERN_WARNING "ata%u: dump register contents\n", ap->print_id);
	printk("   CAP(0x00)  = %#x\n", readl(mmio+0x00));
	printk("    IS(0x08)  = %#x\n", readl(mmio+0x08));
	printk("    PI(0x0C)  = %#x\n", readl(mmio+0x0c));
	printk("TCOUNT(0xA0)  = %#x\n", readl(mmio+0xa0));
	printk("   FEA(0xA4)  = %#x\n", readl(mmio+0xa4));
	printk(" P0CLB(0x100) = %#x\n", readl(mmio+0x100));
	printk("  P0FB(0x108) = %#x\n", readl(mmio+0x108));
	printk("  P0IS(0x110) = %#x\n", readl(mmio+0x110));
	printk("  P0IE(0x114) = %#x\n", readl(mmio+0x114));
	printk(" P0CMD(0x118) = %#x\n", readl(mmio+0x118));
	printk(" P0TFD(0x120) = %#x\n", readl(mmio+0x120));
	printk("P0SSTS(0x128) = %#x\n", readl(mmio+0x128));
	printk("P0SCTL(0x12C) = %#x\n", readl(mmio+0x12c));
	printk("P0SERR(0x130) = %#x\n", readl(mmio+0x130));
	printk("P0SACT(0x134) = %#x\n", readl(mmio+0x134));
	printk("  P0CI(0x138) = %#x\n", readl(mmio+0x138));
	printk(" P0FCR(0x170) = %#x\n", readl(mmio+0x170));
	printk("P0BIST(0x174) = %#x\n", readl(mmio+0x174));
	printk(" P0FSM(0x17C) = %#x\n", readl(mmio+0x17c));
	printk("\n");

	printk(KERN_WARNING "ata%u: dump command header 0\n", ap->print_id);
	printk("DW0 = %#x\n", pp->cmd_slot[0].opts);
	printk("DW1 = %#x\n", pp->cmd_slot[0].status);
	printk("DW2 = %#x\n", pp->cmd_slot[0].tbl_addr);
	printk("DW3 = %#x\n", pp->cmd_slot[0].tbl_addr_hi);
	printk("\n");

    rec_n_sg = (pp->cmd_slot[0].opts >> 16) & 0xFFFF;

	printk(KERN_WARNING "ata%u: dump command table 0\n", ap->print_id);
	pfis = (u32 *) pp->cmd_tbl;
	printk("DW0 = %#x\n", *(pfis+0));
	printk("DW1 = %#x\n", *(pfis+1));
	printk("DW2 = %#x\n", *(pfis+2));
	printk("DW3 = %#x\n", *(pfis+3));
	printk("DW4 = %#x\n", *(pfis+4));
	printk("\n");

	printk(KERN_WARNING "ata%u: dump PRD table\n", ap->print_id);
	ftsata100_sg = pp->cmd_tbl + AHCI_CMD_TBL_HDR_SZ;
	for (i=0; i<rec_n_sg; i++) {
		printk("-- item %d --\n", i);
		printk("DW0 = %#x\n", ftsata100_sg->addr);
		printk("DW1 = %#x\n", ftsata100_sg->addr_hi);
		printk("DW3 = %#x\n", ftsata100_sg->flags_size);
		ftsata100_sg++;
	}
	printk("\n");

}

#define I2C_ALIEN                   0x2000  /* Arbitration lose */
#define I2C_SAMIEN                  0x1000  /* slave address match */
#define I2C_STOPIEN                 0x800   /* stop condition */
#define I2C_BERRIEN                 0x400   /* non ACK response */
#define I2C_DRIEN                   0x200   /* data receive */
#define I2C_DTIEN                   0x100   /* data transmit */
#define I2C_TBEN                    0x80    /* transfer byte enable */
#define I2C_ACKNAK                  0x40    /* ack sent */
#define I2C_STOP                    0x20    /* stop */
#define I2C_START                   0x10    /* start */
#define I2C_GCEN                    0x8     /* general call */
#define I2C_SCLEN                   0x4     /* enable clock */
#define I2C_I2CEN                   0x2     /* enable I2C */
#define I2C_I2CRST                  0x1     /* reset I2C */
#define I2C_ENABLE                  (I2C_ALIEN|I2C_SAMIEN|I2C_STOPIEN|I2C_BERRIEN|I2C_DRIEN|I2C_DTIEN|I2C_SCLEN|I2C_I2CEN)

static inline s8 i2c_do_read(void __iomem *i2c_base, u8 *data, u32 ctrl)
{
    u32 i;

    writel(ctrl, i2c_base + 0x00);

    for (i = 0; i < 100; i++) {
        if (readl(i2c_base + 0x04) & 0x20) {
            *data = (u8) readl(i2c_base + 0x0C);
            return 0;
        }
        udelay(10);
    }
    printk("%s: timeouted on rx data\n", __func__);
    return -1; /* check status timeouted */
}

static inline s8 i2c_do_write(void __iomem *i2c_base, u8 data, u32 ctrl)
{
    u32 i;

    writel(data, i2c_base + 0x0C);
    writel(ctrl, i2c_base + 0x00);

    for (i = 0; i < 100; i++) {
        if (readl(i2c_base + 0x04) & 0x10)
            return 0;
        udelay(10);
    }
    printk("%s: timeouted on tx data\n", __func__);
    return -1; /* check status timeouted */
}

static inline s8 i2c_read_byte(void __iomem *i2c_base, u8 addr, u8 idx, u8 *data)
{
    if (i2c_do_write(i2c_base, addr&0xFE, I2C_ENABLE|I2C_TBEN|I2C_START) < 0)
        return -1;
    if (i2c_do_write(i2c_base, idx, I2C_ENABLE|I2C_TBEN) < 0)
        return -1;
    if (i2c_do_write(i2c_base, addr, I2C_ENABLE|I2C_TBEN|I2C_START) < 0)
        return -1;
    if (i2c_do_read(i2c_base, data, I2C_ENABLE|I2C_TBEN|I2C_STOP|I2C_ACKNAK) < 0)
        return -1;

    return 0;
}

static inline s8 i2c_write_byte(void __iomem *i2c_base, u8 addr, u8 idx, u8 data)
{
    if (i2c_do_write(i2c_base, addr, I2C_ENABLE|I2C_TBEN|I2C_START) < 0)
        return -1;
    if (i2c_do_write(i2c_base, idx, I2C_ENABLE|I2C_TBEN) < 0)
        return -1;
    if (i2c_do_write(i2c_base, data, I2C_ENABLE|I2C_TBEN|I2C_STOP) < 0)
        return -1;

    return 0;
}

static int proc_read_phy_i2c_reg(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    unsigned int len = 0;

    len += sprintf(page+len,
            "Usage :\n"
            "    echo reg_offset [data] > phy_i2c_reg\n"
            "Parameter :\n"
            "    reg_offset = minor address to access on phy (hex)\n"
            "          data = data to write, if specified (hex)\n"
            );

    return len;
}

static int proc_write_phy_i2c_reg(struct file *file, const char *buffer,unsigned long count, void *data)
{
    unsigned int ret = 0, regoff = 0, val = 0;
    unsigned char value[20];
    struct s_host_port *pdev_info = (struct s_host_port *) data;
    void __iomem *i2c_base;

    if (copy_from_user(value, buffer, count))
        return -EFAULT;

    value[count] = '\0';
    ret = sscanf(value, "%x %x\n", &regoff, &val);

#ifdef CONFIG_PLATFORM_GM8210
    switch (pdev_info->id) {
        case 0:
            i2c_base = i2c0_sataphy_va_base;
            break;
        case 1:
            i2c_base = i2c1_sataphy_va_base;
            break;
        case 2:
            i2c_base = i2c2_sataphy_va_base;
            break;
        case 3:
            i2c_base = i2c3_sataphy_va_base;
            break;
        default:
            BUG();
    }
#endif

#ifdef CONFIG_PLATFORM_GM8287
    switch (pdev_info->id) {
        case 0:
            i2c_base = i2c3_sataphy_va_base;
            break;
        case 1:
            i2c_base = i2c4_sataphy_va_base;
            break;
        default:
            BUG();
    }
#endif

    if (ret == 1) {
        i2c_read_byte(i2c_base, i2c_sladdr_r, regoff, (u8 *) &val);
        printk("%#x\n", val);
    } else if (ret == 2) {
        i2c_write_byte(i2c_base, i2c_sladdr_w, regoff, (u8) val);
    }

    return count;
}

static void ftsata100_proc_init(struct ata_port *ap)
{
    unsigned int id = ap->print_id - 1;

    memset(&host_port[id], 0, sizeof(host_port[id]));

    host_port[id].id = id;
    snprintf(host_port[id].name, MAX_NAME_SZ, "%s%d", "host", id);
    host_port[id].mmio_base = ahci_port_base(ap);

    if (p_root == NULL) {
        p_root = create_proc_entry("ftsata", S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    }

    p_host[id] = create_proc_entry(host_port[id].name, S_IFDIR | S_IRUGO | S_IXUGO, p_root);

    p_phy_i2c_reg[id] = create_proc_entry("phy_i2c_reg", S_IRUGO | S_IWUGO, p_host[id]);
    p_phy_i2c_reg[id]->read_proc = (read_proc_t *) proc_read_phy_i2c_reg;
    p_phy_i2c_reg[id]->write_proc = (write_proc_t *) proc_write_phy_i2c_reg;
    p_phy_i2c_reg[id]->data = (void *) &host_port[id];
}

static void ftsata100_proc_remove(struct ata_port *ap)
{
    unsigned int id = ap->print_id - 1;

    if (p_host[id]) {
        if (p_phy_i2c_reg[id])
            remove_proc_entry(p_phy_i2c_reg[id]->name, p_host[id]);

        remove_proc_entry(p_host[id]->name, p_root);
        p_host[id] = NULL;
    }

    //if ((p_host[0]==NULL) && (p_host[1]==NULL))
    //	remove_proc_entry(p_root->name, NULL);
}

static inline void ftsata100_phy_parameter_setting(u32 satac_id, u32 phy_cfg)
{
    void __iomem *i2c_base;
    u32 c_id;
	u8 t_idx, t_val;

#ifdef CONFIG_PLATFORM_GM8210
    switch (satac_id) {
        case FTSATA100_SET_PHY_SATA0:
            c_id = 0;
            i2c_base = i2c0_sataphy_va_base;
            break;
        case FTSATA100_SET_PHY_SATA1:
            c_id = 1;
            i2c_base = i2c1_sataphy_va_base;
            break;
        case FTSATA100_SET_PHY_SATA2:
            c_id = 2;
            i2c_base = i2c2_sataphy_va_base;
            break;
        case FTSATA100_SET_PHY_SATA3:
            c_id = 3;
            i2c_base = i2c3_sataphy_va_base;
            break;
    }
#endif

#ifdef CONFIG_PLATFORM_GM8287
        switch (satac_id) {
            case FTSATA100_SET_PHY_SATA0:
                c_id = 0;
                i2c_base = i2c3_sataphy_va_base;
                break;
            case FTSATA100_SET_PHY_SATA1:
                c_id = 1;
                i2c_base = i2c4_sataphy_va_base;
                break;
        }
#endif

	printk("GM SATA%u config %s PHY setting\n", c_id,
            (phy_cfg == SATAPHY_CFG_ISATA ? "iSATA" : "eSATA"));

    switch (plat_id) {
        case PLAT_8312_ID:
            switch (phy_cfg) {
                case SATAPHY_CFG_ISATA:
                    /* parameter 1 */
                    t_idx = 0x03;
                    t_val = 0x40;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
                    break;
                case SATAPHY_CFG_ESATA:
                    /* parameter 1 */
                    t_idx = 0x03;
                    t_val = 0x40;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
                    break;
            }
            break;
        case PLAT_8287_ID:
            switch (phy_cfg) {
                case SATAPHY_CFG_ISATA:
#ifdef ENABLE_INTERNAL_RESISTER
                    /* config setting of internal resister */
                    /* parameter 1 */
                    t_idx = 0x07;
                    t_val = 0x09;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
                    /* parameter 2 */
                    t_idx = 0x19;
                    t_val = 0x0C;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
                    /* parameter 3 */
                    t_idx = 0x35;
                    t_val = 0x10;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
                    /* parameter 4 */
                    t_idx = 0x36;
                    t_val = 0x80;
                    if (i2c_write_byte(i2c_base, i2c_sladdr_w, t_idx, t_val) < 0)
                        goto get_exit;
                    printk("GM SATA%u PHY program i2c register offset 0x%02x, value=0x%02x\n",
                            c_id, t_idx, t_val);
#endif
                    break;
                case SATAPHY_CFG_ESATA:
                    /* use default value */
                    break;
            }
            break;
        default:
            printk("GM SATA >>> unknow platform id %#x\n", plat_id);
            BUG();
            break;
    }

get_exit:
	return;
}

static inline void ftsata100_phy_config(void)
{
    u32 i2c_clock_off = 0, i2c_gsr = 0x02, i2c_tsr = 0x27, i2c_count;
    u32 scl_clock = 120*1000;

#ifdef CONFIG_PLATFORM_GM8210
    i2c_clock_off = (readl(PCIPMU_FTPMU010_VA_BASE + 0x34) >> 19) & 0xF;

    if (i2c_clock_off) {
        /* turn on I2C clock */
        writel((readl(PCIPMU_FTPMU010_VA_BASE + 0x34) & ~(0xF << 19)), PCIPMU_FTPMU010_VA_BASE + 0x34);
        mdelay(5);
        i2c_clock_off = 0;
    }

    /* config TGSR register of I2C_0 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c0_sataphy_va_base + 0x14);
    /* config TGSR register of I2C_1 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c1_sataphy_va_base + 0x14);
    /* config TGSR register of I2C_2 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c2_sataphy_va_base + 0x14);
    /* config TGSR register of I2C_3 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c3_sataphy_va_base + 0x14);

    /* compute I2C clock division */
    i2c_count = (((50000000)/scl_clock)-i2c_gsr)/2-2;
    //printk("--> i2c_count = %d\n", i2c_count);

    /* config CDR register of I2C_0 */
    writel(i2c_count, i2c0_sataphy_va_base + 0x08);
    /* config CDR register of I2C_1 */
    writel(i2c_count, i2c1_sataphy_va_base + 0x08);
    /* config CDR register of I2C_2 */
    writel(i2c_count, i2c2_sataphy_va_base + 0x08);
    /* config CDR register of I2C_3 */
    writel(i2c_count, i2c3_sataphy_va_base + 0x08);

    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA0, SATAPHY_CFG_ISATA);
    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA1, SATAPHY_CFG_ISATA);
    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA2, SATAPHY_CFG_ISATA);
    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA3, SATAPHY_CFG_ISATA);
#endif //#ifdef CONFIG_PLATFORM_GM8210

#ifdef CONFIG_PLATFORM_GM8287
    i2c_clock_off = (readl(PMU_FTPMU010_VA_BASE + 0xBC) >> 14) & 0x3;

    if (i2c_clock_off) {
        /* turn on I2C clock */
        writel((readl(PMU_FTPMU010_VA_BASE + 0xBC) & ~(0x3 << 14)), PMU_FTPMU010_VA_BASE + 0xBC);
        mdelay(5);
        i2c_clock_off = 0;
    }

    /* config TGSR register of I2C_3 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c3_sataphy_va_base + 0x14);
    /* config TGSR register of I2C_4 */
    writel((i2c_gsr << 10) | i2c_tsr, i2c4_sataphy_va_base + 0x14);

    /* compute I2C clock division */
    i2c_count = (((50000000)/scl_clock)-i2c_gsr)/2-2;
    //printk("--> i2c_count = %d\n", i2c_count);

    /* config CDR register of I2C_3 */
    writel(i2c_count, i2c3_sataphy_va_base + 0x08);
    /* config CDR register of I2C_4 */
    writel(i2c_count, i2c4_sataphy_va_base + 0x08);

    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA0, SATAPHY_CFG_ISATA);
    ftsata100_phy_parameter_setting(FTSATA100_SET_PHY_SATA1, SATAPHY_CFG_ISATA);
#endif //#ifdef CONFIG_PLATFORM_GM8287

#if 0
    if (!i2c_clock_off) {
        /* turn off I2C clock */
        writel((readl(PMU_FTPMU010_0_VA_BASE + 0x3C) | (1 << 22)), PMU_FTPMU010_0_VA_BASE + 0x3C);
        mdelay(5);
    }
#endif
}

int ftsata100_phy_reset(struct ata_link *link)
{
    u32 sstatus, scontrol;
    int rc = -1;

    sata_scr_read(link, SCR_STATUS, &sstatus);

    /* if device is detected, but PHY communication not established, wait for a while */
    if ((sstatus & 0xF) == 1) {
        u32 i, max_tried = 3000;
        struct ata_port *ap = link->ap;
        //void __iomem *port_mmio = ahci_port_base(ap);
        extern int sata_down_spd_limit(struct ata_link *link, u32 spd_limit);
        extern int sata_set_spd(struct ata_link *link);

        rc = 0;
#if 1
		// limit the highest allowable speed to 1.5 Gbps
		printk("GM SATA%u limit highest allowable speed to 1.5 Gbps\n", ap->print_id - 1);
        sata_scr_read(link, SCR_CONTROL, &scontrol);
		scontrol |= 0x10;
        sata_scr_write_flush(link, SCR_CONTROL, scontrol);
#else
        // down speed
        if (sata_down_spd_limit(link, 0) == 0)
            sata_set_spd(link);
#endif

        for (i = 0; i < max_tried; i++) {
            sata_scr_read(link, SCR_CONTROL, &scontrol);
            scontrol &= ~0x0F;
            scontrol |= 0x04;
            sata_scr_write_flush(link, SCR_CONTROL, scontrol);
            ata_msleep(ap, 1);
            sata_scr_read(link, SCR_CONTROL, &scontrol);
            scontrol &= ~0x0F;
            sata_scr_write_flush(link, SCR_CONTROL, scontrol);
            ata_msleep(ap, 3);
            if (ata_link_online(link)) {
                link->sata_spd_limit = 1;
                break;
            }
        }

        printk(">>>>>> GM SATA%u: retry POR reset %d times%s\n",
                ap->print_id-1, i, (i >= max_tried)?", all failed":"");
    }

    return rc;
}
EXPORT_SYMBOL_GPL(ftsata100_phy_reset);

void ftsata100_clk_enable(void)
{
#ifdef CONFIG_PLATFORM_GM8210
    u32 pllout_8312;
#endif
#ifdef CONFIG_PLATFORM_GM8287
    u32 pll5out;
#endif
    u32 val, sataphy_clk, sataphy_pvalue;

#ifdef CONFIG_PLATFORM_GM8210
    pllout_8312 = PCIE_PLL1_CLK_IN;

#ifdef CONFIG_EXTERNAL_CRYSTAL_CLOCK // select external crystal as reference clock input
    /* set GM8312 SATA PHY clock source to external */
    printk("GM SATAPHY clock source : SATA0==SATA1, SATA1==SATA2, SATA2==SATA3, SATA3==XTAL\n");
    val = readl(PCIPMU_FTPMU010_VA_BASE + 0x80);
    val &= ~(1 << 29);
    writel(val, PCIPMU_FTPMU010_VA_BASE + 0x80);
    sataphy_pvalue = 0; // unused variable
    sataphy_clk = SATA_PHY_CLK;
#else
    /* in GM8312, SATA0/1/2/3 PHY clock source default seting is internal */
    printk("GM SATAPHY clock source : SATA0==PLL, SATA1==SATA0, SATA2==SATA1, SATA3==SATA2\n");

    /* SATA PHY clock pvalue */
    sataphy_pvalue = (pllout_8312 / 2 / SATA_PHY_CLK) - 1;
    val = readl(PCIPMU_FTPMU010_VA_BASE + 0x30);
    val = (val & ~(0x1F << 18)) | (sataphy_pvalue << 18);
    writel(val, PCIPMU_FTPMU010_VA_BASE + 0x30);
    val = readl(PCIPMU_FTPMU010_VA_BASE + 0x30);
    sataphy_pvalue = ( val >> 18) & 0x1F;
    sataphy_clk = pllout_8312 / 2 / (sataphy_pvalue + 1);
#endif

    printk(KERN_INFO "GM SATAPHY Clock = %u MHz\n", sataphy_clk / 1000000); // default value in GM8312

    // turn on AHBC clk
    writel((readl(PCIPMU_FTPMU010_VA_BASE + 0x30) & ~(1 << 10)), PCIPMU_FTPMU010_VA_BASE + 0x30);
    // turn on INTC clk
    writel((readl(PCIPMU_FTPMU010_VA_BASE + 0x34) & ~(1 << 8)), PCIPMU_FTPMU010_VA_BASE + 0x34);
    // turn on SATA HCLK
    writel((readl(PCIPMU_FTPMU010_VA_BASE + 0x30) & ~0xF), PCIPMU_FTPMU010_VA_BASE + 0x30);
    mdelay(5);
    /* SATA PHY PORB & 30MHz */
    writel((readl(PCIPMU_FTPMU010_VA_BASE + 0x80) & ~(1 << 7)) | 0x40404040, PCIPMU_FTPMU010_VA_BASE + 0x80);
    mdelay(5);

    /* change H2X write data queue control and prefetch number */
    writel((readl(h2x_sata0_va_base) & ~(0x3 << 2)) | (0x1 << 2), h2x_sata0_va_base);
    writel((readl(h2x_sata1_va_base) & ~(0x3 << 2)) | (0x1 << 2), h2x_sata1_va_base);
    writel((readl(h2x_sata2_va_base) & ~(0x3 << 2)) | (0x1 << 2), h2x_sata2_va_base);
    writel((readl(h2x_sata3_va_base) & ~(0x3 << 2)) | (0x1 << 2), h2x_sata3_va_base);
#endif //#ifdef CONFIG_PLATFORM_GM8210

#ifdef CONFIG_PLATFORM_GM8287
    pll5out = PLL5_CLK_IN;

    /* SATA0/1 PHY clock source default seting is external */
    printk("GM SATAPHY clock source : SATA0==SATA1, SATA1==PLL\n");
    val = readl(PMU_FTPMU010_VA_BASE + 0xD4);
    val |= (1 << 2);
    /* SATA PHY clock pvalue */
    sataphy_pvalue = (pll5out / SATA_PHY_CLK) - 1;
    val = (val & ~(0x3F << 13)) | (sataphy_pvalue << 13);
    writel(val, PMU_FTPMU010_VA_BASE + 0xD4);
    val = readl(PMU_FTPMU010_VA_BASE + 0xD4);
    sataphy_pvalue = ( val >> 13) & 0x3F;
    sataphy_clk = pll5out / (sataphy_pvalue + 1);

    printk(KERN_INFO "GM SATAPHY Clock = %u MHz\n", sataphy_clk / 1000000);

    val = readl(PMU_FTPMU010_VA_BASE + 0xD0);
    val |= (1 << 2);
    writel(val, PMU_FTPMU010_VA_BASE + 0xD0);

    /* SATA PHY PORB & 30MHz */
    writel(readl(PMU_FTPMU010_VA_BASE + 0xD0) | (1 << 3), PMU_FTPMU010_VA_BASE + 0xD0);
    writel(readl(PMU_FTPMU010_VA_BASE + 0xD4) | (1 << 3), PMU_FTPMU010_VA_BASE + 0xD4);
    mdelay(5);
#endif //#ifdef CONFIG_PLATFORM_GM8287
}

#ifdef CONFIG_EP_SATA_ENABLE
/*
 * get hardware interrupt on 1st 8210, and then
 * trigger relative software interrupt on 2nd 8210
 */
static irqreturn_t ftsata100_interrupt_translate(int irq, void *dev_instance)
{
    struct s_ftsata100_host *ft_host = dev_instance;
    void __iomem *mmio, *port_mmio;
    u32 irq_stat;
    int sw_irq;

    mmio = ft_host->mmio;
    port_mmio = mmio + 0x100; // port no.0

    irq_stat = readl(mmio + HOST_IRQ_STAT);
    if (!irq_stat)
        return IRQ_NONE;

    switch (irq) {
        case SATA_FTSATA100_0_IRQ:
            sw_irq = CPU_INT_10;
            break;
        case SATA_FTSATA100_1_IRQ:
            sw_irq = CPU_INT_11;
            break;
        case SATA_FTSATA100_2_IRQ:
            sw_irq = CPU_INT_12;
            break;
        case SATA_FTSATA100_3_IRQ:
            sw_irq = CPU_INT_13;
            break;
        default:
            panic(">>>> %s : invalid platform device irq %d\n", __func__, irq);
    }
    //printk(">>>> GM SATA : new software interrupt %d\n", sw_irq);

	spin_lock(&ft_host->lock);
    /* turn IRQ off */
    writel(0, port_mmio + PORT_IRQ_MASK);
	writel(irq_stat, mmio + HOST_IRQ_STAT);
	spin_unlock(&ft_host->lock);

    /* trigger software interrupt on 2nd 8210 */
    writel(1 << (sw_irq - CPU_INT_BASE), slave_8210_pmu_va_base + 0xA8);

    return IRQ_HANDLED;
}
#endif

static int __init ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	const struct platform_device_id *id = platform_get_device_id(pdev);
	struct ata_port_info pi = ahci_port_info[id ? id->driver_data : 0];
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "no irq\n");
		return -EINVAL;
	}

	if (pdata && pdata->ata_port_info)
		pi = *pdata->ata_port_info;

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		dev_err(dev, "can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_ioremap(dev, mem->start, resource_size(mem));

	if (!hpriv->mmio) {
		dev_err(dev, "can't map %pR\n", mem);
		return -ENOMEM;
	}

#ifdef CONFIG_PLATFORM_GM8210
#ifdef CONFIG_EP_SATA_ENABLE
    if (pci_id != FMEM_PCI_HOST) {
        int xlat_irq;

        /* translate hardware irq to software irq on 2nd 8210 */
        switch (irq) {
            case SATA_FTSATA100_0_IRQ:
                xlat_irq = CPU_INT_10;
                break;
            case SATA_FTSATA100_1_IRQ:
                xlat_irq = CPU_INT_11;
                break;
            case SATA_FTSATA100_2_IRQ:
                xlat_irq = CPU_INT_12;
                break;
            case SATA_FTSATA100_3_IRQ:
                xlat_irq = CPU_INT_13;
                break;
            default:
                panic(">>>> %s : invalid platform device irq %d\n", __func__, irq);
        }

        irq = xlat_irq;
    }
#endif
#endif

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */
	if (pdata && pdata->init) {
		rc = pdata->init(dev, hpriv->mmio);
		if (rc)
			return rc;
	}

	ahci_save_initial_config(dev, hpriv,
		pdata ? pdata->force_port_map : 0,
		pdata ? pdata->mask_port_map  : 0);

	/* prepare host */
	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	/* CAP.NP sometimes indicate the index of the last enabled
	 * port, at other times, that of the last possible port, so
	 * determining the maximum port number requires looking at
	 * both CAP.NP and port_map.
	 */
	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		rc = -ENOMEM;
		goto err0;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		printk(KERN_INFO "ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		/* set enclosure management message type */
		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		/* disabled/not-implemented port */
		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_platform_sht);
	if (rc)
		goto err0;

    /* create host entry under /proc */
    ftsata100_proc_init(host->ports[0]);

	return 0;
err0:
	if (pdata && pdata->exit)
		pdata->exit(dev);
	return rc;
}

static int __devexit ahci_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ata_host *host = dev_get_drvdata(dev);

    /* remove host entry under /proc */
    ftsata100_proc_remove(host->ports[0]);

	ata_host_detach(host);

	if (pdata && pdata->exit)
		pdata->exit(dev);

	return 0;
}

#ifdef CONFIG_PM
static int ahci_suspend(struct device *dev)
{
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	void __iomem *mmio = hpriv->mmio;
	u32 ctl;
	int rc;

	if (hpriv->flags & AHCI_HFLAG_NO_SUSPEND) {
		dev_err(dev, "firmware update required for suspend/resume\n");
		return -EIO;
	}

	/*
	 * AHCI spec rev1.1 section 8.3.3:
	 * Software must disable interrupts prior to requesting a
	 * transition of the HBA to D3 state.
	 */
	ctl = readl(mmio + HOST_CTL);
	ctl &= ~HOST_IRQ_EN;
	writel(ctl, mmio + HOST_CTL);
	readl(mmio + HOST_CTL); /* flush */

	rc = ata_host_suspend(host, PMSG_SUSPEND);
	if (rc)
		return rc;

	if (pdata && pdata->suspend)
		return pdata->suspend(dev);
	return 0;
}

static int ahci_resume(struct device *dev)
{
	struct ahci_platform_data *pdata = dev_get_platdata(dev);
	struct ata_host *host = dev_get_drvdata(dev);
	int rc;

	if (pdata && pdata->resume) {
		rc = pdata->resume(dev);
		if (rc)
			return rc;
	}

	if (dev->power.power_state.event == PM_EVENT_SUSPEND) {
		rc = ahci_reset_controller(host);
		if (rc)
			return rc;

		ahci_init_controller(host);
	}

	ata_host_resume(host);

	return 0;
}

static struct dev_pm_ops ahci_pm_ops = {
	.suspend		= &ahci_suspend,
	.resume			= &ahci_resume,
};
#endif

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "calxeda,hb-ahci", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static struct platform_driver ahci_driver = {
	.remove = __devexit_p(ahci_remove),
	.driver = {
		.name = "ahci",
		.owner = THIS_MODULE,
		.of_match_table = ahci_of_match,
#ifdef CONFIG_PM
		.pm = &ahci_pm_ops,
#endif
	},
	.id_table	= ahci_devtype,
};

static int __init ahci_init(void)
{
	printk(DRV_NAME " driver version " DRV_VERSION "\n");

#ifdef CONFIG_PLATFORM_GM8210
    fmem_get_identifier(&pci_id, &cpu_id);
#ifdef CONFIG_EP_SATA_ENABLE
    if (cpu_id != FMEM_CPU_FA726)
        return 0;

    if (pci_id == FMEM_PCI_HOST) {
        int rc = 0, i, irq;
        resource_size_t mem, mem_size;

        for (i = 0; i < 4; i++) {
            struct s_ftsata100_host *ft_host = &ftsata100_host[i];

            memset(ft_host, 0, sizeof(*ft_host));

            spin_lock_init(&ft_host->lock);
            switch (i) {
                case 0:
                    mem = SATA_FTSATA100_0_PA_BASE;
                    mem_size = SATA_FTSATA100_0_PA_SIZE;
                    irq = SATA_FTSATA100_0_IRQ;
                    break;
                case 1:
                    mem = SATA_FTSATA100_1_PA_BASE;
                    mem_size = SATA_FTSATA100_1_PA_SIZE;
                    irq = SATA_FTSATA100_1_IRQ;
                    break;
                case 2:
                    mem = SATA_FTSATA100_2_PA_BASE;
                    mem_size = SATA_FTSATA100_2_PA_SIZE;
                    irq = SATA_FTSATA100_2_IRQ;
                    break;
                case 3:
                    mem = SATA_FTSATA100_3_PA_BASE;
                    mem_size = SATA_FTSATA100_3_PA_SIZE;
                    irq = SATA_FTSATA100_3_IRQ;
                    break;
            }
            ft_host->mmio = ioremap(mem, mem_size);
            ft_host->id = i;

            rc = request_irq(irq, ftsata100_interrupt_translate, IRQF_SHARED, "ftsata100", ft_host);
            if (rc)
                panic("%s : can't request irq %d\n", __func__, irq);
        }

        slave_8210_pmu_va_base = ioremap(0xE1000000, PMU_FTPMU010_VA_SIZE);

        return 0;
    }
#else
    if ((pci_id != FMEM_PCI_HOST) || (cpu_id != FMEM_CPU_FA726))
        return 0;
#endif //#ifdef CONFIG_EP_SATA_ENABLE

    h2x_sata0_va_base = ioremap(H2X_FTH2X030_SATA_0_PA_BASE, H2X_FTH2X030_SATA_0_PA_SIZE);
    h2x_sata1_va_base = ioremap(H2X_FTH2X030_SATA_1_PA_BASE, H2X_FTH2X030_SATA_1_PA_SIZE);
    h2x_sata2_va_base = ioremap(H2X_FTH2X030_SATA_2_PA_BASE, H2X_FTH2X030_SATA_2_PA_SIZE);
    h2x_sata3_va_base = ioremap(H2X_FTH2X030_SATA_3_PA_BASE, H2X_FTH2X030_SATA_3_PA_SIZE);

    pcie_va_base = ioremap(PCIE_PLDA_0_PA_BASE, PCIE_PLDA_0_PA_SIZE);

    i2c0_sataphy_va_base = ioremap(I2C_PCIE_FTI2C010_0_PA_BASE, I2C_PCIE_FTI2C010_0_PA_SIZE);
    i2c1_sataphy_va_base = ioremap(I2C_PCIE_FTI2C010_1_PA_BASE, I2C_PCIE_FTI2C010_1_PA_SIZE);
    i2c2_sataphy_va_base = ioremap(I2C_PCIE_FTI2C010_2_PA_BASE, I2C_PCIE_FTI2C010_2_PA_SIZE);
    i2c3_sataphy_va_base = ioremap(I2C_PCIE_FTI2C010_3_PA_BASE, I2C_PCIE_FTI2C010_3_PA_SIZE);

    plat_id = PLAT_8312_ID;
#endif //#ifdef CONFIG_PLATFORM_GM8210

#ifdef CONFIG_PLATFORM_GM8287
    i2c3_sataphy_va_base = ioremap(I2C_FTI2C010_3_PA_BASE, I2C_FTI2C010_3_PA_SIZE);
    i2c4_sataphy_va_base = ioremap(I2C_FTI2C010_4_PA_BASE, I2C_FTI2C010_4_PA_SIZE);

    plat_id = PLAT_8287_ID;
#endif

	i2c_sladdr_w = 0x00;
	i2c_sladdr_r = i2c_sladdr_w | 0x01;

    ftsata100_clk_enable();
    ftsata100_phy_config();

	return platform_driver_probe(&ahci_driver, ahci_probe);
}
module_init(ahci_init);

static void __exit ahci_exit(void)
{
#ifdef CONFIG_PLATFORM_GM8210
#ifdef CONFIG_EP_SATA_ENABLE
    if (cpu_id != FMEM_CPU_FA726)
        return;

    if (pci_id == FMEM_PCI_HOST) {
        int i;

        for (i = 0; i < 4; i++) {
            struct s_ftsata100_host *ft_host = &ftsata100_host[i];
            iounmap(ft_host->mmio);
        }

        iounmap(slave_8210_pmu_va_base);

        return;
    }
#else
    if ((pci_id != FMEM_PCI_HOST) || (cpu_id != FMEM_CPU_FA726))
        return;
#endif

    iounmap(h2x_sata0_va_base);
    iounmap(h2x_sata1_va_base);
    iounmap(h2x_sata2_va_base);
    iounmap(h2x_sata3_va_base);

    iounmap(pcie_va_base);

    iounmap(i2c0_sataphy_va_base);
    iounmap(i2c1_sataphy_va_base);
    iounmap(i2c2_sataphy_va_base);
    iounmap(i2c3_sataphy_va_base);
#endif

#ifdef CONFIG_PLATFORM_GM8287
    iounmap(i2c3_sataphy_va_base);
    iounmap(i2c4_sataphy_va_base);
#endif

	platform_driver_unregister(&ahci_driver);
}
module_exit(ahci_exit);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");

