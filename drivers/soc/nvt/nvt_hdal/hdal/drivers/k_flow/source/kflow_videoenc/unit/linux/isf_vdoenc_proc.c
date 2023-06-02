#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kwrap/semaphore.h"
#include "isf_vdoenc_proc.h"
#include "isf_vdoenc_main.h"
#include "isf_vdoenc_api.h"
#include "isf_vdoenc_internal.h"

#include "nmediarec_vdoenc.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_vdoenc_dbg.h"


#define USE_PROC    1
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 200
#define MAX_ARG_NUM     20

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args);
} PROC_CMD, *PPROC_CMD;

static struct seq_file *g_seq_file;

//============================================================================
// Global variable
//============================================================================
ISF_VDOENC_DRV_INFO *p_drv_info_data;

//============================================================================
// Function define
//============================================================================
static int isf_vdoenc_seq_printf(const char *fmtstr, ...)
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

#if USE_PROC
//=============================================================================
// proc "info" file operation functions
//=============================================================================
extern void debug_log_cb(struct seq_file *sfile, UINT32 uid);
extern SEM_HANDLE ISF_VDOENC_PROC_SEM_ID;
extern void isf_vdoenc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);
static int isf_vdoenc_proc_info_show(struct seq_file *sfile, void *v)
{
	#define ISF_VDOENC_DEV_NUM     1
	UINT32 dev;
	SEM_WAIT(ISF_VDOENC_PROC_SEM_ID);
	g_seq_file = sfile;
	//dump info of all devices
	debug_log_cb(g_seq_file, 1); //show hdal version
	for(dev = 0; dev < ISF_VDOENC_DEV_NUM; dev++) {
		UINT32 uid = ISF_UNIT_VDOENC + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(g_seq_file, uid);
		//dump work status of 1 device
		isf_vdoenc_dump_status(isf_vdoenc_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_VDOENC_PROC_SEM_ID);
	return 0;
}

static int isf_vdoenc_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_vdoenc_proc_info_show, NULL);
}

static struct file_operations proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoenc_proc_info_open,
	.release = single_release,
	.read    = seq_read,
	.llseek  = seq_lseek,
};

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
int isf_vdoenc_cmd_isfve(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_isfve(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_vdoenc(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_vdoenc(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_debug(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_debug(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_trace(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_trace(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_probe(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_probe(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_perf(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_perf(sub_cmd_name, cmd_args);
	return 0;
}

int isf_vdoenc_cmd_save(ISF_VDOENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_vdoenc_cmd_save(sub_cmd_name, cmd_args);
	return 0;
}

static PROC_CMD cmd_list[] = {
	// keyword          function name
	{ "isfve",          isf_vdoenc_cmd_isfve    },
	{ "vdoenc",         isf_vdoenc_cmd_vdoenc   },
	{ "debug",          isf_vdoenc_cmd_debug    },
	{ "trace",          isf_vdoenc_cmd_trace    },
	{ "probe",          isf_vdoenc_cmd_probe    },
	{ "perf",           isf_vdoenc_cmd_perf     },
	{ "save",           isf_vdoenc_cmd_save     },
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int isf_vdoenc_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int isf_vdoenc_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, isf_vdoenc_proc_cmd_show, &p_drv_info_data->module_info);
}

static ssize_t isf_vdoenc_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	char *cmd_line_res;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		DBG_ERR("Command length is too long!\n");
		goto ERR_OUT;
	}

	if (len == 0) {
		DBG_ERR("Command length = 0!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	argv[0] = strsep(&cmdstr, delimiters);  // main command   : isfve , vdoenc                      /  debug , trace , probe , perf , save
	argv[1] = strsep(&cmdstr, delimiters);  // sub  command   : showmsg , setcbr, reseti , ooxx...  /  d0
	cmd_line_res = cmdstr;                  // command params : 0 2 1920 1080 ...                   /  i0, i1, i2, o0, o1, o2, mfff ...

	if (argv[0] == NULL) {
		goto ERR_INVALID_CMD;
	}

	if ((strncmp(argv[0], "isfve", MAX_CMD_LENGTH) == 0) || (strncmp(argv[0], "vdoenc", MAX_CMD_LENGTH) == 0)) {
		if ((argv[1] == NULL) || (*argv[1] != '?' && cmd_line_res == NULL)) {
			goto ERR_INVALID_CMD;
		}
	}

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(&p_drv_info_data->module_info, argv[1], cmd_line_res);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}

	return size;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/venc/help\" for help.\r\n");

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoenc_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = isf_vdoenc_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int isf_vdoenc_proc_help_show(struct seq_file *sfile, void *v)
{
	DBG_DUMP("\n\n1. 'cat /proc/hdal/venc/info' will show all the videoenc info\n");
	DBG_DUMP("2. 'echo xxx > /proc/hdal/venc/cmd' can input command for some debug purpose\n");
	DBG_DUMP("   where \"xxx\" is as following ....\n\n");


	_isf_vdoenc_cmd_isfve_showhelp();
	DBG_DUMP("\n");
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo isfve showmsg 0 2 > /proc/hdal/venc/cmd\n\n");

	_isf_vdoenc_cmd_vdoenc_showhelp();
	DBG_DUMP("\n");
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo vdoenc encinfo 0 > /proc/hdal/venc/cmd\n");
	DBG_DUMP("  echo vdoenc setfixqp 0 1 26 26 > /proc/hdal/venc/cmd\n");
	DBG_DUMP("\n");

	_isf_vdoenc_cmd_isfdbg_showhelp();
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo debug d0 o0 mffff > /proc/hdal/venc/cmd\n");
	DBG_DUMP("  echo trace d0 o0 mffff > /proc/hdal/venc/cmd\n");
	DBG_DUMP("  echo probe d0 o0 mffff > /proc/hdal/venc/cmd\n");
	DBG_DUMP("  echo perf d0 i0 > /proc/hdal/venc/cmd\n");
	DBG_DUMP("  echo save d0 i0 > /proc/hdal/venc/cmd\n");
	DBG_DUMP("\n");
	return 0;
}

static int isf_vdoenc_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_vdoenc_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoenc_proc_help_open,
	.release = single_release,
	.read    = seq_read,
	.llseek  = seq_lseek,
};

//=============================================================================
// proc "top" file operation functions
//=============================================================================
static int isf_vdoenc_top_info_show(struct seq_file *sfile, void *v)
{
	#define ISF_VDOENC_DEV_NUM     1
	UINT32 dev;
	SEM_WAIT(ISF_VDOENC_PROC_SEM_ID);
	g_seq_file = sfile;
	//dump info of all devices
	for(dev = 0; dev < ISF_VDOENC_DEV_NUM; dev++) {
		//dump bind, state and param settings of 1 device
		//dump work status of 1 device
		NMR_VdoEnc_Top(isf_vdoenc_seq_printf);
	}
	SEM_SIGNAL(ISF_VDOENC_PROC_SEM_ID);
	return 0;
}

static int isf_vdoenc_proc_top_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_vdoenc_top_info_show, NULL);
}

static struct file_operations proc_top_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_vdoenc_proc_top_open,
	.release = single_release,
	.read    = seq_read,
	.llseek  = seq_lseek,
};
#endif

int isf_vdoenc_proc_init(ISF_VDOENC_DRV_INFO *p_drv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;

#if USE_PROC
	struct proc_dir_entry *pentry = NULL;
#endif

	p_module_root = proc_mkdir("hdal/venc", NULL);
	if (p_module_root == NULL) {
		DBG_ERR("failed to create module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	p_drv_info->p_proc_module_root = p_module_root;

#if USE_PROC
	pentry = proc_create("info", S_IRUGO | S_IXUGO, p_module_root, &proc_info_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc info!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_info_entry = pentry;

	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &proc_cmd_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, p_module_root, &proc_help_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_help_entry = pentry;

	pentry = proc_create("top", S_IRUGO | S_IXUGO, p_module_root, &proc_top_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc top!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_top_entry = pentry;
#endif

	p_drv_info_data = p_drv_info;

	return ret;

#if USE_PROC
remove_cmd:
	if (p_drv_info->p_proc_info_entry) {
		proc_remove(p_drv_info->p_proc_info_entry);
	}
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_help_entry) {
		proc_remove(p_drv_info->p_proc_help_entry);
	}
	if (p_drv_info->p_proc_top_entry) {
		proc_remove(p_drv_info->p_proc_top_entry);
	}
#endif

remove_root:
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return ret;
}

int isf_vdoenc_proc_remove(ISF_VDOENC_DRV_INFO *p_drv_info)
{
	if (p_drv_info->p_proc_top_entry) {
		proc_remove(p_drv_info->p_proc_top_entry);
	}
	if (p_drv_info->p_proc_help_entry) {
		proc_remove(p_drv_info->p_proc_help_entry);
	}
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_info_entry) {
		proc_remove(p_drv_info->p_proc_info_entry);
	}
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return 0;
}
