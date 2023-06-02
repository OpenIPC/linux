/*
 * device driver for Serial NOR Flash on Novatek platform
 * The serial nor interface is largely based on drivers/mtd/m25p80.c,
 * however the SPI interface has been replaced by Novatek.
 *
 * Copyright © 2017 Novatek Microelectronics.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <plat/nvt_flash.h>
#include <plat/nvt-sramctl.h>
#include "../nvt_flash_spi/nvt_flash_spi_reg.h"
#include "../nvt_flash_spi/nvt_flash_spi_int.h"
#include "ext_flash_table.h"

/* SMI clock rate */
#define NVT_FLASH_MAX_CLOCK_FREQ	50000000 /* 50 MHz */

/* MAX time out to safely come out of a erase or write busy conditions */
#define NVT_FLASH_PROBE_TIMEOUT	(HZ / 10)
#define NVT_FLASH_MAX_TIME_OUT	(3 * HZ)

/* timeout for command completion */
#define NVT_FLASH_CMD_TIMEOUT		(HZ / 10)

#define ERASE_MASK_64K	0xFFFF
#define ERASE_64K	0x10000

#ifndef CONFIG_MTD_EXTERNAL_FLASH_TABLE
static struct flash_device flash_devices[] = {
	FLASH_ID("st m25p16"     , 0xd8, 0x00152020, 0x100, 0x10000, 0x200000, 0, 0),
	FLASH_ID("st m25p32"     , 0xd8, 0x00162020, 0x100, 0x10000, 0x400000, 0, 0),
	FLASH_ID("st m25p64"     , 0xd8, 0x00172020, 0x100, 0x10000, 0x800000, 0, 0),
	FLASH_ID("st m25p128"    , 0xd8, 0x00182020, 0x100, 0x40000, 0x1000000, 0, 0),
	FLASH_ID("st m25p05"     , 0xd8, 0x00102020, 0x80 , 0x8000 , 0x10000, 0, 0),
	FLASH_ID("st m25p10"     , 0xd8, 0x00112020, 0x80 , 0x8000 , 0x20000, 0, 0),
	FLASH_ID("st m25p20"     , 0xd8, 0x00122020, 0x100, 0x10000, 0x40000, 0, 0),
	FLASH_ID("st m25p40"     , 0xd8, 0x00132020, 0x100, 0x10000, 0x80000, 0, 0),
	FLASH_ID("st m25p80"     , 0xd8, 0x00142020, 0x100, 0x10000, 0x100000, 0, 0),
	FLASH_ID("st m45pe10"    , 0xd8, 0x00114020, 0x100, 0x10000, 0x20000, 0, 0),
	FLASH_ID("st m45pe20"    , 0xd8, 0x00124020, 0x100, 0x10000, 0x40000, 0, 0),
	FLASH_ID("st m45pe40"    , 0xd8, 0x00134020, 0x100, 0x10000, 0x80000, 0, 0),
	FLASH_ID("st m45pe80"    , 0xd8, 0x00144020, 0x100, 0x10000, 0x100000, 0, 0),
	FLASH_ID("sp s25fl004"   , 0xd8, 0x00120201, 0x100, 0x10000, 0x80000, 0, 0),
	FLASH_ID("sp s25fl008"   , 0xd8, 0x00130201, 0x100, 0x10000, 0x100000, 0, 0),
	FLASH_ID("sp s25fl016"   , 0xd8, 0x00140201, 0x100, 0x10000, 0x200000, 0, 0),
	FLASH_ID("sp s25fl032"   , 0xd8, 0x00150201, 0x100, 0x10000, 0x400000, 0, 0),
	FLASH_ID("sp s25fl064"   , 0xd8, 0x00160201, 0x100, 0x10000, 0x800000, 0, 0),
	FLASH_ID("sp s25fl128"   , 0xd8, 0x00182001, 0x100, 0x10000, 0x1000000, 0, 0),
	FLASH_ID("atmel 25f512"  , 0x52, 0x0065001F, 0x80 , 0x8000 , 0x10000, 0, 0),
	FLASH_ID("atmel 25f1024" , 0x52, 0x0060001F, 0x100, 0x8000 , 0x20000, 0, 0),
	FLASH_ID("atmel 25f2048" , 0x52, 0x0063001F, 0x100, 0x10000, 0x40000, 0, 0),
	FLASH_ID("atmel 25f4096" , 0x52, 0x0064001F, 0x100, 0x10000, 0x80000, 0, 0),
	FLASH_ID("atmel 25fs040" , 0xd7, 0x0004661F, 0x100, 0x10000, 0x80000, 0, 0),
	FLASH_ID("mac 25l512"    , 0xd8, 0x001020C2, 0x010, 0x10000, 0x10000, 0, 0),
	FLASH_ID("mac 25l1005"   , 0xd8, 0x001120C2, 0x010, 0x10000, 0x20000, 0, 0),
	FLASH_ID("mac 25l2005"   , 0xd8, 0x001220C2, 0x010, 0x10000, 0x40000, 0, 0),
	FLASH_ID("mac 25l4005"   , 0xd8, 0x001320C2, 0x010, 0x10000, 0x80000, 0, 0),
	FLASH_ID("mac 25l4005a"  , 0xd8, 0x001320C2, 0x010, 0x10000, 0x80000, 0, 0),
	FLASH_ID("mac 25l8005"   , 0xd8, 0x001420C2, 0x010, 0x10000, 0x100000, 0, 0),
	FLASH_ID("mac 25l1605"   , 0xd8, 0x001520C2, 0x100, 0x10000, 0x200000, 0, 0),
	FLASH_ID("mac 25l1605a"  , 0xd8, 0x001520C2, 0x010, 0x10000, 0x200000, 0, 0),
	FLASH_ID("mac 25l3205"   , 0xd8, 0x001620C2, 0x100, 0x10000, 0x400000, 0, 0),
	FLASH_ID("mac 25l3205a"  , 0xd8, 0x001620C2, 0x100, 0x10000, 0x400000, 0, 0),
	FLASH_ID("mac 25l6405"   , 0xd8, 0x001720C2, 0x100, 0x10000, 0x800000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("mx 25l12835f"  , 0xd8, 0x001820C2, 0x100, 0x10000, 0x1000000, 0, SPI_NOR_QUAD_READ),
	FLASH_ID("mx 25l25645g"  , 0xd8, 0x001920C2, 0x100, 0x10000, 0x2000000, 0, SPI_NOR_QUAD_READ),
	FLASH_ID("mx 25l51245g"  , 0xd8, 0x001A20C2, 0x100, 0x10000, 0x4000000, 0, 0),
	FLASH_ID("wb W25Q32FV"   , 0xd8, 0x001640EF, 0x100, 0x10000,  0x400000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("wb W25Q64FV"   , 0xd8, 0x001740EF, 0x100, 0x10000,  0x800000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("wb W25Q128BV"  , 0xd8, 0x001840EF, 0x100, 0x10000,  0x1000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("wb W25Q256BV"  , 0xd8, 0x001940EF, 0x100, 0x10000,  0x2000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("wb W25Q256BV"  , 0xd8, 0x001970EF, 0x100, 0x10000,  0x2000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("wb W25Q512JV"  , 0xd8, 0x002040EF, 0x100, 0x10000,  0x4000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("gd W25Q64C"    , 0xd8, 0x001740C8, 0x100, 0x10000,  0x800000,  WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("gd W25Q127C"   , 0xd8, 0x001840C8, 0x100, 0x10000,  0x1000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("gd W25Q256C"   , 0xd8, 0x001940C8, 0x100, 0x10000,  0x2000000,      0, SPI_NOR_DUAL_READ),
	FLASH_ID("eon en25qh64"  , 0xd8, 0x0017701C, 0x100, 0x10000,  0x800000,  WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("eon en25qh128" , 0xd8, 0x0018701C, 0x100, 0x10000,  0x1000000, WR_QPP, SPI_NOR_QUAD_READ),
	FLASH_ID("xmc xm25qh128a", 0xd8, 0x00187020, 0x100, 0x10000,  0x1000000, WR_QPP, SPI_NOR_QUAD_READ),
};
#endif

static struct flash_device flash_std_devices[] = {
	FLASH_ID("STDR04FW"     , 0xd8, 0x0013FFFF, 0x100, 0x10000, 0x80000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR08FW"     , 0xd8, 0x0014FFFF, 0x100, 0x10000, 0x100000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR16FW"     , 0xd8, 0x0015FFFF, 0x100, 0x10000, 0x200000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR32FW"     , 0xd8, 0x0016FFFF, 0x100, 0x10000, 0x400000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR64FW"     , 0xd8, 0x0017FFFF, 0x100, 0x10000, 0x800000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR128FW"    , 0xd8, 0x0018FFFF, 0x100, 0x10000, 0x1000000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR256FW"    , 0xd8, 0x0019FFFF, 0x100, 0x10000, 0x2000000, 0, SPI_NOR_DUAL_READ),
	FLASH_ID("STDR512FW"    , 0xd8, 0x001AFFFF, 0x100, 0x10000, 0x4000000, 0, SPI_NOR_DUAL_READ),
};


/* Define spear specific structures */

struct nvt_spinor_flash;
u8 std_path = 0;

/**
 * struct nvt_flash - Structure for NVT Flash Device
 *
 * @clk: functional clock
 * @status: current status register of NVT Flash.
 * @clk_rate: functional clock rate of NVT Flash (default: NVT_FLASH_MAX_CLOCK_FREQ)
 * @lock: lock to prevent parallel access of NVT Flash.
 * @info: drv_nand_dev_info of NVT Flash.
 * @pdev: platform device
 * @flash: separate structure for each Serial NOR-flash attached to NVT Flash.
 */
struct nvt_flash {
	struct clk *clk;
	unsigned long clk_rate;
	struct mutex lock;
	struct drv_nand_dev_info *info;
	struct platform_device *pdev;
	struct nvt_spinor_flash *flash;
};

/**
 * struct nvt_spinor_flash - Structure for Serial NOR Flash
 *
 * @dev_id: Device ID of NOR-flash.
 * @mtd: MTD info for each NOR-flash.
 * @parts: Partition info of NOR-flash.
 * @page_size: Page size of NOR-flash.
 * @base_addr: Base address of NOR-flash.
 * @erase_cmd: erase command may vary on different flash types
 */
struct nvt_spinor_flash {
	u32 dev_id;
	struct mtd_info mtd;
	struct mtd_partition *parts;
	u32 page_size;
	u8 erase_cmd;
	void __iomem *base_addr;
};

static void nvt_spinor_check_fastboot(void)
{
	u32 m_fastboot = 0x0, preload = 0x0;
	struct device_node* of_node = of_find_node_by_path("/fastboot");
	int ret = 0;

	if (of_node) {
		of_property_read_u32(of_node, "enable", &m_fastboot);
	}

	of_node = of_find_node_by_path("/fastboot/spi-nor");

	if (of_node) {
		of_property_read_u32(of_node, "preload", &preload);
	}

	if (m_fastboot && preload) {
		ret = nvt_check_preload_finish();
		if (ret != 1)
			pr_err("error with waiting preload %d\n", ret);
	}
}

static inline struct nvt_spinor_flash *get_flash_data(struct mtd_info *mtd)
{
	return container_of(mtd, struct nvt_spinor_flash, mtd);
}

/**
 * nvt_flash_int_handler - NVT Flash Interrupt Handler.
 * @irq: irq number
 * @dev_id: structure of NVT Flash device, embedded in dev_id.
 *
 * The handler clears all interrupt conditions and records the status in
 * dev->status which is used by the driver later.
 */
static irqreturn_t nvt_flash_int_handler(int irq, void *dev_id)
{
	struct nvt_flash *dev = dev_id;

	dev->info->nand_int_status = NAND_GETREG(dev->info, NAND_CTRL_STS_REG_OFS);

	if (dev->info->nand_int_status) {
		NAND_SETREG(dev->info, NAND_CTRL_STS_REG_OFS, dev->info->nand_int_status);
		complete(&dev->info->cmd_complete);
		return IRQ_HANDLED;
	} else
		return IRQ_NONE;
}

/**
 * nvt_flash_hw_init - initializes the NVT Flash controller.
 * @dev: structure of NVT Flash device
 *
 * this routine initializes the nvt flash controller wit the default values
 */
static void nvt_flash_hw_init(struct nvt_flash *dev)
{
	mutex_lock(&dev->lock);

	NAND_SETREG(dev->info, NAND_TIME0_REG_OFS, 0x06002222);
	NAND_SETREG(dev->info, NAND_TIME1_REG_OFS, 0x7f0f);

	clk_set_rate(dev->clk, dev->info->flash_freq);

	/* Release SRAM */
	nvt_disable_sram_shutdown(NAND_SD);

	nand_host_set_nand_type(dev->info, NANDCTRL_SPI_NOR_TYPE);

	nand_host_settiming2(dev->info, 0x5F51);

	nand_phy_config(dev->info);

	nand_dll_reset(dev->info);

	mutex_unlock(&dev->lock);
}

/**
 * get_flash_index - match chip id from a flash list.
 * @flash_id: a valid nor flash chip id obtained from board.
 *
 * try to validate the chip id by matching from a list, if not found then simply
 * returns negative. In case of success returns index in to the flash devices
 * array.
 */
static int get_flash_index(struct nvt_flash *dev, u32 flash_id)
{
	int index;
	u32 std_id = flash_id & 0xFF0000;

	if (dev->info->trace_stdtable) {
		/* Matches chip-id to entire list of standard table*/
		for (index = 0; index < ARRAY_SIZE(flash_std_devices); index++) {
			if ((flash_std_devices[index].device_id & 0xFF0000) == std_id) {
				std_path = 1;
				return index;
			}
		}
	}

	/* Matches chip-id to entire list of 'serial-nor flash' ids */
	for (index = 0; index < ARRAY_SIZE(flash_devices); index++) {
		if (flash_devices[index].device_id == flash_id)
			return index;
	}

	/* Memory chip is not listed and not supported */
	return -ENODEV;
}

/**
 * nvt_flash_erase_sector - erase one sector of flash
 * @dev: structure of nvt flash information
 * @command: erase command to be send
 * @bytes: size of command
 *
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 * Returns 0 if successful, non-zero otherwise.
 */
static int nvt_flash_erase_sector(struct nvt_flash *dev, u8 cmd, u32 addr)
{
	return spinor_erase_sector(dev->info, cmd, addr);
}

/**
 * nvt_flash_mtd_erase - perform flash erase operation as requested by user
 * @mtd: Provides the memory characteristics
 * @e_info: Provides the erase information
 *
 * Erase an address range on the flash chip. The address range may extend
 * one or more erase sectors. Return an error is there is a problem erasing.
 */
static int nvt_flash_mtd_erase(struct mtd_info *mtd, struct erase_info *e_info)
{
	struct nvt_spinor_flash *flash = get_flash_data(mtd);
	struct nvt_flash *dev = mtd->priv;
	u32 addr;
	int len, ret, blk_erase;

	if (!flash || !dev)
		return -ENODEV;

	addr = e_info->addr;
	len = e_info->len;

	if (len & ERASE_MASK_64K)
		blk_erase = 0;
	else
		blk_erase = 1;

	mutex_lock(&dev->lock);

	/* now erase sectors in loop */
	while (len) {
		/* preparing the command for flash */
		if (blk_erase)
			ret = nvt_flash_erase_sector(dev, flash->erase_cmd, addr);
		else {
			if (flash->mtd.size > SPI_FLASH_16MB_BOUN)
                if (dev->info->enter_4byte_addr_mode == 1)
				    ret = nvt_flash_erase_sector(dev, FLASH_CMD_SECTOR_ERASE, addr);
                else
                    ret = nvt_flash_erase_sector(dev, FLASH_CMD_SECTOR_ERASE_4BYTE, addr);
			else
				ret = nvt_flash_erase_sector(dev, FLASH_CMD_SECTOR_ERASE, addr);
		}

		if (ret) {
			mutex_unlock(&dev->lock);
			return ret;
		}
		if (blk_erase) {
			addr += ERASE_64K;
			len -= ERASE_64K;
		} else {
			addr += mtd->erasesize;
			len -= mtd->erasesize;
		}
	}

	mutex_unlock(&dev->lock);

	return 0;
}

/**
 * nvt_flash_mtd_read - performs flash read operation as requested by the user
 * @mtd: MTD information
 * @from: Address from which to start read
 * @len: Number of bytes to be read
 * @retlen: Fills the Number of bytes actually read
 * @buf: Fills this after reading
 *
 * Read an address range from the flash chip. The address range
 * may be any size provided it is within the physical boundaries.
 * Returns 0 on success, non zero otherwise
 */
static int nvt_flash_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u8 *buf)
{
	struct nvt_spinor_flash *flash = get_flash_data(mtd);
	struct nvt_flash *dev = mtd->priv;
	int ret;
	void *src;

	if (!flash || !dev)
		return -ENODEV;

	src = flash->base_addr;

	mutex_lock(&dev->lock);

	/*Read operation*/

	do {
		if (len > PAGE_SIZE)
			ret = spinor_read_operation(dev->info, from, PAGE_SIZE, (u8 *)src);
		else
			ret = spinor_read_operation(dev->info, from, len, (u8 *)src);

	if (ret) {
		mutex_unlock(&dev->lock);
		return ret;
	}

		if (len > PAGE_SIZE) {
			memcpy_fromio(buf, (u8 *)src, PAGE_SIZE);
			buf += PAGE_SIZE;
			*retlen += PAGE_SIZE;
			from += PAGE_SIZE;
			len -= PAGE_SIZE;
		} else {
			memcpy_fromio(buf, (u8 *)src, len);
			*retlen += len;
			len = 0;
		}
	} while (len > 0);

	mutex_unlock(&dev->lock);

	return 0;
}

static inline int nvt_flash_cpy_toio(struct nvt_flash *dev,
		loff_t dest, const void *src, size_t len)
{
	return spinor_program_operation(dev->info, dest, len, (u8 *)src);
}

/**
 * nvt_flash_mtd_write - performs write operation as requested by the user.
 * @mtd: MTD information.
 * @to:	Address to write.
 * @len: Number of bytes to be written.
 * @retlen: Number of bytes actually wrote.
 * @buf: Buffer from which the data to be taken.
 *
 * Write an address range to the flash chip. Data must be written in
 * flash_page_size chunks. The address range may be any size provided
 * it is within the physical boundaries.
 * Returns 0 on success, non zero otherwise
 */
static int nvt_flash_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u8 *sr_buf)
{
	struct nvt_spinor_flash *flash = get_flash_data(mtd);
	struct nvt_flash *dev = mtd->priv;
	u32 page_offset, page_size;
	int ret;
	void *buf;

	if (!flash || !dev)
		return -ENODEV;

	buf = flash->base_addr;

	mutex_lock(&dev->lock);

	page_offset = (u32)to % flash->page_size;

	// /* do if all the bytes fit onto one page */
	if (page_offset + len <= flash->page_size) {
		memcpy_toio(buf, sr_buf, len);
		ret = nvt_flash_cpy_toio(dev, to, buf, len);
		if (!ret)
			*retlen += len;
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = flash->page_size - page_offset;

		memcpy_toio(buf, sr_buf, page_size);

		ret = nvt_flash_cpy_toio(dev, to, buf,
				page_size);
		if (ret)
			goto err_write;
		else
			*retlen += page_size;

		/* write everything in pagesize chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > flash->page_size)
				page_size = flash->page_size;

			memcpy_toio(buf, sr_buf + i, page_size);

			ret = nvt_flash_cpy_toio(dev, to + i,
					buf, page_size);
			if (ret)
				break;
			else
				*retlen += page_size;
		}
	}

err_write:
	mutex_unlock(&dev->lock);

	return ret;
}

static int nvt_spinor_read_id(struct drv_nand_dev_info *info, uint32_t *id)
{
	uint8_t  card_id[8] = {0};

	if (nand_cmd_read_id(card_id, info) != 0) {
		printk("NOR cmd timeout\r\n");
		return -1;
	} else {
		printk("id =  0x%02x 0x%02x 0x%02x 0x%02x\n",
			card_id[0], card_id[1], card_id[2], card_id[3]);

		*id = card_id[0] | (card_id[1] << 8) | (card_id[2] << 16);
        if (*id == WINBOND_W25Q256FV) {
            info->enter_4byte_addr_mode = 1;
        } else {
            info->enter_4byte_addr_mode = 0;
        }
		return 0;
	}
	return 0;
}

/**
 * nvt_flash_detect - Detects the NOR Flash chip.
 * @dev: structure of NVT Flash information.
 *
 * This routine will check whether there exists a flash chip
 * Return index of the probed flash in flash devices structure
 */
static int nvt_flash_detect(struct nvt_flash *dev)
{
	int ret;
	u32 val = 0;

	mutex_lock(&dev->lock);

	/* send readid command in sw mode */
	ret = nvt_spinor_read_id(dev->info, (uint32_t *)&val);
	if (ret)
		goto err_probe;

	/* get memory chip id */
	val &= 0x00ffffff;
	ret = get_flash_index(dev, val);

err_probe:
	mutex_unlock(&dev->lock);

	return ret;
}


#ifdef CONFIG_OF
static int nvt_flash_probe_config_dt(struct platform_device *pdev,
				     struct device_node *np)
{
	struct nvt_flash_plat_data *pdata = dev_get_platdata(&pdev->dev);
	struct device_node *pp = NULL;
	const __be32 *addr;
	u32 val;
	int len;

	if (!np)
		return -ENODEV;

	of_property_read_u32(np, "clock-frequency", &val);
	pdata->clk_rate = val;

	pdata->board_flash_info = devm_kzalloc(&pdev->dev,
					       sizeof(*pdata->board_flash_info),
					       GFP_KERNEL);

	/* Fill structs for each subnode (flash device) */
	while ((pp = of_get_next_child(np, pp))) {
		struct nvt_flash_info *flash_info;

		flash_info = (void*) &pdata->board_flash_info;
		pdata->np = pp;

		/* Read base-addr and size from DT */
		addr = of_get_property(pp, "reg", &len);
		if (addr) {
			pdata->board_flash_info->mem_base = be32_to_cpup(&addr[1]);
			pdata->board_flash_info->size = be32_to_cpup(&addr[3]);
		}
	}

	return 0;
}
#else
static int nvt_flash_probe_config_dt(struct platform_device *pdev,
				     struct device_node *np)
{
	return -ENOSYS;
}
#endif

static int nvt_read_sfdp(struct drv_nand_dev_info *info)
{
	int ret;
	u8 sfdp_data;
	u32 address = 0x32;

	ret = spinor_read_sfdp(info, address, 1, &sfdp_data);
	if (ret)
		return ret;

	if (sfdp_data == MXIC25L25635F_SFDP)
		info->hspeed_dummy_cycle = 10;

	return E_OK;
}

static int nvt_flash_setup(struct platform_device *pdev,
				 struct device_node *np)
{
	struct nvt_flash *dev = platform_get_drvdata(pdev);
	struct nvt_flash_info *flash_info;
	struct nvt_flash_plat_data *pdata;
	struct nvt_spinor_flash *flash;
#ifndef CONFIG_OF
	struct mtd_part_parser_data ppdata = {};
	struct mtd_partition *parts = NULL;
	int count = 0;
#endif
	int flash_index;
	int ret = 0;
	struct flash_device *trace_flash_devices;

	pdata = dev_get_platdata(&pdev->dev);

	flash_info = pdata->board_flash_info;
	if (!flash_info)
		return -ENODEV;

	flash = devm_kzalloc(&pdev->dev, sizeof(*flash), GFP_ATOMIC);
	if (!flash)
		return -ENOMEM;

	/* verify whether nor flash is really present on board */
	flash_index = nvt_flash_detect(dev);
	if (flash_index < 0) {
		dev_info(&pdev->dev, "spinor not found\n");
		return flash_index;
	}

	/* map the memory for nor flash chip */
	flash->base_addr = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!flash->base_addr)
		return -EIO;

	dev->flash = flash;
	flash->mtd.priv = dev;

#ifdef CONFIG_OF
	of_property_read_string_index(pdev->dev.of_node, "nvt-devname", 0, (void *)&flash_info->name);
#endif

	if (std_path)
		trace_flash_devices = flash_std_devices;
	else
		trace_flash_devices = flash_devices;


	if (flash_info->name)
		flash->mtd.name = flash_info->name;
	else
		flash->mtd.name = trace_flash_devices[flash_index].name;

	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 1;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = trace_flash_devices[flash_index].size_in_bytes;;
	flash->mtd.erasesize = trace_flash_devices[flash_index].sectorsize;
	flash->page_size = trace_flash_devices[flash_index].pagesize;
	flash->mtd.writebufsize = flash->page_size;
	if (flash->mtd.size > SPI_FLASH_16MB_BOUN) {
		if(trace_flash_devices[flash_index].erase_cmd == FLASH_CMD_BLOCK_ERASE)
            if (dev->info->enter_4byte_addr_mode != 1)
			    trace_flash_devices[flash_index].erase_cmd = FLASH_CMD_BLOCK_ERASE_4BYTE;

		if(trace_flash_devices[flash_index].erase_cmd == FLASH_CMD_SECTOR_ERASE)
			trace_flash_devices[flash_index].erase_cmd = FLASH_CMD_SECTOR_ERASE_4BYTE;
	}

	flash->erase_cmd = trace_flash_devices[flash_index].erase_cmd;
	flash->mtd._erase = nvt_flash_mtd_erase;
	flash->mtd._read = nvt_flash_mtd_read;
	flash->mtd._write = nvt_flash_mtd_write;
	flash->dev_id = trace_flash_devices[flash_index].device_id;
	/*Copy flash info*/
	dev->info->flash_info->page_size = flash->page_size;
	dev->info->flash_info->device_size = flash->mtd.size;
	dev->info->flash_info->block_size = flash->mtd.erasesize;
	dev->info->flash_info->chip_sel = 0;
#ifndef CONFIG_FLASH_ONLY_DUAL
	dev->info->flash_info->nor_quad_support = trace_flash_devices[flash_index].quad_mode;
	dev->info->flash_info->nor_read_mode = trace_flash_devices[flash_index].read_mode;
#else
	if (trace_flash_devices[flash_index].quad_mode == WR_QPP)
		dev->info->flash_info->nor_quad_support = 0;

	if (trace_flash_devices[flash_index].read_mode == SPI_NOR_QUAD_READ)
		dev->info->flash_info->nor_read_mode = SPI_NOR_DUAL_READ;
#endif
	dev->info->flash_info->phy_page_ratio = 0;
	dev->info->flash_info->chip_id = flash->dev_id;

	dev_info(&pdev->dev, "mtd .name=%s .size=%llx(%lluM) .erasesize = 0x%x(%uK)\n",
			flash->mtd.name, flash->mtd.size,
			flash->mtd.size / (1024 * 1024),
			flash->mtd.erasesize, flash->mtd.erasesize / 1024);

	dev_info(&pdev->dev, "%d-bit mode @ %d Hz\n",
			(dev->info->flash_info->nor_quad_support == WR_QPP) ? \
			4 : 1, dev->info->flash_freq);

#ifndef CONFIG_OF
	if (flash_info->partitions) {
		parts = flash_info->partitions;
		count = flash_info->nr_partitions;
	}
	ret = mtd_device_parse_register(&flash->mtd, NULL, &ppdata, parts, count);
#else
	flash->mtd.dev.parent = &pdev->dev;

	flash->mtd.dev.of_node = pdev->dev.of_node;

	ret = mtd_device_register(&flash->mtd, NULL, 0);
#endif
	if (ret) {
		dev_err(&pdev->dev, "Err MTD partition=%d\n", ret);
		return ret;
	}

	dev->info->hspeed_dummy_cycle = 8;

	nvt_read_sfdp(dev->info);

	flash_copy_info(dev->info);

	return 0;
}

/**
 * nvt_flash_probe - Entry routine
 * @pdev: platform device structure
 *
 * This is the first routine which gets invoked during booting and does all
 * initialization/allocation work. The routine looks for available memory banks,
 * and do proper init for any found one.
 * Returns 0 on success, non zero otherwise
 */
static int nvt_flash_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct nvt_flash_plat_data *pdata = NULL;
	struct nvt_flash *dev;
	struct resource *nvt_flash_base;
	int ret = 0;

	nvt_spinor_check_fastboot();

	if (np) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: ERROR: no memory", __func__);
			ret = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		ret = nvt_flash_probe_config_dt(pdev, np);
		if (ret) {
			ret = -ENODEV;
			dev_err(&pdev->dev, "no platform data\n");
			goto err;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		if (!pdata) {
			ret = -ENODEV;
			dev_err(&pdev->dev, "no platform data\n");
			goto err;
		}
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "mem alloc fail\n");
		goto err;
	}

	dev->info = kzalloc(sizeof(struct drv_nand_dev_info), GFP_KERNEL);
	if (!dev->info) {
		dev_err(&pdev->dev, "failed to allocate drv_nand_dev_info\n");
		return -ENOMEM;
	}

	dev->info->flash_info = kzalloc(sizeof(struct nvt_nand_flash), GFP_KERNEL);
	if (!dev->info->flash_info) {
		dev_err(&pdev->dev, "failed to allocate nvt_nand_flash\n");
		return -ENOMEM;
	}

	dev->info->irq = platform_get_irq(pdev, 0);
	if (dev->info->irq < 0) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "invalid smi irq\n");
		goto err;
	}

	nvt_flash_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	dev->info->mmio_base = devm_ioremap_resource(&pdev->dev, nvt_flash_base);
	if (IS_ERR(dev->info->mmio_base)) {
		ret = PTR_ERR(dev->info->mmio_base);
		goto err;
	}

	dev->info->pdev = pdev;
	dev->clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(dev->clk)) {
		ret = PTR_ERR(dev->clk);
		goto err;
	} else {
		clk_prepare(dev->clk);
		clk_enable(dev->clk);
	}
	dev->info->clk = dev->clk;

	of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				&dev->info->flash_freq);

	of_property_read_u32(pdev->dev.of_node, "trace-stdtable",
				&dev->info->trace_stdtable);

	mutex_init(&dev->lock);

	init_completion(&dev->info->cmd_complete);

	ret = devm_request_irq(&pdev->dev, dev->info->irq, nvt_flash_int_handler,
				0, pdev->name, dev);
	if (ret) {
		dev_err(&pdev->dev, "NVT SPIFLASH IRQ allocation failed\n");
		goto err_irq;
	}

	nvt_flash_hw_init(dev);

	platform_set_drvdata(pdev, dev);

	ret = nvt_flash_setup(pdev, pdata->np);
	if (ret) {
		dev_err(&pdev->dev, "setup failed\n");
		goto err_setup;
	}

	return 0;

err_setup:
	platform_set_drvdata(pdev, NULL);
err_irq:
	clk_disable_unprepare(dev->clk);
err:
	return ret;
}

/**
 * nvt_flash_remove - Exit routine
 * @pdev: platform device structure
 *
 * free all allocations and delete the partitions.
 */
static int nvt_flash_remove(struct platform_device *pdev)
{
	struct nvt_flash *dev;
	struct nvt_spinor_flash *flash;
	int ret;

	dev = platform_get_drvdata(pdev);
	if (!dev) {
		dev_err(&pdev->dev, "dev is null\n");
		return -ENODEV;
	}

	/* clean up for nor flash */
	flash = dev->flash;

	/* clean up mtd stuff */
	ret = mtd_device_unregister(&flash->mtd);
	if (ret)
		dev_err(&pdev->dev, "error removing mtd\n");

	clk_disable_unprepare(dev->clk);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int nvt_flash_suspend(struct device *dev)
{
	struct nvt_flash *sdev = dev_get_drvdata(dev);

	if (sdev && sdev->clk)
		clk_disable_unprepare(sdev->clk);

	return 0;
}

static int nvt_flash_resume(struct device *dev)
{
	struct nvt_flash *sdev = dev_get_drvdata(dev);
	int ret = -EPERM;

	if (sdev && sdev->clk)
		ret = clk_prepare_enable(sdev->clk);

	if (!ret)
		nvt_flash_hw_init(sdev);
	return ret;
}

static SIMPLE_DEV_PM_OPS(nvt_flash_pm_ops, nvt_flash_suspend, nvt_flash_resume);
#endif

#ifdef CONFIG_OF
static const struct of_device_id nvt_flash_id_table[] = {
	{ .compatible = "nvt,nvt_spinor" },
	{}
};
MODULE_DEVICE_TABLE(of, nvt_flash_id_table);
#endif

static struct platform_driver nvt_flash_driver = {
	.driver = {
		.name = "spi_nor",
		.bus = &platform_bus_type,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(nvt_flash_id_table),
#endif
#ifdef CONFIG_PM
		.pm = &nvt_flash_pm_ops,
#endif
	},
	.probe = nvt_flash_probe,
	.remove = nvt_flash_remove,
};

static int __init spinor_init(void)
{
	return platform_driver_register(&nvt_flash_driver);
}

static void __exit spinor_exit(void)
{
	platform_driver_unregister(&nvt_flash_driver);
}

module_init(spinor_init);
module_exit(spinor_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Howard Chang");
MODULE_DESCRIPTION("MTD Flash driver for serial nor flash chips");
MODULE_VERSION("1.07.120");
