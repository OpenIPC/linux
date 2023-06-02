/**
    NVT check fastboot preload status
    @file       fastboot_preload.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/delay.h>
#include <linux/of.h>
#include <plat/nvt_flash.h>
#include <plat/hardware.h>
#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mutex.h>

static DEFINE_MUTEX(lock);
#define NAND_CTRL_STS_REG_OFS	0x48
#define NAND_COMP_STS           0x00001000  // bit[12]
#define NAND_SPI_CFG_REG_OFS	0x14
#define NAND_SPI_CS_MODE        0x4
#define NAND_SPI_OP_MODE        0x1

int nvt_check_preload_finish(void)
{
	void __iomem *flash_address;
	static int ret = 0;
	u32 status = 0x0, io_reg = 0x0;

	mutex_lock(&lock);
	if (ret) {
		mutex_unlock(&lock);
		return ret;
	}

	flash_address = ioremap_nocache(NVT_NAND_BASE_PHYS, 0x1000);
	if (unlikely(flash_address == 0)) {
		pr_err("%s fails: ioremap_nocache fail\n", __FUNCTION__);
		mutex_unlock(&lock);
		return -1;
	}

	do {
		status = readl(flash_address + NAND_CTRL_STS_REG_OFS);
		if (status & NAND_COMP_STS) {
			writel(status, flash_address + NAND_CTRL_STS_REG_OFS);
			io_reg = readl(flash_address + NAND_SPI_CFG_REG_OFS);
			io_reg |= (0x1 << NAND_SPI_CS_MODE);
			writel(io_reg, flash_address + NAND_SPI_CFG_REG_OFS);
			io_reg = readl(flash_address + NAND_SPI_CFG_REG_OFS);
			io_reg &=~ (0x1 << NAND_SPI_OP_MODE);
			writel(io_reg, flash_address + NAND_SPI_CFG_REG_OFS);

			ret = 1;

			break;
		}
		usleep_range(1, 2);
	} while (1);

	mutex_unlock(&lock);

	iounmap(flash_address);

	return ret;
}
EXPORT_SYMBOL(nvt_check_preload_finish);
