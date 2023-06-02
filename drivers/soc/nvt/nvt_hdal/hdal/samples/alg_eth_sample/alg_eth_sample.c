/**
 * @file vendor_eth_sample.c
 * @brief start eth sample.
 * @author ALG1-CV
 * @date in the year 2018
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "nvt_dis.h"


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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_eth_sample, argc, argv)
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

///////////////////////////////////////////////////////////////////////////////
static HD_COMMON_MEM_VB_BLK  g_eth_blk;
static UINT32         g_egmap_size = (1920 * 1080)>>2;
#define               DUMP_ETH

 ///////////////////////////////////////////////////////////////////////////////


 static HD_RESULT mem_init(void)
 {
	 HD_RESULT				ret;
	 HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	 // config common pool (cap)
	 mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	 mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
												 +VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        											+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	 mem_cfg.pool_info[0].blk_cnt = 2;
	 mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	 // config common pool (main)
	 mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	 mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	 mem_cfg.pool_info[1].blk_cnt = 3;
	 mem_cfg.pool_info[1].ddr_id = DDR_ID0;

 	 // config common pool (eth)
	 mem_cfg.pool_info[2].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	 mem_cfg.pool_info[2].blk_size = (g_egmap_size<<3);
	 mem_cfg.pool_info[2].blk_cnt = 3;
	 mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	 ret = hd_common_mem_init(&mem_cfg);
	 return ret;
 }

 static void share_memory_init(DIS_IPC_INIT *p_share_mem)
 {
	 HD_COMMON_MEM_VB_BLK blk;
	 UINT32 		   pa, va;
	 UINT32 		   blk_size = (g_egmap_size<<3);
	 HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	 HD_RESULT		   ret;

	 memset(p_share_mem, 0x00, sizeof(DIS_IPC_INIT));
	 g_eth_blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	 blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id);
	 if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		 printf("err:get block fail\r\n", blk);
		 return;
	 }
	 printf("blk = 0x%x\r\n",blk);
	 pa = hd_common_mem_blk2pa(blk);
	 if (pa == 0) {
		 printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
		 goto blk2pa_err;
	 }
	 printf("pa = 0x%x\r\n", pa);
	 if (pa > 0) {
		 va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
		 if (va == 0) {
			 goto map_err;
		 }
	 }
	 p_share_mem->addr = va;
	 p_share_mem->size = blk_size;
	 g_eth_blk         = blk;
	 printf("share_mem pa = 0x%x, va=0x%x, size =0x%x\r\n", pa, p_share_mem->addr, p_share_mem->size);
	 return;
 blk2pa_err:
 map_err:
	 ret = hd_common_mem_release_block(blk);
	 if (HD_OK != ret) {
		 printf("err:release blk fail %d\r\n", ret);
		 return;
	 }
 }

 static HD_RESULT share_memory_exit(DIS_IPC_INIT *p_share_mem)
 {
  	 HD_RESULT ret = HD_OK;
	 if (p_share_mem->addr!= 0) {
		 hd_common_mem_munmap((void *)p_share_mem->addr, p_share_mem->size);
	 }
	 if (g_eth_blk != HD_COMMON_MEM_VB_INVALID_BLK) {
	 	ret = hd_common_mem_release_block(g_eth_blk);
		if (ret != HD_OK) {
			printf("mem_uninit : hd_common_mem_release_block fail.\r\n");
		}
		return ret;
	 }
	 memset(p_share_mem, 0x00, sizeof(DIS_IPC_INIT));
	 return HD_OK;
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

/*static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
 {
	 HD_RESULT ret = HD_OK;
	 HD_VIDEOCAP_SYSINFO sys_info = {0};

	 hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu, output_started=%d, cur_dim(%dx%d)\r\n",
		sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count, sys_info.output_started, sys_info.cur_dim.w, sys_info.cur_dim.h);
	 return ret;
 }*/

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
    snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
    printf("sen_ar0237ir Parallel Interface\n");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
#endif
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
		 video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		 video_out_param.dir = HD_VIDEO_DIR_NONE;
		 video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
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

	 HD_VIDEOOUT_HDMI_ID hdmi_id;
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
	 // set videoout config
	 ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
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
#ifdef DUMP_ETH
static BOOL eth_save_result(char* folderName, UINT32 frameIdx, UINT32 addr, UINT32 size)
{
	static CHAR filepath[128];
	FILE *fsave=NULL;

	sprintf(filepath  ,"/mnt/sd/DIS/outBin/%s/eth_%ld.raw" , folderName, frameIdx);

	fsave = fopen(filepath, "w");
	if(fsave == NULL) {
		printf("fopen fail\n");
		return FALSE;
	}

	fwrite((UINT8*)addr, size, 1, fsave);

	fclose(fsave);

	return TRUE;
}
#endif

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_LIVEVIEW stream[1] = {0}; //0: main stream
	UINT32 out_type = 1;
	DIS_IPC_INIT   share_mem = {0};


	DIS_ETH_IN_PARAM                       eth_input_info;
	DIS_ETH_OUT_PARAM                      eth_out_info = {0};
	DIS_ETH_BUFFER_INFO                    eth_out_addr = {0};

	#ifdef DUMP_ETH
	CHAR *p_folder = "IPE_OUT";
	#endif

	 // query program options
	 if (argc >= 2) {
		 out_type = atoi(argv[1]);
		 printf("out_type %d\r\n", out_type);
		 if (out_type > 2) {
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

	 // init share memory
	share_memory_init(&share_mem);

	 // init all modules
	 ret = init_module();
	 if (ret != HD_OK) {
		 printf("init fail=%d\n", ret);
		 goto exit;
	 }

	 // open video_record modules (main)
	 stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	 stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	 ret = open_module(&stream[0], &stream[0].proc_max_dim, out_type);
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

	 // get videoout capability
	 ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	 if (ret != HD_OK) {
		 printf("get out-caps fail=%d\n", ret);
		 goto exit;
	 }
	 stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	 // set videocap parameter
	 stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	 stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	 ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	 if (ret != HD_OK) {
		 printf("set cap fail=%d\n", ret);
		 goto exit;
	 }

	 // set videoproc parameter (main)
	 ret = set_proc_param(stream[0].proc_path, NULL);
	 if (ret != HD_OK) {
		 printf("set proc fail=%d\n", ret);
		 goto exit;
	 }

	 // set videoout parameter (main)
	 stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	 stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	 ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	 if (ret != HD_OK) {
		 printf("set out fail=%d\n", ret);
		 goto exit;
	 }

	 // bind video_liveview modules (main)
	 hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	 hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);

	 // start video_liveview modules (main)
	 hd_videocap_start(stream[0].cap_path);
	 hd_videoproc_start(stream[0].proc_path);
	 // just wait ae/awb stable for auto-test, if don't care, user can remove it
	 sleep(1);
	 hd_videoout_start(stream[0].out_path);

	 // query user key
	 printf("Enter q to exit\n");
	 printf("Enter s to set eth\n");
	 printf("Enter g to get eth\n");
	 printf("Enter e to end eth\n");
	 while (1) {
		key = GETCHAR();
		 if (key == 'q' || key == 0x3) {
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
		 if (key == 's') {
		 	UINT32 func = 0;
			ret = vendor_dis_init(func);
			if (ret != HD_OK) {
				printf("err:vendor_dis_init error %d\r\n",ret);
				goto eth_exit;
			}

			ret = dis_eth_initialize(&share_mem);
			if (ret != HD_OK) {
				printf("err:dis_eth_initialize error %d\r\n",ret);
				goto eth_exit;
			}
		 }
		 if (key == 'e') {
			 ret = dis_eth_uninitialize();
			 if (ret != HD_OK) {
				 printf("err:dis_eth_uninitialize error %d\r\n",ret);
				 goto eth_exit;
			 }
			 UINT32 func = 0;
			 ret = vendor_dis_uninit(func);
			 if (ret != HD_OK) {
				 printf("err:vendor_dis_uninit error %d\r\n",ret);
				 goto eth_exit;
			 }
		 }
		 if (key == 'g') {
			// set input & output info
			ret = dis_eth_get_input_info(&eth_input_info);
			if (ret != HD_OK) {
				printf("err:dis_eth_get_input_info error %d\r\n",ret);
				goto eth_exit;
			}
			printf("eth_in_info: enable = %d, h_out_sel = %d, v_out_sel = %d, out_bit_sel = %d, out_sel = %d,th_high = %d, th_mid = %d, th_low = %d\n\r ",eth_input_info.enable, eth_input_info.h_out_sel, eth_input_info.v_out_sel, eth_input_info.out_bit_sel, eth_input_info.out_sel, eth_input_info.th_high, eth_input_info.th_mid, eth_input_info.th_low);

			ret = dis_eth_get_out_info(&eth_out_info);
			if (ret != HD_OK) {
				printf("err:dis_eth_get_out_info error %d\r\n",ret);
				goto eth_exit;
			}
			printf("eth_out_info: out_lofs = %d, w = %d, h = %d\n\r ",eth_out_info.out_lofs, eth_out_info.w, eth_out_info.h);

			ret = dis_eth_get_out_addr(&eth_out_addr);
			if (ret != HD_OK) {
				printf("err:dis_eth_get_out_addr error %d\r\n",ret);
				goto eth_exit;
			}
			printf("eth_out_addr: uiInAdd0 = 0x%x, buf_size = 0x%x, frame_cnt = %d\n\r ",eth_out_addr.ui_inadd, eth_out_addr.buf_size, eth_out_addr.frame_cnt);

			#ifdef DUMP_ETH
			if (!eth_save_result(p_folder, eth_out_addr.frame_cnt, eth_out_addr.ui_inadd, eth_out_addr.buf_size)) {
				printf("err: eth save result fail\n");
			}
			#endif
		 }
	 }
 eth_exit:
	 // stop video_liveview modules (main)
	 hd_videocap_stop(stream[0].cap_path);
	 hd_videoproc_stop(stream[0].proc_path);
	 hd_videoout_stop(stream[0].out_path);

	 // unbind video_liveview modules (main)
	 hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	 hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

 exit:
	 // close video_liveview modules (main)
	 ret = close_module(&stream[0]);
	 if (ret != HD_OK) {
		 printf("close fail=%d\n", ret);
	 }

	 // uninit all modules
	 ret = exit_module();
	 if (ret != HD_OK) {
		 printf("exit fail=%d\n", ret);
	 }
 	 ret = share_memory_exit(&share_mem);
	 if (ret != HD_OK) {
		 printf("share_memory_exit fail=%d\n", ret);
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
