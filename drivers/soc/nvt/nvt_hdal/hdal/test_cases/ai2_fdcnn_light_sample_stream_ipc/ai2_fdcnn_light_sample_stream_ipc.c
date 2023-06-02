/**
	@brief Source file of vendor net application sample using user-space net flow.

	@file alg_fdcnn_sample_stream.c

	@ingroup alg_fdcnn_sample_stream

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
#include <sys/time.h>
#include <dirent.h>

#include "vendor_isp.h"
#include "vendor_gfx.h"
#include "vendor_ai.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"

#include "fdcnn_lib.h"
#include "pd_shm.h"


#if defined(__LINUX)
#include <sys/ipc.h>
#include <sys/shm.h>
#else
//for delay
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

#define DEBUG_MENU 			1

//#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
//#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
//#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define	SENSOR_291			1
#define	SENSOR_S02K			2
#define	SENSOR_S05A			3
#define	SENSOR_CHOICE		SENSOR_291 // SENSOR_S02K

#define DBGINFO_BUFSIZE()	(0x200)

//RAW
#define VDO_RAW_BUFSIZE(w, h, pxlfmt)   (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(pxlfmt) / 8) * (h))
//RAW compress only support 12bit mode
#define VDO_NRX_BUFSIZE(w, h)           (ALIGN_CEIL_64(w)/64*14*4*h)
//CA for AWB
#define VDO_CA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 3) << 1)
//LA for AE
#define VDO_LA_BUF_SIZE(win_num_w, win_num_h) ALIGN_CEIL_4((win_num_w * win_num_h << 1) << 1)

#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * RAW_COMPRESS_RATIO / 100)

#define AI_IPC              ENABLE

#define SEN_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32

#define VDO_SIZE_W			1920
#define VDO_SIZE_H			1080

#define SENSOR_4M			ENABLE

#define SOURCE_PATH 		HD_VIDEOPROC_0_OUT_1
#define EXTEND_PATH1		HD_VIDEOPROC_0_OUT_5
#define EXTEND_PATH2		HD_VIDEOPROC_0_OUT_6

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420
#define OSG_LCD_WIDTH       960
#define OSG_LCD_HEIGHT      240

#define NN_FDCNN_FD_MODE       ENABLE
#define NN_FDCNN_FD_PROF       ENABLE
#define NN_FDCNN_FD_DUMP       DISABLE
#define NN_FDCNN_FD_DRAW       ENABLE
#define NN_FDCNN_FD_FIX_FRM    DISABLE
#define FDCNN_MAX_OUTNUM       (10)

#define NN_FDCNN_FD_TYPE       FDCNN_NETWORK_V21

#define NN_USE_DRAM2           ENABLE
#define NN_FDCNN_HIGH_PRIORITY DISABLE //**********************************************

#define	NN_USE_HDR			   0		// 1 ON; 0 OFF

#define SHDR_CAP_OUT_FMT HD_VIDEO_PXLFMT_NRX12_SHDR2

#define VENDOR_AI_CFG  0x000f0000  //ai project config

#if (!AI_IPC)
#define FD_MEM_SIZE   fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#else
#define FD_MEM_SIZE   (0x520000)//(NETWORKS_MEM_SIZE + POSTPROC_MEM_SIZE)
#endif

#define HD_VIDEOPROC_PATH(dev_id, in_id, out_id)	(((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

typedef struct _VIDEO_LIVEVIEW {
#if(!AI_IPC)
	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;
#endif
	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;
#if(!AI_IPC)
	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;
#endif
	// (4) --
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;

	// (5) --
	HD_PATH_ID mask_alg_path;

    // osg
#if NN_FDCNN_FD_DRAW
	HD_PATH_ID mask_path0;
    HD_PATH_ID mask_path1;
    HD_PATH_ID mask_path2;
    HD_PATH_ID mask_path3;
#endif

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;

static VENDOR_AIS_FLOW_MEM_PARM g_mem = {0};
static HD_COMMON_MEM_VB_BLK g_blk_info[1];
static UINT32 fps_delay = 0;
static INT32 save_results = 0;

typedef struct _FDCNN_THREAD_PARM {
    VENDOR_AIS_FLOW_MEM_PARM mem;
    VIDEO_LIVEVIEW stream;
} FDCNN_THREAD_PARM;

typedef struct _FDCNN_THREAD_DRAW_PARM {
    VENDOR_AIS_FLOW_MEM_PARM fd_mem;
    VIDEO_LIVEVIEW stream;
} FDCNN_THREAD_DRAW_PARM;

static UINT32 g_shdr_mode = 0;

#if(AI_IPC)
static char   *g_shm = NULL;
#endif

///////////////////////////////////////////////////////////////////////////////

#if(AI_IPC)
void init_share_memory(void)
{
	int shmid = 0;
	key_t key;

	// Segment key.
	key = PD_SHM_KEY;
	// Create the segment.
	if( ( shmid = shmget( key, PD_SHMSZ, 0666 ) ) < 0 ) {
		perror( "shmget" );
		exit(1);
	}
	// Attach the segment to the data space.
	if( ( g_shm = shmat( shmid, NULL, 0 ) ) == (char *)-1 ) {
		perror( "shmat" );
		exit(1);
	}
}

void exit_share_memory(void)
{
	shmdt(g_shm);
}
#endif


///////////////////////////////////////////////////////////////////////////////

static HD_RESULT mem_init(void)
{
	HD_RESULT              ret;
    //UINT32 mem_size = 0;
#if(!AI_IPC)
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	#if NN_FDCNN_FD_MODE
    mem_size += FD_MEM_SIZE;
	#endif

	// config common pool (cap)
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
        													+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        													+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	mem_cfg.pool_info[0].blk_cnt = 2;
    mem_cfg.pool_info[0].ddr_id = DDR_ID0;

	// config common pool (main)
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[1].blk_cnt = 3;
    mem_cfg.pool_info[1].ddr_id = DDR_ID0;

	// config common pool (sub)
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, VDO_FRAME_FORMAT);
	mem_cfg.pool_info[2].blk_cnt = 3;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	// for nn
	mem_cfg.pool_info[3].type 		= HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[3].blk_size 	= mem_size;
	mem_cfg.pool_info[3].blk_cnt 	= 1;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}
#else

	ret = hd_common_mem_init(NULL);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		return ret;
	}
	if (g_mem.va != 0) {
		DBG_DUMP("err: mem has already been inited\r\n");
		return -1;
	}
#endif
	return ret;
}

static INT32 get_mem_block(UINT32 ddr)
{
	HD_RESULT                 ret = HD_OK;
	UINT32                    pa, va;
	HD_COMMON_MEM_VB_BLK      blk;
    UINT32 mem_size = 0;

    HD_COMMON_MEM_DDR_ID      ddr_id = ddr;


#if NN_FDCNN_FD_MODE
    mem_size += FD_MEM_SIZE;
#endif

    blk = hd_common_mem_get_block(HD_COMMON_MEM_CNN_POOL, mem_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		DBG_DUMP("hd_common_mem_get_block fail\r\n");
		ret =  HD_ERR_NG;
		goto exit;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		DBG_DUMP("not get buffer, pa=%08x\r\n", (int)pa);
		return -1;
	}
	va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, mem_size);
	g_blk_info[0] = blk;

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap((void *)va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem unmap fail\r\n");
			return ret;
		}
		return -1;
	}
	g_mem.pa = pa;
	g_mem.va = va;
	g_mem.size = mem_size;

exit:
	return ret;
}

static HD_RESULT release_mem_block(VOID)
{
	HD_RESULT ret = HD_OK;
    UINT32 mem_size = 0;

#if NN_FDCNN_FD_MODE
    mem_size += FD_MEM_SIZE;
#endif

	/* Release in buffer */
	if (g_mem.va) {
		ret = hd_common_mem_munmap((void *)g_mem.va, mem_size);
		if (ret != HD_OK) {
			DBG_DUMP("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(g_blk_info[0]);
	if (ret != HD_OK) {
		DBG_DUMP("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
#if(!AI_IPC)
static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}
#if 0
static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}
#endif
static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
#if SENSOR_CHOICE==SENSOR_291
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x301;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0|PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#elif SENSOR_CHOICE==SENSOR_S02K
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;

	if(g_shdr_mode==1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = HD_VIDEOCAP_SEN_IGNORE;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = HD_VIDEOCAP_SEN_IGNORE;
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	}

	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	if (g_shdr_mode == 1) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}


	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;

	if (g_shdr_mode == 1) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}

	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#elif SENSOR_CHOICE==SENSOR_S05A
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =	0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
	if (chip_name != NULL && strcmp(chip_name, "CHIP_NA51089") == 0) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x01;//PIN_I2C_CFG_CH1
	} else {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;

#endif
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;

		// NOTE: only SHDR with path 1
		if (g_shdr_mode == 1) {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		} else {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}
		///video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;

		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	#if 1 //no crop, full frame
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	}
	#else //HD_CROP_ON
	{
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = 0;
		video_crop_param.win.rect.y = 0;
		video_crop_param.win.rect.w = 1920/2;
		video_crop_param.win.rect.h= 1080/2;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		if (g_shdr_mode == 1) {
			video_out_param.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_out_param.pxlfmt = CAP_OUT_FMT;
		}
		///video_out_param.pxlfmt = CAP_OUT_FMT;

		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
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

	if (p_max_dim != NULL ) {
		video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		video_cfg_param.isp_id = 0;

		if (g_shdr_mode == 1) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		} else {
			video_cfg_param.ctrl_max.func &= ~HD_VIDEOPROC_FUNC_SHDR;
		}
		///video_cfg_param.ctrl_max.func = 0;

		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;

		if (g_shdr_mode == 1) {
			video_cfg_param.in_max.pxlfmt = SHDR_CAP_OUT_FMT;
		} else {
			video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		}
		///video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;

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
		video_out_param.pxlfmt = VDO_FRAME_FORMAT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = 1;	// set > 0 to allow pull out (nn)

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_out_cfg(HD_PATH_ID *p_video_out_ctrl, UINT32 out_type,HD_VIDEOOUT_HDMI_ID hdmi_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOOUT_MODE videoout_mode = {0};
	HD_PATH_ID video_out_ctrl = 0;

	ret = hd_videoout_open(0, HD_VIDEOOUT_0_CTRL, &video_out_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}

	printf("out_type=%d\r\n", out_type);

	#if 1
	videoout_mode.output_type = HD_COMMON_VIDEO_OUT_LCD;
	videoout_mode.input_dim = HD_VIDEOOUT_IN_AUTO;
	videoout_mode.output_mode.lcd = HD_VIDEOOUT_LCD_0;
	if (out_type != 1) {
		printf("520 only support LCD\r\n");
	}
	#else
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
	#endif
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
	video_out_param.pxlfmt = VDO_FRAME_FORMAT;
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
	printf("##video_out_param w:%d,h:%d %x %x\r\n", video_out_param.dim.w, video_out_param.dim.h, video_out_param.pxlfmt, video_out_param.dir);

	return ret;
}
#endif
///////////////////////////////////////////////////////////////////////////////

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
#if(!AI_IPC)
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
#else
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
#endif

	return HD_OK;
}
#if(!AI_IPC)
static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl);
	if (ret != HD_OK) {
		printf("set cap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoproc config for nn
	ret = set_proc_cfg(&p_stream->proc_alg_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg alg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, 1, p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, SOURCE_PATH, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
		return ret;

#if NN_FDCNN_FD_DRAW
	//open a mask in videoout
	if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_0, &p_stream->mask_path0)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_1, &p_stream->mask_path1)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_2, &p_stream->mask_path2)) != HD_OK)
		return ret;
    if((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_MASK_3, &p_stream->mask_path3)) != HD_OK)
		return ret;
#endif
	return HD_OK;
}

static HD_RESULT open_module_extend1(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoout config
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;

#if NN_FDCNN_FD_DRAW
    if ((ret = hd_videoout_close(p_stream->mask_path0)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path1)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path2)) != HD_OK)
		return ret;
    if ((ret = hd_videoout_close(p_stream->mask_path3)) != HD_OK)
		return ret;
#endif

	return HD_OK;
}

static HD_RESULT close_module_extend(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_alg_path)) != HD_OK)
		return ret;
	return HD_OK;
}
#endif

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
#if(!AI_IPC)
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
#else
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
#endif

	return HD_OK;
}

#if NN_FDCNN_FD_DRAW
#if(!AI_IPC)
static int init_mask_param(HD_PATH_ID mask_path)
{
	HD_OSG_MASK_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    // ghost target
    attr.position[0].x = 1;
    attr.position[0].y = 1;
    attr.position[1].x = 9;
    attr.position[1].y = 1;
    attr.position[2].x = 9;
    attr.position[2].y = 9;
    attr.position[3].x = 1;
    attr.position[3].y = 9;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 0;
    attr.color         = 0x00FF0000;
    attr.thickness     = 0;

	return hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);
}
static int fdcnn_mask_draw(HD_PATH_ID mask_path, FDCNN_RESULT *p_face, BOOL bdraw, UINT32 color)
{
    HD_OSG_MASK_ATTR attr;
    HD_RESULT ret = HD_OK;
    memset(&attr, 0, sizeof(HD_OSG_MASK_ATTR));

    if(!bdraw)
        return init_mask_param(mask_path);

    attr.position[0].x = p_face->x;
    attr.position[0].y = p_face->y;
    attr.position[1].x = p_face->x + p_face->w;
    attr.position[1].y = p_face->y;
    attr.position[2].x = p_face->x + p_face->w;
    attr.position[2].y = p_face->y + p_face->h;
    attr.position[3].x = p_face->x;
    attr.position[3].y = p_face->y + p_face->h;
    attr.type          = HD_OSG_MASK_TYPE_HOLLOW;
    attr.alpha         = 255;
    attr.color         = color;
    attr.thickness     = 2;

    ret = hd_videoout_set(mask_path, HD_VIDEOOUT_PARAM_OUT_MASK_ATTR, &attr);

    return ret;
}
#endif
#endif

#if NN_FDCNN_FD_DRAW
static int fdcnn_fd_draw_info(VENDOR_AIS_FLOW_MEM_PARM fd_buf, VIDEO_LIVEVIEW *p_stream)
{
    HD_RESULT ret = HD_OK;
    static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 fdcnn_num;
    static HD_URECT fdcnn_size = {0, 0, 1024, 576};
    fdcnn_num = fdcnn_getresults(fd_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
	#if(!AI_IPC)
    fdcnn_mask_draw(p_stream->mask_path0, fdcnn_info + 0, (BOOL)(fdcnn_num >= 1), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path1, fdcnn_info + 1, (BOOL)(fdcnn_num >= 2), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path2, fdcnn_info + 2, (BOOL)(fdcnn_num >= 3), 0x00FF0000);
    fdcnn_mask_draw(p_stream->mask_path3, fdcnn_info + 3, (BOOL)(fdcnn_num >= 4), 0x00FF0000);
	#else
	{
		PD_SHM_INFO  *p_pd_shm = (PD_SHM_INFO  *)g_shm;
		PD_SHM_RESULT *p_obj;
		UINT32         i;

		if (p_pd_shm->exit) {
			return -1;
		}

		// update pdcnn result to share memory
		p_pd_shm->pd_num = fdcnn_num;
		if(fdcnn_num > 10){
			p_pd_shm->pd_num = 10;
		}
		for (i = 0; i < p_pd_shm->pd_num; i++) {
			p_obj = &p_pd_shm->pd_results[i];
			p_obj->category = fdcnn_info[i].category;
			p_obj->score = (FLOAT)fdcnn_info[i].score / (FLOAT)(1 << 14);
			p_obj->x1 = fdcnn_info[i].x;
			p_obj->x2 = fdcnn_info[i].x + fdcnn_info[i].w;
			p_obj->y1 = fdcnn_info[i].y;
			p_obj->y2 = fdcnn_info[i].y + fdcnn_info[i].h;
		}
	}
	#endif
    return ret;
}
#endif

VOID *fdcnn_draw_thread(VOID *arg)
{
    FDCNN_THREAD_DRAW_PARM *p_draw_parm = (FDCNN_THREAD_DRAW_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_draw_parm->stream;
#if NN_FDCNN_FD_DRAW
    VENDOR_AIS_FLOW_MEM_PARM fd_mem = p_draw_parm->fd_mem;
#endif
    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

    // wait fd ro pd init ready
    sleep(2);

    while(1)
    {
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            break;
        }

#if NN_FDCNN_FD_DRAW
        fdcnn_fd_draw_info(fd_mem, &stream);
#endif

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            printf("ERR : hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            break;
        }
    }
    return 0;
}

static UINT32 fdcnn_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(p_filename, "rb");
	if (!fd) {
		DBG_ERR("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		DBG_ERR("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}

static VENDOR_AIS_FLOW_MEM_PARM fdcnn_getmem(VENDOR_AIS_FLOW_MEM_PARM *valid_mem, UINT32 required_size)
{
	VENDOR_AIS_FLOW_MEM_PARM mem = {0};
	required_size = ALIGN_CEIL_4(required_size);
	if(required_size <= valid_mem->size) {
		mem.va = valid_mem->va;
        mem.pa = valid_mem->pa;
		mem.size = required_size;

		valid_mem->va += required_size;
        valid_mem->pa += required_size;
		valid_mem->size -= required_size;
	} else {
		DBG_ERR("required size %d > total memory size %d\r\n", required_size, valid_mem->size);
	}
	return mem;
}

VOID *fdcnn_fd_thread(VOID *arg)
{
    FDCNN_THREAD_PARM *p_fd_parm = (FDCNN_THREAD_PARM*)arg;
    VIDEO_LIVEVIEW stream = p_fd_parm->stream;
    VENDOR_AIS_FLOW_MEM_PARM fd_mem = p_fd_parm->mem;

    HD_VIDEO_FRAME video_frame = {0};
    HD_RESULT ret = HD_OK;

	UINT32 yuv_va;
	HD_GFX_IMG_BUF input_image;
	BOOL init_image = FALSE;

	//#if NN_FDCNN_FD_DUMP
    static FDCNN_RESULT fdcnn_info[FDCNN_MAX_OUTNUM] = {0};
    UINT32 fdcnn_num = 0, i;
    static HD_URECT fdcnn_size = {0, 0, VDO_SIZE_W, VDO_SIZE_H};
	//#endif

#if NN_FDCNN_FD_PROF
    static struct timeval tstart0, tend0;
    static UINT64 cur_time0 = 0, mean_time0 = 0, sum_time0 = 0;
    static UINT32 icount = 0;
#endif
#if(!AI_IPC)
    UINT32 fdcnn_buf_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
#else
	UINT32 fdcnn_buf_size = FD_MEM_SIZE;
#endif
    VENDOR_AIS_FLOW_MEM_PARM fdcnn_buf  = fdcnn_getmem(&fd_mem, fdcnn_buf_size);

    if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V10) // FDCNN_NETWORK_V10 need 4 file
    {
        CHAR file_path[4][256] =  {      "/mnt/sd/CNNLib/para/fdcnn_method1/file1.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file2.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file3.bin", \
                                         "/mnt/sd/CNNLib/para/fdcnn_method1/file4.bin"  };

        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_2 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_2, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_3 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_3, NN_FDCNN_FD_TYPE);
        UINT32 model_addr_4 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_4, NN_FDCNN_FD_TYPE);

        fdcnn_load_file(file_path[0], model_addr_1);
        fdcnn_load_file(file_path[1], model_addr_2);
        fdcnn_load_file(file_path[2], model_addr_3);
        fdcnn_load_file(file_path[3], model_addr_4);
    }
    else if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V20) // FDCNN_NETWORK_V20 need 1 file
    {
        CHAR file_path[256] =  "/mnt/sd/CNNLib/para/fdcnn_method2/file1.bin";
        UINT32 model_addr = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        fdcnn_load_file(file_path, model_addr);
    }
	else if (NN_FDCNN_FD_TYPE == FDCNN_NETWORK_V21) // FDCNN_LIGHT
    {
        CHAR file_path[256] =  "/mnt/sd/CNNLib/para/fdcnn_method2_light/file1.bin";
        UINT32 model_addr_1 = fdcnn_get_model_addr(fdcnn_buf, FDCNN_FILE_1, NN_FDCNN_FD_TYPE);
        fdcnn_load_file(file_path, model_addr_1);
    }
    else
    {
        DBG_ERR("Not support net type %d !\r\n", NN_FDCNN_FD_TYPE);
        return 0;
    }

    FDCNN_INIT_PARM init_parm = {0};
    init_parm.net_type = NN_FDCNN_FD_TYPE;
    init_parm.net_id = 0;

    ret = fdcnn_init(fdcnn_buf, init_parm);
    if (ret != HD_OK)
    {
        DBG_ERR("fdcnn_init fail=%d\n", ret);
        return 0;
    }

	UINT32 ai_fd_frame = 0;

    while(1)
    {
        ret = hd_videoproc_pull_out_buf(stream.proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
        if(ret != HD_OK)
        {
            DBG_ERR("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
            goto exit;
        }

        // init image
        if(init_image == FALSE){
	        input_image.dim.w  = video_frame.dim.w;
	        input_image.dim.h = video_frame.dim.h;
	        input_image.format = VDO_FRAME_FORMAT;
	        input_image.p_phy_addr[0] = video_frame.phy_addr[0];
	        input_image.p_phy_addr[1] = video_frame.phy_addr[1];
	        input_image.p_phy_addr[2] = video_frame.phy_addr[1]; // for avoid hd_gfx_scale message
	        input_image.lineoffset[0] = video_frame.loff[0];
	        input_image.lineoffset[1] = video_frame.loff[0]; // for avoid hd_gfx_scale message
	        input_image.lineoffset[2] = video_frame.loff[0]; // for avoid hd_gfx_scale message

			yuv_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], video_frame.pw[0]*video_frame.ph[0]*3/2);

			init_image = TRUE;
		}

#if NN_FDCNN_FD_PROF
        gettimeofday(&tstart0, NULL);
#endif

        ret = fdcnn_set_image(fdcnn_buf, &input_image);
        if (ret != HD_OK)
        {
            DBG_ERR("fdcnn_set_image fail=%d\n", ret);
            goto exit;
        }

        ret = fdcnn_process(fdcnn_buf);
        if (ret != HD_OK)
        {
            DBG_ERR("fdcnn_process fail=%d\n", ret);
            goto exit;
        }

#if NN_FDCNN_FD_PROF
        gettimeofday(&tend0, NULL);
        cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        sum_time0 += cur_time0;
        mean_time0 = sum_time0/(++icount);
        //#if (!NN_FDCNN_FD_FIX_FRM)
        printf("[FD] process cur time(us): %lld, mean time(us): %lld\r\n", cur_time0, mean_time0);
        //#endif
#endif
		if(save_results){
			fdcnn_size.w = video_frame.dim.w;
        	fdcnn_size.h = video_frame.dim.h;
        	fdcnn_num = fdcnn_getresults(fdcnn_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);
			if(fdcnn_num > 0){
				if(fdcnn_num > 10){
					fdcnn_num = 10;
				}
				CHAR TXT_FILE[256], YUV_FILE[256];
				FILE *fs, *fb;
				sprintf(TXT_FILE, "/mnt/sd/det_results/FD/txt/%09ld.txt", ai_fd_frame);
				sprintf(YUV_FILE, "/mnt/sd/det_results/FD/yuv/%09ld.bin", ai_fd_frame);
				fs = fopen(TXT_FILE, "w+");
				fb = fopen(YUV_FILE, "wb+");
				fwrite((UINT32 *)yuv_va, sizeof(UINT32), (input_image.dim.h * input_image.dim.w * 3 / 2), fb);
				fclose(fb);
				for(i = 0; i < fdcnn_num; i++ )
       			{
       				INT32 xmin = (INT32)(fdcnn_info[i].x);
					INT32 ymin = (INT32)(fdcnn_info[i].y);
					INT32 width = (INT32)(fdcnn_info[i].w);
					INT32 height = (INT32)(fdcnn_info[i].h);
					FLOAT score = (FLOAT)(fdcnn_info[i].score) / (FLOAT)(1 << 14);
					fprintf(fs, "%f %d %d %d %d\r\n", score, xmin, ymin, width, height);
				}
				fclose(fs);
				ai_fd_frame++;
			}
		}

        ret = hd_videoproc_release_out_buf(stream.proc_alg_path, &video_frame);
        if(ret != HD_OK)
        {
            DBG_ERR("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
            goto exit;
        }

	    gettimeofday(&tend0, NULL);
		cur_time0 = (UINT64)(tend0.tv_sec - tstart0.tv_sec) * 1000000 + (tend0.tv_usec - tstart0.tv_usec);
        if ((fps_delay)&&(cur_time0 < fps_delay)) {
            usleep(fps_delay - cur_time0 + (mean_time0*0));
        }

		//#if NN_FDCNN_FD_FIX_FRM
        //	if (cur_time0 < 100000)
		//   	usleep(100000 - cur_time0 + (mean_time0*0));
		//#endif

#if NN_FDCNN_FD_DUMP
	#if SENSOR_4M
        fdcnn_size.w = 2560;
        fdcnn_size.h = 1440;
	#else
		fdcnn_size.w = 1920;
		fdcnn_size.h = 1080;
	#endif
        fdcnn_num = fdcnn_getresults(fdcnn_buf, fdcnn_info, &fdcnn_size, FDCNN_MAX_OUTNUM);

        printf("[FD] ----------- num : %ld ----------- \n", fdcnn_num);
        for(i = 0; i < fdcnn_num; i++ )
        {
            printf("[FD] %ld\t%ld\t%ld\t%ld\t%f\r\n", fdcnn_info[i].x, fdcnn_info[i].y, fdcnn_info[i].w, fdcnn_info[i].h, (FLOAT)fdcnn_info[i].score / (FLOAT)(1 << 15));
        }
#endif

    }

exit:
    ret = fdcnn_uninit(fdcnn_buf);
    if (ret != HD_OK)
    {
        DBG_ERR("fdcnn_uninit fail=%d\n", ret);
    }
    return 0;
}

int main(int argc, char** argv)
{
	HD_RESULT ret;
    VIDEO_LIVEVIEW stream[2] = {0};
	g_shdr_mode = NN_USE_HDR;

	UINT32 cnn_id = 0;
	UINT32 ddr_id = 0;

    fps_delay = 0 ;

	// query program options
	if (argc >= 2) {
		UINT32 fps = atoi(argv[1]);
        if((fps==0)||(fps>30)) {
            fps = 30;
        }
  		fps_delay = (UINT32)(1000000/fps);
		printf("fps %d fps_delay %d\r\n", fps,fps_delay);
	}
	if (argc >= 3) {
		save_results =  atoi(argv[2]);
	}
	if (argc >= 4) {
		cnn_id =  atoi(argv[3]);
		if (cnn_id > 1) {
			cnn_id = 0;
		}
	}
	if (argc >= 5) {
		ddr_id =  atoi(argv[4]);
		if (ddr_id > 1) {
			ddr_id = 0;
		}
	}

	// init hdal
#if(!AI_IPC)
	UINT32 out_type = 1;
	ret = hd_common_init(0);
#else
	ret = hd_common_init(2);
#endif
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

    //set project config for AI
    hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine
#if(AI_IPC)
	init_share_memory();
#endif
	// init memory
	ret = mem_init();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

	ret = get_mem_block(ddr_id);
	if (ret != HD_OK) {
		DBG_ERR("mem_init fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();	// vdocap, vdoproc, vdoout
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

    ret = hd_gfx_init();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_init fail=%d\n", ret);
		goto exit;
	}

	// config extend engine plugin, process scheduler
    {
        UINT32 schd = VENDOR_AI_PROC_SCHD_FAIR;
        vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
        vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &schd);
    }

    ret = vendor_ai_init();
    if (ret != HD_OK) {
        printf("vendor_ai_init fail=%d\n", ret);
        return ret;
    }

#if(!AI_IPC)
	// open video_liveview modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&stream[0], &stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

    stream[1].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module_extend1(&stream[1], &stream[1].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	#if NN_FDCNN_FD_DRAW
	// must set once before hd_videoout_start
	if(init_mask_param(stream[0].mask_path0)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
	if(init_mask_param(stream[0].mask_path1)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path2)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}
    if(init_mask_param(stream[0].mask_path3)){
		printf("fail to set vo mask\r\n");
		goto exit;
	}

	ret = hd_videoout_start(stream[0].mask_path0);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path1);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path2);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	ret = hd_videoout_start(stream[0].mask_path3);
	if (ret != HD_OK) {
		printf("fail to start vo mask\n");
		goto exit;
	}
	#endif

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[0].out_ctrl, &stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[0].out_max_dim = stream[0].out_syscaps.output_dim;

	// set videocap parameter
	stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (alg)
	stream[0].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[0].proc_alg_max_dim.h = VDO_SIZE_H;
	ret = set_proc_param(stream[0].proc_alg_path, &stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	stream[1].proc_alg_max_dim.w = VDO_SIZE_W;
	stream[1].proc_alg_max_dim.h = VDO_SIZE_H;
    ret = set_proc_param_extend(stream[1].proc_alg_path, SOURCE_PATH, &stream[1].proc_alg_max_dim, HD_VIDEO_DIR_NONE);
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

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);
#else
	if (cnn_id == 0) {
		stream[0].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(6));
	} else {
		stream[0].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(7));
	}
	stream[1].proc_alg_path = HD_VIDEOPROC_PATH(HD_DAL_VIDEOPROC(0), HD_IN(0), HD_OUT(6));
#endif

#if(!AI_IPC)
	// start video_liveview modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	hd_videoproc_start(stream[0].proc_alg_path);
    hd_videoproc_start(stream[1].proc_alg_path);
    hd_videoproc_start(stream[2].proc_alg_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[0].out_path);
#endif
    VENDOR_AIS_FLOW_MEM_PARM local_mem = g_mem;

    // main process
#if NN_FDCNN_FD_MODE
    FDCNN_THREAD_PARM fd_thread_parm;
    pthread_t fd_thread_id;
	#if (!AI_IPC)
    UINT32 fd_mem_size = fdcnn_calcbuffsize(NN_FDCNN_FD_TYPE);
	#else
	UINT32 fd_mem_size = FD_MEM_SIZE;
	#endif
    fd_thread_parm.mem = fdcnn_getmem(&local_mem, fd_mem_size);
    fd_thread_parm.stream = stream[1];
	ret = pthread_create(&fd_thread_id, NULL, fdcnn_fd_thread, (VOID*)(&fd_thread_parm));

    if (ret < 0) {
        printf("create fdcnn fd thread failed");
        goto exit;
    }
#endif


#if NN_FDCNN_FD_DRAW
    FDCNN_THREAD_DRAW_PARM fdcnn_draw_parm;
    pthread_t fdcnn_draw_id;
    fdcnn_draw_parm.stream = stream[0];
    fdcnn_draw_parm.fd_mem = fd_thread_parm.mem;

    ret = pthread_create(&fdcnn_draw_id, NULL, fdcnn_draw_thread, (VOID*)(&fdcnn_draw_parm));
    if (ret < 0) {
        printf("create fdcnn draw thread failed");
        goto exit;
    }
#endif


#if NN_FDCNN_FD_MODE
    pthread_join(fd_thread_id, NULL);
#endif

#if NN_FDCNN_FD_DRAW
    pthread_join(fdcnn_draw_id, NULL);
#endif
#if(!AI_IPC)
	// stop video_liveview modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoproc_stop(stream[0].proc_alg_path);
    hd_videoproc_stop(stream[1].proc_alg_path);
    hd_videoproc_stop(stream[2].proc_alg_path);
	hd_videoout_stop(stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);
    //hd_videoproc_unbind(SOURCE_PATH);
    //hd_videoproc_unbind(EXTEND_PATH1);
    //hd_videoproc_unbind(EXTEND_PATH2);
#endif
exit:

#if(!AI_IPC)
	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

    ret = close_module_extend(&stream[1]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
#endif
	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

    ret = hd_gfx_uninit();
	if (ret != HD_OK) {
		DBG_ERR("hd_gfx_uninit fail=%d\n", ret);
	}

	ret = vendor_ai_uninit();
    if (ret != HD_OK) {
        printf("vendor_ai_uninit fail=%d\n", ret);
    }

	ret = release_mem_block();
	if (ret != HD_OK) {
		DBG_ERR("mem_uninit fail=%d\n", ret);
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
#if(AI_IPC)
	exit_share_memory();
#endif

	return 0;
}

