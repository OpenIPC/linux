/**
	@brief Sample code of ai network with sensor input.\n

	@file alg_net_app_user_sample_stream.c

	@author Joshua Liu

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "kwrap/examsys.h"
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_isp.h"
#include "vqa_lib.h"

#define CLRCST_WRITE_OUT DISABLE

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_clrcst_sample_stream, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DST_IMG_WIDTH  			128 //128
#define DST_IMG_HEIGHT			72 //72
#define DST_IMG_BUF_SIZE		((DST_IMG_WIDTH * DST_IMG_HEIGHT) * 3 / 2)

#define DEBUG_MENU 			1

//#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
//#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
//#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

//#define DBG_DUMP(fmtstr, args...)   printf(fmtstr, ##args)


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

#define SEN_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W		128
#define YOUT_WIN_NUM_H		128
//#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
//#define ETH_OUT_SEL			1 //0: full, 1: subsample 1/2

#define VDO_SIZE_W			1920
#define VDO_SIZE_H			1080

// nn
#define DEFAULT_DEVICE		"/dev/" VENDOR_AIS_FLOW_DEV_NAME

#define YUV_WIDTH			224
#define YUV_HEIGHT			224
#define MAX_FRAME_WIDTH		1280
#define MAX_FRAME_HEIGHT	720

#define FRAME_BUF_SIZE		(2 * MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT)
#define YUV_OUT_BUF_SIZE	(3 * MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT)

#define MBYTE				(1024 * 1024)
#define VENDOR_AI_CFG  		0x000f0000  //ai project config

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420

#define MAX_OBJ_NUM			1024

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

	// (4) --
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;

	// (5) --
	HD_PATH_ID mask_alg_path;

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;


static BOOL is_clrcst_proc					= TRUE;
static BOOL is_clrcst_run					= FALSE;

static VIDEO_LIVEVIEW g_stream[1] = {0}; //0: main stream
static UINT32 g_shdr_mode = 0;


///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              		ret;
	HD_COMMON_MEM_INIT_CONFIG	mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        											 +VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        											 +VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	// config common pool (alg)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(DST_IMG_WIDTH, DST_IMG_HEIGHT, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}

	// - end nn get block

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
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

#if 1
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	printf("sen_imx290 MIPI Interface\n");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
#else
    //snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
    //printf("sen_ar0237ir Parallel Interface\n");
	//cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	//cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	//cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2

	printf("use os02k10\n");
    snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
    cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;

    if(g_shdr_mode==1) {
        cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
        cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
    } else {
        cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
        cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
    }

    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;	


#endif
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	if (g_shdr_mode == 1) {
        cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
    }

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;

	if (g_shdr_mode == 1) {
        iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
    }

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
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);

		video_out_param.depth = 1;	// set > 0 to allow pull out (nn)

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
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
	video_out_param.pxlfmt = VDO_FRAME_FORMAT;
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
	// set videoproc config for nn
	ret = set_proc_cfg(&p_stream->proc_alg_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg alg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
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

static HD_RESULT yuv_copy(HD_VIDEO_FRAME *p_video_frame_dst,HD_VIDEO_FRAME *p_video_frame_src)
{
	HD_GFX_SCALE param;
    HD_RESULT ret=0;
    //copy vout0 to common buffer
    memset(&param, 0, sizeof(HD_GFX_SCALE));
    param.src_img.dim.w            = p_video_frame_src->dim.w;
    param.src_img.dim.h            = p_video_frame_src->dim.h;;
    param.src_img.format           = p_video_frame_src->pxlfmt;
    param.src_img.p_phy_addr[0]    = p_video_frame_src->phy_addr[0];//src_pa;
    param.src_img.p_phy_addr[1]    = p_video_frame_src->phy_addr[1];//src_pa;
    param.src_img.lineoffset[0]    = p_video_frame_src->loff[0];//src_w 
    param.src_img.lineoffset[1]    = p_video_frame_src->loff[1];//src_w
    param.dst_img.dim.w            = p_video_frame_dst->dim.w;//dst_w
    param.dst_img.dim.h            = p_video_frame_dst->dim.h;//dst_w
    param.dst_img.format           = p_video_frame_dst->pxlfmt;
    param.dst_img.p_phy_addr[0]    = p_video_frame_dst->phy_addr[0];//dst_pa;
    param.dst_img.p_phy_addr[1]    = p_video_frame_dst->phy_addr[1];//dst_pa + 1920 * 1080;
    param.dst_img.lineoffset[0]    = p_video_frame_dst->loff[0];
    param.dst_img.lineoffset[1]    = p_video_frame_dst->loff[1];
    param.src_region.x             = 0;
    param.src_region.y             = 0;
    param.src_region.w             = p_video_frame_src->dim.w;
    param.src_region.h             = p_video_frame_src->dim.h;
	param.dst_region.x             = 0;
    param.dst_region.y             = 0;
    param.dst_region.w             = p_video_frame_dst->dim.w;
    param.dst_region.h             = p_video_frame_dst->dim.h;

    ret = hd_gfx_scale(&param);
    if(ret != HD_OK){
        printf("hd_gfx_scale fail=%d\n", ret);
        goto exit;
    }

#if 0
	printf("src_dim_w(%d) src_dim_h(%d)  src_line_offset[0]=%d, src_line_offset[1]=%d, \n\
			dst_dim_w(%d) dst_dim_h(%d)  dst_line_offset[0]=%d, dst_line_offset[1]=%d  \n\
			src_region_x(%d) src_region_y(%d) src_region_w(%d) src_region_h(%d) \
			dst_pos_x(%d) dst_pos_y(%d) \r\n",
			param.src_img.dim.w, param.src_img.dim.h, param.src_img.lineoffset[0], param.src_img.lineoffset[1],
			param.dst_img.dim.w, param.dst_img.dim.h, param.dst_img.lineoffset[0], param.dst_img.lineoffset[1],
			param.src_region.x, param.src_region.y, param.src_region.w, param.src_region.h,
			param.dst_pos.x, param.dst_pos.y);
#endif

exit:
    
    return ret;
}

static VOID *clrcst_thread_api(VOID *arg)
{
	//INT32 key;
	HD_RESULT ret;
	HD_VIDEO_FRAME video_frame = {0};
	HD_VIDEO_FRAME video_frame_new = {0};
	UINT32 src_va;
#if (CLRCST_WRITE_OUT == ENABLE)
	char file_path_main[64]={0};
	FILE *f_out_main;
	UINT8 *ptr;
	UINT32 len;
#endif
	//CLRCST_PARM_S *cl_parm = (CLRCST_PARM_S *) arg;
    CLRCST_STATUS_S cl_status;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 phy_addr_main;
	UINT32 blk_size = DST_IMG_BUF_SIZE;
	HD_VIDEO_FRAME *p_video_frame_dst=0;
    HD_VIDEO_FRAME *p_video_frame_src=0;

	do {
        if (is_clrcst_run) {
			ret = hd_videoproc_pull_out_buf(g_stream[0].proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto exit;
			}

			blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, DDR_ID0); // Get block from mem pool
            if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
                printf("get block fail (0x%x).. try again later.....\r\n", blk);
                goto exit;
            }

			phy_addr_main = hd_common_mem_blk2pa(blk); // Get physical addr
            if (phy_addr_main == 0) {
                printf("blk2pa fail, blk = 0x%x\r\n", blk);
                goto exit;
            }

			video_frame_new.sign        = MAKEFOURCC('V','F','R','M');
            video_frame_new.p_next      = NULL;
            video_frame_new.ddr_id      = DDR_ID0;
            video_frame_new.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
            video_frame_new.dim.w       = DST_IMG_WIDTH;
            video_frame_new.dim.h       = DST_IMG_HEIGHT;
            video_frame_new.count       = 0;
            video_frame_new.timestamp   = hd_gettime_us();
            video_frame_new.loff[0]     = DST_IMG_WIDTH; // Y
            video_frame_new.loff[1]     = DST_IMG_WIDTH; // UV
            video_frame_new.phy_addr[0] = phy_addr_main;                          // Y
            video_frame_new.phy_addr[1] = phy_addr_main + (DST_IMG_WIDTH * DST_IMG_HEIGHT);  // UV pack
            video_frame_new.blk         = blk;

			p_video_frame_src = &video_frame;
            p_video_frame_dst = &video_frame_new;

            ret = yuv_copy(p_video_frame_dst, p_video_frame_src);
            if (ret != HD_OK) {
                printf("vout0 yuv_copy fail %d\r\n", ret);
                goto exit;
            }

			if (is_clrcst_run) {
				//src_pa	= video_frame.phy_addr[0];
				src_va	= (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame_new.phy_addr[0], video_frame_new.dim.w*video_frame_new.dim.h*3/2);
				if (src_va == 0) {
					printf("Error: mmap fail !!\r\n");
					goto exit;
				}

				cl_status = (CLRCST_STATUS_S) vqa_clrcst_detect(src_va, video_frame_new.dim.w, video_frame_new.dim.h);
				if (cl_status == CL_NORMAL_S) {
					printf("===ok===\r\n");
				} else if (cl_status == CL_ALARM_S) {
					printf("======================ALARM=====================\r\n");
				} else {
					printf("CLRCST: Error status.(%d)\r\n", cl_status);
				}

	#if (CLRCST_WRITE_OUT == ENABLE)
				snprintf(file_path_main, 32, "/mnt/sd/dump_yuv420.img");
				printf("dump snapshot yuv420 to file (%s) ....\r\n", file_path_main);

				//----- open output files -----
				if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
					HD_VIDEOENC_ERR("open file (%s) fail....\r\n\r\n", file_path_main);
						goto exit;
				}

				ptr = (UINT8 *) src_va;
				len = video_frame_new.dim.w*video_frame_new.dim.h*3/2;
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
	#endif

				ret = hd_common_mem_munmap((void *)src_va, video_frame_new.dim.w*video_frame_new.dim.h*3/2);
				if (HD_OK != ret) {
					printf("nn_thread_api : (src_img.va)hd_common_mem_munmap fail\r\n");
					goto exit;
				}

	#if (CLRCST_WRITE_OUT == ENABLE)

				// close output file
				fclose(f_out_main);
				is_clrcst_run = 0;
	#endif
				usleep(33000);
			} else {
				usleep(33000);
			}

			ret = hd_common_mem_release_block(blk);
			if (HD_OK != ret) {
				printf("err:release blk fail %d\r\n", ret);
				goto exit;
			}

			ret = hd_videoproc_release_out_buf(g_stream[0].proc_alg_path, &video_frame);
			if(ret != HD_OK) {
				printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto exit;
			}
			usleep(33000);

        }
		usleep(33000);
	} while(is_clrcst_proc);

exit:

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	UINT32 out_type = 1;
	CLRCST_PARM_S cl_parm;
	int rt;	
	pthread_t clrcst_thread_id[1];
	UINT32 vqa_version;

    vqa_version = vqa_get_version(VQA_VERSION);
    printf("VQA: version(0x%x)\r\n", (unsigned int) vqa_version);

	is_clrcst_proc = TRUE;
	is_clrcst_run = FALSE;

	// query program options
	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
    g_stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default

	// query program options
	if (argc >= 3 && (atoi(argv[2]) !=0)) {
		g_stream[0].hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", g_stream[0].hdmi_id);
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

	// open video_liveview modules (main)
	g_stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	g_stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&g_stream[0], &g_stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability
	ret = get_cap_caps(g_stream[0].cap_ctrl, &g_stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(g_stream[0].out_ctrl, &g_stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	g_stream[0].out_max_dim = g_stream[0].out_syscaps.output_dim;

	// set videocap parameter
	g_stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	g_stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(g_stream[0].cap_path, &g_stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(g_stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (alg)
	g_stream[0].proc_alg_max_dim.w = VDO_SIZE_W;
	g_stream[0].proc_alg_max_dim.h = VDO_SIZE_H;
	ret = set_proc_param(g_stream[0].proc_alg_path, &g_stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (main)
	g_stream[0].out_dim.w = g_stream[0].out_max_dim.w; //using device max dim.w
	g_stream[0].out_dim.h = g_stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(g_stream[0].out_path, &g_stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
	
	ret = pthread_create(&clrcst_thread_id[0], NULL, clrcst_thread_api, (VOID*)&cl_parm);
	if (ret < 0) {
		printf("CLRCST: Error, create encode thread failed");
		goto exit;
	}
	// nn E ------------------------------------------------

	ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

	// start video_liveview modules (main)
	hd_videocap_start(g_stream[0].cap_path);
	hd_videoproc_start(g_stream[0].proc_path);
	hd_videoproc_start(g_stream[0].proc_alg_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(g_stream[0].out_path);

	// query user key
	do {
		printf("  enter q: exit\n");
		printf("  enter r: run engine\n");
		printf("  enter u: update engine\n");
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			break;
		} else if (key == 'r') {
			printf("usage:\n");
		
#if 0
			printf("Please input max_yuv_width:");
			rt = scanf("%d", (int *)&cl_parm.cl_max_yuv_width);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_max_yuv_width);
				break;
			}

			printf("Please input max_yuv_height:");
			rt = scanf("%d", (int *)&cl_parm.cl_max_yuv_height);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_max_yuv_height);
				break;
			}

			printf("Please input cl_collect_time:");
			rt = scanf("%d", (int *)&cl_parm.cl_collect_time);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_collect_time);
				break;
			}

			printf("Please input cl_warning_max:");
			rt = scanf("%d", (int *)&cl_parm.cl_warning_max);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_warning_max);
				break;
			}

			printf("Please input cl_diff_th:");
			rt = scanf("%d", (int *)&cl_parm.cl_diff_th);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_diff_th);
				break;
			}

			printf("Please input cl_diff_area:");
			rt = scanf("%d", (int *)&cl_parm.cl_diff_area);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_diff_area);
				break;
			}

			printf("Please input cl_mode:");
			rt = scanf("%d", (int *)&cl_parm.cl_mode);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_mode);
				break;
			}

			printf("Please input cl_backup_cnt:");
			rt = scanf("%d", (int *)&cl_parm.cl_backup_cnt);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_backup_cnt);
				break;
			}

			printf("Please input debug_mode:");
			rt = scanf("%d", (int *)&cl_parm.cl_is_debug);
			if (rt != 1) {
				printf("CLRCST:Invalid option. Try again. rt(%d) input_value(%d)\n", rt, cl_parm.cl_is_debug);
				break;
			}
#else
			cl_parm.cl_max_yuv_width = 1920;
            cl_parm.cl_max_yuv_height = 1080;
            cl_parm.cl_collect_time = 1;
            cl_parm.cl_warning_max = 5;
            cl_parm.cl_diff_th = 2;
            cl_parm.cl_diff_area = 20;
            cl_parm.cl_mode = 2;
            cl_parm.cl_backup_cnt = 65535;
            cl_parm.cl_is_debug = 0;
#endif

			cl_parm.sensor_ch = 0; //if it is HDR sensor, please set value to 1.
			cl_parm.vendor_isp_init = vendor_isp_init;
			cl_parm.vendor_isp_uninit = vendor_isp_uninit;
			cl_parm.vendor_isp_get_common = vendor_isp_get_common;
			
			//  run clrcst
			is_clrcst_proc = TRUE;
            is_clrcst_run = TRUE;

			rt = vqa_clrcst_init(&cl_parm);
			if (rt < 0) {
				printf("CLRCST:Error to init clrcst_init()\r\n");
			}
			
		} else if (key == 'u') {
			vqa_clrcst_uninit(&cl_parm);
			rt = vqa_clrcst_init(&cl_parm);
			if (rt < 0) {
				printf("CLRCST:Error to init clrcst_init()\r\n");
			}
		}
		usleep(33000);
	} while(1);

	is_clrcst_proc = FALSE;
    is_clrcst_run = FALSE;
	pthread_join(clrcst_thread_id[0], NULL);

	vqa_clrcst_uninit(&cl_parm);

	// stop video_liveview modules (main)
	hd_videocap_stop(g_stream[0].cap_path);
	hd_videoproc_stop(g_stream[0].proc_path);
	hd_videoproc_stop(g_stream[0].proc_alg_path);
	hd_videoout_stop(g_stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
	
	// close video_liveview modules (main)
	ret = close_module(&g_stream[0]);
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
