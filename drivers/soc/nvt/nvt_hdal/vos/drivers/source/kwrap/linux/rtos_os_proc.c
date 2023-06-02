#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "rtos_os_proc.h"

#define __MODULE__    rtos_proc
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

extern int test_vos(int argc, char** argv);
extern int vos_task_debug_dump(int argc, char** argv);
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 64
#define MAX_ARG_NUM     6

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_proc_debug_level = NVT_DBG_WRN;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
typedef struct proc_cmd {
    char cmd[MAX_CMD_LENGTH];
    int (*pfunc)(int argc, char** argv);
} PROC_CMD, *PPROC_CMD;

#define NUM_OF_PROC_CMD (sizeof(cmd_list) / sizeof(PROC_CMD))
static PROC_CMD cmd_list[] = {
	{ "test_vos", test_vos},
	{ "tskcell", vos_task_debug_dump},
};

static struct proc_dir_entry *proc_entry_root = NULL;
static struct proc_dir_entry *proc_entry_cmd = NULL;
static struct proc_dir_entry *proc_entry_help = NULL;

static int cmd_ops_show(struct seq_file *sfile, void *v)
{
    return 0;
}

static int cmd_ops_open(struct inode *inode, struct file *file)
{
    return single_open(file, cmd_ops_show, NULL);
}

static ssize_t cmd_ops_write(struct file *file, const char __user *buf,
                                  size_t size, loff_t *off)
{
    char cmd_line[MAX_CMD_LENGTH];
    const char delimiters[] = {' ', '\r', '\n', '\0'};
    char *argv[MAX_ARG_NUM] = {0};
    unsigned char argc = 0;

    char *cmdstr = cmd_line;
    unsigned int cmd_idx;
    int ret = -EINVAL;

    // Note: In fact, size will never be smaller than 1 in proc cmd
    //       to prevent coverity warning, we still check the size < 1
    // e.g. cmd [abcde], size = 6
    // e.g. cmd [], size = 1
    if (size > MAX_CMD_LENGTH || size < 1) {
        DBG_ERR("Invalid cmd size %ld\n", (ULONG)size);
        goto ERR_OUT;
    }

    // copy command string from user space
    if (copy_from_user(cmd_line, buf, size))
        goto ERR_OUT;

    cmd_line[size - 1] = '\0';

	DBG_DUMP("VOS_PROC_CMD: [%s], size %ld\n", cmd_line, (ULONG)size);

    // parse command string
    for (argc = 0; argc < MAX_ARG_NUM; argc++) {
        argv[argc] = strsep(&cmdstr, delimiters);

        if (argv[argc] == NULL)
            break;
    }

    // dispatch command handler
    for (cmd_idx = 0 ; cmd_idx < NUM_OF_PROC_CMD; cmd_idx++) {
        if (strncmp(argv[0], cmd_list[cmd_idx].cmd, MAX_CMD_LENGTH) == 0) {
            ret = cmd_list[cmd_idx].pfunc(argc - 1, &argv[1]);
            break;
        }
    }
    if (cmd_idx >= NUM_OF_PROC_CMD)
		goto ERR_INVALID_CMD;

    return size;

ERR_INVALID_CMD:
    DBG_ERR("Invalid CMD [%s]\n", cmd_line);

ERR_OUT:
    return -EINVAL;
}

static struct file_operations proc_fops_cmd = {
    .owner   = THIS_MODULE,
    .open    = cmd_ops_open,
    .read    = seq_read,
    .write   = cmd_ops_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int help_ops_show(struct seq_file *sfile, void *v)
{
	int cmd_idx;

	seq_printf(sfile, "CMD_LIST:\r\n");
	for (cmd_idx = 0 ; cmd_idx < NUM_OF_PROC_CMD; cmd_idx++) {
		seq_printf(sfile, "%s\r\n", cmd_list[cmd_idx].cmd);
	}

    return 0;
}

static int help_ops_open(struct inode *inode, struct file *file)
{
    return single_open(file, help_ops_show, NULL);
}

static struct file_operations proc_fops_help = {
    .owner   = THIS_MODULE,
    .open    = help_ops_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

void rtos_proc_init(void *param)
{
    int ret = 0;

	proc_entry_root = proc_mkdir("nvt_vos", NULL);
	if(proc_entry_root == NULL) {
		DBG_ERR("failed to create root\n");
        ret = -EINVAL;
		goto rtos_proc_init_err;
	}

	proc_entry_cmd = proc_create("cmd", S_IRUGO | S_IXUGO, proc_entry_root, &proc_fops_cmd);
    if (proc_entry_cmd == NULL) {
        DBG_ERR("failed to create cmd\n");
        ret = -EINVAL;
        goto rtos_proc_init_err;
    }

    proc_entry_help = proc_create("help", S_IRUGO | S_IXUGO, proc_entry_root, &proc_fops_help);
    if (proc_entry_help == NULL) {
        DBG_ERR("failed to create help\n");
        ret = -EINVAL;
        goto rtos_proc_init_err;
    }

	return;

rtos_proc_init_err:
	if (proc_entry_help) {
		proc_remove(proc_entry_help);
		proc_entry_help = NULL;
	}

	if (proc_entry_cmd) {
		proc_remove(proc_entry_cmd);
		proc_entry_cmd = NULL;
	}

	if (proc_entry_root) {
		proc_remove(proc_entry_root);
		proc_entry_root = NULL;
	}
}

void rtos_proc_exit(void)
{
	if (proc_entry_help) {
		proc_remove(proc_entry_help);
		proc_entry_help = NULL;
	}

	if (proc_entry_cmd) {
		proc_remove(proc_entry_cmd);
		proc_entry_cmd = NULL;
	}

	if (proc_entry_root) {
		proc_remove(proc_entry_root);
		proc_entry_root = NULL;
	}
}


