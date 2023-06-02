#ifndef _H26XENC_RC_PARAM_H_
#define _H26XENC_RC_PARAM_H_

#define EVBR_XA         	    1
#if defined (CONFIG_NVT_SMALL_HDAL)
	#define H26X_RC_PROC_ENABLE		0
#else
	#define H26X_RC_PROC_ENABLE		1
#endif

#if H26X_RC_PROC_ENABLE
	#define H26X_LOG_RC_INIT_INFO	1
	#define H26X_RC_BR_MEASURE		1
	#define H26X_RC_DBG_CMD 		1
	#define H26X_RC_LOG_VERSION 	1
#else
	#define H26X_LOG_RC_INIT_INFO	0
	#define H26X_RC_BR_MEASURE		0
	#define H26X_RC_DBG_CMD 		0
	#define H26X_RC_LOG_VERSION 	0
#endif
#define MAX_LOG_CHN             16
#define MAX_RC_PROC_PARAM_NUM   30

typedef enum {
    RC_LOG_ERR = 1,
    RC_LOG_WRN = 2,
    RC_LOG_DBG = 3,
    RC_LOG_INF = 4,
} RC_LOG_LEVEL;

typedef enum{
    RC_P_SLICE = 0,
    RC_B_SLICE = 1,
    RC_I_SLICE = 2,
    RC_IDR_SLICE = 3,
    RC_UNSET_SLICE_TYPE = 0xFF
} RC_SLICE_TYPE;

typedef enum _h26x_rc_mode {
	H26X_RC_CBR = 1,
	H26X_RC_VBR,
	H26X_RC_VBR2,
	H26X_RC_FixQp,
	H26X_RC_EVBR,
	H26X_RC_NOT_SUPPORT,
} H26X_RC_MODE;

typedef struct _H26XEncRCParam
{
    unsigned int uiEncId;
    unsigned int uiRCMode;
    unsigned int uiPicSize;  // resolution
    //unsigned int uiCtuSize;
    unsigned int uiInitIQp;
    unsigned int uiMinIQp;
    unsigned int uiMaxIQp;
    unsigned int uiInitPQp;
    unsigned int uiMinPQp;
    unsigned int uiMaxPQp;
    unsigned int uiBitRate;
    unsigned int uiFrameRateBase;
    unsigned int uiFrameRateIncr; // frame rate = uiFrameRateBase / uiFrameRateIncr
    unsigned int uiGOP;
    //int          iIPQPOffset;
    unsigned int uiRowLevelRCEnable;
    unsigned int uiStaticTime;
    unsigned int uiChangePos;
    unsigned int uiFixIQp;
    unsigned int uiFixPQp;
    unsigned int uiKeyPPeriod; // 0: disable, 1: frame rate, others: r frame period
    int          iIPWeight;
    int          iKPWeight;

	int          iP2Weight;
	int          iP3Weight;
	int          HP_period; // SVC weight update period
	unsigned int uiSvcBAMode;	// SVC mode: 0: weight = qp (origin SVC RC), 1: weight = bit size
	int          iLTWeight;

    int          iMotionAQStrength;
    unsigned int uiStillFrameCnd;
    unsigned int uiMotionRatioThd;
    unsigned int uiIPsnrCnd;
    unsigned int uiPPsnrCnd;
    unsigned int uiKeyPPsnrCnd;
    unsigned int uiLTRInterval;
    unsigned int uiSVCLayer;
#if EVBR_XA
	unsigned int uiMinStillPercent;
	unsigned int uiMinStillIQp;
#endif
	unsigned int uiMaxFrameSize;
}H26XEncRCParam;

typedef struct _H26XEncRCPreparePic
{
    RC_SLICE_TYPE ucPicType;
    unsigned int uiPredSize;
    unsigned int uiLTRFrame;
    unsigned int uiEncodeRatio;
    unsigned int uiMotionAQEnable;
    unsigned int uiSVCLayer;
	unsigned int uiMaxFrameByte;
	unsigned int uiFrameLevel;
	unsigned int uiAslog2;
}H26XEncRCPreparePic;

typedef struct _H26XEncRCUpdatePic
{
    RC_SLICE_TYPE ucPicType;
    //int          iPicCost[2];
    unsigned int uiFrameSize;
    unsigned int uiAvgQP;
    //unsigned int uiQPSum;
    unsigned int uiYMSE[2];
    //unsigned int uiUMSE[2];
    //unsigned int uiVMSE[2];
    unsigned int uiMotionRatio;
    // still: disable row level rc & enable motion aq strength
    // motion: enable row level rc & disable motion aq strength
    unsigned int uiUpdate;
    unsigned int uiRowRCEnable;
    int          iMotionAQStr;
    unsigned char bEVBRStillFlag;
	//hehehehe
	unsigned int uiSkipNum;
	unsigned int uiInterNum;
}H26XEncRCUpdatePic;

typedef struct _H26XEncRCSetCBR
{
    unsigned int uiInitIQp;
    unsigned int uiMinIQp;
    unsigned int uiMaxIQp;
    unsigned int uiInitPQp;
    unsigned int uiMinPQp;
    unsigned int uiMaxPQp;
    unsigned int uiBitRate;
    unsigned int uiFrameRateBase;
    unsigned int uiFrameRateIncr;
    unsigned int uiGOP;
    //int          iIPQPOffset;
    unsigned int uiStaticTime;    // 0: default, unit GOP
    unsigned int uiRowRCEnable;
    int          iIPWeight;
}H26XEncRCSetCBR;

typedef struct _H26XEncRCSetVBR
{
    unsigned int uiInitIQp;
    unsigned int uiMinIQp;
    unsigned int uiMaxIQp;
    unsigned int uiInitPQp;
    unsigned int uiMinPQp;
    unsigned int uiMaxPQp;
    unsigned int uiBitRate;
    unsigned int uiFrameRateBase;
    unsigned int uiFrameRateIncr;
    unsigned int uiGOP;
    //int          iIPQPOffset;
    unsigned int uiStaticTime;    // 0: default, unit GOP
    unsigned int uiChangePos;    // base 100, threshold = GOP bitrate * uiChangePos / 100
    unsigned int uiRowRCEnable;
    int          iIPWeight;
}H26XEncRCSetVBR;

typedef struct _H26XEncRCSetEVBR 
{
    unsigned int uiInitIQp;
    unsigned int uiMinIQp;
    unsigned int uiMaxIQp;
    unsigned int uiInitPQp;
    unsigned int uiMinPQp;
    unsigned int uiMaxPQp;
    unsigned int uiBitRate;
    unsigned int uiFrameRateBase;
    unsigned int uiFrameRateIncr;
    unsigned int uiGOP;
    unsigned int uiKeyPPeriod;

    unsigned int uiStaticTime;
    int          iIPWeight;
    int          iKeyPWeight;
    int          iMotionAQStrength;

    unsigned int uiRowRCEnable;
    unsigned int uiStillFrameCnd;
    unsigned int uiMotionRatioThd;
    unsigned int uiIPsnrCnd;
    unsigned int uiPPsnrCnd;
    unsigned int uiKeyPPsnrCnd;
#if EVBR_XA
	unsigned int uiMinStillPercent;
	unsigned int uiMinStillIQp;
#endif
} H26XEncRCSetEVBR;

typedef struct _H26XEncRCSetFixQP
{
    unsigned int  uiFixIQp;
    unsigned int  uiFixPQp;
}H26XEncRCSetFixQP;

#if H26X_LOG_RC_INIT_INFO
typedef struct _H26XEncRCInitInfo
{
    //int chn;
    unsigned int rc_mode;
    unsigned int gop;
    unsigned int kp_period;
    unsigned int framerate_base;
    unsigned int framerate_incr;
    long long gop_bitrate;
    unsigned int init_i_qp;
    unsigned int min_i_qp;
    unsigned int max_i_qp;
    unsigned int init_p_qp;
    unsigned int min_p_qp;
    unsigned int max_p_qp;
    //int      pic_size;
    unsigned int smooth_win_size;
    int      ip_weight;
    int      kp_weight;
    unsigned int still_i_qp;
    unsigned int still_kp_qp;
    unsigned int still_p_qp;
    //uint32_t still_frame_thd;
    //uint32_t motion_ratio_thd;
    int      motion_aq_str;
    unsigned int change_pos;
    int      active_qp;
	unsigned int max_frame_size;
} H26XEncRCInitInfo;
#endif

struct rc_entity_t {
    int (*rc_init)(unsigned int chip, unsigned int chn, H26XEncRCParam *pRCParam);
    int (*rc_clear)(unsigned int chip, unsigned int chn);
    int (*rc_prepare)(unsigned int chip, unsigned int chn, H26XEncRCPreparePic *pPic);
    int (*rc_update)(unsigned int chip, unsigned int chn, H26XEncRCUpdatePic *pUPic);
};

#endif
