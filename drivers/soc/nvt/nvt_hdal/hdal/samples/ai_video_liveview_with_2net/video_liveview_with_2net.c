/**
	@brief Source file of liveview with ai 2net sample code.

	@file video_liveview_with_2net.c

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"


// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>	
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_video_liveview_with_2net, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

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

#define SEN_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT		HD_VIDEO_PXLFMT_RAW12
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

#define RESOLUTION_SET  0 //0: 2M(IMX290), 1:5M(OS05A) 2: 2M (OS02K10) 3: 2M (AR0237IR) 4: (PATTERN_GEN)
#if ( RESOLUTION_SET == 0)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif (RESOLUTION_SET == 1)
#define VDO_SIZE_W      2592
#define VDO_SIZE_H      1944
#elif ( RESOLUTION_SET == 2)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 3)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#elif ( RESOLUTION_SET == 4)
#define VDO_SIZE_W      1920
#define VDO_SIZE_H      1080
#endif

#define VIDEOCAP_ALG_FUNC HD_VIDEOCAP_FUNC_AE | HD_VIDEOCAP_FUNC_AWB
#define VIDEOPROC_ALG_FUNC HD_VIDEOPROC_FUNC_SHDR | HD_VIDEOPROC_FUNC_WDR | HD_VIDEOPROC_FUNC_3DNR | HD_VIDEOPROC_FUNC_COLORNR | HD_VIDEOPROC_FUNC_DEFOG

static UINT32 g_shdr = 0; //fixed

#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420

#define SUB_VDO_SIZE_W	1920 //max for videoout
#define SUB_VDO_SIZE_H	1080 //max for videoout

#define NN_RUN_NET_NUM	2

#define SOURCE_PATH		HD_VIDEOPROC_0_OUT_4 //out 0~4 is physical path
#define EXTEND_PATH		HD_VIDEOPROC_0_OUT_5 //out 5~15 is extend path
#define EXTEND_PATH2	HD_VIDEOPROC_0_OUT_6 //out 5~15 is extend path
#define EXTEND_PATH3	HD_VIDEOPROC_0_OUT_7 //out 5~15 is extend path

///////////////////////////////////////////////////////////////////////////////

#define NET_PATH_ID		UINT32

#define VENDOR_AI_CFG  				0x000f0000  //vendor ai config

#define AI_RGB_BUFSIZE(w, h)		(ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(HD_VIDEO_PXLFMT_RGB888_PLANAR) / 8) * (h))

#define NET_VDO_SIZE_W	1920 //max for net
#define NET_VDO_SIZE_H	1080 //max for net
#define SUB2_VDO_SIZE_W	NET_VDO_SIZE_W
#define SUB2_VDO_SIZE_H	NET_VDO_SIZE_H

int ai_async = 0;  //0: sync mode will call proc(), 2: sync mode will call proc_buf(), 3: async mode will call push_in_buf() and pull_out_buf()

///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
} MEM_PARM;


/*-----------------------------------------------------------------------------*/
/* Global Functions                                                             */
/*-----------------------------------------------------------------------------*/

static HD_RESULT mem_get(MEM_PARM *mem_parm, UINT32 size, UINT32 id)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;
	HD_COMMON_MEM_VB_BLK blk;
	
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL + id, size, DDR_ID0);
	if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
		printf("hd_common_mem_get_block fail\r\n");
		return HD_ERR_NG;
	}
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("not get buffer, pa=%08x\r\n", (int)pa);
		return HD_ERR_NOMEM;
	}
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, size);

	/* Release buffer */
	if (va == 0) {
		ret = hd_common_mem_munmap(va, size);
		if (ret != HD_OK) {
			printf("mem unmap fail\r\n");
			return ret;
		}
	}

	mem_parm->pa = pa;
	mem_parm->va = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk = blk;

	return HD_OK;
}

static HD_RESULT mem_rel(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;

	/* Release in buffer */
	if (mem_parm->va) {
		ret = hd_common_mem_munmap((void *)mem_parm->va, mem_parm->size);
		if (ret != HD_OK) {
			printf("mem_uninit : (g_mem.va)hd_common_mem_munmap fail.\r\n");
			return ret;
		}
	}
	//ret = hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)g_mem.pa);
	ret = hd_common_mem_release_block(mem_parm->blk);
	if (ret != HD_OK) {
		printf("mem_uninit : (g_mem.pa)hd_common_mem_release_block fail.\r\n");
		return ret;
	}

	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;
	return HD_OK;
}

static HD_RESULT mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size)
{
	HD_RESULT ret = HD_OK;
	UINT32 pa   = 0;
	void  *va   = NULL;

	//alloc private pool
	ret = hd_common_mem_alloc(name, &pa, (void**)&va, size, DDR_ID0);
	if (ret!= HD_OK) {
		return ret;
	}

	mem_parm->pa   = pa;
	mem_parm->va   = (UINT32)va;
	mem_parm->size = size;
	mem_parm->blk  = (UINT32)-1;

	return HD_OK;
}

static HD_RESULT mem_free(MEM_PARM *mem_parm)
{
	HD_RESULT ret = HD_OK;
	
	//free private pool
	ret =  hd_common_mem_free(mem_parm->pa, (void *)mem_parm->va);
	if (ret!= HD_OK) {
		return ret;
	}
	
	mem_parm->pa = 0;
	mem_parm->va = 0;
	mem_parm->size = 0;
	mem_parm->blk = (UINT32)-1;

	return HD_OK;
}

/*
static INT32 mem_load(MEM_PARM *mem_parm, const CHAR *filename)
{
	FILE *fd;
	INT32 size = 0;

	fd = fopen(filename, "rb");

	if (!fd) {
		printf("cannot read %s\r\n", filename);
		return -1;
	}

	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	
	if (size < 0) {
		printf("getting %s size failed\r\n", filename);
	} else if ((INT32)fread((VOID *)mem_parm->va, 1, size, fd) != size) {
		printf("read size < %ld\r\n", size);
		size = -1;
	}
	mem_parm->size = size;

	if (fd) {
		fclose(fd);
	}

	return size;
}
*/

/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////

INT32 cap_mem_config(HD_PATH_ID video_cap_ctrl, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	HD_DIM* p_dim = (HD_DIM*)p_cfg;
	// config common pool (cap)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	p_mem_cfg->pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_RAW_BUFSIZE(p_dim->w, p_dim->h, CAP_OUT_FMT)
        											 +VDO_CA_BUF_SIZE(CA_WIN_NUM_W, CA_WIN_NUM_H)
        											 +VDO_LA_BUF_SIZE(LA_WIN_NUM_W, LA_WIN_NUM_H);
	p_mem_cfg->pool_info[i].blk_cnt = 2;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	return i;
}

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};

	#if (RESOLUTION_SET == 0)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_imx290");
	printf("Using nvt_sen_imx290\n");
	#elif (RESOLUTION_SET == 1)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os05a10");
	printf("Using nvt_sen_os05a10\n");
	#elif (RESOLUTION_SET == 2)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
	printf("Using nvt_sen_os02k10\n");
	#elif (RESOLUTION_SET == 3)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_ar0237ir");
	printf("Using nvt_sen_ar0237ir\n");
	#elif (RESOLUTION_SET == 4)
	snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, HD_VIDEOCAP_SEN_PAT_GEN);
	printf("Using sie pattern gen\n");
	#endif

    
	if(RESOLUTION_SET == 3) {
        cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_P_RAW;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x204; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
	    printf("Parallel interface\n");
    }
    if(RESOLUTION_SET == 0 || RESOLUTION_SET == 1 || RESOLUTION_SET == 2) {
	    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
	    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI
	    printf("MIPI interface\n");
    }
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		printf("Using g_shdr_mode\n");
	} else {
		#if (RESOLUTION_SET == 0)
		printf("Using imx290\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 1)
		printf("Using OS052A\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#elif (RESOLUTION_SET == 2)
		printf("Using OS02K10\n");
		cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xf01;//0xf01;//PIN_MIPI_LVDS_CFG_CLK2 | PIN_MIPI_LVDS_CFG_DAT0 | PIN_MIPI_LVDS_CFG_DAT1 | PIN_MIPI_LVDS_CFG_DAT2 | PIN_MIPI_LVDS_CFG_DAT3
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
	cap_cfg.sen_cfg.sen_dev.pin_cfg.clk_lane_sel = HD_VIDEOCAP_SEN_CLANE_SEL_CSI0_USE_C2;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[0] = 0;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[1] = 1;
	if (g_shdr == 1) {
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
		cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
	} else {
		#if (RESOLUTION_SET == 0)
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 1)
			printf("Using OS052A or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#elif (RESOLUTION_SET == 2)
			printf("Using OS02K10 or shdr\n");
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[2] = 2;
			cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[3] = 3;
		#endif
	}
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[4] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[5] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[6] = HD_VIDEOCAP_SEN_IGNORE;
	cap_cfg.sen_cfg.sen_dev.pin_cfg.sen_2_serial_pin_map[7] = HD_VIDEOCAP_SEN_IGNORE;
	ret = hd_videocap_open(0, HD_VIDEOCAP_0_CTRL, &video_cap_ctrl); //open this for device control
	if (ret != HD_OK) {
		return ret;
	}
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_DRV_CONFIG, &cap_cfg);
	iq_ctl.func = VIDEOCAP_ALG_FUNC;
	ret |= hd_videocap_set(video_cap_ctrl, HD_VIDEOCAP_PARAM_CTRL, &iq_ctl);

	*p_video_cap_ctrl = video_cap_ctrl;
	return ret;
}

static HD_RESULT set_cap_param(HD_PATH_ID video_cap_path, HD_DIM *p_dim)
{
	HD_RESULT ret = HD_OK;
	{//select sensor mode, manually or automatically
		HD_VIDEOCAP_IN video_in_param = {0};

		#if (RESOLUTION_SET == 4) // pattern gen
		video_in_param.sen_mode = HD_VIDEOCAP_PATGEN_MODE(HD_VIDEOCAP_SEN_PAT_COLORBAR, 200);
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		#else
		video_in_param.sen_mode = HD_VIDEOCAP_SEN_MODE_AUTO; //auto select sensor mode by the parameter of HD_VIDEOCAP_PARAM_OUT
		video_in_param.frc = HD_VIDEO_FRC_RATIO(30,1);
		video_in_param.dim.w = p_dim->w;
		video_in_param.dim.h = p_dim->h;
		video_in_param.pxlfmt = SEN_OUT_FMT;
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
		#endif
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
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
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
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT_CROP, &video_crop_param);
		//printf("set_cap_param CROP ON=%d\r\n", ret);
	}
	#endif
	{
		HD_VIDEOCAP_OUT video_out_param = {0};

		//without setting dim for no scaling, using original sensor out size
		video_out_param.pxlfmt = CAP_OUT_FMT;
		video_out_param.dir = HD_VIDEO_DIR_NONE;
		ret = hd_videocap_set(video_cap_path, HD_VIDEOCAP_PARAM_OUT, &video_out_param);
		//printf("set_cap_param OUT=%d\r\n", ret);
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

INT32 proc_mem_config(HD_PATH_ID video_proc_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	HD_DIM* p_dim = (HD_DIM*)p_cfg;
	// config common pool (proc)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
	p_mem_cfg->pool_info[i].blk_size = DBGINFO_BUFSIZE()+VDO_YUV_BUFSIZE(p_dim->w, p_dim->h, HD_VIDEO_PXLFMT_YUV420);  // NOTE: dim need align to 16 for rotate buffer
	p_mem_cfg->pool_info[i].blk_cnt = 3;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	return i;
}

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
		video_cfg_param.ctrl_max.func = 0;
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

	video_ctrl_param.func = 0;
	ret = hd_videoproc_set(video_proc_ctrl, HD_VIDEOPROC_PARAM_CTRL, &video_ctrl_param);

	*p_video_proc_ctrl = video_proc_ctrl;

	return ret;
}

static HD_RESULT set_proc_param(HD_PATH_ID video_proc_path, HD_DIM* p_dim, UINT32 dir, UINT32 depth)
{
	HD_RESULT ret = HD_OK;

	if (p_dim != NULL) { //if videoproc is already binding to dest module, not require to setting this!
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = p_dim->w;
		video_out_param.dim.h = p_dim->h;
		video_out_param.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
		video_out_param.dir = dir;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT video_out_param = {0};
		video_out_param.func = 0;
		video_out_param.dim.w = 0;
		video_out_param.dim.h = 0;
		video_out_param.pxlfmt = 0;
		video_out_param.dir = dir;
		video_out_param.frc = HD_VIDEO_FRC_RATIO(1,1);
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT, &video_out_param);
	}

	return ret;
}

static HD_RESULT set_proc_param_extend(HD_PATH_ID video_proc_path, HD_PATH_ID src_path, HD_URECT* p_crop, HD_DIM* p_dim, UINT32 dir, UINT32 depth)
{
	HD_RESULT ret = HD_OK;

	if ((p_crop != NULL) && !((p_crop->x == 0)&&(p_crop->y == 0)&&(p_crop->w == 0)&&(p_crop->h == 0))) { //if videoproc is already binding to dest module, not require to setting this!
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
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
	} else {
		HD_VIDEOPROC_OUT_EX video_out_param = {0};
		video_out_param.src_path = src_path;
		video_out_param.dim.w = 0; //auto reference to downstream's in dim.w
		video_out_param.dim.h = 0; //auto reference to downstream's in dim.h
		video_out_param.pxlfmt = 0; //auto reference to downstream's in pxlfmt
		video_out_param.dir = dir;
		video_out_param.depth = depth; //set 1 to allow pull
		ret = hd_videoproc_set(video_proc_path, HD_VIDEOPROC_PARAM_OUT_EX, &video_out_param);
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

static HD_RESULT set_out_param(HD_PATH_ID video_out_path, HD_DIM *p_dim, HD_URECT* p_rect)
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

	if (p_rect != NULL) {
		HD_VIDEOOUT_WIN_ATTR video_out_param;
		video_out_param.visible = 1;
		video_out_param.rect.x = p_rect->x;
		video_out_param.rect.y = p_rect->y;
		video_out_param.rect.w = p_rect->w;
		video_out_param.rect.h = p_rect->h;
		video_out_param.layer = HD_LAYER1;

		ret =  hd_videoout_set(video_out_path, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &video_out_param);
	}

	return ret;
}
///////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/

typedef struct _NET_PROC_CONFIG {

	CHAR model_filename[256];
	INT32 binsize;
	int job_method;
	int job_wait_ms;
	int buf_method;
	void *p_share_model;

	CHAR label_filename[256];

} NET_PROC_CONFIG;

typedef struct _NET_PROC {

	NET_PROC_CONFIG net_cfg;
	MEM_PARM proc_mem;
	UINT32 proc_id;
	
	CHAR out_class_labels[MAX_CLASS_NUM * VENDOR_AIS_LBL_LEN];
	MEM_PARM rslt_mem;
	MEM_PARM io_mem;
		
} NET_PROC;

static NET_PROC g_net[16] = {0};

static INT32 _getsize_model(char* filename)
{
	FILE *bin_fd;
	UINT32 bin_size = 0;

	bin_fd = fopen(filename, "rb");
	if (!bin_fd) {
		printf("get bin(%s) size fail\n", filename);
		return (-1);
	}

	fseek(bin_fd, 0, SEEK_END);
	bin_size = ftell(bin_fd);
	fseek(bin_fd, 0, SEEK_SET);
	fclose(bin_fd);

	return bin_size;
}

static UINT32 _load_model(CHAR *filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 model_addr = va;
	//DBG_DUMP("model addr = %08x\r\n", (int)model_addr);

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("load model(%s) fail\r\n", filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)model_addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	
	printf("load model(%s) ok\r\n", filename);
	return read_size;
}

static UINT32 _load_part_model(CHAR *filename, UINT32 part_size, UINT32 va)
{
	FILE  *fd;
	UINT32 read_size = 0;
	const UINT32 model_addr = va;

	fd = fopen(filename, "rb");
	if (!fd) {
		printf("load model(%s) fail\r\n", filename);
		return 0;
	}

	read_size = fread ((void *)model_addr, 1, part_size, fd);
	if (read_size != part_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)part_size);
	}
	fclose(fd);

	printf("load part_model(%s) ok\r\n", filename);
	return read_size;
}

static HD_RESULT _load_label(UINT32 addr, UINT32 line_len, const CHAR *filename)
{
	FILE *fd;
	CHAR *p_line = (CHAR *)addr;

	fd = fopen(filename, "r");
	if (!fd) {
		printf("load label(%s) fail\r\n", filename);
		return HD_ERR_NG;
	}

	while (fgets(p_line, line_len, fd) != NULL) {
		p_line[strlen(p_line) - 1] = '\0'; // remove newline character
		p_line += line_len;
	}

	if (fd) {
		fclose(fd);
	}

	printf("load label(%s) ok\r\n", filename);
	return HD_OK;
}

static HD_RESULT network_init(void)
{
	HD_RESULT ret = HD_OK;
	int i;
	
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

	for (i = 0; i < 16; i++) {
		NET_PROC* p_net = g_net + i;
		p_net->proc_id = i;
	}
	return ret;
}

static HD_RESULT network_uninit(void)
{
	HD_RESULT ret = HD_OK;
	
	ret = vendor_ai_uninit();
	if (ret != HD_OK) {
		printf("vendor_ai_uninit fail=%d\n", ret);
	}
	
	return ret;
}

INT32 network_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i)
{
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	NET_PROC_CONFIG* p_proc_cfg = (NET_PROC_CONFIG*)p_cfg;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;
	
	memcpy((void*)&p_net->net_cfg, (void*)p_proc_cfg, sizeof(NET_PROC_CONFIG));
	if (strlen(p_net->net_cfg.model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", proc_id);
		return i;
	}

	p_net->net_cfg.binsize = _getsize_model(p_net->net_cfg.model_filename);
	if (p_net->net_cfg.binsize <= 0) {
		printf("proc_id(%u) input model is not exist?\r\n", proc_id);
		return i;
	}
	
	printf("proc_id(%u) set net_mem_cfg: model-file(%s), binsize=%d\r\n", 
		proc_id,
		p_net->net_cfg.model_filename,
		p_net->net_cfg.binsize);

	printf("proc_id(%u) set net_mem_cfg: label-file(%s)\r\n", 
		proc_id,
		p_net->net_cfg.label_filename);
	
	// config common pool (in)
	p_mem_cfg->pool_info[i].type = HD_COMMON_MEM_USER_DEFINIED_POOL + proc_id;
	p_mem_cfg->pool_info[i].blk_size = p_net->net_cfg.binsize;
	p_mem_cfg->pool_info[i].blk_cnt = 1;
	p_mem_cfg->pool_info[i].ddr_id = DDR_ID0;
	i++;
	return i;
}

static HD_RESULT network_set_config(NET_PATH_ID net_path, NET_PROC_CONFIG* p_proc_cfg)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 binsize = p_net->net_cfg.binsize;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	memcpy((void*)&p_net->net_cfg, (void*)p_proc_cfg, sizeof(NET_PROC_CONFIG));
	p_net->net_cfg.binsize = binsize;
	printf("proc_id(%u) set net_cfg: job-opt=(%u,%d), buf-opt(%u)\r\n", 
		proc_id,
		p_net->net_cfg.job_method,
		(int)p_net->net_cfg.job_wait_ms,
		p_net->net_cfg.buf_method);
	
	// set buf opt
	{
		VENDOR_AI_NET_CFG_BUF_OPT cfg_buf_opt = {0};
		cfg_buf_opt.method = p_net->net_cfg.buf_method;
		cfg_buf_opt.ddr_id = DDR_ID0;
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_BUF_OPT, &cfg_buf_opt);
	}

	// set job opt
	{
		VENDOR_AI_NET_CFG_JOB_OPT cfg_job_opt = {0};
		cfg_job_opt.method = p_net->net_cfg.job_method;
		cfg_job_opt.wait_ms = p_net->net_cfg.job_wait_ms;
		cfg_job_opt.schd_parm = VENDOR_AI_FAIR_CORE_ALL; //FAIR dispatch to ALL core
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_JOB_OPT, &cfg_job_opt);
	}

	return ret;
}

static HD_RESULT network_alloc_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_NET_CFG_WORKBUF wbuf = {0};

	ret = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) get VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}
	ret = mem_alloc(&p_net->io_mem, "ai_io_buf", wbuf.size);
	if (ret != HD_OK) {
		printf("proc_id(%lu) alloc ai_io_buf fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	wbuf.pa = p_net->io_mem.pa;
	wbuf.va = p_net->io_mem.va;
	wbuf.size = p_net->io_mem.size;
	ret = vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_WORKBUF, &wbuf);
	if (ret != HD_OK) {
		printf("proc_id(%lu) set VENDOR_AI_NET_PARAM_CFG_WORKBUF fail\r\n", proc_id);
		return HD_ERR_FAIL;
	}

	printf("alloc_io_buf: work buf, pa = %#lx, va = %#lx, size = %lu\r\n", wbuf.pa, wbuf.va, wbuf.size);

	return ret;
}

static HD_RESULT network_free_io_buf(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;

	if (p_net->io_mem.pa && p_net->io_mem.va) {
		mem_free(&p_net->io_mem);
	}
	return ret;
}

static HD_RESULT network_open(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id;
	p_net->proc_id = net_path;
	proc_id = p_net->proc_id;

	UINT32 loadsize = 0;

	if (strlen(p_net->net_cfg.model_filename) == 0) {
		printf("proc_id(%u) input model is null\r\n", proc_id);
		return 0;
	}

	if (p_net->net_cfg.p_share_model == NULL ) {
		mem_get(&p_net->proc_mem, p_net->net_cfg.binsize, proc_id);
		//load file
		loadsize = _load_model(p_net->net_cfg.model_filename, p_net->proc_mem.va);

		if (loadsize <= 0) {
			printf("proc_id(%u) input model load fail: %s\r\n", proc_id, p_net->net_cfg.model_filename);
			return 0;
		}

		// load label
		ret = _load_label((UINT32)p_net->out_class_labels, VENDOR_AIS_LBL_LEN, p_net->net_cfg.label_filename);
		if (ret != HD_OK) {
			printf("proc_id(%u) load_label fail=%d\n", proc_id, ret);
			return HD_ERR_FAIL;
		}

		// set model
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&p_net->proc_mem);
	} else {
		//--- this net_path want to share other share_net model ---
		NET_PROC* p_share_net = (NET_PROC*)p_net->net_cfg.p_share_model;
		VENDOR_AI_NET_INFO net_info = {0};

		// query share_net for necessary size( the size which can't reuse )
		vendor_ai_net_get(p_share_net->proc_id, VENDOR_AI_NET_PARAM_INFO, &net_info);
		if (net_info.model_part1_size == 0) {
			printf("proc_id(%u) get share proc_id(%u) necessary size fail\r\n", proc_id, p_share_net->proc_id);
			return HD_ERR_FAIL;
		}

		ret = mem_alloc(&p_net->proc_mem, "ai_parm_buf", net_info.model_part1_size);
		if (ret != HD_OK) {
			printf("proc_id(%u) alloc ai_parm_buf fail\r\n", proc_id);
			return HD_ERR_FAIL;
		}
		// load file ( load share_net model for necessary part only )
		loadsize = _load_part_model(p_share_net->net_cfg.model_filename, net_info.model_part1_size, p_net->proc_mem.va);

		if (loadsize <= 0) {
			printf("proc_id(%u) input model load fail: %s\r\n", proc_id, p_share_net->net_cfg.model_filename);
			return 0;
		}

		// load label ( load share_net label )
		ret = _load_label((UINT32)p_net->out_class_labels, VENDOR_AIS_LBL_LEN, p_share_net->net_cfg.label_filename);
		if (ret != HD_OK) {
			printf("proc_id(%u) load_label fail=%d\n", proc_id, ret);
			return HD_ERR_FAIL;
		}

		// set model
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_MODEL, (VENDOR_AI_NET_CFG_MODEL*)&p_net->proc_mem);
		vendor_ai_net_set(proc_id, VENDOR_AI_NET_PARAM_CFG_SHAREMODEL, (VENDOR_AI_NET_CFG_MODEL*)&p_share_net->proc_mem);
	}

	// open
	vendor_ai_net_open(proc_id);

	if ((ret = network_alloc_io_buf(net_path)) != HD_OK)
		return ret;
	
	return ret;
}

static HD_RESULT network_close(NET_PATH_ID net_path)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;

	if ((ret = network_free_io_buf(net_path)) != HD_OK)
		return ret;
	
	// close
	ret = vendor_ai_net_close(proc_id);
	
	if (p_net->net_cfg.p_share_model == NULL ) {
		mem_rel(&p_net->proc_mem);  // common buffer
	} else {
		mem_free(&p_net->proc_mem); // private buffer
	}

	return ret;
}

static HD_RESULT network_dump_out_buf(NET_PATH_ID net_path, void *p_out)
{
	HD_RESULT ret = HD_OK;
	NET_PROC* p_net = g_net + net_path;
	UINT32 proc_id = p_net->proc_id;
	VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt_info = (VENDOR_AI_POSTPROC_RESULT_INFO *)(p_out);
	UINT32 i, j;
	
	printf("proc_id(%u) classification results:\r\n", proc_id);
	if (p_rslt_info) {
		for (i = 0; i < p_rslt_info->result_num; i++) {
			VENDOR_AI_POSTPROC_RESULT *p_rslt = &p_rslt_info->p_result[i];
			for (j = 0; j < NN_POSTPROC_TOP_N; j++) {
				printf("%ld. no=%ld, label=%s, score=%f\r\n",
					j + 1,
					p_rslt->no[j],
					&p_net->out_class_labels[p_rslt->no[j] * VENDOR_AIS_LBL_LEN],
					p_rslt->score[j]);
			}
		}
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _VIDEO_LIVEVIEW {

	// (1)
	HD_VIDEOCAP_SYSCAPS cap_syscaps;
	HD_PATH_ID cap_ctrl;
	HD_PATH_ID cap_path;

	HD_DIM  cap_dim;
	HD_DIM  proc_max_dim;

	// (2)
	HD_VIDEOPROC_SYSCAPS proc_syscaps;
	HD_PATH_ID proc_ctrl;
	HD_PATH_ID proc_path;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

    HD_VIDEOOUT_HDMI_ID hdmi_id;

	// (4) network 
	NET_PROC_CONFIG net_proc_cfg;
	NET_PATH_ID net_path;

	//ai sync - begin
	pthread_t  net_proc_thread_id;
	UINT32 net_proc_start;
	UINT32 net_proc_exit;
	UINT32 net_proc_oneshot;
	//ai sync - end

	//ai async - begin
	pthread_t  net_push_thread_id;
	UINT32 net_push_start;
	UINT32 net_push_exit;
	UINT32 net_push_oneshot;
	pthread_t  net_pull_thread_id;
	UINT32 net_pull_start;
	UINT32 net_pull_exit;
	UINT32 net_pull_oneshot;
	#define NET_QUEUE_DEPTH 2
	UINT32 net_queue_in_id; //ready to alloc before push in
	UINT32 net_queue_out_id; //ready to free after pull out
	UINT32 net_queue_cnt; //count
	HD_VIDEO_FRAME net_queue[NET_QUEUE_DEPTH+1];
	//ai async - end
} VIDEO_LIVEVIEW;

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	if ((ret = network_init()) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module(VIDEO_LIVEVIEW *p_stream, HD_DIM* p_proc_max_dim)
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
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, SOURCE_PATH, &p_stream->proc_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module_2(VIDEO_LIVEVIEW *p_stream, UINT32 out_type)
{
	HD_RESULT ret;
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0,  &p_stream->out_path)) != HD_OK)
		return ret;

	return HD_OK;
}

static HD_RESULT open_module_3(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH2, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_open(p_stream->net_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT open_module_4(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, EXTEND_PATH3, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_open(p_stream->net_path)) != HD_OK)
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
	return HD_OK;
}

static HD_RESULT close_module_2(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_close(p_stream->out_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module_3(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_close(p_stream->net_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT close_module_4(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret;
	if ((ret = hd_videoproc_close(p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = network_close(p_stream->net_path)) != HD_OK)
		return ret;
	return HD_OK;
}

static HD_RESULT exit_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_uninit()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_uninit()) != HD_OK)
		return ret;
	if ((ret = network_uninit()) != HD_OK)
		return ret;
	return HD_OK;
}
///////////////////////////////////////////////////////////////////////////////

static VOID *network_proc_thread(VOID *arg);

static HD_RESULT network_sync_start(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;

	p_stream->net_proc_start = 0;
	p_stream->net_proc_exit = 0;
	p_stream->net_proc_oneshot = 0;
	
	ret = vendor_ai_net_start(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) start fail !!\n", p_stream->net_path);
	}
	
	ret = pthread_create(&p_stream->net_proc_thread_id, NULL, network_proc_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}

	p_stream->net_proc_start = 1;
	p_stream->net_proc_exit = 0;
	p_stream->net_proc_oneshot = 0;
	
	return ret;
}

static HD_RESULT network_sync_stop(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;
	p_stream->net_proc_exit = 1;
	
	if (p_stream->net_proc_thread_id) {
		pthread_join(p_stream->net_proc_thread_id, NULL);
	}

	//stop: should be call after last time proc
	ret = vendor_ai_net_stop(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) stop fail !!\n", p_stream->net_path);
	}
	
	return ret;
}

static VOID *network_proc_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	
	VIDEO_LIVEVIEW *p_stream = (VIDEO_LIVEVIEW*)arg;

	printf("\r\n");
	while (p_stream->net_proc_start == 0) sleep(1);

	printf("\r\n");
	while (p_stream->net_proc_exit == 0) {

		if (1) {
			VENDOR_AI_BUF	in_buf = {0};
			//VENDOR_AI_BUF	out_buf = {0};
			VENDOR_AI_POSTPROC_RESULT_INFO out_buf = {0};
			HD_VIDEO_FRAME video_frame = {0};

			ret = hd_videoproc_pull_out_buf(p_stream->proc_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto skip;
			}

			//prepare input AI_BUF from videoframe
			in_buf.sign = MAKEFOURCC('A','B','U','F');
			in_buf.width = video_frame.dim.w;
			in_buf.height = video_frame.dim.h;
			in_buf.channel 	= HD_VIDEO_PXLFMT_PLANE(video_frame.pxlfmt); //conver pxlfmt to channel count
			in_buf.line_ofs	= video_frame.loff[0];
			in_buf.fmt = video_frame.pxlfmt;
			in_buf.pa = video_frame.phy_addr[0];
			in_buf.va = 0;
			in_buf.size = video_frame.loff[0]*video_frame.dim.h*3/2;

			if (ai_async == 0) {
				
				// set input image
				ret = vendor_ai_net_set(p_stream->net_path, VENDOR_AI_NET_PARAM_IN(0, 0), &in_buf);
				if (HD_OK != ret) {
					printf("proc_id(%u) set input fail !!\n", p_stream->net_path);
					goto skip;
				}
				
				// do net proc
				ret = vendor_ai_net_proc(p_stream->net_path);
				if (HD_OK != ret) {
					printf("proc_id(%u) proc fail !!\n", p_stream->net_path);
					goto skip;
				}
				
				// get output result
				ret = vendor_ai_net_get(p_stream->net_path, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
				if (HD_OK != ret) {
					printf("proc_id(%u) get output fail !!\n", p_stream->net_path);
					goto skip;
				}
				
			} else if (ai_async == 1) {
			
				// do net proc_buf
				ret = vendor_ai_net_proc_buf(p_stream->net_path, VENDOR_AI_NET_PARAM_IN(0, 0), &in_buf, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf);
				if (HD_OK != ret) {
					printf("proc_id(%u) proc_buf fail !!\n", p_stream->net_path);
					goto skip;
				}
				
			}
			
			ret = network_dump_out_buf(p_stream->net_path, &out_buf);
			if (HD_OK != ret) {
				printf("proc_id(%u) output dump fail !!\n", p_stream->net_path);
				goto skip;
			}

			ret = hd_videoproc_release_out_buf(p_stream->proc_path, &video_frame);
			if(ret != HD_OK) {
				printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto skip;
			}
		}
		usleep(100);
	}
	
skip:

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
static VOID *network_push_thread(VOID *arg);
static VOID *network_pull_thread(VOID *arg);

static HD_RESULT network_async_start(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;

	p_stream->net_queue_in_id = 0; //ready to push in
	p_stream->net_queue_out_id = 0; //ready to pull out
	p_stream->net_queue_cnt = 0;
	
	p_stream->net_push_start = 0;
	p_stream->net_push_exit = 0;
	p_stream->net_push_oneshot = 0;
	p_stream->net_pull_start = 0;
	p_stream->net_pull_exit = 0;
	p_stream->net_pull_oneshot = 0;
	
	ret = vendor_ai_net_start(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) start fail !!\n", p_stream->net_path);
	}
	
	ret = pthread_create(&p_stream->net_push_thread_id, NULL, network_push_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}
	p_stream->net_push_start = 1;
	p_stream->net_push_exit = 0;
	p_stream->net_push_oneshot = 0;

	ret = pthread_create(&p_stream->net_pull_thread_id, NULL, network_pull_thread, (VOID*)(p_stream));
	if (ret < 0) {
		return HD_ERR_FAIL;
	}
	p_stream->net_pull_start = 1;
	p_stream->net_pull_exit = 0;
	p_stream->net_pull_oneshot = 0;
	
	return ret;
}

static HD_RESULT network_async_stop(VIDEO_LIVEVIEW *p_stream)
{
	HD_RESULT ret = HD_OK;

	p_stream->net_push_exit = 1;
	if (p_stream->net_push_thread_id) {
		pthread_join(p_stream->net_push_thread_id, NULL);
	}

	p_stream->net_pull_exit = 1;
	if (p_stream->net_pull_thread_id) {
		pthread_join(p_stream->net_pull_thread_id, NULL);
	}

	//stop: should be call after last time proc
	ret = vendor_ai_net_stop(p_stream->net_path);
	if (HD_OK != ret) {
		printf("proc_id(%u) stop fail !!\n", p_stream->net_path);
	}
	
	return ret;
}

static VOID *network_push_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	BOOL quit = FALSE;
	
	VIDEO_LIVEVIEW *p_stream = (VIDEO_LIVEVIEW*)arg;

	printf("\r\n");
	while (p_stream->net_push_start == 0) sleep(1);

	printf("\r\n");
	while (!quit) {

		UINT32 in_queue_cnt = p_stream->net_queue_cnt;		
		HD_VIDEO_FRAME* p_in_video_frame = p_stream->net_queue + p_stream->net_queue_in_id;

		if (p_stream->net_push_exit != 0) {
			if (in_queue_cnt == 0) {
				quit = TRUE; 
			} 
			goto skip_1;
		}
			
		if (in_queue_cnt >= NET_QUEUE_DEPTH) {
			//printf("proc_id(%u) yuv queue FULL, push in drop\n\r", p_stream->net_path);
 		} else {
			VENDOR_AI_BUF	in_buf = {0};

			ret = hd_videoproc_pull_out_buf(p_stream->proc_path, p_in_video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
			if(ret != HD_OK) {
				printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
				goto error_1;
			}
			if (p_stream->net_queue_in_id == NET_QUEUE_DEPTH) p_stream->net_queue_in_id = 0; else p_stream->net_queue_in_id++; 
			p_stream->net_queue_cnt++;

			//prepare input AI_BUF from videoframe
			in_buf.sign = MAKEFOURCC('A','B','U','F');
			in_buf.width = p_in_video_frame->dim.w;
			in_buf.height = p_in_video_frame->dim.h;
			in_buf.channel 	= HD_VIDEO_PXLFMT_PLANE(p_in_video_frame->pxlfmt); //conver pxlfmt to channel count
			in_buf.line_ofs	= p_in_video_frame->loff[0];
			in_buf.fmt = p_in_video_frame->pxlfmt;
			in_buf.pa = p_in_video_frame->phy_addr[0];
			in_buf.va = 0;
			in_buf.size = p_in_video_frame->loff[0]*p_in_video_frame->dim.h*3/2;

			// do net push in
			ret = vendor_ai_net_push_in_buf(p_stream->net_path, VENDOR_AI_NET_PARAM_IN(0, 0), &in_buf, 0);
			if (HD_OK != ret) {
				printf("proc_id(%u) push input fail !!\n", p_stream->net_path);
				goto error_1;
			}
		}

skip_1:
		
		usleep(10);
	}
	
error_1:

	return 0;
}

static VOID *network_pull_thread(VOID *arg)
{
	HD_RESULT ret = HD_OK;
	BOOL quit = FALSE;
	
	VIDEO_LIVEVIEW *p_stream = (VIDEO_LIVEVIEW*)arg;

	printf("\r\n");
	while (p_stream->net_pull_start == 0) sleep(1);

	printf("\r\n");
	while (!quit) {

		UINT32 out_queue_cnt = p_stream->net_queue_cnt;		
		HD_VIDEO_FRAME* p_out_video_frame = p_stream->net_queue + p_stream->net_queue_out_id;
		
		if (out_queue_cnt == 0) {
			//printf("proc_id(%u) yuv queue EMPTY, pull out drop !!\n", p_stream->net_path);
			if (p_stream->net_pull_exit != 0) {
				quit = TRUE;
				goto skip_2;
			}
			
 		} else {
			VENDOR_AI_POSTPROC_RESULT_INFO out_buf = {0};

			// do net pull out
			ret = vendor_ai_net_pull_out_buf(p_stream->net_path, VENDOR_AI_NET_PARAM_OUT(VENDOR_AI_MAXLAYER, 0), &out_buf, -1);
			if (HD_OK != ret) {
				printf("proc_id(%u) pull out fail !!\n", p_stream->net_path);
				goto error_2;
			}

			ret = network_dump_out_buf(p_stream->net_path, &out_buf);
			if (HD_OK != ret) {
				printf("proc_id(%u) output dump fail !!\n", p_stream->net_path);
				goto error_2;
			}

			ret = hd_videoproc_release_out_buf(p_stream->proc_path, p_out_video_frame);
			if(ret != HD_OK) {
				printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
				goto error_2;
			}
			if (p_stream->net_queue_out_id == NET_QUEUE_DEPTH) p_stream->net_queue_out_id = 0; else p_stream->net_queue_out_id++; 
			p_stream->net_queue_cnt--;
		}

skip_2:
			
		usleep(10);
	}
	
error_2:

	return 0;
}


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	VIDEO_LIVEVIEW stream[4] = {0}; //0: main stream //1: sub stream (vout) //2: sub2 stream (1net) //3: sub3 stream (net2)
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};
	NET_PROC_CONFIG *p_net_cfg = 0;
	INT32 idx;
	HD_RESULT ret;
	INT key;
	UINT32 i;

	//vprc main
	HD_DIM main_dim;
	
	//vprc sub
	HD_DIM sub_dim; //extend scale

	//vprc sub2
	HD_DIM sub2_dim; //extend (share from main)

	//vprc sub3
	HD_DIM sub3_dim; //extend (share from main)
	
    //vout
	UINT32 out_type = 0;
	HD_DIM out_dim;

	//net proc
	NET_PROC_CONFIG net_cfg[NN_RUN_NET_NUM] = {
		{
			.model_filename = "para/nvt_model.bin",
			.label_filename = "accuracy/labels.txt",
			.p_share_model = NULL,
		},
		{
			.model_filename = "para2/nvt_model.bin",
			.label_filename = "accuracy2/labels.txt",
			.p_share_model = NULL,
		}
	};

	if(argc < 3){
		printf("usage : ai_video_liveview_with_2net (out_type) (share_model) (ai_async) (job_opt) (job_sync) (buf_opt)\n");
		printf("        * share_model: (0: off, 1: on)\n");
		return -1;
	}

	idx = 1;
	
	// parse out config
	if (argc > idx) {
		out_type = atoi(argv[idx++]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}

	// parse network config
	{
		int share_model = 0, job_method = 0, job_wait_ms = 0, buf_method = 0;
		if (argc > idx) {
			sscanf(argv[idx++], "%d", &share_model);
			printf("share_model %d\r\n", share_model);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%d", &ai_async);
			printf("ai_async %d\r\n", ai_async);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%d", &job_method);
			printf("job_opt %d\r\n", job_method);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%d", &job_wait_ms);
			printf("job_sync %d\r\n", job_wait_ms);
		}
		if (argc > idx) {
			sscanf(argv[idx++], "%d", &buf_method);
			printf("buf_opt %d\r\n", buf_method);
		}

		for (i = 0; i < NN_RUN_NET_NUM; i++) {
			net_cfg[i].job_method = job_method;
			net_cfg[i].job_wait_ms = job_wait_ms;
			net_cfg[i].buf_method = buf_method;
			// set share model
			if (share_model && i > 0) {
				net_cfg[i].p_share_model = &g_net[0];
			}
		}
	}
	
	printf("\r\n\r\n");
	
	stream[2].net_path = 0; // net1
	stream[3].net_path = 1; // net2
	
	// init hdal
	ret = hd_common_init(0);
	if (ret != HD_OK) {
		printf("hd_common_init fail=%d\n", ret);
		goto exit;
	}
	// set project config for AI
	hd_common_sysconfig(0, (1<<16), 0, VENDOR_AI_CFG); //enable AI engine

	// init mem
	{
		INT32 idx = 0; // mempool index
		
		// config common pool
		stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
		stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
		idx = cap_mem_config(stream[0].proc_path, &mem_cfg, &stream[0].cap_dim, idx);
		
		stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
		stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
		idx = proc_mem_config(stream[0].proc_path, &mem_cfg, &stream[0].proc_max_dim, idx);
		stream[1].proc_max_dim.w = SUB_VDO_SIZE_W; //assign by user
		stream[1].proc_max_dim.h = SUB_VDO_SIZE_H; //assign by user
		idx = proc_mem_config(stream[1].out_path, &mem_cfg, &stream[1].proc_max_dim, idx);

		for (i = 0; i < NN_RUN_NET_NUM; i++) {
			p_net_cfg = &net_cfg[i];
			if (i == 0 || p_net_cfg->p_share_model == NULL) {
				idx = network_mem_config(stream[2+i].net_path, &mem_cfg, p_net_cfg, idx);
			}
		}
	}
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("hd_common_mem_init err: %d\r\n", ret);
		goto exit;
	}
	
	// init all modules
	ret = init_module();
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open video_liveview modules (main)
	ret = open_module(&stream[0], &stream[0].proc_max_dim);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// open video_liveview modules (sub)
	ret = open_module_2(&stream[1], out_type);
	if (ret != HD_OK) {
		printf("open2 fail=%d\n", ret);
		goto exit;
	}

	// set open config
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		p_net_cfg = &net_cfg[i];
		ret = network_set_config(stream[2+i].net_path, p_net_cfg);
		if (HD_OK != ret) {
			printf("proc_id(%u) network_set_config fail=%d\n", stream[2+i].net_path, ret);
			goto exit;
		}
	}

	// open video_liveview modules (sub2)
	ret = open_module_3(&stream[2]);
	if (ret != HD_OK) {
		printf("open3 fail=%d\n", ret);
		goto exit;
	}

	// open video_liveview modules (sub3)
	ret = open_module_4(&stream[3]);
	if (ret != HD_OK) {
		printf("open4 fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability
	ret = get_cap_caps(stream[0].cap_ctrl, &stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(stream[1].out_ctrl, &stream[1].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	stream[1].out_max_dim = stream[1].out_syscaps.output_dim;

	// set videocap parameter
	ret = set_cap_param(stream[0].cap_path, &stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// assign parameter by program options
	main_dim.w = VDO_SIZE_W;
	main_dim.h = VDO_SIZE_H;
	
	out_dim.w = stream[1].out_max_dim.w; //display device size
	out_dim.h = stream[1].out_max_dim.h; //display device size
	sub_dim.w = out_dim.w;
	sub_dim.h = out_dim.h;
	
	sub2_dim.w = main_dim.w;
	sub2_dim.h = main_dim.h;

	sub3_dim.w = main_dim.w;
	sub3_dim.h = main_dim.h;

	// set videoproc parameter (main)
	ret = set_proc_param(stream[0].proc_path, &main_dim, HD_VIDEO_DIR_NONE, 0);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (sub)
	ret = set_proc_param_extend(stream[1].proc_path, SOURCE_PATH, NULL, NULL, HD_VIDEO_DIR_NONE, 0);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (sub2)
	ret = set_proc_param_extend(stream[2].proc_path, SOURCE_PATH, NULL, &sub2_dim, HD_VIDEO_DIR_NONE, 1);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (sub3)
	ret = set_proc_param_extend(stream[3].proc_path, SOURCE_PATH, NULL, &sub3_dim, HD_VIDEO_DIR_NONE, 1);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (sub)
	stream[1].out_dim.w = sub_dim.w;
	stream[1].out_dim.h = sub_dim.h;
	ret = set_out_param(stream[1].out_path, &stream[1].out_dim, NULL);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	//hd_videoproc_bind(SOURCE_PATH, ...); //src_path is not used

	// bind video_liveview modules (sub)
	hd_videoproc_bind(EXTEND_PATH, HD_VIDEOOUT_0_IN_0);

	// bind video_liveview modules (sub2)
	//.... bind EXTEND_PATH2 to network

	// bind video_liveview modules (sub3)
	//.... bind EXTEND_PATH3 to network

	// start video_liveview modules (main)
	hd_videocap_start(stream[0].cap_path);
	hd_videoproc_start(stream[0].proc_path);
	
	// start video_liveview modules (sub)
	hd_videoproc_start(stream[1].proc_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(stream[1].out_path);

	// start video_liveview modules (sub2 & sub3)
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		hd_videoproc_start(stream[2+i].proc_path);
		if (ai_async == 2)
			network_async_start(&stream[2+i]);
		else
			network_sync_start(&stream[2+i]);
	}

	printf("Enter q to quit\n");
	do {
		key = getchar();
		if (key == 'q' || key == 0x3) {
			break;
		}
	} while(1);

	// stop video_liveview modules (main)
	hd_videocap_stop(stream[0].cap_path);
	hd_videoproc_stop(stream[0].proc_path);

	// stop video_liveview modules (sub)
	hd_videoproc_stop(stream[1].proc_path);
	hd_videoout_stop(stream[1].out_path);

	// stop video_liveview modules (sub2 & sub3)
	for (i = 0; i < NN_RUN_NET_NUM; i++) {
		hd_videoproc_stop(stream[2+i].proc_path);
		if (ai_async == 2)
			network_async_stop(&stream[2+i]);
		else
			network_sync_stop(&stream[2+i]);
	}
	
	// destroy aquire_thread
	//pthread_join(stream[0].aquire_thread_id, (void* )NULL);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	//hd_videoproc_unbind(SOURCE_PATH); //src_path is not used

	// unbind video_liveview modules (sub)
	hd_videoproc_unbind(EXTEND_PATH);

	// unbind video_liveview modules (sub2)
	///unbind EXTEND_PATH2 from network

	// unbind video_liveview modules (sub3)
	///unbind EXTEND_PATH3 from network

exit:
	// close video_liveview modules (main)
	ret = close_module(&stream[0]);
	if (ret != HD_OK) {
		printf("close1 fail=%d\n", ret);
	}

	// close video_liveview modules (sub)
	ret = close_module_2(&stream[1]);
	if (ret != HD_OK) {
		printf("close2 fail=%d\n", ret);
	}

	// close video_liveview modules (sub2)
	ret = close_module_3(&stream[2]);
	if (ret != HD_OK) {
		printf("close3 fail=%d\n", ret);
	}

	// close video_liveview modules (sub3)
	ret = close_module_4(&stream[3]);
	if (ret != HD_OK) {
		printf("close4 fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	// uninit memory
	ret = hd_common_mem_uninit();
	if (ret != HD_OK) {
		printf("mem fail=%d\n", ret);
	}

	// uninit hdal
	ret = hd_common_uninit();
	if (ret != HD_OK) {
		printf("common fail=%d\n", ret);
	}

	return ret;
}
