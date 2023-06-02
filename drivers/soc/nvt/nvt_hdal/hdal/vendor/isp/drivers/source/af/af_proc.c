#include "af_alg.h"
#include "aft_api.h"

#include "isp_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "af_dbg.h"
#include "af_main.h"
#include "af_proc.h"

#if defined (CONFIG_NVT_SMALL_HDAL)
#define AF_PROC_REDUCE 1
#else
#define AF_PROC_REDUCE 0
#endif

//=============================================================================
// avoid GPL api
//=============================================================================
#define MUTEX_ENABLE DISABLE

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


//=============================================================================
// routines
//=============================================================================
static inline ISP_MODULE *af_proc_get_mudule_from_file(struct file *file)
{
	return (ISP_MODULE *)((struct seq_file *)file->private_data)->private;
}

// NOTE: mark
#if 0
static inline AF_PARAM *s(UINT32 id)
{
	id = AFALG_MIN(id, (AF_ID_MAX_NUM - 1));

	return af_param[id];
}
#endif


//=============================================================================
// proc "info" file operation functions
//=============================================================================
// TODO:
#if 0
static const INT8 *af_method_name[] = {
	"AF_STEP_SEARCH",
	"AF_POLY_SEARCH",
	"AF_HYBRID_SEARCH",
};

static const INT8 *af_mode_name[] = {
	"AF_MODE_SINGLE",
	"AF_MODE_CONTINUOUS",
};

static const INT8 *af_state_name[] = {
	"AF_STE_INIT",
	"AF_STE_ROUGH_STAGE",
	"AF_STE_FINE_STAGE",
	"AF_STE_FINAL_STAGE",
	"AF_STE_CONVERGE",
	"AF_STE_SCENE_CHG_DET",
	"AF_STE_UNKNOWN",
};
#endif

static INT32 af_proc_info_show(struct seq_file *sfile, void *v)
{
	af_flow_show_info(sfile);

	return 0;
}

static INT32 af_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, af_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations af_proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = af_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "command" file operation functions
//=============================================================================
static AF_MSG_BUF g_cmd_msg;

static inline INT32 af_alloc_msg_buf(void)
{
	g_cmd_msg.buf = isp_uti_mem_alloc(PROC_MSG_BUFSIZE);

	if (g_cmd_msg.buf == NULL) {
		DBG_ERR("fail to allocate AF message buffer!\n");
		return E_SYS;
	}

	g_cmd_msg.size = PROC_MSG_BUFSIZE;
	g_cmd_msg.count = 0;

	return 0;
}

static inline void af_free_msg_buf(void)
{
	if (g_cmd_msg.buf) {
		isp_uti_mem_free(g_cmd_msg.buf);
		g_cmd_msg.buf = NULL;
	}
}

static inline void af_clean_msg_buf(void)
{
	g_cmd_msg.buf[0] = '\0';
	g_cmd_msg.count = 0;
}

static INT32 af_cmd_printf(const s8 *f, ...)
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

static INT32 af_proc_get_cmd_enable(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF is %s\n", (*(af_param[id]->func_en) == TRUE) ? "enable" : "disable");

	return 0;
}

static INT32 af_proc_set_cmd_enable(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->func_en) = (simple_strtoul(argv[3], NULL, 0) != 0) ? TRUE : FALSE;

	return 0;
}

//#if (!AF_PROC_REDUCE)
static INT32 af_proc_get_cmd_alg_method(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF algorithm method: %u\n", *(af_param[id]->alg_method));

	return 0;
}

static INT32 af_proc_set_cmd_alg_method(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->alg_method) = AFALG_MIN(simple_strtoul(argv[3], NULL, 0), AF_ALG_BACK_FORWARD);

	return 0;
}
//#endif

static INT32 af_proc_get_cmd_shot_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF shot mode: %u\n", *(af_param[id]->shot_mode));

	return 0;
}

static INT32 af_proc_set_cmd_shot_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->shot_mode) = AFALG_MIN(simple_strtoul(argv[3], NULL, 0), AF_MODE_CONTINUOUS);

	return 0;
}

#if (!AF_PROC_REDUCE)
static INT32 af_proc_get_cmd_search_dir(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF search direction: %s\n", (*(af_param[id]->search_dir) == 1) ? "Tele2Wide" : "Wide2Tele");

	return 0;
}

static INT32 af_proc_set_cmd_search_dir(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->search_dir) = AFALG_MIN(simple_strtoul(argv[3], NULL, 0), AF_DIR_2WIDE);

	return 0;
}
#endif

static INT32 af_proc_get_cmd_skip_frame(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF skip frame: %u\n", *(af_param[id]->skip_frame));

	return 0;
}

static INT32 af_proc_set_cmd_skip_frame(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->skip_frame) = AFALG_MIN(simple_strtoul(argv[3], NULL, 0), 8);

	return 0;
}

static INT32 af_proc_get_cmd_thres_criterion(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;
	AF_PARAM *paf_param;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	paf_param = af_param[id];

	af_cmd_printf("AF thres criterion: %d, %d, %d\n", paf_param->thres->thres_rough, paf_param->thres->thres_fine, paf_param->thres->thres_final);

	return 0;
}

static INT32 af_proc_set_cmd_thres_criterion(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;
	AF_PARAM *paf_param;

	if (argc < 6) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	paf_param = af_param[id];
	paf_param->thres->thres_rough = AFALG_CLAMP(simple_strtol(argv[3], NULL, 0), 10, 100);
	paf_param->thres->thres_fine  = AFALG_CLAMP(simple_strtol(argv[4], NULL, 0), 60, 100);
	paf_param->thres->thres_final = AFALG_CLAMP(simple_strtol(argv[5], NULL, 0), 80, 100);

	return 0;
}

static INT32 af_proc_get_cmd_step_size(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;
	AF_PARAM *paf_param;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	paf_param = af_param[id];

	af_cmd_printf("AF step size: %d, %d, %d\n", paf_param->step_size->step_rough, paf_param->step_size->step_fine, paf_param->step_size->step_final);

	return 0;
}

static INT32 af_proc_set_cmd_step_size(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;
	AF_PARAM *paf_param;

	if (argc < 6) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	paf_param = af_param[id];
	paf_param->step_size->step_rough = AFALG_CLAMP(simple_strtol(argv[3], NULL, 0), 3, 9);
	paf_param->step_size->step_fine  = AFALG_CLAMP(simple_strtol(argv[4], NULL, 0), 2, 6);
	paf_param->step_size->step_final = AFALG_CLAMP(simple_strtol(argv[5], NULL, 0), 1, 3);

	return 0;
}

static INT32 af_proc_get_cmd_max_search_cnt(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF max search count: %u\n", *(af_param[id]->max_count));

	return 0;
}

static INT32 af_proc_set_cmd_max_search_cnt(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->max_count) = AFALG_CLAMP(simple_strtoul(argv[3], NULL, 0), 3*30, 20*30);

	return 0;
}

#if (!AF_PROC_REDUCE)
static INT32 af_proc_get_cmd_win_weight(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF 8x8 window weight: %u, %u, %u, %u, %u, %u, %u, %u\n", af_param[id]->win_weight->wei[0][0], af_param[id]->win_weight->wei[0][1], af_param[id]->win_weight->wei[0][2], af_param[id]->win_weight->wei[0][3],
										af_param[id]->win_weight->wei[0][4], af_param[id]->win_weight->wei[0][5], af_param[id]->win_weight->wei[0][6], af_param[id]->win_weight->wei[0][7]);

	return 0;
}

static INT32 af_proc_set_cmd_win_weight(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT8 wei = 1;
	UINT32 i, j, id = AF_ID_1;
	AF_PARAM *paf_param;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	wei = (UINT8)AFALG_MIN(simple_strtoul(argv[3], NULL, 0), 15);

	paf_param = af_param[id];

	for (j = 0; j < AF_WIN_ALG_Y; j++) {
		for (i = 0; i < AF_WIN_ALG_X; i++) {
			paf_param->win_weight->wei[j][i] = wei;
		}
	}

	return 0;
}

static INT32 af_proc_get_cmd_va_data(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	// TODO:
	#if 0
	UINT32 id = AF_ID_1;
	AF_INFO *paf_info = (AF_INFO *)pdrv->private;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	paf_info[id].dump_va = TRUE;
	#endif
	return 0;
}
#endif

static INT32 af_proc_set_cmd_re_trigger(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	// TODO:
	UINT32 id = AF_ID_1;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	*(af_param[id]->retrigger) = (simple_strtoul(argv[3], NULL, 0) != 0) ? TRUE : FALSE;
	//paf_info[id].ae_stable_cnt = 0;

	af_cmd_printf("AF retrigger...\n");
	return 0;
}

static INT32 af_proc_get_cmd_dbg_msg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	// TODO:
	#if 0
	UINT32 id = AF_ID_1;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF debug message: 0x%08X (BIT0: general info / BIT1: VA / BIT2: performance measure / BIT16: proc cmd / BIT17: ioctl cmd)\n", *(af_param[id]->dbg_msg));
	#endif
	return 0;
}

static INT32 af_proc_set_cmd_dbg_msg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	UINT32 id = AF_ID_1;
	UINT32 dbg_cmd = 0;

	if (argc < 4) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
	dbg_cmd = simple_strtoul(argv[3], NULL, 0);
	*(af_param[id]->dbg_msg) = dbg_cmd;

	printk("AF dbg_msg === %d\n", dbg_cmd);

	return 0;
}

#if (!AF_PROC_REDUCE)
static INT32 af_proc_get_cmd_rld_cfg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	// TODO:
	#if 0
	UINT32 id = AF_ID_1;
	AF_INFO *paf_info = (AF_INFO *)pdrv->private;

	if (argc < 3) {
		return -EINVAL;
	}

	id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	af_cmd_printf("AF config path: %s\n", paf_info[id].cfg_path);
	#endif
	return 0;
}

static INT32 af_proc_set_cmd_rld_cfg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	// TODO:
	#if 0
	AFT_CFG_INFO cfg_info;
	AF_INFO *paf_info = (AF_INFO *)pdrv->private;
	CFG_FILE_FMT *pcfg_file;

	if (argc < 4) {
		return -EINVAL;
	}

	cfg_info.id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));

	if ((strlen(argv[3]) > 0) && (strlen(argv[3]) <= AF_CFG_NAME_LENGTH)) {
		strncpy(cfg_info.path, argv[3], AF_CFG_NAME_LENGTH-1);
		cfg_info.path[AF_CFG_NAME_LENGTH-1] = '\0';
	} else {
		return -EINVAL;
	}

	cfg_info.path[AF_CFG_NAME_LENGTH-1] = '\0';
	pcfg_file = aft_cfg_open(paf_info[cfg_info.id].cfg_path);
	if (pcfg_file != NULL) {
		DBG_MSG("load cfg success\r\n");

		aft_cfg_load(pcfg_file, af_param[cfg_info.id]);

		aft_cfg_close(pcfg_file);

		paf_info[cfg_info.id].cfg_path[AF_CFG_NAME_LENGTH-1] = '\0';
		return 0;
	} else {
		DBG_ERR("load cfg fail!\r\n");
		return -1;
	}

	#if 0
	if (aft_load_cfg(cfg_info.path, af_get_param(cfg_info.id)) == 0) {
		strncpy(paf_info[cfg_info.id].cfg_path, cfg_info.path, AF_CFG_NAME_LENGTH-1);
		paf_info[cfg_info.id].cfg_path[AF_CFG_NAME_LENGTH-1] = '\0';

		return 0;
	} else {
		return -1;
	}
	#endif
	#endif
	return 0;
}
#endif

static AF_PROC_CMD af_proc_r_cmd_list[] = {
	// keyword                 function name
	{ "enable",          af_proc_get_cmd_enable,           "Get AF enable/disable, param: [id]"   },
	//#if (!AF_PROC_REDUCE)
	{ "alg_method",      af_proc_get_cmd_alg_method,       "Get AF alg method, param: [id]"       },
	//#endif
	{ "shot_mode",       af_proc_get_cmd_shot_mode,        "Get AF shot mode, param: [id]"        },
	#if (!AF_PROC_REDUCE)
	{ "search_dir",      af_proc_get_cmd_search_dir,       "Get AF search direction, param: [id]" },
	#endif
	{ "skip_frame",      af_proc_get_cmd_skip_frame,       "Get AF skip frame, param: [id]"       },
	{ "thres",           af_proc_get_cmd_thres_criterion,  "Get AF thres, param: [id]"            },
	{ "step_size",       af_proc_get_cmd_step_size,        "Get AF step size, param: [id]"        },
	{ "max_search_cnt",  af_proc_get_cmd_max_search_cnt,   "Get AF max count, param: [id]"        },
	#if (!AF_PROC_REDUCE)
	{ "win_weight",      af_proc_get_cmd_win_weight,       "Get AF window weight, param: [id]"    },
	{ "va_data",         af_proc_get_cmd_va_data,          "Get VA statistics, param: [id]"       },
	#endif
	{ "dbg_msg",         af_proc_get_cmd_dbg_msg,          "Get AF debug message, param: [id]"    },
	#if (!AF_PROC_REDUCE)
	{ "rld_cfg",         af_proc_get_cmd_rld_cfg,          "Get AF config file, param: [id]"      },
	#endif
};
#define NUM_OF_R_CMD (sizeof(af_proc_r_cmd_list) / sizeof(AF_PROC_CMD))

static AF_PROC_CMD af_proc_w_cmd_list[] = {
	// keyword                 function name
	{ "enable",          af_proc_set_cmd_enable,           "Set AF enable/disable, param: [id] [enable]"              },
	//#if (!AF_PROC_REDUCE)
	{ "alg_method",      af_proc_set_cmd_alg_method,       "Set AF alg method, param: [id] [value]"                   },
	//#endif
	{ "shot_mode",       af_proc_set_cmd_shot_mode,        "Set AF shot mode, param: [id] [value]"                    },
	#if (!AF_PROC_REDUCE)
	{ "search_dir",      af_proc_set_cmd_search_dir,       "Set AF search direction, param: [id] [value]"             },
	#endif
	{ "skip_frame",      af_proc_set_cmd_skip_frame,       "Set AF skip frame, param: [id] [value]"                   },
	{ "thres",           af_proc_set_cmd_thres_criterion,  "Set AF thres, param: [id] [value1] [value2] [value3]"     },
	{ "step_size",       af_proc_set_cmd_step_size,        "Set AF step size, param: [id] [value1] [value2] [value3]" },
	{ "max_search_cnt",  af_proc_set_cmd_max_search_cnt,   "Set AF max count, param: [id] [value]"                    },
	#if (!AF_PROC_REDUCE)
	{ "win_weight",      af_proc_set_cmd_win_weight,       "Set AF window weight, param: [id] [value]"                },
	#endif
	{ "re_trigger",      af_proc_set_cmd_re_trigger,       "Set AF retrigger, param: [id] [trigger]"                  },
	{ "dbg_msg",         af_proc_set_cmd_dbg_msg,          "Set AF debug message, param: [id] [value]"                },
	#if (!AF_PROC_REDUCE)
	{ "rld_cfg",         af_proc_set_cmd_rld_cfg,          "Set AF config file, param: [id] [path]"                   },
	#endif
};
#define NUM_OF_W_CMD (sizeof(af_proc_w_cmd_list) / sizeof(AF_PROC_CMD))

static INT32 af_proc_command_show(struct seq_file *sfile, void *v)
{
	#if 0
	UINT32 i, j, va_raw = 0, va_wei = 0;
	ISP_MODULE *af_module = (ISP_MODULE *)sfile->private;
	AF_INFO *paf_info = (AF_INFO *)af_module->private;

	if (g_cmd_msg.buf == NULL) {
		return -EINVAL;
	}

	if (g_cmd_msg.buf > 0) {
		seq_printf(sfile, "%s\n", g_cmd_msg.buf);
		af_clean_msg_buf();
	}

	if (paf_info->dump_va == TRUE) {
		paf_info->dump_va = FALSE;

		seq_printf(sfile, "         0      1      2      3      4      5      6      7\n");
		for (i = 0; i < AF_WIN_HW_Y; i++) {
			seq_printf(sfile, "%2d  %6u %6u %6u %6u %6u %6u %6u %6u\n", i,
			paf_info->va_sta.acc[i][0],  paf_info->va_sta.acc[i][1],  paf_info->va_sta.acc[i][2],  paf_info->va_sta.acc[i][3],
			paf_info->va_sta.acc[i][4],  paf_info->va_sta.acc[i][5],  paf_info->va_sta.acc[i][6],  paf_info->va_sta.acc[i][7]);

			for (j = 0; j < AF_WIN_HW_X; j++) {
				va_raw += paf_info->va_sta.acc[i][j];
				va_wei += ((UINT32)paf_info->win_weight.wei[i][j] * paf_info->va_sta.acc[i][j]);
			}
		}

		seq_printf(sfile, "VA = %u, %u (with weighting)\n", va_raw, va_wei);
	}
	#endif
	return 0;
}

static INT32 af_proc_command_open(struct inode *inode, struct file *file)
{
	return single_open(file, af_proc_command_show, PDE_DATA(inode));
}

static ssize_t af_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	CHAR *argv[MAX_CMD_ARGUMENTS] = { NULL };
	const CHAR delimiters[] = { ' ', 0x0A, 0x0D, '\0' };
	INT32 i, argc = 0, len = size, ret = -EINVAL;
	UINT32 id = AF_ID_1;
	ISP_MODULE *af_module = af_proc_get_mudule_from_file(file);
	// TODO:
	//AF_INFO *paf_info = (AF_INFO *)af_module->private;

	// check command length
	if ((len <= 1) || (len > MAX_CMDLINE_LENGTH)) {
		DBG_ERR("command is too short or long!\r\n");
	} else {
		// copy command string from user space
		if (copy_from_user(cmd_line, buf, len)) {
			;
		} else {
			cmd_line[len-1] = '\0';

			// parse command string
			for (i = 0; i < MAX_CMD_ARGUMENTS; i++) {
				if ((argv[i] = strsep(&cmdstr, delimiters)) != NULL) {
					argc++;
				} else {
					break;
				}
			}

			// dispatch command handler
			ret = -EINVAL;

			if (strncmp(argv[0], "r", 2) == 0) {
				for (i = 0; i < NUM_OF_R_CMD; i++) {
					if (strncmp(argv[1], af_proc_r_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = af_proc_r_cmd_list[i].execute(af_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif

						if (ret == 0) {
							id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
							// TODO:
							#if 0
							if (paf_info[id].dbg_msg & AF_DBG_DUMP_PROC) {
								DBG_MSG("proc rcmd #%u\r\n", i);
							}
							#endif
						}
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					printk("[AF_ERR]: => ");
					for (i = 0; i < argc; i++) {
						printk("%s ", argv[i]);
					}
					printk("is not in r_cmd_list!\r\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], af_proc_w_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						#if MUTEX_ENABLE
						down(&mutex);
						#endif
						ret = af_proc_w_cmd_list[i].execute(af_module, argc, argv);
						#if MUTEX_ENABLE
						up(&mutex);
						#endif

						if (ret == 0) {
							id = AFALG_MIN(simple_strtoul(argv[2], NULL, 0), (AF_ID_MAX_NUM - 1));
							// TODO:
							#if 0
							if (paf_info[id].dbg_msg & AF_DBG_DUMP_PROC) {
								DBG_MSG("proc wcmd #%u\r\n", i);
							}
							#endif
						}
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					printk("[AF_ERR]: => ");
					for (i = 0; i < argc; i++) {
						printk("%s ", argv[i]);
					}
					printk("is not in w_cmd_list!\r\n");
				}
			} else {
				printk("[AF_ERR]: =>");
				for (i = 0; i < argc; i++) {
					printk("%s ", argv[i]);
				}
				printk("is not legal command!\r\n");
			}
		}
	}

	if (ret < 0) {
		printk("[AF_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			printk("%s ", argv[i]);
		}
		printk("\r\n");
	}

	return size;
}

static const struct file_operations af_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = af_proc_command_open,
	.read    = seq_read,
	.write   = af_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 af_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/af/info' will show all the AF info\r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/af/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, " AF proc r/w function list\r\n");
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD; loop++) {
		seq_printf(sfile, "r %-14s : %s\r\n", af_proc_r_cmd_list[loop].cmd, af_proc_r_cmd_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD; loop++) {
		seq_printf(sfile, "w %-14s : %s\r\n", af_proc_w_cmd_list[loop].cmd, af_proc_w_cmd_list[loop].text);
	}

	return 0;
}

static INT32 af_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, af_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations af_proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = af_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};


//=============================================================================
// extern functions
//=============================================================================
INT32 af_proc_create(void)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	#if MUTEX_ENABLE
	sema_init(&mutex, 1);
	#endif

	root = proc_mkdir("hdal/vendor/af", NULL);
	if (root == NULL) {
		DBG_ERR("fail to create AF proc root!\r\n");
		af_proc_remove();
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &af_proc_info_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!\r\n");
		af_proc_remove();
		return -EINVAL;
	}
	proc_info = pentry;

	// create "cmd" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &af_proc_command_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc command!\r\n");
		af_proc_remove();
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &af_proc_help_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!\r\n");
		af_proc_remove();
		return -EINVAL;
	}
	proc_help = pentry;

	// allocate memory for massage buffer
	if ((ret = af_alloc_msg_buf()) < 0) {
		af_proc_remove();
	}

	return ret;
}

void af_proc_remove(void)
{
	if (proc_root == NULL) {
		return;
	}

	// remove "info"
	if (proc_info) {
		proc_remove(proc_info);
	}

	// remove "command"
	if (proc_command) {
		proc_remove(proc_command);
	}

	// remove "help"
	if (proc_help) {
		proc_remove(proc_help);
	}

	// remove root entry
	proc_remove(proc_root);

	// free message buffer
	af_free_msg_buf();
}

