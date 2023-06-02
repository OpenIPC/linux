#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "../include/isf_audout_proc.h"
#include "../include/isf_audout_dbg.h"
#include "../include/isf_audout_main.h"
#include "../include/isf_audout_api.h"
#include "../isf_audout_int.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER


//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PISF_AUDOUT_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PISF_AUDOUT_DRV_INFO p_drv_info_data;
static struct seq_file *g_seq_file;

//============================================================================
// Function define
//============================================================================
//debug
static int isf_audout_seq_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);

	seq_printf(g_seq_file, buf);

	return 0;
}

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================

static ssize_t isf_aout_cmd_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len) {
		cmd_line[len - 1] = '\0';
	} else {
		cmd_line[0] = 0;
	}

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 1; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}
	if (ucargc==0) {
		goto ERR_OUT;
	}

	// dispatch command handler
	ret = audout_cmd_execute(ucargc, argv);

	return size;

ERR_OUT:
	return -1;
}

static int isf_aout_cmd_proc_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int isf_aout_cmd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_aout_cmd_proc_show, &p_drv_info_data->module_info);
}

static struct file_operations isf_aout_cmd_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = isf_aout_cmd_proc_open,
	.read    = seq_read,
	.write   = isf_aout_cmd_proc_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int isf_aout_help_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "1. 'cat /proc/hdal/aout/info' will show all the audioout info\r\n");
	return 0;
}

static int isf_aout_help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_aout_help_proc_show, NULL);
}

static struct file_operations isf_aout_help_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = isf_aout_help_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

//=============================================================================
// proc "info" file operation functions
//=============================================================================
extern void debug_log_cb(struct seq_file *sfile, UINT32 uid);

static int isf_aout_info_proc_show(struct seq_file *sfile, void *v)
{
	UINT32 dev;

	SEM_WAIT(ISF_AUDOUT_PROC_SEM_ID);
	g_seq_file = sfile;
	debug_log_cb(g_seq_file, 1);

	for(dev = 0; dev < 2; dev++) {
		UINT32 uid = ISF_UNIT_AUDOUT + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);

		debug_log_cb(g_seq_file, uid);
		isf_audout_dump_status(isf_audout_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_AUDOUT_PROC_SEM_ID);
	return 0;
}

static int isf_aout_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_aout_info_proc_show, NULL);
}


static struct file_operations isf_aout_info_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = isf_aout_info_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


int isf_audout_proc_init(PISF_AUDOUT_DRV_INFO p_drv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	p_module_root = proc_mkdir("hdal/aout", NULL);
	if (p_module_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	p_drv_info->p_proc_module_root = p_module_root;


#if 1
	pentry = proc_create("info", S_IRUGO | S_IXUGO, p_module_root, &isf_aout_info_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc status!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}

	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &isf_aout_cmd_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_info;
	}
	p_drv_info->p_proc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, p_module_root, &isf_aout_help_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_help_entry = pentry;
#endif

	p_drv_info_data = p_drv_info;

	return ret;


remove_cmd:
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}

remove_info:
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_info_entry);
	}

remove_root:
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return ret;
}

int isf_audout_proc_remove(PISF_AUDOUT_DRV_INFO p_drv_info)
{
	if (p_drv_info->p_proc_help_entry) {
		proc_remove(p_drv_info->p_proc_help_entry);
	}
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return 0;
}
