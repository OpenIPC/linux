/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/securec.h>

#ifdef CONFIG_ARCH_BSP
#include <linux/mfd/bsp_fmc.h>
#include <linux/mtd/partitions.h>
#include "../../mtdcore.h"
#endif /* CONFIG_ARCH_BSP */

#ifndef CONFIG_ARCH_BSP
/* Hardware register offsets and field definitions */
#define FMC_CFG				0x00
#define FMC_CFG_OP_MODE_MASK		BIT_MASK(0)
#define FMC_CFG_OP_MODE_BOOT		0
#define FMC_CFG_OP_MODE_NORMAL		1
#define fmc_cfg_ecc_type(type)		(((type) & 0x3) << 1)
#define FMC_CFG_FLASH_SEL_MASK		0x6
#define mfc_ecc_type(type)		(((type) & 0x7) << 5)
#define FMC_ECC_TYPE_MASK		GENMASK(7, 5)
#define SPI_NOR_ADDR_MODE_MASK		BIT_MASK(10)
#define SPI_NOR_ADDR_MODE_3BYTES	(0x0 << 10)
#define SPI_NOR_ADDR_MODE_4BYTES	(0x1 << 10)
#define FMC_GLOBAL_CFG			0x04
#define FMC_GLOBAL_CFG_WP_ENABLE	BIT(6)
#define FMC_SPI_TIMING_CFG		0x08
#define timing_cfg_tcsh(nr)		(((nr) & 0xf) << 8)
#define timing_cfg_tcss(nr)		(((nr) & 0xf) << 4)
#define timing_cfg_tshsl(nr)		((nr) & 0xf)
#define CS_HOLD_TIME			0x6
#define CS_SETUP_TIME			0x6
#define CS_DESELECT_TIME		0xf
#define FMC_INT				0x18
#define FMC_INT_OP_DONE			BIT(0)
#define FMC_INT_CLR			0x20
#define FMC_CMD				0x24
#define fmc_cmd_cmd1(cmd)		((cmd) & 0xff)
#define FMC_ADDRL			0x2c
#define FMC_OP_CFG			0x30
#define op_cfg_fm_cs(cs)		((cs) << 11)
#define op_cfg_mem_if_type(type)	(((type) & 0x7) << 7)
#define op_cfg_addr_num(addr)		(((addr) & 0x7) << 4)
#define op_cfg_dummy_num(dummy)		((dummy) & 0xf)
#define FMC_DATA_NUM			0x38
#define fmc_data_num_cnt(cnt)		((cnt) & GENMASK(13, 0))
#define FMC_OP				0x3c
#define FMC_OP_DUMMY_EN			BIT(8)
#define FMC_OP_CMD1_EN			BIT(7)
#define FMC_OP_ADDR_EN			BIT(6)
#define FMC_OP_WRITE_DATA_EN		BIT(5)
#define FMC_OP_READ_DATA_EN		BIT(2)
#define FMC_OP_READ_STATUS_EN		BIT(1)
#define FMC_OP_REG_OP_START		BIT(0)
#define FMC_DMA_LEN			0x40
#define fmc_dma_len_set(len)		((len) & GENMASK(27, 0))
#define FMC_DMA_SADDR_D0		0x4c
#define FMC_DMA_MAX_LEN			(4096)
#define FMC_DMA_MASK			(FMC_DMA_MAX_LEN - 1)
#define FMC_OP_DMA			0x68
#define op_ctrl_rd_opcode(code)		(((code) & 0xff) << 16)
#define op_ctrl_wr_opcode(code)		(((code) & 0xff) << 8)
#define op_ctrl_rw_op(op)		((op) << 1)
#define OP_CTRL_DMA_OP_READY		BIT(0)
#define FMC_OP_READ			0x0
#define FMC_OP_WRITE			0x1
#define FMC_WAIT_TIMEOUT		1000000

enum fmc_iftype {
	IF_TYPE_STD,
	IF_TYPE_DUAL,
	IF_TYPE_DIO,
	IF_TYPE_QUAD,
	IF_TYPE_QIO,
};
#endif /* CONFIG_ARCH_BSP */

struct fmc_priv {
	u32 chipselect;
	u32 clkrate;
	struct fmc_host *host;
};

#ifndef CONFIG_ARCH_BSP
#define FMC_MAX_CHIP_NUM		2
#endif /* CONFIG_ARCH_BSP */
struct fmc_host {
	struct device *dev;
#ifdef CONFIG_ARCH_BSP
	struct mutex *lock;
#else
	struct mutex lock;
#endif /* CONFIG_ARCH_BSP */

	void __iomem *regbase;
	void __iomem *iobase;
	struct clk *clk;
	void *buffer;
	dma_addr_t dma_buffer;

	struct spi_nor	*nor[FMC_MAX_CHIP_NUM];
	u32 num_chip;
#ifdef CONFIG_ARCH_BSP
	struct fmc_priv priv[FMC_MAX_CHIP_NUM];
	unsigned int dma_len;
#endif /* CONFIG_ARCH_BSP */
};

static inline int bsp_spi_nor_wait_op_finish(const struct fmc_host *host)
{
	u32 reg;

	if (!host)
		return -1;

	return readl_poll_timeout(host->regbase + FMC_INT, reg,
		(reg & FMC_INT_OP_DONE), 0, FMC_WAIT_TIMEOUT);
}

static u8 bsp_spi_nor_get_if_type(enum spi_nor_protocol proto)
{
	enum fmc_iftype if_type;

	switch (proto) {
	case SNOR_PROTO_1_1_2:
		if_type = IF_TYPE_DUAL;
		break;
	case SNOR_PROTO_1_2_2:
		if_type = IF_TYPE_DIO;
		break;
	case SNOR_PROTO_1_1_4:
		if_type = IF_TYPE_QUAD;
		break;
	case SNOR_PROTO_1_4_4:
		if_type = IF_TYPE_QIO;
		break;
	case SNOR_PROTO_1_1_1:
	default:
		if_type = IF_TYPE_STD;
		break;
	}

	return (u8)if_type;
}

#ifdef CONFIG_ARCH_BSP
static void spi_nor_switch_spi_type(struct fmc_host *host)
{
	unsigned int reg;

	reg = readl(host->regbase + FMC_CFG);
	reg &= ~FLASH_TYPE_SEL_MASK;
	reg |= fmc_cfg_ecc_type(0);
	writel(reg, host->regbase + FMC_CFG);
}
#endif /* CONFIG_ARCH_BSP */

static void bsp_spi_nor_init(struct fmc_host *host)
{
	u32 reg;

#ifdef CONFIG_ARCH_BSP
	/* switch the flash type to spi nor */
	spi_nor_switch_spi_type(host);

	/* set the boot mode to normal */
	reg = readl(host->regbase + FMC_CFG);
	if ((reg & FMC_CFG_OP_MODE_MASK) == FMC_CFG_OP_MODE_BOOT) {
		reg |= fmc_cfg_op_mode(FMC_CFG_OP_MODE_NORMAL);
		writel(reg, host->regbase + FMC_CFG);
	}

	/* hold on STR mode */
	reg = readl(host->regbase + FMC_GLOBAL_CFG);
	reg &= (~FMC_GLOBAL_CFG_DTR_MODE);
	writel(reg, host->regbase + FMC_GLOBAL_CFG);
#endif /* CONFIG_ARCH_BSP */

	/* set timming */
	reg = timing_cfg_tcsh(CS_HOLD_TIME) |
		  timing_cfg_tcss(CS_SETUP_TIME) |
		  timing_cfg_tshsl(CS_DESELECT_TIME);
	writel(reg, host->regbase + FMC_SPI_TIMING_CFG);
}

#define clk_fmc_to_crg_mhz(_clk)    ((_clk) * 2000000)

static void hifmc100_voltage_state_check(unsigned long *clkrate)
{
	int nfc_voltage_state = NFC_3_3_STATA;
	unsigned int value = 0;
	void __iomem *sys_ctrl_state_reg = NULL;
	if (clkrate == NULL) {
		return;
	}
	if ((*clkrate) <= (u_long)clk_fmc_to_crg_mhz(NFC_CLK_75MHZ)) {
		return;
	}
	sys_ctrl_state_reg = (void *)ioremap(SYS_CTRL_REG_BASE + SYS_CTRL_STATE_OFF, SYS_CTRL_STATE_SIZE);
	if (sys_ctrl_state_reg == NULL) {
		fmc_pr(PM_DBG, "|-ERR:sys state ioremap fail\n");
		return;
	}
	value = readl(sys_ctrl_state_reg);
	if (((value >> NFC_STATE_BIT_OFF) & NFC_STATA_MASK) == NFC_1_8_STATA) {
		nfc_voltage_state = NFC_1_8_STATA;
		fmc_pr(PM_DBG, "|-nfc voltage is 1.8v\n");
	}
	if ((nfc_voltage_state == NFC_1_8_STATA) && \
			(*clkrate > (u_long)clk_fmc_to_crg_mhz(NFC_CLK_75MHZ))) {
		*clkrate = (u_long)clk_fmc_to_crg_mhz(NFC_CLK_75MHZ);
		fmc_pr(PM_DBG, "|-nfc clk is set to 75MHZ\n");
	}
	iounmap(sys_ctrl_state_reg);
}

static int bsp_spi_nor_prep(struct spi_nor *nor)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	int ret;
#ifdef CONFIG_ARCH_BSP
	unsigned long clkrate;

	mutex_lock(&fmc_switch_mutex);
	mutex_lock(host->lock);

	clkrate = min_t(u32, priv->clkrate, nor->clkrate);

	hifmc100_voltage_state_check(&clkrate);
	ret = clk_set_rate(host->clk, clkrate);
#else
	mutex_lock(&host->lock);

	ret = clk_set_rate(host->clk, priv->clkrate);
#endif /* CONFIG_ARCH_BSP */
	if (ret)
		goto out;

	ret = clk_prepare_enable(host->clk);
	if (ret)
		goto out;

#ifdef CONFIG_ARCH_BSP
	spi_nor_switch_spi_type(host);
#endif /* CONFIG_ARCH_BSP */

	return 0;

out:
#ifdef CONFIG_ARCH_BSP
	mutex_unlock(host->lock);
#else
	mutex_unlock(&host->lock);
#endif /* CONFIG_ARCH_BSP */
	return ret;
}

static void bsp_spi_nor_unprep(struct spi_nor *nor)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;

	clk_disable_unprepare(host->clk);
#ifdef CONFIG_ARCH_BSP
	mutex_unlock(host->lock);
	mutex_unlock(&fmc_switch_mutex);
#else
	mutex_unlock(&host->lock);
#endif /* CONFIG_ARCH_BSP */
}

static int bsp_spi_nor_op_reg(struct spi_nor *nor,
				u8 opcode, size_t len, u8 optype)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	u32 reg;

	reg = fmc_cmd_cmd1(opcode);
	writel(reg, host->regbase + FMC_CMD);

	reg = fmc_data_num_cnt((unsigned int)len);
	writel(reg, host->regbase + FMC_DATA_NUM);

#ifdef CONFIG_ARCH_BSP
	reg = op_cfg_fm_cs(priv->chipselect) | OP_CFG_OEN_EN;
#else
	reg = op_cfg_fm_cs(priv->chipselect);
#endif /* CONFIG_ARCH_BSP */
	writel(reg, host->regbase + FMC_OP_CFG);

	writel(0xff, host->regbase + FMC_INT_CLR);
	reg = FMC_OP_CMD1_EN | FMC_OP_REG_OP_START | optype;
	writel(reg, host->regbase + FMC_OP);

	return bsp_spi_nor_wait_op_finish(host);
}

static int bsp_spi_nor_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf,
				 size_t len)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	int ret;

	ret = bsp_spi_nor_op_reg(nor, opcode, len, FMC_OP_READ_DATA_EN);
	if (ret)
		return ret;

	memcpy_fromio(buf, host->iobase, len);
	return 0;
}

static int bsp_spi_nor_write_reg(struct spi_nor *nor, u8 opcode,
				  const u8 *buf, size_t len)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;

	if (len)
		memcpy_toio(host->iobase, buf, len);

	return bsp_spi_nor_op_reg(nor, opcode, len, FMC_OP_WRITE_DATA_EN);
}

static int bsp_spi_nor_dma_transfer(struct spi_nor *nor, loff_t start_off,
		dma_addr_t dma_buf, size_t len, u8 op_type)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	u8 if_type = 0;
	u32 reg;

	reg = readl(host->regbase + FMC_CFG);
	reg &= ~(FMC_CFG_OP_MODE_MASK | SPI_NOR_ADDR_MODE_MASK);
	reg |= FMC_CFG_OP_MODE_NORMAL;
	reg |= (nor->addr_width == 4) ? SPI_NOR_ADDR_MODE_4BYTES
		: SPI_NOR_ADDR_MODE_3BYTES;
	writel(reg, host->regbase + FMC_CFG);

	writel(start_off, host->regbase + FMC_ADDRL);

#ifdef CONFIG_ARCH_BSP
	reg = (unsigned int)dma_buf;
	writel(reg, host->regbase + FMC_DMA_SADDR_D0);

#ifdef CONFIG_64BIT
	reg = (dma_buf & FMC_DMA_SADDRH_MASK) >> 32;
	writel(reg, host->regbase + FMC_DMA_SADDRH_D0);
#endif
#else
	writel(dma_buf, host->regbase + FMC_DMA_SADDR_D0);
#endif /* CONFIG_ARCH_BSP */

	writel(fmc_dma_len_set(len), host->regbase + FMC_DMA_LEN);

	reg = op_cfg_fm_cs(priv->chipselect);
	if (op_type == FMC_OP_READ)
		if_type = bsp_spi_nor_get_if_type(nor->read_proto);
	else
		if_type = bsp_spi_nor_get_if_type(nor->write_proto);
	reg |= op_cfg_mem_if_type(if_type);
	if (op_type == FMC_OP_READ)
		reg |= op_cfg_dummy_num(nor->read_dummy >> 3);

#ifdef CONFIG_ARCH_BSP
	reg |= OP_CFG_OEN_EN;
#endif /* CONFIG_ARCH_BSP */

	writel(reg, host->regbase + FMC_OP_CFG);

	writel(0xff, host->regbase + FMC_INT_CLR);
	reg = op_ctrl_rw_op(op_type) | OP_CTRL_DMA_OP_READY;
	reg |= (op_type == FMC_OP_READ) ?
		op_ctrl_rd_opcode(nor->read_opcode) :
		op_ctrl_wr_opcode(nor->program_opcode);
	writel(reg, host->regbase + FMC_OP_DMA);

	return bsp_spi_nor_wait_op_finish(host);
}

static ssize_t bsp_spi_nor_read(struct spi_nor *nor, loff_t from, size_t len,
		u_char *read_buf)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	size_t offset;
	int ret;

#ifdef CONFIG_ARCH_BSP
	for (offset = 0; offset < len; offset += host->dma_len) {
		size_t trans = min_t(size_t, host->dma_len, len - offset);
#else
	for (offset = 0; offset < len; offset += FMC_DMA_MAX_LEN) {
		size_t trans = min_t(size_t, FMC_DMA_MAX_LEN, len - offset);
#endif /* CONFIG_ARCH_BSP */

		ret = bsp_spi_nor_dma_transfer(nor,
			from + offset, host->dma_buffer, trans, FMC_OP_READ);
		if (ret) {
			dev_warn(nor->dev, "DMA read timeout\n");
			return ret;
		}

		ret = memcpy_s(read_buf + offset, trans, host->buffer, trans);
		if (ret) {
			printk("%s:memcpy_s failed\n", __func__);
			return ret;
		}
	}

	return len;
}

static ssize_t bsp_spi_nor_write(struct spi_nor *nor, loff_t to,
			size_t len, const u_char *write_buf)
{
	struct fmc_priv *priv = nor->priv;
	struct fmc_host *host = priv->host;
	size_t offset;
	int ret;

#ifdef CONFIG_ARCH_BSP
	for (offset = 0; offset < len; offset += host->dma_len) {
		size_t trans = min_t(size_t, host->dma_len, len - offset);
#else
	for (offset = 0; offset < len; offset += FMC_DMA_MAX_LEN) {
		size_t trans = min_t(size_t, FMC_DMA_MAX_LEN, len - offset);
#endif /* CONFIG_ARCH_BSP */

		ret = memcpy_s(host->buffer, trans, write_buf + offset, trans);
		if (ret) {
			printk("%s,memcpy_s failed\n", __func__);
			return ret;
		}

		ret = bsp_spi_nor_dma_transfer(nor,
			to + offset, host->dma_buffer, trans, FMC_OP_WRITE);
		if (ret) {
			dev_warn(nor->dev, "DMA write timeout\n");
			return ret;
		}
	}

	return len;
}

#ifdef CONFIG_ARCH_BSP
/*
 * The split NOR layout: one "firmware" partition holding a legacy uImage and,
 * at the next erase block after it, the root squashfs; mtdsplit divides it at
 * boot. A fixed-slot layout also starts with a kernel image, but its squashfs
 * sits at the slot boundary, so it never has this shape.
 */
static bool bsp_snor_split_at(struct mtd_info *mtd, u64 offset)
{
	struct {
		__be32 magic, hcrc, time, size;
	} hdr;
	__le32 squashfs;
	size_t retlen;
	u64 kernel;

	if (mtd_read(mtd, offset, sizeof(hdr), &retlen, (u_char *)&hdr) ||
	    retlen != sizeof(hdr) || be32_to_cpu(hdr.magic) != 0x27051956)
		return false;
	kernel = round_up(64 + (u64)be32_to_cpu(hdr.size), mtd->erasesize);
	if (offset + kernel + sizeof(squashfs) > mtd->size)
		return false;
	return !mtd_read(mtd, offset + kernel, sizeof(squashfs), &retlen,
			 (u_char *)&squashfs) &&
		retlen == sizeof(squashfs) &&
		le32_to_cpu(squashfs) == 0x73717368;	/* "hsqs" */
}

static bool bsp_snor_split_layout(struct mtd_info *mtd)
{
	struct device_node *parts, *pp;
	bool split = false;

	/* without the splitters, "firmware" would never become a root */
	if (!IS_ENABLED(CONFIG_MTD_SPLIT_FIRMWARE) || !IS_ENABLED(CONFIG_MTD_SPLIT_UIMAGE_FW))
		return false;

	parts = of_get_child_by_name(mtd_get_of_node(mtd), "partitions");
	if (!parts)
		return false;
	for_each_child_of_node(parts, pp) {
		const __be32 *reg;
		const char *label;
		int len;

		if (of_property_read_string(pp, "label", &label) ||
		    strcmp(label, "firmware"))
			continue;
		reg = of_get_property(pp, "reg", &len);
		if (reg && len >= (int)sizeof(*reg))
			split = bsp_snor_split_at(mtd,
					of_read_number(reg, of_n_addr_cells(pp)));
		of_node_put(pp);
		break;
	}
	of_node_put(parts);
	return split;
}

/**
 * parse partitions info and register spi flash device as mtd device.
 */
static int bsp_snor_device_register(struct mtd_info *mtd)
{
	static const char * const split_probes[] = { "ofpart", NULL };
	bool split = bsp_snor_split_layout(mtd);
	int ret;

	/*
	 * We do not add the whole spi flash as a mtdblock device,
	 * To avoid the number of nand partition +1.
	 */
	INIT_LIST_HEAD(&mtd->partitions);
	mutex_init(&mtd->master.partitions_lock);

	/*
	 * The split layout is the device tree's: whatever mtdparts the
	 * bootloader passes describes a layout that is not on the flash.
	 * Otherwise the command line still comes first.
	 */
	if (split)
		pr_info("%s: split NOR layout, device tree partitions\n", mtd->name);
	ret = mtd_device_parse_register(mtd, split ? split_probes : NULL, NULL, NULL, 0);

	return ret;
}
#endif /* CONFIG_ARCH_BSP */

static const struct spi_nor_controller_ops bsp_controller_ops = {
	.prepare = bsp_spi_nor_prep,
	.unprepare = bsp_spi_nor_unprep,
	.read_reg = bsp_spi_nor_read_reg,
	.write_reg = bsp_spi_nor_write_reg,
	.read = bsp_spi_nor_read,
	.write = bsp_spi_nor_write,
};

/**
 * Get spi flash device information and register it as a mtd device.
 */
static int bsp_spi_nor_register(struct device_node *np,
				struct fmc_host *host)
{
	struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_READ_1_1_2 |
#ifdef CONFIG_ARCH_BSP
			SNOR_HWCAPS_READ_1_2_2 |
#else

			SNOR_HWCAPS_READ_1_1_4 |
#endif
			SNOR_HWCAPS_PP,
	};
	struct device *dev = NULL;
	struct spi_nor *nor = NULL;
	struct fmc_priv *priv = NULL;
	struct mtd_info *mtd = NULL;
	int ret;

	if (!host || !host->dev)
		return -ENXIO;

	dev = host->dev;
	nor = devm_kzalloc(dev, sizeof(*nor), GFP_KERNEL);
	if (!nor)
		return -ENOMEM;

	nor->dev = dev;
	spi_nor_set_flash_node(nor, np);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	ret = of_property_read_u32(np, "reg", &priv->chipselect);
	if (ret) {
		dev_err(dev, "There's no reg property for %pOF\n",
			np);
		return ret;
	}

#ifdef CONFIG_ARCH_BSP
	if (priv->chipselect != host->num_chip) {
		dev_warn(dev, " The CS: %d states in device trees isn't real " \
				"chipselect on board\n, using CS: %d instead. ",
				priv->chipselect, host->num_chip);
		priv->chipselect = host->num_chip;
	}
#endif /* CONFIG_ARCH_BSP */

	ret = of_property_read_u32(np, "spi-max-frequency",
			&priv->clkrate);
	if (ret) {
		dev_err(dev, "There's no spi-max-frequency property for %pOF\n",
			np);
		return ret;
	}
	priv->host = host;
	nor->priv = priv;
	nor->controller_ops = &bsp_controller_ops;

#ifndef CONFIG_CLOSE_SPI_8PIN_4IO
	hwcaps.mask |= SNOR_HWCAPS_READ_1_1_4 |
		       SNOR_HWCAPS_READ_1_4_4 |
		       SNOR_HWCAPS_PP_1_1_4 |
		       SNOR_HWCAPS_PP_1_4_4;
#endif

	ret = spi_nor_scan(nor, NULL, &hwcaps);
	if (ret)
		return ret;

	mtd = &nor->mtd;
	mtd->name = np->name;
#ifdef CONFIG_ARCH_BSP
	ret = bsp_snor_device_register(mtd);
	if (ret < 0)
		return ret;
	/* current chipselect has scanned, to detect next chipselect */
	fmc_cs_user[host->num_chip]++;
#else
	ret = mtd_device_register(mtd, NULL, 0);
	if (ret)
		return ret;
	host->num_chip++;
#endif /* CONFIG_ARCH_BSP */

	host->nor[host->num_chip] = nor;
	return 0;
}

static void bsp_spi_nor_unregister_all(struct fmc_host *host)
{
	int i;

	for (i = 0; i < host->num_chip; i++)
		mtd_device_unregister(&host->nor[i]->mtd);
}

static int bsp_spi_nor_register_all(struct fmc_host *host)
{
	struct device *dev = host->dev;
	struct device_node *np;
	int ret;

	for_each_available_child_of_node(dev->of_node, np) {
#ifdef CONFIG_ARCH_BSP
		if (host->num_chip == FMC_MAX_CHIP_NUM) {
			dev_warn(dev, "Flash device number exceeds the maximum chipselect number\n");
			break;
		}
		if (fmc_cs_user[host->num_chip]) {
			dev_warn(dev, "Current CS(%d) is occupied.\n",
					host->num_chip);
			continue;
		}
#endif /* CONFIG_ARCH_BSP */
		ret = bsp_spi_nor_register(np, host);
		if (ret)
			goto fail;

#ifdef CONFIG_ARCH_BSP
		host->num_chip++;
#endif /* CONFIG_ARCH_BSP */
	}

	return 0;

fail:
	bsp_spi_nor_unregister_all(host);
	return ret;
}

#ifdef CONFIG_ARCH_BSP
static int bsp_init_host(struct platform_device *pdev, struct fmc_host *host)
{
	struct device *dev = &pdev->dev;
	struct bsp_fmc *fmc = dev_get_drvdata(dev->parent);

	if (!fmc) {
		dev_err(&pdev->dev, "get mfd fmc devices failed\n");
		return -ENXIO;
	}

	host->regbase = fmc->regbase;
	host->iobase = fmc->iobase;
	host->clk = fmc->clk;
	host->lock = &fmc->lock;
	host->buffer = fmc->buffer;
	host->dma_buffer = fmc->dma_buffer;
	host->dma_len = fmc->dma_len;

	return 0;
}
#endif /* CONFIG_ARCH_BSP */

static int bsp_spi_nor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#ifndef CONFIG_ARCH_BSP
	struct resource *res;
#endif /* CONFIG_ARCH_BSP */
	struct fmc_host *host;
	int ret;

	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	platform_set_drvdata(pdev, host);
	host->dev = dev;

#ifdef CONFIG_ARCH_BSP
	ret = bsp_init_host(pdev, host);
	if (ret) {
		return ret;
	}
#else
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
	host->regbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(host->regbase))
		return PTR_ERR(host->regbase);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memory");
	host->iobase = devm_ioremap_resource(dev, res);
	if (IS_ERR(host->iobase))
		return PTR_ERR(host->iobase);

	host->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32)); /* 32:Addressing capability of the DMA */
	if (ret) {
		dev_warn(dev, "Unable to set dma mask\n");
		return ret;
	}

	host->buffer = dmam_alloc_coherent(dev, FMC_DMA_MAX_LEN,
			&host->dma_buffer, GFP_KERNEL);
	if (!host->buffer)
		return -ENOMEM;
#endif /* CONFIG_ARCH_BSP */

	ret = clk_prepare_enable(host->clk);
	if (ret)
		return ret;

#ifndef CONFIG_ARCH_BSP
	mutex_init(&host->lock);
#endif /* CONFIG_ARCH_BSP */
	bsp_spi_nor_init(host);
	ret = bsp_spi_nor_register_all(host);
	if (ret) {
#ifdef CONFIG_ARCH_BSP
		dev_warn(dev, "spi nor register fail!\n");
#else
		mutex_destroy(&host->lock);
#endif /* CONFIG_ARCH_BSP */
		clk_disable_unprepare(host->clk);
	}

	return ret;
}

static int bsp_spi_nor_remove(struct platform_device *pdev)
{
	struct fmc_host *host = platform_get_drvdata(pdev);
	unsigned char cs;

	for (cs = 0; cs < FMC_MAX_CHIP_NUM; cs++)
		fmc_cs_user[cs] = 0;

	if (host && host->clk) {
		bsp_spi_nor_unregister_all(host);
#ifndef CONFIG_ARCH_BSP
		mutex_destroy(&host->lock);
#endif /* CONFIG_ARCH_BSP */
		clk_disable_unprepare(host->clk);
	}

	return 0;
}

#ifdef CONFIG_ARCH_BSP

static void bsp_spi_nor_driver_shutdown(struct platform_device *pdev)
{
	int i;
	struct fmc_host *host = platform_get_drvdata(pdev);

	if (!host || !host->clk)
		return;

	mutex_lock(host->lock);
	clk_prepare_enable(host->clk);

	spi_nor_switch_spi_type(host);
	for (i = 0; i < host->num_chip; i++)
		spi_nor_driver_shutdown(host->nor[i]);

	clk_disable_unprepare(host->clk);
	mutex_unlock(host->lock);
	dev_dbg(host->dev, "End of driver shutdown\n");
}
#endif /* CONFIG_ARCH_BSP */

static const struct of_device_id bsp_spi_nor_dt_ids[] = {
	{ .compatible = "vendor,fmc-spi-nor"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, bsp_spi_nor_dt_ids);

static struct platform_driver bsp_spi_nor_driver = {
	.driver = {
		.name	= "bsp-sfc",
		.of_match_table = bsp_spi_nor_dt_ids,
	},
	.probe		= bsp_spi_nor_probe,
	.remove		= bsp_spi_nor_remove,
#ifdef CONFIG_ARCH_BSP
	.shutdown	= bsp_spi_nor_driver_shutdown,
#endif /* CONFIG_ARCH_BSP */
};
module_platform_driver(bsp_spi_nor_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SPI Nor Flash Controller Driver");
