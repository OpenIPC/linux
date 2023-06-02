#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "usb_chrgdet_proc.h"
#include "usb_chrgdet_dbg.h"
#include "usb_chrgdet_main.h"
#include "usb_chrgdet_api.h"
#include "comm/usb_chrgdet.h"


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
	int (*execute)(PUSB_CHRGDET_MODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
PUSB_CHRGDET_DRV_INFO pusb_chrgdet_drv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
	{ "reg",            nvt_usb_chrgdet_api_read_reg           },
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "reg",            nvt_usb_chrgdet_api_write_reg          },
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int nvt_usb_chrgdet_proc_cmd_show(struct seq_file *sfile, void *v)
{
	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	return 0;
}

static int nvt_usb_chrgdet_proc_cmd_open(struct inode *inode, struct file *file)
{
	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	return single_open(file, nvt_usb_chrgdet_proc_cmd_show, &pusb_chrgdet_drv_info_data->module_info);
}

static ssize_t nvt_usb_chrgdet_proc_cmd_write(struct file *file, const char __user *buf,
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

	usb_chrgdet_api("%s\n", __func__);

	// check command length
	if (len > (MAX_CMD_LENGTH - 1)) {
		nvt_dbg(ERR, "Command length is too long!\n");
		goto ERR_OUT;
	}

	// copy command string from user space
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	if (len == 0) {
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
				ret = cmd_read_list[loop].execute(&pusb_chrgdet_drv_info_data->module_info, ucargc - 2, &argv[2]);
				break;
			}
		}
		if (loop >= NUM_OF_READ_CMD) {
			goto ERR_INVALID_CMD;
		}

	} else if (strncmp(argv[0], "w", 2) == 0)  {

		for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
			if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				ret = cmd_write_list[loop].execute(&pusb_chrgdet_drv_info_data->module_info, ucargc - 2, &argv[2]);
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
	.open    = nvt_usb_chrgdet_proc_cmd_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = nvt_usb_chrgdet_proc_cmd_write
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static int nvt_usb_chrgdet_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 plug = 0;
	USB_CHRGDET_STS status;

	usb_chrgdet_api("%s\n", __func__);

	/*
	status = USB_CHRGDET_STS_DISCONNECT;
	usb_chrgdet_set(USB_CHRGDET_PORT_USB30, USB_CHRGDET_TYPE_STD, TRUE);
	usb_chrgdet_get(USB_CHRGDET_PORT_USB30, USB_CHRGDET_PLUG, &plug);
	if (plug)
		usb_chrgdet_get(USB_CHRGDET_PORT_USB30, USB_CHRGDET_STATUS, &status);
	seq_printf(sfile, "\n STD USB3_CHRGDET plug:%d status=%d\n", plug, status);

	status = USB_CHRGDET_STS_DISCONNECT;
	usb_chrgdet_set(USB_CHRGDET_PORT_USB30, USB_CHRGDET_TYPE_STD, FALSE);
	usb_chrgdet_get(USB_CHRGDET_PORT_USB30, USB_CHRGDET_PLUG, &plug);
	if (plug)
		usb_chrgdet_get(USB_CHRGDET_PORT_USB30, USB_CHRGDET_STATUS, &status);
	seq_printf(sfile, "NSTD USB3_CHRGDET plug:%d status=%d\n", plug, status);
	*/

	status = USB_CHRGDET_STS_DISCONNECT;
	usb_chrgdet_set(USB_CHRGDET_PORT_USB20, USB_CHRGDET_TYPE_STD, TRUE);
	usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_PLUG, &plug);
	if (plug)
		usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_STATUS, &status);
	seq_printf(sfile, " STD USB2_CHRGDET plug:%d status=%d\n", plug, status);

	status = USB_CHRGDET_STS_DISCONNECT;
	usb_chrgdet_set(USB_CHRGDET_PORT_USB20, USB_CHRGDET_TYPE_STD, FALSE);
	usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_PLUG, &plug);
	if (plug)
		usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_STATUS, &status);
	seq_printf(sfile, "NSTD USB2_CHRGDET plug:%d status=%d\n", plug, status);

	seq_printf(sfile, "status: (0)pc (1)charging-pc (2)charger (3)disconnect\n");
	return 0;
}

static int nvt_usb_chrgdet_proc_help_open(struct inode *inode, struct file *file)
{
	usb_chrgdet_api("%s\n", __func__);

	return single_open(file, nvt_usb_chrgdet_proc_help_show, NULL);
}

static struct file_operations proc_help_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_usb_chrgdet_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

static int nvt_usb_chrgdet_proc_u2state_show(struct seq_file *sfile, void *v)
{
	UINT32 plug = 0;
	USB_CHRGDET_STS status;


	status = USB_CHRGDET_STS_DISCONNECT;
	usb_chrgdet_set(USB_CHRGDET_PORT_USB20, USB_CHRGDET_TYPE_STD, FALSE);
	usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_PLUG, &plug);
	if (plug)
		usb_chrgdet_get(USB_CHRGDET_PORT_USB20, USB_CHRGDET_STATUS, &status);


	if(plug) {
		if(status == USB_CHRGDET_STS_STANDARD_PC)
			seq_printf(sfile, "PC");
		else if (status == USB_CHRGDET_STS_CHARGING_PC)
			seq_printf(sfile, "CHARGINGPC");
		else if (status == USB_CHRGDET_STS_CHARGER)
			seq_printf(sfile, "CHARGER");
	} else {
		seq_printf(sfile, "DISCONNECTED");
	}

	return 0;
}

static int nvt_usb_chrgdet_proc_u2state_open(struct inode *inode, struct file *file)
{
	usb_chrgdet_api("%s\n", __func__);

	return single_open(file, nvt_usb_chrgdet_proc_u2state_show, NULL);
}

static struct file_operations proc_u2state_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_usb_chrgdet_proc_u2state_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
};

int nvt_usb_chrgdet_proc_init(PUSB_CHRGDET_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	usb_chrgdet_api("%s\n", __func__);

	pmodule_root = proc_mkdir("usb_chrgdet", NULL);
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

	pentry = proc_create("u2state", S_IRUGO | S_IXUGO, pmodule_root, &proc_u2state_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc help!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_u2state_entry = pentry;

	pusb_chrgdet_drv_info_data = pdrv_info;

	return ret;

remove_cmd:
	proc_remove(pdrv_info->pproc_cmd_entry);

remove_root:
	proc_remove(pdrv_info->pproc_module_root);
	return ret;
}

int nvt_usb_chrgdet_proc_remove(PUSB_CHRGDET_DRV_INFO pdrv_info)
{
	usb_chrgdet_api("%s\n", __func__);

	proc_remove(pdrv_info->pproc_u2state_entry);
	proc_remove(pdrv_info->pproc_help_entry);
	proc_remove(pdrv_info->pproc_cmd_entry);
	proc_remove(pdrv_info->pproc_module_root);
	return 0;
}
