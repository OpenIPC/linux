/**
	@brief Demo code of 1 videocap path + 2 videoenc path + 1 videoout path.\n

	@file demo1.c

	@author iVOT/PSW

	@ingroup mhdal

	@note This file is modified from these sample code:
	      video_record_with_substream.c
	      video_liveview.c
	      video_liveview_with_3dnr.c
	      video_liveview_with_shdr.c
	      video_liveview_with_wdr_defog.c

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "demo2_int.h"
#include <string.h>
#include "vendor_isp.h"
#include "vendor_videoprocess.h"
#include "vendor_videoenc.h"
#include "pdcnn_lib_ai2.h"
#include "cddcnn_lib_ai2.h"
#include <pd_shm.h>

#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#include <sys/ipc.h>
#include <sys/shm.h>
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_video_capture_flow, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif


#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

typedef struct _COPY_INFO {
	UINT32 copy_size;
	UINT32 ddr_id;
	UINT32 sleep_us;
} COPY_INFO;

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

#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W	128
#define YOUT_WIN_NUM_H	128
#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL		1 //0: full, 1: subsample 1/2


///////////////////////////////////////////////////////////////////////////////

#define DISABLE			0
#define ENABLE			1

#define SEN_SEL_IMX290   0   //2M_SHDR
#define SEN_SEL_OS05A10  1   //5M_SHDR
#define SEN_SEL_GC4653   2   //4M
#define SEN_SEL_TC358840 3   //2M_HDMI in
#define SEN_SEL_SC401AI  4   //2M_HDMI in


#define SEN_SEL_OS02K10  5
#define SEN_SEL_AR0237IR 6
#define SEN_SEL_IMX415   7   //8M_SHDR
#define SEN_SEL_IMX317   8
#define SEN_SEL_IMX335   9
#define SEN_SEL_F37      10

#define VDO_SIZE_W_8M      3840
#define VDO_SIZE_H_8M      2160
#define VDO_SIZE_W_5M      2592
#define VDO_SIZE_H_5M      1944
#define VDO_SIZE_W_4M      2560
#define VDO_SIZE_H_4M      1440
#define VDO_SIZE_W_3M      2048
#define VDO_SIZE_H_3M      1536
#define VDO_SIZE_W_2M      1920
#define VDO_SIZE_H_2M      1080
#define VDO_SIZE_W_1M      1280
#define VDO_SIZE_H_1M       720
#define VDO_SIZE_W_VGA      648
#define VDO_SIZE_H_VGA      360
#define VDO_SIZE_W_400K     736
#define VDO_SIZE_H_400K     576

// AI size
#define VDO_SIZE_W_AI       1024
#define VDO_SIZE_H_AI        576



#define FUNC_OSG			ENABLE

#define FUNC_ISP			ENABLE

#define FUNC_DEFOG			DISABLE

#define FUNC_COLORNR		ENABLE

#define FUNC_WDR		    DISABLE

#define FUNC_GDC		    DISABLE

#define FUNC_WRITE_FILE		ENABLE

#define FUNC_SHDR			ENABLE

//#define H26X_COL_MV         DISABLE

#define H26X_LTR_MAIN       ENABLE

#define ONEBUF_SUB          ENABLE

#define NN_PDCNN_DRAW       ENABLE

static UINT32 vdo_size_w = VDO_SIZE_W_4M, vdo_size_h = VDO_SIZE_H_4M;
static UINT32 subvdo_size_w = VDO_SIZE_W_400K, subvdo_size_h = VDO_SIZE_H_400K;
static UINT32 sub2vdo_size_w = VDO_SIZE_W_AI, sub2vdo_size_h = VDO_SIZE_H_AI;

static UINT32 vdo_br     = (4 * 1024 * 1024);
static UINT32 subvdo_br  = (800 * 1024);
static UINT32 sub2vdo_br = (300 * 1024);

static UINT32 shdr_mode = 0;
static UINT32 sen_out_fmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
static UINT32 cap_out_fmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
static UINT32 sensor_sel = SEN_SEL_OS05A10;
static UINT32 g_capbind = 1;  //0:D2D, 1:direct, 2: one-buf, 0xff: no-bind
static UINT32 g_write_file = 1;
static UINT32 g_fps = 25;
static UINT32 g_enc_poll = 1;
static UINT32 g_one_buf = 1;    //0x0: vprc noraml, 0x1: one buffer
static UINT32 g_yuvcompress = 1;  //0x0: noraml, 0x1: yuv compress
static UINT32 g_osg = 0;  //0x0: disable, 0x1: osg enable
static CHAR *ai_mode;
static UINT32 pd_max_distance_mode = 2;
static UINT32 cdd_max_distance_mode = 0;



#define VDO_SIZE_W		vdo_size_w
#define VDO_SIZE_H		vdo_size_h
#define VDO_BR     		vdo_br

#define SUB_VDO_SIZE_W	subvdo_size_w
#define SUB_VDO_SIZE_H	subvdo_size_h
#define SUB_VDO_BR		subvdo_br

#define SUB2_VDO_SIZE_W	sub2vdo_size_w
#define SUB2_VDO_SIZE_H	sub2vdo_size_h
#define SUB2_VDO_BR		sub2vdo_br

#define SEN_OUT_FMT		sen_out_fmt
#define CAP_OUT_FMT		cap_out_fmt

#define STAMP_WIDTH     ALIGN_CEIL(vdo_size_w/2, 8)
#define STAMP_HEIGHT    ALIGN_CEIL(vdo_size_h/8, 8)
#define STAMP_PXLFMT	HD_VIDEO_PXLFMT_ARGB1555
#define STAMP_X   	    (0)
#define STAMP_Y  		(vdo_size_h*7/8)
#define STAMP_C 		((0x1 << 15) | (0x1f << 10) | (0x0f << 5) | (0x0f)) //red

#define SUB_STAMP_WIDTH     ALIGN_CEIL(subvdo_size_w/2, 8)
#define SUB_STAMP_HEIGHT    ALIGN_CEIL(subvdo_size_h/8, 8)
#define SUB_STAMP_PXLFMT	HD_VIDEO_PXLFMT_ARGB1555
#define SUB_STAMP_X        (0)
#define SUB_STAMP_Y  	   (subvdo_size_h*7/8)
#define SUB_STAMP_C 	   ((0x1 << 15) | (0x0f << 10) | (0x1f << 5) | (0x0f)) //green

#define VIDEOCAP_ALG_FUNC    (HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB | HD_VIDEOCAP_FUNC_WDR)
//#define VIDEOCAP_ALG_FUNC    ( HD_VIDEOCAP_FUNC_AWB | HD_VIDEOCAP_FUNC_WDR)

#define VIDEOPROC_ALG_FUNC   (HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_WDR)


static int    g_quit = 0;
static char   *g_shm = NULL;
static int    g_shmid = 0;

///////////////////////////////////////////////////////////////////////////////

void init_share_memory(void)
{
	int g_shmid = 0;
    key_t key;
	PD_SHM_INFO  *p_pd_shm;

    // Segment key.
    key = PD_SHM_KEY;
    // Create the segment.
    if( ( g_shmid = shmget( key, PD_SHMSZ, IPC_CREAT | 0666 ) ) < 0 ) {
        perror( "shmget" );
        exit(1);
    }
    // Attach the segment to the data space.
    if( ( g_shm = shmat( g_shmid, NULL, 0 ) ) == (char *)-1 ) {
        perror( "shmat" );
        exit(1);
    }
    // Initialization.
    memset(g_shm, 0, PD_SHMSZ );
	p_pd_shm = (PD_SHM_INFO  *)g_shm;
	p_pd_shm->enc_path[0].w = VDO_SIZE_W;
	p_pd_shm->enc_path[0].h = VDO_SIZE_H;
	p_pd_shm->enc_path[1].w = SUB_VDO_SIZE_W;
	p_pd_shm->enc_path[1].h = SUB_VDO_SIZE_H;
	p_pd_shm->pd_path.w = VDO_SIZE_W_AI;
	p_pd_shm->pd_path.h = VDO_SIZE_H_AI;
}

void exit_share_memory(void)
{
	if (g_shm) {
		shmdt(g_shm);
		shmctl(g_shmid, IPC_RMID, NULL);
	}
}

HD_RESULT mem_init(UINT32 CNN_MEM_SIZE)
{
	//#define CNN_MEM_SIZE      0x580000;

	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	UINT32 id;


	// config common pool (cap)
	id = 0;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	if (g_capbind == 1) {
		//direct ... NOT require raw
		mem_cfg.pool_info[0].blk_size = DBGINFO_BUFSIZE()
										+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
										+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	} else {
		mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(VDO_SIZE_W, VDO_SIZE_H, CAP_OUT_FMT)
										+VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
										+VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	}
	mem_cfg.pool_info[id].blk_cnt = 2;
	if (shdr_mode) {
		mem_cfg.pool_info[id].blk_cnt *= 2;
	}
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	// config common pool (main)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
    if (g_yuvcompress == 1) {
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_NVX_BUFSIZE(VDO_SIZE_W, ALIGN_CEIL_16(VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);
    } else {
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(VDO_SIZE_W, ALIGN_CEIL_16(VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);
    }
    //mem_cfg.pool_info[id].blk_cnt = 1;
	if (sensor_sel == SEN_SEL_TC358840) {
              mem_cfg.pool_info[id].blk_cnt = 3;
    } else {
               mem_cfg.pool_info[id].blk_cnt = 1;
    } 
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	// config common pool (sub)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	if (g_yuvcompress == 1) {
		mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_NVX_BUFSIZE(SUB_VDO_SIZE_W, ALIGN_CEIL_16(SUB_VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);
	} else {
		mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(SUB_VDO_SIZE_W, ALIGN_CEIL_16(SUB_VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);
	}
#if (ONEBUF_SUB == ENABLE)
    mem_cfg.pool_info[id].blk_cnt = 1;
#else
    mem_cfg.pool_info[id].blk_cnt = 3;
#endif
    mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	// config common pool (sub2)
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[id].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(ALIGN_CEIL_16(SUB2_VDO_SIZE_W), ALIGN_CEIL_16(SUB2_VDO_SIZE_H), HD_VIDEO_PXLFMT_YUV420);
	mem_cfg.pool_info[id].blk_cnt = 1;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
		// config common pool (osg main)
		id ++;
		mem_cfg.pool_info[id].type = HD_COMMON_MEM_USER_DEFINIED_POOL+0;
		mem_cfg.pool_info[id].blk_size = osg_query_buf_size(STAMP_WIDTH, STAMP_HEIGHT, STAMP_PXLFMT);
		mem_cfg.pool_info[id].blk_cnt = 1;
		mem_cfg.pool_info[id].ddr_id = DDR_ID0;
		// config common pool (osg sub)
		id ++;
		mem_cfg.pool_info[id].type = HD_COMMON_MEM_USER_DEFINIED_POOL+1;
		mem_cfg.pool_info[id].blk_size = osg_query_buf_size(SUB_STAMP_WIDTH, SUB_STAMP_HEIGHT, SUB_STAMP_PXLFMT);
		mem_cfg.pool_info[id].blk_cnt = 1;
		mem_cfg.pool_info[id].ddr_id = DDR_ID0;
    }
#endif
	id ++;
	mem_cfg.pool_info[id].type = HD_COMMON_MEM_CNN_POOL;
	mem_cfg.pool_info[id].blk_size = CNN_MEM_SIZE;
	mem_cfg.pool_info[id].blk_cnt = 1;
	mem_cfg.pool_info[id].ddr_id = DDR_ID0;
	ret = hd_common_mem_init(&mem_cfg);
	return ret;
}

HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

UINT32 mem_select(void)
{
	UINT32 ai_mode_buf_size=0;
	UINT32 pd_buf_size = 0x46f000;
	UINT32 fd_buf_size = 0x520000;
	UINT32 cdd_buf_size = 0x500000;
	UINT32 rslt_buf_size = 0xe100;
	UINT32 limit_fdet_buf_size = 0;

	UINT32 pd_mode = 1, cdd_mode = 1;

	if(pd_max_distance_mode > 0){
		pd_buf_size = 0x52f000;
	}	

	if(cdd_max_distance_mode > 0){
		cdd_buf_size = 0x5C0000;
	}
	
	if (strcmp(ai_mode, "100") == 0){
		ai_mode_buf_size = pd_buf_size;
		cdd_mode = 0;
	}
	else if (strcmp(ai_mode, "010") == 0){
		ai_mode_buf_size = fd_buf_size;
		pd_mode = 0;
		cdd_mode = 0;
	}
	else if (strcmp(ai_mode, "001") == 0){
		ai_mode_buf_size = cdd_buf_size;
		pd_mode = 0;
	}
	else if (strcmp(ai_mode, "110") == 0){
		ai_mode_buf_size = pd_buf_size + fd_buf_size;
		cdd_mode = 0;
	}
	else if (strcmp(ai_mode, "101") == 0){
		ai_mode_buf_size = pd_buf_size + cdd_buf_size;
	}
	else if (strcmp(ai_mode, "011") == 0){
		ai_mode_buf_size = fd_buf_size + cdd_buf_size;
		pd_mode = 0;
	}
	else if (strcmp(ai_mode, "111") == 0){
		ai_mode_buf_size = pd_buf_size + fd_buf_size + cdd_buf_size;
	}else{
		printf("No mode selected \n");
		return -1;
	}

	if ((LIMIT_FDET_PD && pd_mode) || (LIMIT_FDET_CDD && cdd_mode)){
		limit_fdet_buf_size = 0xcf000 ;
	}
	
	ai_mode_buf_size += rslt_buf_size;
	ai_mode_buf_size += limit_fdet_buf_size;
	
	return ai_mode_buf_size;
}
///////////////////////////////////////////////////////////////////////////////

HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl, HD_CTRL_ID ctrl_id)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	IQT_CFG_INFO cfg_info = {0};
	char  *chip_name = getenv("NVT_CHIP_ID");

	printf("chip_name = %s\r\n", chip_name);
	if (sensor_sel == SEN_SEL_IMX290) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
		printf("Using nvt_sen_imx290\n");
	} else if (sensor_sel == SEN_SEL_OS05A10) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
		printf("Using nvt_sen_os05a10\n");
	} else if (sensor_sel == SEN_SEL_GC4653) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_gc4653");
		printf("Using nvt_sen_gc4653\n");
	} else if (sensor_sel == SEN_SEL_TC358840) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ad_tc358840");
		printf("Using nvt_sen_ad_tc358840\n");
	}  else if (sensor_sel == SEN_SEL_SC401AI) {
		snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_sc401ai");
		printf("Using nvt_sen_sc401ai\n");
	}


	if (sensor_sel == SEN_SEL_TC358840) {
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x20 | 0x200; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0x1 | 0x100 | 0x200 | 0x400 | 0x800;//PIN_MIPI_LVDS_CFG_CLK0 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
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
		cap_cfg.sen_cfg.sen_dev.pin_cfg.ccir_vd_hd_pin = TRUE;
	} else {
		// MIPI interface
		cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux = 0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK

		//Sensor interface choice
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;//PIN_MIPI_LVDS_CFG_CLK0 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
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
	}
	if (shdr_mode) {
		cap_cfg.sen_cfg.shdr_map = HD_VIDEOCAP_SHDR_MAP(HD_VIDEOCAP_HDR_SENSOR1, (HD_VIDEOCAP_0|HD_VIDEOCAP_1));
	}
	ret = hd_videocap_open(0, ctrl_id, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
//	iq_ctl.func = VIDEOCAP_ALG_FUNC;
    if (sensor_sel == SEN_SEL_TC358840)
		iq_ctl.func =  HD_VIDEOCAP_FUNC_AWB | HD_VIDEOCAP_FUNC_WDR;
	else if (sensor_sel == SEN_SEL_GC4653)
		iq_ctl.func = HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB | HD_VIDEOCAP_FUNC_WDR;
	if (shdr_mode) {
		iq_ctl.func |= HD_VIDEOCAP_FUNC_SHDR;
	}
#if (FUNC_WDR == ENABLE)
	iq_ctl.func |= HD_VIDEOCAP_FUNC_WDR;
#endif
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;

	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}
	cfg_info.id = 0;
	if ((sensor_sel == SEN_SEL_IMX290) && (shdr_mode == 0)) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_imx290_0.cfg \n");
	} else if ((sensor_sel == SEN_SEL_IMX290) && (shdr_mode == 1)) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0_hdr.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_imx290_0_hdr.cfg \n");
	} else if ((sensor_sel == SEN_SEL_OS05A10) && (shdr_mode == 0)) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_os05a10_0.cfg \n");
	} else if ((sensor_sel == SEN_SEL_OS05A10) && (shdr_mode == 1)) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_hdr.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_os05a10_0_hdr.cfg \n");
	} else if (sensor_sel == SEN_SEL_IMX317) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_imx290_0.cfg \n");
	} else if (sensor_sel == SEN_SEL_IMX415) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_imx415_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_imx415_0.cfg \n");
	} else if (sensor_sel == SEN_SEL_GC4653) {
		strncpy(cfg_info.path, "/mnt/app/isp/isp_gc4653_0.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/isp_gc4653_0.cfg \n");
	}
	/*} else if (sensor_sel == SEN_SEL_GC4653) {
		strncpy(cfg_info.path, "/mnt/app/isp/test.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/test.cfg \n");
	}*/
	else if (sensor_sel == SEN_SEL_TC358840) {
		strncpy(cfg_info.path, "/mnt/app/isp/test.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/app/isp/test.cfg \n");

	}
	/*else if (sensor_sel == SEN_SEL_TC358840) {
		strncpy(cfg_info.path, "/mnt/sd/test.cfg", CFG_NAME_LENGTH);
		printf("Load /mnt/sd/test.cfg \n");
	}*/
	printf("cfg_info.path=%s \n",cfg_info.path);
	printf("shdr_mode = %d \r\n", shdr_mode);
	if (vendor_isp_set_ae(AET_ITEM_RLD_CONFIG, &cfg_info) != HD_OK) {
		printf("set ae fail\r\n");
		return -1;
	}
	if (vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info) != HD_OK) {
		printf("set awb fail\r\n");
		return -1;
	}
	if (vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info)!= HD_OK) {
		printf("set iq fail\r\n");
		return -1;
	}
	if (vendor_isp_uninit() != HD_OK) {
		return -1;
	}
	return ret;
}

HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim, HD_DIM *p_crop)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(g_fps,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		if (shdr_mode) {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_2;
		} else {
			video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		}
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_IN, &video_in_param);
		//printf("set_cap_param MODE=%d\r\n", ret);
		if (ret != HD_OK) {
			return ret;
		}
	}
	//no crop, full frame
	if ((p_crop->w == p_dim->w)&&(p_crop->h == p_dim->h)){
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_OFF;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
		//printf("set_cap_param CROP NONE=%d\r\n", ret);
	} else {
	//HD_CROP_ON
		HD_VIDEOCAP_CROP video_crop_param = {0};

		video_crop_param.mode = HD_CROP_ON;
		video_crop_param.win.rect.x = (p_dim->w - p_crop->w)/2;
		video_crop_param.win.rect.y = (p_dim->h - p_crop->h)/2;
		video_crop_param.win.rect.w = p_crop->w;
		video_crop_param.win.rect.h= p_crop->h;
		video_crop_param.align.w = 4;
		video_crop_param.align.h = 4;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}
	{
		HD_VIDEOCAP_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		//direct mode
		if (g_capbind == 1) //direct mode
			video_path_param.out_func = HD_VIDEOCAP_OUTFUNC_DIRECT;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_cap_param PATH_CONFIG=0x%X\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

HD_RESULT set_proc_cfg(HD_PATH_ID *p_video_proc_ctrl, HD_DIM* p_max_dim)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOPROC_DEV_CONFIG video_cfg_param = {0};
	HD_VIDEOPROC_CTRL video_ctrl_param = {0};
	HD_PATH_ID video_proc_ctrl = 0;

	ret = hd_videoproc_open(0, HD_VIDEOPROC_0_CTRL, &video_proc_ctrl); //open this for device control
	if (ret != HD_OK)
		return ret;

	if (p_max_dim != NULL ) {
		if (cap_out_fmt == HD_VIDEO_PXLFMT_YUV420) {
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_YUVALL;
		} else {
			video_cfg_param.pipe = HD_VIDEOPROC_PIPE_RAWALL;
		}
		#if (FUNC_ISP == ENABLE)
		video_cfg_param.isp_id = 0;
		#else
		video_cfg_param.isp_id = HD_ISP_DONT_CARE;
		#endif
		video_cfg_param.ctrl_max.func = VIDEOPROC_ALG_FUNC;
		if (shdr_mode) {
			video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_SHDR;
		}
#if (FUNC_DEFOG == ENABLE)
		video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_DEFOG;
#endif
#if (FUNC_COLORNR == ENABLE)
		video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_COLORNR;
#endif
#if (FUNC_WDR == ENABLE)
		video_cfg_param.ctrl_max.func |= HD_VIDEOPROC_FUNC_WDR;
#endif
		video_cfg_param.in_max.func = 0;
		video_cfg_param.in_max.dim.w = p_max_dim->w;
		video_cfg_param.in_max.dim.h = p_max_dim->h;
		video_cfg_param.in_max.pxlfmt = CAP_OUT_FMT;
		video_cfg_param.in_max.frc = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_DEV_CONFIG, &video_cfg_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}
	{
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.in_func = 0;
		if (g_capbind == 1) //direct mode
			video_path_param.in_func |= HD_VIDEOPROC_INFUNC_DIRECT; //direct NOTE: enable direct
		ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
		//printf("set_proc_param PATH_CONFIG=0x%X\r\n", ret);
	}


	video_ctrl_param.func = VIDEOPROC_ALG_FUNC;
	if (shdr_mode) {
		video_ctrl_param.func |= HD_VIDEOPROC_FUNC_SHDR;
	}
#if (FUNC_DEFOG == ENABLE)
	video_ctrl_param.func |= HD_VIDEOPROC_FUNC_DEFOG;
#endif
#if (FUNC_COLORNR == ENABLE)
	video_ctrl_param.func |= HD_VIDEOPROC_FUNC_COLORNR;
#endif
#if (FUNC_WDR == ENABLE)
	video_ctrl_param.func |= HD_VIDEOPROC_FUNC_WDR;
#endif
	video_ctrl_param.ref_path_3dnr = HD_VIDEOPROC_0_OUT_4;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);
	if (ret != HD_OK) {
		return HD_ERR_NG;
	}
	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, HD_VIDEO_PXLFMT pxlfmt, BOOL pull,UINT32 b_one_buf, UINT32 out_order)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = pxlfmt;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = pull ? 1 : 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
		if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

    {
		HD_VIDEOPROC_FUNC_CONFIG video_path_param = {0};

		video_path_param.out_func = 0;
		if (b_one_buf == 1)
			video_path_param.out_func |= HD_VIDEOPROC_OUTFUNC_ONEBUF;
		video_path_param.out_order = out_order;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_FUNC_CONFIG, &video_path_param);
        if (ret != HD_OK) {
			return HD_ERR_NG;
		}
	}

	if (0) /*p_dim != NULL && p_dim->h & 0xF)*/ {
		UINT32 h_align;

		h_align = 16;
		ret = vendor_videoproc_set(video_proc_path, VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN, &h_align);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN failed!(%d)\r\n", ret);
		}
	}
	return ret;
}


static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_PATH_ID src_path, HD_URECT* p_crop, HD_DIM* p_dim, BOOL pull, UINT32 dir)
{
	HD_RESULT ret = HD_OK;

	if (p_crop != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_CROP video_out_param = {0};
		video_out_param.mode = HD_CROP_ON;
		video_out_param.win.rect.x = p_crop->x;
		video_out_param.win.rect.y = p_crop->y;
		video_out_param.win.rect.w = p_crop->w;
		video_out_param.win.rect.h = p_crop->h;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX_CROP, &video_out_param);
	} else {
		HD_VIDEOPROC_CROP video_out_param = {0};
		video_out_param.mode = HD_CROP_OFF;
		video_out_param.win.rect.x = 0;
		video_out_param.win.rect.y = 0;
		video_out_param.win.rect.w = 0;
		video_out_param.win.rect.h = 0;
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX_CROP, &video_out_param);
	}

	if (ret != HD_OK) {
		return ret;
	}

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = dir;
		video_out_param.depth = pull ? 1 : 0;

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = 0; //auto reference to downstream's in dim.w
		video_out_param.dim.h = 0; //auto reference to downstream's in dim.h
		video_out_param.pxlfmt = 0; //auto reference to downstream's in pxlfmt
		video_out_param.dir = dir;
		video_out_param.depth = pull ? 1 : 0;

		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	}
	if (0) /*p_dim != NULL && p_dim->h & 0xF)*/ {
		UINT32 h_align;

		h_align = 16;
		ret = vendor_videoproc_set(video_proc_path, VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN, &h_align);
		if (ret != HD_OK) {
			printf("VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN failed!(%d)\r\n", ret);
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

HD_RESULT set_enc_cfg(HD_PATH_ID video_enc_path, HD_DIM *p_max_dim, UINT32 max_bitrate, INT32 enc_type, BOOL ltr_en, BOOL onebuf_en)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_PATH_CONFIG video_path_config = {0};
	HD_VIDEOENC_FUNC_CONFIG video_func_config = {0};

	//#if (H26X_COL_MV == DISABLE)
	VENDOR_VIDEOENC_H26X_ENC_COLMV h26x_enc_colmv = {0};
	h26x_enc_colmv.h26x_colmv_en = 0;
	vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_COLMV, &h26x_enc_colmv);
	//#endif
	{
		VENDOR_VIDEOENC_FIT_WORK_MEMORY fit_work_mem_cfg = {0};
		fit_work_mem_cfg.b_enable = TRUE;
		vendor_videoenc_set(video_enc_path, VENDOR_VIDEOENC_PARAM_OUT_FIT_WORK_MEMORY, &fit_work_mem_cfg);
	}
	if (p_max_dim != NULL) {

		//--- HD_VIDEOENC_PARAM_PATH_CONFIG ---
		if (enc_type == 0)
			video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H265;
		else if (enc_type == 1)
			video_path_config.max_mem.codec_type = HD_CODEC_TYPE_H264;
		else if (enc_type == 2)
			video_path_config.max_mem.codec_type = HD_CODEC_TYPE_JPEG;

		video_path_config.max_mem.max_dim.w  = p_max_dim->w;
		video_path_config.max_mem.max_dim.h  = p_max_dim->h;
		video_path_config.max_mem.bitrate    = max_bitrate;
		if (g_one_buf == 1) {  //for small buffer
			video_path_config.max_mem.enc_buf_ms = 2700;
			video_path_config.max_mem.svc_layer  = HD_SVC_DISABLE;
		} else {
			video_path_config.max_mem.enc_buf_ms = 3000;
			video_path_config.max_mem.svc_layer  = HD_SVC_4X;
		}
		video_path_config.max_mem.ltr        = ltr_en;
		video_path_config.max_mem.rotate     = FALSE;
		video_path_config.max_mem.source_output   = FALSE;
		video_path_config.isp_id             = 0;
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_PATH_CONFIG, &video_path_config);
		if (ret != HD_OK) {
			printf("set_enc_path_config = %d\r\n", ret);
			return HD_ERR_NG;
		}
	}

	video_func_config.in_func = 0;
	if (g_one_buf == 1 && onebuf_en == TRUE) {
		video_func_config.in_func |= HD_VIDEOENC_INFUNC_ONEBUF;
	}

	ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_FUNC_CONFIG, &video_func_config);
	if (ret != HD_OK) {
		printf("set_enc_path_config = %d\r\n", ret);
		return HD_ERR_NG;
	}

	return ret;
}

HD_RESULT set_enc_param(HD_PATH_ID video_enc_path, HD_DIM *p_dim, HD_VIDEO_PXLFMT pxlfmt, INT32 enc_type, UINT32 bitrate, BOOL ltr_en)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_IN  video_in_param = {0};
	HD_VIDEOENC_OUT video_out_param = {0};
	HD_H26XENC_RATE_CONTROL rc_param = {0};

	if (p_dim != NULL) {
		//--- HD_VIDEOENC_PARAM_IN ---
		video_in_param.dir           = HD_VIDEO_DIR_NONE;
		video_in_param.pxl_fmt = pxlfmt ? pxlfmt : HD_VIDEO_PXLFMT_YUV420;
		video_in_param.dim.w   = p_dim->w;
		video_in_param.dim.h   = p_dim->h;
		video_in_param.frc     = HD_VIDEO_FRC_RATIO(1,1);
		ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_IN, &video_in_param);
		if (ret != HD_OK) {
			printf("set_enc_param_in = %d\r\n", ret);
			return ret;
		}
		if (enc_type == -1) {
            printf("enc_type = %d not encode\r\n", enc_type);
			return HD_OK;
		}
		printf("enc_type=%d\r\n", enc_type);
		if (enc_type == 0) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H265;
			video_out_param.h26x.profile       = HD_H265E_MAIN_PROFILE;
			video_out_param.h26x.level_idc     = HD_H265E_LEVEL_5;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = 0;
			video_out_param.h26x.ltr_interval  = (ltr_en == TRUE)? 6:0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H265E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = g_fps;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;

			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
			{
				HD_H26XENC_VUI vui_cfg = {0};
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_cfg);
				vui_cfg.vui_en = 1;
				hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_cfg);
			}
		} else if (enc_type == 1) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_H264;
			video_out_param.h26x.profile       = HD_H264E_HIGH_PROFILE;
			video_out_param.h26x.level_idc     = HD_H264E_LEVEL_5_1;
			video_out_param.h26x.gop_num       = 15;
			video_out_param.h26x.ltr_interval  = (ltr_en == TRUE)? 6:0;
			video_out_param.h26x.ltr_pre_ref   = 0;
			video_out_param.h26x.gray_en       = 0;
			video_out_param.h26x.source_output = 0;
			video_out_param.h26x.svc_layer     = HD_SVC_DISABLE;
			video_out_param.h26x.entropy_mode  = HD_H264E_CABAC_CODING;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

			//--- HD_VIDEOENC_PARAM_OUT_RATE_CONTROL ---
			rc_param.rc_mode             = HD_RC_MODE_CBR;
			rc_param.cbr.bitrate         = bitrate;
			rc_param.cbr.frame_rate_base = g_fps;
			rc_param.cbr.frame_rate_incr = 1;
			rc_param.cbr.init_i_qp       = 26;
			rc_param.cbr.min_i_qp        = 10;
			rc_param.cbr.max_i_qp        = 45;
			rc_param.cbr.init_p_qp       = 26;
			rc_param.cbr.min_p_qp        = 10;
			rc_param.cbr.max_p_qp        = 45;
			rc_param.cbr.static_time     = 4;
			rc_param.cbr.ip_weight       = 0;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param);
			if (ret != HD_OK) {
				printf("set_enc_rate_control = %d\r\n", ret);
				return ret;
			}
			{
				HD_H26XENC_VUI vui_cfg = {0};
				hd_videoenc_get(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_cfg);
				vui_cfg.vui_en = 1;
				hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_VUI, &vui_cfg);
			}
		} else if (enc_type == 2) {

			//--- HD_VIDEOENC_PARAM_OUT_ENC_PARAM ---
			video_out_param.codec_type         = HD_CODEC_TYPE_JPEG;
			video_out_param.jpeg.retstart_interval = 0;
			video_out_param.jpeg.image_quality = 50;
			ret = hd_videoenc_set(video_enc_path, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &video_out_param);
			if (ret != HD_OK) {
				printf("set_enc_param_out = %d\r\n", ret);
				return ret;
			}

		} else {

			printf("not support enc_type\r\n");
			return HD_ERR_NG;
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT set_acap_cfg(HD_PATH_ID *p_audio_cap_ctrl, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_DEV_CONFIG audio_cfg_param = {0};
	HD_AUDIOCAP_DRV_CONFIG audio_driver_cfg_param = {0};
	HD_AUDIOCAP_VOLUME audio_vol_param = {0};
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
	audio_cfg_param.frame_num_max = 50;
	audio_cfg_param.out_max.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DEV_CONFIG, &audio_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	/*set audio capture driver parameters*/
	audio_driver_cfg_param.mono = HD_AUDIO_MONO_RIGHT;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_DRV_CONFIG, &audio_driver_cfg_param);
	if (ret != HD_OK) {
		return ret;
	}

	audio_vol_param.volume = 100;
	ret = hd_audiocap_set(audio_cap_ctrl, HD_AUDIOCAP_PARAM_VOLUME, &audio_vol_param);
	if (ret != HD_OK) {
		return ret;
	}

	*p_audio_cap_ctrl = audio_cap_ctrl;

	return ret;
}

static HD_RESULT set_acap_param(HD_PATH_ID audio_cap_path, HD_AUDIO_SR sample_rate)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIOCAP_IN audio_cap_in_param = {0};
	HD_AUDIOCAP_OUT audio_cap_out_param = {0};

	// set hd_audiocapture input parameters
	audio_cap_in_param.sample_rate = sample_rate;
	audio_cap_in_param.sample_bit = HD_AUDIO_BIT_WIDTH_16;
	audio_cap_in_param.mode = HD_AUDIO_SOUND_MODE_MONO;
	audio_cap_in_param.frame_sample = 1024;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_IN, &audio_cap_in_param);
	if (ret != HD_OK) {
		return ret;
	}

	// set hd_audiocapture output parameters
	audio_cap_out_param.sample_rate = 0;
	ret = hd_audiocap_set(audio_cap_path, HD_AUDIOCAP_PARAM_OUT, &audio_cap_out_param);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
    if ((ret = hd_videoenc_init()) != HD_OK)
		return ret;
	if ((ret = hd_audiocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_gfx_init()) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT open_module(DEMO *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	// set videocap config
	ret = set_cap_cfg(&p_stream->cap_ctrl, HD_VIDEOCAP_0_CTRL);
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
	// set audiocap config
	ret = set_acap_cfg(&p_stream->acap_ctrl, p_stream->acap_sr_max);
	if (ret != HD_OK) {
		printf("set acap-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_4, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_0, HD_VIDEOENC_0_OUT_0, &p_stream->enc_path)) != HD_OK)
		return ret;
	if ((ret = hd_audiocap_open(HD_AUDIOCAP_0_IN_0, HD_AUDIOCAP_0_OUT_0, &p_stream->acap_path)) != HD_OK)
			return ret;

	return HD_OK;
}

HD_RESULT open_module_2(DEMO *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;

	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0,  &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_open(HD_VIDEOENC_0_IN_1, HD_VIDEOENC_0_OUT_1,  &p_stream->enc_path)) != HD_OK)
		return ret;

	return HD_OK;
}


static HD_RESULT open_module_3(DEMO *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_path)) != HD_OK)  // out 5~15 is extend
		return ret;
	return HD_OK;
}

// open extend path 6 for AI
static HD_RESULT open_module_4(DEMO *p_stream, HD_DIM* p_proc_max_dim)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_6, &p_stream->proc_path)) != HD_OK)  // out 5~15 is extend
		return ret;
	return HD_OK;
}

HD_RESULT close_module(DEMO *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_close(p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	if ((ret = hd_audiocap_close(p_stream->acap_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT close_module_2(DEMO *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT close_module_3(DEMO *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_close(p_stream->enc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT close_module_4(DEMO *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	return HD_OK;
}

HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoenc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_audiocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_gfx_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}



static void *encode_thread(void *arg)
{
	DEMO* p_stream0 = (DEMO *)arg;
	DEMO* p_stream1 = p_stream0 + 1;
	HD_RESULT ret = HD_OK;
	HD_VIDEOENC_BS  data_pull;
	UINT32 j;

	UINT32 vir_addr_main;
	HD_VIDEOENC_BUFINFO phy_buf_main;
	char file_path_main[32] = "/mnt/sd/dump_bs_main.dat";
	FILE *f_out_main;
	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))
	UINT32 vir_addr_sub;
	HD_VIDEOENC_BUFINFO phy_buf_sub;
	char file_path_sub[32]  = "/mnt/sd/dump_bs_sub.dat";
	FILE *f_out_sub;
	#define PHY2VIRT_SUB(pa) (vir_addr_sub + (pa - phy_buf_sub.buf_info.phy_addr))
	HD_VIDEOENC_POLL_LIST poll_list[2];
	UINT32 poll_num = 2;

	//------ wait flow_start ------
	while (p_stream0->flow_start == 0) sleep(1);

	// query physical address of bs buffer ( this can ONLY query after hd_videoenc_start() is called !! )
	hd_videoenc_get(p_stream0->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_main);
	hd_videoenc_get(p_stream1->enc_path, HD_VIDEOENC_PARAM_BUFINFO, &phy_buf_sub);

	// mmap for bs buffer (just mmap one time only, calculate offset to virtual address later)
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);
	vir_addr_sub  = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_sub.buf_info.phy_addr, phy_buf_sub.buf_info.buf_size);

	//----- open output files -----
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_main);
	} else {
		printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}
	if ((f_out_sub = fopen(file_path_sub, "wb")) == NULL) {
		HD_VIDEOENC_ERR("open file (%s) fail....\r\n", file_path_sub);
	} else {
		printf("\r\ndump sub  bitstream to file (%s) ....\r\n", file_path_sub);
	}

	printf("\r\nif you want to stop, enter \"q\" to exit !!\r\n\r\n");

	//--------- pull data test ---------
	poll_list[0].path_id = p_stream0->enc_path;
	poll_list[1].path_id = p_stream1->enc_path;


	//--------- pull data test ---------
	while (p_stream0->enc_exit == 0) {
		if (g_enc_poll && HD_OK == hd_videoenc_poll_list(poll_list, poll_num, -1)) {    // multi path poll_list , -1 = blocking mode
			if (TRUE == poll_list[0].revent.event) {
				//pull data
				ret = hd_videoenc_pull_out_buf(p_stream0->enc_path, &data_pull, -1); // -1 = blocking mode

				if (ret == HD_OK) {
					for (j=0; j< data_pull.pack_num; j++) {
						#if FUNC_WRITE_FILE
						UINT8 *ptr = (UINT8 *)PHY2VIRT_MAIN(data_pull.video_pack[j].phy_addr);
						UINT32 len = data_pull.video_pack[j].size;
						if (g_write_file) {
							if (f_out_main) fwrite(ptr, 1, len, f_out_main);
							if (f_out_main) fflush(f_out_main);
						}
						#endif
					}

					// release data
					ret = hd_videoenc_release_out_buf(p_stream0->enc_path, &data_pull);
				}
			}

			if (TRUE == poll_list[1].revent.event) {
				//pull data
				ret = hd_videoenc_pull_out_buf(p_stream1->enc_path, &data_pull, -1); // -1 = blocking mode
				if (ret == HD_OK) {
					for (j=0; j< data_pull.pack_num; j++) {
						#if FUNC_WRITE_FILE
						UINT8 *ptr = (UINT8 *)PHY2VIRT_SUB(data_pull.video_pack[j].phy_addr);
						UINT32 len = data_pull.video_pack[j].size;
						if (g_write_file) {
							if (f_out_sub) fwrite(ptr, 1, len, f_out_sub);
							if (f_out_sub) fflush(f_out_sub);
						}
						#endif
					}

					// release data
					ret = hd_videoenc_release_out_buf(p_stream1->enc_path, &data_pull);
				}
			}
		} else {
			usleep(100000);
		}
	}

	// mummap for bs buffer
	hd_common_mem_munmap((void *)vir_addr_main, phy_buf_main.buf_info.buf_size);
	hd_common_mem_munmap((void *)vir_addr_sub, phy_buf_sub.buf_info.buf_size);

	// close output file
	if (f_out_main) fclose(f_out_main);
	if (f_out_sub) fclose(f_out_sub);

	return 0;
}


static void *acapture_thread(void *arg)
{
	HD_RESULT ret = HD_OK;
	HD_AUDIO_FRAME  data_pull;
	UINT32 vir_addr_main;
	HD_AUDIOCAP_BUFINFO phy_buf_main;
	char file_path_main[64], file_path_len[64];
	FILE *f_out_main, *f_out_len;
	DEMO* p_cap_only = (DEMO *)arg;

	#define PHY2VIRT_MAIN(pa) (vir_addr_main + (pa - phy_buf_main.buf_info.phy_addr))

	/* config pattern name */
	snprintf(file_path_main, sizeof(file_path_main), "/mnt/sd/audio_bs_%d_%d_%d_pcm.dat", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->acap_sr);
	snprintf(file_path_len, sizeof(file_path_len), "/mnt/sd/audio_bs_%d_%d_%d_pcm.len", HD_AUDIO_BIT_WIDTH_16, HD_AUDIO_SOUND_MODE_MONO, p_cap_only->acap_sr);

	/* wait flow_start */
	while (p_cap_only->acap_enter == 0) {
		sleep(1);
	}

	/* query physical address of bs buffer
	  (this can ONLY query after hd_audiocap_start() is called !!) */
	hd_audiocap_get(p_cap_only->acap_ctrl, HD_AUDIOCAP_PARAM_BUFINFO, &phy_buf_main);

	/* mmap for bs buffer
	  (just mmap one time only, calculate offset to virtual address later) */
	vir_addr_main = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, phy_buf_main.buf_info.phy_addr, phy_buf_main.buf_info.buf_size);

	if (vir_addr_main == 0) {
		printf("acap mmap error\r\n");
		return 0;
	}

	/* open output files */
	if ((f_out_main = fopen(file_path_main, "wb")) == NULL) {
		printf("open file (%s) fail....\r\n", file_path_main);
	} else {
		//printf("\r\ndump main bitstream to file (%s) ....\r\n", file_path_main);
	}

	if ((f_out_len = fopen(file_path_len, "wb")) == NULL) {
		printf("open len file (%s) fail....\r\n", file_path_len);
	}

	/* pull data test */
	while (p_cap_only->acap_exit == 0) {
		// pull data
		ret = hd_audiocap_pull_out_buf(p_cap_only->acap_path, &data_pull, -1); // >1 = timeout mode

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
			ret = hd_audiocap_release_out_buf(p_cap_only->acap_path, &data_pull);
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

static void *memcpy_thread(void *arg)
{
	void                 *va1, *va2;
	UINT32               pa1, pa2;
	UINT32               size = 0x400000;
	HD_RESULT            ret;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	UINT32               ts, te;
	UINT32               total_size = 0;
	BOOL                 is_print = FALSE;
	COPY_INFO            *info = (COPY_INFO *)arg;


	if (NULL != info) {
		size = info->copy_size;
	}
	ret = hd_common_mem_alloc("test1", &pa1, (void **)&va1, size, info->ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", size, ddr_id+1);
		return 0;
	}
	ret = hd_common_mem_alloc("test2", &pa2, (void **)&va2, size, info->ddr_id);
	if (ret != HD_OK) {
		printf("err:alloc size 0x%x, ddr %d\r\n", size, ddr_id+1);
		return 0;
	}
	ts = hd_gettime_ms();
	while (!g_quit) {
		memcpy(va1, va2, size);
		if (info->sleep_us) {
			usleep(info->sleep_us);
		}
		total_size += size;
		te = hd_gettime_ms();
		if (!is_print && te - ts > 5000) {
			printf("total_size = 0x%x, time = %d ms, perf = %f MB/s\r\n", (unsigned int)total_size, te - ts, (float)total_size/(te - ts)/1024);
			is_print = TRUE;
		}
	}
	ret = hd_common_mem_free(pa1, va1);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa1, (int)va1);
		return 0;
	}
	ret = hd_common_mem_free(pa2, va2);
	if (ret != HD_OK) {
		printf("err:free pa = 0x%x, va = 0x%x\r\n", (int)pa2, (int)va2);
		return 0;
	}
	return 0;
}

int main(int argc, char** argv)
{
	HD_RESULT ret;
	INT key;
	int save_ai_result = 0;
	DEMO stream[6] = {0}; //0: main stream, 1: sub stream, 2: disp stream, 3:sub2 stream, 4:AI path
	INT32 enc_type = 0;
	INT32 enc2_type = 0;
	HD_DIM main_dim;
	HD_DIM sub_dim;
	HD_DIM sub2_dim;
	int    main_strem_size = 0;
	int    sub_strem_size =  0;
	int    sub2_strem_size = 0;
	int    cap_size_w = 0;
	int    cap_size_h = 0;
	PD_SHM_INFO  *p_pd_shm;
#if (H26X_LTR_MAIN == ENABLE)
	BOOL   main_ltr_en = TRUE;
#else
	BOOL   main_ltr_en = FALSE;
#endif
	BOOL   sub_ltr_en = FALSE;
	pthread_t  memcpy_thread_id;
	COPY_INFO  copy_info[4];
	BOOL       is_start_memcpy_test = FALSE;
	CHAR command[256];



	if (argc == 1) {
		printf("Usage: <main enc_type> <sub enc_type> <display type> <enable sub2> <sub2 enc_type>.\r\n");
		printf("Help:\r\n");
		printf("  <main stream enc_type> : 0(H265), 1(H264), 2(MJPG)\r\n");
		printf("  <sub stream enc_type>  : 0(H265), 1(H264), 2(MJPG)\r\n");
		printf("  <main streamsize> : 3(3M),4(4M),5(5M)\r\n");
		printf("  <sub streamsize> : 2(2M),1(1M),400(400k),300(300k)\r\n");
		printf("  <sub2 streamsize> : 2(2M),1(1M),400(400k),300(300k)\r\n");
		printf("  <enable shdr>  : 0(disable), 1(enable)\r\n");
		printf("  <fps>  : 20,25,30\r\n");
		printf("  <yuvcompress> :  0(disable), 1(enable)\r\n");
		printf("  <osg> :  0(disable), 1(enable)\r\n");
		printf("  <sensor> :  0(IMX290) , 1(OS05A), 2(GC4653)\r\n");
		return 0;
	}
	// query program options
	if (argc >= 2) {
		enc_type = atoi(argv[1]);
		printf("enc_type %d\r\n", enc_type);
		if(enc_type > 2) {
			printf("error: not support enc_type!\r\n");
			return 0;
		}
	}
	if (argc >= 3) {
		enc2_type = atoi(argv[2]);
		printf("enc2_type %d\r\n", enc2_type);
		if(enc2_type > 2) {
			printf("error: not support enc2_type!\r\n");
			return 0;
		}
	}
	if (argc >= 4) {
		main_strem_size = atoi(argv[3]);
		printf("main stream size %d\r\n", main_strem_size);
	}
	if (argc >= 5) {
		sub_strem_size = atoi(argv[4]);
		printf("sub_strem_size %d\r\n", sub_strem_size);
	}
	if (argc >= 6) {
		sub2_strem_size = atoi(argv[5]);
		printf("sub2_strem_size %d\r\n", sub2_strem_size);
	}
	if (argc >= 7) {
		shdr_mode = atoi(argv[6]);
		printf("shdr_mode %d\r\n", shdr_mode);
	}
	if (argc >= 8) {
		g_fps = atoi(argv[7]);
		printf("g_fps %d\r\n", g_fps);
	}
	if (argc >= 9) {
		g_yuvcompress = atoi(argv[8]);
		printf("g_yuvcompress %d\r\n", g_yuvcompress);
	}
	if (argc >= 10) {
		g_osg = atoi(argv[9]);
		printf("g_osg %d\r\n", g_osg);
	}
	if (argc >= 11) {
		sensor_sel = atoi(argv[10]);
		printf("sensor_sel %d\r\n", sensor_sel);
	}
	
	if (argc >= 12) {
		ai_mode = argv[11];
		printf("ai_mode %s\r\n", ai_mode);
	}
	if (argc >= 13) {
		pd_max_distance_mode= atoi(argv[12]);
		printf("pd_max_distance_mode %d\r\n", pd_max_distance_mode);
	}
	if (argc >= 14) {
		cdd_max_distance_mode= atoi(argv[13]);
		printf("cdd_max_distance_mode %d\r\n", cdd_max_distance_mode);
	}
	if (argc >= 15) {
		save_ai_result = atoi(argv[14]);
		printf("Whether to save the PD, FD or CDD results: %d\r\n", save_ai_result);
	}
	

	if (sensor_sel == SEN_SEL_IMX290) {
		cap_size_w = VDO_SIZE_W_2M;
		cap_size_h = VDO_SIZE_H_2M;
	} else if (sensor_sel == SEN_SEL_OS05A10) {
		cap_size_w = VDO_SIZE_W_5M;
		cap_size_h = VDO_SIZE_H_5M;
	} else if (sensor_sel == SEN_SEL_GC4653) {
		cap_size_w = VDO_SIZE_W_4M;
		cap_size_h = VDO_SIZE_H_4M;
	} else if (sensor_sel == SEN_SEL_TC358840) {
		cap_size_w = VDO_SIZE_W_2M;
		cap_size_h = VDO_SIZE_H_2M;
		g_capbind = 0;
	} else if (sensor_sel == SEN_SEL_SC401AI) {
		cap_size_w = VDO_SIZE_W_4M;
		cap_size_h = VDO_SIZE_H_4M;
	}
	if (main_strem_size != 0) {
		switch (main_strem_size) {
			case 5:
				vdo_size_w = VDO_SIZE_W_5M;
				vdo_size_h = VDO_SIZE_H_5M;
				vdo_br = 5 * 1024 *1024;
				subvdo_size_w = VDO_SIZE_W_2M;
				subvdo_size_h = VDO_SIZE_H_2M;
				subvdo_br = 2 * 1024 * 1024;
				break;
			case 4:
				vdo_size_w = VDO_SIZE_W_4M;
				vdo_size_h = VDO_SIZE_H_4M;
				vdo_br = 4 * 1024 *1024;
				subvdo_size_w = VDO_SIZE_W_1M;
				subvdo_size_h = VDO_SIZE_H_1M;
				subvdo_br = 1024 * 1024;
				break;
			case 3:
				vdo_size_w = VDO_SIZE_W_3M;
				vdo_size_h = VDO_SIZE_H_3M;
				vdo_br = 3 * 1024 *1024;
				subvdo_size_w = VDO_SIZE_W_1M;
				subvdo_size_h = VDO_SIZE_H_1M;
				subvdo_br = 1024 * 1024;
				break;
			case 2:
				vdo_size_w = VDO_SIZE_W_2M;
				vdo_size_h = VDO_SIZE_H_2M;
				vdo_br = 2 * 1024 *1024;
				subvdo_size_w = VDO_SIZE_W_1M;
				subvdo_size_h = VDO_SIZE_H_1M;
				subvdo_br = 1024 *1024;
				break;
			default:

				break;
		}
	}

    if (sub_strem_size != 0) {
		switch (sub_strem_size) {
    		case 2:
				subvdo_size_w = VDO_SIZE_W_2M;
				subvdo_size_h = VDO_SIZE_H_2M;
				subvdo_br = 2 * 1024 * 1024;
				break;
    		case 1:
				subvdo_size_w = VDO_SIZE_W_1M;
				subvdo_size_h = VDO_SIZE_H_1M;
				subvdo_br = 1024 *1024;
				break;
    		case 400:
				subvdo_size_w = VDO_SIZE_W_400K;
				subvdo_size_h = VDO_SIZE_H_400K;
				subvdo_br = (800 * 1024);
				break;
    		case 300:
				subvdo_size_w = VDO_SIZE_W_VGA;
				subvdo_size_h = VDO_SIZE_W_VGA;
				subvdo_br = (600 * 1024);
				break;
			default:
				break;
		}
	}


    if (sub2_strem_size != 0) {
		switch (sub2_strem_size) {
    		case 2:
				sub2vdo_size_w = VDO_SIZE_W_2M;
				sub2vdo_size_h = VDO_SIZE_H_2M;
				sub2vdo_br = 2 * 1024 * 1024;
				break;
    		case 1:
				sub2vdo_size_w = VDO_SIZE_W_1M;
				sub2vdo_size_h = VDO_SIZE_H_1M;
				sub2vdo_br = 1024 *1024;
				break;
    		case 400:
				sub2vdo_size_w = VDO_SIZE_W_400K;
				sub2vdo_size_h = VDO_SIZE_H_400K;
				sub2vdo_br = (800 * 1024);
				break;
    		case 300:
				sub2vdo_size_w = VDO_SIZE_W_VGA;
				sub2vdo_size_h = VDO_SIZE_W_VGA;
				sub2vdo_br = (600 * 1024);
				break;
			default:
				break;
		}
    }

    printf("main %d*%d sub %d*%d sub2 %d*%d\r\n",vdo_size_w,vdo_size_h,subvdo_size_w,subvdo_size_h,sub2vdo_size_w,sub2vdo_size_h);


	if (shdr_mode) {
		sen_out_fmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
		cap_out_fmt = HD_VIDEO_PXLFMT_NRX12_SHDR2;
	} else {
		if (sensor_sel == SEN_SEL_TC358840) {
			sen_out_fmt = HD_VIDEO_PXLFMT_YUV422;
			cap_out_fmt = HD_VIDEO_PXLFMT_YUV420;
		} else {
			sen_out_fmt = HD_VIDEO_PXLFMT_RAW12;
			cap_out_fmt = HD_VIDEO_PXLFMT_NRX12;
		}
	}

	//
	init_share_memory();
	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
		goto exit;
	}

	// init memory
	/* select ai_mode buf. */
	UINT32 ai_mode_buf_size = 0;
	//pdcnn/fdcnn/cddcnn demo require mem size, ai_mode=100 -> pdcnn, ai_mode=010 -> fdcnn, ai_mode=001 -> cddcnn.
	ai_mode_buf_size = mem_select();
	printf("!!!!!!ai_mode_buf_size = %ld \n",ai_mode_buf_size);
	ret = mem_init(ai_mode_buf_size);
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
		goto exit;
	}

#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
	osg_init(0, &(stream[0].osg),
		STAMP_WIDTH, STAMP_HEIGHT, STAMP_PXLFMT, STAMP_C);
	osg_init(1, &(stream[1].osg),
		SUB_STAMP_WIDTH, SUB_STAMP_HEIGHT, SUB_STAMP_PXLFMT, SUB_STAMP_C);
    }
#endif

	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open demo modules (main)
	stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user

	stream[0].acap_sr_max = HD_AUDIO_SR_48000;
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
		osg_open(0, &(stream[0].osg), OSG_TYPE_VIDEOENC, HD_VIDEOENC_0_IN_0);
		osg_open(1, &(stream[1].osg), OSG_TYPE_VIDEOENC, HD_VIDEOENC_0_IN_1);
    }
#endif

	// open demo modules (sub)
	stream[1].proc_max_dim.w = SUB_VDO_SIZE_W; //assign by user
	stream[1].proc_max_dim.h = SUB_VDO_SIZE_H; //assign by user
	ret = open_module_2(&stream[1], &stream[1].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// open demo modules (sub2)
	stream[3].proc_max_dim.w = SUB2_VDO_SIZE_W; //assign by user
	stream[3].proc_max_dim.h = SUB2_VDO_SIZE_H; //assign by user
	ret = open_module_3(&stream[3], &stream[3].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}
	// open demo modules (alg path)
	stream[4].proc_max_dim.w = SUB2_VDO_SIZE_W; //assign by user
	stream[4].proc_max_dim.h = SUB2_VDO_SIZE_H; //assign by user
	ret = open_module_4(&stream[4], &stream[4].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
		osg_start(0, &(stream[0].osg), OSG_TYPE_VIDEOENC, STAMP_X, STAMP_Y);
		osg_start(1, &(stream[1].osg), OSG_TYPE_VIDEOENC, SUB_STAMP_X, SUB_STAMP_Y);
    }
#endif

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	/////////////////////////////////////////////////////////////////////
	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	sub_dim.w = SUB_VDO_SIZE_W;
	sub_dim.h = SUB_VDO_SIZE_H;
	sub2_dim.w = SUB2_VDO_SIZE_W;
	sub2_dim.h = SUB2_VDO_SIZE_H;
	// set videocap parameter
	stream[0].cap_dim.w = cap_size_w;
	stream[0].cap_dim.h = cap_size_h;
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim, &main_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
#if (ONEBUF_SUB == ENABLE)
	ret = set_proc_param(stream[0].proc_path, NULL, 0, FALSE, g_one_buf, 1); // encode order : sub  -> main
#else
	ret = set_proc_param(stream[0].proc_path, NULL, 0, FALSE, g_one_buf, 0); // encode order : main -> sub
#endif
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (sub)
#if (ONEBUF_SUB == ENABLE)
	ret = set_proc_param(stream[1].proc_path, NULL, 0, FALSE, TRUE,  0);
#else
	ret = set_proc_param(stream[1].proc_path, NULL, 0, FALSE, FALSE, 1);
#endif
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	// set videoproc parameter (sub2)
    set_proc_param(stream[3].proc_path, &sub2_dim, HD_VIDEO_PXLFMT_YUV420,TRUE, g_one_buf, 2); //no bind, must set out dim, must enable pull
    if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}
	// set extend path for AI path
	ret = set_proc_param_extend(stream[4].proc_path, HD_VIDEOPROC_0_OUT_1, NULL, &sub2_dim, TRUE, 0);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (main)
	stream[0].enc_max_dim.w = main_dim.w;
	stream[0].enc_max_dim.h = main_dim.h;
	ret = set_enc_cfg(stream[0].enc_path, &stream[0].enc_max_dim, VDO_BR, enc_type, main_ltr_en, TRUE);
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (main)
	stream[0].enc_dim.w = main_dim.w;
	stream[0].enc_dim.h = main_dim.h;
	//printf("set main stream dim %dx%d\r\n", stream[0].enc_dim.w, stream[0].enc_dim.h);
	if (g_yuvcompress == 1 && ((enc_type == 0)||(enc_type == 1))) {
		ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, HD_VIDEO_PXLFMT_YUV420_NVX2, enc_type, VDO_BR, main_ltr_en);
	} else {
		ret = set_enc_param(stream[0].enc_path, &stream[0].enc_dim, 0, enc_type, VDO_BR, main_ltr_en);
	}
	if (ret != HD_OK) {
		printf("set enc fail=%d\n", ret);
		goto exit;
	}

	// set videoenc config (sub)
	stream[1].enc_max_dim.w = sub_dim.w;
	stream[1].enc_max_dim.h = sub_dim.h;
#if (ONEBUF_SUB == ENABLE)
	ret = set_enc_cfg(stream[1].enc_path, &stream[1].enc_max_dim, SUB_VDO_BR, enc2_type, sub_ltr_en, TRUE);
#else
	ret = set_enc_cfg(stream[1].enc_path, &stream[1].enc_max_dim, SUB_VDO_BR, enc2_type, sub_ltr_en, FALSE);
#endif
	if (ret != HD_OK) {
		printf("set enc-cfg fail=%d\n", ret);
		goto exit;
	}

	// set videoenc parameter (sub)
	if (enc2_type >= 0) {
		stream[1].enc_dim.w = sub_dim.w;
		stream[1].enc_dim.h = sub_dim.h;
		//printf("set sub stream dim %dx%d\r\n", stream[1].enc_dim.w, stream[1].enc_dim.h);
		if (g_yuvcompress == 1 && ((enc_type == 0)||(enc_type == 1))) {
			ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, HD_VIDEO_PXLFMT_YUV420_NVX2, enc2_type, SUB_VDO_BR, sub_ltr_en);
		} else {
			ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, 0, enc2_type, SUB_VDO_BR, sub_ltr_en);
		}
		if (ret != HD_OK) {
			printf("set enc fail=%d\n", ret);
			goto exit;
		}
	}

    // audio
	stream[0].acap_sr = HD_AUDIO_SR_16000;
	ret = set_acap_param(stream[0].acap_path, stream[0].acap_sr);
	if (ret != HD_OK) {
		printf("set acap fail=%d\n", ret);
		goto exit;
	}

	if (sensor_sel == SEN_SEL_TC358840) {
		sleep(2);
	}
	// bind demo modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_4, HD_VIDEOENC_0_IN_0);
	// bind demo modules (sub)
	if (enc2_type >= 0) {
		hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOENC_0_IN_1);
	}
	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].enc_thread_id, NULL, encode_thread, (void *)stream);
	if (ret < 0) {
		printf("create encode thread failed\r\n");
		goto exit;
	}
	// start demo modules (main)
	//direct NOTE: ensure videocap start after 1st videoproc phy path start
	hd_videoproc_start(stream[0].proc_path);
	hd_videocap_start(stream[0].cap_path);
	// start demo modules (sub)
	if (enc2_type >= 0)	{
		hd_videoproc_start(stream[1].proc_path);
	}
	hd_videoproc_start(stream[3].proc_path);
	// start demo modules (alg)
	hd_videoproc_start(stream[4].proc_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	if (enc_type >= 0) {
		hd_videoenc_start(stream[0].enc_path);
	}
	if (enc2_type >= 0)	{
		hd_videoenc_start(stream[1].enc_path);
	}
	// create encode_thread (pull_out bitstream)
	ret = pthread_create(&stream[0].acap_thread_id, NULL, acapture_thread, (void *)stream);
	if (ret < 0) {
		printf("create acapture_thread failed");
		goto exit;
	}
	hd_audiocap_start(stream[0].acap_path);
	stream[0].acap_enter = 1;

	// let encode_thread start to work
	stream[0].flow_start= 1;

	#if 0
    // start demo
    {
		printf("start demo\r\n");
		g_enc_poll = 0;
		system("ai2_pdcnn_sample_stream_ipc 10 1 &");
		system("nvtrtspd_ipc &");
		while (1) {
			sleep(1);
		}
    }
	#endif

	// query user key
	printf("Enter q to exit\n");
	printf("Enter 0 to stop cap\n");
	printf("Enter 1 to start cap\n");
	printf("Enter 2 to stop vprc\n");
	printf("Enter 3 to start vprc\n");
	printf("Enter 4 to stop encode\n");
	printf("Enter 5 to start encode\n");
	printf("Enter 6 to change resolution\n");
	printf("Enter 8 to stop write file\n");
	printf("Enter 9 to start write file\n");
	printf("Enter a to stop encode poll\n");
	printf("Enter b to start encode poll\n");
	printf("Enter c to start demo\n");
	printf("Enter m to test memcpy\n");
	while (1) {
		key = getchar();
		if (key == 'q' || key == 0x3) {
			// let encode_thread stop loop and exit
			stream[0].enc_exit = 1;
			stream[0].acap_exit = 1;
			// quit program
			g_quit = 1;
			break;
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
		if (key == '0') {
			hd_videocap_stop(stream[0].cap_path);
			printf("stop cap\r\n");
		}
		if (key == '1') {
			hd_videocap_start(stream[0].cap_path);
			printf("start cap\r\n");
		}
		if (key == '2') {
			hd_videoproc_stop(stream[0].proc_path);
			hd_videoproc_stop(stream[1].proc_path);
			hd_videoproc_stop(stream[3].proc_path);
			hd_videoproc_stop(stream[4].proc_path);
			printf("stop proc\r\n");
		}
		if (key == '3') {
			hd_videoproc_start(stream[0].proc_path);
			hd_videoproc_start(stream[1].proc_path);
			hd_videoproc_start(stream[3].proc_path);
			hd_videoproc_start(stream[4].proc_path);
			printf("start proc\r\n");
		}
		if (key == '4') {
			hd_videoenc_stop(stream[0].enc_path);
			hd_videoenc_stop(stream[1].enc_path);
			printf("stop encode\r\n");
		}
		if (key == '5') {
			hd_videoenc_start(stream[0].enc_path);
			hd_videoenc_start(stream[1].enc_path);
			printf("start encode\r\n");
		}
		if (key == '6') {
			stream[1].enc_dim.w = 320;
			stream[1].enc_dim.h = 240;
			hd_videoproc_stop(stream[1].proc_path);
			hd_videoenc_stop(stream[1].enc_path);
			if (g_yuvcompress == 1 && ((enc2_type == 0)||(enc2_type == 1))) {
				ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, HD_VIDEO_PXLFMT_YUV420_NVX2, enc2_type, VDO_BR, sub_ltr_en);
		    } else {
				ret = set_enc_param(stream[1].enc_path, &stream[1].enc_dim, 0, enc2_type, VDO_BR, sub_ltr_en);
		    }
			hd_videoproc_start(stream[1].proc_path);
			hd_videoenc_start(stream[1].enc_path);
			printf("change resolution\r\n");
		}
		if (key == '8') {
			g_write_file = 0;
			printf("stop write file\r\n");
		}
		if (key == '9') {
			g_write_file = 1;
			printf("start write file\r\n");
		}
		if (key == 'a') {
			g_enc_poll = 0;
			printf("stop encode poll\r\n");
		}
		if (key == 'b') {
			g_enc_poll = 1;
			printf("start encode poll\r\n");
		}
		if (key == 'z') {
			g_enc_poll = 0;
			system("nvtrtspd_ipc &");
			printf("nvtrtspd_ipc\r\n");
		}

		if (key == 'c') {
			printf("start demo\r\n");
			if(save_ai_result){

					sprintf(command,"ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc 10 1 1 %s %ld %ld %ld &", ai_mode, pd_max_distance_mode, cdd_max_distance_mode,ai_mode_buf_size);

					system(command);
			}else{
				
					sprintf(command,"ai2_pdcnn_fdcnn_cddcnn_sample_stream_ipc 10 1 0 %s %ld %ld %ld &", ai_mode, pd_max_distance_mode, cdd_max_distance_mode,ai_mode_buf_size);
						
					system(command);
				
			}
		}
		if (key == 'e') {
				
			system("ai2_pdcnn_sample_stream_ipc 10 1 0 &");
				
		}

		
		if (key == 'm') {
			copy_info[0].copy_size = 0x500000;
			copy_info[0].ddr_id = DDR_ID0;
			copy_info[0].sleep_us = 5000;

			if (TRUE == is_start_memcpy_test) {
				continue;
			}
			printf("start memcpy test\r\n");
			ret = pthread_create(&memcpy_thread_id, NULL, memcpy_thread, &copy_info[0]);
			if (ret < 0) {
				printf("create memcpy thread failed");
				goto exit;
			}
			is_start_memcpy_test = TRUE;
		}
	}
	// notify child process
	p_pd_shm = (PD_SHM_INFO  *)g_shm;
	p_pd_shm->exit = 1;
	sleep(1);

	// stop demo modules (main)
	hd_videoproc_stop(stream[0].proc_path);
	hd_videoenc_stop(stream[0].enc_path);

	// stop demo modules (sub)
	hd_videoproc_stop(stream[1].proc_path);
	hd_videoenc_stop(stream[1].enc_path);

	hd_videoproc_stop(stream[3].proc_path);
	hd_videoproc_stop(stream[4].proc_path);

	// stop acap module
	hd_audiocap_stop(stream[0].acap_path);

	// direct mode should close vcap last
	hd_videocap_stop(stream[0].cap_path);

	// unbind demo modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_4);

	// unbind demo modules (sub)
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_1);

	// destroy encode thread
	pthread_join(stream[0].enc_thread_id, NULL);
	pthread_join(stream[0].acap_thread_id, NULL);
	if (TRUE == is_start_memcpy_test) {
		pthread_join(memcpy_thread_id, NULL);
	}

exit:
	g_quit = 1;
	// close demo modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
		osg_close(0, &(stream[0].osg), OSG_TYPE_VIDEOENC);
		osg_close(1, &(stream[1].osg), OSG_TYPE_VIDEOENC);
    }
#endif
	// close demo modules (sub)
	ret = close_module_2(&stream[1]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	// close demo modules (sub2)
	ret = close_module_3(&stream[3]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	// close ai path
	ret = close_module_4(&stream[4]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}
	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}
#if (FUNC_OSG == ENABLE)
    if (g_osg > 0) {
		osg_uninit(0, &(stream[0].osg));
		osg_uninit(1, &(stream[1].osg));
    }
#endif
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
	exit_share_memory();
	return 0;
}
