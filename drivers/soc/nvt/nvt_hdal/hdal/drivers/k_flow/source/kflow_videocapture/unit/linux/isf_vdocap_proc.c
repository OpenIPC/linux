#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "isf_vdocap_proc.h"
#include "isf_vdocap_main.h"
#include "../isf_vdocap_api.h"
//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "../isf_vdocap_dbg.h"
#include "../isf_vdocap_int.h"
#include "kwrap/semaphore.h"
#include <linux/kmod.h>

//#ifdef DEBUG
unsigned int isf_vdocap_debug_level = NVT_DBG_WRN;
module_param_named(isf_vdocap_debug_level, isf_vdocap_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_vdocap_debug_level, "vdocap debug level");
//#endif
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
	int (*execute)(PMODULE_INFO p_drv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

static struct seq_file *g_seq_file;

//============================================================================
// Global variable
//============================================================================
static ISF_VDOCAP_DRV_INFO *p_drv_info_data;

//============================================================================
// Function define
//============================================================================

static int isf_vdocap_seq_printf(const char *fmtstr, ...)
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
// proc "info" file operation functions
//=============================================================================
extern void debug_log_cb(struct seq_file *sfile, UINT32 uid);
static int isf_vdocap_info_proc_show(struct seq_file *sfile, void *v)
{
	UINT32 dev;
	SEM_WAIT(ISF_VDOCAP_PROC_SEM_ID);
	g_seq_file = sfile;
	//dump info of all devices
	debug_log_cb(g_seq_file, 1); //show hdal version
	for(dev = 0; dev < VDOCAP_MAX_NUM; dev++) {
		UINT32 uid = ISF_UNIT_VDOCAP + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(g_seq_file, uid);
		//dump work status of 1 device
		isf_vdocap_dump_status(isf_vdocap_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOCAP_PROC_SEM_ID);
	return 0;
}

static int isf_vdocap_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_vdocap_info_proc_show, NULL);
}

static struct file_operations isf_vdocap_info_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = isf_vdocap_info_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

#if 1
//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static int isf_vdocap_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int isf_vdocap_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, isf_vdocap_proc_cmd_show, &p_drv_info_data->module_info);
}

static ssize_t isf_vdocap_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
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

	ucargc = 0; argv[0] = 0;
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
	ret = vdocap_cmd_execute(ucargc, argv);

	return size;

ERR_OUT:
	return -1;
}

static struct file_operations isf_vdocap_cmd_proc_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdocap_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = isf_vdocap_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int isf_vdocap_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int isf_vdocap_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_vdocap_proc_help_show, NULL);
}

static struct file_operations isf_vdocap_help_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = isf_vdocap_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};
#endif

int isf_vdocap_proc_init(ISF_VDOCAP_DRV_INFO *p_drv_info)
{
	int ret = 0;
	//struct proc_dir_entry *p_hdal_root = NULL;
	struct proc_dir_entry *p_module_root = NULL;
	struct proc_dir_entry *p_entry = NULL;
	/*
	p_hdal_root = proc_mkdir("hdal", NULL);
	if (p_hdal_root == NULL) {
		DBG_ERR("failed to create hdal root\r\n");
		ret = -EINVAL;
		return ret;
	}
	p_drv_info->p_proc_hdal_root = p_hdal_root;
	*/
	p_module_root = proc_mkdir("hdal/vcap", NULL);
	if (p_module_root == NULL) {
		DBG_ERR("failed to create Module root\r\n");
		ret = -EINVAL;
		//goto remove_hdal;
		return ret;
	}
	p_drv_info->p_proc_module_root = p_module_root;
	p_entry = proc_create("info", S_IRUGO | S_IXUGO, p_module_root, &isf_vdocap_info_proc_fops);
	if (p_entry == NULL) {
		DBG_ERR("failed to create proc status!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}
	p_drv_info->p_proc_info_entry = p_entry;
	p_entry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &isf_vdocap_cmd_proc_fops);
	if (p_entry == NULL) {
		DBG_ERR("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_info;
	}
	p_drv_info->p_proc_cmd_entry = p_entry;
	p_entry = proc_create("help", S_IRUGO | S_IXUGO, p_module_root, &isf_vdocap_help_proc_fops);
	if (p_entry == NULL) {
		DBG_ERR("failed to create proc meminfo!\r\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_help_entry = p_entry;
	p_drv_info_data = p_drv_info;
	return ret;

remove_cmd:
	proc_remove(p_drv_info->p_proc_cmd_entry);
remove_info:
	proc_remove(p_drv_info->p_proc_info_entry);
remove_root:
	proc_remove(p_drv_info->p_proc_module_root);
//remove_hdal:
//	proc_remove(p_drv_info->p_proc_hdal_root);
	return ret;
}

int isf_vdocap_proc_remove(ISF_VDOCAP_DRV_INFO *p_drv_info)
{
	if (p_drv_info_data) {
		proc_remove(p_drv_info->p_proc_help_entry);
		proc_remove(p_drv_info->p_proc_cmd_entry);
		proc_remove(p_drv_info->p_proc_info_entry);
		proc_remove(p_drv_info->p_proc_module_root);
//		proc_remove(p_drv_info->p_proc_hdal_root);
		p_drv_info_data = NULL;
	}
	return 0;
}


void _vdocap_print_ctl_sie_info(VDOCAP_CONTEXT *p_ctx)
{
	char * envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
	char * argv[4];
	char temp[48];

	if (p_ctx == NULL || p_ctx->started == 0)
		return;

	snprintf(temp, sizeof(temp)-1, "echo dbgtype %d 7 > /proc/nvt_ctl_sie/cmd", p_ctx->id);
	argv[0] = "/bin/ash";
	argv[1] = "-c";
	argv[2] = temp;
	argv[3] = NULL;

	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}
void _vdocap_print_ctl_sen_info(VDOCAP_CONTEXT *p_ctx)
{
	char * envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
	char * argv[4];
	char temp[48];
	UINT32 shdr_seq;

	shdr_seq = _vdocap_shdr_map_to_seq(p_ctx->shdr_map);

	if (p_ctx->started == 0 || shdr_seq > 0)
		return;

	argv[0] = "/bin/ash";
	argv[1] = "-c";
	argv[2] = temp;
	argv[3] = NULL;

	snprintf(temp, sizeof(temp)-1, "echo dbg %d 0x000084AF > /proc/ctl_sen/cmd", p_ctx->id);
	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
}

BOOL _isf_vdocap_api_drv(unsigned char argc, char **pargv)
{
#if defined(__LINUX)
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap0.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap1.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap2.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap3.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap4.refdata);
#if defined(_BSP_NA51000_)
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap5.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap6.refdata);
	_vdocap_print_ctl_sie_info((VDOCAP_CONTEXT *)isf_vdocap7.refdata);
#endif
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap0.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap1.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap2.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap3.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap4.refdata);
#if defined(_BSP_NA51000_)
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap5.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap6.refdata);
	_vdocap_print_ctl_sen_info((VDOCAP_CONTEXT *)isf_vdocap7.refdata);
#endif

#endif
	return 1;
}

BOOL _isf_vdocap_api_reg(unsigned char argc, char **pargv)
{
	VDOCAP_CONTEXT *p_ctx;

	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap0.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE1]\r\n");
		debug_dumpmem(0xF0C00000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap1.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE2]\r\n");
		debug_dumpmem(0xF0D20000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap2.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE3]\r\n");
		debug_dumpmem(0xF0D30000, 0x8B4);
	}
	if( nvt_get_chip_id() != CHIP_NA51055)
	{
		p_ctx = (VDOCAP_CONTEXT *)isf_vdocap3.refdata;
		if (p_ctx && p_ctx->sie_hdl) {
			DBG_DUMP("[SIE4]\r\n");
			debug_dumpmem(0xF0D40000, 0x8B4);
		}
		p_ctx = (VDOCAP_CONTEXT *)isf_vdocap4.refdata;
		if (p_ctx && p_ctx->sie_hdl) {
			DBG_DUMP("[SIE5]\r\n");
			debug_dumpmem(0xF0D50000, 0x8B4);
		}
	}
#if defined(_BSP_NA51000_)
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap5.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE6]\r\n");
		debug_dumpmem(0xF0D60000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap6.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE7]\r\n");
		debug_dumpmem(0xF0D70000, 0x8B4);
	}
	p_ctx = (VDOCAP_CONTEXT *)isf_vdocap7.refdata;
	if (p_ctx && p_ctx->sie_hdl) {
		DBG_DUMP("[SIE8]\r\n");
		debug_dumpmem(0xF0D80000, 0x8B4);
	}
#endif
	return 1;
}