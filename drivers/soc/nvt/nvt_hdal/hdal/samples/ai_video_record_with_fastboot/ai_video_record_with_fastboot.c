/**
	@brief Sample code of video record with mode and pxlfmt.\n

	@file ai_video_record_with_fastboot.c

	@author Boyan Huang

	@ingroup mhdal

	@note This file is modified from video_record_with_fastboot.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "mem_hotplug.h"
#if defined(__LINUX)
#include "vendor_isp.h"
#endif

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_video_record_with_fastboot, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif
#define VOUT_DISP       DISABLE
#define USE_AI       	ENABLE
#define USE_AI_MODEL    1 //0: inception_v2, 1:pdcnn
#define DEBUG_MENU 		1
#define DUMP_FBOOT_RSLT 0

#if (USE_AI==ENABLE)
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"

#define NET_PATH_ID		UINT32
#define VENDOR_AI_CFG  	0x000f0000  //vendor ai config
#endif

#if (USE_AI_MODEL == 0)
#include "vendor_ai_cpu_postproc.h" //for dump result of inception_v2 
#endif
#if (USE_AI_MODEL == 1)
#include "pdcnn_user_plugin.h" //for install user plugin, and dump result of pdcnn
#endif

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////
#define ISP_RELOAD_CFG 0
#define ISP_RELOAD_DTSI 1
#define ISP_RELOAD ISP_RELOAD_CFG

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)

#if (USE_AI==ENABLE)
#define AI_RGB_BUFSIZE(w, h)		(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(HD_VIDEO_PXLFMT_RGB888_PLANAR) / 8) * (h))
#endif

///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_RAW12_SHDR2
#define CA_WIN_NUM_W    32
#define CA_WIN_NUM_H    32
#define LA_WIN_NUM_W    32
#define LA_WIN_NUM_H    32
#define VA_WIN_NUM_W    16
#define VA_WIN_NUM_H    16
#define YOUT_WIN_NUM_W  128
#define YOUT_WIN_NUM_H  128
#define ETH_8BIT_SEL    0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL     1 //0: full, 1: subsample 1/2

#define AUTO_RESOLUTION_SET 1 //1: use linux environment to decide RESOLUTION_SET
static UINT32 RESOLUTION_SET = 0; //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR) 4: (PATTERN_GEN) 5: 2M (F37) 6: 2M (F35)
static char * SENSOR_NAME_TBL[] = {"sen_imx290", "sen_os05a10", "sen_os02k10", "sen_ar0237ir", "sen_off", "sen_f37", "sen_f35"};
static UINT32 VDO_SIZE_W_TBL[] = {1920, 2592, 1920, 1920, 1920, 1920, 1920};
static UINT32 VDO_SIZE_H_TBL[] = {1080, 1944, 1080, 1080, 1080, 1080, 1080};

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_SHDR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_DEFOG

static UINT32 g_shdr = 0; //fixed
static UINT32 g_capbind = 0;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
static UINT32 g_capfmt = 0; //0:RAW, 1:RAW-compress
static UINT32 g_prcbind = 0;  //0:D2D, 1:lowlatency, 2: one-buf, 0xff: no-bind
static UINT32 g_prcfmt = 0; //0:YUV, 1:YUV-compress
static UINT32 g_acapen = 1; //audio cap enable
static UINT32 g_snapshoten = 0;
static UINT32 g_suben = 0;

///////////////////////////////////////////////////////////////////////////////
static UINT32 VDO_SIZE_W;
static UINT32 VDO_SIZE_H;

#if (USE_AI==ENABLE)
#if (USE_AI_MODEL == 0)
#define NET_VDO_SIZE_W	1920 //input dim of net
#define NET_VDO_SIZE_H	1080 //input dim of net
#endif
#if (USE_AI_MODEL == 1)
#define NET_VDO_SIZE_W	PDCNN_IN_SIZE_W //input dim of net
#define NET_VDO_SIZE_H	PDCNN_IN_SIZE_H //input dim of net
#endif
static UINT32 AI_BUF_SIZE = 0;
#endif

#define MODEL_HEADER_LEN       100 * 1024
#define FASTBOOT_DUMP_MODEL    1

#if FASTBOOT_DUMP_MODEL
extern HD_RESULT _vendor_ai_net_set_pd_plugin(void* p_param);
extern HD_RESULT _vendor_ai_net_get_fastboot_bufsize(UINT32 proc_id, UINT32 model_addr, UINT32 *p_parm_sz, UINT32 *p_model_sz, UINT32 *p_buf_sz);
extern INT vendor_ais_is_fastboot(void);
#endif

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_capbind == 0) || (g_capbind == 0xff)) {
		//normal
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		if (g_capfmt == HD_VIDEO_PXLFMT_NRX12)
			mem_cfg.pool_info[0].blk_size += VDO_NRX_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H);
		else
			mem_cfg.pool_info[0].blk_size += VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, g_capfmt);
		mem_cfg.pool_info[0].blk_cnt = 2;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	} else if (g_capbind == 1) {
		//direct ... NOT require raw
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		mem_cfg.pool_info[0].blk_cnt = 2;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	} else if (g_capbind == 2) {
		//one-buf
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
		mem_cfg.pool_info[0].blk_cnt = 1;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	} else {
		//not support
		mem_cfg.pool_info[0].blk_size = 0;
		mem_cfg.pool_info[0].blk_cnt = 0;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	}
	if (g_shdr == 1) {
		mem_cfg.pool_info[0].blk_cnt *= 2;
	}
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_prcbind == 0) || (g_prcbind == 0xff)) {
		//normal
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 3; //venc release blk is slow, depend on its waiting list
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	} else if (g_prcbind == 1) {
		//low-latency
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 2;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	} else if (g_prcbind == 2) {
		//one-buf
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 1;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	} else {
		//not support
		mem_cfg.pool_info[1].blk_size = 0;
		mem_cfg.pool_info[1].blk_cnt = 0;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	}

	// config common pool (sub)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

#if (VOUT_DISP==ENABLE) /*always alloc,avoid change fastboot-hdal-mem.dtsi*/
	// config vout pool
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[3].blk_cnt = 3;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;
#endif
#if (USE_AI==ENABLE)
	// config ai "input buffer" pool
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_prcbind == 0) || (g_prcbind == 0xff)) {
		//normal
		mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(NET_VDO_SIZE_W, NET_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[3].blk_cnt = 3; //venc release blk is slow, depend on its waiting list
		mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	} else if (g_prcbind == 1) {
		//low-latency
		mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(NET_VDO_SIZE_W, NET_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[3].blk_cnt = 2;
		mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	} else if (g_prcbind == 2) {
		//one-buf
		mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(NET_VDO_SIZE_W, NET_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[3].blk_cnt = 1;
		mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	} else {
		//not support
		mem_cfg.pool_info[3].blk_size = 0;
		mem_cfg.pool_info[3].blk_cnt = 0;
		mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	}
	// config ai "model bin" pool
	mem_cfg.pool_info[4].type = HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[4].blk_size = AI_BUF_SIZE;
	mem_cfg.pool_info[4].blk_cnt = 1;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;
#endif

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	switch(RESOLUTION_SET) {
	case 0:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
		printf("Using nvt_sen_imx290\n");
		break;
	case 1:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
		printf("Using nvt_sen_os05a10\n");
		break;
	case 2:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
		printf("Using nvt_sen_os02k10\n");
		break;
	case 3:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
		printf("Using nvt_sen_ar0237ir\n");
		break;
	case 4:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
		printf("Using sie pattern gen\n");
		break;
	case 5:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f37");
		printf("Using nvt_sen_f37\n");
		break;
	case 6:
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_f35");
		printf("Using nvt_sen_f35\n");
		break;
	default:
		printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
		return HD_ERR_NOT_SUPPORT;
	}

	if(RESOLUTION_SET == 3) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
		printf("Parallel interface\n");
	}
	if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2 || RESOLUTION_SET == 5 || RESOLUTION_SET == 6) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
		printf("MIPI interface\n");
	}
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		printf("Using g_shdr_mode\n");
	} else {
		switch(RESOLUTION_SET) {
		case 0:
			printf("Using imx290\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
			break;
		case 1:
			printf("Using OS052A\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
			break;
		case 2:
			printf("Using OS02K10\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
			break;
		case 3:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		case 4:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		case 5:
			printf("Using F37\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
			break;
		case 6:
			printf("Using F35\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
			break;
		default:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		}
	}
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	} else {
		switch(RESOLUTION_SET) {
		case 0:
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
			break;
		case 1:
			printf("Using OS052A or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
			break;
		case 2:
			printf("Using OS02K10 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
			break;
		case 3:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		case 4:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		case 5:
			printf("Using F37 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
			break;
		case 6:
			printf("Using F35 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
			break;
		default:
			printf("unhandled RESOLUTION_SET:%d\n", RESOLUTION_SET);
			return HD_ERR_NOT_SUPPORT;
		}
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;
	if (g_shdr == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);
	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, UINT32 path)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		switch(RESOLUTION_SET) {
		case 4:
			// pattern gen
			video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
			video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
			video_in_param.dim.w = p_dim->w;
			video_in_param.dim.h = p_dim->h;
			break;
		default:
			video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
			video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
			video_in_param.dim.w = p_dim->w;
			video_in_param.dim.h = p_dim->h;
			video_in_param.pxlfmt = SEN_OUT_FMT;
			if ((path == 0) && (g_shdr == 1)) {
				video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
			} else {
				video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
			}
			break;
		}
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = 1920/2;
		video_crop_param.win.rect.h= 1080/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		// NOTE: only SHDR with path 1
		if ((path == 0) && (g_shdr == 1)) {
			video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
		    video_out_param.pxlfmt = g_capfmt;
		}
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.depth = 0;
		if (g_capbind == 0xff) //no-bind mode
			video_out_param.depth = 1;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (g_capbind == 1) //direct mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		if (g_capbind == 2) //one-buf mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_ONEBUF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_VIDEOPROC_LL_CONFIG video_ll_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0;
		} else {
			video_cfg_param.isp_id = 1;
		}

		video_cfg_param.ctrl_max.func = HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA;
        if (g_shdr == 1) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		}
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
        // NOTE: only SHDR with path 1
		if (g_shdr == 1) {
			video_cfg_param.in_max.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
		    video_cfg_param.in_max.pxlfmt = g_capfmt;
        }
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
		if (g_capbind == 1)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		if (g_capbind == 2)
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_ONEBUF;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}

	video_ctrl_param.func = HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_3DNR_STA;
	video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_0;
    if (g_shdr == 1) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	}
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	video_ll_param.delay_trig_lowlatency = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_LL_CONFIG, &video_ll_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, UINT32 depth)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = 0;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (g_prcbind == 1)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_LOWLATENCY; //enable low-latency
		if (g_prcbind == 2)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#if (VOUT_DISP==ENABLE)
static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	//printf("out_type=%d\r\n", out_type);

	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	//printf("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#endif

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = 3000;
		video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		video_path_config.max_mem.ltr        = TRUE;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = 0;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}

		video_func_config.in_func = 0;
		if (g_prcbind == 1)
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_LOWLATENCY; //enable low-latency
		if (g_prcbind == 2)
			video_func_config.in_func |= HD_VIDEOENC_INFUNC_ONEBUF;

		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &video_func_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, UINT32 enc_type, UINT32 bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

	if (p_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}

		printf("enc_type=%d\r\n", enc_type);

		if (enc_type == 0) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			if (video_in_param.dim.w > 1920 && video_in_param.dim.h > 1080) {
				video_out_param.h26x.level_idc	   = HD_H265E_LEVEL_5;
			} else {
				video_out_param.h26x.level_idc	   = HD_H265E_LEVEL_4_1;
			}
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;

			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
		} else if (enc_type == 1) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			if (video_in_param.dim.w > 1920 && video_in_param.dim.h > 1080) {
				video_out_param.h26x.level_idc	   = HD_H264E_LEVEL_5_1;
			} else {
				video_out_param.h26x.level_idc	   = HD_H264E_LEVEL_4_1;
			}
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = 30;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;

			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}

		} else if (enc_type == 2) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

static HD_RESULT set_acap_cfg(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_AUDIOCAP_VOLUME audio_vol_param = {0};
	HD_PATH_ID audio_cap_ctrl = 0;

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture maximum parameters*/
	audio_cfg_param.in_max.sample_rate = sample_rate;
	audio_cfg_param.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.in_max.frame_sample = 1024;
	audio_cfg_param.frame_num_max = 50;
	audio_cfg_param.out_max.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	audio_vol_param.volume = 100;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_vol_param);
	if (ret != HD_OK) {
		return ret;
	}

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_acap_param(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_in_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};

	// set hd_audiocapture input parameters
	audio_cap_in_param.sample_rate = sample_rate;
	audio_cap_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_in_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_cap_in_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_in_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);

	return ret;
}


///////////////////////////////////////////////////////////////////////////////

#if (USE_AI==ENABLE)

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
} MEM_PARM;

#if FASTBOOT_DUMP_MODEL
typedef struct _AI_MEM_CFG {
	MEM_PARM kerl_parm;
	MEM_PARM user_model;
	MEM_PARM io_buff;
} AI_MEM_CFG;
AI_MEM_CFG g_fastboot_mem_cfg = {0};
#endif

/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

static HD_RESULT mem_get(MEM_PARM *mem_parm, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;
	
	blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;

#if FASTBOOT_DUMP_MODEL
	// alloc pa
	g_fastboot_mem_cfg.kerl_parm.pa = pa;
	g_fastboot_mem_cfg.user_model.pa = ALIGN_CEIL_16(g_fastboot_mem_cfg.kerl_parm.pa + g_fastboot_mem_cfg.kerl_parm.size);
	g_fastboot_mem_cfg.io_buff.pa = ALIGN_CEIL_32(g_fastboot_mem_cfg.user_model.pa + g_fastboot_mem_cfg.user_model.size);

	// alloc va
	g_fastboot_mem_cfg.kerl_parm.va = (UINT32)va;
	g_fastboot_mem_cfg.user_model.va = ALIGN_CEIL_16(g_fastboot_mem_cfg.kerl_parm.va + g_fastboot_mem_cfg.kerl_parm.size);
	g_fastboot_mem_cfg.io_buff.va = ALIGN_CEIL_32(g_fastboot_mem_cfg.user_model.va + g_fastboot_mem_cfg.user_model.size);

	printf("=============== [sample] alloc fastboot mem =================\r\n");
	printf("kerl_parm:  pa = 0x%lx, va = 0x%lx, size = 0x%lx\r\n", g_fastboot_mem_cfg.kerl_parm.pa, g_fastboot_mem_cfg.kerl_parm.va, g_fastboot_mem_cfg.kerl_parm.size);
	printf("user_model: pa = 0x%lx, va = 0x%lx, size = 0x%lx\r\n", g_fastboot_mem_cfg.user_model.pa, g_fastboot_mem_cfg.user_model.va, g_fastboot_mem_cfg.user_model.size);
	printf("io_buff:    pa = 0x%lx, va = 0x%lx, size = 0x%lx\r\n", g_fastboot_mem_cfg.io_buff.pa, g_fastboot_mem_cfg.io_buff.va, g_fastboot_mem_cfg.io_buff.size);
#endif

	return HD_OK;
}

static HD_RESULT mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;
	return HD_OK;
}

#if !FASTBOOT_DUMP_MODEL
static HD_RESULT mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;

	//alloc private pool
	ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID0);
	if (ret!= HD_OK) {
		return ret;
	}

	mem_parm->pa   = pa;
	mem_parm->va   = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk  = (UINT32)-1;

	return HD_OK;
}
#endif

static HD_RESULT mem_free(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;
	
	//free private pool
	ret =  hd_common_mem_free(mem_parm->pa, (void *)mem_parm->va);
	if (ret!= HD_OK) {
		return ret;
	}
	
	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;

	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/

typedef struct _NET_PROC_CONFIG {

	CHAR model_filename[256];
	INT32 binsize;
	int job_method;
	int job_wait_ms;
	int buf_method;

	CHAR label_filename[256];

} NET_PROC_CONFIG;

typedef struct _NET_PROC {

	NET_PROC_CONFIG net_cfg;
	MEM_PARM proc_mem;
	UINT32 proc_id;
	
#if (USE_AI_MODEL == 0)
	CHAR out_class_labels[MAX_CLASS_NUM * VENDOR_AIS_LBL_LEN];
#endif
	MEM_PARM rslt_mem;
	MEM_PARM io_mem;
		
} NET_PROC;

static NET_PROC g_net[16] = {0};

#if !FASTBOOT_DUMP_MODEL
static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}
#endif

#if FASTBOOT_DUMP_MODEL
static UINT32 _load_model_header(CHAR *filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;
	//DBG_DUMP("model addr = %08x\r\n", (int)model_addr);

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("load model(%s) fail\r\n", filename);
		return 0;
	}

	file_size = MODEL_HEADER_LEN;

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);

	printf("load model_head(%s) ok\r\n", filename);
	return read_size;
}
#endif

static UINT32 _load_model(CHAR *filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;
	//DBG_DUMP("model addr = %08x\r\n", (int)model_addr);

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("load model(%s) fail\r\n", filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	
	printf("load model(%s) ok\r\n", filename);
	return read_size;
}

#if (USE_AI_MODEL == 0)
static HD_RESULT _load_label(UINT32 addr, UINT32 line_len, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;

	fd = fopen(filename, "r");
	if (!fd) {
		printf("load label(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}

	while (fgets(p_line, line_len, fd) != NULL) {
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	printf("load label(%s) ok\r\n", filename);
	return HD_OK;
}
#endif

static HD_RESULT network_init(void)
{
	HD_RESULT ret = HD_OK;
	int i;
	
	// config extend engine plugin, process scheduler
	{
		UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
		vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
		vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
	}

	// config fastboot proc period
	{
		UINT32 cfg_id = 0Xafb00001; // use a magic number as a temporary id
		UINT32 proc_period = 2; // do proc_net every 2 yuv frames
		vendor_ai_cfg_set(cfg_id, &proc_period);
	}

	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		return ret;
	}

	for (i = 0; i < 16; i++) {
		NET_PROC* p_net = g_net + i;
		p_net->proc_id = i;
	}
	return ret;
}

static HD_RESULT network_uninit(void)
{
	HD_RESULT ret = HD_OK;
	
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}
	
	return ret;
}

UINT32 network_mem_config(NET_PATH_ID net_path, void* p_cfg)
{
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	NET_PROC_CONFIG* p_proc_cfg = (NET_PROC_CONFIG*)p_cfg;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;
	
	memcpy((void*)&p_net->net_cfg, (void*)p_proc_cfg, sizeof(NET_PROC_CONFIG));
	if (strlen(p_net->net_cfg.model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", proc_id);
		return 0;
	}

#if (USE_AI_MODEL == 1)
	// set user postproc
	{
		_vendor_ai_net_set_pd_plugin(get_pdcnn_postproc_plugin());
	}
#endif

#if FASTBOOT_DUMP_MODEL
	{
		CHAR *data;
		UINT32 loadsize;
		UINT32 parm_sz, model_sz, iobuf_sz, total_sz;

		data = (CHAR *)malloc(MODEL_HEADER_LEN);
		if (!data) {
			printf("Error allocation\n");
			return HD_ERR_NG;
		}

		// load model header
		loadsize = _load_model_header(p_net->net_cfg.model_filename, (UINT32)data);
		if (loadsize <= 0) {
			printf("proc_id(%u) input model load fail: %s\r\n", proc_id, p_net->net_cfg.model_filename);
			return HD_ERR_NG;
		}

		_vendor_ai_net_get_fastboot_bufsize(proc_id, (UINT32)data, &parm_sz, &model_sz, &iobuf_sz);
		g_fastboot_mem_cfg.kerl_parm.size = parm_sz;
		g_fastboot_mem_cfg.user_model.size = model_sz;
		g_fastboot_mem_cfg.io_buff.size = iobuf_sz;

		total_sz = parm_sz * 2 + model_sz + iobuf_sz + 32;
		printf("======================== Parse model ========================\n");
		printf("parm_sz(0x%lx), model_sz(0x%lx), iobuf_sz(0x%lx), total_sz(0x%lx)\n", parm_sz, model_sz, iobuf_sz, total_sz);

		p_net->net_cfg.binsize = (INT32)total_sz;
		printf("total binsize = 0x%lx\n", p_net->net_cfg.binsize);
	}
#else
	p_net->net_cfg.binsize = _getsize_model(p_net->net_cfg.model_filename);
#endif

	if (p_net->net_cfg.binsize <= 0) {
		printf("proc_id(%u) input model is not exist?\r\n", proc_id);
		return 0;
	}
	
	printf("proc_id(%u) set net_mem_cfg: model-file(%s), binsize=%d\r\n", 
		proc_id,
		p_net->net_cfg.model_filename,
		p_net->net_cfg.binsize);

	printf("proc_id(%u) set net_mem_cfg: label-file(%s)\r\n", 
		proc_id,
		p_net->net_cfg.label_filename);
	
	// config buf size
	AI_BUF_SIZE = p_net->net_cfg.binsize;
	return AI_BUF_SIZE;
}

static HD_RESULT network_set_config(NET_PATH_ID net_path, NET_PROC_CONFIG* p_proc_cfg)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 binsize = p_net->net_cfg.binsize;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	memcpy((void*)&p_net->net_cfg, (void*)p_proc_cfg, sizeof(NET_PROC_CONFIG));
	p_net->net_cfg.binsize = binsize;
	printf("proc_id(%u) set net_cfg: job-opt=(%u,%d), buf-opt(%u)\r\n", 
		proc_id,
		p_net->net_cfg.job_method,
		(int)p_net->net_cfg.job_wait_ms,
		p_net->net_cfg.buf_method);
	
	// set buf opt
	{
		VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
		cfg_buf_opt.method = p_net->net_cfg.buf_method;
		cfg_buf_opt.ddr_id = DDR_ID0;
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	}

	// set job opt
	{
		VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
		cfg_job_opt.method = p_net->net_cfg.job_method;
		cfg_job_opt.wait_ms = p_net->net_cfg.job_wait_ms;
		cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
	}

#if (USE_AI_MODEL == 1)
	// set user postproc
	{
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_USER_POSTPROC, get_pdcnn_postproc_plugin());
	}
#endif

	return ret;
}

static HD_RESULT network_alloc_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf); // need this will assign p_proc->rsltbuf.size in lib
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

#if FASTBOOT_DUMP_MODEL
	// update to global variable
	p_net->io_mem.pa = g_fastboot_mem_cfg.io_buff.pa;
	p_net->io_mem.va = g_fastboot_mem_cfg.io_buff.va;
	p_net->io_mem.size = g_fastboot_mem_cfg.io_buff.size;
#else
	ret = mem_alloc(&p_net->io_mem, "ai_io_buf", wbuf.size);
	if (ret != HD_OK) {
		printf("proc_id(%lu) alloc ai_io_buf fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}
#endif

	wbuf.pa = p_net->io_mem.pa;
	wbuf.va = p_net->io_mem.va;
	wbuf.size = p_net->io_mem.size;
	printf("network_alloc_io_buf: pa=0x%lx, va=0x%lx, size=0x%lx", wbuf.pa, wbuf.va, wbuf.size);
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %lu\r\n", wbuf.pa, wbuf.va, wbuf.size);

	return ret;
}

static HD_RESULT network_free_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;

	if (p_net->io_mem.pa && p_net->io_mem.va) {
		mem_free(&p_net->io_mem);
	}
	return ret;
}

static HD_RESULT network_open(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;
	UINT32 loadsize = 0;

	if (strlen(p_net->net_cfg.model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", proc_id);
		return 0;
	}

#if (USE_AI_MODEL == 0)
	// load label
	ret = _load_label((UINT32)p_net->out_class_labels, VENDOR_AIS_LBL_LEN, p_net->net_cfg.label_filename);
	if (ret != HD_OK) {
		printf("proc_id(%u) load_label fail=%d\n", proc_id, ret);
		return HD_ERR_FAIL;
	}
#endif

	mem_get(&p_net->proc_mem, p_net->net_cfg.binsize);

	if (vendor_ais_is_fastboot() == DISABLE) {
		//load file
		loadsize = _load_model(p_net->net_cfg.model_filename, p_net->proc_mem.va);
		if (loadsize <= 0) {
			printf("proc_id(%u) input model load fail: %s\r\n", proc_id, p_net->net_cfg.model_filename);
			return 0;
		}

		// set model
		#if FASTBOOT_DUMP_MODEL
		printf("======================== Config model ========================\n");
		printf("pa = 0x%lx, va = 0x%lx, size = 0x%lx (va range: 0x%lx ~ 0x%lx)\n",
			p_net->proc_mem.pa, p_net->proc_mem.va, p_net->proc_mem.size, p_net->proc_mem.va, p_net->proc_mem.va+p_net->proc_mem.size);
		#endif
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&p_net->proc_mem);
	}

	// open
	vendor_ai_net_open(proc_id);

	if ((ret = network_alloc_io_buf(net_path)) != HD_OK)
		return ret;

	return ret;
}

static HD_RESULT network_close(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;

	if ((ret = network_free_io_buf(net_path)) != HD_OK)
		return ret;
	
	// close
	ret = vendor_ai_net_close(proc_id);
	
	mem_rel(&p_net->proc_mem);

	return ret;
}

#if (USE_AI_MODEL == 0)
static HD_RESULT network_dump_out_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt_info = (VENDOR_AI_POSTPROC_RESULT_INFO *)(p_out);
	UINT32 i, j;
	
	printf("proc_id(%u) classification results:\r\n", proc_id);
	if (p_rslt_info) {
		for (i = 0; i < p_rslt_info->result_num; i++) {
			VENDOR_AI_POSTPROC_RESULT *p_rslt = &p_rslt_info->p_result[i];
			for (j = 0; j < NN_POSTPROC_TOP_N; j++) {
				printf("%ld. no=%ld, label=%s, score=%f\r\n",
					j + 1,
					p_rslt->no[j],
					&p_net->out_class_labels[p_rslt->no[j] * VENDOR_AIS_LBL_LEN],
					p_rslt->score[j]);
			}
		}
	}

	return ret;
}
#endif
#if (USE_AI_MODEL == 1)
static HD_RESULT network_dump_out_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;

	FLOAT ratiow = 0.0, ratioh = 0.0;
	ratiow = (FLOAT)VDO_SIZE_W / (FLOAT)NET_VDO_SIZE_W;
	ratioh = (FLOAT)VDO_SIZE_H / (FLOAT)NET_VDO_SIZE_H;
	PD_RESULT_INFO *p_rslt_info = (PD_RESULT_INFO *)p_out;
	PD_RESULT* final_out_results = p_rslt_info->p_result;
	UINT32 num;

	printf("proc_id(%u) detection results:\r\n", proc_id);
	for (num = 0; num < p_rslt_info->result_num; num++) {
		INT32 xmin = (INT32)(final_out_results[num].x1 * ratiow + 0.5);
		INT32 ymin = (INT32)(final_out_results[num].y1 * ratioh + 0.5);
		INT32 width = (INT32)(final_out_results[num].x2 * ratiow + 0.5) - xmin;
		INT32 height = (INT32)(final_out_results[num].y2 * ratioh + 0.5) - ymin;
		//INT32 classtype = topbox[num].classType;
		FLOAT score = final_out_results[num].score;

		printf("no=%ld score=%f x,y,w,h=(%d, %d, %d, %d)\r\n", num, score, xmin, ymin, width, height);
	}

	return ret;
}

static HD_RESULT network_get_fastboot_rslt(void)
{
	HD_RESULT ret = HD_OK;
	PD_RESULT_INFO rslt_info = {0};
	UINT32 rslt_num = 0;
	UINT32 cfg_id = 0Xafb00000; // use a magic number as a temporary id
#if DUMP_FBOOT_RSLT
	UINT32 num = 0;
	FLOAT ratiow = 0.0, ratioh = 0.0;
	ratiow = (FLOAT)VDO_SIZE_W / (FLOAT)NET_VDO_SIZE_W;
	ratioh = (FLOAT)VDO_SIZE_H / (FLOAT)NET_VDO_SIZE_H;
#endif

	while (vendor_ai_cfg_get(cfg_id, (void *)&rslt_info) == HD_OK) {
#if DUMP_FBOOT_RSLT
		PD_RESULT* final_out_results = rslt_info.p_result;
		printf("detection results:\r\n");
		for (num = 0; num < rslt_info.result_num; num++) {
			INT32 xmin = (INT32)(final_out_results[num].x1 * ratiow + 0.5);
			INT32 ymin = (INT32)(final_out_results[num].y1 * ratioh + 0.5);
			INT32 width = (INT32)(final_out_results[num].x2 * ratiow + 0.5) - xmin;
			INT32 height = (INT32)(final_out_results[num].y2 * ratioh + 0.5) - ymin;
			//INT32 classtype = topbox[num].classType;
			FLOAT score = final_out_results[num].score;

			printf("no=%ld score=%f x,y,w,h=(%d, %d, %d, %d)\r\n", num, score, xmin, ymin, width, height);
		}
#endif
		rslt_num++;
	}

	printf("========= get fastboot rslt finished, num(%d) =========\r\n", rslt_num);
	return ret;
}
#endif

#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_RECORD {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;

	// only used when (g_capbind = 0xff)  //no-bind mode
	pthread_t  cap_thread_id;
	UINT32	cap_enter;
	UINT32	cap_exit;
	UINT32	cap_count;
	UINT32 	cap_loop;

	// (2)
	HD_DIM  proc_max_dim;
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	// only used when (g_prcbind = 0xff)  //no-bind mode
	pthread_t  prc_thread_id;
	UINT32	prc_enter;
	UINT32	prc_exit;
	UINT32 	prc_count;
	UINT32 	prc_loop;

	// (3)
	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// user pull
	pthread_t  enc_thread_id;
	UINT32     enc_enter;
	UINT32     enc_exit;

	// (4)
	pthread_t  acap_thread_id;
	HD_PATH_ID  acap_ctrl;
	HD_PATH_ID  acap_path;
	HD_AUDIO_SR acap_sr_max;
	HD_AUDIO_SR acap_sr;
	UINT32      acap_enter;
	UINT32      acap_exit;
#if (VOUT_DISP==ENABLE)
    // (5) display
	HD_DIM  out_max_dim;
	HD_DIM  out_dim;
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
	HD_PATH_ID proc_path2;
#endif
#if (USE_AI==ENABLE)
	// (5) network 
	NET_PROC_CONFIG net_proc_cfg;
	NET_PATH_ID net_path;
	pthread_t  net_proc_thread_id;
	UINT32 net_proc_start;
	UINT32 net_proc_exit;
	UINT32 net_proc_oneshot;
#endif
} VIDEO_RECORD;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
	if (g_acapen) {
		if ((ret = hd_audiocap_init()) != HD_OK)
			return ret;
	}
#if (VOUT_DISP==ENABLE)
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_RECORD *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, HD_VIDEOPROC_0_CTRL);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	if (g_acapen) {
		// set audiocap config
		ret = set_acap_cfg(&p_stream->acap_ctrl, p_stream->acap_sr_max);
		if (ret != HD_OK) {
			printf("set acap-cfg fail=%d\n", ret);
			return HD_ERR_NG;
		}
		if ((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_stream->acap_path)) != HD_OK)
			return ret;
	}
#if (VOUT_DISP==ENABLE)
	// set videoout config 1:only support lcd
	ret = set_out_cfg(&p_stream->out_ctrl, 1,0);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
#endif
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;
#if (VOUT_DISP==ENABLE)
	 if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_2, &p_stream->proc_path2)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT open_module_2(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1,  &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module_3(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_2, HD_VIDEOENC_0_OUT_2,  &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

#if (USE_AI==ENABLE)
static HD_RESULT open_module_4(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_2, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_open(p_stream->net_path)) != HD_OK)
		return ret;
	
	return HD_OK;
}
#endif

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	if (g_acapen) {
		if ((ret = hd_audiocap_close(p_stream->acap_path)) != HD_OK)
			return ret;
	}

#if (VOUT_DISP==ENABLE)
	if ((ret = hd_videoproc_close(p_stream->proc_path2)) != HD_OK)
		return ret;

	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
#endif

	return HD_OK;
}

static HD_RESULT close_module_2(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module_3(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

#if (USE_AI==ENABLE)
static HD_RESULT close_module_4(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_close(p_stream->net_path)) != HD_OK)
		return ret;
	return HD_OK;
}
#endif

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
	if (g_acapen) {
		if ((ret = hd_audiocap_uninit()) != HD_OK)
			return ret;
	}
#if (VOUT_DISP==ENABLE)
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
#endif
#if (USE_AI==ENABLE)
	if ((ret = network_uninit()) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

// only used when (g_capbind = 0xff)  //no-bind mode
static void *capture_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;

	HD_VIDEO_FRAME video_frame = {0};

	p_stream0->cap_exit = 0;
	p_stream0->cap_loop = 0;
	p_stream0->cap_count = 0;
	//------ wait flow_start ------
	while (p_stream0->cap_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->cap_exit == 0) {

		//printf("cap_pull ....\r\n");
		ret = hd_videocap_pull_out_buf(p_stream0->cap_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("cap_pull error=%d !!\r\n\r\n", ret);
    			goto skip;
		}
		//printf("cap frame.count = %llu\r\n", video_frame.count);

		//printf("proc_push ....\r\n");
		ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			printf("proc_push error=%d !!\r\n\r\n", ret);
    			goto skip;
		}

		//printf("cap_release ....\r\n");
		ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame);
		if (ret != HD_OK) {
			printf("cap_release error=%d !!\r\n\r\n", ret);
    			goto skip;
		}

		p_stream0->cap_count ++;
		//printf("capture count = %d\r\n", p_stream0->cap_count);
skip:
		p_stream0->cap_loop++;
		usleep(100); //sleep for getchar()
	}

	return 0;
}

// only used when (g_prcbind = 0xff)  //no-bind mode
static void *process_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;

	HD_VIDEO_FRAME video_frame = {0};

	p_stream0->prc_exit = 0;
	p_stream0->prc_loop = 0;
	p_stream0->prc_count = 0;
	//------ wait flow_start ------
	while (p_stream0->prc_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->prc_exit == 0) {

		//printf("proc_pull ....\r\n");
		ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("proc_pull error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}
		//printf("proc frame.count = %llu\r\n", video_frame.count);

		//printf("enc_push ....\r\n");
		ret = hd_videoenc_push_in_buf(p_stream0->enc_path, &video_frame, NULL, -1); // blocking mode
		if (ret != HD_OK) {
			printf("enc_push error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}

		//printf("proc_release ....\r\n");
		ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
		if (ret != HD_OK) {
			printf("proc_release error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}

		p_stream0->prc_count ++;
		//printf("process count = %d\r\n", p_stream0->prc_count);
skip2:
		p_stream0->prc_loop++;
		usleep(100); //sleep for getchar()
	}

	return 0;
}

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	VIDEO_RECORD* p_stream1 = p_stream0 + 1;
	VIDEO_RECORD* p_stream2 = p_stream0 + 2;
	HD_RESULT ret = HD_OK;
	UINT32 poll_num = 1;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;
	HD_VIDEOENC_POLL_LIST poll_list[3];

	UINT32 vir_addr_main = 0;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main = NULL;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	UINT32 vir_addr_snapshot = 0;
	HD_VIDEOENC_BUFINFO phy_buf_snapshot;
	char file_path_snapshot[32]  = "/mnt/sd/dump_bs_snapshot.dat";
	FILE *f_out_snapshot = NULL;
	#define PHY2VIRT_SNAPSHOT(pa) (vir_addr_snapshot + (pa - phy_buf_snapshot.buf_info.phy_addr))

	UINT32 vir_addr_sub = 0;
	HD_VIDEOENC_BUFINFO phy_buf_sub;
	char file_path_sub[32] = "/mnt/sd/dump_bs_sub.dat";
	FILE *f_out_sub = NULL;
	#define PHY2VIRT_SUB(pa) (vir_addr_sub + (pa - phy_buf_sub.buf_info.phy_addr))

	if (g_snapshoten) {
		poll_num++;
	}
	if (g_suben) {
		poll_num++;
	}
	//------ wait flow_start ------
	while (p_stream0->enc_enter == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
	if (g_snapshoten) {
		hd_videoenc_get(p_stream1->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_snapshot);
	}
	if (g_suben) {
		hd_videoenc_get(p_stream2->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub);
	}

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
	if (g_snapshoten) {
		vir_addr_snapshot  = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_snapshot.buf_info.phy_addr, phy_buf_snapshot.buf_info.buf_size);
	}
	if (g_suben) {
		vir_addr_sub = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub.buf_info.phy_addr, phy_buf_sub.buf_info.buf_size);
	}
	//----- open output files -----
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
	if (g_snapshoten) {
		if ((f_out_snapshot = fopen(file_path_snapshot, "wb")) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_snapshot);
		} else {
			printf("\r\ndump sub  bitstream to file (%s) ....\r\n", file_path_snapshot);
		}
	}
	if (g_suben) {
		if ((f_out_sub = fopen(file_path_sub, "wb")) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_sub);
		} else {
			printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_sub);
		}
	}

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	poll_list[0].path_id = p_stream0->enc_path;
	if (g_snapshoten) {
		poll_list[1].path_id = p_stream1->enc_path;
	}
	if (g_suben) {
		if (g_snapshoten) {
			poll_list[2].path_id = p_stream2->enc_path;
		} else {
			poll_list[1].path_id = p_stream2->enc_path;
		}
	}

	while (p_stream0->enc_exit == 0) {
		if (HD_OK == hd_videoenc_poll_list(poll_list, poll_num, -1)) {    // multi path poll_list , -1 = blocking mode
			if (TRUE == poll_list[0].revent.event) {
				//pull data
				ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, 0); // 0 = non-blocking mode

				if (ret == HD_OK) {
					for (j=0; j< data_pull.pack_num; j++) {
						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
						UINT32 len = data_pull.video_pack[j].size;
						if (f_out_main) fwrite(ptr, 1, len, f_out_main);
						if (f_out_main) fflush(f_out_main);
					}

					// release data
					ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
					if (ret != HD_OK) {
						printf("enc_release error=%d !!\r\n", ret);
					}
				}
			}

			if (g_snapshoten) {
				if (TRUE == poll_list[1].revent.event) {
					//pull data
					ret = hd_videoenc_pull_out_buf(p_stream1->enc_path, &data_pull, 0); // 0 = non-blocking mode

					if (ret == HD_OK) {
						for (j=0; j< data_pull.pack_num; j++) {
							UINT8 *ptr = (UINT8 *)PHY2VIRT_SNAPSHOT(data_pull.video_pack[j].phy_addr);
							UINT32 len = data_pull.video_pack[j].size;
							if (f_out_snapshot) fwrite(ptr, 1, len, f_out_snapshot);
							if (f_out_snapshot) fflush(f_out_snapshot);
						}

						// release data
						ret = hd_videoenc_release_out_buf(p_stream1->enc_path, &data_pull);
						if (ret != HD_OK) {
							printf("enc_release error=%d !!\r\n", ret);
						}
					}
				}
			}

			if (g_suben) {
				if (TRUE == poll_list[2].revent.event) {
					//pull data
					ret = hd_videoenc_pull_out_buf(p_stream2->enc_path, &data_pull, 0); // 0 = non-blocking mode

					if (ret == HD_OK) {
						for (j=0; j< data_pull.pack_num; j++) {
							UINT8 *ptr = (UINT8 *)PHY2VIRT_SUB(data_pull.video_pack[j].phy_addr);
							UINT32 len = data_pull.video_pack[j].size;
							if (f_out_sub) fwrite(ptr, 1, len, f_out_sub);
							if (f_out_sub) fflush(f_out_sub);
						}

						// release data
						ret = hd_videoenc_release_out_buf(p_stream2->enc_path, &data_pull);
						if (ret != HD_OK) {
							printf("enc_release error=%d !!\r\n", ret);
						}
					}
				}
			}
		}
	}

	// mummap for bs buffer
	if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	if (g_snapshoten) {
		if (vir_addr_snapshot)  hd_common_mem_munmap((void *)vir_addr_snapshot, phy_buf_snapshot.buf_info.buf_size);
	}
	if (g_suben) {
		if (vir_addr_sub) hd_common_mem_munmap((void *)vir_addr_sub, phy_buf_sub.buf_info.buf_size);
	}
	// close output file
	if (f_out_main) fclose(f_out_main);
	if (g_snapshoten) {
		if (f_out_snapshot) fclose(f_out_snapshot);
	}
	if (g_suben) {
		if (f_out_sub) fclose(f_out_sub);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

#if (USE_AI==ENABLE)
static VOID *network_proc_thread(VOID *arg);

static HD_RESULT network_user_start(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret = HD_OK;

	p_stream->net_proc_start = 0;
	p_stream->net_proc_exit = 0;
	p_stream->net_proc_oneshot = 0;
	
	ret = vendor_ai_net_start(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) start fail !!\n", p_stream->net_path);
	}

	ret = pthread_create(&p_stream->net_proc_thread_id, NULL, network_proc_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}

	p_stream->net_proc_start = 1;
	p_stream->net_proc_exit = 0;
	p_stream->net_proc_oneshot = 0;
	
	return ret;
}


static HD_RESULT network_user_stop(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->net_proc_exit = 1;
	
	if (p_stream->net_proc_thread_id) {
		pthread_join(p_stream->net_proc_thread_id, NULL);
	}

	//stop: should be call after last time proc
	ret = vendor_ai_net_stop(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) stop fail !!\n", p_stream->net_path);
	}
	
	return ret;
}

static VOID *network_proc_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	
	VIDEO_RECORD *p_stream = (VIDEO_RECORD*)arg;

	printf("\r\n");
	while (p_stream->net_proc_start == 0) sleep(1);

#if (USE_AI_MODEL == 1)
	// get ai fastboot rslt
	network_get_fastboot_rslt();
#endif

	printf("\r\n");
	while (p_stream->net_proc_exit == 0) {

		if (1) {
			
			HD_VIDEO_FRAME video_frame = {0};
			VENDOR_AI_BUF	in_buf = {0};
#if (USE_AI_MODEL == 0)
			VENDOR_AI_POSTPROC_RESULT_INFO out_buf = {0};
#endif
#if (USE_AI_MODEL == 1)
			PD_RESULT_INFO out_buf = {0};
#endif

			ret = hd_videoproc_pull_out_buf(p_stream->proc_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto skip;
			}

			//prepare input AI_BUF from videoframe
			in_buf.sign = MAKEFOURCC('A','B','U','F');
			in_buf.width = video_frame.dim.w;
			in_buf.height = video_frame.dim.h;
			in_buf.channel 	= HD_VIDEO_PXLFMT_PLANE(video_frame.pxlfmt); //conver pxlfmt to channel count
			in_buf.line_ofs	= video_frame.loff[0];
			in_buf.fmt = video_frame.pxlfmt;
			in_buf.pa = video_frame.phy_addr[0];
			in_buf.va = 0;
			in_buf.size = video_frame.loff[0]*video_frame.dim.h*3/2;

			// set input image
			ret = vendor_ai_net_set(p_stream->net_path, VENDOR_AI_NET_PARAM_IN(0, 0), &in_buf);
			if (HD_OK != ret) {
				printf("proc_id(%u)push input fail !!\n", p_stream->net_path);
				goto skip;
			}

			// do net proc
			ret = vendor_ai_net_proc(p_stream->net_path);
			if (HD_OK != ret) {
				printf("proc_id(%u) proc fail !!\n", p_stream->net_path);
				goto skip;
			}

			// get output result
			ret = vendor_ai_net_get(p_stream->net_path, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
			if (HD_OK != ret) {
				printf("proc_id(%u) output get fail !!\n", p_stream->net_path);
				goto skip;
			}
			
			ret = network_dump_out_buf(p_stream->net_path, &out_buf);
			if (HD_OK != ret) {
				printf("proc_id(%u) output dump fail !!\n", p_stream->net_path);
				goto skip;
			}
			
			ret = hd_videoproc_release_out_buf(p_stream->proc_path, &video_frame);
			if(ret != HD_OK) {
				printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto skip;
			}
		}
		usleep(100);
	}
	
skip:

	return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////

static int setup_vdo_size(void)
{
#if (AUTO_RESOLUTION_SET && defined(__LINUX))
	char *sensor1_name = getenv("SENSOR1");
	if (sensor1_name) {
		int i;
		int resolution = -1;
		int n = sizeof(SENSOR_NAME_TBL)/sizeof(SENSOR_NAME_TBL[0]);
		for (i = 0; i < n; i++) {
			if (strncmp(sensor1_name, SENSOR_NAME_TBL[i], strlen(SENSOR_NAME_TBL[i])+1) == 0) {
				resolution = i;
				break;
			}
		}
		if (resolution == -1) {
			printf("unable to handle sensor: %s, does its support add to SENSOR_NAME_TBL, VDO_SIZE_W_TBL and VDO_SIZE_W_TBL", RESOLUTION_SET);
			return -1;
		}
		RESOLUTION_SET = (UINT32)resolution;
	} else {
		printf("failed to getenv:'SENSOR1', use default RESOLUTION_SET: %d", RESOLUTION_SET);
	}
#endif
	if (RESOLUTION_SET >= sizeof(VDO_SIZE_W_TBL)/sizeof(VDO_SIZE_W_TBL[0])) {
		printf("invalid RESOLUTION_SET: %d\n", RESOLUTION_SET);
		return -1;
	}
	printf("setup RESOLUTION_SET to %s\n", SENSOR_NAME_TBL[RESOLUTION_SET]);
	VDO_SIZE_W = VDO_SIZE_W_TBL[RESOLUTION_SET];
	VDO_SIZE_H = VDO_SIZE_H_TBL[RESOLUTION_SET];
	return 0;
}

static void *acapture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
	VIDEO_RECORD* p_cap_only = (VIDEO_RECORD *)arg;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->acap_sr);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->acap_sr);

	/* wait flow_start */
	while (p_cap_only->acap_enter == 0) {
		usleep(100);
	}

	/* query physical address of bs buffer
	  (this can ONLY query after hd_audiocap_start() is called !!) */
	hd_audiocap_get(p_cap_only->acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main);

	/* mmap for bs buffer
	  (just mmap one time only, calculate offset to virtual address later) */
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	if (vir_addr_main == 0) {
		printf("acap mmap error\r\n");
		return 0;
	}

	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		//printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}

	/* pull data test */
	while (p_cap_only->acap_exit == 0) {
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->acap_path, &data_pull, -1); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);

			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->acap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}
	}

	/* mummap for bs buffer */
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	/* close output file */
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);

	return 0;
}


MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[4] = {0}; //0: main stream
	UINT32 enc_type = 0;
	UINT32 snapshot_enc_type = 0;
	UINT32 sub_enc_type = 0;
	UINT32 ai_dump_fastboot = 0;
	HD_DIM main_dim;
	HD_DIM sub_dim;
	#if defined(__LINUX)
	#if (ISP_RELOAD == ISP_RELOAD_DTSI)
	IQT_DTSI_INFO iq_dtsi_info = {0};
	AET_DTSI_INFO ae_dtsi_info = {0};
	AWBT_DTSI_INFO awb_dtsi_info = {0};
	#else
	AET_CFG_INFO cfg_info = {0};
	#endif
	#endif
	#if (USE_AI==ENABLE)
	#if (USE_AI_MODEL == 0)
	//net proc
	NET_PROC_CONFIG net_cfg = {
		.model_filename = "para/nvt_model.bin",
		.label_filename = "accuracy/labels.txt"
	};
	#endif
	#if (USE_AI_MODEL == 1)
	//net proc
	NET_PROC_CONFIG net_cfg = {
		.model_filename = "para/560_pd_model.bin",
		.label_filename = "accuracy/labels.txt"
	};
	#endif
	#endif

	if (setup_vdo_size() != 0) {
		return 0;
	}

	if (argc == 1 || argc > 12) {
		printf("Usage: <enc-type> <cap_out_bind> <cap_out_fmt> <prc_out_bind> <prc_out_fmt> <acap_enable> <shdr_enable> <snapshot_enable> <sub_enable> <sub_enc_type> <ai_dump_fastboot>.\r\n");
		printf("Help:\r\n");
		printf("  <enc_type> : 0(H265), 1(H264), 2(MJPG)\r\n");
		printf("  <cap_out_bind>: 0(D2D Mode), 1(Direct Mode), 2(OneBuf Mode), x(no-bind)\r\n");
		printf("  <cap_out_fmt> : 0(RAW12),  1(NRX12 - RAW compress)\r\n");
		printf("  <prc_out_bind>: 0(D2D Mode), 1(LowLatency Mode), 2(OneBuf Mode), x(no-bind)\r\n");
		printf("  <prc_out_fmt> : 0(YUV420), 1(NVX - YUV compress)\r\n");
		printf("  <acap_enable> : 0(Disable acap), 1(Enable acap)\r\n");
		printf("  <shdr_enable> : 0(Disable SHDR), 1(Enable SHDR)\r\n");
		printf("  <snapshot_enable> : 0(Disable snapshot), 1(Enable snapshot)\r\n");
		printf("  <sub_enable> : 0(Disable substream), 1(Enable substream)\r\n");
		printf("  <sub_enc_type> : 0(H265), 1(H264), 2(MJPG)\r\n");
		printf("  <ai_dump_fastboot> : 0(Disable ai dump fasboot), 1(Enable ai dump fastboot)\r\n");
		return 0;
	}

	// query program options
	if (argc > 1) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}
	if (argc > 2) {
		if (argv[2][0] == 'x') {
			g_capbind = 0xff;
			printf("CAP-BIND x\r\n");
		} else {
			g_capbind = atoi(argv[2]);
			printf("CAP-BIND %d\r\n", g_capbind);
			if(g_capbind > 3) {
				printf("error: not support CAP-BIND! (%d) \r\n", g_capbind);
				return 0;
			}
		}
	}
	if (argc > 3) {
		g_capfmt = atoi(argv[3]);
		printf("CAP-FMT %d\r\n", g_capfmt);
		if(g_capfmt > 1) {
			printf("error: not support CAP-FMT! (%d) \r\n", g_capfmt);
			return 0;
		}
	}
	if (argc > 4) {
		if (argv[4][0] == 'x') {
			g_prcbind = 0xff;
			printf("PRC-BIND x\r\n");
		} else {
			g_prcbind = atoi(argv[4]);
			printf("PRC-BIND %d\r\n", g_prcbind);
			if(g_prcbind > 3) {
				printf("error: not support PRC-BIND! (%d) \r\n", g_prcbind);
				return 0;
			}
		}
	}
	if (argc > 5) {
		g_prcfmt = atoi(argv[5]);
		printf("PRC-FMT %d\r\n", g_prcfmt);
		if(g_prcfmt > 2) {
			printf("error: not support PRC-FMT ! (%d) \r\n", g_prcfmt);
			return 0;
		}
	}
	if (argc > 6) {
		g_acapen = atoi(argv[6]);
		printf("ACAP-EN %d\r\n", g_acapen);
	}
	if (argc > 7) {
		g_shdr = atoi(argv[7]);
		printf("SHDR-EN %d\r\n", g_shdr);
	}
	if (argc > 8) {
		g_snapshoten = atoi(argv[8]);
		printf("SNAPSHOT-EN %d\r\n", g_snapshoten);
	}
	if (argc > 9) {
		g_suben = atoi(argv[9]);
		printf("SUBSTREAM-EN %d\r\n", g_suben);
	}
	if (argc > 10) {
		sub_enc_type = atoi(argv[10]);
		printf("sub_enc_type %d\r\n", sub_enc_type);
		if(enc_type > 2) {
			printf("error: not support sub_enc_type!\r\n");
			return 0;
		}
	}
	if (argc > 11) {
		ai_dump_fastboot = atoi(argv[11]);
		printf("ai_dump_fastboot %d\r\n", ai_dump_fastboot);
	}

	// assign g_capfmt
	if (g_capfmt == 0) {
		g_capfmt = HD_VIDEO_PXLFMT_RAW12;
	} else {
		g_capfmt =  HD_VIDEO_PXLFMT_NRX12;
	}

	// assign g_prcfmt
	if (g_prcfmt == 0) {
		g_prcfmt = HD_VIDEO_PXLFMT_YUV420;
	} else {
		#if 0 //680
			if (enc_type == 0) {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H265;
			} else if (enc_type == 1) {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX1_H264;
			} else {
				g_prcfmt = HD_VIDEO_PXLFMT_YUV420;
			}
		#else //520
			g_prcfmt = HD_VIDEO_PXLFMT_YUV420_NVX2;
		#endif
	}

	#if defined(__LINUX)
	// load ae/awb/iq paramters
	if (vendor_isp_init() == HD_OK) {
		#if (ISP_RELOAD == ISP_RELOAD_DTSI)
		iq_dtsi_info.id = 0;
		ae_dtsi_info.id = 0;
		awb_dtsi_info.id = 0;

		switch(RESOLUTION_SET) {
		case 0:
			if (g_shdr == 1) {
				strncpy(ae_dtsi_info.node_path, "/isp/ae/imx290_ae_0", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.node_path, "/isp/awb/imx290_awb_0", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.node_path, "/isp/iq/imx290_iq_hdr_0", DTSI_NAME_LENGTH);
				strncpy(ae_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp.dtb\n");
			} else {
				strncpy(ae_dtsi_info.node_path, "/isp/ae/imx290_ae_0", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.node_path, "/isp/awb/imx290_awb_0", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.node_path, "/isp/iq/imx290_iq_0", DTSI_NAME_LENGTH);
				strncpy(ae_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp.dtb\n");
			}
			break;
		case 5:
				strncpy(ae_dtsi_info.node_path, "/isp/ae/f37_ae_0", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.node_path, "/isp/awb/f37_awb_0", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.node_path, "/isp/iq/f37_iq_0", DTSI_NAME_LENGTH);
				strncpy(ae_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp.dtb\n");
			break;
		case 6:
				strncpy(ae_dtsi_info.node_path, "/isp/ae/f37_ae_0", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.node_path, "/isp/awb/f37_awb_0", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.node_path, "/isp/iq/f37_iq_0", DTSI_NAME_LENGTH);
				strncpy(ae_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(awb_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				strncpy(iq_dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp.dtb\n");
			break;
		default:
			printf("Does not load ISP dtsi.\n");
			break;
		}
		vendor_isp_set_ae(AET_ITEM_RLD_DTSI, &ae_dtsi_info);
		vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &awb_dtsi_info);
		vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &iq_dtsi_info);	
		
		#else
		cfg_info.id = 0;

		switch(RESOLUTION_SET) {
		case 0:
			if (g_shdr == 1) {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0_hdr.cfg", CFG_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp_imx290_0_hdr.cfg\n");
			} else {
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
				printf("Load /mnt/app/isp/isp_imx290_0.cfg\n");
			}
			break;
		case 5:
			strncpy(cfg_info.path, "/mnt/app/isp/isp_f37.cfg", CFG_NAME_LENGTH);
			printf("Load /mnt/app/isp/isp_f37.cfg\n");
			break;
		case 6:
			strncpy(cfg_info.path, "/mnt/app/isp/isp_f37.cfg", CFG_NAME_LENGTH);
			printf("Load /mnt/app/isp/isp_f37.cfg\n");
			break;
		default:
			printf("Does not load ISP cfg.\n");
			break;
		}
		vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
		vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
		vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
		#endif
		vendor_isp_uninit();
	} else {
		printf("Init vendor isp fail. \n");
	}
	#endif

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}
	
	#if (USE_AI==ENABLE)
	// set system config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init network
	if ((ret = network_init()) != HD_OK)
		return ret;

	// set optimize config for AI
	net_cfg.job_method = 11;
	net_cfg.job_wait_ms = 0;
	net_cfg.buf_method = 2;

	// init stream path for AI
	stream[3].net_path = 0;
	
	// set mem config for AI
	network_mem_config(stream[3].net_path, &net_cfg);
	#endif
	
	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	// memory hotplug
	ret = memory_hotplug();
	if (ret != HD_OK) {
		printf("memory hotplug fail=%d\n", ret);
		//goto exit;
	}

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open video_record modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	if (g_acapen) {
		stream[0].acap_sr_max = HD_AUDIO_SR_48000;
	}
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	if (g_snapshoten) {
		// open video_record modules (snapshot)
		stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
		stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
		ret = open_module_2(&stream[1]);
		if (ret != HD_OK) {
			printf("open2 fail=%d\n", ret);
			goto exit;
		}
	}
	if (g_suben) {
		// open video_record modules (sub)
		stream[2].proc_max_dim.w = VDO_SIZE_W; //assign by user
		stream[2].proc_max_dim.h = VDO_SIZE_H; //assign by user
		ret = open_module_3(&stream[2]);
		if (ret != HD_OK) {
			printf("open3 fail=%d\n", ret);
			goto exit;
		}
	}

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, 0);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	sub_dim.w = VDO_SIZE_W;
	sub_dim.h = VDO_SIZE_H;
	// set videoproc parameter (main)
	if (g_prcbind == 0xff) //no-bind mode
		ret = set_proc_param(stream[0].proc_path, &main_dim, 1); //must set vprc's out dim
	else
		ret = set_proc_param(stream[0].proc_path, NULL, 0); //keep vprc's out dim = {0,0}, it means auto sync from venc's in dim
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (main)
	stream[0].enc_max_dim.w = main_dim.w;
	stream[0].enc_max_dim.h = main_dim.h;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_dim.w = main_dim.w;
	stream[0].enc_dim.h = main_dim.h;
	ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, enc_type, 2 * 1024 * 1024);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	if (g_snapshoten) {
		// set videoenc config (sub)
		stream[1].enc_max_dim.w = sub_dim.w;
		stream[1].enc_max_dim.h = sub_dim.h;
		ret = set_enc_cfg(stream[1].enc_path, &stream[1].enc_max_dim, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}

		// set videoenc parameter (sub)
		snapshot_enc_type = 2;
		stream[1].enc_dim.w = sub_dim.w;
		stream[1].enc_dim.h = sub_dim.h;
		ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, snapshot_enc_type, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}
	}

	if (g_suben) {
		// set videoproc parameter (sub)
		ret = set_proc_param(stream[2].proc_path, NULL, 0); //keep vprc's out dim = {0,0}, it means auto sync from venc's in dim
		if (ret != HD_OK) {
			printf("set proc fail=%d\n", ret);
			goto exit;
		}

		// set videoenc config (sub)
		stream[2].enc_max_dim.w = sub_dim.w;
		stream[2].enc_max_dim.h = sub_dim.h;
		ret = set_enc_cfg(stream[2].enc_path, &stream[2].enc_max_dim, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc-cfg fail=%d\n", ret);
			goto exit;
		}

		// set videoenc parameter (sub)
		stream[2].enc_dim.w = sub_dim.w;
		stream[2].enc_dim.h = sub_dim.h;
		ret = set_enc_param(stream[2].enc_path, &stream[2].enc_dim, sub_enc_type, 2 * 1024 * 1024);
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}
	}

	if (g_acapen) {
		stream[0].acap_sr = HD_AUDIO_SR_16000;
		ret = set_acap_param(stream[0].acap_path, stream[0].acap_sr);
		if (ret != HD_OK) {
			printf("set acap fail=%d\n", ret);
			goto exit;
		}
	}

	if (g_capbind == 0xff) { //no-bind mode
		// create capture_thread (pull_out frame, push_in frame then pull_out frame)
		ret = pthread_create(&stream[0].cap_thread_id, NULL, capture_thread, (void *)stream);
		if (ret < 0) {
			printf("create capture_thread failed");
			goto exit;
		}
	} else {
		// bind video_record modules (main)
		hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	}
	if (g_prcbind == 0xff) { //no-bind mode
		// create process_thread (pull_out frame, push_in frame then pull_out bitstream)
		ret = pthread_create(&stream[0].prc_thread_id, NULL, process_thread, (void *)stream);
		if (ret < 0) {
			printf("create process_thread failed");
			goto exit;
		}
	} else {
		// bind video_record modules (main)
		hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);
	}
	if (g_suben) {
		// bind video_record modules (sub)
		hd_videoproc_bind(HD_VIDEOPROC_0_OUT_1, HD_VIDEOENC_0_IN_2);
	}

#if (VOUT_DISP==ENABLE)

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_2, HD_VIDEOOUT_0_IN_0);
#endif

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode_thread failed");
		goto exit;
	}

	// start video_record modules (main)
	if (g_capbind == 1) {
		//direct NOTE: ensure videocap start after 1st videoproc phy path start
		hd_videoproc_start(stream[0].proc_path);
		hd_videocap_start(stream[0].cap_path);
	} else {
		hd_videocap_start(stream[0].cap_path);
		hd_videoproc_start(stream[0].proc_path);
	}
#if (VOUT_DISP==ENABLE)
	hd_videoproc_start(stream[0].proc_path2);
	hd_videoout_start(stream[0].out_path);
#endif
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	//sleep(1);
	hd_videoenc_start(stream[0].enc_path);
	if (g_snapshoten) {
		hd_videoenc_start(stream[1].enc_path);
	}
	if (g_suben) {
		hd_videoproc_start(stream[2].proc_path);
		hd_videoenc_start(stream[2].enc_path);
	}

	if (g_acapen) {
		// create encode_thread (pull_out bitstream)
		ret = pthread_create(&stream[0].acap_thread_id, NULL, acapture_thread, (void *)stream);
		if (ret < 0) {
			printf("create encode_thread failed");
			goto exit;
		}
		hd_audiocap_start(stream[0].acap_path);
	}

	if (g_capbind == 0xff) { //no-bind mode
		// start capture_thread
		stream[0].cap_enter = 1;
	}
	if (g_prcbind == 0xff) { //no-bind mode
		// start process_thread
		stream[0].prc_enter = 1;
	}
	// start encode_thread
	stream[0].enc_enter = 1;

	if (g_acapen) {
		// start acapture_thread
		stream[0].acap_enter = 1;
	}

#if (USE_AI==ENABLE)
	// set AI open config
	ret = network_set_config(stream[3].net_path, &net_cfg);
	if (HD_OK != ret) {
		printf("proc_id(%u) network_set_config fail=%d\n", stream[3].net_path, ret);
		goto exit;
	}

	// set videoproc open config for AI
	stream[3].proc_max_dim.w = NET_VDO_SIZE_W; //assign by user
	stream[3].proc_max_dim.h = NET_VDO_SIZE_H; //assign by user

	// open AI modules
	ret = open_module_4(&stream[3]);
	if (ret != HD_OK) {
		printf("open4 fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter for AI
	ret = set_proc_param(stream[3].proc_path, &stream[3].proc_max_dim, 1); //output dim == AI input dim
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// start vproc for AI path
	hd_videoproc_start(stream[3].proc_path);

	// need to echo ai dump fastboot before start
	{
		CHAR cmd[64] = {0};
		printf("dumping fastboot ai model and dtsi to /tmp \r\n");
		system("mkdir -p /tmp/fastboot");
		sprintf(cmd, "echo dump_fastboot %lu > /proc/kflow_ai/kcmd", ai_dump_fastboot);
		system(cmd);
	}

	// start network
	network_user_start(&stream[3]);
#endif

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// quit program
			break;
		}

		if (key == 'a') {
			hd_videoproc_stop(stream[0].proc_path);
			hd_videoproc_start(stream[0].proc_path);
		}

		if (key == 'i') {
			printf("dumping fastboot dtsi to /tmp \r\n");
			system("echo dump_fastboot  /tmp/fastboot/nvt-fastboot-sie.dtsi > /proc/nvt_ctl_sie/cmd");
			system("echo dump_fastboot /tmp/fastboot/nvt-fastboot-ipp.dtsi > /proc/kflow_ctl_ipp/cmd");
			system("echo vdoenc dump_fastboot /tmp/fastboot/nvt-fastboot-venc.dtsi > /proc/hdal/venc/cmd");
			system("echo dump_fastboot /tmp/fastboot/nvt-fastboot-acap.dtsi > /proc/hdal/acap/cmd");
			// to solve 'Command length is too long!'
			system("echo nvtmpp fastboot_mem /tmp/hdal-mem.dtsi > /proc/hdal/comm/cmd");
			system("mv /tmp/hdal-mem.dtsi /tmp/fastboot/nvt-fastboot-hdal-mem.dtsi");
			printf("finish. \r\n");
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	if (g_capbind == 0xff) { //no-bind mode
		// stop capture_thread
		stream[0].cap_exit = 1;
	}
	if (g_prcbind == 0xff) { //no-bind mode
		// stop process_thread
		stream[0].prc_exit = 1;
	}
	// stop encode_thread
	stream[0].enc_exit = 1;

	if (g_acapen) {
		// start acapture_thread
		stream[0].acap_exit = 1;
	}

#if (VOUT_DISP==ENABLE)
	hd_videoproc_stop(stream[0].proc_path2);
	hd_videoout_stop(stream[0].out_path);
#endif
#if (USE_AI==ENABLE)
	network_user_stop(&stream[3]);
	hd_videoproc_stop(stream[3].proc_path);
#endif

	if (g_suben) {
		hd_videoproc_stop(stream[2].proc_path);
		hd_videoenc_stop(stream[2].enc_path);
	}

	// stop video_record modules (main)
	if (g_capbind == 1) {
		//direct NOTE: ensure videocap stop after all videoproc path stop
		hd_videoproc_stop(stream[0].proc_path);
		hd_videocap_stop(stream[0].cap_path);
	} else {
		hd_videocap_stop(stream[0].cap_path);
		hd_videoproc_stop(stream[0].proc_path);
	}

	hd_videoenc_stop(stream[0].enc_path);
	if (g_snapshoten) {
		hd_videoenc_stop(stream[1].enc_path);
	}

	if (g_acapen) {
		hd_audiocap_stop(stream[0].acap_path);
	}

	// destroy encode_thread
	pthread_join(stream[0].enc_thread_id, NULL); //NOTE: before destory, call stop to breaking pull(-1)

	if (g_capbind == 0xff) { //no-bind mode
		// destroy capture_thread
		pthread_join(stream[0].cap_thread_id, NULL);  //NOTE: before destory, call stop to breaking pull(-1)
	} else {
		// unbind video_record modules (main)
		hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	}

	if (g_prcbind == 0xff) { //no-bind mode
		// destroy process_thread
		pthread_join(stream[0].prc_thread_id, NULL);  //NOTE: before destory, call stop to breaking pull(-1)
	} else {
		// unbind video_record modules (main)
		hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
	}
	if (g_suben) {
		// unbind video_record modules (sub)
		hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_1);
	}
#if (VOUT_DISP==ENABLE)
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_2);
#endif

	if (g_acapen) {
		pthread_join(stream[0].acap_thread_id, NULL);
	}

exit:
	// close video_record modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// close video_record modules (snapshot)
	if (g_snapshoten) {
		ret = close_module_2(&stream[1]);
		if (ret != HD_OK) {
			printf("close2 fail=%d\n", ret);
		}
	}

	// close video_record modules (sub)
	if (g_suben) {
		ret = close_module_3(&stream[2]);
		if (ret != HD_OK) {
			printf("close3 fail=%d\n", ret);
		}
	}

	#if (USE_AI==ENABLE)
	// close AI modules
	ret = close_module_4(&stream[3]);
	if (ret != HD_OK) {
		printf("close4 fail=%d\n", ret);
	}
	#endif

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return 0;
}
