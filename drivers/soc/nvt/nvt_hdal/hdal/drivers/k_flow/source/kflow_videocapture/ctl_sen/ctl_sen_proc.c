#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ctl_sen_proc.h"
#include "ctl_sen_dbg.h"
#include "ctl_sen_main.h"
#include "ctl_sen_api.h"
#include "sen_int.h"


//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     30

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PCTL_SEN_MODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PCTL_SEN_DRV_INFO pdrv_info_data;
static int nvt_ctl_sen_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_ctl_sen_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_sen_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_ctl_sen_proc_cmd(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	} else if (len < 1) {
		nvt_dbg(ERR, "Command length is too short!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	CTL_SEN_DBG_IND("CMD:%s\n", cmd_line);

	argv[0] = CTL_SEN_PROC_NAME;
	// parse command string
	for (ucargc = 1; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (ctl_sen_cmd_execute(ucargc, &argv[0]) == 0) {
		return size;
	}

	return size;

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ctl_sen_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ctl_sen_proc_cmd
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ctl_sen_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000001 > /proc/ctl_sen/cmd ) to dump sensor (id) ctl sen get_cfg\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000002 > /proc/ctl_sen/cmd ) to dump sensor (id) sen ext info\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000004 > /proc/ctl_sen/cmd ) to dump sensor (id) sen drv info\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000008 > /proc/ctl_sen/cmd ) to dump sensor (id) vd\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000010 > /proc/ctl_sen/cmd ) to dump sensor (id) vd2\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000020 > /proc/ctl_sen/cmd ) to dump sensor (id) fmd\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000040 > /proc/ctl_sen/cmd ) to dump sensor (id) fmd2\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000080 > /proc/ctl_sen/cmd ) to dump sensor (id) map tbl\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000100 > /proc/ctl_sen/cmd ) to dump sensor (id) proc time\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00000400 > /proc/ctl_sen/cmd ) to dump sensor (id) ctl info\n");
	seq_printf(sfile, "Press ( echo dbg (id) 0x00008000 > /proc/ctl_sen/cmd ) to dump sensor (id) er\n");

	seq_printf(sfile, "Press ( echo w_reg (id) (addr) (length) (value0) (value1) > /proc/ctl_sen/cmd ) to write sensor register\n");
	seq_printf(sfile, "Press ( echo r_reg (id) (addr) (length) > /proc/ctl_sen/cmd ) to read sensor register\n");

	seq_printf(sfile, "Press ( echo dump_op_msg (id) (en) > /proc/ctl_sen/cmd ) to show sensor operation msg\n");
	seq_printf(sfile, "Press ( echo dbg_lv (lv) > /proc/ctl_sen/cmd ) set ctl sen dbg msg level\n");

	seq_printf(sfile, "=====================================================================\n");

	return 0;
}

static int nvt_ctl_sen_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_sen_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ctl_sen_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ctl_sen_proc_init(PCTL_SEN_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("ctl_sen", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_ctl_sen_proc_remove(PCTL_SEN_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
