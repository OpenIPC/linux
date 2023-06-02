#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include "nvtmpp_int.h"
#include "nvtmpp_id.h"
#include "nvtmpp_proc.h"
#include "nvtmpp_debug.h"
#include "nvtmpp_debug_cmd.h"



static struct seq_file *g_seq_file;

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH  64
#define MAX_ARG_NUM     20
//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(NVTMPP_DRV_INFO *pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static NVTMPP_DRV_INFO *pdrv_info_data;

//============================================================================
// Function define
//============================================================================

int nvtmpp_proc_cmd(NVTMPP_DRV_INFO *p_drv, unsigned char argc, char **argv)
{
	nvtmpp_cmd_execute(argc, argv);
	return 0;
}

//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_list[] = {
	// keyword          function name
	{ "nvtmpp",			nvtmpp_proc_cmd	},
};

#define NUM_OF_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))

static int nvtmpp_seq_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	/* Initialize variable arguments. */
	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	seq_printf(g_seq_file, buf);
	return 0;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static int nvtmpp_info_proc_show(struct seq_file *sfile, void *v)
{
	SEM_WAIT(NVTMPP_PROC_SEM_ID);
	g_seq_file = sfile;
	nvtmpp_dump_status(nvtmpp_seq_printf);
	nvtmpp_dump_mem_range(nvtmpp_seq_printf);
	nvtmpp_dump_err_status(nvtmpp_seq_printf);
	SEM_SIGNAL(NVTMPP_PROC_SEM_ID);
	return 0;
}

static int nvtmpp_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtmpp_info_proc_show, NULL);
}

static struct file_operations nvtmpp_info_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = nvtmpp_info_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

//=============================================================================
// proc "task" file operation functions
//=============================================================================
static int nvtmpp_task_proc_show(struct seq_file *sfile, void *v)
{
	SEM_WAIT(NVTMPP_PROC_SEM_ID);
	g_seq_file = sfile;
	vos_task_dump(nvtmpp_seq_printf);
	vos_flg_dump(nvtmpp_seq_printf);
	vos_sem_dump(nvtmpp_seq_printf, 0);
	SEM_SIGNAL(NVTMPP_PROC_SEM_ID);
	return 0;
}

static int nvtmpp_task_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtmpp_task_proc_show, NULL);
}

static struct file_operations nvtmpp_task_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = nvtmpp_task_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

//=============================================================================
// proc "sem" file operation functions
//=============================================================================
static int nvtmpp_sem_proc_show(struct seq_file *sfile, void *v)
{
	SEM_WAIT(NVTMPP_PROC_SEM_ID);
	g_seq_file = sfile;
	vos_sem_dump(nvtmpp_seq_printf, 1);
	SEM_SIGNAL(NVTMPP_PROC_SEM_ID);
	return 0;
}

static int nvtmpp_sem_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtmpp_sem_proc_show, NULL);
}

static struct file_operations nvtmpp_sem_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = nvtmpp_sem_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


//=============================================================================
// proc "test1" file operation functions
//=============================================================================
static ssize_t nvtmpp_cmd_proc_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	int len = size;
	int ret = -EINVAL;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	unsigned char argc = 0;
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char loop;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}
	if (len == 0) {
		nvt_dbg(ERR, "Command length is 0!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	DBG_IND("CMD:%s\n", cmd_line);

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
			ret = cmd_list[loop].execute(pdrv_info_data, argc , &argv[0]);
			break;
		}
	}
	if (loop >= NUM_OF_CMD) {
		goto ERR_INVALID_CMD;
	}
	return size;

ERR_INVALID_CMD:
	nvt_dbg(ERR, "Invalid CMD !!\r\n");
ERR_OUT:
	return -1;
}

static int nvtmpp_cmd_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "showmsg_level=%d\r\n", nvtmpp_vb_get_showmsg_level());
	return 0;
}

static int nvtmpp_cmd_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtmpp_cmd_proc_show, PDE_DATA(inode));
}

// plug file-system
static const struct file_operations nvtmpp_cmd_proc_fops = {
	.owner = THIS_MODULE,
	.open  = nvtmpp_cmd_proc_open,
	.read  = seq_read,
	.write   = nvtmpp_cmd_proc_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "meminfo" file operation functions
//=============================================================================
static int nvtmpp_help_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "1. 'cat /proc/hdal/comm/info' will show all the media memory info\r\n");
	seq_printf(sfile, "2. 'cat /proc/hdal/comm/task' will show all the media tasks info\r\n");
	seq_printf(sfile, "3. 'cat /proc/hdal/comm/sem'  will show all the media created semaphores info\r\n");
	seq_printf(sfile, "4. 'echo nvtmpp xxx > /proc/hdal/comm/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");
	SEM_WAIT(NVTMPP_PROC_SEM_ID);
	g_seq_file = sfile;
	nvtmpp_cmd_showhelp(nvtmpp_seq_printf);
	SEM_SIGNAL(NVTMPP_PROC_SEM_ID);
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "Ex1: 'echo nvtmpp showmsg 1 > /proc/hdal/comm/cmd' \r\n");
	seq_printf(sfile, "Ex2: 'echo nvtmpp dumpmax 0 3 > /proc/hdal/comm/cmd' \r\n");
	return 0;
}

static int nvtmpp_help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvtmpp_help_proc_show, NULL);
}

static struct file_operations nvtmpp_help_proc_fops = {
	.owner  = THIS_MODULE,
	.open   = nvtmpp_help_proc_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};


int nvtmpp_proc_init(PNVTMPP_DRV_INFO pdrv_info)
{
	int ret = 0;
	//struct proc_dir_entry *phdal_root = NULL;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	// hdal root
#if 0   // the root has created by nvt_util.ko
	phdal_root = proc_mkdir("hdal", NULL);
	if (phdal_root == NULL) {
		DBG_ERR("failed to create hdal root\r\n");
		ret = -EINVAL;
		return ret;
	}
	pdrv_info->pproc_hdal_root = phdal_root;
#endif
	// hdal/comm root
	pmodule_root = proc_mkdir("hdal/comm", NULL);
	if (pmodule_root == NULL) {
		DBG_ERR("failed to create Module root\r\n");
		ret = -EINVAL;
		goto remove_hdal;
	}
	pdrv_info->pproc_module_root = pmodule_root;
	// info
	pentry = proc_create("info", S_IRUGO | S_IXUGO, pmodule_root, &nvtmpp_info_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc info!\r\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_info_entry = pentry;
	// task
	pentry = proc_create("task", S_IRUGO | S_IXUGO, pmodule_root, &nvtmpp_task_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc task!\r\n");
		ret = -EINVAL;
		goto remove_info;
	}
	pdrv_info->pproc_task_entry = pentry;
	// semaphore
	pentry = proc_create("sem", S_IRUGO | S_IXUGO, pmodule_root, &nvtmpp_sem_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc sem!\r\n");
		ret = -EINVAL;
		goto remove_task;
	}
	pdrv_info->pproc_sem_entry = pentry;
	// cmd
	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &nvtmpp_cmd_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc cmd!\r\n");
		ret = -EINVAL;
		goto remove_sem;
	}
	pdrv_info->pproc_cmd_entry = pentry;
	// help
	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &nvtmpp_help_proc_fops);
	if (pentry == NULL) {
		DBG_ERR("failed to create proc help!\r\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_help_entry = pentry;
	pdrv_info_data = pdrv_info;
	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);
remove_sem:
	proc_remove(pdrv_info->pproc_sem_entry);
remove_task:
	proc_remove(pdrv_info->pproc_task_entry);
remove_info:
	proc_remove(pdrv_info->pproc_info_entry);
remove_root:
	proc_remove(pdrv_info->pproc_module_root);
remove_hdal:
	proc_remove(pdrv_info->pproc_hdal_root);
	return ret;
}

void nvtmpp_proc_exit(PNVTMPP_DRV_INFO pdrv_info)
{
	if (pdrv_info_data) {
		proc_remove(pdrv_info->pproc_help_entry);
		proc_remove(pdrv_info->pproc_cmd_entry);
		proc_remove(pdrv_info->pproc_sem_entry);
		proc_remove(pdrv_info->pproc_task_entry);
		proc_remove(pdrv_info->pproc_info_entry);
		proc_remove(pdrv_info->pproc_module_root);
		proc_remove(pdrv_info->pproc_hdal_root);
		pdrv_info_data = NULL;
	}
}


