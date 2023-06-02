/**
	@brief Sample code of ai network with sensor input.\n

	@file ai2_pdcnn_sample_stream_ipc.c

	@author Nan Xu

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
#include <sys/time.h>
#include <dirent.h>

#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include "pdcnn_lib_ai2.h"
#include "pd_shm.h"
#include "limit_fdet_lib.h"


// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_liveview, argc, argv)
#endif


///////////////////////////////////////////////////////////////////////////////
#define AI_IPC              ENABLE
#define DBG_OBJ_INFOR       DISABLE
#define PD_MAX_DISTANCE_MODE    2
#if PD_MAX_DISTANCE_MODE
#define PD_BASE_BUF			(0x52f000)
#else
#define PD_BASE_BUF         (0x46f000)
#endif
#if LIMIT_FDET_PD
#define AI_PD_BUF_SIZE      (PD_BASE_BUF + 0xcf000)
#else
#define AI_PD_BUF_SIZE      PD_BASE_BUF
#endif

#define VENDOR_AI_CFG  		0x000f0000  //ai project config

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420

#define AI_NN_PROF		ENABLE

#define HD_VIDEOPROC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

typedef struct _VIDEO_LIVEVIEW {
#if(!AI_IPC)
	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;
#endif
	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

#if(!AI_IPC)
	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
#endif
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

static CHAR model_name[2][256]	= { {"/mnt/sd/CNNLib/para/pdcnn/nvt_model.bin"},
									{"/mnt/sd/CNNLib/para/public/nvt_model.bin"}
	                              }; 

static BOOL is_net_proc						= TRUE;
static BOOL is_net_run						= FALSE;
static UINT32 fps_delay = 0;
static INT32 save_results = 0;
static INT32 g_blk = 0;
typedef struct _PD_THREAD_PARM {
	VENDOR_AIS_FLOW_MEM_PARM pd_mem;
	VIDEO_LIVEVIEW stream;
} PD_THREAD_PARM;

#if(AI_IPC)
static char   *g_shm = NULL;

void init_share_memory(void)
{
	int shmid = 0;
	key_t key;

	// Segment key.
	key = PD_SHM_KEY;
	// Create the segment.
	if( ( shmid = shmget( key, PD_SHMSZ, 0666 ) ) < 0 ) {
		perror( "shmget" );
		exit(1);
	}
	// Attach the segment to the data space.
	if( ( g_shm = shmat( shmid, NULL, 0 ) ) == (char *)-1 ) {
		perror( "shmat" );
		exit(1);
	}
}

void exit_share_memory(void)
{
	shmdt(g_shm);
}
#endif

///////////////////////////////////////////////////////////////////////////////
static HD_RESULT mem_init(UINT32 ddr, VENDOR_AIS_FLOW_MEM_PARM *buf)
{
	HD_RESULT              		ret;
	HD_COMMON_MEM_DDR_ID	  	ddr_id = ddr;
	HD_COMMON_MEM_VB_BLK      	blk;
	UINT32                    	pa, va;


	ret = hd_common_mem_init(NULL);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}

	/* Allocate parameter buffer */
	if (buf->va != 0) {
		printf("err: mem has already been inited\r\n");
		return -1;
	}

	// nn get block ---
	blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, AI_PD_BUF_SIZE, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, AI_PD_BUF_SIZE);

	// Release buffer
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, AI_PD_BUF_SIZE);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}

	buf->pa = pa;
	buf->va = va;
	buf->size = AI_PD_BUF_SIZE;
	g_blk = blk;

exit:
	// - end nn get block

	return ret;
}

HD_RESULT mem_exit(VENDOR_AIS_FLOW_MEM_PARM *buf)
{
	HD_RESULT ret = HD_OK;
	/* Release in buffer */
	if(buf->va > 0) {
		ret = hd_common_mem_munmap((void *)buf->va, AI_PD_BUF_SIZE);
		if (ret != HD_OK) {
			printf("mem_uninit : hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk);
	if (ret != HD_OK) {
		printf("mem_exit: hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	buf->pa = 0;
	buf->va = 0;
	buf->size = 0;
	g_blk = (UINT32)-1;

	hd_common_mem_uninit();
	return ret;
}

//////////////////////////////////////////////////////////////////////////////
static HD_RESULT init_module(void)
{
	HD_RESULT ret;

	if ((ret = hd_videoproc_init()) != HD_OK)
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
//model load
static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail!!!!!!!!\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}
static HD_RESULT network_open(UINT32 proc_id, VENDOR_AIS_FLOW_MEM_PARM *mem)
{
	HD_RESULT ret = HD_OK;

	// set model
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)mem);
	if(ret != HD_OK){
		printf("ERR: vendor_ai_net_set fail (%ld)!!\r\n", ret);
		return ret;
	}
	// open
	ret = vendor_ai_net_open(proc_id);
	if(ret != HD_OK){
		printf("ERR: vendor_ai_net_open fail (%ld)!!\r\n", ret);
		return ret;
	}

	return ret;
}
static HD_RESULT network_close(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;
	// close
	ret = vendor_ai_net_close(proc_id);

	return ret;
}

VOID network_set_opt(UINT32 proc_id)
{
	VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
	cfg_buf_opt.method = VENDOR_AI_NET_BUF_OPT_NONE;
	cfg_buf_opt.ddr_id = DDR_ID0;
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);

	// set job option
	VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
	cfg_job_opt.method = VENDOR_AI_NET_JOB_OPT_LINEAR_O1;
	cfg_job_opt.wait_ms = 0;
	cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
	vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
}

HD_RESULT load_model(CHAR *filename, UINT32 va)
{
	FILE *fd;
	INT32 size = 0;
	fd = fopen(filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return HD_ERR_NOT_OPEN;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	};

	if (fd) {
		fclose(fd);
	};
	return HD_OK;
}
HD_RESULT ai_rslt_trans_info(PDCNN_MEM *pd_mem)
{
	PDCNN_RESULT* final_rslts = (PDCNN_RESULT*)pd_mem->final_result.va;
	{
		PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;
		PD_SHM_RESULT *p_obj;
		UINT32         i;
		
		if (p_pd_shm->exit) {
			return HD_ERR_NOT_SUPPORT;
		}
		
		// update pdcnn result to share memory
		p_pd_shm->pd_num = pd_mem->out_num;
		if(pd_mem->out_num > 10){
			p_pd_shm->pd_num = 10;
		}
		for (i = 0; i < p_pd_shm->pd_num; i++) {
			p_obj = &p_pd_shm->pd_results[i];
			p_obj->category = final_rslts[i].category;
			p_obj->score = final_rslts[i].score;
			p_obj->x1 = final_rslts[i].x1;
			p_obj->x2 = final_rslts[i].x2;
			p_obj->y1 = final_rslts[i].y1;
			p_obj->y2 = final_rslts[i].y2;
		}
	}

    return HD_OK;
}

HD_RESULT pdcnn_preset(PDCNN_MEM *pdcnn_mem, VENDOR_AIS_FLOW_MEM_PARM *buf, PD_PROPOSAL_PARAM pd_proposal_params)
{
	HD_RESULT ret;

	network_set_opt(pd_proposal_params.run_id);
 
	UINT32 model_size = _getsize_model(model_name[0]);
	if (model_size <= 0) {
		printf("ERR: pdcnn input model not exist: %s\r\n", model_name[0]);
		return HD_ERR_NOT_OPEN;
	}
	
	ret = get_pd_mem(buf, &(pdcnn_mem->model_mem), model_size, 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get model mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = load_model(model_name[0], pdcnn_mem->model_mem.va);
	if(ret != HD_OK){
		printf("ERR: pdcnn load model fail (%ld)!!\r\n", ret);
		return ret;
	}

	pdcnn_get_version();
#if 1
	ret = pdcnn_version_check(&(pdcnn_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: pdcnn version check fail (%d)!!\r\n", ret);
		return ret;
	}
#endif
	// open pdcnn net
	ret = network_open(pd_proposal_params.run_id, &(pdcnn_mem->model_mem)); 
	if(ret != HD_OK){
		printf("ERR: pdcnn network_open fail !!\r\n");
		return ret;
	}

	//set work buf and assign pdcnn mem
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(pd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("ERR: pdcnn get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n");
		return ret;
	}

	ret = get_pd_mem(buf, &(pdcnn_mem->io_mem), wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}

#if PD_MAX_DISTANCE_MODE
	/*get max distance buf*/
	ret = get_pd_mem(buf, &(pdcnn_mem->scale_buf), (PD_YUV_WIDTH * PD_YUV_HEIGHT * 3 / 2), 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get scale_buf fail (%d)!!\r\n", ret);
		return ret;
	}
#endif
	ret = vendor_ai_net_set(pd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &(pdcnn_mem->io_mem));
	if (ret != HD_OK) {
		printf("ERR: pdcnn set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail (%d)\r\n", ret);
		return ret;
	}

	//get pdcnn postproc mem
	ret = get_pd_post_mem(buf, pdcnn_mem);
	if(ret != HD_OK){
		printf("ERR: get pdcnn postprocess mem fail (%d)!!\r\n", ret);
		return ret;
	}
	
	return HD_OK;
}

HD_RESULT limit_fdet_preset(LIMIT_FDET_MEM *limit_fdet_mem, VENDOR_AIS_FLOW_MEM_PARM *buf, LIMIT_FDET_PARAM limit_fdet_params)
{
	HD_RESULT ret;

	network_set_opt(limit_fdet_params.run_id);
	
	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->input_mem), LIMIT_FDET_WIDTH * LIMIT_FDET_WIDTH * 3 / 2, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get input_mem fail (%d)!!\r\n", ret);
		return ret;
	}

	UINT32 limit_net_size = _getsize_model(model_name[1]);
	if (limit_net_size <= 0) {
		printf("ERR: limit_fdet model not exist: %s\r\n", model_name[1]);
		return -1;
	}
	
	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->model_mem), limit_net_size, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get model mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = load_model(model_name[1], limit_fdet_mem->model_mem.va);
	if(ret != HD_OK){
		printf("ERR: limit_fdet load model fail (%ld)!!\r\n", ret);
		return ret;
	}
	
	ret = limit_fdet_version_check(&(limit_fdet_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: limit_fdet version check fail (%d)!!\r\n", ret);
		return ret;
	}
	
	// open limit net
	ret = network_open(limit_fdet_params.run_id, &(limit_fdet_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: limit_fdet net open fail (%ld)!!\r\n", ret);
		return ret;
	}
	//set limit net work buf
	VENDOR_AI_NET_CFG_WORKBUF limit_fdet_wbuf = {0};
	ret = vendor_ai_net_get(limit_fdet_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &limit_fdet_wbuf);
	if (ret != HD_OK) {
		printf("ERR: limit_fdet get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail (%ld)!!\r\n", ret);
		return ret;
	}
	//printf("pdcnn work buf size: %ld\r\n", wbuf.size);
	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->io_mem), limit_fdet_wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}
	ret = vendor_ai_net_start(limit_fdet_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet start fail (%ld)!!\r\n", ret);
		return ret;
	}
	
	return HD_OK;
}


static VOID *pd_thread_api(VOID *arg)
{
#if AI_NN_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

	HD_RESULT ret;
	PD_THREAD_PARM* pd_parm = (PD_THREAD_PARM*)arg;
	BOOL image_init = FALSE;
	HD_VIDEO_FRAME video_frame = {0};
	VENDOR_AI_BUF src_img = {0};
	
#if DBG_OBJ_INFOR
	FLOAT dump_ratios[2] = {0};
#endif
	VIDEO_LIVEVIEW stream = pd_parm->stream;
	
	VENDOR_AIS_FLOW_MEM_PARM pdcnn_buf = pd_parm->pd_mem;
	PDCNN_MEM pdcnn_mem = {0};
	FLOAT pd_score_thr = 0.45, pd_nms_thr = 0.1;
	CHAR pd_para_file[] = "/mnt/sd/CNNLib/para/pdcnn/para.txt";
	PD_PROPOSAL_PARAM pd_proposal_params;
	PD_LIMIT_PARAM pd_limit_param = {0};
	pd_proposal_params.score_thres = pd_score_thr;
	pd_proposal_params.nms_thres = pd_nms_thr;
	pd_proposal_params.run_id = 0;
#if LIMIT_FDET_PD
	LIMIT_FDET_PARAM limit_fdet_param = {0};
	LIMIT_FDET_MEM limit_fdet_mem = {0};	
	limit_fdet_param.run_id= 1;
	pd_limit_param.limit_fdet = 1;
#endif

#if PD_MAX_DISTANCE_MODE
	pd_limit_param.max_distance = 1;
	pd_limit_param.sm_thr_num = 2;
#else
	pd_limit_param.max_distance = 0;
	pd_limit_param.sm_thr_num = 6;
#endif

	ret = pdcnn_preset(&pdcnn_mem, &pdcnn_buf, pd_proposal_params);
	if(ret != HD_OK){
		printf("ERR: pdcnn_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}	
	PD_BACKBONE_OUT* pd_backbone_outputs = (PD_BACKBONE_OUT*)pdcnn_mem.backbone_output.va;
	// start pdcnn network
	ret = vendor_ai_net_start(pd_proposal_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: pdcnn start fail!!\r\n");
		goto exit_thread;
	}

	ret = pdcnn_init(&pd_proposal_params, pd_backbone_outputs, &pd_limit_param, pd_para_file);
	if(ret != HD_OK){
		printf("ERR: pdcnn init fail!\r\n");
		goto exit_thread;
	}

#if LIMIT_FDET_PD
	ret = limit_fdet_preset(&limit_fdet_mem, &pdcnn_buf, limit_fdet_param);
	if(ret != HD_OK){
		printf("ERR: limit_fdet_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}

#endif

	UINT32 ai_pd_frame = 0;
	do {
        if (is_net_run) {
			ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("ERR : pd_thread hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto exit_thread;
			}
#if AI_NN_PROF
            gettimeofday(&tstart0, NULL);
#endif
			if(!image_init){
#if DBG_OBJ_INFOR
				dump_ratios[0] = (FLOAT)2560 / (FLOAT)video_frame.dim.w;
				dump_ratios[1] = (FLOAT)1440 / (FLOAT)video_frame.dim.h;
#endif
				src_img.width   	= video_frame.dim.w;
				src_img.height  	= video_frame.dim.h;
				src_img.channel 	= 2;
				src_img.line_ofs	= video_frame.loff[0];
				src_img.fmt 		= video_frame.pxlfmt;
				src_img.pa			= video_frame.phy_addr[0];
				src_img.va			= (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], video_frame.pw[0]*video_frame.ph[0]*3/2);;
				src_img.sign    	= MAKEFOURCC('A','B','U','F');
				src_img.size     	= video_frame.dim.w * video_frame.dim.h * 3 / 2;
				
				image_init = TRUE;
			}

			ret = pdcnn_process(&pd_proposal_params, &pdcnn_mem, &pd_limit_param, pd_backbone_outputs, &src_img, (UINT32)PD_MAX_DISTANCE_MODE);
			if(ret != HD_OK){
				printf("ERR: pdcnn_process fail!\r\n");
				goto exit_thread;
			}

#if LIMIT_FDET_PD
			limit_fdet_param.ratiow = (FLOAT)src_img.width / (FLOAT)PD_YUV_WIDTH;
			limit_fdet_param.ratioh = (FLOAT)src_img.height / (FLOAT)PD_YUV_HEIGHT;
			limit_fdet_param.cls_id = 0;
			limit_fdet_param.proc_num = pdcnn_mem.out_num;
			limit_fdet_mem.det_in_mem = pdcnn_mem.out_result;
			limit_fdet_mem.det_out_mem = pdcnn_mem.final_result;
			ret = limit_fdet_process(&limit_fdet_param, &limit_fdet_mem, &src_img);
			if(ret != HD_OK){
				printf("ERR: limit_fdet_process fail (%ld)!\r\n", ret);
				goto exit_thread;
			}
			pdcnn_mem.out_num = limit_fdet_param.proc_num;
#endif
			
#if AI_NN_PROF
            gettimeofday(&tend0, NULL);
            cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
            sum_time0 += cur_time0;
            mean_time0 = sum_time0 / (++icount);
            printf("[PD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
#endif
			ret = ai_rslt_trans_info(&pdcnn_mem);
			if(ret != HD_OK){
				//printf("WRN: ai_rslt_trans_info fail (%ld) !!\r\n", ret);
				printf("WRN: shm info may be not exist or programm will exit. \r\n");
			}
			if(save_results){
				if (pdcnn_mem.out_num > 0) {
					PDCNN_RESULT* final_rslts = (PDCNN_RESULT*)pdcnn_mem.final_result.va;
					CHAR TXT_FILE[256], YUV_FILE[256];
					FILE *fs, *fb;
					sprintf(TXT_FILE, "/mnt/sd/det_results/PD/txt/%09ld.txt", ai_pd_frame);
					sprintf(YUV_FILE, "/mnt/sd/det_results/PD/yuv/%09ld.bin", ai_pd_frame);
					fs = fopen(TXT_FILE, "w+");
					fb = fopen(YUV_FILE, "wb+");
					fwrite((UINT32 *)src_img.va, sizeof(UINT32), (src_img.width * src_img.height * 3 / 2), fb);
					fclose(fb);

					for(INT32 num = 0; num < pdcnn_mem.out_num; num++){
						INT32 xmin = (INT32)(final_rslts[num].x1);
						INT32 ymin = (INT32)(final_rslts[num].y1);
						INT32 width = (INT32)(final_rslts[num].x2 - xmin);
						INT32 height = (INT32)(final_rslts[num].y2 - ymin);
						INT32 category = (INT32)(final_rslts[num].category);
						FLOAT score = final_rslts[num].score;
						fprintf(fs, "%d %f %d %d %d %d\r\n", category, score, xmin, ymin, width, height);
					}
					fclose(fs);
					ai_pd_frame++;
				}
			}
#if DBG_OBJ_INFOR
			print_pdcnn_results(&pdcnn_mem, dump_ratios[0], dump_ratios[1]);
#endif
            gettimeofday(&tend0, NULL);
			cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
            if ((fps_delay)&&(cur_time0 < fps_delay)) {
                usleep(fps_delay - cur_time0 + (mean_time0*0));
            }

			ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
		    if(ret != HD_OK) {
				printf("ERR : pd_thread hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto exit_thread;
			}

        }
	} while(is_net_proc);
	ret = hd_common_mem_munmap((void *)src_img.va, video_frame.pw[0]*video_frame.ph[0]*3/2);
	if (ret != HD_OK) {
		printf("ERR: (hd_common_mem_munmap fail\r\n");
		goto exit_thread;
	}

exit_thread:

	ret = vendor_ai_net_stop(pd_proposal_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: pdcnn stop fail !!\r\n");
	}
	
	ret = network_close(pd_proposal_params.run_id);
	if(ret != HD_OK) {
		printf("ERR: pdcnn network close fail!!\r\n");
	}
#if LIMIT_FDET_PD
	ret = vendor_ai_net_stop(limit_fdet_param.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet stop fail !!\r\n");
	}
	
	ret = network_close(limit_fdet_param.run_id);
	if(ret != HD_OK) {
		printf("ERR: limit_fdet network close fail!!\r\n");
	}
#endif

	return 0;
}

MAIN(argc, argv)
{
	printf("start ai2_pdcnn_sample_stream_ipc \n");
	HD_RESULT ret;
	INT key;
	PD_THREAD_PARM pd_parm = {0};
	VIDEO_LIVEVIEW stream = {0};

	pthread_t pd_thread_id;

	int    start_run = 0;
	UINT32 cnn_id = 0;
	UINT32 ddr_id = 0;

    fps_delay = 0 ;
	save_results = 0;
	if (argc >= 2) {
		UINT32 fps = atoi(argv[1]);
  		fps_delay = (UINT32)(1000000/fps);
		printf("fps %d fps_delay %d\r\n", fps,fps_delay);
	}
	if (argc >= 3) {
		start_run =  atoi(argv[2]);
	}
	if (argc >= 4) {
		save_results =  atoi(argv[3]);
	}

	// init hdal
#if(!AI_IPC)
	ret = hd_common_init(0);
#else
	ret = hd_common_init(2);
#endif
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	//set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine
#if(AI_IPC)
	init_share_memory();
#endif
	//init gfx for img
	ret = hd_gfx_init();
	if (ret != HD_OK) {
		printf("hd_gfx_init fail\r\n");
		goto exit;
	}

	//ai cfg set
	UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
	vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
	vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		goto exit;
	}
	
	//mem init
	VENDOR_AIS_FLOW_MEM_PARM pd_buf = {0};
	ret = mem_init(ddr_id, &pd_buf);
	if(ret != HD_OK){
		printf("PD all mem get fail (%d)!!\r\n", ret);
		goto exit;
	}
	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	
	if (cnn_id == 0) {
		stream.proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(6));
	} else {
		stream.proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(7));
	}
	pd_parm.stream = stream;
	pd_parm.pd_mem = pd_buf;

	ret = pthread_create(&pd_thread_id, NULL, pd_thread_api, (VOID*)(&pd_parm));
	if (ret < 0) {
		printf("create pd_fd_thread encode thread failed");
		goto exit;
	}

#if(AI_IPC)
	//  run network directly
	is_net_proc = TRUE;
    is_net_run = TRUE;
#endif

	if (!start_run) {
		// query user key
		do {
			printf("usage:\n");
			printf("  enter q: exit\n");
			printf("  enter r: run engine\n");
			key = getchar();
			if (key == 'q' || key == 0x3) {
				is_net_proc = FALSE;
	            is_net_run = FALSE;
				break;
			} else if (key == 'r') {
				//  run network
				is_net_proc = TRUE;
	            is_net_run = TRUE;
				printf("[ai sample] write result done!\n");
				continue;
			}

		} while(1);
	}

	pthread_join(pd_thread_id, NULL);


exit:

	// unint hd_gfx
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail!!\r\n");
	}
	ret = mem_exit(&pd_buf);
	if(ret != HD_OK){
		printf("mem_exit fail!!\r\n");
	}
	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
	// uninit network modules
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}
	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

#if(AI_IPC)
	exit_share_memory();
#endif

	return 0;
}
