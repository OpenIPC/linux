
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/g2d_driver.h>
#include <linux/fs.h>
#include "vin_video_api.h"
#define LOG_TAG "rt_venc_comp"

#include "rt_common.h"
#include "rt_venc_component.h"
#include "rt_message.h"
#include "vencoder.h"
#include "../_uapi_rt_media.h"

#define TEST_ENCODER_BY_SELF (0)

#define ENABLE_SAVE_NATIVE_OVERLAY_DATA (0)

DEFINE_MUTEX(venc_mutex);

extern int g2d_blit_h_new(g2d_blt_h *para);
extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
extern void g2d_ioctl_mutex_lock(void);
extern void g2d_ioctl_mutex_unlock(void);

typedef struct catch_jpeg_cxt {
	int enable;
	int encoder_finish_flag;
	VideoEncoder *vencoder;
	int width;
	int height;
	int qp;
	wait_queue_head_t wait_enc_finish;
	unsigned int wait_enc_finish_condition;
} catch_jpeg_cxt;

typedef struct venc_comp_ctx {
	int vencoder_init_flag;
	VideoEncoder *vencoder;
	comp_state_type state;
	struct task_struct *venc_thread;
	message_queue_t msg_queue;
	comp_callback_type *callback;
	void *callback_data;
	rt_component_type *self_comp;
	wait_queue_head_t wait_reply[WAIT_REPLY_NUM];
	unsigned int wait_reply_condition[WAIT_REPLY_NUM];
	venc_inbuf_manager in_buf_manager;
	venc_outbuf_manager out_buf_manager;
	comp_tunnel_info in_port_tunnel_info;
	comp_tunnel_info out_port_tunnel_info;
	venc_comp_base_config base_config;
	venc_comp_normal_config normal_config;
	unsigned char *vbvStartAddr;
	catch_jpeg_cxt jpeg_cxt;
	int is_first_frame;
} venc_comp_ctx;

#define GLOBAL_OSD_MAX_NUM (3)

typedef struct native_osd_info {
	VencOverlayInfoS sOverlayInfo;

	unsigned int ori_frame_w;
	unsigned int ori_frame_h;
	struct mem_interface *memops;

	/* the g2d_in_buf will not release at all, so it can setup to vencoder immediately when start */
	unsigned char *g2d_in_ion_vir_addr[GLOBAL_OSD_MAX_NUM];
	unsigned int g2d_in_buf_size[GLOBAL_OSD_MAX_NUM];
} native_osd_info;

static native_osd_info *global_osd_info;

static int thread_process_venc(void *param);
static int adjust_native_overlay_info(venc_comp_ctx *venc_comp);

static int config_VencOutputBuffer_by_videostream(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
#if (COPY_STREAM_DATA_FROM_KERNEL == 0)
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}
#endif

	output_buffer->nID    = video_stream->id;
	output_buffer->nPts   = video_stream->pts;
	output_buffer->nFlag  = video_stream->flag;
	output_buffer->nSize0 = video_stream->size0;
	output_buffer->nSize1 = video_stream->size1;
	output_buffer->nSize2 = video_stream->size2;
	output_buffer->pData0 = venc_comp->vbvStartAddr + video_stream->offset0;
	output_buffer->pData1 = venc_comp->vbvStartAddr + video_stream->offset1;
	output_buffer->pData2 = venc_comp->vbvStartAddr + video_stream->offset2;

	output_buffer->pData0 = video_stream->data0;
	output_buffer->pData1 = video_stream->data1;
	output_buffer->pData2 = video_stream->data2;
	RT_LOGD("request stream: pData0 =%p, %p, id = %d",
		video_stream->data0, output_buffer->pData0, output_buffer->nID);

	return 0;
}

static int config_videostream_by_VencOutputBuffer(venc_comp_ctx *venc_comp,
						  VencOutputBuffer *output_buffer,
						  video_stream_s *video_stream)
{
#if (COPY_STREAM_DATA_FROM_KERNEL == 0)
	if (venc_comp->vbvStartAddr == NULL) {
		RT_LOGE("venc_comp->vbvStartAddr is null");
		return -1;
	}
#endif

	video_stream->id      = output_buffer->nID;
	video_stream->pts     = output_buffer->nPts;
	video_stream->flag    = output_buffer->nFlag;
	video_stream->size0   = output_buffer->nSize0;
	video_stream->size1   = output_buffer->nSize1;
	video_stream->size2   = output_buffer->nSize2;
	video_stream->offset0 = output_buffer->pData0 - venc_comp->vbvStartAddr;

	if (output_buffer->pData1)
		video_stream->offset1 = output_buffer->pData1 - venc_comp->vbvStartAddr;
	if (output_buffer->pData2)
		video_stream->offset2 = output_buffer->pData2 - venc_comp->vbvStartAddr;

	video_stream->data0 = output_buffer->pData0;
	video_stream->data1 = output_buffer->pData1;
	video_stream->data2 = output_buffer->pData2;

	RT_LOGD("reqeust stream: pData0 =%p, %p, id = %d",
		video_stream->data0, output_buffer->pData0,
		output_buffer->nID);

	if (video_stream->flag & VENC_BUFFERFLAG_KEYFRAME)
		video_stream->keyframe_flag = 1;
	else
		video_stream->keyframe_flag = 0;

	return 0;
}

static int init_default_encpp_param(sEncppSharpParamDynamic *pSharpParamDynamic,
	sEncppSharpParamStatic *pSharpParamStatic, s3DfilterParam *p3DfilterParam, s2DfilterParam *p2DfilterParam)
{
    p3DfilterParam->enable_3d_filter = 1;
    p3DfilterParam->adjust_pix_level_enable = 0;
    p3DfilterParam->smooth_filter_enable = 1;
    p3DfilterParam->max_pix_diff_th = 6;
    p3DfilterParam->max_mad_th = 8;
    p3DfilterParam->max_mv_th = 8;
    p3DfilterParam->min_coef = 15;
    p3DfilterParam->max_coef = 16;

    p2DfilterParam->enable_2d_filter = 1;
    p2DfilterParam->filter_strength_uv = 46;
    p2DfilterParam->filter_strength_y = 46;
    p2DfilterParam->filter_th_uv = 3;
    p2DfilterParam->filter_th_y = 2;

    pSharpParamDynamic->ss_ns_lw  = 0;           //[0,255];
    pSharpParamDynamic->ss_ns_hi  = 0;           //[0,255];
    pSharpParamDynamic->ls_ns_lw  = 0;           //[0,255];
    pSharpParamDynamic->ls_ns_hi  = 0;           //[0,255];
    pSharpParamDynamic->ss_lw_cor = 180;           //[0,255];
    pSharpParamDynamic->ss_hi_cor = 255;           //[0,255];
    pSharpParamDynamic->ls_lw_cor = 120;           //[0,255];
    pSharpParamDynamic->ls_hi_cor = 255;           //[0,255];
    pSharpParamDynamic->ss_blk_stren = 552;      //[0,4095];
    pSharpParamDynamic->ss_wht_stren = 423;      //[0,4095];
    pSharpParamDynamic->ls_blk_stren = 1408;      //[0,4095];
    pSharpParamDynamic->ls_wht_stren = 952;      //[0,4095];
    pSharpParamDynamic->ss_avg_smth  = 0;         //[0,255];
    pSharpParamDynamic->ss_dir_smth  = 0;       //[0,16];
    pSharpParamDynamic->dir_smth[0] = 0;         //[0,16];
    pSharpParamDynamic->dir_smth[1] = 0;         //[0,16];
    pSharpParamDynamic->dir_smth[2] = 0;         //[0,16];
    pSharpParamDynamic->dir_smth[3] = 0;         //[0,16];
    pSharpParamDynamic->hfr_smth_ratio  = 0;      //[0,32];
    pSharpParamDynamic->hfr_hf_wht_stren = 0;    //[0,4095];
    pSharpParamDynamic->hfr_hf_blk_stren  = 0;    //[0,4095];
    pSharpParamDynamic->hfr_mf_wht_stren = 0;    //[0,4095];
    pSharpParamDynamic->hfr_mf_blk_stren  = 0;    //[0,4095];
    pSharpParamDynamic->hfr_hf_cor_ratio  = 0;    //[0,255];
    pSharpParamDynamic->hfr_mf_cor_ratio = 0;    //[0,255];
    pSharpParamDynamic->hfr_hf_mix_ratio = 390;  //[0,1023];
    pSharpParamDynamic->hfr_mf_mix_ratio  = 390;  //[0,1023];
    pSharpParamDynamic->hfr_hf_mix_min_ratio = 0;   //[0,255];
    pSharpParamDynamic->hfr_mf_mix_min_ratio = 0;   //[0,255];
    pSharpParamDynamic->hfr_hf_wht_clp  = 32;     //[0,255];
    pSharpParamDynamic->hfr_hf_blk_clp  = 32;     //[0,255];
    pSharpParamDynamic->hfr_mf_wht_clp  = 32;     //[0,255];
    pSharpParamDynamic->hfr_mf_blk_clp  = 32;     //[0,255];
    pSharpParamDynamic->wht_clp_para = 256;       //[0,1023];
    pSharpParamDynamic->blk_clp_para = 256;       //[0,1023];
    pSharpParamDynamic->wht_clp_slp  = 16;        //[0,63];
    pSharpParamDynamic->blk_clp_slp  = 8;         //[0,63];
    pSharpParamDynamic->max_clp_ratio  = 64;        //[0,255];

    pSharpParamDynamic->sharp_edge_lum[0] = 0;
    pSharpParamDynamic->sharp_edge_lum[1] = 14;
    pSharpParamDynamic->sharp_edge_lum[2] = 25;
    pSharpParamDynamic->sharp_edge_lum[3] = 32;
    pSharpParamDynamic->sharp_edge_lum[4] = 39;
    pSharpParamDynamic->sharp_edge_lum[5] = 51;
    pSharpParamDynamic->sharp_edge_lum[6] = 65;
    pSharpParamDynamic->sharp_edge_lum[7] = 80;
    pSharpParamDynamic->sharp_edge_lum[8] = 99;
    pSharpParamDynamic->sharp_edge_lum[9] = 124;
    pSharpParamDynamic->sharp_edge_lum[10] = 152;
    pSharpParamDynamic->sharp_edge_lum[11] = 179;
    pSharpParamDynamic->sharp_edge_lum[12] = 203;
    pSharpParamDynamic->sharp_edge_lum[13] = 224;
    pSharpParamDynamic->sharp_edge_lum[14] = 237;
    pSharpParamDynamic->sharp_edge_lum[15] = 245;
    pSharpParamDynamic->sharp_edge_lum[16] = 271;
    pSharpParamDynamic->sharp_edge_lum[17] = 336;
    pSharpParamDynamic->sharp_edge_lum[18] = 428;
    pSharpParamDynamic->sharp_edge_lum[19] = 526;
    pSharpParamDynamic->sharp_edge_lum[20] = 623;
    pSharpParamDynamic->sharp_edge_lum[21] = 709;
    pSharpParamDynamic->sharp_edge_lum[22] = 767;
    pSharpParamDynamic->sharp_edge_lum[23] = 782;
    pSharpParamDynamic->sharp_edge_lum[24] = 772;
    pSharpParamDynamic->sharp_edge_lum[25] = 761;
    pSharpParamDynamic->sharp_edge_lum[26] = 783;
    pSharpParamDynamic->sharp_edge_lum[27] = 858;
    pSharpParamDynamic->sharp_edge_lum[28] = 941;
    pSharpParamDynamic->sharp_edge_lum[29] = 988;
    pSharpParamDynamic->sharp_edge_lum[30] = 1006;
    pSharpParamDynamic->sharp_edge_lum[31] = 1016;
    pSharpParamDynamic->sharp_edge_lum[32] = 1023;

    pSharpParamStatic->ss_shp_ratio = 0;         //[0,255];
    pSharpParamStatic->ls_shp_ratio = 0;         //[0,255];
    pSharpParamStatic->ss_dir_ratio = 64;         //[0,1023];
    pSharpParamStatic->ls_dir_ratio = 255;         //[0,1023];
    pSharpParamStatic->ss_crc_stren = 0;        //[0,1023];
    pSharpParamStatic->ss_crc_min = 16;        //[0,255];
    pSharpParamStatic->wht_sat_ratio = 64;
    pSharpParamStatic->blk_sat_ratio = 64;
    pSharpParamStatic->wht_slp_bt = 1;
    pSharpParamStatic->blk_slp_bt = 1;

    pSharpParamStatic->sharp_ss_value[0] = 262;
    pSharpParamStatic->sharp_ss_value[1] = 375;
    pSharpParamStatic->sharp_ss_value[2] = 431;
    pSharpParamStatic->sharp_ss_value[3] = 395;
    pSharpParamStatic->sharp_ss_value[4] = 316;
    pSharpParamStatic->sharp_ss_value[5] = 250;
    pSharpParamStatic->sharp_ss_value[6] = 206;
    pSharpParamStatic->sharp_ss_value[7] = 179;
    pSharpParamStatic->sharp_ss_value[8] = 158;
    pSharpParamStatic->sharp_ss_value[9] = 132;
    pSharpParamStatic->sharp_ss_value[10] = 107;
    pSharpParamStatic->sharp_ss_value[11] = 91;
    pSharpParamStatic->sharp_ss_value[12] = 79;
    pSharpParamStatic->sharp_ss_value[13] = 67;
    pSharpParamStatic->sharp_ss_value[14] = 56;
    pSharpParamStatic->sharp_ss_value[15] = 50;
    pSharpParamStatic->sharp_ss_value[16] = 39;
    pSharpParamStatic->sharp_ss_value[17] = 19;
    pSharpParamStatic->sharp_ss_value[18] = 0;
    pSharpParamStatic->sharp_ss_value[19] = 0;
    pSharpParamStatic->sharp_ss_value[20] = 0;
    pSharpParamStatic->sharp_ss_value[21] = 1;
    pSharpParamStatic->sharp_ss_value[22] = 0;
    pSharpParamStatic->sharp_ss_value[23] = 0;
    pSharpParamStatic->sharp_ss_value[24] = 0;
    pSharpParamStatic->sharp_ss_value[25] = 0;
    pSharpParamStatic->sharp_ss_value[26] = 0;
    pSharpParamStatic->sharp_ss_value[27] = 0;
    pSharpParamStatic->sharp_ss_value[28] = 0;
    pSharpParamStatic->sharp_ss_value[29] = 0;
    pSharpParamStatic->sharp_ss_value[30] = 0;
    pSharpParamStatic->sharp_ss_value[31] = 0;
    pSharpParamStatic->sharp_ss_value[32] = 0;

    pSharpParamStatic->sharp_ls_value[0] = 341;
    pSharpParamStatic->sharp_ls_value[1] = 489;
    pSharpParamStatic->sharp_ls_value[2] = 564;
    pSharpParamStatic->sharp_ls_value[3] = 515;
    pSharpParamStatic->sharp_ls_value[4] = 395;
    pSharpParamStatic->sharp_ls_value[5] = 254;
    pSharpParamStatic->sharp_ls_value[6] = 90;
    pSharpParamStatic->sharp_ls_value[7] = 56;
    pSharpParamStatic->sharp_ls_value[8] = 54;
    pSharpParamStatic->sharp_ls_value[9] = 52;
    pSharpParamStatic->sharp_ls_value[10] = 51;
    pSharpParamStatic->sharp_ls_value[11] = 48;
    pSharpParamStatic->sharp_ls_value[12] = 42;
    pSharpParamStatic->sharp_ls_value[13] = 36;
    pSharpParamStatic->sharp_ls_value[14] = 31;
    pSharpParamStatic->sharp_ls_value[15] = 28;
    pSharpParamStatic->sharp_ls_value[16] = 23;
    pSharpParamStatic->sharp_ls_value[17] = 11;
    pSharpParamStatic->sharp_ls_value[18] = 0;
    pSharpParamStatic->sharp_ls_value[19] = 0;
    pSharpParamStatic->sharp_ls_value[20] = 0;
    pSharpParamStatic->sharp_ls_value[21] = 1;
    pSharpParamStatic->sharp_ls_value[22] = 0;
    pSharpParamStatic->sharp_ls_value[23] = 0;
    pSharpParamStatic->sharp_ls_value[24] = 0;
    pSharpParamStatic->sharp_ls_value[25] = 0;
    pSharpParamStatic->sharp_ls_value[26] = 0;
    pSharpParamStatic->sharp_ls_value[27] = 0;
    pSharpParamStatic->sharp_ls_value[28] = 0;
    pSharpParamStatic->sharp_ls_value[29] = 0;
    pSharpParamStatic->sharp_ls_value[30] = 0;
    pSharpParamStatic->sharp_ls_value[31] = 0;
    pSharpParamStatic->sharp_ls_value[32] = 0;

    pSharpParamStatic->sharp_hsv[0] = 287;
    pSharpParamStatic->sharp_hsv[1] = 286;
    pSharpParamStatic->sharp_hsv[2] = 283;
    pSharpParamStatic->sharp_hsv[3] = 276;
    pSharpParamStatic->sharp_hsv[4] = 265;
    pSharpParamStatic->sharp_hsv[5] = 256;
    pSharpParamStatic->sharp_hsv[6] = 256;
    pSharpParamStatic->sharp_hsv[7] = 269;
    pSharpParamStatic->sharp_hsv[8] = 290;
    pSharpParamStatic->sharp_hsv[9] = 314;
    pSharpParamStatic->sharp_hsv[10] = 336;
    pSharpParamStatic->sharp_hsv[11] = 361;
    pSharpParamStatic->sharp_hsv[12] = 397;
    pSharpParamStatic->sharp_hsv[13] = 446;
    pSharpParamStatic->sharp_hsv[14] = 492;
    pSharpParamStatic->sharp_hsv[15] = 514;
    pSharpParamStatic->sharp_hsv[16] = 498;
    pSharpParamStatic->sharp_hsv[17] = 457;
    pSharpParamStatic->sharp_hsv[18] = 411;
    pSharpParamStatic->sharp_hsv[19] = 376;
    pSharpParamStatic->sharp_hsv[20] = 352;
    pSharpParamStatic->sharp_hsv[21] = 336;
    pSharpParamStatic->sharp_hsv[22] = 324;
    pSharpParamStatic->sharp_hsv[23] = 315;
    pSharpParamStatic->sharp_hsv[24] = 307;
    pSharpParamStatic->sharp_hsv[25] = 299;
    pSharpParamStatic->sharp_hsv[26] = 290;
    pSharpParamStatic->sharp_hsv[27] = 281;
    pSharpParamStatic->sharp_hsv[28] = 272;
    pSharpParamStatic->sharp_hsv[29] = 261;
    pSharpParamStatic->sharp_hsv[30] = 247;
    pSharpParamStatic->sharp_hsv[31] = 230;
    pSharpParamStatic->sharp_hsv[32] = 215;
    pSharpParamStatic->sharp_hsv[33] = 209;
    pSharpParamStatic->sharp_hsv[34] = 215;
    pSharpParamStatic->sharp_hsv[35] = 229;
    pSharpParamStatic->sharp_hsv[36] = 243;
    pSharpParamStatic->sharp_hsv[37] = 252;
    pSharpParamStatic->sharp_hsv[38] = 257;
    pSharpParamStatic->sharp_hsv[39] = 258;
    pSharpParamStatic->sharp_hsv[40] = 257;
    pSharpParamStatic->sharp_hsv[41] = 256;
    pSharpParamStatic->sharp_hsv[42] = 256;
    pSharpParamStatic->sharp_hsv[43] = 260;
    pSharpParamStatic->sharp_hsv[44] = 266;
    pSharpParamStatic->sharp_hsv[45] = 289;




    return 0;
}

static int venc_comp_event_handler(VideoEncoder *pEncoder, void *pAppData, VencEventType eEvent,
		unsigned int nData1, unsigned int nData2, void *pEventData)
{
	venc_comp_ctx *venc_comp = (venc_comp_ctx *)pAppData;

	if (VencEvent_UpdateIspToVeParam == eEvent) {
		int vipp_id = venc_comp->base_config.channel_id;
		int en_encpp = 0;
		sEncppSharpParam mSharpParam;
		sEncppSharpParamDynamic dynamic_sharp;
		sEncppSharpParamStatic static_sharp;
		s3DfilterParam m3DfilterParam;
		s2DfilterParam m2DfilterParam;

		vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);

		RT_LOGD("en_encpp %d", en_encpp);
		if (en_encpp) {
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_DYNAMIC_CFG, &dynamic_sharp);
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_STATIC_CFG, &static_sharp);
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCODER_3DNR_CFG, &m3DfilterParam);
			vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCODER_2DNR_CFG, &m2DfilterParam);

			RT_LOGD("hfr_hf_wht_clp %d max_clp_ratio %d", dynamic_sharp.hfr_hf_wht_clp, dynamic_sharp.max_clp_ratio);
			RT_LOGD("ls_dir_ratio %d ss_dir_ratio %d", static_sharp.ls_dir_ratio, static_sharp.ss_dir_ratio);

		} else {
			init_default_encpp_param(&dynamic_sharp, &static_sharp, &m3DfilterParam, &m2DfilterParam);
		}
		//en_encpp = 1;//en_cpp default to 1, no matter what the value from vin_get_encpp_cfg
		mSharpParam.pDynamicParam = &dynamic_sharp;
		mSharpParam.pStaticParam = &static_sharp;
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSharpConfig, &mSharpParam);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam3DFilterNew, &m3DfilterParam);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParam2DFilter, &m2DfilterParam);
	}
	return 0;
}
static int setup_vbv_buffer_size(venc_comp_ctx *venc_comp)
{
	int bit_rate_kb;
	int cache_size;
	int vbv_size;
	int min_size;
	int thresh_size;
	unsigned int bit_rate = venc_comp->base_config.bit_rate;
	int buffer_seconds    = 4; /* unit:s. */

	min_size    = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height * 3 / 2;
	thresh_size = venc_comp->base_config.dst_width * venc_comp->base_config.dst_height;

	if (thresh_size > 7 * 1024 * 1024) {
		RT_LOGW("Be careful! threshSize[%d]bytes too large, reduce to 7MB", thresh_size);
		thresh_size = 7 * 1024 * 1024;
	}

	if (bit_rate > 0) {
		bit_rate_kb = bit_rate >> 10;
		cache_size  = (bit_rate_kb * buffer_seconds) * (1024 / 8);
		vbv_size    = cache_size + thresh_size;
	} else {
		RT_LOGW("the bitrate error: %d", bit_rate);
		vbv_size = min_size;
	}

#if 0
	if (vbv_size < min_size) {
		vbv_size = min_size;
	}
#endif

	vbv_size = RT_ALIGN(vbv_size, 1024);

	if (vbv_size <= thresh_size) {
		RT_LOGE("fatal error! vbv_size[%d] <= thresh_size[%d]", vbv_size, thresh_size);
	}

	if (vbv_size > 12 * 1024 * 1024) {
		RT_LOGE("Be careful! vbv_size[%d] too large, exceed 24M byte", vbv_size);
		vbv_size = 12 * 1024 * 1024;
	}

	RT_LOGD("bit rate is %dKb, set encode vbv size %d, frame length threshold %d, minSize = %d",
		bit_rate_kb, vbv_size, thresh_size, min_size);

	if (venc_comp->base_config.dst_width >= 3840) {
		vbv_size    = 2 * 1024 * 1024;
		thresh_size = 1 * 1024 * 1024;
		RT_LOGW("the size is too large[%d x %d], so we reset vbv size and thresh_size to: %d, %d",
			venc_comp->base_config.dst_width, venc_comp->base_config.dst_height,
			vbv_size, thresh_size);
	}

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetFrameLenThreshold, &thresh_size);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int vencoder_create(venc_comp_ctx *venc_comp)
{
	VENC_CODEC_TYPE type = VENC_CODEC_H264;

	/* map codec type */
	switch (venc_comp->base_config.codec_type) {
	case VENC_COMP_CODEC_H264: {
		type = VENC_CODEC_H264;
		break;
	}
	case VENC_COMP_CODEC_H265: {
		type = VENC_CODEC_H265;
		break;
	}
	case VENC_COMP_CODEC_JPEG: {
		type = VENC_CODEC_JPEG;
		break;
	}
	default: {
		RT_LOGE("nor support codec type: %d", venc_comp->base_config.codec_type);
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}
	LOCK_MUTEX(&venc_mutex);
	venc_comp->vencoder = VencCreate(type);
	UNLOCK_MUTEX(&venc_mutex);
	return -1;
}

static int vencoder_init(venc_comp_ctx *venc_comp)
{
	VencBaseConfig base_config;
	int en_encpp = 0;
	int vipp_id = venc_comp->base_config.channel_id;

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bOnlineMode		   = 1; //venc_comp->base_config.bOnlineMode;
	base_config.bOnlineChannel	 = 0; //venc_comp->base_config.bOnlineChannel;
	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = venc_comp->base_config.dst_width;
	base_config.nDstHeight		   = venc_comp->base_config.dst_height;
	base_config.nStride		   = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = venc_comp->base_config.pixelformat;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;
	base_config.bIsVbvNoCache = 1;
	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	RT_LOGW("input format = %d, 2_5x_flag = %d", base_config.eInputFormat,
		base_config.bLbcLossyComEnFlag2_5x);

	vin_get_encpp_cfg(vipp_id, RT_CTRL_ENCPP_EN, &en_encpp);
	RT_LOGD("en_encpp %d", en_encpp);
	en_encpp = 1;//default open encpp
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, &en_encpp);
	/*int64_t time_start = get_cur_time();*/
	LOCK_MUTEX(&venc_mutex);
	VencInit(venc_comp->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);
	/*
	int64_t time_end = get_cur_time();
	RT_LOGD("time of VideoEncInit: %lld",(time_end - time_start));
	*/

	RT_LOGD("VencInit finish");

#if TEST_ENCODER_BY_SELF
	VencAllocateBufferParam bufferParam;
	memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
	bufferParam.nSizeY     = base_config.nInputWidth * base_config.nInputHeight;
	bufferParam.nSizeC     = base_config.nInputWidth * base_config.nInputHeight / 2;
	bufferParam.nBufferNum = 1;
	AllocInputBuffer(venc_comp->vencoder, &bufferParam);
#endif

	return 0;
}

static VENC_H264PROFILETYPE match_h264_profile(int src_profile)
{
	VENC_H264PROFILETYPE h264_profile = VENC_H264ProfileHigh;

	switch (src_profile) {
	case XM_Video_H264ProfileBaseline:
	case XM_Video_H264ProfileMain:
	case XM_Video_H264ProfileHigh:
		h264_profile = src_profile;
		break;
	default: {
		RT_LOGW("can not match the h264 profile: %d, use defaut: %d", src_profile, h264_profile);
		break;
	}
	}

	return h264_profile;
}

static VENC_H264LEVELTYPE match_h264_level(int src_level)
{
	VENC_H264LEVELTYPE h264_level = VENC_H264Level51;

	switch (src_level) {
	case XM_Video_H264Level1:
	case XM_Video_H264Level11:
	case XM_Video_H264Level12:
	case XM_Video_H264Level13:
	case XM_Video_H264Level2:
	case XM_Video_H264Level21:
	case XM_Video_H264Level22:
	case XM_Video_H264Level3:
	case XM_Video_H264Level31:
	case XM_Video_H264Level32:
	case XM_Video_H264Level4:
	case XM_Video_H264Level41:
	case XM_Video_H264Level42:
	case XM_Video_H264Level5:
	case XM_Video_H264Level51:
		h264_level = src_level;
		break;
	default: {
		RT_LOGW("can not match the h264 level: %d, defaut: %d", src_level, h264_level);
		break;
	}
	}

	return h264_level;
}

static VENC_H265PROFILETYPE match_h265_profile(int src_profile)
{
	VENC_H265PROFILETYPE h265_profile = VENC_H265ProfileMain;

	switch (src_profile) {
	case XM_Video_H265ProfileMain:
	case XM_Video_H265ProfileMain10:
	case XM_Video_H265ProfileMainStill:
		h265_profile = src_profile;
		break;
	default: {
		RT_LOGW("can not match the h265 profile: %d, defaut: %d", src_profile, h265_profile);
		break;
	}
	}

	return h265_profile;
}

static VENC_H265LEVELTYPE match_h265_level(int src_level)
{
	VENC_H265LEVELTYPE h265_level = VENC_H265Level51;

	switch (src_level) {
	case XM_Video_H265Level1:
	case XM_Video_H265Level2:
	case XM_Video_H265Level21:
	case XM_Video_H265Level3:
	case XM_Video_H265Level31:
	case XM_Video_H265Level41:
	case XM_Video_H265Level5:
	case XM_Video_H265Level51:
	case XM_Video_H265Level52:
	case XM_Video_H265Level6:
	case XM_Video_H265Level61:
	case XM_Video_H265Level62:
		h265_level = src_level;
		break;
	default: {
		RT_LOGW("can not match the h265 level: %d, defaut: %d", src_level, h265_level);
		break;
	}
	}

	return h265_level;
}

static int set_param_h264(venc_comp_ctx *venc_comp)
{
	VencH264VideoTiming video_time;
	VencH264Param param_h264;

	memset(&param_h264, 0, sizeof(VencH264Param));

	param_h264.sProfileLevel.nProfile  = match_h264_profile(venc_comp->base_config.profile);
	param_h264.sProfileLevel.nLevel    = match_h264_level(venc_comp->base_config.level);
	param_h264.bEntropyCodingCABAC     = 1;
	param_h264.sQPRange.nMinqp	 = venc_comp->base_config.qp_range.i_min_qp;
	param_h264.sQPRange.nMaxqp	 = venc_comp->base_config.qp_range.i_max_qp;
	param_h264.sQPRange.nMinPqp	= venc_comp->base_config.qp_range.p_min_qp;
	param_h264.sQPRange.nMaxPqp	= venc_comp->base_config.qp_range.p_max_qp;
	param_h264.sQPRange.nQpInit	= venc_comp->base_config.qp_range.i_init_qp;
	param_h264.nFramerate		   = venc_comp->base_config.frame_rate;
	param_h264.nSrcFramerate	   = venc_comp->base_config.frame_rate;
	param_h264.nBitrate		   = venc_comp->base_config.bit_rate;
	param_h264.nMaxKeyInterval	 = venc_comp->base_config.max_keyframe_interval;
	param_h264.nCodingMode		   = VENC_FRAME_CODING;
	param_h264.sGopParam.bUseGopCtrlEn = 1;
	param_h264.sGopParam.eGopMode      = AW_NORMALP;

	if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264CBR)
		param_h264.sRcParam.eRcMode = AW_CBR;
	else if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264VBR) {
		param_h264.sRcParam.eRcMode		  = AW_VBR;
		param_h264.sRcParam.sVbrParam.uMaxBitRate = venc_comp->base_config.vbr_param.max_bit_rate;
		param_h264.sRcParam.sVbrParam.nMovingTh   = venc_comp->base_config.vbr_param.moving_th;
		param_h264.sRcParam.sVbrParam.nQuality    = venc_comp->base_config.vbr_param.quality;
	} else {
		RT_LOGW("not support the rc_mode: %d", venc_comp->base_config.rc_mode);
	}

	RT_LOGI("h264 param: profile = %d, level = %d, min_qp = %d, max_qp = %d",
		param_h264.sProfileLevel.nProfile,
		param_h264.sProfileLevel.nLevel,
		param_h264.sQPRange.nMinqp,
		param_h264.sQPRange.nMaxqp);

	memset(&video_time, 0, sizeof(VencH264VideoTiming));

	video_time.num_units_in_tick     = 1;
	video_time.time_scale		 = 32;
	video_time.fixed_frame_rate_flag = 1;

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264Param, &param_h264);
	//VideoEncSetParameter(venc_comp->vencoder, VENC_IndexParamH264VideoTiming, &video_time);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int set_param_h265(venc_comp_ctx *venc_comp)
{
	VencH265Param h265Param;

	memset(&h265Param, 0, sizeof(VencH265Param));

	h265Param.sGopParam.bUseGopCtrlEn = 1;
	h265Param.sGopParam.eGopMode      = AW_NORMALP;
	h265Param.nBitrate		  = venc_comp->base_config.bit_rate;
	h265Param.nFramerate		  = venc_comp->base_config.frame_rate;
	h265Param.nQPInit		  = 35;
	h265Param.idr_period		  = venc_comp->base_config.max_keyframe_interval;
	h265Param.nGopSize		  = h265Param.idr_period;
	h265Param.nIntraPeriod		  = h265Param.idr_period;
	h265Param.sProfileLevel.nProfile  = match_h265_profile(venc_comp->base_config.profile);
	h265Param.sProfileLevel.nLevel    = match_h265_level(venc_comp->base_config.profile);

	h265Param.sQPRange.nMinqp  = venc_comp->base_config.qp_range.i_min_qp;
	h265Param.sQPRange.nMaxqp  = venc_comp->base_config.qp_range.i_max_qp;
	h265Param.sQPRange.nMinPqp = venc_comp->base_config.qp_range.p_min_qp;
	h265Param.sQPRange.nMaxPqp = venc_comp->base_config.qp_range.p_max_qp;
	h265Param.sQPRange.nQpInit = venc_comp->base_config.qp_range.i_init_qp;

	if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264CBR)
		h265Param.sRcParam.eRcMode = AW_CBR;
	else if (venc_comp->base_config.rc_mode == VENC_COMP_RC_MODE_H264VBR) {
		h265Param.sRcParam.eRcMode		 = AW_VBR;
		h265Param.sRcParam.sVbrParam.uMaxBitRate = venc_comp->base_config.vbr_param.max_bit_rate;
		h265Param.sRcParam.sVbrParam.nMovingTh   = venc_comp->base_config.vbr_param.moving_th;
		h265Param.sRcParam.sVbrParam.nQuality    = venc_comp->base_config.vbr_param.quality;
	} else {
		RT_LOGW("not support the rc_mode: %d", venc_comp->base_config.rc_mode);
	}

	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamH265Param, &h265Param);
	UNLOCK_MUTEX(&venc_mutex);

	return 0;
}

static int set_param_jpeg(venc_comp_ctx *venc_comp)
{
	LOCK_MUTEX(&venc_mutex);
	VencSetParameter(venc_comp->vencoder, VENC_IndexParamJpegQuality,
			 &venc_comp->base_config.quality);
	UNLOCK_MUTEX(&venc_mutex);
	return 0;
}

static void check_qp_value(video_qp_range *qp_range)
{
	int min_qp = qp_range->i_min_qp;
	int max_qp = qp_range->i_max_qp;

	if (!(min_qp >= 1 && min_qp <= 51)) {
		RT_LOGW("i_min_qp should in range:[1,51]! but usr_SetVal: %d! change it to default: 1", min_qp);
		min_qp = 1;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		RT_LOGW("i_max_qp should in range:[min_qp,51]! but usr_SetVal: %d! change it to default: 51", max_qp);
		max_qp = 51;
	}

	qp_range->i_min_qp = min_qp;
	qp_range->i_max_qp = max_qp;

	min_qp = qp_range->p_min_qp;
	max_qp = qp_range->p_max_qp;
	if (!(min_qp >= 1 && min_qp <= 51)) {
		RT_LOGW("p_min_qp should in range:[1,51]! but usr_SetVal: %d! change it to default: 1", min_qp);
		min_qp = 1;
	}
	if (!(max_qp >= min_qp && max_qp >= 1 && max_qp <= 51)) {
		RT_LOGW("p_max_qp should in range:[min_qp,51]! but usr_SetVal: %d! change it to default: 51", max_qp);
		max_qp = 51;
	}

	qp_range->p_min_qp = min_qp;
	qp_range->p_max_qp = max_qp;

	if (qp_range->i_init_qp <= 0 || qp_range->i_init_qp > 51)
		qp_range->i_init_qp = 35;
}

static int vencoder_set_param(venc_comp_ctx *venc_comp)
{
#if 0
	int rc_policy = BITRATE_PRI;
#endif
	struct file g2d_file;

	check_qp_value(&venc_comp->base_config.qp_range);

	RT_LOGI("i_qp = %d~%d, p_qp = %d~%d",
		venc_comp->base_config.qp_range.i_min_qp, venc_comp->base_config.qp_range.i_max_qp,
		venc_comp->base_config.qp_range.p_min_qp, venc_comp->base_config.qp_range.p_max_qp);

	if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_H264)
		set_param_h264(venc_comp);
	else if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_H265)
		set_param_h265(venc_comp);
	else if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_JPEG)
		set_param_jpeg(venc_comp);
#if 0 //need to update
	if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_H264
		|| venc_comp->base_config.codec_type == VENC_COMP_CODEC_H265) {
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetRCPriority, &rc_policy);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearch, &venc_comp->base_config.motion_search_param);
		UNLOCK_MUTEX(&venc_mutex);
	}
#endif
	setup_vbv_buffer_size(venc_comp);

	if (venc_comp->vencoder_init_flag == 0) {
		vencoder_init(venc_comp);
		venc_comp->vencoder_init_flag = 1;
	}

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	RT_LOGW("enable_overlay = %d", venc_comp->base_config.enable_overlay);
	if (venc_comp->base_config.enable_overlay == 1) {
		if (adjust_native_overlay_info(venc_comp) == 0) {
			LOCK_MUTEX(&venc_mutex);
			g2d_open(0, &g2d_file);
			VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, &global_osd_info->sOverlayInfo);
			g2d_release(0, &g2d_file);
			UNLOCK_MUTEX(&venc_mutex);
		}
	}
#endif

	return 0;
}

static int commond_process(struct venc_comp_ctx *venc_comp, message_t *msg)
{
	int cmd_error		      = 0;
	int cmd			      = msg->command;
	wait_queue_head_t *wait_reply = (wait_queue_head_t *)msg->wait_queue;
	unsigned int *wait_condition  = (unsigned int *)msg->wait_condition;

	RT_LOGD("cmd process: cmd = %d, state = %d, wait_reply = %p, wait_condition = %p",
		cmd, venc_comp->state, wait_reply, wait_condition);

	if (cmd == COMP_COMMAND_INIT) {
		if (venc_comp->state != COMP_STATE_IDLE) {
			cmd_error = 1;
		} else {
			vencoder_create(venc_comp);
			venc_comp->state = COMP_STATE_INITIALIZED;
		}
	} else if (cmd == COMP_COMMAND_START) {
		if (venc_comp->state != COMP_STATE_INITIALIZED && venc_comp->state != COMP_STATE_PAUSE) {
			cmd_error = 1;
		} else {
#if 0
			if (venc_comp->state == COMP_STATE_INITIALIZED)
				vencoder_set_param(venc_comp);
#endif

			venc_comp->state = COMP_STATE_EXECUTING;
		}
	} else if (cmd == COMP_COMMAND_PAUSE) {
		venc_comp->state = COMP_STATE_PAUSE;
	} else if (cmd == COMP_COMMAND_STOP) {
		venc_comp->state = COMP_STATE_IDLE;
	}

	if (cmd_error == 1) {
		venc_comp->callback->EventHandler(
			venc_comp->self_comp,
			venc_comp->callback_data,
			COMP_EVENT_CMD_ERROR,
			COMP_COMMAND_INIT,
			0,
			NULL);
	} else {
		venc_comp->callback->EventHandler(
			venc_comp->self_comp,
			venc_comp->callback_data,
			COMP_EVENT_CMD_COMPLETE,
			cmd,
			0,
			NULL);
	}

	if (wait_reply) {
		RT_LOGD("wait up: cmd = %d, state = %d", cmd, venc_comp->state);
		*wait_condition = 1;
		wake_up(wait_reply);
	}

	return 0;
}

/* the input buffer had useded, return it*/
/* valid_list --> empty_list */
static int venc_empty_in_buffer_done(void *venc_comp,
				     void *frame_info)
{
	struct venc_comp_ctx *pvenc_comp = (struct venc_comp_ctx *)venc_comp;
	video_frame_s *pframe_info       = (video_frame_s *)frame_info;

	comp_buffer_header_type buffer_header;
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));
	buffer_header.private = pframe_info;

	if (pvenc_comp->in_port_tunnel_info.valid_flag == 0) {
		pvenc_comp->callback->empty_in_buffer_done(
			pvenc_comp->self_comp,
			pvenc_comp->callback_data,
			&buffer_header);
	} else {
		comp_fill_this_out_buffer(pvenc_comp->in_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGD("venc_comp %p channel id %d release input FrameId[%d]", venc_comp, pvenc_comp->base_config.channel_id, pframe_info->id);
	return ERROR_TYPE_OK;
}

/* empty_list --> valid_list */
static int venc_fill_out_buffer_done(struct venc_comp_ctx *venc_comp,
				     VencOutputBuffer *out_buffer)
{
	video_stream_node *stream_node = NULL;
	error_type error	       = ERROR_TYPE_OK;
	comp_buffer_header_type buffer_header;
	video_stream_s mvideo_stream;

	memset(&mvideo_stream, 0, sizeof(video_stream_s));
	memset(&buffer_header, 0, sizeof(comp_buffer_header_type));

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);

	config_videostream_by_VencOutputBuffer(venc_comp, out_buffer, &stream_node->video_stream);
	memcpy(&mvideo_stream, &stream_node->video_stream, sizeof(video_stream_s));

	venc_comp->out_buf_manager.empty_num--;
	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.valid_stream_list);
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	buffer_header.private = &mvideo_stream;
	if (venc_comp->out_port_tunnel_info.valid_flag == 0) {
		venc_comp->callback->fill_out_buffer_done(
			venc_comp->self_comp,
			venc_comp->callback_data,
			&buffer_header);
	} else {
		comp_empty_this_in_buffer(venc_comp->out_port_tunnel_info.tunnel_comp, &buffer_header);
	}

	RT_LOGD(" id = %d", mvideo_stream.id);
	return error;
}

#if 0
static void init_jpeg_exif(EXIFInfo *exifinfo)
{
	exifinfo->ThumbWidth = 640;
	exifinfo->ThumbHeight = 480;

	strcpy((char *)exifinfo->CameraMake,        "allwinner make test");
	strcpy((char *)exifinfo->CameraModel,        "allwinner model test");
	strcpy((char *)exifinfo->DateTime,         "2014:02:21 10:54:05");
	strcpy((char *)exifinfo->gpsProcessingMethod,  "allwinner gps");

	exifinfo->Orientation = 0;

	exifinfo->ExposureTime.num = 2;
	exifinfo->ExposureTime.den = 1000;

	exifinfo->FNumber.num = 20;
	exifinfo->FNumber.den = 10;
	exifinfo->ISOSpeed = 50;

	exifinfo->ExposureBiasValue.num = -4;
	exifinfo->ExposureBiasValue.den = 1;

	exifinfo->MeteringMode = 1;
	exifinfo->FlashUsed = 0;

	exifinfo->FocalLength.num = 1400;
	exifinfo->FocalLength.den = 100;

	exifinfo->DigitalZoomRatio.num = 4;
	exifinfo->DigitalZoomRatio.den = 1;

	exifinfo->WhiteBalance = 1;
	exifinfo->ExposureMode = 1;

	exifinfo->enableGpsInfo = 1;

	exifinfo->gps_latitude = 23.2368;
	exifinfo->gps_longitude = 24.3244;
	exifinfo->gps_altitude = 1234.5;

	exifinfo->gps_timestamp = 0;

	strcpy((char *)exifinfo->CameraSerialNum,  "123456789");
	strcpy((char *)exifinfo->ImageName,  "exif-name-test");
	strcpy((char *)exifinfo->ImageDescription,  "exif-descriptor-test");
}
#endif

static int catch_jpeg_encoder(venc_comp_ctx *venc_comp, VencInputBuffer *in_buf)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	int result		 = 0;

	result = VencQueueInputBuf(jpeg_cxt->vencoder, in_buf);
	if (result != 0) {
		RT_LOGE("fatal error! VencQueueInputBuf fail[%d]", result);
	}

	LOCK_MUTEX(&venc_mutex);
	result = encodeOneFrame(jpeg_cxt->vencoder);
	UNLOCK_MUTEX(&venc_mutex);

	RT_LOGI("encoder result = %d", result);

	jpeg_cxt->wait_enc_finish_condition = 1;
	wake_up(&jpeg_cxt->wait_enc_finish);

	jpeg_cxt->encoder_finish_flag = 1;

	return 0;
}

static int catch_jpeg_start(venc_comp_ctx *venc_comp, catch_jpeg_config *jpeg_config)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	VENC_CODEC_TYPE type     = VENC_CODEC_JPEG;
	VencBaseConfig base_config;
	unsigned int vbv_size = jpeg_config->width * jpeg_config->height / 2;
	unsigned int quality  = jpeg_config->qp;

	jpeg_cxt->width  = jpeg_config->width;
	jpeg_cxt->height = jpeg_config->height;
	jpeg_cxt->qp     = jpeg_config->qp;

	LOCK_MUTEX(&venc_mutex);
	jpeg_cxt->vencoder = VencCreate(type);
	UNLOCK_MUTEX(&venc_mutex);

	if (!jpeg_cxt->vencoder) {
		RT_LOGE("create vencoder failed");
		return -1;
	}

/* setup exif */
#if 0
	EXIFInfo exif_info;
	memset(&exif_info, 0, sizeof(EXIFInfo));
	init_jpeg_exif(&exif_info);
	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamJpegExifInfo, &exif_info);
#endif

	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamSetVbvSize, &vbv_size);
	VencSetParameter(jpeg_cxt->vencoder, VENC_IndexParamJpegQuality, &quality);

	memset(&base_config, 0, sizeof(VencBaseConfig));

	base_config.bEncH264Nalu	   = 0;
	base_config.nInputWidth		   = venc_comp->base_config.src_width;
	base_config.nInputHeight	   = venc_comp->base_config.src_height;
	base_config.nDstWidth		   = jpeg_cxt->width;
	base_config.nDstHeight		   = jpeg_cxt->height;
	base_config.nStride		   = RT_ALIGN(base_config.nInputWidth, 16);
	base_config.eInputFormat	   = VENC_PIXEL_YUV420SP;
	base_config.bOnlyWbFlag		   = 0;
	base_config.memops		   = NULL;
	base_config.veOpsS		   = NULL;
	base_config.bLbcLossyComEnFlag2x   = 0;
	base_config.bLbcLossyComEnFlag2_5x = 0;

	if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_25X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2_5x = 1;
	} else if (venc_comp->base_config.pixelformat == RT_PIXEL_LBC_2X) {
		base_config.eInputFormat	   = VENC_PIXEL_LBC_AW;
		base_config.bLbcLossyComEnFlag2x = 1;
	}

	LOCK_MUTEX(&venc_mutex);
	VencInit(jpeg_cxt->vencoder, &base_config);
	UNLOCK_MUTEX(&venc_mutex);

	/* timeout is 2000 ms: HZ is 100, HZ == 1s, so 1 jiffies is 10 ms */
	jpeg_cxt->wait_enc_finish_condition = 0;
	jpeg_cxt->enable		    = 1;
	wait_event_timeout(jpeg_cxt->wait_enc_finish, jpeg_cxt->wait_enc_finish_condition, 200);

	if (jpeg_cxt->wait_enc_finish_condition == 0) {
		RT_LOGE("wait for enc finish timeout");
		return -1;
	}

	return 0;
}

static int catch_jpeg_stop(venc_comp_ctx *venc_comp)
{
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;

	if (jpeg_cxt->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(jpeg_cxt->vencoder);
		jpeg_cxt->vencoder = NULL;
		UNLOCK_MUTEX(&venc_mutex);
	} else {
		RT_LOGE("the jpeg_cxt->vencoder is null");
		return -1;
	}

	jpeg_cxt->enable = 0;

	return 0;
}

static int catch_jpeg_get_data(venc_comp_ctx *venc_comp, void *user_buf_info)
{
	int result = 0;
	VencOutputBuffer out_buffer;
	catch_jpeg_cxt *jpeg_cxt = &venc_comp->jpeg_cxt;
	catch_jpeg_buf_info src_buf_info;

	if (jpeg_cxt->enable == 0 || !jpeg_cxt->vencoder) {
		RT_LOGE("error: enable = %d, vencoder = %p", jpeg_cxt->enable, jpeg_cxt->vencoder);
		return -1;
	}

	memset(&src_buf_info, 0, sizeof(catch_jpeg_buf_info));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	if (copy_from_user(&src_buf_info, (void __user *)user_buf_info, sizeof(struct catch_jpeg_buf_info))) {
		RT_LOGE("IOCTL_CATCH_JPEG_START copy_from_user fail\n");
		return -EFAULT;
	}

	result = VencDequeueOutputBuf(jpeg_cxt->vencoder, &out_buffer);
	if (result != 0) {
		RT_LOGE("have no bitstream");
		return -1;
	}

	RT_LOGI("data_size = %d, max_size = %d",
		out_buffer.nSize0, src_buf_info.buf_size);

	if (out_buffer.nSize0 > src_buf_info.buf_size) {
		RT_LOGE("buf_size overflow: %d > %d", out_buffer.nSize0, src_buf_info.buf_size);
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)src_buf_info.buf, out_buffer.pData0, out_buffer.nSize0)) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

	if (copy_to_user((void *)user_buf_info, &out_buffer.nSize0, sizeof(unsigned int))) {
		RT_LOGE(" copy_to_user fail\n");
		result = -1;
		goto catch_jpeg_get_data_exit;
	}

catch_jpeg_get_data_exit:

	VencQueueOutputBuf(jpeg_cxt->vencoder, &out_buffer);

	return result;
}

typedef struct osd_convert_dst_info {
	unsigned int dst_ext_buf_size;
	unsigned int dst_w_ext;
	unsigned int dst_h_ext;
	unsigned int dst_crop_x;
	unsigned int dst_crop_y;
	unsigned int dst_crop_w;
	unsigned int dst_crop_h;
	unsigned int dst_start_x;
	unsigned int dst_start_y;
} osd_convert_dst_info;

typedef struct osd_convert_src_info {
	unsigned int start_x;
	unsigned int start_y;
	unsigned int widht;
	unsigned int height;
} osd_convert_src_info;

static int convert_osd_pos_info(venc_comp_ctx *venc_comp, osd_convert_dst_info *dst_info, osd_convert_src_info *src_info)
{
	/* 1X: 100 */
	unsigned int w_ratio	= 100;
	unsigned int h_ratio	= 100;
	unsigned int is_left_corner = 0;

	if (global_osd_info->ori_frame_w != venc_comp->base_config.dst_width || global_osd_info->ori_frame_h != venc_comp->base_config.dst_height) {
		w_ratio = venc_comp->base_config.dst_width * 100 / global_osd_info->ori_frame_w;
		h_ratio = venc_comp->base_config.dst_height * 100 / global_osd_info->ori_frame_h;
		RT_LOGI("ori_w&h = %d, %d, cur_w&h = %d, %d; w_ratio = %d, h_ratio = %d",
			global_osd_info->ori_frame_w, global_osd_info->ori_frame_h,
			venc_comp->base_config.dst_width, venc_comp->base_config.dst_height,
			w_ratio, h_ratio);
	}

	if (src_info->start_x > 160)
		is_left_corner = 0;
	else
		is_left_corner = 1;

	/* compulte the scale ratio */
	if (is_left_corner == 1) {
		dst_info->dst_start_x = 0;
		dst_info->dst_start_y = 0;
		dst_info->dst_w_ext   = src_info->start_x + src_info->widht;
		dst_info->dst_h_ext   = src_info->start_y + src_info->height;
		dst_info->dst_crop_x  = src_info->start_x;
		dst_info->dst_crop_y  = src_info->start_y;
		dst_info->dst_crop_w  = src_info->widht;
		dst_info->dst_crop_h  = src_info->height;

		dst_info->dst_w_ext  = dst_info->dst_w_ext * w_ratio / 100;
		dst_info->dst_h_ext  = dst_info->dst_h_ext * h_ratio / 100;
		dst_info->dst_crop_x = dst_info->dst_crop_x * w_ratio / 100;
		dst_info->dst_crop_y = dst_info->dst_crop_y * h_ratio / 100;
		dst_info->dst_crop_w = dst_info->dst_crop_w * w_ratio / 100;
		dst_info->dst_crop_h = dst_info->dst_crop_h * h_ratio / 100;
	} else {
		dst_info->dst_start_y = 0;
		dst_info->dst_start_x = src_info->start_x;
		dst_info->dst_start_x = (dst_info->dst_start_x / 16) * 16;

		dst_info->dst_w_ext = global_osd_info->ori_frame_w - dst_info->dst_start_x;
		dst_info->dst_h_ext = src_info->start_y + src_info->height;

		dst_info->dst_crop_x = src_info->start_x - dst_info->dst_start_x;
		dst_info->dst_crop_y = src_info->start_y;
		dst_info->dst_crop_w = src_info->widht;
		dst_info->dst_crop_h = src_info->height;

		if (w_ratio != 100) {
			unsigned int dst_start_x_ori = 0;

			dst_start_x_ori     = dst_info->dst_start_x * w_ratio / 100;
			dst_info->dst_w_ext = dst_info->dst_w_ext * w_ratio / 100;
			dst_info->dst_h_ext = dst_info->dst_h_ext * h_ratio / 100;

			dst_info->dst_start_x = (dst_start_x_ori / 16) * 16;
			dst_info->dst_w_ext   = venc_comp->base_config.dst_width - dst_info->dst_start_x;

			dst_info->dst_crop_x = dst_info->dst_crop_x * w_ratio / 100;
			dst_info->dst_crop_y = dst_info->dst_crop_y * h_ratio / 100;
			dst_info->dst_crop_w = dst_info->dst_crop_w * w_ratio / 100;
			dst_info->dst_crop_h = dst_info->dst_crop_h * h_ratio / 100;

			dst_info->dst_crop_x += dst_start_x_ori - dst_info->dst_start_x;
		}
	}

	dst_info->dst_w_ext = RT_ALIGN(dst_info->dst_w_ext, 16);
	dst_info->dst_h_ext = RT_ALIGN(dst_info->dst_h_ext, 16);

	dst_info->dst_ext_buf_size = dst_info->dst_w_ext * dst_info->dst_h_ext * 4;

	RT_LOGI("is_left = %d, dst_w&h_e = %d, %d; crop = %d, %d, %d, %d; ratio = %d, %d",
		is_left_corner, dst_info->dst_w_ext, dst_info->dst_h_ext,
		dst_info->dst_crop_x, dst_info->dst_crop_y, dst_info->dst_crop_w, dst_info->dst_crop_h,
		w_ratio, h_ratio);

	return 0;
}

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
int convert_overlay_info_native(venc_comp_ctx *venc_comp, VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	struct mem_param param;
	unsigned int in_phy_addr = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	if (global_osd_info->memops == NULL) {
		global_osd_info->memops = mem_create(MEM_TYPE_ION, param);
		if (global_osd_info->memops == NULL) {
			RT_LOGE("mem_create failed\n");
			return -1;
		}
	}

	if (global_osd_info->ori_frame_w == 0 || global_osd_info->ori_frame_h == 0) {
		global_osd_info->ori_frame_w = venc_comp->base_config.dst_width;
		global_osd_info->ori_frame_h = venc_comp->base_config.dst_height;
	}

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;
	pvenc_osd->is_user_buf_flag = 0;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		if (i >= GLOBAL_OSD_MAX_NUM) {
			RT_LOGW("osd num overlay: blk_num = %d, GLOBAL_OSD_MAX_NUM = %d",
				pvenc_osd->blk_num, GLOBAL_OSD_MAX_NUM);
			break;
		}
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];
		/* copy osd data to native */
		if (global_osd_info->g2d_in_ion_vir_addr[i] == NULL || global_osd_info->g2d_in_buf_size[i] < src_item->data_size) {
			if (global_osd_info->g2d_in_ion_vir_addr[i])
				cdc_mem_pfree(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

			global_osd_info->g2d_in_ion_vir_addr[i] = cdc_mem_palloc(global_osd_info->memops, src_item->data_size);
			global_osd_info->g2d_in_buf_size[i]     = src_item->data_size;
		}
		if (copy_from_user(global_osd_info->g2d_in_ion_vir_addr[i], (void __user *)src_item->data_buf, src_item->data_size)) {
			RT_LOGE("g2d_scale copy_from_user fail\n");
			return -1;
		}

		cdc_mem_flush_cache(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i], src_item->data_size);

		in_phy_addr = cdc_mem_get_phy(global_osd_info->memops, global_osd_info->g2d_in_ion_vir_addr[i]);

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

		osd_src_info.start_x = src_item->start_x;
		osd_src_info.start_y = src_item->start_y;
		osd_src_info.widht   = src_item->widht;
		osd_src_info.height  = src_item->height;

		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->overlay_blk_addr    = src_item->data_buf;
		dst_item->bitmap_size	 = osd_dst_info.dst_ext_buf_size;
		dst_item->overlay_type	= NORMAL_OVERLAY;
		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
#if 0
		/* consfer info */
		dst_item->mScaleInfo.bEnable = 1;
		dst_item->mScaleInfo.src_buf_phy_addr = in_phy_addr;
		dst_item->mScaleInfo.src_buf_size     = src_item->data_size;
		dst_item->mScaleInfo.src_w = src_item->widht;
		dst_item->mScaleInfo.src_h = src_item->height;

		dst_item->mScaleInfo.dst_w_ext  = osd_dst_info.dst_w_ext;
		dst_item->mScaleInfo.dst_h_ext  = osd_dst_info.dst_h_ext;
		dst_item->mScaleInfo.dst_crop_x = osd_dst_info.dst_crop_x;
		dst_item->mScaleInfo.dst_crop_y = osd_dst_info.dst_crop_y;
		dst_item->mScaleInfo.dst_crop_w = osd_dst_info.dst_crop_w;
		dst_item->mScaleInfo.dst_crop_h = osd_dst_info.dst_crop_h;

		dst_item->mScaleInfo.ori_start_x = src_item->start_x;
		dst_item->mScaleInfo.ori_start_y = src_item->start_y;
#endif
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %p",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}

	return 0;
}

static int adjust_native_overlay_info(venc_comp_ctx *venc_comp)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	int align_end_x		     = 0;
	int align_end_y		     = 0;
	osd_convert_dst_info osd_dst_info;
	osd_convert_src_info osd_src_info;

	memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
	memset(&osd_src_info, 0, sizeof(osd_convert_src_info));

	if (global_osd_info == NULL) {
		RT_LOGW("the global_osd_info is null");
		return -1;
	}

	for (i = 0; i < global_osd_info->sOverlayInfo.blk_num; i++) {
		dst_item = &global_osd_info->sOverlayInfo.overlayHeaderList[i];

		memset(&osd_dst_info, 0, sizeof(osd_convert_dst_info));
		memset(&osd_src_info, 0, sizeof(osd_convert_src_info));
#if 0
		osd_src_info.start_x = dst_item->mScaleInfo.ori_start_x;
		osd_src_info.start_y = dst_item->mScaleInfo.ori_start_y;
		osd_src_info.widht   = dst_item->mScaleInfo.src_w;
		osd_src_info.height  = dst_item->mScaleInfo.src_h;
#endif
		convert_osd_pos_info(venc_comp, &osd_dst_info, &osd_src_info);

		align_end_x = osd_dst_info.dst_start_x + osd_dst_info.dst_w_ext;
		align_end_y = osd_dst_info.dst_start_y + osd_dst_info.dst_h_ext;

		dst_item->start_mb_x = osd_dst_info.dst_start_x / 16;
		dst_item->start_mb_y = osd_dst_info.dst_start_y / 16;
		dst_item->end_mb_x   = align_end_x / 16 - 1;
		dst_item->end_mb_y   = align_end_y / 16 - 1;

		dst_item->bitmap_size = osd_dst_info.dst_ext_buf_size;
#if 0
		dst_item->mScaleInfo.bEnable = 1;
		dst_item->mScaleInfo.dst_w_ext  = osd_dst_info.dst_w_ext;
		dst_item->mScaleInfo.dst_h_ext  = osd_dst_info.dst_h_ext;
		dst_item->mScaleInfo.dst_crop_x = osd_dst_info.dst_crop_x;
		dst_item->mScaleInfo.dst_crop_y = osd_dst_info.dst_crop_y;
		dst_item->mScaleInfo.dst_crop_w = osd_dst_info.dst_crop_w;
		dst_item->mScaleInfo.dst_crop_h = osd_dst_info.dst_crop_h;
#endif
		RT_LOGI("osd item[%d]: s_x&y = %d, %d; e_x&y = %d, %d, size = %d, buf = %p",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}

#else
static int convert_overlay_info(VencOverlayInfoS *pvenc_osd, VideoInputOSD *user_osd_info)
{
	int i			     = 0;
	VencOverlayHeaderS *dst_item = NULL;
	OverlayItemInfo *src_item    = NULL;
	int align_start_x	    = 0;
	int align_start_y	    = 0;
	int align_end_x		     = 0;
	int align_end_y		     = 0;

	pvenc_osd->blk_num	  = user_osd_info->osd_num;
	pvenc_osd->argb_type	= user_osd_info->argb_type;
	pvenc_osd->is_user_buf_flag = 1;

	for (i = 0; i < pvenc_osd->blk_num; i++) {
		dst_item = &pvenc_osd->overlayHeaderList[i];
		src_item = &user_osd_info->item_info[i];

		align_start_x = RT_ALIGN(src_item->start_x, 16);
		align_start_y = RT_ALIGN(src_item->start_y, 16);
		align_end_x   = align_start_x + src_item->widht;
		align_end_y   = align_start_y + src_item->height;

		dst_item->overlay_blk_addr = src_item->data_buf;
		dst_item->bitmap_size      = src_item->data_size;
		dst_item->start_mb_x       = align_start_x / 16;
		dst_item->start_mb_y       = align_start_y / 16;
		dst_item->end_mb_x	 = align_end_x / 16 - 1;
		dst_item->end_mb_y	 = align_end_y / 16 - 1;

		dst_item->overlay_type	= NORMAL_OVERLAY;
		dst_item->extra_alpha_flag    = 0;
		dst_item->bforce_reverse_flag = 0;
		RT_LOGI("osd item[%d]: s_x = %d, s_y = %d, e_x = %d, e_y = %d, size = %d, buf = %p",
			i, dst_item->start_mb_x, dst_item->start_mb_y, dst_item->end_mb_x,
			dst_item->end_mb_y, dst_item->bitmap_size, dst_item->overlay_blk_addr);
	}
	return 0;
}
#endif
static int flush_buffer(venc_comp_ctx *venc_comp)
{
	video_frame_node *frame_node = NULL;
	video_frame_s cur_video_frame;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));

	mutex_lock(&venc_comp->in_buf_manager.mutex);
	while (!list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;

		RT_LOGW("flush in frame buf, frame_node = %p", frame_node);

		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
	}
	mutex_unlock(&venc_comp->in_buf_manager.mutex);

	return 0;
}
static error_type config_dynamic_param(
	venc_comp_ctx *venc_comp,
	comp_index_type index,
	void *param_data)
{
	error_type error = ERROR_TYPE_OK;
	int ret		 = 0;

	if (venc_comp->vencoder == NULL) {
		RT_LOGW("vencoder is not be created when config dynamic param");
		return ERROR_TYPE_ERROR;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame: {
		RT_LOGI("*****ForceKeyFrame***** ");
		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamForceKeyFrame, param_data);
		UNLOCK_MUTEX(&venc_mutex);

		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START: {
		catch_jpeg_config *jpeg_config = (catch_jpeg_config *)param_data;
		if (catch_jpeg_start(venc_comp, jpeg_config) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP: {
		if (catch_jpeg_stop(venc_comp) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA: {
		if (catch_jpeg_get_data(venc_comp, param_data) != 0)
			error = ERROR_TYPE_ERROR;
		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_OSD: {
#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
		VencOverlayInfoS *pOverlayInfo = &global_osd_info->sOverlayInfo;
		VideoInputOSD inputOsd;
		struct file g2d_file;

		memset(pOverlayInfo, 0, sizeof(VencOverlayInfoS));
		memset(&inputOsd, 0, sizeof(VideoInputOSD));

		if (copy_from_user(&inputOsd, (void __user *)param_data, sizeof(VideoInputOSD))) {
			RT_LOGE("set osd copy_from_user fail\n");
			return -EFAULT;
		}

		convert_overlay_info_native(venc_comp, pOverlayInfo, &inputOsd);

		LOCK_MUTEX(&venc_mutex);
		g2d_open(0, &g2d_file);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, pOverlayInfo);
		g2d_release(0, &g2d_file);
		UNLOCK_MUTEX(&venc_mutex);
#else
		VencOverlayInfoS sOverlayInfo;
		VideoInputOSD inputOsd;

		memset(&sOverlayInfo, 0, sizeof(VencOverlayInfoS));
		memset(&inputOsd, 0, sizeof(VideoInputOSD));

		if (copy_from_user(&inputOsd, (void __user *)param_data, sizeof(VideoInputOSD))) {
			RT_LOGE("set osd copy_from_user fail\n");
			return -EFAULT;
		}

		convert_overlay_info(&sOverlayInfo, &inputOsd);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetOverlay, &sOverlayInfo);
		UNLOCK_MUTEX(&venc_mutex);
#endif
		RT_LOGI("ENABLE_SAVE_NATIVE_OVERLAY_DATA = %d", ENABLE_SAVE_NATIVE_OVERLAY_DATA);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: {
#if 0
		rt_venc_bin_image_param *bin_param = (rt_venc_bin_image_param *)param_data;
		VencBinImageParam venc_bin_param;

		venc_bin_param.enable = bin_param->enable;
		venc_bin_param.moving_th = bin_param->moving_th;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE: enable = %d, th = %d",
				venc_bin_param.enable, venc_bin_param.moving_th);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableGetBinImage, &venc_bin_param);
#endif
		break;
	}
	case COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO: {
		//VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableMvInfo, param_data);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_Flush_buffer: {
		flush_buffer(venc_comp);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE: {
		VencQPRange mVencQpRange;
		video_qp_range *pQp_range = (video_qp_range *)param_data;

		RT_LOGW("SET_QP_RANGE: i_qp = %d ~ %d, p_qp = %d ~ %d, bEnMbQpLimit = %d",
			pQp_range->i_min_qp, pQp_range->i_max_qp,
			pQp_range->p_min_qp, pQp_range->p_max_qp, pQp_range->enable_mb_qp_limit);

		mVencQpRange.nMinqp       = pQp_range->i_min_qp;
		mVencQpRange.nMaxqp       = pQp_range->i_max_qp;
		mVencQpRange.nMinPqp      = pQp_range->p_min_qp;
		mVencQpRange.nMaxPqp      = pQp_range->p_max_qp;
		mVencQpRange.nQpInit      = pQp_range->i_init_qp;
		mVencQpRange.bEnMbQpLimit = pQp_range->enable_mb_qp_limit;

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamH264QPRange, &mVencQpRange);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE: {
		int bitrate = (int)param_data;

		RT_LOGI("SET_BITRATE: bitrate = %d", bitrate);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamBitrate, &bitrate);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS: {
		int fps = (int)param_data;

		RT_LOGI("SET_FPS: fps = %d", fps);

		LOCK_MUTEX(&venc_mutex);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamFramerate, &fps);
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM: {
		RTVencVbrParam *rt_vbr_param = (RTVencVbrParam *)param_data;
		VencVbrParam mVbrParam;

		memset(&mVbrParam, 0, sizeof(VencVbrParam));

		mVbrParam.uMaxBitRate   = rt_vbr_param->uMaxBitRate * 1024;
		mVbrParam.nMovingTh     = rt_vbr_param->nMovingTh;
		mVbrParam.nQuality      = rt_vbr_param->nQuality;
		mVbrParam.nIFrmBitsCoef = rt_vbr_param->nIFrmBitsCoef;
		mVbrParam.nPFrmBitsCoef = rt_vbr_param->nPFrmBitsCoef;

		RT_LOGI("SET_VBR_PARAM: uMaxBitRate = %d, nMovingTh = %d, nQuality = %d, coef = %d, %d",
			mVbrParam.uMaxBitRate, mVbrParam.nMovingTh,
			mVbrParam.nQuality, mVbrParam.nIFrmBitsCoef, mVbrParam.nPFrmBitsCoef);

		LOCK_MUTEX(&venc_mutex);
		ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSetVbrParam, &mVbrParam);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
		UNLOCK_MUTEX(&venc_mutex);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO: {
		RTVencMBSumInfo *prt_mb_sum = (RTVencMBSumInfo *)param_data;
		VencMBSumInfo venc_mb_sum;

		memset(&venc_mb_sum, 0, sizeof(VencMBSumInfo));

		ret = VencGetParameter(venc_comp->vencoder, VENC_IndexParamMBSumInfoOutput, &venc_mb_sum);
		if (ret != 0)
			error = ERROR_TYPE_ERROR;
#if 0 //todo
		prt_mb_sum->sum_mad = venc_mb_sum.sum_mad;
		prt_mb_sum->sum_qp  = venc_mb_sum.sum_qp;
		prt_mb_sum->sum_sse = venc_mb_sum.sum_sse;
		prt_mb_sum->avg_sse = venc_mb_sum.avg_sse;
#endif
		RT_LOGI("GET_SUM_MB_INFO: sum_mad = %d, sum_qp = %d, sum_sse = %lld, avg_sse = %d",
			prt_mb_sum->sum_mad, prt_mb_sum->sum_qp, prt_mb_sum->sum_sse, prt_mb_sum->avg_sse);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE: {
		int is_night_case = *(int *)param_data;

		RT_LOGW("is_night_case = %d", is_night_case);

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamIsNightCaseFlag, &is_night_case);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM: {
		RTVencSuperFrameConfig *pconfig = (RTVencSuperFrameConfig *)param_data;
		VencSuperFrameConfig mvenc_config;

		memset(&mvenc_config, 0, sizeof(VencSuperFrameConfig));

		if (pconfig->bDropSuperFrame == 1)
			mvenc_config.eSuperFrameMode = VENC_SUPERFRAME_DISCARD;
		else
			mvenc_config.eSuperFrameMode = VENC_SUPERFRAME_NONE;

		mvenc_config.nMaxIFrameBits = pconfig->nMaxIFrameBits;
		mvenc_config.nMaxPFrameBits = pconfig->nMaxPFrameBits;

		if (venc_comp->vencoder) {
			LOCK_MUTEX(&venc_mutex);
			ret = VencSetParameter(venc_comp->vencoder, VENC_IndexParamSuperFrameConfig, &mvenc_config);
			if (ret != 0)
				error = ERROR_TYPE_ERROR;
			UNLOCK_MUTEX(&venc_mutex);
		}

		break;
	}
	case COMP_INDEX_VENC_CONFIG_SET_SHARP: {
		int enableSharp = 1;
		sEncppSharpParam mSharpParam;
		sEncppSharpParamDynamic dynamic_sharp;
		sEncppSharpParamStatic static_sharp;

		if (copy_from_user(&mSharpParam, (void __user *)param_data, sizeof(sEncppSharpParam))) {
			RT_LOGE("COMP_INDEX_VENC_CONFIG_SET_SHARP copy_from_user fail\n");
			return -EFAULT;
		}

		if (copy_from_user(&dynamic_sharp, (void __user *)mSharpParam.pDynamicParam, sizeof(sEncppSharpParamDynamic))) {
			RT_LOGE("pDynamicParam copy_from_user fail\n");
			return -EFAULT;
		}
		if (copy_from_user(&static_sharp, (void __user *)mSharpParam.pStaticParam, sizeof(sEncppSharpParamStatic))) {
			RT_LOGE("pStaticParam copy_from_user fail\n");
			return -EFAULT;
		}

		mSharpParam.pDynamicParam = &dynamic_sharp;
		mSharpParam.pStaticParam = &static_sharp;

		RT_LOGD("hfr_hf_wht_clp %d max_clp_ratio %d", mSharpParam.pDynamicParam->hfr_hf_wht_clp, mSharpParam.pDynamicParam->max_clp_ratio);
		RT_LOGD("ls_dir_ratio %d ss_dir_ratio %d", mSharpParam.pStaticParam->ls_dir_ratio, mSharpParam.pStaticParam->ss_dir_ratio);

		VencSetParameter(venc_comp->vencoder, VENC_IndexParamSharpConfig, &mSharpParam);
		VencSetParameter(venc_comp->vencoder, VENC_IndexParamEnableEncppSharp, &enableSharp);

		break;
	}
	default: {
		RT_LOGE("nor support config index");
		return ERROR_TYPE_ILLEGAL_PARAM;
		break;
	}
	}

	return error;
}

static int post_msg_and_wait(venc_comp_ctx *venc_comp, int cmd_id)
{
	message_t cmd_msg;
	memset(&cmd_msg, 0, sizeof(message_t));
	cmd_msg.command	= cmd_id;
	cmd_msg.wait_queue     = (char *)&venc_comp->wait_reply[cmd_id];
	cmd_msg.wait_condition = (char *)&venc_comp->wait_reply_condition[cmd_id];

	venc_comp->wait_reply_condition[cmd_id] = 0;

	put_message(&venc_comp->msg_queue, &cmd_msg);

	RT_LOGD("wait for : cmd[%d], start", cmd_id);
	wait_event(venc_comp->wait_reply[cmd_id],
		   venc_comp->wait_reply_condition[cmd_id]);
	RT_LOGD("wait for : cmd[%d], finish", cmd_id);

	return 0;
}

error_type venc_comp_init(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_init: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(venc_comp, COMP_COMMAND_INIT);
	return error;
}

error_type venc_comp_start(
	PARAM_IN comp_handle component)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);

	/* set param for vencoder here as:
	 *	vencoder_set_param will palloc vbv-buf, and we must make sure the pid
	 *	of 'palloc vbv-buf' code is the same as ioctl-calling, to valid ion share
	 *	between user and kenrl.
	 */
	vencoder_set_param(venc_comp);

	VencStart(venc_comp->vencoder);
	post_msg_and_wait(venc_comp, COMP_COMMAND_START);
	return error;
}

error_type venc_comp_pause(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_start: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencPause");
	VencPause(venc_comp->vencoder);

	post_msg_and_wait(venc_comp, COMP_COMMAND_PAUSE);

	return error;
}

error_type venc_comp_stop(
	PARAM_IN comp_handle component)
{
	error_type error = ERROR_TYPE_OK;

	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_stop: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("Ryan VencDestroy");
	//VencDestroy(venc_comp->vencoder);

	post_msg_and_wait(venc_comp, COMP_COMMAND_STOP);
	return error;
}

error_type venc_comp_destroy(
	PARAM_IN comp_handle component)
{
	int free_frame_cout		= 0;
	int free_stream_cout		= 0;
	video_stream_node *stream_node  = NULL;
	video_frame_node *frame_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_destroy: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	/*should make sure the thread do nothing importent */
	if (venc_comp->venc_thread)
		kthread_stop(venc_comp->venc_thread);

	message_destroy(&venc_comp->msg_queue);

	if (venc_comp->vencoder) {
		LOCK_MUTEX(&venc_mutex);
		VencDestroy(venc_comp->vencoder);
		UNLOCK_MUTEX(&venc_mutex);
	}

	while ((!list_empty(&venc_comp->in_buf_manager.empty_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
	}

	while ((!list_empty(&venc_comp->in_buf_manager.valid_frame_list))) {
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		if (frame_node) {
			list_del(&frame_node->mList);
			free_frame_cout++;
			kfree(frame_node);
		}
	}

	if (free_frame_cout != VENC_IN_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of frame node is not match: %d, %d", free_frame_cout, VENC_IN_BUFFER_LIST_NODE_NUM);

	RT_LOGD("free_frame_cout = %d", free_frame_cout);

	while ((!list_empty(&venc_comp->out_buf_manager.empty_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.empty_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			kfree(stream_node);
		}
	}

	while ((!list_empty(&venc_comp->out_buf_manager.valid_stream_list))) {
		stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);
		if (stream_node) {
			list_del(&stream_node->mList);
			free_stream_cout++;
			kfree(stream_node);
		}
	}

	if (free_stream_cout != VENC_OUT_BUFFER_LIST_NODE_NUM)
		RT_LOGE("free num of stream node is not match: %d, %d", free_stream_cout, VENC_OUT_BUFFER_LIST_NODE_NUM);

	RT_LOGD("free_stream_cout = %d", free_stream_cout);

	kfree(venc_comp);

	return error;
}

error_type venc_comp_get_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_INOUT void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}
	RT_LOGD("venc_comp_get_config : %d, start", index);
	switch (index) {
	case COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO: {
		KERNEL_VBV_BUFFER_INFO *param_vbv = (KERNEL_VBV_BUFFER_INFO *)param_data;
		VbvInfo vbv_info;
		int share_fd = 0;

		memset(&vbv_info, 0, sizeof(VbvInfo));

		if (venc_comp->vencoder_init_flag == 0) {
			RT_LOGD("get vbv info: vencoder not init");
			return ERROR_TYPE_ERROR;
		}

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		param_vbv->size = vbv_info.vbv_size;

		VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetVbvShareFd, &share_fd);
		param_vbv->share_fd = share_fd;

		RT_LOGW("get vbv info: share_fd = %d, size = %d", param_vbv->share_fd, param_vbv->size);
		break;
	}
	case COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER: {
		venc_comp_header_data *header_data = (venc_comp_header_data *)param_data;
		VencHeaderData sps_pps_data;
		memset(&sps_pps_data, 0, sizeof(VencHeaderData));

		if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_H264) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH264SPSPPS, &sps_pps_data);
		} else if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_H265) {
			VencGetParameter(venc_comp->vencoder, VENC_IndexParamH265Header, &sps_pps_data);
		} else {
			RT_LOGW("not support COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO yet");
			return ERROR_TYPE_ERROR;
		}
		RT_LOGD("1 get stream header = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[0],
			sps_pps_data.pBuffer[1],
			sps_pps_data.pBuffer[2],
			sps_pps_data.pBuffer[3],
			sps_pps_data.pBuffer[4],
			sps_pps_data.pBuffer[5],
			sps_pps_data.pBuffer[6],
			sps_pps_data.pBuffer[7]);
		RT_LOGD("2 get stream header size = %d, data = %x %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[8],
			sps_pps_data.pBuffer[9],
			sps_pps_data.pBuffer[10],
			sps_pps_data.pBuffer[11],
			sps_pps_data.pBuffer[12],
			sps_pps_data.pBuffer[13],
			sps_pps_data.pBuffer[14],
			sps_pps_data.pBuffer[15]);
		RT_LOGD("3 get stream header size = %d, data = %x %x %x %x %x %x %x", sps_pps_data.nLength,
			sps_pps_data.pBuffer[16],
			sps_pps_data.pBuffer[17],
			sps_pps_data.pBuffer[18],
			sps_pps_data.pBuffer[19],
			sps_pps_data.pBuffer[20],
			sps_pps_data.pBuffer[21],
			sps_pps_data.pBuffer[22]);

		header_data->pBuffer = sps_pps_data.pBuffer;
		header_data->nLength = sps_pps_data.nLength;

		break;
	}
#if 0	
	case COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA:
	{
		VencGetBinImageBufInfo venc_bin_buf_info;
		VideoGetBinImageBufInfo bin_buf_info;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA ");

		memset(&bin_buf_info, 0, sizeof(VideoGetBinImageBufInfo));
		memset(&venc_bin_buf_info, 0, sizeof(VencGetBinImageBufInfo));
		if (copy_from_user(&bin_buf_info, (void __user *)param_data, sizeof(struct VideoGetBinImageBufInfo))) {
			RT_LOGE("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA copy_from_user fail\n");
			return -EFAULT;
		}
		venc_bin_buf_info.buf = bin_buf_info.buf;
		venc_bin_buf_info.max_size = bin_buf_info.max_size;

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetBinImageData, &venc_bin_buf_info);
	}
	case COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA:
	{
		VencGetMvInfoBufInfo venc_mv_buf_info;
		VideoGetMvInfoBufInfo mv_buf_info;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA");

		memset(&mv_buf_info, 0, sizeof(VideoGetMvInfoBufInfo));
		memset(&venc_mv_buf_info, 0, sizeof(VencGetMvInfoBufInfo));

		if (copy_from_user(&mv_buf_info, (void __user *)param_data, sizeof(struct VideoGetMvInfoBufInfo))) {
			RT_LOGE("COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA copy_from_user fail\n");
			return -EFAULT;
		}

		venc_mv_buf_info.buf = mv_buf_info.buf;
		venc_mv_buf_info.max_size = mv_buf_info.max_size;

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamGetMvInfoData, &venc_mv_buf_info);
	}
#endif
	case COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT: {
		RTVencMotionSearchRegion *motion_result = (RTVencMotionSearchRegion *)param_data;

		RT_LOGI("COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT");

		return VencGetParameter(venc_comp->vencoder, VENC_IndexParamMotionSearchResult, motion_result);
	}
	default: {
		RT_LOGW("get config: not support the index = 0x%x", index);
		break;
	}
	}
	RT_LOGD("venc_comp_get_config : %d, finish", index);
	return error;
}

error_type venc_comp_set_config(
	PARAM_IN comp_handle component,
	PARAM_IN comp_index_type index,
	PARAM_IN void *param_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_config: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	switch (index) {
	case COMP_INDEX_VENC_CONFIG_Base: {
		venc_comp_base_config *base_config = (venc_comp_base_config *)param_data;
		memcpy(&venc_comp->base_config, base_config, sizeof(venc_comp_base_config));
		break;
	}
	case COMP_INDEX_VENC_CONFIG_Normal: {
		venc_comp_normal_config *normal_config = (venc_comp_normal_config *)param_data;
		memcpy(&venc_comp->normal_config, normal_config, sizeof(venc_comp_normal_config));
		break;
	}
	default: {
		error = config_dynamic_param(venc_comp, index, param_data);
		break;
	}
	}

	return error;
}

error_type venc_comp_get_state(
	PARAM_IN comp_handle component,
	PARAM_OUT comp_state_type *pState)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_get_state: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	*pState = venc_comp->state;
	return error;
}

/* empty_list --> valid_list */
error_type venc_comp_empty_this_in_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *buffer_header)
{
	video_frame_node *pNode		= NULL;
	video_frame_s *src_frame	= NULL;
	video_frame_node *first_node    = NULL;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	message_t msg;

	memset(&msg, 0, sizeof(message_t));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_empty_this_in_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	if (venc_comp->state != COMP_STATE_EXECUTING) {
		RT_LOGW("submit inbuf when venc state[0x%x] isn not executing", venc_comp->state);
	}

	src_frame = (video_frame_s *)buffer_header->private;

	if (src_frame == NULL) {
		RT_LOGE("submit in buf: frame is null");
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto submit_exit;
	}

	RT_LOGI("empty buf: pts = %lld", src_frame->pts);

	mutex_lock(&venc_comp->in_buf_manager.mutex);

	if (list_empty(&venc_comp->in_buf_manager.empty_frame_list)) {
		RT_LOGW("Low probability! venc idle frame is empty!");
		if (venc_comp->in_buf_manager.empty_num != 0) {
			RT_LOGE("fatal error! empty_num must be zero!");
		}

		pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			goto submit_exit;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
	}

	first_node = list_first_entry(&venc_comp->in_buf_manager.empty_frame_list, video_frame_node, mList);

	memcpy(&first_node->video_frame, src_frame, sizeof(video_frame_s));
	RT_LOGD("add valid_frame_list ");
	venc_comp->in_buf_manager.empty_num--;

	list_move_tail(&first_node->mList, &venc_comp->in_buf_manager.valid_frame_list);
#if 1
	static int cnt_1 = 1;
	if (cnt_1) {
		cnt_1 = 0;
		RT_LOGW("channel %d first time add buffer_node to valid_frame_list, time: %lld",
			venc_comp->base_config.channel_id, get_cur_time());
	}
#endif
	if (venc_comp->in_buf_manager.no_frame_flag) {
		venc_comp->in_buf_manager.no_frame_flag = 0;
		msg.command				      = COMP_COMMAND_VENC_INPUT_FRAME_VALID;
		put_message(&venc_comp->msg_queue, &msg);
	}

	mutex_unlock(&venc_comp->in_buf_manager.mutex);
submit_exit:

	return error;
}

/* valid_list --> empty_list */
error_type venc_comp_fill_this_out_buffer(
	PARAM_IN comp_handle component,
	PARAM_IN comp_buffer_header_type *pBuffer)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;
	video_stream_s *src_stream      = NULL;
	video_stream_node *stream_node  = NULL;
	VencOutputBuffer output_buffer;

	memset(&output_buffer, 0, sizeof(VencOutputBuffer));

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	src_stream = (video_stream_s *)pBuffer->private;

	RT_LOGD(" id = %d", src_stream->id);

	/* get the ready frame */
	mutex_lock(&venc_comp->out_buf_manager.mutex);

	if (list_empty(&venc_comp->out_buf_manager.valid_stream_list)) {
		RT_LOGE("the valid stream list is empty when return out buffer");
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return ERROR_TYPE_ERROR;
	}
	stream_node = list_first_entry(&venc_comp->out_buf_manager.valid_stream_list, video_stream_node, mList);

	if (src_stream->id != stream_node->video_stream.id) {
		RT_LOGE("fill this out buf: buf is not match, id = %d ,%d",
			src_stream->id, stream_node->video_stream.id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}

	memcpy(&stream_node->video_stream, src_stream, sizeof(video_stream_s));

	/* free bitstream frame to vencoder */
	config_VencOutputBuffer_by_videostream(venc_comp, &output_buffer, &stream_node->video_stream);

	LOCK_MUTEX(&venc_mutex);
	VencQueueOutputBuf(venc_comp->vencoder, &output_buffer);
	UNLOCK_MUTEX(&venc_mutex);

	list_move_tail(&stream_node->mList, &venc_comp->out_buf_manager.empty_stream_list);
	venc_comp->out_buf_manager.empty_num++;
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	return error;
}

#if 0
error_type venc_comp_get_filled_out_buffer(
		PARAM_IN  comp_handle component,
		PARAM_IN  comp_buffer_header_type *pBuffer)
{
	error_type error = ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *) component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_fill_this_out_buffer: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	RT_LOGE("venc_comp_get_filled_out_buffer not support");

	/* get the ready frame */
	mutex_lock(&venc_comp->out_buf_manager.mutex);

	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	return error;
}
#endif
error_type venc_comp_set_callbacks(
	PARAM_IN comp_handle component,
	PARAM_IN comp_callback_type *callback,
	PARAM_IN void *callback_data)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks: param error");
		return ERROR_TYPE_ILLEGAL_PARAM;
	}

	venc_comp->callback      = callback;
	venc_comp->callback_data = callback_data;
	return error;
}

error_type venc_comp_setup_tunnel(
	PARAM_IN comp_handle component,
	PARAM_IN comp_port_type port_type,
	PARAM_IN comp_handle tunnel_comp,
	PARAM_IN int connect_flag)
{
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)rt_component->component_private;

	if (rt_component == NULL || venc_comp == NULL || tunnel_comp == NULL) {
		RT_LOGE("venc_comp_set_callbacks, param error: %p, %p, %p",
			rt_component, venc_comp, tunnel_comp);
		error = ERROR_TYPE_ILLEGAL_PARAM;
		goto setup_tunnel_exit;
	}

	if (port_type == COMP_INPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("in port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->in_port_tunnel_info.valid_flag  = 1;
				venc_comp->in_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->in_port_tunnel_info.valid_flag == 1 && venc_comp->in_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->in_port_tunnel_info.valid_flag  = 0;
				venc_comp->in_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect in tunnel failed:  valid_flag = %d, tunnel_comp = %p, %p",
					venc_comp->in_port_tunnel_info.valid_flag,
					venc_comp->in_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	} else if (port_type == COMP_OUTPUT_PORT) {
		if (connect_flag == 1) {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1) {
				RT_LOGE("out port tunnel had setuped !!");
				error = ERROR_TYPE_ERROR;
				goto setup_tunnel_exit;
			} else {
				venc_comp->out_port_tunnel_info.valid_flag  = 1;
				venc_comp->out_port_tunnel_info.tunnel_comp = tunnel_comp;
			}
		} else {
			if (venc_comp->out_port_tunnel_info.valid_flag == 1 && venc_comp->out_port_tunnel_info.tunnel_comp == tunnel_comp) {
				venc_comp->out_port_tunnel_info.valid_flag  = 0;
				venc_comp->out_port_tunnel_info.tunnel_comp = NULL;
			} else {
				RT_LOGE("disconnect out tunnel failed:	valid_flag = %d, tunnel_comp = %p, %p",
					venc_comp->out_port_tunnel_info.valid_flag,
					venc_comp->out_port_tunnel_info.tunnel_comp, tunnel_comp);
				error = ERROR_TYPE_ERROR;
			}
		}
	}

setup_tunnel_exit:

	return error;
}

error_type venc_comp_component_init(PARAM_IN comp_handle component, int channel_id)
{
	int i				= 0;
	int ret				= 0;
	error_type error		= ERROR_TYPE_OK;
	rt_component_type *rt_component = (rt_component_type *)component;
	struct venc_comp_ctx *venc_comp = NULL;

	venc_comp = kmalloc(sizeof(struct venc_comp_ctx), GFP_KERNEL);
	if (venc_comp == NULL) {
		RT_LOGE("kmalloc for venc_comp failed");
		error = ERROR_TYPE_NOMEM;
		goto EXIT;
	}
	memset(venc_comp, 0, sizeof(struct venc_comp_ctx));

#if ENABLE_SAVE_NATIVE_OVERLAY_DATA
	if (global_osd_info == NULL) {
		global_osd_info = kmalloc(sizeof(struct native_osd_info), GFP_KERNEL);
		if (global_osd_info == NULL) {
			RT_LOGW("kmalloc for global_osd_info failed, size = %d", sizeof(struct native_osd_info));
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		} else
			memset(global_osd_info, 0, sizeof(struct native_osd_info));

		/* force set ori size to 2560x1440*/
		global_osd_info->ori_frame_w = 2560;
		global_osd_info->ori_frame_h = 1440;
		RT_LOGI("ori_frame_w = %d, h = %d", global_osd_info->ori_frame_w, global_osd_info->ori_frame_h);
	}
#endif

	ret = message_create(&venc_comp->msg_queue);
	if (ret < 0) {
		RT_LOGE("message create failed");
		error = ERROR_TYPE_ERROR;
		goto EXIT;
	}

	rt_component->component_private    = venc_comp;
	rt_component->init		   = venc_comp_init;
	rt_component->start		   = venc_comp_start;
	rt_component->pause		   = venc_comp_pause;
	rt_component->stop		   = venc_comp_stop;
	rt_component->destroy		   = venc_comp_destroy;
	rt_component->get_config	   = venc_comp_get_config;
	rt_component->set_config	   = venc_comp_set_config;
	rt_component->get_state		   = venc_comp_get_state;
	rt_component->empty_this_in_buffer = venc_comp_empty_this_in_buffer;
	rt_component->fill_this_out_buffer = venc_comp_fill_this_out_buffer;
	rt_component->set_callbacks	= venc_comp_set_callbacks;
	rt_component->setup_tunnel	 = venc_comp_setup_tunnel;

	venc_comp->base_config.channel_id = channel_id;
	venc_comp->self_comp = rt_component;
	venc_comp->state     = COMP_STATE_IDLE;

	for (i = 0; i < WAIT_REPLY_NUM; i++) {
		init_waitqueue_head(&venc_comp->wait_reply[i]);
	}

	init_waitqueue_head(&venc_comp->jpeg_cxt.wait_enc_finish);

	/* init in buf manager*/
	mutex_init(&venc_comp->in_buf_manager.mutex);
	venc_comp->in_buf_manager.empty_num = VENC_IN_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.empty_frame_list);
	INIT_LIST_HEAD(&venc_comp->in_buf_manager.valid_frame_list);
	for (i = 0; i < VENC_IN_BUFFER_LIST_NODE_NUM; i++) {
		video_frame_node *pNode = kmalloc(sizeof(video_frame_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_frame_node));
		list_add_tail(&pNode->mList, &venc_comp->in_buf_manager.empty_frame_list);
	}

	/* init out buf manager*/
	mutex_init(&venc_comp->out_buf_manager.mutex);
	venc_comp->out_buf_manager.empty_num = VENC_OUT_BUFFER_LIST_NODE_NUM;
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.empty_stream_list);
	INIT_LIST_HEAD(&venc_comp->out_buf_manager.valid_stream_list);
	for (i = 0; i < VENC_OUT_BUFFER_LIST_NODE_NUM; i++) {
		video_stream_node *pNode = kmalloc(sizeof(video_stream_node), GFP_KERNEL);
		if (NULL == pNode) {
			RT_LOGE("fatal error! kmalloc fail!");
			error = ERROR_TYPE_NOMEM;
			goto EXIT;
		}
		memset(pNode, 0, sizeof(video_stream_node));
		list_add_tail(&pNode->mList, &venc_comp->out_buf_manager.empty_stream_list);
	}
	RT_LOGS("channel_id %d venc_comp %p create thread_process_venc", venc_comp->base_config.channel_id, venc_comp);
	venc_comp->venc_thread = kthread_create(thread_process_venc,
						venc_comp, "venc comp thread");
	wake_up_process(venc_comp->venc_thread);

EXIT:

	return error;
}

#if TEST_ENCODER_BY_SELF
static void loop_encode_one_frame(struct venc_comp_ctx *venc_comp)
{
	VencInputBuffer in_buf;
	video_frame_s cur_video_frame;
	video_frame_node *frame_node = NULL;
	int result		     = 0;

	if (venc_comp->is_first_frame == 0) {
		mutex_lock(&venc_comp->in_buf_manager.mutex);
		if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
			RT_LOGD("valid frame list is null, wait for it  start");
			venc_comp->in_buf_manager.no_frame_flag = 1;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
			RT_LOGD("valid frame list is null, wait for it finish");
			return;
		}
		frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
		memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		memcpy(in_buf.pAddrVirY, cur_video_frame.vir_addr[0], cur_video_frame.buf_size);
		FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);

		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		RT_LOGW("first encode, ret = %d, buf_size = %d, vir_addr = %p, %p",
			result, cur_video_frame.buf_size,
			in_buf.pAddrVirY, cur_video_frame.vir_addr[0]);

		mutex_lock(&venc_comp->in_buf_manager.mutex);
		list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
		venc_comp->in_buf_manager.empty_num++;
		mutex_unlock(&venc_comp->in_buf_manager.mutex);

		venc_empty_in_buffer_done(venc_comp, &cur_video_frame);
		venc_comp->is_first_frame = 1;
	} else {
		GetOneAllocInputBuffer(venc_comp->vencoder, &in_buf);
		//int y_size = venc_comp->base_config.src_width*venc_comp->base_config.src_height;
		//memset(in_buf.pAddrVirY, 0x80, y_size);
		//memset(in_buf.pAddrVirC, 0x10, y_size/2);

		//FlushCacheAllocInputBuffer(venc_comp->vencoder, &in_buf);
		VencQueueInputBuf(venc_comp->vencoder, &in_buf);
		result = encodeOneFrame(venc_comp->vencoder);

		if (result != 0)
			msleep(20);
		RT_LOGD("**test copy encoder, result = %d", result);
	}
}
#endif


static int venc_comp_stream_buffer_to_valid_list(struct venc_comp_ctx *venc_comp, VencOutputBuffer *p_out_buffer)
{
	int result = 0;

	mutex_lock(&venc_comp->out_buf_manager.mutex);
	if (list_empty(&venc_comp->out_buf_manager.empty_stream_list)) {
		if (list_empty(&venc_comp->out_buf_manager.empty_stream_list))
			RT_LOGD("venc_comp %p channel_id %d empty list is null", venc_comp, venc_comp->base_config.channel_id);
		mutex_unlock(&venc_comp->out_buf_manager.mutex);
		return -1;
	}
	mutex_unlock(&venc_comp->out_buf_manager.mutex);

	LOCK_MUTEX(&venc_mutex);
	result = VencDequeueOutputBuf(venc_comp->vencoder, p_out_buffer);
	UNLOCK_MUTEX(&venc_mutex);
	if (result != 0) {
		RT_LOGD(" have no bitstream, result = %d", result);
		return -1;
	}
	RT_LOGI(" get bitstream , result = %d, pts = %lld", result, p_out_buffer->nPts);

	if (venc_comp->vbvStartAddr == NULL) {
		VbvInfo vbv_info;

		memset(&vbv_info, 0, sizeof(VbvInfo));
		VencGetParameter(venc_comp->vencoder, VENC_IndexParamVbvInfo, &vbv_info);
		venc_comp->vbvStartAddr = vbv_info.start_addr;
	}

	venc_fill_out_buffer_done(venc_comp, p_out_buffer);
	return 0;
}

static int thread_process_venc(void *param)
{
	struct venc_comp_ctx *venc_comp = (struct venc_comp_ctx *)param;
	message_t cmd_msg;
	int result		     = 0;
	video_frame_node *frame_node = NULL;
	VencOutputBuffer out_buffer;
	VencInputBuffer in_buf;
	int64_t time_start  = 0;
	int64_t time_finish = 0;
	video_frame_s cur_video_frame;

	memset(&cur_video_frame, 0, sizeof(video_frame_s));
	memset(&out_buffer, 0, sizeof(VencOutputBuffer));

	RT_LOGS("channel_id %d thread_process_venc start !", venc_comp->base_config.channel_id);
	while (1) {
	process_message:
		if (kthread_should_stop())
			break;

		if (get_message(&venc_comp->msg_queue, &cmd_msg) == 0) {
			commond_process(venc_comp, &cmd_msg);
			/* continue process message */
			continue;
		}

		if (venc_comp->state == COMP_STATE_EXECUTING) {
/* get the ready frame */
#if TEST_ENCODER_BY_SELF
			loop_encode_one_frame(venc_comp);
#else

#if DEBUG_SHOW_ENCODE_TIME
			static int cnt_0 = 1;
			if (cnt_0) {
			    cnt_0 = 0;
			    RT_LOGW("first time in execution state, time: %lld codec_type %d", get_cur_time(), venc_comp->base_config.codec_type);
			}
#endif
			if (venc_comp->base_config.codec_type == VENC_COMP_CODEC_JPEG
				&& VencGetValidOutputBufNum(venc_comp->vencoder)) {
				result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				RT_LOGD("ValidOutputBufNum %d result %d", VencGetValidOutputBufNum(venc_comp->vencoder), result);
			}
			/* get frame from valid_list */
			mutex_lock(&venc_comp->in_buf_manager.mutex);
			if (list_empty(&venc_comp->in_buf_manager.valid_frame_list)) {
				static int count = 0;
				if (count % 100 == 0)
					RT_LOGD("valid frame list is null, wait for it  start");
				venc_comp->in_buf_manager.no_frame_flag = 1;
				mutex_unlock(&venc_comp->in_buf_manager.mutex);
				TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
				if (count % 100 == 0) {
					RT_LOGD("valid frame list is null, wait for it finish");
				}
				count++;
				goto process_message;
			}
#if DEBUG_SHOW_ENCODE_TIME
			static int cnt_1 = 1;
			if (cnt_1) {
			    cnt_1 = 0;
			    RT_LOGW("channel %d first time found valid frame buffer, time: %lld",
					venc_comp->base_config.channel_id, get_cur_time());
			}
#endif
			frame_node = list_first_entry(&venc_comp->in_buf_manager.valid_frame_list, video_frame_node, mList);
			memcpy(&cur_video_frame, &frame_node->video_frame, sizeof(video_frame_s));

			list_move_tail(&frame_node->mList, &venc_comp->in_buf_manager.empty_frame_list);
			venc_comp->in_buf_manager.empty_num++;
			mutex_unlock(&venc_comp->in_buf_manager.mutex);

			/*process frame start*/
			memset(&in_buf, 0, sizeof(VencInputBuffer));
			in_buf.nID       = cur_video_frame.id;
			in_buf.nPts      = cur_video_frame.pts;
			in_buf.nFlag     = 0;
			in_buf.pAddrPhyY = (unsigned char *)cur_video_frame.phy_addr[0];
			in_buf.pAddrPhyC = (unsigned char *)cur_video_frame.phy_addr[1];
			in_buf.pAddrVirY = (unsigned char *)cur_video_frame.vir_addr[0];
			in_buf.pAddrVirC = (unsigned char *)cur_video_frame.vir_addr[1];

			RT_LOGD("phyY = %p, phyC = %p, %p, %p", in_buf.pAddrPhyY, in_buf.pAddrPhyC,
				in_buf.pAddrVirY, in_buf.pAddrVirC);
			if (venc_comp->jpeg_cxt.enable == 1 && venc_comp->jpeg_cxt.encoder_finish_flag == 0) {
				catch_jpeg_encoder(venc_comp, &in_buf);
			}

			VencCbType vencCallBack;
			vencCallBack.empty_in_buffer_done = venc_empty_in_buffer_done;
			vencCallBack.EventHandler = venc_comp_event_handler;

			VencSetCallbacks(venc_comp->vencoder, &vencCallBack, venc_comp, &cur_video_frame);

			result = VencQueueInputBuf(venc_comp->vencoder, &in_buf);
			if (result != 0) {
				RT_LOGE("fatal error! VencQueueInputBuf fail[%d]", result);
			}

/* process frame end */
#endif

			/* get bitstream */
			while (1) {
				result = venc_comp_stream_buffer_to_valid_list(venc_comp, &out_buffer);
				if (result != 0) {
					break;
				}
			}

		} else {
			static int count = 0;
			count++;
			if (count % 50 == 0) {
				RT_LOGD("TMessage_WaitQueueNotEmpty  !");
			}
			TMessage_WaitQueueNotEmpty(&venc_comp->msg_queue, 10);
		}
	}

	RT_LOGD("thread_process_venc finish !");
	return 0;
}
