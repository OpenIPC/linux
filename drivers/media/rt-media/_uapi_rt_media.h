
#ifndef __UAPI_RT_MEDIA_H_
#define __UAPI_RT_MEDIA_H_

#define ENABLE_SETUP_RECODER_IN_KERNEL (1)

#define COPY_STREAM_DATA_FROM_KERNEL (0)

enum RT_MEDIA_IOCTL_CMD {
	IOCTL_UNKOWN = 0x100,

	IOCTL_CONFIG, //+1
	IOCTL_START, //+2
	IOCTL_PAUSE, //+3
	IOCTL_DESTROY, //+4

	IOCTL_GET_VBV_BUFFER_INFO, //+5
	IOCTL_GET_STREAM_HEADER_SIZE, //+6
	IOCTL_GET_STREAM_HEADER_DATA, //+7

	IOCTL_GET_STREAM_DATA, //+8
	IOCTL_RETURN_STREAM_DATA, //+9

	IOCTL_CATCH_JPEG_START,
	IOCTL_CATCH_JPEG_GET_DATA,
	IOCTL_CATCH_JPEG_STOP,

	IOCTL_GET_YUV_FRAME,
	IOCTL_SET_OSD,
	IOCTL_GET_BIN_IMAGE_DATA,
	IOCTL_GET_MV_INFO_DATA,

	IOCTL_CONFIG_YUV_BUF_INFO,
	IOCTL_REQUEST_YUV_FRAME,
	IOCTL_RETURN_YUV_FRAME,

	IOCTL_GET_ISP_LV,
	IOCTL_GET_ISP_HIST,
	IOCTL_GET_ISP_EXP_GAIN,
	IOCTL_GET_ISP_AE_METERING_MODE,
	IOCTL_SET_IR_PARAM,
	IOCTL_SET_VERTICAL_FLIP,
	IOCTL_SET_HORIZONTAL_FLIP,
	IOCTL_SET_ISP_ATTR_CFG,

	IOCTL_SET_FORCE_KEYFRAME,
	IOCTL_RESET_ENCODER_TYPE,

	IOCTL_REQUEST_ENC_YUV_FRAME,
	IOCTL_SUBMIT_ENC_YUV_FRAME,

	IOCTL_SET_POWER_LINE_FREQ,
	IOCTL_SET_QP_RANGE,
	IOCTL_SET_BITRATE,
	IOCTL_SET_VBR_PARAM,
	IOCTL_GET_SUM_MB_INFO,

	IOCTL_GET_ENC_MOTION_SEARCH_RESULT,
	IOCTL_SET_VENC_SUPER_FRAME_PARAM,
	IOCTL_SET_FPS,
	IOCTL_SET_SHARP,
};

typedef struct video_qp_range {
	int i_min_qp;
	int i_max_qp;
	int p_min_qp;
	int p_max_qp;
	int i_init_qp;
	int enable_mb_qp_limit;
} video_qp_range;

typedef enum rt_pixelformat_type {
	RT_PIXEL_YUV420SP, //nv12
	RT_PIXEL_YVU420SP, //nv21
	RT_PIXEL_YUV420P,
	RT_PIXEL_YVU420P,
	RT_PIXEL_LBC_25X,
	RT_PIXEL_LBC_2X,
	RT_PIXEL_NUM
} rt_pixelformat_type;

typedef enum {
	XM_Video_H264ProfileBaseline = 66, /**< Baseline profile */
	XM_Video_H264ProfileMain     = 77, /**< Main profile */
	XM_Video_H264ProfileHigh     = 100, /**< High profile */
} XM_Video_H264PROFILETYPE;

/**
 * H264 level types
 */
typedef enum {
	XM_Video_H264Level1  = 10, /**< Level 1 */
	XM_Video_H264Level11 = 11, /**< Level 1.1 */
	XM_Video_H264Level12 = 12, /**< Level 1.2 */
	XM_Video_H264Level13 = 13, /**< Level 1.3 */
	XM_Video_H264Level2  = 20, /**< Level 2 */
	XM_Video_H264Level21 = 21, /**< Level 2.1 */
	XM_Video_H264Level22 = 22, /**< Level 2.2 */
	XM_Video_H264Level3  = 30, /**< Level 3 */
	XM_Video_H264Level31 = 31, /**< Level 3.1 */
	XM_Video_H264Level32 = 32, /**< Level 3.2 */
	XM_Video_H264Level4  = 40, /**< Level 4 */
	XM_Video_H264Level41 = 41, /**< Level 4.1 */
	XM_Video_H264Level42 = 42, /**< Level 4.2 */
	XM_Video_H264Level5  = 50, /**< Level 5 */
	XM_Video_H264Level51 = 51, /**< Level 5.1 */
} XM_Video_H264LEVELTYPE;

typedef enum {
	XM_Video_H265ProfileMain      = 1,
	XM_Video_H265ProfileMain10    = 2,
	XM_Video_H265ProfileMainStill = 3
} XM_VideoH265PROFILETYPE;

typedef enum {
	XM_Video_H265Level1  = 30, /**< Level 1 */
	XM_Video_H265Level2  = 60, /**< Level 2 */
	XM_Video_H265Level21 = 63, /**< Level 2.1 */
	XM_Video_H265Level3  = 90, /**< Level 3 */
	XM_Video_H265Level31 = 93, /**< Level 3.1 */
	XM_Video_H265Level41 = 123, /**< Level 4.1 */
	XM_Video_H265Level5  = 150, /**< Level 5 */
	XM_Video_H265Level51 = 153, /**< Level 5.1 */
	XM_Video_H265Level52 = 156, /**< Level 5.2 */
	XM_Video_H265Level6  = 180, /**< Level 6 */
	XM_Video_H265Level61 = 183, /**< Level 6.1 */
	XM_Video_H265Level62 = 186 /**< Level 6.2 */
} XM_Video_H265LEVELTYPE;

typedef struct KERNEL_VBV_BUFFER_INFO {
	int share_fd;
	int size;
} KERNEL_VBV_BUFFER_INFO;

enum VIDEO_OUTPUT_MODE {
	OUTPUT_MODE_STREAM	= 0, // yuv图像经过编码器输出码流数据；
	OUTPUT_MODE_YUV		  = 1, // 直接调用接口获取yuv数据
	OUTPUT_MODE_ONLY_ENCDODER = 2, // 调试用：不启用isp，应用层传递yuv给编码器进行编码
};

typedef struct {
	int pix_x_bgn;
	int pix_x_end;
	int pix_y_bgn;
	int pix_y_end;
	int total_num;
	int intra_num;
	int large_mv_num;
	int small_mv_num;
	int zero_mv_num;
	int large_sad_num;
	int is_motion;
} RTVencMotionSearchRegion;

typedef struct {
	int dis_default_para; //0: use default param, 1: use seting param by user
	int large_mv_th;
	int least_motion_region_num;
	int dis_search_region[50];
	float large_mv_th_coef;
	float large_mv_ratio_th; // include intra and large mv
	float non_zero_mv_ratio_th; // include intra, large mv and samll mv
	float large_sad_ratio_th;
} RTVencMotionSearchParam;

typedef struct rt_media_config_s {
	int channelId;
	int encodeType; //0)h264 1)mjpeg
	int width; // resolution width
	int height; // resolution height
	int fps; // fps   //1~30
	int bitrate; // h264 bitrate (kbps)
	int gop;
	int vbr; // VBR=1, CBR=0
	int profile;
	int level;
	int drop_frame_num;
	video_qp_range qp_range;
	enum VIDEO_OUTPUT_MODE output_mode;
	int enable_bin_image; // 0: disable, 1: enable
	int bin_image_moving_th;
	int enable_mv_info; // 0: disable, 1: enable

	/* about isp */
	int enable_wdr;

	rt_pixelformat_type pixelformat;
	RTVencMotionSearchParam motion_search_param; // should setup the param if use motion search by encoder
	int enable_overlay;
} rt_media_config_s;

//* dynamic config
typedef struct video_stream_s {
	int id;
	uint64_t pts;
	unsigned int flag;
	unsigned int size0;
	unsigned int size1;
	unsigned int size2;

	unsigned int offset0;
	unsigned int offset1;
	unsigned int offset2;

	int keyframe_flag;
	struct timeval tv;

	unsigned char *data0;
	unsigned char *data1;
	unsigned char *data2;

#if COPY_STREAM_DATA_FROM_KERNEL
	unsigned char *buf;
	int buf_size;
	int buf_valid_size;
#endif

} video_stream_s;

typedef struct catch_jpeg_config {
	int width;
	int height;
	int qp;
} catch_jpeg_config;

typedef struct catch_jpeg_buf_info {
	unsigned int buf_size; //* set the max_size, return valid_size
	unsigned char *buf;
} catch_jpeg_buf_info;

/* overlay related */
#define MAX_OVERLAY_ITEM_SIZE (8)

typedef enum OVERLAY_ARGB_TYPE {
	OVERLAY_ARGB_MIN = -1,
	OVERLAY_ARGB8888 = 0,
	OVERLAY_ARGB4444 = 1,
	OVERLAY_ARGB1555 = 2,
	OVERLAY_ARGB_MAX = 3,
} OVERLAY_ARGB_TYPE;

typedef struct OverlayItemInfo_t {
	unsigned short start_x;
	unsigned short start_y;
	unsigned int widht;
	unsigned int height;
	unsigned char *data_buf; //the vir addr of overlay block
	unsigned int data_size; //the size of bitmap
} OverlayItemInfo;

typedef struct VideoInputOSD_t {
	unsigned char osd_num; //num of overlay region
	OVERLAY_ARGB_TYPE argb_type; //reference define of VENC_ARGB_TYPE
	OverlayItemInfo item_info[MAX_OVERLAY_ITEM_SIZE];
} VideoInputOSD;

typedef struct VideoGetBinImageBufInfo {
	unsigned int max_size;
	unsigned char *buf;
} VideoGetBinImageBufInfo;

typedef struct VideoGetMvInfoBufInfo {
	unsigned int max_size;
	unsigned char *buf;
} VideoGetMvInfoBufInfo;

#define CONFIG_YUV_BUF_NUM (3)

typedef struct config_yuv_buf_info {
	unsigned char *phy_addr[CONFIG_YUV_BUF_NUM];
	int buf_size;
	int buf_num;
} config_yuv_buf_info;

enum RT_POWER_LINE_FREQUENCY {
	RT_FREQUENCY_DISABLED = 0,
	RT_FREQUENCY_50HZ     = 1,
	RT_FREQUENCY_60HZ     = 2,
	RT_FREQUENCY_AUTO     = 3,
};

enum RT_AE_METERING_MODE {
	RT_AE_METERING_MODE_AVERAGE = 0,
	RT_AE_METERING_MODE_CENTER  = 1,
	RT_AE_METERING_MODE_SPOT    = 2,
	RT_AE_METERING_MODE_MATRIX  = 3,
};

enum RT_ISP_CTRL_CFG_IDS {
	/*isp_ctrl*/
	RT_ISP_CTRL_MODULE_EN = 0,
	RT_ISP_CTRL_DIGITAL_GAIN,
	RT_ISP_CTRL_PLTMWDR_STR,
	RT_ISP_CTRL_DN_STR,
	RT_ISP_CTRL_3DN_STR,
	RT_ISP_CTRL_HIGH_LIGHT,
	RT_ISP_CTRL_BACK_LIGHT,
	RT_ISP_CTRL_WB_MGAIN,
	RT_ISP_CTRL_AGAIN_DGAIN,
	RT_ISP_CTRL_COLOR_EFFECT,
	RT_ISP_CTRL_AE_ROI,
	RT_ISP_CTRL_AF_METERING,
	RT_ISP_CTRL_COLOR_TEMP,
	RT_ISP_CTRL_EV_IDX,
};

typedef struct {
	unsigned int uMaxBitRate;
	unsigned int nMovingTh; //range[1,31], 1:all frames are moving,
	//            31:have no moving frame, default: 20
	int nQuality; //range[1,50], 1:worst quality, 10: common quality, 50:best quality
	int nIFrmBitsCoef; //range[1, 20], 1:worst quality, 20:best quality
	int nPFrmBitsCoef; //range[1, 50], 1:worst quality, 50:best quality
} RTVencVbrParam;

typedef struct {
	unsigned int sum_mad;
	unsigned int sum_qp;
	unsigned long long sum_sse;
	unsigned int avg_sse;
} RTVencMBSumInfo;

typedef struct {
	int grey; // 0:color    1: grey
	int ir_on;
	int ir_flash_on;
} RTIrParam;

typedef struct {
	int exp_val;
	int gain_val;
	int r_gain;
	int b_gain;
} RTIspExpGain;

typedef struct RTVencSuperFrameConfig {
	unsigned int bDropSuperFrame;
	unsigned int nMaxIFrameBits;
	unsigned int nMaxPFrameBits;
} RTVencSuperFrameConfig;

typedef struct {
	enum RT_ISP_CTRL_CFG_IDS isp_ctrl_id;
	int value;
} RTIspCtrlAttr;

#endif
