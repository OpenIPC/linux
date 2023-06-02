/**
	@brief Sample code of audio record.\n

	@file audio_record.c

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

#define AUD_CHANNEL_COUNT(mode)	(mode == HD_AUDIO_SOUND_MODE_MONO) ? 1 : 2

#define AUD_COMPRESSION_RATIO(codec)	(codec == HD_AUDIO_CODEC_PCM) ? 1 : 2

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

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_audio_cap_ctrl)
{
	HD_RESULT ret;
	HD_PATH_ID audio_cap_ctrl = 0;
	HD_AUDIOCAP_DEV_CONFIG audio_dev_cfg = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_drv_cfg = {0};

	ret = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &audio_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap dev parameter
	audio_dev_cfg.in_max.sample_rate = HD_AUDIO_SR_48000;
	audio_dev_cfg.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_dev_cfg.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_dev_cfg.in_max.frame_sample = 1024;
	audio_dev_cfg.frame_num_max = 10;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_dev_cfg);
	if (ret != HD_OK) {
		return ret;
	}

	// set audiocap drv parameter
	audio_drv_cfg.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_drv_cfg);

	*p_audio_cap_ctrl = audio_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID audio_cap_path)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_param = {0};

	// set audiocap input parameter
	audio_cap_param.sample_rate = HD_AUDIO_SR_48000;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cap_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_enc_cfg(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_PATH_CONFIG audio_path_cfg = {0};

	// set audioenc path config
	audio_path_cfg.max_mem.codec_type = enc_type;
	audio_path_cfg.max_mem.sample_rate = HD_AUDIO_SR_48000;
	audio_path_cfg.max_mem.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_path_cfg.max_mem.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_PATH_CONFIG, &audio_path_cfg);

	return ret;
}

static HD_RESULT set_enc_param(HD_PATH_ID audio_enc_path, UINT32 enc_type)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_IN audio_in_param = {0};
	HD_AUDIOENC_OUT audio_out_param = {0};

	// set audioenc input parameter
	audio_in_param.sample_rate = HD_AUDIO_SR_48000;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_IN, &audio_in_param);
	if (ret != HD_OK) {
		printf("set_enc_param_in = %d\r\n", ret);
		return ret;
	}

	// set audioenc output parameter
	audio_out_param.codec_type = enc_type;
	audio_out_param.aac_adts = (enc_type == HD_AUDIO_CODEC_AAC) ? TRUE : FALSE;
	ret = hd_audioenc_set(audio_enc_path, HD_AUDIOENC_PARAM_OUT, &audio_out_param);
	if (ret != HD_OK) {
		printf("set_enc_param_out = %d\r\n", ret);
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_RECORD {

	// (1) audio cap
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	// (2) audio enc
	HD_PATH_ID enc_path;
	UINT32 enc_type;

	// (3) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

} AUDIO_RECORD;

static HD_RESULT init_module(void)
{
    HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
        return ret;
    if((ret = hd_audioenc_init()) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT open_module(AUDIO_RECORD *p_stream)
{
	HD_RESULT ret;
	// set audiocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\r\n");
		return HD_ERR_NG;
	}

	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
        return ret;
	if((ret = hd_audioenc_open(HD_AUDIOENC_0_IN_0, HD_AUDIOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
        return ret;

	return HD_OK;
}

static HD_RESULT close_module(AUDIO_RECORD *p_stream)
{
    HD_RESULT ret;
	if((ret = hd_audiocap_close(p_stream->cap_path)) != HD_OK)
        return ret;
	if((ret = hd_audioenc_close(p_stream->enc_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT exit_module(void)
{
    HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
        return ret;
    if((ret = hd_audioenc_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

static void *encode_thread(void *arg)
{
	AUDIO_RECORD* p_stream0 = (AUDIO_RECORD *)arg;
	HD_RESULT ret = HD_OK;
	HD_AUDIO_BS  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIOENC_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64], codec_name[8];
	FILE *f_out_main, *f_out_len;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	// wait flow_start
	while (p_stream0->flow_start == 0) {
		sleep(1);
	}

	// assign audio info
	HD_AUDIO_SR sample_rate = HD_AUDIO_SR_48000;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = HD_AUDIO_SOUND_MODE_STEREO;
	switch (p_stream0->enc_type) {
		case HD_AUDIO_CODEC_AAC:
			snprintf(codec_name, sizeof(codec_name), "aac");
			break;
		case HD_AUDIO_CODEC_ULAW:
			snprintf(codec_name, sizeof(codec_name), "g711u");
			break;
		case HD_AUDIO_CODEC_ALAW:
			snprintf(codec_name, sizeof(codec_name), "g711a");
			break;
		default:
			printf("invalid audio codec(%d)\n", p_stream0->enc_type);
			break;
	}

	// config pattern name
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/dump_bs_%d_%d_%d_%s.dat", sample_bit, mode, sample_rate, codec_name);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/dump_bs_%d_%d_%d_%s.len", sample_bit, mode, sample_rate, codec_name);

	// query physical address of bs buffer (this can ONLY query after hd_audioenc_start() is called !!)
	hd_audioenc_get(p_stream0->enc_path, HD_AUDIOENC_PARAM_BUFINFO, &phy_buf_main);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	// open output files
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	// pull bs data
	while (p_stream0->enc_exit == 0) {
		// pull data
		ret = hd_audioenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();
			// write bs
			if (f_out_main) fwrite(ptr, 1, size, f_out_main);
			if (f_out_main) fflush(f_out_main);

			// write bs len
			if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
			if (f_out_len) fflush(f_out_len);

			// release data
			ret = hd_audioenc_release_out_buf(p_stream0->enc_path, &data_pull);
			if (ret != HD_OK) {
				printf("release_ouf_buf fail, ret(%d)\r\n", ret);
			}
		}
	}

	// mummap for bs buffer
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);

	// close output file
	if (f_out_main) fclose(f_out_main);
	if (f_out_len) fclose(f_out_len);

	return 0;
}

EXAMFUNC_ENTRY(hd_audio_record, argc, argv)
{
	HD_RESULT ret;
    INT key;
	AUDIO_RECORD stream[1] = {0}; //0: main stream
	UINT32 enc_type = 0;

	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(stream[0].enc_type > 2) {
			printf("error: not support enc_type %d\r\n", enc_type);
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

	// open audio_record modules
	ret = open_module(&stream[0]);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
        goto exit;
    }

	// set audiocap parameter
	ret = set_cap_param(stream[0].cap_path);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// query encode type
	if (enc_type == 0) {
		stream[0].enc_type = HD_AUDIO_CODEC_AAC;
	} else if (enc_type == 1) {
		stream[0].enc_type = HD_AUDIO_CODEC_ULAW;
	} else {
		stream[0].enc_type = HD_AUDIO_CODEC_ALAW;
	}

	// set audioenc config
	ret = set_enc_cfg(stream[0].enc_path, stream[0].enc_type);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set audioenc paramter
	ret = set_enc_param(stream[0].enc_path, stream[0].enc_type);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// bind audio_record modules
	hd_audiocap_bind(HD_AUDIOCAP_0_OUT_0, HD_AUDIOENC_0_IN_0);

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed\n");
		goto exit;
	}

	// start audio_record modules
	hd_audioenc_start(stream[0].enc_path);
	hd_audiocap_start(stream[0].cap_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;

	// query user key
	printf("Enter q to exit, Enter d to debug\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;
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

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);

	// stop audio_record modules
	hd_audiocap_stop(stream[0].cap_path);
	hd_audioenc_stop(stream[0].enc_path);

	// unbind audio_record modules
	hd_audiocap_unbind(HD_AUDIOCAP_0_OUT_0);

exit:
	// close audio_record modules
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
