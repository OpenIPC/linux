#include "isp_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "awb_alg.h"
#include "awbt_api.h"

#include "awb_dbg.h"
#include "awb_main.h"
#include "awb_proc.h"

#if 1
#define AWB_PROC_REDUCE 0
#else
#if defined (CONFIG_NVT_SMALL_HDAL)
#define AWB_PROC_REDUCE 1
#else
#define AWB_PROC_REDUCE 0
#endif
#endif

//=============================================================================
// avoid GPL api
//=============================================================================
#define MUTEX_ENABLE DISABLE

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
#if MUTEX_ENABLE
static struct semaphore mutex;
#endif
static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_info;
static struct proc_dir_entry *proc_command;
static struct proc_dir_entry *proc_help;
static UINT32 ca_f = AWB_ID_MAX_NUM;
static UINT32 flag_f = AWB_ID_MAX_NUM;
static UINT32 param_f = AWB_ID_MAX_NUM;

//=============================================================================
// routines
//=============================================================================
static inline ISP_MODULE *awb_proc_get_mudule_from_file(struct file *file)
{
	return (ISP_MODULE *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 awb_proc_info_show(struct seq_file *sfile, void *v)
{
	awb_flow_show_info(sfile);

	return 0;
}

static INT32 awb_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, awb_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations awb_proc_info_fops = {
	.owner	 = THIS_MODULE,
	.open	 = awb_proc_info_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "command" file operation functions
//=============================================================================
static AWB_MSG_BUF g_cmd_msg;

static inline INT32 awb_alloc_msg_buf(void)
{
	g_cmd_msg.buf = isp_uti_mem_alloc(PROC_MSG_BUFSIZE);

	if (g_cmd_msg.buf == NULL) {
		DBG_ERR("fail to allocate AWB message buffer!\n");
		return -E_SYS;
	}

	g_cmd_msg.size = PROC_MSG_BUFSIZE;
	g_cmd_msg.count = 0;

	return 0;
}

static inline void awb_free_msg_buf(void)
{
	if (g_cmd_msg.buf) {
		isp_uti_mem_free(g_cmd_msg.buf);
		g_cmd_msg.buf = NULL;
	}
}

static inline void awb_clean_msg_buf(void)
{
	g_cmd_msg.buf[0] = '\0';
	g_cmd_msg.count = 0;
}

static INT32 awb_cmd_printf(const s8 *f, ...)
{
	INT32 len;
	va_list args;

	if (g_cmd_msg.count < g_cmd_msg.size) {
		va_start(args, f);
		len = vsnprintf(g_cmd_msg.buf + g_cmd_msg.count, g_cmd_msg.size - g_cmd_msg.count, f, args);
		va_end(args);

		if ((g_cmd_msg.count + len) < g_cmd_msg.size) {
			g_cmd_msg.count += len;

			return 0;
		}
	}

	g_cmd_msg.count = g_cmd_msg.size;

	return -1;
}

static INT32 awb_proc_cmd_get_dbg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	u32 cmd;

	awb_cmd_printf("awb_proc_cmd_get_dbg \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	cmd = awb_get_dbg_mode(id);
	awb_cmd_printf("0: general info / BIT0: SYNC / BIT1: loag cfg / BIT4: awb uart cmd / BIT5: dbg flow / BIT6: dbg alg\r\n");
	awb_cmd_printf("id =%d, debug message 0x%08X \n", id, cmd);

	return 0;
}

static INT32 awb_proc_cmd_set_dbg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	u32 level = 0;
	u32 freq = 0;

	awb_cmd_printf("awb_proc_cmd_set_dbg \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	if (argc == 4) {
		level = simple_strtoul(argv[3], NULL, 0);
		freq = 60;
	} else if (argc == 5) {
		level = simple_strtoul(argv[3], NULL, 0);
		freq = simple_strtoul(argv[4], NULL, 0);
	} else {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	if (freq < 1) {
		freq = 1;
		awb_cmd_printf("invalid frequency: %d; frequency is more then 0\r\n", freq);
	}

	awb_cmd_printf("awb dbg mode id: %d, level: %d, frequency: %d \n", id, level, freq);
	awb_set_dbg_mode(id, level, freq);
	return 0;
}

#if (!AWB_PROC_REDUCE)
static INT32 awb_proc_cmd_get_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;

	awb_cmd_printf("awb_proc_cmd_get_param \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	param_f = id;

	return 0;
}

static INT32 awb_proc_cmd_set_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;

	awb_cmd_printf("awb_proc_cmd_set_param \n");

	if (argc < 6) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set element1 element2 value\r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	if (strncmp(argv[3], "th", 20) == 0) {
		if (strncmp(argv[4], "y_l", 10) == 0) {
			awb_param[id]->th->y_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_u", 10) == 0) {
			awb_param[id]->th->y_u = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_l", 10) == 0) {
			awb_param[id]->th->rpb_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_u", 10) == 0) {
			awb_param[id]->th->rpb_u = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_l", 10) == 0) {
			awb_param[id]->th->rsb_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_u", 10) == 0) {
			awb_param[id]->th->rsb_u = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r2g_l", 10) == 0) {
			awb_param[id]->th->r2g_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r2g_u", 10) == 0) {
			awb_param[id]->th->r2g_u = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b2g_l", 10) == 0) {
			awb_param[id]->th->b2g_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b2g_u", 10) == 0) {
			awb_param[id]->th->b2g_u = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rmb_l", 10) == 0) {
			awb_param[id]->th->rmb_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rmb_u", 10) == 0) {
			awb_param[id]->th->rmb_u = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "lv", 20) == 0) {
		if (strncmp(argv[4], "night_l", 10) == 0) {
			awb_param[id]->lv->night_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_h", 10) == 0) {
			awb_param[id]->lv->night_h = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_l", 10) == 0) {
			awb_param[id]->lv->in_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_h", 10) == 0) {
			awb_param[id]->lv->in_h = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_l", 10) == 0) {
			awb_param[id]->lv->out_l = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_u", 10) == 0) {
			awb_param[id]->lv->out_h = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, lv element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "ct_weight", 20) == 0) {
		if (strncmp(argv[4], "ctmp_0", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ctmp_1", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ctmp_2", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ctmp_3", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ctmp_4", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ctmp_5", 20) == 0) {
			awb_param[id]->ct_weight->ctmp[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_0", 20) == 0) {
			awb_param[id]->ct_weight->cx[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_1", 20) == 0) {
			awb_param[id]->ct_weight->cx[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_2", 20) == 0) {
			awb_param[id]->ct_weight->cx[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_3", 20) == 0) {
			awb_param[id]->ct_weight->cx[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_4", 20) == 0) {
			awb_param[id]->ct_weight->cx[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_5", 20) == 0) {
			awb_param[id]->ct_weight->cx[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_0", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_1", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_2", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_3", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_4", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "out_weight_5", 20) == 0) {
			awb_param[id]->ct_weight->out_weight[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_0", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_1", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_2", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_3", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_4", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "in_weight_5", 20) == 0) {
			awb_param[id]->ct_weight->in_weight[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_0", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_1", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_2", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_3", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_4", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "night_weight_5", 20) == 0) {
			awb_param[id]->ct_weight->night_weight[5] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "target", 20) == 0) {
		if (strncmp(argv[4], "cx_l", 10) == 0) {
			awb_param[id]->target->cx[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_m", 10) == 0) {
			awb_param[id]->target->cx[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "cx_h", 10) == 0) {
			awb_param[id]->target->cx[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rg_l", 10) == 0) {
			awb_param[id]->target->rg_ratio[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rg_m", 10) == 0) {
			awb_param[id]->target->rg_ratio[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rg_h", 10) == 0) {
			awb_param[id]->target->rg_ratio[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "bg_l", 10) == 0) {
			awb_param[id]->target->bg_ratio[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "bg_m", 10) == 0) {
			awb_param[id]->target->bg_ratio[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "bg_h", 10) == 0) {
			awb_param[id]->target->bg_ratio[2] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, target element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "ct_info", 20) == 0) {
		if (strncmp(argv[4], "temperature_0", 20) == 0) {
			awb_param[id]->ct_info->temperature[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "temperature_1", 20) == 0) {
			awb_param[id]->ct_info->temperature[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "temperature_2", 20) == 0) {
			awb_param[id]->ct_info->temperature[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "temperature_3", 20) == 0) {
			awb_param[id]->ct_info->temperature[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "temperature_4", 20) == 0) {
			awb_param[id]->ct_info->temperature[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "temperature_5", 20) == 0) {
			awb_param[id]->ct_info->temperature[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_0", 20) == 0) {
			awb_param[id]->ct_info->r_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_1", 20) == 0) {
			awb_param[id]->ct_info->r_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_2", 20) == 0) {
			awb_param[id]->ct_info->r_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_3", 20) == 0) {
			awb_param[id]->ct_info->r_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_4", 20) == 0) {
			awb_param[id]->ct_info->r_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_5", 20) == 0) {
			awb_param[id]->ct_info->r_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_0", 20) == 0) {
			awb_param[id]->ct_info->g_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_1", 20) == 0) {
			awb_param[id]->ct_info->g_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_2", 20) == 0) {
			awb_param[id]->ct_info->g_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_3", 20) == 0) {
			awb_param[id]->ct_info->g_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_4", 20) == 0) {
			awb_param[id]->ct_info->g_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_5", 20) == 0) {
			awb_param[id]->ct_info->g_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_0", 20) == 0) {
			awb_param[id]->ct_info->b_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_1", 20) == 0) {
			awb_param[id]->ct_info->b_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_2", 20) == 0) {
			awb_param[id]->ct_info->b_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_3", 20) == 0) {
			awb_param[id]->ct_info->b_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_4", 20) == 0) {
			awb_param[id]->ct_info->b_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_5", 20) == 0) {
			awb_param[id]->ct_info->b_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "mwb_gain", 20) == 0) {
		if (strncmp(argv[4], "r_gain_0", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_1", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_2", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_3", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_4", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_5", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_6", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[6] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_7", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[7] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_8", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[8] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_9", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[9] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_10", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[10] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "r_gain_11", 10) == 0) {
			awb_param[id]->mwb_gain->r_gain[11] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_0", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_1", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_2", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_3", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_4", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_5", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_6", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[6] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_7", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[7] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_8", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[8] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_9", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[9] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_10", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[10] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "g_gain_11", 10) == 0) {
			awb_param[id]->mwb_gain->g_gain[11] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_0", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_1", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_2", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_3", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_4", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_5", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_6", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[6] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_7", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[7] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_8", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[8] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_9", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[9] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_10", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[10] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "b_gain_11", 10) == 0) {
			awb_param[id]->mwb_gain->b_gain[11] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "converge", 20) == 0) {
		if (strncmp(argv[4], "skip_frame", 10) == 0) {
			awb_param[id]->converge->skip_frame = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "speed", 10) == 0) {
			awb_param[id]->converge->speed = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "tolerance", 10) == 0) {
			awb_param[id]->converge->tolerance = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, converge element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "expand_block", 20) == 0) {
		if (strncmp(argv[4], "mode_0", 10) == 0) {
			awb_param[id]->expand_block->mode[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "mode_1", 10) == 0) {
			awb_param[id]->expand_block->mode[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "mode_2", 10) == 0) {
			awb_param[id]->expand_block->mode[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "mode_3", 10) == 0) {
			awb_param[id]->expand_block->mode[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_l_0", 10) == 0) {
			awb_param[id]->expand_block->lv_l[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_l_1", 10) == 0) {
			awb_param[id]->expand_block->lv_l[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_l_2", 10) == 0) {
			awb_param[id]->expand_block->lv_l[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_l_3", 10) == 0) {
			awb_param[id]->expand_block->lv_l[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_h_0", 10) == 0) {
			awb_param[id]->expand_block->lv_h[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_h_1", 10) == 0) {
			awb_param[id]->expand_block->lv_h[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_h_2", 10) == 0) {
			awb_param[id]->expand_block->lv_h[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "lv_h_3", 10) == 0) {
			awb_param[id]->expand_block->lv_h[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_l_0", 10) == 0) {
			awb_param[id]->expand_block->y_l[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_l_1", 10) == 0) {
			awb_param[id]->expand_block->y_l[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_l_2", 10) == 0) {
			awb_param[id]->expand_block->y_l[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_l_3", 10) == 0) {
			awb_param[id]->expand_block->y_l[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_u_0", 10) == 0) {
			awb_param[id]->expand_block->y_u[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_u_1", 10) == 0) {
			awb_param[id]->expand_block->y_u[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_u_2", 10) == 0) {
			awb_param[id]->expand_block->y_u[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_u_3", 10) == 0) {
			awb_param[id]->expand_block->y_u[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_l_0", 10) == 0) {
			awb_param[id]->expand_block->rpb_l[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_l_1", 10) == 0) {
			awb_param[id]->expand_block->rpb_l[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_l_2", 10) == 0) {
			awb_param[id]->expand_block->rpb_l[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_l_3", 10) == 0) {
			awb_param[id]->expand_block->rpb_l[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_u_0", 10) == 0) {
			awb_param[id]->expand_block->rpb_u[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_u_1", 10) == 0) {
			awb_param[id]->expand_block->rpb_u[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_u_2", 10) == 0) {
			awb_param[id]->expand_block->rpb_u[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rpb_u_3", 10) == 0) {
			awb_param[id]->expand_block->rpb_u[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_l_0", 10) == 0) {
			awb_param[id]->expand_block->rsb_l[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_l_1", 10) == 0) {
			awb_param[id]->expand_block->rsb_l[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_l_2", 10) == 0) {
			awb_param[id]->expand_block->rsb_l[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_l_3", 10) == 0) {
			awb_param[id]->expand_block->rsb_l[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_u_0", 10) == 0) {
			awb_param[id]->expand_block->rsb_u[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_u_1", 10) == 0) {
			awb_param[id]->expand_block->rsb_u[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_u_2", 10) == 0) {
			awb_param[id]->expand_block->rsb_u[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "rsb_u_3", 10) == 0) {
			awb_param[id]->expand_block->rsb_u[3] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else if (strncmp(argv[3], "luma_weight", 20) == 0) {
		if (strncmp(argv[4], "en", 10) == 0) {
			awb_param[id]->luma_weight->en = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_0", 10) == 0) {
			awb_param[id]->luma_weight->y[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_1", 10) == 0) {
			awb_param[id]->luma_weight->y[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_2", 10) == 0) {
			awb_param[id]->luma_weight->y[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_3", 10) == 0) {
			awb_param[id]->luma_weight->y[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_4", 10) == 0) {
			awb_param[id]->luma_weight->y[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_5", 10) == 0) {
			awb_param[id]->luma_weight->y[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_6", 10) == 0) {
			awb_param[id]->luma_weight->y[6] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "y_7", 10) == 0) {
			awb_param[id]->luma_weight->y[7] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_0", 10) == 0) {
			awb_param[id]->luma_weight->w[0] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_1", 10) == 0) {
			awb_param[id]->luma_weight->w[1] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "ww_2", 10) == 0) {
			awb_param[id]->luma_weight->w[2] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_3", 10) == 0) {
			awb_param[id]->luma_weight->w[3] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_4", 10) == 0) {
			awb_param[id]->luma_weight->w[4] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_5", 10) == 0) {
			awb_param[id]->luma_weight->w[5] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_6", 10) == 0) {
			awb_param[id]->luma_weight->w[6] = simple_strtoul(argv[5], NULL, 0);
		} else if (strncmp(argv[4], "w_7", 10) == 0) {
			awb_param[id]->luma_weight->w[7] = simple_strtoul(argv[5], NULL, 0);
		} else {
			awb_cmd_printf("input error, th element: %s\r\n", argv[4]);
			return -EINVAL;
		}
	} else {
		awb_cmd_printf("input error, awb_param element: %s\r\n", argv[3]);
		return -EINVAL;
	}
	awb_cmd_printf("set awb_param %s->%s : %s\r\n", argv[3], argv[4], argv[5]);

	return 0;
}

static INT32 awb_proc_cmd_get_wb(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	AWBALG_GAIN wb_gain;

	awb_cmd_printf("awb_proc_cmd_get_wb \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}
	awb_flow_get_wb(id, &wb_gain);
	awb_cmd_printf("get wb id =%d,(Rgain, Ggain, Bgain) = (%d, %d, %d) \n", id, wb_gain.r_gain,
		wb_gain.g_gain, wb_gain.b_gain);

	return 0;
}

static INT32 awb_proc_cmd_set_wb(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	AWB_MANUAL mwb;

	awb_cmd_printf("awb_proc_cmd_set_wb \n");

	if (argc < 7) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id en rgain gain bgain\r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	mwb.en = simple_strtoul(argv[3], NULL, 0);
	mwb.r_gain = simple_strtoul(argv[4], NULL, 0);
	mwb.g_gain = simple_strtoul(argv[5], NULL, 0);
	mwb.b_gain = simple_strtoul(argv[6], NULL, 0);

	awb_flow_set_mwb(id, &mwb);

	awb_cmd_printf("set wb id =%d, en =%d, (Rgain, Ggain, Bgain) = (%d, %d, %d) \n", id, mwb.en,
		mwb.r_gain, mwb.g_gain, mwb.b_gain);

	return 0;
}
#endif

static INT32 awb_proc_cmd_get_ca(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;

	awb_cmd_printf("awb_proc_cmd_get_ca \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	ca_f = id;

	return 0;
}

static INT32 awb_proc_cmd_get_flag(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;

	awb_cmd_printf("awb_proc_cmd_get_flag \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	flag_f = id;

	return 0;
}

#if (!AWB_PROC_REDUCE)
static INT32 awb_proc_cmd_get_ui_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 ui_scene;
	UINT32 ui_r_ratio;
	UINT32 ui_b_ratio;

	awb_cmd_printf("awb_proc_cmd_get_ui_param \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	ui_scene = awb_ui_get_info(id, AWB_UI_SCENE);
	ui_r_ratio = awb_ui_get_info(id, AWB_UI_WB_R_RATIO);
	ui_b_ratio = awb_ui_get_info(id, AWB_UI_WB_B_RATIO);

	awb_cmd_printf("id : %d\r\n", id);
	awb_cmd_printf("get ui scene : %d\r\n", ui_scene);
	awb_cmd_printf("get ui Rratio : %d\r\n", ui_r_ratio);
	awb_cmd_printf("get ui Bratio : %d\r\n", ui_b_ratio);

	return 0;
}

static INT32 awb_proc_cmd_set_ui_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	AWB_UI_INFO type;
	UINT32 value;

	awb_cmd_printf("awb_proc_cmd_set_ui_param \n");

	if (argc < 5) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id, type, value\r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	type = simple_strtoul(argv[3], NULL, 0);
	value = simple_strtoul(argv[4], NULL, 0);

	awb_cmd_printf("id : %d\r\n", id);
	awb_cmd_printf("set ui type : %d, value : %d\r\n", type, value);

	awb_ui_set_info(id, type, value);

	return 0;
}

static INT32 awb_proc_cmd_get_expand_ct(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 expand_ct_en;

	awb_cmd_printf("awb_proc_cmd_get_expand_ct \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}
	awb_flow_get_expand_ct_en(id, &expand_ct_en);
	awb_cmd_printf("id = %d, expand_ct_en = %d \n", id, expand_ct_en);

	return 0;
}

static INT32 awb_proc_cmd_set_expand_ct(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 en;

	awb_cmd_printf("awb_proc_cmd_set_expand_ct \n");

	if (argc < 4) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id en \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	en = simple_strtoul(argv[3], NULL, 0);
	awb_flow_set_expand_ct_en(id, en);

	awb_cmd_printf("id =%d, expand_ct_en =%d \n", id, en);
	return 0;
}

static INT32 awb_proc_cmd_get_weight_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 weight_en;
	UINT32 weight_ct_idx;

	awb_cmd_printf("awb_proc_cmd_get_weight_mode \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}
	awb_flow_get_weight_mode(id, &weight_en, &weight_ct_idx);
	awb_cmd_printf("id = %d, weight_mode = %d , ct_number = %d \n", id, weight_en, weight_ct_idx);

	return 0;
}

static INT32 awb_proc_cmd_set_weight_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 weight_en;
	UINT32 weight_ct_idx;

	awb_cmd_printf("awb_proc_cmd_set_weight_mode \n");

	if (argc < 4) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id en \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	weight_en = simple_strtoul(argv[3], NULL, 0);
	awb_flow_set_weight_mode(id, weight_en);
	awb_flow_get_weight_mode(id, &weight_en, &weight_ct_idx);

	awb_cmd_printf("id = %d, weight_mode = %d , ct_number = %d \n", id, weight_en, weight_ct_idx);
	return 0;
}

static INT32 awb_proc_cmd_get_gray_world_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 gray_en;

	awb_cmd_printf("awb_proc_cmd_get_gray_world_en \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}
	awb_flow_get_gray_world_en(id, &gray_en);
	awb_cmd_printf("id = %d, gray_world_en = %d \n", id, gray_en);

	return 0;
}

static INT32 awb_proc_cmd_set_gray_world_en(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	UINT32 gray_en;

	awb_cmd_printf("awb_proc_cmd_set_gray_world_en \n");

	if (argc < 4) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id en \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	gray_en = simple_strtoul(argv[3], NULL, 0);
	awb_flow_set_gray_world_en(id, gray_en);

	awb_cmd_printf("id = %d, gray_world_en = %d \n", id, gray_en);
	return 0;
}

static INT32 awb_proc_cmd_get_ca_th(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	AWBALG_CA_THRESHOLD ca_th;

	awb_cmd_printf("awb_proc_cmd_get_ca_th \n");

	if (argc < 3) {
		awb_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}
	awb_flow_get_ca_th(id, &ca_th);
	awb_cmd_printf("id = %d, en = %d, mode = %d  , g(%d,%d), r(%d,%d), b(%d,%d), p(%d,%d) \n", id, ca_th.enable, ca_th.mode,
		ca_th.g_l, ca_th.g_u, ca_th.r_l, ca_th.r_u, ca_th.b_l, ca_th.b_u, ca_th.p_l, ca_th.p_u);

	return 0;
}

static INT32 awb_proc_cmd_set_ca_th(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWB_ID id;
	AWBALG_CA_THRESHOLD ca_th;

	awb_cmd_printf("awb_proc_cmd_set_ca_th \n");

	if (argc < 13) {
		awb_cmd_printf("wrong argument:%d\r\n", argc);
		awb_cmd_printf("please set id, en, mode, parameter[8] \r\n");
		return -EINVAL;
	}
	id = simple_strtoul(argv[2], NULL, 0);

	if (!awb_flow_get_id_valid(id)) {
		awb_cmd_printf("invalid id: %d\r\n", id);
		return -EINVAL;
	}

	ca_th.enable = simple_strtoul(argv[3], NULL, 0);
	ca_th.mode = simple_strtoul(argv[4], NULL, 0);
	ca_th.g_l = simple_strtoul(argv[5], NULL, 0);
	ca_th.g_u = simple_strtoul(argv[6], NULL, 0);
	ca_th.r_l = simple_strtoul(argv[7], NULL, 0);
	ca_th.r_u = simple_strtoul(argv[8], NULL, 0);
	ca_th.b_l = simple_strtoul(argv[9], NULL, 0);
	ca_th.b_u = simple_strtoul(argv[10], NULL, 0);
	ca_th.p_l = simple_strtoul(argv[11], NULL, 0);
	ca_th.p_u = simple_strtoul(argv[12], NULL, 0);
	awb_flow_set_ca_th(id, &ca_th);

	awb_cmd_printf("id = %d, en = %d, mode = %d  , g(%d,%d), r(%d,%d), b(%d,%d), p(%d,%d) \n", id, ca_th.enable, ca_th.mode,
		ca_th.g_l, ca_th.g_u, ca_th.r_l, ca_th.r_u, ca_th.b_l, ca_th.b_u, ca_th.p_l, ca_th.p_u);
	return 0;
}
#endif

static INT32 awb_proc_cmd_reload_cfg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWBT_CFG_INFO cfg_info = {0};

	if (argc < 4) {
		return -EINVAL;
	}
	cfg_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(cfg_info.path, argv[3], AWB_CFG_NAME_LENGTH-1);
	cfg_info.path[AWB_CFG_NAME_LENGTH-1] = '\0';

	awbt_api_set_cmd(AWBT_ITEM_RLD_CONFIG, (UINT32)&cfg_info);

	return 0;
}

static INT32 awb_proc_cmd_reload_dtsi(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AWBT_DTSI_INFO dtsi_info = {0};

	if (argc < 4) {
		return -EINVAL;
	}
	dtsi_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(dtsi_info.node_path, argv[3], AWB_DTSI_NAME_LENGTH-1);
	dtsi_info.node_path[AWB_DTSI_NAME_LENGTH-1] = '\0';
	awbt_api_set_cmd(AWBT_ITEM_RLD_DTSI, (UINT32)&dtsi_info);
	strncpy(dtsi_info.file_path, argv[4], AWB_DTSI_NAME_LENGTH - 1);
	dtsi_info.file_path[AWB_DTSI_NAME_LENGTH - 1] = '\0';

	return 0;
}

static AWB_PROC_CMD awb_proc_r_cmd_list[] = {
	// keyword              function name
	#if (!AWB_PROC_REDUCE)
	{ "param",              awb_proc_cmd_get_param,          "get parameter, param1 is awb_id(0~2)"},
	#endif
	{ "dbg",                awb_proc_cmd_get_dbg,            "get awb dbg level, param1 is awb_id(0~2)"},
	#if (!AWB_PROC_REDUCE)
	{ "wb",                 awb_proc_cmd_get_wb,             "get wb gain, param1 is awb_id(0~2)"},
	#endif
	{ "ca",                 awb_proc_cmd_get_ca,             "get ca, param1 is awb_id(0~2)"},
	{ "flag",               awb_proc_cmd_get_flag,           "get flag, param1 is awb_id(0~2)"},
	#if (!AWB_PROC_REDUCE)
	{ "ui_param",           awb_proc_cmd_get_ui_param,       "get ui parameter, param1 is awb_id(0~2)"},
	{ "expand_ct",          awb_proc_cmd_get_expand_ct,      "get expand_ct_en, param1 is awb_id(0~2)"},
	{ "ct_wet_mode",        awb_proc_cmd_get_weight_mode,    "get ct weight mode, param1 is awb_id(0~2)"},
	{ "gray_world",         awb_proc_cmd_get_gray_world_en,  "get gray world en, param1 is awb_id(0~2)"},
	{ "ca_th",              awb_proc_cmd_get_ca_th,          "get ca th en, param1 is awb_id(0~2)"}
	#endif
};
#define NUM_OF_R_CMD (sizeof(awb_proc_r_cmd_list) / sizeof(AWB_PROC_CMD))

static AWB_PROC_CMD awb_proc_w_cmd_list[] = {
	// keyword              function name
	#if (!AWB_PROC_REDUCE)
	{ "param",              awb_proc_cmd_set_param,          "set parameter, param1 is awb_id(0~2), param2 is elenent1, param2 is element2, param3 is value"},
	#endif
	{ "dbg",                awb_proc_cmd_set_dbg,            "set awb dbg level, param1 is awb_id(0~2), param2 is dbg level, param3 is frequency(1~120)"},
	#if (!AWB_PROC_REDUCE)
	{ "wb",                 awb_proc_cmd_set_wb,             "set wb gain, param1 is awb_id(0~2), param2 is en(0 or 1), param3 is Rgain(1~2047), param4 is Ggain(1~2047), param5 is Bgain(1~2047)"},
	{ "ui_param",           awb_proc_cmd_set_ui_param,       "set ui parameter, param1 is awb_id(0~2), param2 is type(0~3), param3 is value"},
	{ "expand_ct",          awb_proc_cmd_set_expand_ct,      "set expand ct, param1 is awb_id(0~2), param2 is en(0 or 1)"},
	{ "ct_wet_mode",        awb_proc_cmd_set_weight_mode,    "set ct weight mode, param1 is awb_id(0~2), param2 is en(0 or 1)"},
	{ "gray_world",         awb_proc_cmd_set_gray_world_en,  "set gray world en, param1 is awb_id(0~2), param2 is en(0 or 1)"},
	{ "ca_th",              awb_proc_cmd_set_ca_th,          "set ca th en, param1 is awb_id(0~2), param2 is en(0 or 1)"},
	#endif
	{ "reload_cfg",         awb_proc_cmd_reload_cfg,         "reload cfg file, param1 is awb_id(0~2), param2 is path of cfg file"},
	{ "reload_dtsi",        awb_proc_cmd_reload_dtsi,        "reload dtsi file, param1 is iq_id(0~2), param2 is node path, param3 is file path"}
};
#define NUM_OF_W_CMD (sizeof(awb_proc_w_cmd_list) / sizeof(AWB_PROC_CMD))

static ssize_t awb_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;

	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	INT32 argc = 0;
	UINT32 i;
	ISP_MODULE *awb_module = awb_proc_get_mudule_from_file(file);

	// check command length
	if ((len <= 1) || (len > MAX_CMDLINE_LENGTH)) {
		DBG_ERR("command is too short or long!\n");
	} else {
		// copy command string from user space
		if (copy_from_user(cmd_line, buf, len)) {
			;
		} else {
			cmd_line[len-1] = '\0';

			// parse command string
			for (i = 0; i < MAX_CMD_ARGUMENTS; i++) {
				argv[i] = strsep(&cmdstr, delimiters);
				if (argv[i] != NULL) {
					argc++;
				} else {
					break;
				}
			}

			// dispatch command handler
			ret = -EINVAL;

			if (strncmp(argv[0], "r", 2) == 0) {
				for (i = 0; i < NUM_OF_R_CMD; i++) {
					if (strncmp(argv[1], awb_proc_r_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = awb_proc_r_cmd_list[i].execute(awb_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					DBG_ERR("[AWB_ERR]: => ");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in r_cmd_list!\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], awb_proc_w_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = awb_proc_w_cmd_list[i].execute(awb_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[AWB_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!\n");
				}
			} else {
				DBG_ERR("[AWB_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[AWB_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\n");
	}

	return size;
}

static INT32 awb_proc_command_show(struct seq_file *sfile, void *v)
{
	if (g_cmd_msg.buf == NULL && ca_f >= AWB_ID_MAX_NUM && flag_f >= AWB_ID_MAX_NUM && param_f >= AWB_ID_MAX_NUM) {
		return -EINVAL;
	}

	if (g_cmd_msg.buf > 0) {
		seq_printf(sfile, "%s\n", g_cmd_msg.buf);
		awb_clean_msg_buf();
	}
	awb_flow_get_ca_msg(sfile, ca_f);
	ca_f = AWB_ID_MAX_NUM;
	awb_flow_get_flag(sfile, flag_f);
	flag_f = AWB_ID_MAX_NUM;

	//print parameter
	if (param_f < AWB_ID_MAX_NUM) {

		seq_printf(sfile, "th \n");
		seq_printf(sfile, "y_l   %4d \n", awb_param[param_f]->th->y_l);
		seq_printf(sfile, "y_u   %4d \n", awb_param[param_f]->th->y_u);
		seq_printf(sfile, "rpb_l %4d \n", awb_param[param_f]->th->rpb_l);
		seq_printf(sfile, "rpb_u %4d \n", awb_param[param_f]->th->rpb_u);
		seq_printf(sfile, "rsb_l %4d \n", awb_param[param_f]->th->rsb_l);
		seq_printf(sfile, "rsb_u %4d \n", awb_param[param_f]->th->rsb_u);
		seq_printf(sfile, "r2g_l %4d \n", awb_param[param_f]->th->r2g_l);
		seq_printf(sfile, "r2g_u %4d \n", awb_param[param_f]->th->r2g_u);
		seq_printf(sfile, "b2g_l %4d \n", awb_param[param_f]->th->b2g_l);
		seq_printf(sfile, "b2g_u %4d \n", awb_param[param_f]->th->b2g_u);
		seq_printf(sfile, "rmb_l %4d \n", awb_param[param_f]->th->rmb_l);
		seq_printf(sfile, "rmb_u %4d \n", awb_param[param_f]->th->rmb_u);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "lv \n");
		seq_printf(sfile, "night %4d %4d \n", awb_param[param_f]->lv->night_l, awb_param[param_f]->lv->night_h);
		seq_printf(sfile, "in    %4d %4d \n", awb_param[param_f]->lv->in_l, awb_param[param_f]->lv->in_h);
		seq_printf(sfile, "out   %4d %4d \n", awb_param[param_f]->lv->out_l, awb_param[param_f]->lv->out_h);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "ct_weight \n");
		seq_printf(sfile, "ctmp    %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_weight->ctmp[0], awb_param[param_f]->ct_weight->ctmp[1], awb_param[param_f]->ct_weight->ctmp[2], awb_param[param_f]->ct_weight->ctmp[3], awb_param[param_f]->ct_weight->ctmp[4], awb_param[param_f]->ct_weight->ctmp[5]);
		seq_printf(sfile, "cx      %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_weight->cx[0], awb_param[param_f]->ct_weight->cx[1], awb_param[param_f]->ct_weight->cx[2], awb_param[param_f]->ct_weight->cx[3], awb_param[param_f]->ct_weight->cx[4], awb_param[param_f]->ct_weight->cx[5]);
		seq_printf(sfile, "out_w   %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_weight->out_weight[0], awb_param[param_f]->ct_weight->out_weight[1], awb_param[param_f]->ct_weight->out_weight[2], awb_param[param_f]->ct_weight->out_weight[3], awb_param[param_f]->ct_weight->out_weight[4], awb_param[param_f]->ct_weight->out_weight[5]);
		seq_printf(sfile, "in_w    %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_weight->in_weight[0], awb_param[param_f]->ct_weight->in_weight[1], awb_param[param_f]->ct_weight->in_weight[2], awb_param[param_f]->ct_weight->in_weight[3], awb_param[param_f]->ct_weight->in_weight[4], awb_param[param_f]->ct_weight->in_weight[5]);
		seq_printf(sfile, "night_w %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_weight->night_weight[0], awb_param[param_f]->ct_weight->night_weight[1], awb_param[param_f]->ct_weight->night_weight[2], awb_param[param_f]->ct_weight->night_weight[3], awb_param[param_f]->ct_weight->night_weight[4], awb_param[param_f]->ct_weight->night_weight[5]);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "target ratio \n");
		seq_printf(sfile, "   L-CT M-CT H-CT \n");
		seq_printf(sfile, "cx %4d %4d %4d \n", awb_param[param_f]->target->cx[0], awb_param[param_f]->target->cx[1], awb_param[param_f]->target->cx[2]);
		seq_printf(sfile, "rg %4d %4d %4d \n", awb_param[param_f]->target->rg_ratio[0], awb_param[param_f]->target->rg_ratio[1], awb_param[param_f]->target->rg_ratio[2]);
		seq_printf(sfile, "bg %4d %4d %4d \n", awb_param[param_f]->target->bg_ratio[0], awb_param[param_f]->target->bg_ratio[1], awb_param[param_f]->target->bg_ratio[2]);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "ct_info \n");
		seq_printf(sfile, "color temperature    %5d %5d %5d %5d %5d %5d\n", awb_param[param_f]->ct_info->temperature[0], awb_param[param_f]->ct_info->temperature[1], awb_param[param_f]->ct_info->temperature[2],
			awb_param[param_f]->ct_info->temperature[3], awb_param[param_f]->ct_info->temperature[4], awb_param[param_f]->ct_info->temperature[5]);
		seq_printf(sfile, "Rgain %5d %5d %5d %5d %5d %5d \n", awb_param[param_f]->ct_info->r_gain[0], awb_param[param_f]->ct_info->r_gain[1], awb_param[param_f]->ct_info->r_gain[2],
			awb_param[param_f]->ct_info->r_gain[3], awb_param[param_f]->ct_info->r_gain[4], awb_param[param_f]->ct_info->r_gain[5]);
		seq_printf(sfile, "Ggain %5d %5d %5d %5d %5d %5d \n", awb_param[param_f]->ct_info->g_gain[0], awb_param[param_f]->ct_info->g_gain[1], awb_param[param_f]->ct_info->g_gain[2],
			awb_param[param_f]->ct_info->g_gain[3], awb_param[param_f]->ct_info->g_gain[4], awb_param[param_f]->ct_info->g_gain[5]);
		seq_printf(sfile, "Bgain %5d %5d %5d %5d %5d %5d \n", awb_param[param_f]->ct_info->b_gain[0], awb_param[param_f]->ct_info->b_gain[1], awb_param[param_f]->ct_info->b_gain[2],
			awb_param[param_f]->ct_info->b_gain[3], awb_param[param_f]->ct_info->b_gain[4], awb_param[param_f]->ct_info->b_gain[5]);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "mwb_gain \n");
		seq_printf(sfile, "Rgain %d %d %d %d %d %d %d %d %d %d %d %d \n", awb_param[param_f]->mwb_gain->r_gain[0], awb_param[param_f]->mwb_gain->r_gain[1], awb_param[param_f]->mwb_gain->r_gain[2],
			awb_param[param_f]->mwb_gain->r_gain[3], awb_param[param_f]->mwb_gain->r_gain[4], awb_param[param_f]->mwb_gain->r_gain[5], awb_param[param_f]->mwb_gain->r_gain[6], awb_param[param_f]->mwb_gain->r_gain[7],
			awb_param[param_f]->mwb_gain->r_gain[8], awb_param[param_f]->mwb_gain->r_gain[9], awb_param[param_f]->mwb_gain->r_gain[10], awb_param[param_f]->mwb_gain->r_gain[11]);
		seq_printf(sfile, "Ggain %d %d %d %d %d %d %d %d %d %d %d %d \n", awb_param[param_f]->mwb_gain->g_gain[0], awb_param[param_f]->mwb_gain->g_gain[1], awb_param[param_f]->mwb_gain->g_gain[2],
			awb_param[param_f]->mwb_gain->g_gain[3], awb_param[param_f]->mwb_gain->g_gain[4], awb_param[param_f]->mwb_gain->g_gain[5], awb_param[param_f]->mwb_gain->g_gain[6], awb_param[param_f]->mwb_gain->g_gain[7],
			awb_param[param_f]->mwb_gain->g_gain[8], awb_param[param_f]->mwb_gain->g_gain[9], awb_param[param_f]->mwb_gain->g_gain[10], awb_param[param_f]->mwb_gain->g_gain[11]);
		seq_printf(sfile, "Bgain %d %d %d %d %d %d %d %d %d %d %d %d \n", awb_param[param_f]->mwb_gain->b_gain[0], awb_param[param_f]->mwb_gain->b_gain[1], awb_param[param_f]->mwb_gain->b_gain[2],
			awb_param[param_f]->mwb_gain->b_gain[3], awb_param[param_f]->mwb_gain->b_gain[4], awb_param[param_f]->mwb_gain->b_gain[5], awb_param[param_f]->mwb_gain->b_gain[6], awb_param[param_f]->mwb_gain->b_gain[7],
			awb_param[param_f]->mwb_gain->b_gain[8], awb_param[param_f]->mwb_gain->b_gain[9], awb_param[param_f]->mwb_gain->b_gain[10], awb_param[param_f]->mwb_gain->b_gain[11]);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "converge \n");
		seq_printf(sfile, "skip frame %3d \n", awb_param[param_f]->converge->skip_frame);
		seq_printf(sfile, "speed      %3d \n", awb_param[param_f]->converge->speed);
		seq_printf(sfile, "tolerance  %3d \n", awb_param[param_f]->converge->tolerance);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "expand_block \n");
		seq_printf(sfile, "mode  %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->mode[0], awb_param[param_f]->expand_block->mode[1], awb_param[param_f]->expand_block->mode[2], awb_param[param_f]->expand_block->mode[3]);
		seq_printf(sfile, "lv_l  %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->lv_l[0], awb_param[param_f]->expand_block->lv_l[1], awb_param[param_f]->expand_block->lv_l[2], awb_param[param_f]->expand_block->lv_l[3]);
		seq_printf(sfile, "lv_u  %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->lv_h[0], awb_param[param_f]->expand_block->lv_h[1], awb_param[param_f]->expand_block->lv_h[2], awb_param[param_f]->expand_block->lv_h[3]);
		seq_printf(sfile, "y_l   %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->y_l[0], awb_param[param_f]->expand_block->y_l[1], awb_param[param_f]->expand_block->y_l[2], awb_param[param_f]->expand_block->y_l[3]);
		seq_printf(sfile, "y_u   %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->y_u[0], awb_param[param_f]->expand_block->y_u[1], awb_param[param_f]->expand_block->y_u[2], awb_param[param_f]->expand_block->y_u[3]);
		seq_printf(sfile, "rpb_l %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->rpb_l[0], awb_param[param_f]->expand_block->rpb_l[1], awb_param[param_f]->expand_block->rpb_l[2], awb_param[param_f]->expand_block->rpb_l[3]);
		seq_printf(sfile, "rpb_u %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->rpb_u[0], awb_param[param_f]->expand_block->rpb_u[1], awb_param[param_f]->expand_block->rpb_u[2], awb_param[param_f]->expand_block->rpb_u[3]);
		seq_printf(sfile, "rsb_l %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->rsb_l[0], awb_param[param_f]->expand_block->rsb_l[1], awb_param[param_f]->expand_block->rsb_l[2], awb_param[param_f]->expand_block->rsb_l[3]);
		seq_printf(sfile, "rsb_u %4d %4d %4d %4d \n", awb_param[param_f]->expand_block->rsb_u[0], awb_param[param_f]->expand_block->rsb_u[1], awb_param[param_f]->expand_block->rsb_u[2], awb_param[param_f]->expand_block->rsb_u[3]);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "luma weight \n");
		seq_printf(sfile, "en %4d \n", awb_param[param_f]->luma_weight->en);
		seq_printf(sfile, "y  %4d %4d %4d %4d %4d %4d %4d %4d \n", awb_param[param_f]->luma_weight->y[0], awb_param[param_f]->luma_weight->y[1], awb_param[param_f]->luma_weight->y[2], awb_param[param_f]->luma_weight->y[3],
			awb_param[param_f]->luma_weight->y[4], awb_param[param_f]->luma_weight->y[5], awb_param[param_f]->luma_weight->y[6], awb_param[param_f]->luma_weight->y[7]);
		seq_printf(sfile, "w  %4d %4d %4d %4d %4d %4d %4d %4d \n", awb_param[param_f]->luma_weight->w[0], awb_param[param_f]->luma_weight->w[1], awb_param[param_f]->luma_weight->w[2], awb_param[param_f]->luma_weight->w[3],
			awb_param[param_f]->luma_weight->w[4], awb_param[param_f]->luma_weight->w[5], awb_param[param_f]->luma_weight->w[6], awb_param[param_f]->luma_weight->w[7]);
		seq_printf(sfile, "\n");

		param_f = AWB_ID_MAX_NUM;
	}

	return 0;
}

static INT32 awb_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, awb_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, awb_proc_command_show, PDE_DATA(inode), 7000*sizeof(u32));

}

static const struct file_operations awb_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = awb_proc_command_open,
	.read    = seq_read,
	.write   = awb_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

static int awb_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/awb/info' will show all the awb info\r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/awb/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "  %s\n", "awb");
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", awb_proc_r_cmd_list[loop].cmd, awb_proc_r_cmd_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", awb_proc_w_cmd_list[loop].cmd, awb_proc_w_cmd_list[loop].text);
	}

	seq_printf(sfile, "EXAMPLE\n");
	seq_printf(sfile, "======================================================================================================================================\n");
	seq_printf(sfile, " 'cat /proc/hdal/vendor/awb/help'                                                                     - Get AWB command help\n");
	seq_printf(sfile, "======================================================================================================================================\n");
	seq_printf(sfile, " 'echo w open ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                           - Open AWB\n");
	seq_printf(sfile, " 'echo w close ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                          - Close AWB\n");
	seq_printf(sfile, " 'echo w init ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                           - Init AWB\n");
	seq_printf(sfile, " 'echo w trig ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                           - Trig AWB\n");
	seq_printf(sfile, " 'echo w pause ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                          - Pause AWB\n");
	seq_printf(sfile, " 'echo w resume ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                         - Resume AWB\n");
	seq_printf(sfile, "======================================================================================================================================\n");
	seq_printf(sfile, " 'echo r dbg ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                            - Get AWB debug message\n");
	seq_printf(sfile, " 'echo w dbg ID VALUE Freq > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                 - Set AWB debug message as VALUE\n");
	seq_printf(sfile, " 'echo r param ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                          - Get AWB parameter\n");
	seq_printf(sfile, " 'echo w param ID ELEMENT1 ELEMENT2 VALUE > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'  - Set AWB parameter\n");
	seq_printf(sfile, " 'echo r wb ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                             - Get AWB gain\n");
	seq_printf(sfile, " 'echo w wb ID EN RGAIN GGAIN BGAIN > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'        - Set AWB gain\n");
	seq_printf(sfile, " 'echo r ca ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                             - Get AWB ca\n");
	seq_printf(sfile, " 'echo r flag ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                           - Get AWB flag\n");
	seq_printf(sfile, " 'echo r ui_param ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                       - Get AWB ui parameter\n");
	seq_printf(sfile, " 'echo w ui_param ID TYPE VALUE > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'            - Set AWB ui parameter\n");
	seq_printf(sfile, " 'echo r expand_ct ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                      - Get AWB expand ct enable\n");
	seq_printf(sfile, " 'echo w expand_ct ID ENABLE > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'               - Set AWB expand ct enable\n");
	seq_printf(sfile, " 'echo r ct_wet_mode ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                    - Get AWB ct weight auto mode en\n");
	seq_printf(sfile, " 'echo w ct_wet_mode ID ENABLE > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'             - Set AWB ct weight auto mode en\n");
	seq_printf(sfile, " 'echo r gray_world ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                     - Get AWB gary mode enable\n");
	seq_printf(sfile, " 'echo w gray_world ID ENABLE > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'              - Set AWB gary mode enable\n");
	seq_printf(sfile, " 'echo r ca_th ID > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'                          - Get AWB ca th parameter\n");
	seq_printf(sfile, " 'echo w ca_th ID ENABLE mode par[0~7] > /proc/hdal/vendor/awb/cmd;cat /proc/hdal/vendor/awb/cmd'     - Set AWB ca th parameter\n");
	seq_printf(sfile, "======================================================================================================================================\n");
	seq_printf(sfile, "dbg level------------------------------------------------------------------------------------------------------------|\n");
	seq_printf(sfile, "| BIT          | dbg msg        | level                                                                              |\n");
	seq_printf(sfile, "|--------------|----------------|------------------------------------------------------------------------------------|\n");
	seq_printf(sfile, "|              | disable dbg msg| 0x0                                                                                |\n");
	seq_printf(sfile, "| BIT0         | SYNC           | 0x1                                                                                |\n");
	seq_printf(sfile, "| BIT1         | loag cfg       | 0x2                                                                                |\n");
	seq_printf(sfile, "| BIT4         | awb uart cmd   | 0x10                                                                               |\n");
	seq_printf(sfile, "| BIT5         | dbg flow       | 0x20                                                                               |\n");
	seq_printf(sfile, "| BIT6         | dbg alg        | 0x40                                                                               |\n");
	seq_printf(sfile, "| BIT7         | dbg ca         | 0x80                                                                               |\n");
	seq_printf(sfile, "parameter------------------------------------------------------------------------------------------------------------|\n");
	seq_printf(sfile, "| ELEMENT1     | ELEMENT2                                                                                            |\n");
	seq_printf(sfile, "|--------------|----------------|------------------------------------------------------------------------------------|\n");
	seq_printf(sfile, "| ca_info      | win_num_x      | win_num_y      | rg_u           | rg_l           | gg_u           | gg_l           |\n");
	seq_printf(sfile, "|              | bg_u           | bg_l           | rbg_u          | rbg_l          |                |                |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|--------------------------------------------------|\n");
	seq_printf(sfile, "| th           | y_l            | y_u            | rpb_l          | rpb_u          | rsb_l          | rsb_u          |\n");
	seq_printf(sfile, "|              | r2g_l          | r2g_u          | b2g_l          | b2g_u          | rmb_l          | rmb_u          |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|----------------|----------------|----------------|\n");
	seq_printf(sfile, "| lv           | night_l        | night_h        | in_l           | in_h           | out_l          | out_h          |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|----------------|----------------|----------------|\n");
	seq_printf(sfile, "| ct_weight    | ctmp_0         | ctmp_1         | ctmp_2         | ctmp_3         | ctmp_4         | ctmp_5         |\n");
	seq_printf(sfile, "|              | cx_0           | cx_1           | cx_2           | cx_3           | cx_4           | cx_5           |\n");
	seq_printf(sfile, "|              | out_weight_0   | out_weight_1   | out_weight_2   | out_weight_3   | out_weight_4   | out_weight_5   |\n");
	seq_printf(sfile, "|              | in_weight_0    | in_weight_1    | in_weight_2    | in_weight_3    | in_weight_4    | in_weight_5    |\n");
	seq_printf(sfile, "|              | night_weight_0 | night_weight_1 | night_weight_2 | night_weight_3 | night_weight_4 | night_weight_5 |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|--------------------------------------------------|\n");
	seq_printf(sfile, "| target       | cx_l           | cx_m           | cx_h           |                                                  |\n");
	seq_printf(sfile, "|              | rg_l           | rg_m           | rg_h           |                                                  |\n");
	seq_printf(sfile, "|              | bg_l           | bg_m           | bg_h           |                                                  |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|--------------------------------------------------|\n");
	seq_printf(sfile, "| ct_info      | temperature_0  | temperature_1  | temperature_2  | temperature_3  | temperature_4  | temperature_5  |\n");
	seq_printf(sfile, "|              | r_gain_0       | r_gain_1       | r_gain_2       | r_gain_3       | r_gain_4       | r_gain_5       |\n");
	seq_printf(sfile, "|              | g_gain_0       | g_gain_1       | g_gain_2       | g_gain_3       | g_gain_4       | g_gain_5       |\n");
	seq_printf(sfile, "|              | b_gain_0       | b_gain_1       | b_gain_2       | b_gain_3       | b_gain_4       | b_gain_5       |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|----------------|----------------|----------------|\n");
	seq_printf(sfile, "| mwb_gain     | r_gain_0       | r_gain_1       | r_gain_2       | r_gain_3       | r_gain_4       | r_gain_5       |\n");
	seq_printf(sfile, "|              | r_gain_6       | r_gain_7       | r_gain_8       | r_gain_9       | r_gain_10      | r_gain_11      |\n");
	seq_printf(sfile, "|              | g_gain_0       | g_gain_1       | g_gain_2       | g_gain_3       | g_gain_4       | g_gain_5       |\n");
	seq_printf(sfile, "|              | g_gain_6       | g_gain_7       | g_gain_8       | g_gain_9       | g_gain_10      | g_gain_11      |\n");
	seq_printf(sfile, "|              | b_gain_0       | b_gain_1       | b_gain_2       | b_gain_3       | b_gain_4       | b_gain_5       |\n");
	seq_printf(sfile, "|              | b_gain_6       | b_gain_7       | b_gain_8       | b_gain_9       | b_gain_10      | b_gain_11      |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|--------------------------------------------------|\n");
	seq_printf(sfile, "| converge     | skip_frame     | speed          | tolerance      |                                                  |\n");
	seq_printf(sfile, "|--------------|----------------|----------------|----------------|--------------------------------------------------|\n");
	seq_printf(sfile, "| expand_block | mode_0         | mode_1         | mode_2         | mode_3         |                                 |\n");
	seq_printf(sfile, "|              | lv_l_0         | lv_l_1         | lv_l_2         | lv_l_3         |                                 |\n");
	seq_printf(sfile, "|              | lv_h_0         | lv_h_1         | lv_h_2         | lv_h_3         |                                 |\n");
	seq_printf(sfile, "|              | y_l_0          | y_l_1          | y_l_2          | y_l_3          |                                 |\n");
	seq_printf(sfile, "|              | y_u_0          | y_u_1          | y_u_2          | y_u_3          |                                 |\n");
	seq_printf(sfile, "|              | rpb_l_0        | rpb_l_1        | rpb_l_2        | rpb_l_3        |                                 |\n");
	seq_printf(sfile, "|              | rpb_u_0        | rpb_u_1        | rpb_u_2        | rpb_u_3        |                                 |\n");
	seq_printf(sfile, "|              | rsb_l_0        | rsb_l_1        | rsb_l_2        | rsb_l_3        |                                 |\n");
	seq_printf(sfile, "|              | rsb_u_0        | rsb_u_1        | rsb_u_2        | rsb_u_3        |                                 |\n");
	seq_printf(sfile, "|--------------------------------------------------------------------------------------------------------------------|\n");
	seq_printf(sfile, "| luma_weight  | en             |                |                |                |                                 |\n");
	seq_printf(sfile, "|              | y_0            | y_1            | y_2            | y_3            |                                 |\n");
	seq_printf(sfile, "|              | y_4            | y_5            | y_6            | y_7            |                                 |\n");
	seq_printf(sfile, "|              | w_0            | w_1            | w_2            | w_3            |                                 |\n");
	seq_printf(sfile, "|              | w_4            | w_5            | w_6            | w_7            |                                 |\n");
	seq_printf(sfile, "|--------------------------------------------------------------------------------------------------------------------|\n");

	return 0;
}

static INT32 awb_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, awb_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations awb_proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = awb_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// extern functions
//=============================================================================

INT32 awb_proc_create(void)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	#if MUTEX_ENABLE
	sema_init(&mutex, 1);
	#endif

	root = proc_mkdir("hdal/vendor/awb", NULL);

	if (root == NULL) {
		DBG_ERR("fail to create AWB proc root!\n");
		awb_proc_remove();
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, proc_root, &awb_proc_info_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!\n");
		awb_proc_remove();
		return -EINVAL;
	}
	proc_info = pentry;

	// create "cmd" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, proc_root, &awb_proc_command_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc command!\n");
		awb_proc_remove();
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, proc_root, &awb_proc_help_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc command!\n");
		awb_proc_remove();
		return -EINVAL;
	}
	proc_help = pentry;

	// allocate memory for massage buffer
	ret = awb_alloc_msg_buf();
	if (ret < 0) {
		awb_proc_remove();
	}

	return ret;
}

void awb_proc_remove(void)
{
	if (proc_root == NULL) {
		return;
	}

	// remove "info"
	if (proc_info) {
		proc_remove(proc_info);
	}

	// remove root entry
	proc_remove(proc_root);

	// free message buffer
	awb_free_msg_buf();
}

