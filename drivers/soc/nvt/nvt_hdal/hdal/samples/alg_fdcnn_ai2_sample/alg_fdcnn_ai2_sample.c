/**
	@brief Source file of vendor net application sample using user-space net flow.

	@file alg_fdcnn_sample.c

	@ingroup alg_fdcnn_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "hdal.h"
#include "hd_type.h"
#include "hd_common.h"

#include "hd_debug.h"
#include "hd_common.h"
#include "vendor_ai.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"

#include "fdcnn_lib.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define DEFAULT_DEVICE          "/dev/" VENDOR_AIS_FLOW_DEV_NAME

#define MAX_INPUT_WIDTH         1920
#define MAX_INPUT_HEIGHT        1080
#define MAX_INPUT_IMAGE_SIZE    (1920*1080)

#define NN_FDCNN_FD_MODE        ENABLE
#define NN_FDCNN_FD_PROF        ENABLE
#define NN_FDCNN_FD_DUMP        ENABLE
#define NN_FDCNN_FD_SAVE        ENABLE
#define NN_FDCNN_FD_SAVE_LD     ENABLE
#define FDCNN_MAX_OUTNUM        (10)

#define NN_FDCNN_FD_TYPE        FDCNN_NETWORK_V21
#define NN_FDCNN_FD_FIX_FRM     DISABLE

#define NN_USE_DRAM2            ENABLE
#define VENDOR_AI_CFG  0x000f0000  //ai project config

typedef struct _FDCNN_THREAD_PARM {
    VENDOR_AIS_FLOW_MEM_PARM mem;
    CHAR   *dir;
    UINT32  start;
    UINT32  end;
} FDCNN_THREAD_PARM;


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

static VENDOR_AIS_FLOW_MEM_PARM g_mem       = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1]   = {0};


/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
    UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE) + MAX_INPUT_IMAGE_SIZE*2;
#endif

    mem_cfg.pool_info[0].type = HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[0].blk_size = mem_size; // fd buff
	mem_cfg.pool_info[0].blk_cnt = 1;

#if NN_USE_DRAM2
    mem_cfg.pool_info[0].ddr_id = DDR_ID1;
#else
    mem_cfg.pool_info[0].ddr_id = DDR_ID0;
#endif

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
	}
	return ret;
}

static INT32 get_mem_block(VOID)
{
	HD_RESULT                 ret = HD_OK;
	UINT32                    pa, va;
	HD_COMMON_MEM_VB_BLK      blk;
    UINT32 mem_size = 0;

#if NN_USE_DRAM2
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID1;
#else
    HD_COMMON_MEM_DDR_ID      ddr_id = DDR_ID0;
#endif


#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE) + MAX_INPUT_IMAGE_SIZE*2;
#endif

    /* Allocate parameter buffer */
	if (g_mem.va != 0) {
		DBG_DUMP("err: mem has already been inited\r\n");
		return -1;
	}
    blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		DBG_DUMP("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_DUMP("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, mem_size);
	g_blk_info[0] = blk;

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}
	g_mem.pa = pa;
	g_mem.va = va;
	g_mem.size = mem_size;

exit:
	return ret;
}

static HD_RESULT release_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;
    UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE) + MAX_INPUT_IMAGE_SIZE*2;
#endif

	/* Release in buffer */
	if (g_mem.va) {
		ret = hd_common_mem_munmap((void *)g_mem.va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk_info[0]);
	if (ret != HD_OK) {
		DBG_DUMP("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	return ret;
}
static HD_RESULT mem_uninit(void)
{
	return hd_common_mem_uninit();
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

static UINT32 fdcnn_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		DBG_ERR("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		DBG_ERR("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

static VENDOR_AIS_FLOW_MEM_PARM fdcnn_getmem(VENDOR_AIS_FLOW_MEM_PARM *valid_mem, UINT32 required_size)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.va = valid_mem->va;
        mem.pa = valid_mem->pa;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		DBG_ERR("required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}

VOID *fdcnn_fd_thread(VOID *arg)
{
    HD_RESULT ret;
	FDCNN_THREAD_PARM *p_fd_parm = (FDCNN_THREAD_PARM*)arg;
	VENDOR_AIS_FLOW_MEM_PARM fd_mem = p_fd_parm->mem;
    CHAR   *dir   = p_fd_parm->dir;
    UINT32  start = p_fd_parm->start;
    UINT32  end   = p_fd_parm->end;

    UINT32 height, width, bitcount;
    UINT32 idx;
    CHAR filename[256];

#if (NN_FDCNN_FD_SAVE || NN_FDCNN_FD_DUMP)
    static FDCNN_RESULT fdcnn_info[256] = {0};
    UINT32 fdcnn_num = 0;
    static HD_URECT fdcnn_size = {0, 0, MAX_INPUT_WIDTH, MAX_INPUT_HEIGHT};
    UINT32 i;
#if NN_FDCNN_FD_SAVE_LD
    UINT32 j;
#endif
#endif

#if NN_FDCNN_FD_SAVE
    FILE *fout;
    UINT32 out_size = 0;
    CHAR out_file[256] = {0};
#endif

#if NN_FDCNN_FD_PROF
    static struct timeval tstart, tend;
    static UINT64 cur_time = 0, mean_time = 0, sum_time = 0;
    static UINT32 icount = 0;
#endif

    VENDOR_AIS_FLOW_MEM_PARM input_y  = fdcnn_getmem(&fd_mem, MAX_INPUT_IMAGE_SIZE);
    VENDOR_AIS_FLOW_MEM_PARM input_uv = fdcnn_getmem(&fd_mem, MAX_INPUT_IMAGE_SIZE);

    UINT32 fdcnn_buf_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);

    VENDOR_AIS_FLOW_MEM_PARM fdcnn_buf  = fdcnn_getmem(&fd_mem, fdcnn_buf_size);


    if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V10) // FDCNN_NETWORK_V10 need 4 file
    {
        CHAR file_path[4][256] =  {      "/mnt/sd/CNNLib/para/fdcnn_method1/file1.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file2.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file3.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file4.bin"  };

        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_2 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_2, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_3 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_3, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_4 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_4, NN_FDCNN_FD_TYPE);

        fdcnn_load_file(file_path[0], model_addr_1);
        fdcnn_load_file(file_path[1], model_addr_2);
        fdcnn_load_file(file_path[2], model_addr_3);
        fdcnn_load_file(file_path[3], model_addr_4);
    }
    else if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V20) // FDCNN_NETWORK_V20 need 1 file
    {
        CHAR file_path[256] =  "/mnt/sd/CNNLib/para/fdcnn_method2/file1.bin";
        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        fdcnn_load_file(file_path, model_addr_1);
    }
    else if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V21) // FDCNN_LIGHT
    {
        CHAR file_path[256] =  "/mnt/sd/CNNLib/para/fdcnn_method2_light/file1.bin";
        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        fdcnn_load_file(file_path, model_addr_1);
    }
    else
    {
        DBG_ERR("Not support net type %d !\r\n", NN_FDCNN_FD_TYPE);
        return 0;
    }

    FDCNN_INIT_PARM init_parm = {0};
    init_parm.net_type = NN_FDCNN_FD_TYPE;
    init_parm.net_id = 2;

    ret = fdcnn_init(fdcnn_buf, init_parm);
    if (ret != HD_OK)
    {
        DBG_ERR("fdcnn_init fail=%d\n", ret);
        return 0;
    }

	for(idx = start; idx <= end; idx++)
	{
		// input image ratio must be 16:9
		sprintf(filename, "/mnt/sd/FDCNN/%s_uv/%06ld.bmp", dir, idx);
		fdcnn_readbmpheader(&height, &width, &bitcount, filename);
        if (width > MAX_INPUT_WIDTH || height > MAX_INPUT_HEIGHT)
        {
            DBG_ERR("input image width %ld > MAX_INPUT_WIDTH or input image height %ld > MAX_INPUT_HEIGHT\n", width, height);
            goto exit;
        }
        if (bitcount != 8)
        {
            DBG_ERR("input image bitcount %ld must equal 8\n", bitcount);
            goto exit;
        }
		printf("read uv image : %s %ldx%ld  bit %ld\r\n", filename, width, height, bitcount);
		fdcnn_readbmpbody((UINT8 *)input_uv.va, height, width, bitcount, NULL, 1, filename);

		sprintf(filename, "/mnt/sd/FDCNN/%s_y/%06ld.bmp", dir, idx);
		fdcnn_readbmpheader(&height, &width, &bitcount, filename);
        if (width > MAX_INPUT_WIDTH || height > MAX_INPUT_HEIGHT)
        {
            DBG_ERR("input image width %ld > MAX_INPUT_WIDTH or input image height %ld > MAX_INPUT_HEIGHT\n", width, height);
            goto exit;
        }
        if (bitcount != 8)
        {
            DBG_ERR("input image bitcount %ld must equal 8\n", bitcount);
            goto exit;
        }
		printf("read Y image : %s %ldx%ld  bit %ld\r\n", filename, width, height, bitcount);
		fdcnn_readbmpbody((UINT8 *)input_y.va, height, width, bitcount, NULL, 1, filename);

		// init image
		HD_GFX_IMG_BUF input_image;
		input_image.dim.w = width;
		input_image.dim.h = height;
		input_image.format = HD_VIDEO_PXLFMT_YUV420;
		input_image.p_phy_addr[0] = input_y.pa;
		input_image.p_phy_addr[1] = input_uv.pa;
		input_image.p_phy_addr[2] = input_uv.pa;
		input_image.lineoffset[0] = ALIGN_CEIL_4(width);
		input_image.lineoffset[1] = ALIGN_CEIL_4(width);
		input_image.lineoffset[2] = ALIGN_CEIL_4(width);
#if NN_FDCNN_FD_FIX_FRM
		while (1)
        {
#endif
#if NN_FDCNN_FD_PROF
    		gettimeofday(&tstart, NULL);
#endif
            ret = fdcnn_set_image(fdcnn_buf, &input_image);
            if (ret != HD_OK)
            {
                DBG_ERR("fdcnn_set_image fail=%d\n", ret);
                goto exit;
            }

            ret = fdcnn_process(fdcnn_buf);
            if (ret != HD_OK)
            {
                DBG_ERR("fdcnn_process fail=%d\n", ret);
                goto exit;
            }

#if NN_FDCNN_FD_PROF
    		gettimeofday(&tend, NULL);
    		cur_time = (UINT64)(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec);
    		sum_time += cur_time;
    		mean_time = sum_time/(++icount);
			#if (!NN_FDCNN_FD_FIX_FRM)
    		printf("[FD] cur time(us): %lld, mean time(us): %lld\r\n", cur_time, mean_time);
            #endif
#endif

#if (NN_FDCNN_FD_SAVE || NN_FDCNN_FD_DUMP)
    		fdcnn_size.w = width;
    		fdcnn_size.h = height;

    		fdcnn_num = fdcnn_getresults(fdcnn_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
#endif

#if NN_FDCNN_FD_DUMP
            printf("[FD] ----------- num : %ld ----------- \n", fdcnn_num);
            for(i = 0; i < fdcnn_num; i++ )
            {
                //printf("[FD] %ld\t%ld\t%ld\t%ld\t%ld\r\n", fdcnn_info[i].x, fdcnn_info[i].y, fdcnn_info[i].w, fdcnn_info[i].h, fdcnn_info[i].score);
            }
#endif

#if NN_FDCNN_FD_FIX_FRM
    		if (cur_time < 100000)
    			usleep(100000 - cur_time + (mean_time*0));
    	}
#endif

#if NN_FDCNN_FD_SAVE
		// save result (must creat dir in SD card before)
		sprintf(filename, "/mnt/sd/FDCNN/result/fd/%06ld.txt", idx);
		fout = fopen(filename, "w");
		if (NULL == fout){
			DBG_ERR("Open file %s fail error!\r\n", filename);
            goto exit;
		}
		out_size = sprintf(out_file, "%ld\r\n", fdcnn_num);
		fwrite((UINT8 *)(out_file), out_size, 1, fout);

		for(i = 0; i < fdcnn_num; i++ )
		{
			out_size = sprintf(out_file, "%ld\t%ld\t%ld\t%ld\t%ld", fdcnn_info[i].x, fdcnn_info[i].y, fdcnn_info[i].w, fdcnn_info[i].h, fdcnn_info[i].score);
			fwrite((UINT8 *)(out_file), out_size, 1, fout);
#if NN_FDCNN_FD_SAVE_LD
            for(j = 0; j < 5; j++)
            {
                out_size = sprintf(out_file, "\t%ld\t%ld", fdcnn_info[i].landmark_points[j].x, fdcnn_info[i].landmark_points[j].y);
                fwrite((UINT8 *)(out_file), out_size, 1, fout);
            }
#endif
            out_size = sprintf(out_file, "\r\n");
            fwrite((UINT8 *)(out_file), out_size, 1, fout);
		}
		fclose(fout);
#endif

	}

exit:
    ret = fdcnn_uninit(fdcnn_buf);
    if (ret != HD_OK)
    {
		DBG_ERR("fdcnn_uninit fail=%d\n", ret);
    }
    return 0;
}

int main(int argc, char *argv[])
{
	HD_RESULT           ret;

    if(argc != 4)
    {
        printf("ERR cmd : alg_fdcnn_sample [dir name] [start idx] [end idx]\n");
        return HD_ERR_NOT_SUPPORT;
    }
    UINT32 start_idx = atoi(argv[2]);
    UINT32 end_idx = atoi(argv[3]);

	ret = hd_common_init(0);
	if (ret != HD_OK) {
		DBG_ERR("hd_common_init fail=%d\n", ret);
		goto exit;
	}

    //set project config for AI
    hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	ret = get_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	ret = hd_videoproc_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_init fail=%d\n", ret);
		goto exit;
	}

    ret = hd_gfx_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_init fail=%d\n", ret);
		goto exit;
	}

    {
        UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
        vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
        vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
    }

    ret = vendor_ai_init();
    if (ret != HD_OK) {
        printf("vendor_ai_init fail=%d\n", ret);
        return ret;
    }

    VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;

#if NN_FDCNN_FD_MODE
    FDCNN_THREAD_PARM fd_thread_parm;
    pthread_t fd_thread_id;
    UINT32 fd_mem_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE) + MAX_INPUT_IMAGE_SIZE*2;

    fd_thread_parm.mem   = fdcnn_getmem(&local_mem, fd_mem_size);
    fd_thread_parm.dir   = argv[1];
    fd_thread_parm.start = start_idx;
    fd_thread_parm.end   = end_idx;

	ret = pthread_create(&fd_thread_id, NULL, fdcnn_fd_thread, (VOID*)(&fd_thread_parm));
    if (ret < 0) {
        DBG_ERR("create fdcnn fd thread failed");
        goto exit;
    }
#endif

#if NN_FDCNN_FD_MODE
    pthread_join(fd_thread_id, NULL);
#endif

exit:
    ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_uninit fail=%d\n", ret);
	}

	ret = hd_videoproc_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_videoproc_uninit fail=%d\n", ret);
	}

    ret = vendor_ai_uninit();
    if (ret != HD_OK) {
        printf("vendor_ai_uninit fail=%d\n", ret);
    }

	ret = release_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
	}

	ret = mem_uninit();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
	}

	ret = hd_common_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_common_uninit fail=%d\n", ret);
	}

	DBG_DUMP("network test finish...\r\n");

	return ret;
}


