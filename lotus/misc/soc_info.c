
// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <mach/platform.h>
#include <linux/lotus/soc_chip.h>

#define _SOC_INFO_PROC_NAME "soc_info"

static int proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ChipID      : 0x%8X\n", get_chipid());
	seq_printf(m, "CpuFreq     : %d\n", get_cpu_freq());

	return 0;
}

static int __init lotus_soc_info_init(void)
{
	pr_info("ChipID: 0x%8X\n", get_chipid());
	pr_info("CpuFreq: %d\n", get_cpu_freq());

	if (!proc_create_single(_SOC_INFO_PROC_NAME, 0, NULL, proc_show)) {
		pr_err("Fail to create proc "_SOC_INFO_PROC_NAME"!\n");
		return -ENOMEM;
	}

	return 0;
}

early_initcall(lotus_soc_info_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lotus");
