/**
	@brief Sample code of video capture from proc and encode to bs.\n

	@file video_capture_flow.c

	@author Jeah Yen

	@ingroup mhdal

	@note This file is modified from video_liveview.c and video_snapshot.c.

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_capture_flow, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU      1
#define LOAD_ISP_CFG    0
#define LOAD_ISP_DTSI   0

#include "vendor_isp.h"

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)


#define RESOLUTION_SET      1 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR)

#define PHOTO_BRC_MODE      0 //1: Photo BRC mode , 0: fix quality mode

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
#define SHDR_CAP_OUT_FMT	HD_VIDEO_PXLFMT_NRX12_SHDR2
#define PRC_OUT_FMT		HD_VIDEO_PXLFMT_YUV420_NVX2
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

#if ( RESOLUTION_SET == 0)
#define VDO_SIZE_W      	1920
#define VDO_SIZE_H      	1080
#elif (RESOLUTION_SET == 1)
#define VDO_SIZE_W      	1920
#define VDO_SIZE_H      	1080
#elif ( RESOLUTION_SET == 2)
#define VDO_SIZE_W      	1920
#define VDO_SIZE_H      	1080
#elif ( RESOLUTION_SET == 3)
#define VDO_SIZE_W      	1920
#define VDO_SIZE_H      	1080
#endif

#if ( RESOLUTION_SET == 0)
#define CAP_SIZE_W		1920
#define CAP_SIZE_H		1080
#elif (RESOLUTION_SET == 1)
#define CAP_SIZE_W		2592
#define CAP_SIZE_H		1944
#elif ( RESOLUTION_SET == 2)
#define CAP_SIZE_W		1920
#define CAP_SIZE_H		1080
#elif ( RESOLUTION_SET == 3)
#define CAP_SIZE_W		1920
#define CAP_SIZE_H		1080
#endif

#define SNL_SIZE_W		720
#define SNL_SIZE_H		480

#define TNL_SIZE_W		360
#define TNL_SIZE_H		240

#if PHOTO_BRC_MODE
#define CAP_TBR			(2 * 1024 * 1024) //bits-per-sec
#else
#define CAP_TBR			(((3840 * 2160 * 3/2) /5) * 8) //fix-quality 70, assume compress ratio = 1/5, for max resolution 3840x2160
#endif
#define CAP_FIX_TBR(w,h)	((((w) * (h) * 3/2) /5) * 8) //fix-quality 70, assume compress ratio = 1/5, for max resolution w x h
#define CAP_BUF			10000 //ms

#define VCAP_ALG_FUNC 	HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VPRC_ALG_FUNC 	0

#define g_shdr_mode       0

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (raw for liveview: vcap)(raw for capture: vcap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(CAP_SIZE_W, CAP_SIZE_H,CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (yuv for liveview: vprc + vout)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	// config common pool (yuv for capture: vprc + venc)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(CAP_SIZE_W, CAP_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

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
	#endif


    if(RESOLUTION_SET == 3){
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2){
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	if (g_shdr_mode == 1) {
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
	if (g_shdr_mode == 1) {
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

	if (g_shdr_mode == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VCAP_ALG_FUNC;

	if (g_shdr_mode == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, BOOL is_pull)
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
		video_out_param.depth = is_pull; //set 1 to allow pull
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_VIDEO_PXLFMT fmt, HD_OUT_ID _out_id, BOOL is_cap)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, _out_id, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	{
		if (is_cap)
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWCAP;
		else
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		if ((HD_CTRL_ID)_out_id == HD_VIDEOPROC_0_CTRL) {
			video_cfg_param.isp_id = 0x00000000;
		} else {
			video_cfg_param.isp_id = 0x00010000;
		}
		video_cfg_param.ctrl_max.func = VPRC_ALG_FUNC;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = (p_max_dim != NULL ) ? p_max_dim->w : 0;
		video_cfg_param.in_max.dim.h = (p_max_dim != NULL ) ? p_max_dim->h : 0;
		video_cfg_param.in_max.pxlfmt = (fmt != 0) ? fmt : 0;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	video_ctrl_param.func = VPRC_ALG_FUNC;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, BOOL is_pull)
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
		video_out_param.depth = is_pull; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = 0;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = is_pull; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_CTRL_ID ctrl_id,HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, ctrl_id, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	printf("out_type=%d\r\n", out_type);

	#if 1
	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	#else
	switch(out_type){
	case 0:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs= HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi= hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	#endif
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
	printf("##devcount %d\r\n", video_out_dev.max_dev_count);

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

static HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 enc_type, UINT32 max_bitrate, UINT32 buf_ms)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};

	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		if (enc_type == 2) {
			video_path_config.max_mem.codec_type = HD_CODEC_TYPE_JPEG;
		} else {
			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		video_path_config.max_mem.enc_buf_ms = buf_ms;
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
	HD_VIDEOENC_OUT2 video_out_param = {0};

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

		if (enc_type == 2) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
#if PHOTO_BRC_MODE
			video_out_param.jpeg.image_quality = 0;
			video_out_param.jpeg.bitrate       = bitrate;
#else
			video_out_param.jpeg.image_quality = 70;
			video_out_param.jpeg.bitrate       = 0;
#endif
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM2, &video_out_param);
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

typedef struct _VIDEO_CAPTURE_SIZE {
	UINT32 w;
	UINT32 h;
} VIDEO_CAPTURE_SIZE;

#if ( RESOLUTION_SET == 0)
VIDEO_CAPTURE_SIZE cap_size[3] = {
	{1920, 1080}, //2M
	{1280, 720},
	{864, 480},
};
#elif (RESOLUTION_SET == 1)
VIDEO_CAPTURE_SIZE cap_size[3] = {
	//{2880, 2160},
	//{2592, 1936}, //5M
	{2592, 1944}, //5M
	//{1920, 1440},
	{1280, 960},
	{640, 480},
};
#elif ( RESOLUTION_SET == 2)
VIDEO_CAPTURE_SIZE cap_size[3] = {
	{1920, 1080}, //2M
	{1280, 720},
	{864, 480},
};
#elif ( RESOLUTION_SET == 3)
VIDEO_CAPTURE_SIZE cap_size[3] = {
	{1920, 1080}, //2M
	{1280, 720},
	{864, 480},
};
#endif

typedef struct _VIDEO_LIVEVIEW {

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

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
    HD_VIDEOOUT_HDMI_ID hdmi_id;

	HD_DIM  out1_max_dim;
	HD_DIM  out1_dim;

	// (4)
	HD_VIDEOOUT_SYSCAPS out1_syscaps;
	HD_PATH_ID out1_ctrl;
	HD_PATH_ID out1_path;

} VIDEO_LIVEVIEW;


typedef struct _VIDEO_CAPTURE {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;  //for capture to venc
	HD_DIM  capture_dim;

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (3) user pull and push
	UINT32 	flow_max_shot;
	UINT32 	sel_cap_size; //to select cap_size[]

	pthread_t  prc_thread_id;
	UINT32	prc_enter;
	UINT32	prc_exit;
	UINT32	prc_count;
	UINT32 	prc_loop;

	pthread_t  enc_thread_id;
	UINT32	enc_enter;
	UINT32	enc_exit;
	UINT32 	enc_count;
	UINT32 	enc_loop;

	pthread_t  save_thread_id;
	UINT32	save_enter;
	UINT32	save_exit;
	UINT32 	save_count;
	UINT32 	save_loop;

	pthread_t  flow_thread_id;
	UINT32	flow_run;
	UINT32	flow_quit;
	UINT32	flow_state;

} VIDEO_CAPTURE;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, HD_VIDEO_PXLFMT fmt, UINT32 out_type)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, fmt, HD_VIDEOPROC_0_CTRL, FALSE); //open for liveview
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(HD_VIDEOOUT_0_CTRL,&p_stream->out_ctrl, out_type, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module_2(VIDEO_CAPTURE *p_stream, HD_DIM* p_proc_max_dim, HD_VIDEO_PXLFMT fmt)
{
	HD_RESULT ret;

	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, fmt, HD_VIDEOPROC_1_CTRL, TRUE); //open for capture
	if (ret != HD_OK) {
		printf("set proc-cfg2 fail=%d\n", ret);
		return HD_ERR_NG;
	}

    if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module_2(VIDEO_CAPTURE *p_stream)
{
	HD_RESULT ret;
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
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}
#define FLOW_ON_OPEN		1
#define FLOW_ON_SHOT		2
#define FLOW_ON_CLOSE		3
#define FLOW_ON_STOP		4



static void *process_thread(void *arg)
{
	VIDEO_CAPTURE* p_stream0 = (VIDEO_CAPTURE *)arg;
	HD_RESULT ret = HD_OK;

	HD_VIDEO_FRAME video_frame = {0};

	p_stream0->prc_exit = 0;
	p_stream0->prc_loop = 0;
	p_stream0->prc_count = 0;
	//------ wait flow_start ------
	while (p_stream0->prc_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->prc_exit == 0) {

		//printf("cap_pull ....\r\n");
		ret = hd_videocap_pull_out_buf(p_stream0->cap_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("cap_pull error=%d !!\r\n\r\n", ret);
    			goto skip;
		}
		//printf("cap frame.count = %llu\r\n", video_frame.count);

		//printf("proc_push ....\r\n");
		//printf("proc %d/%d\r\n", p_stream0->prc_count+1, p_stream0->flow_max_shot);
		ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			printf("proc_push error=%d !!\r\n\r\n", ret);
    			goto skip;
		}
		p_stream0->enc_enter = 1;

		//printf("cap_release ....\r\n");
		ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame);
		if (ret != HD_OK) {
			printf("cap_release error=%d !!\r\n\r\n", ret);
    			goto skip;
		}

		p_stream0->prc_count ++;
		//printf("process count = %d\r\n", p_stream0->prc_count);
		if ((p_stream0->prc_count >= p_stream0->flow_max_shot) || (p_stream0->flow_run == FLOW_ON_STOP)) {
			// let process_thread stop loop and exit
			p_stream0->prc_exit = 1;
		}
skip:
		p_stream0->prc_loop++;
		usleep(100); //sleep for getchar()
	}

	return 0;
}

static void *encode_thread(void *arg)
{
	VIDEO_CAPTURE* p_stream0 = (VIDEO_CAPTURE *)arg;
	HD_RESULT ret = HD_OK;

	HD_VIDEO_FRAME video_frame = {0};

	p_stream0->enc_exit = 0;
	p_stream0->enc_loop = 0;
	p_stream0->enc_count = 0;
	//------ wait flow_start ------
	while (p_stream0->enc_enter == 0) usleep(100);

	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {

		//printf("proc_pull ....\r\n");
		ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("proc_pull error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}
		//printf("proc frame.count = %llu\r\n", video_frame.count);

		//printf("enc_push ....\r\n");
		//printf("encode %d/%d\r\n", p_stream0->enc_count+1, p_stream0->flow_max_shot);
		ret = hd_videoenc_push_in_buf(p_stream0->enc_path, &video_frame, NULL, -1); // blocking mode
		if (ret != HD_OK) {
			printf("enc_push error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}
		p_stream0->save_enter = 1;

		//printf("proc_release ....\r\n");
		ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
		if (ret != HD_OK) {
			printf("proc_release error=%d !!\r\n\r\n", ret);
    			goto skip2;
		}

		p_stream0->enc_count ++;
		//printf("encode count = %d\r\n", p_stream0->enc_count);
		if ((p_stream0->enc_count >= p_stream0->flow_max_shot) || (p_stream0->flow_run == FLOW_ON_STOP)) {
			// let encode_thread stop loop and exit
			p_stream0->enc_exit = 1;
		}
skip2:
		p_stream0->enc_loop++;
		usleep(100); //sleep for getchar()
	}

	return 0;
}

static void *save_thread(void *arg)
{
	VIDEO_CAPTURE* p_stream0 = (VIDEO_CAPTURE *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	char file_path_main[64] = {0};
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	p_stream0->save_exit = 0;
	p_stream0->save_loop = 0;
	p_stream0->save_count = 0;
	//------ wait flow_start ------
	while (p_stream0->save_enter == 0) usleep(100);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
	if (vir_addr_main == 0) {
		printf("mmap error !!\r\n\r\n");
		return 0;
	}

	//--------- pull data test ---------
	while (p_stream0->save_exit == 0) {

		//printf("enc_pull ....\r\n");
		ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode
		if (ret != HD_OK) {
			if (ret != HD_ERR_UNDERRUN)
			printf("enc_pull error=%d !!\r\n\r\n", ret);
    			goto skip3;
		}

		snprintf(file_path_main, 64, "/mnt/sd/cap_%lux%lu_%lu_%lu.jpg",
			p_stream0->capture_dim.w, p_stream0->capture_dim.h,
			p_stream0->flow_max_shot, p_stream0->save_count+1);
		printf("save %d/%d (%s) .... ", p_stream0->save_count+1, p_stream0->flow_max_shot, file_path_main);

		//----- open output files -----
		if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
			HD_VIDEOENC_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
    			goto skip3;
		}

		for (j=0; j< data_pull.pack_num; j++) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
			UINT32 len = data_pull.video_pack[j].size;
			if (f_out_main) fwrite(ptr, 1, len, f_out_main);
			if (f_out_main) fflush(f_out_main);
		}

		// close output file
		fclose(f_out_main);

		printf("ok\r\n");

		//printf("enc_release ....\r\n");
		ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
		if (ret != HD_OK) {
			printf("enc_release error=%d !!\r\n", ret);
		}

		p_stream0->save_count ++;
		//printf("save count = %d\r\n", p_stream0->save_count);
		if ((p_stream0->save_count >= p_stream0->flow_max_shot) || (p_stream0->flow_run == FLOW_ON_STOP)) {
			// let save_thread stop loop and exit
			p_stream0->save_exit = 1;
		}
skip3:
		p_stream0->save_loop++;
		usleep(100); //sleep for getchar()
	}

	// mummap for bs buffer
	ret = hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	if (ret != HD_OK) {
		printf("mnumap error !!\r\n\r\n");
	}

	return 0;
}

static void *flow_thread(void *arg)
{
	VIDEO_LIVEVIEW* p_stream0 = (VIDEO_LIVEVIEW *)((UINT32*)arg)[0];
	VIDEO_CAPTURE* p_stream2 = (VIDEO_CAPTURE *)((UINT32*)arg)[1];
	UINT32 enc_type = 0;
	AET_OPERATION ae_ui_operation = {0};
	AWBT_OPERATION awb_ui_operation = {0};
	IQT_OPERATION iq_ui_operation = {0};
#if (LOAD_ISP_CFG == 1)
	IQT_CFG_INFO cfg_info = {0};
#endif
#if (LOAD_ISP_DTSI == 1)
	IQT_DTSI_INFO dtsi_info = {0};
#endif
	HD_RESULT rt = HD_OK;

	// always encode to JPEG
	enc_type = 2;

	while (p_stream2->flow_quit == 0 ) {

		if (p_stream2->flow_run == FLOW_ON_OPEN) {
			HD_RESULT ret;
            p_stream2->flow_state = FLOW_ON_OPEN;
			// get videocap capability
			ret = get_cap_caps(p_stream0->cap_ctrl, &p_stream0->cap_syscaps);
			if (ret != HD_OK) {
				printf("get cap-caps fail=%d\n", ret);
				goto exit2;
			}

			// get videoout capability
			ret = get_out_caps(p_stream0->out_ctrl, &p_stream0->out_syscaps);
			if (ret != HD_OK) {
				printf("get out-caps fail=%d\n", ret);
				goto exit2;
			}
			p_stream0->out_max_dim = p_stream0->out_syscaps.output_dim;

			// set videocap parameter
			p_stream0->cap_dim.w = VDO_SIZE_W; //assign by user
			p_stream0->cap_dim.h = VDO_SIZE_H; //assign by user
			ret = set_cap_param(p_stream0->cap_path, &p_stream0->cap_dim, FALSE);
			if (ret != HD_OK) {
				printf("set cap fail=%d\n", ret);
				goto exit2;
			}

			// set videoproc parameter (liveview)
			ret = set_proc_param(p_stream0->proc_path, NULL, FALSE);
			if (ret != HD_OK) {
				printf("set proc fail=%d\n", ret);
				goto exit2;
			}

			// set videoout parameter (liveview)
			p_stream0->out_dim.w = p_stream0->out_max_dim.w; //using device max dim.w
			p_stream0->out_dim.h = p_stream0->out_max_dim.h; //using device max dim.h
			ret = set_out_param(p_stream0->out_path, &p_stream0->out_dim);
			if (ret != HD_OK) {
				printf("set out fail=%d\n", ret);
				goto exit2;
			}

			// set videoenc config (capture)
			p_stream2->enc_max_dim.w = CAP_SIZE_W; //assign by user
			p_stream2->enc_max_dim.h = CAP_SIZE_H; //assign by user
			ret = set_enc_cfg(p_stream2->enc_path, &p_stream2->enc_max_dim, enc_type, CAP_TBR, CAP_BUF);
			if (ret != HD_OK) {
				printf("set enc-cfg fail=%d\n", ret);
			}

			// bind video_liveview modules (liveview)
			hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
			hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);

			// start video_liveview modules (liveview)
			hd_videocap_start(p_stream0->cap_path);
			hd_videoproc_start(p_stream0->proc_path);
			// just wait ae/awb stable for auto-test, if don't care, user can remove it
			sleep(1);
			hd_videoout_start(p_stream0->out_path);

			p_stream2->flow_max_shot= 1; //single shot
		}
		if (p_stream2->flow_run == FLOW_ON_CLOSE) {
            p_stream2->flow_state = FLOW_ON_CLOSE;
			// stop video_liveview modules (liveview)
			hd_videocap_stop(p_stream0->cap_path);
			hd_videoproc_stop(p_stream0->proc_path);
			hd_videoout_stop(p_stream0->out_path);

			// unbind video_liveview modules (liveview)
			hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
			hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
		}
		if (p_stream2->flow_run == FLOW_ON_SHOT) {
			HD_RESULT ret = HD_OK;
            p_stream2->flow_state = FLOW_ON_SHOT;
			if (vendor_isp_init() == HD_ERR_NG) {
				return 0;
			}

			ae_ui_operation.id = 0;
			ae_ui_operation.operation = AE_OPERATION_CAPTURE;
			rt |= vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation);
			awb_ui_operation.id = 0;
			awb_ui_operation.operation = AWB_OPERATION_CAPTURE;
			rt |= vendor_isp_set_awb(AWBT_ITEM_OPERATION, &awb_ui_operation);
			iq_ui_operation.id = 0;
			iq_ui_operation.operation = IQ_UI_OPERATION_CAPTURE;
			rt |= vendor_isp_set_iq(IQT_ITEM_OPERATION, &iq_ui_operation);
			if (rt != HD_OK) {
				printf("set 3A/IQ OPERATION_CAPTURE fail!! \r\n");
			}

			usleep(34000); // default set to wait 1 frame time (> 33ms)

			if (vendor_isp_uninit() == HD_ERR_NG) {
				return 0;
			}

			//printf("stop liveview - begin\n");
			hd_videocap_stop(p_stream0->cap_path);
			hd_videoproc_stop(p_stream0->proc_path);
			hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
			//printf("stop liveview - end\n");

			// create process_thread (pull_out frame, push_in frame then pull_out frame)
			ret = pthread_create(&p_stream2->prc_thread_id, NULL, process_thread, (void *)p_stream2);
			if (ret < 0) {
				printf("create capture thread failed");
				goto exit2;
			}
			// create encode_thread (pull_out frame, push_in frame then pull_out bitstream)
			ret = pthread_create(&p_stream2->enc_thread_id, NULL, encode_thread, (void *)p_stream2);
			if (ret < 0) {
				printf("create encode thread failed");
				goto exit2;
			}
			// create encode_thread (pull_out bitstream, save bitstream)
			ret = pthread_create(&p_stream2->save_thread_id, NULL, save_thread, (void *)p_stream2);
			if (ret < 0) {
				printf("create save thread failed");
				goto exit2;
			}

			if (vendor_isp_init() == HD_ERR_NG) {
				return 0;
			}

#if (LOAD_ISP_CFG == 1)
			cfg_info.id = 0;
			strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_cap.cfg", CFG_NAME_LENGTH);
			rt |= vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			rt |= vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			rt |= vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			if (rt != HD_OK) {
				printf("set capture cfg fail!! \r\n");
			}
#endif
#if (LOAD_ISP_DTSI == 1)
			dtsi_info.id = 0;
			strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0_cap", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			rt |= vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
			if (rt != HD_OK) {
				printf("set capture dtsi fail!! \r\n");
			}
#endif

			if (vendor_isp_uninit() == HD_ERR_NG) {
				return 0;
			}

			printf("start shot.\r\n");
			//printf("start capture - begin\n");
			// set videocap parameter (capture)
			p_stream2->cap_path = p_stream0->cap_path; //using the same cap_path
			p_stream2->cap_dim.w = CAP_SIZE_W; //assign by user
			p_stream2->cap_dim.h = CAP_SIZE_H; //assign by user
			ret = set_cap_param(p_stream2->cap_path, &p_stream2->cap_dim, TRUE);
			if (ret != HD_OK) {
				printf("set cap fail=%d\n", ret);
			}
			// set videoproc parameter (capture)
			p_stream2->capture_dim.w = cap_size[p_stream2->sel_cap_size].w; //assign by user
			p_stream2->capture_dim.h = cap_size[p_stream2->sel_cap_size].h; //assign by user
			ret = set_proc_param(p_stream2->proc_path, &p_stream2->capture_dim, TRUE);
			if (ret != HD_OK) {
				printf("set proc2 capture fail=%d\n", ret);
				goto exit2;
			}
			// set videoenc parameter (capture)
			p_stream2->enc_dim.w = cap_size[p_stream2->sel_cap_size].w; //assign by user
			p_stream2->enc_dim.h = cap_size[p_stream2->sel_cap_size].h; //assign by user
			ret = set_enc_param(p_stream2->enc_path, &p_stream2->enc_dim, enc_type, CAP_FIX_TBR(p_stream2->enc_dim.w, p_stream2->enc_dim.h));
			if (ret != HD_OK) {
				printf("set enc fail=%d\n", ret);
				goto exit2;
			}
			hd_videocap_start(p_stream2->cap_path);
			hd_videoproc_start(p_stream2->proc_path);
			hd_videoenc_start(p_stream2->enc_path);
			// let first thread start to work
			p_stream2->prc_enter = 1;
			//printf("start capture - end\n");

			//printf("wait ...\n");
			// destroy capture thread
			pthread_join(p_stream2->prc_thread_id, NULL);
			// destroy encode thread
			pthread_join(p_stream2->enc_thread_id, NULL);
			// destroy save thread
			pthread_join(p_stream2->save_thread_id, NULL);

			printf("stop shot.\r\n");
            p_stream2->flow_state = FLOW_ON_STOP;
			//printf("stop capture - begin\n");
			p_stream2->prc_enter = 0;
			p_stream2->enc_enter = 0;
			p_stream2->save_enter = 0;
			hd_videocap_stop(p_stream2->cap_path);
			hd_videoproc_stop(p_stream2->proc_path);
			hd_videoenc_stop(p_stream2->enc_path);
			//printf("stop capture - end\n");

			if (vendor_isp_init() == HD_ERR_NG) {
				return 0;
			}

			ae_ui_operation.id = 0;
			ae_ui_operation.operation = AE_OPERATION_MOVIE;
			rt |= vendor_isp_set_ae(AET_ITEM_OPERATION, &ae_ui_operation);
			awb_ui_operation.id = 0;
			awb_ui_operation.operation = AWB_OPERATION_MOVIE;
			rt |= vendor_isp_set_awb(AWBT_ITEM_OPERATION, &awb_ui_operation);
			iq_ui_operation.id = 0;
			iq_ui_operation.operation = IQ_UI_OPERATION_MOVIE;
			rt |= vendor_isp_set_iq(IQT_ITEM_OPERATION, &iq_ui_operation);
			if (rt != HD_OK) {
				printf("set 3A/IQ OPERATION_MOVIE fail!! \r\n");
			}

#if (LOAD_ISP_CFG == 1)
			cfg_info.id = 0;
			strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
			rt |= vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info);
			rt |= vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			rt |= vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			if (rt != HD_OK) {
				printf("set movie cfg fail!! \r\n");
			}
#endif
#if (LOAD_ISP_DTSI == 1)
			dtsi_info.id = 0;
			strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			rt |= vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
			if (rt != HD_OK) {
				printf("set capture dtsi fail!! \r\n");
			}
#endif

			if (vendor_isp_uninit() == HD_ERR_NG) {
				return 0;
			}

			//printf("start liveview - begin\n");
			// set videocap parameter (liveview)
			p_stream0->cap_dim.w = VDO_SIZE_W; //assign by user
			p_stream0->cap_dim.h = VDO_SIZE_H; //assign by user
			ret = set_cap_param(p_stream0->cap_path, &p_stream0->cap_dim, FALSE);
			if (ret != HD_OK) {
				printf("set cap fail=%d\n", ret);
				goto exit2;
			}
			// set videoproc parameter (liveview)
			ret = set_proc_param(p_stream0->proc_path, NULL, FALSE);
			if (ret != HD_OK) {
				printf("set proc fail=%d\n", ret);
				goto exit2;
			}
			hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
			hd_videocap_start(p_stream0->cap_path);
			hd_videoproc_start(p_stream0->proc_path);
			//printf("start liveview - end\n");

		}
exit2:
		p_stream2->flow_run = 0;
		usleep(100);
	}
	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[1] = {0}; //0: liveview stream
	VIDEO_CAPTURE stream2[1] = {0}; //0: capture stream
	UINT32 stream_list[2] = {((UINT32)&stream[0]), ((UINT32)&stream2[0])};
	UINT32 out_type = 1;

	// query program options
	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
    stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
	// query program options
	if (argc >= 3 && (atoi(argv[2]) !=0)) {
		stream[0].hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", stream[0].hdmi_id);
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

	// open video_liveview modules (liveview)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim, CAP_OUT_FMT, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// open video_capture modules (capture)
	stream2[0].proc_max_dim.w = CAP_SIZE_W; //assign by user
	stream2[0].proc_max_dim.h = CAP_SIZE_H; //assign by user
	ret = open_module_2(&stream2[0], &stream2[0].proc_max_dim, CAP_OUT_FMT);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// create flow_thread
	ret = pthread_create(&stream2[0].flow_thread_id, NULL, flow_thread, (void *)stream_list);
	if (ret < 0) {
		printf("create flow thread failed");
		goto exit;
	}

	// query user key
	printf("Enter q to exit\n");
	printf("\r\nif you want to capture 1, enter \"s\" to trigger !!\r\n");

	stream2[0].flow_run = FLOW_ON_OPEN;
	while (stream2[0].flow_run != 0) usleep(100); //wait unitl flow idle

	while (1) {
		key = GETCHAR();
		if (key == '1') {
			printf("select DRIVE mode: 1-shot !!\r\n");
			stream2[0].flow_max_shot = 1;
		}
		if (key == '3') {
			printf("select DRIVE mode: 3-shot !!\r\n");
			stream2[0].flow_max_shot = 3;
		}
		if (key == '5') {
			printf("select DRIVE mode: 5-shot !!\r\n");
			stream2[0].flow_max_shot = 5;
		}
		if (key == 'a') {
			printf("select DRIVE mode: 10-shot !!\r\n");
			stream2[0].flow_max_shot = 10;
		}
		if (key == 'c') {
			printf("select DRIVE mode: continue-shot !!\r\n");
			stream2[0].flow_max_shot = 9999;
		}

		if (key == 'x') {
			printf("select SIZE 0: %lux%lu !!\r\n", cap_size[0].w, cap_size[0].h);
			stream2[0].sel_cap_size = 0;
		}
		if (key == 'y') {
			printf("select SIZE 1: %lux%lu !!\r\n", cap_size[1].w, cap_size[1].h);
			stream2[0].sel_cap_size = 1;
		}
		if (key == 'z') {
			printf("select SIZE 2: %lux%lu !!\r\n", cap_size[2].w, cap_size[2].h);
			stream2[0].sel_cap_size = 2;
		}

		if (key == 's') {
			if (stream2[0].flow_run == 0) { //flow is idle
				stream2[0].flow_run = FLOW_ON_SHOT; //start shot
	            while (stream2[0].flow_state != FLOW_ON_SHOT) usleep(100); //wait unitl flow record
			} else { //flow is still under shot
				stream2[0].flow_run = FLOW_ON_STOP; //force stop shot
	            while (stream2[0].flow_state != FLOW_ON_STOP) usleep(100); //wait unitl flow stop
	            while (stream2[0].flow_run != 0) usleep(100); //wait unitl flow idle
			}
		}

		if (key == 'q' || key == 0x3) {
			// quit thread
			if((stream2[0].flow_state == FLOW_ON_STOP)||(stream2[0].flow_state == FLOW_ON_OPEN)){
    			break;
            } else{
                printf("stop first\r\n");
            }
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}
	while (stream2[0].flow_run != 0) usleep(100); //wait unitl flow idle

	stream2[0].flow_run = FLOW_ON_CLOSE;
	while (stream2[0].flow_state != FLOW_ON_CLOSE) usleep(100); //wait unitl flow idle

	stream2[0].flow_quit = 1;
	// destroy save flow_thread
	pthread_join(stream2[0].flow_thread_id, NULL);

exit:
	// close video_liveview modules (liveview)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// close video_capture modules (capture)
	ret = close_module_2(&stream2[0]);
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
