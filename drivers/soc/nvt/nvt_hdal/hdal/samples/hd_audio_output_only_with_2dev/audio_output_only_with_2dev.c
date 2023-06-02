/**
	@brief Sample code of audio output with 2 audioout.\n

	@file audio_output_only_with_2out.c

	@author HM Tseng

	@ingroup mhdal

	@note This file is modified from audio_output_only.c.

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
	mem_cfg.pool_info[1].blk_cnt = 1;
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
	audio_driver_cfg_param.output = HD_AUDIOOUT_OUTPUT_LINE;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);

	*p_audio_out_ctrl = audio_out_ctrl;

	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
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
	audio_out_vol.volume = 50;
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

typedef struct _AUDIO_OUTONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	HD_PATH_ID out_ctrl2;
	HD_PATH_ID out_path2;

	UINT32 out_exit;
	UINT32 out_pause;
} AUDIO_OUTONLY;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audioout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_OUTONLY *p_outonly)
{
	HD_RESULT ret;
	ret = set_out_cfg(HD_AUDIOOUT_0_CTRL, &p_outonly->out_ctrl, p_outonly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_outonly->out_path)) != HD_OK)
		return ret;

	ret = set_out_cfg(HD_AUDIOOUT_1_CTRL, &p_outonly->out_ctrl2, p_outonly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audioout_open(HD_AUDIOOUT_1_IN_0, HD_AUDIOOUT_1_OUT_0, &p_outonly->out_path2)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(AUDIO_OUTONLY *p_outonly)
{
	HD_RESULT ret;
	if((ret = hd_audioout_close(p_outonly->out_path)) != HD_OK)
		return ret;
	if((ret = hd_audioout_close(p_outonly->out_path2)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audioout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static void *playback_thread(void *arg)
{
	INT ret, bs_size, result;
	CHAR filename[50];
	FILE *bs_fd, *len_fd;
	HD_AUDIO_FRAME  bs_in_buf = {0};
	HD_COMMON_MEM_VB_BLK blk;
	UINT32 pa, va;
	UINT32 blk_size = 0x100000;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 bs_buf_start, bs_buf_curr, bs_buf_end;
	INT au_frame_ms, elapse_time, au_buf_time, timestamp;
	UINT start_time, data_time;
	AUDIO_OUTONLY *p_out_only = (AUDIO_OUTONLY *)arg;

	/* read test pattern */
	snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	bs_fd = fopen(filename, "rb");
	if (bs_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		return 0;
	}
	printf("play file: [%s]\n", filename);

	snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_out_only->sample_rate);
	len_fd = fopen(filename, "rb");
	if (len_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename);

	au_frame_ms = FRAME_SAMPLES * 1000 / p_out_only->sample_rate - 5; // the time(in ms) of each audio frame
	start_time = hd_gettime_ms();
	data_time = 0;

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

	while (1) {
retry:
		if (p_out_only->out_exit == 1) {
			break;
		}

		if (p_out_only->out_pause == 1) {
			usleep(10000);
			goto retry;
		}

		elapse_time = TIME_DIFF(hd_gettime_ms(), start_time);
		au_buf_time = data_time - elapse_time;
		if (au_buf_time > AUD_BUFFER_CNT * au_frame_ms) {
			//usleep(au_frame_ms);
			//goto retry;
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

		/* push in buffer */
		data_time += au_frame_ms;
resend1:
		ret = hd_audioout_push_in_buf(p_out_only->out_path, &bs_in_buf, -1);
		if (ret != HD_OK) {
			//printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
			usleep(10000);
			goto resend1;
		}

resend2:
		ret = hd_audioout_push_in_buf(p_out_only->out_path2, &bs_in_buf, -1);
		if (ret != HD_OK) {
			//printf("hd_audioout_push_in_buf fail, ret(%d)\n", ret);
			usleep(10000);
			goto resend2;
		}

		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next
	}

	/* release memory */
	hd_common_mem_munmap((void*)va, blk_size);
	ret = hd_common_mem_release_block(blk);
	if (HD_OK != ret) {
		printf("release blk fail, ret(%d)\n", ret);
		goto play_fclose;
	}

play_fclose:
	if (bs_fd != NULL) { fclose(bs_fd);}
	if (len_fd != NULL) { fclose(len_fd);}

	return 0;
}

EXAMFUNC_ENTRY(hd_audio_output_only_with_2dev, argc, argv)
{
	HD_RESULT ret;
	INT key;
	pthread_t out_thread_id;
	AUDIO_OUTONLY outonly = {0};

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
	outonly.sample_rate_max = HD_AUDIO_SR_48000; //assign by user
	ret = open_module(&outonly);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audioout parameter
	outonly.sample_rate = HD_AUDIO_SR_48000; //assign by user
	ret = set_out_param(outonly.out_ctrl, outonly.out_path, outonly.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	ret = set_out_param(outonly.out_ctrl2, outonly.out_path2, outonly.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	//create output thread
	ret = pthread_create(&out_thread_id, NULL, playback_thread, (void *)&outonly);
	if (ret < 0) {
		printf("create playback thread failed");
		goto exit;
	}

	//start output module
	hd_audioout_start(outonly.out_path);
	hd_audioout_start(outonly.out_path2);

	printf("Enter q to exit\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			outonly.out_exit = 1;
			break;
		}
		if (key == 'p') {
			outonly.out_pause = 1;
		}
		if (key == 'r') {
			outonly.out_pause = 0;
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	pthread_join(out_thread_id, NULL);

	//stop output module
	hd_audioout_stop(outonly.out_path);
	hd_audioout_stop(outonly.out_path2);

exit:
	//close output module
	ret = close_module(&outonly);
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
