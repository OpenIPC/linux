/**
	@brief Sample code of audio capture with anr.\n

	@file audio_capture_only_with_anr.c

	@author HM Tseng

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
#endif

#define CHKPNT  printf("\033[37mCHK: %s, %s: %d\033[0m",__FILE__,__func__,__LINE__)
#define DEBUG_MENU 1

///////////////////////////////////////////////////////////////////////////////

static int mem_init(void)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	/*dummy buffer, not for audio module*/
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;

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
	audio_cfg_param.in_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.in_max.frame_sample = 1024;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.out_max.sample_rate = 0;

	/*set audio ANR function parameter*/
	audio_cfg_param.anr_max.enabled = TRUE;
	audio_cfg_param.anr_max.suppress_level = 13;
	audio_cfg_param.anr_max.hpf_cut_off_freq = 200;
	audio_cfg_param.anr_max.bias_sensitive= 9;
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

static HD_RESULT set_cap_param(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate, HD_AUDIO_SOUND_MODE mode)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_in_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};
	HD_AUDIOCAP_ANR audio_cap_anr_param = {0};

	// set hd_audiocapture input parameters
	audio_cap_in_param.sample_rate = sample_rate;
	audio_cap_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_in_param.mode = mode;
	audio_cap_in_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_in_param);
	if (ret != HD_OK) {
		return ret;
	}

	audio_cap_anr_param.enabled = TRUE;
	audio_cap_anr_param.suppress_level = 13;
	audio_cap_anr_param.hpf_cut_off_freq = 200;
	audio_cap_anr_param.bias_sensitive = 9;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT_ANR, &audio_cap_anr_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_CAPONLY {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;
	HD_AUDIO_SOUND_MODE mode;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	UINT32 cap_exit;
	UINT32 flow_start;
	UINT32 flow_test;
} AUDIO_CAPONLY;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
		return ret;

	return HD_OK;
}


static HD_RESULT open_module(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	ret = set_cap_cfg(&p_caponly->cap_ctrl, p_caponly->sample_rate_max);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(AUDIO_CAPONLY *p_caponly)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_caponly->cap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static void *capture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
	AUDIO_CAPONLY *p_cap_only = (AUDIO_CAPONLY *)arg;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, p_cap_only->mode, p_cap_only->sample_rate);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, p_cap_only->mode, p_cap_only->sample_rate);

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

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	/* pull data test */
	while (p_cap_only->cap_exit == 0) {
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->cap_path, &data_pull, 200); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr[0]);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();

			if (!p_cap_only->flow_test) {
				// write bs
				if (f_out_main) fwrite(ptr, 1, size, f_out_main);
				if (f_out_main) fflush(f_out_main);

				// write bs len
				if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
				if (f_out_len) fflush(f_out_len);
			}

			// release data
			ret = hd_audiocap_release_out_buf(p_cap_only->cap_path, &data_pull);
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

	return 0;
}


EXAMFUNC_ENTRY(hd_audio_capture_only_with_anr, argc, argv)
{
	HD_RESULT ret;
	INT key;
	AUDIO_CAPONLY caponly = {0};
	pthread_t cap_thread_id;

	// query program options
	if (argc == 3) {
		UINT32 sample_rate;
		UINT32 mode;

		sample_rate = atoi(argv[1]);
		mode = atoi(argv[2]);

		if (sample_rate == 0) {
			caponly.sample_rate = HD_AUDIO_SR_8000;
		} else if (sample_rate == 1) {
			caponly.sample_rate = HD_AUDIO_SR_16000;
		} else if (sample_rate == 2) {
			caponly.sample_rate = HD_AUDIO_SR_32000;
		} else if (sample_rate == 3) {
			caponly.sample_rate = HD_AUDIO_SR_48000;
		} else {
			caponly.sample_rate = HD_AUDIO_SR_16000;
		}

		if (mode == 0) {
			caponly.mode= HD_AUDIO_SOUND_MODE_MONO;
		} else if (mode == 1) {
			caponly.mode = HD_AUDIO_SOUND_MODE_STEREO;
		} else {
			caponly.mode = HD_AUDIO_SOUND_MODE_MONO;
		}

		printf("sample rate %d, mode %d\r\n", caponly.sample_rate, caponly.mode);
	} else if (argc == 4) {
		UINT32 sample_rate;
		UINT32 mode;

		sample_rate = atoi(argv[1]);
		mode = atoi(argv[2]);

		if (sample_rate == 0) {
			caponly.sample_rate = HD_AUDIO_SR_8000;
		} else if (sample_rate == 1) {
			caponly.sample_rate = HD_AUDIO_SR_16000;
		} else if (sample_rate == 2) {
			caponly.sample_rate = HD_AUDIO_SR_32000;
		} else if (sample_rate == 3) {
			caponly.sample_rate = HD_AUDIO_SR_48000;
		} else {
			caponly.sample_rate = HD_AUDIO_SR_16000;
		}

		if (mode == 0) {
			caponly.mode= HD_AUDIO_SOUND_MODE_MONO;
		} else if (mode == 1) {
			caponly.mode = HD_AUDIO_SOUND_MODE_STEREO;
		} else {
			caponly.mode = HD_AUDIO_SOUND_MODE_MONO;
		}
		caponly.flow_test = atoi(argv[3]);
		printf("sample rate %d, mode %d, flow_test %d\r\n", caponly.sample_rate, caponly.mode, caponly.flow_test);
	} else {
		caponly.sample_rate = HD_AUDIO_SR_16000;
		caponly.mode = HD_AUDIO_SOUND_MODE_MONO;
	}

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
	//capture module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	//open capture module
	caponly.sample_rate_max = HD_AUDIO_SR_48000; //assign by user
	ret = open_module(&caponly);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audiocap parameter
	//caponly.sample_rate = HD_AUDIO_SR_16000; //assign by user
	ret = set_cap_param(caponly.cap_path, caponly.sample_rate, caponly.mode);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
	//create capture thread
	ret = pthread_create(&cap_thread_id, NULL, capture_thread, (void *)&caponly);
	if (ret < 0) {
		printf("create encode thread failed");
		goto exit;
	}

	//start capture module
	hd_audiocap_start(caponly.cap_path);

	caponly.flow_start = 1;

	printf("Enter q to exit, Enter d to debug\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			caponly.cap_exit = 1;
			break;
		}
		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}

	pthread_join(cap_thread_id, NULL);

	//stop capture module
	hd_audiocap_stop(caponly.cap_path);

exit:
	//close all module
	ret = close_module(&caponly);
	if(ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	//uninit all module
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
