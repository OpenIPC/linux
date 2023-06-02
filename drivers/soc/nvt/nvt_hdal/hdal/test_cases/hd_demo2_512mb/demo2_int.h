/**
	@brief Sample code of demo 2 enc + 1 out.\n

	@file demo1.c

	@author Jeah Yen

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
#include <pthread.h>
#include "hdal.h"
#include "hd_common.h"
#include "hd_debug.h"

////////////////////////////

#define OSG_TYPE_VIDEOPROC		0
#define OSG_TYPE_VIDEOENC		1
#define OSG_TYPE_VIDEOOUT		2

typedef struct _DEMO_OSG {
	// osg
	HD_PATH_ID vprc_stamp_path;
	HD_PATH_ID venc_stamp_path;
	HD_PATH_ID vout_stamp_path;

	HD_DIM stamp_dim;
	UINT32 stamp_fmt;
	UINT32 stamp_blk;
	UINT32 stamp_pa;
	UINT32 stamp_size;
	unsigned short *stamp_data;

} DEMO_OSG;

typedef struct _DEMO {

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

	// (2.out) user pull + push
	pthread_t  relay_thread_id;
	UINT32     proc_exit;

	HD_DIM  enc_max_dim;
	HD_DIM  enc_dim;

	// (3)
	HD_VIDEOENC_SYSCAPS enc_syscaps;
	HD_PATH_ID enc_path;

	// (3.out) user pull
	pthread_t  enc_thread_id;
	UINT32     enc_exit;
	UINT32     flow_start;

	HD_DIM  out_max_dim;
	HD_DIM  out_dim;

	// (3)
	HD_VIDEOOUT_SYSCAPS out_syscaps;
	HD_PATH_ID out_ctrl;
	HD_PATH_ID out_path;

    // (4)
	pthread_t  acap_thread_id;
	HD_PATH_ID  acap_ctrl;
	HD_PATH_ID  acap_path;
	HD_AUDIO_SR acap_sr_max;
	HD_AUDIO_SR acap_sr;
	UINT32      acap_enter;
	UINT32      acap_exit;

	HD_VIDEOOUT_HDMI_ID hdmi_id;

	DEMO_OSG osg;

} DEMO;


////////////////////////////

int osg_query_buf_size(UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt);

HD_RESULT osg_init(UINT32 id, DEMO_OSG* osg, UINT32 w, UINT32 h, HD_VIDEO_PXLFMT pxlfmt, unsigned short c);
HD_RESULT osg_open(UINT32 id, DEMO_OSG* osg, UINT32 type, UINT32 venc_in);
HD_RESULT osg_start(UINT32 id, DEMO_OSG* osg, UINT32 type, UINT32 x, UINT32 y);
HD_RESULT osg_close(UINT32 id, DEMO_OSG* osg, UINT32 type);
HD_RESULT osg_uninit(UINT32 id, DEMO_OSG* osg);

