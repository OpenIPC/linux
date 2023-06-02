/**
 * @file kdrv_videoenc.h
 * @brief type definition of KDRV API.
 * @author ALG2
 * @date in the year 2018
 */

#ifndef __KDRV_VIDEOENC_H__
#define __KDRV_VIDEOENC_H__

#include "kwrap/type.h"
#include "kdrv_type.h"

typedef enum {
	KDRV_VDOENC_ID_1,                			        ///< video encoder ID 1
	KDRV_VDOENC_ID_2,                			        ///< video encoder ID 2
	KDRV_VDOENC_ID_3,                			        ///< video encoder ID 3
	KDRV_VDOENC_ID_4,                			        ///< video encoder ID 4
	KDRV_VDOENC_ID_5,                			        ///< video encoder ID 5
	KDRV_VDOENC_ID_6,                			        ///< video encoder ID 6
	KDRV_VDOENC_ID_7,                			        ///< video encoder ID 7
	KDRV_VDOENC_ID_8,                			        ///< video encoder ID 8
	KDRV_VDOENC_ID_9,                			        ///< video encoder ID 9
	KDRV_VDOENC_ID_10,               			        ///< video encoder ID 10
	KDRV_VDOENC_ID_11,               			        ///< video encoder ID 11
	KDRV_VDOENC_ID_12,               			        ///< video encoder ID 12
	KDRV_VDOENC_ID_13,               			        ///< video encoder ID 13
	KDRV_VDOENC_ID_14,               			        ///< video encoder ID 14
	KDRV_VDOENC_ID_15,               			        ///< video encoder ID 15
	KDRV_VDOENC_ID_16,               			        ///< video encoder ID 16
	KDRV_VDOENC_ID_MAX,              			        ///< video encoder ID maximum
	ENUM_DUMMY4WORD(KDRV_VDOENC_ID)
} KDRV_VDOENC_ID;

typedef enum {
	VDOENC_TYPE_JPEG = 0,
	VDOENC_TYPE_H264,
	VDOENC_TYPE_H265,
} KDRV_VDOENC_TYPE;

/*********** jpeg encode ************/
typedef enum {
	KDRV_JPEGYUV_FORMAT_422 = 0,			///< [r/w] JPEG input format is YUV422
	KDRV_JPEGYUV_FORMAT_420 = 1,			///< [r/w] JPEG input format is YUV420
	KDRV_JPEGYUV_FORMAT_100 = 4 			///< [r/w] JPEG input format is YUV100
} KDRV_JPEGYUV_FORMAT;

typedef struct {
	UINT32 retstart_interval;				///< [r/w] JPEG restart interval. default: 0, range: 0~65536
	UINT32 encode_width;					///< [r/w] JPEG encode width.
	UINT32 encode_height;					///< [r/w] JPEG encode height.
	KDRV_JPEGYUV_FORMAT in_fmt;				///< [r/w] JPEG input YUV format.0: YUV422,1:YUV420
} KDRV_JPEG_INFO;


/*********** video encode ************/
typedef enum {
	KDRV_VDOENC_DAR_DEFAULT,         		///< default video display aspect ratio (the same as encoded image)
	KDRV_VDOENC_DAR_16_9,            		///< 16:9 video display aspect ratio
	KDRV_VDOENC_DAR_MAX,             		///< maximum video display aspect ratio
	ENUM_DUMMY4WORD(KDRV_VDOENC_DAR)
} KDRV_H26XENC_DAR;

typedef enum {
	KDRV_VDOENC_SVC_DISABLE = 0,       		///< disable SVC
	KDRV_VDOENC_SVC_2X,          			///< enable  SVC_2X (SVC layer 1)
	KDRV_VDOENC_SVC_4X,          			///< enable  SVC_2X (SVC layer 2)
	KDRV_VDOENC_SVC_MAX,             		///< SVC setting maximum
	ENUM_DUMMY4WORD(KDRV_VDOENC_SVC)
} KDRV_VDOENC_SVC;

typedef enum {
	KDRV_VDOENC_PROFILE_BASELINE,
	KDRV_VDOENC_PROFILE_MAIN,
	KDRV_VDOENC_PROFILE_HIGH,
	KDRV_VDOENC_PROFILE_MAX,
} KDRV_H26XENC_PROFILE;

typedef enum {
	KDRV_VDOENC_CAVLC = 0,
	KDRV_VDOENC_CABAC,
	ENUM_DUMMY4WORD(KDRV_VDOENC_ENTROPY)
}KDRV_VDOENC_ENTROPY;

/************ rate control ************/
typedef enum {
	VDOENC_RC_MODE_CBR = 1,
	VDOENC_RC_MODE_VBR = 2,
	VDOENC_RC_MODE_FIX_QP = 3,
	VDOENC_RC_MODE_EVBR = 4
} KDRV_VDOENC_RC_MODE;

/************ quality level ************/
typedef enum {
	KDRV_VDOENC_QUALITY_MAIN = 0,	///< set tile config and search range as original policy
	KDRV_VDOENC_QUALITY_BASE		///< set tile config and search range as saving memory
} KDRV_VDOENC_QLVL;

typedef struct {
	UINT32 enable;				///< [r/w] enable CBR. 0: disable, 1: enable, defulat: 0
	UINT32 static_time;			///< [r/w] Rate control's static time.      default:  0, range: 0~20
	UINT32 byte_rate;			///< [r/w] Byte rate (byte rate per second)
	UINT32 frame_rate;			///< [r/w] Frame rate
	UINT32 gop;					///< [r/w] Gop Size
	UINT32 init_i_qp;			///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT32 min_i_qp;			///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT32 max_i_qp;			///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT32 init_p_qp;			///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT32 min_p_qp;			///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 max_p_qp;			///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	INT32  ip_weight;			///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
        UINT32 key_p_period;                    ///< [r/w] Key P frame interval.            default: 0, range: 0~4096
        INT32  kp_weight;                       ///< [r/w] Rate control's KP/P frame weight.default: 0, range: -100~100
        INT32  p2_weight;                       ///< [r/w] Rate control's SVC P2/P frame weight. default: 0, range: -100~100
        INT32  p3_weight;                       ///< [r/w] Rate control's SVC P3/P frame weight. default: 0, range: -100~100
        INT32  lt_weight;                       ///< [r/w] Rate control's long term P/P frame weight. default: 0, range: -100~100
        INT32  motion_aq_str;                   ///< [r/w] Motion aq strength for smart ROI. default: 0, range: -15~15
        UINT32 max_frame_size;                  ///< [r/w] max frame size. Default: 0, 0: limited by buffer size. Others: max size limited (byte)
	UINT32 svc_weight_mode;		///< [r/w] SVC weight mode: 0: weight = qp (origin SVC RC), 1: weight = bit size
} KDRV_VDOENC_CBR;

typedef struct {
	UINT32 enable;				///< [r/w] enable VBR. 0: disable, 1: enable, defulat: 0
	UINT32 static_time;			///< [r/w] Rate control's static time.      default:  0, range: 0~20
	UINT32 byte_rate;			///< [r/w] Byte rate (byte rate per second)
	UINT32 frame_rate;			///< [r/w] Frame rate
	UINT32 gop;					///< [r/w] Gop Size
	UINT32 init_i_qp;			///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT32 min_i_qp;			///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT32 max_i_qp;			///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT32 init_p_qp;			///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT32 min_p_qp;			///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 max_p_qp;			///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	INT32  ip_weight;			///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
	UINT32 change_pos;			///< [r/w] Early limit bitate.              default:  0, range: 0~100 (0: disable)
	UINT32 key_p_period;			///< [r/w] Key P frame interval.            default: 0, range: 0~4096
	INT32  kp_weight;			///< [r/w] Rate control's KP/P frame weight.default: 0, range: -100~100
	INT32  p2_weight;			///< [r/w] Rate control's SVC P2/P frame weight. default: 0, range: -100~100
	INT32  p3_weight;			///< [r/w] Rate control's SVC P3/P frame weight. default: 0, range: -100~100
	INT32  lt_weight;			///< [r/w] Rate control's long term P/P frame weight. default: 0, range: -100~100
	INT32  motion_aq_str;			///< [r/w] Motion aq strength for smart ROI. default: 0, range: -15~15
	UINT32 max_frame_size;			///< [r/w] max frame size. Default: 0, 0: limited by buffer size. Others: max size limited (byte)
	UINT32 policy;				///< [r/w] vbr policy. default: 0, 0: old, 1: new
	UINT32 svc_weight_mode;		///< [r/w] SVC weight mode: 0: weight = qp (origin SVC RC), 1: weight = bit size
} KDRV_VDOENC_VBR;

typedef struct {
	UINT32 enable;				///< [r/w] enable EVBR. 0: disable, 1: enable, defulat: 0
	UINT32 static_time;			///< [r/w] Rate control's static time.      default:  0, range: 0~20
	UINT32 byte_rate;			///< [r/w] Byte rate (byte rate per second)
	UINT32 frame_rate;			///< [r/w] Frame rate
	UINT32 gop;					///< [r/w] Gop Size
        UINT32 key_p_period;		///< [r/w] Key P frame interval.  default: frame rate*2, range: 0~4096
	UINT32 init_i_qp;			///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT32 min_i_qp;			///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT32 max_i_qp;			///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT32 init_p_qp;			///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT32 min_p_qp;			///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 max_p_qp;			///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	INT32  ip_weight;			///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
        INT32  kp_weight;		        ///< [r/w] Rate control's KP/P frame weight. default: 0, range: -100~100
        INT32  motion_aq_st;		        ///< [r/w] Motion aq strength for smart ROI. default: -6, range: -15~15
        UINT32 still_frm_cnd;		        ///< [r/w] Condition of still environment of EVBR. default: 100, range: 1~4096
        UINT32 motion_ratio_thd;	        ///< [r/w] Threshold of motion ratio to decide motion frame and still frame. default: 30, range: 1~100
	UINT32 i_psnr_cnd;			///< [r/w] Still mode qp of I frame.        default: 28, range: 0~51
	UINT32 p_psnr_cnd;			///< [r/w] Still mode qp of P frame.        default: 36, range: 0~51
	UINT32 kp_psnr_cnd;		///< [r/w] Still mode qp of key P frame.    default: 30, range: 0~51
        INT32  p2_weight;                       ///< [r/w] Rate control's SVC P2/P frame weight. default: 0, range: -100~100
        INT32  p3_weight;                       ///< [r/w] Rate control's SVC P3/P frame weight. default: 0, range: -100~100
        INT32  lt_weight;                       ///< [r/w] Rate control's long term P/P frame weight. default: 0, range: -100~100
        UINT32 max_frame_size;                  ///< [r/w] max frame size. Default: 0, 0: limited by buffer size. Others: max size limited (byte)
	UINT32 svc_weight_mode;		///< [r/w] SVC weight mode: 0: weight = qp (origin SVC RC), 1: weight = bit size
} KDRV_VDOENC_EVBR;

typedef struct {
	UINT32 enable;				///< [r/w] enable fixQP. 0: disable, 1: enable, defulat: 0
	UINT32 fix_i_qp;            ///< [r/w] Fix qp of I frame. default: 26, range: 0~51
	UINT32 fix_p_qp;            ///< [r/w] Fix qp of P frame. default: 26, range: 0~51
	UINT32 frame_rate;			///< [r/w] Frame rate
} KDRV_VDOENC_FIXQP;

typedef struct {
	KDRV_VDOENC_RC_MODE     rc_mode;    ///< rate control mode. default: 1, range: 1~4 (1: CBR, 2: VBR, 3: FixQP, 4: EVBR)
	union {
		KDRV_VDOENC_CBR     cbr;        ///< parameter of rate control mode CBR
		KDRV_VDOENC_VBR     vbr;        ///< parameter of rate control mode VBR
		KDRV_VDOENC_FIXQP   fixqp;      ///< parameter of rate control mode FixQP
		KDRV_VDOENC_EVBR    evbr;       ///< parameter of rate control mode EVBR
	} rc_param;
}KDRV_VDOENC_RATE_CONTROL;

/************ row rc ************/
typedef struct {
	BOOL            enable;             ///< [r/w] enable row rc. default: 1, range: 0~1 (0: disable, 1: enable)
	UINT8           i_qp_range;         ///< [r/w] qp range of I frame for row-level rata control. default: 2, range: 0~15
	UINT8           i_qp_step;          ///< [r/w] qp step  of I frame for row-level rata control. default: 1, range: 0~15
	UINT8           p_qp_range;         ///< [r/w] qp range of P frame for row-level rata control. default: 4, range: 0~15
	UINT8           p_qp_step;          ///< [r/w] qp step  of P frame for row-level rata control. default: 1, range: 0~15
	UINT8           min_i_qp;           ///< [r/w] min qp of I frame for row-level rata control. default:  1, range: 0~51
	UINT8           max_i_qp;           ///< [r/w] max qp of I frame for row-level rata control. default: 51, range: 0~51
	UINT8           min_p_qp;           ///< [r/w] min qp of P frame for row-level rata control. default:  1, range: 0~51
	UINT8           max_p_qp;           ///< [r/w] max qp of P frame for row-level rata control. default: 51, range: 0~51
} KDRV_VDOENC_ROW_RC;

/************ encode set param ************/
/********* user-defined QP map *******/
typedef struct {
	BOOL            enable;             ///< [w] enable user qp. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8*          qp_map_addr;        ///< [w] buffer address of user qp map. two bytes per cu16
											///< bit[0:5] qp value (default: 0; if qp mode is 3 then qp value means fixed qp [range: 0~51], otherwise qp value means delta qp [range: -32~31])
	UINT32			qp_map_size;		///<[w] [520]user qp map size
	UINT32			qp_map_loft;		///<[w] [520]user qp map lineoffset
}KDRV_VDOENC_USR_QP;

/************ aq ************/
typedef struct {
	BOOL            enable;             ///< [r/w] AQ enable. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8           i_str;              ///< [r/w] aq strength of I frame. default: 3, range: 1~8
	UINT8           p_str;              ///< [r/w] aq strength of P frame. default: 3, range: 1~8
	INT8            max_delta_qp;       ///< [r/w] max delta qp of aq. default: -6, range: 0 ~ 8
	INT8            min_delta_qp;       ///< [r/w] min delta qp of aq. default:  6, range:  -8 ~ 0
	BOOL            mode;                ///< [r/w] AQ mode. default: 0, 0: original, 1: dependent on cu size, HEVC only
	UINT8           i_str1;              ///< [r/w] aq strength of I frame of CU32. default: 3(same as i_str), range: 1~8, HEVC only
	UINT8           p_str1;              ///< [r/w] aq strength of P frame of CU32. default: 3(same as i_str), range: 1~8, HEVC only
	UINT8           i_str2;              ///< [r/w] aq strength of I frame of CU16. default: 1, range: 1~8, HEVC only
	UINT8           p_str2;              ///< [r/w] aq strength of P frame of CU16. default: 1, range: 1~8, HEVC only
} KDRV_VDOENC_AQ;

/************ roi ************/
typedef struct {
	BOOL            enable;             ///< [r/w] enable roi qp. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT16          coord_X;            ///< [r/w] coordinate x of roi. range: 0~encode width -1
	UINT16          coord_Y;            ///< [r/w] coordinate y of roi. range: 0~encode height-1
	UINT16          width;              ///< [r/w]  width of roi. range: 0~encode width -1
	UINT16          height;             ///< [r/w] height of roi. range: 0~encode height-1
	INT8            qp;                 ///< [r/w] qp value. default: 0; if qp mode is 3 then qp value means fixed qp (range: 0~51), otherwise qp value means delta qp (range: -32~31)
	UINT8			qp_mode;			///< [r/w] [520]qp mode , TODO
} KDRV_VDOENC_ROI_WIN;

typedef struct {
	//UINT8           roi_qp_mode;        ///< [r/w][520][RESERVED, remove to each window] roi qp mode.  default: 0, range: 0~1 (0: delta qp, 1: fixed qp)
	KDRV_VDOENC_ROI_WIN    st_roi[10]; ///< [r/w] roi window settings. ROIs can be overlaid, and the priority of the ROIs is based on index number, index 0 is highest priority and index 9 is lowest.
	UINT32 uiDeltaQp;	// [RESERVED] //
	UINT32 uiRoiCount;	// [RESERVED] //
} KDRV_VDOENC_ROI;

/************ multi slice ************/
typedef struct  {
	BOOL	enable;            			///< [r/w] enable multiple slice. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT32	slice_row_num;     			///< [r/w] number of macroblock/ctu rows occupied by a slice, range: 1 ~ number of macroblock/ctu row
} KDRV_VDOENC_SLICE_SPLIT;

/************ gdr ************/
typedef struct {
	BOOL            enable;             ///< [r/w] enable gdr. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT32          period;             ///< [r/w] intra refresh period. default: 0, range: 0~0xFFFFFFFF (0: always refresh, others: intra refresh frame period)
	UINT32          number;             ///< [r/w] intra refresh row number. default: 1, range: 1 ~ number of macroblock/ctu row
} KDRV_VDOENC_GDR;

/************ source decompression  ************/
typedef struct {
	BOOL enable;						///< [r/w] y and cbcr decompression enable.
	UINT32 width;
	UINT32 height;
	UINT32 y_lofst;
	UINT32 c_lofst;
} KDRV_VDOENC_SDC;

/************ osg ************/
typedef struct {
	UINT8  rgb2yuv[3][3];
} KDRV_VDOENC_OSG_RGB;

typedef struct {
	UINT8 idx;

	UINT8 alpha;
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} KDRV_VDOENC_OSG_PAL;

typedef struct {
	UINT8  type;
	UINT16 width;
	UINT16 height;
	UINT16 line_offset;
	UINT32 addr;
} KDRV_VDOENC_OSG_GRAP;

typedef struct {
	UINT8  mode;
	UINT16 str_x;
	UINT16 str_y;
	UINT8  bg_alpha;
	UINT8  fg_alpha;
	UINT8  mask_type;
	UINT8  mask_bd_size;
	UINT8  mask_y[2];
	UINT8  mask_cb;
	UINT8  mask_cr;
} KDRV_VDOENC_OSG_DISP;

typedef struct {
	BOOL enable;

	UINT8 blk_width;
	UINT8 blk_height;
	UINT8 blk_num;
	UINT8 org_color_level;
	UINT8 inv_color_level;
	UINT8 nor_diff_th;
	UINT8 inv_diff_th;
	UINT8 sta_only_mode;
	UINT8 full_eval_mode;
	UINT8 eval_lum_targ;
} KDRV_VDOENC_OSG_GCAC;

typedef struct {
	UINT8 lpm_mode;
	UINT8 tnr_mode;
	UINT8 fro_mode;
	UINT8 qp_mode;
	INT8  qp;
} KDRV_VDOENC_OSG_QPMAP;

typedef struct {
	BOOL  enable;
	BOOL  alpha_en;
	UINT8 alpha;
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} KDRV_VDOENC_OSG_COLOR_KEY;

typedef struct {
	UINT8 layer_idx;
	UINT8 win_idx;

	BOOL enable;

	KDRV_VDOENC_OSG_GRAP st_grap;
	KDRV_VDOENC_OSG_DISP st_disp;
	KDRV_VDOENC_OSG_GCAC st_gcac;
	KDRV_VDOENC_OSG_QPMAP st_qp_map;
	KDRV_VDOENC_OSG_COLOR_KEY st_key;
} KDRV_VDOENC_OSG_WIN;

typedef struct {
	UINT32 motion_buf_addr[3];
	UINT32 motion_buf_loft;
	UINT8  motion_buf_num;
} KDRV_VDOENC_MOT_ADDR;

typedef struct {
	UINT8  nr_3d_mode;						///< [r/w] codec tnr enable/disable, range: 0 ~ 1 (0: disable, 1: enable)
	UINT8  tnr_osd_mode;					/// RESERVED
	UINT8  mctf_p2p_pixel_blending;			///< [r/w] codec tnr blending enable, range: 0 ~ 1 (0: disable, 1: enable)
	UINT8  tnr_p2p_sad_mode;				///< [r/w] codec tnr p2p sad mode, range: 0 ~ 3 (0: pixel, 1: 8x8, 2:16x16, 3:auto)
	UINT8  tnr_mctf_sad_mode;				///< [r/w] codec tnr mctf sad mode, range: 0 ~ 3 (0: pixel, 1: 8x8, 2:16x16, 3:auto)
	UINT8  tnr_mctf_bias_mode;				///< [r/w] codec tnr mctf bias mode, range: 0 ~ 3 (0: pixel, 1: 8x8, 2:16x16, 3:auto)

	UINT8  nr_3d_adp_th_p2p[3];				///< [r/w] codec tnr p2p sad threshold, range: 0 ~ 255
	UINT8  nr_3d_adp_weight_p2p[3];			///< [r/w] codec tnr p2p weight, range: 0 ~ 16
	UINT8  tnr_p2p_border_check_th;			///< [r/w] codec tnr p2p border check threshold, range: 0 ~ 255
	UINT8  tnr_p2p_border_check_sc;			///< [r/w] codec tnr p2p border check scale, range: 0 ~ 7
	UINT8  tnr_p2p_input;					///< [r/w] codec tnr p2p input, range: 0 ~ 1 (0: original source , 1: mctf result)
	UINT8  tnr_p2p_input_weight;			///< [r/w] codec tnr p2p input weight, range: 0 ~ 3
	UINT8  cur_p2p_mctf_motion_th;			///< [r/w] codec tnr p2p mctf current motion threshold, range: 0 ~ 255
	UINT8  ref_p2p_mctf_motion_th;			///< [r/w] codec tnr p2p mctf reference motion threshold, range: 0 ~ 255
	UINT8  tnr_p2p_mctf_motion_wt[4];		///< [r/w] codec tnr p2p mctf motion weight, range: 0 ~ 3

	UINT8  nr3d_temporal_spatial_y[3];		///< [r/w] codec tnr p2p reference luma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_spatial_c[3];		///< [r/w] codec tnr p2p reference chroma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_range_y[3];		///< [r/w] codec tnr p2p luma range, range: 0 ~ 255
	UINT8  nr3d_temporal_range_c[3];		///< [r/w] codec tnr p2p chroma range, range: 0 ~ 255
	UINT8  nr3d_clampy_th;					///< [r/w] codec tnr p2p clamp luma threshold, range: 0 ~ 255
	UINT8  nr3d_clampy_div;					///< [r/w] codec tnr p2p clamp luma lsr, range: 0 ~ 7
	UINT8  nr3d_clampc_th;					///< [r/w] codec tnr p2p clamp chroma threshold, range: 0 ~ 255
	UINT8  nr3d_clampc_div;					///< [r/w] codec tnr p2p clamp chroma lsr, range: 0 ~ 7

	UINT8  nr3d_temporal_spatial_y_mctf[3];	///< [r/w] codec tnr mctf reference luma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_spatial_c_mctf[3];	///< [r/w] codec tnr mctf reference chroma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_range_y_mctf[3];	///< [r/w] codec tnr mctf luma range, range: 0 ~ 255
	UINT8  nr3d_temporal_range_c_mctf[3];	///< [r/w] codec tnr mctf chroma range, range: 0 ~ 255
	UINT8  nr3d_clampy_th_mctf;				///< [r/w] codec tnr mctf clamp luma threshold, range: 0 ~ 255
	UINT8  nr3d_clampy_div_mctf;			///< [r/w] codec tnr mctf clamp luma lsr, range: 0 ~ 7
	UINT8  nr3d_clampc_th_mctf;				///< [r/w] codec tnr mctf clamp chroma threshold, range: 0 ~ 255
	UINT8  nr3d_clampc_div_mctf;			///< [r/w] codec tnr mctf clamp chroma lsr, range: 0 ~ 7

	UINT8  cur_motion_rat_th;				///< [r/w] codec tnr ratio threshold for current motion, range: 0 ~ 15
	UINT8  cur_motion_sad_th;				///< [r/w] codec tnr sad threshold for current motion, range: 0 ~ 255
	UINT8  ref_motion_twr_p2p_th[2];		///< [r/w] codec tnr refresh threshold current motion for  p2p's temporal weight, range: 0 ~ 255
	UINT8  cur_motion_twr_p2p_th[2];		///< [r/w] codec tnr refresh threshold reference motion for  p2p's temporal weight, range: 0 ~ 255
	UINT8  ref_motion_twr_mctf_th[2];		///< [r/w] codec tnr refresh threshold current motion for  mctf's temporal weight, range: 0 ~ 255
	UINT8  cur_motion_twr_mctf_th[2];		///< [r/w] codec tnr refresh threshold reference motion for  mctf's temporal weight, range: 0 ~ 255
	UINT8  nr3d_temporal_spatial_y_1[3];	///< [r/w] codec tnr p2p reference luma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_spatial_c_1[3];	///< [r/w] codec tnr p2p reference chroma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_spatial_y_mctf_1[3];	///< [r/w] codec tnr mctf reference luma spatial, range: 0 ~ 7
	UINT8  nr3d_temporal_spatial_c_mctf_1[3];	///< [r/w] codec tnr mctf reference chroma spatial, range: 0 ~ 7

	UINT8  sad_twr_p2p_th [2];				///< [r/w] codec tnr the refresh threshold of p2p sad for  p2p's temporal weight, range: 0 ~ 255
	UINT8  sad_twr_mctf_th[2];				///< [r/w] codec tnr the refresh threshold of mctf sad for mctf's temporal weight, range: 0 ~ 255
} KDRV_VDOENC_3DNR;

typedef struct {
	void (*vdoenc_3dnr_cb)(UINT32 path_id, UINT32 config);
	UINT32 id;
} KDRV_VDOENC_3DNRCB;

/************ rdo ************/
typedef enum {
	VDOENC_RDO_CODEC_264 = 0,
	VDOENC_RDO_CODEC_265 = 1
} KDRV_VDOENC_RDO_CODEC;

typedef struct {
	UINT8 avc_intra_4x4_cost_bias;			// [r/w] default: 8, range: 0~31, //INTRA 4x4 block number decrease as the value increase
	UINT8 avc_intra_8x8_cost_bias;			// [r/w] default: 8, range: 0~31, //INTRA 8x8 block number decrease as the value increase
	UINT8 avc_intra_16x16_cost_bias;		// [r/w] default: 8, range: 0~31, //INTRA 16x16 block number decrease as the value increase
	UINT8 avc_inter_tu4_cost_bias;			// [r/w] default: 8, range: 0~31, //INTER TU4 block number decrease as the value increase
	UINT8 avc_inter_tu8_cost_bias;			// [r/w] default: 8, range: 0~31, //INTER TU8 block number decrease as the value increase
	UINT8 avc_inter_skip_cost_bias;			// [r/w] default: 8, range: 0~31, //Skip mode number decrease as the value increase
} KDRV_VDOENC_RDO_264;

typedef struct {
	UINT8 hevc_intra_32x32_cost_bias;		// [r/w] default: 0, range: 0~15, //INTRA 32x32 block number decrease as the value increase
	UINT8 hevc_intra_16x16_cost_bias;		// [r/w] default: 0, range: 0~15, //INTRA 16x16 block number decrease as the value increase
	UINT8 hevc_intra_8x8_cost_bias;			// [r/w] default: 0, range: 0~15, //INTRA 8x8 block number decrease as the value increase
	INT8  hevc_inter_skip_cost_bias;		// [r/w] default: 0, range: -16~15, //skip mode number decrease as the value increase
	INT8  hevc_inter_merge_cost_bias;		// [r/w] default: 0, range: -16~15, //merge mode number decrease as the value increase
	UINT8 hevc_inter_64x64_cost_bias;		// [r/w] default: 14, range: 0~31, //INTER 64x64 block number decrease as the value increase
	UINT8 hevc_inter_64x32_32x64_cost_bias;		// [r/w] default: 28, range: 0~31, //INTER 64x32 and 32x64 block number decrease as the value increase
	UINT8 hevc_inter_32x32_cost_bias;		// [r/w] default: 14, range: 0~31, //INTER 32x32 block number decrease as the value increase
	UINT8 hevc_inter_32x16_16x32_cost_bias;		// [r/w] default: 28, range: 0~31, //INTER 32x16 and 16x32 block number decrease as the value increase
	UINT8 hevc_inter_16x16_cost_bias;		// [r/w] default: 7, range: 0~31, //INTER 16x16 block number decrease as the value increase
} KDRV_VDOENC_RDO_265;

typedef struct {
	KDRV_VDOENC_RDO_CODEC		rdo_codec;	///< rdo codec. 0: h264, 1: h265
	union {
		KDRV_VDOENC_RDO_264	rdo_264;        ///< parameter of h264 rdo
		KDRV_VDOENC_RDO_265	rdo_265;        ///< parameter of h265 rdo
	} rdo_param;
}KDRV_VDOENC_RDO;

/************ jnd ************/
typedef struct {
	BOOL            enable;				///< [r/w] jnd enable. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8           str;				///< [r/w] jnd filter strength. default: 10, range: 0~15
	UINT8           level;				///< [r/w] jnd edge level. default: 12, range: 0~15
	UINT8           threshold;			///< [r/w] jnd CM edge detection threshold. default: 25, range: 0~255
	UINT8		ucCStr;				///< [r/w] jnd chroma filter strength. default: 10(same as luma str), range: 0~15
	UINT8		ucR5Flag;			///< [r/w] jnd chroma R5 flag. default: 0, 0: R5, 1: iR5
	UINT8		ucLsigmaTh;			///< [r/w] jnd limit sigma maxG threshold. default: 10, range: 0~255
	UINT8		ucLsigma;			///< [r/w] jnd limit sigma. default: 10, range: 0~255
} KDRV_VDOENC_JND;

typedef struct {
	UINT32 addr;
	UINT32 size;
} KDRV_VDOENC_LL_MEM;
/************ end encode set param ************/

/************ encode get param ************/
/************ get memory info ********/
typedef struct {
	UINT32 width;							///< [w] encode frame width
	UINT32 height;							///< [w] encode frame height
	UINT32 svc_layer;						///< [w] encode svc layer
	UINT32 ltr_interval;					///< [w] encode long-term interval
	BOOL   tile_mode_en;                    ///< [w] encode tile mode enable
	BOOL   d2d_mode_en;                     ///< [w] encode d2d mode enable
	BOOL   gdc_mode_en;                     ///< [w] encode gdc mode enable
	UINT32 size;							///< [r]     return encode memory require size
	BOOL   colmv_en;                        ///< [w] encode colocated mv enable
	BOOL   comm_recfrm_en;                  ///< [w] encode reconstruct frame from common pool
	UINT32 recfrm_size;                     ///< [w] encode  common reconstruct frame size
	UINT32 recfrm_num;                      ///< [w] encode  common reconstruct frame number
	KDRV_VDOENC_QLVL quality_level;         ///< [w] encode quality level for setting search range and tile config
	UINT32 ltr_bs_buffer;			///< [w] encode long-term internal bs buffer size
	KDRV_VDOENC_TYPE codec_type;			///< [w] encode codec_type , only support h264/h265, not support jpeg
} KDRV_VDOENC_MEM_INFO;

/************ get sps/pps ********/
typedef struct {
	UINT32 addr;
	UINT32 size;
}KDRV_VDOENC_DESC;

typedef struct {
    UINT8    ratio_base;
    UINT8    edge_ratio;     ///< [r] edge str = origin edge str * edge ratio / ratio_base
    UINT8    dn_2d_ratio;    ///< [r] 2dnr str = origin 2dnr str * 2dnr str / ratio_base
    UINT8    dn_3d_ratio;    ///< [r] 3dnr str = origin 3dbr str * 3dnr str / ratio_base
} KDRV_VDOENC_ISP_RATIO;

typedef struct {
    void (*vdoenc_isp_cb)(UINT32 path_id, UINT32 config);
    UINT32   id;
} KDRV_VDOENC_ISPCB;

typedef struct {
	UINT32 size;
} KDRV_VDOENC_LL_MEM_INFO;

typedef struct {
	UINT16 align_w;						///< [r/w] source out yuv align width
	UINT16 align_h;						///< [r/w] source out yuv align height
} KDRV_VDOENC_SRCOUTYUV_WH;

/************ end encode get param ************/




/************ RESERVED  *****************/
/************ source decompress *****************/
typedef struct {
        UINT32 sideinfo_addr;                                   ///< [w] source decompress sideinfo addr
        UINT32 ktable_0;                                                ///< [w] source decompress ktable0
        UINT32 ktable_1;                                                ///< [w] source decompress ktable1
        UINT32 ktable_2;                                                ///< [w] source decompress ktable2
        UINT32 stripe_num;                                              ///< [w] source decompress stripe number
        UINT32 stripe_01_size;                                  ///< [w] source decompress stripe01 size, [12:0] stripe0 size, [28:16] stripe1 size
        UINT32 stripe_23_size;                                  ///< [w] source decomppres stripe23 size, [12:0] stripe2 size, [28:16] stripe3 size
} KDRV_H26XENC_SRC_COMPRESS_INFO;

/************ smart roi ************/
typedef struct {
	UINT32 class;						///< [r/w] smart roi window class type.
	UINT32 scoure;						///< [r/w] smart roi window score.
	UINT32 coord_x;						///< [r/w] coordinate x of roi. range: 0~encode width -1
	UINT32 coord_y;						///< [r/w] coordinate y of roi. range: 0~encode height-1
	UINT32 width;			            ///< [r/w]  width of roi. range: 0~encode width -1
	UINT32 height;						///< [r/w] height of roi. range: 0~encode height-1
} KDRV_H26XENC_SMART_ROI_WIN;

typedef struct {
	UINT32 roi_num;					///< [w] smart roi window number. default: 0, range: 0 ~ 9
	KDRV_H26XENC_SMART_ROI_WIN roi_win[10];	///< [w] smart roi window settings.
} KDRV_H26XENC_SMART_ROI_INFO;

/************ MD bitmap info ************/
typedef struct {
    UINT32 md_width;
    UINT32 md_height;
    UINT32 md_lofs;
    UINT32 md_buf_adr;
    UINT32 rotation;
} KDRV_VDOENC_MD_INFO;

/************ end RESERVED  *****************/

/*********** encode init ***********/
typedef struct {
    BOOL  bEnable;          ///< [r/w] Enable sharpen
    UINT8 ucConEng;         ///< [r/w] The weight of the local contrast  0~8
    UINT8 ucSlopConEng;     ///< [r/w] Transition region slop 0~255
    UINT8 ucBHC;            ///< [r/w] Bright halo clip ratio 0~128
    UINT8 ucDHC;            ///< [r/w] Dark halo clip ratio 0~128
    UINT8 ucEWT;            ///< [r/w] Edge weight coring threshold 0~255
    UINT8 ucEWG;            ///< [r/w] Edge weight gain 0~255
    UINT8 ucEdgeSharpStr1;  ///< [r/w] Sharpen strength 1 of edge region 0~255
    UINT8 ucCT;             ///< [r/w] Coring threshold 0~255
    UINT8 ucNL;             ///< [r/w] Noise level 0~255
    UINT8 ucBIG;            ///< [r/w] Blending ratio of HPF results 0~128
    UINT16 usFlatTh;        ///< [r/w] flat region threshold 0~2047
    UINT16 usEdgeTh;        ///< [r/w] Edge region threshold 0~2047
    UINT8 ucLowWt;          ///< [r/w] Region classification weight threshold 1 0~16
    UINT8 ucHightWt;        ///< [r/w] Region classification weight threshold 2 0~16
    UINT16 usHLDFlatTh;     ///< [r/w] High light flat region threshold 0~2047
    UINT16 usHLDEdgeTh;     ///< [r/w] High light edge threshold 0~2047
    UINT8 ucHLDLowWt;       ///< [r/w] High light reigon classification weight threshold 1 0~16
    UINT8 ucHLDHightWt;     ///< [r/w] High light region classification weight threshold 2 0~16
    UINT8 ucEdgeStr;        ///< [r/w] Edge weight strength 0~255
    UINT8 ucTransitionStr;  ///< [r/w] Transition region weight strength 0~255
    UINT8 ucMotionStr;      ///< [r/w] Motion region edge weight 0~255
    UINT8 ucStaticStr;      ///< [r/w] Static region edge weight strength 0~255
    UINT8 ucHLDSlopConEng;  ///< [r/w] High light transition region slop 0~255
    UINT8 ucFlatStr;        ///< [r/w] Flag region weight strength 0~16
    UINT16 usHLDLumTh;      ///< [r/w] High light luminance  threshold 0~255
    UINT8 ucNC[17];         ///< [r/w] 17 control points of noise modulation curve
} KDRV_H26XENC_SPN;

typedef struct {
	void (*vdoenc_spn_cb)(UINT32 path_id, UINT32 config);
	UINT32 id;
} KDRV_VDOENC_SPNCB;
typedef struct {
	UINT32 buf_addr;						///< [w] encode buffer addr
	UINT32 buf_size;                        ///< [w] encode buffer size
	UINT32 width;                           ///< [w] frame uiWidth
	UINT32 height;                          ///< [w] frame uiHeight
	UINT32 byte_rate;                       ///< [w] byte rate
	UINT32 frame_rate;                      ///< [w] frame rate
	UINT32 gop;								///< [w] GOP
	UINT32 init_i_qp;                       ///< [w] I Qp initial value
	UINT32 min_i_qp;                        ///< [w] I Qp min value
	UINT32 max_i_qp;                        ///< [w] I Qp max value
	UINT32 init_p_qp;                       ///< [w] P Qp initial value
	UINT32 min_p_qp;                        ///< [w] P Qp min value
	UINT32 max_p_qp;                        ///< [w] P Qp max value
	UINT32 user_qp_en; 						///< [w] use user Qp (0:default, 1:enable)
	UINT32 static_time;       				///< [w] rc static time (unit: sec)
	INT32  ip_weight;						///< [w] rc I/P weight
	KDRV_H26XENC_DAR e_dar;                 ///< [w] video display aspect ratio
	KDRV_VDOENC_SVC e_svc;                 ///< [w] SVC Layer
	UINT32 ltr_interval;      				///< [w] long-term reference frame interval (range: 0 ~ 4095)
	UINT32 ltr_pre_ref;                     ///< [w] long-term reference setting (0:all long-term reference to IDR frame, 1:reference latest long-term reference frame)
	UINT32 rotate;           	            ///< [w] rotate (0:disable, 1:CCW, 2:CW)
	UINT32 jpeg_yuv_format;                 ///< [w] YUV format for jpeg
	UINT32 fast_search; 					///< [w] fast Search (0:small range, 1:big range)
	KDRV_H26XENC_PROFILE e_profile;			///< [w] profile
	UINT32 multi_layer;						///< [w] multi temporary layer
	UINT32 color_range; 					///< [w] color range (0:tv range (15-235), 1:full range (0-255))
	UINT32 project_mode;					///< [w] prj mode for brc (0:IPCam, 1:DVCam)
	UINT32 sei_idf_en;						///< [w] add SEI ID data (0: disable, 1:enable)
	KDRV_VDOENC_ENTROPY e_entropy;			///< entropy mode (0 : cavlc (H264 only), 1 : cabac)
	UINT8  level_idc;						///< level idc
	UINT32 sar_width;						///< Sample Aspect Ratio width, ex: 1920 x 1080, width = 1920
	UINT32 sar_height;						///< Sample Aspect Ratio height, ex: 1920 x 1080, height = 1080 (not 1088)
	BOOL   gray_en;							///< encode gray enable (0 : disable , 1 : enable)
	UINT8  disable_db;						///< disable_filter_idc (1'b0=Filter, 1'b1=No Filter, 2'b10 = Slice Mode, 3'b100 = Tile Mode(h265 only))
	INT8   db_alpha; 		  				///< deblocking Alpha & C0 offset div. 2, {-6, -5, ... 0, +1, .. +6}
	INT8   db_beta;			  				///< deblocking Beta offset div. 2, {-6, -5, ... 0, +1, .. +6}
	BOOL   bVUIEn;							///< encode VUI header(0:disable, 1 : enable)
	UINT8  matrix_coef;						///< matrix_coef of VUI header
	UINT8  transfer_characteristics;		///< transfer_characteristics of VUI header
	UINT8  colour_primaries;				///< colour_primaries of VUI header
	UINT8  video_format;					///< video_format of VUI header
	BOOL   time_present_flag;				///< time_present_flag of VUI header, if frame_rate = 0, default disable time_present_flag
	// 680
	UINT32 uiRecCompressEn;					///< [NOT USE, PLEASE USE bFBCEn]H26x reconstruct compression (ECLS)
	// [520] //
	BOOL bFBCEn;							///< reconstruct compression control (0:disalbe, 1:enable)
	BOOL bTileEn;                           ///< tile mode control (0:disable, 1:enable)
	BOOL bD2dEn;                            ///< d2d mode control (0:disable, 1:enable)
	BOOL gdc_mode_en;                       ///< d2d mode for gdc control (0:disable, 1:enable)
	INT8 chrm_qp_idx;
	INT8 sec_chrm_qp_idx;
	BOOL hw_padding_en;
	BOOL sao_en;                          ///< Sample Adaptive Offset (0: enable, 1: disable)
	BOOL sao_luma_flag;                   ///< sao luma flag
	BOOL sao_chroma_flag;                 ///< sao chroma flag
	// 520 fastboot builtin
	BOOL builtin_init;                    ///< fastboot builtin_init flag
	BOOL colmv_en;                        ///< colocated mv (0:disable, 1:enable)
	// 560 UVC
	BOOL comm_recfrm_en;                  ///< common reconstruct frame (0:disable, 1:enable)
	UINT32 recfrm_addr[3];                ///< common reconstruct frame address
	UINT32 recfrm_size;                   ///< common reconstruct frame size
	UINT32 recfrm_num;                    ///< common reconstruct frame number
	KDRV_VDOENC_QLVL quality_level;		  ///< quality level for setting search range and tile config
	UINT32 ltr_bs_buffer;				  ///< bs buffer for ltr
	UINT32 max_path_num;				  ///< video encode max path number
} KDRV_VDOENC_INIT;

/************ encode one frame info *******************/
typedef struct {
	UINT32 y_addr;										///< [w] encoding y address
	UINT32 c_addr;                                    	///< [w] encoding cb address
	UINT32 y_line_offset;                               ///< [w] Y line offset
	UINT32 c_line_offset;                              	///< [w] UV line offset
	UINT32 bs_start_addr;                               ///< [w] output bit-stream start address if enc space not enough
	UINT32 bs_addr_1;                                   ///< [w] 1st output bit-stream address
	UINT32 bs_size_1;                                   ///< [w] 1st output bit-stream size
	UINT32 bs_addr_2;                                   ///< [w] 2nd output bit-stream address
	UINT32 bs_size_2;                                   ///< [w] 2nd output bit-stream size
	UINT32 bs_end_addr;                                 ///< [w] output bit-stream end address
	UINT32 src_out_en;                              	///< [w] enable/disable src output
	UINT32 src_out_y_addr;                            	///< [w] src output Y address
	UINT32 src_out_c_addr;                              ///< [w] src output UV address
	UINT32 src_out_y_line_offset;                       ///< [w] src output Y line offset
	UINT32 src_out_c_line_offset;                       ///< [w] src output UV line offset
	UINT32 bs_shift;                                   	///< [w] I frame need VPS(not 4-byte align), BS should shift
	// 520 source compression different with 510/680 , do not use src_cmps_en and src_cmps_info, use src_cmps_y_en and src_cmps_c_en//
	UINT32 src_cmps_en;                            		///< [==== RESERVED ===== ] [w] src compression
	KDRV_H26XENC_SRC_COMPRESS_INFO src_cmps_info;   	///< [==== RESERVED ===== ][w] src compression info
	UINT32 quality;                                   	///< [w] initial quality value for jpeg (0~100)
	UINT32 raw_y_addr;                                  ///< [r] RAW Yaddr
	UINT32 svc_hdr_size;                                ///< [==== RESERVED ===== ][r] SVC header size
	UINT32 temproal_id;                                	///< [r] SVC temporal layer ID (0, 1, 2)
	UINT32 frm_type; 									///< [==== RESERVED ===== ][r] (0:P, 1:B, 2:I, 3:IDR, 4:KP)
	UINT32 encode_err;									///< [r] output: re-encode or not
	UINT32 re_encode_en;								///< [r] output: re-encode or not
	UINT32 nxt_frm_type;								///< [r] suggest next frame type
	UINT32 base_qp;										///< [r] base qo
	UINT32 retstart_interval;							///< [w] JPEG restart interval. default: 0, range: 0~65536
	UINT32 encode_time;                                 ///<  [==== RESERVED ===== ]HW encode time(unit: us)
	UINT32 encode_width;								///< [w] JPEG encode width.
	UINT32 encode_height;								///< [w] JPEG encode height.
	UINT32 target_rate;                                 ///< [w] target rate (in bytes)
	UINT32 frame_rate;                                  ///< [w] frame rate
	UINT32 vbr_mode;                                    ///< [w] VBR mode setting
	UINT32 motion_ratio;                                ///< [w] enc motion ratio
	KDRV_JPEGYUV_FORMAT in_fmt;							///< [w] JPEG input YUV format.0: YUV422,1:YUV420
	// for 520 //
	BOOL   src_cbcr_iv;
	BOOL   src_out_mode;
	UINT32 nalu_len_addr;
	UINT32 time_stamp;
	BOOL   skip_frm_en;
	KDRV_VDOENC_SDC st_sdc;								/// < [w] source decompression parameter
	BOOL   src_d2d_en;									/// < [w] source d2d enable, 0: disable, 1 : enable
	UINT8  src_d2d_mode;								/// < [w] source d2d mode, (0: codec 1 tile ISP 1 strip, 1: codec 1 tile ISP 2 strip, (2~4 h265 only) 2: codec 2 tile ISP 1 strip, 3 : codec 2 tile ISP 2 strip, 4 : codec 3 tile ISP 3 strip)
	UINT32 src_d2d_strp_size[3];						/// < [w] source d2d stripe size
	BOOL   fmt_trans_en;                                /// < [w] JPEG format trans from 420 to 422
	UINT32 min_quality;                                 ///< [w] min quality value for jpeg (0~100)
	UINT32 max_quality;                                 ///< [w] max quality value for jpeg (0~100)
	BOOL   all_job_done;                                /// < [w] all job encode done flag for non-blocking mode
	UINT32 y_mse;										///< [r] y mean square error
	UINT32 u_mse;										///< [r] u mean square error
	UINT32 v_mse;										///< [v] v mean square error
	UINT32 nalu_num;                                    ///< [r] nalu_num
	UINT32 nalu_size_addr;                              ///< [r] address for nalu_size(s)
} KDRV_VDOENC_PARAM;

typedef enum {
	VDOENC_SET_CBR, 						///< set CBR info
	VDOENC_SET_EVBR, 						///< set EVBR info
	VDOENC_SET_VBR, 						///< set VBR info
	VDOENC_SET_FIXQP, 						///< set FIXQP info
	VDOENC_SET_ROWRC, 						///< set ROWRC info
	VDOENC_SET_QPMAP, 						///< set QP Map
	VDOENC_SET_AQ, 							///< set AQ info
	VDOENC_SET_3DNR,						///< set 3DNR info
	VDOENC_SET_3DNRCB, 						///< set 3DNR setting callback
	VDOENC_SET_ROI, 						///< set ROI info
	VDOENC_SET_SMART_ROI, 					///< set Smart ROI info (from OD)
	VDOENC_SET_MD, 							///< set MD info
	VDOENC_SET_SLICESPLIT,					///< set Slice Split info
	VDOENC_SET_GDR,							///< set intra refresh
	VDOENC_SET_RESET_IFRAME, 				///< set reset i frame
	VDOENC_SET_DUMPINFO, 					///< set dump info
	VDOENC_SET_RESET, 						///< set reset
	VDOENC_SET_COE,					    	///< set COE info
	VDOENC_SET_JPEG_FREQ,					///< set JPEG engine freq
	VDOENC_SET_CODEC,						///< set codec type
	VDOENC_SET_INIT,						///< set init encode info
	VDOENC_SET_CLOSE,						///< set video encode channel close
	VDOENC_SET_OSG_RGB,						/// < set osg rgb
	VDOENC_SET_OSG_PAL,						/// < set osg pal
	VDOENC_SET_OSG_WIN,						/// < set osg window
	VDOENC_SET_MOT_ADDR,                                            /// < set motion buffer address
	VDOENC_SET_ISPCB,                                               /// < set ISP callback function
	VDOENC_SET_RDO,                                                 /// < set rdo info
	VDOENC_SET_JND,                                                 /// < set jnd info
	VDOENC_SET_LL_MEM,						///< set link-list command buffer address
	VDOENC_SET_GOPNUM,						///< set gop number
	VDOENC_SET_LSC, 						///< set long start code
	VDOENC_SET_JPEG_OSG_RGB,
	VDOENC_SET_JPEG_OSG_PAL,
	VDOENC_SET_JPEG_OSG_WIN,
	VDOENC_SET_JPEG_ROTATION,
    VDOENC_SET_DMA_ABORT,                   ///< set dma abort
    VDOENC_SET_SPN,                         ///< set post sharpen
    VDOENC_SET_SPNCB,
	ENUM_DUMMY4WORD(KDRV_VDOENC_SET_PARAM_ID)
} KDRV_VDOENC_SET_PARAM_ID;

typedef enum {
	VDOENC_GET_MEM_SIZE,						///< get codec needed size
	VDOENC_GET_DESC, 						///< get header description
	VDOENC_GET_ISIFRAME,						///< check whether is I frame
	VDOENC_GET_GOPNUM, 						///< get gop number
	VDOENC_GET_CBR, 						///< get CBR info
	VDOENC_GET_EVBR, 						///< get EVBR info
	VDOENC_GET_VBR, 						///< get VBR info
	VDOENC_GET_FIXQP, 						///< get FIXQP info
	VDOENC_GET_AQ, 							///< get AQ info
	VDOENC_GET_RC, 							///< get RC info
	VDOENC_GET_3DNR, 						///< get 3DNR info
	VDOENC_GET_GDR,                                                 ///< get GDR info
	VDOENC_GET_COE, 						///< get COE info
	VDOENC_GET_JPEG_FREQ, 						///< get JPEG engine frequency
	VDOENC_GET_BS_LEN,						///< get currenct encoded length
	VDOENC_GET_ISP_RATIO,						///< get ISP ratio
	VDOENC_GET_ROWRC,						///< get ROWRC info
	VDOENC_GET_RDO,							///< get rdo info
	VDOENC_GET_LL_MEM_SIZE,						///< get link-list command buffer memory size
	VDOENC_GET_SRCOUTYUV_WH,					///< get source out yuv align width/height
	VDOENC_GET_RECONFRM_SIZE,					///< get reconstruct frame size
	VDOENC_GET_RECONFRM_NUM,					///< get reconstruct frame number

	VDOENC_GET_JPEG_OSG_RGB,
	VDOENC_GET_JPEG_OSG_PAL,
	VDOENC_GET_JPEG_OSG_WIN,
	VDOENC_GET_JPEG_ROTATION,
	ENUM_DUMMY4WORD(KDRV_VDOENC_GET_PARAM_ID)
} KDRV_VDOENC_GET_PARAM_ID;
/*!
 * @fn INT32 kdrv_videoenc_open(UINT32 chip, UINT32 engine, UINT32 max_path_num)
 * @brief open hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @param max_path_num	the max encode path number
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videoenc_open(UINT32 chip, UINT32 engine, UINT32 max_path_num);

/*!
 * @fn INT32 kdrv_videoenc_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip		the chip id of hardware
 * @param engine	the engine id of hardware
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videoenc_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_videoenc_trigger(KDRV_VDOENC_ID handler, KDRV_H26XENC_PARAM *p_enc_param,
								  VOID *p_cb_func,
								  VOID *user_data);
 * @brief trigger hardware engine
 * @param handler 				the handler of hardware
 * @param p_enc_param		 	the encode one frame settings
 * @param p_cb_func 			the callback function
 * @param user_data 			the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videoenc_trigger(UINT32 id, KDRV_VDOENC_PARAM *p_enc_param,
						  KDRV_CALLBACK_FUNC *p_cb_func,
						  VOID *p_user_data);

/*!
 * @fn INT32 kdrv_videoenc_get(KDRV_VDOENC_ID handler, DEC_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param handler	the handler of hardware
 * @param id 		the id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videoenc_get(UINT32 id, KDRV_VDOENC_GET_PARAM_ID parm_id, VOID *param);

/*!
 * @fn INT32 kdrv_videoenc_set(KDRV_VDOENC_ID handler, DEC_ID id, VOID *param)
 * @brief get parameters to hardware engine
 * @param handler	the handler of hardware
 * @param id 		the id of parameters
 * @param param 	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_videoenc_set(UINT32 id, KDRV_VDOENC_SET_PARAM_ID parm_id, VOID *param);


#endif
