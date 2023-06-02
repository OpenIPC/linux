/**
	@brief Sample code of video playback.\n

	@file video_playback.c

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
#include <kwrap/util.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 1
#define SVC_BITSTREAM_TESTING 0

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

//header
#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NRX: RAW compress: Only support 12bit mode
#define RAW_COMPRESS_RATIO 59
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_4(ALIGN_CEIL_64(w) * 12 / 8 * RAW_COMPRESS_RATIO / 100 * (h)))
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

//YUV
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)
 
///////////////////////////////////////////////////////////////////////////////

#define VDO_SIZE_W          1920          // video pattern width
#define VDO_SIZE_H          1080          // video pattern height
#define DISP_SIZE_W         1920         // display panel width
#define DISP_SIZE_H         1080         // display panel height
#define BS_BLK_SIZE         0x200000     // bitstream buffer size
#define BS_BLK_DESC_SIZE    0x200     // bitstream buffer size
#define H26X_NAL_MAXSIZE    512          // H.265 NAL maximum length

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	// config common pool (decode)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(VDO_SIZE_W), ALIGN_CEIL_64(VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);  // align to 64 for h265 raw buffer
	mem_cfg.pool_info[0].blk_cnt = 5;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	// config common pool (scale & display)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_16(DISP_SIZE_W), ALIGN_CEIL_16(DISP_SIZE_H), HD_VIDEO_PXLFMT_YUV420);  // align to 16 for rotation panel
	mem_cfg.pool_info[1].blk_cnt = 5;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;
	// config common pool for bs pushing in
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[2].blk_size = BS_BLK_SIZE;
	mem_cfg.pool_info[2].blk_cnt = 1;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	// config common pool for bs description pushing in
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_USER_POOL_BEGIN;
	mem_cfg.pool_info[3].blk_size = BS_BLK_DESC_SIZE;
	mem_cfg.pool_info[3].blk_cnt = 1;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

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

static HD_RESULT set_dec_cfg(HD_PATH_ID video_dec_path, HD_DIM *p_max_dim, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_PATH_CONFIG video_path_cfg = {0};

	if (p_max_dim != NULL) {
		// set videodec path config
		video_path_cfg.max_mem.codec_type = dec_type;
		video_path_cfg.max_mem.dim.w = p_max_dim->w;
		video_path_cfg.max_mem.dim.h = p_max_dim->h;
		ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_PATH_CONFIG, &video_path_cfg);
	} else {
		ret = HD_ERR_NG;
	}

	return ret;
}

static HD_RESULT parse_h26x_desc(UINT32 codec, UINT32 src_addr, UINT32 size, BOOL *is_desc)
{
	HD_RESULT r = HD_OK;
	UINT32 start_code = 0, count = 0;
	UINT8 *ptr8 = NULL;

	if (src_addr == 0) {
		printf("buf_addr is 0\r\n");
		return HD_ERR_NG;
	}
	if (size == 0) {
		printf("size is 0\r\n");
		return HD_ERR_NG;
	}
	if (!is_desc) {
		printf("is_desc is null\r\n");
		return HD_ERR_NG;
	}

	ptr8 = (UINT8 *)src_addr;
	count = size;

	if (codec == HD_CODEC_TYPE_H264) {
		while (count--) {
			// search start code to skip (sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01) && (*(ptr8 + 4) == 0x67 || *(ptr8 + 4) == 0x68)) {
	            start_code++;
			}
			if (start_code == 2) {
				*is_desc = TRUE;
				return HD_OK;
			}
			ptr8++;
		}
	} else if (codec == HD_CODEC_TYPE_H265) {
		while (count--) {
			// search start code to skip (vps, sps, pps)
			if ((*ptr8 == 0x00) && (*(ptr8 + 1) == 0x00) && (*(ptr8 + 2) == 0x00) && (*(ptr8 + 3) == 0x01) && ((*(ptr8 + 4) == 0x40) || (*(ptr8 + 4) == 0x42) || (*(ptr8 + 4) == 0x44))) {
	            start_code++;
			}
			if (start_code == 3) {
				*is_desc = TRUE;
				return HD_OK;
			}
			ptr8++;
		}
	} else {
		printf("unknown codec (%d)\r\n", codec);
		return HD_ERR_NG;
	}

	*is_desc = FALSE;
	return r;
}

static HD_RESULT set_dec_param(HD_PATH_ID video_dec_path, UINT32 dec_type)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEODEC_IN video_in_param = {0};

	//--- HD_VIDEODEC_PARAM_IN ---
	video_in_param.codec_type = dec_type;
	ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN, &video_in_param);

	//--- HD_VIDEODEC_PARAM_IN_DESC ---
	if (dec_type == HD_CODEC_TYPE_H264 || dec_type == HD_CODEC_TYPE_H265) {
		//UINT32 desc_len = 0;
		BOOL is_desc = FALSE;
		INT bs_size = 0, read_len = 0;
		char codec_name[8], file_name[128];
		UINT8 desc_buf[H26X_NAL_MAXSIZE+1] = {0};
		FILE *bs_fd = 0, *bslen_fd = 0;
		HD_COMMON_MEM_VB_BLK blk;
		HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
		UINT32 blk_size = BS_BLK_DESC_SIZE;
		UINT32 pa = 0, va = 0, bs_buf_start = 0, bs_buf_curr = 0;

		// assign video codec
		if (dec_type == HD_CODEC_TYPE_H264) {
			snprintf(codec_name, sizeof(codec_name), "h264");
		} else if (dec_type == HD_CODEC_TYPE_H265) {
			snprintf(codec_name, sizeof(codec_name), "h265");
		}

		// open input files
#if SVC_BITSTREAM_TESTING
		sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s_svc.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
		sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
		if ((bs_fd = fopen(file_name, "rb")) == NULL) {
			printf("open file (%s) fail !!\r\n", file_name);
			return HD_ERR_SYS;
		}
#if SVC_BITSTREAM_TESTING
		snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s_svc.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
		snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
		if ((bslen_fd = fopen(file_name, "rb")) == NULL) {
			printf("open file (%s) fail !!\r\n", file_name);
			ret = HD_ERR_SYS;
			goto rel_fd;
		}

		// get memory
		blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("get block fail, blk = 0x%x\n", blk);
			ret = HD_ERR_SYS;
			goto rel_fd;
		}
		
		pa = hd_common_mem_blk2pa(blk); // get physical addr
		if (pa == 0) {
			printf("blk2pa fail, blk(0x%x)\n", blk);
			ret = HD_ERR_SYS;
			goto rel_blk;
		}
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size); // Get virtual addr
			if (va == 0) {
				printf("get va fail, va(0x%x)\n", blk);
				ret = HD_ERR_SYS;
				goto rel_blk;
			}
			// allocate bs buf
			bs_buf_start = va;
			bs_buf_curr = bs_buf_start;
		}

		// get bs size
		if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
			printf("[ERROR] fscanf error\n");
			ret = HD_ERR_SYS;
			goto set_desc_q;
		}

		if (bs_size == 0 || bs_size > H26X_NAL_MAXSIZE) {
			printf("[ERROR] DESC bs_size is 0 !!!\n");
			ret = HD_ERR_SYS;
			goto set_desc_q;
		}

		// read bs from file
		read_len = fread((void *)&desc_buf, 1, bs_size, bs_fd);
		desc_buf[read_len] = '\0';
		if (read_len != bs_size) {
			printf("set_desc reading error (read_len=%d, bs_size=%d)\n", read_len, bs_size);
			ret = HD_ERR_SYS;
			goto set_desc_q;
		}

		// parse and get h.26x desc
		parse_h26x_desc(dec_type, (UINT32)&desc_buf, bs_size, &is_desc);
		if (is_desc) {
			HD_H26XDEC_DESC desc_info = {0};
			memcpy((UINT8 *)bs_buf_curr, desc_buf, read_len);
			desc_info.addr = bs_buf_curr;//(UINT32)&desc_buf;
			desc_info.len = read_len;
			ret = hd_videodec_set(video_dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
			if (ret != HD_OK) {
				printf("set desc error(%d) !!\r\n\r\n", ret);
				ret = HD_ERR_SYS;
				goto set_desc_q;
			}
		} else {
			printf("invalid desc_addr = 0x%x, len = 0x%x\n", (UINT32)&desc_buf, read_len);
		}

set_desc_q:
		// mummap
		ret = hd_common_mem_munmap((void*)va, blk_size);
		if (ret != HD_OK) {
			printf("mnumap error(%d) !!\r\n\r\n", ret);
		}

rel_blk:
		// release blk
		ret = hd_common_mem_release_block(blk);
		if (ret != HD_OK) {
			printf("release error(%d) !!\n", ret);
		}

rel_fd:
		if (bs_fd) fclose(bs_fd);
		if (bslen_fd) fclose(bslen_fd);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_SCALE;
		video_cfg_param.isp_id = HD_ISP_DONT_CARE;
		video_cfg_param.ctrl_max.func = 0;
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = ALIGN_CEIL_64(p_max_dim->w);
		video_cfg_param.in_max.dim.h = ALIGN_CEIL_64(p_max_dim->h);
		video_cfg_param.in_max.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type, HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	switch(out_type){
	case 0:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_CVBS;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.cvbs= HD_VIDEOOUT_CVBS_NTSC;
	break;
	case 1:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	break;
	case 2:
		videoout_mode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
		videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
		videoout_mode.output_mode.hdmi= hdmi_id;
	break;
	default:
		printf("not support out_type\r\n");
	break;
	}
	ret = hd_videoout_set(video_out_ctrl, HD_VIDEOOUT_PARAM_MODE, &videoout_mode);

	*p_video_out_ctrl=video_out_ctrl ;
	return ret;
}

static HD_RESULT get_out_caps(HD_PATH_ID video_out_ctrl,HD_VIDEOOUT_SYSCAPS *p_video_out_syscaps)
{
	HD_RESULT ret = HD_OK;
    HD_DEVCOUNT video_out_dev = {0};

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_DEVCOUNT, &video_out_dev);
	if (ret != HD_OK) {
		return ret;
	}
	printf("##devcount %d\r\n", video_out_dev.max_dev_count);

	ret = hd_videoout_get(video_out_ctrl, HD_VIDEOOUT_PARAM_SYSCAPS, p_video_out_syscaps);
	if (ret != HD_OK) {
		return ret;
	}
	return ret;
}

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_IN video_out_param={0};

	video_out_param.dim.w = p_dim->w;
	video_out_param.dim.h = p_dim->h;
	video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	video_out_param.dir = HD_VIDEO_DIR_NONE;
	ret = hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	memset((void *)&video_out_param,0,sizeof(HD_VIDEOOUT_IN));
	ret = hd_videoout_get(video_out_path, HD_VIDEOOUT_PARAM_IN, &video_out_param);
	if (ret != HD_OK) {
		return ret;
	}
	//printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_PLAYBACK {

	// (1)
	HD_VIDEODEC_SYSCAPS  dec_syscaps;
	HD_PATH_ID           dec_path;
	HD_DIM               dec_max_dim;
	UINT32               dec_type;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID           proc_ctrl;
	HD_PATH_ID           proc_path;
	HD_DIM               proc_max_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS  out_syscaps;
	HD_PATH_ID           out_ctrl;
	HD_PATH_ID           out_path;
	HD_DIM               out_max_dim;
	HD_DIM               out_dim;
	HD_VIDEOOUT_HDMI_ID  hdmi_id;

	// (4) user push
	pthread_t            dec_thread_id;
	UINT32               dec_exit;
	UINT32               flow_start;

} VIDEO_PLAYBACK;

static HD_RESULT init_module(void)
{
    HD_RESULT ret;
	if ((ret = hd_videodec_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_PLAYBACK *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videodec_open(HD_VIDEODEC_0_IN_0, HD_VIDEODEC_0_OUT_0, &p_stream->dec_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT close_module(VIDEO_PLAYBACK *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videodec_close(p_stream->dec_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videodec_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}

static BOOL check_test_pattern(UINT32 dec_type)
{
	FILE *f_in;
	char codec_name[8], filename[64], filepath[128];

	// assign video codec
	switch (dec_type) {
		case HD_CODEC_TYPE_JPEG:	snprintf(codec_name, sizeof(codec_name), "jpeg");		break;
		case HD_CODEC_TYPE_H264:	snprintf(codec_name, sizeof(codec_name), "h264");		break;
		case HD_CODEC_TYPE_H265:	snprintf(codec_name, sizeof(codec_name), "h265");		break;
		default:
			printf("invalid video codec(%d)\n", dec_type);
			return FALSE;
	}

	// check .dat file
#if SVC_BITSTREAM_TESTING
	sprintf(filename, "video_bs_%d_%d_%s_svc.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	sprintf(filename, "video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	sprintf(filepath, "/mnt/sd/%s", filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		if (dec_type == HD_CODEC_TYPE_H265) {
			printf("Do hd_video_record_self 0 first to generate %s\n", filename);
		} else if (dec_type == HD_CODEC_TYPE_H264) {
			printf("Do hd_video_record_self 1 first to generate %s\n", filename);
		} else if (dec_type == HD_CODEC_TYPE_JPEG) {
			printf("Do hd_video_record_self 2 first to generate %s\n", filename);
		}
		return FALSE;
	}
	fclose(f_in);

	// check .len file
#if SVC_BITSTREAM_TESTING
	sprintf(filename, "video_bs_%d_%d_%s_svc.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	sprintf(filename, "video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	sprintf(filepath, "/mnt/sd/%s", filename);

	if ((f_in = fopen(filepath, "rb")) == NULL) {
		printf("fail to open %s\n", filepath);
		if (dec_type == HD_CODEC_TYPE_H265) {
			printf("Do hd_video_record_self 0 first to generate %s\n", filename);
		} else if (dec_type == HD_CODEC_TYPE_H264) {
			printf("Do hd_video_record_self 1 first to generate %s\n", filename);
		} else if (dec_type == HD_CODEC_TYPE_JPEG) {
			printf("Do hd_video_record_self 2 first to generate %s\n", filename);
		}
		return FALSE;
	}
	fclose(f_in);

	return TRUE;
}

static HD_RESULT get_video_frame_buf(HD_VIDEO_FRAME *p_video_frame)
{
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = 0;
	UINT32 blk_size = 0, pa = 0;
	UINT32 width = 0, height = 0;
	HD_VIDEO_PXLFMT pxlfmt = 0;

	if (p_video_frame == NULL) {
		printf("config_vdo_frm: p_video_frame is null\n");
		return HD_ERR_SYS;
	}

	// config yuv info
	ddr_id = DDR_ID0;
	width = VDO_SIZE_W;
	height = VDO_SIZE_H;
	pxlfmt = HD_VIDEO_PXLFMT_YUV420;
	blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_64(width), ALIGN_CEIL_64(height), pxlfmt);  // align to 64 for h265 raw buffer

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("config_vdo_frm: get blk fail, blk(0x%x)\n", blk);
		return HD_ERR_SYS;
	}

	pa = hd_common_mem_blk2pa(blk); // get physical addr
	if (pa == 0) {
		printf("config_vdo_frm: blk2pa fail, blk(0x%x)\n", blk);
		return HD_ERR_SYS;
	}

	p_video_frame->sign = MAKEFOURCC('V', 'F', 'R', 'M');
	p_video_frame->ddr_id = ddr_id;
	p_video_frame->pxlfmt = pxlfmt;
	p_video_frame->dim.w = width;
	p_video_frame->dim.h = height;
	p_video_frame->phy_addr[0] = pa;
	p_video_frame->blk = blk;

	return HD_OK;
}

static void *feed_bs_thread(void *arg)
{
	VIDEO_PLAYBACK *p_stream0 = (VIDEO_PLAYBACK *)arg;
	char codec_name[8], file_name[128];
	FILE *bs_fd = 0, *bslen_fd = 0;
	HD_RESULT ret = HD_OK;
	HD_COMMON_MEM_VB_BLK blk;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32 blk_size = BS_BLK_SIZE;
	UINT32 pa = 0, va = 0, bs_buf_start = 0, bs_buf_curr = 0, bs_buf_end = 0;
	INT bs_size = 0, read_len = 0;
	BOOL is_desc = FALSE;

	// wait flow_start
	while (p_stream0->flow_start == 0) sleep(1);

	// assign video codec
	switch (p_stream0->dec_type) {
		case HD_CODEC_TYPE_JPEG:
			snprintf(codec_name, sizeof(codec_name), "jpeg");
			break;
		case HD_CODEC_TYPE_H264:
			snprintf(codec_name, sizeof(codec_name), "h264");
			break;
		case HD_CODEC_TYPE_H265:
			snprintf(codec_name, sizeof(codec_name), "h265");
			break;
		default:
			printf("invalid video codec(%d)\n", p_stream0->dec_type);
			break;
	}

	// open input files
#if SVC_BITSTREAM_TESTING
	sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s_svc.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	sprintf(file_name, "/mnt/sd/video_bs_%d_%d_%s.dat", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	if ((bs_fd = fopen(file_name, "rb")) == NULL) {
		if (p_stream0->dec_type == HD_CODEC_TYPE_H265) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 0 first !!\r\n\r\n", file_name);
		} else if (p_stream0->dec_type == HD_CODEC_TYPE_H264) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 1 first !!\r\n\r\n", file_name);
		} else if (p_stream0->dec_type == HD_CODEC_TYPE_JPEG) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 2 first !!\r\n\r\n", file_name);
		}

		return 0;
	}
	printf("bs file: [%s]\n", file_name);

#if SVC_BITSTREAM_TESTING
	snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s_svc.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#else
	snprintf(file_name, sizeof(file_name), "/mnt/sd/video_bs_%d_%d_%s.len", VDO_SIZE_W, VDO_SIZE_H, codec_name);
#endif
	if ((bslen_fd = fopen(file_name, "rb")) == NULL) {
		if (p_stream0->dec_type == HD_CODEC_TYPE_H265) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 0 first !!\r\n\r\n", file_name);
		} else if (p_stream0->dec_type == HD_CODEC_TYPE_H264) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 1 first !!\r\n\r\n", file_name);
		} else if (p_stream0->dec_type == HD_CODEC_TYPE_JPEG) {
			printf("open file (%s) fail !!....\r\nPlease do hd_video_record_self 2 first !!\r\n\r\n", file_name);
		}
		goto quit_rel_fd;
	}
	printf("bslen file: [%s]\n", file_name);

	// get memory
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_POOL_BEGIN, blk_size, ddr_id); // Get block from mem pool
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail, blk = 0x%x\n", blk);
		goto rel_blk;
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
	}

	// feed bs
	while (p_stream0->dec_exit == 0) {

		// get bs size
		if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
			// reach EOF, read from the beginning
			fseek(bs_fd, 0, SEEK_SET);
			fseek(bslen_fd, 0, SEEK_SET);
			if (fscanf(bslen_fd, "%d\n", &bs_size) == EOF) {
				printf("[ERROR] fscanf error\n");
				continue;
			}
		}
		if (bs_size == 0) {
			printf("Invalid bs_size(%d)\n", bs_size);
			continue;
		}

		// check bs buf rollback
		if ((bs_buf_curr + bs_size) > bs_buf_end) {
			bs_buf_curr = bs_buf_start;
		}

		// read bs from file
		read_len = fread((void *)bs_buf_curr, 1, bs_size, bs_fd);
		if (read_len != bs_size) {
			printf("reading error\n");
			continue;
		}

		if (p_stream0->dec_type == HD_CODEC_TYPE_H264 || p_stream0->dec_type == HD_CODEC_TYPE_H265) {
			// get h.26x desc
			is_desc = FALSE;
			parse_h26x_desc(p_stream0->dec_type, bs_buf_curr, bs_size, &is_desc);
			if (is_desc) {
				HD_H26XDEC_DESC desc_info = {0};
				desc_info.addr = bs_buf_curr;
				desc_info.len = read_len;
				if (0) { // update desc [ToDo]
					ret = hd_videodec_set(p_stream0->dec_path, HD_VIDEODEC_PARAM_IN_DESC, &desc_info);
					if (ret != HD_OK) {
						printf("set DESC fail\r\n");
						goto rel_blk;
					}
					hd_videodec_start(p_stream0->dec_path); // will apply in next I-frame, set if need (not support)
				}
			}
		}

		// push in
		if (!is_desc) { // only push I or P
			HD_VIDEODEC_BS video_bitstream = {0};
			HD_VIDEO_FRAME video_frame = {0};

			// config video bs
			video_bitstream.sign          = MAKEFOURCC('V','S','T','M');
			video_bitstream.p_next        = NULL;
			video_bitstream.ddr_id        = ddr_id;
			video_bitstream.vcodec_format = p_stream0->dec_type;
			video_bitstream.timestamp     = hd_gettime_us();
			video_bitstream.blk           = blk;
			video_bitstream.count         = 0;
			video_bitstream.phy_addr      = pa + (bs_buf_curr - bs_buf_start);
			video_bitstream.size          = bs_size;

			// get video frame
			ret = get_video_frame_buf(&video_frame);
			if (ret != HD_OK) {
				printf("get video frame error(%d) !!\r\n", ret);
				continue;
			}

			ret = hd_videodec_push_in_buf(p_stream0->dec_path, &video_bitstream, &video_frame, 0); // always blocking mode
			if (ret != HD_OK) {
				printf("push_in error(%d) !!\r\n", ret);
				// release video frame buf
				ret = hd_videodec_release_out_buf(p_stream0->dec_path, &video_frame);
				if (ret != HD_OK) {
					printf("release video frame error(%d) !!\r\n\r\n", ret);
				}
				continue;
			}
			// release video frame buf
			ret = hd_videodec_release_out_buf(p_stream0->dec_path, &video_frame);
			if (ret != HD_OK) {
				printf("release video frame error(%d) !!\r\n\r\n", ret);
			}
		}

		bs_buf_curr += ALIGN_CEIL_64(bs_size); // shift to next

		usleep(30000); // sleep 30 ms
	}

	// mummap
	ret = hd_common_mem_munmap((void*)va, blk_size);
	if (ret != HD_OK) {
		printf("mnumap error(%d) !!\r\n\r\n", ret);
	}

rel_blk:
	// release blk
	ret = hd_common_mem_release_block(blk);
	if (ret != HD_OK) {
		printf("release error(%d) !!\n", ret);
	}

quit_rel_fd:
	if (bs_fd) fclose(bs_fd);
	if (bslen_fd) fclose(bslen_fd);

	return 0;
}

EXAMFUNC_ENTRY(hd_video_playback, argc, argv)
{
	HD_RESULT ret;
    INT key;
	VIDEO_PLAYBACK stream[1] = {0}; //0: main stream
	UINT32 dec_type = 0;
	UINT32 out_type = 0;
	HD_DIM main_dim = {0};

	// query program options
	if (argc == 2) {
		dec_type = atoi(argv[1]);
		printf("dec_type %d\r\n", dec_type);
		if(stream[0].dec_type > 2) {
			printf("error: not support dec_type %d\r\n", dec_type);
			return 0;
		}
	}

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;

	if (dec_type == 0) {
		stream[0].dec_type = HD_CODEC_TYPE_H265;
	} else if (dec_type == 1) {
		stream[0].dec_type = HD_CODEC_TYPE_H264;
	} else if (dec_type == 2) {
		stream[0].dec_type = HD_CODEC_TYPE_JPEG;
	} else {
		printf("error codec type\r\n");
		return 0;
	}

	// check TEST pattern exist
	if (check_test_pattern(stream[0].dec_type) == FALSE) {
		printf("test_pattern isn't exist\r\n");
		exit(0);
	}

	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
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
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open video_playback modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	out_type = 1;                          //assign by user, 0:CVBS, 1:LCD, 2:HDMI
	stream[0].hdmi_id = HD_VIDEOOUT_HDMI_1920X1080I60; //default
	ret = open_module(&stream[0], &stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}

	// check display buffer size
	if (stream[0].out_syscaps.output_dim.w > DISP_SIZE_W ||
		stream[0].out_syscaps.output_dim.h > DISP_SIZE_H) {
		printf("display buffer size is not enough!\n");
		printf("display buffer size(%dx%d) < panel size(%dx%d)\r\n",
			stream[0].out_syscaps.output_dim.w,
			stream[0].out_syscaps.output_dim.h,
			DISP_SIZE_W,
			DISP_SIZE_H);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videodec config (main)
	stream[0].dec_max_dim.w = main_dim.w;
	stream[0].dec_max_dim.h = main_dim.h;
	ret = set_dec_cfg(stream[0].dec_path, &stream[0].dec_max_dim, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videodec parameter (main)
	ret = set_dec_param(stream[0].dec_path, stream[0].dec_type);
	if (ret != HD_OK) {
		printf("set dec fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (main)
	stream[0].out_dim.w = stream[0].out_max_dim.w; //using device max dim.w
	stream[0].out_dim.h = stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(stream[0].out_path, &stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_playback modules (main)
	hd_videodec_bind(HD_VIDEODEC_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);

	// start video_playback modules (main)
	hd_videodec_start(stream[0].dec_path);
	hd_videoproc_start(stream[0].proc_path);
	// wait ae/awb stable
	//sleep(1);
	hd_videoout_start(stream[0].out_path);

	// let encode_thread start to work
	stream[0].flow_start = 1;

	// create decode_thread (push_in bitstream)
	ret = pthread_create(&stream[0].dec_thread_id, NULL, feed_bs_thread, (void *)stream);
	if (ret < 0) {
		printf("create playback thread failed");
		goto exit;
	}

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			// let feed_bs_thread stop loop and exit
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

	// destroy encode thread
	pthread_join(stream[0].dec_thread_id, NULL);

	// stop video_playback modules (main)
	hd_videodec_stop(stream[0].dec_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoout_stop(stream[0].out_path);

	// unbind video_playback modules (main)
	hd_videodec_unbind(HD_VIDEODEC_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
	// close video_playback modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = mem_exit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return 0;
}
