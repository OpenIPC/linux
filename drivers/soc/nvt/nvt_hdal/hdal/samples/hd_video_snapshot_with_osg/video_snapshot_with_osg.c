/**
	@brief Sample code of video snapshot with osg.\n

	@file video_snapshot_with_osg.c

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_snapshot_with_osg, argc, argv)
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

#define VDO_SIZE_W		1920
#define VDO_SIZE_H		1080

#define USE_MOSAIC    0
#define STAMP_WIDTH   1000
#define STAMP_HEIGHT  200

#define MAX_SNAPSHOT_SIZE  VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420) / 5    // YUV420, compress ratio = 1/5

///////////////////////////////////////////////////////////////////////////////

static int query_osg_buf_size(void)
{
	HD_VIDEO_FRAME frame = {0};
	int stamp_size;

	frame.sign   = MAKEFOURCC('O','S','G','P');
	frame.dim.w  = STAMP_WIDTH;
	frame.dim.h  = STAMP_HEIGHT;
	frame.pxlfmt = HD_VIDEO_PXLFMT_ARGB4444;

	//get required buffer size for a single image
	stamp_size = hd_common_mem_calc_buf_size(&frame);
	if(!stamp_size){
		printf("fail to query buffer size\n");
		return -1;
	}

	//ping pong buffer needs double size
	stamp_size *= 2;

	return stamp_size;
}

static HD_RESULT mem_init(UINT32 stamp_size)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	if(!stamp_size){
		printf("size of osg buffer is unknown\n");
		return -1;
	}

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 3;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	// config common pool (osg)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_OSG_POOL;
	mem_cfg.pool_info[2].blk_size = stamp_size;
	mem_cfg.pool_info[2].blk_cnt = 1;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	// config common pool (snapshot bs)
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[3].blk_size = MAX_SNAPSHOT_SIZE; // for JPEG snapshot bs
	mem_cfg.pool_info[3].blk_cnt = 1;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;


	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static unsigned int mem_alloc(UINT32 stamp_size, UINT32 *stamp_blk, UINT32 *stamp_pa)
{
	UINT32                  pa;
	HD_COMMON_MEM_VB_BLK    blk;

	if(!stamp_size){
		printf("stamp_size is unknown\n");
		return -1;
	}

	//get osd stamps' block
	blk = hd_common_mem_get_block(HD_COMMON_MEM_OSG_POOL, stamp_size, DDR_ID0);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail\r\n", blk);
		return -1;
	}

	if(stamp_blk)
		*stamp_blk = blk;

	//translate stamp block to physical address
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("blk2pa fail, blk = 0x%x\r\n", blk);
		return -1;
	}

	if(stamp_pa)
		*stamp_pa = pa;

	return 0;
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
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
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
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
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

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;
#if (USE_MOSAIC == 1)
		video_cfg_param.ctrl_max.func = HD_VIDEOPROC_FUNC_MOSAIC;
#else
		video_cfg_param.ctrl_max.func = 0;
#endif
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

#if (USE_MOSAIC == 1)
	video_ctrl_param.func = HD_VIDEOPROC_FUNC_MOSAIC;
#else
	video_ctrl_param.func = 0;
#endif
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
		video_path_config.max_mem.source_output   = TRUE;
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
			video_out_param.h26x.source_output = 1;
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
			video_out_param.h26x.source_output = 1;
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
			video_out_param.jpeg.image_quality = 70;
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

static int set_vp_stamp_param(HD_PATH_ID stamp_path, UINT32 stamp_pa, UINT32 stamp_size, unsigned short *novatek_logo)
{
	HD_OSG_STAMP_BUF  buf;
	HD_OSG_STAMP_IMG  img;
	HD_OSG_STAMP_ATTR attr;

	if(!stamp_pa){
		printf("stamp buffer is not allocated\n");
		return -1;
	}

	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = stamp_pa;
	buf.size      = stamp_size;

	if(hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_BUF, &buf) != HD_OK){
		printf("fail to set stamp buffer\n");
		return -1;
	}

	memset(&img, 0, sizeof(HD_OSG_STAMP_IMG));

	img.fmt        = HD_VIDEO_PXLFMT_ARGB4444;
	img.dim.w      = STAMP_WIDTH;
	img.dim.h      = STAMP_HEIGHT;
	img.p_addr     = (UINT32)novatek_logo;

	if(hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_IMG, &img) != HD_OK){
		printf("fail to set stamp image\n");
		return -1;
	}

	memset(&attr, 0, sizeof(HD_OSG_STAMP_ATTR));

	attr.position.x = 0;
	attr.position.y = 0;
	attr.alpha      = 255;
	attr.layer      = 0;
	attr.region     = 0;

	return hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_ATTR, &attr);
}

static int set_enc_stamp_param(HD_PATH_ID stamp_path, UINT32 stamp_pa, UINT32 stamp_size, unsigned short *novatek_logo)
{
	HD_OSG_STAMP_BUF  buf;
	HD_OSG_STAMP_IMG  img;
	HD_OSG_STAMP_ATTR attr;

	if(!stamp_pa){
		printf("stamp buffer is not allocated\n");
		return -1;
	}

	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = stamp_pa;
	buf.size      = stamp_size;

	if(hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf) != HD_OK){
		printf("fail to set stamp buffer\n");
		return -1;
	}

	memset(&img, 0, sizeof(HD_OSG_STAMP_IMG));

	img.fmt        = HD_VIDEO_PXLFMT_ARGB4444;
	img.dim.w      = STAMP_WIDTH;
	img.dim.h      = STAMP_HEIGHT;
	img.p_addr     = (UINT32)novatek_logo;

	if(hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_IMG, &img) != HD_OK){
		printf("fail to set stamp image\n");
		return -1;
	}

	memset(&attr, 0, sizeof(HD_OSG_STAMP_ATTR));

	attr.position.x = 0;
	attr.position.y = 240;
	attr.alpha      = 255;
	attr.layer      = 0;
	attr.region     = 0;

	return hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_ATTR, &attr);
}

static int set_vp_mask_param(HD_PATH_ID mask_path)
{
	HD_OSG_MASK_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

	attr.position[0].x = 500;
	attr.position[0].y = 0;
	attr.position[1].x = 600;
	attr.position[1].y = 0;
	attr.position[2].x = 600;
	attr.position[2].y = 120;
	attr.position[3].x = 500;
	attr.position[3].y = 120;
	attr.type          = HD_OSG_MASK_TYPE_SOLID;
	attr.alpha         = 255;
	attr.color         = 0x00FF0000;

	return hd_videoproc_set(mask_path, HD_VIDEOPROC_PARAM_IN_MASK_ATTR, &attr);
}

#if (USE_MOSAIC == 1)
static int set_vp_mosaic_param(HD_PATH_ID mosaic_path)
{
	HD_OSG_MOSAIC_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MOSAIC_ATTR));

	attr.position[0].x = 700;
	attr.position[0].y = 0;
	attr.position[1].x = 900;
	attr.position[1].y = 0;
	attr.position[2].x = 900;
	attr.position[2].y = 120;
	attr.position[3].x = 700;
	attr.position[3].y = 120;
	attr.mosaic_blk_w  = 64;
	attr.mosaic_blk_h  = 64;

	return hd_videoproc_set(mosaic_path, HD_VIDEOPROC_PARAM_IN_MOSAIC_ATTR, &attr);
}
#endif

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_RECORD {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (4) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

	// (5) user trigger snapshot
	pthread_t  snap_thread_id;
	UINT32     do_snap;
	UINT32     shot_count;

	// osg
	HD_PATH_ID vp_mask_path;
	HD_PATH_ID vp_stamp_path;
	HD_PATH_ID enc_stamp_path;
#if (USE_MOSAIC == 1)
	HD_PATH_ID vp_mosaic_path;
#endif

	UINT32 stamp_blk;
	UINT32 stamp_pa;
	UINT32 stamp_size;
	unsigned short *novatek_logo;

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
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
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

	if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_0, &p_stream->vp_mask_path)) != HD_OK)
		return ret;
#if (USE_MOSAIC == 1)
	if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_MASK_1, &p_stream->vp_mosaic_path)) != HD_OK)
		return ret;
#endif
	if((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_STAMP_0, &p_stream->vp_stamp_path)) != HD_OK)
		return ret;
	if((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_STAMP_0, &p_stream->enc_stamp_path)) != HD_OK)
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
	if((ret = hd_videoproc_close(p_stream->vp_mask_path)) != HD_OK)
		return ret;
#if (USE_MOSAIC == 1)
	if((ret = hd_videoproc_close(p_stream->vp_mosaic_path)) != HD_OK)
		return ret;
#endif
	if((ret = hd_videoproc_close(p_stream->vp_stamp_path)) != HD_OK)
		return ret;
	if((ret = hd_videoenc_close(p_stream->enc_stamp_path)) != HD_OK)
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

static HD_RESULT take_snapshot(VIDEO_RECORD* p_stream0)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_VB_BLK blk = 0;
	UINT32 blk_size = MAX_SNAPSHOT_SIZE;
	UINT32 pa = 0, va = 0;
	HD_H26XENC_TRIG_SNAPSHOT snap_param = {0};

	//--- Get memory ---
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, DDR_ID0); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("userqp, get block fail (0x%x).. !!.\r\n", blk);
		goto quit;
	}

	pa = hd_common_mem_blk2pa(blk); // Get physical addr
	if (pa == 0) {
		printf("blk2pa fail, blk = 0x%x\r\n", blk);
		goto rel_blk;
	}

	// mmap
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
	if (va == 0) {
		printf("Error: mmap fail !! pa = 0x%x, blk = 0x%x\r\n", pa, blk);
		goto rel_blk;
	}

	// trigger snapshot
	snap_param.phy_addr      = pa;
	snap_param.size          = blk_size;
	snap_param.image_quality = 50;

	ret = hd_videoenc_set(p_stream0->enc_path, HD_VIDEOENC_PARAM_OUT_TRIG_SNAPSHOT, &snap_param);
	if (ret != HD_OK) {
		goto snapshot_fail;
	}
#if 0
	{
		unsigned char *ptr = (unsigned char*)va;
		printf("snapshot got , size = %u, data = %02x %02x %02x %02x\r\n", snap_param.size, ptr[0], ptr[1], ptr[2], ptr[3]);
	}
#endif
	// write file
	{
		char file_path[32] = {0};
		snprintf(file_path, 32, "/mnt/sd/dump_bs_snap%lu.jpg", p_stream0->shot_count);

		FILE *f_out = fopen(file_path, "wb");
		if (f_out) {
			if (snap_param.size) {
				if (f_out) fwrite((void *)va, 1, snap_param.size, f_out);
				if (f_out) fflush(f_out);

				p_stream0->shot_count++;
				printf("dump snapshot bitstream to file (%s) ....\r\n", file_path);
			}
			fclose(f_out);
		}
	}

	// mummap
	if (va) hd_common_mem_munmap((void *)va, blk_size);

rel_blk:
	//--- Release memory ---
	if (blk) {
		hd_common_mem_release_block(blk);
	}

	return HD_OK;

snapshot_fail:
	// mummap
	if (va) hd_common_mem_munmap((void *)va, blk_size);

	//--- Release memory ---
	if (blk) {
		hd_common_mem_release_block(blk);
	}
quit:
	printf("snapshot failed !!\r\n");
	return HD_ERR_NG;
}

static void *snapshot_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		if (p_stream0->do_snap) {
			p_stream0->do_snap = 0;
			printf("trigger snapshot..\r\n");
			take_snapshot(p_stream0);
		}
		usleep(30000);
	}

	return 0;
}

static void *encode_thread(void *arg)
{
	VIDEO_RECORD* p_stream0 = (VIDEO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

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

	printf("\r\nif you want to snapshot, enter \"s\" to trigger !!\r\n");
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

static int init_novatek_logo(unsigned short *novatek_logo)
{
	int fd, ret;

	fd = open("/mnt/sd/video_frm_1000_200_1_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		printf("video_frm_1000_200_1_argb4444.dat is in SDK/code/hdal/samples/pattern/\n");
		return -1;
	}

	ret = read(fd, novatek_logo, (STAMP_WIDTH * STAMP_HEIGHT * sizeof(unsigned short)));
	close(fd);

	if(ret != (STAMP_WIDTH * STAMP_HEIGHT * sizeof(unsigned short))){
		printf("fail to read /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		return -1;
	}else
		return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_RECORD stream[1] = {0}; //0: main stream
	UINT32 enc_type = 0;
	HD_DIM main_dim;

	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}

	//allocate logo buffer
	stream[0].novatek_logo = malloc(STAMP_WIDTH * STAMP_HEIGHT * sizeof(unsigned short));
	if(!stream[0].novatek_logo){
		printf("fail to allocate logo buffer\n");
		return -1;
	}

	//load novatek logo from sd card
	if(init_novatek_logo(stream[0].novatek_logo)){
		printf("fail to load stamp image\n");
		free(stream[0].novatek_logo);
		return -1;
	}

	// init stamp data
	stream[0].stamp_blk  = 0;
	stream[0].stamp_pa   = 0;
	stream[0].stamp_size = query_osg_buf_size();
	if(stream[0].stamp_size <= 0){
		printf("query_osg_buf_size() fail\n");
		return -1;
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init(stream[0].stamp_size);
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

	ret = mem_alloc(stream[0].stamp_size, &(stream[0].stamp_blk), &(stream[0].stamp_pa));
	if(ret){
		printf("fail to allocate stamp buffer\n");
		goto exit;
	}

	//setup enc stamp parameter
	if(set_enc_stamp_param(stream[0].enc_stamp_path, stream[0].stamp_pa, stream[0].stamp_size, stream[0].novatek_logo)){
		printf("fail to set enc stamp\r\n");
		goto exit;
	}

	//render enc stamp
	ret = hd_videoenc_start(stream[0].enc_stamp_path);
	if (ret != HD_OK) {
		printf("start enc stamp fail=%d\n", ret);
		goto exit;
	}

	//setup vp mask parameter
	if(set_vp_mask_param(stream[0].vp_mask_path)){
		printf("fail to set vp mask attr\r\n");
		goto exit;
	}

	//render vp mask
	ret = hd_videoproc_start(stream[0].vp_mask_path);
	if (ret != HD_OK) {
		printf("fail to start vp mask\n");
		goto exit;
	}

#if (USE_MOSAIC == 1)
	//setup vp mosaic parameter
	if(set_vp_mosaic_param(stream[0].vp_mosaic_path)){
		printf("fail to set vp mosaic attr\r\n");
		goto exit;
	}

	//render vp mosaic
	ret = hd_videoproc_start(stream[0].vp_mosaic_path);
	if (ret != HD_OK) {
		printf("fail to start vp mosaic\n");
		goto exit;
	}
#endif

	//setup vp stamp parameter
	if(set_vp_stamp_param(stream[0].vp_stamp_path, stream[0].stamp_pa, stream[0].stamp_size, stream[0].novatek_logo)){
		printf("fail to set vp stamp\r\n");
		goto exit;
	}

	//render vp stamp
	ret = hd_videoproc_start(stream[0].vp_stamp_path);
	if (ret != HD_OK) {
		printf("start vp stamp fail=%d\n", ret);
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
	ret = set_proc_param(stream[0].proc_path, NULL);
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

	// bind video_record modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_0);

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	// create snap_thread (trigger snapshot)
	ret = pthread_create(&stream[0].snap_thread_id, NULL, snapshot_thread, (void *)stream);
	if (ret < 0) {
		printf("create snapshot thread failed");
		goto exit;
	}

	// start video_record modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoenc_start(stream[0].enc_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();

		if (key == 's') {
			stream[0].do_snap = 1;
		}

		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;
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

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);

	// destroy snapshot thread
	pthread_join(stream[0].snap_thread_id, NULL);

	// stop video_record modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoenc_stop(stream[0].enc_path);

	// unbind video_record modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

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

	if(stream[0].stamp_blk)
		hd_common_mem_release_block(stream[0].stamp_blk);

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

	if(stream[0].novatek_logo)
		free(stream[0].novatek_logo);

	return 0;
}