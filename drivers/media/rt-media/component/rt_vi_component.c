
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/fs.h>
#define LOG_TAG "rt_vi_comp"

#include "rt_common.h"
#include "rt_vi_component.h"
#include "rt_message.h"

#include "mem_interface.h"

#define VI_OUT_BUFFER_LIST_NODE_NUM (3)

#define RT_VI_SHOW_TIME_COST (0)

#define CLEAR(x) (memset(&(x), 0, sizeof(x)))

struct vi_comp_ctx *g_vi_comp[VIDEO_INPUT_CHANNEL_NUM];

static int64_t time_vi_start;
static int64_t time_first_cb;

static int callback_count;

typedef struct reduce_fps_info {
	int bEnable;
	int drop_frame_nums; /*need drop frame nums per second*/
	int drop_cnt_in_second;
	int drop_point_in_second;
	int frame_cnt_in_second;
	uint64_t pre_pts_in_seconds;
} reduce_fps_info;

typedef struct vi_comp_ctx {
	comp_state_type state;
	struct task_struct *vi_thread;
	message_queue_t msg_queue;
	comp_callback_type *callback;
	void *callback_data;
	rt_component_type *self_comp;
	wait_queue_head_t wait_reply[WAIT_REPLY_NUM];
	unsigned int wait_reply_condition[WAIT_REPLY_NUM];
	vi_outbuf_manager out_buf_manager;
	comp_tunnel_info in_port_tunnel_info;
	comp_tunnel_info out_port_tunnel_info;
	vi_comp_base_config base_config;
	vi_comp_normal_config normal_config;
	config_yuv_buf_info yuv_buf_info;
	int config_yuv_buf_flag;
	int align_width;
	int align_height;
	int buf_size;

	struct vin_buffer vin_buf[VI_OUT_BUFFER_LIST_NODE_NUM];
	int vipp_id;
	struct mem_interface *memops;

	spinlock_t vi_spin_lock;
	wait_queue_head_t wait_in_buf;
	unsigned int wait_in_buf_condition;
	int catch_jpeg_flag;
	int max_fps;

	reduce_fps_info reduce_fps;
} vi_comp_ctx;

static int thread_process_vi(void *param);

static int config_videostream_by_vinbuf(vi_comp_ctx *vi_comp,
					video_frame_s *pvideo_stream,
					struct vin_buffer *pvin_buf)
{
	int y_size		= vi_comp->align_width * vi_comp->base_config.height;
	unsigned char *phy_addr = pvin_buf->paddr;
	unsigned char *vir_addr = cdc_mem_get_vir(vi_comp->memops, (unsigned long)phy_addr);

	//pvideo_stream->pts         = (uint64_t)(pvin_buf->timestamp/1000);
	pvideo_stream->phy_addr[0] = (unsigned int)phy_addr;
	pvideo_stream->phy_addr[1] = (unsigned int)(phy_addr + y_size);
	pvideo_stream->vir_addr[0] = (void *)vir_addr;
	pvideo_stream->vir_addr[1] = (void *)(vir_addr + y_size);
	pvideo_stream->private     = (void *)pvin_buf;
	pvideo_stream->buf_size    = vi_comp->buf_size;

	if (vi_comp->vipp_id == 0)
		RT_LOGI("boot id = %d, video pts = %llu us, cur_time = %llu us,",
			vi_comp->vipp_id, pvideo_stream->pts, ktime_get_ns() / 1000);

	return 0;
}

void vin_buffer_callback(int id)
{
	vi_comp_ctx *vi_comp = g_vi_comp[id];

	RT_LOGD("callback, id = %d", id);

	time_first_cb = get_cur_time();
	callback_count++;

	if (callback_count == 1)
		RT_LOGW("callback, time = %lld", time_first_cb - time_vi_start);

	vi_comp->wait_in_buf_condition = 1;
	wake_up(&vi_comp->wait_in_buf);

	return;
}
#if ENABLE_SAVE_VIN_OUT_DATA
void vi_comp_save_outdata(struct vi_comp_ctx *pvi_comp, video_frame_s *pvideo_frame)
{
	struct mem_interface *_memops = pvi_comp->memops;
    int nSaveLen = pvideo_frame->buf_size;
	unsigned char *pVirBuf = pvideo_frame->vir_addr[0];
	int width = pvi_comp->base_config.width;
	int height = pvi_comp->base_config.height;
    int ret = 0;
    mm_segment_t old_fs;
	char name[128];
	struct file *fp;
	static int count;
	if (pvi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X
		|| pvi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		sprintf(name, "/mnt/extsd/lbc_%d_%d_%d.bin", width, height, count++);
	} else {
		sprintf(name, "/mnt/extsd/yuv_%d_%d_%d.yuv", width, height, count++);
	}
	if (count > 5)
		count = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    fp = filp_open(name, O_CREAT|O_RDWR, 0600);
	if (IS_ERR(fp)) {
		RT_LOGE("open file failed");
		set_fs(old_fs);
		return ;
	}
	RT_LOGW("file name = %s, nSaveLen = %d, fp = %p, id = %d pVirBuf %p",
	     name, nSaveLen, fp, pvideo_frame->id, pVirBuf);

	cdc_mem_flush_cache(_memops, pVirBuf, nSaveLen);
	ret = vfs_write(fp, (char __user *)pVirBuf, nSaveLen, &fp->f_pos);
	RT_LOGW("vfs_write ret: %d", ret);
	filp_close(fp, NULL);
    set_fs(old_fs);
	return ;
}
#endif
/* empty_list --> valid_list */
static int vi_fill_out_buffer_done(struct vi_comp_ctx *vi_comp,
				   struct vin_buffer *vin_buf)
{
	video_frame_node *first_node = NULL;
	comp_buffer_header_type buffer_header;
	video_frame_s mvideo_frame;

	memset(&mvideo_frame, 0, sizeof(video_frame_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));
	config_videostream_by_vinbuf(vi_comp, &mvideo_frame, vin_buf);
	mutex_lock(&vi_comp->out_buf_manager.mutex);
	if (list_empty(&vi_comp->out_buf_manager.empty_frame_list)) {
		RT_LOGE("error: empty_frame_list is empty");
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		return -1;
	}

	first_node = list_first_entry(&vi_comp->out_buf_manager.empty_frame_list, video_frame_node, mList);
	memcpy(&first_node->video_frame, &mvideo_frame, sizeof(video_frame_s));

	vi_comp->out_buf_manager.empty_num--;
	list_move_tail(&first_node->mList, &vi_comp->out_buf_manager.valid_frame_list);

	mutex_unlock(&vi_comp->out_buf_manager.mutex);

	buffer_header.private = &mvideo_frame;

	#if ENABLE_SAVE_VIN_OUT_DATA
		vi_comp_save_outdata(vi_comp, &mvideo_frame);
	#endif
	if (vi_comp->out_port_tunnel_info.valid_flag == 1 && vi_comp->out_port_tunnel_info.tunnel_comp != NULL) {
		comp_empty_this_in_buffer(vi_comp->out_port_tunnel_info.tunnel_comp, &buffer_header);
	} else {
		RT_LOGE("tunnel_comp is null: %d, %p, return to isp", vi_comp->out_port_tunnel_info.valid_flag,
			vi_comp->out_port_tunnel_info.tunnel_comp);
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
	}

	return 0;
}

static int check_reduce_fps(vi_comp_ctx *vi_comp, uint64_t pts)
{
	int bNeed_drop_frame	 = 0;
	reduce_fps_info *preduce_fps = &vi_comp->reduce_fps;

	if (preduce_fps->bEnable == 1 && preduce_fps->drop_frame_nums > 0 && preduce_fps->drop_point_in_second > 0) {
		uint64_t cur_pts_in_seconds = pts / 1000 / 1000 / 1000;

		if (cur_pts_in_seconds != preduce_fps->pre_pts_in_seconds) {
			preduce_fps->drop_cnt_in_second  = 0;
			preduce_fps->frame_cnt_in_second = 0;
			preduce_fps->pre_pts_in_seconds  = cur_pts_in_seconds;
		}

		preduce_fps->frame_cnt_in_second++;

		if (preduce_fps->frame_cnt_in_second % preduce_fps->drop_point_in_second == 0) {
			if (preduce_fps->drop_cnt_in_second < preduce_fps->drop_frame_nums)
				bNeed_drop_frame = 1;
			preduce_fps->drop_cnt_in_second++;
		}
		RT_LOGI("need_drop = %d, cur*pre_pts = %llu, %llu, drop_cnt = %d, drop_num = %d, frame_cnt = %d, drop_point = %d",
			bNeed_drop_frame, cur_pts_in_seconds, preduce_fps->pre_pts_in_seconds,
			preduce_fps->drop_cnt_in_second, preduce_fps->drop_frame_nums,
			preduce_fps->frame_cnt_in_second, preduce_fps->drop_point_in_second);
	}

	return bNeed_drop_frame;
}

static int dequeue_count;

int dequeue_buffer(vi_comp_ctx *vi_comp)
{
	struct vin_buffer *vin_buf = NULL;
	int ret			   = 0;

	ret = vin_dqbuffer_special(vi_comp->vipp_id, &vin_buf);

	/* RT_LOGD("dequeue buf, ret = %d",ret); */

	if (ret != 0)
		return -1;

	dequeue_count++;

	if (dequeue_count == 1) {
		int64_t time_cur = get_cur_time();
		RT_LOGW("wait first dequeue time = %lld", time_cur - time_first_cb);
	}

	/* just queue back buf if output mode is yuv */
	if (vi_comp->base_config.output_mode == OUTPUT_MODE_YUV) {
		if (vi_comp->catch_jpeg_flag == 1) {
			vi_comp->catch_jpeg_flag = 0;
			vi_fill_out_buffer_done(vi_comp, vin_buf);
		} else {
			vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		}
	} else {
		if (vi_comp->base_config.drop_frame_num > 0) {
			RT_LOGI("drop_frame_num = %d", vi_comp->base_config.drop_frame_num);
			vi_comp->base_config.drop_frame_num--;
			vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		} else {
			int bDrop_flag = check_reduce_fps(vi_comp, 0); //vin_buf->timestamp);

			if (bDrop_flag == 1)
				vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
			else {
				vi_fill_out_buffer_done(vi_comp, vin_buf);
			}
		}
	}

	return 0;
}

static int vipp_create(vi_comp_ctx *vi_comp)
{
	RT_LOGI("vipp create not support");

	return -1;
}

static int alloc_vin_buf(vi_comp_ctx *vi_comp)
{
	struct mem_param param;
	int i = 0;

	int buf_size		  = vi_comp->align_width * vi_comp->align_height * 3 / 2;
	unsigned int lbc_ext_size = 0;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		int y_stride		   = 0;
		int yc_stride		   = 0;
		int bit_depth		   = 8;
		int com_ratio_even	 = 0;
		int com_ratio_odd	  = 0;
		int pic_width_32align      = (vi_comp->base_config.width + 31) & ~31;
		int pic_height_16align     = (vi_comp->base_config.height + 15) & ~15;
		int bLbcLossyComEnFlag2x   = 0;
		int bLbcLossyComEnFlag2_5x = 0;

		if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
			bLbcLossyComEnFlag2_5x = 1;
		} else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
			bLbcLossyComEnFlag2x = 1;
		}
		RT_LOGD("bLbcLossyComEnFlag2x %d bLbcLossyComEnFlag2_5x %d", bLbcLossyComEnFlag2x, bLbcLossyComEnFlag2_5x);
		if (bLbcLossyComEnFlag2x == 1) {
			com_ratio_even = 600;
			com_ratio_odd  = 450;
			y_stride       = ((com_ratio_even * pic_width_32align * bit_depth / 1000 + 511) & (~511)) >> 3;
			yc_stride      = ((com_ratio_odd * pic_width_32align * bit_depth / 500 + 511) & (~511)) >> 3;
		} else if (bLbcLossyComEnFlag2_5x == 1) {
			com_ratio_even = 440;
			com_ratio_odd  = 380;
			y_stride       = ((com_ratio_even * pic_width_32align * bit_depth / 1000 + 511) & (~511)) >> 3;
			yc_stride      = ((com_ratio_odd * pic_width_32align * bit_depth / 500 + 511) & (~511)) >> 3;
		} else {
			y_stride  = ((pic_width_32align * bit_depth + pic_width_32align / 16 * 2 + 511) & (~511)) >> 3;
			yc_stride = ((pic_width_32align * bit_depth * 2 + pic_width_32align / 16 * 4 + 511) & (~511)) >> 3;
		}

		buf_size = (y_stride + yc_stride) * pic_height_16align / 2;

		//* add more 1KB to fix ve-lbc-error
		lbc_ext_size = 2 * 1024;
		buf_size += lbc_ext_size;

		RT_LOGW("LBC in buf:com_ratio: %d, %d, w32alin = %d, h16alin = %d, y_s = %d, yc_s = %d, total_len = %d,\n",
			com_ratio_even, com_ratio_odd,
			pic_width_32align, pic_height_16align,
			y_stride, yc_stride, buf_size);
	} else if (vi_comp->base_config.pixelformat == RT_PIXEL_YUV420SP) {
		buf_size = vi_comp->align_width * vi_comp->align_height * 3 / 2;
	}

	vi_comp->buf_size = buf_size - lbc_ext_size;
	RT_LOGW("buf_size = %d, align_w = %d, align_h = %d", buf_size,
		vi_comp->align_width, vi_comp->align_height);
	if (vi_comp->memops == NULL) {
		vi_comp->memops = mem_create(MEM_TYPE_ION, param);
		if (vi_comp->memops == NULL) {
			RT_LOGE("mem_create failed\n");
			return -1;
		}
	}

	if (vi_comp->base_config.output_mode == OUTPUT_MODE_YUV) {
		RT_LOGW("vi_comp->vipp_id %d config yuv buf, size = %d, %d, num = %d", vi_comp->vipp_id,
			buf_size, vi_comp->yuv_buf_info.buf_size,
			vi_comp->yuv_buf_info.buf_num);
		if (vi_comp->yuv_buf_info.buf_num != VI_OUT_BUFFER_LIST_NODE_NUM) {
			RT_LOGE("yuv_buf_info.buf_num is not match: %d, %d",
				vi_comp->yuv_buf_info.buf_num, VI_OUT_BUFFER_LIST_NODE_NUM);
			return -1;
		}
		for (i = 0; i < vi_comp->yuv_buf_info.buf_num; i++) {
			vi_comp->vin_buf[i].paddr = vi_comp->yuv_buf_info.phy_addr[i];
			vin_qbuffer_special(vi_comp->vipp_id, &vi_comp->vin_buf[i]);
		}
		vi_comp->config_yuv_buf_flag = 1;
	} else {
		for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
			unsigned char *vir_addr = cdc_mem_palloc(vi_comp->memops, buf_size);

			if (vir_addr == NULL) {
				RT_LOGE("cdc_mem_palloc failed, size = %d", buf_size);
				return -1;
			}
			vi_comp->vin_buf[i].paddr = (void *)cdc_mem_get_phy(vi_comp->memops, vir_addr);

			RT_LOGW("palloc vin buf: vir = %p, phy = %p, i = %d", vir_addr, vi_comp->vin_buf[i].paddr, i);
			vin_qbuffer_special(vi_comp->vipp_id, &vi_comp->vin_buf[i]);
		}
	}

	return 0;
}

#if 0
static int vipp_init(vi_comp_ctx *vi_comp)
{
	int ret = 0;
	int id = vi_comp->base_config.channel_id;
	struct v4l2_streamparm parms;
	int mode = 4; /* NV12 */
	struct v4l2_format fmt;
	int wdr_param = 0;

	//if(vi_comp->base_config.width >= 1280)
	if (vi_comp->base_config.pixelformat == RT_PIXEL_YUV420SP)
		mode = 4; /* NV12 */
	else if (vi_comp->base_config.pixelformat == RT_PIXELFORMAT_NV21)
		mode = 8; /* NV21 */
	else if (vi_comp->base_config.pixelformat == RT_PIXELFORMAT_YV12)
		mode = 2; /* YV12 */
	else if (vi_comp->base_config.pixelformat == RT_PIXELFORMAT_YV21)
		mode = 9; /* YV21 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X)
		mode = 7; /* LBC2.5 */

	RT_LOGW("vipp init: widht = %d, height = %d, fps = %d, id = %d, mode = %d, format = %d",
			vi_comp->base_config.width, vi_comp->base_config.height,
			vi_comp->base_config.frame_rate, id, mode,
			vi_comp->base_config.pixelformat);

	callback_count = 0;
	dequeue_count = 0;
	time_vi_start = 0;
	time_first_cb = 0;

	ret = vin_open_special(id);
	RT_LOGI("vin open, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_start = get_cur_time();
#endif

	ret = vin_s_input_special(id, 0);
	RT_LOGI("vin s input, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_end = get_cur_time();
	RT_LOGE("time of s input: %lld", (time3_end - time3_start));
#endif

	if (vi_comp->base_config.enable_wdr == 1)
		wdr_param = 1;
	else
		wdr_param = 2;
	RT_LOGI("wdr_param = %d, enable_wdr = %d", wdr_param, vi_comp->base_config.enable_wdr);

	/* set paramter */
	CLEAR(parms);
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = vi_comp->base_config.frame_rate;
	parms.parm.capture.capturemode = V4L2_MODE_VIDEO;
	/* parms.parm.capture.capturemode = V4L2_MODE_IMAGE; */
	/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
	parms.parm.capture.reserved[0] = 0;
	parms.parm.capture.reserved[1] = wdr_param;/*2:command, 1: wdr, 0: normal*/


	ret = vin_s_parm_special(id, NULL, &parms);
	RT_LOGI("vin s parm, ret = %d, id = %d", ret, id);

	/* set format */
	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width  = vi_comp->base_config.width;
	fmt.fmt.pix_mp.height = vi_comp->base_config.height;
	switch (mode) {
	case 0:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR8;
		break;
	case 1:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	case 2:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
		break;
	case 3:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
		break;
	case 4:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		break;
	case 5:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR10;
		break;
	case 6:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR12;
		break;
	case 7:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_LBC_2_5X;
		break;
	case 8:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21;
		break;
	case 9:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YVU420;
		break;
	default:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	}
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

#if RT_VI_SHOW_TIME_COST
	int64_t time2_start = get_cur_time();
#endif

	ret = vin_s_fmt_special(id, &fmt);
	RT_LOGI("vin s fmt, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time2_end = get_cur_time();
	RT_LOGE("time of s fmt: %lld", (time2_end - time2_start));
#endif

	ret = vin_g_fmt_special(id, &fmt);
	RT_LOGI("vin g fmt, ret = %d, id = %d", ret, id);

	RT_LOGD("resolution got from sensor = %d*%d num_planes = %d\n",
			fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
			fmt.fmt.pix_mp.num_planes);

	/* queue buffer */
	vi_comp->vipp_id = id;
	g_vi_comp[id] = vi_comp;

	vin_register_buffer_done_callback(id, vin_buffer_callback);

#if RT_VI_SHOW_TIME_COST
	int64_t time_start = get_cur_time();
#endif

	alloc_vin_buf(vi_comp);

#if RT_VI_SHOW_TIME_COST
	int64_t time_end = get_cur_time();
	RT_LOGE("time of alloc buf: %lld", (time_end - time_start));
#endif

	/* stream on */
#if RT_VI_SHOW_TIME_COST
	int64_t time1_start = get_cur_time();
#endif

	ret = vin_streamon_special(vi_comp->vipp_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	RT_LOGI("vin streamon, ret = %d, id = %d", ret, id);

	time_vi_start = get_cur_time();

#if RT_VI_SHOW_TIME_COST
	int64_t time1_end = get_cur_time();
	RT_LOGE("time of stream on: %lld", (time1_end - time1_start));
#endif

	return 0;
}
#endif

static void highframe_callback(int id)
{
	vi_comp_ctx *vi_comp = g_vi_comp[id];
	message_t cmd_msg;

	RT_LOGW("%s:change to low frame, id = %d", __func__, id);

	memset(&cmd_msg, 0, sizeof(message_t));
	cmd_msg.command = COMP_COMMAND_RESET_HIGH_FPS;

	put_message(&vi_comp->msg_queue, &cmd_msg);
	return;
}

static int highframe_alloc_vin_buf(vi_comp_ctx *vi_comp, int width, int height)
{
	struct mem_param param;
	int i = 0;

	int buf_size = width * height * 3 / 2;

	vi_comp->buf_size = buf_size;
	RT_LOGW("%s: buf_size = %d, align_w = %d, align_h = %d", __func__, buf_size,
		width, height);

	if (vi_comp->memops == NULL) {
		vi_comp->memops = mem_create(MEM_TYPE_ION, param);
		if (vi_comp->memops == NULL) {
			RT_LOGE("mem_create failed\n");
			return -1;
		}
	}

	for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
		unsigned char *vir_addr = cdc_mem_palloc(vi_comp->memops, buf_size);

		if (vir_addr == NULL) {
			RT_LOGE("cdc_mem_palloc failed, size = %d", buf_size);
			return -1;
		}
		vi_comp->vin_buf[i].paddr = (void *)cdc_mem_get_phy(vi_comp->memops, vir_addr);

		RT_LOGI("palloc vin buf: vir = %p, phy = %p, i = %d", vir_addr, vi_comp->vin_buf[i].paddr, i);
		vin_qbuffer_special(vi_comp->vipp_id, &vi_comp->vin_buf[i]);
	}

	return 0;
}

static void highframe_free_vin_buf(vi_comp_ctx *vi_comp)
{
	int i = 0;

	RT_LOGW("%s:free vin buffer\n", __func__);

	for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
		if (vi_comp->vin_buf[i].paddr) {
			unsigned char *virAddr = (unsigned char *)cdc_mem_get_vir(vi_comp->memops,
										  (unsigned long)vi_comp->vin_buf[i].paddr);
			cdc_mem_pfree(vi_comp->memops, virAddr);
		}
	}
}

static int highframe_vipp_init(vi_comp_ctx *vi_comp)
{
	int ret = 0;
	int id  = vi_comp->base_config.channel_id;
	struct v4l2_streamparm parms;
	int mode = 4; /* NV12 */
	struct v4l2_format fmt;
	int wdr_param, frame_rate, isp_drop_frame;
	int width, height;

	callback_count = 0;
	dequeue_count  = 0;
	time_vi_start  = 0;
	time_first_cb  = 0;

	ret = vin_open_special(id);
	RT_LOGI("vin open, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_start = get_cur_time();
#endif

	ret = vin_s_input_special(id, 0);
	RT_LOGI("vin s input, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_end = get_cur_time();
	RT_LOGE("time of s input: %lld", (time3_end - time3_start));
#endif

	wdr_param      = 2;
	frame_rate     = 120;
	isp_drop_frame = 7;
	/* set paramter */
	CLEAR(parms);
	parms.type				    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.timeperframe.numerator   = 1;
	parms.parm.capture.timeperframe.denominator = frame_rate;
	parms.parm.capture.capturemode		    = V4L2_MODE_VIDEO;
	/* parms.parm.capture.capturemode = V4L2_MODE_IMAGE; */
	/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
	parms.parm.capture.reserved[0] = 0;
	parms.parm.capture.reserved[1] = wdr_param; /*2:command, 1: wdr, 0: normal*/
	parms.parm.capture.reserved[3] = isp_drop_frame;

	ret = vin_s_parm_special(id, NULL, &parms);
	RT_LOGI("vin s parm, ret = %d, id = %d", ret, id);

	/* set format */
	width  = 1280;
	height = 720;
	CLEAR(fmt);
	fmt.type	      = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width  = width;
	fmt.fmt.pix_mp.height = height;
	switch (mode) {
	case 0:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR8;
		break;
	case 1:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	case 2:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
		break;
	case 3:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
		break;
	case 4:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		break;
	case 5:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR10;
		break;
	case 6:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR12;
		break;
	case 7:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_LBC_2_5X;
		break;
	case 8:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21;
		break;
	case 9:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YVU420;
		break;
	default:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	}
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

#if RT_VI_SHOW_TIME_COST
	int64_t time2_start = get_cur_time();
#endif

	ret = vin_s_fmt_special(id, &fmt);
	RT_LOGI("vin s fmt, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time2_end = get_cur_time();
	RT_LOGE("time of s fmt: %lld", (time2_end - time2_start));
#endif

	ret = vin_g_fmt_special(id, &fmt);
	RT_LOGI("vin g fmt, ret = %d, id = %d", ret, id);

	RT_LOGD("resolution got from sensor = %d*%d num_planes = %d\n",
		fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
		fmt.fmt.pix_mp.num_planes);

	/* queue buffer */
	vi_comp->vipp_id = id;
	g_vi_comp[id]    = vi_comp;

	vin_register_buffer_done_callback(id, highframe_callback);

#if RT_VI_SHOW_TIME_COST
	int64_t time_start = get_cur_time();
#endif

	highframe_alloc_vin_buf(vi_comp, width, height);

#if RT_VI_SHOW_TIME_COST
	int64_t time_end = get_cur_time();
	RT_LOGE("time of alloc buf: %lld", (time_end - time_start));
#endif

/* stream on */
#if RT_VI_SHOW_TIME_COST
	int64_t time1_start = get_cur_time();
#endif

	RT_LOGW("highframe vipp init: %d*%d@%dfps, drop frame %d", width, height, frame_rate, isp_drop_frame);

	ret = vin_streamon_special(vi_comp->vipp_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	RT_LOGI("vin streamon, ret = %d, id = %d", ret, id);

	time_vi_start = get_cur_time();

#if RT_VI_SHOW_TIME_COST
	int64_t time1_end = get_cur_time();
	RT_LOGE("time of stream on: %lld", (time1_end - time1_start));
#endif

	return 0;
}

static int vipp_init(vi_comp_ctx *vi_comp)
{
	int ret = 0;
	int id  = vi_comp->base_config.channel_id;
	struct v4l2_streamparm parms;
	int mode = 4; /* NV12 */
	struct v4l2_format fmt;
	int wdr_param = 0;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_YUV420SP)
		mode = 4; /* NV12 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_YVU420SP)
		mode = 8; /* NV21 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_YUV420P)
		mode = 2; /* YV12 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_YVU420P)
		mode = 9; /* YV21 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X)
		mode = 7; /* LBC2.5 */
	else if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X)
		mode = 10; /* LBC2.0 */

	RT_LOGW("vipp init: widht = %d, height = %d, fps = %d, id = %d, mode = %d, format = %d",
		vi_comp->base_config.width, vi_comp->base_config.height,
		vi_comp->base_config.frame_rate, id, mode,
		vi_comp->base_config.pixelformat);

	callback_count = 0;
	dequeue_count  = 0;
	time_vi_start  = 0;
	time_first_cb  = 0;

	if (vi_comp->base_config.enable_high_fps_transfor == 0)
		ret = vin_open_special(id);
//RT_LOGI("vin open, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_start = get_cur_time();
#endif

	if (vi_comp->base_config.enable_high_fps_transfor == 0)
		ret = vin_s_input_special(id, 0);
//RT_LOGI("vin s input, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time3_end = get_cur_time();
	RT_LOGE("time of s input: %lld", (time3_end - time3_start));
#endif

	if (vi_comp->base_config.enable_wdr == 1)
		wdr_param = 1;
	else
		wdr_param = 2;
	RT_LOGI("wdr_param = %d, enable_wdr = %d", wdr_param, vi_comp->base_config.enable_wdr);

	/* set parameter */
	CLEAR(parms);
	parms.type				    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.timeperframe.numerator   = 1;
	parms.parm.capture.timeperframe.denominator = vi_comp->base_config.frame_rate;
	parms.parm.capture.capturemode		    = V4L2_MODE_VIDEO;
	/* parms.parm.capture.capturemode = V4L2_MODE_IMAGE; */
	/*when different video have the same sensor source, 1:use sensor current win, 0:find the nearest win*/
	parms.parm.capture.reserved[0] = 0;
	parms.parm.capture.reserved[1] = wdr_param; /*2:command, 1: wdr, 0: normal*/

	ret = vin_s_parm_special(id, NULL, &parms);
	RT_LOGI("vin s parm, ret = %d, id = %d", ret, id);

	/* set format */
	CLEAR(fmt);
	fmt.type	      = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width  = vi_comp->base_config.width;
	fmt.fmt.pix_mp.height = vi_comp->base_config.height;
	switch (mode) {
	case 0:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR8;
		break;
	case 1:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	case 2:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
		break;
	case 3:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
		break;
	case 4:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		break;
	case 5:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR10;
		break;
	case 6:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_SBGGR12;
		break;
	case 7:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_LBC_2_5X;
		break;
	case 8:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21;
		break;
	case 9:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YVU420;
		break;
	case 10:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_LBC_2_0X;
		break;
	default:
		fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
		break;
	}
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;

#if RT_VI_SHOW_TIME_COST
	int64_t time2_start = get_cur_time();
#endif

	ret = vin_s_fmt_special(id, &fmt);
	RT_LOGI("vin s fmt, ret = %d, id = %d", ret, id);

#if RT_VI_SHOW_TIME_COST
	int64_t time2_end = get_cur_time();
	RT_LOGE("time of s fmt: %lld", (time2_end - time2_start));
#endif

	ret = vin_g_fmt_special(id, &fmt);
	RT_LOGI("vin g fmt, ret = %d, id = %d", ret, id);

	RT_LOGD("resolution got from sensor = %d*%d num_planes = %d\n",
		fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
		fmt.fmt.pix_mp.num_planes);

	/* queue buffer */
	vi_comp->vipp_id = id;
	g_vi_comp[id]    = vi_comp;

	vin_register_buffer_done_callback(id, vin_buffer_callback);

#if RT_VI_SHOW_TIME_COST
	int64_t time_start = get_cur_time();
#endif

#if RT_VI_SHOW_TIME_COST
	int64_t time_end = get_cur_time();
	RT_LOGE("time of alloc buf: %lld", (time_end - time_start));
#endif

/* stream on */
#if RT_VI_SHOW_TIME_COST
	int64_t time1_start = get_cur_time();
#endif

	time_vi_start = get_cur_time();

#if RT_VI_SHOW_TIME_COST
	int64_t time1_end = get_cur_time();

	RT_LOGE("time of stream on: %lld", (time1_end - time1_start));
#endif

	return 0;
}

static int reset_high_fps(vi_comp_ctx *vi_comp)
{
	vin_streamoff_special(vi_comp->vipp_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	RT_LOGW("vin streamoff\n");

	highframe_free_vin_buf(vi_comp);

	vipp_init(vi_comp);

	return 0;
}

static int request_yuv_frame(struct vi_comp_ctx *vi_comp, unsigned char **pp_phy_addr)
{
	unsigned long flags;
	int ret			   = -1;
	struct vin_buffer *vin_buf = NULL;
	int data_size		   = vi_comp->align_width * vi_comp->base_config.height * 3 / 2;
	int loop_cnt		   = 0;
	int max_cnt		   = 200; /* 2s */

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X)
		data_size = vi_comp->buf_size;

	RT_LOGI("pp_phy_addr = %p, vi_comp = %p", pp_phy_addr, vi_comp);

	while (ret != 0) {
		ret = vin_dqbuffer_special(vi_comp->vipp_id, &vin_buf);

		if (ret != 0) {
			spin_lock_irqsave(&vi_comp->vi_spin_lock, flags);
			vi_comp->wait_in_buf_condition = 0;
			spin_unlock_irqrestore(&vi_comp->vi_spin_lock, flags);
			/* timeout is 10 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
			wait_event_timeout(vi_comp->wait_in_buf, vi_comp->wait_in_buf_condition, 1);
		}
		loop_cnt++;
		if (loop_cnt > max_cnt) {
			RT_LOGE("loop timeout: loop_cnt = %d\n", loop_cnt);
			break;
		}
	}

	RT_LOGI("ret = %d, loop_cnt = %d, data_size = %d\n", ret, loop_cnt, data_size);

	if (ret != 0) {
		RT_LOGE(" dequeue buf failed\n");
		return ret;
	}

	*pp_phy_addr = (unsigned char *)vin_buf->paddr;

	return data_size;
}

static int return_yuv_frame(struct vi_comp_ctx *vi_comp, unsigned char *phy_addr)
{
	int i = 0;

	for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
		if (vi_comp->vin_buf[i].paddr == phy_addr) {
			vin_qbuffer_special(vi_comp->vipp_id, &vi_comp->vin_buf[i]);
			break;
		}
	}

	if (i >= VI_OUT_BUFFER_LIST_NODE_NUM) {
		RT_LOGE("can not match phy_addr: %p", phy_addr);
		return -1;
	}

	return 0;
}

static int get_yuv_frame(struct vi_comp_ctx *vi_comp, void *user_buf)
{
	unsigned long flags;
	int ret			   = -1;
	unsigned char *virAddr     = NULL;
	struct vin_buffer *vin_buf = NULL;
	int data_size		   = vi_comp->align_width * vi_comp->base_config.height * 3 / 2;
	int loop_cnt		   = 0;
	int max_cnt		   = 100;

	if (vi_comp->base_config.pixelformat == RT_PIXEL_LBC_25X || vi_comp->base_config.pixelformat == RT_PIXEL_LBC_2X)
		data_size = vi_comp->buf_size;

	RT_LOGI("user_buf = %p, vi_comp = %p", user_buf, vi_comp);

	while (ret != 0) {
		ret = vin_dqbuffer_special(vi_comp->vipp_id, &vin_buf);

		if (ret != 0) {
			spin_lock_irqsave(&vi_comp->vi_spin_lock, flags);
			vi_comp->wait_in_buf_condition = 0;
			spin_unlock_irqrestore(&vi_comp->vi_spin_lock, flags);
			/* timeout is 10 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
			wait_event_timeout(vi_comp->wait_in_buf, vi_comp->wait_in_buf_condition, 1);
		}
		loop_cnt++;
		if (loop_cnt > max_cnt) {
			RT_LOGE("loop timeout: loop_cnt = %d\n", loop_cnt);
			break;
		}
	}

	RT_LOGI("ret = %d, loop_cnt = %d, data_size = %d\n", ret, loop_cnt, data_size);

	if (ret != 0) {
		RT_LOGE(" dequeue buf failed\n");
		return ret;
	}
	virAddr = (unsigned char *)cdc_mem_get_vir(vi_comp->memops, (unsigned long)vin_buf->paddr);

	cdc_mem_flush_cache(vi_comp->memops, virAddr, data_size);

	if (copy_to_user((void *)user_buf, virAddr, data_size)) {
		RT_LOGE(" copy_to_user fail, usr_buf = %p, virAddr = %p, size = %d\n",
			user_buf, virAddr, data_size);
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
		return -1;
	}

	vin_qbuffer_special(vi_comp->vipp_id, vin_buf);
	return data_size;
}

static int commond_process_vi(struct vi_comp_ctx *vi_comp, message_t *msg)
{
	int ret = 0;
	int cmd_error		      = 0;
	int cmd			      = msg->command;
	wait_queue_head_t *wait_reply = (wait_queue_head_t *)msg->wait_queue;
	unsigned int *wait_condition  = (unsigned int *)msg->wait_condition;

	RT_LOGD("cmd process: cmd = %d, state = %d, wait_reply = %p, wait_condition = %p",
		cmd, vi_comp->state, wait_reply, wait_condition);

	if (cmd == COMP_COMMAND_INIT) {
		if (vi_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vipp_create(vi_comp);
			vipp_init(vi_comp);
			vi_comp->state = COMP_STATE_INITIALIZED;
		}

	} else if (cmd == COMP_COMMAND_START) {
		if (vi_comp->state != COMP_STATE_INITIALIZED && vi_comp->state != COMP_STATE_PAUSE) {
			cmd_error = 1;
		} else {
			if (vi_comp->state == COMP_STATE_INITIALIZED) {
#if RT_VI_SHOW_TIME_COST
				int64_t time_start = get_cur_time();
#endif
				if (vi_comp->base_config.enable_high_fps_transfor == 1) {
					highframe_vipp_init(vi_comp);
				} else {
					alloc_vin_buf(vi_comp);
				    ret = vin_streamon_special(vi_comp->base_config.channel_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
					RT_LOGW("vin streamon, ret = %d, id = %d", ret, vi_comp->base_config.channel_id);
				}

#if RT_VI_SHOW_TIME_COST
				int64_t time_end = get_cur_time();
				RT_LOGW("time of vipp_init: %lld time: %lld", (time_end - time_start), time_end);
#endif
			}

			vi_comp->state = COMP_STATE_EXECUTING;
		}

	} else if (cmd == COMP_COMMAND_PAUSE) {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			cmd_error = 1;
		} else {
			vi_comp->state = COMP_STATE_PAUSE;
		}
	} else if (cmd == COMP_COMMAND_STOP) {
		vin_streamoff_special(vi_comp->vipp_id, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
		vi_comp->state = COMP_STATE_IDLE;
	} else if (cmd == COMP_COMMAND_RESET_HIGH_FPS) {
		RT_LOGW("*** process COMP_COMMAND_RESET_HIGH_FPS ***");
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_RESET_ISP_HIGH_FPS,
			0,
			0,
			NULL);
	}

	if (cmd_error == 1) {
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_ERROR,
			COMP_COMMAND_INIT,
			0,
			NULL);
	} else {
		vi_comp->callback->EventHandler(
			vi_comp->self_comp,
			vi_comp->callback_data,
			COMP_EVENT_CMD_COMPLETE,
			cmd,
			0,
			NULL);
	}

	if (wait_reply) {
		RT_LOGD("wait up: cmd = %d", cmd);
		*wait_condition = 1;
		wake_up(wait_reply);
	}
	return 0;
}

static error_type config_dynamic_param(
	vi_comp_ctx *vi_comp,
	comp_index_type index,
	void *param_data)
{
	error_type error = ERROR_TYPE_OK;

	RT_LOGW("not support config dynamic param yet");

	return error;
}

static int post_msg_and_wait(vi_comp_ctx *vi_comp, int cmd_id, int msg_param)
{
	message_t cmd_msg;
	memset(&cmd_msg, 0, sizeof(message_t));
	cmd_msg.command	= cmd_id;
	cmd_msg.para0	  = msg_param;
	cmd_msg.wait_queue     = (char *)&vi_comp->wait_reply[cmd_id];
	cmd_msg.wait_condition = (char *)&vi_comp->wait_reply_condition[cmd_id];

	vi_comp->wait_reply_condition[cmd_id] = 0;

	put_message(&vi_comp->msg_queue, &cmd_msg);

	RT_LOGD("wait for : cmd[%d], start", cmd_id);
	wait_event(vi_comp->wait_reply[cmd_id],
		   vi_comp->wait_reply_condition[cmd_id]);
	RT_LOGD("wait for : cmd[%d], finish", cmd_id);

	return 0;
}

error_type vi_comp_init(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_init: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(vi_comp, COMP_COMMAND_INIT, 0);
	return error;
}

error_type vi_comp_start(
	PARAM_IN comp_handle component)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(vi_comp, COMP_COMMAND_START, 0);
	return error;
}

error_type vi_comp_pause(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(vi_comp, COMP_COMMAND_PAUSE, 0);
	return error;
}

error_type vi_comp_stop(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_stop: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(vi_comp, COMP_COMMAND_STOP, 0);
	return error;
}

error_type vi_comp_destroy(PARAM_IN comp_handle component)
{
	int i				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;
	video_frame_node *frame_node    = NULL;
	int free_frame_cout		= 0;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_destroy: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	vin_close_special(vi_comp->vipp_id);
	/*should make sure the thread do nothing importent */
	if (vi_comp->vi_thread)
		kthread_stop(vi_comp->vi_thread);

	message_destroy(&vi_comp->msg_queue);

	while ((!list_empty(&vi_comp->out_buf_manager.empty_frame_list))) {
		frame_node = list_first_entry(&vi_comp->out_buf_manager.empty_frame_list, video_frame_node, mList);
		if (frame_node) {
			free_frame_cout++;
			list_del(&frame_node->mList);
			kfree(frame_node);
		}
	}

	while ((!list_empty(&vi_comp->out_buf_manager.valid_frame_list))) {
		frame_node = list_first_entry(&vi_comp->out_buf_manager.valid_frame_list, video_frame_node, mList);
		if (frame_node) {
			free_frame_cout++;
			list_del(&frame_node->mList);
			kfree(frame_node);
		}
	}

	RT_LOGD("free_frame_cout = %d", free_frame_cout);

	if (free_frame_cout != VI_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of frame node is not match: %d, %d", free_frame_cout, VI_OUT_BUFFER_LIST_NODE_NUM);

	if (vi_comp->config_yuv_buf_flag == 0) {
		for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
			if (vi_comp->vin_buf[i].paddr) {
				unsigned char *virAddr = (unsigned char *)cdc_mem_get_vir(vi_comp->memops,
											  (unsigned long)vi_comp->vin_buf[i].paddr);
				cdc_mem_pfree(vi_comp->memops, virAddr);
			}
		}
	}

	if (vi_comp->memops)
		mem_destory(MEM_TYPE_ION, vi_comp->memops);

	kfree(vi_comp);

	return error;
}

error_type vi_comp_get_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_INOUT void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_get_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	return error;
}

error_type vi_comp_set_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_IN void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_set_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VI_CONFIG_Base: {
		vi_comp_base_config *base_config = (vi_comp_base_config *)param_data;
		memcpy(&vi_comp->base_config, base_config, sizeof(vi_comp_base_config));
		vi_comp->align_width  = RT_ALIGN(vi_comp->base_config.width, 16);
		vi_comp->align_height = RT_ALIGN(vi_comp->base_config.height, 16);
		vi_comp->max_fps      = base_config->frame_rate;

		if (vi_comp->max_fps <= 0)
			vi_comp->max_fps = 15;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Normal: {
		vi_comp_normal_config *normal_config = (vi_comp_normal_config *)param_data;
		memcpy(&vi_comp->normal_config, normal_config, sizeof(vi_comp_normal_config));
		break;
	}
	case COMP_INDEX_VI_CONFIG_SET_YUV_BUF_INFO: {
		memcpy(&vi_comp->yuv_buf_info, (config_yuv_buf_info *)param_data, sizeof(config_yuv_buf_info));
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_YUV_FRAME: {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when get yuv frame: %d", vi_comp->state);
			break;
		}

		post_msg_and_wait(vi_comp, COMP_COMMAND_PAUSE, 0);

		error = get_yuv_frame(vi_comp, (void *)param_data);
		RT_LOGI("get yuv frame, ret = %d\n", error);

		post_msg_and_wait(vi_comp, COMP_COMMAND_START, 0);

		if (error < 0)
			error = ERROR_TYPE_ERROR;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_REQUEST_YUV_FRAME: {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when request yuv frame: %d", vi_comp->state);
			break;
		}

		post_msg_and_wait(vi_comp, COMP_COMMAND_PAUSE, 0);

		error = request_yuv_frame(vi_comp, (unsigned char **)param_data);
		RT_LOGI("request yuv frame, ret = %d, phy_addr = %p\n", error, *(unsigned char **)param_data);

		post_msg_and_wait(vi_comp, COMP_COMMAND_START, 0);

		if (error < 0)
			error = ERROR_TYPE_ERROR;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_RETURN_YUV_FRAME: {
		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when request yuv frame: %d", vi_comp->state);
			break;
		}

		error = return_yuv_frame(vi_comp, (unsigned char *)param_data);
		RT_LOGI("return yuv frame, ret = %d, phy_addr = %p\n", error, param_data);

		if (error < 0)
			error = ERROR_TYPE_ERROR;

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_LV: {
		int lv = 0;

		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when get lv: %d", vi_comp->state);
			break;
		}

		//lv = vin_isp_get_lv_special(vi_comp->vipp_id, 0);
		RT_LOGI("get lv = %d\n", lv);

		return lv;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_IR_PARAM: {
		RTIrParam *ir_param = (RTIrParam *)param_data;
		int mode_flag       = 0;

		if (vi_comp->state != COMP_STATE_EXECUTING) {
			RT_LOGW("the state is not executing when get lv: %d", vi_comp->state);
			break;
		}

		/* ir_mode of user -- 0 :color, 1: grey */
		if (ir_param->grey == 1)
			mode_flag = 2;
		else
			mode_flag = 0;

		RT_LOGI("set ir param: mode_flag = %d, ir_on = %d, ir_flash_on = %d",
			mode_flag, ir_param->ir_on, ir_param->ir_flash_on);

		/* ir_mode of isp --  2: grey, other: color */
		//vin_server_reset_special(vi_comp->vipp_id, mode_flag, ir_param->ir_on, ir_param->ir_flash_on);

		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_H_FLIP: {
		int bhflip	   = (int)param_data;
		unsigned int ctrl_id = V4L2_CID_HFLIP;
		int val		     = bhflip;

		RT_LOGI("set h flip: %d", bhflip);

		//vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, val);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_V_FLIP: {
		int bvflip	   = (int)param_data;
		unsigned int ctrl_id = V4L2_CID_VFLIP;
		int val		     = bvflip;

		RT_LOGI("set v flip: %d", bvflip);

		//vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, val);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG: {
		vi_comp->catch_jpeg_flag = (int)param_data;
		RT_LOGI("vi_comp->catch_jpeg_flag = %d", vi_comp->catch_jpeg_flag);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_POWER_LINE_FREQ: {
		enum RT_POWER_LINE_FREQUENCY ePower_line = (enum RT_POWER_LINE_FREQUENCY)param_data;
		unsigned int ctrl_id			 = V4L2_CID_POWER_LINE_FREQUENCY;

		RT_LOGI("ePower_line_freq = %d", ePower_line);

		//vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ePower_line);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_METERING_MODE: {
		enum RT_AE_METERING_MODE ae_metering_mode = (enum RT_AE_METERING_MODE)param_data;
		unsigned int ctrl_id			  = V4L2_CID_EXPOSURE_METERING;

		RT_LOGI("ae_metering_mode = %d", ae_metering_mode);

		//vin_s_ctrl_special(vi_comp->vipp_id, ctrl_id, ae_metering_mode);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_EXP_GAIN: {
		RTIspExpGain *exp_gain = (RTIspExpGain *)param_data;

		//vin_isp_get_exp_gain_special(vi_comp->vipp_id, (struct sensor_exp_gain *)exp_gain);

		RT_LOGI("get exp&gain = %d/%d/%d/%d", exp_gain->exp_val, exp_gain->gain_val,
			exp_gain->r_gain, exp_gain->b_gain);
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_GET_HIST: {
		unsigned int *hist = (unsigned int *)param_data;

		//vin_isp_get_hist_special(vi_comp->vipp_id, hist);

		RT_LOGI("get hist");
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG: {
		RTIspCtrlAttr *ctrlattr = (RTIspCtrlAttr *)param_data;

		//vin_isp_set_attr_cfg(vi_comp->vipp_id, ctrlattr->isp_ctrl_id, ctrlattr->value);

		RT_LOGD("set isp attr cfg");
		break;
	}
	case COMP_INDEX_VI_CONFIG_SET_RESET_HIGH_FPS: {
		if (vi_comp->state != COMP_STATE_PAUSE)
			RT_LOGE("the state is not pause when reset-high-fps");

		reset_high_fps(vi_comp);

		break;
	}
	case COMP_INDEX_VI_CONFIG_ENABLE_HIGH_FPS: {
		vi_comp->base_config.enable_high_fps_transfor = 1;
		break;
	}
	case COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS: {
		int fps      = (int)param_data;
		int diff_fps = vi_comp->max_fps - fps;

		if (diff_fps > 0 && diff_fps < vi_comp->max_fps) {
			vi_comp->reduce_fps.drop_frame_nums      = diff_fps;
			vi_comp->reduce_fps.drop_point_in_second = vi_comp->max_fps / vi_comp->reduce_fps.drop_frame_nums;
			vi_comp->reduce_fps.bEnable		 = 1;
		} else {
			vi_comp->reduce_fps.bEnable	 = 0;
			vi_comp->reduce_fps.drop_frame_nums = 0;
		}

		RT_LOGW("set fps = %d, max_fps = %d, bEnable_reduce_fps = %d, reduce_nums = %d",
			fps, vi_comp->max_fps, vi_comp->reduce_fps.bEnable, vi_comp->reduce_fps.drop_frame_nums);
		break;
	}
	default: {
		error = config_dynamic_param(vi_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type vi_comp_get_state(
	PARAM_IN comp_handle component,
	PARAM_OUT comp_state_type *pState)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_get_state: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	*pState = vi_comp->state;
	return error;
}

/* valid_list --> empty_list */
error_type vi_comp_fill_this_out_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;
	video_frame_s *src_stream       = NULL;
	video_frame_node *frame_node    = NULL;
	struct vin_buffer *vin_buf      = NULL;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	src_stream = (video_frame_s *)pBuffer->private;

	/* get the ready frame */
	mutex_lock(&vi_comp->out_buf_manager.mutex);

	if (list_empty(&vi_comp->out_buf_manager.valid_frame_list)) {
		RT_LOGE("error: valid_frame_list is null");
		mutex_unlock(&vi_comp->out_buf_manager.mutex);
		return ERROR_TYPE_ERROR;
	}
	frame_node = list_first_entry(&vi_comp->out_buf_manager.valid_frame_list, video_frame_node, mList);
	memcpy(&frame_node->video_frame, src_stream, sizeof(video_frame_s));

	list_move_tail(&frame_node->mList, &vi_comp->out_buf_manager.empty_frame_list);

	mutex_unlock(&vi_comp->out_buf_manager.mutex);

	vin_buf = src_stream->private;

	RT_LOGD("vin_buf = %p", vin_buf);

	if (vin_buf == NULL)
		RT_LOGE("error: vin_buf is null");
	else
		vin_qbuffer_special(vi_comp->vipp_id, vin_buf);

	return error;
}

error_type vi_comp_set_callbacks(
	PARAM_IN comp_handle component,
	PARAM_IN comp_callback_type *callback,
	PARAM_IN void *callback_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	vi_comp->callback      = callback;
	vi_comp->callback_data = callback_data;
	return error;
}

error_type vi_comp_setup_tunnel(
	PARAM_IN comp_handle component,
	PARAM_IN comp_port_type port_type,
	PARAM_IN comp_handle tunnel_comp,
	PARAM_IN int connect_flag)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = (struct vi_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || vi_comp == NULL || tunnel_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks, param error: %p, %p, %p",
			rt_component, vi_comp, tunnel_comp);
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto setup_tunnel_exit;
	}

	if (port_type == COMP_INPUT_PORT) {
		if (connect_flag == 1) {
			if (vi_comp->in_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("in port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				vi_comp->in_port_tunnel_info.valid_flag  = 1;
				vi_comp->in_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (vi_comp->in_port_tunnel_info.valid_flag == 1 && vi_comp->in_port_tunnel_info.tunnel_comp == tunnel_comp) {
				vi_comp->in_port_tunnel_info.valid_flag  = 0;
				vi_comp->in_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect in tunnel failed:  valid_flag = %d, tunnel_comp = %p, %p",
					vi_comp->in_port_tunnel_info.valid_flag,
					vi_comp->in_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	} else if (port_type == COMP_OUTPUT_PORT) {
		if (connect_flag == 1) {
			if (vi_comp->out_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("out port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				vi_comp->out_port_tunnel_info.valid_flag  = 1;
				vi_comp->out_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (vi_comp->out_port_tunnel_info.valid_flag == 1 && vi_comp->out_port_tunnel_info.tunnel_comp == tunnel_comp) {
				vi_comp->out_port_tunnel_info.valid_flag  = 0;
				vi_comp->out_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect out tunnel failed:  valid = %d, tunnel_comp = %p, %p",
					vi_comp->out_port_tunnel_info.valid_flag,
					vi_comp->out_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	}

setup_tunnel_exit:

	return error;
}

error_type vi_comp_component_init(PARAM_IN comp_handle component, int channel_id)
{
	int i				= 0;
	int ret				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct vi_comp_ctx *vi_comp     = NULL;
	struct sched_param param = {.sched_priority = 1 };

	vi_comp = kmalloc(sizeof(struct vi_comp_ctx), GFP_KERNEL);
	vi_comp->base_config.channel_id = channel_id;
	if (vi_comp == NULL) {
		RT_LOGE("kmalloc for vi_comp failed");
		error = ERROR_TYPE_NOMEM;
		goto EXIT;
	}
	memset(vi_comp, 0, sizeof(struct vi_comp_ctx));

	ret = message_create(&vi_comp->msg_queue);
	if (ret < 0) {
		RT_LOGE("message create failed");
		error = ERROR_TYPE_ERROR;
		goto EXIT;
	}

	rt_component->component_private    = vi_comp;
	rt_component->init		   = vi_comp_init;
	rt_component->start		   = vi_comp_start;
	rt_component->pause		   = vi_comp_pause;
	rt_component->stop		   = vi_comp_stop;
	rt_component->destroy		   = vi_comp_destroy;
	rt_component->get_config	   = vi_comp_get_config;
	rt_component->set_config	   = vi_comp_set_config;
	rt_component->get_state		   = vi_comp_get_state;
	rt_component->fill_this_out_buffer = vi_comp_fill_this_out_buffer;
	rt_component->set_callbacks	= vi_comp_set_callbacks;
	rt_component->setup_tunnel	 = vi_comp_setup_tunnel;

	vi_comp->self_comp = rt_component;
	vi_comp->state     = COMP_STATE_IDLE;

	init_waitqueue_head(&vi_comp->wait_in_buf);
	for (i = 0; i < WAIT_REPLY_NUM; i++) {
		init_waitqueue_head(&vi_comp->wait_reply[i]);
	}

	/* init out buf manager*/
	mutex_init(&vi_comp->out_buf_manager.mutex);
	vi_comp->out_buf_manager.empty_num = VI_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&vi_comp->out_buf_manager.empty_frame_list);
	INIT_LIST_HEAD(&vi_comp->out_buf_manager.valid_frame_list);
	for (i = 0; i < VI_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &vi_comp->out_buf_manager.empty_frame_list);
	}

	spin_lock_init(&vi_comp->vi_spin_lock);

	vi_comp->vi_thread = kthread_create(thread_process_vi,
					    vi_comp, "vi comp thread");
	sched_setscheduler(vi_comp->vi_thread, SCHED_FIFO, &param);
	wake_up_process(vi_comp->vi_thread);

EXIT:

	return error;
}

static int thread_process_vi(void *param)
{
	struct vi_comp_ctx *vi_comp = (struct vi_comp_ctx *)param;
	message_t cmd_msg;
	unsigned long flags;

	RT_LOGD("thread_process_vi start !");
	while (1) {
		if (kthread_should_stop())
			break;

		if (get_message(&vi_comp->msg_queue, &cmd_msg) == 0) {
			/* todo */
			commond_process_vi(vi_comp, &cmd_msg);
			/* continue process message */
			continue;
		}

		if (vi_comp->state == COMP_STATE_EXECUTING) {
			/* get out buffer */
#if 1
			static int cnt_1 = 1;
			if (cnt_1) {
			    cnt_1 = 0;
			    RT_LOGW("channel %d vin comp first time EXECUTING state, time: %lld",
					vi_comp->base_config.channel_id, get_cur_time());
			}
#endif
			if (dequeue_buffer(vi_comp) != 0) {
				spin_lock_irqsave(&vi_comp->vi_spin_lock, flags);
				vi_comp->wait_in_buf_condition = 0;
				spin_unlock_irqrestore(&vi_comp->vi_spin_lock, flags);
				/* timeout is 10 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
				wait_event_timeout(vi_comp->wait_in_buf, vi_comp->wait_in_buf_condition, 1);
				continue;
			}
		} else {
			TMessage_WaitQueueNotEmpty(&vi_comp->msg_queue, 10);
		}
	}

	RT_LOGD("thread_process_vi finish !");
	return 0;
}
