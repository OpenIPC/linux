#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "timer_proc.h"
#include "timer_dbg.h"
#include "timer_main.h"
#include "timer_api.h"

#if !defined(CONFIG_NVT_SMALL_HDAL)
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6
#define MAX_TIMER_CNT   20

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PMODULE_INFO pdrv, unsigned char argc, char **argv);
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
#if 0
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
	{ "reg",            nvt_timer_api_read_reg           },
};
#endif

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "auto",           nvt_timer_api_auto_test          },
	{ "pattern",		nvt_timer_api_write_pattern		}
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int nvt_timer_proc_cmd_show(struct seq_file *sfile, void *v)
{
	nvt_dbg(IND, "\n");
	return 0;
}

static int nvt_timer_proc_cmd_open(struct inode *inode, struct file *file)
{
	nvt_dbg(IND, "\n");
	return single_open(file, nvt_timer_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_timer_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
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
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL)
		    break;
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
#if 0
		for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD)
			goto ERR_INVALID_CMD;
#endif
	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= NUM_OF_WRITE_CMD)
		goto ERR_INVALID_CMD;

	} else
		goto ERR_INVALID_CMD;

	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_timer_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_timer_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_timer_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "Disable timer#number warning log\n");
	seq_printf(sfile, "echo #number > /proc/nvt_timer_module/silence\n\n");
	seq_printf(sfile, "Read silent timers\n");
	seq_printf(sfile, "cat /proc/nvt_timer_module/silence\n\n");
	seq_printf(sfile, "Clean silent timer settings\n");
	seq_printf(sfile, "echo 0xCFF > /proc/nvt_timer_module/silence\n\n");
	return 0;
}

static int nvt_timer_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_timer_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_timer_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


//=============================================================================
// proc "slience" file operation functions
//=============================================================================
static int nvt_timer_proc_silence_show(struct seq_file *sfile, void *v)
{
	int silence_timers = nvt_get_silence_timer();
	int i = 0;

	for (i = 0; i < MAX_TIMER_CNT; i++) {
		seq_printf(sfile, "timer %-3d: silence log %-3s losing event count %d\n", i, 
			((silence_timers >> i) & 0x1) ? "on" : "off", 
			nvt_get_losing_event_count(i));
	}
	return 0;
}

static int nvt_timer_proc_silence_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_timer_proc_silence_show, NULL);
}

static ssize_t nvt_timer_proc_silence_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned long config = 0x0;

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

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL)
		    break;
	}

	if (ucargc > 1)
		goto ERR_INVALID_CMD;


	if (kstrtoul(argv[0], 0, &config)) {
		nvt_dbg(ERR, "invalid config:%s\n", argv[0]);
		goto ERR_OUT;
	}

	if ((config >= MAX_TIMER_CNT) && (config != 0xCFF)) {
		nvt_dbg(ERR, "invalid config:%s\n", argv[0]);
		goto ERR_OUT;
	}

	if (config == 0xCFF)
		nvt_clear_silence_timer();
	else
		nvt_set_silence_timer(config);

	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
}

static struct file_operations proc_silence_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_timer_proc_silence_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
	.write   = nvt_timer_proc_silence_write,
};

int nvt_timer_proc_init(PXXX_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("nvt_timer_module", NULL);
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
		goto remove_help;
	}
	pdrv_info->pproc_help_entry = pentry;

	pentry = proc_create("silence", S_IRUGO | S_IXUGO, pmodule_root, &proc_silence_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc silence!\n");
		ret = -EINVAL;
		goto remove_slience;
	}
	pdrv_info->pproc_silence_entry = pentry;

	pdrv_info_data = pdrv_info;

	return ret;

remove_slience:
	proc_remove(pdrv_info->pproc_silence_entry);

remove_help:
	proc_remove(pdrv_info->pproc_help_entry);

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_timer_proc_remove(PXXX_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_silence_entry);
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}

#endif