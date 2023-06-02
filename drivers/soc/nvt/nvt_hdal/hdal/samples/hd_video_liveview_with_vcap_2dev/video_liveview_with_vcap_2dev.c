/**
	@brief Sample code of video liveview with 2 videocapture.\n

	@file video_liveview_with_vcap_2dev.c

	@author Ben Wang

	@ingroup mhdal

	@note This file is modified from video_liveview.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_videocapture.h"

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_liveview_with_vcap_2dev, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define YUV_BLK_SIZE      (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420))
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

#define VDO_SIZE_W     1920
#define VDO_SIZE_H     1080
#define VDOOUT0_TYPE   1 //1 for LCD, 2 for HDMI, 0 for TV
#define VDOOUT1_TYPE   2 //1 for LCD, 2 for HDMI, 0 for TV

#define VDOOUT_HDMI  HD_VIDEOOUT_HDMI_1920X1080I60

#define MCLK_SRC_SYNC_FUNC    0
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
	mem_cfg.pool_info[0].blk_cnt = 4;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = YUV_BLK_SIZE;
	mem_cfg.pool_info[1].blk_cnt = 6+1; //for side by side copy
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

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

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu, output_started=%d, cur_dim(%dx%d)\r\n",
		sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count, sys_info.output_started, sys_info.cur_dim.w, sys_info.cur_dim.h);
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
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK0 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1
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

	#if MCLK_SRC_SYNC_FUNC
	{
		UINT32 mclk_sync_set = (HD_VIDEOCAP_0|HD_VIDEOCAP_1);
		//MUST be set prior to HD_VIDEOCAP_PARAM_DRV_CONFIG !!!
		ret = vendor_videocap_set(video_cap_ctrl, VENDOR_VIDEOCAP_PARAM_MCLK_SRC_SYNC_SET, &mclk_sync_set);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOCAP_PARAM_DATA_LANE failed!(%d)\r\n", ret);
		}
	}
	#endif

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}
static HD_RESULT set_cap2_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	#if MCLK_SRC_SYNC_FUNC
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x420; //PIN_SENSOR2_CFG_MIPI|PIN_SENSOR2_CFG_MCLK_2ND
	#else
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x20; //PIN_SENSOR2_CFG_MIPI
	#endif
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xC02;//PIN_MIPI_LVDS_CFG_CLK1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3

	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel =  HD_VIDEOCAP_SEN_CLANE_SEL_CSI1_USE_C1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_1_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	#if MCLK_SRC_SYNC_FUNC
	{
		UINT32 mclk_sync_set = (HD_VIDEOCAP_0|HD_VIDEOCAP_1);
		//MUST be set prior to HD_VIDEOCAP_PARAM_DRV_CONFIG !!!
		ret = vendor_videocap_set(video_cap_ctrl, VENDOR_VIDEOCAP_PARAM_MCLK_SRC_SYNC_SET, &mclk_sync_set);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOCAP_PARAM_DATA_LANE failed!(%d)\r\n", ret);
		}
	}
	#endif

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
	UINT32 data_lane = 2;
	ret = vendor_videocap_set(video_cap_path, VENDOR_VIDEOCAP_PARAM_DATA_LANE, &data_lane);
	if (ret != HD_OK) {
		printf("VENDOR_VIDEOCAP_PARAM_DATA_LANE failed!(%d)\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim, HD_OUT_ID _out_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
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
		video_cfg_param.ctrl_max.func = 0;
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

static HD_RESULT set_out_cfg(HD_CTRL_ID ctrl_id,HD_PATH_ID *p_video_out_ctrl, UINT32 out_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, ctrl_id, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

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
		videoout_mode.output_mode.hdmi= VDOOUT_HDMI;
	break;
	default:
		printf("not support out_type\r\n");
	break;
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

	HD_DIM  out1_max_dim;
	HD_DIM  out1_dim;

	// (4)
	HD_VIDEOOUT_SYSCAPS out1_syscaps;
	HD_PATH_ID out1_ctrl;
	HD_PATH_ID out1_path;

	// (5) user pull
	pthread_t  aquire_thread_id;
	UINT32     proc_exit;
	UINT32     flow_start;
	UINT32 	   copy_new_buf;
	INT32      wait_ms;
    HD_URECT   pip_rect;
} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videocap config
	#if (MCLK_SRC_SYNC_FUNC == 0)
	//all mclk-source-synced VCAPs should be set HD_VIDEOCAP_PARAM_DRV_CONFIG before any VCAPs open i/o path
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	#endif
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, HD_VIDEOPROC_0_CTRL);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(HD_VIDEOOUT_0_CTRL,&p_stream->out_ctrl, out_type);
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

static HD_RESULT open_module_2(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;

	// set videocap config
	#if (MCLK_SRC_SYNC_FUNC == 0)
	//all mclk-source-synced VCAPs should be set HD_VIDEOCAP_PARAM_DRV_CONFIG before any VCAPs open i/o path
	ret = set_cap2_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg2 fail=%d\n", ret);
		return HD_ERR_NG;
	}
    #endif
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim, HD_VIDEOPROC_1_CTRL);
	if (ret != HD_OK) {
		printf("set proc-cfg2 fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if((ret = hd_videocap_open(HD_VIDEOCAP_1_IN_0, HD_VIDEOCAP_1_OUT_0, &p_stream->cap_path)) != HD_OK)
        return ret;
    if ((ret = hd_videoproc_open(HD_VIDEOPROC_1_IN_0, HD_VIDEOPROC_1_OUT_0, &p_stream->proc_path)) != HD_OK)
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

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}
static HD_RESULT yuv_copy(HD_VIDEO_FRAME *p_video_frame_dst,HD_VIDEO_FRAME *p_video_frame_src,HD_URECT *p_rect)
{
    HD_GFX_COPY param;
    HD_RESULT ret=0;
    //copy vout0 to common buffer
	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = p_video_frame_src->dim.w;
	param.src_img.dim.h            = p_video_frame_src->dim.h;;
	param.src_img.format           = p_video_frame_src->pxlfmt;
	param.src_img.p_phy_addr[0]    = p_video_frame_src->phy_addr[0];//src_pa;
	param.src_img.p_phy_addr[1]    = p_video_frame_src->phy_addr[1];//src_pa;
	param.src_img.lineoffset[0]    = p_video_frame_src->loff[0];
	param.src_img.lineoffset[1]    = p_video_frame_src->loff[1];
	param.dst_img.dim.w            = p_video_frame_dst->dim.w;
	param.dst_img.dim.h            = p_video_frame_dst->dim.h;
	param.dst_img.format           = p_video_frame_dst->pxlfmt;
	param.dst_img.p_phy_addr[0]    = p_video_frame_dst->phy_addr[0];//dst_pa;
	param.dst_img.p_phy_addr[1]    = p_video_frame_dst->phy_addr[1];//dst_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = p_video_frame_dst->loff[0];
	param.dst_img.lineoffset[1]    = p_video_frame_dst->loff[1];
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = p_video_frame_src->dim.w;
	param.src_region.h             = p_video_frame_src->dim.h;
	param.dst_pos.x                = p_rect->x;
	param.dst_pos.y                = p_rect->y;
	param.colorkey                 = 0;
	param.alpha                    = 255;

	ret = hd_gfx_copy(&param);
	if(ret != HD_OK){
		printf("hd_gfx_copy fail=%d\n", ret);
	}
    return ret;

}
static void *aquire_yuv_thread(void *arg)
{
	VIDEO_LIVEVIEW* p_stream0 = (VIDEO_LIVEVIEW *)arg;
	VIDEO_LIVEVIEW* p_stream1 = p_stream0+1;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME *p_video_frame_dst=0;
	HD_VIDEO_FRAME *p_video_frame_src=0;
	HD_VIDEO_FRAME video_frame = {0};
	HD_VIDEO_FRAME video_frame1 = {0};
	HD_VIDEO_FRAME video_frame_new = {0};
	UINT32 phy_addr_main=0;
	UINT32 blk_size = YUV_BLK_SIZE;
	HD_COMMON_MEM_VB_BLK blk =0;

	printf("\r\nif you want to stop, enter \"q\" to exit !! \r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->proc_exit == 0) {
		ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, p_stream0->wait_ms); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if (ret != HD_OK) {
		    printf("0pull_out(%d) error = %d!!\r\n", p_stream0->wait_ms, ret);
			goto rel_none;
		}
		ret = hd_videoproc_pull_out_buf(p_stream1->proc_path, &video_frame1, p_stream1->wait_ms); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if (ret != HD_OK) {
		    printf("1pull_out(%d) error = %d!!\r\n", p_stream1->wait_ms, ret);
			goto rel_0;
		}

        if(p_stream0->copy_new_buf) {

    		//--- Get memory ---
    		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0); // Get block from mem pool
    		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
    			printf("get block fail (0x%x).. try again later.....\r\n", blk);
    			goto rel_1;
    		}

            phy_addr_main = hd_common_mem_blk2pa(blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", blk);
        		goto rel_new;
			}

			video_frame_new.sign        = MAKEFOURCC('V','F','R','M');
			video_frame_new.p_next      = NULL;
			video_frame_new.ddr_id      = DDR_ID0;
			video_frame_new.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
			video_frame_new.dim.w       = p_stream0->out_max_dim.w;
			video_frame_new.dim.h       = p_stream0->out_max_dim.h;
			video_frame_new.count       = 0;
			video_frame_new.timestamp   = hd_gettime_us();
			video_frame_new.loff[0]     = p_stream0->out_max_dim.w; // Y
			video_frame_new.loff[1]     = p_stream0->out_max_dim.w; // UV
			video_frame_new.phy_addr[0] = phy_addr_main;                          // Y
			video_frame_new.phy_addr[1] = phy_addr_main+p_stream0->out_max_dim.w*p_stream0->out_max_dim.h;  // UV pack
			video_frame_new.blk         = blk;

            p_video_frame_src = &video_frame;
            p_video_frame_dst = &video_frame_new;

            ret = yuv_copy(p_video_frame_dst,p_video_frame_src,&p_stream0->pip_rect);
			if (ret != HD_OK) {
				printf("vout0 yuv_copy fail %d\r\n", ret);
        		goto rel_new;
			}
            p_video_frame_src = &video_frame1;
            p_video_frame_dst = &video_frame_new;

        } else {
           p_video_frame_src = &video_frame1;
           p_video_frame_dst = &video_frame;

        }

        ret = yuv_copy(p_video_frame_dst,p_video_frame_src,&p_stream1->pip_rect);
		if (ret != HD_OK) {
			printf("vout1 yuv_copy fail %d\r\n", ret);
    		goto rel_new;
		}

        ret = hd_videoout_push_in_buf(p_stream0->out_path, p_video_frame_dst, NULL, 0);
        ret = hd_videoout_push_in_buf(p_stream1->out_path, &video_frame1, NULL, 0);
		goto rel_new;

rel_new:
        if(p_stream0->copy_new_buf) {
			ret = hd_common_mem_release_block(blk);
    		if (ret != HD_OK) {
    			printf("release new_buf error %d!!\r\n\r\n",ret);
    		}
        }
rel_1:
		ret = hd_videoproc_release_out_buf(p_stream1->proc_path, &video_frame1);
		if (ret != HD_OK) {
			printf("release_out1 error %d!!\r\n\r\n",ret);
		}
rel_0:
		ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
		if (ret != HD_OK) {
			printf("release_out0 error %d!!\r\n\r\n",ret);
		}
rel_none:
		usleep(1000); //delay 1 ms
	}
    printf("exit flow\r\n");
	return 0;
}
MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[2] = {0}; //0: shdr main stream, 1: shdr sub stream
    HD_DIM  sub_dim;
    HD_DIM  main_dim;
    INT display = 2;  //default is pip view
	if (argc >= 2) {
        display = atoi(argv[1]);
        switch(display)
        {
            case 0:
                    //cap0 ->vout0
            break;
            case 1:
                    //cap1->vout0
            break;
            case 2:
                    //cap0+cap1 ->vout0 (pip)
                stream[0].copy_new_buf = 0;
            break;
            case 3:
                    //cap0+cap1 ->vout0 (side by side)
                stream[0].copy_new_buf = 1;
            break;
            default:
            {
                printf("not sup %d\r\n",display);
                return 0;
            }
        }
		printf("###display %d,copy_new_buf %d\r\n",display,stream[0].copy_new_buf);
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

	#if MCLK_SRC_SYNC_FUNC
	//all mclk-source-synced VCAPs should be set HD_VIDEOCAP_PARAM_DRV_CONFIG before any VCAPs open i/o path
	ret = set_cap_cfg(&stream[0].cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		goto exit;
	}
	ret = set_cap2_cfg(&stream[1].cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg2 fail=%d\n", ret);
		goto exit;
	}
	#endif

	// open video liview modules (sensor 1st)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
    stream[0].wait_ms = -1; //blocking mode
	ret = open_module(&stream[0], &stream[0].proc_max_dim, VDOOUT0_TYPE);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// open video liview modules (sensor 2nd)
	stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
    stream[1].wait_ms = 0; //none-blocking mode

	ret = open_module_2(&stream[1], &stream[1].proc_max_dim, VDOOUT0_TYPE);
	if (ret != HD_OK) {
		printf("open2 fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability (sensor 1st)
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability (sensor 2nd)
	ret = get_cap_caps(stream[1].cap_ctrl, &stream[1].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap2-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability (sensor 1st)
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	#if 0
	// get videoout capability (sensor 2nd)
	ret = get_out_caps(stream[1].out_ctrl, &stream[1].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[1].out_max_dim = stream[1].out_syscaps.output_dim;
	#endif

	// set videocap parameter (sensor 1st)
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter (sensor 2nd)
	stream[1].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[1].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[1].cap_path, &stream[1].cap_dim);
	if (ret != HD_OK) {
		printf("set cap2 fail=%d\n", ret);
		goto exit;
	}

    if(!stream[0].copy_new_buf){
        stream[0].pip_rect.x = 0;
        stream[0].pip_rect.y = 0;
        stream[0].pip_rect.w = stream[0].out_max_dim.w;
        stream[0].pip_rect.h = stream[0].out_max_dim.h;
    } else {
        stream[0].pip_rect.x = 0;
        stream[0].pip_rect.y = 0;
        stream[0].pip_rect.w = stream[0].out_max_dim.w/2;
        stream[0].pip_rect.h = stream[0].out_max_dim.h;
    }
    main_dim.w = stream[0].pip_rect.w;
    main_dim.h = stream[0].pip_rect.h;
	// set videoproc parameter (sensor 1st)
	ret = set_proc_param(stream[0].proc_path, &main_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
    if(!stream[0].copy_new_buf) {
        stream[1].pip_rect.x = 50;
        stream[1].pip_rect.y = 50;
        stream[1].pip_rect.w = stream[0].out_syscaps.output_dim.w/2;
        stream[1].pip_rect.h = stream[0].out_syscaps.output_dim.h/2;
    } else {
        stream[1].pip_rect.x = stream[0].out_max_dim.w/2;
        stream[1].pip_rect.y = 0;
        stream[1].pip_rect.w = stream[0].out_max_dim.w/2;
        stream[1].pip_rect.h = stream[0].out_max_dim.h;
    }
    sub_dim.w = stream[1].pip_rect.w;
    sub_dim.h = stream[1].pip_rect.h;
	// set videoproc parameter (sensor 2nd)
	ret = set_proc_param(stream[1].proc_path, &sub_dim);
	if (ret != HD_OK) {
		printf("set proc2 fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (sensor 1st)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}
	#if 0
	// set videoout parameter (sensor 2nd)
	stream[1].out_dim.w = stream[1].out_max_dim.w; //using device max dim.w
	stream[1].out_dim.h = stream[1].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[1].out_path, &stream[1].out_dim);
	if (ret != HD_OK) {
		printf("set out1 fail=%d\n", ret);
		goto exit;
	}
	#endif
	// bind video_liveview modules (sensor 1st)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);

	// bind video_liveview modules (sensor 2nd)
	hd_videocap_bind(HD_VIDEOCAP_1_OUT_0, HD_VIDEOPROC_1_IN_0);

    if(display ==0) {
        hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
    } else if(display ==1) {
        hd_videoproc_bind(HD_VIDEOPROC_1_OUT_0, HD_VIDEOOUT_0_IN_0);
    }

    ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

	// start video_liveview modules (sensor 1st)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	hd_videoout_start(stream[0].out_path);

	// start video_liveview modules (sensor 2nd)
	hd_videocap_start(stream[1].cap_path);
	hd_videoproc_start(stream[1].proc_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);

    if((display ==2)||(display ==3)) {
    	// let encode_thread start to work
    	stream[0].flow_start = 1;
    	// create aquire_thread (pull_out bitstream)
    	ret = pthread_create(&stream[0].aquire_thread_id, NULL, aquire_yuv_thread, (void *)&stream[0]);
    	if (ret < 0) {
    		printf("create aquire thread failed");
    		goto exit;
    	}
    }
	// query user key
    printf("\r\nEnter q to exit, Enter d to debug\r\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let feed_thread, aquire_thread stop loop and exit
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
		if (key == '0') {
			get_cap_sysinfo(stream[0].cap_ctrl);
		}
	}

	// stop video_liveview modules (sensor 1st)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);

	hd_videoout_stop(stream[0].out_path);

	// stop video_liveview modules (sensor 2nd)
	hd_videocap_stop(stream[1].cap_path);
	hd_videoproc_stop(stream[1].proc_path);
    if((display ==2)||(display ==3)) {
    	// destroy thread (pull_out bitstream)
    	pthread_join(stream[0].aquire_thread_id, (void* )NULL);
    }
	// unbind video_liveview modules (sensor 1st)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);

	// unbind video_liveview modules (sensor 2nd)
	hd_videocap_unbind(HD_VIDEOCAP_1_OUT_0);
    if(display ==0){
        hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
    } else if(display ==1) {
        hd_videoproc_unbind(HD_VIDEOPROC_1_IN_0);
    }

exit:
	// close video_liveview modules (sensor 1st)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// close video_liveview modules (sub)
	ret = close_module(&stream[1]);
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
