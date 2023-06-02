#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kdrv_sie_proc.h"
#include "kdrv_sie_api.h"
#include "sie_dbg.h"
#include "kdrv_sie_main.h"



//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     30

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PSIE_MODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;


//============================================================================
// Global variable
//============================================================================
PKDRV_SIE_DRV_INFO pkdrvdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static int nvt_kdrv_sie_proc_cmd_show(struct seq_file *sfile, void *v)
{
	return 0;
}

static int nvt_kdrv_sie_proc_cmd_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvt_kdrv_sie_proc_cmd_show, &pkdrvdrv_info_data->module_info);
}

static ssize_t nvt_kdrv_sie_proc_cmd(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
    int len = size;
    char cmd_line[MAX_CMD_LENGTH];
    char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		kdrv_sie_dbg_dump("Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len == 0) {
		cmd_line[0] = '\0';
	} else {
		cmd_line[len - 1] = '\0';
	}

	kdrv_sie_dbg_dump("CMD:%s\n", cmd_line);

	argv[0] = "kdrv_sie";
	// parse command string
	for (ucargc = 1; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}
	if (kdrv_sie_cmd_execute(ucargc, &argv[0]) == 0) {
		return size;
	}

ERR_OUT:
	return -1;
}

static struct file_operations proc_cmd_fops = {
    .owner   = THIS_MODULE,
    .open    = nvt_kdrv_sie_proc_cmd_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
    .write   = nvt_kdrv_sie_proc_cmd
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_kdrv_sie_proc_help_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=============================Cmd====================================\n");
    seq_printf(sfile, " Add message here\n");
    seq_printf(sfile, "=====================================================================\n");
    return 0;
}

static int nvt_kdrv_sie_proc_help_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvt_kdrv_sie_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
    .owner  = THIS_MODULE,
    .open   = nvt_kdrv_sie_proc_help_open,
    .release = single_release,
    .read   = seq_read,
    .llseek = seq_lseek,
};

int nvt_kdrv_sie_proc_init(PKDRV_SIE_DRV_INFO pdrv_info)
{
    int ret = KDRV_SIE_E_OK;
    struct proc_dir_entry *pmodule_root = NULL;
    struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir(MODULE_NAME, NULL);
	if (pmodule_root == NULL) {
		kdrv_sie_dbg_dump("failed to create Module root\n");
        ret = KDRV_SIE_E_NODEV;
		goto remove_root;
	}
	pdrv_info->pproc_module_root = pmodule_root;


	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
	if (pentry == NULL) {
		kdrv_sie_dbg_dump("failed to create proc cmd!\n");
        ret = KDRV_SIE_E_NODEV;
        goto remove_cmd;
    }
    pdrv_info->pproc_cmd_entry = pentry;

	pentry = proc_create("help", S_IRUGO | S_IXUGO, pmodule_root, &proc_help_fops);
	if (pentry == NULL) {
		kdrv_sie_dbg_dump("failed to create proc help!\n");
        ret = KDRV_SIE_E_NODEV;
        goto remove_cmd;
    }
    pdrv_info->pproc_help_entry = pentry;

	pkdrvdrv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
     return ret;
}

int nvt_kdrv_sie_proc_remove(PKDRV_SIE_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return KDRV_SIE_E_OK;
}
