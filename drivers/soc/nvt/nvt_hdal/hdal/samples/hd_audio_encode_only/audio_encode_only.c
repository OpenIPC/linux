/**
	@brief Sample code of audio encode only.\n

	@file audio_encode_only.c

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

#define MAX_FRM_BLK_CNT    3
#define MAX_FRM_BLK_SIZE   0x2000

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (main)
	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = MAX_FRM_BLK_SIZE;
	mem_cfg.pool_info[0].blk_cnt  = MAX_FRM_BLK_CNT;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;

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

static HD_RESULT set_enc_param(HD_PATH_ID audio_enc_path, UINT32 enc_type, HD_AUDIO_SR sample_rate, HD_AUDIO_SOUND_MODE mode)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOENC_IN audio_in_param = {0};
	HD_AUDIOENC_OUT audio_out_param = {0};

	// set audioenc input parameter
	audio_in_param.sample_rate = sample_rate;
	audio_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_in_param.mode = mode;
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

typedef struct _AUDIO_ENCODE {

	// (1) audio enc
	HD_PATH_ID enc_path;
	UINT32 enc_type;
	UINT32 flow_start;

	// (2) user push
	pthread_t  feed_thread_id;
	UINT32     feed_exit;

	// (3) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;

	HD_AUDIO_SR sample_rate;
	HD_AUDIO_SOUND_MODE mode;
	BOOL flow_test;

} AUDIO_ENCODE;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if((ret = hd_audioenc_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(AUDIO_ENCODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_audioenc_open(HD_AUDIOENC_0_IN_0, HD_AUDIOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT close_module(AUDIO_ENCODE *p_stream)
{
    HD_RESULT ret;
    if((ret = hd_audioenc_close(p_stream->enc_path)) != HD_OK)
        return ret;
    return HD_OK;
}

static HD_RESULT exit_module(void)
{
    HD_RESULT ret;
    if((ret = hd_audioenc_uninit()) != HD_OK)
        return ret;
    return HD_OK;
}

static INT32 blk2idx(HD_COMMON_MEM_VB_BLK blk)   // convert blk(0xXXXXXXXX) to index (0, 1, 2)
{
	static HD_COMMON_MEM_VB_BLK blk_registered[MAX_FRM_BLK_CNT] = {0};
	INT32 i;
	for (i=0; i< MAX_FRM_BLK_CNT; i++) {
		if (blk_registered[i] == blk) return i;

		if (blk_registered[i] == 0) {
			blk_registered[i] = blk;
			return i;
		}
	}

	printf("convert blk(%0x%x) to index fail !!!!\r\n", blk);
	return (-1);
}

static void *feed_frm_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	int i;
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	AUDIO_ENCODE* p_stream0 = (AUDIO_ENCODE *)arg;
	FILE *frm_fd, *len_fd;
	char file_path[128];
	INT frm_size, result, au_frame_ms = 0, timestamp;
	INT32 blk_idx;
	UINT32 blk_size = MAX_FRM_BLK_SIZE;
	UINT32 pa[MAX_FRM_BLK_CNT] = {0}, va[MAX_FRM_BLK_CNT] = {0};


	// wait flow_start
	while (p_stream0->flow_start == 0) {
		sleep(1);
	}

	// assign audio info
	HD_AUDIO_SR sample_rate = p_stream0->sample_rate;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = p_stream0->mode;

	// open input files
	snprintf(file_path, sizeof(file_path), "/mnt/sd/audio_frm_%d_%d_%d_pcm.dat", sample_bit, mode, sample_rate);
	if ((frm_fd = fopen(file_path, "rb")) == NULL) {
		printf("open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", file_path);
		return 0;
	}
	printf("input file: [%s]\n", file_path);

	snprintf(file_path, sizeof(file_path), "/mnt/sd/audio_frm_%d_%d_%d_pcm.len", sample_bit, mode, sample_rate);
	if ((len_fd = fopen(file_path, "rb")) == NULL) {
		printf("Open file (%s) fail !!....\r\nPlease copy test pattern to SD Card !!\r\n\r\n", file_path);
		goto fclose;
	}
	printf("len file: [%s]\n", file_path);

	// feed audio frame
	while (p_stream0->feed_exit == 0) {
		blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail (0x%x).. try again later.....\r\n", blk);
			sleep(1);
			continue;
		}

		if ((blk_idx = blk2idx(blk)) == -1) {
			printf("ERROR !! blk to idx fail !!\r\n");
			goto rel_blk;
		}

		pa[blk_idx] = hd_common_mem_blk2pa(blk); // Get physical addr
		if (pa[blk_idx] == 0) {
			printf("blk2pa fail, blk = 0x%x\r\n", blk);
			goto rel_blk;
		}

		if (va[blk_idx] == 0) { // if NOT mmap yet, mmap it
			va[blk_idx] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa[blk_idx], blk_size); // Get virtual addr
			if (va[blk_idx] == 0) {
				printf("Error: mmap fail !! pa[%d], blk = 0x%x\r\n", blk_idx, blk);
				goto rel_blk;
			}
		}

		// Get frame size
		if (fscanf(len_fd, "%d %d\n", &frm_size, &timestamp) == EOF) {
			// reach EOF, read from the beginning
			fseek(frm_fd, 0, SEEK_SET);
			fseek(len_fd, 0, SEEK_SET);
			if (fscanf(len_fd, "%d %d\n", &frm_size, &timestamp) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (frm_size <= 0 || frm_size > MAX_FRM_BLK_SIZE) {
			printf("Invalid frm_size(%d)\n", frm_size);
			continue;
		}
		au_frame_ms = frm_size * 1000 / sample_rate; // the time(in ms) of each audio frame

		// Read frame from file
		result = fread((void *)va[blk_idx], 1, frm_size, frm_fd);
		if (result != frm_size) {
			printf("read frame error\n");
			continue;
		}

		// Push in audio frame
		{
			HD_AUDIO_FRAME audio_frame = {0};

			audio_frame.sign        = MAKEFOURCC('A','F','R','M');
			audio_frame.p_next      = NULL;
			audio_frame.ddr_id      = ddr_id;
			audio_frame.size        = frm_size;
			audio_frame.phy_addr[0] = pa[blk_idx];
			audio_frame.bit_width   = sample_bit;
			audio_frame.sound_mode  = mode;
			audio_frame.sample_rate = sample_rate;
			audio_frame.count       = 0;
			audio_frame.timestamp   = hd_gettime_us();

			ret = hd_audioenc_push_in_buf(p_stream0->enc_path, &audio_frame, NULL, 0); // only support non-blocking mode now
			if (ret != HD_OK) {
				printf("push_in error, ret(%d)\r\n", ret);
			}
		}

rel_blk:
		// Release memory
		if ((ret = hd_common_mem_release_block(blk)) != HD_OK) {
			printf("release_blk fail, ret(%d)\n", ret);
		}

		usleep(au_frame_ms * 1000);
	}

	// mummap for frame buffer
	for (i=0; i< MAX_FRM_BLK_CNT; i++) {
		if (va[i] != 0) {
			if ((ret = hd_common_mem_munmap((void *)va[i], blk_size)) != HD_OK) {
				printf("munmap fail, ret(%d)\n", ret);
				goto fclose;
			}
		}
	}

fclose:
	// close files
	if (frm_fd != NULL) { fclose(frm_fd);}
	if (len_fd != NULL) { fclose(len_fd);}

	return 0;
}

static void *encode_thread(void *arg)
{
	AUDIO_ENCODE* p_stream0 = (AUDIO_ENCODE *)arg;
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
	HD_AUDIO_SR sample_rate = p_stream0->sample_rate;
	HD_AUDIO_BIT_WIDTH sample_bit = HD_AUDIO_BIT_WIDTH_16;
	HD_AUDIO_SOUND_MODE mode = p_stream0->mode;
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
		ret = hd_audioenc_pull_out_buf(p_stream0->enc_path, &data_pull, 300); // >1 = timeout mode

		if (ret == HD_OK) {
			UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.phy_addr);
			UINT32 size = data_pull.size;
			UINT32 timestamp = hd_gettime_ms();

			if (!p_stream0->flow_test) {
				// write bs
				if (f_out_main) fwrite(ptr, 1, size, f_out_main);
				if (f_out_main) fflush(f_out_main);

				// write bs len
				if (f_out_len) fprintf(f_out_len, "%d %d\n", size, timestamp);
				if (f_out_len) fflush(f_out_len);
			}

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

EXAMFUNC_ENTRY(hd_audio_encode_only, argc, argv)
{
	HD_RESULT ret;
    INT key;
	AUDIO_ENCODE stream[1] = {0}; //0: main stream
	UINT32 enc_type = 0;

	// query program options
	if (argc == 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type %d\r\n", enc_type);
			return 0;
		}
	} else if (argc == 4) {
		UINT32 sample_rate;
		UINT32 mode;

		enc_type = atoi(argv[1]);

		sample_rate = atoi(argv[2]);
		mode = atoi(argv[3]);

		if (sample_rate == 0) {
			stream[0].sample_rate = HD_AUDIO_SR_8000;
		} else if (sample_rate == 1) {
			stream[0].sample_rate = HD_AUDIO_SR_16000;
		} else if (sample_rate == 2) {
			stream[0].sample_rate = HD_AUDIO_SR_32000;
		} else if (sample_rate == 3) {
			stream[0].sample_rate = HD_AUDIO_SR_48000;
		} else {
			stream[0].sample_rate = HD_AUDIO_SR_16000;
		}

		if (mode == 0) {
			stream[0].mode= HD_AUDIO_SOUND_MODE_MONO;
		} else if (mode == 1) {
			stream[0].mode = HD_AUDIO_SOUND_MODE_STEREO;
		} else {
			stream[0].mode = HD_AUDIO_SOUND_MODE_MONO;
		}

		printf("enc_type %d, sample rate %d, mode %d\r\n", enc_type, stream[0].sample_rate, stream[0].mode);
		if(enc_type > 2) {
			printf("error: not support enc_type %d\r\n", enc_type);
			return 0;
		}
	} else if (argc == 5) {
		UINT32 sample_rate;
		UINT32 mode;

		enc_type = atoi(argv[1]);

		sample_rate = atoi(argv[2]);
		mode = atoi(argv[3]);

		if (sample_rate == 0) {
			stream[0].sample_rate = HD_AUDIO_SR_8000;
		} else if (sample_rate == 1) {
			stream[0].sample_rate = HD_AUDIO_SR_16000;
		} else if (sample_rate == 2) {
			stream[0].sample_rate = HD_AUDIO_SR_32000;
		} else if (sample_rate == 3) {
			stream[0].sample_rate = HD_AUDIO_SR_48000;
		} else {
			stream[0].sample_rate = HD_AUDIO_SR_16000;
		}

		if (mode == 0) {
			stream[0].mode= HD_AUDIO_SOUND_MODE_MONO;
		} else if (mode == 1) {
			stream[0].mode = HD_AUDIO_SOUND_MODE_STEREO;
		} else {
			stream[0].mode = HD_AUDIO_SOUND_MODE_MONO;
		}

		stream[0].flow_test = atoi(argv[4]);
		printf("enc_type %d, sample rate %d, mode %d, flow_test %d\r\n", enc_type, stream[0].sample_rate, stream[0].mode, stream[0].flow_test);
		if(enc_type > 2) {
			printf("error: not support enc_type %d\r\n", enc_type);
			return 0;
		}
	} else {
		stream[0].sample_rate = HD_AUDIO_SR_48000;
		stream[0].mode = HD_AUDIO_SOUND_MODE_STEREO;
	}

	// init hdal
	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
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

    // open all modules
	ret = open_module(&stream[0]);
    if(ret != HD_OK) {
        printf("open fail=%d\n", ret);
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
	ret = set_enc_param(stream[0].enc_path, stream[0].enc_type, stream[0].sample_rate, stream[0].mode);
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode_thread failed\n");
		goto exit;
	}

	// create feed_frm_thread (push_in frame)
	ret = pthread_create(&stream[0].feed_thread_id, NULL, feed_frm_thread, (void *)stream);
	if (ret < 0) {
		printf("create feed_frm_thread failed\n");
		goto exit;
	}

	// start audio_encode_only modules
	hd_audioenc_start(stream[0].enc_path);

	// let thread start to work
	stream[0].flow_start = 1;

	// query user key
	printf("Enter q to exit, Enter d to debug\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let thread stop loop and exit
			stream[0].enc_exit = 1;
			usleep(100000); // delay 100 ms wait for encode_thread stop
			stream[0].feed_exit = 1;
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

	// destroy all thread
	pthread_join(stream[0].enc_thread_id, NULL);
	pthread_join(stream[0].feed_thread_id, NULL);

	// stop all modules
	hd_audioenc_stop(stream[0].enc_path);

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
