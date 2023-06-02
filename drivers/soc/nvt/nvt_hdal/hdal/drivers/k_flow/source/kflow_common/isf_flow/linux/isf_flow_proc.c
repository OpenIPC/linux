#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "isf_flow_proc.h"
#include "isf_flow_main.h"
//#include "../isf_flow_api.h"
#include "../isf_flow_int.h"
#include "isf_flow_dbg.h"


//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 	100
#define MAX_ARG_NUM     	6

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PMODULE_INFO p_drv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
ISF_FLOW_DRV_INFO *p_drv_info_data;

//============================================================================
// Function define
//============================================================================

static int isf_flow_cmd_dump_all(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	char str3[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	str3[0] = 0;
	sscanf_s(strCmd, "%s %s %s", str1, sizeof(str1), str2, sizeof(str2), str3, sizeof(str3));
	if (str1[0] == 0) {
    	    _isf_flow_dump_streambyname(NULL);
    	} else if ((str1[0] != 0) && (str2[0] == 0) && (str3[0] == 0)) {
    	    _isf_flow_dump_streambyname(str1);
    	} else if ((str1[0] == '*') && (str2[0] != 0)) {
    	    _isf_flow_dump_port(str2, str3);
    	}
#else
	#if 0
	if (argc == 0) {
    	    _isf_flow_dump_streambyname(NULL);
    	} else if (argc == 1) {
    	    _isf_flow_dump_streambyname(pargv[0]);
    	} else if ((argc >= 2) && (pargv[0][0] == '*')) {
    	    _isf_flow_dump_port(pargv[1], pargv[2]);
    	}
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_dump_backtrace(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	sscanf_s(strCmd, "%s %s", str1, sizeof(str1), str2, sizeof(str2));
	if ((str1[0] != 0) && (str2[0] != 0)) {
    	    _isf_flow_dump_port3(str1, str2);
    	}
#else
	#if 0
	if (argc == 2) {
    	    _isf_flow_dump_port3(pargv[0], pargv[1]);
    	}
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_probe_port(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	char str3[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	str3[0] = 0;
	sscanf_s(strCmd, "%s %s %s", str1, sizeof(str1), str2, sizeof(str2), str3, sizeof(str3));
	if ((str1[0] == 0))
    	    _isf_flow_probe_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_probe_port(str1, str2, str3);
#else
	#if 0
	if (argc == 0)
    	    _isf_flow_probe_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_probe_port(pargv[0], pargv[1], pargv[2]);
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_perf_port(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	sscanf_s(strCmd, "%s %s", str1, sizeof(str1), str2, sizeof(str2));
	if ((str1[0] == 0))
    	    _isf_flow_perf_port(NULL, NULL);
    	else
    	    _isf_flow_perf_port(str1, str2);
#else
	#if 0
	if (argc == 0)
    	    _isf_flow_perf_port(NULL, NULL);
    	else
    	    _isf_flow_perf_port(pargv[0], pargv[1]);
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_save_port(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	char str3[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	str3[0] = 0;
	sscanf_s(strCmd, "%s %s %s", str1, sizeof(str1), str2, sizeof(str2), str3, sizeof(str3));
	if ((str1[0] == 0))
    	    _isf_flow_save_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_save_port(str1, str2, str3);
#else
	#if 0
	if (argc == 0)
    	    _isf_flow_save_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_save_port(pargv[0], pargv[1], pargv[2]);
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_debug_port(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	char str3[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	str3[0] = 0;
	sscanf_s(strCmd, "%s %s %s", str1, sizeof(str1), str2, sizeof(str2), str3, sizeof(str3));
	if ((str1[0] == 0))
    	    _isf_flow_debug_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_debug_port(str1, str2, str3);
#else
	#if 0
	if (argc == 0)
    	    _isf_flow_debug_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_debug_port(pargv[0], pargv[1], pargv[2]);
    	#endif
#endif
	return 0;
}

static int isf_flow_cmd_trace_port(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#if 0
	char str1[20] = {0};
	char str2[20] = {0};
	char str3[20] = {0};
	str1[0] = 0;
	str2[0] = 0;
	str3[0] = 0;
	sscanf_s(strCmd, "%s %s %s", str1, sizeof(str1), str2, sizeof(str2), str3, sizeof(str3));
	if ((str1[0] == 0))
    	    _isf_flow_trace_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_trace_port(str1, str2, str3);
#else
	#if 0
	if (argc == 0)
    	    _isf_flow_trace_port(NULL, NULL, NULL);
    	else
    	    _isf_flow_trace_port(pargv[0], pargv[1], pargv[2]);
    	#endif
#endif
	return 0;
}

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_list[] = {
	// keyword          function name
	{ "dump",			isf_flow_cmd_dump_all		},
	{ "back", 			isf_flow_cmd_dump_backtrace	},
	{ "probe", 			isf_flow_cmd_probe_port		},
	{ "perf", 			isf_flow_cmd_perf_port		},
	{ "save", 			isf_flow_cmd_save_port		},
	{ "debug", 			isf_flow_cmd_debug_port		},
	{ "trace", 			isf_flow_cmd_trace_port		},
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int isf_flow_proc_cmd_show(struct seq_file *sfile, void *v)
{
	DBG_IND("\n");
	return 0;
}

static int isf_flow_proc_cmd_open(struct inode *inode, struct file *file)
{
	DBG_IND("\n");
	return single_open(file, isf_flow_proc_cmd_show, &p_drv_info_data->module_info);
}

static ssize_t isf_flow_proc_cmd_write(struct file *file, const char __user *buf,
		size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
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

	if (len == 0)
		cmd_line[0] = '\0';
	else
		cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(&p_drv_info_data->module_info, ucargc - 1, &argv[1]);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}

	return size;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = isf_flow_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = isf_flow_proc_cmd_write
};

int isf_flow_proc_init(ISF_FLOW_DRV_INFO *p_drv_info)
{
	int ret = 0;
	struct proc_dir_entry *p_module_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	p_module_root = proc_mkdir(MODULE_NAME, NULL);
	if (p_module_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	p_drv_info->p_proc_module_root = p_module_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, p_module_root, &proc_cmd_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	p_drv_info->p_proc_cmd_entry = pentry;

	p_drv_info_data = p_drv_info;

	return ret;

remove_cmd:
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}

remove_root:
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return ret;
}

int isf_flow_proc_remove(ISF_FLOW_DRV_INFO *p_drv_info)
{
	if (p_drv_info->p_proc_cmd_entry) {
		proc_remove(p_drv_info->p_proc_cmd_entry);
	}
	if (p_drv_info->p_proc_module_root) {
		proc_remove(p_drv_info->p_proc_module_root);
	}
	return 0;
}
