#include "ae_alg.h"
#include "aet_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "ae_dbg.h"
#include "ae_main.h"
#include "ae_proc.h"
#include "ae_version.h"

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
// NOTE: avoid GPL api
//static struct semaphore mutex;
static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_info;
static struct proc_dir_entry *proc_command;
static struct proc_dir_entry *proc_help;
static struct proc_dir_entry *proc_status;
static UINT32 la_f = AE_ID_MAX_NUM;
static UINT32 param_f = AE_ID_MAX_NUM;

//=============================================================================
// routines
//=============================================================================
static inline ISP_MODULE *ae_proc_get_mudule_from_file(struct file *file)
{
	return (ISP_MODULE *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 ae_proc_info_show(struct seq_file *sfile, void *v)
{
	AE_ID id;
	AE_PROC_CMD_OBJ *ae_proc_cmd[AE_ID_MAX_NUM];
	UINT32 version = ae_get_version();

	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
		ae_proc_cmd[id] = ae_flow_get_proc_cmd(id);
	}

	seq_printf(sfile, "----------------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "NVT_AE v%d.%d.%d.%d\n", (version>>24) & 0xFF, (version>>16) & 0xFF, (version>>8) & 0xFF, version & 0xFF);
	seq_printf(sfile, "----------------------------------------------------------------------------------------------\r\n");

	seq_printf(sfile, "          id :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", id);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "----------------------------------------------------------------------------------------------\r\n");

	seq_printf(sfile, "    proc_cmd :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->cmd);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "     dbg_cmd :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->dbg_cmd);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "     dbg_cyc :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->dbg_cyc);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "     ae mode :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->mode);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    target_y :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->target_y);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  converge_l :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->converge_th.l);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  converge_h :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->converge_th.h);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "       speed :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->speed);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "expt_bound_l :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->expt_bound.l);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "expt_bound_h :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->expt_bound.h);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "gain_bound_l :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->gain_bound.l);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "gain_bound_h :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->gain_bound.h);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    mae_expt :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->mae_expt);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    mae_gain :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->mae_gain);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  power_freq :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->power_freq);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "      alc_en :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->alc_en);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  overexp_en :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->overexp_en);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, " overexp_thr :");
	for (id = AE_ID_1; id < AE_ID_MAX_NUM; id++) {
	seq_printf(sfile, " %8d", ae_proc_cmd[id]->overexp_thr);
	}
	seq_printf(sfile, "\n");

	return 0;
}

static INT32 ae_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, ae_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations ae_proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = ae_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "command" file operation functions
//=============================================================================
static AE_MSG_BUF g_cmd_msg;

static inline INT32 ae_alloc_msg_buf(void)
{
	g_cmd_msg.buf = isp_uti_mem_alloc(PROC_MSG_BUFSIZE);

	if (g_cmd_msg.buf == NULL) {
		DBG_ERR("fail to allocate AE message buffer!\n");
		return E_SYS;
	}

	g_cmd_msg.size = PROC_MSG_BUFSIZE;
	g_cmd_msg.count = 0;

	return 0;
}

static inline void ae_free_msg_buf(void)
{
	if (g_cmd_msg.buf) {
		isp_uti_mem_free(g_cmd_msg.buf);
		g_cmd_msg.buf = NULL;
	}
}

static inline void ae_clean_msg_buf(void)
{
	g_cmd_msg.buf[0] = '\0';
	g_cmd_msg.count = 0;
}

static INT32 ae_cmd_printf(const s8 *f, ...)
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

static s32 ae_proc_cmd_dbg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 cmd = 0, cyc = 0;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc < 3) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	if (argc == 4) {
		cmd = simple_strtoul(argv[3], NULL, 0);
		cyc = 30;
	} else if (argc == 5) {
		cmd = simple_strtoul(argv[3], NULL, 0);
		cyc = simple_strtoul(argv[4], NULL, 0);
	} else {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	ae_cmd_printf("id = %d, cmd = 0x%x, cyc = %d\n", id, cmd, cyc);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	ae_proc_cmd->dbg_cmd = cmd;
	ae_proc_cmd->dbg_cyc = cyc;

	ae_dbg_set_dbg_mode(id, cmd);

	return 0;
}

static s32 ae_proc_cmd_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	AE_MODE mode;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	mode = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->mode = mode;

	ae_cmd_printf("id = %d, ae_mode = %d\n", id, mode);

	ae_proc_cmd->cmd = AE_PROC_CMD_MODE;

	return 0;
}

static s32 ae_proc_cmd_target_y(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 target_y;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	target_y = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->target_y = target_y;

	ae_cmd_printf("id = %d, target_y = %d\n", id, ae_proc_cmd->target_y);

	ae_proc_cmd->cmd = AE_PROC_CMD_TARGET_Y;

	return 0;
}

static s32 ae_proc_cmd_isocoef(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 isocoef;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	isocoef = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->isocoef = isocoef;

	ae_cmd_printf("id = %d, isocoef = %d\n", id, ae_proc_cmd->isocoef);

	ae_proc_cmd->cmd = AE_PROC_CMD_ISOCOEF;

	return 0;
}

static s32 ae_proc_cmd_converge_th(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 thr;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if ((argc != 4) && (argc != 5)) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	if (argc == 4) {
		thr = simple_strtoul(argv[3], NULL, 0);
		ae_proc_cmd->converge_th.l = thr;
		ae_proc_cmd->converge_th.h = thr;
	} else if (argc == 5) {
		thr = simple_strtoul(argv[3], NULL, 0);
		ae_proc_cmd->converge_th.l = thr;
		thr = simple_strtoul(argv[4], NULL, 0);
		ae_proc_cmd->converge_th.h = thr;
	}

	ae_cmd_printf("id = %d, converge_th = %d, %d\n", id, ae_proc_cmd->converge_th.l, ae_proc_cmd->converge_th.h);

	ae_proc_cmd->cmd = AE_PROC_CMD_CONVERGE_TH;

	return 0;
}

static s32 ae_proc_cmd_speed(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 speed;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	speed = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->speed = speed;

	ae_cmd_printf("id = %d, speed = %d\n", id, ae_proc_cmd->speed);

	ae_proc_cmd->cmd = AE_PROC_CMD_SPEED;

	return 0;
}

static s32 ae_proc_cmd_expt_bound(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 expt_bound_l, expt_bound_h;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	expt_bound_l = simple_strtoul(argv[3], NULL, 0);
	expt_bound_h = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->expt_bound.l = expt_bound_l;
	ae_proc_cmd->expt_bound.h = expt_bound_h;

	ae_cmd_printf("id = %d, exp_bound = %d, %d\n", id, ae_proc_cmd->expt_bound.l, ae_proc_cmd->expt_bound.h);

	ae_proc_cmd->cmd = AE_PROC_CMD_EXPT_BOUND;

	return 0;
}

static s32 ae_proc_cmd_gain_bound(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 gain_bound_l, gain_bound_h;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	gain_bound_l = simple_strtoul(argv[3], NULL, 0);
	gain_bound_h = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->gain_bound.l = gain_bound_l;
	ae_proc_cmd->gain_bound.h = gain_bound_h;

	ae_cmd_printf("id = %d, gain_bound = %d, %d\n", id, ae_proc_cmd->gain_bound.l, ae_proc_cmd->gain_bound.h);

	ae_proc_cmd->cmd = AE_PROC_CMD_GAIN_BOUND;

	return 0;
}

static s32 ae_proc_cmd_la_data(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;

	if (argc != 3) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	la_f = id;

	return 0;
}

static s32 ae_proc_cmd_mae(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 mae_expt, mae_gain, mae_aperture;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 6) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	mae_expt = simple_strtoul(argv[3], NULL, 0);
	mae_gain = simple_strtoul(argv[4], NULL, 0);

	if(argc > 5) {
		mae_aperture = simple_strtoul(argv[5], NULL, 0);
	} else {
		mae_aperture = 0;
	}
	
	ae_proc_cmd->mae_expt = mae_expt;
	ae_proc_cmd->mae_gain = mae_gain;
	ae_proc_cmd->mae_aperture = mae_aperture;

	ae_cmd_printf("[MAE] id = %d, expt = %d, iso = %d, aperture = %d\n", id, ae_proc_cmd->mae_expt, ae_proc_cmd->mae_gain, ae_proc_cmd->mae_aperture);

	ae_proc_cmd->cmd = AE_PROC_CMD_MAE;

	return 0;
}

static s32 ae_proc_cmd_mtg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 mtg, unit;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	mtg = simple_strtoul(argv[3], NULL, 0);

	if(argc > 4) {
		unit = simple_strtoul(argv[4], NULL, 0);
	} else {
		unit = 1;
	}

	if(unit < 1) {
		unit = 1;
	}

	ae_proc_cmd->mae_totalgain = (u64)((u64)mtg * (u64)unit);

	//ae_cmd_printf("id = %d, totalgain = %lld, unit = %d\n", id, ae_proc_cmd->mae_totalgain, unit);

	ae_proc_cmd->cmd = AE_PROC_CMD_MTG;

	return 0;
}

static s32 ae_proc_cmd_power_freq(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 freq;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (strncmp(argv[2], "?", 1) == 0) {
		ae_cmd_printf("%2d. FREQ_50HZ\n", AE_FLICKER_50HZ);
		ae_cmd_printf("%2d. FREQ_60HZ\n", AE_FLICKER_60HZ);
		ae_cmd_printf("%2d. FREQ_55HZ\n", AE_FLICKER_55HZ);
		return 0;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	freq = simple_strtoul(argv[3], NULL, 0);

	ae_proc_cmd->power_freq = freq;

	ae_cmd_printf("id = %d, freq = %d\n", id, ae_proc_cmd->power_freq);

	ae_proc_cmd->cmd = AE_PROC_CMD_POWER_FREQ;

	return 0;
}

static s32 ae_proc_cmd_flicker_mode(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 flicker_mode;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (strncmp(argv[2], "?", 1) == 0) {
		ae_cmd_printf("%2d. AE_FLICKER_NORMAL\n", AE_FLICKER_NORMAL);
		ae_cmd_printf("%2d. AE_FLICKER_EXPT_FREE\n", AE_FLICKER_EXPT_FREE);
		ae_cmd_printf("%2d. AE_FLICKER_INDOOR\n", AE_FLICKER_INDOOR);
		return 0;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	flicker_mode = simple_strtoul(argv[3], NULL, 0);

	ae_proc_cmd->flicker_mode = flicker_mode;

	ae_cmd_printf("id = %d, flicker_mode = %d\n", id, ae_proc_cmd->flicker_mode);

	ae_proc_cmd->cmd = AE_PROC_CMD_FLICKER_MODE;

	return 0;
}

static s32 ae_proc_cmd_alc(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 enable;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	enable = simple_strtoul(argv[3], NULL, 0);

	ae_proc_cmd->alc_en = enable;

	ae_cmd_printf("id = %d, alc_en = %d\n", id, ae_proc_cmd->alc_en);

	ae_proc_cmd->cmd = AE_PROC_CMD_ALC;

	return 0;
}

static s32 ae_proc_cmd_overexp(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 enable, thr;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if ((argc != 4) && (argc != 5)) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	enable = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->overexp_en = enable;

	if (argc == 5) {
		thr = simple_strtoul(argv[4], NULL, 0);
		ae_proc_cmd->overexp_thr = thr;
	}

	ae_cmd_printf("id = %d, overexp_en = %d, overexp_thr = %d\n", id, ae_proc_cmd->overexp_en, ae_proc_cmd->overexp_thr);

	ae_proc_cmd->cmd = AE_PROC_CMD_OVEREXP;

	return 0;
}

static s32 ae_proc_cmd_dump_curve(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 type;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc != 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	type = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->curve_type = type;

	ae_cmd_printf("id = %d, curve_type = %d\n", id, ae_proc_cmd->curve_type);

	ae_proc_cmd->cmd = AE_PROC_CMD_DUMP_CURVE;

	return 0;
}

static s32 ae_proc_cmd_ui(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 cmd, value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (strncmp(argv[2], "?", 1) == 0) {
		ae_cmd_printf("%2d. AE_UI_FREQUENCY\n", AE_UI_FREQUENCY);
		ae_cmd_printf("%2d. AE_UI_METER\n", AE_UI_METER);
		ae_cmd_printf("%2d. AE_UI_EV\n", AE_UI_EV);
		ae_cmd_printf("%2d. AE_UI_ISO\n", AE_UI_ISO);
		ae_cmd_printf("%2d. AE_UI_CAP_LONGEXP\n", AE_UI_CAP_LONGEXP);
		ae_cmd_printf("%2d. AE_UI_CAP_LONGEXP_EXPT\n", AE_UI_CAP_LONGEXP_EXPT);
		ae_cmd_printf("%2d. AE_UI_CAP_LONGEXP_ISO\n", AE_UI_CAP_LONGEXP_ISO);
		return 0;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	cmd = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->ui_cmd = cmd;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->ui_value = value;

	ae_cmd_printf("id = %d, ui_cmd = %d, ui_value = %d\n", id, ae_proc_cmd->ui_cmd, ae_proc_cmd->ui_value);

	ae_proc_cmd->cmd = AE_PROC_CMD_UI;

	return 0;
}

static s32 ae_proc_cmd_pri_expt(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->priority_expt = value;

	if(argc < 4) {
		value = simple_strtoul(argv[4], NULL, 0);
		ae_proc_cmd->priority_aperture = value;
	} else {
		ae_proc_cmd->priority_aperture = 0;
	}

	ae_proc_cmd->priority_mode = AEALG_PRIORITY_EXPT;

	ae_cmd_printf("id = %d, priority_expt = %d, priority_aperture = %d\n", id, ae_proc_cmd->priority_expt, ae_proc_cmd->priority_aperture);

	ae_proc_cmd->cmd = AE_PROC_CMD_PRI_EXPT;

	return 0;
}

static s32 ae_proc_cmd_pri_iso(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->priority_iso = value;

	if(argc < 4) {
		value = simple_strtoul(argv[4], NULL, 0);
		ae_proc_cmd->priority_aperture = value;
	} else {
		ae_proc_cmd->priority_aperture = 0;
	}

	ae_proc_cmd->priority_mode = AEALG_PRIORITY_ISO;

	ae_cmd_printf("id = %d, priority_iso = %d, priority_aperture = %d\n", id, ae_proc_cmd->priority_iso, ae_proc_cmd->priority_aperture);

	ae_proc_cmd->cmd = AE_PROC_CMD_PRI_ISO;

	return 0;
}

static s32 ae_proc_cmd_pri_aperture(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 5) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->priority_expt = value;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->priority_iso = value;

	ae_proc_cmd->priority_mode = AEALG_PRIORITY_APERTURE;

	ae_cmd_printf("id = %d, priority_expt = %d, priority_iso = %d\n", id, ae_proc_cmd->priority_expt, ae_proc_cmd->priority_iso);

	ae_proc_cmd->cmd = AE_PROC_CMD_PRI_APERTURE;

	return 0;
}

static s32 ae_proc_cmd_roi(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 8) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->roi_start_x = value;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->roi_start_y = value;

	value = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->roi_end_x = value;

	value = simple_strtoul(argv[6], NULL, 0);
	ae_proc_cmd->roi_end_y = value;

	if (argc == 7) {
		ae_proc_cmd->roi_option = ROI_ROUNDING;
	} else {
		value = simple_strtoul(argv[7], NULL, 0);
		ae_proc_cmd->roi_option = value;
	}

	ae_cmd_printf("id = %d, roi_area = {%d, %d} - {%d, %d}, round = %d\n", id, ae_proc_cmd->roi_start_x, ae_proc_cmd->roi_start_y, ae_proc_cmd->roi_end_x, ae_proc_cmd->roi_end_y, ae_proc_cmd->roi_option);

	ae_proc_cmd->cmd = AE_PROC_CMD_ROI;

	return 0;
}

static s32 ae_proc_cmd_freeze(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 6) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->freeze_en = value;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->freeze_cyc = value;

	value = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->freeze_thr = value;

	ae_cmd_printf("id = %d, enable = %d, cyc = %d, thr = %d\n", id, ae_proc_cmd->freeze_en, ae_proc_cmd->freeze_cyc, ae_proc_cmd->freeze_thr);

	ae_proc_cmd->cmd = AE_PROC_CMD_FREEZE;

	return 0;
}

static s32 ae_proc_cmd_slow_shoot(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 6) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->slowshoot_en = value;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->slowshoot_range = value;

	value = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->slowshoot_thr = value;

	ae_cmd_printf("id = %d, enable = %d, range = %d, thr = %d\n", id, ae_proc_cmd->slowshoot_en, ae_proc_cmd->slowshoot_range, ae_proc_cmd->slowshoot_thr);

	ae_proc_cmd->cmd = AE_PROC_CMD_SLOW_SHOOT;

	return 0;
}

static s32 ae_proc_cmd_pdctrl(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 7) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->p_factor[AEALG_SHOOT_DOWN]  = value;

	value = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->d_factor[AEALG_SHOOT_DOWN] = value;

	value = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->p_factor[AEALG_SHOOT_UP]  = value;

	value = simple_strtoul(argv[6], NULL, 0);
	ae_proc_cmd->d_factor[AEALG_SHOOT_UP] = value;

	ae_cmd_printf("id = %d, pdtrl_down = {%d, %d}, pdtrl_up = {%d, %d}\n", id, ae_proc_cmd->p_factor[AEALG_SHOOT_DOWN], ae_proc_cmd->d_factor[AEALG_SHOOT_DOWN], ae_proc_cmd->p_factor[AEALG_SHOOT_UP], ae_proc_cmd->d_factor[AEALG_SHOOT_UP]);

	ae_proc_cmd->cmd = AE_PROC_CMD_PDCTRL;

	return 0;
}

static s32 ae_proc_cmd_rate(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->rate = value;

	ae_cmd_printf("id = %d, rate = %d\n", id, ae_proc_cmd->rate);

	ae_proc_cmd->cmd = AE_PROC_CMD_RATE;

	return 0;
}

static s32 ae_proc_cmd_smart_ir_weight(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	AE_SMART_IR_WEIGHT weight;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 6) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	weight.center = simple_strtoul(argv[3], NULL, 0);
	weight.around = simple_strtoul(argv[4], NULL, 0);
	weight.other = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->smart_ir_weight = weight;

	ae_cmd_printf("id = %d, smart_ir_weight = %d, %d, %d \n", id, ae_proc_cmd->smart_ir_weight.center, ae_proc_cmd->smart_ir_weight.around, ae_proc_cmd->smart_ir_weight.other);

	ae_proc_cmd->cmd = AE_PROC_CMD_SMART_IR_WEIGHT;

	return 0;
}

static s32 ae_proc_cmd_perf_cmd(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->perf_cmd = value;

	ae_cmd_printf("id = %d, perf_cmd = %d\n", id, ae_proc_cmd->perf_cmd);

	ae_proc_cmd->cmd = AE_PROC_CMD_PERF_CMD;

	return 0;
}

static s32 ae_proc_cmd_test(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	u32 value;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 4) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);

	value = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->test = value;

	ae_cmd_printf("id = %d, test_value = %d\n", id, ae_proc_cmd->test);

	ae_proc_cmd->cmd = AE_PROC_CMD_TEST;

	return 0;
}

static s32 ae_proc_cmd_curve_tracking(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;
	AE_PROC_CMD_OBJ *ae_proc_cmd;

	if (argc > 8) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	ae_proc_cmd = ae_flow_get_proc_cmd(id);
	
	ae_proc_cmd->ct_period = simple_strtoul(argv[3], NULL, 0);
	ae_proc_cmd->ct_interval = simple_strtoul(argv[4], NULL, 0);
	ae_proc_cmd->ct_range.l = simple_strtoul(argv[5], NULL, 0);
	ae_proc_cmd->ct_range.h = simple_strtoul(argv[6], NULL, 0);
	ae_proc_cmd->ct_tg_unit = simple_strtoul(argv[7], NULL, 0);
	
	ae_cmd_printf("id          = %d\n", id);
	ae_cmd_printf("ct_period   = %d\n", ae_proc_cmd->ct_period);
	ae_cmd_printf("ct_interval = %d\n", ae_proc_cmd->ct_interval);
	ae_cmd_printf("ct_range    = %d, %d\n", ae_proc_cmd->ct_range.l, ae_proc_cmd->ct_range.h);
	ae_cmd_printf("ct_tg_unit  = %d\n", ae_proc_cmd->ct_tg_unit);

	ae_proc_cmd->cmd = AE_PROC_CMD_CURVE_TRACKING;

	return 0;
}

static INT32 ae_proc_cmd_reload_cfg(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AET_CFG_INFO cfg_info = {0};

	if (argc < 4) {
		return -EINVAL;
	}
	cfg_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(cfg_info.path, argv[3], AE_CFG_NAME_LENGTH-1);
	cfg_info.path[AE_CFG_NAME_LENGTH-1] = '\0';

	aet_api_set_cmd(AET_ITEM_RLD_CONFIG, (UINT32)&cfg_info);

	return 0;
}

static INT32 ae_proc_cmd_reload_dtsi(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AET_DTSI_INFO dtsi_info = {0};

	if (argc < 4) {
		return -EINVAL;
	}
	dtsi_info.id = simple_strtoul(argv[2], NULL, 0);
	strncpy(dtsi_info.node_path, argv[3], AE_DTSI_NAME_LENGTH-1);
	dtsi_info.node_path[AE_DTSI_NAME_LENGTH-1] = '\0';
	strncpy(dtsi_info.file_path, argv[4], AE_DTSI_NAME_LENGTH - 1);
	dtsi_info.file_path[AE_DTSI_NAME_LENGTH - 1] = '\0';

	aet_api_set_cmd(AET_ITEM_RLD_DTSI, (UINT32)&dtsi_info);

	return 0;
}

static INT32 ae_proc_cmd_get_param(ISP_MODULE *pdrv, INT32 argc, CHAR **argv)
{
	AE_ID id;

	if (argc != 3) {
		ae_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	param_f = id;

	return 0;
}

static AE_PROC_CMD ae_proc_r_cmd_list[] = {
	// keyword              function name
	{ "la_data",            ae_proc_cmd_la_data,       "get la statistic data, param: [id]"},
	{ "get_param",          ae_proc_cmd_get_param,     "get ae parameter, param: [id]"}
};
#define NUM_OF_R_CMD (sizeof(ae_proc_r_cmd_list) / sizeof(AE_PROC_CMD))

static AE_PROC_CMD ae_proc_w_cmd_list[] = {
	// keyword              function name
	{ "dbg",                ae_proc_cmd_dbg,           "show/select debug message, param: [id] [option] [cycle]"},
	{ "ae_mode",            ae_proc_cmd_mode,          "set ae mode, param: [id] [ae_mode]"},
	{ "target_y",           ae_proc_cmd_target_y,      "set ae target y, param: [id] [target_y]"},
	{ "isocoef",            ae_proc_cmd_isocoef,       "set ae isocoef, param: [id] [isocoef]"},
	{ "converge_th",        ae_proc_cmd_converge_th,   "set ae convergence threshold, param: [id] [percent] [percent_l|x]"},
	{ "speed",              ae_proc_cmd_speed,         "set ae convergence speed, param: [id] [speed]"},
	{ "exp_bound",          ae_proc_cmd_expt_bound,    "set ae exposure time boundary, unit: us, param: [id] [time_min] [time_max]"},
	{ "gain_bound",         ae_proc_cmd_gain_bound,    "set ae gain boundary, unit: ratio 100, param: [id] [gain_min] [gain_max]"},
	{ "mae",                ae_proc_cmd_mae,           "set manual exposure time and gain, param: [id] [expotime] [gain]"},
	{ "mtg",                ae_proc_cmd_mtg,           "set manual total gain: [id] [totalgain] [unit]"},
	{ "freq",               ae_proc_cmd_power_freq,    "set power frequency, param: [id] [freq]"},
	{ "flicker_mode",       ae_proc_cmd_flicker_mode,  "set flicker mode, param: [id] [flicker_mode]"},
	{ "alc",                ae_proc_cmd_alc,           "set auto low light enable, param: [id] [enable]"},
	{ "overexp",            ae_proc_cmd_overexp,       "set over-exposure function enable & threshold, param: [id] [enable] [threshold|x]"},
	{ "curve",              ae_proc_cmd_dump_curve,    "dump ae curve, param, param: [id] [option]"},
	{ "ui",                 ae_proc_cmd_ui,            "set ui command, param: [id] [option] [value]"},
	{ "pri_expt",           ae_proc_cmd_pri_expt,      "set priority expotime, param: [id] [expt] [aperture]"},
	{ "pri_iso",            ae_proc_cmd_pri_iso,       "set priority iso_gain, param: [id] [iso] [aperture]"},
	{ "pri_aperture",       ae_proc_cmd_pri_aperture,  "set priority aperture, param: [id] [expt] [iso]"},
	{ "roi",                ae_proc_cmd_roi,           "set roi area, param: [id] [start_x] [start_y] [end_x] [end_y] [round]"},
	{ "freeze",             ae_proc_cmd_freeze,        "set freeze, param: [id] [en] [cyc] [thr]"},
	{ "slowshoot",          ae_proc_cmd_slow_shoot,    "set slow shoot, param: [id] [en] [range] [thr]"},
	{ "pdctrl",             ae_proc_cmd_pdctrl,        "set pdctrl, param: [id] [p_factor] [d_factor]"},
	{ "rate",               ae_proc_cmd_rate,          "set rate, param: [id] [rate]"},
	{ "smartir_weight",     ae_proc_cmd_smart_ir_weight, "set smart ir level, param: [id] [smart_ir_level]"},
	{ "perf",               ae_proc_cmd_perf_cmd,      "set perf cmd, param: [id] [perf_cmd]"},
	{ "test",               ae_proc_cmd_test,          "set test value, param: [id] [test]"},
	{ "curve_tracking",     ae_proc_cmd_curve_tracking,"set curve tracking param, param: [id] [period] [tg_interval] [ct_range_l] [ct_range_h] [ct_tg_unit]"},
	{ "reload_cfg",         ae_proc_cmd_reload_cfg,    "reload cfg file, param: [id] [file_path]"},
	{ "reload_dtsi",        ae_proc_cmd_reload_dtsi,   "reload dtsi file, param: [id] [node_path] [file_path]"}
};
	
#define NUM_OF_W_CMD (sizeof(ae_proc_w_cmd_list) / sizeof(AE_PROC_CMD))

static INT32 ae_proc_command_show(struct seq_file *sfile, void *v)
{
#define LA_WIN_X 32
#define LA_WIN_Y 32

	u32 ix, iy, iyw;
	ISP_LA_RSLT *isp_la_rslt;
	UINT32 i;

	if (g_cmd_msg.buf == NULL && la_f >= AE_ID_MAX_NUM && param_f >= AE_ID_MAX_NUM) {
		return -EINVAL;
	}

	if (g_cmd_msg.buf > 0) {
		seq_printf(sfile, "%s\n", g_cmd_msg.buf);
		ae_clean_msg_buf();
	}

	if (la_f < AE_ID_MAX_NUM) {
		seq_printf(sfile, "id = %d\n", la_f);

		isp_la_rslt = isp_dev_get_la(la_f);

		if (isp_la_rslt == NULL) {
			seq_printf(sfile, "isp_dev_get_la(id = %d) NULL\r\n", la_f);
		} else {
			for (iy = 0; iy < LA_WIN_Y; iy++) {
				iyw = iy * LA_WIN_X;
				for (ix = 0; ix < LA_WIN_X; ix++) {
					seq_printf(sfile, "%3d,", ((isp_la_rslt->lum_1[iyw + ix] + 8) >> 4));
				}
				seq_printf(sfile, "\r\n");
			}
		}
		la_f = AE_ID_MAX_NUM;
	}

	if (param_f < AE_ID_MAX_NUM) {
		seq_printf(sfile, "base_iso\n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->base_iso);

		seq_printf(sfile, "base_gain_ratio\n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->base_gain_ratio);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "expect_lum\n");
		seq_printf(sfile, "%d\n", ae_param[param_f]->expect_lum->lum_mov);
		seq_printf(sfile, "%d\n", ae_param[param_f]->expect_lum->lum_photo);
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->expect_lum->tab_ratio_mov[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->expect_lum->tab_ratio_photo[i]);
		}
		seq_printf(sfile, "\n");

		seq_printf(sfile, "la_clamp \n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->la_clamp->tab_normal_h[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->la_clamp->tab_normal_l[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->la_clamp->tab_shdr_h[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->la_clamp->tab_shdr_l[i]);
		}
		seq_printf(sfile, "\n");
		seq_printf(sfile, "\n");

		seq_printf(sfile, "over_exposure \n");
		seq_printf(sfile, "%d\n", ae_param[param_f]->over_exposure->enable);
		seq_printf(sfile, "%d\n", ae_param[param_f]->over_exposure->lum);
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->over_exposure->tab_ratio[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->over_exposure->tab_thr_mov[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->over_exposure->tab_thr_photo[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->over_exposure->tab_maxcnt[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->over_exposure->tab_mincnt[i]);
		}
		seq_printf(sfile, "\n");
		seq_printf(sfile, "\n");

		seq_printf(sfile, "convergence \n");
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->speed);
		seq_printf(sfile, "%d %d\n", ae_param[param_f]->convergence->range_conv.h, ae_param[param_f]->convergence->range_conv.l);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->freeze_en);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->freeze_cyc);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->freeze_thr);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->slowshoot_en);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->slowshoot_range);
		seq_printf(sfile, "%d\n", ae_param[param_f]->convergence->slowshoot_thr);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "curve_gen_movie \n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->iso_calcoef);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->freq);
		for (i = 0; i < AEALG_CURVEGEN_NODE_MAX; i++) {
			seq_printf(sfile, "%d %d %d %d \n", ae_param[param_f]->curve_gen_movie->node[i].expt, ae_param[param_f]->curve_gen_movie->node[i].iso, ae_param[param_f]->curve_gen_movie->node[i].aperture, ae_param[param_f]->curve_gen_movie->node[i].reserved);
		}
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->node_num);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->iso_max);
		seq_printf(sfile, "%d %d\n", ae_param[param_f]->curve_gen_movie->hdr_ratio[0], ae_param[param_f]->curve_gen_movie->hdr_ratio[1]);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->auto_lowlight_en);
		for (i = 0; i < 5; i++) {
			seq_printf(sfile, "%d %d ", ae_param[param_f]->curve_gen_movie->ext_fps[i].sensor_fps, ae_param[param_f]->curve_gen_movie->ext_fps[i].extend_fps);
		}
		seq_printf(sfile, "\n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->iso_mode);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->isp_gain_thres);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_movie->flicker_mode);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "meter_win \n");
		for (i = 0; i < AE_WIN_Y; i++) {
			seq_printf(sfile, "%d %d %d %d %d %d %d %d \n", ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+1], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+2], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+3],
												ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+4], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+5], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+6], ae_param[param_f]->meter_win->matrix[i*AE_WIN_Y+7]);
		}
		seq_printf(sfile, "\n");

		seq_printf(sfile, "lum_gamma \n");
		for (i = 0; i < 65; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->lum_gamma->gamma[i]);
		}
		seq_printf(sfile, "\n");
		seq_printf(sfile, "\n");

		seq_printf(sfile, "shdr \n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->shdr->expy_le);
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->shdr->tab_ratio_le[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->shdr->tab_reserved0[i]);
		}
		seq_printf(sfile, "\n");
		for (i = 0; i < AEALG_DYNAMIC_LV_NUM; i++) {
			seq_printf(sfile, "%d ", ae_param[param_f]->shdr->tab_reserved1[i]);
		}
		seq_printf(sfile, "\n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->shdr->fixed_iso_en);
		seq_printf(sfile, "\n");
		seq_printf(sfile, "\n");

		seq_printf(sfile, "iris_cfg \n");
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->enable);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->exptime_min);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->probe_balance);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->balance_ratio);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->driving_ratio);
		seq_printf(sfile, "%d %d, %d\n", ae_param[param_f]->iris_cfg->iris_ctrl.kp, ae_param[param_f]->iris_cfg->iris_ctrl.ki, ae_param[param_f]->iris_cfg->iris_ctrl.kd);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->pwm_id);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->drv_max);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->freeze_time);
		seq_printf(sfile, "%d \n", ae_param[param_f]->iris_cfg->unfreeze_time);
		seq_printf(sfile, "\n");

		seq_printf(sfile, "curve_gen_photo \n");
		for (i = 0; i < AEALG_CURVEGEN_NODE_MAX; i++) {
			seq_printf(sfile, "%d %d %d %d \n", ae_param[param_f]->curve_gen_photo->node[i].expt, ae_param[param_f]->curve_gen_photo->node[i].iso, ae_param[param_f]->curve_gen_photo->node[i].aperture, ae_param[param_f]->curve_gen_photo->node[i].reserved);
		}
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_photo->node_num);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_photo->expt_max);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_photo->iso_max);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_photo->iso_mode);
		seq_printf(sfile, "%d \n", ae_param[param_f]->curve_gen_photo->isp_gain_thres);
		seq_printf(sfile, "\n");

		param_f = AE_ID_MAX_NUM;
	}

	return 0;
}

static INT32 ae_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, ae_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, ae_proc_command_show, PDE_DATA(inode), 2000*sizeof(u32));
}

static ssize_t ae_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;

	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	UINT32 i;
	INT32 argc = 0;
	ISP_MODULE *ae_module = ae_proc_get_mudule_from_file(file);

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
					if (strncmp(argv[1], ae_proc_r_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						// NOTE: avoid GPL api
						//down(&mutex);
						ret = ae_proc_r_cmd_list[i].execute(ae_module, argc, argv);
						// NOTE: avoid GPL api
						//up(&mutex);
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					DBG_ERR("[AE_ERR]: => ");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in r_cmd_list!\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], ae_proc_w_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						// NOTE: avoid GPL api
						//down(&mutex);
						ret = ae_proc_w_cmd_list[i].execute(ae_module, argc, argv);
						// NOTE: avoid GPL api
						//up(&mutex);
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[AE_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!\n");
				}
			} else {
				DBG_ERR("[AE_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[AE_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\n");
	}

	return size;
}

static const struct file_operations ae_proc_command_fops = {
	.owner   = THIS_MODULE,
	.open    = ae_proc_command_open,
	.read    = seq_read,
	.write   = ae_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 ae_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/ae/info' will show all the ae info\r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/ae/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "  %s\n", "ae proc r/w function list");
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", ae_proc_r_cmd_list[loop].cmd, ae_proc_r_cmd_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", ae_proc_w_cmd_list[loop].cmd, ae_proc_w_cmd_list[loop].text);
	}

	//seq_printf(sfile, " func_en = \r\n");
	//seq_printf(sfile, "   | 0x%4x = WDR  | 0x%4x = SHDR  | 0x%4x = DEFOG  |\r\n", ISP_FUNC_EN_WDR, ISP_FUNC_EN_SHDR, ISP_FUNC_EN_DEFOG);

	seq_printf(sfile, "Ex1: 'echo r get_param 0 > /proc/hdal/vendor/ae/cmd ;cat /proc/hdal/vendor/ae/cmd' \r\n");
	seq_printf(sfile, "Ex2: 'echo w freq 0 1 > /proc/hdal/vendor/ae/cmd ;cat /proc/hdal/vendor/ae/cmd' \r\n");

	seq_printf(sfile, "========================================================================================================================\n");
	seq_printf(sfile, "Parameter--------------------------------------------------------------------------------------------------------------\n");
	seq_printf(sfile, "| Command      | ELEMENT1   | ELEMENT2                      | ELEMENT3                                                     \n");
	seq_printf(sfile, "|--------------|------------|-------------------------------|--------------------------------------------------------------\n");
	seq_printf(sfile, "| dbgcmd       | ID         | option                        | freq                                                         \n");
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_CMD_NONE    | set to 30 when no input                                      \n", AE_DBG_NONE);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_ERR_MSG     |                                                              \n", AE_DBG_ERR_MSG);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_SYNC        |                                                              \n", AE_DBG_SYNC);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_CFG         |                                                              \n", AE_DBG_CFG);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_INFO        |                                                              \n", AE_DBG_INFO);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_OVEREXP     |                                                              \n", AE_DBG_OVEREXP);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_HDR         |                                                              \n", AE_DBG_HDR);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_SMARTIR     |                                                              \n", AE_DBG_SMARTIR);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_ROI         |                                                              \n", AE_DBG_ROI);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_STATUS      |                                                              \n", AE_DBG_STATUS);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_CAPTURE     |                                                              \n", AE_DBG_CAPTURE);
	seq_printf(sfile, "|              |            | 0x%8x: AE_DBG_PERFORMANCE |                                                              \n", AE_DBG_PERFORMANCE);
	seq_printf(sfile, "|--------------|------------|-------------------------------|--------------------------------------------------------------\n");
	seq_printf(sfile, "| curve        | ID         | option                        |                                                              \n");
	seq_printf(sfile, "|              |            | 0: Preview curve              |                                                              \n");
	seq_printf(sfile, "|              |            | 1: Capture curve              |                                                              \n");
	seq_printf(sfile, "|              |            | 2: Prv & Cap curve            |                                                              \n");
	seq_printf(sfile, "|--------------|------------|-------------------------------|--------------------------------------------------------------\n");
	seq_printf(sfile, "| ui           | ID         | option                        | value                                                        \n");
	seq_printf(sfile, "|              |            | 0: AE_UI_FREQUENCY            |  0: 50Hz                                                     \n");
	seq_printf(sfile, "|              |            |                               |  1: 60Hz                                                     \n");
	seq_printf(sfile, "|              |            | 1: AE_UI_METER                |  0: AE_METER_CENTERWEIGHTED                                  \n");
	seq_printf(sfile, "|              |            |                               |  1: AE_METER_SPOT                                            \n");
	seq_printf(sfile, "|              |            |                               |  2: AE_METER_MATRIX                                          \n");
	seq_printf(sfile, "|              |            |                               |  3: AE_METER_EVALUATIVE                                      \n");
	seq_printf(sfile, "|              |            |                               |  4: AE_METER_FACEDETECTION                                   \n");
	seq_printf(sfile, "|              |            |                               |  5: AE_METER_USERDEFINE                                      \n");
	seq_printf(sfile, "|              |            |                               |  6: AE_METER_SMARTIR                                         \n");
	seq_printf(sfile, "|              |            |                               |  7: AE_METER_ROI                                             \n");
	seq_printf(sfile, "|              |            | 2: AE_UI_EV                   |  0: AE_EV_N4                                                 \n");
	seq_printf(sfile, "|              |            |                               |  4: AE_EV_N3                                                 \n");
	seq_printf(sfile, "|              |            |                               |  8: AE_EV_N2                                                 \n");
	seq_printf(sfile, "|              |            |                               | 12: AE_EV_N1                                                 \n");
	seq_printf(sfile, "|              |            |                               | 16: AE_EV_0                                                  \n");
	seq_printf(sfile, "|              |            |                               | 20: AE_EV_P1                                                 \n");
	seq_printf(sfile, "|              |            |                               | 24: AE_EV_P2                                                 \n");
	seq_printf(sfile, "|              |            |                               | 28: AE_EV_P3                                                 \n");
	seq_printf(sfile, "|              |            |                               | 32: AE_EV_P4                                                 \n");
	seq_printf(sfile, "|              |            | 3: AE_UI_ISO                  |  0: Auto                                                     \n");
	seq_printf(sfile, "|              |            |                               |100: 100                                                      \n");
	seq_printf(sfile, "|              |            |                               |800: 800                                                      \n");
	seq_printf(sfile, "|              |            | 4: AE_UI_CAP_LONGEXP          |  0: Disable                                                  \n");
	seq_printf(sfile, "|              |            |                               |  1: Enable                                                   \n");
	seq_printf(sfile, "|              |            | 5: AE_UI_CAP_LONGEXP_EXPT     | LongExp Time                                                 \n");
	seq_printf(sfile, "|              |            | 6: AE_UI_CAP_LONGEXP_ISO      | LongExp Gain                                                 \n");
	seq_printf(sfile, "|              |            | 7: AE_UI_OPERATION            | X                                                            \n");
	seq_printf(sfile, "|--------------|------------|-------------------------------|--------------------------------------------------------------\n");

	return 0;
}

static INT32 ae_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, ae_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations ae_proc_help_fops = {
	.owner	 = THIS_MODULE,
	.open	 = ae_proc_help_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "status" file operation functions
//=============================================================================
static INT32 ae_proc_status_show(struct seq_file *sfile, void *v)
{
	AE_ID id;
	UINT32 i;
	UINT32 isp_gain[AE_ID_MAX_NUM] = {128, 128, 128, 128, 128};

	seq_printf(sfile, "-----------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "       id:           0            1\r\n");
	seq_printf(sfile, "-----------------------------------------------------------------------------\r\n");

	seq_printf(sfile, "         LV: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		id = AE_ID_1;
		if(ae_param[id]->status->lv_base == 0) {
			seq_printf(sfile, " 0.000000");
			seq_printf(sfile, "    ");
		} else {
			seq_printf(sfile, "%2d.%06d", ae_param[id]->status->lv/ae_param[id]->status->lv_base, ae_param[id]->status->lv%ae_param[id]->status->lv_base);
			seq_printf(sfile, "    ");
		}

	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			if(ae_param[id]->status->lv_base == 0) {
				seq_printf(sfile, " 0.000000");
				seq_printf(sfile, "    ");
			} else {
				seq_printf(sfile, "%2d.%06d", ae_param[id]->status->lv/ae_param[id]->status->lv_base, ae_param[id]->status->lv%ae_param[id]->status->lv_base);
				seq_printf(sfile, "    ");
			}
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "AE mode: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[AE_ID_1]->manual->mode);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->manual->mode);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "    ExpTime: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		for(i = 0; i < ae_param[AE_ID_1]->status->mf_num; i++) {
			seq_printf(sfile, " %8d", ae_param[AE_ID_1]->status->expotime[i]);
			seq_printf(sfile, "    ");
		}
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, " %8d", ae_param[id]->status->expotime[0]);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "    ISOGain: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		for(i = 0; i < ae_param[AE_ID_1]->status->mf_num; i++) {
			seq_printf(sfile, " %8d", ae_param[AE_ID_1]->status->iso_gain[i]);
			seq_printf(sfile, "    ");
		}
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, " %8d", ae_param[id]->status->iso_gain[0]);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "   Aperture: ");
	
	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		for(i = 0; i < ae_param[AE_ID_1]->status->mf_num; i++) {
			seq_printf(sfile, " %8d", ae_param[AE_ID_1]->status->aperture);
			seq_printf(sfile, "    ");
		}
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, " %8d", ae_param[id]->status->aperture);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "ISP_Gain_TH: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		for(i = 0; i < ae_param[AE_ID_1]->status->mf_num; i++) {
			seq_printf(sfile, " %8d", ae_param[AE_ID_1]->curve_gen_movie->isp_gain_thres);
			seq_printf(sfile, "    ");
		}
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, " %8d", ae_param[id]->curve_gen_movie->isp_gain_thres);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "   ISP_Gain: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		for(i = 0; i < ae_param[AE_ID_1]->status->mf_num; i++) {
			isp_dev_get_sync_item(i, ISP_SYNC_SEL_SIE, ISP_SYNC_AE_DGAIN, &isp_gain[i]);
			seq_printf(sfile, "      %3d", isp_gain[i]);
			seq_printf(sfile, "    ");
		}
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			isp_dev_get_sync_item(id, ISP_SYNC_SEL_SIE, ISP_SYNC_AE_DGAIN, &isp_gain);
			seq_printf(sfile, "      %3d", isp_gain[id]);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "    Lum.(Y): ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "      %3d", ae_param[AE_ID_1]->status->lum);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "      %3d", ae_param[id]->status->lum);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "  ExpectLum: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "      %3d", ae_param[AE_ID_1]->status->expect_lum);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "      %3d", ae_param[id]->status->expect_lum);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, " OverExp_En: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[AE_ID_1]->over_exposure->enable);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->over_exposure->enable);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, " OverExpAdj: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "      %3d", ae_param[AE_ID_1]->status->overexp_adj);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "      %3d", ae_param[id]->status->overexp_adj);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, " OverExpCnt: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "      %3d", ae_param[AE_ID_1]->status->overexp_cnt);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "      %3d", ae_param[id]->status->overexp_cnt);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "SlowShootEn: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[AE_ID_1]->convergence->slowshoot_en);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->convergence->slowshoot_en);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "   FreezeEn: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[AE_ID_1]->convergence->freeze_en);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->convergence->freeze_en);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "  ADJ_State: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[id]->status->state_adj);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->status->state_adj);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "  FrameRate: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "    %2d.%02d", (ae_param[AE_ID_1]->status->fps/100), (ae_param[AE_ID_1]->status->fps%100));
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "    %2d.%02d", (ae_param[id]->status->fps/100), (ae_param[id]->status->fps%100));
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n");

	seq_printf(sfile, "MultiFrmNum: ");

	if(ae_param[AE_ID_1]->status->mf_num >= 2) {
		seq_printf(sfile, "        %d", ae_param[AE_ID_1]->status->mf_num);
		seq_printf(sfile, "    ");
	} else {
		for(id = AE_ID_1; id <= AE_ID_2; id++) {
			seq_printf(sfile, "        %d", ae_param[id]->status->mf_num);
			seq_printf(sfile, "    ");
		}
	}

	seq_printf(sfile, "\r\n\r\n");

	return 0;
}

static INT32 ae_proc_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, ae_proc_status_show, PDE_DATA(inode));
}

static const struct file_operations ae_proc_status_fops = {
	.owner	 = THIS_MODULE,
	.open	 = ae_proc_status_open,
	.read	 = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// extern functions
//=============================================================================

INT32 ae_proc_create()
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL, *pentry = NULL;

	// initialize synchronization mechanism
	// NOTE: avoid GPL api
	//sema_init(&mutex, 1);

	root = proc_mkdir("hdal/vendor/ae", NULL);

	if (root == NULL) {
		DBG_ERR("fail to create AE proc root!\n");
		ae_proc_remove();
		return -EINVAL;
	}
	proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &ae_proc_info_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!\n");
		ae_proc_remove();
		return -EINVAL;
	}
	proc_info = pentry;

	// create "command" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &ae_proc_command_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc command!\n");
		ae_proc_remove();
		return -EINVAL;
	}
	proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &ae_proc_help_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!\n");
		ae_proc_remove();
		return -EINVAL;
	}
	proc_help = pentry;

	// create "status" entry
	pentry = proc_create_data("status", S_IRUGO | S_IXUGO, root, &ae_proc_status_fops, NULL);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc status!\n");
		ae_proc_remove();
		return -EINVAL;
	}
	proc_status = pentry;

	// allocate memory for massage buffer
	ret = ae_alloc_msg_buf();
	if (ret < 0) {
		ae_proc_remove();
	}

	return ret;
}

void ae_proc_remove()
{
	if (proc_root == NULL) {
		return;
	}

	// remove "info"
	if (proc_info) {
		proc_remove(proc_info);
	}
	// remove "cmd"
	if (proc_info) {
		proc_remove(proc_command);
	}
	// remove "help"
	if (proc_help) {
		proc_remove(proc_help);
	}
	// remove "status"
	if (proc_status) {
		proc_remove(proc_status);
	}

	// remove root entry
	proc_remove(proc_root);

	// free message buffer
	ae_free_msg_buf();
}

