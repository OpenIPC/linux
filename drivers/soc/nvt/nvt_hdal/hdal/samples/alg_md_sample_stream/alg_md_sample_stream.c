#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"
#include <kwrap/examsys.h>
#include "math.h"
#include "vendor_md.h"
#include "libmd.h"
#include "kwrap/perf.h"


#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/task.h>
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    vos_task_delay_ms(1000*x)
#define msleep(x)   vos_task_delay_ms(x)
#define usleep(x)   vos_task_delay_us(x)
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(alg_md_sample_stream, argc, argv)
#endif

#define DEBUG_MENU 			1
#define DEBUG_FILE 			0
#define OUTPUT_BMP 			0
#define SHARE_MEM_NUM		8
//#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
//#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
//#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

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
#define VENDOR_CV_CFG  		0x00f00000  //cv project config
#define MD_THREAD_NUM		1

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

typedef struct _MEM_RANGE {
	UINT32               va;        ///< Memory buffer starting address
	UINT32               addr;      ///< Memory buffer starting address
	UINT32               size;      ///< Memory buffer size
	HD_COMMON_MEM_VB_BLK blk;
} MEM_RANGE, *PMEM_RANGE;

typedef enum {
	LOW_SENSI         = 0,
	MED_SENSI         = 1,
	HIGH_SENSI        = 2,
	DEFAULT_SENSI     = 3,
	ENUM_DUMMY4WORD(MDBC_PARA_SENSI)
} MDBC_PARA_SENSI;

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

UINT32 g_sensi = HIGH_SENSI;//DEFAULT_SENSI;
VIDEO_LIVEVIEW g_stream[1] = {0}; //0: main stream
UINT32 g_endFlag;
int g_scene_change_alarm_th = 50;

static INT32 share_memory_init(MEM_RANGE *p_share_mem)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT8 i;
	UINT32 pa, va;
	UINT32 blk_size = IMG_BUF_SIZE;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_RESULT ret = HD_OK;

	for(i=0;i<SHARE_MEM_NUM;i++){
		p_share_mem[i].addr = 0x00;
		p_share_mem[i].va   = 0x00;
		p_share_mem[i].size = 0x00;
		p_share_mem[i].blk  = HD_COMMON_MEM_VB_INVALID_BLK;
	}
	for(i=0;i<SHARE_MEM_NUM;i++){
		if(i==0)blk_size = IMG_BUF_SIZE;
		else if(i==1 || i==2)blk_size = IMG_BUF_SIZE/2;
		else if(i==3)blk_size = IMG_BUF_SIZE*40;
		else if(i==4)blk_size = IMG_BUF_SIZE*6;
		else if(i==5)blk_size = ((IMG_BUF_SIZE+15)/16)*16*12;
		else if(i==6)blk_size = ((IMG_BUF_SIZE+7)/8);
		else if(i==7)blk_size = IMG_BUF_SIZE; // result transform
		blk = hd_common_mem_get_block(HD_COMMON_MEM_GLOBAL_MD_POOL, blk_size, ddr_id);
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
		printf("share_mem[%d] pa = 0x%x, va=0x%x, size =0x%x\r\n",i, p_share_mem[i].addr, p_share_mem[i].va, p_share_mem[i].size);
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

static INT32 share_memory_exit(MEM_RANGE *p_share_mem)
{
	UINT8 i;
	HD_RESULT ret = HD_OK;
	for(i=0;i<SHARE_MEM_NUM;i++){
		if (p_share_mem[i].va != 0) {
			hd_common_mem_munmap((void *)p_share_mem[i].va, p_share_mem[i].size);
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
	return HD_OK;
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

	// for md
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[2].blk_size = IMG_BUF_SIZE;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;

	mem_cfg.pool_info[3].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[3].blk_size = IMG_BUF_SIZE/2;
	mem_cfg.pool_info[3].blk_cnt = 2;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;

	mem_cfg.pool_info[4].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[4].blk_size = IMG_BUF_SIZE*40;
	mem_cfg.pool_info[4].blk_cnt = 1;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;

	mem_cfg.pool_info[5].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[5].blk_size = IMG_BUF_SIZE*6;
	mem_cfg.pool_info[5].blk_cnt = 1;
	mem_cfg.pool_info[5].ddr_id = DDR_ID0;

	mem_cfg.pool_info[6].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[6].blk_size = ((IMG_BUF_SIZE+15)/16)*16*12;
	mem_cfg.pool_info[6].blk_cnt = 1;
	mem_cfg.pool_info[6].ddr_id = DDR_ID0;

	mem_cfg.pool_info[7].type = HD_COMMON_MEM_GLOBAL_MD_POOL;
	mem_cfg.pool_info[7].blk_size = ((IMG_BUF_SIZE+7)/8);
	mem_cfg.pool_info[7].blk_cnt = 1;
	mem_cfg.pool_info[7].ddr_id = DDR_ID0;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		printf("err:hd_common_mem_init err: %d\r\n", ret);
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
	// set videoproc config for md
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

/*
static UINT32 md_load_file(CHAR *p_filename, UINT32 va)
{
	FILE  *fd;
	UINT32 file_size = 0, read_size = 0;
	const UINT32 addr = va;
	//printf("addr = %08x\r\n", (int)addr);

	fd = fopen(p_filename, "rb");
	if (!fd) {
		printf("cannot read %s\r\n", p_filename);
		return 0;
	}

	fseek ( fd, 0, SEEK_END );
	file_size = ALIGN_CEIL_4( ftell(fd) );
	fseek ( fd, 0, SEEK_SET );

	read_size = fread ((void *)addr, 1, file_size, fd);
	if (read_size != file_size) {
		printf("size mismatch, real = %d, idea = %d\r\n", (int)read_size, (int)file_size);
	}
	fclose(fd);
	return read_size;
}
*/

void bc_reorgS1(UINT8* inputS, UINT8* outputS,UINT32 width, UINT32 height)
{
	UINT32 i,j,count,size;
	count=0;
	size = width*height;
	for(j = 0; j < MDBC_ALIGN(size,8)/8; j++) {
		UINT8 c = inputS[j];
        for(i = 0; i < 8; i++) {
			if(count<size)
			{
				outputS[count] = c & 0x1;
				c = c>>1;
				count++;
			}
        }
    }
}

void bc_writebmpfile(char* name, UINT8* raw_img,
    int width, int height, UINT16 bits)
{
    if(!(name && raw_img)) {
        printf("Error bmpWrite.");
        return;
    }
	int i,j;//,length;
    // FileHeader
    struct BmpFileHeader file_h = {
        .bfType=0x4d42,
        .bfSize=0,
        .bfReserved1=0,
        .bfReserved2=0,
        .bfOffBits=54,
    };
    file_h.bfSize = file_h.bfOffBits + width*height * bits/8;
    if(bits==8) {file_h.bfSize += 1024, file_h.bfOffBits += 1024;}
    // BmpInfoHeader
    struct BmpInfoHeader info_h = {
        .biSize=40,
        .biWidth=0,
        .biHeight=0,
        .biPlanes=1,
        .biBitCount=0,
        .biCompression=0,
        .biSizeImage=0,
        .biXPelsPerMeter=0,
        .biYPelsPerMeter=0,
        .biClrUsed=0,
        .biClrImportant=0,
    };
    info_h.biWidth = width;
    info_h.biHeight = height;
    info_h.biBitCount = bits;
    info_h.biSizeImage = width*height * bits/8;
    if(bits == 8) {
        info_h.biClrUsed=256;
        info_h.biClrImportant=256;
    }
    // Write Header
	/*
	INT32 FileHandleStatus = 0;
	FST_FILE filehdl = NULL;
	filehdl = FileSys_OpenFile(name, FST_OPEN_WRITE | FST_CREATE_ALWAYS);
	if (!filehdl) {
		emu_msg(("^ROpen file fail - %s...\r\n", name));
	}
	length = sizeof(file_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&file_h, &length, 0, NULL);
	length = sizeof(info_h);
	FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&info_h, &length, 0, NULL);
	*/
    FILE *pFile = fopen(name,"wb+");
    if(!pFile) {
        printf("Error opening file.");
        return;
    }
    fwrite((char*)&file_h, sizeof(char), sizeof(file_h), pFile);
    fwrite((char*)&info_h, sizeof(char), sizeof(info_h), pFile);

	//length = 1;
	// Write colormap
	//printf("Write colormap...\r\n");
    if(bits == 8) {
        for(i = 0; i < 256; ++i) {
            UINT8 c = i;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
    // Write raw img
	//printf("Write raw img...\r\n");
    UINT8 alig = ((width*bits/8)*3) % 4;
    for(j = height-1; j >= 0; --j) {
		//printf("j : %d\r\n",j);
        for(i = 0; i < width; ++i) {
			UINT8 c;
            if(bits == 24) {
				//c = raw_img[(j*width+i)*3 + 2];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 1];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
				//c = raw_img[(j*width+i)*3 + 0];
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&raw_img[(j*width+i)*3 + 2], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 1], sizeof(char), sizeof(UINT8), pFile);
                fwrite((char*)&raw_img[(j*width+i)*3 + 0], sizeof(char), sizeof(UINT8), pFile);
            } else if(bits == 8) {
				if(raw_img[j*width+i]==1){ c = 255;}
				else if(raw_img[j*width+i]==0){ c = 0;}
				else{
					printf("raw_img[j*width+i] = %d, (%d,%d)\n",raw_img[j*width+i],i,j);
					c = 0;
				}
				//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
                fwrite((char*)&c, sizeof(char), sizeof(UINT8), pFile);
            }
        }
        // 4byte align
        for(i = 0; i < alig; ++i) {
			//UINT8 c = 0;
			//FileHandleStatus = FileSys_WriteFile(filehdl, (UINT8 *)&c, &length, 0, NULL);
            fwrite("", sizeof(char), sizeof(UINT8), pFile);
        }
    }
	//FileHandleStatus = FileSys_CloseFile(filehdl);
	//if (FileHandleStatus != FST_STA_OK) {
	//	emu_msg(("^RClose file fail - %s...\r\n", name));
	//}
    fclose(pFile);
}

static void md_get_para_sensitivity(void)
{
	switch (g_sensi) {
	case LOW_SENSI:
		printf("\r\n--- MDBC low sensitivity ---\r\n");
		break;
	case MED_SENSI:
		printf("\r\n--- MDBC medium sensitivity ---\r\n");
		break;
	case HIGH_SENSI:
		printf("\r\n--- MDBC high sensitivity ---\r\n");
		break;
	case DEFAULT_SENSI:
	default:
		printf("\r\n--- MDBC default sensitivity ---\r\n");
		break;
	}
}

static void md_set_para_default_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0xa;
	mdbc_parm->MdmatchPara.d_colour   = 0xf;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 0x3;
	mdbc_parm->MdmatchPara.r_lbsp     = 0x5;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 0x14;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x4;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x8;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x1;
	mdbc_parm->MorPara.mor_sel1   = 0x1;
	mdbc_parm->MorPara.mor_sel2   = 0x1;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x4;
	mdbc_parm->UpdPara.maxT           = 0x80;
	mdbc_parm->UpdPara.maxFgFrm       = 0xff;
	mdbc_parm->UpdPara.deghost_dth    = 0xa;
	mdbc_parm->UpdPara.deghost_sth    = 0xf0;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 10;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_high_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 8;
	mdbc_parm->MdmatchPara.r_colour   = 0x10;
	mdbc_parm->MdmatchPara.d_lbsp     = 3;
	mdbc_parm->MdmatchPara.r_lbsp     = 5;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 0x33;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 0x28;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 8;
	mdbc_parm->MdmatchPara.s_alpha    = 0x28;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x10;
	mdbc_parm->UpdPara.maxT           = 0x80;
	mdbc_parm->UpdPara.maxFgFrm       = 0x0;
	mdbc_parm->UpdPara.deghost_dth    = 0xf;
	mdbc_parm->UpdPara.deghost_sth    = 0xf0;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_medium_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 10;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 4;
	mdbc_parm->MdmatchPara.r_lbsp     = 8;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 25;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 100;
	mdbc_parm->MdmatchPara.min_match  = 2;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 10;
	mdbc_parm->MdmatchPara.s_alpha    = 100;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 12;
	mdbc_parm->UpdPara.maxT           = 0x80;
	mdbc_parm->UpdPara.maxFgFrm       = 0x80;
	mdbc_parm->UpdPara.deghost_dth    = 50;
	mdbc_parm->UpdPara.deghost_sth    = 205;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static void md_set_para_low_sensitivity(VENDOR_MD_PARAM *mdbc_parm)
{
	mdbc_parm->MdmatchPara.lbsp_th    = 0x0;
	mdbc_parm->MdmatchPara.d_colour   = 12;
	mdbc_parm->MdmatchPara.r_colour   = 0x1e;
	mdbc_parm->MdmatchPara.d_lbsp     = 5;
	mdbc_parm->MdmatchPara.r_lbsp     = 10;
	mdbc_parm->MdmatchPara.model_num  = 0x8;
	mdbc_parm->MdmatchPara.t_alpha    = 25;
	mdbc_parm->MdmatchPara.dw_shift   = 0x4;
	mdbc_parm->MdmatchPara.dlast_alpha= 100;
	mdbc_parm->MdmatchPara.min_match  = 1;
	mdbc_parm->MdmatchPara.dlt_alpha  = 0xa;
	mdbc_parm->MdmatchPara.dst_alpha  = 0x28;
	mdbc_parm->MdmatchPara.uv_thres   = 16;
	mdbc_parm->MdmatchPara.s_alpha    = 100;
	mdbc_parm->MdmatchPara.dbg_lumDiff= 0x0;
	mdbc_parm->MdmatchPara.dbg_lumDiff_en = 0x0;

	mdbc_parm->MorPara.mor_th0 	  = 0x0;
	mdbc_parm->MorPara.mor_th1    = 0x0;
	mdbc_parm->MorPara.mor_th2    = 0x0;
	mdbc_parm->MorPara.mor_th3    = 0x0;
	mdbc_parm->MorPara.mor_th_dil = 0x0;
	mdbc_parm->MorPara.mor_sel0   = 0x0;
	mdbc_parm->MorPara.mor_sel1   = 0x0;
	mdbc_parm->MorPara.mor_sel2   = 0x0;
	mdbc_parm->MorPara.mor_sel3   = 0x0;

	mdbc_parm->UpdPara.minT           = 0x8;
	mdbc_parm->UpdPara.maxT           = 0x80;
	mdbc_parm->UpdPara.maxFgFrm       = 0x80;
	mdbc_parm->UpdPara.deghost_dth    = 50;
	mdbc_parm->UpdPara.deghost_sth    = 205;
	mdbc_parm->UpdPara.stable_frm     = 0x78;
	mdbc_parm->UpdPara.update_dyn     = 0x80;
	mdbc_parm->UpdPara.va_distth      = 32;
	mdbc_parm->UpdPara.t_distth       = 24;
	mdbc_parm->UpdPara.dbg_frmID      = 0x0;
	mdbc_parm->UpdPara.dbg_frmID_en   = 0x0;
	mdbc_parm->UpdPara.dbg_rnd        = 0x0;
	mdbc_parm->UpdPara.dbg_rnd_en     = 0x0;
}

static HD_RESULT md_set_para(MEM_RANGE *p_share_mem, UINT32 mode, UINT32 sensi)
{
	VENDOR_MD_PARAM 		   mdbc_parm;
	HD_RESULT ret = HD_OK;

	mdbc_parm.mode = mode;
	mdbc_parm.controlEn.update_nei_en = 1;
	mdbc_parm.controlEn.deghost_en    = 1;
	mdbc_parm.controlEn.roi_en0       = 0;
	mdbc_parm.controlEn.roi_en1       = 0;
	mdbc_parm.controlEn.roi_en2       = 0;
	mdbc_parm.controlEn.roi_en3       = 0;
	mdbc_parm.controlEn.roi_en4       = 0;
	mdbc_parm.controlEn.roi_en5       = 0;
	mdbc_parm.controlEn.roi_en6       = 0;
	mdbc_parm.controlEn.roi_en7       = 0;
	mdbc_parm.controlEn.chksum_en     = 0;
	mdbc_parm.controlEn.bgmw_save_bw_en = 1;
	mdbc_parm.controlEn.bc_y_only_en 	= 0;
#if 0
	if(ping_pong_id == 0)
	{
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[3].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[4].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[5].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[7].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[8].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[9].addr;
	} else {
		mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
		mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
		mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
		mdbc_parm.InInfo.uiInAddr3 = p_share_mem[7].addr;
		mdbc_parm.InInfo.uiInAddr4 = p_share_mem[8].addr;
		mdbc_parm.InInfo.uiInAddr5 = p_share_mem[9].addr;
		mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
		mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[3].addr;
		mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[4].addr;
		mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[5].addr;
	}
#endif
	mdbc_parm.InInfo.uiInAddr0 = p_share_mem[0].addr;
	mdbc_parm.InInfo.uiInAddr1 = p_share_mem[1].addr;
	mdbc_parm.InInfo.uiInAddr2 = p_share_mem[2].addr;
	mdbc_parm.InInfo.uiInAddr3 = p_share_mem[3].addr;
	mdbc_parm.InInfo.uiInAddr4 = p_share_mem[4].addr;
	mdbc_parm.InInfo.uiInAddr5 = p_share_mem[5].addr;
	mdbc_parm.OutInfo.uiOutAddr0 = p_share_mem[6].addr;
	mdbc_parm.OutInfo.uiOutAddr1 = p_share_mem[3].addr;
	mdbc_parm.OutInfo.uiOutAddr2 = p_share_mem[4].addr;
	mdbc_parm.OutInfo.uiOutAddr3 = p_share_mem[5].addr;
	
	mdbc_parm.uiLLAddr          = 0x0;
	mdbc_parm.InInfo.uiLofs0    = MDBC_ALIGN(IMG_WIDTH,4);//160;
	mdbc_parm.InInfo.uiLofs1    = MDBC_ALIGN(IMG_WIDTH,4);//160;
	mdbc_parm.Size.uiMdbcWidth  = IMG_WIDTH;
	mdbc_parm.Size.uiMdbcHeight = IMG_HEIGHT;

	switch (sensi) {
	case LOW_SENSI:
		md_set_para_low_sensitivity(&mdbc_parm);
		break;
	case MED_SENSI:
		md_set_para_medium_sensitivity(&mdbc_parm);
		break;
	case HIGH_SENSI:
		md_set_para_high_sensitivity(&mdbc_parm);
		break;
	case DEFAULT_SENSI:
	default:
		md_set_para_default_sensitivity(&mdbc_parm);
		break;
	}

	ret = vendor_md_set(VENDOR_MD_PARAM_ALL, &mdbc_parm);
	if (HD_OK != ret) {
		printf("set img fail, error code = %d\r\n", ret);
	}
	return ret;
}

static VOID *md_thread_api(VOID *arg)
{
	HD_VIDEO_FRAME video_frame = {0};
	UINT32 is_Init=0;
	HD_RESULT ret = HD_OK;
	UINT32 reg_id=0;
	MEM_RANGE *p_share_mem = (MEM_RANGE*)arg;
	UINT32 imgY_va,imgUV_va;
	VENDOR_MD_TRIGGER_PARAM md_trig_param;
	LIB_MD_MDT_LIB_INFO mdt_lib_param;
	LIB_MD_MDT_RESULT_INFO lib_md_rst;
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
	ret = vendor_md_init();
	if (HD_OK != ret) {
		printf("init fail, error code = %d\r\n", ret);
		goto gen_init_fail;
	}

	md_trig_param.is_nonblock = 0;
	md_trig_param.time_out_ms = 0;
	// LibMD motion detection info
	mdt_lib_param.mdt_info.libmd_enabled = 1;
	mdt_lib_param.mdt_info.phy_md_x_num = IMG_WIDTH;
	mdt_lib_param.mdt_info.phy_md_y_num = IMG_HEIGHT;
	mdt_lib_param.mdt_info.phy_md_rst.p_md_bitmap = (UINT8*)p_share_mem[7].va;
	mdt_lib_param.mdt_info.phy_md_rst.md_bitmap_sz = IMG_WIDTH*IMG_HEIGHT;
	if ((ret = lib_md_set(0, LIB_MD_MOTION_DETECT_INFO, &mdt_lib_param.mdt_info)) != HD_OK) {
		printf("lib_md_set enable fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD init (must after lib_md_set LIB_MD_MOTION_DETECT_INFO)
	ret = lib_md_init(0);
	if (HD_OK != ret) {
		printf("lib_md_init fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD function enable
	mdt_lib_param.mdt_enable.globel_md_alarm_detect_en = 1;
	mdt_lib_param.mdt_enable.subregion_md_alarm_detect_en = 1;
	mdt_lib_param.mdt_enable.scene_change_alarm_detect_en = 0;
	mdt_lib_param.mdt_enable.md_obj_detect_en = 0;
	if ((ret = lib_md_set(0, LIB_MD_AP_ENABLE_PARAM, &mdt_lib_param.mdt_enable)) != HD_OK) {
		printf("lib_md_set enable fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD global motion alarm
	mdt_lib_param.mdt_global_param.motion_alarm_th = 50;
	if ((ret = lib_md_set(0, LIB_MD_AP_GLOBAL_MOTION_ALARM_PARAM, &mdt_lib_param.mdt_global_param)) != HD_OK) {
		printf("lib_md_set global motion alarm param fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD sub-region motion alarm
	mdt_lib_param.mdt_subregion_param.sub_region_num = 1;
	mdt_lib_param.mdt_subregion_param.sub_region[0].enabled = 1;
	mdt_lib_param.mdt_subregion_param.sub_region[0].x_start = 0;
	mdt_lib_param.mdt_subregion_param.sub_region[0].y_start = 0;
	mdt_lib_param.mdt_subregion_param.sub_region[0].x_end = 160;
	mdt_lib_param.mdt_subregion_param.sub_region[0].y_end = 90;
	mdt_lib_param.mdt_subregion_param.sub_region[0].alarm_th = 50;
	if ((ret = lib_md_set(0, LIB_MD_AP_SUBREGION_MOTION_ALARM_PARAM, &mdt_lib_param.mdt_subregion_param)) != HD_OK) {
		printf("lib_md_set sub-region motion alarm param fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD scene change alarm
	mdt_lib_param.mdt_scene_change_param.scene_change_alarm_th = g_scene_change_alarm_th;
	if ((ret = lib_md_set(0, LIB_MD_AP_SCENE_CHANGE_ALARM_PARAM, &mdt_lib_param.mdt_scene_change_param)) != HD_OK) {
		printf("lib_md_set scene change alarm param fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}
	// LibMD Obj detect
	mdt_lib_param.mdt_obj.obj_size_th = IMG_WIDTH*IMG_HEIGHT/100;
	if ((ret = lib_md_set(0, LIB_MD_AP_OBJ_PARAM, &mdt_lib_param.mdt_obj)) != HD_OK) {
		printf("lib_md_set obj threshold param fail, error code = %d\r\n", ret);
		goto libmd_init_fail;
	}

	while(g_endFlag==0)
	{
		ret = hd_videoproc_pull_out_buf(g_stream[0].proc_alg_path, &video_frame, -1); // -1 = blocking mode, 0 = non-blocking mode, >0 = blocking-timeout mode
		if(ret != HD_OK) {
			printf("hd_videoproc_pull_out_buf fail (%d)\n\r", ret);
			goto libmd_init_fail;
		}
		if(counter >30) {
			//printf("video_frame.dim.w : %d\n\r", video_frame.dim.w);
			//printf("video_frame.dim.h : %d\n\r", video_frame.dim.h);
			//printf("video_frame.phy_addr[0] : 0x%08x\n\r", video_frame.phy_addr[0]);
			//printf("video_frame.phy_addr[1] : 0x%08x\n\r", video_frame.phy_addr[1]);
			imgY_va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, video_frame.phy_addr[0], IMG_BUF_SIZE+(IMG_BUF_SIZE/2));
			imgUV_va = imgY_va+IMG_BUF_SIZE;
	#if (DEBUG_FILE == 1)
			//printf("video_frame[0].imgY_va : 0x%08x\n\r", imgY_va);
			snprintf(out_file1, 64, "//mnt//sd//MDBCP//Debug//debug_y.bin");
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
			snprintf(out_file2, 64, "//mnt//sd//MDBCP//Debug//debug_uv.bin");
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
			md_set_para(p_share_mem, is_Init,g_sensi);

			ret = vendor_md_trigger(&md_trig_param);
			if (HD_OK != ret) {
				printf("trigger fail, error code = %d\r\n", ret);
				return 0;
			}
			memcpy((UINT32 *)p_share_mem[2].va , (UINT32 *)imgUV_va , IMG_BUF_SIZE/2);

			if(is_Init == 1) {
				bc_reorgS1((UINT8*)p_share_mem[6].va,(UINT8*)p_share_mem[7].va, IMG_WIDTH, IMG_HEIGHT);
	#if OUTPUT_BMP
				frmidx++;
				snprintf(ImgFilePath, 64, "//mnt//sd//MDBCP//Debug//output_bmp//output_%04d.bmp", frmidx);
				bc_writebmpfile(ImgFilePath, (UINT8*)p_share_mem[7].va, IMG_WIDTH, IMG_HEIGHT, 8);
				printf("============frmidx = %d=============\r\n",frmidx);
	#endif
				memset((void *)&lib_md_rst,0,sizeof(LIB_MD_MDT_RESULT_INFO));
				if ((ret = lib_md_get(0, LIB_MD_RESULT_INFO, &lib_md_rst)) != HD_OK) {
					printf("LIB_MD_RESULT_INFO fail\n");
				}

				if(lib_md_rst.global_motion_alarm == 1){
					printf("[WRN] global motion alarm\n");
				}
				for(reg_id =0; reg_id<mdt_lib_param.mdt_subregion_param.sub_region_num; reg_id++){
					if(lib_md_rst.sub_motion_alarm[reg_id] == 1){
						printf("[WRN] sub_region[%d] motion alarm\n", (int)reg_id);
					}
				}
				if (lib_md_rst.scene_change_alarm == 1) {
					printf("[WRN] scene change alarm\n");
				}				
				if(lib_md_rst.obj_num>0){
					UINT32 obj_counter;
					for(obj_counter = 0;obj_counter<lib_md_rst.obj_num;obj_counter++){
						printf("[OBJ %d]: x0=%d,x1=%d,y0=%d,y1=%d\r\n",obj_counter,
							lib_md_rst.obj[obj_counter].start_x,lib_md_rst.obj[obj_counter].end_x,
							lib_md_rst.obj[obj_counter].start_y,lib_md_rst.obj[obj_counter].end_y);
					}
				}
			}
            hd_common_mem_munmap((void *)imgY_va, IMG_BUF_SIZE+(IMG_BUF_SIZE/2));
			//ping_pong_id = (ping_pong_id+1)%2;
			if(is_Init==0)is_Init=1;
		}
		else if(counter <=30){
			counter++;
		}
		ret = hd_videoproc_release_out_buf(g_stream[0].proc_alg_path, &video_frame);
		if(ret != HD_OK) {
			printf("hd_videoproc_release_out_buf fail (%d)\n\r", ret);
			goto libmd_init_fail;
		}
	}

libmd_init_fail:
	ret = lib_md_uninit(0);
	if (HD_OK != ret) {
		printf("lib_md_uninit fail, error code = %d\r\n", ret);
	}
gen_init_fail:	
	ret = vendor_md_uninit();
	if (HD_OK != ret) {
		printf("uninit fail, error code = %d\r\n", ret);
	}
	return 0;
}


MAIN(argc, argv)
{
	HD_RESULT ret;
	INT key,idx=0;
	MEM_RANGE share_mem[SHARE_MEM_NUM];
	UINT32 out_type=1;
	pthread_t md_thread_id[MD_THREAD_NUM];
	CHAR *lib_md_version;

	g_endFlag = 0;
	md_get_para_sensitivity();
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
	//set project config for AI
	//hd_common_sysconfig(0, (1<<16), 0, VENDOR_CV_CFG); //enable CV engine

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
	for (idx = 0; idx < MD_THREAD_NUM; idx++) {
		// create thread
		ret = pthread_create(&md_thread_id[idx], NULL, md_thread_api, (VOID*)share_mem);
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

	lib_md_version = (CHAR *) lib_md_get_version();
	printf("MD: version(%s)\r\n", (char *) lib_md_version);

	// query user key
	printf("Enter q to exit\n");
	while (1) {
		key = NVT_EXAMSYS_GETCHAR();
		if (key == 'q' || key == 0x3) {
			g_endFlag = 1;
			// quit program
			break;
		}

		if (key == 'h') {
			g_sensi = HIGH_SENSI;
			printf("\r\n--- Switch to high sensitivity ---\r\n");
		}

		if (key == 'm') {
			g_sensi = MED_SENSI;
			printf("\r\n--- Switch to medium sensitivity ---\r\n");
		}

		if (key == 'l') {
			g_sensi = LOW_SENSI;
			printf("\r\n--- Switch to low sensitivity ---\r\n");
		}

		if (key == 'p') {
			g_sensi = DEFAULT_SENSI;
			printf("\r\n--- Switch to default sensitivity ---\r\n");
		}

		#if (DEBUG_MENU == 1)
		if (key == 'd') {
			// enter debug menu
			hd_debug_run_menu();
			printf("\r\nEnter q to exit, Enter d to debug\r\n");
		}
		#endif
		if (key == '0') {
			get_cap_sysinfo(g_stream[0].cap_ctrl);
		}
	}

	// wait encode thread destroyed
	for (idx = 0; idx < MD_THREAD_NUM; idx++) {
		pthread_join(md_thread_id[idx], NULL);
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
