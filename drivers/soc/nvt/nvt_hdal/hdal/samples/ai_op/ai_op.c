/**
	@brief Source file of vendor ai net sample code.

	@file ai_op.c

	@ingroup ai_net_sample

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
#include "hd_debug.h"
#include "vendor_ai.h"


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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_op, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define NET_PATH_ID		UINT32

#define VENDOR_AI_CFG  				0x000f0000  //vendor ai config

#define AI_RGB_BUFSIZE(w, h)		(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(HD_VIDEO_PXLFMT_RGB888_PLANAR) / 8) * (h))

#define NET_VDO_SIZE_W	1920 //max for net
#define NET_VDO_SIZE_H	1080 //max for net

#define SV_LENGTH 		10240
#define SV_FEA_LENGTH 	256

#define SCALE_DIM_W 384
#define SCALE_DIM_H 282

typedef enum _AI_OP {
    AI_OP_FC                             = 0, 
	AI_OP_PREPROC_YUV2RGB                = 1,  
	AI_OP_PREPROC_YUV2RGB_SCALE          = 2,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE  = 3,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_DC     = 4,
	ENUM_DUMMY4WORD(AI_OP)
} AI_OP;
///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
} MEM_PARM;


/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

#if 0
static HD_RESULT mem_get(MEM_PARM *mem_parm, UINT32 size, UINT32 id)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK      blk;

	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL + id, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;

	return HD_OK;
}

static HD_RESULT mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;
	return HD_OK;
}
#endif

static HD_RESULT mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;

	//alloc private pool
	ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID0);
	if (ret!= HD_OK) {
		return ret;
	}

	mem_parm->pa   = pa;
	mem_parm->va   = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk  = (UINT32)-1;

	return HD_OK;
}

static HD_RESULT mem_free(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;
	
	//free private pool
	ret =  hd_common_mem_free(mem_parm->pa, (void *)mem_parm->va);
	if (ret!= HD_OK) {
		return ret;
	}
	
	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;

	return HD_OK;
}

#if 0
static INT32 mem_load(MEM_PARM *mem_parm, const CHAR *filename)
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
	mem_parm->size = size;

	if (fd) {
		fclose(fd);
	}

	return size;
}
#endif

static INT32 mem_save(MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	UINT32 size = 0;

	fd = fopen(filename, "wb");

	if (!fd) {
		printf("ERR: cannot open %s for write!\r\n", filename);
		return -1;
	}

	size = (INT32)fwrite((VOID *)mem_parm->va, 1, mem_parm->size, fd);
	if (size != mem_parm->size) {
		printf("ERR: write %s with size %ld < wanted %ld?\r\n", filename, size, mem_parm->size);
	} else {
		printf("write %s with %ld bytes.\r\n", filename, mem_parm->size);
	}

	if (fd) {
		fclose(fd);
	}

	return size;
}

static VOID mem_fill(MEM_PARM *mem_parm, int mode)
{
    UINT32 i = 0;

	if (mode == 0) {
		// clear
		memset((VOID *)mem_parm->va, 1, mem_parm->size); 
	} else {
	    // struct timeval time_temp;
	    // gettimeofday(&time_temp, NULL);
	    // srand((time_temp.tv_sec - time_temp.tv_sec) * 1000000 + (time_temp.tv_usec - time_temp.tv_usec));
	    // for(i = 0; i < mem_parm->size; i++)
	    // {
	    //     ((UINT8 *)mem_parm->va)[i] = rand() & 0xff;
	    // }
	    for(i = 0; i < mem_parm->size; i++) {
	        ((INT8 *)mem_parm->va)[i] = (i & 0x07);
	    }
	}
}

static INT32 mem_load(MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (!fd) {
		printf("cannot read %s\r\n", filename);
		size = -1;
		goto exit;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
		goto exit;
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
		goto exit;
	}
	mem_parm->size = size;

	// we use cpu to read memory, which needs to deal cache flush.
	if(hd_common_mem_flush_cache((VOID *)mem_parm->va, mem_parm->size) != HD_OK) {
        printf("flush cache failed.\r\n");
    }

exit:
	if (fd) {
		fclose(fd);
	}

	return size;
}

/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/
typedef struct _NET_IN_CONFIG {

	CHAR input_filename[256];
	UINT32 w;
	UINT32 h;
	UINT32 c;
	UINT32 loff;
	UINT32 fmt;
	
} NET_IN_CONFIG;

typedef struct _NET_IN {

	NET_IN_CONFIG in_cfg;
	//MEM_PARM input_mem;
	UINT32 in_id;
	//VENDOR_AI_BUF src_img;
		
} NET_IN;

static NET_IN g_in[16] = {0};

static HD_RESULT input_init(void)
{
	HD_RESULT ret = HD_OK;
	int  i;
	
	for (i = 0; i < 16; i++) {
		NET_IN* p_net = g_in + i;
		p_net->in_id = i;
	}
	return ret;
}

static HD_RESULT input_uninit(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

INT32 input_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	return i;
}

static HD_RESULT input_set_config(NET_PATH_ID net_path, NET_IN_CONFIG* p_in_cfg)
{
	HD_RESULT ret = HD_OK;
	NET_IN* p_in = g_in + net_path;
	UINT32 proc_id = p_in->in_id;
	
	memcpy((void*)&p_in->in_cfg, (void*)p_in_cfg, sizeof(NET_IN_CONFIG));
	printf("proc_id(%u) set in_cfg: file(%s), buf=(%u,%u,%u,%u,%08x)\r\n", 
		proc_id,
		p_in->in_cfg.input_filename,
		p_in->in_cfg.w,
		p_in->in_cfg.h,
		p_in->in_cfg.c,
		p_in->in_cfg.loff,
		p_in->in_cfg.fmt);
	
	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/

static HD_RESULT network_init(void)
{
	HD_RESULT ret = HD_OK;
	
	ret = hd_videoproc_init();
	if (ret != HD_OK) {
		printf("hd_videoproc_init fail=%d\n", ret);
		return ret;
	}
	ret = vendor_ai_init();
	if (ret != HD_OK) {
		printf("vendor_ai_init fail=%d\n", ret);
		return ret;
	}

	return ret;
}

static HD_RESULT network_uninit(void)
{
	HD_RESULT ret = HD_OK;
	
	ret = hd_videoproc_uninit();
	if (ret != HD_OK) {
		printf("hd_videoproc_uninit fail=%d\n", ret);
	}
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}
	
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
typedef struct _OP_PROC {

	UINT32 proc_id;
    int op_opt;
	MEM_PARM input_mem;
	MEM_PARM weight_mem;
	MEM_PARM output_mem;
		
} OP_PROC;

static OP_PROC g_op[16] = {0};

static HD_RESULT op_init(void)
{
	HD_RESULT ret = HD_OK;
	int  i;	
	for (i = 0; i < 16; i++) {
		OP_PROC* p_op = g_op + i;
		p_op->proc_id = i;
	}
	return ret;
}

static HD_RESULT op_uninit(void)
{
	HD_RESULT ret = HD_OK;
	return ret;
}

static HD_RESULT operator_set_config(NET_PATH_ID net_path, int in_op_opt)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + net_path;
    p_op->op_opt = in_op_opt;
	return ret;
}

static HD_RESULT operator_alloc_out_buf(NET_PATH_ID op_path, NET_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	NET_IN* p_in = g_in + in_path;
	UINT32 proc_id = p_op->proc_id;
	
	// alloc result buff
    switch (p_op->op_opt) {
	    case AI_OP_FC: //VENDOR_AI_OP_FC
        {
	        ret = mem_alloc(&p_op->output_mem, "user_out_buf", SV_LENGTH*4);
	        if (ret != HD_OK) {
	       	    printf("proc_id(%u) alloc out_buf fail\r\n", proc_id);
	      	    return HD_ERR_FAIL;
	        }
            else {
                printf("proc_id(%u) alloc out_buf OK, size = %d\r\n", proc_id, SV_LENGTH*4);
            }
    	    mem_fill(&p_op->output_mem, 1); 
	        mem_save(&p_op->output_mem, "./user_out_ori.bin");
    	}
         break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
        {
			if(p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE) {
            	ret = mem_alloc(&p_op->output_mem, "user_out_buf", AI_RGB_BUFSIZE(SCALE_DIM_W, SCALE_DIM_H));
				if (ret != HD_OK) {
	        		printf("proc_id(%u) alloc out_buf fail\r\n", proc_id);
	        		return HD_ERR_FAIL;
	        	}
            	else {
                	printf("proc_id(%u) alloc out_buf OK, size = %d\r\n", proc_id, AI_RGB_BUFSIZE(SCALE_DIM_W, SCALE_DIM_H));
            	}
			}
			else {
				ret = mem_alloc(&p_op->output_mem, "user_out_buf", AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
				if (ret != HD_OK) {
	        		printf("proc_id(%u) alloc out_buf fail\r\n", proc_id);
	        		return HD_ERR_FAIL;
	        	}
				else {
                	printf("proc_id(%u) alloc out_buf OK, size = %d\r\n", proc_id, AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
            	}
			}
        }
        break;
        default:
        break;
    }
	return ret;
}

static HD_RESULT operator_free_out_buf(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	
	// free result buff
	mem_free(&p_op->output_mem);
	
	return ret;
}

static HD_RESULT operator_open(NET_PATH_ID op_path, NET_PATH_ID in_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;
	NET_IN* p_in = g_in + in_path;
	UINT32 proc_id = p_op->proc_id;

	// alloc buffer
	switch (p_op->op_opt) {
		case AI_OP_FC: 
        {
	        ret = mem_alloc(&p_op->input_mem, "user_in_buf", SV_FEA_LENGTH);
	        if (ret != HD_OK) {
	        	printf("proc_id(%u) alloc in_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }
	        ret = mem_alloc(&p_op->weight_mem, "user_weight_buf", SV_LENGTH*SV_FEA_LENGTH);
	        if (ret != HD_OK) {
	        	printf("proc_id(%u) alloc weight_buf fail\r\n", proc_id);
	        	return HD_ERR_FAIL;
	        }

	        // fill buffer
	        mem_fill(&p_op->input_mem, 1); 
	        mem_fill(&p_op->weight_mem, 1); 
	        // save buffer
	        mem_save(&p_op->input_mem, "./user_in.bin"); 
	        mem_save(&p_op->weight_mem, "./user_weight.bin");
		}
        break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
        {
			if(p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE)
            	ret = mem_alloc(&p_op->input_mem, "user_in_buf", 2*AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
			else
				ret = mem_alloc(&p_op->input_mem, "user_in_buf", AI_RGB_BUFSIZE(p_in->in_cfg.w, p_in->in_cfg.h));
			if (ret != HD_OK) {
	    	    printf("proc_id(%lu) alloc in_buf fail\r\n", proc_id);
	    	    return HD_ERR_FAIL;
	        }
            
            INT32 file_len;
            file_len = mem_load(&p_op->input_mem, p_in->in_cfg.input_filename);
	        if (file_len < 0) {
		        printf("load buf(%s) fail\r\n", p_in->in_cfg.input_filename);
		        return HD_ERR_NG;
	        }
        	printf("load buf(%s) ok, size = %d\r\n", p_in->in_cfg.input_filename, file_len);
        }
        break;
        default:
        {
            printf("Unknown op_opt");
	        return HD_ERR_LIMIT;
        }
        break;
	}

	// open
	ret = vendor_ai_op_open(op_path);
	return ret;
}

static HD_RESULT operator_close(NET_PATH_ID op_path)
{
	HD_RESULT ret = HD_OK;
	OP_PROC* p_op = g_op + op_path;

	// close
	ret = vendor_ai_op_close(op_path);
    // free buffer
	switch (p_op->op_opt) {
		case AI_OP_FC: //VENDOR_AI_OP_FC
        {
			mem_free(&p_op->input_mem);
			mem_free(&p_op->weight_mem);
		}
        break;
		case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC:
        {
            mem_free(&p_op->input_mem);
        }
        break;
        default:
        break;
	}
	return ret;
}


///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

    // (1) input 
	NET_IN_CONFIG net_in_cfg;
	NET_PATH_ID in_path;
    

	// (2) operator	
	NET_PATH_ID op_path;
    int net_op_opt;
	pthread_t  op_thread_id;
	UINT32 op_start;
	UINT32 op_exit;
	UINT32 op_oneshot;

} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
    if ((ret = op_init()) != HD_OK)
		return ret;
	if ((ret = input_init()) != HD_OK)
		return ret;
	if ((ret = network_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = operator_open(p_stream->op_path, p_stream->in_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = operator_close(p_stream->op_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = op_uninit()) != HD_OK)
		return ret;
	if ((ret = input_uninit()) != HD_OK)
		return ret;
    if ((ret = network_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}
///////////////////////////////////////////////////////////////////////////////
static VOID *operator_user_thread(VOID *arg);

static HD_RESULT operator_user_start(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;

	p_stream->op_start = 0;
	p_stream->op_exit = 0;
	p_stream->op_oneshot = 0;
	
	ret = pthread_create(&p_stream->op_thread_id, NULL, operator_user_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}

	p_stream->op_start = 1;
	p_stream->op_exit = 0;
	p_stream->op_oneshot = 0;
	
	return ret;
}

static HD_RESULT operator_user_oneshot(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->op_oneshot = 1;
	return ret;
}

static HD_RESULT operator_user_stop(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->op_exit = 1;
	
	pthread_join(p_stream->op_thread_id, NULL);

	return ret;
}

static VOID *operator_user_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	
	VIDEO_LIVEVIEW *p_stream = (VIDEO_LIVEVIEW*)arg;
	OP_PROC* p_op = g_op + p_stream->op_path;
	NET_IN* p_in = g_in + p_stream->in_path;

	printf("\r\n");
	while (p_stream->op_start == 0) sleep(1);

	printf("\r\n");
	ret = operator_alloc_out_buf(p_stream->op_path, p_stream->in_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) alloc output fail !!\n", p_stream->op_path);
		goto skip;
	}
	
	printf("\r\n");
    
    switch (p_op->op_opt) {
		case AI_OP_FC: 
        {
	        while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
            

	                // 2. flush input
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->weight_mem)->va, (&p_op->weight_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
	        		ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
		        	// 3. run OP
		        	{
		        		/*
		        		The code below is the flow of using FC 
		        		suppose the input feature size is 256 bytes (defined as SV_FEA_LENGTH)
		        		and the desired output length is 10240 (defined as SV_LENGTH)
		        		the following sample will transpose the input 256 bytes feature (1 byte per element) into 10240*4 bytes feature (4 bytes per element)
		        		
		        		fc_init_param is for setting parameter of FC
			        	user should set input/output/weight address
			        	*/
			        	VENDOR_AI_BUF src[2] = {0};
			        	VENDOR_AI_BUF dest[1] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
			        	MEM_PARM* weight_buf = &p_op->weight_mem;
		        		//pprintf("input addr pa = 0x%08X\n", (unsigned int)(in_buf->pa));
			        	//pprintf("output addr pa = 0x%08X\n", (unsigned int)(out_buf->pa));
			        	//pprintf("weight addr pa = 0x%08X\n", (unsigned int)(weight_buf->pa));
				        
		          		//set src1 as 1d tensor
			        	src[0].sign = MAKEFOURCC('A','B','U','F');
			        	src[0].ddr_id = 0;
			        	src[0].va = in_buf->va; //< input address	 (size = SV_FEA_LENGTH bytes)
			        	src[0].pa = in_buf->pa;
			        	src[0].size = SV_FEA_LENGTH;
			        	src[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			        	src[0].width = SV_FEA_LENGTH;
			        	src[0].height = 1;
			        	src[0].channel = 1;
			        	src[0].batch_num = 1;
			        	
		        		//set src2 as 2d tensor
		        		src[1].sign = MAKEFOURCC('A','B','U','F');
		        		src[1].ddr_id = 0;
		        		src[1].va = weight_buf->va; //< sv weight address (size = SV_LENGTH*SV_FEA_LENGTH bytes)
		        		src[1].pa = weight_buf->pa;
		        		src[1].size = SV_FEA_LENGTH*SV_LENGTH;
		        		src[1].fmt = HD_VIDEO_PXLFMT_AI_SFIXED8(0); //fixpoint s7.0
			        	src[1].width = SV_FEA_LENGTH;
			        	src[1].height = SV_LENGTH;
			        	src[1].channel = 1;
			        	src[1].batch_num = 1;
		        		
		        		//set dest as 1d tensor
			        	dest[0].sign = MAKEFOURCC('A','B','U','F');
		        		dest[0].ddr_id = 0;
			        	dest[0].va = out_buf->va; //< output address	 (size = SV_LENGTH*4 bytes)
			        	dest[0].pa = out_buf->pa;
			        	dest[0].size = SV_LENGTH*4;
			        	dest[0].fmt = HD_VIDEO_PXLFMT_AI_SFIXED32(0); //fixpoint s31.0
			        	dest[0].width = SV_LENGTH;
			        	dest[0].height = 1;
			        	dest[0].channel = 1;
			        	dest[0].batch_num = 1;

			        	ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_FC, NULL, 2, src, 1, dest);
		        	}
		        	if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                  if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
        
		        	mem_save(&p_op->output_mem, "./op_user_out.bin"); 
        
		        }
		        usleep(100);
	        }
		}
        break;
        case AI_OP_PREPROC_YUV2RGB:
		case AI_OP_PREPROC_YUV2RGB_SCALE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE:
		case AI_OP_PREPROC_YUV2RGB_MEANSUB_DC: 
        {
            while (p_stream->op_exit == 0) {

	        	if (p_stream->op_oneshot) {
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->input_mem)->va, (&p_op->input_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }
                    // 3. run OP
		        	{
                        VENDOR_AI_BUF src[2] = {0};
			        	VENDOR_AI_BUF dest[3] = {0};
			        	MEM_PARM* in_buf = &p_op->input_mem;
			        	MEM_PARM* out_buf = &p_op->output_mem;
                        NET_IN_CONFIG in_cfg = p_in->in_cfg;
                        VENDOR_AI_OP_PREPROC_PARAM p_parm = {0};

                        //set src1 as 1d tensor
						src[0].sign = MAKEFOURCC('A','B','U','F');
						src[0].ddr_id = 0;
						src[0].va = in_buf->va; //< input address	 
						src[0].pa = in_buf->pa;
						src[0].size = in_cfg.loff * in_cfg.h;
						src[0].fmt = HD_VIDEO_PXLFMT_Y8;
						src[0].width = in_cfg.w;
						src[0].height = in_cfg.h;
						src[0].line_ofs = in_cfg.loff;
						src[0].channel = 1;
						src[0].batch_num = 1;

                        //set src2 as 1d tensor
						src[1].sign = MAKEFOURCC('A','B','U','F');
						src[1].ddr_id = 0;
						src[1].va = in_buf->va + src[0].size; //< input address	 
						src[1].pa = in_buf->pa + src[0].size;
						src[1].size = in_cfg.loff * in_cfg.h;
						src[1].fmt = HD_VIDEO_PXLFMT_UV;
						src[1].width = in_cfg.w;
						src[1].height = in_cfg.h;
						src[1].line_ofs = in_cfg.loff;
						src[1].channel = 1;
						src[1].batch_num = 1;

						if(p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE) {
							//set dest1 as 1d tensor
							dest[0].sign = MAKEFOURCC('A','B','U','F');
							dest[0].ddr_id = 0;
							dest[0].va = out_buf->va; //< output address	 
							dest[0].pa = out_buf->pa;
							dest[0].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[0].fmt = HD_VIDEO_PXLFMT_R8;
							dest[0].width = SCALE_DIM_W;
							dest[0].height = SCALE_DIM_H;
							dest[0].line_ofs = SCALE_DIM_W;
							dest[0].channel = 1;
							dest[0].batch_num = 1;

							//set dest2 as 1d tensor
							dest[1].sign = MAKEFOURCC('A','B','U','F');
							dest[1].ddr_id = 0;
							dest[1].va = out_buf->va + dest[0].size; //< output address	 
							dest[1].pa = out_buf->pa + dest[0].size;
							dest[1].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[1].fmt = HD_VIDEO_PXLFMT_G8;
							dest[1].width = SCALE_DIM_W;
							dest[1].height = SCALE_DIM_H;
							dest[1].line_ofs = SCALE_DIM_W;
							dest[1].channel = 1;
							dest[1].batch_num = 1;

							//set dest3 as 1d tensor
							dest[2].sign = MAKEFOURCC('A','B','U','F');
							dest[2].ddr_id = 0;
							dest[2].va = out_buf->va + 2*dest[0].size; //< output address		 
							dest[2].pa = out_buf->pa + 2*dest[0].size;
							dest[2].size = SCALE_DIM_W * SCALE_DIM_H;
							dest[2].fmt = HD_VIDEO_PXLFMT_B8;
							dest[2].width = SCALE_DIM_W;
							dest[2].height = SCALE_DIM_H;
							dest[2].line_ofs = SCALE_DIM_W;
							dest[2].channel = 1;
							dest[2].batch_num = 1;
						}
						else {
							//set dest1 as 1d tensor
							dest[0].sign = MAKEFOURCC('A','B','U','F');
							dest[0].ddr_id = 0;
							dest[0].va = out_buf->va; //< output address	 
							dest[0].pa = out_buf->pa;
							dest[0].size = in_cfg.loff * in_cfg.h;
							dest[0].fmt = HD_VIDEO_PXLFMT_R8;
							dest[0].width = in_cfg.w;
							dest[0].height = in_cfg.h;
							dest[0].line_ofs = in_cfg.w;
							dest[0].channel = 1;
							dest[0].batch_num = 1;

							//set dest2 as 1d tensor
							dest[1].sign = MAKEFOURCC('A','B','U','F');
							dest[1].ddr_id = 0;
							dest[1].va = out_buf->va + dest[0].size; //< output address	 
							dest[1].pa = out_buf->pa + dest[0].size;
							dest[1].size = in_cfg.loff * in_cfg.h;
							dest[1].fmt = HD_VIDEO_PXLFMT_G8;
							dest[1].width = in_cfg.w;
							dest[1].height = in_cfg.h;
							dest[1].line_ofs = in_cfg.w;
							dest[1].channel = 1;
							dest[1].batch_num = 1;

							//set dest3 as 1d tensor
							dest[2].sign = MAKEFOURCC('A','B','U','F');
							dest[2].ddr_id = 0;
							dest[2].va = out_buf->va + 2*dest[0].size; //< output address		 
							dest[2].pa = out_buf->pa + 2*dest[0].size;
							dest[2].size = in_cfg.loff * in_cfg.h;
							dest[2].fmt = HD_VIDEO_PXLFMT_B8;
							dest[2].width = in_cfg.w;
							dest[2].height = in_cfg.h;
							dest[2].line_ofs = in_cfg.w;
							dest[2].channel = 1;
							dest[2].batch_num = 1;
						}

                        // set func parameter

						//scale
						if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_SCALE) {
							p_parm.scale_dim.w = SCALE_DIM_W;
							p_parm.scale_dim.h = SCALE_DIM_H;
						}				
						
                        // plane mode
                        if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE) {
							memset((VOID *)(in_buf->va + src[0].size*3), 0x80808080, src[0].size*3);   //clear buffer for sub
							ret = hd_common_mem_flush_cache((VOID *)(in_buf->va + src[0].size*3), src[0].size*3);
							if(HD_OK != ret) {
								printf("flush cache failed.\n");
							}
							p_parm.p_out_sub.pa = in_buf->pa + 3*src[0].size;
							p_parm.p_out_sub.va = in_buf->va + 3*src[0].size;                       
							p_parm.p_out_sub.width = in_cfg.w;
							p_parm.p_out_sub.height = in_cfg.h;
							p_parm.p_out_sub.line_ofs = in_cfg.w*3;
                        }

						// dc mode
						if (p_op->op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_DC) {                 
							p_parm.out_sub_color[0] = 128;
							p_parm.out_sub_color[1] = 127;
							p_parm.out_sub_color[2] = 126;
						}
         
                        ret = vendor_ai_op_proc(p_stream->op_path, VENDOR_AI_OP_PREPROC, &p_parm, 2, src, 3, dest);

                    }
                    if (ret != 0) {
		        		printf("op inference fail\n");
		        		return 0;
		        	}
        
		        	p_stream->op_oneshot = FALSE;
		                	
		        	printf("inference done!\n");
		        	ret = hd_common_mem_flush_cache((VOID *)(&p_op->output_mem)->va, (&p_op->output_mem)->size);
                    if(HD_OK != ret) {
                        printf("flush cache failed.\n");
                    }

                    mem_save(&p_op->output_mem, "./op_user_out.bin");
					

	        	}
                usleep(100);
            }
        }
        break;
        default:
        break;
    }
	
skip:
	ret = operator_free_out_buf(p_stream->op_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) free output fail !!\n", p_stream->op_path);
		goto skip;
	}
	
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	VIDEO_LIVEVIEW stream[2] = {0}; //0: net proc, 1: op path
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
    INT32 idx;
	HD_RESULT ret;
	INT key;

	NET_IN_CONFIG in_cfg = {
		.input_filename = "jpg/YUV420_SP_W512H376.bin",
		.w = 512,
		.h = 376,
		.c = 2,
		.loff = 512,
		.fmt = HD_VIDEO_PXLFMT_YUV420
	};

    if(argc < 2){
		printf("usage : ai_op (op_opt)\n"
			   "op-opt:\n"
			   "0 FC\n"
			   "1 PREPROC (YUV2RGB)\n"
			   "2 PREPROC (YUV2RGB & scale)\n"
			   "3 PREPROC (YUV2RGB & meansub_plane)\n"
			   "4 PREPROC (YUV2RGB & meansub_dc)\n");
		return -1;
	}

	idx = 1;

	// parse operator config
	if (argc > idx) {
		sscanf(argv[idx++], "%d", &stream[0].net_op_opt);
	}		
	
	printf("\r\n\r\n");

	stream[0].op_path = 1;
    stream[0].in_path = 1;

	if (stream[0].net_op_opt == AI_OP_FC)
		printf("Run FC!\r\n");
	else if (stream[0].net_op_opt == AI_OP_PREPROC_YUV2RGB)
		printf("Run PREPROC (YUV2RGB)!\r\n");
	else if (stream[0].net_op_opt == AI_OP_PREPROC_YUV2RGB_SCALE)
		printf("Run PREPROC (YUV2RGB & scale)!\r\n");
	else if (stream[0].net_op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE)
		printf("Run PREPROC (YUV2RGB & meansub_plane)!\r\n");
	else if (stream[0].net_op_opt == AI_OP_PREPROC_YUV2RGB_MEANSUB_DC)
		printf("Run PREPROC (YUV2RGB & meansub_dc)!\r\n");
	else {
		printf("Unknown op-opt = %d",stream[0].net_op_opt);
		return -1;
	}
	
	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		goto exit;
	}
    
	// set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init mem
	if(stream[0].net_op_opt != AI_OP_FC) { //for preproc
		INT32 idx = 0; // mempool index
		input_mem_config(stream[0].in_path, &mem_cfg, 0, idx);
	}
    
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		goto exit;
	}
	
	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

    if(stream[0].net_op_opt != AI_OP_FC) { //for preproc
        // set open config
	    ret = input_set_config(stream[0].in_path, &in_cfg);
	    if (HD_OK != ret) {
	    	printf("proc_id(%u) input_set_config fail=%d\n", stream[0].in_path, ret);
		    goto exit;
	    }
	}

    // set operator config
    ret = operator_set_config(stream[0].op_path, stream[0].net_op_opt);
    if (HD_OK != ret) {
	    printf("proc_id(%u) operator_set_config fail=%d\n", stream[0].in_path, ret);
		goto exit;
	}

	// open video_liveview modules
	ret = open_module(&stream[0]);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

// start
	operator_user_start(&stream[0]);

	printf("Enter q to quit\n");
	printf("Enter r to run once\n");
	do {
		key = getchar();
		if (key == 'r') {
		
			operator_user_oneshot(&stream[0]);
			continue;
		}
		if (key == 'q' || key == 0x3) {

			break;
		}
	} while(1);

	// stop
	operator_user_stop(&stream[0]);

exit:
	// close video_liveview modules
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
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return ret;
}
