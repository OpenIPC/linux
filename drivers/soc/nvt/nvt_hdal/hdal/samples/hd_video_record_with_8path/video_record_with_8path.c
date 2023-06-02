/**
	@brief Sample code of video record with 8 streams with two pass video process.\n

	@file video_record_with_8path.c

	@author Jeah Yen

	@ingroup mhdal

	@note This file is modified from video_record_with_2proc.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_record_with_8path, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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

///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W	128
#define YOUT_WIN_NUM_H	128
#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL		1 //0: full, 1: subsample 1/2

//2M
#define VDO_SIZE_W     	1920
#define VDO_SIZE_H     	1080
#define MAIN_BR			(2 * 1024 * 1024)

//1M
#define SUB_VDO_SIZE_W	640
#define SUB_VDO_SIZE_H	480
#define SUB_BR			(1 * 1024 * 1024)

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 3;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (0)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	// config common pool (1)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	// config common pool (2)
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[3].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[3].blk_cnt = 3;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	// config common pool (4)
	mem_cfg.pool_info[4].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[4].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[4].blk_cnt = 3;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;
	// config common pool (0)
	mem_cfg.pool_info[5].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[5].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[5].blk_cnt = 3;
	mem_cfg.pool_info[5].ddr_id = DDR_ID0;
	// config common pool (1)
	mem_cfg.pool_info[6].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[6].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[6].blk_cnt = 3;
	mem_cfg.pool_info[6].ddr_id = DDR_ID0;
	// config common pool (2)
	mem_cfg.pool_info[7].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[7].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[7].blk_cnt = 3;
	mem_cfg.pool_info[7].ddr_id = DDR_ID0;
	// config common pool (4)
	mem_cfg.pool_info[8].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[8].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, SUB_VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[8].blk_cnt = 3;
	mem_cfg.pool_info[8].ddr_id = DDR_ID0;

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

#if 0
static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}
#endif

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
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
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_OUT_ID ctrl, HD_PATH_ID *p_video_proc_ctrl, HD_VIDEOPROC_PIPE pipe, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, ctrl, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = pipe;
		video_cfg_param.isp_id = (pipe == HD_VIDEOPROC_PIPE_SCALE) ? HD_ISP_DONT_CARE : 0;
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = (pipe == HD_VIDEOPROC_PIPE_SCALE) ? HD_VIDEO_PXLFMT_YUV420 : CAP_OUT_FMT;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

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
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 1;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};

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
	HD_DIM  proc0_max_dim;
	HD_DIM  proc0_dim0;
	HD_DIM  proc0_dim1;
	HD_DIM  proc0_dim2;
	HD_DIM  proc0_dim4;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc0_syscaps;
	HD_PATH_ID proc0_ctrl;
	HD_PATH_ID proc0_path0;
	HD_PATH_ID proc0_path1;
	HD_PATH_ID proc0_path2;
	HD_PATH_ID proc0_path4;

	HD_DIM  proc1_max_dim;
	HD_DIM  proc1_dim0;
	HD_DIM  proc1_dim1;
	HD_DIM  proc1_dim2;
	HD_DIM  proc1_dim4;

	// (3)
	HD_VIDEOPROC_SYSCAPS proc1_syscaps;
	HD_PATH_ID proc1_ctrl;
	HD_PATH_ID proc1_path0;
	HD_PATH_ID proc1_path1;
	HD_PATH_ID proc1_path2;
	HD_PATH_ID proc1_path4;

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (4)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path[8];

	// (5) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

	// (5) user pull
	pthread_t  relay_thread_id;
	UINT32     proc_exit;

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

static HD_RESULT open_module(VIDEO_RECORD *p_stream, HD_DIM* p_proc0_max_dim, HD_DIM* p_proc1_max_dim)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set 1st videoproc config
	ret = set_proc_cfg(HD_VIDEOPROC_0_CTRL, &p_stream->proc0_ctrl, HD_VIDEOPROC_PIPE_RAWALL, p_proc0_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set 2nd videoproc config
	ret = set_proc_cfg(HD_VIDEOPROC_1_CTRL, &p_stream->proc1_ctrl, HD_VIDEOPROC_PIPE_SCALE, p_proc1_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc0_path0)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc0_path1)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_2, &p_stream->proc0_path2)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_4, &p_stream->proc0_path4)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1, &p_stream->enc_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_2, HD_VIDEOENC_0_OUT_2, &p_stream->enc_path[2])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_3, HD_VIDEOENC_0_OUT_3, &p_stream->enc_path[3])) != HD_OK)
		return ret;

	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &p_stream->proc1_path0)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_1, &p_stream->proc1_path1)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_2, &p_stream->proc1_path2)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_4, &p_stream->proc1_path4)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_4, HD_VIDEOENC_0_OUT_4, &p_stream->enc_path[4])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_5, HD_VIDEOENC_0_OUT_5, &p_stream->enc_path[5])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_6, HD_VIDEOENC_0_OUT_6, &p_stream->enc_path[6])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_7, HD_VIDEOENC_0_OUT_7, &p_stream->enc_path[7])) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_RECORD *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc0_path0)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc0_path1)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc0_path2)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc0_path4)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[0])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[1])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[2])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[3])) != HD_OK)
		return ret;

	if ((ret = hd_videoproc_close(p_stream->proc1_path0)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc1_path1)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc1_path2)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc1_path4)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[4])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[5])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[6])) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path[7])) != HD_OK)
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

static void *relay_yuv_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
#if 0
	VIDEO_RECORD* p_stream1 = p_stream0 + 1;
#endif
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	//printf("\r\relay main frame  ....\r\n");

	//--------- pull data test ---------
	while (p_stream0->proc_exit == 0) {
		{
			//pull data
			ret = hd_videoproc_pull_out_buf(p_stream0->proc0_path0, &video_frame, -1); // -1: blocking mode
			if (ret != HD_OK) {
				printf("pull_out [0] error %d!!\r\n\r\n", ret);
				continue;
			}
			//push data
			ret = hd_videoproc_push_in_buf(p_stream0->proc1_path0, &video_frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				printf("push_in [1] error %d!!\r\n", ret);
			}
			//release data
			ret = hd_videoproc_release_out_buf(p_stream0->proc0_path0, &video_frame);
			if (ret != HD_OK) {
				printf("release_out [0] error %d!!\r\n\r\n", ret);
			}
		}

		//usleep(30000); // sleep 30 ms
	}

	return 0;
}

typedef struct _VIDEO_FILE {
	UINT32 enc_path;
	HD_VIDEOENC_BUFINFO phy_buf;
	UINT32 vir_addr;
	char file_path[32];
	FILE *fd;
} VIDEO_FILE;

static int video_file_open(VIDEO_FILE* fh, UINT32 enc_path, char* filename)
{
	if (!fh) return -1;

	fh->fd = 0;
	fh->enc_path = enc_path;
	strncpy(fh->file_path, filename, sizeof(fh->file_path)-1);
	fh->file_path[sizeof(fh->file_path)-1] = '\0';

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(enc_path, HD_VIDEOENC_PARAM_BUFINFO, &(fh->phy_buf));

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	fh->vir_addr = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, fh->phy_buf.buf_info.phy_addr, fh->phy_buf.buf_info.buf_size);

	//----- open output files -----
	if ((fh->fd = fopen(fh->file_path, "wb")) == NULL) {

		// mummap for bs buffer
		if (fh->vir_addr) hd_common_mem_munmap((void *)(fh->vir_addr), fh->phy_buf.buf_info.buf_size);

		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", fh->file_path);
		return -2;
	} else {
		printf("\r\ndump bitstream to file (%s) ....\r\n", fh->file_path);
	}
	return 0;
}

static int video_file_push_in_buf(VIDEO_FILE* fh, HD_VIDEOENC_BS* p_data_push)
{
	UINT32 j;
	if (!fh) return -1;
	if (!fh->fd) return -2;

	#define PHY2VIRT_MAIN(pa) ((fh->vir_addr) + (pa - (fh->phy_buf.buf_info.phy_addr)))
	for (j=0; j< p_data_push->pack_num; j++) {
		UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(p_data_push->video_pack[j].phy_addr);
		UINT32 len = p_data_push->video_pack[j].size;
		if (fh->fd) fwrite(ptr, 1, len, fh->fd);
		if (fh->fd) fflush(fh->fd);
	}
	return 0;
}

static int video_file_close(VIDEO_FILE* fh)
{
	if (!fh) return -1;
	if (!fh->fd) return -2;

	// mummap for bs buffer
	if (fh->vir_addr) hd_common_mem_munmap((void *)(fh->vir_addr), fh->phy_buf.buf_info.buf_size);

	// close output file
	fclose(fh->fd);
	fh->fd = 0;

	return 0;
}

static char* fn[8] = {
	"/mnt/sd/dump_bs_stream0.dat",
	"/mnt/sd/dump_bs_stream1.dat",
	"/mnt/sd/dump_bs_stream2.dat",
	"/mnt/sd/dump_bs_stream3.dat",
	"/mnt/sd/dump_bs_stream4.dat",
	"/mnt/sd/dump_bs_stream5.dat",
	"/mnt/sd/dump_bs_stream6.dat",
	"/mnt/sd/dump_bs_stream7.dat"
};

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	HD_VIDEOENC_POLL_LIST poll_list[8];
	VIDEO_FILE vfile[8];
	int i;

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	for(i=0;i<8;i++)
		video_file_open(&vfile[i], p_stream0->enc_path[i], fn[i]);

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	for(i=0;i<8;i++) {
		poll_list[i].path_id = p_stream0->enc_path[i];
	}

	while (p_stream0->enc_exit == 0) {
		if (HD_OK == hd_videoenc_poll_list(poll_list, 8, -1)) {  // -1 = blocking mode
			for(i=0;i<8;i++) {
				if (TRUE == poll_list[i].revent.event) {
					//pull data
					ret = hd_videoenc_pull_out_buf(p_stream0->enc_path[i], &data_pull, 0);
					if (ret != HD_OK)
						continue;
					//push data
					video_file_push_in_buf(&vfile[i], &data_pull);
					// release data
					ret = hd_videoenc_release_out_buf(p_stream0->enc_path[i], &data_pull);
					if (ret != HD_OK) {
						printf("enc_release error=%d !!\r\n", ret);
					}
				}
			}
		}
	}

	for(i=0;i<8;i++)
		video_file_close(&vfile[i]);

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[1] = {0}; //0: main stream
	UINT32 enc_type = 0;
	HD_DIM main_dim;
	HD_DIM sub_dim;

	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
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

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	sub_dim.w = SUB_VDO_SIZE_W;
	sub_dim.h = SUB_VDO_SIZE_H;

	// open video_record modules (0)
	stream[0].proc0_max_dim = main_dim;
	// open video_record modules (1)
	stream[0].proc1_max_dim = sub_dim;
	ret = open_module(&stream[0], &stream[0].proc0_max_dim, &stream[0].proc1_max_dim);
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
	stream[0].cap_dim = main_dim;
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (0)
	ret = set_proc_param(stream[0].proc0_path0, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc0_path1, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc0_path2, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc0_path4, &main_dim); // NOTE : HW limit, this path MUST same as sensor resolution
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	// set videoproc parameter (1)
	ret = set_proc_param(stream[0].proc1_path0, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc1_path1, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc1_path2, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	ret = set_proc_param(stream[0].proc1_path4, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (0)
	stream[0].enc_max_dim.w = sub_dim.w;
	stream[0].enc_max_dim.h = sub_dim.h;
	ret = set_enc_cfg(stream[0].enc_path[0], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[1], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[2], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[3], &main_dim, MAIN_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	// set videoenc config (1)
	ret = set_enc_cfg(stream[0].enc_path[4], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[5], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[6], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_cfg(stream[0].enc_path[7], &stream[0].enc_max_dim, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (0)
	ret = set_enc_param(stream[0].enc_path[0], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[1], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[2], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[3], &main_dim, enc_type, MAIN_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	// set videoenc parameter (1)
	ret = set_enc_param(stream[0].enc_path[4], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[5], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[6], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}
	ret = set_enc_param(stream[0].enc_path[7], &sub_dim, enc_type, SUB_BR);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// bind video_record modules (0)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_1, HD_VIDEOENC_0_IN_1);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_2, HD_VIDEOENC_0_IN_2);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_4, HD_VIDEOENC_0_IN_3);
	// bind video_record modules (1)
	hd_videoproc_bind(HD_VIDEOPROC_1_OUT_0, HD_VIDEOENC_0_IN_4);
	hd_videoproc_bind(HD_VIDEOPROC_1_OUT_1, HD_VIDEOENC_0_IN_5);
	hd_videoproc_bind(HD_VIDEOPROC_1_OUT_2, HD_VIDEOENC_0_IN_6);
	hd_videoproc_bind(HD_VIDEOPROC_1_OUT_4, HD_VIDEOENC_0_IN_7);

	// create relay_yuv_thread (pull_out frame and push_in frame)
	ret = pthread_create(&stream[0].relay_thread_id, NULL, relay_yuv_thread, (void *)stream);
	if (ret < 0) {
		printf("create relay thread failed");
		goto exit;
	}
	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	// start video_record modules (0)
	hd_videocap_start(stream[0].cap_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoproc_start(stream[0].proc0_path0);
	hd_videoproc_start(stream[0].proc0_path1);
	hd_videoproc_start(stream[0].proc0_path2);
	hd_videoproc_start(stream[0].proc0_path4);
	hd_videoenc_start(stream[0].enc_path[0]);
	hd_videoenc_start(stream[0].enc_path[1]);
	hd_videoenc_start(stream[0].enc_path[2]);
	hd_videoenc_start(stream[0].enc_path[3]);
	// start video_record modules (1)
	hd_videoproc_start(stream[0].proc1_path0);
	hd_videoproc_start(stream[0].proc1_path1);
	hd_videoproc_start(stream[0].proc1_path2);
	hd_videoproc_start(stream[0].proc1_path4);
	hd_videoenc_start(stream[0].enc_path[4]);
	hd_videoenc_start(stream[0].enc_path[5]);
	hd_videoenc_start(stream[0].enc_path[6]);
	hd_videoenc_start(stream[0].enc_path[7]);

	// let encode_thread start to work
	stream[0].flow_start= 1;

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;
			stream[0].proc_exit = 1;
			// quit program
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	// stop video_record modules (0)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc0_path0);
	hd_videoproc_stop(stream[0].proc0_path1);
	hd_videoproc_stop(stream[0].proc0_path2);
	hd_videoproc_stop(stream[0].proc0_path4);
	hd_videoenc_stop(stream[0].enc_path[0]);
	hd_videoenc_stop(stream[0].enc_path[1]);
	hd_videoenc_stop(stream[0].enc_path[2]);
	hd_videoenc_stop(stream[0].enc_path[3]);
	// stop video_record modules (1)
	hd_videoproc_stop(stream[0].proc1_path0);
	hd_videoproc_stop(stream[0].proc1_path1);
	hd_videoproc_stop(stream[0].proc1_path2);
	hd_videoproc_stop(stream[0].proc1_path4);
	hd_videoenc_stop(stream[0].enc_path[4]);
	hd_videoenc_stop(stream[0].enc_path[5]);
	hd_videoenc_stop(stream[0].enc_path[6]);
	hd_videoenc_stop(stream[0].enc_path[7]);

	// unbind video_record modules (0)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_1);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_2);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_4);
	// unbind video_record modules (1)
	hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_1);
	hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_2);
	hd_videoproc_unbind(HD_VIDEOPROC_1_OUT_4);

	// destroy relay thread
	pthread_join(stream[0].relay_thread_id, NULL);
	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);

exit:

	// close video_record modules
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
