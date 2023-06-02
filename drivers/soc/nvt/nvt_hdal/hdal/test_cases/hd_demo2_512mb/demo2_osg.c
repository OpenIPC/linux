/**
	@brief Demo code of 1 videocap path + 2 videoenc path + 1 videoout path.\n

	@file demo1_osg.c

	@author iVOT/PSW

	@ingroup mhdal

	@note This file is modified from these sample code:
	      video_record_with_osg.c
	      video_liveview_with_osg.c

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
#include "demo2_int.h"


#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)


///////////////////////////////////////////////////////////////////////////////

int osg_query_buf_size(UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt)
{
	HD_VIDEO_FRAME frame = {0};
	int stamp_size;

	frame.sign   = MAKEFOURCC('O','S','G','P');
	frame.dim.w  = w;
	frame.dim.h  = h;
	frame.pxlfmt = pxlfmt;

	//get required buffer size for a single image
	stamp_size = hd_common_mem_calc_buf_size(&frame);
	if(!stamp_size){
		printf("fail to query buffer size\n");
		return -1;
	}

	//ping pong buffer needs double size
	stamp_size *= 2;
	return stamp_size;
}

///////////////////////////////////////////////////////////////////////////////

static int osg_alloc_stamp_data(UINT32 id, UINT32 stamp_size, UINT32 *stamp_blk, UINT32 *stamp_pa)
{
	UINT32                  pa;
	HD_COMMON_MEM_VB_BLK    blk;

	if(!stamp_size){
		printf("stamp_size is unknown\n");
		return -1;
	}

	//get osd stamps' block
	blk = hd_common_mem_get_block(HD_COMMON_MEM_USER_DEFINIED_POOL+id, stamp_size, DDR_ID0);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		printf("stamp %d get block fail, blk = 0x%x\r\n", id, blk);
		return -1;
	}

	if(stamp_blk)
		*stamp_blk = blk;

	//translate stamp block to physical address
	pa = hd_common_mem_blk2pa(blk);
	if (pa == 0) {
		printf("blk2pa fail, blk = 0x%x\r\n", blk);
		return -1;
	}

	if(stamp_pa)
		*stamp_pa = pa;

	return 0;
}

static void osg_free_stamp_data(UINT32 id, UINT32 stamp_blk)
{
	HD_RESULT         ret;

	ret = hd_common_mem_release_block(stamp_blk);
	if (HD_OK != ret) {
		printf("release blk fail %d\r\n", ret);
	}
}

static int osg_fill_stamp_data(UINT32 img_w, UINT32 img_h, unsigned short *stamp_data, unsigned short c)
{
	register UINT32 x, y;
	for (y = 0; y < img_h; y++) {
		unsigned short *line = stamp_data + (y * img_w);
		for (x = 0; x < img_w; x++) {
			unsigned short *pixel = line + x;
			(*pixel) = c;
		}
	}
	return 0;
}

static int set_vprc_stamp_img(UINT32 id, HD_PATH_ID stamp_path, UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt, UINT32 stamp_pa, UINT32 stamp_size, unsigned short *stamp_data)
{
	HD_OSG_STAMP_BUF  buf;
	HD_OSG_STAMP_IMG  img;

	if(!stamp_pa){
		printf("stamp buffer %d is not allocated\n", id);
		return -1;
	}

	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = stamp_pa;
	buf.size      = stamp_size;

	if(hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_BUF, &buf) != HD_OK){
		printf("fail to set vprc stamp buffer %d\n", id);
		return -1;
	}

	memset(&img, 0, sizeof(HD_OSG_STAMP_IMG));

	img.fmt        = pxlfmt;
	img.dim.w      = w;
	img.dim.h      = h;
	img.p_addr     = (UINT32)stamp_data;

	if(hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_IMG, &img) != HD_OK){
		printf("fail to set vprc stamp image %d\n", id);
		return -1;
	}

	return 0;
}

static int set_vprc_stamp_attr(UINT32 id, HD_PATH_ID stamp_path, UINT32 x, UINT32 y)
{
	HD_OSG_STAMP_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_STAMP_ATTR));

	attr.position.x = x;
	attr.position.y = y;
	attr.alpha      = 255;
	attr.layer      = 0;
	attr.region     = 0;

	return hd_videoproc_set(stamp_path, HD_VIDEOPROC_PARAM_IN_STAMP_ATTR, &attr);
}

static int set_venc_stamp_img(UINT32 id, HD_PATH_ID stamp_path, UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt, UINT32 stamp_pa, UINT32 stamp_size, unsigned short *stamp_data)
{
	HD_OSG_STAMP_BUF  buf;
	HD_OSG_STAMP_IMG  img;

	if(!stamp_pa){
		printf("stamp buffer %d is not allocated\n", id);
		return -1;
	}

	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = stamp_pa;
	buf.size      = stamp_size;

	if(hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_BUF, &buf) != HD_OK){
		printf("fail to set venc stamp buffer%d\n", id);
		return -1;
	}

	memset(&img, 0, sizeof(HD_OSG_STAMP_IMG));

	img.fmt        = pxlfmt;
	img.dim.w      = w;
	img.dim.h      = h;
	img.p_addr     = (UINT32)stamp_data;

	if(hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_IMG, &img) != HD_OK){
		printf("fail to set venc stamp image %d\n", id);
		return -1;
	}

	return 0;
}

static int set_venc_stamp_attr(UINT32 id, HD_PATH_ID stamp_path, UINT32 x, UINT32 y)
{
	HD_OSG_STAMP_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_STAMP_ATTR));

	attr.position.x = x;
	attr.position.y = y;
	attr.alpha      = 255;
	attr.layer      = 0;
	attr.region     = 0;

	return hd_videoenc_set(stamp_path, HD_VIDEOENC_PARAM_IN_STAMP_ATTR, &attr);
}

static int set_vout_stamp_img(UINT32 id, HD_PATH_ID stamp_path, UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt, UINT32 stamp_pa, UINT32 stamp_size, unsigned short *stamp_data)
{
	HD_OSG_STAMP_BUF  buf;
	HD_OSG_STAMP_IMG  img;

	if(!stamp_pa){
		printf("stamp buffer %d is not allocated\n", id);
		return -1;
	}

	memset(&buf, 0, sizeof(HD_OSG_STAMP_BUF));

	buf.type      = HD_OSG_BUF_TYPE_PING_PONG;
	buf.p_addr    = stamp_pa;
	buf.size      = stamp_size;

	if(hd_videoout_set(stamp_path, HD_VIDEOOUT_PARAM_OUT_STAMP_BUF, &buf) != HD_OK){
		printf("fail to set vout stamp buffer %d\n", id);
		return -1;
	}

	memset(&img, 0, sizeof(HD_OSG_STAMP_IMG));

	img.fmt        = pxlfmt;
	img.dim.w      = w;
	img.dim.h      = h;
	img.p_addr     = (UINT32)stamp_data;

	if(hd_videoout_set(stamp_path, HD_VIDEOOUT_PARAM_OUT_STAMP_IMG, &img) != HD_OK){
		printf("fail to set vout stamp image %d\n", id);
		return -1;
	}

	return 0;
}

static int set_vout_stamp_attr(UINT32 id, HD_PATH_ID stamp_path, UINT32 x, UINT32 y)
{
	HD_OSG_STAMP_ATTR attr;

	memset(&attr, 0, sizeof(HD_OSG_STAMP_ATTR));

	attr.position.x = x;
	attr.position.y = y;
	attr.alpha      = 255;
	attr.layer      = 0;
	attr.region     = 0;

	return hd_videoout_set(stamp_path, HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR, &attr);
}

////////////////////////////

HD_RESULT osg_init(UINT32 id, DEMO_OSG* osg, UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt, unsigned short c)
{
	HD_RESULT ret;

	osg->stamp_dim.w = w;
	osg->stamp_dim.h = h;
	osg->stamp_fmt = pxlfmt;

	// allocate user stamp buffer
	osg->stamp_data = malloc(osg->stamp_dim.w * osg->stamp_dim.h * sizeof(unsigned short));
	if(!osg->stamp_data){
		printf("fail to allocate logo buffer\n");
		return HD_ERR_NOMEM;
	}

	// fill stamp pattern
	if(osg_fill_stamp_data(osg->stamp_dim.w, osg->stamp_dim.h, osg->stamp_data, c)){
		printf("fail to fill stamp image\n");
		free(osg->stamp_data);
		return HD_ERR_RESOURCE;
	}

	// get blk of stamp data
	osg->stamp_blk  = 0;
	osg->stamp_pa   = 0;
	osg->stamp_size = osg_query_buf_size(osg->stamp_dim.w, osg->stamp_dim.h, osg->stamp_fmt);
	if(osg->stamp_size <= 0){
		printf("osg_query_buf_size() fail\n");
		return HD_ERR_LIMIT;
	}

	ret = osg_alloc_stamp_data(id, osg->stamp_size, &(osg->stamp_blk), &(osg->stamp_pa));
	if(ret){
		printf("fail to allocate stamp buffer %d\n", id);
		return HD_ERR_NOMEM;
	}

	return HD_OK;
}

HD_RESULT osg_open(UINT32 id, DEMO_OSG* osg, UINT32 type, UINT32 path)
{
	HD_RESULT ret;
	if(type == OSG_TYPE_VIDEOPROC) {

		if((ret = hd_videoproc_open(path, HD_STAMP_0, &osg->vprc_stamp_path)) != HD_OK) {
			printf("fail to open vprc stamp\n");
			return ret;
		}

	} else if(type == OSG_TYPE_VIDEOENC) {

		if((ret = hd_videoenc_open(path, HD_STAMP_0, &osg->venc_stamp_path)) != HD_OK) {
			printf("fail to open venc stamp\n");
			return ret;
		}

	} else if(type == OSG_TYPE_VIDEOOUT) {

		if((ret = hd_videoout_open(path, HD_STAMP_0, &osg->vout_stamp_path)) != HD_OK) {
			printf("fail to open vout stamp\n");
			return ret;
		}
	}

	return HD_OK;
}

HD_RESULT osg_close(UINT32 id, DEMO_OSG* osg, UINT32 type)
{
	HD_RESULT ret;
	if(type == OSG_TYPE_VIDEOPROC) {

		if((ret = hd_videoproc_close(osg->vprc_stamp_path)) != HD_OK)
			return ret;

	} else if(type == OSG_TYPE_VIDEOENC) {

		if((ret = hd_videoenc_close(osg->venc_stamp_path)) != HD_OK)
			return ret;

	} else if(type == OSG_TYPE_VIDEOOUT) {

		if((ret = hd_videoout_close(osg->vout_stamp_path)) != HD_OK)
			return ret;

	}

	return HD_OK;
}

HD_RESULT osg_start(UINT32 id, DEMO_OSG* osg, UINT32 type, UINT32 x, UINT32 y)
{
	HD_RESULT ret;

	if(type == OSG_TYPE_VIDEOPROC) {

		// setup vprc stamp image
		if(set_vprc_stamp_img(id, osg->vprc_stamp_path, osg->stamp_dim.w,  osg->stamp_dim.h, osg->stamp_fmt, osg->stamp_pa, osg->stamp_size, osg->stamp_data)){
			printf("fail to set vprc stamp img\r\n");
			return HD_ERR_ABORT;
		}

		// setup vprc stamp x,y
		if(set_vprc_stamp_attr(id, osg->vprc_stamp_path, x, y)){
			printf("fail to set vprc stamp attr\r\n");
			return HD_ERR_ABORT;
		}

		// render vprc stamp
		ret = hd_videoproc_start(osg->vprc_stamp_path);
		if (ret != HD_OK) {
			printf("start vprc stamp fail=%d\r\n", ret);
			return HD_ERR_ABORT;
		}

	} else if(type == OSG_TYPE_VIDEOENC) {

		// setup venc stamp image
		if(set_venc_stamp_img(id, osg->venc_stamp_path, osg->stamp_dim.w,  osg->stamp_dim.h, osg->stamp_fmt, osg->stamp_pa, osg->stamp_size, osg->stamp_data)){
			printf("fail to set venc stamp img\r\n");
			return HD_ERR_ABORT;
		}

		// setup venc stamp x,y
		if(set_venc_stamp_attr(id, osg->venc_stamp_path, x, y)){
			printf("fail to set venc stamp attr\r\n");
			return HD_ERR_ABORT;
		}

		// render venc stamp
		ret = hd_videoenc_start(osg->venc_stamp_path);
		if (ret != HD_OK) {
			printf("start venc stamp fail=%d\r\n", ret);
			return HD_ERR_ABORT;
		}

	} else if(type == OSG_TYPE_VIDEOOUT) {

		// setup vout stamp image
		if(set_vout_stamp_img(id, osg->vout_stamp_path, osg->stamp_dim.w,  osg->stamp_dim.h, osg->stamp_fmt, osg->stamp_pa, osg->stamp_size, osg->stamp_data)){
			printf("fail to set vout stamp img\r\n");
			return HD_ERR_ABORT;
		}

		// setup vout stamp x,y
		if(set_vout_stamp_attr(id, osg->vout_stamp_path, x, y)){
			printf("fail to set vout stamp attr\r\n");
			return HD_ERR_ABORT;
		}

		// render vout stamp
		ret = hd_videoout_start(osg->vout_stamp_path);
		if (ret != HD_OK) {
			printf("start vout stamp fail=%d\r\n", ret);
			return HD_ERR_ABORT;
		}
	}
	return HD_OK;
}

HD_RESULT osg_uninit(UINT32 id, DEMO_OSG* osg)
{
	// release blk of stamp data
	osg_free_stamp_data(id, osg->stamp_blk);

	// free user stamp buffer
	free(osg->stamp_data);

	return HD_OK;
}

