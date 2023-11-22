/*
 * spinand.c- Sigmastar
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
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/partitions.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <ms_platform.h>
#include <mtdcore.h>
#include <mdrvSpinandBbtBbm.h>
#include <mdrvFlashOsImpl.h>
#include <drvSPINAND.h>
#include <drvFSP_QSPI.h>
#include <ms_msys.h>
#include <generated/uapi/linux/version.h>
#include <nand/raw/internals.h>
#include <cam_os_wrapper.h>

typedef struct
{
    u32 u32_command_ctrl;
    u32 u32_r_ptr;
    u32 u32_w_ptr;
    u32 u32_buf_size;
} COMMAND_OPS;

struct bdma_alloc_dmem
{
    dma_addr_t  bdma_phy_addr;
    const char *DMEM_BDMA_INPUT;
    u8 *        bdma_vir_addr;
};

#define DRIVER_NAME "ms-spinand"

#if 1
#define spi_nand_msg(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#define spi_nand_debug(fmt, ...)
#else
#define spi_nand_msg(fmt, ...)
#define spi_nand_debug(fmt, ...) printk(KERN_NOTICE "%s: " fmt "\n", __func__, ##__VA_ARGS__)
#endif

static struct mutex             flash_lock;
static struct mtd_ooblayout_ops nand_ooblayout   = {.ecc = NULL, .free = NULL};
static void *                   g_sni_buf        = NULL;
static void *                   g_bbt_buf        = NULL;
static void *                   g_socecc_cache   = NULL;
static COMMAND_OPS              g_command_ops    = {0, 0, 0};
struct bdma_alloc_dmem          _BDMA_ALLOC_DMEM = {0, "BDMA", 0};
CamOsTsem_t                     flash_bdma_sem;

#if 0
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

static void spi_nand_alloc_bdma_buffer(u32 u32DataSize)
{
    if (!(_BDMA_ALLOC_DMEM.bdma_vir_addr =
              alloc_dmem(_BDMA_ALLOC_DMEM.DMEM_BDMA_INPUT, u32DataSize, &_BDMA_ALLOC_DMEM.bdma_phy_addr)))
    {
        spi_nand_msg(KERN_ERR "[SPINAND] unable to allocate bmda buffer\n");
    }
    // memset(_BDMA_ALLOC_DMEM.bdma_vir_addr, 0, u32DataSize);

    _BDMA_ALLOC_DMEM.bdma_vir_addr = (u8 *)kzalloc(u32DataSize, GFP_KERNEL);
}

static void spi_nand_free_bdma_buffer(u32 u32DataSize)
{
    if (_BDMA_ALLOC_DMEM.bdma_vir_addr != 0)
    {
        free_dmem(_BDMA_ALLOC_DMEM.DMEM_BDMA_INPUT, u32DataSize, _BDMA_ALLOC_DMEM.bdma_vir_addr,
                  _BDMA_ALLOC_DMEM.bdma_phy_addr);
        _BDMA_ALLOC_DMEM.bdma_vir_addr = 0;
    }
}
#else
static void spi_nand_alloc_bdma_buffer(u32 u32DataSize)
{
    _BDMA_ALLOC_DMEM.bdma_vir_addr = (u8 *)kzalloc(u32DataSize, GFP_KERNEL);
}

static void spi_nand_free_bdma_buffer(u32 u32DataSize)
{
    kfree((void *)(_BDMA_ALLOC_DMEM.bdma_vir_addr));
    _BDMA_ALLOC_DMEM.bdma_vir_addr = NULL;
}
#endif

static u8 spi_nand_read_byte(struct nand_chip *chip)
{
    u8  u8_byte;
    u8 *pu8_data;

    mutex_lock(&flash_lock);

    pu8_data = _BDMA_ALLOC_DMEM.bdma_vir_addr;

    u8_byte = pu8_data[g_command_ops.u32_r_ptr];

    g_command_ops.u32_r_ptr += 1;

    if (g_command_ops.u32_r_ptr == g_command_ops.u32_w_ptr)
        g_command_ops.u32_r_ptr = 0;

    mutex_unlock(&flash_lock);
    return u8_byte;
}

static void spi_nand_read_buf(struct nand_chip *chip, u8 *buf, int len)
{
    u32 u32_read_byte;

    mutex_lock(&flash_lock);

    while (len != 0)
    {
        u32_read_byte = (g_command_ops.u32_w_ptr - g_command_ops.u32_r_ptr);
        u32_read_byte = (u32_read_byte > len) ? len : u32_read_byte;

        memcpy((void *)buf, (const void *)(_BDMA_ALLOC_DMEM.bdma_vir_addr + g_command_ops.u32_r_ptr), u32_read_byte);

        len -= u32_read_byte;
        buf += u32_read_byte;
        g_command_ops.u32_r_ptr += u32_read_byte;

        if (g_command_ops.u32_r_ptr == g_command_ops.u32_w_ptr)
            g_command_ops.u32_r_ptr = 0;
    }
    mutex_unlock(&flash_lock);
}

static void spi_nand_select_chip(struct nand_chip *chip, int cs)
{
    spi_nand_debug("spi_nand_select_chip  Not support\r\n");
}

static void spi_nand_cmd_ctrl(struct nand_chip *chip, int dat, unsigned int ctrl)
{
    spi_nand_debug("spi_nand_cmd_ctrl Not support\r\n");
}

static int spi_nand_dev_ready(struct nand_chip *chip)
{
    spi_nand_debug("spi_nand_dev_ready Not support\r\n");

    return 1;
}

static void spi_nand_cmdfunc(struct nand_chip *chip, unsigned command, int column, int page_addr)
{
    struct mtd_info *mtd = nand_to_mtd(chip);

    mutex_lock(&flash_lock);

    g_command_ops.u32_r_ptr = 0;

    switch (command)
    {
        case NAND_CMD_STATUS:
            spi_nand_debug("NAND_CMD_STATUS");
            g_command_ops.u32_command_ctrl = NAND_CMD_STATUS;
            g_command_ops.u32_w_ptr        = 1;
            mdrv_spinand_read_status(_BDMA_ALLOC_DMEM.bdma_vir_addr);
            break;

        case NAND_CMD_READOOB:
            spi_nand_debug("NAND_CMD_READOOB %d", column);
            g_command_ops.u32_command_ctrl = NAND_CMD_READOOB;
            g_command_ops.u32_w_ptr        = 1;
            mdrv_spinand_page_read(page_addr, (u16)(column + mtd->writesize), _BDMA_ALLOC_DMEM.bdma_vir_addr, 1);
            break;

        case NAND_CMD_READID:
            spi_nand_debug("NAND_CMD_READID");
            g_command_ops.u32_command_ctrl = NAND_CMD_READID;
            g_command_ops.u32_w_ptr        = 6;
            mdrv_spinand_read_id(_BDMA_ALLOC_DMEM.bdma_vir_addr, 6);
            break;

        case NAND_CMD_ERASE2:
            spi_nand_debug("NAND_CMD_ERASE2");
            break;

        case NAND_CMD_ERASE1:
            spi_nand_debug("NAND_CMD_ERASE1, page_addr: 0x%x", page_addr);
            mdrv_spinand_block_erase(page_addr);
            break;

        case NAND_CMD_RESET:
            spi_nand_debug("NAND_CMD_RESET");
            mdrv_spinand_reset();
            mdrv_spinand_setup();
            break;

        default:
            printk("unsupported command %02Xh", command);
            break;
    }
    mutex_unlock(&flash_lock);
    return;
}

static int spi_nand_waitfunc(struct nand_chip *chip)
{
    u8 u8_status;

    mutex_lock(&flash_lock);

    spi_nand_debug("spi_nand_waitfunc\r\n");

    u8_status = mdrv_spinand_read_status(NULL);

    mutex_unlock(&flash_lock);
    return (ERR_SPINAND_E_FAIL > u8_status) ? NAND_STATUS_READY : NAND_STATUS_FAIL;
}

static void spi_nand_ecc_hwctl(struct nand_chip *chip, int mode)
{
    spi_nand_debug(" spi_nand_ecc_hwctl Not support");
}

static int spi_nand_ecc_calculate(struct nand_chip *chip, const uint8_t *dat, uint8_t *ecc_code)
{
    spi_nand_debug("spi_nand_ecc_calculate Not support");
    return 0;
}

static int spi_nand_ecc_correct(struct nand_chip *chip, uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
    spi_nand_debug(" spi_nand_ecc_correct Not support");
    return 0;
}

static int spi_nand_ecc_read_page_raw(struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;
    u32              u32_read_size;

    mutex_lock(&flash_lock);

    pu8_data      = _BDMA_ALLOC_DMEM.bdma_vir_addr;
    u32_read_size = oob_required ? (mtd->writesize + mtd->oobsize) : mtd->writesize;

    if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read_raw(page, 0, pu8_data, u32_read_size))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    memcpy((void *)buf, (const void *)pu8_data, mtd->writesize);
    if (oob_required)
    {
        memcpy((void *)(chip->oob_poi), (const void *)(pu8_data + mtd->writesize), mtd->oobsize);
    }

    mutex_unlock(&flash_lock);
    return 0;
}

static int spi_nand_ecc_write_page_raw(struct nand_chip *chip, const uint8_t *buf, int oob_required, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;
    u32              u32_write_size;

    mutex_lock(&flash_lock);
    spi_nand_debug("spi_nand_ecc_write_page_raw\r\n");

    pu8_data       = _BDMA_ALLOC_DMEM.bdma_vir_addr;
    u32_write_size = oob_required ? (mtd->writesize + mtd->oobsize) : mtd->writesize;

    memcpy((void *)pu8_data, (const void *)buf, mtd->writesize);
    if (oob_required)
        memcpy((void *)(pu8_data + mtd->writesize), (const void *)(chip->oob_poi), mtd->oobsize);

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_page_program_raw(page, 0, pu8_data, u32_write_size))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return 0;
}

static int spi_nand_ecc_read_page(struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8               u8_status;
    u8 *             pu8_data;
    u32              u32_read_size;

    mutex_lock(&flash_lock);

    pu8_data      = _BDMA_ALLOC_DMEM.bdma_vir_addr;
    u32_read_size = oob_required ? (mtd->writesize + mtd->oobsize) : mtd->writesize;

    u8_status = mdrv_spinand_page_read(page, 0, pu8_data, u32_read_size);

    memcpy((void *)buf, (const void *)pu8_data, mtd->writesize);
    if (oob_required)
        memcpy((void *)(chip->oob_poi), (const void *)(pu8_data + mtd->writesize), mtd->oobsize);

    if (ERR_SPINAND_ECC_NOT_CORRECTED == u8_status)
    {
        mtd->ecc_stats.failed++;
        spi_nand_msg("ecc failed");
    }
    else if (ERR_SPINAND_ECC_CORRECTED == u8_status)
        mtd->ecc_stats.corrected++;
    else if (ERR_SPINAND_TIMEOUT <= u8_status)
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return (ERR_SPINAND_ECC_CORRECTED == u8_status) ? 1 : 0;
}

static int spi_nand_ecc_read_subpage(struct nand_chip *chip, uint32_t offs, uint32_t len, uint8_t *buf, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8               u8_status;
    u8 *             pu8_data;

    mutex_lock(&flash_lock);
    spi_nand_debug("page = 0x%x, offs = 0x%x, len = 0x%x", page, offs, len);

    pu8_data = _BDMA_ALLOC_DMEM.bdma_vir_addr;

    u8_status = mdrv_spinand_page_read(page, (u16)offs, pu8_data, len);

    memcpy((void *)(buf + offs), (const void *)pu8_data, len);

    if (ERR_SPINAND_ECC_NOT_CORRECTED == u8_status)
    {
        mtd->ecc_stats.failed++;
        spi_nand_msg("ecc failed");
    }
    else if (ERR_SPINAND_ECC_CORRECTED == u8_status)
        mtd->ecc_stats.corrected++;
    else if (ERR_SPINAND_TIMEOUT <= u8_status)
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return (ERR_SPINAND_ECC_CORRECTED == u8_status) ? 1 : 0;
}

static int spi_nand_ecc_write_subpage(struct nand_chip *chip, uint32_t offset, uint32_t data_len,
                                      const uint8_t *data_buf, int oob_required, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;
    u32              u32_write_size;

    mutex_lock(&flash_lock);

    spi_nand_debug("spi_nand_ecc_write_subpage\r\n");

    pu8_data       = _BDMA_ALLOC_DMEM.bdma_vir_addr;
    u32_write_size = oob_required ? (data_len + mtd->oobsize) : data_len;

    memcpy((void *)pu8_data, (const void *)data_buf, data_len);
    if (oob_required)
        memcpy((void *)(pu8_data + data_len), (const void *)(chip->oob_poi), mtd->oobsize);

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_page_program(page, (u16)offset, pu8_data, u32_write_size))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return 0;
}

static int spi_nand_ecc_write_page(struct nand_chip *chip, const uint8_t *buf, int oob_required, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;
    u32              u32_write_size;

    mutex_lock(&flash_lock);
    spi_nand_debug("spi_nand_ecc_write_page\r\n");

    pu8_data       = _BDMA_ALLOC_DMEM.bdma_vir_addr;
    u32_write_size = oob_required ? (mtd->writesize + mtd->oobsize) : mtd->writesize;

    memcpy((void *)pu8_data, (const void *)buf, mtd->writesize);
    if (oob_required)
        memcpy((void *)(pu8_data + mtd->writesize), (const void *)(chip->oob_poi), mtd->oobsize);

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_page_program(page, 0, pu8_data, u32_write_size))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return 0;
}

static int spi_nand_read_oob_std(struct nand_chip *chip, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;

    mutex_lock(&flash_lock);

    pu8_data = _BDMA_ALLOC_DMEM.bdma_vir_addr;

    if (ERR_SPINAND_TIMEOUT <= mdrv_spinand_page_read(page, (u16)(mtd->writesize), pu8_data, mtd->oobsize))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    memcpy((void *)(chip->oob_poi), (const void *)pu8_data, mtd->oobsize);

    mutex_unlock(&flash_lock);
    return 0;
}

static int spi_nand_write_oob_std(struct nand_chip *chip, int page)
{
    struct mtd_info *mtd = nand_to_mtd(chip);
    u8 *             pu8_data;

    mutex_lock(&flash_lock);

    pu8_data = _BDMA_ALLOC_DMEM.bdma_vir_addr;

    memcpy((void *)pu8_data, (const void *)chip->oob_poi, mtd->oobsize);

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_page_program(page, (u16)(mtd->writesize), pu8_data, mtd->oobsize))
    {
        mutex_unlock(&flash_lock);
        return -EIO;
    }

    mutex_unlock(&flash_lock);
    return 0;
}

static int nand_get_fact_prot_info(struct mtd_info *mtd, size_t len, size_t *retlen, struct otp_info *buf)
{
    unsigned int start, length;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 0))
        return -EIO;

    *retlen = 0;

    if (length)
    {
        buf[0].start  = start;
        buf[0].length = length;
        buf[0].locked = 1;
        *retlen       = sizeof(struct otp_info);
    }

    return 0;
}

static int nand_get_user_prot_info(struct mtd_info *mtd, size_t len, size_t *retlen, struct otp_info *buf)
{
    unsigned int start, length;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 1))
        return -EIO;

    mutex_lock(&flash_lock);
    *retlen = 0;

    if (length)
    {
        buf[0].start  = start;
        buf[0].length = length;
        buf[0].locked = mdrv_spinand_get_otp_lock();
        *retlen       = sizeof(struct otp_info);
    }
    mutex_unlock(&flash_lock);

    return 0;
}

static int nand_read_fact_prot_reg(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
    unsigned int start, length, end;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 0))
        return -EIO;

    if (buf == NULL)
        return ERR_SPINAND_SUCCESS;

    end = start + length;
    if (from < start || from >= end)
    {
        *retlen = 0;
        return -EFAULT;
    }

    len = min_t(size_t, end - from, len);

    mutex_lock(&flash_lock);
    *retlen = mdrv_spinand_read_otp(from / mtd->writesize, from % mtd->writesize, buf, len);
    mutex_unlock(&flash_lock);

    return 0;
}

static int nand_read_user_prot_reg(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
    unsigned int start, length, end;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 1))
        return -EIO;

    if (buf == NULL)
        return 0;

    end = start + length;
    if (from < start || from >= end)
    {
        *retlen = 0;
        return -EFAULT;
    }

    len = min_t(size_t, end - from, len);

    mutex_lock(&flash_lock);
    *retlen = mdrv_spinand_read_otp(from / mtd->writesize, from % mtd->writesize, buf, len);
    mutex_unlock(&flash_lock);

    return 0;
}

static int nand_write_user_prot_reg(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, u_char *buf)
{
    unsigned int start, length, end;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 1))
        return -EIO;

    if (buf == NULL)
        return 0;

    end = start + length;
    if (to < start || to >= end)
    {
        *retlen = 0;
        return -EFAULT;
    }

    len = min_t(size_t, end - to, len);

    mutex_lock(&flash_lock);
    *retlen = mdrv_spinand_write_otp(to / mtd->writesize, to % mtd->writesize, buf, len);
    mutex_unlock(&flash_lock);

    return 0;
}

static int nand_lock_user_prot_reg(struct mtd_info *mtd, loff_t from, size_t len)
{
    int          ret;
    unsigned int start, length;

    if (ERR_SPINAND_SUCCESS != mdrv_spinand_get_otp_layout(&start, &length, 1))
        return -EIO;

    if (from != start || len != length)
    {
        return -EFAULT;
    }

    mutex_lock(&flash_lock);
    ret = mdrv_spinand_set_otp_lock();
    mutex_unlock(&flash_lock);

    return (ret != ERR_SPINAND_SUCCESS) ? -EIO : ret;
}

static int sstar_nand_attach_chip(struct nand_chip *chip)
{
    /*The nand_scan_ident will change the value of bits_per_cell,
    so we should change it back for the flow of nand_scan_tail*/
    if (chip)
        chip->base.memorg.bits_per_cell = 1;
    else
        return -EIO;

    return 0;
}

static const struct nand_controller_ops sstar_nand_controller_ops = {
    .attach_chip = sstar_nand_attach_chip,
};

static int sstar_spinand_probe(struct platform_device *pdev)
{
    u8  u8_dev_id      = 0;
    u32 u32_flash_size = 0;

    struct nand_flash_dev *type = NULL;
    struct nand_chip *     nand = NULL;
    struct mtd_info *      mtd  = NULL;
    FLASH_NAND_INFO_t      st_flash_nand_info;

    if (Chip_Get_Storage_Type() != MS_STORAGE_SPINAND_ECC)
        return 0;

    do
    {
        if (NULL == g_sni_buf)
        {
            if (NULL == (g_sni_buf = kzalloc(FLASH_SNI_TABLE_SIZE, GFP_KERNEL)))
            {
                break;
            }
        }

        if (NULL == g_bbt_buf)
        {
            if (NULL == (g_bbt_buf = kzalloc(MAX_BBT_TBL_SIZE, GFP_KERNEL)))
            {
                break;
            }
        }

        if (mdrv_spinand_is_socecc() && (NULL == g_socecc_cache))
        {
            if (NULL == (g_socecc_cache = kzalloc(FLASH_SOC_ECC_CACHE_SIZE, GFP_KERNEL)))
            {
                break;
            }

            mdrv_spinand_socecc_init(NULL, (u8 *)g_socecc_cache);
        }

        if (NULL == (nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL)))
        {
            break;
        }

        mtd       = nand_to_mtd(nand);
        mtd->priv = nand;
        CamOsTsemInit(&flash_bdma_sem, 0);

        if (ERR_SPINAND_SUCCESS != mdrv_spinand_hardware_init((u8 *)g_sni_buf))
        {
            break;
        }

        MDRV_BBT_init((u8 *)g_bbt_buf);

        mdrv_spinand_info(&st_flash_nand_info);

        u32_flash_size = st_flash_nand_info.u32_Capacity;

        u8_dev_id = 0xEE;

        for (type = nand_flash_ids; type->name != NULL; type++)
        {
            if (u8_dev_id == type->dev_id)
            {
                printk("[FLASH] dev_id = 0x%x\r\n", type->dev_id);
                type->mfr_id = st_flash_nand_info.au8_ID[0];
                type->dev_id = st_flash_nand_info.au8_ID[1];
                type->id_len = st_flash_nand_info.u8_IDByteCnt;
                strncpy(type->id, st_flash_nand_info.au8_ID, st_flash_nand_info.u8_IDByteCnt);
                type->chipsize        = u32_flash_size >> 20;
                type->pagesize        = st_flash_nand_info.u16_PageSize;
                type->oobsize         = st_flash_nand_info.u16_OobSize;
                type->erasesize       = st_flash_nand_info.u32_BlockSize;
                type->ecc.strength_ds = st_flash_nand_info.u16_SectorSize;
                type->ecc.step_ds     = st_flash_nand_info.u16_PageSize / st_flash_nand_info.u16_SectorSize;
                spi_nand_alloc_bdma_buffer(type->pagesize + type->oobsize);
                printk("[FLASH] mfr_id = 0x%x, dev_id= 0x%x id_len = 0x%x\r\n", type->id[0], type->id[1], type->id_len);
                break;
            }
        }

        mutex_init(&flash_lock);

        nand->options          = NAND_BROKEN_XD | NAND_SKIP_BBTSCAN | NAND_SUBPAGE_READ | NAND_NO_SUBPAGE_WRITE;
        nand->legacy.read_byte = spi_nand_read_byte;
        // nand->read_word = spi_nand_read_word;
        nand->legacy.read_buf             = spi_nand_read_buf;
        nand->legacy.select_chip          = spi_nand_select_chip;
        nand->legacy.cmd_ctrl             = spi_nand_cmd_ctrl;
        nand->legacy.dev_ready            = spi_nand_dev_ready;
        nand->legacy.cmdfunc              = spi_nand_cmdfunc;
        nand->legacy.waitfunc             = spi_nand_waitfunc;
        nand->legacy.dummy_controller.ops = &sstar_nand_controller_ops;
        nand->legacy.chip_delay           = 0;
        nand->ecc.engine_type             = NAND_ECC_ENGINE_TYPE_ON_DIE;
        nand->bbt_options                 = NAND_BBT_USE_FLASH;
        nand->ecc.size                    = st_flash_nand_info.u16_SectorSize;
        nand->ecc.steps                   = st_flash_nand_info.u16_PageSize / nand->ecc.size;
        nand->ecc.strength                = nand->ecc.steps;
        nand->ecc.hwctl                   = spi_nand_ecc_hwctl;
        nand->ecc.calculate               = spi_nand_ecc_calculate;
        nand->ecc.correct                 = spi_nand_ecc_correct;
        nand->ecc.read_page_raw           = spi_nand_ecc_read_page_raw;
        nand->ecc.write_page_raw          = spi_nand_ecc_write_page_raw;
        nand->ecc.read_page               = spi_nand_ecc_read_page;
        nand->ecc.read_subpage            = spi_nand_ecc_read_subpage;
        nand->ecc.write_page              = spi_nand_ecc_write_page;
        nand->ecc.write_subpage           = spi_nand_ecc_write_subpage;
        nand->ecc.read_oob                = spi_nand_read_oob_std;
        nand->ecc.write_oob               = spi_nand_write_oob_std;
        nand->priv                        = NULL;
        if (!mdrv_spinand_is_support_otp())
        {
            mtd->_get_fact_prot_info  = nand_get_fact_prot_info;
            mtd->_read_fact_prot_reg  = nand_read_fact_prot_reg;
            mtd->_get_user_prot_info  = nand_get_user_prot_info;
            mtd->_read_user_prot_reg  = nand_read_user_prot_reg;
            mtd->_write_user_prot_reg = nand_write_user_prot_reg;
            mtd->_lock_user_prot_reg  = nand_lock_user_prot_reg;
        }
        mtd->ooblayout         = &nand_ooblayout;
        mtd->bitflip_threshold = 0xFF;
        if (!mdrv_spinand_is_support_sniecc() && !mdrv_spinand_is_support_ubibbm())
        {
            mtd->bitflip_threshold = 1;
        }
        mtd->name  = "nand0";
        mtd->owner = THIS_MODULE;

        if (0 != nand_scan(nand, 1))
        {
            break;
        }

        mtd->ooblayout = NULL;
        platform_set_drvdata(pdev, mtd);

        return mtd_device_register(mtd, NULL, 0);
    } while (0);

    if (g_sni_buf)
    {
        kfree((void *)g_sni_buf);
        g_sni_buf = NULL;
    }

    if (g_bbt_buf)
    {
        kfree((void *)g_bbt_buf);
        g_bbt_buf = NULL;
    }

    if (g_socecc_cache)
    {
        kfree((void *)g_socecc_cache);
        g_socecc_cache = NULL;
    }

    if (nand)
    {
        kfree(nand);
    }

    return -1;
}

static int sstar_spinand_remove(struct platform_device *pdev)
{
    struct nand_chip *nand;
    struct mtd_info * mtd;

    mtd  = platform_get_drvdata(pdev);
    nand = mtd->priv;

    if (mtd)
    {
        spi_nand_free_bdma_buffer(mtd->erasesize + mtd->oobsize);
    }

    mutex_destroy(&flash_lock);

    if (g_sni_buf)
    {
        kfree((void *)g_sni_buf);
        g_sni_buf = NULL;
    }

    if (g_bbt_buf)
    {
        kfree((void *)g_bbt_buf);
        g_bbt_buf = NULL;
    }

    if (g_socecc_cache)
    {
        kfree((void *)g_socecc_cache);
        g_socecc_cache = NULL;
    }

    if (nand)
    {
        kfree(nand);
    }

    platform_set_drvdata(pdev, NULL);
    return 0;
}

#ifdef CONFIG_PM
static int sstar_spinand_suspend(struct platform_device *pdev, pm_message_t state)
{
    spi_nand_debug("%s:%d enter \n", __func__, __LINE__);
    return 0;
}

static int sstar_spinand_resume(struct platform_device *pdev)
{
    spi_nand_debug("%s:%d enter \n", __func__, __LINE__);
    DRV_FSP_QSPI_init();
#if defined(CONFIG_FLASH_HW_CS)
    DRV_QSPI_use_sw_cs(0);
    DRV_QSPI_set_timeout(1, 0);
#endif
    mdrv_spinand_reset();
    mdrv_spinand_setup();
    return 0;
}
#endif

static const struct of_device_id spinand_of_dt_ids[] = {{.compatible = "ms-spinand"}, {/* sentinel */}};
MODULE_DEVICE_TABLE(of, spinand_of_dt_ids);

static struct platform_driver sstar_spinand_driver = {
    .probe  = sstar_spinand_probe,
    .remove = sstar_spinand_remove,
#ifdef CONFIG_PM
    .suspend = sstar_spinand_suspend,
    .resume  = sstar_spinand_resume,
#endif
    .driver =
        {
            .name           = DRIVER_NAME,
            .owner          = THIS_MODULE,
            .of_match_table = (spinand_of_dt_ids),
        },
};
module_platform_driver(sstar_spinand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("Sstar MTD SPI NAND driver");
