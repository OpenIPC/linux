/**
 * @file vendor_ive_sample.c
 * @brief start ive sample.
 * @author ALG1-CV
 * @date in the year 2018
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hd_debug.h"
#include "hdal.h"
#include "vendor_ive.h"

#if defined(__LINUX)
#include <pthread.h>            //for pthread API
#define MAIN(argc, argv)        int main(int argc, char** argv)
#define GETCHAR()               getchar()
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>     //for sleep API
#define sleep(x)                vos_util_delay_ms(1000*(x))
#define msleep(x)               vos_util_delay_ms(x)
#define usleep(x)               vos_util_delay_us(x)
#include <kwrap/examsys.h>  //for MAIN(), GETCHAR() API
#define MAIN(argc, argv)        EXAMFUNC_ENTRY(alg_ive_irv_sample, argc, argv)
#define GETCHAR()               NVT_EXAMSYS_GETCHAR()
#endif

#define MAX_IMG_WIDTH         4000
#define MAX_IMG_HEIGHT        4000
#define IMG_BUF_SIZE          (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)

typedef struct _MEM_RANGE {
	UINT32 va;        ///< Memory buffer starting address
	UINT32 addr;      ///< Memory buffer starting address
	UINT32 size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

static void share_memory_init(MEM_RANGE *p_share_mem)
{

    HD_COMMON_MEM_VB_BLK blk;
	UINT8 i;
	UINT32 pa, va;
	UINT32 blk_size = IMG_BUF_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret;

	for(i=0;i<2;i++){
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	for(i=0;i<2;i++){
		blk_size = IMG_BUF_SIZE;
		//printf("blk_size : %d ",blk_size);
		blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_BLK, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("err:get block fail\r\n", blk);
			return;
		}
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
			goto blk2pa_err;
		}
		//printf("pa = 0x%x\r\n", pa);
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
			if (va == 0) {
				goto map_err;
			}
		}
		p_share_mem[i].addr = pa;
		p_share_mem[i].va   = va;
		p_share_mem[i].size = blk_size;
		p_share_mem[i].blk  = blk;
		
	}
	return;
blk2pa_err:
map_err:
	for (; i > 0 ;) {
		i -= 1;
		ret = hd_common_mem_release_block(p_share_mem[i].blk);
		if (HD_OK != ret) {
			printf("err:release blk fail %d\r\n", ret);
			return;
		}
	}
}

static void share_memory_exit(MEM_RANGE *p_share_mem)
{
	UINT8 i;
	HD_RESULT ret;
	for(i=0;i<2;i++){
		if (p_share_mem[i].va != 0) {
			ret = hd_common_mem_munmap((void *)p_share_mem[i].va, p_share_mem[i].size);
			if (HD_OK != ret) {
                printf("err:hd_common_mem_munmap err: \r\n\r\n");
            }
		}
		if (p_share_mem[i].blk != HD_COMMON_MEM_VB_INVALID_BLK) {
			ret = hd_common_mem_release_block(p_share_mem[i].blk);
			if (HD_OK != ret) {
				printf("err:hd_common_mem_release_block err: %d\r\n", ret);
			}
		}
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
}

static int mem_init(void)
{
	HD_RESULT                 ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg  = {0};

	mem_cfg.pool_info[0].type = HD_COMMON_MEM_USER_BLK;
	mem_cfg.pool_info[0].blk_size = IMG_BUF_SIZE;
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
	}

	return ret;



}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	//share_memory_exit();
	ret = hd_common_mem_uninit();
	return ret;
}

static UINT32 ive_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

MAIN(argc, argv)
{
    HD_RESULT                          ret;

	INT32                              i;
	UINT32                             ch = 0;
	VENDOR_IVE_IN_IMG_INFO             ive_input_info;
	VENDOR_IVE_IMG_IN_DMA_INFO         ive_input_addr;
	VENDOR_IVE_IMG_OUT_DMA_INFO        ive_output_addr;
	VENDOR_IVE_MEDIAN_FILTER_PARAM     ive_median_filter_param;
	VENDOR_IVE_EDGE_FILTER_PARAM       ive_edge_filter_param;
	VENDOR_IVE_THRES_LUT_PARAM         ive_thres_lut_param;
	VENDOR_IVE_MORPH_FILTER_PARAM      ive_morph_filter_param;
	VENDOR_IVE_INTEGRAL_IMG_PARAM      ive_integral_img_param;
	VENDOR_IVE_TRIGGER_PARAM           ive_trigger_param;
	VENDOR_IVE_OUTSEL_PARAM            ive_outsel_param;
	VENDOR_IVE_IRV_PARAM               ive_irv_param;

	UINT32 edge_filter_coef[2][VENDOR_IVE_EDGE_COEFF_NUM] ={{25,4,9,31,29,5,4,29,15},{30,4,11,11,30,31,28,13,31}};
	UINT32 lut_thres_coef[VENDOR_IVE_EDGE_TH_LUT_TAP] = {12, 25, 46, 47, 48, 87, 92, 117, 139, 165, 187, 194, 195, 211, 225};
	BOOL morph_coef[VENDOR_IVE_MORPH_NEIGH_NUM] =  {FALSE, FALSE, FALSE, FALSE, 
													FALSE, TRUE, FALSE, TRUE, 
													FALSE, FALSE, TRUE, FALSE, 
													TRUE, FALSE, FALSE, TRUE,
                                                    TRUE, TRUE, TRUE, TRUE,
													TRUE, FALSE, FALSE, FALSE};


    char out_file1[64],in_file1[64];
    FILE  *fd;
    UINT32 file_size = 0;
    UINT32 out_lofs =0, out_selection=0;
    int t_index= 11246;
    int output_size = 32*200;

    MEM_RANGE share_mem[2] = {0};
	// init common module
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto comm_init_fail;
    }
	// init memory
	ret = mem_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto mem_init_fail;
    }

    // init share memory
	share_memory_init(share_mem);

	// init gfx for scale
    ret = hd_gfx_init();
    if (HD_OK != ret) {
        printf("hd_gfx_init fail\r\n");
        goto exit;
    }


	//ive_load_input();
#if defined(__FREERTOS)
	snprintf(in_file1, 64, "A:\\IVEP\\1_All\\02_di\\ive%05d.bin", t_index);
#else
    snprintf(in_file1, 64, "//mnt//sd//IVEP//1_All//02_di//ive%05d.bin", t_index);
#endif
    file_size = ive_load_file(in_file1, share_mem[0].va);
    if (file_size == 0) {
			printf("load dram_in_y.bin : %s\r\n", in_file1);
			return 0;
    }

	// init ive module
	ret = vendor_ive_init();
	if (ret != HD_OK) {
		printf("err:vendor_ive_init error %d\r\n",ret);
		goto exit;
	}

	// set input & output info
	ive_input_addr.addr = share_mem[0].addr;
	ive_output_addr.addr = share_mem[1].addr;
	ive_input_info.width = 256;
	ive_input_info.height = 200;
	ive_input_info.lineofst = 256;
	out_lofs = 32;
	out_selection =0;


	ret = vendor_ive_set_param(VENDOR_IVE_INPUT_INFO, &ive_input_info, ch);
	if (ret != HD_OK) {
		printf("err: ive set input info error %d\n", ret);
		goto exit;
	}
	ret = vendor_ive_set_param(VENDOR_IVE_INPUT_ADDR, &ive_input_addr, ch);
	if (ret != HD_OK) {
		printf("err: ive set input addr error %d\n", ret);
		goto exit;
	}
	ret = vendor_ive_set_param(VENDOR_IVE_OUTPUT_ADDR, &ive_output_addr, ch);
	if (ret != HD_OK) {
		printf("err: ive set output addr error %d\n", ret);
		goto exit;
	}

	memset((void *) share_mem[1].va, 0, IMG_BUF_SIZE);

	// set func enable
	ive_irv_param.enable = 1;
	ive_median_filter_param.enable  = 1;
	ive_edge_filter_param.enable    = 1;
	ive_thres_lut_param.enable      = 1;
	ive_morph_filter_param.enable   = 1;
	ive_integral_img_param.enable   = 0;
	ive_trigger_param.time_out_ms   = 0;
	ive_trigger_param.wait_end      = 1;
    ive_trigger_param.is_nonblock   = 0;



    // set general filter param
	ive_irv_param.hist_mode_sel = 1;
	ive_irv_param.invalid_val = 0;
	ive_irv_param.thr_s = 62;
	ive_irv_param.thr_h = 52;


    // set median filter param
    if (ive_median_filter_param.enable) {
        ive_median_filter_param.mode = VENDOR_IVE_MAX;
		ive_median_filter_param.medn_inval_th = 0;
	}


    // set edge filter param
    ive_edge_filter_param.mode = VENDOR_IVE_BI_DIR;
    ive_edge_filter_param.AngSlpFact = 9;
    for (i=0;i<VENDOR_IVE_EDGE_COEFF_NUM;i++){
        ive_edge_filter_param.edge_coeff1[i] = edge_filter_coef[0][i];        
        ive_edge_filter_param.edge_coeff2[i] = edge_filter_coef[1][i];        
    }
    ive_edge_filter_param.edge_shift_bit = 9;

    // set thres lut param
    for (i=0; i<VENDOR_IVE_EDGE_TH_LUT_TAP; i++)
        ive_thres_lut_param.thres_lut[i] = lut_thres_coef[i];

    // set morph filter param
    ive_morph_filter_param.in_sel = VENDOR_IVE_TH_LUT_IN;
    ive_morph_filter_param.operation = VENDOR_IVE_ERODE;
    for (i=0; i<VENDOR_IVE_MORPH_NEIGH_NUM; i++)
        ive_morph_filter_param.neighbor[i] = morph_coef[i];

    //set output select and output lofset
    ive_outsel_param.OutDataSel = out_selection;
    ive_outsel_param.Outlofs = out_lofs;

    ret = vendor_ive_set_param(VENDOR_IVE_OUTSEL, &ive_outsel_param, ch);
    if (ret != HD_OK) {
        printf("err: ive set ive_outsel_param error %d\n", ret);
        goto exit;
    }

    ret = vendor_ive_set_param(VENDOR_IVE_IRV, &ive_irv_param, ch);
    ret = vendor_ive_set_param(VENDOR_IVE_MEDIAN_FILTER, &ive_median_filter_param, ch);
    ret = vendor_ive_set_param(VENDOR_IVE_EDGE_FILTER, &ive_edge_filter_param, ch);
	ret = vendor_ive_set_param(VENDOR_IVE_THRES_LUT, &ive_thres_lut_param, ch);
	ret = vendor_ive_set_param(VENDOR_IVE_MORPH_FILTER, &ive_morph_filter_param, ch);
	ret = vendor_ive_set_param(VENDOR_IVE_INTEGRAL_IMG, &ive_integral_img_param, ch);


    // trigger engine
    ret = vendor_ive_trigger(&ive_trigger_param, ch);
    if (ret != HD_OK) {
        printf("err: ive trigger engine error %d\n", ret);
        goto exit;
    }

#if defined(__FREERTOS)
	snprintf(out_file1, 64, "A:\\IVEP\\1_All\\05_DO\\ive%05d.bin", t_index);
#else
    snprintf(out_file1, 64, "//mnt//sd//IVEP//1_All//05_DO//ive%05d.bin", t_index);
#endif
	fd = fopen(out_file1, "wb");
	if (!fd) {
		printf("cannot open %s\r\n", out_file1);
		return 0;
	}

	file_size = fwrite((const void *)share_mem[1].va,1,output_size,fd);

	fclose(fd);
    
	printf("output writeout size : %d \n",file_size);
    printf("[ive sample] write result done!\n");


    ret = vendor_ive_uninit();
	if (HD_OK != ret)
		printf("uninit fail, error code = %d\r\n", ret);

exit:
	ret = hd_gfx_uninit();
	if (HD_OK != ret) {
		printf("hd_gfx_uninit fail\r\n");
	}
	share_memory_exit(share_mem);
	ret = mem_exit();
	if (ret != HD_OK) {
        printf("mem fail=%d\n", ret);
    }
mem_init_fail:
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("uninit fail=%d\n", ret);

    }
comm_init_fail:
	return 0;


}

