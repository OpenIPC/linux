#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "tse_proc.h"
#include "tse_dbg.h"
#include "tse_main.h"
#include "tse_api.h"


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
	int (*execute)(PMODULE_INFO pdrv, unsigned char argc, char **argv);
	char *msg;
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PTSE_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
#if (NVT_TSE_TEST_CMD == 1)
	{"dbglevel", nvt_tse_get_dbglevel, "get dbg level (0~8, def:1)"},
#endif
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
#if (NVT_TSE_TEST_CMD == 1)
	{"dbglevel", nvt_tse_set_dbglevel, "set dbg level (0~8, def:1)"},
	{"reset", nvt_tse_reset, "pll reset"},
	{"memset", nvt_tse_mem_set_test, "memset self test"},
	{"memcpy", nvt_tse_mem_cpy_test, "memcpy self test"},
	{"mux", nvt_tse_mux_demux_test, "mux/demux self test"},
#endif
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int nvt_tse_proc_cmd_show(struct seq_file *sfile, void *v)
{
	int i;

	DBG_DUMP("[echo w param > cmd]\n");
	for (i = 0; i < (int)NUM_OF_WRITE_CMD; i ++) {
		DBG_DUMP("%s: %s\n", cmd_write_list[i].cmd, cmd_write_list[i].msg);
	}

	DBG_DUMP("[echo r param > cmd]\n");
	for (i = 0; i < (int)NUM_OF_READ_CMD; i ++) {
		DBG_DUMP("%s: %s\n", cmd_read_list[i].cmd, cmd_read_list[i].msg);
	}
	return 0;
}

static int nvt_tse_proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_tse_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_tse_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
#if (NVT_TSE_TEST_CMD == 1)
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;
	unsigned char loop;

	// check command length
	if ((len > (MAX_CMD_LENGTH - 1)) || (len == 0)) {
		DBG_ERR("Command length error (%d)!\n", len);
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len))
		goto ERR_OUT;

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

	// parse command string
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL)
		    break;
	}

	// dispatch command handler
	if (strncmp(argv[0], "r", 2) == 0) {
		for (loop = 0 ; loop < (int)NUM_OF_READ_CMD; loop++) {
			if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_read_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= (int)NUM_OF_READ_CMD)
			goto ERR_INVALID_CMD;
	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < (int)NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pdrv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}

		if (loop >= (int)NUM_OF_WRITE_CMD)
		goto ERR_INVALID_CMD;

	} else
		goto ERR_INVALID_CMD;

	return size;

ERR_INVALID_CMD:
	DBG_ERR("Invalid CMD \"%s\"\n", cmd_line);

ERR_OUT:
	return -1;
#else
	return 0;
#endif
}

static struct file_operations proc_cmd_fops = {
	.owner   = THIS_MODULE,
	.open    = nvt_tse_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_tse_proc_cmd_write
};

int nvt_tse_proc_init(PTSE_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("nvt_drv_tse", NULL);
	if (pmodule_root == NULL) {
		DBG_ERR("failed to create Module root\n");
		ret = -EINVAL;
		return ret;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_cmd_entry = pentry;

	pdrv_info_data = pdrv_info;
	return ret;

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_tse_proc_remove(PTSE_DRV_INFO pdrv_info)
{
	if (pdrv_info->pproc_cmd_entry) {
		proc_remove(pdrv_info->pproc_cmd_entry);
	}

	if (pdrv_info->pproc_module_root) {
		proc_remove(pdrv_info->pproc_module_root);
	}
	return 0;
}
