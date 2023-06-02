/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-08-20     wangyl       add license Apache-2.0
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/pmu.h>
#include <mach/fh_chipid.h>

#define CHIP_INFO(__plat_id, __chip_id, __chip_mask, chip, size) \
    { \
        ._plat_id = __plat_id, \
        ._chip_id = __chip_id, \
        ._chip_mask = __chip_mask, \
        .chip_id = FH_CHIP_##chip, \
        .ddr_size = size, \
        .chip_name = #chip, \
    },

static struct fh_chip_info chip_infos[] = {
    CHIP_INFO(0x46488302, 0x37, 0x3F, FH8632, 512)
    CHIP_INFO(0x46488302, 0x07, 0x3F, FH8632v2, 512)
    CHIP_INFO(0x17092901, 0xC, 0xF, FH8852, 512)
    CHIP_INFO(0x17092901, 0xD, 0xF, FH8856, 1024)
    CHIP_INFO(0x18112301, 0x3, 0x3, FH8626V100, 512)
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id)
{
    unsigned int _plat_id = 0;

    _plat_id = fh_pmu_get_reg(REG_PMU_CHIP_ID);
    if (plat_id != NULL)
        *plat_id = _plat_id;

    if (chip_id != NULL)
    {
        switch (_plat_id)
        {
        case 0x46488302:
            *chip_id = fh_pmu_get_reg(REG_PMU_SPC_IO_STATUS);
            break;
        default:
            *chip_id = fh_pmu_get_reg(REG_PMU_IP_VER);
            break;
        }
    }
}
EXPORT_SYMBOL(fh_get_chipid);

#define FH_GET_CHIP_ID(plat_id, chip_id) \
    int plat_id = 0;\
    int chip_id = 0;\
    fh_get_chipid(&plat_id, &chip_id)


struct fh_chip_info *fh_get_chip_info(void)
{
    static struct fh_chip_info *chip_info = NULL;
    struct fh_chip_info *info = NULL;
    int plat_id = 0;
    int chip_id = 0;
    int i = 0;

    if (chip_info != NULL)
        return chip_info;

    fh_get_chipid(&plat_id, &chip_id);

    for (i = 0; i < ARRAY_SIZE(chip_infos); i++)
    {
        info = &chip_infos[i];
        if (plat_id == info->_plat_id && (chip_id & info->_chip_mask) == info->_chip_id)
        {
            chip_info = info;
            return info;
        }
    }

    return NULL;
}
EXPORT_SYMBOL(fh_get_chip_info);

unsigned int fh_get_ddrsize_mbit(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->ddr_size;
    return 0;
}
EXPORT_SYMBOL(fh_get_ddrsize_mbit);

char *fh_get_chipname(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return info->chip_name;
    return "UNKNOWN";
}
EXPORT_SYMBOL(fh_get_chipname);

#define DEFINE_FUNC_FH_IS(name, chip) \
unsigned int fh_is_##name(void) \
{ \
    struct fh_chip_info *info = fh_get_chip_info(); \
 \
    if (info) \
        return info->chip_id == FH_CHIP_##chip; \
    return 0; \
} \
EXPORT_SYMBOL(fh_is_##name)

unsigned int fh_is_8632(void)
{
    struct fh_chip_info *info = fh_get_chip_info();

    if (info)
        return (info->chip_id == FH_CHIP_FH8632 || \
                info->chip_id == FH_CHIP_FH8632v2);
    return 0;
}
EXPORT_SYMBOL(fh_is_8632);

DEFINE_FUNC_FH_IS(8830, FH8830);
DEFINE_FUNC_FH_IS(8852, FH8852);
DEFINE_FUNC_FH_IS(8856, FH8856);
DEFINE_FUNC_FH_IS(8626v100, FH8626V100);

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter;
    if (*pos == 0)
        return &counter;
    else {
        *pos = 0;
        return NULL;
    }
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{
    unsigned int plat_id = 0;
    unsigned int chip_id = 0;
    fh_get_chipid(&plat_id, &chip_id);

    seq_printf(sfile, "chip_name\t: %s\n", fh_get_chipname());
    seq_printf(sfile, "ddr_size\t: %dMbit\n", fh_get_ddrsize_mbit());
    seq_printf(sfile, "plat_id\t\t: 0x%x\npkg_id\t\t: 0x%x\n",
            plat_id, chip_id);
    return 0;
}

static const struct seq_operations chipid_seq_ops = {
    .start = v_seq_start,
    .next = v_seq_next,
    .stop = v_seq_stop,
    .show = v_seq_show
};

static int fh_chipid_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &chipid_seq_ops);
}

static const struct file_operations fh_chipid_proc_ops = {
    .owner = THIS_MODULE,
    .open = fh_chipid_proc_open,
    .read = seq_read,
};

#define FH_CHIPID_PROC_FILE "driver/chip"
static struct proc_dir_entry *chipid_proc_file;

int fh_chipid_init(void)
{
    chipid_proc_file = proc_create(FH_CHIPID_PROC_FILE, 0644, NULL,
            &fh_chipid_proc_ops);

    if (!chipid_proc_file) {
        pr_err("%s: ERROR: %s proc file create failed",
                __func__, "CHIP ID");
        return -EINVAL;
    }

    return 0;
}

