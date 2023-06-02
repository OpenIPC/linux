/**
	@brief Sample code of audio livesound.\n

	@file audio_livesound.c

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

#define DEBUG_MENU 1

#define CHKPNT    printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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
	HD_RESULT ret = HD_OK;
	//set hd_audiocapture input parameters
	HD_AUDIOCAP_IN audio_cap_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};

	audio_cap_param.sample_rate = sample_rate;
	audio_cap_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cap_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_param);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_audio_out_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOOUT_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOOUT_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_PATH_ID audio_out_ctrl = 0;

	ret = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &audio_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio out maximum parameters*/
	audio_cfg_param.out_max.sample_rate = sample_rate;
	audio_cfg_param.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cfg_param.out_max.mode = HD_AUDIO_SOUND_MODE_STEREO;
	audio_cfg_param.frame_sample_max = 1024;
	audio_cfg_param.frame_num_max = 10;
	audio_cfg_param.in_max.sample_rate = 0;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio out driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_LEFT;
	audio_driver_cfg_param.output = HD_AUDIOOUT_OUTPUT_SPK;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_DRV_CONFIG, &audio_driver_cfg_param);

	*p_audio_out_ctrl = audio_out_ctrl;

	return ret;
}


static HD_RESULT set_out_param(HD_PATH_ID audio_out_ctrl, HD_PATH_ID audio_out_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	//set hd_audioout output parameters
	HD_AUDIOOUT_OUT audio_out_out_param = {0};
	HD_AUDIOOUT_VOLUME audio_out_vol = {0};
	HD_AUDIOOUT_IN audio_out_in_param = {0};

	audio_out_out_param.sample_rate = sample_rate;
	audio_out_out_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_out_out_param.mode = HD_AUDIO_SOUND_MODE_STEREO;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_OUT, &audio_out_out_param);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audioout volume
	audio_out_vol.volume = 50;
	ret = hd_audioout_set(audio_out_ctrl, HD_AUDIOOUT_PARAM_VOLUME, &audio_out_vol);
	if (ret != HD_OK) {
		return ret;
	}

	//set hd_audioout input parameters
	audio_out_in_param.sample_rate = 0;
	ret = hd_audioout_set(audio_out_path, HD_AUDIOOUT_PARAM_IN, &audio_out_in_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _AUDIO_LIVESOUND {
	HD_AUDIO_SR sample_rate_max;
	HD_AUDIO_SR sample_rate;

	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
} AUDIO_LIVESOUND;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_init()) != HD_OK)
		return ret;
	if((ret = hd_audioout_init()) != HD_OK)
		return ret;
	return HD_OK;
}


static HD_RESULT open_module(AUDIO_LIVESOUND *p_livesound)
{
	HD_RESULT ret;
	ret = set_cap_cfg(&p_livesound->cap_ctrl, p_livesound->sample_rate_max);
	if (ret != HD_OK) {
		printf("set cap-cfg fail\n");
		return HD_ERR_NG;
	}
	ret = set_out_cfg(&p_livesound->out_ctrl, p_livesound->sample_rate_max);
	if (ret != HD_OK) {
		printf("set out-cfg fail\n");
		return HD_ERR_NG;
	}
	if((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_livesound->cap_path)) != HD_OK)
		return ret;
	if((ret = hd_audioout_open(HD_AUDIOOUT_0_IN_0, HD_AUDIOOUT_0_OUT_0, &p_livesound->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(AUDIO_LIVESOUND *p_livesound)
{
	HD_RESULT ret;
	if((ret = hd_audiocap_close(p_livesound->cap_path)) != HD_OK)
		return ret;
	if((ret = hd_audioout_close(p_livesound->out_path)) != HD_OK)
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

EXAMFUNC_ENTRY(hd_audio_livesound, argc, argv)
{
	HD_RESULT ret;
	INT key;
	AUDIO_LIVESOUND livesound = {0};

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
	//livesound module init
	ret = init_module();
	if(ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}
	//open livesound module
	livesound.sample_rate_max = HD_AUDIO_SR_48000; //assign by user
	ret = open_module(&livesound);
	if(ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	//set audiocap parameter
	livesound.sample_rate = HD_AUDIO_SR_48000; //assign by user
	ret = set_cap_param(livesound.cap_path, livesound.sample_rate);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}
	//set audioout parameter
	ret = set_out_param(livesound.out_ctrl, livesound.out_path, livesound.sample_rate);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	//bind livesound module
	hd_audiocap_bind(HD_AUDIOCAP_0_OUT_0, HD_AUDIOOUT_0_IN_0);

	//start livesound module
	hd_audiocap_start(livesound.cap_path);
	hd_audioout_start(livesound.out_path);

	printf("\r\nEnter q to exit, Enter d to debug\r\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			hd_debug_run_menu(); // call debug menu
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
	}
	//stop livesound module
	hd_audiocap_stop(livesound.cap_path);
	hd_audioout_stop(livesound.out_path);
	//unbind livesound module
	hd_audiocap_unbind(HD_AUDIOCAP_0_OUT_0);

exit:
	//close all module
	ret = close_module(&livesound);
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
