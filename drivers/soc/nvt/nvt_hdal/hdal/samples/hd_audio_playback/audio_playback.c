/**
	@brief Sample code of audio playback.\n

	@file audio_playback.c

	@author Adam SY Su

	@ingroup mhdal

	@note Nothing.

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

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define MAX_BITSTREAM_NUM   1
#define BITSTREAM_SIZE      12800
#define FRAME_SAMPLES       1024
#define AUD_BUFFER_CNT      2

#define TIME_DIFF(new_val, old_val)     (new_val - old_val)

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool for bs pushing in
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[1].blk_size = 0x100000;
	mem_cfg.pool_info[1].blk_cnt = 1;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_dec_cfg(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_PATH_CONFIG audio_path_cfg = {0};

	// set audiodec path config
	audio_path_cfg.max_mem.codec_type = dec_type;
	audio_path_cfg.max_mem.sample_rate = HD_AUDIO_SR_48000;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_PATH_CONFIG, &audio_path_cfg);

	return ret;
}

static HD_RESULT set_dec_param(HD_PATH_ID audio_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIODEC_IN audio_in_param = {0};

	audio_in_param.codec_type = dec_type;
	audio_in_param.sample_rate = HD_AUDIO_SR_48000;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audiodec_set(audio_dec_path, HD_AUDIODEC_PARAM_IN, &audio_in_param);
	if (ret != HD_OK) {
		printf("set_dec_param_in = %d\r\n", ret);
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_audio_out_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_PATH_ID audio_out_ctrl = 0;
	HD_AUDIOOUT_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_drv_cfg = {0};

	ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &audio_out_ctrl); //open this for device control

	if (ret != HD_OK) {
		return ret;
	}

	/* set audio out maximum parameters */
	audio_dev_cfg.out_max.sample_rate = HD_AUDIO_SR_48000;
	audio_dev_cfg.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.out_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_dev_cfg.frame_sample_max = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	/* set audio capture maximum parameters */
	audio_drv_cfg.mono = HD_AUDIO_MONO_LEFT;
	audio_drv_cfg.output = HD_AUDIOOUT_OUTPUT_SPK;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_drv_cfg);

	*p_audio_out_ctrl = audio_out_ctrl;
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_OUT audio_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};

	// set audioout output parameters
	audio_out_param.sample_rate = HD_AUDIO_SR_48000;
	audio_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audioout volume
	audio_out_vol.volume = 100;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_PLAYBACK {

	// (1) audio dec
	HD_PATH_ID dec_path;
	UINT32 dec_type;

	// (2) audio out
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

	// (3) user push
	pthread_t  dec_thread_id;
	UINT32     dec_exit;
	UINT32     flow_start;

} AUDIO_PLAYBACK;

static HD_RESULT init_module(void)
{
    HD_RESULT ret;
	if((ret = hd_audiodec_init()) != HD_OK)
        return ret;
    if((ret = hd_audioout_init()) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT open_module(AUDIO_PLAYBACK *p_stream)
{
	HD_RESULT ret;
	// set audioout config
	ret = set_out_cfg(&p_stream->out_ctrl);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiodec_open(HD_AUDIODEC_0_IN_0, HD_AUDIODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK)
        return ret;
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
        return ret;

    return HD_OK;
}

static HD_RESULT close_module(AUDIO_PLAYBACK *p_stream)
{
    HD_RESULT ret;
	if((ret = hd_audiodec_close(p_stream->dec_path)) != HD_OK)
        return ret;
	if((ret = hd_audioout_close(p_stream->out_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT exit_module(void)
{
    HD_RESULT ret;
	if((ret = hd_audiodec_uninit()) != HD_OK)
        return ret;
    if((ret = hd_audioout_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

static void *playback_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	CHAR filename[50], codec_name[8] = {0};
	FILE *bs_fd, *len_fd;
	HD_AUDIO_BS  bs_in_buf = {0};
	AUDIO_PLAYBACK* p_stream0 = (AUDIO_PLAYBACK *)arg;
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	INT bs_size, result, timestamp;
	UINT64 au_frame_us, elapse_time, start_time, data_time;
	UINT32 pa, va, bs_buf_start, bs_buf_curr, bs_buf_end;
	UINT32 blk_size = 0x100000;

	// assign audio info
	HD_AUDIO_SR sample_rate = HD_AUDIO_SR_48000;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = HD_AUDIO_SOUND_MODE_STEREO;
	switch (p_stream0->dec_type) {
		case HD_AUDIO_CODEC_AAC:
			snprintf(codec_name, sizeof(codec_name), "aac");
			break;
		case HD_AUDIO_CODEC_ULAW:
			snprintf(codec_name, sizeof(codec_name), "g711u");
			break;
		case HD_AUDIO_CODEC_ALAW:
			snprintf(codec_name, sizeof(codec_name), "g711a");
			break;
		case HD_AUDIO_CODEC_PCM:
			snprintf(codec_name, sizeof(codec_name), "pcm");
			break;
		default:
			printf("invalid audio codec(%d)\n", p_stream0->dec_type);
			break;
	}

	// config pattern name
	if (p_stream0->dec_type == HD_AUDIO_CODEC_PCM) {
		snprintf(filename, sizeof(filename), "/mnt/sd/audio_frm_%d_%d_%d_%s.dat", sample_bit, mode, sample_rate, codec_name);
	} else {
		snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_%s.dat", sample_bit, mode, sample_rate, codec_name);
	}
	bs_fd = fopen(filename, "rb");
	if (bs_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		return 0;
	}
	printf("play file: [%s]\n", filename);

	if (p_stream0->dec_type == HD_AUDIO_CODEC_PCM) {
		snprintf(filename, sizeof(filename), "/mnt/sd/audio_frm_%d_%d_%d_%s.len", sample_bit, mode, sample_rate, codec_name);
	} else {
		snprintf(filename, sizeof(filename), "/mnt/sd/audio_bs_%d_%d_%d_%s.len", sample_bit, mode, sample_rate, codec_name);
	}
	len_fd = fopen(filename, "rb");
	if (len_fd == NULL) {
		printf("[ERROR] Open %s failed!!\n", filename);
		goto play_fclose;
	}
	printf("len file: [%s]\n", filename);

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk);
		goto play_fclose;
	}
	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("blk2pa fail, blk(0x%x)\n", blk);
		goto rel_blk;
	}
	if (pa > 0) {
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
		if (va == 0) {
			printf("get va fail, va(0x%x)\n", blk);
			goto rel_blk;
		}
		// allocate bs buf
		bs_buf_start = va;
		bs_buf_curr = bs_buf_start;
		bs_buf_end = bs_buf_start + blk_size;
		printf("alloc bs_buf: start(0x%x) curr(0x%x) end(0x%x) size(0x%x)\n", bs_buf_start, bs_buf_curr, bs_buf_end, blk_size);
	}

	au_frame_us = FRAME_SAMPLES * 1000000 / sample_rate; // the time(in us) of each audio frame
	start_time = hd_gettime_us();
	data_time = 0;

	while (1) {
retry:
		if (p_stream0->dec_exit == 1) {
			break;
		}

		elapse_time = TIME_DIFF(hd_gettime_us(), start_time);

		if (((INT64)data_time - (INT64)elapse_time) > 10000 && data_time != 0) {
			usleep(9000);
			goto retry;
		}

		// get bs size
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

		// check bs buf rollback
		if ((bs_buf_curr + bs_size) > bs_buf_end) {
			bs_buf_curr = bs_buf_start;
		}

		// read bs from file
		result = fread((void *)bs_buf_curr, 1, bs_size, bs_fd);
		if (result != bs_size) {
			printf("reading error\n");
			continue;
		}
		bs_in_buf.sign = MAKEFOURCC('A','S','T','M');
		bs_in_buf.acodec_format = p_stream0->dec_type;
		bs_in_buf.phy_addr = pa + (bs_buf_curr - bs_buf_start); // needs to add offset
		bs_in_buf.size = bs_size;
		bs_in_buf.ddr_id = ddr_id;
		bs_in_buf.timestamp = hd_gettime_us();

		// push in buffer
		data_time += au_frame_us;
		ret = hd_audiodec_push_in_buf(p_stream0->dec_path, &bs_in_buf, NULL, 0); // only support non-blocking mode now
		if (ret != HD_OK) {
			printf("hd_audiodec_push_in_buf fail, ret(%d)\n", ret);
		}

		bs_buf_curr += ALIGN_CEIL_4(bs_size); // shift to next
	}

	// release memory
	hd_common_mem_munmap((void*)va, blk_size);
rel_blk:
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

EXAMFUNC_ENTRY(hd_audio_playback, argc, argv)
{
    HD_RESULT ret;
    INT key;
	AUDIO_PLAYBACK stream[1] = {0}; //0: main stream
	UINT32 dec_type = 0;

	// query program options
	if (argc == 2) {
		dec_type = atoi(argv[1]);
		printf("dec_type %d\r\n", dec_type);
		if(stream[0].dec_type > 2) {
			printf("error: not support dec_type %d\r\n", dec_type);
			return 0;
		}
	}

	// init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common-init fail=%d\n", ret);
        goto exit;
    }

	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

    // open audio_playback modules
    ret = open_module(&stream[0]);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
        goto exit;
    }

	// query decode type
	if (dec_type == 0) {
		stream[0].dec_type = HD_AUDIO_CODEC_AAC;
	} else if (dec_type == 1) {
		stream[0].dec_type = HD_AUDIO_CODEC_ULAW;
	} else if (dec_type == 2) {
		stream[0].dec_type = HD_AUDIO_CODEC_ALAW;
	} else {
		stream[0].dec_type = HD_AUDIO_CODEC_PCM;
	}

	// set audiodec config
	ret = set_dec_cfg(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec-cfg fail=%d\n", ret);
		goto exit;
	}

	// set audiodec parameter
    ret = set_dec_param(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec fail=%d\n", ret);
		goto exit;
	}

	// set audioout parameter
    ret = set_out_param(stream[0].out_ctrl, stream[0].out_path);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

    // bind audio_playback modules
    hd_audiodec_bind(HD_AUDIODEC_0_OUT_0, HD_AUDIOOUT_0_IN_0);

	// start audio_playback modules
    hd_audiodec_start(stream[0].dec_path);
	hd_audioout_start(stream[0].out_path);

	// create decode_thread (push_in bitstream)
	ret = pthread_create(&stream[0].dec_thread_id, NULL, playback_thread, (void *)stream);
	if (ret < 0) {
		printf("create playback thread failed");
		return -1;
	}

	// query user key
	printf("Enter q to exit, Enter d to debug\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let decode_thread stop loop and exit
			stream[0].dec_exit = 1;
			// quit program
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	// destroy decode thread
	pthread_join(stream[0].dec_thread_id, NULL);

	// stop audio_playback modules
	hd_audiodec_stop(stream[0].dec_path);
	hd_audioout_stop(stream[0].out_path);

	// unbind audio_playback modules
	hd_audiodec_unbind(HD_AUDIODEC_0_OUT_0);

exit:
	// close audio_playback modules
	ret = close_module(&stream[0]);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if(ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        printf("common-uninit fail=%d\n", ret);
    }

	return 0;
}
