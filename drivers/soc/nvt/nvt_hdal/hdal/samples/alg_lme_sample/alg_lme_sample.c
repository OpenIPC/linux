/**
 * @file vendor_dis_sample.c
 * @brief start dis sample.
 * @author ALG1-CV
 * @date in the year 2018
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_lme_sample, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif


typedef struct _MEM_RANGE {
	UINT32               va;        ///< Memory buffer starting address
	UINT32               addr;      ///< Memory buffer starting address
	UINT32               size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

static INT32			  g_mem_init = 0;
static INT32			  g_mem_blk = -1;
static MEM_RANGE          g_mem = {0};

static DIS_IPC_INIT       g_cur_egmap_addr, g_ref_egmap_addr;
static DIS_IPC_INIT       g_dis_outbuf_addr;
static DIS_IPC_INIT       g_ix_addr, g_iy_addr, g_valid_addr, g_availbuf_addr;
static UINT32             g_ipe_out_hsize_hw = 1920, g_ipe_out_vsize_hw = 1080;
static UINT32             g_Ipe_lineofs_hw = 480;
static UINT32             g_egmap_size = (1920 * 1080)>>2;
static DIS_MDS_DIM		  g_mds_dim;



static HD_RESULT share_memory_exit(void)
{
	HD_RESULT ret = HD_OK;
	if (g_mem_init == 1) {
		ret = hd_common_mem_release_block(g_mem_blk);
		if (ret != HD_OK) {
			printf("mem_uninit : hd_common_mem_release_block fail.\r\n");
			return ret;
		}
		g_mem_init = 0;
	}
	return HD_OK;
}
static UINT32 alg_dis_calcbuffsize(void)
{
	UINT32 buffersize;
	buffersize  = dis_get_prvmaxBuffer();
	buffersize += g_egmap_size*2;  //input img;
	buffersize += DIS_MVNUMMAX*12;  //output img;
	buffersize += 0x10000;  //output ;

	return buffersize;	
}

static int alg_dis_mem_init(void)
{
	HD_RESULT                 ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg  = {0};
	HD_COMMON_MEM_VB_BLK      blk;
	UINT32                    pa, va;
	HD_COMMON_MEM_DDR_ID      ddr_id   = DDR_ID0;
	UINT32                    buffer_size;
	
	buffer_size = alg_dis_calcbuffsize();

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = buffer_size;
	mem_cfg.pool_info[0].blk_cnt  = 1;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
	}

	if (g_mem_init != 0) {
		printf("err: mem has already been inited\r\n");
		return -1;
	}
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, buffer_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("err:get block fail\r\n");
		return -1;
	}
	printf("blk = 0x%x\r\n",blk);
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
		return -1;
	}
	printf("pa = 0x%x\r\n", pa);
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, buffer_size);
		if (va == 0) {
			ret = hd_common_mem_release_block(blk);
			if (HD_OK != ret) {
				printf("err:release blk fail %d\r\n", ret);
				return ret;
			}
		}
	}
	g_mem.addr = pa;
	g_mem.va = va;
	g_mem.size = buffer_size;

	printf("input pa = 0x%x, va=0x%x, size =0x%x\r\n", g_mem.addr, g_mem.va, g_mem.size);

	g_dis_outbuf_addr.addr = g_mem.va;
	g_dis_outbuf_addr.size = dis_get_prvmaxBuffer();
	
	printf("g_dis_outbuf_addr va=0x%x, size ==0x%x\r\n", g_dis_outbuf_addr.addr,  g_dis_outbuf_addr.size);

	g_cur_egmap_addr.addr = g_dis_outbuf_addr.addr + dis_get_prvmaxBuffer();
	g_cur_egmap_addr.size = g_egmap_size;

	g_ref_egmap_addr.addr = g_cur_egmap_addr.addr + g_egmap_size;
	g_ref_egmap_addr.size = g_egmap_size;

	printf("g_cur_egmap_addr va=0x%x, size =0x%x\r\n", g_cur_egmap_addr.addr, g_cur_egmap_addr.size);
	printf("g_ref_egmap_addr va=0x%x, size ==0x%x\r\n", g_ref_egmap_addr.addr, g_ref_egmap_addr.size);
	
	g_ix_addr.addr = g_ref_egmap_addr.addr + g_egmap_size;
	g_ix_addr.size = DIS_MVNUMMAX*4;

	g_iy_addr.addr = g_ix_addr.addr + DIS_MVNUMMAX*4;
	g_iy_addr.size = DIS_MVNUMMAX*4;

	g_valid_addr.addr = g_iy_addr.addr + DIS_MVNUMMAX*4;
	g_valid_addr.size = DIS_MVNUMMAX*4;
	
	printf("g_ix_addr va=0x%x, size =0x%x\r\n", g_ix_addr.addr, g_ix_addr.size);
	printf("g_iy_addr va=0x%x, size ==0x%x\r\n", g_iy_addr.addr, g_iy_addr.size);
	printf("g_valid_addr va=0x%x, size ==0x%x\r\n", g_valid_addr.addr, g_valid_addr.size);

	g_availbuf_addr.addr = g_valid_addr.addr + DIS_MVNUMMAX*4;
	g_availbuf_addr.size = 0x10000;

	
	if ((g_valid_addr.addr+g_valid_addr.size - va)>buffer_size) {
		printf("err:memmory 0x%x not enough for 0x%x \r\n", buffer_size, (g_valid_addr.addr+g_valid_addr.size - va));
		return -1;		
	}

	g_mem_init = 1;
	g_mem_blk  = blk;
	return ret;

}
static HD_RESULT alg_dis_mem_exit(void)
{
	HD_RESULT ret = HD_OK;

	ret = share_memory_exit();
	if (ret != HD_OK) {
		printf("mem_uninit : share_memory_exit fail.\r\n");
		return ret;
	}
		
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem_uninit : hd_common_mem_uninit fail.\r\n");
		return ret;
	}

	return ret;
}
static BOOL dis_load_input_test(char* p_filepath, UINT32 ADDR)
{
	INT32  fd;
	INT32  flags = O_RDWR;
	INT32  ret = 0;
	/****open CurEgMap****/
	fd = open(p_filepath, flags, 0777);
	if (fd < 0)    {
		printf("err:open file %s\r\n",p_filepath);
		return FALSE;
	}
	if (lseek(fd, 0, SEEK_SET) == -1) {
		printf("err:seek fail\r\n");
		close(fd);
		return FALSE;
	}
	ret = read(fd, (void *)ADDR, g_egmap_size);
	if (ret < (int)g_egmap_size) {
		printf("err:read header =%d\r\n", ret);
		close(fd);
		return FALSE;
	}
	hd_common_mem_flush_cache((void *)ADDR, g_egmap_size);
	close(fd);

    return TRUE;
}
static BOOL dis_save_result_test(char* p_folder_name,UINT32 frameIdx)
{
	static CHAR filepath[128];
	static CHAR filepath2[128];
	static CHAR filepath3[128];
	FILE *p_fsave=NULL;
	UINT32 save_size = g_mds_dim.ui_blknum_h* g_mds_dim.ui_mdsnum*g_mds_dim.ui_blknum_v*sizeof(UINT32);

	sprintf(filepath  ,"/mnt/sd/DIS/outBin/%s/alg_%ld_iX_test.raw" ,p_folder_name, frameIdx);

	/****SAVE p_ix****/
	p_fsave = fopen(filepath, "w");
	if (p_fsave == NULL) {
		printf("fopen fail\n");
		return FALSE;
	}

	fwrite((UINT8*)g_ix_addr.addr, save_size, 1, p_fsave);

	fclose(p_fsave);

	sprintf(filepath2  ,"/mnt/sd/DIS/outBin/%s/alg_%ld_iY_test.raw" ,p_folder_name, frameIdx);

	/****SAVE p_iy****/
	p_fsave = fopen(filepath2, "w");
	if (p_fsave == NULL) {
		printf("fopen fail\n");
		return FALSE;
	}

	fwrite((UINT8*)g_iy_addr.addr, save_size, 1, p_fsave);

	fclose(p_fsave);

	sprintf(filepath3  ,"/mnt/sd/DIS/outBin/%s/alg_%ld_valid_test.raw" ,p_folder_name, frameIdx);

	/****SAVE p_valid****/
	p_fsave = fopen(filepath3, "w");
	if (p_fsave == NULL) {
		printf("fopen fail\n");
		return FALSE;
	}

	fwrite((UINT8*)g_valid_addr.addr, save_size, 1, p_fsave);

	fclose(p_fsave);

	return TRUE;
}
static UINT32 dis_disdebug_writedatatodram(UINT32 frmcnt, UINT32 bufstart, DIS_MOTION_INFOR motresult[])
{
    UINT32 blknumh, blknumv, ix, iy;
    UINT32 uiidx;
    UINT32 uicurbufptr;
    uicurbufptr = bufstart;
    blknumh = g_mds_dim.ui_blknum_h* g_mds_dim.ui_mdsnum;
    blknumv = g_mds_dim.ui_blknum_v; 

    uicurbufptr += sprintf((char *)uicurbufptr, "[ frame %ld ]\t", frmcnt);
    uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    uicurbufptr += sprintf((char *)uicurbufptr, "MV_X\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%ld\t", motresult[uiidx].ix);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "MV_Y\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%ld\t", motresult[uiidx].iy);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "bValid\r\n");
    for (iy=0; iy<blknumv; iy++) {
        uicurbufptr += sprintf((char *)uicurbufptr, "\t");
        for (ix=0; ix<blknumh; ix++) {
            uiidx = iy*blknumh + ix;
            uicurbufptr += sprintf((char *)uicurbufptr, "%d\t", motresult[uiidx].bvalid);
        }
        uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    }
    uicurbufptr += sprintf((char *)uicurbufptr, "\r\n");
    return uicurbufptr;
}

MAIN(argc, argv)
{
    HD_RESULT                          ret;
    DIS_PARAM                          disinfo;
	DIS_MV_INFO_SIZE                   motresult;
	static DIS_MOTION_INFOR            mvresult[DIS_MVNUMMAX];	
	
	UINT32	blkpMds;	
	UINT32	i,j;
	UINT32 *p_ix, *p_iy, *p_valid;

	UINT32 frm;
	UINT32 idxSt = 131;
	UINT32 idxEd = 570; 
	static char filename[100];

	static UINT64 time_stamp  = 123456;
	static UINT32 frame_count = 1;

	CHAR *p_folder = "alg_dis";

	// init common module
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }
	// init memory
	ret = alg_dis_mem_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

	// init dis infor
	dis_initialize(&g_dis_outbuf_addr);	

	// set dis infor
	disinfo.in_add0    = g_cur_egmap_addr.addr;
	disinfo.in_add1    = g_ref_egmap_addr.addr;
	disinfo.in_add2    = g_ref_egmap_addr.addr;
	disinfo.in_size_h  = g_ipe_out_hsize_hw;
	disinfo.in_size_v  = g_ipe_out_vsize_hw;
	disinfo.in_lineofs = g_Ipe_lineofs_hw;
			
    for ( frm= idxSt; frm <= idxEd; frm++ ) {
		sprintf(filename  ,"/mnt/sd/DIS/IN/img_5m/frame%ld_rawData.bin" , frm);
		printf("\r\nimage: %s................................\r\n", filename);
		// load input
		dis_load_input_test(filename, g_ref_egmap_addr.addr);

		if (frm == idxSt) {
			memcpy((UINT8 *)g_cur_egmap_addr.addr, (UINT8 *)g_ref_egmap_addr.addr, g_egmap_size*sizeof(UINT8));
			frame_count++;
			continue;
		}
		
		dis_push_time_stamp(time_stamp++, frame_count);

		disinfo.frame_cnt = frame_count;

		dis_set_disinfor(&disinfo);
	
		// dis process
		dis_process_no_compen_info();
		
		g_mds_dim = dis_get_mds_dim();

		motresult.motvec = mvresult;
		dis_get_ready_motionvec(&motresult);

		memcpy((UINT8 *)g_cur_egmap_addr.addr, (UINT8 *)g_ref_egmap_addr.addr, g_egmap_size*sizeof(UINT8));

		blkpMds = g_mds_dim.ui_blknum_h* g_mds_dim.ui_mdsnum;

		p_ix = (UINT32*)g_ix_addr.addr;
		p_iy = (UINT32*)g_iy_addr.addr;
		p_valid = (UINT32*)g_valid_addr.addr;	

		for (j = 0; j < g_mds_dim.ui_blknum_v; j++) {
			for (i = 0; i < blkpMds; i++){			
				p_ix[j*blkpMds+i] = motresult.motvec[j*blkpMds+i].ix;
				p_iy[j*blkpMds+i] = motresult.motvec[j*blkpMds+i].iy;
				p_valid[j*blkpMds+i]= motresult.motvec[j*blkpMds+i].bvalid;
				}
			}
		
		if (!dis_save_result_test(p_folder,frame_count++))
			printf("err: dis save result fail\n");
		printf("[dis sample] write result done!\n");

		static UINT32 frmcnt = 2;
		UINT32 uiEndBuffer = 0;
		UINT32 out_length = 0;
		char filename[200] = {0};
		FILE *p_fsave=NULL;
		static UINT32 saveMvInit = 1;
		uiEndBuffer = dis_disdebug_writedatatodram(frmcnt, g_availbuf_addr.addr, motresult.motvec);
		out_length = uiEndBuffer - g_availbuf_addr.addr;
		sprintf(filename, "/mnt/sd/DIS/alg/motResult_bin_alg_dis.txt");

		if ( 1 == saveMvInit ) {
		    p_fsave = fopen(filename, "w+");
			if(p_fsave == NULL) {
				printf("fopen fail\n");
				return FALSE;
			}
			saveMvInit = 0;
		} else {
		    p_fsave = fopen(filename, "a+");	
			if(p_fsave == NULL) {
				printf("fopen fail\n");
				return FALSE;
			}
			fseek(p_fsave,0,SEEK_END);
		}
		fwrite((UINT8*)g_availbuf_addr.addr, out_length, 1, p_fsave);
		fclose(p_fsave);

		frmcnt++;
    }

exit:
	dis_end();
	ret = alg_dis_mem_exit();
    if(ret != HD_OK) {
        printf("alg_dis_mem_exit fail=%d\n", ret);
        goto exit;
    }	
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("uninit fail=%d\n", ret);
        goto exit;
    }
	return 0;
}
