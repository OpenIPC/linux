#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_proc.h"
#include "kdrv_vdocdc_api.h"
#include "kdrv_vdocdc_main.h"
#include "kdrv_vdocdc_comn.h"

#include "h26x_def.h"
#if H26X_MEM_USAGE
#include "h26x_common.h"
#include "kdrv_videoenc/kdrv_videoenc.h"
#endif
// proc entries
static struct proc_dir_entry *proc_root_entry = NULL;
//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
static struct proc_dir_entry *proc_cmd_entry = NULL;
static struct proc_dir_entry *proc_version_entry = NULL;
#endif
#if JND_DEFAULT_ENABLE
static struct proc_dir_entry *proc_jnd_entry = NULL;
#endif
#if LPM_PROC_ENABLE
static struct proc_dir_entry *proc_lpm_entry = NULL;
#endif
#if H26X_USE_DIFF_MAQ
static struct proc_dir_entry *proc_diff_maq_entry = NULL;
#endif
#if H26X_MEM_USAGE
static struct proc_dir_entry *proc_mem_usage_entry = NULL;
#endif

#if H26X_SET_PROC_PARAM
static struct proc_dir_entry *proc_param_entry = NULL;
#endif

//#if (H26X_SAVE_KO_SIZE == 0)
#if (H26X_PROC_CMD)
//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     8

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static PROC_CMD cmd_read_list[] = {
	// keyword          function name
};

#define NUM_OF_READ_CMD (sizeof(cmd_read_list) / sizeof(PROC_CMD))

static PROC_CMD cmd_write_list[] = {
	// keyword          function name
	{ "sim",		kdrv_vdocdc_api_wt_sim		},
	{ "dbg",		kdrv_vdocdc_api_wt_dbg		},
	{ "perf",		kdrv_vdocdc_api_wt_perf		},
	{ "int",		kdrv_vdocdc_api_wt_int		},
};

#define NUM_OF_WRITE_CMD (sizeof(cmd_write_list) / sizeof(PROC_CMD))

static int proc_cmd_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "=====================================================================\n");
    seq_printf(sfile, " kdrv_vdocdc_proc_cmd : \n");
    seq_printf(sfile, "=====================================================================\n");

	return 0;
}

static int proc_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_cmd_show, NULL);
}

static int proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
	const char delim[4] = {' ', 0x0A, 0x0D, '\0'};
	char cmd[MAX_CMD_LENGTH];
	char *p_cmd = cmd;
	unsigned char loop;
	unsigned char argc;
	char *argv[MAX_ARG_NUM] = {0};
	int ret = -EINVAL;

	if (copy_from_user(cmd, buf, size) != 0) {
		DBG_ERR("h26x proc comd copy error\r\n");
        return -1;
    }

	if (size != 0)
		cmd[size - 1] = '\0';

	nvt_dbg(INFO, "cmd_wt : %s\n", cmd);

	for (argc = 0; argc < 8; argc++) {
		argv[argc] = strsep(&p_cmd, delim);

		if (argv[argc] == NULL)
			break;
	}

	if (strncmp(argv[0], "r", 2) == 0) {
        for (loop = 0 ; loop < NUM_OF_READ_CMD; loop++) {
            if (strncmp(argv[1], cmd_read_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
                ret = cmd_read_list[loop].execute(argc - 2, &argv[2]);
                break;
            }
        }
        if (loop >= NUM_OF_READ_CMD)
			return -1;

    } else if (strncmp(argv[0], "w", 2) == 0)  {
        for (loop = 0 ; loop < NUM_OF_WRITE_CMD ; loop++) {
            if (strncmp(argv[1], cmd_write_list[loop].cmd, MAX_CMD_LENGTH) == 0) {
                ret = cmd_write_list[loop].execute(argc - 2, &argv[2]);
                break;
            }
        }

        if (loop >= NUM_OF_WRITE_CMD)
			return -1;

    } else
		return -1;

	return size;
}

static struct file_operations proc_cmd_fops = {
	.owner  = THIS_MODULE,
    .open   = proc_cmd_open,
    .release = single_release,
    .read   = seq_read,
    .write  = proc_cmd_write,
    .llseek = seq_lseek,
};
#endif //H26X_SAVE_KO_SIZE

#if H26X_SET_PROC_PARAM
//=============================================================================
// proc "rc" file operation functions
//=============================================================================
#include "h264enc_api.h"
#include "h265enc_api.h"
#include "h26xenc_api.h"

#define PROC_MAX_BUFFER 256
#define MAX_PARAM_NUM   20

typedef struct h26x_param_mapping_st
{
    char *tokenName;
    int (*get_param)(void);
    int (*set_param)(int);
    int lb;
    int ub;
    char *note;
} H26XParamMapInfo;

static int h26xEnc_setVersion(int val)
{
    return 0;
}
static const H26XParamMapInfo h26x_enc_syntax[] = {
	#if (H26X_SAVE_KO_SIZE == 0)
    {"H264RowRCStopFactor",     &h264Enc_getRowRCStopFactor,    &h264Enc_setRowRCStopFactor,    0,  0xFFFF, "H264 Row RC stop factor"},
    {"H265RowRCStopFactor",     &h265Enc_getRowRCStopFactor,    &h265Enc_setRowRCStopFactor,    0,  0xFFFF, "H265 Row RC stop factor"},
    {"H264PReduce16Planar",     &h264Enc_getPReduce16Planar,    &h264Enc_setPReduce16Planar,    0,  1,      "H264 P frame reduce intra16 planar"},
    //{"H264FrameNumGapAllow",    &gH264FrameNumGapAllow, STX_INT32,  1,      0,  1,      1,  "H264 frame_num gap allow"},
    //{"H264FixLog2Poc",          &gFixSPSLog2Poc,        STX_INT32,  0,      0,  16,     1,  "H264 fix log2 poc lsb"},

	{"RRCNDQPStep",				&h26xEnc_getNDQPStep,			&h26xEnc_setNDQPStep,			0,	15,		"Row rc negative qp step"},
	{"RRCNDQPRange",			&h26xEnc_getNDQPRange,			&h26xEnc_setNDQPRange,			0,	15,		"Row rc negative qp range"},
	#endif
	{"RCDumpLog",               &h26xEnc_getRCDumpLog,          &h26xEnc_setRCDumpLog,          0,  1,      "RC dump log"},
    {"Version",                 &h26xEnc_getVersion,            &h26xEnc_setVersion,            0,  1,      "version"},
    {NULL,                      NULL,                           NULL,                           0,  0,      NULL}
};

static int h26xenc_show_enc_param_syntax(struct seq_file *sfile)
{
    int idx = 0;
    seq_printf(sfile, "      parameter        value                   note\r\n");
    seq_printf(sfile, "=====================  =====  ======================================\r\n");
    for (idx = 0; idx < (int)(sizeof(h26x_enc_syntax)/sizeof(H26XParamMapInfo)); idx++) {
        if (NULL == h26x_enc_syntax[idx].tokenName)
            break;
        seq_printf(sfile, "%-22s  ", h26x_enc_syntax[idx].tokenName);

        seq_printf(sfile, " 0x%08x  ", h26x_enc_syntax[idx].get_param());
        seq_printf(sfile, "%s (range: %d ~ %d)\r\n", h26x_enc_syntax[idx].note, h26x_enc_syntax[idx].lb, h26x_enc_syntax[idx].ub);
    }
    return 0;
}

int h26xenc_set_enc_param_syntax(char *str)
{
    int value;
    char cmd_str[0x80];
    int i, idx = -1;

    sscanf(str, "%s %d\n", cmd_str, &value);

    for (i = 0; i < (int)(sizeof(h26x_enc_syntax)/sizeof(H26XParamMapInfo)); i++) {
        if (NULL == h26x_enc_syntax[i].tokenName)
            break;
        if (strcmp(h26x_enc_syntax[i].tokenName, cmd_str) == 0) {
            idx = i;
            break;
        }
    }
    if (idx >= 0) {
        if (value < h26x_enc_syntax[idx].lb || value > h26x_enc_syntax[idx].ub) {
            DBG_ERR("%s(%d) is out of range! (%d ~ %d)\n", h26x_enc_syntax[idx].tokenName, value, h26x_enc_syntax[idx].lb, h26x_enc_syntax[idx].ub);
        }
        else {
            h26x_enc_syntax[idx].set_param(value);
        }
    }
    else {
        DBG_ERR("unknown \"%s\"\n", cmd_str);
    }
    return 0;
}


static int proc_param_show(struct seq_file *sfile, void *v)
{
    seq_printf(sfile, "usage:\n");
    h26xenc_show_enc_param_syntax(sfile);
    return 0;
}

static int proc_param_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_param_show, NULL);
}

static int proc_param_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];

    if (count > sizeof(proc_buffer) - 1) {
        DBG_ERR("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        DBG_ERR("proc dbg copy from user failed\n");
        return -EFAULT;
    }
    proc_buffer[count] = '\0';

    h26xenc_set_enc_param_syntax(proc_buffer);

    return count;
}

static struct file_operations proc_param_fops = {
    .owner = THIS_MODULE,
    .open = proc_param_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_param_write
};
#endif

#if JND_DEFAULT_ENABLE
static int proc_jnd_show(struct seq_file *sfile, void *v)
{
    H26XEncJndCfg jnd_param;
    h26xEnc_getJNDParam(&jnd_param);
    seq_printf(sfile, "usage: echo [en] [str] [level] [th] > /proc/kdrv_vdocdc/jnd\n");
    seq_printf(sfile, "== jnd en %d, str %d, level %d, th %d ==\n", jnd_param.bEnable, jnd_param.ucStr, jnd_param.ucLevel, jnd_param.ucTh);
    return 0;
}

static int proc_jnd_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_jnd_show, NULL);
}

static int proc_jnd_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];
    int en, str, level, th;

    if (count > sizeof(proc_buffer) - 1) {
        DBG_ERR("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        DBG_ERR("proc dbg copy from user failed\n");
        return -EFAULT;
    }
    proc_buffer[count] = '\0';

    sscanf(proc_buffer, "%d %d %d %d", &en, &str, &level, &th);
    if (0 != en && 1 != en)
        goto exit_write;
    if (str < 0 || str > 15)
        goto exit_write;
    if (level < 0 || level > 15)
        goto exit_write;
    if (th < 0 || th > 255)
        goto exit_write;
    h26xEnc_setJNDParam((UINT8)en, (UINT8)str, (UINT8)level, (UINT8)th);
exit_write:
    return count;
}

static struct file_operations proc_jnd_fops = {
    .owner = THIS_MODULE,
    .open = proc_jnd_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_jnd_write
};
#endif

#if LPM_PROC_ENABLE
static int proc_lpm_show(struct seq_file *sfile, void *v)
{
    H26XEncLpmCfg lpm_param;

    h26XEnc_getLpmCfg(&g_enc_info[0].enc_var, &lpm_param);
    //seq_printf(sfile, "usage: echo [en] [str] [level] [th] > /proc/kdrv_vdocdc/lpm\n");
    seq_printf(sfile, "lpm (en:%d, RmdSadEn:%d, ChrmDmEn:%d, IME:(%d, %d), Rdo:(%d, %d), (%d, %d, %d, %d, %d)) ==\n", lpm_param.bEnable, lpm_param.ucRmdSadEn, lpm_param.ucChrmDmEn,
    																lpm_param.ucIMEStopEn, lpm_param.ucIMEStopTh,
    																lpm_param.ucRdoStopEn, lpm_param.ucRdoStopTh,
    																lpm_param.ucChrmDmEn, lpm_param.ucQPMapDefulat,lpm_param.ucI16On, lpm_param.ucIraEn,lpm_param.ucIraTh);
    return 0;
}

static int proc_lpm_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_lpm_show, NULL);
}

static int proc_lpm_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    char proc_buffer[PROC_MAX_BUFFER];

	int ch_id, en, rmdsad_en, chrmdm_en, ime_stop_en, ime_stop_th, rdo_stop_en, rdo_stop_th;
	int chrm_dm_en, qpmap_default, i16_on, ira_en, ira_th;

    H26XEncLpmCfg stLpmCfg = {0};

    if (count > sizeof(proc_buffer) - 1) {
        DBG_ERR("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        DBG_ERR("proc dbg copy from user failed\n");
        return -EFAULT;
    }
    proc_buffer[count] = '\0';

    sscanf(proc_buffer, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &ch_id,	&en, &rmdsad_en, &chrmdm_en,
															&ime_stop_en, &ime_stop_th,
															&rdo_stop_en, &rdo_stop_th,
															&chrm_dm_en, &qpmap_default, &i16_on, &ira_en, &ira_th);

	stLpmCfg.bEnable = en;
	stLpmCfg.ucRmdSadEn = rmdsad_en;
	stLpmCfg.ucChrmDmEn = chrmdm_en;
	stLpmCfg.ucIMEStopEn = ime_stop_en;
	stLpmCfg.ucIMEStopTh = ime_stop_th;
	stLpmCfg.ucRdoStopEn = rdo_stop_en;
	stLpmCfg.ucRdoStopTh = rdo_stop_th;

	stLpmCfg.ucChrmDmEn = chrm_dm_en;
	stLpmCfg.ucQPMapDefulat = qpmap_default;
	stLpmCfg.ucI16On = i16_on;
	stLpmCfg.ucIraEn = ira_en;
	stLpmCfg.ucIraTh = ira_th;

	h26XEnc_setLpmCfg(&g_enc_info[ch_id].enc_var, &stLpmCfg);
    //h26xEnc_setJNDParam((UINT8)en, (UINT8)str, (UINT8)level, (UINT8)th);

    return count;
}

static struct file_operations proc_lpm_fops = {
    .owner = THIS_MODULE,
    .open = proc_lpm_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_lpm_write
};
#endif

#if H26X_USE_DIFF_MAQ
static int proc_diff_maq_show(struct seq_file *sfile, void *v)
{
    int str, num, type, idx_1, idx_2;
    seq_printf(sfile, "usage: echo [str] [num] [morphology] [diff_idx_1] [diff_idx_2] > /proc/kdrv_vdocdc/diff_maq\n");
    h26xEnc_getDiffMAQParam(&str, &num, &type, &idx_1, &idx_2);
    seq_printf(sfile, "==== current value: str %d, num %d, morphology %d idx_1 %d idx_2 %d====\n", str, num, type, idx_1, idx_2);
    return 0;
}

static int proc_diff_maq_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_diff_maq_show, NULL);
}

static int proc_diff_maq_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    int str, num, type, idx_1, idx_2;
    char proc_buffer[PROC_MAX_BUFFER];

    if (count > sizeof(proc_buffer) - 1) {
        DBG_ERR("input is too large\n");
        return -EINVAL;
    }
    if (copy_from_user(proc_buffer, buffer, count)) {
        DBG_ERR("proc dbg copy from user failed\n");
        return -EFAULT;
    }
    proc_buffer[count] = '\0';

    sscanf(proc_buffer, "%d %d %d %d %d", &str, &num, &type, &idx_1, &idx_2);
    h26xEnc_setDiffMAQParam(str, num, type, idx_1, idx_2);

    return count;
}

static struct file_operations proc_diff_maq_fops = {
    .owner = THIS_MODULE,
    .open = proc_diff_maq_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_diff_maq_write
};
#endif

//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
static int proc_version_show(struct seq_file *sfile, void *v)
{
    int nvt_drv_ver = h26xEnc_getVersion();
    seq_printf(sfile, "h26x kdrv version: %s, nvt codec version %d.%d.%d.%d\n", H26XE_VER_STR,
        (nvt_drv_ver>>24)&0xFF, (nvt_drv_ver>>16)&0xFF, (nvt_drv_ver>>8)&0xFF, nvt_drv_ver&0xFF);
    return 0;
}

static int proc_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_version_show, NULL);
}

static int proc_version_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    return count;
}

static struct file_operations proc_version_fops = {
    .owner = THIS_MODULE,
    .open = proc_version_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_version_write
};
#endif

#if H26X_MEM_USAGE
static int proc_mem_usage_show(struct seq_file *sfile, void *v)
{
	H26XMemUsage *p_mem_usage;
	UINT32 i=0, cdc_type, enc_id;

	for (cdc_type = 0; cdc_type < 2; cdc_type++) {
		for (enc_id = 0; enc_id < KDRV_VDOENC_ID_MAX; enc_id++) {
			p_mem_usage = (H26XMemUsage *)h26xEnc_getMemUsage(cdc_type, enc_id);
			if (p_mem_usage->cxt_size) {
				if (cdc_type == 0)
					seq_printf(sfile, "codec_type: h265, chn:%d, enc_buf:%d\r\n", enc_id, p_mem_usage->cxt_size);
				else if (cdc_type == 1)
					seq_printf(sfile, "codec_type: h264, chn:%d, enc_buf:%d\r\n", enc_id, p_mem_usage->cxt_size);
				seq_printf(sfile, "addr_start	addr_end	size	name\r\n");

				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"vdo_ctx");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"func_ctx");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"comn_ctx");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"frm_st0");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"colmv");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"extra_st0");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"si_st0");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"frm_st1");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"colmv_st1");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"extra_st1");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"si_st1");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"frm_lt");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"colmv_lt");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"extra_lt");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"si_lt");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"apb");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"ll");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"bsdma");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"nalu_len");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"rc_ref");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"seq_hdr");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"pic_hdr");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"usrqp");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"md");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"hist");
				i++;
				seq_printf(sfile, "x%08x	x%08x	x%08x	%s\r\n",
						p_mem_usage->buf_cmd[i].st_adr,
						p_mem_usage->buf_cmd[i].st_adr+p_mem_usage->buf_cmd[i].size,
						p_mem_usage->buf_cmd[i].size,
						"gcac");
				i++;
			}
		}
	}
	return 0;
}

static int proc_mem_usage_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_mem_usage_show, NULL);
}

static int proc_mem_usage_write(struct file *file, const char __user * buffer, size_t count, loff_t *ppos)
{
    return count;
}

static struct file_operations proc_mem_usage_fops = {
    .owner = THIS_MODULE,
    .open = proc_mem_usage_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = proc_mem_usage_write
};
#endif

int kdrv_vdocdc_proc_init(void)
{
	int ret = 0;

	if ((proc_root_entry = proc_mkdir("kdrv_vdocdc", NULL)) == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
        ret = -EINVAL;
		goto FAIL_PROC;
	}

//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
	if ((proc_cmd_entry = proc_create("cmd", S_IRUGO | S_IXUGO, proc_root_entry, &proc_cmd_fops)) == NULL) {
		nvt_dbg(ERR, "failed to create proc cmd!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif
#if JND_DEFAULT_ENABLE
    if ((proc_jnd_entry = proc_create("jnd", S_IRUGO | S_IXUGO, proc_root_entry, &proc_jnd_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc dbg!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif
#if LPM_PROC_ENABLE
	if ((proc_lpm_entry = proc_create("lpm", S_IRUGO | S_IXUGO, proc_root_entry, &proc_lpm_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc lpm!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif

#if H26X_USE_DIFF_MAQ
    if ((proc_diff_maq_entry = proc_create("diff_maq", S_IRUGO | S_IXUGO, proc_root_entry, &proc_diff_maq_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc diff_maq!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif
//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
    if ((proc_version_entry = proc_create("version", S_IRUGO | S_IXUGO, proc_root_entry, &proc_version_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc version!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif
#if H26X_MEM_USAGE
    if ((proc_mem_usage_entry = proc_create("mem_usage", S_IRUGO | S_IXUGO, proc_root_entry, &proc_mem_usage_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc mem_usage!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif
#if H26X_SET_PROC_PARAM
    if ((proc_param_entry = proc_create("param", S_IRUGO | S_IXUGO, proc_root_entry, &proc_param_fops)) == NULL) {
        nvt_dbg(ERR, "failed to create proc dbg!\n");
		ret = -EINVAL;
		goto FAIL_PROC;
    }
#endif

	return ret;

FAIL_PROC:
//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
    if (proc_cmd_entry)
    	proc_remove(proc_cmd_entry);
    if (proc_version_entry)
        proc_remove(proc_version_entry);
#endif
#if H26X_MEM_USAGE
    if (proc_param_entry)
	    proc_remove(proc_mem_usage_entry);
#endif
#if H26X_USE_DIFF_MAQ
    if (proc_diff_maq_entry)
        proc_remove(proc_diff_maq_entry);
#endif
#if JND_DEFAULT_ENABLE
    if (proc_jnd_entry)
        proc_remove(proc_jnd_entry);
#endif
#if LPM_PROC_ENABLE
	if (proc_lpm_entry)
		proc_remove(proc_lpm_entry);
#endif
#if H26X_SET_PROC_PARAM
    if (proc_param_entry)
	    proc_remove(proc_param_entry);
#endif
    if (proc_root_entry) {
    	proc_remove(proc_root_entry);
    }

	return ret;
}

void kdrv_vdocdc_proc_remove(void)
{
//#if (H26X_SAVE_KO_SIZE == 0)
#if H26X_PROC_CMD
    if (proc_cmd_entry)
    	proc_remove(proc_cmd_entry);
    if (proc_version_entry)
        proc_remove(proc_version_entry);
#endif
#if H26X_MEM_USAGE
    if (proc_mem_usage_entry)
        proc_remove(proc_mem_usage_entry);
#endif
#if H26X_USE_DIFF_MAQ
    if (proc_diff_maq_entry)
        proc_remove(proc_diff_maq_entry);
#endif
#if JND_DEFAULT_ENABLE
    if (proc_jnd_entry)
        proc_remove(proc_jnd_entry);
#endif
#if LPM_PROC_ENABLE
	if (proc_lpm_entry)
		proc_remove(proc_lpm_entry);
#endif
#if H26X_SET_PROC_PARAM
    if (proc_param_entry)
        proc_remove(proc_param_entry);
#endif
    if (proc_root_entry)
    	proc_remove(proc_root_entry);
}
