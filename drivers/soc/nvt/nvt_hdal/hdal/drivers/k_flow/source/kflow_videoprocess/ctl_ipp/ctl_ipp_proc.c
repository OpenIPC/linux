#if defined(__LINUX)
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#elif defined(__FREERTOS)
#endif
#include "ctl_ipp_proc.h"
#include "ctl_ipp_dbg.h"
#include "ctl_ipp_main.h"
#include "ctl_ipp_api.h"
#include "ipp_debug_int.h"
#include "ipp_event_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_flow_task_int.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_int.h"
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     10

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PMODULE_INFO pdrv, unsigned char argc, char  **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PCTL_IPP_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================

static int nvt_ctl_ipp_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_ctl_ipp_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_ipp_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_ctl_ipp_proc_cmd_write(struct file *file, const char __user *buf,
											size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len == 0 || len > (MAX_CMD_LENGTH - 1)) {
		CTL_IPP_DBG_ERR("Command length is too long or 0!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len))
	    goto ERR_OUT;

	cmd_line[len - 1] = '\0';

	CTL_IPP_DBG_TRC("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
	    argv[ucargc] = strsep(&cmdstr, delimiters);

	    if (argv[ucargc] == NULL)
			break;
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		ctl_ipp_cmd_execute(ucargc - 1, &argv[1]);
	} else if (strncmp(argv[0], "w", 2) == 0)  {
		ctl_ipp_cmd_execute(ucargc - 1, &argv[1]);
	} else {
		ctl_ipp_cmd_execute(ucargc, &argv[0]);
	}

	return size;

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ctl_ipp_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ctl_ipp_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ctl_ipp_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " ctl ipp message\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_ctl_ipp_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_ipp_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ctl_ipp_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static int nvt_ctl_ipp_seq_printf(const char *fmtstr, ...)
{
	va_list marker;

	va_start(marker, fmtstr);
	seq_vprintf(pdrv_info_data->info_seq_file, fmtstr, marker);
	va_end(marker);

	return 0;
}

static int nvt_ctl_ipp_proc_info_show(struct seq_file *sfile, void *v)
{
	pdrv_info_data->info_seq_file = sfile;

	ctl_ipp_dbg_hdl_dump_all(nvt_ctl_ipp_seq_printf);
	ctl_ipp_buf_msg_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_ise_dumpinfo(nvt_ctl_ipp_seq_printf);
	ctl_ipp_isp_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_info_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_dbg_ctxbuf_log_dump(nvt_ctl_ipp_seq_printf);

	ctl_ipp_dbg_ts_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_dbg_inbuf_cbtime_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_dbg_outbuf_cbtime_dump(nvt_ctl_ipp_seq_printf);
	ctl_ipp_isp_dbg_cbtime_dump(nvt_ctl_ipp_seq_printf);
	ipp_event_dump_proc_time(nvt_ctl_ipp_seq_printf);

	pdrv_info_data->info_seq_file = NULL;

	return 0;
}

static int nvt_ctl_ipp_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ctl_ipp_proc_info_show, NULL);
}

static struct file_operations proc_info_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ctl_ipp_proc_info_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ctl_ipp_proc_init(PCTL_IPP_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir(MODULE_NAME, NULL);
	if (pmodule_root == NULL) {
		CTL_IPP_DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		CTL_IPP_DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		CTL_IPP_DBG_ERR("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;

	pentry = proc_create("info", S_IRUGO | S_IXUGO, pmodule_root, &proc_info_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc info!\n");
		ret = -EINVAL;
		goto remove_help;
	}
	pdrv_info->pproc_info_entry = pentry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_help:
	proc_remove(pdrv_info->pproc_help_entry);

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_ctl_ipp_proc_remove(PCTL_IPP_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_info_entry);
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
