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
#include <time.h>
#include "kwrap/examsys.h"
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
#define MAIN(argc, argv)        EXAMFUNC_ENTRY(alg_ive_lut_sample_nonblocking, argc, argv)
#define GETCHAR()               NVT_EXAMSYS_GETCHAR()
#endif

#define MAX_IMG_WIDTH         4000
#define MAX_IMG_HEIGHT        6000
#define IMG_BUF_SIZE          (MAX_IMG_WIDTH * MAX_IMG_HEIGHT)

#define IVE_CEILING(a, n)  (((a) + ((n)-1)) & (~((n)-1)))
#define IVE_4_BYTE_ALIGN_CEILING(a)  IVE_CEILING((a),4)

typedef struct _MEM_RANGE {
	UINT32 va;        ///< Memory buffer starting address
	UINT32 addr;      ///< Memory buffer starting address
	UINT32 size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

#define IVE_MAX_MEM_NUM  4

static CHAR   version_info[32];

static void share_memory_init(MEM_RANGE *p_share_mem)
{

    HD_COMMON_MEM_VB_BLK blk;
	UINT8 i;
	UINT32 pa, va;
	UINT32 blk_size = IMG_BUF_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret;

	if (p_share_mem == NULL) {
		printf("err:p_share_mem == NULL fail\r\n", blk);
		return;
	}

	for(i=0;i<IVE_MAX_MEM_NUM;i++){
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	for(i=0;i<IVE_MAX_MEM_NUM;i++){
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

	if (p_share_mem == NULL) {
		printf("err:p_share_mem == NULL fail\r\n");
		return;
	}

	for(i=0;i<IVE_MAX_MEM_NUM;i++){
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
	mem_cfg.pool_info[0].blk_cnt = IVE_MAX_MEM_NUM;
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

UINT8 ive_exit[2] = {0};

static HD_RESULT ive_lut_binary_process(UINT32 in_width, UINT32 in_height, 
												UINT32 ive_in_addr, UINT32 ive_out_addr, UINT32 check_val, UINT32 in_lofs, UINT32 out_lofs, UINT32 ch)
{
	HD_RESULT 						ret=HD_OK;
	INT32                          	i;
	VENDOR_IVE_IN_IMG_INFO         	ive_input_info;
	VENDOR_IVE_IMG_IN_DMA_INFO     	ive_input_addr;
	VENDOR_IVE_IMG_OUT_DMA_INFO    	ive_output_addr;
	VENDOR_IVE_THRES_LUT_PARAM     	ive_thres_lut_param;
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
    ret = vendor_ive_trigger_nonblock(ch);
    if (ret != HD_OK) {
        //printf("err: ive trigger engine error %d\n", ret);
        goto exit;
    }

exit:

	return ret;
}

static HD_RESULT ive_lut_binary_process_wait_done(UINT32 ch)
{
	HD_RESULT  ret=HD_OK;
	UINT32     timeout=0;

	ret = vendor_ive_waitdone_nonblock(&timeout, ch);
	if (ret == VENDOR_IVE_MSG_TIMEOUT) {
		printf("err: timeout at vendor_ive_waitdone_nonblock\n");
		ret = HD_ERR_TIMEDOUT;
		goto exit;
	} else {
		if (ret != HD_OK) {
			//printf("err: vendor_ive_waitdone_nonblock error %d\n", ret);
			goto exit;
		}
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

static VOID ive_prepare_input_data(char *in_buf, UINT32 test_width, UINT32 test_height, UINT32 in_lofs, UINT32 out_lofs, UINT32 check_val)
{
	UINT32 	data_val;
	UINT32	in_x,in_y; 

	if (in_buf == NULL) {
		printf("err: in_buf==NULL fail.\r\n");
		return;
	}
	
	if (out_lofs == 0) {
		out_lofs = 4;
	}

	//prepare input data
	data_val = check_val + 1;
	for(in_y = 0; in_y < test_height; in_y++) {
		for (in_x = 0; in_x < test_width; in_x++) {
			if (data_val == (check_val + 1)) {
				in_buf[in_y*in_lofs + in_x] = data_val;
				data_val = (check_val - 1);
			} else {
				in_buf[in_y*in_lofs + in_x] = data_val;
				data_val = (check_val + 1);
			}
		}
	}
}

static INT32 ive_check_output_data(char *in_buf, char *out_buf, UINT32 test_width, UINT32 test_height, UINT32 in_lofs, UINT32 out_lofs, 
									UINT32 check_val, UINT32 bits_per_byte, UINT32 thread_id)
{
	UINT8 	is_check_ok;
	UINT32	in_x,in_y; 
	INT32 	ret = 0;

	if (in_buf == NULL) {
		printf("err: in_buf==NULL fail.\r\n");
		ret = -1;
		goto ERR_EXIT;
	}

	if (out_buf == NULL) {
		printf("err: out_buf==NULL fail.\r\n");
		ret = -1;
        goto ERR_EXIT;
	}

	is_check_ok = 1;
	in_x = 0;
	in_y = 0;
	for(in_y = 0; in_y < test_height; in_y++) {

		for (in_x = 0; in_x < test_width; in_x++) {
			if (ive_get_in_byte(in_x, in_y, in_lofs, in_buf) > check_val) {
				if (ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf) == 1) {
					//check ok
				} else {
					printf("IVE: data check error at in_x(%d) in_y(%d) in_8bit_val(%d) out_1bit_val(%d), check(%d) at thread(%d)\r\n",
							in_x, in_y, ive_get_in_byte(in_x, in_y, in_lofs, in_buf),
							ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf), check_val, thread_id);
					is_check_ok = 0;
					break;
				}
			} else if (ive_get_in_byte(in_x, in_y, in_lofs, in_buf) <= check_val) {
				if (ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf) == 0) {
					//check ok
				} else {
					printf("IVE: data check error at in_x(%d) in_y(%d) in_8bit_val(%d) out_1bit_val(%d), check(%d) at thread(%d)\r\n",
							in_x, in_y, ive_get_in_byte(in_x, in_y, in_lofs, in_buf),
							ive_get_out_bit(in_x, in_y, out_lofs, bits_per_byte, out_buf), check_val, thread_id);
					is_check_ok = 0;
					break;
				}
			}
		}
	}

	if (is_check_ok == 1) {
		printf("IVE: data check ok. at test_width=%d at thread(%d)\r\n", test_width, thread_id);
	} else {
		ret = -1;
		goto ERR_EXIT;
	}

ERR_EXIT:

	return ret;

}


static VOID *ive_thread_1_api(VOID *arg)
{
	MEM_RANGE 	*p_share_mem = (MEM_RANGE *)arg;
	HD_RESULT   ret;
	UINT32      ch;
	UINT32		test_width;
	UINT32		test_height;
	UINT32 		ive_in_addr;
    UINT32 		ive_out_addr;
    UINT32 		check_val = 64;
	char        *in_buf = 0;
	char 		*out_buf = 0;
	UINT32 		in_lofs;
	UINT32 		out_lofs;
	UINT32		line_size;
	UINT32  	byte_per_pixel = 1;
	UINT32 		bits_per_byte = 8;
	UINT32  	out_bit_per_byte = 1;
	UINT32		thread_id = 1;

	ch = 0;
	test_width = 4;
	test_height = 11;
	do{
		ret = ive_lut_binary_process_wait_done(ch);
		if (ret != HD_OK) {
			printf("Error(thread 1),wait_done fail.");
			usleep(1000*(rand()%200));
			continue;
		} else {
			if (in_buf != 0) {
				ret = ive_check_output_data(in_buf, out_buf, test_width, test_height, in_lofs, out_lofs, check_val, bits_per_byte, thread_id);
				if (ret != 0) {
					printf("IVE(thread_1): data check error and exit.\n");
					break;
				}
				test_width++;
				if (test_width < 640) {
					//do not thing
				} else {
					test_width = 4;
				}
			}
		}

		in_buf = (char *) p_share_mem[0].va;
		out_buf = (char *) p_share_mem[1].va;
		ive_in_addr = p_share_mem[0].addr;
		ive_out_addr = p_share_mem[1].addr;

		in_lofs = IVE_4_BYTE_ALIGN_CEILING(test_width * byte_per_pixel);
		line_size = (in_lofs * byte_per_pixel * out_bit_per_byte);
		out_lofs = IVE_4_BYTE_ALIGN_CEILING((line_size / bits_per_byte) + (line_size % bits_per_byte));

		ive_prepare_input_data(in_buf, test_width, test_height, in_lofs, out_lofs, check_val);
		ret = ive_lut_binary_process(test_width, test_height, ive_in_addr, ive_out_addr, check_val, in_lofs, out_lofs, ch);
		if (HD_OK != ret) {
			//printf("IVE: process error.\r\n");
			usleep(1000*(rand()%200));
			continue;
		}

		usleep(1000*(rand()%200));

	} while(ive_exit[0] == 0);


	return 0;
}

#if 1
static VOID *ive_thread_2_api(VOID *arg)
{
	MEM_RANGE 	*p_share_mem = (MEM_RANGE *)arg;
	HD_RESULT   ret;
	UINT32      ch;
	UINT32		test_width;
	UINT32		test_height;
	UINT32 		ive_in_addr;
    UINT32 		ive_out_addr;
    UINT32 		check_val = 64;
	char        *in_buf = 0;
	char 		*out_buf = 0;
	UINT32 		in_lofs;
	UINT32 		out_lofs;
	UINT32		line_size;
	UINT32  	byte_per_pixel = 1;
	UINT32 		bits_per_byte = 8;
	UINT32  	out_bit_per_byte = 1;
	UINT32		thread_id = 2;

	ch = 1;
	test_width = 4;
	test_height = 11;
	do{
		ret = ive_lut_binary_process_wait_done(ch);
		if (ret != HD_OK) {
			printf("Error(thread 2), wait_done fail.");
			usleep(1000*(rand()%200));
			continue;
		} else {
			if (in_buf != 0) {
				ret = ive_check_output_data(in_buf, out_buf, test_width, test_height, in_lofs, out_lofs, check_val, bits_per_byte, thread_id);
				if (ret != 0) {
					printf("IVE(thread_2): data check error and exit.\n");
					break;
				}
				test_width++;
				if (test_width < 640) {
					//do not thing
				} else {
					test_width = 4;
				}
			}
		}

		in_buf = (char *) p_share_mem[2].va;
		out_buf = (char *) p_share_mem[3].va;
		ive_in_addr = p_share_mem[2].addr;
		ive_out_addr = p_share_mem[3].addr;

		in_lofs = IVE_4_BYTE_ALIGN_CEILING(test_width * byte_per_pixel);
		line_size = (in_lofs * byte_per_pixel * out_bit_per_byte);
		out_lofs = IVE_4_BYTE_ALIGN_CEILING((line_size / bits_per_byte) + (line_size % bits_per_byte));

		ive_prepare_input_data(in_buf, test_width, test_height, in_lofs, out_lofs, check_val);
		ret = ive_lut_binary_process(test_width, test_height, ive_in_addr, ive_out_addr, check_val, in_lofs, out_lofs, ch);
		if (HD_OK != ret) {
			//printf("IVE: process error.\r\n");
			usleep(1000*(rand()%200));
			continue;
		}

		usleep(1000*(rand()%200));

	} while(ive_exit[0] == 0);

	return 0;
}
#endif

MAIN(argc, argv)
{
    HD_RESULT ret;
	pthread_t ive_thread_id[2];
    MEM_RANGE share_mem[IVE_MAX_MEM_NUM] = {0};
	INT key;

	srand(time(0));
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


	ive_exit[0] = 0;
	ive_exit[1] = 0;

	ret = pthread_create(&ive_thread_id[0], NULL, ive_thread_1_api, (VOID*)share_mem);
	if (ret < 0) {
		printf("CLRCST: Error, create encode thread failed");
		goto exit;
	}
	ret = pthread_create(&ive_thread_id[1], NULL, ive_thread_2_api, (VOID*)share_mem);
	if (ret < 0) {
		printf("CLRCST: Error, create encode thread failed");
		goto exit;
	}

	do {
        printf("  enter q: exit\n");
        key = NVT_EXAMSYS_GETCHAR();
        if (key == 'q' || key == 0x3) {
			ive_exit[0] = 1;
			ive_exit[1] = 1;
            break;
        }
	} while(1);

	pthread_join(ive_thread_id[0], NULL);
	pthread_join(ive_thread_id[1], NULL);
	
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

