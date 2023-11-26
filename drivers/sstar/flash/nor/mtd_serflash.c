/*
 * mtd_serflash.c- Sigmastar
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <ms_platform.h>
#include <drvSPINOR.h>
#include <ms_msys.h>
#include <drvFSP_QSPI.h>
#include <mdrvFlashOsImpl.h>
#include <cam_os_wrapper.h>

#define FLASH_DBG 0

#define BLOCK_ERASE_SIZE 0x10000

#if FLASH_DBG
#define spi_nor_msg(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define spi_nor_debug(fmt, ...)
#else
#define spi_nor_msg(fmt, ...)   printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define spi_nor_debug(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#endif

struct serflash
{
    struct mutex    lock;
    u8              suspended;
    struct mtd_info mtd;
    void *          sni_buf;
};

struct bdma_alloc_dmem
{
    dma_addr_t  bdma_phy_addr;
    const char *DMEM_BDMA_INPUT;
    u8 *        bdma_vir_addr;
};

struct bdma_alloc_dmem _BDMA_ALLOC_DMEM = {0, "BDMA", 0};
CamOsTsem_t            flash_bdma_sem;

static inline struct serflash *mtd_to_serflash(struct mtd_info *mtd)
{
    return container_of(mtd, struct serflash, mtd);
}

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

static void free_dmem(const char *name, unsigned int size, void *virt, dma_addr_t addr)
{
    MSYS_DMEM_INFO dmem;
    memcpy(dmem.name, name, strlen(name) + 1);
    dmem.length = size;
    dmem.kvirt  = virt;
    dmem.phys   = (unsigned long long)((uintptr_t)addr);
    msys_release_dmem(&dmem);
}

void _serflash_alloc_bdma_buffer(u32 u32DataSize)
{
    if (!(_BDMA_ALLOC_DMEM.bdma_vir_addr =
              alloc_dmem(_BDMA_ALLOC_DMEM.DMEM_BDMA_INPUT, u32DataSize, &_BDMA_ALLOC_DMEM.bdma_phy_addr)))
    {
        spi_nor_msg(KERN_ERR "[SPINAND] unable to allocate bmda buffer\n");
    }
    memset(_BDMA_ALLOC_DMEM.bdma_vir_addr, 0, u32DataSize);
    printk("%s _BDMA_ALLOC_DMEM.bdma_vir_addr 0x%lx", __func__, (unsigned long)_BDMA_ALLOC_DMEM.bdma_vir_addr);
}

void _serflash_free_bdma_buffer(u32 u32DataSize)
{
    if (_BDMA_ALLOC_DMEM.bdma_vir_addr != 0)
    {
        free_dmem(_BDMA_ALLOC_DMEM.DMEM_BDMA_INPUT, u32DataSize, _BDMA_ALLOC_DMEM.bdma_vir_addr,
                  _BDMA_ALLOC_DMEM.bdma_phy_addr);
        _BDMA_ALLOC_DMEM.bdma_vir_addr = 0;
    }
}

static void serflash_release_device(struct serflash *flash)
{
    mutex_unlock(&flash->lock);
}

static int serflash_get_device(struct serflash *flash)
{
    mutex_lock(&flash->lock);
    if (flash->suspended)
    {
        mutex_unlock(&flash->lock);
        return -EBUSY;
    }

    return 0;
}

static void serflash_shutdown(struct mtd_info *mtd)
{
    struct serflash *flash = mtd_to_serflash(mtd);

    mutex_lock(&flash->lock);
    flash->suspended = 1;
    mutex_unlock(&flash->lock);
}

/* Erase flash fully or part of it */
static int serflash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct serflash *flash = mtd_to_serflash(mtd);
    uint64_t         addr_temp, len_temp;
    u8               u8_status        = 0;
    u32              u32_bytes_offset = 0;
    u32              u32_size         = 0;
    int              ret              = 0;

    /* sanity checks */
    if (!instr->len)
    {
        return 0;
    }

    /* range and alignment check */
    if (instr->addr + instr->len > mtd->size)
    {
        return -EINVAL;
    }

    addr_temp = instr->addr;
    len_temp  = instr->len;
    if ((do_div(addr_temp, mtd->erasesize) != 0) || (do_div(len_temp, mtd->erasesize) != 0))
    {
        return -EINVAL;
    }

    ret = serflash_get_device(flash);
    if (ret)
        return ret;

    u32_bytes_offset = instr->addr;
    u32_size         = instr->len;

    u8_status = mdrv_spinor_erase(u32_bytes_offset, u32_size);

    if (ERR_SPINOR_DEVICE_FAILURE == u8_status)
    {
        spi_nor_msg(KERN_ERR "[FLASH_ERR] erase fail\r\n");
        serflash_release_device(flash);
        return -EIO;
    }
    else if (ERR_SPINOR_INVALID == u8_status)
    {
        serflash_release_device(flash);
        return -EINVAL;
    }

    serflash_release_device(flash);
    return 0;
}

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int serflash_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
    struct serflash *flash           = mtd_to_serflash(mtd);
    u8               u8_status       = 0;
    u8               u8_aligne_size  = 0;
    u32              u32_aligne_addr = 0;
    u32              u32_read_size;
    int              ret = 0;

    /* sanity checks */
    if (!len)
    {
        return 0;
    }
    if (from + len > flash->mtd.size)
    {
        return -EINVAL;
    }

    ret = serflash_get_device(flash);
    if (ret)
        return ret;

    *retlen = len;

#if !defined(CONFIG_FLASH_FIX_BDMA_ALIGN)
    u8_aligne_size = from % CONFIG_FLASH_ADDR_ALIGN;
#endif

    if (u8_aligne_size)
    {
        u32_aligne_addr = from - u8_aligne_size;
        len += u8_aligne_size;
    }
    else
        u32_aligne_addr = from;

    while (0 != len)
    {
        u32_read_size = BLOCK_ERASE_SIZE;
        if (len < u32_read_size)
            u32_read_size = len;
        if (ERR_SPINOR_SUCCESS
            != (u8_status = mdrv_spinor_read(u32_aligne_addr, (u8 *)(_BDMA_ALLOC_DMEM.bdma_vir_addr), u32_read_size)))
        {
            *retlen = 0;
            serflash_release_device(flash);
            return (-EIO);
        }

        memcpy((void *)buf, (const void *)(_BDMA_ALLOC_DMEM.bdma_vir_addr + u8_aligne_size),
               (u32_read_size - u8_aligne_size));

        len -= u32_read_size;
        buf = buf + u32_read_size - u8_aligne_size;
        u32_aligne_addr += u32_read_size;
        u8_aligne_size = 0;
    }

    serflash_release_device(flash);
    return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int serflash_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
    struct serflash *flash          = mtd_to_serflash(mtd);
    u32              u32_write_size = 0;
    int              ret            = 0;

    if (retlen)
        *retlen = 0;

    /* sanity checks */
    if (!len)
    {
        return (0);
    }
    if (to + len > flash->mtd.size)
    {
        return -EINVAL;
    }

    ret = serflash_get_device(flash);
    if (ret)
        return ret;

    *retlen = len; // if success,return the input length

    while (0 != len)
    {
        u32_write_size = BLOCK_ERASE_SIZE;
        if (len < u32_write_size)
            u32_write_size = len;

        memcpy((void *)_BDMA_ALLOC_DMEM.bdma_vir_addr, (const void *)(buf), u32_write_size);
        if (ERR_SPINOR_SUCCESS != mdrv_spinor_program(to, (u8 *)(_BDMA_ALLOC_DMEM.bdma_vir_addr), u32_write_size))
        {
            *retlen = 0;
            serflash_release_device(flash);
            return (-EIO);
        }

        len -= u32_write_size;
        buf += u32_write_size;
        to += u32_write_size;
    }

    serflash_release_device(flash);
    return 0;
}

#ifdef CONFIG_CAM_CLK
#include "drv_camclk_Api.h"
void **pvSerflashclk     = NULL;
u32    SerflashParentCnt = 1;

u8 serflash_ClkDisable(void)
{
    u32 u32clknum = 0;

    for (u32clknum = 0; u32clknum < SerflashParentCnt; u32clknum++)
    {
        if (pvSerflashclk[u32clknum])
        {
            CamClkSetOnOff(pvSerflashclk[u32clknum], 0);
        }
    }
    return 1;
}
u8 serflash_ClkEnable(void)
{
    u32 u32clknum = 0;

    for (u32clknum = 0; u32clknum < SerflashParentCnt; u32clknum++)
    {
        if (pvSerflashclk[u32clknum])
        {
            CamClkSetOnOff(pvSerflashclk[u32clknum], 1);
        }
    }

    return 1;
}
u8 serflash_ClkRegister(struct device *dev)
{
    u32 u32clknum;
    u32 SerFlashClk;
    u8  str[16];

    if (of_find_property(dev->of_node, "camclk", &SerflashParentCnt))
    {
        SerflashParentCnt /= sizeof(int);
        // spi_nor_msg( "[%s] Number : %d\n", __func__, num_parents);
        if (SerflashParentCnt < 0)
        {
            spi_nor_msg("[%s] Fail to get parent count! Error Number : %d\n", __func__, SerflashParentCnt);
            return 0;
        }
        pvSerflashclk = kzalloc((sizeof(void *) * SerflashParentCnt), GFP_KERNEL);
        if (!pvSerflashclk)
        {
            return 0;
        }
        for (u32clknum = 0; u32clknum < SerflashParentCnt; u32clknum++)
        {
            SerFlashClk = 0;
            of_property_read_u32_index(dev->of_node, "camclk", u32clknum, &(SerFlashClk));
            if (!SerFlashClk)
            {
                spi_nor_msg("[%s] Fail to get clk!\n", __func__);
            }
            else
            {
                CamOsSnprintf(str, 16, "serflash_%d ", u32clknum);
                CamClkRegister(str, SerFlashClk, &(pvSerflashclk[u32clknum]));
            }
        }
    }
    else
    {
        spi_nor_msg("[%s] W/O Camclk \n", __func__);
    }
    return 1;
}
u8 serflash_ClkUnregister(void)
{
    u32 u32clknum;

    for (u32clknum = 0; u32clknum < SerflashParentCnt; u32clknum++)
    {
        if (pvSerflashclk[u32clknum])
        {
            spi_nor_msg(KERN_DEBUG "[%s] %p\n", __func__, pvSerflashclk[u32clknum]);
            CamClkUnregister(pvSerflashclk[u32clknum]);
            pvSerflashclk[u32clknum] = NULL;
        }
    }
    kfree(pvSerflashclk);

    return 1;
}
#endif

/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int serflash_probe(struct platform_device *pdev)
{
    void *           sni_buf = NULL;
    struct serflash *flash   = NULL;
    FLASH_NOR_INFO_t st_flash_nor_info;

#ifndef CONFIG_CAM_CLK
    int          num_parents;
    struct clk **spi_clks;
    int          i;
#endif
    if (Chip_Get_Storage_Type() != MS_STORAGE_NOR)
        return 0;
#ifdef CONFIG_CAM_CLK
    serflash_ClkRegister(&pdev->dev);
    serflash_ClkEnable();
#else
    num_parents = of_clk_get_parent_count(pdev->dev.of_node);
    if (num_parents > 0)
    {
        spi_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (spi_clks == NULL)
        {
            spi_nor_msg("[serflash_probe] kzalloc Fail!\n");
            return -1;
        }
        // enable all clk
        for (i = 0; i < num_parents; i++)
        {
            spi_clks[i] = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(spi_clks[i]))
            {
                spi_nor_msg("[serflash_probe] Fail to get clk!\n");
                kfree(spi_clks);
                return -1;
            }
            else
            {
                clk_prepare_enable(spi_clks[i]);
            }
            clk_put(spi_clks[i]);
        }
        kfree(spi_clks);
    }
#endif
    CamOsTsemInit(&flash_bdma_sem, 0);

    // jedec_probe() will read id, so initialize hardware first
    if (NULL == sni_buf)
    {
        sni_buf = kzalloc(FLASH_SNI_TABLE_SIZE, GFP_KERNEL);

        if (!sni_buf)
        {
            spi_nor_msg("[serflash_probe]: Failed to allocate memory!\n");
            return -ENOMEM;
        }
        // jedec_probe() will read id, so initialize hardware first
        if (mdrv_spinor_hardware_init(sni_buf))
        {
            spi_nor_msg("[serflash_probe] flash init failed!\n");
            goto out;
        }
    }

    mdrv_spinor_info(&st_flash_nor_info);

    _serflash_alloc_bdma_buffer(BLOCK_ERASE_SIZE); // for bdma read

    if (NULL == (flash = kzalloc(sizeof *flash, GFP_KERNEL)))
    {
        goto out;
    }

    mutex_init(&flash->lock);

    flash->mtd.priv         = flash;
    flash->mtd.name         = "NOR_FLASH";
    flash->mtd.type         = MTD_NORFLASH;
    flash->mtd.writesize    = 1;
    flash->mtd.writebufsize = flash->mtd.writesize;
    flash->mtd.flags        = MTD_CAP_NORFLASH;
    flash->mtd.size         = st_flash_nor_info.u32_capacity;
    flash->mtd._erase       = serflash_erase;
    flash->mtd._read        = serflash_read;
    flash->mtd._write       = serflash_write;
    flash->mtd.erasesize    = st_flash_nor_info.u32_sectorSize;
    flash->mtd._reboot      = serflash_shutdown;
    flash->sni_buf          = sni_buf;
    spi_nor_msg(KERN_DEBUG
                "mtd .name = %s, .size = 0x%.8x (%uMiB)\n"
                " .erasesize = 0x%.8x .numeraseregions = %d\n",
                flash->mtd.name, (unsigned int)flash->mtd.size, (unsigned int)flash->mtd.size / (1024 * 1024),
                (unsigned int)flash->mtd.erasesize, flash->mtd.numeraseregions);
    platform_set_drvdata(pdev, &flash->mtd);
    return mtd_device_register(&flash->mtd, NULL, 0);
out:
    if (flash)
    {
        kfree((void *)flash);
        flash = NULL;
    }

    if (sni_buf)
    {
        kfree((void *)sni_buf);
        sni_buf = NULL;
    }

    return -ENOMEM;
}

static int serflash_remove(struct platform_device *pdev)
{
    struct mtd_info *mtd;
    struct serflash *flash;
#ifdef CONFIG_CAM_CLK
    serflash_ClkDisable();
    serflash_ClkUnregister();
#else
#if defined(CONFIG_OF)
    int          num_parents, i;
    struct clk **spi_clks;

    num_parents = of_clk_get_parent_count(pdev->dev.of_node);
    if (num_parents > 0)
    {
        spi_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (spi_clks == NULL)
        {
            spi_nor_msg("[serflash_probe] kzalloc Fail!\n");
            return -1;
        }
        // disable all clk
        for (i = 0; i < num_parents; i++)
        {
            spi_clks[i] = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(spi_clks[i]))
            {
                spi_nor_msg("[serflash_cleanup] Fail to get clk!\n");
                kfree(spi_clks);
                return -1;
            }
            else
            {
                clk_disable_unprepare(spi_clks[i]);
            }
            clk_put(spi_clks[i]);
        }
        kfree(spi_clks);
    }
#endif
#endif
    mtd   = platform_get_drvdata(pdev);
    flash = mtd->priv;
    if (flash)
    {
        if (flash->sni_buf)
        {
            kfree((void *)(flash->sni_buf));
        }
        _serflash_free_bdma_buffer(BLOCK_ERASE_SIZE);
        if (flash->mtd.numeraseregions)
            kfree(flash->mtd.eraseregions);
        kfree(flash);
    }
    platform_set_drvdata(pdev, NULL);

    return 0;
}

#if defined(CONFIG_OF)
static struct of_device_id flashisp_of_device_ids[] = {
    {.compatible = "mtd-flashisp"},
    {},
};
#endif

#ifdef CONFIG_PM
static int serflash_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef CONFIG_CAM_CLK
    serflash_ClkDisable();
#else
#if defined(CONFIG_OF)
    int          num_parents, i;
    struct clk **spi_clks;

    num_parents = of_clk_get_parent_count(pdev->dev.of_node);
    if (num_parents)
    {
        spi_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (spi_clks == NULL)
        {
            spi_nor_msg("[serflash_probe] kzalloc Fail!\n");
            return -1;
        }

        // disable all clk
        for (i = 0; i < num_parents; i++)
        {
            spi_clks[i] = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(spi_clks[i]))
            {
                spi_nor_msg("[serflash_suspend] Fail to get clk!\n");
                kfree(spi_clks);
                return -1;
            }
            else
            {
                clk_disable_unprepare(spi_clks[i]);
            }
            clk_put(spi_clks[i]);
        }
        kfree(spi_clks);
    }
#endif
#endif
    return 0;
}

static int serflash_resume(struct platform_device *pdev)
{
    void *sni_buf = NULL;

#ifdef CONFIG_CAM_CLK
    serflash_ClkEnable();
#else
#if defined(CONFIG_OF)
    int          num_parents, i;
    struct clk **spi_clks;

    num_parents = of_clk_get_parent_count(pdev->dev.of_node);
    if (num_parents)
    {
        spi_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (spi_clks == NULL)
        {
            spi_nor_msg("[serflash_resume] kzalloc Fail!\n");
            return -1;
        }

        // enable all clk
        for (i = 0; i < num_parents; i++)
        {
            spi_clks[i] = of_clk_get(pdev->dev.of_node, i);
            if (IS_ERR(spi_clks[i]))
            {
                spi_nor_msg("[serflash_cleanup] Fail to get clk!\n");
                kfree(spi_clks);
                return -1;
            }
            else
            {
                clk_prepare_enable(spi_clks[i]);
            }
            clk_put(spi_clks[i]);
        }
        kfree(spi_clks);
    }
#endif
#endif

    if (NULL == sni_buf)
    {
        sni_buf = kzalloc(FLASH_SNI_TABLE_SIZE, GFP_KERNEL);

        if (!sni_buf)
        {
            spi_nor_msg("[serflash_resume]: Failed to allocate memory!\n");
            return -ENOMEM;
        }

        // jedec_probe() will read id, so initialize hardware first
        if (mdrv_spinor_hardware_init(sni_buf))
        {
            spi_nor_msg("[serflash_resume] flash init failed!\n");
            if (sni_buf)
            {
                kfree((void *)sni_buf);
                sni_buf = NULL;
                return -1;
            }
        }
    }

    return 0;
}
#endif

static struct platform_driver ms_flash_driver = {
    .probe  = serflash_probe,
    .remove = serflash_remove,
#ifdef CONFIG_PM
    .suspend = serflash_suspend,
    .resume  = serflash_resume,
#endif
    .driver =
        {
            .name = "mtd-flashisp",
#if defined(CONFIG_OF)
            .of_match_table = flashisp_of_device_ids,
#endif
            .owner = THIS_MODULE,
        },
};

module_platform_driver(ms_flash_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tao.Zhou");
MODULE_DESCRIPTION("MTD Mstar driver for spi flash chips");
