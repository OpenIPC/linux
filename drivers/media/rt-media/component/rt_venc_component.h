

#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "rt_common.h"
#include "../_uapi_rt_media.h"

#ifndef _RT_VENC_COMPONENT_H_
#define _RT_VENC_COMPONENT_H_

#define VENC_IN_BUFFER_LIST_NODE_NUM (3)
#define VENC_OUT_BUFFER_LIST_NODE_NUM (32)

typedef struct rt_venc_bin_image_param {
	unsigned int enable;
	unsigned int moving_th; //range[1,31], 1:all frames are moving,
	//			31:have no moving frame, default: 20
} rt_venc_bin_image_param;

typedef struct venc_comp_header_data {
	unsigned char *pBuffer;
	unsigned int nLength;
} venc_comp_header_data;

typedef struct VENC_VBV_BUFFER_INFO {
	int handle_id;
	int size;
} VENC_VBV_BUFFER_INFO;

typedef enum VENC_COMP_RC_MODE {
	VENC_COMP_RC_MODE_H264CBR = 1,
	VENC_COMP_RC_MODE_H264VBR,

	VENC_RC_MODE_BUTT,

} VENC_COMP_RC_MODE;

typedef enum {
	VENC_COMP_CODEC_H264 = 0,
	VENC_COMP_CODEC_H265 = 1,
	VENC_COMP_CODEC_JPEG = 2,
} VENC_COMP_CODEC_TYPE;

typedef enum VENC_COMP_LBC_SUBFMT {
	VENC_COMP_LBC_LOSSLESS   = 0,
	VENC_COMP_LBC_LOSSY_2X   = 1,
	VENC_COMP_LBC_LOSSY_2_5X = 2,
} VENC_COMP_LBC_SUBFMT;

typedef struct venc_comp_vbr_param {
	unsigned int max_bit_rate;
	unsigned int moving_th; //range[1,31], 1:all frames are moving,
	//            31:have no moving frame, default: 20
	int quality; //range[1,10], 1:worst quality, 10:best quality
} venc_comp_vbr_param;

typedef struct venc_comp_base_config {
	VENC_COMP_CODEC_TYPE codec_type;
	unsigned int src_width;
	unsigned int src_height;
	unsigned int dst_width;
	unsigned int dst_height;
	int bit_rate;
	int frame_rate;
	int max_keyframe_interval;
	video_qp_range qp_range;
	int profile;
	int level;
	rt_pixelformat_type pixelformat;

	VENC_COMP_RC_MODE rc_mode;
	/* should config the param when rc_mode is vbr*/
	venc_comp_vbr_param vbr_param;

	/* should config the param when pixel_format is lbc*/
	VENC_COMP_LBC_SUBFMT lbc_sub_format;

	int quality; // for jpeg
	RTVencMotionSearchParam motion_search_param;
	int enable_overlay;

	int bOnlineMode;
	int bOnlineChannel;
	int channel_id;
} venc_comp_base_config;

typedef struct venc_comp_normal_config {
	int tmp;
} venc_comp_normal_config;

typedef struct video_stream_node {
	video_stream_s video_stream;
	struct list_head mList;
} video_stream_node;

typedef struct venc_outbuf_manager {
	struct list_head empty_stream_list;
	struct list_head valid_stream_list;
	struct mutex mutex;
	int empty_num;
	int no_empty_stream_flag;
} venc_outbuf_manager;

typedef struct venc_inbuf_manager {
	struct list_head empty_frame_list;
	struct list_head valid_frame_list;
	struct mutex mutex;
	int empty_num;
	int no_frame_flag;
} venc_inbuf_manager;

error_type venc_comp_component_init(PARAM_IN comp_handle component, int channel_id);

#endif
