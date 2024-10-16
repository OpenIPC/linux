

#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "vin_video_api.h"
#include "../_uapi_rt_media.h"
#include "rt_common.h"

#ifndef _RT_VI_COMPONENT_H_
#define _RT_VI_COMPONENT_H_

#define ENABLE_SAVE_VIN_OUT_DATA 0
typedef struct vi_comp_base_config {
	unsigned int         width;
	unsigned int         height;
	int                  frame_rate;
	int                  channel_id;
	rt_pixelformat_type  pixelformat;
	enum VIDEO_OUTPUT_MODE output_mode;
	int                    drop_frame_num;
	int                    enable_wdr;
	int 				   enable_high_fps_transfor;
} vi_comp_base_config;

typedef struct vi_comp_normal_config {
	int tmp;
} vi_comp_normal_config;

/* dynamic config */

typedef struct vi_outbuf_manager {
	struct list_head    empty_frame_list;
	struct list_head    valid_frame_list;
	struct mutex        mutex;
	int                 empty_num;
	int                 no_empty_frame_flag;
} vi_outbuf_manager;

error_type vi_comp_component_init(PARAM_IN comp_handle component, int channel_id);

#endif

