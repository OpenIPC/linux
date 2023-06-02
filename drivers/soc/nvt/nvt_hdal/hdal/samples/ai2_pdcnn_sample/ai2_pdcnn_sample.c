/**
	@brief Source file of vendor ai net sample code.

	@file ai2_pdcnn_sample.c

	@ingroup ai2_pdcnn_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_debug.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include <arm_neon.h>
#include <pdcnn_lib_ai2.h>
#include <sys/time.h>
#include "limit_fdet_lib.h"


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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_net_with_buf, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif
#define AI_POST_PROC	1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define NET_PATH_ID		UINT32

#define VENDOR_AI_CFG  				0x000f0000  //vendor ai config

#define SAVE_SCALE		DISABLE
#define PROF			ENABLE

#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    gettimeofday(&tstart, NULL);
	#define PROF_END(msg)   gettimeofday(&tend, NULL);  \
			printf("%s time (us): %lu\r\n", msg,         \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif
#define PD_MAX_DISTANCE_MODE    0
static CHAR model_name[2][256]	= { {"/mnt/sd/CNNLib/para/pdcnn/nvt_model.bin"},
									{"/mnt/sd/CNNLib/para/public/nvt_model.bin"}
	                              };
static VENDOR_AIS_FLOW_MEM_PARM g_mem       = {0};
///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef struct _PD_THREAD_PARM {
	VENDOR_AIS_FLOW_MEM_PARM pd_mem;
	VENDOR_AIS_FLOW_MEM_PARM scale_mem;
} PD_THREAD_PARM;

typedef struct _NET_IN {
	CHAR input_filename[256];
	UINT32 w;
	UINT32 h;
	UINT32 c;
	UINT32 loff;
	UINT32 fmt;
	VENDOR_AI_BUF src_img;
} NET_IN;
static HD_COMMON_MEM_VB_BLK g_blk = 0;
/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

static HD_RESULT mem_get(VOID)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;
	
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL, PD_MAX_MEM_SIZE, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, PD_MAX_MEM_SIZE);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, PD_MAX_MEM_SIZE);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	g_mem.pa = pa;
	g_mem.va = (UINT32)va;
	g_mem.size = PD_MAX_MEM_SIZE;
	g_blk = blk;
	
	return HD_OK;
}



static INT32 mem_load(VENDOR_AIS_FLOW_MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	}
	//mem_parm->size = size;

	if (fd) {
		fclose(fd);
	}

	return size;
}

/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////


static HD_RESULT input_open(NET_IN *p_nn_in, PDCNN_MEM *pdcnn_mem)
{
	HD_RESULT ret = HD_OK;



	UINT32 file_len = mem_load(&(pdcnn_mem->input_mem), p_nn_in->input_filename);
	if (file_len < 0) {
		printf("load buf(%s) fail\r\n", p_nn_in->input_filename);
		return HD_ERR_NG;
	}
	printf("load buf(%s) ok\r\n", p_nn_in->input_filename);
	hd_common_mem_flush_cache((VOID *)pdcnn_mem->input_mem.va, file_len);

	p_nn_in->src_img.width 		= p_nn_in->w;
	p_nn_in->src_img.height 	= p_nn_in->h;
	p_nn_in->src_img.channel 	= p_nn_in->c;
	p_nn_in->src_img.line_ofs 	= p_nn_in->loff;
	p_nn_in->src_img.fmt      	= p_nn_in->fmt;
	p_nn_in->src_img.pa      	= pdcnn_mem->input_mem.pa;
	p_nn_in->src_img.va   		= pdcnn_mem->input_mem.va;
	p_nn_in->src_img.sign 		= MAKEFOURCC('A','B','U','F');
	p_nn_in->src_img.size 		= p_nn_in->loff * p_nn_in->h * 3 / 2;

	return ret;
}


/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/


//static NET_PROC g_net[16] = {0};

static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
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
	//mem_parm->size = size;

	if (fd) {
		fclose(fd);
	};
	printf("model buf size: %d\r\n", size);
	return HD_OK;
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
	if(ret != HD_OK){
		printf("proc net id (%ld) close fail (%ld) !!\r\n", proc_id, ret);
		return ret;
	}
	
	return ret;
}

static HD_RESULT network_get_layer0_info(UINT32 proc_id)
{
	HD_RESULT ret = HD_OK;
	VENDOR_AI_BUF p_inbuf = {0};
	VENDOR_AI_BUF p_outbuf = {0};
		
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_IN(0, 0), &p_inbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 inbuf fail !!\n", proc_id);
		return ret;
	}
	
	// get layer0 in buf
	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(0, 0), &p_outbuf);
	if (HD_OK != ret) {
		printf("proc_id(%u) get layer0 outbuf fail !!\n", proc_id);
		return ret;
	}
	
	printf("dump layer0 info:\n");
	printf("   channel(%lu)\n", p_inbuf.channel);
	printf("   fmt(0x%lx)\n", p_inbuf.fmt);
	printf("   width(%lu)\n", p_outbuf.width);
	printf("   height(%lu)\n", p_outbuf.height);
	printf("   channel(%lu)\n", p_outbuf.channel);
	printf("   batch_num(%lu)\n", p_outbuf.batch_num);
	printf("   fmt(0x%lx)\n", p_outbuf.fmt);
	printf("\n");

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

static HD_RESULT mem_rel(VOID)
{
	HD_RESULT ret = HD_OK;
	/* Release in buffer */
	if (g_mem.va > 0) {
		ret = hd_common_mem_munmap((void *)g_mem.va, g_mem.size);
		if (ret != HD_OK) {
			printf("mem_uninit : hd_common_mem_munmap fail (%ld).\r\n", ret);
			return ret;
		}
	}

	ret = hd_common_mem_release_block(g_blk);
	if (ret != HD_OK) {
		printf("mem_uninit : hd_common_mem_release_block fail (%ld).\r\n", ret);
		return ret;
	}

	hd_common_mem_uninit();
	return HD_OK;
}


HD_RESULT pdcnn_preset(PDCNN_MEM *pdcnn_mem, VENDOR_AIS_FLOW_MEM_PARM *buf, PD_PROPOSAL_PARAM pd_proposal_params)
{
	HD_RESULT ret;

	network_set_opt(pd_proposal_params.run_id);

	ret = get_pd_mem(buf, &(pdcnn_mem->input_mem), PD_MAX_FRAME_WIDTH * PD_MAX_FRAME_HEIGHT * 3 / 2, 32);
	if(ret != HD_OK){
		printf("get input YUV mem fail (%d)!!\r\n", ret);
		return ret;
	}
 
	UINT32 model_size = _getsize_model(model_name[0]);
	if (model_size <= 0) {
		printf("ERR: pdcnn input model1 not exist: %s\r\n", model_name[0]);
		return HD_ERR_NOT_OPEN;
	}
	
	ret = get_pd_mem(buf, &(pdcnn_mem->model_mem), model_size, 32);
	if(ret != HD_OK){
		printf("ERR: pdcnn get model1 mem fail (%d)!!\r\n", ret);
		return ret;
	}

	ret = load_model(model_name[0], pdcnn_mem->model_mem.va);
	if(ret != HD_OK){
		printf("ERR: pdcnn load model1 fail (%ld)!!\r\n", ret);
		return ret;
	}

	pdcnn_get_version();
#if 1
	ret = pdcnn_version_check(&(pdcnn_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: pdcnn version check model1 fail (%d)!!\r\n", ret);
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
	//printf("pdcnn work buf size: %ld\r\n", wbuf.size);
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

	ret = get_pd_mem(buf, &(limit_fdet_mem->input_mem), LIMIT_FDET_WIDTH * LIMIT_FDET_WIDTH * 3 / 2, 32);
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
	
	// open limit_fdet net
	ret = network_open(limit_fdet_params.run_id, &(limit_fdet_mem->model_mem));
	if(ret != HD_OK){
		printf("ERR: limit_fdet net open fail (%ld)!!\r\n", ret);
		return ret;
	}
	//set limit_fdet work buf
	VENDOR_AI_NET_CFG_WORKBUF limit_fdet_wbuf = {0};
	ret = vendor_ai_net_get(limit_fdet_params.run_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &limit_fdet_wbuf);
	if (ret != HD_OK) {
		printf("ERR: limit_fdet get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail  (%ld)!!\r\n", ret);
		return ret;
	}

	ret = get_limit_fdet_mem(buf, &(limit_fdet_mem->io_mem), limit_fdet_wbuf.size, 32);
	if(ret != HD_OK){
		printf("ERR: limit_fdet get io_mem fail (%d)!!\r\n", ret);
		return ret;
	}
	
	ret = vendor_ai_net_start(limit_fdet_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet start fail (%d)!!\r\n", ret);
		return ret;
	}
	
	return HD_OK;
}


VOID assign_ai_buf(PD_THREAD_PARM *parm)
{
	parm->scale_mem.pa = g_mem.pa;
	parm->scale_mem.va = g_mem.va;
	parm->scale_mem.size = PD_SCALE_BUF_SIZE + 32;
	
	parm->pd_mem.pa = parm->scale_mem.pa + parm->scale_mem.size;
	parm->pd_mem.va = parm->scale_mem.va + parm->scale_mem.size;
	parm->pd_mem.size = g_mem.size - parm->scale_mem.size;
}


static VOID *nn_thread_api(VOID *arg)
{
	HD_RESULT ret;
	PD_THREAD_PARM* pd_parm = (PD_THREAD_PARM*)arg;
	VENDOR_AIS_FLOW_MEM_PARM pd_buf = pd_parm->pd_mem;
	VENDOR_AIS_FLOW_MEM_PARM scale_buf = pd_parm->scale_mem;
	HD_GFX_IMG_BUF gfx_img = {0};
	VENDOR_AI_BUF	p_src_img = {0};
	INT32 base_imgsize[2] = {1024, 576};

	CHAR para_file[] = "/mnt/sd/CNNLib/para/pdcnn/para.txt";
	PD_PROPOSAL_PARAM proposal_params;
	FLOAT score_thr = 0.45, nms_thr = 0.2;
	PD_LIMIT_PARAM limit_param = {0};
	proposal_params.score_thres = score_thr;
	proposal_params.nms_thres = nms_thr;
	proposal_params.run_id = 0;
#if LIMIT_FDET_PD
	LIMIT_FDET_PARAM limit_fdet_param = {0};
	LIMIT_FDET_MEM limit_fdet_mem = {0};	
	limit_fdet_param.run_id= 1;
	limit_param.limit_fdet = 1;
#endif

#if PD_MAX_DISTANCE_MODE
	limit_param.max_distance = 1;
	limit_param.sm_thr_num = 2;
#else
	limit_param.max_distance = 0;
	limit_param.sm_thr_num = 6;
#endif	
	
	PDCNN_MEM pdcnn_mem;
	ret = pdcnn_preset(&pdcnn_mem, &pd_buf, proposal_params);
	if(ret != HD_OK){
		printf("ERR: pdcnn_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
	// start network
	ret = vendor_ai_net_start(proposal_params.run_id);
	if (HD_OK != ret) {
		printf("ERR: pdcnn start fail !!\n");
		goto exit_thread;
	}

	//get nn layer 0 info
	network_get_layer0_info(proposal_params.run_id);

	//image parameter
	NET_IN nn_in;
	nn_in.c = 2,
	nn_in.fmt = HD_VIDEO_PXLFMT_YUV420;

	//pdcnn_mem_init(&pdcnn_mem);
	PD_BACKBONE_OUT* backbone_outputs = (PD_BACKBONE_OUT*)pdcnn_mem.backbone_output.va;
	ret = pdcnn_init(&proposal_params, backbone_outputs, &limit_param, para_file);
	if(ret != HD_OK){
		printf("ERR: pdcnn init fail!\r\n");
		goto exit_thread;
	}

#if LIMIT_FDET_PD
	ret = limit_fdet_preset(&limit_fdet_mem, &pd_buf, limit_fdet_param);
	if(ret != HD_OK){
		printf("ERR: limit_fdet_preset fail (%d)!!\r\n", ret);
		goto exit_thread;
	}
#endif

	UINT32 all_time = 0;
	INT32 img_num = 0;
#if SAVE_SCALE
	CHAR BMP_FILE[256];
#endif
	CHAR IMG_PATH[256];
	CHAR SAVE_TXT[256];
	CHAR IMG_LIST[256];
	CHAR list_infor[256];
	CHAR *line_infor;
	BOOL INPUT_STATE = TRUE;

	sprintf(IMG_LIST, "/mnt/sd/jpg/pd_image_list.txt");
	sprintf(IMG_PATH, "/mnt/sd/jpg/PD");

	FILE *fs, *fr;

    sprintf(SAVE_TXT, "/mnt/sd/det_results/pd_test_results.txt");
	
	fr = fopen(IMG_LIST, "r");
	fs = fopen(SAVE_TXT, "w+");

	INT32 len = 0;
	CHAR img_name[256]={0};
	CHAR *token;
	INT32 sl = 0;

	if(NULL == fr)
	{
		printf("Failed to open img_list!\r\n");	
	} 
	while(fgets(list_infor, 256, fr) != NULL){
		len = strlen(list_infor);
		list_infor[len - 1] = '\0';
		sl = 0;
		line_infor = list_infor;

		while((token = strtok(line_infor, " ")) != NULL)
		{
			if(sl > 2){
				break;
			}
			if (sl == 0){
				strcpy(img_name, token);
				sprintf(nn_in.input_filename, "%s/%s", IMG_PATH, token);
				printf("%s ", token);
			}
			if (sl == 1){
				nn_in.w = atoi(token);
				nn_in.loff = ALIGN_CEIL_4(nn_in.w);
				printf("%s ", token);
			}
			if (sl == 2){
				nn_in.h = atoi(token);
				printf("%s\r\n", token);
			}
			line_infor = NULL;
			sl++;
		}

		if ((ret = input_open(&nn_in, &pdcnn_mem)) != HD_OK) {
			printf("ERR: pdcnn input image open fail !!\n");
			goto exit_thread;
		}

		proposal_params.ratiow = (FLOAT)nn_in.w / (FLOAT)base_imgsize[0];
		proposal_params.ratioh = (FLOAT)nn_in.h / (FLOAT)base_imgsize[1];
		
		gfx_img.dim.w = base_imgsize[0];
		gfx_img.dim.h = base_imgsize[1];
		gfx_img.format = nn_in.src_img.fmt;
		gfx_img.lineoffset[0] = ALIGN_CEIL_4(base_imgsize[0]);
		gfx_img.lineoffset[1] = ALIGN_CEIL_4(base_imgsize[1]);
		gfx_img.p_phy_addr[0] = scale_buf.pa;
		gfx_img.p_phy_addr[1] = scale_buf.pa + base_imgsize[0] * base_imgsize[1];

		PD_IRECT roi = {0, 0, nn_in.src_img.width, nn_in.src_img.height};
		ret = pdcnn_crop_img(&gfx_img, &(nn_in.src_img), HD_GFX_SCALE_QUALITY_NULL, &roi);
		if (ret != HD_OK) {
			printf("ai_crop_img fail=%d\n", ret);
		}
		pd_gfx_img_to_vendor(&p_src_img, &gfx_img, (UINT32)scale_buf.va);
#if SAVE_SCALE
		FILE *fb;
		sprintf(BMP_FILE, "/mnt/sd/save_bmp/%s_scale.bin", img_name);
		fb = fopen(BMP_FILE, "wb+");
		fwrite((UINT32 *)scale_buf.va, sizeof(UINT32), (gfx_img.dim.h * gfx_img.dim.w + gfx_img.dim.h * gfx_img.dim.w / 2), fb);
		fclose(fb);
#endif

		PROF_START();

		ret = pdcnn_process(&proposal_params, &pdcnn_mem, &limit_param, backbone_outputs, &p_src_img, (UINT32)PD_MAX_DISTANCE_MODE);

		if(ret != HD_OK){
			printf("pdcnn_process fail!\r\n");
			goto exit_thread;
		}
#if LIMIT_FDET_PD
		limit_fdet_param.ratiow = (FLOAT)p_src_img.width / (FLOAT)PD_YUV_WIDTH;
		limit_fdet_param.ratioh = (FLOAT)p_src_img.height / (FLOAT)PD_YUV_HEIGHT;
		limit_fdet_param.cls_id = 0;
		limit_fdet_param.proc_num = pdcnn_mem.out_num;
		limit_fdet_mem.det_in_mem = pdcnn_mem.out_result;
		limit_fdet_mem.det_out_mem = pdcnn_mem.final_result;
		ret = limit_fdet_process(&limit_fdet_param, &limit_fdet_mem, &p_src_img);
		if(ret != HD_OK){
			printf("ERR: limit_fdet_process fail (%ld)!\r\n", ret);
			goto exit_thread;
		}
		pdcnn_mem.out_num = limit_fdet_param.proc_num;	
#endif
		PROF_END("PDCNN");
		all_time += (tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec); 

		PDCNN_RESULT *final_out_results = (PDCNN_RESULT*)pdcnn_mem.final_result.va;
		for(INT32 num = 0; num < pdcnn_mem.out_num; num++){
			//printf("before: %f %f %f %f %f.\r\n", final_out_results[num].score, final_out_results[num].x1, final_out_results[num].y1, final_out_results[num].x2 - final_out_results[num].x1, final_out_results[num].y2 - final_out_results[num].y1);
			FLOAT xmin = final_out_results[num].x1 * proposal_params.ratiow;
			FLOAT ymin = final_out_results[num].y1 * proposal_params.ratioh;
			FLOAT width = final_out_results[num].x2 * proposal_params.ratiow - xmin;
			FLOAT height = final_out_results[num].y2 * proposal_params.ratioh - ymin;
			FLOAT score = final_out_results[num].score;
			
			printf("obj information is: (socre: %f [%f %f %f %f])\r\n", score, xmin, ymin, width, height);
			fprintf(fs, "%s %f %f %f %f %f\r\n", img_name, score, xmin, ymin, width, height);
		}
		
		img_num++;

	}
	if (INPUT_STATE == TRUE){
    	printf("all test img number is: %d\r\n", img_num);
	}
	fclose(fs);
	fclose(fr);
	//printf("mean_time: %d\r\n", all_time / img_num);
	printf("mean_time: %d\r\n", all_time / img_num);
	
exit_thread:
	ret = vendor_ai_net_stop(proposal_params.run_id);
	if (HD_OK != ret) {
		printf("pdcnn stop fail (%d)!!\n", ret);
	}

	// close network modules
	ret = network_close(proposal_params.run_id);
	if(ret != HD_OK) {
		printf("ERR: pdcnn close fail (%d)!!\n", ret);
	}
#if LIMIT_FDET_PD	
	ret = vendor_ai_net_stop(limit_fdet_param.run_id);
	if (HD_OK != ret) {
		printf("ERR: limit_fdet stop fail (%d)!!\n", ret);
	}
	ret = network_close(limit_fdet_param.run_id);
	if(ret != HD_OK) {
		printf("ERR: limit_fdet close fail (%d)!!\n", ret);
	}
#endif
		
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	HD_RESULT ret;
	pthread_t nn_thread_id;
	PD_THREAD_PARM pd_thread_parm;

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		goto exit;
	}
	// set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	ret = hd_gfx_init();
	if (ret != HD_OK) {
		printf("hd_gfx_init fail\r\n");
		goto exit;
	}

	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_DEFINIED_POOL;
	mem_cfg.pool_info[0].blk_size = PD_MAX_MEM_SIZE;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		goto exit;
	}
	
	ret = mem_get();
	if(ret != HD_OK){
		printf("pdcnn all mem get fail (%d)!!\r\n", ret);
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
	//printf("before NN g_mem: pa=(%#x), va=(%#x), size=(%ld)\r\n", g_mem.pa, g_mem.va, g_mem.size);
	assign_ai_buf(&pd_thread_parm);

	ret = pthread_create(&nn_thread_id, NULL, nn_thread_api, (VOID *)(&pd_thread_parm));
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}
	
	pthread_join(nn_thread_id, NULL);

exit:
	ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		printf("hd_gfx_uninit fail\r\n");
	}

    ret = vendor_ai_uninit();
    if (ret != HD_OK) {
        printf("vendor_ai_uninit fail=%d\n", ret);
    }
	
	ret = mem_rel();
	if(ret != HD_OK){
		printf("release pdcnn buf fail!!\r\n");
	}
			
	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}
	printf("test finish!!!\r\n");
	return ret;
}
