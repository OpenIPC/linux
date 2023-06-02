/**
	@brief Sample code of gfx.\n

	@file gfx_only.c

	@author YongChang Qui

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "hdal.h"
#include "hd_debug.h"

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
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(hd_gfx_only, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

///////////////////////////////////////////////////////////////////////////////

//setup all gfx's buffer to demo purpose
static HD_RESULT mem_init(HD_COMMON_MEM_VB_BLK *buf_blk, UINT32 buf_size, UINT32 *buf_pa)
{
	HD_RESULT                    ret;
	HD_COMMON_MEM_INIT_CONFIG    mem_cfg = {0};

	mem_cfg.pool_info[0].type     = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = buf_size;
	mem_cfg.pool_info[0].blk_cnt  = 1;
	mem_cfg.pool_info[0].ddr_id   = DDR_ID0;

	//register gfx's buffer to common memory pool
	ret = hd_common_mem_init(&mem_cfg);
	if(ret != HD_OK){
		printf("fail to allocate %d bytes from common pool\n", buf_size);
		return ret;
	}

	//get gfx's buffer block
	*buf_blk = hd_common_mem_get_block(HD_COMMON_MEM_COMMON_POOL, buf_size, DDR_ID0);
	if (*buf_blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("get block fail\r\n");
		return HD_ERR_NOMEM;
	}

	//translate gfx's buffer block to physical address
	*buf_pa = hd_common_mem_blk2pa(*buf_blk);
	if (*buf_pa == 0) {
		printf("blk2pa fail, buf_blk = 0x%x\r\n", *buf_blk);
		return HD_ERR_NOMEM;
	}

	return HD_OK;
}

static HD_RESULT mem_exit(void)
{
	HD_RESULT ret = HD_OK;
	hd_common_mem_uninit();
	return ret;
}

///////////////////////////////////////////////////////////////////////////////

//copy an argb4444 logo to a yuv420 background image
static HD_RESULT test_copy(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_COPY         param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                 len;

	//calculate logo's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = 200;
	frame.loff[0] = 1000;
	frame.loff[1] = 1000;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load logo from sd card
	fd = open("/mnt/sd/video_frm_1000_200_1_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		printf("hd_video_record_osg.bin is in SDK/code/hdal/samples/pattern/\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to copy the logo to the background image
	memset(&param, 0, sizeof(HD_GFX_COPY));
	param.src_img.dim.w            = 1000;
	param.src_img.dim.h            = 200;
	param.src_img.format           = HD_VIDEO_PXLFMT_ARGB4444;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.lineoffset[0]    = 1000 * 2;
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = 1000;
	param.src_region.h             = 200;
	param.dst_pos.x                = 300;
	param.dst_pos.y                = 500;
	param.colorkey                 = 0;
	param.alpha                    = 128;

	ret = hd_gfx_copy(&param);
	if(ret != HD_OK){
		printf("hd_gfx_copy fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_copy_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_copy_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//scale a 1920x1080 yuv420 image up to 3840x2160
static HD_RESULT test_scale(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_SCALE        param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                 len;

	//calculate 1920x1080 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	//calculate 3840x2160 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 2160;
	frame.loff[0] = 3840;
	frame.loff[1] = 3840;
	dst_size = hd_common_mem_calc_buf_size(&frame);
	if(!dst_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load 1920x1080 yuv420 image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)src_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to scale up 1920x1080 image
	memset(&param, 0, sizeof(HD_GFX_SCALE));
	param.src_img.dim.w            = 1920;
	param.src_img.dim.h            = 1080;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + 1920 * 1080;
	param.src_img.lineoffset[0]    = 1920;
	param.src_img.lineoffset[1]    = 1920;
	param.dst_img.dim.w            = 3840;
	param.dst_img.dim.h            = 2160;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 3840 * 2160;
	param.dst_img.lineoffset[0]    = 3840;
	param.dst_img.lineoffset[1]    = 3840;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = 1920;
	param.src_region.h             = 1080;
	param.dst_region.x             = 0;
	param.dst_region.y             = 0;
	param.dst_region.w             = 3840;
	param.dst_region.h             = 2160;

	ret = hd_gfx_scale(&param);
	if(ret != HD_OK){
		printf("hd_gfx_scale fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_scale_3840_2160_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_scale_3840_2160_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/hd_gfx_scale_3840_2160_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_scale_3840_2160_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//create mirror for a yuv420 image
static HD_RESULT test_rotation(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_ROTATE       param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              size, src_pa, src_va, dst_pa, dst_va;
	int                 len;

	//calculate yuv420 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if((size * 2) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size * 2, buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + size;


	//load yuv420 image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to mirror the image
	memset(&param, 0, sizeof(HD_GFX_ROTATE));
	param.src_img.dim.w            = 1920;
	param.src_img.dim.h            = 1080;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + 1920 * 1080;
	param.src_img.lineoffset[0]    = 1920;
	param.src_img.lineoffset[1]    = 1920;
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 1080 * 1920;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = 1920;
	param.src_region.h             = 1080;
	param.dst_pos.x                = 0;
	param.dst_pos.y                = 0;
	param.angle                    = HD_VIDEO_DIR_MIRRORY;

	ret = hd_gfx_rotate(&param);
	if(ret != HD_OK){
		printf("hd_gfx_rotate fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_rotate_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//transform an argb4444 image to a yuv420 image
static HD_RESULT test_color_transform(UINT32 buf_pa, UINT32 buf_size)
{
	int                       fd, i;
	HD_GFX_COLOR_TRANSFORM    param;
	void                      *va;
	HD_RESULT                 ret;
	HD_VIDEO_FRAME            frame;
	UINT32                    rgb_size, yuv_size;
	int                       len;
	int                       logo_w = 1000, logo_h = 200;
	UINT32                    rgb_pa, rgb_va, yuv_pa, yuv_va;

	//calculate yuv420 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = logo_h;
	frame.loff[0] = logo_w;
	frame.loff[1] = logo_w;
	yuv_size = hd_common_mem_calc_buf_size(&frame);
	if(!yuv_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate yuv size\n");
		return -1;
	}

	//calculate argb4444 image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_ARGB4444;
	frame.dim.h   = logo_h;
	frame.loff[0] = logo_w;
	rgb_size = hd_common_mem_calc_buf_size(&frame);
	if(!rgb_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate yuv size\n");
		return -1;
	}

	if((yuv_size + rgb_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (yuv_size + rgb_size), buf_size);
		return -1;
	}

	rgb_pa = buf_pa;
	yuv_pa = rgb_pa + rgb_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	rgb_va = (UINT32)va;
	yuv_va = rgb_va + rgb_size;

	fd = open("/mnt/sd/video_frm_1000_200_1_argb4444.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		printf("hd_video_record_osg.bin is in SDK/code/hdal/samples/pattern/\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//load argb4444 image from sd card
	len = read(fd, (void*)rgb_va, rgb_size);
	close(fd);
	if(len != (int)rgb_size){
		printf("fail to read /mnt/sd/video_frm_1000_200_1_argb4444.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	for(i = 0 ; i < logo_w*logo_h ; ++i)
		if(!((unsigned short*)rgb_va)[i])
			((unsigned short*)rgb_va)[i] = 0xFFFF;

	//use gfx engine to transform argb4444 image to yuv420
	memset(&param, 0, sizeof(HD_GFX_COLOR_TRANSFORM));
	param.src_img.dim.w            = logo_w;
	param.src_img.dim.h            = logo_h;
	param.src_img.format           = HD_VIDEO_PXLFMT_ARGB4444;
	param.src_img.p_phy_addr[0]    = rgb_pa;
	param.src_img.lineoffset[0]    = logo_w * 2;
	param.dst_img.dim.w            = logo_w;
	param.dst_img.dim.h            = logo_h;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = yuv_pa;
	param.dst_img.p_phy_addr[1]    = yuv_pa + logo_w * logo_h;
	param.dst_img.lineoffset[0]    = logo_w;
	param.dst_img.lineoffset[1]    = logo_w;

	ret = hd_gfx_color_transform(&param);
	if(ret != HD_OK){
		printf("hd_gfx_color_transform fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)yuv_va, yuv_size);
	close(fd);
	if(len != (int)yuv_size){
		printf("fail to write /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_color_transform_1000_200_1.yuv420\n");

	ret = HD_OK;

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//transform a 1920x1080 yuv420 3 planar image to a yuv420 2 planar image
static HD_RESULT test_4203P_to_4202P(UINT32 buf_pa, UINT32 buf_size)
{
	int                       fd;
	HD_GFX_COLOR_TRANSFORM    param;
	void                      *va;
	HD_RESULT                 ret;
	HD_VIDEO_FRAME            frame;
	UINT32                    file_size, packed_size;
	int                       len;
	int                       w = 1920, h = 1080;
	UINT32                    file_pa, file_va, packed_pa, packed_va;

	//calculate planar image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	frame.dim.h   = h;
	frame.loff[0] = w;
	frame.loff[1] = w;
	frame.loff[2] = w;
	file_size = hd_common_mem_calc_buf_size(&frame);
	if(!file_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate file_size size\n");
		return -1;
	}

	//calculate packed image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = h;
	frame.loff[0] = w;
	frame.loff[1] = w;
	packed_size = hd_common_mem_calc_buf_size(&frame);
	if(!packed_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate packed size\n");
		return -1;
	}

	if((file_size + packed_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (file_size + packed_size), buf_size);
		return -1;
	}

	file_pa = buf_pa;
	packed_pa = file_pa + file_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	file_va = (UINT32)va;
	packed_va = file_va + file_size;

	//load test image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv4203P.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)file_va, file_size);
	close(fd);
	if(len != (int)file_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to transform 420 planar image to 420 packed
	memset(&param, 0, sizeof(HD_GFX_COLOR_TRANSFORM));
	param.src_img.dim.w            = w;
	param.src_img.dim.h            = h;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	param.src_img.p_phy_addr[0]    = file_pa;
	param.src_img.p_phy_addr[1]    = file_pa + w * h;
	param.src_img.p_phy_addr[2]    = file_pa + w * h * 3 / 2;
	param.src_img.lineoffset[0]    = w;
	param.src_img.lineoffset[1]    = w;
	param.src_img.lineoffset[2]    = w;
	param.dst_img.dim.w            = w;
	param.dst_img.dim.h            = h;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = packed_pa;
	param.dst_img.p_phy_addr[1]    = packed_pa + w * h;
	param.dst_img.lineoffset[0]    = w;
	param.dst_img.lineoffset[1]    = w;

	ret = hd_gfx_color_transform(&param);
	if(ret != HD_OK){
		printf("hd_gfx_color_transform fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)packed_va, packed_size);
	close(fd);
	if(len != (int)packed_size){
		printf("fail to write /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_3p_to_2p_1920_1080_1_yuv420.dat\n");

	ret = HD_OK;

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_4203P_scale(UINT32 buf_pa, UINT32 buf_size)
{
	int                       fd;
	HD_GFX_SCALE              param;
	void                      *va;
	HD_RESULT                 ret;
	HD_VIDEO_FRAME            frame;
	UINT32                    file_size, scale_size;
	int                       len;
	int                       sw = 1920, sh = 1080, dw = 480, dh = 270;
	UINT32                    file_pa, file_va, scale_pa, scale_va;

	//calculate file's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	frame.dim.h   = sh;
	frame.loff[0] = sw;
	frame.loff[1] = sw;
	frame.loff[2] = sw;
	file_size = hd_common_mem_calc_buf_size(&frame);
	if(!file_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate file_size size\n");
		return -1;
	}

	//calculate planar image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	frame.dim.h   = dh;
	frame.loff[0] = dw;
	frame.loff[1] = dw;
	frame.loff[2] = dw;
	scale_size = hd_common_mem_calc_buf_size(&frame);
	if(!scale_size){
		printf("hd_common_mem_calc_buf_size() fail to calculate planar_size size\n");
		return -1;
	}

	if((file_size + scale_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (file_size + scale_size), buf_size);
		return -1;
	}

	file_pa = buf_pa;
	scale_pa = file_pa + file_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	file_va = (UINT32)va;
	scale_va = file_va + file_size;

	//load test image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv4203P.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)file_va, file_size);
	close(fd);
	if(len != (int)file_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv4203P.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	
	//use gfx engine to scale 420 planar image
	memset(&param, 0, sizeof(HD_GFX_SCALE));
	param.src_img.dim.w            = sw;
	param.src_img.dim.h            = sh;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	param.src_img.p_phy_addr[0]    = file_pa;
	param.src_img.p_phy_addr[1]    = file_pa + sw * sh;
	param.src_img.p_phy_addr[2]    = file_pa + sw * sh * 3 / 2;
	param.src_img.lineoffset[0]    = sw;
	param.src_img.lineoffset[1]    = sw;
	param.src_img.lineoffset[2]    = sw;
	param.dst_img.dim.w            = dw;
	param.dst_img.dim.h            = dh;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420_PLANAR;
	param.dst_img.p_phy_addr[0]    = scale_pa;
	param.dst_img.p_phy_addr[1]    = scale_pa + dw * dh;
	param.dst_img.p_phy_addr[2]    = scale_pa + dw * dh * 3 / 2;
	param.dst_img.lineoffset[0]    = dw;
	param.dst_img.lineoffset[1]    = dw;
	param.dst_img.lineoffset[2]    = dw;
	param.src_region.x             = 0;
	param.src_region.y             = 0;
	param.src_region.w             = sw;
	param.src_region.h             = sh;
	param.dst_region.x             = 0;
	param.dst_region.y             = 0;
	param.dst_region.w             = dw;
	param.dst_region.h             = dh;

	ret = hd_gfx_scale(&param);
	if(ret != HD_OK){
		printf("hd_gfx_scale fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_scale_420_3p_480_270_1_yuv4203p.dat", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_scale_420_3p_480_270_1_yuv4203p.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)scale_va, scale_size);
	close(fd);
	if(len != (int)scale_size){
		printf("fail to write /mnt/sd/hd_gfx_scale_420_3p_480_270_1_yuv4203p.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_scale_420_3p_480_270_1_yuv4203p.dat\n");

	ret = HD_OK;

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//draw a line on an image
static HD_RESULT test_draw_line(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_DRAW_LINE    param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              size;
	int                 len;

	//calculate image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if(size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size, buf_size);
		return -1;
	}

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//coverity[string_null_argument]
	len = read(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to draw a line on the image
	memset(&param, 0, sizeof(HD_GFX_DRAW_LINE));
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = buf_pa;
	param.dst_img.p_phy_addr[1]    = buf_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.color                    = 0x0FF00;
	param.start.x                  = 500;
	param.start.y                  = 250;
	param.end.x                    = 500;
	param.end.y                    = 750;
	param.thickness                = 30;

	ret = hd_gfx_draw_line(&param);
	if(ret != HD_OK){
		printf("hd_gfx_draw_line fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_draw_line_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_draw_line_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_draw_line_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_draw_line_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//draw a line on an image
static HD_RESULT test_draw_rectangle(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_DRAW_RECT    param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              size;
	int                 len;

	//calculate image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if(size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size, buf_size);
		return -1;
	}

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//coverity[string_null_argument]
	len = read(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	//use gfx engine to draw a rectangle on the image
	memset(&param, 0, sizeof(HD_GFX_DRAW_RECT));
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1080;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = buf_pa;
	param.dst_img.p_phy_addr[1]    = buf_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.color                    = 0x0FF00;
	param.rect.x                   = 1000;
	param.rect.y                   = 500;
	param.rect.w                   = 500;
	param.rect.h                   = 250;
	param.type                     = HD_GFX_RECT_SOLID;

	ret = hd_gfx_draw_rect(&param);
	if(ret != HD_OK){
		printf("hd_gfx_draw_rect fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_draw_rect_1920_1080_1.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

//use hardware copy engine to copy 1M bytes
static HD_RESULT test_dma_copy(UINT32 buf_pa, UINT32 buf_size)
{
	void                *va;
	UINT32              src_pa, src_va, dst_pa, dst_va;
	int                 i;
	UINT32              size = 1024*1024;
	HD_RESULT           ret = HD_ERR_NOT_SUPPORT;
	
	if((size * 2) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 2), buf_size);
		return -1;
	}
	
	src_pa = buf_pa;
	dst_pa = src_pa + size;
	
	//map 1MB buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}
	
	src_va = (UINT32)va;
	dst_va = src_va + size;
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i)
		((unsigned char*)src_va)[i] = (i%8);
	memset((void*)dst_va, 0, size);
	
	//use gfx engine to copy 1MB buffer
	if(hd_gfx_memcpy(dst_pa, src_pa, size) == NULL){
		printf("hd_gfx_memcpy fail\n");
		goto exit;
	}
	
	//check data integrity
	if(memcmp((void*)src_va, (void*)dst_va, size))
		printf("dma fails to copy %d bytes\n", size);
	else{
		printf("dma manages to copy %d bytes\n", size);
		ret = HD_OK;
	}

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

//use hardware engine to minus two 16bits arrays of 1024 elements
static HD_RESULT test_arithmetic(UINT32 buf_pa, UINT32 buf_size)
{
	void                *va;
	UINT32              op1_pa, op1_va, op2_pa, op2_va, out_pa, out_va;
	int                 i;
	UINT32              size = 1024;
	HD_RESULT           ret = HD_ERR_NOT_SUPPORT;
	HD_GFX_ARITHMETIC   param;
	
	if((size * 3 * sizeof(short)) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (size * 3 * sizeof(short)), buf_size);
		return -1;
	}
	
	op1_pa = buf_pa;
	op2_pa = op1_pa + (size * sizeof(short));
	out_pa = op2_pa + (size * sizeof(short));
	
	//map physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	op1_va = (UINT32)va;
	op2_va = op1_va + (size * sizeof(short));
	out_va = op2_va + (size * sizeof(short));
	
	//generate random data
	for(i = 0 ; i < (int)size ; ++i){
		((short*)op1_va)[i] = 6000 + (i & 32767);
		((short*)op2_va)[i] = (i & 32767);
	}
	memset((void*)out_va, 0, size * sizeof(short));

	//use gfx engine to perform array minus 
	memset(&param, 0, sizeof(HD_GFX_ARITHMETIC));
	param.p_op1        = op1_pa;
	param.p_op2        = op2_pa;
	param.p_out        = out_pa;
	param.size         = size;
	param.operation    = HD_GFX_ARITH_OP_MINUS;
	param.bits         = 16;
	
	if(hd_gfx_arithmetic(&param) != HD_OK){
		printf("hd_gfx_arithmetic fail\n");
		goto exit;
	}
	
	//check data integrity
	for(i = 0 ; i < (int)size ; ++i)
		if(((short*)out_va)[i] != 6000){
			printf("out[%d] is %d, not 6000. op1 is %d, op2 is %d\n", i, 
				((short*)out_va)[i], ((short*)op1_va)[i], ((short*)op2_va)[i]);
			goto exit;
		}
		
	printf("minus operation of two 16bits arrays ok\n");
	ret = HD_OK;

exit:
		
	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);
	
	return ret;
}

//draw 4 lines to form a hollow frame and 4 rectangles on an image
static HD_RESULT test_job_list(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_DRAW_LINE    line[4];
	HD_GFX_DRAW_RECT    rect[4];
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              size;
	int                 i, len;
	HD_GFX_HANDLE       handle;

	//calculate image's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	size = hd_common_mem_calc_buf_size(&frame);
	if(!size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	if(size > buf_size){
		printf("required size(%d) > allocated size(%d)\n", size, buf_size);
		return -1;
	}

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	//load image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	//coverity[string_null_argument]
	len = read(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	memset(line, 0, sizeof(line));
	memset(rect, 0, sizeof(rect));
	
	//get a job/list handle
	ret = hd_gfx_begin_job(&handle);
	if(ret){
		printf("hd_gfx_begin_job fail\n");
		goto exit;
	}

	//configure image information for all lines and rectangles
	for(i = 0 ; i < 4 ; ++i){
		line[i].dst_img.dim.w            = 1920;
		line[i].dst_img.dim.h            = 1080;
		line[i].dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
		line[i].dst_img.p_phy_addr[0]    = buf_pa;
		line[i].dst_img.p_phy_addr[1]    = buf_pa + 1920 * 1080;
		line[i].dst_img.lineoffset[0]    = 1920;
		line[i].dst_img.lineoffset[1]    = 1920;
		rect[i].dst_img.dim.w            = 1920;
		rect[i].dst_img.dim.h            = 1080;
		rect[i].dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
		rect[i].dst_img.p_phy_addr[0]    = buf_pa;
		rect[i].dst_img.p_phy_addr[1]    = buf_pa + 1920 * 1080;
		rect[i].dst_img.lineoffset[0]    = 1920;
		rect[i].dst_img.lineoffset[1]    = 1920;
	}
	
	//prepare 1st line
	line[0].color     = 0x0FF00;
	line[0].start.x   = 250;
	line[0].start.y   = 250;
	line[0].end.x     = 750;
	line[0].end.y     = 250;
	line[0].thickness = 30;
	//prepare 2nd line
	line[1].color     = 0x0FF;
	line[1].start.x   = 750;
	line[1].start.y   = 250;
	line[1].end.x     = 750;
	line[1].end.y     = 750;
	line[1].thickness = 30;
	//prepare 3rd line
	line[2].color     = 0x0FF0000;
	line[2].start.x   = 250;
	line[2].start.y   = 750;
	line[2].end.x     = 750;
	line[2].end.y     = 750;
	line[2].thickness = 30;
	//prepare 4th line
	line[3].color     = 0x0FFFFFF;
	line[3].start.x   = 250;
	line[3].start.y   = 250;
	line[3].end.x     = 250;
	line[3].end.y     = 750;
	line[3].thickness = 30;

	//add 4 lines to the list
	ret = hd_gfx_add_draw_line_list(handle, line, 4);
	if(ret != HD_OK){
		printf("hd_gfx_add_draw_line_list fail=%d\n", ret);
		hd_gfx_cancel_job(handle);
		goto exit;
	}
	
	//prepare 1st rectangle
	rect[0].color  = 0x0FF00;
	rect[0].rect.x = 1000;
	rect[0].rect.y = 0;
	rect[0].rect.w = 500;
	rect[0].rect.h = 200;
	rect[0].type   = HD_GFX_RECT_SOLID;
	//prepare 2nd rectangle
	rect[1].color  = 0x0FF0000;
	rect[1].rect.x = 1000;
	rect[1].rect.y = 250;
	rect[1].rect.w = 500;
	rect[1].rect.h = 200;
	rect[1].type   = HD_GFX_RECT_SOLID;
	//prepare 3rd rectangle
	rect[2].color  = 0x0FF;
	rect[2].rect.x = 1000;
	rect[2].rect.y = 500;
	rect[2].rect.w = 500;
	rect[2].rect.h = 200;
	rect[2].type   = HD_GFX_RECT_SOLID;
	//prepare 4th rectangle
	rect[3].color  = 0x0FFFFFF;
	rect[3].rect.x = 1000;
	rect[3].rect.y = 750;
	rect[3].rect.w = 500;
	rect[3].rect.h = 200;
	rect[3].type   = HD_GFX_RECT_SOLID;
	
	//add 4 rectangles to the list
	ret = hd_gfx_add_draw_rect_list(handle, rect, 4);
	if(ret != HD_OK){
		printf("hd_gfx_add_draw_rect_list fail=%d\n", ret);
		hd_gfx_cancel_job(handle);
		goto exit;
	}
	
	//use gfx engine to draw 4 lines and 4 rectangles
	ret = hd_gfx_end_job(handle);
	if(ret != HD_OK){
		printf("hd_gfx_end_job fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/hd_gfx_job_list_1920_1080_1_yuv420.dat", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/hd_gfx_job_list_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, va, size);
	close(fd);
	if(len != (int)size){
		printf("fail to write /mnt/sd/hd_gfx_job_list_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/hd_gfx_job_list_1920_1080_1_yuv420.dat\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

static HD_RESULT test_yuv420_affine(UINT32 buf_pa, UINT32 buf_size)
{
	int                 fd;
	HD_GFX_AFFINE       param;
	void                *va;
	HD_RESULT           ret;
	HD_VIDEO_FRAME      frame;
	UINT32              src_size, src_pa, src_va, dst_size, dst_pa, dst_va;
	int                 len;
#define AFFINE_PI (3.14159265359)
	float               fAngel   = +15.0;//or -15.0
	float               fXoffset = -300;
	float               fYoffset = -300;

	//calculate background's buffer size
	frame.sign = MAKEFOURCC('V','F','R','M');
	frame.pxlfmt  = HD_VIDEO_PXLFMT_YUV420;
	frame.dim.h   = 1080;
	frame.loff[0] = 1920;
	frame.loff[1] = 1920;
	src_size = hd_common_mem_calc_buf_size(&frame);
	if(!src_size){
		printf("hd_common_mem_calc_buf_size() fail\n");
		return -1;
	}
	dst_size = src_size;

	if((src_size + dst_size) > buf_size){
		printf("required size(%d) > allocated size(%d)\n", (src_size + dst_size), buf_size);
		return -1;
	}

	src_pa = buf_pa;
	dst_pa = src_pa + src_size;

	//map gfx's buffer from physical address to user space
	va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, buf_pa, buf_size);
	if(!va){
		printf("hd_common_mem_mmap() fail\n");
		return -1;
	}

	src_va = (UINT32)va;
	dst_va = src_va + src_size;

	//load background image from sd card
	fd = open("/mnt/sd/video_frm_1920_1080_1_yuv420.dat", O_RDONLY);
	if(fd == -1){
		printf("fail to open /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		printf("video_frm_1920_1080_1_yuv420.dat is in SDK/code/hdal/samples/pattern/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_NOT_FOUND;
		goto exit;
	}

	len = read(fd, (void*)src_va, src_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to read /mnt/sd/video_frm_1920_1080_1_yuv420.dat\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
		
	//zero out all buffer for non-8-aligned area and not-rotate area
	memset((void*)dst_va, 0, dst_size);

	//rotate image with specified origin by affine engine
	memset(&param, 0, sizeof(HD_GFX_AFFINE));
	param.src_img.dim.w            = 1920;
	param.src_img.dim.h            = 1072;
	param.src_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.src_img.p_phy_addr[0]    = src_pa;
	param.src_img.p_phy_addr[1]    = src_pa + 1920 * 1080;
	param.src_img.lineoffset[0]    = 1920;
	param.src_img.lineoffset[1]    = 1920;
	param.dst_img.dim.w            = 1920;
	param.dst_img.dim.h            = 1072;
	param.dst_img.format           = HD_VIDEO_PXLFMT_YUV420;
	param.dst_img.p_phy_addr[0]    = dst_pa;
	param.dst_img.p_phy_addr[1]    = dst_pa + 1920 * 1080;
	param.dst_img.lineoffset[0]    = 1920;
	param.dst_img.lineoffset[1]    = 1920;
	param.coeff_a                  = cos(fAngel * AFFINE_PI / 180.0);
	param.coeff_b                  = -sin(fAngel * AFFINE_PI / 180.0);
	param.coeff_c                  = fXoffset;
	param.coeff_d                  = sin(fAngel * AFFINE_PI / 180.0);
	param.coeff_e                  = cos(fAngel * AFFINE_PI / 180.0);
	param.coeff_f                  = fYoffset;

	ret = hd_gfx_affine(&param);
	if(ret != HD_OK){
		printf("hd_gfx_affine fail=%d\n", ret);
		goto exit;
	}

	//save the result image to sd card
	fd = open("/mnt/sd/affine_1920_1080.yuv420", O_WRONLY | O_CREAT, 0644);
	if(fd == -1){
		printf("fail to open /mnt/sd/affine_1920_1080.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}

	len = write(fd, (void*)dst_va, dst_size);
	close(fd);
	if(len != (int)dst_size){
		printf("fail to write /mnt/sd/affine_1920_1080.yuv420\n");
		ret = HD_ERR_SYS;
		goto exit;
	}
	printf("result is /mnt/sd/affine_1920_1080.yuv420\n");

exit:

	if(hd_common_mem_munmap(va, buf_size))
		printf("fail to unmap va(%x)\n", (int)va);

	return ret;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct _GFX {

	// (1)
	HD_COMMON_MEM_VB_BLK buf_blk;
	UINT32               buf_size;
	UINT32               buf_pa;
	
} GFX;

MAIN(argc, argv)
{
    HD_RESULT    ret;
	GFX          gfx;
    INT          key;

	gfx.buf_blk = 0;
	gfx.buf_size = 16 * 1024 * 1024;
	gfx.buf_pa = 0;

	ret = hd_common_init(0);
    if(ret != HD_OK) {
        printf("common fail=%d\n", ret);
        goto exit;
    }

	ret = mem_init(&gfx.buf_blk, gfx.buf_size, &gfx.buf_pa);
    if(ret != HD_OK) {
        printf("mem fail=%d\n", ret);
        goto exit;
    }

    ret = hd_gfx_init();
    if(ret != HD_OK) {
        printf("init fail=%d\n", ret);
        goto exit;
    }

	while (1) {
		printf("Enter c to test copy\n");
		printf("Enter s to test scale\n");
		printf("Enter o to test rotation\n");
		printf("Enter t to test color transform\n");
		printf("Enter l to test draw line\n");
		printf("Enter r to test draw rectangle\n");
		printf("Enter m to test dma memory copy\n");
		printf("Enter a to test arithmetic\n");
		printf("Enter j to test job/list\n");
		printf("Enter p to transform yuv420 3p to 2p\n");
		printf("Enter d to scale down yuv420 3p image\n");
		printf("Enter f to affine a yuv420 image\n");
		key = GETCHAR();
		if (key == 'c') {
			ret = test_copy(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("copy fail=%d\n", ret);
			break;
		}
		else if (key == 's') {
			ret = test_scale(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("scale fail=%d\n", ret);
			break;
		}
		else if (key == 'o') {
			ret = test_rotation(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("rotation fail=%d\n", ret);
			break;
		}
		else if (key == 't') {
			ret = test_color_transform(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("color transform fail=%d\n", ret);
			break;
		}
		else if (key == 'l') {
			ret = test_draw_line(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("drawing line fail=%d\n", ret);
			break;
		}
		else if (key == 'r') {
			ret = test_draw_rectangle(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("drawing rectangle fail=%d\n", ret);
			break;
		}
		else if (key == 'm') {
			ret = test_dma_copy(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("dma copy fail=%d\n", ret);
			break;
		}
		else if (key == 'a') {
			ret = test_arithmetic(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("arithmetic fail=%d\n", ret);
			break;
		}
		else if (key == 'j') {
			ret = test_job_list(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("job/list fail=%d\n", ret);
			break;
		}
		else if (key == 'p') {
			ret = test_4203P_to_4202P(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("420 3p to 2p fail=%d\n", ret);
			break;
		}
		else if (key == 'd') {
			ret = test_4203P_scale(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("420 3p scale fail=%d\n", ret);
			break;
		}
		else if (key == 'f') {
			ret = test_yuv420_affine(gfx.buf_pa, gfx.buf_size);
			if(ret != HD_OK)
				printf("affine fail=%d\n", ret);
			break;
		}
	}

exit:

	if(gfx.buf_blk)
		if(HD_OK != hd_common_mem_release_block(gfx.buf_blk))
			printf("hd_common_mem_release_block() fail\n");

	ret = hd_gfx_uninit();
	if(ret != HD_OK)
		printf("uninit fail=%d\n", ret);

	ret = mem_exit();
	if(ret != HD_OK)
		printf("mem fail=%d\n", ret);

	ret = hd_common_uninit();
	if(ret != HD_OK)
		printf("common fail=%d\n", ret);

	return 0;
}
