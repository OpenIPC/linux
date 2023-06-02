#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kflow_common/isf_flow_def.h"
#include "kwrap/semaphore.h"
#include "isf_audenc_proc.h"
#include "isf_audenc_main.h"
#include "isf_audenc_api.h"
#include "kflow_audioenc/isf_audenc.h"
// [ToDo]
//#include "isf_audenc_int.h"

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_audenc_dbg.h"

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
	int (*execute)(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args);
} PROC_CMD, *PPROC_CMD;

static struct seq_file *g_seq_file;

//============================================================================
// Global variable
//============================================================================
ISF_AUDENC_DRV_INFO *p_drv_info_data;

//============================================================================
// Function define
//============================================================================
static int isf_audenc_seq_printf(const char *fmtstr, ...)
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
extern SEM_HANDLE ISF_AUDENC_PROC_SEM_ID;
extern void isf_audenc_dump_status(int (*dump)(const char *fmt, ...), ISF_UNIT *p_thisunit);
static int isf_audenc_proc_info_show(struct seq_file *sfile, void *v)
{
	#define ISF_AUDENC_DEV_NUM     1
	UINT32 dev;
	SEM_WAIT(ISF_AUDENC_PROC_SEM_ID);
	g_seq_file = sfile;
	//dump info of all devices
	debug_log_cb(g_seq_file, 1); //show hdal version
	for(dev = 0; dev < ISF_AUDENC_DEV_NUM; dev++) {
		UINT32 uid = ISF_UNIT_AUDENC + dev;
		ISF_UNIT *p_unit = isf_unit_ptr(uid);
		//dump bind, state and param settings of 1 device
		debug_log_cb(g_seq_file, uid);
		//dump work status of 1 device
		isf_audenc_dump_status(isf_audenc_seq_printf, p_unit);
	}
	SEM_SIGNAL(ISF_AUDENC_PROC_SEM_ID);
	return 0;
}

static int isf_audenc_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_audenc_proc_info_show, NULL);
}

static struct file_operations proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_audenc_proc_info_open,
	.release = single_release,
	.read    = seq_read,
	.llseek  = seq_lseek,
};

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
int isf_audenc_cmd_isfae(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_isfae(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_audenc(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_audenc(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_debug(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_debug(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_trace(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_trace(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_probe(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_probe(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_perf(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_perf(sub_cmd_name, cmd_args);
	return 0;
}

int isf_audenc_cmd_save(ISF_AUDENC_INFO *p_drv, char* sub_cmd_name, char *cmd_args)
{
	_isf_audenc_cmd_save(sub_cmd_name, cmd_args);
	return 0;
}

static PROC_CMD cmd_list[] = {
	// keyword          function name
	{ "isfae",          isf_audenc_cmd_isfae    },
	{ "audenc",         isf_audenc_cmd_audenc   },
	{ "debug",          isf_audenc_cmd_debug    },
	{ "trace",          isf_audenc_cmd_trace    },
	{ "probe",          isf_audenc_cmd_probe    },
	{ "perf",           isf_audenc_cmd_perf     },
	{ "save",           isf_audenc_cmd_save     },
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int isf_audenc_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int isf_audenc_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, isf_audenc_proc_cmd_show, &p_drv_info_data->module_info);
}

static ssize_t isf_audenc_proc_cmd_write(struct file *file, const char __user *buf,
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
	argv[0] = strsep(&cmdstr, delimiters);  // main command   : isfae , audenc                      /  debug , trace , probe , perf , save
	argv[1] = strsep(&cmdstr, delimiters);  // sub  command   : showmsg , setcbr, reseti , ooxx...  /  d0
	cmd_line_res = cmdstr;                  // command params : 0 2 1920 1080 ...                   /  i0, i1, i2, o0, o1, o2, mfff ...

	if (argv[0] == NULL) {
		goto ERR_INVALID_CMD;
	}

	if ((strncmp(argv[0], "isfae", MAX_CMD_LENGTH) == 0) || (strncmp(argv[0], "audenc", MAX_CMD_LENGTH) == 0)) {
		if ((argv[0] == NULL) || (argv[1] == NULL) || (*argv[1] != '?' && cmd_line_res == NULL)) {
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
	DBG_ERR("Invalid CMD !!\r\n  Usage : type  \"cat /proc/hdal/aenc/help\" for help.\r\n");

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_audenc_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = isf_audenc_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int isf_audenc_proc_help_show(struct seq_file *sfile, void *v)
{
	DBG_DUMP("\n\n1. 'cat /proc/hdal/aenc/info' will show all the audioenc info\n");
	DBG_DUMP("2. 'echo xxx > /proc/hdal/aenc/cmd' can input command for some debug purpose\n");
	DBG_DUMP("   where \"xxx\" is as following ....\n\n");

	_isf_audenc_cmd_isfae_showhelp();
	DBG_DUMP("\n");
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo isfae showmsg 0 2 > /proc/hdal/aenc/cmd\n\n");

	_isf_audenc_cmd_audenc_showhelp();
	DBG_DUMP("\n");
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo audenc showmsg 0 1 > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("\n");

	_isf_audenc_cmd_isfdbg_showhelp();
	DBG_DUMP("\n[ Sample ]\n");
	DBG_DUMP("  echo debug d0 o0 mfff > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("  echo trace d0 o0 mfff > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("  echo probe d0 o0 mfff > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("  echo perf d0 i0 > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("  echo save d0 i0 > /proc/hdal/aenc/cmd\n");
	DBG_DUMP("\n");
	return 0;
}

static int isf_audenc_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, isf_audenc_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_audenc_proc_help_open,
	.release = single_release,
	.read    = seq_read,
	.llseek  = seq_lseek,
};
#endif

int isf_audenc_proc_init(ISF_AUDENC_DRV_INFO *p_drv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;

#if USE_PROC
	struct proc_dir_entry *pentry = NULL;
#endif

	p_module_root = proc_mkdir("hdal/aenc", NULL);
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
#endif

remove_root:
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return ret;
}

int isf_audenc_proc_remove(ISF_AUDENC_DRV_INFO *p_drv_info)
{
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