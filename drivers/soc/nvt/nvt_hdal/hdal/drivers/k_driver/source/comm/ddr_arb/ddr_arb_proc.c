#include <linux/random.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ddr_arb_proc.h"
#include "ddr_arb_main.h"
#include "ddr_arb_api.h"
#include "ddr_arb_int.h"
#include "comm/timer.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include <mach/fmem.h>


#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#ifdef CONFIG_HAVE_HW_BREAKPOINT
#define CWP_FUNC   1
#else
#define CWP_FUNC   0
#endif

#ifdef CONFIG_NVT_DMA_MONITOR
#define DPROF_PROC_EN  1
#else
#define DPROF_PROC_EN  0
#endif

#ifdef CONFIG_NVT_SMALL_HDAL
#define HEAVYLOAD_EN 0
#else
#define HEAVYLOAD_EN 1
#endif

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 50
#define MAX_ARG_NUM     6

#define DBG_TEST_EN	(0)
#define DM_DBG_EN	(0)

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
//PXXX_DRV_INFO pdrv_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================

#if (DBG_TEST_EN == 1)
static PROC_CMD cmd_write_list[] = {
        // keyword          function name
        { "auto",           nvt_ddr_arb_api_auto_test          },
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))


static int nvt_ddr_arb_proc_cmd_show(struct seq_file *sfile, void *v)
{
        nvt_dbg(IND, "\n");
        return 0;
}

static int nvt_ddr_arb_proc_cmd_open(struct inode *inode, struct file *file)
{
        nvt_dbg(IND, "\n");
        return single_open(file, nvt_ddr_arb_proc_cmd_show, &pdrv_info_data->module_info);
}

static ssize_t nvt_ddr_arb_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
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
        .open    = nvt_ddr_arb_proc_cmd_open,
        .read    = seq_read,
        .llseek  = seq_lseek,
        .release = single_release,
        .write   = nvt_ddr_arb_proc_cmd_write
};
#endif

//=============================================================================
// proc "help" file operation functions
//=============================================================================
#define DBGUT_DMA_USAGE_HW_PERIOD_MS 25
#define FLGDBGUT_UNKNOWN    FLGPTN_BIT(0)
#define FLGDBGUT_QUIT       FLGPTN_BIT(1)
#define FLGDBGUT_DMA_USAGE  FLGPTN_BIT(2)   //notify task there's command coming
#define FLGDBGUT_DMA2_USAGE FLGPTN_BIT(3)  //notify task there's command coming
#define FLGDBGUT_DMA_HVYLOAD  FLGPTN_BIT(4)   //notify task there's command coming
#define FLGDBGUT_DMA2_HVYLOAD FLGPTN_BIT(5)  //notify task there's command coming
#define FLGDBGUT_QUIT_HVY     FLGPTN_BIT(6)
#define FLGDBGUT_QUIT_HVY2    FLGPTN_BIT(7)


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
	UINT32          efficiency;
	TIMER_ID        timer_id;        ///< used timer id for trigger task
	BOOL            is_start;
	UINT32          reset_cnt;     ///< when remain_cnt is 0, reset remain_cnt to nResetCnt
	INT32           remain_cnt;     ///< if <10 ms 1 time count 1 hits, >10ms 1 time count 10 hits
	UINT64          acc_usage;     ///< account for sum of 10 times usage
	UINT64          acc_efficiency;
	UINT32          interval_cnt;  ///< a count for each interval occured
} DBGUT_CTRL_DMA_USAGE;

static VOID dbgut_dma_timer_cb(UINT32 uiEvent);


static DBGUT_CTRL_DMA_USAGE nvt_ddr_proc_cfg;
#if HEAVYLOAD_EN
static DRAM_CONSUME_ATTR nvt_heavyload_cfg;
static FLGPTN nvt_dram1_heavyload_flag_id;
static THREAD_HANDLE nvt_dram1_heavyload_tsk_id;
static void *handle = NULL;
#endif
static FLGPTN nvt_ddr_proc_flag_id;
static THREAD_HANDLE nvt_ddr_proc_tsk_id;

static UINT64 ddr_cnt[2] = {0, 0};
static int data_monitor[2] = {0, 0};



static int run_dma_usage(void)
{
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;
	UINT64 cal_usage = 0;
	UINT64 cal_efficiency = 0;
	UINT64 cal_mon_period = 0;
	UINT64 cal_bandwidth = 0;
	UINT32 lock;

	p_dma->remain_cnt--;

	p_dma->acc_usage += dma_get_utilization(DDR_ARB_1);
	p_dma->acc_efficiency += dma_get_efficiency(DDR_ARB_1);
	lock = ddr_arb_platform_spin_lock();
	ddr_cnt[0] +=  dma_get_efficiency(DDR_ARB_1);
	ddr_arb_platform_spin_unlock(lock);
	//printk("%lld\r\n", ddr_cnt[0]);

	if (p_dma->remain_cnt <= 0) {
		cal_usage = p_dma->acc_usage*100;
		cal_efficiency = p_dma->acc_efficiency*100;
		cal_mon_period = p_dma->reset_cnt * dma_get_monitor_period(DDR_ARB_1);
		do_div(cal_efficiency, p_dma->acc_usage);
		do_div(cal_usage, cal_mon_period);
		p_dma->usage = (UINT32)cal_usage;
		p_dma->efficiency = (UINT32)cal_efficiency;
		cal_bandwidth = p_dma->acc_efficiency*4;
		do_div(cal_bandwidth, 1024*1024);
		p_dma->acc_usage = 0;
		p_dma->acc_efficiency = 0;
		p_dma->remain_cnt = (INT32)p_dma->reset_cnt;
		if (data_monitor[0] == 0)
			printk("dram1: %d, %d, %lld MB/s\r\n", p_dma->usage, p_dma->efficiency, cal_bandwidth);
		p_dma->interval_cnt++;
	}

	return 0;
}

static THREAD_DECLARE(dbgut_tsk, arglist)
{
	FLGPTN flg_ptn = 0;
	ID flag_id = nvt_ddr_proc_flag_id;
	FLGPTN mask = FLGDBGUT_DMA_USAGE | FLGDBGUT_DMA2_USAGE | FLGDBGUT_QUIT;

	while (1) {
		wai_flg(&flg_ptn, flag_id, mask, TWF_ORW | TWF_CLR);
		if ((flg_ptn & FLGDBGUT_DMA_USAGE) != 0U) {
			run_dma_usage();
		}
		if ((flg_ptn & FLGDBGUT_DMA2_USAGE) != 0U) {
			//run_dma2_usage();
		}
		if ((flg_ptn & FLGDBGUT_QUIT) != 0U) {
			break;
		}
	}
	THREAD_RETURN(0);
}
#if HEAVYLOAD_EN
static THREAD_DECLARE(heavyload_tsk, arglist)
{
	UINT32 count[1];
	UINT32 test1_start, test2_start, test3_start;
	UINT32 test1_size, test2_size, test3_size;
	DMA_HEAVY_LOAD_PARAM hvy_param0, hvy_param1, hvy_param2;
	BOOL err = FALSE;
	FLGPTN flg_ptn = 0;
	ID flag_id = nvt_dram1_heavyload_flag_id;
	FLGPTN mask = FLGDBGUT_DMA_HVYLOAD | FLGDBGUT_QUIT_HVY;

	while (1) {
		wai_flg(&flg_ptn, flag_id, mask, TWF_ORW | TWF_CLR);

		if ((flg_ptn & FLGDBGUT_DMA_HVYLOAD) != 0U) {
			test1_size =
				test2_size =
					test3_size = ALIGN_FLOOR_32(nvt_heavyload_cfg.size / 3);

			test1_start = nvt_heavyload_cfg.addr;
			test2_start = test1_start + test1_size;
			test3_start = test2_start + test2_size;

			get_random_bytes(&count[0], 4);
			count[0] = count[0] & 0xffff;

			hvy_param0.burst_len    = 127;
			hvy_param0.dma_size     = test1_size;
			hvy_param0.test_method  = DMA_HEAVY_LOAD_RW_BURST_INTERLEAVE;
			hvy_param0.start_addr   = test1_start;
			hvy_param0.test_times   = count[0];

			hvy_param1.burst_len    = 126;
			hvy_param1.dma_size     = test2_size;
			hvy_param1.test_method  = DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF;
			hvy_param1.start_addr   = test2_start;
			hvy_param1.test_times   = count[0];

			hvy_param2.burst_len    = 128;
			hvy_param2.dma_size     = test3_size;
			hvy_param2.test_method  = DMA_HEAVY_LOAD_READ_AFTER_WRITE_ALL_BUF;
			hvy_param2.start_addr   = test3_start;
			hvy_param2.test_times   = count[0];

			DBG_IND("*************ConsumeTsk Information*******************\r\n");
			DBG_IND("*   (@)Loading Degree ==================>[     %05d]*\r\n", nvt_heavyload_cfg.load_degree);


			// Easy
			if (nvt_heavyload_cfg.load_degree == DRAM_CONSUME_EASY_LOADING) {
				if (hvy_param0.test_times > 2000) {
					hvy_param0.test_times = 2000;
				}
				DBG_IND("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", hvy_param0.burst_len);
				DBG_IND("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param0.start_addr);
				DBG_IND("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", hvy_param0.dma_size);
				DBG_IND("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", hvy_param0.test_times);
				DBG_IND("*************ConsumeTsk Information*******************\r\n");


				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param0) != E_OK) {
					DBG_ERR("Consume task open fail degree[%d]-1\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}

				dma_trig_heavyload(DDR_ARB_1, 0x1);
			}
			// Normal
			else if (nvt_heavyload_cfg.load_degree == DRAM_CONSUME_NORMAL_LOADING) {
				DBG_IND("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", hvy_param0.burst_len);
				DBG_IND("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param0.start_addr);
				DBG_IND("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", hvy_param0.dma_size);
				DBG_IND("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", hvy_param0.test_times);
				DBG_IND("*   (@)Channel1 Burst Length============>[     %05d]*\r\n", hvy_param1.burst_len);
				DBG_IND("*   (@)Channel1 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param1.start_addr);
				DBG_IND("*   (@)Channel1 Test DMA size ==========>[0x%08x]*\r\n", hvy_param1.dma_size);
				DBG_IND("*   (@)Channel1 Test count  ============>[     %05d]*\r\n", hvy_param1.test_times);
				DBG_IND("*************ConsumeTsk Information*******************\r\n");
				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param1) != E_OK) {
					DBG_ERR("Consume task open fail degree[%d]-1\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}
				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH1, &hvy_param1) != E_OK) {
					DBG_ERR("Consume task open fail degree[%d]-2\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}

				dma_trig_heavyload(DDR_ARB_1, 0x3);
			}
			// Heavy
			else {
				DBG_IND("*   (@)Channel0 Burst Length============>[     %05d]*\r\n", hvy_param0.burst_len);
				DBG_IND("*   (@)Channel0 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param0.start_addr);
				DBG_IND("*   (@)Channel0 Test DMA size ==========>[0x%08x]*\r\n", hvy_param0.dma_size);
				DBG_IND("*   (@)Channel0 Test count  ============>[     %05d]*\r\n", hvy_param0.test_times);
				DBG_IND("*   (@)Channel1 Burst Length============>[     %05d]*\r\n", hvy_param1.burst_len);
				DBG_IND("*   (@)Channel1 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param1.start_addr);
				DBG_IND("*   (@)Channel1 Test DMA size ==========>[0x%08x]*\r\n", hvy_param1.dma_size);
				DBG_IND("*   (@)Channel1 Test count  ============>[     %05d]*\r\n", hvy_param1.test_times);
				DBG_IND("*   (@)Channel2 Burst Length============>[     %05d]*\r\n", hvy_param2.burst_len);
				DBG_IND("*   (@)Channel2 Test DMA addr ==========>[0x%08x]*\r\n", hvy_param2.start_addr);
				DBG_IND("*   (@)Channel2 Test DMA size ==========>[0x%08x]*\r\n", hvy_param2.dma_size);
				DBG_IND("*   (@)Channel2 Test count  ============>[     %05d]*\r\n", hvy_param2.test_times);
				DBG_IND("*************ConsumeTsk Information*******************\r\n");


				//dma_setChannelPriority(DMA_CH_CPU, DMA_PRIORITY_HIGH);
				//dma_setChannelPriority(DMA_CH_CPU2, DMA_PRIORITY_HIGH);
				//dma_setChannelPriority(DMA_CH_HLOAD_0, DMA_PRIORITY_MIDDLE);
				//dma_setChannelPriority(DMA_CH_HLOAD_1, DMA_PRIORITY_MIDDLE);
				//dma_setChannelPriority(DMA_CH_HLOAD_2, DMA_PRIORITY_MIDDLE);

				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH0, &hvy_param0) != E_OK) {
					DBG_ERR("Consume task open fail degree[%d]-1\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}
				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH1, &hvy_param1) != E_OK) {
					DBG_ERR("Consume task open fail degree[%d]-2\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}
				if (dma_enable_heavyload(DDR_ARB_1, DMA_HEAVY_LOAD_CH2, &hvy_param2) != E_OK) {              // Enable maximun 3 channel at the same time
					DBG_ERR("Consume task open fail degree[%d]-3\r\n", nvt_heavyload_cfg.load_degree);
					err = TRUE;
				}

				dma_trig_heavyload(DDR_ARB_1, 0x7);
			}

			if (err == FALSE) {
				if (dma_wait_heavyload_done_polling(DDR_ARB_1, NULL) == TRUE) {
					DBG_IND("SUCCESS\r\n");

					if (nvt_heavyload_cfg.is_start == TRUE)
						set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
					else
						fmem_release_from_cma(handle, 0);
				} else {
					DBG_ERR("FAIL \r\n");
					DBG_ERR("Terminate Dram consume task... \r\n");
				}
			} else {
				nvt_heavyload_cfg.is_start = FALSE;
				fmem_release_from_cma(handle, 0);
			}
		}
		if ((flg_ptn & FLGDBGUT_QUIT_HVY) != 0U) {
			break;
		}
	}
	THREAD_RETURN(0);
}
#endif

static VOID dbgut_dma_timer_cb(UINT32 uiEvent)
{
	set_flg(nvt_ddr_proc_flag_id, FLGDBGUT_DMA_USAGE);
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
	p_dma->efficiency = 0xFFFFFFFF;
	p_dma->acc_efficiency = 0;
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
	UINT32 lock;
	DBGUT_CTRL_DMA_USAGE *p_dma = &nvt_ddr_proc_cfg;

	if (p_dma->is_start) {
		lock = ddr_arb_platform_spin_lock();
		data_monitor[0] = 0;
		ddr_arb_platform_spin_unlock(lock);
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
	UINT32 lock;
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
	lock = ddr_arb_platform_spin_lock();
	ddr_cnt[0] = 0;
	data_monitor[0] = 0;
	ddr_arb_platform_spin_unlock(lock);
    return 0;
}

static UINT32 attrib_mask[DMA_CH_GROUP_CNT] = {0};
static int nvt_ddr_proc_mwp_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	UINT32  pa = 0 , length = 0;
	UINT32  enable = 0;
	UINT32  set_mask = 0;

    if(argc<3){
        DBG_DUMP("mwp [addr] [len] [enable] [set_mask]");
        return -1;
    }

	//sscanf_s(str_cmd, "%x %x", &pa, &length);
	if (argc >= 1) {
		sscanf(pargv[0], "%x", (int *)&pa);
	}
	if (argc >= 2) {
		sscanf(pargv[1], "%x", (int *)&length);
	}
	if (argc >= 3) {
		sscanf(pargv[2], "%d", (int *)&enable);
	}
	if (argc >= 4) {
		sscanf(pargv[3], "%d", (int *)&set_mask);
	}

    DBG_DUMP("pa 0x%x, length= 0x%x  enable %d set_mask %d \r\n", pa, length,enable,set_mask);

    if(enable)
    {
        DMA_WRITEPROT_ATTR attrib = {0};

        memset((void *)&attrib.mask, 0xff, sizeof(DMA_CH_MSK));

        attrib.level = DMA_WPLEL_UNWRITE;
        attrib.protect_rgn_attr[0].en = ENABLE;
        attrib.protect_rgn_attr[0].starting_addr = pa;
        attrib.protect_rgn_attr[0].size = length;

        if(set_mask) {
            memcpy((void *)&attrib.mask, (void *)attrib_mask, sizeof(DMA_CH_MSK));
            {
                UINT32 i=0;
                for(i=0;i<DMA_CH_GROUP_CNT;i++) {
                    DBG_DUMP("0x%x ",attrib_mask[i]);
                }
             }
        }
        arb_enable_wp(DDR_ARB_1, WPSET_0, &attrib);
    }
    else {
        arb_disable_wp(DDR_ARB_1, WPSET_0);
    }
	return 0;
}
#define MASK2_GROUP_START  (4)

static int nvt_ddr_proc_mwp_mask1(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
    UINT32  i=0;

    if(argc<1){
        DBG_DUMP("mask1 [group0] [group1] [group2] [group3]");
        return -1;
    }

    if (argc >= 1) {
        for(i=0;i<MASK2_GROUP_START;i++) {
        	if (argc >= i+1) {
        		sscanf(pargv[i], "%x", (int *)&attrib_mask[i]);
                //DBG_DUMP("mask[%d] 0x%x \n",i,attrib_mask[i]);
        	}
        }
    }

    return 0;
}
static int nvt_ddr_proc_mwp_mask2(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
    UINT32  i=0;

    if(argc<1){
        DBG_DUMP("mask2 [group4] [group5]");
        return -1;
    }

    if (argc >= 1) {
        for(i=MASK2_GROUP_START;i<sizeof(DMA_CH_MSK)/sizeof(UINT32);i++) {
        	if (argc >= (i-MASK2_GROUP_START)+1) {
        		sscanf(pargv[(i-MASK2_GROUP_START)], "%x", (int *)&attrib_mask[i]);
                //DBG_DUMP("mask[%d] 0x%x \n",i,attrib_mask[i]);
        	}
        }
    }

    return 0;
}
#if DPROF_PROC_EN
static int nvt_ddr_proc_monitor_hdl(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int ch, dir;

	if (argc < 3) {
		printk("%s: parameter error\r\n", __func__);
		return -1;
	}
	sscanf(pargv[1], "%d", (int *)&ch);
	sscanf(pargv[2], "%d", (int *)&dir);

	printk("argv: %s\r\n", pargv[0]);
	if (strncmp(pargv[0], "start", 5) == 0) {
		dprof_start(DDR_ARB_1, (DPROF_CH)ch, (DMA_DIRECTION)dir);
	} else if (strncmp(pargv[0], "stop", 4) == 0) {
		dprof_dump_and_stop(DDR_ARB_1);

	} else {
		printk("%s: input %s not support\r\n", __func__, pargv[0]);
		return -1;
	}

	return 0;
}
#endif
#if CWP_FUNC
struct perf_event * __percpu *sample_hbp;

static void sample_hbp_handler(struct perf_event *bp,
           struct perf_sample_data *data,
           struct pt_regs *regs)
{
    printk(KERN_INFO "value is changed\r\n");
    dump_stack();
    printk(KERN_INFO "Dump stack from sample_hbp_handler\r\n");
}

static int cpu_enable_watch(unsigned int wp_addr)
{
   struct perf_event_attr attr;
   int    ret = 0;

   hw_breakpoint_init(&attr);
   attr.bp_addr = wp_addr;
   attr.bp_len = HW_BREAKPOINT_LEN_4;
   attr.bp_type = HW_BREAKPOINT_W;
   printk("\r\ncwp address: 0x%x\r\n", (int)attr.bp_addr);
   sample_hbp = register_wide_hw_breakpoint(&attr, sample_hbp_handler, NULL);
   if (IS_ERR((void __force *)sample_hbp)) {
    ret = PTR_ERR((void __force *)sample_hbp);
    printk("\r\nregister_wide_hw_breakpoint fail\r\n");
  }
  return ret;
}
static void cpu_disable_watch(void)
{
    unregister_wide_hw_breakpoint(sample_hbp);
}

//cat /proc/hdal/comm/task
//protect  Stack Area: va[0x80A8A000 ~ 0x80A8C000]
static int nvt_ddr_proc_cwp_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
    UINT32  enable=0;
    UINT32  addr=0;

    if(argc<1){
        DBG_DUMP("cwp [enable] [v_addr]");
        return -1;
    }

    sscanf(pargv[0], "%d", &enable);
    if(!enable) {
        cpu_disable_watch();
    } else {
        if(argc>=2) {
        	sscanf(pargv[1], "%x", &addr);
            cpu_enable_watch(addr);
        }else {
            DBG_DUMP("cwp [enable] [v_addr]");
            return -1;
        }
    }
	return 0;
}
#else
static int nvt_ddr_proc_cwp_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
    DBG_DUMP("kenrl HW_BREAKPOINT not cfg\r\n");
    return 0;
}
#endif

#if DM_DBG_EN
static int nvt_ddr_data_monitor_start(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int id = 0;


	sscanf(pargv[0], "%d", (int *)&id);


	ddr_data_monitor_start((DDR_ARB)id);

	return 0;
}

static int nvt_ddr_data_monitor_stop(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int id = 0;


	sscanf(pargv[0], "%d", (int *)&id);


	ddr_data_monitor_stop((DDR_ARB)id);

    return 0;
}

static int nvt_ddr_data_monitor_reset(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int id = 0;


	sscanf(pargv[0], "%d", (int *)&id);


	ddr_data_monitor_reset((DDR_ARB)id);

    return 0;
}

static int nvt_ddr_data_monitor_get(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int id = 0;
	UINT64 cnt = 0;
	UINT64 data = 0;


	sscanf(pargv[0], "%d", (int *)&id);

	ddr_data_monitor_get((DDR_ARB)id, &cnt, &data);

	printk("id%d, cnt %lld; data %lld Bytes\r\n", id, cnt, data);

    return 0;
}

#endif

static int nvt_ddr_proc_monitor_start_t(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	mau_ch_mon_start(0, 2, 0);
    return 0;
}
static int nvt_ddr_proc_monitor_stop_t(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	pr_info("CPU %lld bytes\r\n", mau_ch_mon_stop(0, 0));
	return 0;
}


static PROC_CMD nvt_ddr_proc_cmd_write_list[] = {
	// keyword   function name
	{ "cfg",     nvt_ddr_proc_usage_cfg},
	{ "start",   nvt_ddr_proc_usage_start},
	{ "stop",    nvt_ddr_proc_usage_stop},
	{ "mwp",     nvt_ddr_proc_mwp_cfg},
	{ "mask1",   nvt_ddr_proc_mwp_mask1},
	{ "mask2",   nvt_ddr_proc_mwp_mask2},
#if DPROF_PROC_EN
	{ "mon",     nvt_ddr_proc_monitor_hdl},
#endif
	{ "cwp",     nvt_ddr_proc_cwp_cfg},
#if DM_DBG_EN
	{ "dm_start",     nvt_ddr_data_monitor_start},
	{ "dm_stop",     nvt_ddr_data_monitor_stop},
	{ "dm_rst",     nvt_ddr_data_monitor_reset},
	{ "dm_get",     nvt_ddr_data_monitor_get},
#endif
	{ "mon_start", nvt_ddr_proc_monitor_start_t},
	{ "mon_stop", nvt_ddr_proc_monitor_stop_t}

};

#define NVT_DDR_PROC_NUM_OF_WRITE_CMD (sizeof(nvt_ddr_proc_cmd_write_list) / sizeof(PROC_CMD))

static int nvt_ddr_proc_show(struct seq_file *sfile, void *v)
{
	UINT64 cal_usage = 0;
	UINT64 cal_efficiency = 0;
	cal_usage = dma_get_utilization(DDR_ARB_1) * 100;
	do_div(cal_usage, dma_get_monitor_period(DDR_ARB_1));
	cal_efficiency = dma_get_efficiency(DDR_ARB_1) * 100;
	do_div(cal_efficiency, dma_get_utilization(DDR_ARB_1));
	seq_printf(sfile, "utilization: %d\n", (int)cal_usage);
	seq_printf(sfile, "efficiency: %d\n", (int)cal_efficiency);
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

int ddr_data_monitor_start(DDR_ARB id)
{
	UINT32 lock;
	DBGUT_CTRL_DMA_USAGE *p_dma;
	int er;


	p_dma = &nvt_ddr_proc_cfg;

	p_dma->interval_ms = 1000; //default 1 sec

	p_dma->interval_ms = p_dma->interval_ms / DBGUT_DMA_USAGE_HW_PERIOD_MS * DBGUT_DMA_USAGE_HW_PERIOD_MS;

	if (p_dma->interval_ms < DBGUT_DMA_USAGE_HW_PERIOD_MS) {
		p_dma->interval_ms = DBGUT_DMA_USAGE_HW_PERIOD_MS;
	}

	//nvt_dbg(WRN, "truncate interval time of dma usage to %d ms\n", p_dma->interval_ms);

	if (p_dma->is_start) {
		dbgut_timer_unlock(DBGUT_TM_TYPE_DMA);

	}

	// always sample with 25 ms
	p_dma->reset_cnt = p_dma->interval_ms / 25;

	p_dma->usage = 0xFFFFFFFF;
	p_dma->acc_usage = 0;
	p_dma->efficiency = 0xFFFFFFFF;
	p_dma->acc_efficiency = 0;
	p_dma->remain_cnt = (INT32)p_dma->reset_cnt;
	p_dma->cycle_us = p_dma->interval_ms * 1000 / p_dma->reset_cnt;

	if (p_dma->is_start) {

		dbgut_timer_lock(DBGUT_TM_TYPE_DMA);
	}



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
	lock = ddr_arb_platform_spin_lock();
	data_monitor[id] = 1;
	ddr_arb_platform_spin_unlock(lock);
	return 0;

}

int ddr_data_monitor_stop(DDR_ARB id)
{
	UINT32 lock;
	int er;
	DBGUT_CTRL_DMA_USAGE *p_dma;
	p_dma = &nvt_ddr_proc_cfg;


	if (p_dma->is_start == FALSE) {
		nvt_dbg(WRN, "DBGUT_WR_DMA_USAGE_ALREADY_STOPED");
		return 0;
	}

	p_dma->is_start = FALSE;
	lock = ddr_arb_platform_spin_lock();
	ddr_cnt[id] = 0;
	ddr_arb_platform_spin_unlock(lock);

	er = dbgut_timer_unlock(DBGUT_TM_TYPE_DMA);

	if (er != 0) {
		return er;
	}
	lock = ddr_arb_platform_spin_lock();
	data_monitor[id] = 0;
	ddr_arb_platform_spin_unlock(lock);
    return 0;

}

void ddr_data_monitor_reset(DDR_ARB id)
{
	UINT32 lock;
	lock = ddr_arb_platform_spin_lock();
	ddr_cnt[id] = 0;
	ddr_arb_platform_spin_unlock(lock);
}

void ddr_data_monitor_get(DDR_ARB id, UINT64 *cnt, UINT64 *byte)
{
	*cnt = ddr_cnt[id];
	*byte = ddr_cnt[id]*4;
}
#if HEAVYLOAD_EN
static int nvt_dram_heavyload_cfg(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;
	struct nvt_fmem_mem_info_t      buf_info = {0};
	int ret = 0;

	dram_consume_cfg->load_degree = DRAM_CONSUME_HEAVY_LOADING;
	dram_consume_cfg->size = 0x100000;

	if (argc > 0) {
		sscanf(pargv[0], "%d", (UINT32 *)&dram_consume_cfg->load_degree);
		sscanf(pargv[1], "%d", (UINT32 *)&dram_consume_cfg->size);
	}

	ret = nvt_fmem_mem_info_init(&buf_info, NVT_FMEM_ALLOC_CACHE, dram_consume_cfg->size, NULL);
	if (ret >= 0) {
		handle = fmem_alloc_from_cma(&buf_info, 0);
	} else {
		nvt_dbg(WRN, "heavyload allocate mem fail!!\r\n");
		return ret;
	}

	dram_consume_cfg->addr = (UINT32)buf_info.vaddr;
	dram_consume_cfg->is_start = FALSE;

	return 0;
}
static int nvt_dram_heavyload_start(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;

	if (dram_consume_cfg->is_start) {
		nvt_dbg(WRN, "DRAM HEAVYLOAD IS START\r\n");
		return 0;
	}

	dram_consume_cfg->is_start = TRUE;

	set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_DMA_HVYLOAD);
    return 0;
}
static int nvt_dram_heavyload_stop(PMODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DRAM_CONSUME_ATTR *dram_consume_cfg = &nvt_heavyload_cfg;

	if (!dram_consume_cfg->is_start) {
		nvt_dbg(WRN, "DRAM HEAVYLOAD IS NOT START\r\n");
		return 0;
	}
	dma_stop_heavyload(DDR_ARB_1);
	dram_consume_cfg->is_start = FALSE;
    return 0;
}

static PROC_CMD nvt_dram_heavyload_cmd_write_list[] = {
	// keyword   function name
	{ "cfg",     nvt_dram_heavyload_cfg},
	{ "start",   nvt_dram_heavyload_start},
	{ "stop",    nvt_dram_heavyload_stop},
};

#define NVT_DRAM_HEAVYLOAD_NUM_OF_WRITE_CMD (sizeof(nvt_dram_heavyload_cmd_write_list) / sizeof(PROC_CMD))

static int nvt_dram_proc_heavyload_show(struct seq_file *sfile, void *v)
{
    nvt_dbg(IND, "\n");
    return 0;
}

static int nvt_dram_proc_heavyload_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_dram_proc_heavyload_show, NULL);
}

static ssize_t nvt_dram_proc_heavyload_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
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
			for (loop = 0 ; loop < NVT_DRAM_HEAVYLOAD_NUM_OF_WRITE_CMD ; loop++) {
				if (strncmp(argv[1], nvt_dram_heavyload_cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
				        ret = nvt_dram_heavyload_cmd_write_list[loop].execute(NULL, ucargc - 2, &argv[2]);
				        break;
				}
			}
	        if (loop >= NVT_DRAM_HEAVYLOAD_NUM_OF_WRITE_CMD) {
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


static struct file_operations proc_dram_heavyload_fops = {
    .owner  = THIS_MODULE,
    .open   = nvt_dram_proc_heavyload_open,
    .release = single_release,
    .read   = seq_read,
    .llseek = seq_lseek,
    .write   = nvt_dram_proc_heavyload_cmd_write
};
#endif
int nvt_ddr_arb_proc_init(PXXX_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

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
#if HEAVYLOAD_EN
	pentry = proc_create("dram1_heavyload", S_IRUGO | S_IXUGO, pmodule_root, &proc_dram_heavyload_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram1!\n");
		ret = -EINVAL;
		goto remove_dram1_heavyload_proc;
	}
	pdrv_info->pproc_dram1_heavyload_entry = pentry;
#endif
	nvt_ddr_proc_cfg.timer_id = TIMER_INVALID;
	OS_CONFIG_FLAG(nvt_ddr_proc_flag_id);
	THREAD_CREATE(nvt_ddr_proc_tsk_id, dbgut_tsk, NULL, "nvt_ddr_proc_tsk");
	THREAD_RESUME(nvt_ddr_proc_tsk_id);
#if HEAVYLOAD_EN
	OS_CONFIG_FLAG(nvt_dram1_heavyload_flag_id);
	THREAD_CREATE(nvt_dram1_heavyload_tsk_id, heavyload_tsk, NULL, "nvt_dram1_heavyload_tsk");
	THREAD_RESUME(nvt_dram1_heavyload_tsk_id);
#endif



#if (DBG_TEST_EN == 1)
	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
        if (pentry == NULL) {
                nvt_dbg(ERR, "failed to create proc cmd!\n");
                ret = -EINVAL;
                goto remove_cmd;
        }
        pdrv_info->pproc_cmd_entry = pentry;
#endif

	return ret;

#if HEAVYLOAD_EN
remove_dram1_heavyload_proc:
	proc_remove(pdrv_info->pproc_dram1_heavyload_entry);
#endif
remove_cmd:
	proc_remove(pdrv_info->pproc_dram1_entry);

remove_root:
	proc_remove(pdrv_info->pproc_ddr_root);
	return ret;
}

int nvt_ddr_arb_proc_remove(PXXX_DRV_INFO pdrv_info)
{
printk("%s: rm thread 0x%x\r\n", __func__, (UINT32)nvt_ddr_proc_tsk_id);
	set_flg(nvt_ddr_proc_flag_id, FLGDBGUT_QUIT);
	THREAD_DESTROY(nvt_ddr_proc_tsk_id);
printk("%s: rm flag\r\n", __func__);
	rel_flg(nvt_ddr_proc_flag_id);
printk("%s: rm proc\r\n", __func__);
#if HEAVYLOAD_EN
	set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_QUIT_HVY);
	THREAD_DESTROY(nvt_dram1_heavyload_tsk_id);
	rel_flg(nvt_dram1_heavyload_flag_id);
	proc_remove(pdrv_info->pproc_dram1_heavyload_entry);
#endif
	proc_remove(pdrv_info->pproc_dram1_entry);
	proc_remove(pdrv_info->pproc_ddr_root);
	return 0;
}
