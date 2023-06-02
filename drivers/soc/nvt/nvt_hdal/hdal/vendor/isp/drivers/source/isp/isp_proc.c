#include <linux/slab.h>

#include "isp_builtin.h"
#include "isp_dbg.h"
#include "isp_main.h"
#include "isp_proc.h"
#include "isp_version.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"

//=============================================================================
// function declaration
//=============================================================================
INT32 isp_proc_init(ISP_DRV_INFO *pdrv_info);
void isp_proc_remove(ISP_DRV_INFO *pdrv_info);

#define ISP_PROC_MSG_BUFSIZE 2048

static ISP_PROC_MSG_BUF isp_proc_msg;
static ISP_PROC_R_ITEM isp_proc_r_item = ISP_PROC_R_ITEM_NONE;
static ISP_ID isp_proc_id;
static ISP_PROC_R_PARAM isp_proc_r_param;
struct proc_dir_entry *vendor_root = NULL;

//=============================================================================
// interanl functions
//=============================================================================
static inline INT32 isp_proc_alloc_msgbuf(void)
{
	isp_proc_msg.buf = kzalloc(ISP_PROC_MSG_BUFSIZE, GFP_KERNEL);

	if (isp_proc_msg.buf == NULL) {
		DBG_ERR("fail to allocate message buffer!\n");
		return -ENOMEM;
	}

	isp_proc_msg.size = ISP_PROC_MSG_BUFSIZE;
	isp_proc_msg.count = 0;

	return 0;
}

static inline void isp_proc_free_msgbuf(void)
{
	if (isp_proc_msg.buf) {
		kfree(isp_proc_msg.buf);
		isp_proc_msg.buf = NULL;
	}
}

static inline void isp_proc_clean_msgbuf(void)
{
	isp_proc_msg.buf[0] = '\0';
	isp_proc_msg.count = 0;
}

static INT32 isp_proc_cmd_printf(const CHAR *f, ...)
{
	va_list args;
	INT32 len;

	if (isp_proc_msg.count < isp_proc_msg.size) {
		va_start(args, f);
		len = vsnprintf(isp_proc_msg.buf + isp_proc_msg.count, isp_proc_msg.size - isp_proc_msg.count, f, args);
		va_end(args);

		if (isp_proc_msg.count + len < isp_proc_msg.size) {
			isp_proc_msg.count += len;
			return 0;
		}
	}

	isp_proc_msg.count = isp_proc_msg.size;

	return -1;
}

static INT32 isp_proc_cmd_get_param(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	if (argc < 4) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	isp_proc_r_item = ISP_PROC_R_ITEM_PARAM;
	isp_proc_id = simple_strtoul(argv[2], NULL, 0);
	isp_proc_r_param = simple_strtoul(argv[3], NULL, 0);

	return 0;
}

static inline ISP_DRV_INFO *isp_proc_get_drv_info(struct file *file)
{
	// get driver info from file.
	return (ISP_DRV_INFO *)((struct seq_file *)file->private_data)->private;
}

//=============================================================================
// proc "info" file operation functions
//=============================================================================
static INT32 isp_proc_info_show(struct seq_file *sfile, void *v)
{
	ISP_ID id;
	UINT32 i;
	UINT32 version = isp_get_version();
	ISP_FUNC_EN fun_en;
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_FUNC_INFO func_info[ISP_ID_MAX_NUM] = {0};
	ISP_SENSOR_INFO sensor_info = {0};
	ISP_SENSOR_DIRECTION sensor_direction[CTL_SEN_ID_MAX] = {0};

	if (pdev_info == NULL) {
		return 0;
	}

	isp_api_get_sensor_info(&sensor_info);
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (sensor_info.src_id_mask[id] != 0) {
			isp_api_get_func(id, &func_info[id]);
			isp_api_get_direction(id, &sensor_direction[id]);
		}
	}

	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "NVT_ISP            v%d.%d.%d.%d\n", (version>>24) & 0xFF, (version>>16) & 0xFF, (version>>8) & 0xFF, version & 0xFF);
	if (isp_api_get_fastboot_valid()) {
		seq_printf(sfile, "NVT_ISP_BUILTIN    v%d.%d.%d.%d\n", (isp_builtin_get_isp_version()>>24) & 0xFF, (isp_builtin_get_isp_version()>>16) & 0xFF, (isp_builtin_get_isp_version()>>8) & 0xFF, isp_builtin_get_isp_version() & 0xFF);
		seq_printf(sfile, "NVT_AE_BUILTIN     v%d.%d.%d.%d\n", (isp_builtin_get_ae_version()>>24) & 0xFF, (isp_builtin_get_ae_version()>>16) & 0xFF, (isp_builtin_get_ae_version()>>8) & 0xFF, isp_builtin_get_ae_version() & 0xFF);
		seq_printf(sfile, "NVT_AWB_BUILTIN    v%d.%d.%d.%d\n", (isp_builtin_get_awb_version()>>24) & 0xFF, (isp_builtin_get_awb_version()>>16) & 0xFF, (isp_builtin_get_awb_version()>>8) & 0xFF, isp_builtin_get_awb_version() & 0xFF);
		seq_printf(sfile, "NVT_IQ_BUILTIN     v%d.%d.%d.%d\n", (isp_builtin_get_iq_version()>>24) & 0xFF, (isp_builtin_get_iq_version()>>16) & 0xFF, (isp_builtin_get_iq_version()>>8) & 0xFF, isp_builtin_get_iq_version() & 0xFF);
		seq_printf(sfile, "NVT_SENSOR_BUILTIN v%d.%d.%d.%d\n", (isp_builtin_get_sensor_version()>>24) & 0xFF, (isp_builtin_get_sensor_version()>>16) & 0xFF, (isp_builtin_get_sensor_version()>>8) & 0xFF, isp_builtin_get_sensor_version() & 0xFF);
	}

	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "Module Info: \n");
	seq_printf(sfile, "AE: %s, AF: %s, AWB: %s, DR: %s, IQ: %s \n", pdev_info->isp_ae->name, pdev_info->isp_af->name, pdev_info->isp_awb->name, pdev_info->isp_dr->name, pdev_info->isp_iq->name);
	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "           id: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		seq_printf(sfile, " %8d", id);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "  AE:   0x1,   AWB:   0x2,   AF:  0x4,  WDR:  0x8           \n");
	seq_printf(sfile, "SHDR:  0x10, DEFOG:  0x20,  DIS: 0x40,  RSC: 0x80\n");
	seq_printf(sfile, " ETH: 0x100,   GDC: 0x200                      \n");
	seq_printf(sfile, " CAP func.(h): ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			isp_get_sie_func(id, &fun_en);
			seq_printf(sfile, " %8X", fun_en);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "PROC func.(h): ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			isp_get_ipp_func(id, &fun_en);
			seq_printf(sfile, " %8X", fun_en);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    CAP valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].sie_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "   PROC valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].ipp_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "     AE valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].ae_valid);
		} else {
			seq_printf(sfile, "         ");
		}

	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "     AF valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].af_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    AWB valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].awb_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  Defog valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].defog_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "   SHDR valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].shdr_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "    WDR valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", func_info[id].wdr_valid);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  Sensor name: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8s", sensor_info.name[id]);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "  Sensor mask: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, " %8d", sensor_info.src_id_mask[id]);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "   Sensor dir: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			seq_printf(sfile, "  M:%1d/F:%1d", sensor_direction[id].mirror, sensor_direction[id].flip);
		} else {
			seq_printf(sfile, "         ");
		}
	}
	seq_printf(sfile, "\n");

	for (i = 0; i < ISP_YUV_OUT_CH; i++) {
		seq_printf(sfile, " YUV output %d: ", i);
		for (id = 0; id < ISP_ID_MAX_NUM; id++) {
			if (!isp_get_id_valid(id)) {
				continue;
			}
			if (sensor_info.src_id_mask[id] != 0) {
				seq_printf(sfile, " %4d%4d", func_info[id].yuv_out_ch[i].w, func_info[id].yuv_out_ch[i].h);
			} else {
				seq_printf(sfile, "         ");
			}
		}
		seq_printf(sfile, "\n");
	}
	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	seq_printf(sfile, "    Error Cnt: ");
	seq_printf(sfile, " %8d \n", isp_dbg_get_err_msg());
	seq_printf(sfile, "  Warning Cnt: ");
	seq_printf(sfile, " %8d \n", isp_dbg_get_wrn_msg());
	seq_printf(sfile, "-----------------------------------------------------------------------------------------\r\n");
	return 0;
}

static INT32 isp_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_proc_info_show, PDE_DATA(inode));
}

static const struct file_operations isp_proc_info_fops = {
	.owner   = THIS_MODULE,
	.open    = isp_proc_info_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

#if 0
static INT32 isp_proc_cmd_empty(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	return 0;
}
#endif

static INT32 isp_proc_cmd_set_dbg(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;
	UINT32 cmd;

	if (argc < 4) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cmd = simple_strtoul(argv[3], NULL, 0);

	isp_proc_cmd_printf("set isp(%d) dbg level(0x%X) \n", (int)id, (unsigned int)cmd);
	isp_dbg_set_dbg_mode(id, cmd);

	return 0;
}

static INT32 isp_proc_cmd_set_bypass(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;
	UINT32 cmd;

	if (argc < 4) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	cmd = simple_strtoul(argv[3], NULL, 0);

	isp_proc_cmd_printf("set isp(%d) bypass eng(0x%X) \n", (int)id, (unsigned int)cmd);
	isp_dbg_set_bypass_eng(id, cmd);

	return 0;
}

static INT32 isp_proc_cmd_set_sensor_dir(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;
	ISP_SENSOR_DIRECTION sensor_dir = {0};

	if (argc < 4) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	sensor_dir.mirror = simple_strtoul(argv[3], NULL, 0);
	sensor_dir.flip = simple_strtoul(argv[4], NULL, 0);

	isp_api_set_direction(id, &sensor_dir);

	return 0;
}

static INT32 isp_proc_cmd_set_sensor_sleep(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;

	if (argc < 2) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	isp_api_set_sensor_sleep(id);

	return 0;
}

static INT32 isp_proc_cmd_set_sensor_wakeup(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;

	if (argc < 2) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);

	isp_api_set_sensor_wakeup(id);

	return 0;
}

static INT32 isp_proc_cmd_set_emu_new_buf(ISP_DRV_INFO *pdrv, INT32 argc, CHAR **argv)
{
	ISP_ID id;
	CTL_SIE_ISP_SIM_BUF_NEW buf_new = {0};
	ER rt = E_OK;

	if (argc < 4) {
		isp_proc_cmd_printf("wrong argument:%d", argc);
		return -EINVAL;
	}

	id = simple_strtoul(argv[2], NULL, 0);
	buf_new.frm_cnt = simple_strtoul(argv[3], NULL, 0);
	buf_new.buf_size = simple_strtoul(argv[4], NULL, 0);

	rt = isp_api_set_emu_new_buf(id, &buf_new);

	printk("rt = %d, id = %d, frm_cnt = %d, buf_size = 0x%X, buf_id = 0x%X, buf_addr = 0x%X \r\n", rt, id, buf_new.frm_cnt, buf_new.buf_size, buf_new.buf_id, buf_new.buf_addr);

	return 0;
}

static ISP_PROC_CMD isp_proc_r_cmd_list[] = {
	// keyword              function name
	{ "get_param",          isp_proc_cmd_get_param,          "get isp param, param1 is isp_id(0~1), param2 is param_sel."},
};
#define NUM_OF_R_CMD (sizeof(isp_proc_r_cmd_list) / sizeof(ISP_PROC_CMD))

static ISP_PROC_CMD isp_proc_w_cmd_list[] = {
	// keyword              function name
	{ "dbg",                isp_proc_cmd_set_dbg,            "set isp dbg level, param1 is isp_id(0~1, 8 means apply to each id), param2 is dbg_lv."},
	{ "bypass",             isp_proc_cmd_set_bypass,         "set isp bypass eng, param1 is isp_id(0~1, 8 means apply to each id), param2 is bypass_eng."},
	{ "dir",                isp_proc_cmd_set_sensor_dir,     "set sensor direction, param1 is isp_id(0~1), param2 is mirror, param3 is flip."},
	{ "sleep",              isp_proc_cmd_set_sensor_sleep,   "set sensor sleep, param1 is isp_id(0~1)."},
	{ "wakeup",             isp_proc_cmd_set_sensor_wakeup,  "set sensor wakeup, param1 is isp_id(0~1)."},
	{ "emu_new",            isp_proc_cmd_set_emu_new_buf,    "set emu new buffer, param1 is isp_id(0~1), param2 is frm_cnt, param3 is buf_size."},
};
#define NUM_OF_W_CMD (sizeof(isp_proc_w_cmd_list) / sizeof(ISP_PROC_CMD))

static INT32 isp_proc_command_show(struct seq_file *sfile, void *v)
{
	ISP_CA_RSLT *ca_rslt;
	ISP_LA_RSLT *la_rslt;
	ISP_VA_RSLT *va_rslt;
	ISP_HISTO_RSLT *histo_rslt;
	UINT32 i, j;

	if ((isp_proc_msg.buf == NULL) && (isp_proc_r_item == ISP_PROC_R_ITEM_NONE)) {
		return -EINVAL;
	}

	if (isp_proc_msg.buf > 0) {
		seq_printf(sfile, "%s\n", isp_proc_msg.buf);
		isp_proc_clean_msgbuf();
	}

	if (isp_proc_r_item == ISP_PROC_R_ITEM_PARAM) {
		switch (isp_proc_r_param) {
		case ISP_PROC_R_PARAM_CA:
			ca_rslt = isp_dev_get_ca(isp_proc_id);

			if (ca_rslt == NULL) {
				seq_printf(sfile, "Get ca result fail. \n");
				break;
			}

			seq_printf(sfile, "CA R Result: \n");
			for (i = 0; i < ISP_CA_W_WINNUM; i++) {
				for (j = 0; j < ISP_CA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", ca_rslt->r[i*ISP_CA_W_WINNUM+j]);
				}
				seq_printf(sfile, "\n");
			}
			seq_printf(sfile, "\n");
			seq_printf(sfile, "CA G Result: \n");
			for (i = 0; i < ISP_CA_W_WINNUM; i++) {
				for (j = 0; j < ISP_CA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", ca_rslt->g[i*ISP_CA_W_WINNUM+j]);
				}
				seq_printf(sfile, "\n");
			}
			seq_printf(sfile, "\n");
			seq_printf(sfile, "CA B Result: \n");
			for (i = 0; i < ISP_CA_W_WINNUM; i++) {
				for (j = 0; j < ISP_CA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", ca_rslt->b[i*ISP_CA_W_WINNUM+j]);
				}
				seq_printf(sfile, "\n");
			}
			break;

		case ISP_PROC_R_PARAM_CA_ACC_CNT:
			ca_rslt = isp_dev_get_ca(isp_proc_id);
			
			if (ca_rslt == NULL) {
				seq_printf(sfile, "Get ca result fail. \n");
				break;
			}

			seq_printf(sfile, "CA ACC Result: \n");
			for (i = 0; i < ISP_CA_W_WINNUM; i++) {
				for (j = 0; j < ISP_CA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", ca_rslt->acc_cnt[i*ISP_CA_W_WINNUM+j]);
				}
				seq_printf(sfile, "\n");
			}
			break;

		case ISP_PROC_R_PARAM_LA:
			la_rslt = isp_dev_get_la(isp_proc_id);

			if (la_rslt == NULL) {
				seq_printf(sfile, "Get la result fail. \n");
				break;
			}

			seq_printf(sfile, "LA Result: \n");
			for (i = 0; i < ISP_LA_W_WINNUM; i++) {
				for (j = 0; j < ISP_LA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", la_rslt->lum_1[i*ISP_LA_W_WINNUM+j]);
				}
				seq_printf(sfile, "\n");
			}
			break;

		case ISP_PROC_R_PARAM_VA:
			va_rslt = isp_dev_get_va(isp_proc_id);

			if (va_rslt == NULL) {
				seq_printf(sfile, "Get va result fail. \n");
				break;
			}

			seq_printf(sfile, "VA Result: \n");
			for (i = 0; i < ISP_VA_W_WINNUM; i++) {
				for (j = 0; j < ISP_VA_H_WINNUM; j++) {
					seq_printf(sfile, "%4d ", (va_rslt->g1_h[i*ISP_VA_W_WINNUM+j]+va_rslt->g1_v[i*ISP_VA_W_WINNUM+j]+va_rslt->g2_h[i*ISP_VA_W_WINNUM+j]+va_rslt->g2_v[i*ISP_VA_W_WINNUM+j])>>2);
				}
				seq_printf(sfile, "\n");
			}
			break;

		case ISP_PROC_R_PARAM_HISTO:
			histo_rslt = isp_dev_get_histo(isp_proc_id);

			if (histo_rslt == NULL) {
				seq_printf(sfile, "Get histo result fail. \n");
				break;
			}

			seq_printf(sfile, "HISTO Result: \n");
			seq_printf(sfile, "pre : ");
			for (i = 0; i < ISP_HISTO_MAX_SIZE; i++) {
				seq_printf(sfile, "%4d ", histo_rslt->hist_stcs_pre_wdr[i]);
			}
			seq_printf(sfile, "\n");
			seq_printf(sfile, "post : ");
			for (i = 0; i < ISP_HISTO_MAX_SIZE; i++) {
				seq_printf(sfile, "%4d ", histo_rslt->hist_stcs_post_wdr[i]);
			}
			seq_printf(sfile, "\n");
			break;

		default:
			break;
		}
	}
	isp_proc_r_item = ISP_PROC_R_ITEM_NONE;
	return 0;
}

static INT32 isp_proc_command_open(struct inode *inode, struct file *file)
{
	//return single_open(file, isp_proc_command_show, PDE_DATA(inode));
	return single_open_size(file, isp_proc_command_show, PDE_DATA(inode), 4096*sizeof(u32));
}

static ssize_t isp_proc_command_write(struct file *file, const CHAR __user *buf, size_t size, loff_t *off)
{
	INT32 len = size;
	INT32 ret = -EINVAL;
	ISP_DRV_INFO *pdrv_info = isp_proc_get_drv_info(file);
	ISP_DEV_INFO *pdev_info = &pdrv_info->dev_info;
	CHAR cmd_line[MAX_CMDLINE_LENGTH];
	CHAR *cmdstr = cmd_line;
	const CHAR delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	CHAR *argv[MAX_CMD_ARGUMENTS] = {NULL};
	INT32 argc = 0;
	UINT32 i;

	isp_proc_clean_msgbuf();

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
					if (strncmp(argv[1], isp_proc_r_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						down(&pdev_info->proc_mutex);
						ret = isp_proc_r_cmd_list[i].execute(pdrv_info, argc, argv);
						up(&pdev_info->proc_mutex);
						break;
					}
				}

				if (i >= NUM_OF_R_CMD) {
					DBG_ERR("[ISP_ERR]: => ");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in r_cmd_list!\n");
				}
			} else if (strncmp(argv[0], "w", 2) == 0) {
				for (i = 0; i < NUM_OF_W_CMD; i++) {
					if (strncmp(argv[1], isp_proc_w_cmd_list[i].cmd, MAX_CMD_LENGTH) == 0) {
						down(&pdev_info->proc_mutex);
						ret = isp_proc_w_cmd_list[i].execute(pdrv_info, argc, argv);
						up(&pdev_info->proc_mutex);
						break;
					}
				}

				if (i >= NUM_OF_W_CMD) {
					DBG_ERR("[ISP_ERR]: =>");
					for (i = 0; i < argc; i++) {
						DBG_ERR("%s ", argv[i]);
					}
					DBG_ERR("is not in w_cmd_list!\n");
				}
			} else {
				DBG_ERR("[ISP_ERR]: =>");
				for (i = 0; i < argc; i++) {
					DBG_ERR("%s ", argv[i]);
				}
				DBG_ERR("is not legal command!\n");
			}
		}
	}

	if (ret < 0) {
		DBG_ERR("[ISP_ERR]: fail to execute: ");
		for (i = 0; i < argc; i++) {
			DBG_ERR("%s ", argv[i]);
		}
		DBG_ERR("\n");
	}

	return size;
}

static const struct file_operations isp_proc_command_fops = {
	.owner	 = THIS_MODULE,
	.open	 = isp_proc_command_open,
	.read	 = seq_read,
	.write	 = isp_proc_command_write,
	.llseek  = seq_lseek,
	.release = single_release,
};

//=============================================================================
// proc "help" file operation functions
//=============================================================================
static INT32 isp_proc_help_show(struct seq_file *sfile, void *v)
{
	UINT32 loop;

	seq_printf(sfile, "1. 'cat /proc/hdal/vendor/isp/info' will show all the isp info\r\n");
	seq_printf(sfile, "2. 'echo r/w xxx > /proc/hdal/vendor/isp/cmd' can input command for some debug purpose\r\n");
	seq_printf(sfile, "The currently support input command are below:\r\n");

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "  %s\n", "isp");
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");

	for (loop = 0 ; loop < NUM_OF_R_CMD ; loop++) {
		seq_printf(sfile, "r %15s : %s\r\n", isp_proc_r_cmd_list[loop].cmd, isp_proc_r_cmd_list[loop].text);
	}

	for (loop = 0 ; loop < NUM_OF_W_CMD ; loop++) {
		seq_printf(sfile, "w %15s : %s\r\n", isp_proc_w_cmd_list[loop].cmd, isp_proc_w_cmd_list[loop].text);
	}

	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, " dbg_lv = \r\n");
	seq_printf(sfile, " | 0x%8X = AE STATUS    | 0x%8X = TOTAL GAIN   | 0x%8X = D GAIN       | \r\n", ISP_DBG_SYNC_AE_STATUS, ISP_DBG_SYNC_TOTAL_GAIN, ISP_DBG_SYNC_D_GAIN);
	seq_printf(sfile, " | 0x%8X = LV           | 0x%8X = LV_BASE      | \r\n", ISP_DBG_SYNC_LV, ISP_DBG_SYNC_LV_BASE);
	seq_printf(sfile, " | 0x%8X = C GAIN       | 0x%8X = FINAL C GAIN | 0x%8X = CT           | 0x%8X = CAPTURE      | \r\n", ISP_DBG_SYNC_C_GAIN, ISP_DBG_SYNC_FINAL_C_GAIN, ISP_DBG_SYNC_CT, ISP_DBG_SYNC_CAPTURE);
	seq_printf(sfile, " | 0x%8X = SENSOR EXPT  | 0x%8X = SENSOR GAIN  | 0x%8X = SENSOR REG.  | \r\n", ISP_DBG_SENSOR_EXPT, ISP_DBG_SENSOR_GAIN, ISP_DBG_SENSOR_REG);
	seq_printf(sfile, " | 0x%8X = SIE CB       | 0x%8X = IPP CB       | 0x%8X = ENC CB       | \r\n", ISP_DBG_SIE_CB, ISP_DBG_IPP_CB, ISP_DBG_ENC_CB);
	seq_printf(sfile, " | 0x%8X = SIE SET      | 0x%8X = IPP SET      | 0x%8X = ENC SET      | \r\n", ISP_DBG_SIE_SET, ISP_DBG_IPP_SET, ISP_DBG_ENC_SET);
	seq_printf(sfile, " | 0x%8X = ERR MSG      | 0x%8X = WRN MSG      | \r\n", ISP_DBG_ERR_MSG, ISP_DBG_WRN_MSG);
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, " bypass_eng = \r\n");
	seq_printf(sfile, " | 0x%8X = SIE_ROI      | 0x%8X = SIE_PARAM    | 0x%8X = IFE_PARAM    | 0x%8X = DCE_PARAM    | \r\n", ISP_BYPASS_SIE_ROI, ISP_BYPASS_SIE_PARAM, ISP_BYPASS_IFE_PARAM, ISP_BYPASS_DCE_PARAM);
	seq_printf(sfile, " | 0x%8X = IPE_PARAM    | 0x%8X = IFE2_PARAM   | 0x%8X = IME_PARAM    | 0x%8X = IFE_VIG_CENT | \r\n", ISP_BYPASS_IPE_PARAM, ISP_BYPASS_IFE2_PARAM, ISP_BYPASS_IME_PARAM, ISP_BYPASS_IFE_VIG_CENT);
	seq_printf(sfile, " | 0x%8X = DCE_DC_CENT  | 0x%8X = IFE2_FILT_T  | 0x%8X = IME_LCA_SIZE | 0x%8X = IPE_VA_WIN_S | \r\n", ISP_BYPASS_DCE_DC_CENT, ISP_BYPASS_IFE2_FILT_TIME, ISP_BYPASS_IME_LCA_SIZE, ISP_BYPASS_IPE_VA_WIN_SIZE);
	seq_printf(sfile, " | 0x%8X = ENC_NR_PARAM | 0x%8X = ENC_SP_PARAM | 0x%8X = SEN_REG      | 0x%8X = SEN_EXPT     | \r\n", ISP_BYPASS_ENC_3DNR_PARAM, ISP_BYPASS_ENC_3DNR_PARAM, ISP_BYPASS_SEN_REG, ISP_BYPASS_SEN_EXPT);
	seq_printf(sfile, " | 0x%8X = SEN_GAIN     | 0x%8X = SEN_DIR      | 0x%8X = SEN_SLEEP    | 0x%8X = SEN_WAKEUP   | \r\n", ISP_BYPASS_SEN_GAIN, ISP_BYPASS_SEN_DIR, ISP_BYPASS_SEN_SLEEP, ISP_BYPASS_SEN_WAKEUP);
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, " param_sel = \r\n");
	seq_printf(sfile, " | 0x%8X = CA DATA      | 0x%8X = LA DATA     | 0x%8X = VA DATA     | 0x%8X = HISTO DATA  | \r\n", (unsigned int)ISP_PROC_R_PARAM_CA, (unsigned int)ISP_PROC_R_PARAM_LA, (unsigned int)ISP_PROC_R_PARAM_VA, (unsigned int)ISP_PROC_R_PARAM_HISTO);
	seq_printf(sfile, "---------------------------------------------------------------------\r\n");
	seq_printf(sfile, "Ex: 'echo w dbg 8 0x00000 > /proc/hdal/vendor/isp/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w dbg 8 0x03000 > /proc/hdal/vendor/isp/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w dbg 8 0x30000 > /proc/hdal/vendor/isp/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w bypass 8 0x0000 > /proc/hdal/vendor/isp/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w bypass 8 0x07FC > /proc/hdal/vendor/isp/cmd' \r\n");
	seq_printf(sfile, "Ex: 'echo w dir 0 0 0 > /proc/hdal/vendor/isp/cmd'    \r\n");
	seq_printf(sfile, "Ex: 'echo w sleep 0 > /proc/hdal/vendor/isp/cmd'      \r\n");
	seq_printf(sfile, "Ex: 'echo w wakeup 0 > /proc/hdal/vendor/isp/cmd'     \r\n");
	seq_printf(sfile, "Ex: 'echo r get_param 0 2 > /proc/hdal/vendor/isp/cmd;cat /proc/hdal/vendor/isp/cmd' \r\n");

	return 0;
}

static INT32 isp_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, isp_proc_help_show, PDE_DATA(inode));
}

static const struct file_operations isp_proc_help_fops = {
	.owner   = THIS_MODULE,
	.open    = isp_proc_help_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

//=============================================================================
// extern functions
//=============================================================================
INT32 isp_proc_init(ISP_DRV_INFO *pdrv_info)
{
	INT32 ret = 0;
	struct proc_dir_entry *root = NULL;
	struct proc_dir_entry *pentry = NULL;

	// create root entry
	vendor_root = proc_mkdir("hdal/vendor", NULL);
	if (vendor_root == NULL) {
		DBG_ERR("failed to create hdal/vendor root\r\n");
		ret = -EINVAL;
		return ret;
	}

	// create root entry
	root = proc_mkdir("hdal/vendor/isp", NULL);
	if (root == NULL) {
		DBG_ERR("fail to create proc root!\n");
		return -EINVAL;
	}
	pdrv_info->proc_root = root;

	// create "info" entry
	pentry = proc_create_data("info", S_IRUGO | S_IXUGO, root, &isp_proc_info_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc info!\n");
		isp_proc_remove(pdrv_info);
		return -EINVAL;
	}
	pdrv_info->proc_info = pentry;

	// create "command" entry
	pentry = proc_create_data("cmd", S_IRUGO | S_IXUGO, root, &isp_proc_command_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc command!\n");
		isp_proc_remove(pdrv_info);
		return -EINVAL;
	}
	pdrv_info->proc_command = pentry;

	// create "help" entry
	pentry = proc_create_data("help", S_IRUGO | S_IXUGO, root, &isp_proc_help_fops, (void *)pdrv_info);
	if (pentry == NULL) {
		DBG_ERR("fail to create proc help!\n");
		isp_proc_remove(pdrv_info);
		return -EINVAL;
	}
	pdrv_info->proc_help = pentry;

	// allocate memory for massage buffer
	ret = isp_proc_alloc_msgbuf();
	if (ret < 0) {
		isp_proc_remove(pdrv_info);
	}

	return ret;
}

void isp_proc_remove(ISP_DRV_INFO *pdrv_info)
{
	if (pdrv_info->proc_root == NULL) {
		return;
	}

	// remove "info"
	if (pdrv_info->proc_info) {
		proc_remove(pdrv_info->proc_info);
	}

	// remove "command"
	if (pdrv_info->proc_command) {
		proc_remove(pdrv_info->proc_command);
	}

	// remove "help"
	if (pdrv_info->proc_help) {
		proc_remove(pdrv_info->proc_help);
	}

	// remove root entry
	proc_remove(pdrv_info->proc_root);

	// remove vendor root entry
	proc_remove(vendor_root);

	// free message buffer
	isp_proc_free_msgbuf();
}
