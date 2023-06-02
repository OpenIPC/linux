#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kdrv_ai_dbg.h"
#include "ai_proc.h"
#include "ai_main.h"
#include "ai_api.h"

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH  30
#define MAX_ARG_NUM     6

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PAI_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PAI_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================
extern int nvt_ai_api_read_cycle(PAI_INFO pmodule_info, unsigned char argc, char **pargv);
extern int nvt_ai_api_read_version(PAI_INFO pmodule_info, unsigned char argc, char **pargv);
extern int nvt_kdrv_ai_set_layer_cycle(PAI_INFO pmodule_info, unsigned char argc, char **pargv);

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
#if !defined(CONFIG_NVT_SMALL_HDAL)
	{ "reg",            nvt_ai_api_read_reg           }, // read reg
#endif
	{ "cycle",          nvt_ai_api_read_cycle         }, // read cycle
	{ "version",        nvt_ai_api_read_version       },  // read kdrv version
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
#if !defined(CONFIG_NVT_SMALL_HDAL)
	{ "reg",            nvt_ai_api_write_reg            },
	{ "pattern",        nvt_ai_api_write_pattern        },
	{ "kdrv",           nvt_kdrv_ai_api_test        	},
	{ "emu",            nvt_kdrv_ai_module_test        	},
#endif
	{ "cycle",         	nvt_kdrv_ai_set_layer_cycle     },
	{ "reset",         	nvt_kdrv_ai_reset               },
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

extern UINT32 is_ai_builtin_on; //0:off, 1:on
extern UINT32 is_ai_on; //0:off, 1:on

static int nvt_ai_proc_cmd_show(struct seq_file *sfile, void *v)
{
	if ((sfile == NULL) || (v == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	return 0;
}

static int nvt_ai_proc_cmd_open(struct inode *inode, struct file *file)
{
	if ((inode == NULL) || (file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	return single_open(file, nvt_ai_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_ai_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	if ((file == NULL) || (buf == NULL) || (off == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}
	if (len == 0) {
		nvt_dbg(ERR, "Command length is zero!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	nvt_dbg(IND, "CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD) {
			goto ERR_INVALID_CMD;
		}
	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= NUM_OF_WRITE_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else {
		goto ERR_INVALID_CMD;
	}

	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_ai_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_ai_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_ai_proc_help_show(struct seq_file *sfile, void *v)
{
	if ((sfile == NULL) || (v == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " the network supported number is %d\n", (unsigned int) kdrv_ai_drv_get_net_supported_num());
	seq_printf(sfile, " please insmod kdrv_ai.ko ai_net_supported_num=N(unsigned integer), the N is decided by user.\n");
	seq_printf(sfile, " info: builtin(0x%x) kdrv(0x%x).\n", (unsigned int) is_ai_builtin_on, (unsigned int) is_ai_on);
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int nvt_ai_proc_help_open(struct inode *inode, struct file *file)
{
	if ((inode == NULL) || (file == NULL)) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}

	return single_open(file, nvt_ai_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ai_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_ai_proc_init(PAI_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	pmodule_root = proc_mkdir("kdrv_ai", NULL);
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

int nvt_ai_proc_remove(PAI_DRV_INFO pdrv_info)
{
	if (pdrv_info == NULL) {
		nvt_dbg(ERR, "invalid input\n");
		return -EINVAL;
	}
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
