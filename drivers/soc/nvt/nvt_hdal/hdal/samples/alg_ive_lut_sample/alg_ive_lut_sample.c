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
#define MAIN(argc, argv)        EXAMFUNC_ENTRY(alg_ive_lut_sample, argc, argv)
#define GETCHAR()               NVT_EXAMSYS_GETCHAR()
#endif

#define MAX_IMG_WIDTH         4000
#define MAX_IMG_HEIGHT        4000
#define IMG_BUF_SIZE          (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)

#define IVE_CEILING(a, n)  (((a) + ((n)-1)) & (~((n)-1)))
#define IVE_4_BYTE_ALIGN_CEILING(a)  IVE_CEILING((a),4)

typedef struct _MEM_RANGE {
	UINT32 va;        ///< Memory buffer starting address
	UINT32 addr;      ///< Memory buffer starting address
	UINT32 size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

CHAR   version_info[32];

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

static HD_RESULT ive_lut_binary_process(UINT32 in_width, UINT32 in_height, 
												UINT32 ive_in_addr, UINT32 ive_out_addr, UINT32 check_val, UINT32 in_lofs, UINT32 out_lofs)
{
	HD_RESULT 						ret=HD_OK;
	INT32                          	i;
	UINT32                         	ch = 0;
	VENDOR_IVE_IN_IMG_INFO         	ive_input_info;
	VENDOR_IVE_IMG_IN_DMA_INFO     	ive_input_addr;
	VENDOR_IVE_IMG_OUT_DMA_INFO    	ive_output_addr;
	VENDOR_IVE_THRES_LUT_PARAM     	ive_thres_lut_param;
	VENDOR_IVE_TRIGGER_PARAM       	ive_trigger_param;
	VENDOR_IVE_OUTSEL_PARAM        	ive_outsel_param;
	UINT32 lut_thres_coef[VENDOR_IVE_EDGE_TH_LUT_TAP] = {0};
	UINT32 out_selection;
	
	// set input & output info
	ive_input_addr.addr = ive_in_addr;
	ive_output_addr.addr = ive_out_addr;
	ive_input_info.width = in_width;
	ive_input_info.height = in_height;
	ive_input_info.lineofst = in_lofs;
	out_selection = 0;
	lut_thres_coef[0] = check_val;

	ret = vendor_ive_get_param(VENDOR_IVE_VERSION, version_info, ch);
	if (ret != HD_OK) {
		printf("err: ive get version info error %d\n", ret);
		goto exit;
	}
	printf("vendor_ive: ive_version(%s)\r\n", version_info);

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

	// set func enable
	ive_thres_lut_param.enable      = 1;
	ive_trigger_param.time_out_ms   = 0;
	ive_trigger_param.wait_end      = 1;
    ive_trigger_param.is_nonblock   = 0;
    
    // set thres lut param
    for (i=0; i<VENDOR_IVE_EDGE_TH_LUT_TAP; i++)
        ive_thres_lut_param.thres_lut[i] = lut_thres_coef[i];
   
    //set output select and output lofset
    ive_outsel_param.OutDataSel = out_selection;
    ive_outsel_param.Outlofs = out_lofs;

    ret = vendor_ive_set_param(VENDOR_IVE_OUTSEL, &ive_outsel_param, ch);
    if (ret != HD_OK) {
        printf("err: ive set ive_outsel_param error %d\n", ret);
        goto exit;
    }

	ret = vendor_ive_set_param(VENDOR_IVE_THRES_LUT, &ive_thres_lut_param, ch);
	if (ret != HD_OK) {
        printf("err: ive set ive_thres_lut_param error %d\n", ret);
        goto exit;
    }

    // trigger engine
    ret = vendor_ive_trigger(&ive_trigger_param, ch);
    if (ret != HD_OK) {
        printf("err: ive trigger engine error %d\n", ret);
        goto exit;
    }

exit:

	return ret;
}

static UINT8 ive_get_in_byte(UINT32 in_x, UINT32 in_y, UINT32 in_lofs, char *in_buf)
{
	return in_buf[in_y * in_lofs + in_x];
}

static BOOL ive_get_out_bit(UINT32 in_x, UINT32 in_y, UINT32 out_lofs, UINT32 bits_per_byte, char *out_buf)
{
	UINT32 out_offset_bits;
	UINT32 out_offset_byte;
	UINT32 bit_offset;

	out_offset_bits = in_y * out_lofs * bits_per_byte + in_x;
	out_offset_byte = out_offset_bits / bits_per_byte;
 	bit_offset = out_offset_bits % bits_per_byte;	

#if 0
	printf("IVE: out_offset_byte(%d) bit_offset(%d) in_y(%d) out_lofs(%d) bits_per_byte(%d), in_x(%d)\r\n",
				out_offset_byte, bit_offset, in_y, out_lofs, bits_per_byte, in_x);
#endif

	return ((out_buf[out_offset_byte] & (1 << bit_offset)) ? 1 : 0);
}

MAIN(argc, argv)
{
    HD_RESULT ret;
	UINT32 in_width=0;
	UINT32 in_height=0;
	UINT32 in_x,in_y;
	char *in_buf=0;
	char *out_buf=0;
	UINT32 ive_in_addr;
	UINT32 ive_out_addr;
	UINT32 data_val;
	UINT32 check_val = 64;
	UINT8 is_check_ok;
    MEM_RANGE share_mem[2] = {0};
	UINT32 in_lofs;
	UINT32 out_lofs;
	UINT32 bits_per_byte=8;
	UINT32 byte_per_pixel=1;
	UINT32 out_bit_per_byte=1;
	UINT32 line_size;
	UINT32 test_width;
	

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

	// init ive module
	ret = vendor_ive_init();
	if (ret != HD_OK) {
		printf("err:vendor_ive_init error %d\r\n",ret);
		goto exit;
	}

	vendor_ive_get_version(version_info);
	printf("vendor_ive: ive_version(%s)\r\n", version_info);

	test_width = 4;
	while(test_width < 640) {
		printf("IVE: test test_width = %d\r\n", test_width);
		//prepare parameter
		in_width = test_width;
		in_height = 11;
		bits_per_byte = 8;
		in_buf = (char *) share_mem[0].va;
		out_buf = (char *) share_mem[1].va;
		ive_in_addr = share_mem[0].addr;
		ive_out_addr = share_mem[1].addr;
		// set input & output info
    	in_lofs = IVE_4_BYTE_ALIGN_CEILING(in_width * byte_per_pixel);
		line_size = (in_lofs * byte_per_pixel * out_bit_per_byte);
		out_lofs = IVE_4_BYTE_ALIGN_CEILING((line_size / bits_per_byte) + (line_size % bits_per_byte));
		if (out_lofs == 0) {
			out_lofs = 4;
		}

		printf("IVE: in_lofs(%d) out_lofs(%d)\r\n", in_lofs, out_lofs);

		//prepare input data
		data_val = check_val + 1;
		for(in_y = 0; in_y < in_height; in_y++) {
			for (in_x = 0; in_x < in_width; in_x++) {
				if (data_val == (check_val + 1)) {
					in_buf[in_y*in_lofs + in_x] = data_val;
					data_val = (check_val - 1);
				} else {
					in_buf[in_y*in_lofs + in_x] = data_val;
					data_val = (check_val + 1);
				}
			}
		}

		ret = ive_lut_binary_process(in_width, in_height, ive_in_addr, ive_out_addr, check_val, in_lofs, out_lofs);
		if (HD_OK != ret) {
			printf("IVE: process error.\r\n");
			goto ive_err;
		}

	#if 0
		in_x= 0;
		in_y = 0;
		for(in_y = 0; in_y < in_height; in_y++) {
			printf("in_x(%d) in_y(%d) ", in_x, in_y);
			for (in_x = 0; in_x < in_lofs; in_x++) {
				printf("%d ", in_buf[(in_y*in_lofs) + in_x]);
			}
			printf("\r\n");
		}

		in_x= 0;
		in_y = 0;
		for(in_y = 0; in_y < in_height; in_y++) {
			printf("in_x(%d) in_y(%d) ", in_x, in_y);
			for (in_x = 0; in_x < out_lofs; in_x++) {
				printf("%d ", out_buf[(in_y*out_lofs) + in_x]);
			}
			printf("\r\n");
		}
	#endif

		
		//check output buffer
		is_check_ok = 1;
		in_x = 0;
		in_y = 0;
		for(in_y = 0; in_y < in_height; in_y++) {
			for (in_x = 0; in_x < in_width; in_x++) {
				if (ive_get_in_byte(in_x, in_y, in_lofs, in_buf) > check_val) {
					if (ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf) == 1) {
						//check ok
					} else {
						printf("IVE: data check error at in_x(%d) in_y(%d) in_8bit_val(%d) out_1bit_val(%d), check(%d)\r\n", 
								in_x, in_y, ive_get_in_byte(in_x, in_y, in_lofs, in_buf),
								ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf), check_val);
						is_check_ok = 0;
						break;
					}	
				} else if (ive_get_in_byte(in_x, in_y, in_lofs, in_buf) <= check_val) {
					if (ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf) == 0) {
						//check ok
					} else {
						printf("IVE: data check error at in_x(%d) in_y(%d) in_8bit_val(%d) out_1bit_val(%d), check(%d)\r\n",
								in_x, in_y, ive_get_in_byte(in_x, in_y, in_lofs, in_buf),
								ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf), check_val);
						is_check_ok = 0;
						break;
					}
				}
			}
		}

		if (is_check_ok == 1) {
			printf("IVE: data check ok. at test_width=%d\r\n", test_width);
		} else {
			break;
		}

		test_width++;
	}

ive_err:

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

