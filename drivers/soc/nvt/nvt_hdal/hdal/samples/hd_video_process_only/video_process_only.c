/**
	@brief Sample code of video snapshot from proc to frame.\n

	@file video_process_only.c

	@author Jeah Yen

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_process_only, argc, argv)
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

#define VDO_SIZE_W         352
#define VDO_SIZE_H         288
#define VDO_FRM_NUM        30     // pattern file frame number

#define MAX_YUV_BLK_CNT    3
#define YUV_BLK_SIZE       (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420))

#define VDO_DDR_ID		DDR_ID0

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[0].blk_cnt = 3;
	mem_cfg.pool_info[0].ddr_id = VDO_DDR_ID;

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
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_SCALE; //HD_VIDEOPROC_PIPE_RAWALL
		video_cfg_param.isp_id = HD_ISP_DONT_CARE; //0
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};
		video_path_param.in_func = 0;
		video_path_param.ddr_id = VDO_DDR_ID;
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
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
		video_out_param.depth = 1; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};
		video_path_param.out_func = 0;
		video_path_param.ddr_id = VDO_DDR_ID;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_PROCESS {

	// (1) user push
	pthread_t  feed_thread_id;

	HD_DIM  proc_max_dim;

	// (1)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  proc_dim;

	// (4) user pull
	pthread_t  aquire_thread_id;
	UINT32     proc_exit;
	UINT32   flow_start;
	UINT32 	do_snap;
	UINT32 	shot_count;
	INT32    wait_ms;
	
	UINT32 	show_ret;
	
} VIDEO_PROCESS;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_PROCESS *p_stream, HD_DIM* p_proc_max_dim)
{
    HD_RESULT ret;
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_PROCESS *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static BOOL check_test_pattern(void)
{
	FILE *f_in;
	char filename[64];
	char filepath[128];

	sprintf(filename, "video_frm_%d_%d_%d_yuv420.dat", VDO_SIZE_W, VDO_SIZE_H, VDO_FRM_NUM);
	sprintf(filepath, "/mnt/sd/%s", filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		printf("%s is in SDK/code/hdal/samples/pattern/%s\n", filename, filename);
		return FALSE;
	}

	fclose(f_in);
	return TRUE;
}

static INT32 blk2idx(HD_COMMON_MEM_VB_BLK blk)   // convert blk(0xXXXXXXXX) to index (0, 1, 2)
{
	static HD_COMMON_MEM_VB_BLK blk_registered[MAX_YUV_BLK_CNT] = {0};
	INT32 i;
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (blk_registered[i] == blk) return i;

		if (blk_registered[i] == 0) {
			blk_registered[i] = blk;
			return i;
		}
	}

	printf("convert blk(%0x%x) to index fail !!!!\r\n", blk);
	return (-1);
}

static void *feed_yuv_thread(void *arg)
{
	VIDEO_PROCESS* p_stream0 = (VIDEO_PROCESS *)arg;
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = YUV_BLK_SIZE;
	UINT32 yuv_size = VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);
	char filepath_yuv_main[128];
	FILE *f_in_main;
	UINT32 pa_yuv_main[MAX_YUV_BLK_CNT] = {0};
	UINT32 va_yuv_main[MAX_YUV_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 read_len;


	//------ [1] wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	//------ [2] open input files ------
	sprintf(filepath_yuv_main, "/mnt/sd/video_frm_%d_%d_%d_yuv420.dat", VDO_SIZE_W, VDO_SIZE_H, VDO_FRM_NUM);
	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return 0;
	}

get_blk:
		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			//continue;
			goto get_blk;
		}

		if ((blkidx = blk2idx(blk)) == -1) {
			printf("ERROR !! blk to idx fail !!\r\n");
			goto rel_blk;
		}

		pa_yuv_main[blkidx] = hd_common_mem_blk2pa(blk); // Get physical addr
		if (pa_yuv_main[blkidx] == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", blk);
			goto rel_blk;
		}

		if (va_yuv_main[blkidx] == 0) { // if NOT mmap yet, mmap it
			va_yuv_main[blkidx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa_yuv_main[blkidx], blk_size); // Get virtual addr
			if (va_yuv_main[blkidx] == 0) {
				printf("Error: mmap fail !! pa_yuv_main[%d], blk = 0x%x\r\n", blkidx, blk);
				goto rel_blk;
			}
		}

		//--- Read YUV from file ---
		read_len = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
		if (read_len != yuv_size) {
			if (feof(f_in_main)) {
				fseek(f_in_main, 0, SEEK_SET);  //rewind and try again
				read_len = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
				if (read_len != yuv_size) {
					printf("reading len error\n");
					goto quit;
				}
			}
		}
		
		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[blkidx], yuv_size);

	//------ [3] feed yuv ------
	while (p_stream0->proc_exit == 0) {

		
		//--- push_in ---
		{
			HD_VIDEO_FRAME video_frame = {0};
			/*
			video_frame.sign        = MAKEFOURCC('V','F','R','M');
			video_frame.p_next      = NULL;
			video_frame.ddr_id      = ddr_id;
			video_frame.pxlfmt      = HD_VIDEO_PXLFMT_RAW12 | HD_VIDEO_PIX_RGGB_R;
			video_frame.dim.w       = VDO_SIZE_W;
			video_frame.dim.h       = VDO_SIZE_H;
			video_frame.count       = 0;
			video_frame.timestamp   = hd_gettime_us();
			video_frame.pw[0]       = VDO_SIZE_W; // RAW
			video_frame.ph[0]       = VDO_SIZE_H; // RAW
			video_frame.loff[0]     = ALIGN_CEIL_4((video_frame.dim.w) * HD_VIDEO_PXLFMT_BPP(video_frame.pxlfmt) / 8); // RAW
			video_frame.phy_addr[0] = pa_yuv_main[blkidx]; // RAW
			video_frame.blk         = blk;
			*/
			video_frame.sign        = MAKEFOURCC('V','F','R','M');
			video_frame.p_next      = NULL;
			video_frame.ddr_id      = ddr_id;
			video_frame.pxlfmt      = HD_VIDEO_PXLFMT_YUV420;
			video_frame.dim.w       = VDO_SIZE_W;
			video_frame.dim.h       = VDO_SIZE_H;
			video_frame.count       = 0;
			video_frame.timestamp   = hd_gettime_us();
			video_frame.pw[0]       = VDO_SIZE_W; // Y
			video_frame.ph[0]       = VDO_SIZE_H; // UV
			video_frame.pw[1]       = VDO_SIZE_W >> 1; // Y
			video_frame.ph[1]       = VDO_SIZE_H >> 1; // UV
			video_frame.loff[0]     = ALIGN_CEIL_4(video_frame.pw[0]); // Y
			video_frame.loff[1]     = video_frame.loff[0]; // UV
			video_frame.phy_addr[0] = pa_yuv_main[blkidx];                          // Y
			video_frame.phy_addr[1] = pa_yuv_main[blkidx] + video_frame.loff[0]*video_frame.ph[0];  // UV
			video_frame.blk         = blk;

			if(p_stream0->show_ret) printf("push_in(%d)!\r\n", 0);
			ret = hd_videoproc_push_in_buf(p_stream0->proc_path, &video_frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				printf("push_in(%d) error = %d!!\r\n", 0, ret);
			}
		}

		usleep(33000); // sleep 33 ms
	}

rel_blk:
		//--- Release memory ---
		hd_common_mem_release_block(blk);

quit:
	//------ [4] uninit & exit ------
	// mummap for yuv buffer
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (va_yuv_main[i] != 0) {
			ret = hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
			}
		}
	}

	// close file
	if (f_in_main)  fclose(f_in_main);


	return 0;
}

static void *aquire_yuv_thread(void *arg)
{
	VIDEO_PROCESS* p_stream0 = (VIDEO_PROCESS *)arg;
	HD_RESULT ret = HD_OK;
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 phy_addr_main, vir_addr_main;
	UINT32 yuv_size;
	char file_path_main[32] = {0};
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + ((pa) - phy_addr_main))

	printf("\r\nif you want to snapshot, enter \"s\" to trigger !!\r\n");
	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	while (p_stream0->proc_exit == 0) {

		if(1) {
			ret = hd_videoproc_pull_out_buf(p_stream0->proc_path, &video_frame, p_stream0->wait_ms); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if (ret != HD_OK) {
				if(p_stream0->show_ret) printf("pull_out(%d) error = %d!!\r\n", p_stream0->wait_ms, ret);
        			goto skip;
			}

			if(p_stream0->show_ret) printf("pull_out(%d) ok!!\r\n", p_stream0->wait_ms);
			if(!p_stream0->do_snap) {
				ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
				if (ret != HD_OK) {
					printf("release_out() error = %d!!\r\n", ret);
				}
				goto skip;
			}
			p_stream0->do_snap = 0;

			phy_addr_main = hd_common_mem_blk2pa(video_frame.blk); // Get physical addr
			if (phy_addr_main == 0) {
				printf("blk2pa fail, blk = 0x%x\r\n", video_frame.blk);
        			goto skip;
			}

			yuv_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, HD_VIDEO_PXLFMT_YUV420);

			// mmap for frame buffer (just mmap one time only, calculate offset to virtual address later)
			vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_addr_main, yuv_size);
			if (vir_addr_main == 0) {
				printf("mmap error !!\r\n\r\n");
        			goto skip;
			}

			snprintf(file_path_main, 32, "/mnt/sd/dump_frm_yuv420_%lu.dat", p_stream0->shot_count);
			printf("dump snapshot frame to file (%s) ....\r\n", file_path_main);

			//----- open output files -----
			if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
				printf("open file (%s) fail....\r\n\r\n", file_path_main);
        			goto skip;
			}

			//save Y plane
			{
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[0]);
				UINT32 len = video_frame.loff[0]*video_frame.ph[0];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}
			//save UV plane
			{
				UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(video_frame.phy_addr[1]);
				UINT32 len = video_frame.loff[1]*video_frame.ph[1];
				if (f_out_main) fwrite(ptr, 1, len, f_out_main);
				if (f_out_main) fflush(f_out_main);
			}

			// mummap for frame buffer
			ret = hd_common_mem_munmap((void *)vir_addr_main, yuv_size);
			if (ret != HD_OK) {
				printf("mnumap error !!\r\n\r\n");
        			goto skip;
			}

			if(p_stream0->show_ret) printf("release_out() ....\r\n");
			ret = hd_videoproc_release_out_buf(p_stream0->proc_path, &video_frame);
			if (ret != HD_OK) {
				printf("release_out() error !!\r\n\r\n");
        			goto skip;
			}

			// close output file
			fclose(f_out_main);

			printf("dump snapshot ok\r\n\r\n");

			p_stream0->shot_count ++;
		}
skip:
		usleep(1000); //delay 1 ms
	}

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	VIDEO_PROCESS stream[1] = {0}; //0: main stream
	UINT32 pull_type = 0;
	HD_DIM main_dim;

	// query program options
	if (argc >= 2) {
		pull_type = atoi(argv[1]);
		printf("pull_type %d\r\n", pull_type);
		if(pull_type > 2) {
			printf("error: not support pull_type!\r\n");
			return 0;
		}
		stream[0].show_ret = 1; //show pull result
	}
	
	// check TEST pattern exist
	if (check_test_pattern() == FALSE) {
		printf("test_pattern isn't exist\r\n");
		exit(0);
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
	
	// open video_process modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	stream[0].proc_dim.w = main_dim.w;
	stream[0].proc_dim.h = main_dim.h;

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, &stream[0].proc_dim);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// create feed_thread (push_in frame)
	ret = pthread_create(&stream[0].feed_thread_id, NULL, feed_yuv_thread, (void *)stream);
	if (ret < 0) {
		printf("create feed thread failed");
		goto exit;
	}

	// create aquire_thread (pull_out frame)
	ret = pthread_create(&stream[0].aquire_thread_id, NULL, aquire_yuv_thread, (void *)stream);
	if (ret < 0) {
		printf("create aquire thread failed");
		goto exit;
	}

	// start video_process modules (main)
	hd_videoproc_start(stream[0].proc_path);

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
			// let feed_thread, aquire_thread stop loop and exit
			stream[0].proc_exit = 1; 
			// quit program
			break;
		}

		if (key == 'o') {
			ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &(stream[0].proc_path));
			if (ret != HD_OK) {
				printf("open fail=%d\n", ret);
			}
		}
		if (key == 'b') {
			ret = hd_videoproc_start(stream[0].proc_path);
			if (ret != HD_OK) {
				printf("start fail=%d\n", ret);
			}
		}
		if (key == 'e') {
			ret = hd_videoproc_stop(stream[0].proc_path); // it could force abort pull_out with blocking mode
			if (ret != HD_OK) {
				printf("stop fail=%d\n", ret);
			}
		}
		if (key == 'c') {
			ret = hd_videoproc_close(stream[0].proc_path); // it could force abort pull_out with blocking mode
			if (ret != HD_OK) {
				printf("close fail=%d\n", ret);
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

	// stop video_process modules (main)
	hd_videoproc_stop(stream[0].proc_path); // it could force abort pull_out with blocking mode

	// destroy feed_thread
	pthread_join(stream[0].feed_thread_id,  (void* )NULL);
	// destroy encode thread
	pthread_join(stream[0].aquire_thread_id, (void* )NULL);


exit:

	// close video_process modules (main)
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
