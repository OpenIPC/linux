#if defined (__LINUX)
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "kwrap/stdio.h"
#include "kdrv_tge_int_drv.h"
#elif defined (__FREERTOS)
#include "stdio.h"
#include "string.h"
#endif

#include "kdrv_type.h"
#include "kwrap/type.h"
#include "kwrap/sxcmd.h"
#include "kwrap/cmdsys.h"
#include "kwrap/file.h"
#include "kwrap/cpu.h"
#include "kwrap/util.h"
#include "kwrap/mem.h"
#include "kwrap/perf.h"
//#include "kwrap/error_no.h"
#include "comm/hwclock.h"
#include "tge_platform.h"
#include "tge_lib.h"
#include "kdrv_tge_int_api.h"
#include "kdrv_tge_int_dbg.h"
#include "kdrv_videocapture/kdrv_tge.h"

#define KDRV_TGE_TEST_CMD 0

#define KDRV_TGE_ID(ch) KDRV_DEV_ID(KDRV_CHIP0, KDRV_VDOCAP_TGE_ENGINE0, (ch))
#define KDRV_TGE_CH_BIT(ch) (1 << (ch))

#if KDRV_TGE_TEST_CMD || !defined(CONFIG_NVT_SMALL_HDAL)
ER nvt_kdrv_tge_open(void)
{
	KDRV_TGE_OPENCFG kdrv_tge_open_obj = {0};

	kdrv_tge_open_obj.tge_clock_sel = KDRV_TGE_CLK_MCLK1;
	kdrv_tge_open_obj.tge_clock_sel2 = KDRV_TGE_CLK_MCLK2;

	if (kdrv_tge_set(KDRV_TGE_ID(0), KDRV_TGE_PARAM_IPL_OPENCFG, (void *)(&kdrv_tge_open_obj))) {
		goto fail;
	}

	if (kdrv_tge_open(KDRV_CHIP0, KDRV_VDOCAP_TGE_ENGINE0)) {
		goto fail;
	}

	return E_OK;

fail:
	return E_SYS;
}

ER nvt_kdrv_tge_set_vdhd_param(UINT32 ch, UINT32 vp, UINT32 va, UINT32 vf,
	UINT32 hp, UINT32 ha, UINT32 hc, UINT32 vph, UINT32 hph)
{
	KDRV_TGE_VDHD_INFO vdhd_info = {0};

	vdhd_info.mode = KDRV_MODE_MASTER;
	vdhd_info.vd_period = vp;
	vdhd_info.vd_assert = va;
	vdhd_info.vd_frontblnk = vf;
	vdhd_info.hd_period = hp;
	vdhd_info.hd_assert = ha;
	vdhd_info.hd_cnt = hc;
	vdhd_info.vd_phase = vph;
	vdhd_info.hd_phase = hph;

	return kdrv_tge_set(KDRV_TGE_ID(ch), KDRV_TGE_PARAM_IPL_VDHD, (void *)(&vdhd_info));
}

ER nvt_kdrv_tge_get_vdhd_param(UINT32 ch)
{
	KDRV_TGE_VDHD_INFO vdhd_info = {0};
	UINT32 _ch;

	if (kdrv_tge_get(KDRV_TGE_ID(ch), KDRV_TGE_PARAM_IPL_VDHD, (void *)(&vdhd_info))) {
		return E_SYS;
	}

	for (_ch = 0; _ch < KDRV_TGE_VDHD_CH_MAX; _ch++) {
		if ((1 << _ch) & ch) {
			break;
		}
	}

	DBG_DUMP("CH%d info: vd assert/frntbl/cnt/period = %d/%d/%d/%d\r\n", _ch, vdhd_info.vd_assert, vdhd_info.vd_frontblnk, vdhd_info.hd_cnt, vdhd_info.vd_period);
	DBG_DUMP("CH%d info: hd assert/period = %d/%d\r\n", _ch, vdhd_info.hd_assert, vdhd_info.hd_period);

	return E_OK;
}

ER nvt_kdrv_tge_set_bp_line(UINT32 ch, UINT32 bl)
{
	KDRV_TGE_BP_INFO bp_info = {0};

	bp_info.bp_line = bl;

	return kdrv_tge_set(KDRV_TGE_ID(ch), KDRV_TGE_PARAM_IPL_VD_BP, (void *)(&bp_info));
}

ER nvt_kdrv_tge_get_bp_line(UINT32 ch)
{
	KDRV_TGE_BP_INFO bp_info = {0};
	UINT32 _ch;

	if (kdrv_tge_get(KDRV_TGE_ID(ch), KDRV_TGE_PARAM_IPL_VD_BP, (void *)(&bp_info))) {
		return E_SYS;
	}

	for (_ch = 0; _ch < KDRV_TGE_VDHD_CH_MAX; _ch++) {
		if ((1 << _ch) & ch) {
			break;
		}
	}

	DBG_DUMP("CH%d bp = %d\r\n", _ch, bp_info.bp_line);

	return E_OK;
}

ER nvt_kdrv_tge_trig_vd_hd(UINT32 ch, UINT32 wait_event)
{
	KDRV_TGE_TRIG_INFO trig_info = {0};

	trig_info.trig_type = KDRV_TGE_TRIG_VDHD;
	trig_info.ch_enable = TRUE;
	trig_info.wait_end_enable = TRUE;
	trig_info.wait_event = wait_event;

	return kdrv_tge_trigger(KDRV_TGE_ID(ch), NULL, NULL, (void *)&trig_info);
}

ER nvt_kdrv_tge_trig_stop(void)
{
	KDRV_TGE_TRIG_INFO trig_info = {0};

	trig_info.trig_type = KDRV_TGE_TRIG_STOP;
	return kdrv_tge_trigger(KDRV_TGE_ID(0), NULL, NULL, (void *)&trig_info);
}
#endif

#if 0
#endif

BOOL nvt_kdrv_tge_cmd_dump_info(unsigned char argc, char **pargv)
{
	kdrv_tge_dump_info();

	return TRUE;
}

BOOL nvt_kdrv_tge_cmd_dbg_lv(unsigned char argc, char **pargv)
{
	UINT32 arg;

	if (strcmp(pargv[0], "err") == 0 || (sscanf(pargv[0], "%u", (unsigned int *)&arg) == 1 && arg == 1)) {
		tge_debug_level = 1;
	} else if (strcmp(pargv[0], "wrn") == 0 || (sscanf(pargv[0], "%u", (unsigned int *)&arg) == 1 && arg == 2)) {
		tge_debug_level = 2;
	} else if (strcmp(pargv[0], "func") == 0 || (sscanf(pargv[0], "%u", (unsigned int *)&arg) == 1 && arg == 4)) {
		tge_debug_level = 4;
	} else if (strcmp(pargv[0], "ind") == 0 || (sscanf(pargv[0], "%u", (unsigned int *)&arg) == 1 && arg == 5)) {
		tge_debug_level = 5;
	} else {
		DBG_ERR("dbglv err\r\n");
	}

	return TRUE;
}

#if !defined(CONFIG_NVT_SMALL_HDAL)
static BOOL nvt_kdrv_tge_cmd_set_help(unsigned char argc, char **pargv)
{
	DBG_DUMP("[SET]\r\n");
	DBG_DUMP("vd CH(u) VP(u) VA(u) VF(u) HP(u) HA(u) HC(u) VPH(u) HPH(u). Set VDHD param.\r\n");
	DBG_DUMP("  VP: vd period, VA: vd assert, VF: vd front blank,\r\n");
	DBG_DUMP("  HP: hd period, HA: hd assert, HC: hd count,\r\n");
	DBG_DUMP("  VPH: vd phase, HPH: hd phase\r\n");
	DBG_DUMP("bp CH(u) BL(u). Set break point line. BL: break point line number\r\n");

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_set_vdhd_param(unsigned char argc, char **pargv)
{
	UINT32 ch, vp, va, vf, hp, ha, hc, vph, hph;

	if (argc < 9) {
		DBG_ERR("err input num %u\r\n", argc);
		return FALSE;
	}

	if (sscanf(pargv[0], "%u", (unsigned int *)&ch) != 1 ||
		sscanf(pargv[1], "%u", (unsigned int *)&vp) != 1 ||
		sscanf(pargv[2], "%u", (unsigned int *)&va) != 1 ||
		sscanf(pargv[3], "%u", (unsigned int *)&vf) != 1 ||
		sscanf(pargv[4], "%u", (unsigned int *)&hp) != 1 ||
		sscanf(pargv[5], "%u", (unsigned int *)&ha) != 1 ||
		sscanf(pargv[6], "%u", (unsigned int *)&hc) != 1 ||
		sscanf(pargv[7], "%u", (unsigned int *)&vph) != 1 ||
		sscanf(pargv[8], "%u", (unsigned int *)&hph) != 1) {
		DBG_ERR("err input\r\n");
		return FALSE;
	}

	nvt_kdrv_tge_set_vdhd_param(KDRV_TGE_CH_BIT(ch), vp, va, vf, hp, ha, hc, vph, hph);

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_set_bp_line(unsigned char argc, char **pargv)
{
	UINT32 ch, bl;

	if (argc < 2) {
		DBG_ERR("err input num %u\r\n", argc);
		return FALSE;
	}

	if (sscanf(pargv[0], "%u", (unsigned int *)&ch) != 1 ||
		sscanf(pargv[1], "%u", (unsigned int *)&bl) != 1) {
		DBG_ERR("err input\r\n");
		return FALSE;
	}

	nvt_kdrv_tge_set_bp_line(KDRV_TGE_CH_BIT(ch), bl);

	return TRUE;
}

BOOL nvt_kdrv_tge_cmd_set(unsigned char argc, char **pargv)
{
	if (argc == 0 || strcmp(pargv[0], "?") == 0 || strcmp(pargv[0], "help") == 0) {
		nvt_kdrv_tge_cmd_set_help(argc, pargv);

	} else if (strcmp(pargv[0], "vd") == 0) {
		return nvt_kdrv_tge_cmd_set_vdhd_param(argc-1, &pargv[1]);

	} else if (strcmp(pargv[0], "bp") == 0) {
		return nvt_kdrv_tge_cmd_set_bp_line(argc-1, &pargv[1]);
	}

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_get_help(unsigned char argc, char **pargv)
{
	DBG_DUMP("[GET]\r\n");
	DBG_DUMP("vd CH(u). Get VDHD param.\r\n");
	DBG_DUMP("bp CH(u). Get break point line.\r\n");

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_get_vdhd_param(unsigned char argc, char **pargv)
{
	UINT32 ch;

	if (argc < 1) {
		DBG_ERR("err input num %u\r\n", argc);
		return FALSE;
	}

	if (sscanf(pargv[0], "%u", (unsigned int *)&ch) != 1) {
		DBG_ERR("err input\r\n");
		return FALSE;
	}

	nvt_kdrv_tge_get_vdhd_param(KDRV_TGE_CH_BIT(ch));

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_get_bp_line(unsigned char argc, char **pargv)
{
	UINT32 ch;

	if (argc < 1) {
		DBG_ERR("err input num %u\r\n", argc);
		return FALSE;
	}

	if (sscanf(pargv[0], "%u", (unsigned int *)&ch) != 1) {
		DBG_ERR("err input\r\n");
		return FALSE;
	}

	nvt_kdrv_tge_get_bp_line(KDRV_TGE_CH_BIT(ch));

	return TRUE;
}

BOOL nvt_kdrv_tge_cmd_get(unsigned char argc, char **pargv)
{
	if (argc == 0 || strcmp(pargv[0], "?") == 0 || strcmp(pargv[0], "help")) {
		nvt_kdrv_tge_cmd_get_help(argc, pargv);

	} else if (strcmp(pargv[0], "vd") == 0) {
		return nvt_kdrv_tge_cmd_get_vdhd_param(argc-1, &pargv[1]);

	} else if (strcmp(pargv[0], "bp") == 0) {
		return nvt_kdrv_tge_cmd_get_bp_line(argc-1, &pargv[1]);
	}

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_trig_help(unsigned char argc, char **pargv)
{
	DBG_DUMP("[TRIG]\r\n");
	DBG_DUMP("vd CH(u) WAT_EVT(u). Trigger VDHD. WAT_EVT: wait event\r\n");
	DBG_DUMP("stop. Stop engine.\r\n");

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_trig_vd_hd(unsigned char argc, char **pargv)
{
	UINT32 ch, wait_event;

	if (argc < 2) {
		DBG_ERR("err input num %u\r\n", argc);
		return FALSE;
	}

	if (sscanf(pargv[0], "%u", (unsigned int *)&ch) != 1 ||
		sscanf(pargv[1], "%u", (unsigned int *)&wait_event) != 1) {
		DBG_ERR("err input\r\n");
		return FALSE;
	}

	nvt_kdrv_tge_trig_vd_hd(KDRV_TGE_CH_BIT(ch), wait_event);

	return TRUE;
}

static BOOL nvt_kdrv_tge_cmd_trig_stop(unsigned char argc, char **pargv)
{
	nvt_kdrv_tge_trig_stop();

	return TRUE;
}

BOOL nvt_kdrv_tge_cmd_trig(unsigned char argc, char **pargv)
{
	if (argc == 0 || strcmp(pargv[0], "?") == 0 || strcmp(pargv[0], "help")) {
		nvt_kdrv_tge_cmd_trig_help(argc, pargv);

	} else if (strcmp(pargv[0], "vd") == 0) {
		return nvt_kdrv_tge_cmd_trig_vd_hd(argc-1, &pargv[1]);

	} else if (strcmp(pargv[0], "stop") == 0) {
		return nvt_kdrv_tge_cmd_trig_stop(argc-1, &pargv[1]);
	}

	return TRUE;
}
#endif

#if (KDRV_TGE_TEST_CMD != 0)
BOOL nvt_kdrv_tge_cmd_test(unsigned char argc, char **pargv)
{
	UINT32 vd_period = 1000 * 100;
	UINT32 vd_assert = 2000 * 7 / 2;
	UINT32 hd_period = 1000;
	UINT32 hd_assert = 700;
	UINT32 vd2_period = 2000 * 100;
	UINT32 vd2_assert = 2000 * 7 / 2;
	UINT32 hd2_period = 2000;
	UINT32 hd2_assert = 500;
	UINT32 vd2_pb_line = 4000;

	if (nvt_kdrv_tge_open()) {
		goto fail;
	}

	if (nvt_kdrv_tge_set_vdhd_param(KDRV_TGE_VDHD_CH1, vd_period, vd_assert, 0,
		hd_period, hd_assert, 0, KDRV_TGE_PHASE_RISING, KDRV_TGE_PHASE_RISING)) {
		goto fail;
	}

	if (nvt_kdrv_tge_set_vdhd_param(KDRV_TGE_VDHD_CH2, vd2_period, vd2_assert, 0,
		hd2_period, hd2_assert, 0, KDRV_TGE_PHASE_RISING, KDRV_TGE_PHASE_RISING)) {
		goto fail;
	}

	if (nvt_kdrv_tge_set_bp_line(KDRV_TGE_VDHD_CH2, vd2_pb_line)) {
		goto fail;
	}

	if (nvt_kdrv_tge_trig_vd_hd(KDRV_TGE_VDHD_CH1, TGE_WAIT_VD)) {
		goto fail;
	}
	DBG_DUMP("TGE trigger1\r\n");

	if (nvt_kdrv_tge_trig_vd_hd(KDRV_TGE_VDHD_CH2, TGE_WAIT_VD)) {
		goto fail;
	}
	DBG_DUMP("TGE trigger2\r\n");

	if (nvt_kdrv_tge_trig_vd_hd(KDRV_TGE_VDHD_CH1 | KDRV_TGE_VDHD_CH2,
		TGE_WAIT_VD | TGE_WAIT_VD2 | TGE_WAIT_VD2_BP1)) {
		goto fail;
	}
	DBG_DUMP("TGE trigger3\r\n");

	if (nvt_kdrv_tge_get_vdhd_param(KDRV_TGE_VDHD_CH2)) {
		goto fail;
	}

	if (nvt_kdrv_tge_get_bp_line(KDRV_TGE_VDHD_CH2)) {
		goto fail;
	}

	if (nvt_kdrv_tge_trig_stop()) {
		goto fail;
	}

	if (nvt_kdrv_tge_trig_vd_hd(KDRV_TGE_VDHD_CH1, TGE_WAIT_VD)) {
		goto fail;
	}
	DBG_DUMP("TGE trigger\r\n");

	return TRUE;

fail:
	return FALSE;
}
#endif

static SXCMD_BEGIN(kdrv_tge_cmd_tbl, "kdrv_tge")
SXCMD_ITEM("dumpinfo", 		nvt_kdrv_tge_cmd_dump_info, 	"Show kdrv_tge information")
SXCMD_ITEM("dbglv LV(u)", 	nvt_kdrv_tge_cmd_dbg_lv, 		"Change debug level. LV: 1(err), 2(wrn), 4(func), 5(ind)")
#if !defined(CONFIG_NVT_SMALL_HDAL)
SXCMD_ITEM("set [CMD]", 	nvt_kdrv_tge_cmd_set, 			"Set command. CMD: ?/help/(NULL) for more information")
SXCMD_ITEM("get [CMD]", 	nvt_kdrv_tge_cmd_get, 			"Get command. CMD: ?/help/(NULL) for more information")
SXCMD_ITEM("trig [CMD]", 	nvt_kdrv_tge_cmd_trig, 			"Trigger command. CMD: ?/(NULL) for more information")
#endif

#if (KDRV_TGE_TEST_CMD != 0)
SXCMD_ITEM("test", 			nvt_kdrv_tge_cmd_test, 			"Test command")
#endif
SXCMD_END();

void nvt_kdrv_tge_cmd_help(void)
{
	UINT32 i, cmd_num = SXCMD_NUM(kdrv_tge_cmd_tbl);

	for (i = 1; i <= cmd_num; i++) {
		DBG_DUMP("CMD: %s\r\n", kdrv_tge_cmd_tbl[i].p_name);
		DBG_DUMP("DESC: %s\r\n", kdrv_tge_cmd_tbl[i].p_desc);
	}
}

#if defined(__LINUX)
int kdrv_tge_cmd_execute(unsigned char argc, char **argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_tge_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 1 || strcmp(argv[0], "?") == 0 || strcmp(argv[0], "help") == 0) {
		nvt_kdrv_tge_cmd_help();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[0], kdrv_tge_cmd_tbl[loop].p_name, strlen(argv[0])) == 0) {
			ret = kdrv_tge_cmd_tbl[loop].p_func(argc-1, &argv[1]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		nvt_kdrv_tge_cmd_help();
		return -1;
	}

	return 0;
}
#else
MAINFUNC_ENTRY(kdrv_tge, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(kdrv_tge_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2 || strcmp(argv[1], "?") == 0 || strcmp(argv[1], "help") == 0) {
		nvt_kdrv_tge_cmd_help();
		return 0;
	}

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], kdrv_tge_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = kdrv_tge_cmd_tbl[loop].p_func(argc-1, &argv[2]);
			return ret;
		}
	}

	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		nvt_kdrv_tge_cmd_help();
		return -1;
	}

	return 0;
}
#endif
