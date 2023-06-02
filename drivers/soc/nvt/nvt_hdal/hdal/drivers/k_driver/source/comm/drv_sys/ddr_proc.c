#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <mach/dma_protected.h>
#include "timer_proc.h"
#include "timer_dbg.h"
#include "timer_main.h"
#include "timer_api.h"
#include "comm/timer.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"

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
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
static UINT32 v_setup_tbl[] = {
0xE92D41F0,
0xE1A08000,
0xE3082100,
0xE34F2D00,
0xE3083100,
0xE34F3D10,
0xE3A05000,
0xE59F605C,
0xE59F4068,
0xE0844008,
0xE7941105,
0xE7821105,
0xE7831105,
0xE2855001,
0xE2566001,
0xE59F1050,
0xE0811008,
0x51A0F001,
0xE320F000,
0xE8BD81F0,
0xE320F000,
0xAAA80000,
0x0000002A,
0xAB000000,
0x00AAAEAA,
0x02B00000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x0000000B,
0x00000014,
0xEAFFFFFE,
0xE320F000,
0x00000054,
0x00000028,








};

//============================================================================
// Function define
//============================================================================
static void nvt_ddr_priority(void) {
	UINT32 address = (UINT32)(&v_setup_tbl[0]);

	__asm__("MOV r0, %0\n\t"
		"MOV lr, pc\n\t"
		"MOV pc, %0\n\t"
            :
            :"r"(address)
            : "lr", "r0", "r1", "r2", "r3");
}


//=============================================================================
// proc "help" file operation functions
//=============================================================================
#define DBGUT_DMA_USAGE_HW_PERIOD_MS 25
#define FLGDBGUT_UNKNOWN    FLGPTN_BIT(0)
#define FLGDBGUT_QUIT       FLGPTN_BIT(1)
#define FLGDBGUT_DMA_USAGE  FLGPTN_BIT(2)   //notify task there's command coming
#define FLGDBGUT_DMA2_USAGE FLGPTN_BIT(3)  //notify task there's command coming

/**
     Timer Type.
*/
typedef enum _DBGUT_TM_TYPE {
	DBGUT_TM_TYPE_DMA,
	DBGUT_TM_TYPE_DMA2,
	ENUM_DUMMY4WORD(DBGUT_TM_TYPE)
} DBGUT_TM_TYPE;


typedef struct _DBGUT_CTRL_DMA_USAGE {
	UINT32          interval_ms; ///< time Interval in Ms
	UINT32          cycle_us;
	UINT32          usage;        ///< reported usage, by acc_usage/nResetCnt
	TIMER_ID        timer_id;        ///< used timer id for trigger task
	BOOL            is_start;
	UINT32          reset_cnt;     ///< when remain_cnt is 0, reset remain_cnt to nResetCnt
	INT32           remain_cnt;     ///< if <10 ms 1 time count 1 hits, >10ms 1 time count 10 hits
	UINT32          acc_usage;     ///< account for sum of 10 times usage
	UINT32          interval_cnt;  ///< a count for each interval occured
} DBGUT_CTRL_DMA_USAGE;

static VOID dbgut_dma_timer_cb(UINT32 uiEvent);
static VOID dbgut_dma2_timer_cb(UINT32 uiEvent);


static DBGUT_CTRL_DMA_USAGE nvt_ddr_proc_cfg;
static DBGUT_CTRL_DMA_USAGE nvt_dram2_proc_cfg;
static FLGPTN nvt_ddr_proc_flag_id;
static THREAD_HANDLE nvt_ddr_proc_tsk_id;


static int run_dma_usage(void)
{
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;

	p_dma->remain_cnt--;

	p_dma->acc_usage += dma_get_utilization(DMA_ID_1);

	if (p_dma->remain_cnt <= 0) {
		p_dma->usage = p_dma->acc_usage / p_dma->reset_cnt;
		//p_dma->usage = p_dma->usage;
		p_dma->acc_usage = 0;
		p_dma->remain_cnt = (INT32)p_dma->reset_cnt;
		pr_info("dram1: %d\r\n", p_dma->usage);
		p_dma->interval_cnt++;
	}

	return 0;
}

static int run_dma2_usage(void)
{
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_dram2_proc_cfg;

	p_dma->remain_cnt--;
	p_dma->acc_usage += dma_get_utilization(DMA_ID_2);

	if (p_dma->remain_cnt <= 0) {
		p_dma->usage = p_dma->acc_usage / p_dma->reset_cnt;
		//p_dma->usage = p_dma->usage;
		p_dma->acc_usage = 0;
		p_dma->remain_cnt = (INT32)p_dma->reset_cnt;

		pr_info("dram2: %d\r\n", p_dma->usage);

		p_dma->interval_cnt++;
	}

	return 0;
}

static THREAD_DECLARE(dbgut_tsk, arglist)
{
	FLGPTN flg_ptn;
	ID flag_id = nvt_ddr_proc_flag_id;
	FLGPTN mask = FLGDBGUT_DMA_USAGE | FLGDBGUT_DMA2_USAGE;
	//coverity[no_escape]
	while (1) {
		wai_flg(&flg_ptn, flag_id, mask, TWF_ORW | TWF_CLR);
		if ((flg_ptn & FLGDBGUT_DMA_USAGE) != 0U) {
			run_dma_usage();
		}
		if ((flg_ptn & FLGDBGUT_DMA2_USAGE) != 0U) {
			run_dma2_usage();
		}
		if ((flg_ptn & FLGDBGUT_QUIT) != 0U) {
			break;
		}
	}
	THREAD_RETURN(0);
}



static VOID dbgut_dma_timer_cb(UINT32 uiEvent)
{
	set_flg(nvt_ddr_proc_flag_id, FLGDBGUT_DMA_USAGE);
}

static VOID dbgut_dma2_timer_cb(UINT32 uiEvent)
{
	set_flg(nvt_ddr_proc_flag_id, FLGDBGUT_DMA2_USAGE);
}

static int dbgut_timer_lock(DBGUT_TM_TYPE type)
{
	TIMER_ID *p_id = NULL;
	DRV_CB fp_cb = NULL;
	switch (type) {
	case DBGUT_TM_TYPE_DMA:
		p_id = &nvt_ddr_proc_cfg.timer_id;
		fp_cb = dbgut_dma_timer_cb;
		break;
	case DBGUT_TM_TYPE_DMA2:
		p_id = &nvt_dram2_proc_cfg.timer_id;
		fp_cb = dbgut_dma2_timer_cb;
		break;
	default:
		break;
	}

	if (p_id == NULL) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_LOCK_EXCEPTION");
		return -1;
	}

	if (*p_id != TIMER_INVALID) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_ALREADY_LOCK");
		return -1;
	}

	if (timer_open(p_id, fp_cb) != E_OK) {
		*p_id = TIMER_INVALID;
		nvt_dbg(ERR, "DBGUT_ER_TIMER_LOCK_FAIL");
		return -1;
	}

	return 0;

}

static int dbgut_timer_unlock(DBGUT_TM_TYPE type)
{
	TIMER_ID *p_id = NULL;
	FLGPTN flag = 0;

	switch (type) {
	case DBGUT_TM_TYPE_DMA:
		p_id = &nvt_ddr_proc_cfg.timer_id;
		flag = FLGDBGUT_DMA_USAGE;
		break;
	case DBGUT_TM_TYPE_DMA2:
		p_id = &nvt_dram2_proc_cfg.timer_id;
		flag = FLGDBGUT_DMA2_USAGE;
		break;
	default:
		flag = 0;
		break;
	}

	if (p_id == NULL) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_UNLOCK_EXCEPTION");
		return -1;
	}

	if (*p_id == TIMER_INVALID) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_ALREADY_UNLOCK");
		return -1;
	}

	timer_close(*p_id);

	if (flag) {
		clr_flg(nvt_ddr_proc_flag_id, flag);
	}

	*p_id = TIMER_INVALID;
	return 0;
}

static int dbgut_timer_cfg(DBGUT_TM_TYPE type)
{
	UINT32 uiUs = 0;
	TIMER_ID *p_id = NULL;

	switch (type) {
	case DBGUT_TM_TYPE_DMA:
		p_id = &nvt_ddr_proc_cfg.timer_id;
		uiUs = nvt_ddr_proc_cfg.cycle_us;
		break;
	case DBGUT_TM_TYPE_DMA2:
		p_id = &nvt_dram2_proc_cfg.timer_id;
		uiUs = nvt_dram2_proc_cfg.cycle_us;
		break;
	default:
		break;
	}

	if (p_id == NULL) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_CFG_EXCEPTION");
		return -1;
	}

	if (*p_id == TIMER_INVALID) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_CFG_INVALID_TID");
		return -1;
	}

	if (timer_cfg(*p_id, uiUs, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY) != E_OK) {
		nvt_dbg(ERR, "DBGUT_ER_TIMER_CFG_FAIL");
		return -1;
	}
	return 0;
}

static int nvt_ddr_proc_usage_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;
	p_dma->interval_ms = 1000; //default 1 sec
	if (argc > 0) {
		sscanf(pargv[0], "%d", (int *)&p_dma->interval_ms);
	}

	p_dma->interval_ms = p_dma->interval_ms / DBGUT_DMA_USAGE_HW_PERIOD_MS * DBGUT_DMA_USAGE_HW_PERIOD_MS;

	if (p_dma->interval_ms < DBGUT_DMA_USAGE_HW_PERIOD_MS) {
		p_dma->interval_ms = DBGUT_DMA_USAGE_HW_PERIOD_MS;
	}

	nvt_dbg(WRN, "truncate interval time of dma usage to %d ms\n", p_dma->interval_ms);

	if (p_dma->is_start) {
		dbgut_timer_unlock(DBGUT_TM_TYPE_DMA);
	}

	// always sample with 25 ms
	p_dma->reset_cnt = p_dma->interval_ms / 25;

	p_dma->usage = 0xFFFFFFFF;
	p_dma->acc_usage = 0;
	p_dma->remain_cnt = (INT32)p_dma->reset_cnt;
	p_dma->cycle_us = p_dma->interval_ms * 1000 / p_dma->reset_cnt;

	if (p_dma->is_start) {
		dbgut_timer_lock(DBGUT_TM_TYPE_DMA);
	}
	return 0;
}
static int nvt_ddr_proc_usage_start(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int er;
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;

	if (p_dma->is_start) {
		nvt_dbg(WRN, "DBGUT_WR_DMA_USAGE_START_TWICE");
		return 0;
	}

	p_dma->interval_cnt = 0;
	p_dma->is_start = TRUE;

	er = dbgut_timer_lock(DBGUT_TM_TYPE_DMA);
	if (er != 0) {
		return er;
	}
	er = dbgut_timer_cfg(DBGUT_TM_TYPE_DMA);
	if (er != 0) {
		return er;
	}
    return 0;
}
static int nvt_ddr_proc_usage_stop(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int er;
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;

	if (p_dma->is_start == FALSE) {
		nvt_dbg(WRN, "DBGUT_WR_DMA_USAGE_ALREADY_STOPED");
		return 0;
	}

	p_dma->is_start = FALSE;

	er = dbgut_timer_unlock(DBGUT_TM_TYPE_DMA);
	if (er != 0) {
		return er;
	}
    return 0;
}

static PROC_CMD nvt_ddr_proc_cmd_write_list[] = {
	// keyword   function name
	{ "cfg",     nvt_ddr_proc_usage_cfg},
	{ "start",   nvt_ddr_proc_usage_start},
	{ "stop",    nvt_ddr_proc_usage_stop},
};

#define NVT_DDR_PROC_NUM_OF_WRITE_CMD (sizeof(nvt_ddr_proc_cmd_write_list) / sizeof(PROC_CMD))

static int nvt_ddr_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "utilization: %d\n", dma_get_utilization(DMA_ID_1));
	return 0;
}

static int nvt_ddr_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_ddr_proc_show, NULL);
}

static ssize_t nvt_ddr_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
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
		if (copy_from_user(cmd_line, buf, len)) {
                goto ERR_OUT;
		}

		if (len == 0) {
                cmd_line[0] = '\0';
		} else {
                cmd_line[len - 1] = '\0';
		}

        // parse command string
        for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
                argv[ucargc] = strsep(&cmdstr, delimiters);

                if (argv[ucargc] == NULL)
                    break;
        }

	// dispatch command handler
        if (strncmp(argv[0], "r", 2) == 0) {
			// nothing to do
        } else if (strncmp(argv[0], "w", 2) == 0)  {
			for (loop = 0 ; loop < NVT_DDR_PROC_NUM_OF_WRITE_CMD ; loop++) {
				if (strncmp(argv[1], nvt_ddr_proc_cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				        ret = nvt_ddr_proc_cmd_write_list[loop].execute(NULL, ucargc - 2, &argv[2]);
				        break;
				}
			}
	        if (loop >= NVT_DDR_PROC_NUM_OF_WRITE_CMD) {
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

static struct file_operations proc_ddr_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_ddr_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
    .write   = nvt_ddr_proc_cmd_write
};

static int nvt_dram2_proc_usage_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_dram2_proc_cfg;
	p_dma->interval_ms = 1000; //default 1 sec
	if (argc > 0) {
		sscanf(pargv[0], "%d", (int *)&p_dma->interval_ms);
	}

	p_dma->interval_ms = p_dma->interval_ms / DBGUT_DMA_USAGE_HW_PERIOD_MS * DBGUT_DMA_USAGE_HW_PERIOD_MS;

	if (p_dma->interval_ms < DBGUT_DMA_USAGE_HW_PERIOD_MS) {
		p_dma->interval_ms = DBGUT_DMA_USAGE_HW_PERIOD_MS;
	}

	nvt_dbg(WRN, "truncate interval time of dma usage to %d ms\n", p_dma->interval_ms);

	if (p_dma->is_start) {
		dbgut_timer_unlock(DBGUT_TM_TYPE_DMA2);
	}

	// always sample with 25 ms
	p_dma->reset_cnt = p_dma->interval_ms / 25;

	p_dma->usage = 0xFFFFFFFF;
	p_dma->acc_usage = 0;
	p_dma->remain_cnt = (INT32)p_dma->reset_cnt;
	p_dma->cycle_us = p_dma->interval_ms * 1000 / p_dma->reset_cnt;

	if (p_dma->is_start) {
		dbgut_timer_lock(DBGUT_TM_TYPE_DMA2);
	}
	return 0;
}
static int nvt_dram2_proc_usage_start(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int er;
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_dram2_proc_cfg;

	if (p_dma->is_start) {
		nvt_dbg(WRN, "DBGUT_WR_DMA_USAGE_START_TWICE");
		return 0;
	}

	p_dma->interval_cnt = 0;
	p_dma->is_start = TRUE;

	er = dbgut_timer_lock(DBGUT_TM_TYPE_DMA2);
	if (er != 0) {
		return er;
	}
	er = dbgut_timer_cfg(DBGUT_TM_TYPE_DMA2);
	if (er != 0) {
		return er;
	}
    return 0;
}
static int nvt_dram2_proc_usage_stop(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int er;
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_dram2_proc_cfg;

	if (p_dma->is_start == FALSE) {
		nvt_dbg(WRN, "DBGUT_WR_DMA_USAGE_ALREADY_STOPED");
		return 0;
	}

	p_dma->is_start = FALSE;

	er = dbgut_timer_unlock(DBGUT_TM_TYPE_DMA2);
	if (er != 0) {
		return er;
	}
    return 0;
}

static PROC_CMD nvt_dram2_proc_cmd_write_list[] = {
	// keyword   function name
	{ "cfg",     nvt_dram2_proc_usage_cfg},
	{ "start",   nvt_dram2_proc_usage_start},
	{ "stop",    nvt_dram2_proc_usage_stop},
};

#define NVT_DRAM2_PROC_NUM_OF_WRITE_CMD (sizeof(nvt_dram2_proc_cmd_write_list) / sizeof(PROC_CMD))

static int nvt_dram2_proc_show(struct seq_file *sfile, void *v)
{
        seq_printf(sfile, "utilization: %d\n", dma_get_utilization(DMA_ID_2));
        return 0;
}

static int nvt_dram2_proc_help_open(struct inode *inode, struct file *file)
{
        return single_open(file, nvt_dram2_proc_show, NULL);
}

static ssize_t nvt_dram2_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
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
		if (copy_from_user(cmd_line, buf, len)) {
                goto ERR_OUT;
		}

		if (len == 0) {
                cmd_line[0] = '\0';
		} else {
                cmd_line[len - 1] = '\0';
		}

        // parse command string
        for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
                argv[ucargc] = strsep(&cmdstr, delimiters);

                if (argv[ucargc] == NULL)
                    break;
        }

	// dispatch command handler
        if (strncmp(argv[0], "r", 2) == 0) {
			// nothing to do
        } else if (strncmp(argv[0], "w", 2) == 0)  {
			for (loop = 0 ; loop < NVT_DRAM2_PROC_NUM_OF_WRITE_CMD ; loop++) {
				if (strncmp(argv[1], nvt_dram2_proc_cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				        ret = nvt_dram2_proc_cmd_write_list[loop].execute(NULL, ucargc - 2, &argv[2]);
				        break;
				}
			}
	        if (loop >= NVT_DRAM2_PROC_NUM_OF_WRITE_CMD) {
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


static struct file_operations proc_dram2_fops = {
    .owner  = THIS_MODULE,
    .open   = nvt_dram2_proc_help_open,
    .release = single_release,
    .read   = seq_read,
    .llseek = seq_lseek,
    .write   = nvt_dram2_proc_cmd_write
};

int nvt_ddr_proc_init(PXXX_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	nvt_ddr_priority();

	pmodule_root = proc_mkdir("nvt_drv_sys", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}
	pdrv_info->pproc_ddr_root = pmodule_root;


	pentry = proc_create("dram1_info", S_IRUGO | S_IXUGO, pmodule_root, &proc_ddr_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram1!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_dram1_entry = pentry;


	pentry = proc_create("dram2_info", S_IRUGO | S_IXUGO, pmodule_root, &proc_dram2_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram2!\n");
		ret = -EINVAL;
		goto remove_dram2_proc;
	}
	pdrv_info->pproc_dram2_entry = pentry;

	nvt_ddr_proc_cfg.timer_id = TIMER_INVALID;
	nvt_dram2_proc_cfg.timer_id = TIMER_INVALID;
	OS_CONFIG_FLAG(nvt_ddr_proc_flag_id);
	THREAD_CREATE(nvt_ddr_proc_tsk_id, dbgut_tsk, NULL, "nvt_ddr_proc_tsk");
	THREAD_RESUME(nvt_ddr_proc_tsk_id);


	return ret;

remove_dram2_proc:
	proc_remove(pdrv_info->pproc_dram2_entry);

remove_cmd:
	proc_remove(pdrv_info->pproc_dram1_entry);

remove_root:
	proc_remove(pdrv_info->pproc_ddr_root);
	return ret;
}

int nvt_ddr_proc_remove(PXXX_DRV_INFO pdrv_info)
{
	THREAD_DESTROY(nvt_ddr_proc_tsk_id);
	rel_flg(nvt_ddr_proc_flag_id);
	proc_remove(pdrv_info->pproc_dram1_entry);
	proc_remove(pdrv_info->pproc_ddr_root);
	return 0;
}
