/**
	@brief Sample code of videocapture only.\n

	@file video_capture_only.c

	@author Ben Wang

	@ingroup mhdal

	@note Nothing.

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_capture_only, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 1

#define CHKPNT      					printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)						printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x) 						printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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

#define CAP_OUT_Q_DEPTH  1

#define USE_REAL_SENSOR  1  // set 0 for patgen

#define RAW_BUFFER_DDR_ID DDR_ID0
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(UINT32 sen_hdr, UINT32 *p_blk_size)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H,CAP_OUT_FMT)
														+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
														+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2 + CAP_OUT_Q_DEPTH;
	if (sen_hdr) {
		mem_cfg.pool_info[0].blk_cnt *= 2;
	}
	mem_cfg.pool_info[0].ddr_id = RAW_BUFFER_DDR_ID;
	#if 0
	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	#endif
	ret = hd_common_mem_init(&mem_cfg);
	*p_blk_size = mem_cfg.pool_info[0].blk_size;
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl, HD_OUT_ID _out_id, UINT32 *p_cap_pin_map, UINT32 cap_shdr_map, char *p_driver_name)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	#if USE_REAL_SENSOR
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");
	UINT32 i;


	strncpy(cap_cfg.sen_cfg.sen_dev.driver_name, p_driver_name, (HD_VIDEOCAP_SEN_NAME_LEN - 1));
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	for (i=0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[i] = *(p_cap_pin_map+i);
	}
	cap_cfg.sen_cfg.shdr_map = cap_shdr_map;
	#else
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	#endif
	ret = hd_videocap_open(0, _out_id, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	#if USE_REAL_SENSOR
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	if (cap_shdr_map) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);
	#endif
	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, HD_VIDEOCAP_SEN_FRAME_NUM frame_num)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};
		#if USE_REAL_SENSOR
		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = frame_num;
		#else
		video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
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
		if (ret != HD_OK) {
			return ret;
		}
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
		if (frame_num > 1) {
			//just convert RAW to SHDR RAW format
			//e.g. HD_VIDEO_PXLFMT_RAW8 -> HD_VIDEO_PXLFMT_RAW8_SHDR2
			video_out_param.pxlfmt &=~ 0x0F000000;
			video_out_param.pxlfmt |= (frame_num << 24);
		}
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.depth = CAP_OUT_Q_DEPTH*frame_num;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.ddr_id = RAW_BUFFER_DDR_ID;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	CHAR cap_drv_name[HD_VIDEOCAP_SEN_NAME_LEN];
	UINT32 cap_pin_map[HD_VIDEOCAP_SEN_SER_MAX_DATALANE];
	UINT32 cap_shdr_map;
	HD_VIDEOCAP_SEN_FRAME_NUM cap_frame_num;
	HD_DIM  proc_max_dim;
	// (4) user pull
	pthread_t  cap_thread_id;
	UINT32     cap_exit;
	UINT32     cap_snap;
	UINT32     flow_start;
	UINT32     blk_size;
	INT32    wait_ms;
	UINT32 	show_ret;
} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl, HD_VIDEOCAP_0_CTRL, p_stream->cap_pin_map, p_stream->cap_shdr_map, p_stream->cap_drv_name);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;

	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////

static void *cap_raw_thread(void *arg)
{
	VIDEO_LIVEVIEW *p_stream0 = (VIDEO_LIVEVIEW *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame[2] = {0};
	char file_path_main[2][32] = {0};
	FILE *f_out_main;
	UINT32 phy_addr_main[2], vir_addr_main[2];
	UINT32 shot_count = 0;
	UINT32 index;
	UINT32 i;
	 #define PHY2VIRT_MAIN(pa, i) (vir_addr_main[i] + ((pa) - phy_addr_main[i]))

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	//hd_videoenc_get(video_enc_path0, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	//vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	printf("\r\nif you want to capture raw, enter \"s\" to trigger !!\r\n");
	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->cap_exit == 0) {

		if(p_stream0->cap_snap) {
			p_stream0->cap_snap = 0;
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				ret = hd_videocap_pull_out_buf(p_stream0->cap_path, &video_frame[i], p_stream0->wait_ms);// -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
				if (ret != HD_OK) {
					if(p_stream0->show_ret) printf("pull_out(%d) error = %d!!\r\n", p_stream0->wait_ms, ret);
					goto skip;
				}

				if(p_stream0->show_ret) printf("pull_out(%d) ok!!\r\n", p_stream0->wait_ms);
				#if 0
				if(!p_stream0->cap_snap) {
					ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame[i]);
					if (ret != HD_OK) {
						printf("release_out() error = %d!!\r\n", ret);
					}
					goto skip;
				}
				#endif


				phy_addr_main[i] = hd_common_mem_blk2pa(video_frame[i].blk); // Get physical addr
				if (phy_addr_main[i] == 0) {
					printf("hd_common_mem_blk2pa error !!\r\n\r\n");
					goto release_out;
				}
				vir_addr_main[i] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main[i], p_stream0->blk_size);
				if (vir_addr_main[i] == 0) {
					printf("memory map error !!\r\n\r\n");
					goto release_out;
				}





				printf("fmt           resolution     lineoffset     PA             blk_size\r\n");
				printf("0x08%X    %4dx%4d      %4d           0x%08X     %d\r\n",video_frame[i].pxlfmt,
																					video_frame[i].dim.w,
																					video_frame[i].dim.h,
																					video_frame[i].loff[0],
																					video_frame[i].phy_addr[0],
																					p_stream0->blk_size);


				index = (video_frame[i].pxlfmt >> 12) & 0xF;
				snprintf(file_path_main[i], 32, "/mnt/sd/vdocap%lu_%lu.raw", index, shot_count);
				printf("dump raw data file (%s) ....\r\n", file_path_main[i]);
			}
			//pull all cap_frame_num before saving raw files
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				//----- open output files -----
				if ((f_out_main = fopen(file_path_main[i], "wb")) == NULL) {
					printf("open file (%s) fail....\r\n\r\n", file_path_main[i]);
					goto skip;
				}
				if (f_out_main) {
					UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame[i].phy_addr[0], i);
					UINT32 len = video_frame[i].loff[0]*video_frame[i].ph[0];

					fwrite(ptr, 1, len, f_out_main);
					fflush(f_out_main);
				}
				// close output file
				fclose(f_out_main);
				hd_common_mem_munmap((void *)vir_addr_main[i], p_stream0->blk_size);

				printf("dump raw ok\r\n\r\n");
			}
			shot_count ++;
release_out:
			for (i = 0; i < p_stream0->cap_frame_num; i++) {
				if(p_stream0->show_ret) printf("release_out() ....\r\n");
				ret = hd_videocap_release_out_buf(p_stream0->cap_path, &video_frame[i]);
				printf("cap_released ....\r\n");
				if (ret != HD_OK) {
					printf("cap_release error !!\r\n\r\n");
				}
			}
		}
skip:
		usleep(200000);
	}



	return 0;
}

MAIN(argc, argv)
{
    HD_RESULT ret;
    INT key;
    VIDEO_LIVEVIEW stream[2] = {0}; //0: shdr main stream, 1: shdr sub stream
    UINT32 sen_hdr = 0;
    UINT32 i;
    UINT32 pull_type = 0;

    // query program options
	if (argc >= 2) {
		sen_hdr = atoi(argv[1]);
		printf("sen_hdr %d\r\n", sen_hdr);
	}
	if (argc >= 3) {
		pull_type = atoi(argv[2]);
		stream[0].show_ret = 1; //show pull result
		printf("pull_type %d\r\n", pull_type);
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	ret = mem_init(sen_hdr, &stream[0].blk_size);
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

	if (pull_type == 0) {
		stream[0].wait_ms = 0;
		printf("pull(%d) with non-blocking\r\n", stream[0].wait_ms);
	} else if (pull_type == 1) {
		stream[0].wait_ms = 500;
		printf("pull(%d) with blocking-timeout\r\n", stream[0].wait_ms);
	} else {
		stream[0].wait_ms = -1;
		printf("pull(%d) with blocking\r\n", stream[0].wait_ms);
	}

	// open video liview modules (main)
	for (i = 0; i < HD_VIDEOCAP_SEN_SER_MAX_DATALANE; i++) {
		stream[0].cap_pin_map[i] = HD_VIDEOCAP_SEN_IGNORE;
	}
	stream[0].cap_pin_map[0] = 0;
	stream[0].cap_pin_map[1] = 1;

	snprintf(stream[0].cap_drv_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	if (sen_hdr) {
		stream[0].cap_pin_map[2] = 2;
		stream[0].cap_pin_map[3] = 3;
		stream[0].cap_shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	} else {
		stream[0].cap_shdr_map = 0;
	}
	ret = open_module(&stream[0]);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// set videocap parameter (main)
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	if (sen_hdr) {
		stream[0].cap_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
	} else {
		stream[0].cap_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
	}
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, stream[0].cap_frame_num);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}


	ret = pthread_create(&stream[0].cap_thread_id, NULL, cap_raw_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed");
		return -1;
	}

	// start capture modules (main)
	ret = hd_videocap_start(stream[0].cap_path);
	if (ret != HD_OK) {
		printf("start fail=%d\n", ret);
		goto exit;
	}

	// let cap_raw_thread start to work
	stream[0].flow_start = 1;

	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 's') {
			stream[0].cap_snap = 1;
		}
		if (key == 'q' || key == 0x3) {
			// quit program
			stream[0].cap_exit = 1;
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

	// stop capture modules (main)
	hd_videocap_stop(stream[0].cap_path);

	// destroy capture thread
	pthread_join(stream[0].cap_thread_id, NULL);


exit:
	// close capture modules (main)
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
