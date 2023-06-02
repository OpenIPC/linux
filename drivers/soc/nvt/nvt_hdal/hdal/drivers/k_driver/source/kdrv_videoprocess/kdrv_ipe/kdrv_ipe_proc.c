#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kdrv_ipe_int_proc.h"
#include "kdrv_ipe_int_dbg.h"
#include "kdrv_ipe_int_main.h"
#include "kdrv_ipe_int_api.h"

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     10

//============================================================================
// Declaration
//============================================================================

//============================================================================
// Global variable
//============================================================================
static PNVT_KDRV_IPE_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================

static int nvt_kdrv_ipe_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_kdrv_ipe_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_kdrv_ipe_proc_cmd_show, NULL);
}

static ssize_t nvt_kdrv_ipe_proc_cmd_write(struct file *file, const char __user *buf,
									  size_t size, loff_t *off)
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
	if (copy_from_user(cmd_line, buf, len) != 0) {
		goto ERR_OUT;
	}

	if (len > 0) {
		cmd_line[len - 1] = '\0';
	}

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		kdrv_ipe_cmd_execute(ucargc - 1, &argv[1]);
	} else if (strncmp(argv[0], "w", 2) == 0)  {
		kdrv_ipe_cmd_execute(ucargc - 1, &argv[1]);
	} else {
		kdrv_ipe_cmd_execute(ucargc, &argv[0]);
	}

	return size;

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_kdrv_ipe_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_kdrv_ipe_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_kdrv_ipe_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_kdrv_ipe_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_kdrv_ipe_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_kdrv_ipe_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_kdrv_ipe_proc_init(PNVT_KDRV_IPE_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("kdrv_ipe", NULL);
	if (pmodule_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_root = pmodule_root;


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


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_root);
	return ret;
}

int nvt_kdrv_ipe_proc_remove(PNVT_KDRV_IPE_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_root);
	pdrv_info_data = NULL;

	return 0;
}
