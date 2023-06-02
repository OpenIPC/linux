#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>
#include "math.h"
#include "vqa_lib.h"


#if defined(__LINUX)
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
//for delay
#include <kwrap/task.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_vqa_sample_stream, argc, argv)
#endif

#define DEBUG_MENU 			1
#define DEBUG_FILE 			0
#define OUTPUT_BMP 			0
//#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
//#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
//#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////
#define HD_COMMON_MEM_VQA_POOL HD_COMMON_MEM_USER_DEFINIED_POOL

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
#define VDO_YUV_BUFSIZE(w, h, pxlfmt)	ALIGN_CEIL_4(((w) * (h) * HD_VIDEO_PXLFMT_BPP(pxlfmt)) / 8)
//NVX: YUV compress
#define YUV_COMPRESS_RATIO 75
#define VDO_NVX_BUFSIZE(w, h, pxlfmt)	(VDO_YUV_BUFSIZE(w, h, pxlfmt) * YUV_COMPRESS_RATIO / 100)
 
///////////////////////////////////////////////////////////////////////////////

#define SEN_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CAP_OUT_FMT			HD_VIDEO_PXLFMT_RAW12
#define CA_WIN_NUM_W		32
#define CA_WIN_NUM_H		32
#define LA_WIN_NUM_W		32
#define LA_WIN_NUM_H		32
#define VA_WIN_NUM_W		16
#define VA_WIN_NUM_H		16
#define YOUT_WIN_NUM_W		128
#define YOUT_WIN_NUM_H		128
#define ETH_8BIT_SEL		0 //0: 2bit out, 1:8 bit out
#define ETH_OUT_SEL			1 //0: full, 1: subsample 1/2

#define VDO_SIZE_W			1920
#define VDO_SIZE_H			1080
#define	VDO_FRAME_FORMAT	HD_VIDEO_PXLFMT_YUV420

#define MDBC_ALIGN(a, b) (((a) + ((b) - 1)) / (b) * (b))
#define IMG_WIDTH           320
#define IMG_HEIGHT          180
#define IMG_BUF_SIZE        (IMG_WIDTH * IMG_HEIGHT)
#define VQA_THREAD_NUM		1

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

	// (4) --
	HD_VIDEOPROC_SYSCAPS proc_alg_syscaps;
	HD_PATH_ID proc_alg_ctrl;
	HD_PATH_ID proc_alg_path;

	HD_DIM  proc_alg_max_dim;
	HD_DIM  proc_alg_dim;

	// (5) --
	HD_PATH_ID mask_alg_path;

    HD_VIDEOOUT_HDMI_ID hdmi_id;
} VIDEO_LIVEVIEW;

typedef struct _VQA_MEM_RANGE {
	UINT32               va;        ///< Memory buffer starting address
	UINT32               addr;      ///< Memory buffer starting address
	UINT32               size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} VQA_MEM_RANGE, *PVQA_MEM_RANGE;

#pragma pack(2)
struct BmpFileHeader {
    UINT16 bfType;
    UINT32 bfSize;
    UINT16 bfReserved1;
    UINT16 bfReserved2;
    UINT32 bfOffBits;
};
struct BmpInfoHeader {
    UINT32 biSize;
    UINT32 biWidth;
    UINT32 biHeight;
    UINT16 biPlanes; // 1=defeaul, 0=custom
    UINT16 biBitCount;
    UINT32 biCompression;
    UINT32 biSizeImage;
    UINT32 biXPelsPerMeter; // 72dpi=2835, 96dpi=3780
    UINT32 biYPelsPerMeter; // 120dpi=4724, 300dpi=11811
    UINT32 biClrUsed;
    UINT32 biClrImportant;
};
#pragma pack()

static VIDEO_LIVEVIEW g_stream[1] = {0}; //0: main stream
static UINT32 g_endFlag;
static UINT32 g_shdr_mode = 0;

static INT32 share_memory_init(VQA_MEM_RANGE *p_share_mem)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT8 i;
	UINT32 pa, va;
	UINT32 blk_size = IMG_BUF_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret = HD_OK;

	for(i=0;i<4;i++){
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	for(i=0;i<4;i++){
		if(i==0)blk_size = IMG_BUF_SIZE;
		else if(i==1 || i==2)blk_size = IMG_BUF_SIZE/2;
		else if(i==3)blk_size = vqa_calc_buf_size();
		blk = hd_common_mem_get_block(HD_COMMON_MEM_VQA_POOL, blk_size, ddr_id);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			printf("err:get block fail\r\n", blk);
			ret =  HD_ERR_NG;
			return ret;
		}
		pa = hd_common_mem_blk2pa(blk);
		if (pa == 0) {
			printf("err:blk2pa fail, blk = 0x%x\r\n", blk);
			goto blk2pa_err;
		}
		printf("pa = 0x%x\r\n", pa);
		if (pa > 0) {
			va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
			if (va == 0) {
				goto map_err;
			}
		}
		p_share_mem[i].addr = pa;
		p_share_mem[i].va   = va;
		p_share_mem[i].size = blk_size;
		p_share_mem[i].blk  = blk;
		printf("share_mem pa = 0x%x, va=0x%x, size =0x%x\r\n", p_share_mem[i].addr, p_share_mem[i].va, p_share_mem[i].size);
	}
	return ret;
blk2pa_err:
map_err:
	for (; i > 0 ;) {
		i -= 1;
		ret = hd_common_mem_release_block(p_share_mem[i].blk);
		if (HD_OK != ret) {
			printf("err:release blk fail %d\r\n", ret);
			ret =  HD_ERR_NG;
			return ret;
		}
	}
	return ret;
}

static INT32 share_memory_exit(VQA_MEM_RANGE *p_share_mem)
{
	UINT8 i;
	HD_RESULT ret = HD_OK;

	for(i=0;i<4;i++){
		if (p_share_mem[i].va != 0) {
			ret = hd_common_mem_munmap((void *)p_share_mem[i].va, p_share_mem[i].size);
			if (HD_OK != ret) {
                printf("mnumap error !!\r\n\r\n");
				ret = HD_ERR_NG;
				return ret;
            }
		}
		if (p_share_mem[i].blk != HD_COMMON_MEM_VB_INVALID_BLK) {
			ret = hd_common_mem_release_block(p_share_mem[i].blk);
			if (HD_OK != ret) {
				printf("err:release blk fail %d\r\n", ret);
				ret =  HD_ERR_NG;
				return ret;
			}
		}
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	return ret;
}

static HD_RESULT mem_init(void)
{
	HD_RESULT ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

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
	
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_VQA_POOL;
	mem_cfg.pool_info[2].blk_size = IMG_BUF_SIZE;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;
	
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_VQA_POOL;
	mem_cfg.pool_info[3].blk_size = IMG_BUF_SIZE/2;
	mem_cfg.pool_info[3].blk_cnt = 2;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;
		
	// vqa buffer
	mem_cfg.pool_info[4].type = HD_COMMON_MEM_VQA_POOL;
	mem_cfg.pool_info[4].blk_size = vqa_calc_buf_size();
	mem_cfg.pool_info[4].blk_cnt = 1;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;
	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
		ret =  HD_ERR_NG;
		return ret;
	}
	return ret;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	ret = hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT get_cap_caps(HD_PATH_ID video_cap_ctrl, HD_VIDEOCAP_SYSCAPS *p_video_cap_syscaps)
{
	HD_RESULT ret = HD_OK;
	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSCAPS, p_video_cap_syscaps);
	return ret;
}

static HD_RESULT get_cap_sysinfo(HD_PATH_ID video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_SYSINFO sys_info = {0};

	hd_videocap_get(video_cap_ctrl, HD_VIDEOCAP_PARAM_SYSINFO, &sys_info);
	printf("sys_info.devid =0x%X, cur_fps[0]=%d/%d, vd_count=%llu\r\n", sys_info.dev_id, GET_HI_UINT16(sys_info.cur_fps[0]), GET_LO_UINT16(sys_info.cur_fps[0]), sys_info.vd_count);
	return ret;
}

static HD_RESULT set_cap_cfg(HD_PATH_ID *p_video_cap_ctrl)
{
	HD_RESULT ret = HD_OK;
	HD_VIDEOCAP_DRV_CONFIG cap_cfg = {0};
	HD_PATH_ID video_cap_ctrl = 0;
	HD_VIDEOCAP_CTRL iq_ctl = {0};
	char *chip_name = getenv("NVT_CHIP_ID");

#if 1
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
#else

	printf("use os02k10\n");
    snprintf(cap_cfg.sen_cfg.sen_dev.driver_name, HD_VIDEOCAP_SEN_NAME_LEN-1, "nvt_sen_os02k10");
    cap_cfg.sen_cfg.sen_dev.if_type = HD_COMMON_VIDEO_IN_MIPI_CSI;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.sensor_pinmux =  0x220; //PIN_SENSOR_CFG_MIPI | PIN_SENSOR_CFG_MCLK
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.serial_if_pinmux = 0xF01;
    cap_cfg.sen_cfg.sen_dev.pin_cfg.pinmux.cmd_if_pinmux = 0x10;//PIN_I2C_CFG_CH2
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

#endif


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
		video_in_param.out_frame_num = HD_VIDEOCAP_SEN_FRAME_NUM_1;
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
		video_out_param.pxlfmt = CAP_OUT_FMT;
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

///////////////////////////////////////////////////////////////////////////////

static HD_RESULT init_module(void)
{
	HD_RESULT ret;
	if ((ret = hd_videocap_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_init()) != HD_OK)
		return ret;
	if ((ret = hd_videoout_init()) != HD_OK)
		return ret;
	return HD_OK;
}

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
	// set videoproc config
	ret = set_proc_cfg(&p_stream->proc_alg_ctrl, p_proc_max_dim);
	if (ret != HD_OK) {
		printf("set proc-cfg alg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	// set videoout config
	ret = set_out_cfg(&p_stream->out_ctrl, out_type,p_stream->hdmi_id);
	if (ret != HD_OK) {
		printf("set out-cfg fail=%d\n", ret);
		return HD_ERR_NG;
	}
	if ((ret = hd_videocap_open(HD_VIDEOCAP_0_IN_0, HD_VIDEOCAP_0_OUT_0, &p_stream->cap_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_0, &p_stream->proc_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoproc_open(HD_VIDEOPROC_0_IN_0, HD_VIDEOPROC_0_OUT_1, &p_stream->proc_alg_path)) != HD_OK)
		return ret;
	if ((ret = hd_videoout_open(HD_VIDEOOUT_0_IN_0, HD_VIDEOOUT_0_OUT_0, &p_stream->out_path)) != HD_OK)
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
	return HD_OK;
}

static HD_RESULT vqa_set_para(UINT8 en_blur, UINT32 blur_strength, UINT32 blur_cover_th, UINT32 blur_alarm_times)
{
	vqa_param_t vqa_param = {0};
	HD_RESULT ret = HD_OK;

	// init VQA default param
	//enable
	vqa_param.enable_param.en_too_light = 1;
	vqa_param.enable_param.en_too_dark = 1;
	vqa_param.enable_param.en_blur = en_blur;
	vqa_param.enable_param.en_auto_adj_param = 0;
	vqa_param.enable_param.en_ref_md = 0;
	//global
	vqa_param.global_param.height = 480;
	vqa_param.global_param.width = 640;
	vqa_param.global_param.mb_y_size = 40;
	vqa_param.global_param.mb_x_size = 32;
	vqa_param.global_param.auto_adj_period = 5;
	vqa_param.global_param.g_alarm_frame_num = 20;
	//overexposure
	vqa_param.light_param.too_light_strength_th = 215;
	vqa_param.light_param.too_light_cover_th = 15;
	vqa_param.light_param.too_light_alarm_times = 5;
	//underexposure
	vqa_param.light_param.too_dark_strength_th = 50;
	vqa_param.light_param.too_dark_cover_th = 75;
	vqa_param.light_param.too_dark_alarm_times = 5;
	//blur
	vqa_param.contrast_param.blur_strength = blur_strength;
	vqa_param.contrast_param.blur_cover_th = blur_cover_th;
	vqa_param.contrast_param.blur_alarm_times = blur_alarm_times;
	vqa_set_param(0, &vqa_param);
	
	
	return ret;
}

static VOID *vqa_thread_api(VOID *arg)
{
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 ping_pong_id = 0,is_Init=0;
	HD_RESULT ret = HD_OK;
	//UINT32 reg_id=0;
	VQA_MEM_RANGE *p_share_mem = (VQA_MEM_RANGE*)arg;
	UINT32 imgY_va,imgUV_va;
	int counter = 0;
#if OUTPUT_BMP
	char ImgFilePath[64];
	int frmidx = 0;
#endif
#if (DEBUG_FILE == 1)
	char out_file1[64];
	char out_file2[64];
	UINT32 file_size = 0;
	FILE  *fd;
#endif
	vqa_res_t vqa_rst;
	vqa_init(0, p_share_mem[3].va);
	vqa_set_para(1, 8, 60, 5);
	while(g_endFlag==0) 
	{
		ret = hd_videoproc_pull_out_buf(g_stream[0].proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if(ret != HD_OK) {
			printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
			goto gen_init_fail;
		}
		if(counter >30) {	
			//printf("video_frame.dim.w : %d\n\r", video_frame.dim.w);
			//printf("video_frame.dim.h : %d\n\r", video_frame.dim.h);
			//printf("video_frame.phy_addr[0] : 0x%08x\n\r", video_frame.phy_addr[0]);
			//printf("video_frame.phy_addr[1] : 0x%08x\n\r", video_frame.phy_addr[1]);
			imgY_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], IMG_BUF_SIZE+(IMG_BUF_SIZE/2));
			if (imgY_va == 0) {
                printf("Error: mmap fail !!\r\n");
                goto gen_init_fail;
            }

			imgUV_va = imgY_va+IMG_BUF_SIZE;
	#if (DEBUG_FILE == 1)
			//printf("video_frame[0].imgY_va : 0x%08x\n\r", imgY_va);
			snprintf(out_file1, 64, "//mnt//sd//output_y//debug_y.bin");	
			frame_idx++;
			fd = fopen(out_file1, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file1);
				return 0;
			}
			file_size = fwrite((const void *)imgY_va,1,IMG_BUF_SIZE,fd);
			if (file_size == 0) {
				printf("cannot write debug_y.bin\r\n");
				return 0;
			}
			fclose(fd);		
			//printf("video_frame[1].imgUV_va : 0x%08x\n\r", imgUV_va);
			//printf("uv[0] : 0x%08x\n\r", *((UINT32*)imgUV_va));
			//printf("uv[1] : 0x%08x\n\r", *((UINT32*)imgUV_va+4));
			snprintf(out_file2, 64, "//mnt//sd//output_uv//debug_uv.bin");	
			fd = fopen(out_file2, "wb");
			if (!fd) {
				printf("cannot read %s\r\n", out_file2);
				return 0;
			}
			file_size = fwrite((const void *)imgUV_va,1,(IMG_BUF_SIZE/2),fd);
			if (file_size == 0) {
				printf("cannot write debug_uv.bin\r\n");
				return 0;
			}
			fclose(fd);	
	#endif
			
			p_share_mem[0].addr = video_frame.phy_addr[0];
			p_share_mem[1].addr = video_frame.phy_addr[1];

			memcpy((UINT32 *)p_share_mem[2].va , (UINT32 *)imgUV_va , IMG_BUF_SIZE/2);
			
			if(is_Init == 1) {
				//p_share_mem[10].va : motion bitmap 8 bit per pixel
				ret = vqa_run(0, 0, (UINT8*)imgY_va, IMG_WIDTH, IMG_HEIGHT, 0, &vqa_rst);
				if (ret != HD_OK) {
					printf("run vqa fail!\n");
				}
				//if (vqa_rst.res_too_light > 0 || vqa_rst.res_too_dark > 0 || vqa_rst.res_blur > 0) {
				printf("too light:%d, too dark:%d, blur:%d\n", (int)vqa_rst.res_too_light, (int)vqa_rst.res_too_dark, (int)vqa_rst.res_blur);
				//}
			}	
			ret = hd_common_mem_munmap((void *)imgY_va, IMG_BUF_SIZE+(IMG_BUF_SIZE/2));
			if (HD_OK != ret) {
                printf("mnumap error !!\r\n\r\n");
				goto gen_init_fail;
            }
			
			ping_pong_id = (ping_pong_id+1)%2;
			if(is_Init==0)is_Init=1;
		}  
		else if(counter <=30){
			counter++;
		}
		ret = hd_videoproc_release_out_buf(g_stream[0].proc_alg_path, &video_frame);
		if(ret != HD_OK) {
			printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
			goto gen_init_fail;
		}
	}

gen_init_fail:

	return 0;
}

//int main(int argc, char** argv)
MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key, idx=0;
	//INT idx=0;
	VQA_MEM_RANGE share_mem[4];
	UINT32 out_type=1;
	pthread_t vqa_thread_id[VQA_THREAD_NUM];
	UINT32 blur_strength=8;
	UINT32 blur_cover_th=85;
	UINT32 blur_alarm_times=5;
	UINT8  en_blur=1;
	UINT32 vqa_version;

	vqa_version = vqa_get_version(VQA_VERSION);
	printf("VQA: version(0x%x)\r\n", (unsigned int) vqa_version);

	g_endFlag = 0;
	// query program options
	if (argc >= 2) {
		out_type = atoi(argv[1]);
		printf("out_type %d\r\n", out_type);
		if(out_type > 2) {
			printf("error: not support out_type!\r\n");
			return 0;
		}
	}
    g_stream[0].hdmi_id=HD_VIDEOOUT_HDMI_1920X1080I60;//default
    
	// query program options
	if (argc >= 3 && (atoi(argv[2]) !=0)) {
		g_stream[0].hdmi_id = atoi(argv[2]);
		printf("hdmi_mode %d\r\n", g_stream[0].hdmi_id);
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

	ret = share_memory_init(share_mem);
	if (ret != HD_OK) {
		printf("mem_init fail=%d\n", ret);
		goto exit;
	}

	// init all modules
	ret = init_module();	// vdocap, vdoproc, vdoout
	if (ret != HD_OK) {
		printf("init fail=%d\n", ret);
		goto exit;
	}

	// open video_liveview modules (main)
	g_stream[0].proc_max_dim.w = VDO_SIZE_W; //assign by user
	g_stream[0].proc_max_dim.h = VDO_SIZE_H; //assign by user
	ret = open_module(&g_stream[0], &g_stream[0].proc_max_dim, out_type);
	if (ret != HD_OK) {
		printf("open fail=%d\n", ret);
		goto exit;
	}

	// get videocap capability
	ret = get_cap_caps(g_stream[0].cap_ctrl, &g_stream[0].cap_syscaps);
	if (ret != HD_OK) {
		printf("get cap-caps fail=%d\n", ret);
		goto exit;
	}

	// get videoout capability
	ret = get_out_caps(g_stream[0].out_ctrl, &g_stream[0].out_syscaps);
	if (ret != HD_OK) {
		printf("get out-caps fail=%d\n", ret);
		goto exit;
	}
	g_stream[0].out_max_dim = g_stream[0].out_syscaps.output_dim;

	// set videocap parameter
	g_stream[0].cap_dim.w = VDO_SIZE_W; //assign by user
	g_stream[0].cap_dim.h = VDO_SIZE_H; //assign by user
	ret = set_cap_param(g_stream[0].cap_path, &g_stream[0].cap_dim);
	if (ret != HD_OK) {
		printf("set cap fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (main)
	ret = set_proc_param(g_stream[0].proc_path, NULL);
	if (ret != HD_OK) {
		printf("set proc fail=%d\n", ret);
		goto exit;
	}

	// set videoproc parameter (alg)
	g_stream[0].proc_alg_max_dim.w = IMG_WIDTH;//VDO_SIZE_W;
	g_stream[0].proc_alg_max_dim.h = IMG_HEIGHT;//VDO_SIZE_H;
	ret = set_proc_param(g_stream[0].proc_alg_path, &g_stream[0].proc_alg_max_dim);
	if (ret != HD_OK) {
		printf("set proc alg fail=%d\n", ret);
		goto exit;
	}

	// set videoout parameter (main)
	g_stream[0].out_dim.w = g_stream[0].out_max_dim.w; //using device max dim.w
	g_stream[0].out_dim.h = g_stream[0].out_max_dim.h; //using device max dim.h
	ret = set_out_param(g_stream[0].out_path, &g_stream[0].out_dim);
	if (ret != HD_OK) {
		printf("set out fail=%d\n", ret);
		goto exit;
	}

	// bind video_liveview modules (main)
	hd_videocap_bind(HD_VIDEOCAP_0_OUT_0, HD_VIDEOPROC_0_IN_0);
	hd_videoproc_bind(HD_VIDEOPROC_0_OUT_0, HD_VIDEOOUT_0_IN_0);

	// create thread (pull_out bitstream)
	for (idx = 0; idx < VQA_THREAD_NUM; idx++) {
		// create thread
		ret = pthread_create(&vqa_thread_id[idx], NULL, vqa_thread_api, (VOID*)share_mem);
		if (ret < 0) {
			printf("create encode thread failed");
			goto exit;
		}
	}

	// start video_liveview modules (main)
	hd_videocap_start(g_stream[0].cap_path);
	hd_videoproc_start(g_stream[0].proc_path);
	hd_videoproc_start(g_stream[0].proc_alg_path);
	// just wait ae/awb stable for auto-test, if don't care, user can remove it
	sleep(1);
	hd_videoout_start(g_stream[0].out_path);

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			g_endFlag = 1;
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
		
		if (key == 'e') {
            if (en_blur == 0) {
                en_blur = 1;
            } else {
				en_blur = 0;
			}
            vqa_set_para(en_blur, blur_strength, blur_cover_th, blur_alarm_times);
            printf("VQA: updat en_blur=%d\r\n", en_blur);
        }

		if (key == 'u') {
			blur_strength--;
			if (blur_strength == 0) {
				blur_strength = 8;
			}
			vqa_set_para(en_blur, blur_strength, blur_cover_th, blur_alarm_times);
			printf("VQA: updat blur_strength=%d\r\n", blur_strength);
		}

		if (key == 'v') {
			blur_cover_th -= 10;
			if (blur_cover_th < 10) {
				blur_cover_th = 95;
			}
			vqa_set_para(en_blur, blur_strength, blur_cover_th, blur_alarm_times);
			printf("VQA: updat blur_cover_th=%d\r\n", blur_cover_th);
		}

		if (key == 'r') {
			blur_alarm_times += 50;
			if (blur_alarm_times > 200) {
				blur_alarm_times = 5;
			}
			vqa_set_para(en_blur, blur_strength, blur_cover_th, blur_alarm_times);
			printf("VQA: updat blur_alarm_times=%d\r\n", blur_alarm_times);
		}

		if (key == '0') {
			get_cap_sysinfo(g_stream[0].cap_ctrl);
		}
	}


	// wait encode thread destroyed
	for (idx = 0; idx < VQA_THREAD_NUM; idx++) {
		pthread_join(vqa_thread_id[idx], NULL);
	}

	// stop video_liveview modules (main)
	hd_videocap_stop(g_stream[0].cap_path);
	hd_videoproc_stop(g_stream[0].proc_path);
	hd_videoproc_stop(g_stream[0].proc_alg_path);
	hd_videoout_stop(g_stream[0].out_path);

	// unbind video_liveview modules (main)
	hd_videocap_unbind(HD_VIDEOCAP_0_OUT_0);
	hd_videoproc_unbind(HD_VIDEOPROC_0_OUT_0);

exit:
	
	// close video_liveview modules (main)
	ret = close_module(&g_stream[0]);
	if (ret != HD_OK) {
		printf("close fail=%d\n", ret);
	}

	// uninit all modules
	ret = exit_module();
	if (ret != HD_OK) {
		printf("exit fail=%d\n", ret);
	}

	ret = share_memory_exit(share_mem);
	if (ret != HD_OK) {
		printf("mem_uninit fail=%d\n", ret);
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
