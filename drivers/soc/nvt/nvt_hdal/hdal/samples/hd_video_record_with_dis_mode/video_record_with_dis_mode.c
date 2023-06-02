/**
	@brief Sample code of video record with mode and pxlfmt.\n

	@file video_record_with_mode.c

	@author Boyan Huang

	@ingroup mhdal

	@note This file is modified from video_record.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_record_with_dis_mode, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)


#define VENDOR_AI_CFG 					0x000f0000  //ai

#define HD_VIDEOPROC_CFG				0x000f0000	//vprc
#define HD_VIDEOPROC_CFG_STRIP_MASK		0x00000007  //vprc stripe rule mask: (default 0)
#define HD_VIDEOPROC_CFG_STRIP_LV1		0x00000000  //vprc "0: cut w>1280, GDC =  on, 2D_LUT off after cut (LL slow)
#define HD_VIDEOPROC_CFG_STRIP_LV2		0x00010000  //vprc "1: cut w>2048, GDC = off, 2D_LUT off after cut (LL fast)
#define HD_VIDEOPROC_CFG_STRIP_LV3		0x00020000  //vprc "2: cut w>2688, GDC = off, 2D_LUT off after cut (LL middle)(2D_LUT best)
#define HD_VIDEOPROC_CFG_STRIP_LV4		0x00030000  //vprc "3: cut w> 720, GDC =  on, 2D_LUT off after cut (LL not allow)(GDC best)
#define HD_VIDEOPROC_CFG_DISABLE_GDC	HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_LL_FAST		HD_VIDEOPROC_CFG_STRIP_LV2
#define HD_VIDEOPROC_CFG_2DLUT_BEST		HD_VIDEOPROC_CFG_STRIP_LV3
#define HD_VIDEOPROC_CFG_GDC_BEST		HD_VIDEOPROC_CFG_STRIP_LV4

///////////////////////////////////////////////////////////////////////////////

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

//DIS: YUV scale up
#define YUV_DIS_RATIO 1100
#define VDO_DIS_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(ALIGN_CEIL_16(((w) * YUV_DIS_RATIO + 1000) / 1000), (((h) * YUV_DIS_RATIO + 1000) / 1000), pxlfmt))

#define HD_VIDEOPROC_PIPE_OUT_DIS 0x800

///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT     HD_VIDEO_PXLFMT_RAW12
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

#define RESOLUTION_SET  0 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR) 4: (PATTERN_GEN)
#if ( RESOLUTION_SET == 0)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif (RESOLUTION_SET == 1)
#define VDO_SIZE_W      2592
#define VDO_SIZE_H      1944
#elif ( RESOLUTION_SET == 2)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 3)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 4)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#endif

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_SHDR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_DEFOG

static UINT32 g_shdr = 0; //fixed
static UINT32 g_capbind = 0;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
static UINT32 g_capfmt = 0; //0:RAW, 1:RAW-compress
static UINT32 g_prcbind = 0;  //0:D2D, 1:lowlatency, 2: one-buf, 0xff: no-bind
static UINT32 g_prcfmt = 0; //0:YUV, 1:YUV-compress
static UINT32 g_prccfg = 0; //0(STRP_LV1), 1(STRP_LV2), 2(STRP_LV3), 3(GDC_OFF), 4(AI_CNN_ON)

///////////////////////////////////////////////////////////////////////////////


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
                if (g_capfmt == HD_VIDEO_PXLFMT_NRX12)
                        mem_cfg.pool_info[0].blk_size += VDO_NRX_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H);
                else
                        mem_cfg.pool_info[0].blk_size += VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, g_capfmt);

		mem_cfg.pool_info[0].blk_cnt = 1;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	} else {
		//not support
		mem_cfg.pool_info[0].blk_size = 0;
		mem_cfg.pool_info[0].blk_cnt = 0;
		mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	}
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	if ((g_prcbind == 0) || (g_prcbind == 0xff)) {
		//normal
		mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_DIS_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
		mem_cfg.pool_info[1].blk_cnt = 3+1; //venc release blk is slow, depend on its waiting list
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

    #if (RESOLUTION_SET == 0)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	printf("Using nvt_sen_imx290\n");
	#elif (RESOLUTION_SET == 1)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	printf("Using nvt_sen_os05a10\n");
	#elif (RESOLUTION_SET == 2)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	printf("Using nvt_sen_os02k10\n");
	#elif (RESOLUTION_SET == 3)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	printf("Using nvt_sen_ar0237ir\n");
	#elif (RESOLUTION_SET == 4)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	printf("Using sie pattern gen\n");
	#endif


    if(RESOLUTION_SET == 3) {
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2) {
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		printf("Using g_shdr_mode\n");
	} else {
		#if (RESOLUTION_SET == 0)
		printf("Using imx290\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 1)
		printf("Using OS052A\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 2)
		printf("Using OS02K10\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#endif
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
		#if (RESOLUTION_SET == 0)
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 1)
			printf("Using OS052A or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 2)
			printf("Using OS02K10 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		#if (RESOLUTION_SET == 4) // pattern gen
		video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		#else
		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		#endif
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
		video_out_param.pxlfmt = g_capfmt;
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
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL | HD_VIDEOPROC_PIPE_OUT_DIS;
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0;
		} else {
			video_cfg_param.isp_id = 1;
		}

		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = g_capfmt;
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

	video_ctrl_param.func = 0;
	video_ctrl_param.ref_path_3dnr = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	video_ll_param.delay_trig_lowlatency = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_LL_CONFIG, &video_ll_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = g_prcfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 0;
		if (g_prcbind == 0xff) //no-bind mode
			video_out_param.depth = 1;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = g_prcfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 0;
		if (g_prcbind == 0xff) //no-bind mode
			video_out_param.depth = 1;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (g_prcbind == 1)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_LOWLATENCY; //enable low-latency
		if (g_prcbind == 2)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
		video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_DIS;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

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
		video_in_param.pxl_fmt = g_prcfmt;
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
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
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
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
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

	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
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
static char file_path_main[64] = {0};

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;
	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	//------ wait flow_start ------
	while (p_stream0->enc_enter == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	//----- open output files -----
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		//pull data
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode

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

	// mummap for bs buffer
	if (vir_addr_main) hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	// close output file
	if (f_out_main) fclose(f_out_main);

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[1] = {0}; //0: main stream
	UINT32 enc_type = 0;
	HD_DIM main_dim;

	if (argc == 1 || argc > 7) {
		printf("Usage: <enc-type> <cap_out_bind> <cap_out_fmt> <prc_out_bind> <prc_out_fmt>.\r\n");
		printf("Help:\r\n");
		printf("  <enc_type> : 0(H265), 1(H264), 2(MJPG)\r\n");
		printf("  <cap_out_bind>: 0(D2D Mode), 1(Direct Mode), 2(OneBuf Mode), x(no-bind)\r\n");
		printf("  <cap_out_fmt> : 0(RAW12),  1(NRX12 - RAW compress)\r\n");
		printf("  <prc_out_bind>: 0(D2D Mode), 1(LowLatency Mode), 2(OneBuf Mode), x(no-bind)\r\n");
		printf("  <prc_out_fmt> : 0(YUV420), 1(NVX - YUV compress)\r\n");
		printf("  <prc_syscfg> : 0(STRP_LV1), 1(STRP_LV2), 2(STRP_LV3), 3(STRP_LV4), 4(GDC_OFF), 5(AI_CNN_ON)\r\n");
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
		g_prccfg = atoi(argv[6]);
		printf("PRC-CFG %d\r\n", g_prccfg);
		if(g_prccfg > 5) {
			printf("error: not support PRC-CFG ! (%d) \r\n", g_prccfg);
			return 0;
		}
	}
    snprintf(file_path_main, 64, "/mnt/sd/dump_bs_main_%lu_%lu_%x_%lu_%x_%lu.dat", enc_type, g_capbind,(unsigned int)g_capfmt,g_prcbind,(unsigned int)g_prcfmt,g_prccfg);
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

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// set videoproc sysconfig
	if (g_prccfg == 0) {
		hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_STRIP_LV1, 0); //default
	} else if (g_prccfg == 1) {
		hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_STRIP_LV2, 0);
	} else if (g_prccfg == 2) {
		hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_STRIP_LV3, 0);
	} else if (g_prccfg == 3) {
		hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_STRIP_LV4, 0);
	} else if (g_prccfg == 4) {
		hd_common_sysconfig(HD_VIDEOPROC_CFG, 0, HD_VIDEOPROC_CFG_DISABLE_GDC, 0);
	} else if (g_prccfg == 5) {
		hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine
	}

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
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
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
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
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	// set videoproc parameter (main)
	if (g_prcbind == 0xff) //no-bind mode
		ret = set_proc_param(stream[0].proc_path, &main_dim); //must set vprc's out dim
	else
		ret = set_proc_param(stream[0].proc_path, NULL); //keep vprc's out dim = {0,0}, it means auto sync from venc's in dim
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
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoenc_start(stream[0].enc_path);

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

    sleep(1);
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

exit:
	// close video_record modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

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
