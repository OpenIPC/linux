#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ctl_sie_proc.h"
#include "ctl_sie_dbg.h"
#include "ctl_sie_main.h"
#include "ctl_sie_api.h"
#include "ctl_sie_debug_int.h"
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     30 //10

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PMODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PCTL_SIE_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static int nvt_ctl_sie_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int nvt_ctl_sie_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, nvt_ctl_sie_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_ctl_sie_proc_cmd(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		DBG_ERR("Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len == 0) {
		cmd_line[0] = '\0';
	} else {
		cmd_line[len - 1] = '\0';
	}

	DBG_IND("CMD:%s\n", cmd_line);

	argv[0] = "ctl_sie";
	// parse command string
	for (ucargc = 1; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}
	if (ctl_sie_cmd_execute(ucargc, &argv[0]) == 0) {
		return size;
	}

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ctl_sie_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ctl_sie_proc_cmd
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ctl_sie_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=============================cat====================================\n");
	seq_printf(sfile, "info: for dump kflow debug info\n");
	seq_printf(sfile, "=============================Cmd====================================\n");
	seq_printf(sfile, "Press ( echo savemem addr w h > /proc/nvt_ctl_sie/cmd ) to dump memory to file sys.\n");
	seq_printf(sfile, "Press ( echo saveraw 0 > /proc/nvt_ctl_sie/cmd ) to dump id 0 raw image to file sys.\n");
	seq_printf(sfile, "Press ( echo dbglevel 1 > /proc/nvt_ctl_sie/cmd ) to enable kflow error msg, leveltype plz. refer. to CTL_SIE_DBG_LVL\n");
	seq_printf(sfile, "Press ( echo isp_cb_thr 0 1000 > /proc/nvt_ctl_sie/cmd ) to dump id 0 isp_cp_fp process time over than 1000 usec\n");
	seq_printf(sfile, "=============================dbgtype====================================\n");
	seq_printf(sfile, "Press ( echo dbgtype id type > /proc/nvt_ctl_sie/cmd ) to dump dbg info, id: 0~7, type: plz refer. to CTL_SIE_DBG_MSG_TYPE\n");
	seq_printf(sfile, "      type 0: disable\n");
	seq_printf(sfile, "      type 1: ctl sie info\n");
	seq_printf(sfile, "      type 2: process time\n");
	seq_printf(sfile, "      type 3: buffer io msg lite\n");
	seq_printf(sfile, "      type 4: buffer io msg full\n");
	seq_printf(sfile, "      type 5: problem step record\n");
	seq_printf(sfile, "      type 6: all dbg msg\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_ctl_sie_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_sie_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ctl_sie_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static int nvt_ctl_sie_seq_printf(const char *fmtstr, ...)
{
	va_list marker;

	va_start(marker, fmtstr);
	seq_vprintf(pdrv_info_data->info_seq_file, fmtstr, marker);
	va_end(marker);

	return 0;
}

static int nvt_ctl_sie_proc_info_show(struct seq_file *sfile, void *v)
{
	pdrv_info_data->info_seq_file = sfile;
	ctl_sie_dbg_dump_info(nvt_ctl_sie_seq_printf);
	ctl_sie_dbg_dump_ts(0, nvt_ctl_sie_seq_printf);
	pdrv_info_data->info_seq_file = NULL;
	return 0;
}

static int nvt_ctl_sie_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_sie_proc_info_show, NULL);
}

static struct file_operations proc_info_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ctl_sie_proc_info_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ctl_sie_proc_init(PCTL_SIE_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir(MODULE_NAME, NULL);
	if (pmodule_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;

	pentry = proc_create("info", S_IRUGO | S_IXUGO, pmodule_root, &proc_info_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc info!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_info_entry = pentry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_ctl_sie_proc_remove(PCTL_SIE_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
