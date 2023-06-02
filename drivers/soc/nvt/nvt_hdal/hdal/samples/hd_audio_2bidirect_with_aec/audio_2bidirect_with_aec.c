/**
	@brief Sample code of audio 2 captures and 2 outputs with AEC.\n

	@file audio_2bidirect_with_aec.c

	@author HM Tseng

	@ingroup mhdal

	@note This file is modified from audio_capture_only.c and audio_output_only.c.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>

#if defined(__LINUX)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 1

///////////////////////////////////////////////////////////////////////////////

#define BITSTREAM_SIZE      12800
#define FRAME_SAMPLES       1024
#define AUD_BUFFER_CNT      5

#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))

///////////////////////////////////////////////////////////////////////////////

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	/* dummy buffer, not for audio module */
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	/* user buffer for bs pushing in */
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[1].blk_size = 0x100000;
	mem_cfg.pool_info[1].blk_cnt = 2;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return -1;
	}
	return 0;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_cap_ctrl = 0;

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture maximum parameters*/
	audio_cfg_param.in_max.sample_rate = sample_rate;
	audio_cfg_param.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO_PLANAR;
	audio_cfg_param.in_max.frame_sample = 1024;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.out_max.sample_rate = 0;

	/*set audio AEC function parameter*/
	audio_cfg_param.aec_max.enabled = TRUE;
	audio_cfg_param.aec_max.leak_estimate_enabled = FALSE;
	audio_cfg_param.aec_max.leak_estimate_value = 99;
	audio_cfg_param.aec_max.echo_cancel_level = -50;
	audio_cfg_param.aec_max.noise_cancel_level = -30;
	audio_cfg_param.aec_max.filter_length = 1024;
	audio_cfg_param.aec_max.frame_size = 128;
	audio_cfg_param.aec_max.notch_radius = 992;
	audio_cfg_param.aec_max.lb_channel = HD_AUDIOCAP_LB_CH_STEREO;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_driver_cfg_param);

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret;
	HD_AUDIOCAP_IN audio_cap_in_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};
	HD_AUDIOCAP_AEC audio_cap_aec_param = {0};

	// set hd_audiocapture input parameters
	audio_cap_in_param.sample_rate = sample_rate;
	audio_cap_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_in_param.mode = HD_AUDIO_SOUND_MODE_STEREO_PLANAR;
	audio_cap_in_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_in_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	audio_cap_aec_param.enabled = TRUE;
	audio_cap_aec_param.leak_estimate_enabled = FALSE;
	audio_cap_aec_param.leak_estimate_value = 99;
	audio_cap_aec_param.echo_cancel_level = -50;
	audio_cap_aec_param.noise_cancel_level = -30;
	audio_cap_aec_param.filter_length = 1024;
	audio_cap_aec_param.frame_size = 128;
	audio_cap_aec_param.notch_radius = 992;

	if (HD_GET_OUT(audio_cap_path) == HD_OUT(0)) {
		audio_cap_aec_param.lb_channel = HD_AUDIOCAP_LB_CH_LEFT;
	} else {
		audio_cap_aec_param.lb_channel = HD_AUDIOCAP_LB_CH_RIGHT;
	}

	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT_AEC, &audio_cap_aec_param);

	return ret;
}

static HD_RESULT set_out_cfg(HD_CTRL_ID ctrl_id, HD_PATH_ID *p_audio_out_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_out_ctrl = 0;

	ret = hd_audioout_open(0, ctrl_id, &audio_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/* set audio out maximum parameters */
	audio_cfg_param.out_max.sample_rate = sample_rate;
	audio_cfg_param.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.out_max.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_cfg_param.frame_sample_max = 1024;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.in_max.sample_rate = 0;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/* set audio out driver parameters */
	if (ctrl_id == HD_AUDIOOUT_0_CTRL) {
		audio_driver_cfg_param.mono = HD_AUDIO_MONO_LEFT;
	} else {
		audio_driver_cfg_param.mono = HD_AUDIO_MONO_RIGHT;
	}
	audio_driver_cfg_param.output = HD_AUDIOOUT_OUTPUT_SPK;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);

	*p_audio_out_ctrl = audio_out_ctrl;

	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret;
	HD_AUDIOOUT_OUT audio_out_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};
	HD_AUDIOOUT_IN audio_out_in_param = {0};

	// set hd_audioout output parameters
	audio_out_out_param.sample_rate = sample_rate;
	audio_out_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_out_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audioout volume
	audio_out_vol.volume = 100;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if (ret != HD_OK) {
		return ret;
	}


	// set hd_audioout input parameters
	audio_out_in_param.sample_rate = 0;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_IN, &audio_out_in_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_CAP_OUT2 {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;
	HD_PATH_ID cap_path2;

	UINT32 cap_exit;
	UINT32 flow_start;

	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	HD_PATH_ID out_ctrl2;
	HD_PATH_ID out_path2;

	UINT32 out_exit;
	UINT32 out_pause;
} AUDIO_CAP_OUT2;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
		return ret;
	if((ret = hd_audioout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_CAP_OUT2 *p_cap_out_2)
{
	HD_RESULT ret;

	ret = set_cap_cfg(&p_cap_out_2->cap_ctrl, p_cap_out_2->sample_rate_max);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_cap_out_2->cap_path)) != HD_OK)
		return ret;
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_1, &p_cap_out_2->cap_path2)) != HD_OK)
		return ret;

	ret = set_out_cfg(HD_AUDIOOUT_0_CTRL, &p_cap_out_2->out_ctrl, p_cap_out_2->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_cap_out_2->out_path)) != HD_OK)
		return ret;

	ret = set_out_cfg(HD_AUDIOOUT_1_CTRL, &p_cap_out_2->out_ctrl2, p_cap_out_2->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_1_IN_0, HD_AUDIOOUT_1_OUT_0, &p_cap_out_2->out_path2)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(AUDIO_CAP_OUT2 *p_cap_out_2)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_cap_out_2->cap_path)) != HD_OK)
		return ret;
	if((ret = hd_audiocap_close(p_cap_out_2->cap_path2)) != HD_OK)
		return ret;
	if((ret = hd_audioout_close(p_cap_out_2->out_path)) != HD_OK)
		return ret;
	if((ret = hd_audioout_close(p_cap_out_2->out_path2)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	if((ret = hd_audioout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static void *playback_thread(void *arg)
{
	INT ret, bs_size, result;
	CHAR filename[50];
	CHAR filename2[50];
	FILE *bs_fd = 0, *len_fd = 0;
	FILE *bs_fd2 = 0, *len_fd2 = 0;
	HD_AUDIO_FRAME  bs_in_buf = {0};
	HD_COMMON_MEM_VB_BLK blk = 0;
	UINT32 pa = 0, va = 0;
	HD_COMMON_MEM_VB_BLK blk2 = 0;
	UINT32 pa2 = 0, va2 = 0;
	UINT32 blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_buf_start = 0, bs_buf_curr = 0, bs_buf_end = 0;
	UINT32 bs_buf_start2 = 0, bs_buf_curr2 = 0, bs_buf_end2 = 0;
	AUDIO_CAP_OUT2 *p_out_only = (AUDIO_CAP_OUT2 *)arg;
	INT timestamp;

	/* read test pattern */
	snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_pcm_out_p0.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	bs_fd = fopen(filename, "rb");
	if (bs_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		goto play_fclose;
	}
	printf("play file: [%s]\n", filename);

	snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_pcm_out_p0.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	len_fd = fopen(filename, "rb");
	if (len_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename);

	/*read test pattern 2*/

	snprintf(filename2, sizeof(filename2), "/mnt/sd/audio_bs_%d_%d_%d_pcm_out_p1.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	bs_fd2 = fopen(filename2, "rb");
	if (bs_fd2 == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename2);
		goto play_fclose;
	}
	printf("play file: [%s]\n", filename2);

	snprintf(filename2, sizeof(filename2), "/mnt/sd/audio_bs_%d_%d_%d_pcm_out_p1.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	len_fd2 = fopen(filename2, "rb");
	if (len_fd2 == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename2);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename2);

	/* get memory */
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk);
		goto play_fclose;
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("blk2pa fail, blk(0x%x)\n", blk);
		goto play_fclose;
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
		if (va == 0) {
			printf("get va fail, va(0x%x)\n", blk);
			goto play_fclose;
		}
		/* allocate bs buf */
		bs_buf_start = va;
		bs_buf_curr = bs_buf_start;
		bs_buf_end = bs_buf_start + blk_size;
		printf("alloc bs_buf: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n", bs_buf_start, bs_buf_curr, bs_buf_end, blk_size);
	}

	/* get memory */
	blk2 = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk2 == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk2);
		goto play_fclose;
	}
	pa2 = hd_common_mem_blk2pa(blk2); // get physical addr
	if (pa2 == 0) {
		printf("blk2pa fail, blk(0x%x)\n", blk2);
		goto play_fclose;
	}
	if (pa2 > 0) {
		va2 = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa2, blk_size); // Get virtual addr
		if (va2 == 0) {
			printf("get va fail, va(0x%x)\n", blk2);
			goto play_fclose;
		}
		/* allocate bs buf */
		bs_buf_start2 = va2;
		bs_buf_curr2 = bs_buf_start2;
		bs_buf_end2 = bs_buf_start2 + blk_size;
		printf("alloc bs_buf: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n", bs_buf_start2, bs_buf_curr2, bs_buf_end2, blk_size);
	}


	while (1) {
retry:
		if (p_out_only->out_exit == 1) {
			break;
		}

		if (p_out_only->out_pause == 1) {
			usleep(10000);
			goto retry;
		}

		/* get bs size */
		if (fscanf(len_fd, "%d %d\n", &bs_size, &timestamp) == EOF) {
			// reach EOF, read from the beginning
			fseek(bs_fd, 0, SEEK_SET);
			fseek(len_fd, 0, SEEK_SET);
			if (fscanf(len_fd, "%d %d\n", &bs_size, &timestamp) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (bs_size == 0 || bs_size > BITSTREAM_SIZE) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		/* check bs buf rollback */
		if ((bs_buf_curr + bs_size) > bs_buf_end) {
			bs_buf_curr = bs_buf_start;
		}

		/* read bs from file */
		result = fread((void *)bs_buf_curr, 1, bs_size, bs_fd);
		if (result != bs_size) {
			printf("reading error\n");
			continue;
		}
		bs_in_buf.sign = MAKEFOURCC('A','F','R','M');
		bs_in_buf.phy_addr[0] = pa + (bs_buf_curr - bs_buf_start); // needs to add offset
		bs_in_buf.size = bs_size;
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();
		bs_in_buf.bit_width = HD_AUDIO_BIT_WIDTH_16;
		bs_in_buf.sound_mode = HD_AUDIO_SOUND_MODE_MONO;
		bs_in_buf.sample_rate = p_out_only->sample_rate;

resend1:
		ret = hd_audioout_push_in_buf(p_out_only->out_path, &bs_in_buf, -1);
		if (ret != HD_OK) {
			//printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
			usleep(10000);
			goto resend1;
		}

		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next

////////////////////////////////////////////////////////////////////////////////

		/* get bs size */
		if (fscanf(len_fd2, "%d %d\n", &bs_size, &timestamp) == EOF) {
			// reach EOF, read from the beginning
			fseek(bs_fd2, 0, SEEK_SET);
			fseek(len_fd2, 0, SEEK_SET);
			if (fscanf(len_fd2, "%d %d\n", &bs_size, &timestamp) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (bs_size == 0 || bs_size > BITSTREAM_SIZE) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		/* check bs buf rollback */
		if ((bs_buf_curr2 + bs_size) > bs_buf_end2) {
			bs_buf_curr2 = bs_buf_start2;
		}

		/* read bs from file */
		result = fread((void *)bs_buf_curr2, 1, bs_size, bs_fd2);
		if (result != bs_size) {
			printf("reading error\n");
			continue;
		}
		bs_in_buf.sign = MAKEFOURCC('A','F','R','M');
		bs_in_buf.phy_addr[0] = pa2 + (bs_buf_curr2 - bs_buf_start2); // needs to add offset
		bs_in_buf.size = bs_size;
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();
		bs_in_buf.bit_width = HD_AUDIO_BIT_WIDTH_16;
		bs_in_buf.sound_mode = HD_AUDIO_SOUND_MODE_MONO;
		bs_in_buf.sample_rate = p_out_only->sample_rate;
resend2:
		ret = hd_audioout_push_in_buf(p_out_only->out_path2, &bs_in_buf, -1);
		if (ret != HD_OK) {
			//printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
			usleep(10000);
			goto resend2;
		}

		bs_buf_curr2 += ALIGN_CEIL_4(bs_size); // shift to next
	}

	/* release memory */
	hd_common_mem_munmap((void*)va, blk_size);
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		printf("release blk fail, ret(%d)\n", ret);
		goto play_fclose;
	}

	/* release memory */
	hd_common_mem_munmap((void*)va2, blk_size);
	ret = hd_common_mem_release_block(blk2);
	if (HD_OK != ret) {
		printf("release blk fail, ret(%d)\n", ret);
		goto play_fclose;
	}

play_fclose:
	if (bs_fd != NULL) { fclose(bs_fd);}
	if (len_fd != NULL) { fclose(len_fd);}

	if (bs_fd2 != NULL) { fclose(bs_fd2);}
	if (len_fd2 != NULL) { fclose(len_fd2);}

	return 0;
}

static void *capture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	char file_path_main1[64], file_path_len1[64];
	FILE *f_out_main, *f_out_len;
	FILE *f_out_main1, *f_out_len1;
	AUDIO_CAP_OUT2 *p_cap_only = (AUDIO_CAP_OUT2 *)arg;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm_cap_p0.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm_cap_p0.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
	snprintf(file_path_main1, sizeof(file_path_main1), "/mnt/sd/audio_bs_%d_%d_%d_pcm_cap_p1.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);
	snprintf(file_path_len1, sizeof(file_path_len1), "/mnt/sd/audio_bs_%d_%d_%d_pcm_cap_p1.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->sample_rate);

	/* wait flow_start */
	while (p_cap_only->flow_start == 0) {
		sleep(1);
	}

	/* query physical address of bs buffer
	  (this can ONLY query after hd_audiocap_start() is called !!) */
	hd_audiocap_get(p_cap_only->cap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main);

	/* mmap for bs buffer
	  (just mmap one time only, calculate offset to virtual address later) */
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	if (vir_addr_main == 0) {
		printf("mmap error\r\n");
		return 0;
	}

	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}

	/* open output files */
	if ((f_out_main1 = fopen(file_path_main1, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main1);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main1);
	}

	if ((f_out_len1 = fopen(file_path_len1, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len1);
	}

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	/* pull data test */
	while (p_cap_only->cap_exit == 0) {
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path, &data_pull, -1); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);

			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}

		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path2, &data_pull, -1); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write bs
			if (f_out_main1) fwrite(ptr, 1, size, f_out_main1);
			if (f_out_main1) fflush(f_out_main1);

			// write bs len
			if (f_out_len1) fprintf(f_out_len1, "%d %d\n", size, timestamp);
			if (f_out_len1) fflush(f_out_len1);

			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path2, &data_pull);
			if (ret != HD_OK) {
				printf("release buffer failed. ret=%x\r\n", ret);
			}
		}
	}

	/* mummap for bs buffer */
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	/* close output file */
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);

	if (f_out_main1) fclose(f_out_main1);
	if (f_out_len1) fclose(f_out_len1);

	return 0;
}

EXAMFUNC_ENTRY(hd_audio_2bidirect_with_aec, argc, argv)
{
	HD_RESULT ret;
	INT key;
	pthread_t out_thread_id;
	pthread_t cap_thread_id;
	AUDIO_CAP_OUT2 cap_out_2 = {0};

	//init hdal
	ret = hd_common_init(0);
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}
	//output module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	//open output module
	cap_out_2.sample_rate_max = HD_AUDIO_SR_48000; //assign by user
	ret = open_module(&cap_out_2);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audioout parameter
	cap_out_2.sample_rate = HD_AUDIO_SR_16000; //assign by user

	ret = set_cap_param(cap_out_2.cap_path, cap_out_2.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	ret = set_cap_param(cap_out_2.cap_path2, cap_out_2.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	ret = set_out_param(cap_out_2.out_ctrl, cap_out_2.out_path, cap_out_2.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	ret = set_out_param(cap_out_2.out_ctrl2, cap_out_2.out_path2, cap_out_2.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	//create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&cap_out_2);
	if (ret < 0) {
		printf("create record thread failed");
		goto exit;
	}

	//create output thread
	ret = pthread_create(&out_thread_id, NULL, playback_thread, (void *)&cap_out_2);
	if (ret < 0) {
		printf("create playback thread failed");
		goto exit;
	}

	//start capture module
	hd_audiocap_start(cap_out_2.cap_path);
	hd_audiocap_start(cap_out_2.cap_path2);

	cap_out_2.flow_start = 1;

	//start output module
	hd_audioout_start(cap_out_2.out_path);
	hd_audioout_start(cap_out_2.out_path2);

	printf("Enter q to exit\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			cap_out_2.out_exit = 1;
			cap_out_2.cap_exit = 1;
			break;
		}
		if (key == 'p') {
			cap_out_2.out_pause = 1;
		}
		if (key == 'r') {
			cap_out_2.out_pause = 0;
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	pthread_join(out_thread_id, NULL);
	pthread_join(cap_thread_id, NULL);

	//stop capture module
	hd_audiocap_stop(cap_out_2.cap_path);
	hd_audiocap_stop(cap_out_2.cap_path2);

	//stop output module
	hd_audioout_stop(cap_out_2.out_path);
	hd_audioout_stop(cap_out_2.out_path2);

exit:
	//close output module
	ret = close_module(&cap_out_2);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	//uninit output module
	ret = exit_module();
	if(ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
	// uninit memory
	ret = mem_exit();
	if(ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}
	// uninit hdal
	ret = hd_common_uninit();
	if(ret != HD_OK) {
		printf("common-uninit fail=%d\n", ret);
	}

	return 0;
}
