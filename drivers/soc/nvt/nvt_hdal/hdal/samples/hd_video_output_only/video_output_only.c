/**
 * @file video_output_only.c
 * @brief videoout with user pushin sample.
 * @author Janice Huang
 * @date in the year 2018
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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_output_only, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define CHKPNT      					printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)						    printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x) 						printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

#define VDO_DDR_ID		DDR_ID0
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

#if (0)
#define VDO_SIZE_W         1920
#define VDO_SIZE_H         1080
#else
#define VDO_SIZE_W         320
#define VDO_SIZE_H         240
#endif
#define VDO_FMT            HD_VIDEO_PXLFMT_YUV422
#define MAX_YUV_BLK_CNT    3
#define YUV_BLK_SIZE       (DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FMT))
#define MAX_FILE_NAME		64
#define PATTEN_PATH         "/mnt/sd/video_frm_320_240_3_yuv422.dat"

typedef struct _VIDEOOUT_ONLY {
    UINT32 start;
    UINT32 stop;
    char filename[MAX_FILE_NAME];

    HD_VIDEOOUT_SYSCAPS out_syscaps;
    HD_PATH_ID out_ctrl;
    HD_PATH_ID out_path;
    HD_VIDEOOUT_HDMI_ID hdmi_id;

} VIDEOOUT_ONLY;

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = YUV_BLK_SIZE;
	mem_cfg.pool_info[0].blk_cnt  = MAX_YUV_BLK_CNT;
	mem_cfg.pool_info[0].ddr_id   = VDO_DDR_ID;

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}


static HD_RESULT init_module(void)
{
    HD_RESULT ret;
    if((ret = hd_videoout_init()) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl,UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode;
    HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if(ret!=HD_OK)
		return ret;

	memset((void *)&videoout_mode,0,sizeof(HD_VIDEOOUT_MODE));

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
		printf("not support\r\n");
	break;
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);
	printf("set_out_cfg=%d\r\n", ret);
    *p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT open_module(VIDEOOUT_ONLY *p_vdoout_only,UINT32 out_type)
{
    HD_RESULT ret;

	ret = set_out_cfg(&p_vdoout_only->out_ctrl,out_type,p_vdoout_only->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}

    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_vdoout_only->out_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT exit_module(void)
{
    HD_RESULT ret;
    if((ret = hd_videoout_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT close_module(VIDEOOUT_ONLY *p_vdoout_only)
{
    HD_RESULT ret;
    if((ret = hd_videoout_close(p_vdoout_only->out_path)) != HD_OK)
        return ret;
    return HD_OK;
}


static HD_RESULT set_out_param(HD_PATH_ID video_out_path,HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	//buffer size is different from device size,vodout would scale
	video_out_param.dim.w = VDO_SIZE_W;
	video_out_param.dim.h = VDO_SIZE_H;
	video_out_param.pxlfmt = VDO_FMT;
	video_out_param.dir = HD_VIDEO_DIR_NONE;

	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if(ret!=HD_OK)
		return ret;
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if(ret!= HD_OK)
		return ret;
	printf("##video_out_param w:%d,h:%d %x %x\r\n",video_out_param.dim.w,video_out_param.dim.h,video_out_param.pxlfmt,video_out_param.dir);

	return ret;
}
static BOOL check_test_pattern(char *filename)
{
	FILE *f_in;
	char filepath[64];

	sprintf(filepath, filename);

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
    VIDEOOUT_ONLY *p_vdoout_only = (VIDEOOUT_ONLY *)arg;
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_DDR_ID ddr_id = VDO_DDR_ID;
	UINT32 blk_size = YUV_BLK_SIZE;
	UINT32 yuv_size = VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FMT);
	char filepath_yuv_main[64];
	FILE *f_in_main = 0;
	UINT32 pa_yuv_main[MAX_YUV_BLK_CNT] = {0};
	UINT32 va_yuv_main[MAX_YUV_BLK_CNT] = {0};
	INT32  blkidx;
	HD_COMMON_MEM_VB_BLK blk;
    UINT32 readbyte = 0 ;

	//------ [1] wait flow_start ------
	while (p_vdoout_only->start== 0) sleep(1);

	//------ [2] open input files ------
   	sprintf(filepath_yuv_main, p_vdoout_only->filename);
	if ((f_in_main = fopen(filepath_yuv_main, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", filepath_yuv_main);
		return 0;
	}
	//------ [3] feed yuv ------
	while (p_vdoout_only->stop == 0) {

		//--- Get memory ---
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			continue;
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
		readbyte = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
        if(readbyte<yuv_size) {
            printf("read data %d < %d\r\n",readbyte,yuv_size);
        }
		//printf("blkidx %d yuv_size %x %x\r\n ",blkidx,yuv_size,*(UINT32 *)va_yuv_main[blkidx]);

		if (feof(f_in_main)) {
			fseek(f_in_main, 0, SEEK_SET);  //rewind
			readbyte = fread((void *)va_yuv_main[blkidx], 1, yuv_size, f_in_main);
            if(readbyte<yuv_size) {
                printf("read data %d < %d\r\n",readbyte,yuv_size);
            }
		}

		//--- data is written by CPU, flush CPU cache to PHY memory ---
		hd_common_mem_flush_cache((void *)va_yuv_main[blkidx], yuv_size);

		//--- push_in ---
		{
			HD_VIDEO_FRAME video_frame;

			video_frame.sign        = MAKEFOURCC('V','F','R','M');
			video_frame.p_next      = NULL;
			video_frame.ddr_id      = ddr_id;
			video_frame.pxlfmt      = VDO_FMT;
			video_frame.dim.w       = VDO_SIZE_W;
			video_frame.dim.h       = VDO_SIZE_H;
			video_frame.count       = 0;
			video_frame.timestamp   = hd_gettime_us();
			video_frame.loff[0]     = VDO_SIZE_W; // Y
			video_frame.loff[1]     = VDO_SIZE_W; // UV
			video_frame.phy_addr[0] = pa_yuv_main[blkidx];                          // Y
			video_frame.phy_addr[1] = pa_yuv_main[blkidx] + VDO_SIZE_W*VDO_SIZE_H;  // UV pack
			video_frame.blk         = blk;

			ret = hd_videoout_push_in_buf(p_vdoout_only->out_path, &video_frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				printf("push_in error %d!!\r\n",ret);
			}
		}
rel_blk:
		//--- Release memory ---
		ret = hd_common_mem_release_block(blk);
        if (HD_OK != ret) {
				printf("err:release blk fail %d\r\n", ret);
		}

		sleep(3); // sleep 3 sec
	}

	//------ [4] uninit & exit ------
	// mummap for yuv buffer
	for (i=0; i< MAX_YUV_BLK_CNT; i++) {
		if (va_yuv_main[i] != 0)
			hd_common_mem_munmap((void *)va_yuv_main[i], blk_size);
	}

	// close file
	if (f_in_main)  fclose(f_in_main);
	return 0;
}

MAIN(argc, argv)
{
    HD_RESULT ret;
    UINT32 out_type = 1;
    VIDEOOUT_ONLY vdoout_only = {0};
    INT key;
    pthread_t yuv_thread_id;

	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("###out_type %d\r\n",out_type);
	}

    vdoout_only.hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
	// query program options
	if ((argc >= 3)&&(atoi(argv[2])!=0)) {
		vdoout_only.hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", vdoout_only.hdmi_id);
	}
    #if 0  //file format,w,h is limited
	if (argc >= 4) {
		memset((void *)&vdoout_only.filename[0],0,MAX_FILE_NAME);
		strncpy(&vdoout_only.filename[0],argv[3],MAX_FILE_NAME-1);
		printf("###filename %s\r\n",&vdoout_only.filename[0]);
	} else
	#endif
	{
	    strncpy(&vdoout_only.filename[0],PATTEN_PATH,MAX_FILE_NAME-1);
	}

    //init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
        goto exit;
    }
    //vdoout_only memory init
	ret = mem_init();
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
        goto exit;
    }
    //vdoout_only module init
    ret = init_module();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }
    //open vdoout_only module
    ret = open_module(&vdoout_only,out_type);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
        goto exit;
    }

	// check TEST pattern exist
	if (check_test_pattern(vdoout_only.filename) == FALSE) {
		printf("test_pattern isn't exist\r\n");
		goto exit;
	}
	ret = set_out_param(vdoout_only.out_path,&vdoout_only.out_syscaps.output_dim);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		return -1;
	}

	ret = pthread_create(&yuv_thread_id, NULL, feed_yuv_thread, (void *)&vdoout_only);
	if (ret < 0) {
		printf("create encode thread failed");
		return -1;
	}

	hd_videoout_start(vdoout_only.out_path);

	vdoout_only.start = 1;

	printf("Enter q to exit\n");
	while (1) {
		key = GETCHAR();
		if (key == 'q' || key == 0x3) {
			vdoout_only.stop = 1;
			break;
		}

		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
	}

	pthread_join(yuv_thread_id, NULL);
	hd_videoout_stop(vdoout_only.out_path);

exit:
    //close all module
	ret = close_module(&vdoout_only);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
    //uninit all module
	ret = exit_module();
	if(ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
    //uninit memory
    ret = mem_exit();
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
    }

	return 0;
}
