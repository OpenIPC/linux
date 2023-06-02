#include <stdio.h>
#include <string.h>

#include "kwrap/nvt_type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "isp_dbg.h"
#include "isp_api.h"
#include "isp_dev_int.h"
#include "isp_version.h"

static BOOL isp_sxcmd_dump_info(unsigned char argc, char **argv)
{
	ISP_ID id;
	UINT32 i;
	UINT32 version = isp_get_version();
	ISP_FUNC_EN fun_en;
	ISP_DEV_INFO *pdev_info = isp_get_dev_info();
	ISP_FUNC_INFO func_info[ISP_ID_MAX_NUM] = {0};
	ISP_SENSOR_INFO sensor_info = {0};
	ISP_SENSOR_DIRECTION sensor_direction[CTL_SEN_ID_MAX] = {0};

	isp_api_get_sensor_info(&sensor_info);
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (sensor_info.src_id_mask[id] != 0) {
			isp_api_get_func(id, &func_info[id]);
			isp_api_get_direction(id, &sensor_direction[id]);
		}
	}

	printf("-----------------------------------------------------------------------------------------\r\n");
	printf("NVT_ISP v%d.%d.%d.%d \n"
		, (int)(version>>24) & 0xFF, (int)(version>>16) & 0xFF, (int)(version>>16) & 0xFF, (int)(version) & 0xFF);

	printf("-----------------------------------------------------------------------------------------\r\n");
	printf("Module Info: \n");
	printf("AE: %s, AF: %s, AWB: %s, DR: %s, IQ: %s \n", pdev_info->isp_ae->name, pdev_info->isp_af->name, pdev_info->isp_awb->name, pdev_info->isp_dr->name, pdev_info->isp_iq->name);
	printf("-----------------------------------------------------------------------------------------\r\n");
	printf("           id: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		printf(" %8d", id);
	}
	printf("\n");

	printf("-----------------------------------------------------------------------------------------\r\n");
	printf("  AE:   0x1,   AWB:   0x2,   AF:  0x4,  WDR:  0x8           \n");
	printf("SHDR:  0x10, DEFOG:  0x20,  DIS: 0x40,  RSC: 0x80\n");
	printf(" ETH: 0x100,   GDC: 0x200                      \n");
	printf(" CAP func.(h): ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			isp_get_sie_func(id, &fun_en);
			printf(" %8X", fun_en);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("PROC func.(h): ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			isp_get_ipp_func(id, &fun_en);
			printf(" %8X", fun_en);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("    CAP valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].sie_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("   PROC valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].ipp_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("     AE valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].ae_valid);
		} else {
			printf("         ");
		}

	}
	printf("\n");

	printf("     AF valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].af_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("    AWB valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].awb_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("  Defog valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].defog_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("   SHDR valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].shdr_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("    WDR valid: ");
	for (id = ISP_ID_1; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", func_info[id].wdr_valid);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("  Sensor name: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8s", sensor_info.name[id]);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("  Sensor mask: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf(" %8d", (int)sensor_info.src_id_mask[id]);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	printf("   Sensor dir: ");
	for (id = 0; id < ISP_ID_MAX_NUM; id++) {
		if (!isp_get_id_valid(id)) {
			continue;
		}
		if (sensor_info.src_id_mask[id] != 0) {
			printf("  M:%1d/F:%1d", sensor_direction[id].mirror, sensor_direction[id].flip);
		} else {
			printf("         ");
		}
	}
	printf("\n");

	for (i = 0; i < ISP_YUV_OUT_CH; i++) {
		printf(" YUV output %d: ", (int)i);
		for (id = 0; id < ISP_ID_MAX_NUM; id++) {
			if (!isp_get_id_valid(id)) {
				continue;
			}
			if (sensor_info.src_id_mask[id] != 0) {
				printf(" %4d%4d", (int)func_info[id].yuv_out_ch[i].w, (int)func_info[id].yuv_out_ch[i].h);
			} else {
				printf("         ");
			}
		}
		printf("\n");
	}
	printf("-----------------------------------------------------------------------------------------\r\n");
	printf("    Error Cnt: ");
	printf(" %8d \n", (int)isp_dbg_get_err_msg());
	printf("  Warning Cnt: ");
	printf(" %8d \n", (int)isp_dbg_get_wrn_msg());
	printf("-----------------------------------------------------------------------------------------\r\n");

	return TRUE;
}

static BOOL isp_sxcmd_dump_ca(unsigned char argc, char **argv)
{
	ISP_CA_RSLT *ca_rslt;
	ISP_ID id;
	UINT32 i, j;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);

	ca_rslt = isp_dev_get_ca(id);
	if (ca_rslt == NULL) {
		printf("Get ca result fail. \n");
		return FALSE;
	}

	printf("CA R Result: \n");
	for (i = 0; i < ISP_CA_W_WINNUM; i++) {
		for (j = 0; j < ISP_CA_H_WINNUM; j++) {
			printf("%4d ", ca_rslt->r[i*ISP_CA_W_WINNUM+j]);
		}
		printf("\n");
	}
	printf("\n");
	printf("CA G Result: \n");
	for (i = 0; i < ISP_CA_W_WINNUM; i++) {
		for (j = 0; j < ISP_CA_H_WINNUM; j++) {
			printf("%4d ", ca_rslt->g[i*ISP_CA_W_WINNUM+j]);
		}
		printf("\n");
	}
	printf("\n");
	printf("CA B Result: \n");
	for (i = 0; i < ISP_CA_W_WINNUM; i++) {
		for (j = 0; j < ISP_CA_H_WINNUM; j++) {
			printf("%4d ", ca_rslt->b[i*ISP_CA_W_WINNUM+j]);
		}
		printf("\n");
	}

	return TRUE;
}

static BOOL isp_sxcmd_dump_la(unsigned char argc, char **argv)
{
	ISP_LA_RSLT *la_rslt;
	ISP_ID id;
	UINT32 i, j;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);

	la_rslt = isp_dev_get_la(id);
	if (la_rslt == NULL) {
		printf("Get la result fail. \n");
		return FALSE;
	}

	printf("LA Result: \n");
	for (i = 0; i < ISP_LA_W_WINNUM; i++) {
		for (j = 0; j < ISP_LA_H_WINNUM; j++) {
			printf("%4d ", la_rslt->lum_1[i*ISP_LA_W_WINNUM+j]);
		}
		printf("\n");
	}

	return TRUE;
}

static BOOL isp_sxcmd_dump_va(unsigned char argc, char **argv)
{
	ISP_VA_RSLT *va_rslt;
	ISP_ID id;
	UINT32 i, j;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);

	va_rslt = isp_dev_get_va(id);
	if (va_rslt == NULL) {
		printf("Get va result fail. \n");
		return FALSE;
	}

	printf("VA Result: \n");
	for (i = 0; i < ISP_VA_W_WINNUM; i++) {
		for (j = 0; j < ISP_VA_H_WINNUM; j++) {
			printf("%4d ", (int)(va_rslt->g1_h[i*ISP_VA_W_WINNUM+j]+va_rslt->g1_v[i*ISP_VA_W_WINNUM+j]+va_rslt->g2_h[i*ISP_VA_W_WINNUM+j]+va_rslt->g2_v[i*ISP_VA_W_WINNUM+j])>>2);
		}
		printf("\n");
	}

	return TRUE;
}

static BOOL isp_sxcmd_set_dbg(unsigned char argc, char **argv)
{
	ISP_ID id;
	UINT32 cmd;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);
	sscanf(argv[1], "%x", (int *)&cmd);

	printf("set isp(%d) dbg level(0x%X) \n", (int)id, (unsigned int)cmd);
	isp_dbg_set_dbg_mode(id, cmd);

	return TRUE;
}

static BOOL isp_sxcmd_set_bypass(unsigned char argc, char **argv)
{
	ISP_ID id;
	UINT32 cmd;

	if (argc < 2) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);
	sscanf(argv[1], "%x", (int *)&cmd);

	printf("set isp(%d) bypass eng(0x%X) \n", (int)id, (unsigned int)cmd);
	isp_dbg_set_bypass_eng(id, cmd);

	return TRUE;
}

static BOOL isp_sxcmd_set_sensor_dir(unsigned char argc, char **argv)
{
	ISP_ID id;
	ISP_SENSOR_DIRECTION sensor_dir = {0};

	if (argc < 3) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);
	sscanf(argv[1], "%x", &sensor_dir.mirror);
	sscanf(argv[2], "%x", &sensor_dir.flip);

	isp_api_set_direction(id, &sensor_dir);

	return TRUE;
}

static BOOL isp_sxcmd_set_sensor_sleep(unsigned char argc, char **argv)
{
	ISP_ID id;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);

	isp_api_set_sensor_sleep(id);

	return TRUE;
}

static BOOL isp_sxcmd_set_sensor_wakeup(unsigned char argc, char **argv)
{
	ISP_ID id;

	if (argc < 1) {
		DBG_DUMP("wrong argument:%d \r\n", argc);
		return FALSE;
	}

	sscanf(argv[0], "%x", &id);

	isp_api_set_sensor_wakeup(id);

	return TRUE;
}

static SXCMD_BEGIN(isp_cmd_tbl, "isp")
SXCMD_ITEM("info",              isp_sxcmd_dump_info,               "dump isp info")
SXCMD_ITEM("ca %d",             isp_sxcmd_dump_ca,                 "dump ca, param1 is isp_id(0~1)")
SXCMD_ITEM("la %d",             isp_sxcmd_dump_la,                 "dump la, param1 is isp_id(0~1)")
SXCMD_ITEM("va %d",             isp_sxcmd_dump_va,                 "dump va, param1 is isp_id(0~1)")
SXCMD_ITEM("dbg %d %d",         isp_sxcmd_set_dbg,                 "set isp dbg level, param1 is isp_id(0~1, >1 means apply to each id), param2 is dbg_lv.")
SXCMD_ITEM("bypass %d %d",      isp_sxcmd_set_bypass,              "set isp bypass eng, param1 is isp_id(0~1, 8 means apply to each id), param2 is bypass_eng.")
SXCMD_ITEM("dir %d %d %d",      isp_sxcmd_set_sensor_dir,          "set sensor direction, param1 is isp_id(0~1), param2 is mirror, param3 is flip.")
SXCMD_ITEM("sleep %d",          isp_sxcmd_set_sensor_sleep,        "set sensor sleep, param1 is isp_id(0~1).")
SXCMD_ITEM("wakeup %d",         isp_sxcmd_set_sensor_wakeup,       "set sensor wakeup, param1 is isp_id(0~1).")
SXCMD_END()

int isp_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(isp_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "isp");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", isp_cmd_tbl[loop].p_name, isp_cmd_tbl[loop].p_desc);
	}

	dump("---------------------------------------------------------------------\r\n");
	dump(" dbg_lv = \r\n");
	dump(" | 0x%8X = AE STATUS    | 0x%8X = TOTAL GAIN   | 0x%8X = D GAIN       | \r\n", ISP_DBG_SYNC_AE_STATUS, ISP_DBG_SYNC_TOTAL_GAIN, ISP_DBG_SYNC_D_GAIN);
	dump(" | 0x%8X = LV           | 0x%8X = LV_BASE      | \r\n", ISP_DBG_SYNC_LV, ISP_DBG_SYNC_LV_BASE);
	dump(" | 0x%8X = C GAIN       | 0x%8X = FINAL C GAIN | 0x%8X = CT           | 0x%8X = CAPTURE      | \r\n", ISP_DBG_SYNC_C_GAIN, ISP_DBG_SYNC_FINAL_C_GAIN, ISP_DBG_SYNC_CT, ISP_DBG_SYNC_CAPTURE);
	dump(" | 0x%8X = SENSOR EXPT  | 0x%8X = SENSOR GAIN  | 0x%8X = SENSOR REG.  | \r\n", ISP_DBG_SENSOR_EXPT, ISP_DBG_SENSOR_GAIN, ISP_DBG_SENSOR_REG);
	dump(" | 0x%8X = SIE CB       | 0x%8X = IPP CB       | 0x%8X = ENC CB       | \r\n", ISP_DBG_SIE_CB, ISP_DBG_IPP_CB, ISP_DBG_ENC_CB);
	dump(" | 0x%8X = SIE SET      | 0x%8X = IPP SET      | 0x%8X = ENC SET      | \r\n", ISP_DBG_SIE_SET, ISP_DBG_IPP_SET, ISP_DBG_ENC_SET);
	dump(" | 0x%8X = ERR MSG      | 0x%8X = WRN MSG      | \r\n", ISP_DBG_ERR_MSG, ISP_DBG_WRN_MSG);
	dump("---------------------------------------------------------------------\r\n");
	dump(" bypass_eng = \r\n");
	dump(" | 0x%8X = SIE_ROI      | 0x%8X = SIE_PARAM    | 0x%8X = IFE_PARAM    | 0x%8X = DCE_PARAM    | \r\n", ISP_BYPASS_SIE_ROI, ISP_BYPASS_SIE_PARAM, ISP_BYPASS_IFE_PARAM, ISP_BYPASS_DCE_PARAM);
	dump(" | 0x%8X = IPE_PARAM    | 0x%8X = IFE2_PARAM   | 0x%8X = IME_PARAM    | 0x%8X = IFE_VIG_CENT | \r\n", ISP_BYPASS_IPE_PARAM, ISP_BYPASS_IFE2_PARAM, ISP_BYPASS_IME_PARAM, ISP_BYPASS_IFE_VIG_CENT);
	dump(" | 0x%8X = DCE_DC_CENT  | 0x%8X = IFE2_FILT_T  | 0x%8X = IME_LCA_SIZE | 0x%8X = IPE_VA_WIN_S | \r\n", ISP_BYPASS_DCE_DC_CENT, ISP_BYPASS_IFE2_FILT_TIME, ISP_BYPASS_IME_LCA_SIZE, ISP_BYPASS_IPE_VA_WIN_SIZE);
	dump(" | 0x%8X = ENC_NR_PARAM | 0x%8X = ENC_SP_PARAM | 0x%8X = SEN_REG      | 0x%8X = SEN_EXPT     | \r\n", ISP_BYPASS_ENC_3DNR_PARAM, ISP_BYPASS_ENC_3DNR_PARAM, ISP_BYPASS_SEN_REG, ISP_BYPASS_SEN_EXPT);
	dump(" | 0x%8X = SEN_GAIN     | 0x%8X = SEN_DIR      | 0x%8X = SEN_SLEEP    | 0x%8X = SEN_WAKEUP   | \r\n", ISP_BYPASS_SEN_GAIN, ISP_BYPASS_SEN_DIR, ISP_BYPASS_SEN_SLEEP, ISP_BYPASS_SEN_WAKEUP);
	dump("---------------------------------------------------------------------\r\n");
	dump("Ex: 'isp dbg 8 0x0000' \r\n");
	dump("Ex: 'isp dbg 8 0x0003' \r\n");
	dump("Ex: 'isp dbg 8 0x0300' \r\n");
	dump("Ex: 'isp dbg 8 0x8000' \r\n");
	dump("Ex: 'isp bypass 8 0x0000' \r\n");
	dump("Ex: 'isp bypass 8 0x07FC' \r\n");
	dump("Ex: 'isp dir 0 0 0'    \r\n");
	dump("Ex: 'isp sleep 0'      \r\n");
	dump("Ex: 'isp wakeup 0'     \r\n");

	return 0;
}

MAINFUNC_ENTRY(isp, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(isp_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		isp_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], isp_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = isp_cmd_tbl[loop].p_func(argc - 2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		isp_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

