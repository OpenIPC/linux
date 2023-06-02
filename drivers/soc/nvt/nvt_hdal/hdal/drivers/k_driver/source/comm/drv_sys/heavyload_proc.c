#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "heavyload.h"
#include "heavyload_proc.h"

#define __MODULE__          heavyload_proc
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

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
	int (*execute)(unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PXXX_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
int heavyload_proc_cmd(unsigned char argc, char **argv)
{
	heavyload_cmd_execute(argc, argv);
	return 0;
}

static PROC_CMD cmd_list[] = {
	// keyword          function name
	{ "heavyload",		heavyload_proc_cmd		},
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int heavyload_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int heavyload_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, heavyload_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t heavyload_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char argc = 0;
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len))
		goto ERR_OUT;

	if (len == 0)
		cmd_line[0] = '\0';
	else
		cmd_line[len - 1] = '\0';

	nvt_dbg(IND, "CMD:%s\n", cmd_line);


	// parse command string
	for (argc = 0; argc < MAX_ARG_NUM; argc++) {
		argv[argc] = strsep(&cmdstr, delimiters);
		if (argv[argc] == NULL) {
			break;
		}
	}
	if (argc < 2) {
		goto ERR_INVALID_CMD;
	}

	// dispatch command handler
	for (loop = 0 ; loop < NUM_OF_CMD; loop++) {
		if (strncmp(argv[0], cmd_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
			ret = cmd_list[loop].execute(argc, &argv[0]);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
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
	.open    = heavyload_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = heavyload_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int heavyload_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
	seq_printf(sfile, " Add message here\n");
	seq_printf(sfile, "=====================================================================\n");
	return 0;
}

static int heavyload_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, heavyload_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = heavyload_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int heavyload_proc_init(PXXX_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("heavyload", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_heavload_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_heavload_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_heavload_help_entry = pentry;


	pdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_heavload_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_heavload_module_root);
	return ret;
}

int heavyload_proc_remove(PXXX_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_heavload_help_entry);
	proc_remove(pdrv_info->pproc_heavload_cmd_entry);
	proc_remove(pdrv_info->pproc_heavload_module_root);
	return 0;
}
