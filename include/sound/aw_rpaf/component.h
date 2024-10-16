/*
 * linux/sound/aw_rpaf/component.h -- Remote Process Audio Framework Layer
 *
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef _AW_RPAF_COMPONENT_H_
#define _AW_RPAF_COMPONENT_H_

#include <sound/aw_rpaf/common.h>

/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
#define RPAF_COMPONENT_MAX_NUM (sizeof(uint32_t) << 3)

/*
 * dsp 音频组件部分
 */
enum SND_DSP_COMPONENT_TYPE {
	/* 前处理 */
	SND_DSP_COMPONENT_AGC = 0, /* 自动增益控制 */
	SND_DSP_COMPONENT_AEC = 1, /* 回声消除 */
	SND_DSP_COMPONENT_NS,  /* 噪声抑制 */

	/* 后处理 */
	SND_DSP_COMPONENT_DECODEC, /* 解码，暂保留 */
	SND_DSP_COMPONENT_EQ,	   /* 均衡器 */
	SND_DSP_COMPONENT_MIXER,   /* 左右声道混音 */
	SND_DSP_COMPONENT_DRC,	   /* 动态范围控制 */
	SND_DSP_COMPONENT_REVERB,  /* 混响 */

	/* 通用组件 */
	SND_DSP_COMPONENT_RESAMPLE,	/* 重采样，暂保留 */
	SND_DSP_COMPONENT_OTHER,

	SND_DSP_COMPONENT_USER = 20,

	SND_DSP_COMPONENT_MAX = 31,
};

enum snd_dsp_comp_mode {
	SND_DSP_COMPONENT_MODE_INDEPENDENCE = 0,
	SND_DSP_COMPONENT_MODE_STREAM,
	SND_DSP_COMPONENT_MODE_ALGO,
};

#define LABEL_SND_DSP_COMPONENT_NAME(constant) {#constant, constant}

struct label_snd_dsp_component_name {
	char *name;
	uint32_t value;
};

/* init at dsp after get component. */
/* 64bit variable, such as ssize_t, is not recommended */
struct snd_soc_dsp_component {
	uint32_t cmd_val;
	uint32_t params_val;
	uint32_t used;

	struct snd_soc_dsp_pcm_params params;

	/* 唯一身份识别号 */
	uint32_t id;

	/* share data address */
	uint32_t write_addr;
	/* buf_length < buf_size */
	uint32_t write_length;
	uint32_t write_size;

	uint32_t dump_addr[RPAF_COMPONENT_MAX_NUM];
	uint32_t dump_length[RPAF_COMPONENT_MAX_NUM];
	uint32_t dump_size;

	uint32_t read_addr;
	/* buf_length < buf_size */
	uint32_t read_length;
	uint32_t read_size;

	uint32_t transfer_type;

	/* 代表的伴随音频流的组件还是独立操作用的组件 */
	uint32_t comp_mode;
	/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
	uint32_t component_type; /* 0x1 << SND_DSP_AUDIO_COMPONENT_AGC/... */
	uint32_t sort_index;
	/* 当前流的各个组件状态值 */
	int32_t status[RPAF_COMPONENT_MAX_NUM];
	int32_t component_sort[RPAF_COMPONENT_MAX_NUM];

	/*API调用完毕之后需要判断该值 */
	int32_t ret_val;

	/* dsp cannot used */
	struct list_head list;
};

struct snd_soc_dsp_component_config {
	uint32_t comp_mode;
	/* 表示对于绑定alsa straem组件中dsp dump某个组件处理后数据 */
	uint32_t transfer_type;
	uint32_t component_used;
	/* 代表的是该音频流有多少个组件（最多支持32个for 32bit machine）在用 */
	uint32_t component_type; /* 0x1 << SND_DSP_AUDIO_COMPONENT_AGC/... */
	uint32_t sort_index;
	int32_t component_sort[RPAF_COMPONENT_MAX_NUM];
	struct snd_soc_dsp_pcm_params pcm_params;
};

struct snd_soc_dsp_native_component {
	struct snd_soc_dsp_component soc_component;

	unsigned char soc_suspended;
	unsigned char soc_resumed;

	/* wakeup */
	uint32_t keyword_wakeup;

	/* dump flag */
	uint32_t dump_start[RPAF_COMPONENT_MAX_NUM];

	/* 给 algorithmic 分配使用 */
	void *handle[RPAF_COMPONENT_MAX_NUM];
	void *private_data[RPAF_COMPONENT_MAX_NUM];

	struct list_head list;
};

/*
 * For DSP Audio Framework API
 * 均要考虑资源要访问互斥，尤其针对队列操作函数
 */
struct snd_dsp_hal_component_ops {
	/* 实现初始化，包括配置初始化substream的组件 */
	//int32_t (*create)(struct snd_dsp_hal_component *component);
	int32_t (*create)(void *component);
	/* 去初始化，释放相应资源 */
	int32_t (*remove)(void *component);
	/* 告知Linux音频组件挂起或者已经恢复 */
	int32_t (*suspend)(void *component);
	int32_t (*resume)(void *component);

	int32_t (*status)(void *component);
	int32_t (*sw_params)(void *component);

	/*
	 * 用于某个流的组件的启用和关闭，需配合substream使用，
	 * dump数据可调用，内部需要引用计数
	 */
	int32_t (*start)(void *component);
	int32_t (*stop)(void *component);
	/* 主要用于播放经过编码的数据，保留 */
	int32_t (*write)(void *component);
	/* 主要用于dump 每个组件之后经过处理的数据 */
	int32_t (*read)(void *component);
};

struct msg_component_package {
	wait_queue_head_t tsleep;
	spinlock_t lock;
	int32_t wakeup_flag;
	struct snd_soc_dsp_component soc_component;
	struct list_head list;
};

#endif
