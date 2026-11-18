// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <generated/compile.h>

const char ax_proc_banner[] =
	"Ax_Version "
	BUILD_AXVERSION "\n";

static int version_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, linux_proc_banner,
		utsname()->sysname,
		utsname()->release,
		utsname()->version);
	return 0;
}

static int ax_version_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, ax_proc_banner);
	return 0;
}


static int __init proc_version_init(void)
{
	proc_create_single("version", 0, NULL, version_proc_show);
	proc_create_single("ax_proc/version", 0, NULL, ax_version_proc_show);
	return 0;
}
fs_initcall(proc_version_init);
