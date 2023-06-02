/**
	@brief Sample code of ai network with sensor input.\n

	@file ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc.c

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
#include "fdcnn_lib.h"
#include "cddcnn_lib_ai2.h"
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
#define MAX_AIOB_NUM        10

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

static CHAR model_name[4][256]	= { {"/mnt/sd/CNNLib/para/pdcnn/nvt_model.bin"},
									{"/mnt/sd/CNNLib/para/fdcnn_method2_light/file1.bin"},
									{"/mnt/sd/CNNLib/para/cddcnn/nvt_model.bin"},
									{"/mnt/sd/CNNLib/para/public/nvt_model.bin"}
	                              }; //PD nvr_model, FD nvt_model

static BOOL is_net_proc						= TRUE;
static BOOL is_net_run						= FALSE;
static UINT32 fps_delay = 0;
static INT32 save_results = 0;

static INT32 PDCNN_FDCNN_CDDCNN = 1;
static INT32 PDCNN_MODE = 1;
static INT32 FDCNN_MODE = 1;
static INT32 CDDCNN_MODE = 1;
static FDCNN_NETWORK_TYPE NN_FDCNN_FD_TYPE;

static UINT32 PD_MAX_DISTANCE_MODE = 2;
static UINT32 CDD_MAX_DISTANCE_MODE = 0;
static UINT32 AI_TOTAL_BUF = 0xfce100;
static UINT32 AI_PD_BUF_SIZE = 0x52f000;
static UINT32 AI_FD_BUF_SIZE = 0x520000;
static UINT32 AI_CDD_BUF_SIZE = 0x5C0000;
static UINT32 AI_OUT_BUF_SIZE = 0xe100;
#if (LIMIT_FDET_PD || LIMIT_FDET_CDD)
static UINT32 AI_LIMIT_FDET_BUF_SIZE = 0xcf000;//0xcef00 ;
#endif



static INT32 g_blk = 0;
typedef struct _PD_FD_CDD_THREAD_PARM {
	VENDOR_AIS_FLOW_MEM_PARM rslt_mem;
	VENDOR_AIS_FLOW_MEM_PARM pd_mem;
	VENDOR_AIS_FLOW_MEM_PARM cdd_mem;
	VENDOR_AIS_FLOW_MEM_PARM fd_mem;
	VENDOR_AIS_FLOW_MEM_PARM limit_fdet_mem;
	VIDEO_LIVEVIEW stream;	
} PD_FD_CDD_THREAD_PARM;

typedef struct _AI_RSLT_PARM {
	VENDOR_AIS_FLOW_MEM_PARM rslt_mem;
	UINT32 obj_num;
} AI_RSLT_PARM;


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
	blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, AI_TOTAL_BUF, ddr_id);
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
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, AI_TOTAL_BUF);
	//g_blk_info[0] = blk;

	// Release buffer
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, AI_TOTAL_BUF);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}

	buf->pa = pa;
	buf->va = va;
	buf->size = AI_TOTAL_BUF;
	g_blk = blk;

exit:
	// - end nn get block

	return ret;
}

HD_RESULT mem_exit(VENDOR_AIS_FLOW_MEM_PARM buf)
{
	HD_RESULT ret = HD_OK;
	/* Release in buffer */
	if(buf.va > 0) {
		ret = hd_common_mem_munmap((void *)buf.va, AI_TOTAL_BUF);
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

	hd_common_mem_uninit();
	return ret;
}

HD_RESULT mem_select(void)
{
	HD_RESULT ret = HD_OK;
	if (PDCNN_MODE==1){
		if (PD_MAX_DISTANCE_MODE> 0){
			AI_PD_BUF_SIZE =0x52f000; //0x560000;
		}else{
			AI_PD_BUF_SIZE = 0x46f000;//0x4E0000;
		}
	}
	if (FDCNN_MODE == 1){
		AI_FD_BUF_SIZE = 0x520000;
	}
	if (CDDCNN_MODE == 1){
		if (CDD_MAX_DISTANCE_MODE> 0){
			AI_CDD_BUF_SIZE = 0x5C0000;
		}else{
			AI_CDD_BUF_SIZE = 0x500000;
		}
	}
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
///////////////////////// (PDCNN_MODE || CDDCNN_MODE) ///////////////////////////
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

////////////////// (PDCNN_MODE || FDCNN_MODE || CDDCNN_MODE) /////////////////////
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
	//mem_parm->size = size;

	if (fd) {
		fclose(fd);
	};
	printf("model buf size: %d\r\n", size);
	return HD_OK;
}
HD_RESULT ai_rslt_trans_info(AI_RSLT_PARM *rslts)
{
	PDCNN_RESULT* det_rslts = (PDCNN_RESULT*)rslts->rslt_mem.va;

	{
		PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;
		PD_SHM_RESULT *p_obj;
		UINT32         i;

		if (p_pd_shm->exit) {
			return HD_ERR_NOT_SUPPORT;
		}

		// update pdcnn result to share memory
		p_pd_shm->pd_num = rslts->obj_num;
		if(rslts->obj_num > 10){
			p_pd_shm->pd_num = 10;
		}
		for (i = 0; i < p_pd_shm->pd_num; i++) {
			p_obj = &p_pd_shm->pd_results[i];
			p_obj->category = det_rslts[i].category;
			p_obj->score = det_rslts[i].score;
			p_obj->x1 = det_rslts[i].x1;
			p_obj->x2 = det_rslts[i].x2;
			p_obj->y1 = det_rslts[i].y1;
			p_obj->y2 = det_rslts[i].y2;
		}
	}

    return HD_OK;
}




/////////////////////////// PDCNN_MODE //////////////////////////////////
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
		printf("ERR: pdcnn version check net fail (%d)!!\r\n", ret);
		return ret;
	}
#endif
	// open pdcnn net
	ret = network_open(pd_proposal_params.run_id, &(pdcnn_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: pdcnn net open fail !!\r\n");
		return ret;
	}
	//set work buf and assign pdcnn mem
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(pd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("ERR: pdcnn get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n");
		return ret;
	}
	//printf("pdcnn work buf size: %ld\r\n", wbuf.size);
	ret = get_pd_mem(buf, &(pdcnn_mem->io_mem), wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}

if (PD_MAX_DISTANCE_MODE>0)
{
	/*get max distance buf*/
	ret = get_pd_mem(buf, &(pdcnn_mem->scale_buf), (PD_YUV_WIDTH * PD_YUV_HEIGHT * 3 / 2), 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get scale_buf fail (%d)!!\r\n", ret);
		return ret;
	}
}
	ret = vendor_ai_net_set(pd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &(pdcnn_mem->io_mem));
	if (ret != HD_OK) {
		printf("ERR: pdcnn net set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail (%d)\r\n", ret);
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

VOID ai_rslt_integrate_pd(AI_RSLT_PARM *rslt, PDCNN_MEM *pdcnn_mem)
{
	PDCNN_RESULT *final_results = (PDCNN_RESULT*)rslt->rslt_mem.va;
	INT32 i = 0;

	PDCNN_RESULT *pdcnn_rslts = (PDCNN_RESULT*)pdcnn_mem->final_result.va;
	for(i = 0; i < pdcnn_mem->out_num; i++){
		final_results[rslt->obj_num + i].category = 1;
		final_results[rslt->obj_num + i].score = pdcnn_rslts[i].score;
		final_results[rslt->obj_num + i].x1 = pdcnn_rslts[i].x1;
		final_results[rslt->obj_num + i].y1 = pdcnn_rslts[i].y1;
		final_results[rslt->obj_num + i].x2 = pdcnn_rslts[i].x2;
		final_results[rslt->obj_num + i].y2 = pdcnn_rslts[i].y2;
	}
	rslt->obj_num += pdcnn_mem->out_num;
}

/////////////////////////////// FDCNN_MODE ////////////////////////////////
VOID ai_rslt_integrate_fd(AI_RSLT_PARM *rslt, VENDOR_AIS_FLOW_MEM_PARM buf)
{
	PDCNN_RESULT *final_results = (PDCNN_RESULT*)rslt->rslt_mem.va;
	UINT32 i = 0;

	FDCNN_RESULT fdcnn_info[MAX_AIOB_NUM] = {0};
	HD_URECT fdcnn_size = {0, 0, 1024, 576};
	UINT32 fdcnn_num = fdcnn_getresults(buf, fdcnn_info, &fdcnn_size, MAX_AIOB_NUM);
	for(i = 0; i < fdcnn_num; i++){
		final_results[rslt->obj_num + i].category = 2;
		final_results[rslt->obj_num + i].score = (FLOAT)(fdcnn_info[i].score) / (FLOAT)(1 << 14);
		final_results[rslt->obj_num + i].x1 = fdcnn_info[i].x;
		final_results[rslt->obj_num + i].y1 = fdcnn_info[i].y;
		final_results[rslt->obj_num + i].x2 = fdcnn_info[i].x + fdcnn_info[i].w;
		final_results[rslt->obj_num + i].y2 = fdcnn_info[i].y + fdcnn_info[i].h;
	}
	rslt->obj_num += fdcnn_num;
}

////////////////////////////////// CDDCNN_MODE //////////////////////////////
HD_RESULT cddcnn_preset(CDDCNN_MEM *cddcnn_mem, VENDOR_AIS_FLOW_MEM_PARM *buf, PROPOSAL_PARAM cdd_proposal_params)
{
	HD_RESULT ret;

	network_set_opt(cdd_proposal_params.run_id);
 	//UINT32 buf_size = buf->size;
	UINT32 model_size = _getsize_model(model_name[2]);
	if (model_size <= 0) {
		printf("ERR: cddcnn input model not exist: %s\r\n", model_name[2]);
		return HD_ERR_NOT_OPEN;
	}

	ret = get_cdd_mem(buf, &(cddcnn_mem->model_mem), model_size, 32);
	if(ret != HD_OK){
		printf("ERR: cddcnn get model mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = load_model(model_name[2], cddcnn_mem->model_mem.va);
	if (ret != HD_OK) {
		printf("ERR: pdcnn load model fail (%ld)!!\r\n", ret);
		return ret;
	}

	cddcnn_get_version();
	ret = cddcnn_version_check(&(cddcnn_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: cddcnn version check fail (%d)!!\r\n", ret);
		return ret;
	}
	// open cddcnn network
	ret = network_open(cdd_proposal_params.run_id, &(cddcnn_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: pdcnn network_open fail !!\r\n");
		return ret;
	}
	//set work buf and assign cddcnn mem
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};
	ret = vendor_ai_net_get(cdd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("ERR: cddcnn get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n");
		return ret;
	}
	//printf("work buf size: %ld\r\n", wbuf.size);
	ret = get_cdd_mem(buf, &(cddcnn_mem->io_mem), wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: cddcnn get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}

    if (CDD_MAX_DISTANCE_MODE>0){
		ret = get_cdd_mem(buf, &(cddcnn_mem->scale_buf), (YUV_WIDTH * YUV_HEIGHT * 3 / 2), 32);
		if(ret != HD_OK){
			printf("ERR: cddcnn get scale_buf fail (%d)!!\r\n", ret);
			return ret;
		}
	}

	ret = vendor_ai_net_set(cdd_proposal_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &(cddcnn_mem->io_mem));
	if (ret != HD_OK) {
		printf("ERR: cddcnn set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail (%d)\r\n", ret);
		return ret;
	}

	ret = get_post_mem(buf, cddcnn_mem);
	if(ret != HD_OK){
		printf("ERR: get cddcnn postprocess mem fail (%d)!!\r\n", ret);
		return ret;
	}

	return HD_OK;
}

VOID ai_rslt_integrate_cdd(AI_RSLT_PARM *rslt, CDDCNN_MEM *cddcnn_mem)
{
	PDCNN_RESULT *final_results = (PDCNN_RESULT*)rslt->rslt_mem.va;
	INT32 i = 0;

	CDDCNN_RESULT *cddcnn_rslts = (CDDCNN_RESULT*)cddcnn_mem->final_result.va;
	for(i = 0; i < cddcnn_mem->out_num; i++){
		final_results[rslt->obj_num + i].category = 3;
		final_results[rslt->obj_num + i].score = cddcnn_rslts[i].score;
		final_results[rslt->obj_num + i].x1 = cddcnn_rslts[i].x1;
		final_results[rslt->obj_num + i].y1 = cddcnn_rslts[i].y1;
		final_results[rslt->obj_num + i].x2 = cddcnn_rslts[i].x2;
		final_results[rslt->obj_num + i].y2 = cddcnn_rslts[i].y2;
	}
	rslt->obj_num += cddcnn_mem->out_num;
}

////////////////////////////limit_fdet mode/////////////////////////
HD_RESULT limit_fdet_preset(LIMIT_FDET_MEM *limit_fdet_mem, VENDOR_AIS_FLOW_MEM_PARM *buf, LIMIT_FDET_PARAM limit_fdet_params)
{
	HD_RESULT ret;
	network_set_opt(limit_fdet_params.run_id);

	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->input_mem), LIMIT_FDET_WIDTH * LIMIT_FDET_WIDTH * 3 / 2, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get input_mem fail (%d)!!\r\n", ret);
		return ret;
	}

	UINT32 limit_net_size = _getsize_model(model_name[3]);
	if (limit_net_size <= 0) {
		printf("ERR: limit_fdet model not exist: %s\r\n", model_name[3]);
		return -1;
	}

	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->model_mem), limit_net_size, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get model mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = load_model(model_name[3], limit_fdet_mem->model_mem.va);
	if(ret != HD_OK){
		printf("ERR: limit_fdet load model fail (%ld)!!\r\n", ret);
		return ret;
	}
	ret = limit_fdet_version_check(&(limit_fdet_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: limit_fdet version check fail (%d)!!\r\n", ret);
		return ret;
	}
	// open cls net
	ret = network_open(limit_fdet_params.run_id, &(limit_fdet_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: limit_fdet open fail !!\r\n");
		return ret;
	}

	//set cls work buf
	VENDOR_AI_NET_CFG_WORKBUF limit_fdet_wbuf = {0};
	ret = vendor_ai_net_get(limit_fdet_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &limit_fdet_wbuf);
	if (ret != HD_OK) {
		printf("ERR: limit_fdet get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail (%ld)\r\n", ret);
		return ret;
	}

	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->io_mem), limit_fdet_wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = vendor_ai_net_start(limit_fdet_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet start fail!!\r\n");
		return ret;
	}
	
	return HD_OK;
}

///////////////////////////////////////////////////////////////////////////////
HD_RESULT assign_ai_buf(VENDOR_AIS_FLOW_MEM_PARM *buf, PD_FD_CDD_THREAD_PARM *parm)
{
	HD_RESULT ret = HD_OK;

	mem_select();
	parm->pd_mem.pa = buf->pa;
	parm->pd_mem.va = buf->va;
if (PDCNN_MODE==1){
	parm->pd_mem.size = AI_PD_BUF_SIZE;
}

	parm->fd_mem.pa = parm->pd_mem.pa + parm->pd_mem.size;
	parm->fd_mem.va = parm->pd_mem.va + parm->pd_mem.size;
if (FDCNN_MODE==1){
	parm->fd_mem.size = AI_FD_BUF_SIZE;
}

	parm->cdd_mem.pa = parm->fd_mem.pa + parm->fd_mem.size;
	parm->cdd_mem.va = parm->fd_mem.va + parm->fd_mem.size;
if (CDDCNN_MODE==1){
	parm->cdd_mem.size = AI_CDD_BUF_SIZE;
}

	parm->limit_fdet_mem.pa = parm->cdd_mem.pa + parm->cdd_mem.size;
	parm->limit_fdet_mem.va = parm->cdd_mem.va + parm->cdd_mem.size;
if ((LIMIT_FDET_PD && PDCNN_MODE) || (LIMIT_FDET_CDD && CDDCNN_MODE)){
	parm->limit_fdet_mem.size = AI_LIMIT_FDET_BUF_SIZE;
}

	parm->rslt_mem.pa = parm->limit_fdet_mem.pa + parm->limit_fdet_mem.size;
	parm->rslt_mem.va = parm->limit_fdet_mem.va + parm->limit_fdet_mem.size;
	parm->rslt_mem.size = AI_OUT_BUF_SIZE;

	UINT32 all_bufsize = parm->pd_mem.size + parm->fd_mem.size + parm->cdd_mem.size + parm->rslt_mem.size + parm->limit_fdet_mem.size;

	if(all_bufsize > AI_TOTAL_BUF){
		printf("ERR: ai flow require mem size: %ld, but only allocate mem size: %ld.\r\n", all_bufsize, AI_TOTAL_BUF);
		return HD_ERR_NOMEM;
	}

	return ret;
}

static VOID *pd_fd_cdd_thread_api(VOID *arg)
{
#if AI_NN_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif

	HD_RESULT ret;
	PD_FD_CDD_THREAD_PARM* pd_fd_cdd_parm = (PD_FD_CDD_THREAD_PARM*)arg;
	BOOL image_init = FALSE;
	UINT32 yuv_va = 0;
	HD_VIDEO_FRAME video_frame = {0};


	VENDOR_AI_BUF pd_cdd_src_img = {0};
	HD_GFX_IMG_BUF fd_src_img={0};
	VENDOR_AIS_FLOW_MEM_PARM fdcnn_buf = pd_fd_cdd_parm->fd_mem;
	VENDOR_AIS_FLOW_MEM_PARM pdcnn_buf = pd_fd_cdd_parm->pd_mem;
	PD_PROPOSAL_PARAM pd_proposal_params={0};
	PDCNN_MEM pdcnn_mem = {0};
	PD_LIMIT_PARAM pd_limit_param={0};
	PD_BACKBONE_OUT* pd_backbone_outputs=NULL;
	VENDOR_AIS_FLOW_MEM_PARM cddcnn_buf = pd_fd_cdd_parm->cdd_mem;
	PROPOSAL_PARAM cdd_proposal_params;
	CDDCNN_MEM cddcnn_mem = {0};
	LIMIT_PARAM cdd_limit_param;
	BACKBONE_OUT* cdd_backbone_outputs = NULL;

	//PD_LIMIT_PARAM limit_param={0};

#if DBG_OBJ_INFOR
	FLOAT dump_ratios[2] = {0};
#endif
	AI_RSLT_PARM det_rslts = {0};
	det_rslts.rslt_mem = pd_fd_cdd_parm->rslt_mem;
	VIDEO_LIVEVIEW stream = pd_fd_cdd_parm->stream;

#if (LIMIT_FDET_PD || LIMIT_FDET_CDD)
	LIMIT_FDET_PARAM limit_fdet_params={0};
	LIMIT_FDET_MEM limit_fdet_mem = {0};
	limit_fdet_params.run_id = 3;
	VENDOR_AIS_FLOW_MEM_PARM all_limit_buf = pd_fd_cdd_parm->limit_fdet_mem;
#endif


if (PDCNN_MODE==1)
{
	FLOAT pd_score_thr = 0.45, pd_nms_thr = 0.1;
	CHAR pd_para_file[] = "/mnt/sd/CNNLib/para/pdcnn/para.txt";
	pd_proposal_params.score_thres = pd_score_thr;
	pd_proposal_params.nms_thres = pd_nms_thr;
	pd_proposal_params.run_id = 0;
	#if LIMIT_FDET_PD
	pd_limit_param.limit_fdet = 1;
	#endif	
	if (PD_MAX_DISTANCE_MODE > 0){
	pd_limit_param.max_distance = 1;
	pd_limit_param.sm_thr_num = 2;
	}else{
	pd_limit_param.max_distance = 0;
	pd_limit_param.sm_thr_num = 6;
    }

	ret = pdcnn_preset(&pdcnn_mem, &pdcnn_buf, pd_proposal_params);
	if(ret != HD_OK){
		printf("ERR: pdcnn_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
    pd_backbone_outputs = (PD_BACKBONE_OUT*)pdcnn_mem.backbone_output.va;

	// start pdcnn network
	ret = vendor_ai_net_start(pd_proposal_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: pdcnn net1 start fail!!\r\n");
		goto exit_thread;
	}

	ret = pdcnn_init(&pd_proposal_params, pd_backbone_outputs, &pd_limit_param, pd_para_file);
	if(ret != HD_OK){
		printf("ERR: pdcnn init fail!\r\n");
		goto exit_thread;
	}
	//printf("end pdcnn_init\n");
}

if (FDCNN_MODE == 1)
{
	//printf("start FDCNN_MODE .\n");
	UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
    ret = load_model(model_name[1], model_addr_1);
	if (ret != HD_OK){
		printf("ERR: fdcnn load model fail!!\r\n");
		goto exit_thread;
	}

	FDCNN_INIT_PARM init_parm = {0};
    init_parm.net_type = NN_FDCNN_FD_TYPE;
    init_parm.net_id = 1;

    ret = fdcnn_init(fdcnn_buf, init_parm);
    if (ret != HD_OK)
    {
        printf("fdcnn_init fail=%d\n", ret);
        return 0;
    }
}

if (CDDCNN_MODE==1)
{
	//printf("start CDDCNN_MODE .\n");
	FLOAT cdd_score_thr = 0.55, cdd_nms_thr = 0.1;
	CHAR cdd_para_file[] = "/mnt/sd/CNNLib/para/cddcnn/para.txt";

	cdd_limit_param.ratiow = (FLOAT)1920 / (FLOAT)YUV_WIDTH;
	cdd_limit_param.ratioh = (FLOAT)1080 / (FLOAT)YUV_HEIGHT;
	cdd_limit_param.sm_thr_num = 6;
	cdd_proposal_params.score_thres = cdd_score_thr;
	cdd_proposal_params.nms_thres = cdd_nms_thr;
	cdd_proposal_params.run_id = 2;

	ret = cddcnn_preset(&cddcnn_mem, &cddcnn_buf, cdd_proposal_params);
	if(ret != HD_OK){
		printf("ERR: cddcnn_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
	cdd_backbone_outputs = (BACKBONE_OUT*)cddcnn_mem.backbone_output.va;
	// start cddcnn network
	ret = vendor_ai_net_start(cdd_proposal_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: cddcnn start fail!!\r\n");
		goto exit_thread;
	}
	ret = cddcnn_init(&cdd_proposal_params, cdd_backbone_outputs, &cdd_limit_param, cdd_para_file);
	if(ret != HD_OK){
		printf("ERR: cddcnn init fail!\r\n");
		goto exit_thread;
	}
}

if ((LIMIT_FDET_PD && PDCNN_MODE) || (LIMIT_FDET_CDD && CDDCNN_MODE)){	
	ret = limit_fdet_preset(&limit_fdet_mem, &all_limit_buf, limit_fdet_params);
	if(ret != HD_OK){
		printf("ERR: limit_fdet_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
}


	UINT32 ai_pd_frame = 0;
	do {
        if (is_net_run) {
			//printf("before hd_videoproc_pull_out_buf. \n");
			ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("ERR : pd_thread hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto exit_thread;
			}
			//printf("after hd_videoproc_pull_out_buf. \n");
#if AI_NN_PROF
            gettimeofday(&tstart0, NULL);
#endif
			if(!image_init){
				yuv_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], video_frame.pw[0]*video_frame.ph[0]*3/2);
#if DBG_OBJ_INFOR
				dump_ratios[0] = (FLOAT)2560 / (FLOAT)video_frame.dim.w;
				dump_ratios[1] = (FLOAT)1440 / (FLOAT)video_frame.dim.h;
#endif
				if (PDCNN_MODE==1|| CDDCNN_MODE==1)
				{
					pd_cdd_src_img.width   	= video_frame.dim.w;
					pd_cdd_src_img.height  	= video_frame.dim.h;
					pd_cdd_src_img.channel 	= 2;
					pd_cdd_src_img.line_ofs	= video_frame.loff[0];
					pd_cdd_src_img.fmt 		= video_frame.pxlfmt;
					pd_cdd_src_img.pa		= video_frame.phy_addr[0];
					pd_cdd_src_img.va		= yuv_va;
					pd_cdd_src_img.sign     = MAKEFOURCC('A','B','U','F');
					pd_cdd_src_img.size     = video_frame.loff[0]*video_frame.dim.h*3/2;
				}
				if (FDCNN_MODE==1)
				{
					fd_src_img.dim.w  = video_frame.dim.w;
			        fd_src_img.dim.h = video_frame.dim.h;
			        fd_src_img.format = VDO_FRAME_FORMAT;
			        fd_src_img.p_phy_addr[0] = video_frame.phy_addr[0];
			        fd_src_img.p_phy_addr[1] = video_frame.phy_addr[1];
			        fd_src_img.p_phy_addr[2] = video_frame.phy_addr[1]; // for avoid hd_gfx_scale message
			        fd_src_img.lineoffset[0] = video_frame.loff[0];
			        fd_src_img.lineoffset[1] = video_frame.loff[0]; // for avoid hd_gfx_scale message
			        fd_src_img.lineoffset[2] = video_frame.loff[0]; // for avoid hd_gfx_scale message
				}
				image_init = TRUE;
			}
			if (PDCNN_MODE==1)
			{
	            ret = pdcnn_process(&pd_proposal_params, &pdcnn_mem, &pd_limit_param, pd_backbone_outputs, &pd_cdd_src_img, (UINT32)PD_MAX_DISTANCE_MODE);
				if(ret != HD_OK){
					printf("ERR: pdcnn_process fail!\r\n");
					goto exit_thread;
				}
			#if LIMIT_FDET_PD
				limit_fdet_params.ratiow = (FLOAT)pd_cdd_src_img.width / (FLOAT)PD_YUV_WIDTH;
				limit_fdet_params.ratioh = (FLOAT)pd_cdd_src_img.height / (FLOAT)PD_YUV_HEIGHT;
				limit_fdet_params.cls_id = 0;
				limit_fdet_params.proc_num = pdcnn_mem.out_num;
				limit_fdet_mem.det_in_mem = pdcnn_mem.out_result;
				limit_fdet_mem.det_out_mem = pdcnn_mem.final_result;
				ret = limit_fdet_process(&limit_fdet_params, &limit_fdet_mem, &pd_cdd_src_img);
				if(ret != HD_OK){
					printf("ERR: pd limit_fdet_process fail (%ld)!\r\n", ret);
					goto exit_thread;
				}
				pdcnn_mem.out_num = limit_fdet_params.proc_num;
			#endif
				ai_rslt_integrate_pd(&det_rslts, &pdcnn_mem);
				//printf("after pdcnn_process .\n");
			}
			if (FDCNN_MODE==1)
			{
				ret = fdcnn_set_image(fdcnn_buf, &fd_src_img);
				if (ret != HD_OK)
				{
					printf("fdcnn_set_image fail=%d\n", ret);
					goto exit_thread;
				}

				ret = fdcnn_process(fdcnn_buf);
				if (ret != HD_OK)
				{
					printf("fdcnn_process fail=%d\n", ret);
					goto exit_thread;
				}
				ai_rslt_integrate_fd(&det_rslts, fdcnn_buf);
			}
			if (CDDCNN_MODE==1)
			{
				ret = cddcnn_process(&cdd_proposal_params, &cddcnn_mem, &cdd_limit_param, cdd_backbone_outputs, &pd_cdd_src_img,(UINT32)CDD_MAX_DISTANCE_MODE);
				if(ret != HD_OK){
					printf("ERR: cddcnn_process fail!\r\n");
					goto exit_thread;
				}
			#if LIMIT_FDET_CDD
				limit_fdet_params.ratiow = 1;
				limit_fdet_params.ratioh = 1;
				limit_fdet_params.cls_id = 1;
				limit_fdet_params.proc_num = cddcnn_mem.out_num;
				limit_fdet_mem.det_in_mem = cddcnn_mem.out_result;
				limit_fdet_mem.det_out_mem = cddcnn_mem.final_result;
				ret = limit_fdet_process(&limit_fdet_params, &limit_fdet_mem, &pd_cdd_src_img);
				if(ret != HD_OK){
					printf("ERR: cdd limit_fdet_process fail (%ld)!\r\n", ret);
					goto exit_thread;
				}
				cddcnn_mem.out_num = limit_fdet_params.proc_num;
			#endif
				ai_rslt_integrate_cdd(&det_rslts, &cddcnn_mem);
			}

#if AI_NN_PROF
            gettimeofday(&tend0, NULL);
            cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
            sum_time0 += cur_time0;
            mean_time0 = sum_time0 / (++icount);
            printf("[AI] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
#endif
			if (PDCNN_MODE==1 || FDCNN_MODE==1 || CDDCNN_MODE==1)
			{
				PDCNN_RESULT *ai_final_rslts = (PDCNN_RESULT*)det_rslts.rslt_mem.va;
				det_rslts.obj_num = pdcnn_nonmax_suppress(ai_final_rslts, det_rslts.obj_num, 0.8, 1, MAX_AIOB_NUM);
				ret = ai_rslt_trans_info(&det_rslts);
				if(ret != HD_OK){
				  //printf("ERR: ai_rslt_trans_info fail (%d)!!\r\n", ret);
					printf("WRN: shm info may be not exist or programm will exit. \r\n");
				}

				if(save_results){
					if (det_rslts.obj_num > 0) {
						CHAR TXT_FILE[256], YUV_FILE[256];
						FILE *fs, *fb;
						sprintf(TXT_FILE, "/mnt/sd/det_results/AI/txt/%09ld.txt", ai_pd_frame);
						sprintf(YUV_FILE, "/mnt/sd/det_results/AI/yuv/%09ld.bin", ai_pd_frame);
						fs = fopen(TXT_FILE, "w+");
						fb = fopen(YUV_FILE, "wb+");
						fwrite((UINT32 *)yuv_va, sizeof(UINT32), (video_frame.dim.w * video_frame.dim.h * 3 / 2), fb);
						fclose(fb);

						for(UINT32 num = 0; num < det_rslts.obj_num; num++){
							INT32 xmin = (INT32)(ai_final_rslts[num].x1);
							INT32 ymin = (INT32)(ai_final_rslts[num].y1);
							INT32 width = (INT32)(ai_final_rslts[num].x2 - xmin);
							INT32 height = (INT32)(ai_final_rslts[num].y2 - ymin);
							INT32 category = (INT32)(ai_final_rslts[num].category);
							FLOAT score = ai_final_rslts[num].score;
							fprintf(fs, "%d %f %d %d %d %d\r\n", category, score, xmin, ymin, width, height);
						}
						fclose(fs);
						ai_pd_frame++;
					}
				}
				#if DBG_OBJ_INFOR
				if (det_rslts.obj_num > 0) {
					for(UINT32 num = 0; num < det_rslts.obj_num; num++){
						INT32 xmin = (INT32)(ai_final_rslts[num].x1 * dump_ratios[0]);
						INT32 ymin = (INT32)(ai_final_rslts[num].y1 * dump_ratios[1]);
						INT32 width = (INT32)(ai_final_rslts[num].x2 * dump_ratios[0] - xmin);
						INT32 height = (INT32)(ai_final_rslts[num].y2 * dump_ratios[1] - ymin);
						INT32 category = (INT32)(ai_final_rslts[num].category);
						FLOAT score = ai_final_rslts[num].score;
						printf("Objects: %d %f %d %d %d %d\r\n", category, score, xmin, ymin, width, height);
					}
				}
					
				#endif
				
			}
			
			det_rslts.obj_num = 0;
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
	ret = hd_common_mem_munmap((void *)yuv_va, video_frame.pw[0]*video_frame.ph[0]*3/2);
	if (ret != HD_OK) {
		printf("ERR: (yuv_va)hd_common_mem_munmap fail\r\n");
		goto exit_thread;
	}

exit_thread:
	if (PDCNN_MODE==1)
	{
		ret = vendor_ai_net_stop(pd_proposal_params.run_id);
		if (HD_OK != ret) {
			printf("ERR: pdcnn net1 stop fail (ret)!!\r\n", ret);
		}

		ret = network_close(pd_proposal_params.run_id);
		if(ret != HD_OK) {
			printf("ERR: pdcnn net1 close fail (%d)!!\r\n", ret);
		}
	}
	if (FDCNN_MODE==1)
	{
	    ret = fdcnn_uninit(fdcnn_buf);
	    if (ret != HD_OK) {
	        printf("ERR: fdcnn_uninit fail (%d)!!\r\n", ret);
	    }
	}
	if (CDDCNN_MODE==1)
	{
		ret = vendor_ai_net_stop(cdd_proposal_params.run_id);
		if (HD_OK != ret) {
			printf("ERR: cddcnn stop fail (%d)!!\r\n", ret);
		}

		ret = network_close(cdd_proposal_params.run_id);
		if(ret != HD_OK) {
			printf("ERR: cddcnn network close fail (%d)!!\r\n", ret);
		}
	}
if ((LIMIT_FDET_PD && PDCNN_MODE) || (LIMIT_FDET_CDD && CDDCNN_MODE)){
	ret = vendor_ai_net_stop(limit_fdet_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet stop fail (%d)!!\r\n", ret);
	}

	ret = network_close(limit_fdet_params.run_id);
	if(ret != HD_OK) {
		printf("ERR: limit_fdet close fail (%d)!!\r\n", ret);
	}
}

	return 0;
}

MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key;
	PD_FD_CDD_THREAD_PARM pd_fd_cdd_parm = {0};
	VIDEO_LIVEVIEW stream = {0};

	pthread_t pd_fd_cdd_thread_id;

	int    start_run = 0;
	//int    switch_mode = 1;
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

	if (argc >= 5) {
		PDCNN_FDCNN_CDDCNN =  atoi(argv[4]);
		CDDCNN_MODE = PDCNN_FDCNN_CDDCNN % 2;
		PDCNN_FDCNN_CDDCNN = PDCNN_FDCNN_CDDCNN /10;
		FDCNN_MODE = PDCNN_FDCNN_CDDCNN % 2;
		PDCNN_FDCNN_CDDCNN = PDCNN_FDCNN_CDDCNN /10;
		PDCNN_MODE = PDCNN_FDCNN_CDDCNN % 2;
		PDCNN_FDCNN_CDDCNN = PDCNN_FDCNN_CDDCNN /10;

		if (FDCNN_MODE==1) NN_FDCNN_FD_TYPE=FDCNN_NETWORK_V21;

		printf("PDCNN_MODE=%d FDCNN_MODE=%d CDDCNN_MODE=%d \n",PDCNN_MODE,FDCNN_MODE,CDDCNN_MODE);
	}

	if (argc >= 6){
		PD_MAX_DISTANCE_MODE = atoi(argv[5]);
		printf("PD_MAX_DISTANCE_MODE=%d \n", PD_MAX_DISTANCE_MODE);
	}

	if (argc >= 7){
		CDD_MAX_DISTANCE_MODE = atoi(argv[6]);
		printf("CDD_MAX_DISTANCE_MODE=%d \n", CDD_MAX_DISTANCE_MODE);
	}

	if (argc >= 8){
		AI_TOTAL_BUF = atoi(argv[7]);
		printf("AI_TOTAL_BUF=%d \n", AI_TOTAL_BUF);
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
	VENDOR_AIS_FLOW_MEM_PARM allbuf = {0};
	ret = mem_init(ddr_id, &allbuf);
	if(ret != HD_OK){
		printf("AI all mem get fail (%d)!!\r\n", ret);
		goto exit;
	}

	ret = assign_ai_buf(&allbuf, &pd_fd_cdd_parm);
	if(ret != HD_OK){
		printf("ERR: ai buf assign fail (%ld)!!\r\n", ret);
		goto exit;
	}
	printf("pdcnn_allbuf: pa=(%#x), va=(%#x), size=(%ld)\r\n", pd_fd_cdd_parm.pd_mem.pa, pd_fd_cdd_parm.pd_mem.va, pd_fd_cdd_parm.pd_mem.size);
	printf("fdcnn_allbuf: pa=(%#x), va=(%#x), size=(%ld)\r\n", pd_fd_cdd_parm.fd_mem.pa, pd_fd_cdd_parm.fd_mem.va, pd_fd_cdd_parm.fd_mem.size);
	printf("cddcnn_allbuf: pa=(%#x), va=(%#x), size=(%ld)\r\n", pd_fd_cdd_parm.cdd_mem.pa, pd_fd_cdd_parm.cdd_mem.va, pd_fd_cdd_parm.cdd_mem.size);
	printf("limit_fdet_buf: pa=(%#x), va=(%#x), size=(%ld)\r\n", pd_fd_cdd_parm.limit_fdet_mem.pa, pd_fd_cdd_parm.limit_fdet_mem.va, pd_fd_cdd_parm.limit_fdet_mem.size);
	printf("rslt_outbuf: pa=(%#x), va=(%#x), size=(%ld)\r\n", pd_fd_cdd_parm.rslt_mem.pa, pd_fd_cdd_parm.rslt_mem.va, pd_fd_cdd_parm.rslt_mem.size);
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
	pd_fd_cdd_parm.stream = stream;

	ret = pthread_create(&pd_fd_cdd_thread_id, NULL, pd_fd_cdd_thread_api, (VOID*)(&pd_fd_cdd_parm));
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

	pthread_join(pd_fd_cdd_thread_id, NULL);


exit:

	// unint hd_gfx
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail!!\r\n");
	}
	ret = mem_exit(allbuf);
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
