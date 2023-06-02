#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "kdrv_tge_int_proc.h"
#include "kdrv_tge_int_dbg.h"
#include "kdrv_tge_int_main.h"
#include "kdrv_tge_int_api.h"



//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM     20

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
    char cmd[MAX_CMD_LENGTH];
    int (*execute)(PTGE_MODULE_INFO pdrv, unsigned char argc, char** argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static PTGE_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================

static int nvt_tge_proc_cmd_show(struct seq_file *sfile, void *v)
{
    return 0;
}

static int nvt_tge_proc_cmd_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvt_tge_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_tge_proc_cmd_write(struct file *file, const char __user *buf,
                                  size_t size, loff_t *off)
{
    int len = size;
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

	if (len > 0) {
    	cmd_line[len - 1] = '\0';
	}

	nvt_dbg(IND, "CMD:%s\n", cmd_line);

    // parse command string
    for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
        argv[ucargc] = strsep(&cmdstr, delimiters);

        if (argv[ucargc] == NULL)
            break;
    }

    // dispatch command handler
	kdrv_tge_cmd_execute(ucargc, &argv[0]);

    return size;

ERR_OUT:
    return -1;
}

static struct file_operations proc_cmd_fops = {
    .owner   = THIS_MODULE,
    .open    = nvt_tge_proc_cmd_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
    .write   = nvt_tge_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_tge_proc_help_show(struct seq_file *sfile, void *v)
{
	nvt_kdrv_tge_cmd_help();
    return 0;
}

static int nvt_tge_proc_help_open(struct inode *inode, struct file *file)
{
    return single_open(file, nvt_tge_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
    .owner  = THIS_MODULE,
    .open   = nvt_tge_proc_help_open,
    .release = single_release,
    .read   = seq_read,
    .llseek = seq_lseek,
};

int nvt_tge_proc_init(PTGE_DRV_INFO pdrv_info)
{
    int ret = 0;
    struct proc_dir_entry *pmodule_root = NULL;
    struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("kdrv_tge", NULL);
	if(pmodule_root == NULL) {
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

int nvt_tge_proc_remove(PTGE_DRV_INFO pdrv_info)
{
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
