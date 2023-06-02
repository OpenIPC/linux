#ifndef _H26XENC_RATE_CONTROL_H_
#define _H26XENC_RATE_CONTROL_H_

#include "h26xenc_rc_param.h"

/*
0.1.0: initial version
0.1.1(2017/10/13): enlarge row level qp range to handle bitrate overflow when rc not converge
0.1.3(2017/10/17): 1. row level pred bit from budget, 2. reduce computation time
0.1.4(2017/10/19): fix bug of divide by zero
0.1.5(2017/10/23): disable dump warning message
0.1.6(2017/11/10): add I frame model (XA version)
0.1.7(2017/11/17): adjust method for TEncRCGOP_updateAfterPicture (XA)
0.1.9(2017/11/29): reduce GOP array
0.1.10(2017/12/06): add rate control of key p frame & support ip weight
0.1.11(2017/12/11): support evbr
0.1.12(2017/12/13): update QP by PSNR and disable motion AQ when I and key P frame
0.1.13(2017/12/21): qp sum error use slice qp
0.1.14(2017/12/26): reduce IP weight cost time
0.1.15(2018/01/04): 1. disable LOOKUP_TABLE_METHOD, 2. fixed reverse_lookup_table bug
0.1.16(2018/01/18): fixed : compile error when SPECIAL_P_BA disable
0.1.17(2018/01/25): fix bug of calculate average of qp sum
0.1.18(2018/01/29): fix dead code
0.1.19(2018/01/31): adjust method for IP weight, using deltaQP
0.1.20(2018/02/07): fix bug of to enable row rc
0.1.21(2018/03/02): 1. update measurement of bitrate overflow, 2. handle bitrate underflow
0.1.22(2018/03/13): 1. update measurement of bitrate overflow (gop), 2. state change when key p frame
0.1.23(2018/03/16): 1. remove redundant code, 2. improve EVBR mode, 3. fix porting bug and NEWTON_WEIGHT_METHOD bug
0.1.24(2018/03/22): handle different period of ltr & key p
0.1.25(2018/03/29): add rc output log
0.1.26(2018/04/11): adjust restriction for I frame QP of second GOP
0.1.27(2018/04/17): force increase qp when bitstream overflow
0.1.28(2018/04/30): add define to enable user init quant
0.1.29(2018/05/07): 1. add check average qp to avoid EVBR state change frequantly, 2. reduce row rc of key p
0.1.30(2018/05/25): VBR prelimit bitrate
0.1.31(2018/07/16): VBR use min bitrate
0.1.32(2018/08/06): support SVC RC
0.1.33(2018/12/07): new update
0.1.34(2019/01/17): clear bit diff when VBR under init quant
0.1.35(2019/01/29): support LT SVC RC
0.1.36(2019/02/25): update qp factor
0.1.37(2019/02/28): limit frame size
0.1.38(2019/06/03): fix bug of compute weight underflow
0.1.39(2019/06/18): fix buf of compute weight unferflow (consider ipweight)
0.1.40(2019/08/19): update proc & add dump rc log
0.1.41(2019/08/26): 1. EVBR use different gop-pic weight, 2. add proc to dump init parameter, 3. update default value of row rc (cardv)
0.1.41.1(2019/10/09): change tool chain
0.1.42(2020/01/08): add control of adjust qp weight
0.1.43(2020/02/26): encode rc log
0.1.44(2020/03/11): CBR/VBR enable motion aq. support CBR2, VBR2 & EVBR2
0.1.45(2020/03/19): support VBR2
0.1.46(2020/04/13): FIX_BUG_I_QP
0.1.47(2020/04/28): add VBR2 log
0.1.48(2020/05/13): VBR2 check change pos
0.1.49(2020/05/29): limit gop bitrate difference to reduce underflow propagate
0.1.50(2020/07/27): EVBR initial state: motion state, avoid bitrate overflow
0.1.51(2020/10/29): support fix SVC weight
0.1.52(2020/11/11): merge SUPPORT_VBR2_CVTE
0.1.53(2020/11/12): enhance SUPPORT_VBR2_CVTE
0.1.54(2020/11/17): suppport input max frame size
0.1.55(2021/02/25): when limit I frame size, disable limit qp change
*/
#define H26X_RC_VERSION     0x00013700
#define H26X_RC_VER_STR		"0.1.55"

#if defined(WIN32) || !defined(__LINUX)
    #include <stdint.h>
#else
    #include <linux/types.h>
#endif

//#define DIVIDE_RC   1
#define FRAMERATE_BASE      1000

#define EVALUATE_BITRATE            1
#define DUMP_RC_INIT_INFO           1

#define BITRATE_OVERFLOW_CHECK      1
#define NVT_OPTIMIZED               1
#if NVT_OPTIMIZED
    #define LOOKUP_TABLE_METHOD     1   /* XA, 20171110 */
#else
    #define LOOKUP_TABLE_METHOD     0
#endif
#define SPECIAL_P_BA                1
#if SPECIAL_P_BA
    #define SUPPORT_EVBR            1
#else
    #define SUPPORT_EVBR            0
#endif
#define USE_IP_DELTAQP              1
#define WEIGHT_ADJUST               1
#define UPDATE_QP_BY_PSNR           0
#define REDUCE_WEIGHT_CALNUM        1
#define NEWTON_WEIGHT_METHOD        1   // 1: weight calcalation ueing newton,0 :original dichotomy method
#define EVBR_MEASURE_BR_METHOD      1   // 0: measure base frame rate, 1: measure base gop
#define EVBR_STILL_START_FROM_KEY   1
#define REDUCE_I_FRAME_ROW_RC       1
#define REDUCE_P_FRAME_AQ           1
#define EVBR_MODE_SWITCH_NEW_METHOD 1
#define HANDLE_DIFF_LTR_KEYP_PERIOD 1
#define HANDLE_BITSTREAM_OVERFLOW   1
#define USER_INIT_QUANT             1
#if REDUCE_I_FRAME_ROW_RC
    #define REDUCE_KP_FRAME_ROW_RC  1
#endif
#define EVBR_CHECK_QP_UNDER_STILL   1
#define VBR_PRELIMIT_BITRATE        1
#define SUPPORT_SVC_RC              1
#define SUPPORT_LT_SVC_RC           1
#define USE_HP_DELTAQP              1
#if SUPPORT_SVC_RC
#define SUPPORT_SVC_FIXED_WEIGHT_BA 1
#endif
#define RD_LAMBDA_UPDATE            1
#define MAX_SIZE_BUFFER             1
#define FIX_BUG_HK                  1
#define SMOOTH_FRAME_QP             1
#define SUPPORT_VBR2                1
#if SUPPORT_VBR2
#define SUPPORT_VBR2_CVTE           1
#define MAX_CHANGE_FRAME_NUM        30
#endif
#define FIX_BUG_I_QP                1
#define LIMIT_GOP_DIFFERNT_BIT		1

#define TIMES_10                    10000
#define TIMES_10_NUM                4
#define INT_EXP                     27183   //((int)(2.718281 * TIMES_10))
#define SHIFT_11                    11
#define SHIFT_VALUE                 (1 << SHIFT_11)
#define MAX_INTEGER                 0x7FFFFFFF
#define LOG10_EXP                   4342    //(lookupLogListTable(INT_EXP, 1)
#define MAX_RC_LOG_CHN              4

#define RC_CLIP3(low, high, val)    ((val)>(high)?(high):((val)<(low)?(low):(val)))
#define RC_ABS(value)               ((value) >= 0 ? (value) : -(value))
#define RC_MIN(val0, val1)          ((val0) < (val1) ? (val0) : (val1))
#define RC_MAX(val0, val1)          ((val0) > (val1) ? (val0) : (val1))

#define ALPHA       13833           // (int)(6.7542 * SHIFT_VALUE)
#define BETA1       2563            // (int)(1.2517 * SHIFT_VALUE)
#define BETA2       3658            // (int)(1.7860 * SHIFT_VALUE)

#define MAX_QUANT   51
#define MIN_QUANT   1

/* Can adjust item */
#define Default_TERM_TAYLOR     10
#define MAX_GOP_SIZE            600
/*
typedef unsigned long long  uint64_t;
typedef signed long long    int64_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
*/
typedef enum
{
    RC_I_FRAME_LEVEL = 0,
    RC_P_FRAME_LEVEL,
    RC_KEY_P_FRAME_LEVEL,
#if SUPPORT_LT_SVC_RC
    RC_LT_P_FRAME_LEVEL,
#endif
#if SUPPORT_SVC_RC
	RC_P2_FRAME_LEVEL,
	RC_P3_FRAME_LEVEL,
#endif
    RC_MAX_FRAME_LEVEL
} RC_FRAME_LEVEL;

// old definition
//#define RC_RDMODEL_WIN_SIZE 16
#if EVALUATE_BITRATE
typedef struct rc_bitrate_info_t
{
    uint32_t total_byte;
    uint32_t frame_cnt;
    uint32_t i_frame_cnt;
} RCBitrate;
#endif
#if BITRATE_OVERFLOW_CHECK
typedef struct
{
    int bit_record[MAX_GOP_SIZE];
    int64_t bitrate_record;
    int push_idx;
    int pop_idx;
    int recrod_num;
} TRCBitrateRecord;
#endif

typedef struct rc_log_info_t
{
    int frameLevel;
    int ave_ROWQP;
    int currQP;
    int picActualBits;
    int picTargetBits;
    int calLambda;
    int picLambda;
    int alpha;
    int beta;
    int64_t bitsLeft;
    uint64_t frame_MSE;

	uint32_t maxBitrate;
	uint32_t changePosThd;
	uint32_t overflowLevel;
	uint32_t currIQP;
	uint32_t currPQP;
	uint32_t currKPQP;
	uint32_t currLTPQP;
	uint32_t currP2QP;
	uint32_t currP3QP;
	uint32_t targetBits;
	unsigned int frameSize;
} TRCLogInfo;

/****************** local parameter *************************/
typedef struct
{
    int m_alpha;
    int m_beta;
} TRCParameter;

typedef struct
{
#if NVT_OPTIMIZED
    int64_t m_bitsleft;
    int m_seqframeleft;
#endif
#if WEIGHT_ADJUST
    int m_updateLevel;
    int m_updatealpha[5];
    int m_updatebeta[5];
    int keyframe_num;
#endif
    int m_targetRate;
    int m_fbase;
    int m_fincr;
    int m_GOPSize;
    int m_mbCount;
    int m_numberOfLevel;
    int m_numberOfMB;
    int m_seqTargetBpp;
    int m_alphaUpdate;
    int m_betaUpdate;
    int m_lastLambda;
    int m_adaptiveBit;

    int m_bitsRatio[RC_MAX_FRAME_LEVEL];     // by slice type
    int m_GOPID2Level[RC_MAX_FRAME_LEVEL];   // by slice type

    TRCParameter  m_picPara[8];
    int64_t m_bitsDiff;
    int64_t m_averageBits;
    int64_t m_targetBits;
} TEncRCSeq;

typedef struct
{
    int64_t m_picTargetBitInGOP[RC_MAX_FRAME_LEVEL];
    int m_numPic;
    int64_t m_targetBits;
    int m_picLeft;
    int64_t m_bitsLeft;
} TEncRCGOP;

#if SUPPORT_SVC_RC
typedef struct
{
	int m_numPic;
	int64_t m_targetBits;
	int m_picLeft;
	int64_t m_bitsLeft;

	//JingHE
#if !USE_HP_DELTAQP
	int64_t m_lambdaRatioLevel[RC_MAX_FRAME_LEVEL];
	int m_lambdaRatioFix;
	int m_distPropFactor;
	int64_t m_skipRatioHP;
	int64_t m_interRatioHP;
	int64_t m_HPCnt;
	int64_t m_skipRatioSum;
	int64_t m_interRatioSum;
	int64_t m_sumCnt;
#endif
} TEncRCHPGOP;
#endif

typedef struct
{
    int m_frameLevel;
    int m_numberOfMB;
    int m_targetBits;
    int m_estHeaderBits;
    int m_estPicQP;

    int m_estPicLambda;
    int m_picLambda;

    int m_totalCostIntra;
    int m_bitsLeft;
    int m_pixelsLeft;

    int m_picActualHeaderBits;    // only SH and potential APS
    int m_picActualBits;          // the whole picture, including header
    int m_picQP;                  // in integer form
#if UPDATE_QP_BY_PSNR
    uint32_t m_psnr;
#endif
} TEncRCPic;

#if SUPPORT_EVBR
#define EVBR_STILL_STATE    0x00
#define EVBR_MOTION_STATE   0x01
typedef struct _H26XEncEVBR
{
    uint32_t m_evbrState;
    uint32_t m_stillIQP;
    uint32_t m_stillKeyPQP;
    uint32_t m_stillPQP;
	uint32_t m_IPsnrCnd;          // 1~51: fix qp, > 100: PSNR cond
	uint32_t m_PPsnrCnd;          // 1~51: fix qp, > 100: PSNR cond
	uint32_t m_KeyPPsnrCnd;       // 1~51: fix qp, > 100: PSNR cond
    uint32_t m_stillFrameCnt;
    uint32_t m_stillFrameThd;
    uint32_t m_motionRatioThd;
    #if EVBR_XA
	uint32_t m_minstillPercent;
	int64_t  m_evbrStillThd;
	int64_t  m_evbrStillupThd;
	uint32_t m_startpic;
	int      m_stillOverflow;
	uint32_t m_tmp_stillIQP;
	uint32_t m_tmp_stillKeyPQP;
	uint32_t m_stillIadd;
	uint32_t m_stillKeyPadd;
	uint32_t m_stillPadd;
	uint32_t m_tmp2_stillIQP;
	uint32_t m_tmp2_stillKeyPQP;
	uint32_t m_last_stillIQP;
	uint32_t m_last_stillKeyPQP;
	uint32_t m_last_stillPQP;
	uint32_t uiMinStillIQp;
    #endif

    #if BITRATE_OVERFLOW_CHECK
    TRCBitrateRecord m_encRCBr;
    int64_t m_evbrOverflowThd;
    uint32_t m_measurePeriod;
    #endif
    #if EVBR_STILL_START_FROM_KEY
    uint32_t m_triggerStillMode;
    #endif
    #if EVBR_CHECK_QP_UNDER_STILL
    uint32_t m_motionQPSum;
    uint32_t m_motionFrameCnt;
    #endif
} TEncRCEVBR;
#endif

#if SUPPORT_VBR2
#define VBR2_VBR_STATE    0x00
#define VBR2_CBR_STATE    0x01
typedef struct _H26XEncVBR2
{
	uint32_t m_vbr2State;
	uint32_t m_maxBitrate;
	uint32_t m_initIQP;
	uint32_t m_minIQP;
	uint32_t m_maxIQP;
	uint32_t m_initPQP;
	uint32_t m_minPQP;
	uint32_t m_maxPQP;
	uint32_t m_initKPQP;
	uint32_t m_initLTPQP;
	uint32_t m_initP2QP;
	uint32_t m_initP3QP;

	uint32_t m_currIQP;
	uint32_t m_currPQP;
	uint32_t m_currKPQP;
	uint32_t m_currLTPQP;
	uint32_t m_currP2QP;
	uint32_t m_currP3QP;
	uint32_t m_lastPQP;

	uint32_t m_deltaIQP;
	uint32_t m_deltaKPQP;
	uint32_t m_deltaLTPQP;
	uint32_t m_deltaP2QP;
	uint32_t m_deltaP3QP;

	uint32_t m_isFirstPic;

#if BITRATE_OVERFLOW_CHECK
	TRCBitrateRecord m_encRCBr;
	uint32_t m_overflowLevel;
	uint32_t m_changePosThd;
	uint32_t m_overflowThd;
	uint32_t m_overflowThd2;
	uint32_t m_underflowThd;
	uint32_t m_underflowThd2;
#if SUPPORT_VBR2_CVTE
	uint32_t m_changeFrmCnt;
#endif

	int64_t m_rateOverflowThd;
	int64_t m_rateChangePosThd;
	uint32_t m_measurePeriod;
	uint32_t m_lastISize;
	uint32_t m_lastKPSize;
	uint32_t m_lastLTPSize;
	uint32_t m_lastP2Size;
	uint32_t m_lastP3Size;
	uint32_t m_lastPSize;

	uint32_t m_targetBits;

	uint32_t m_bitRatio[RC_MAX_FRAME_LEVEL];
#endif
} TEncRCVBR2;
#endif

typedef struct _H26XEncRC
{
    TEncRCSeq m_encRCSeq;
    TEncRCGOP m_encRCGOP;
    TEncRCPic m_encRCPic;
#if SUPPORT_SVC_RC
    TEncRCHPGOP m_encRCHPGOP;
#endif

    int     m_chn;
    uint32_t  m_initIQp;
    uint32_t  m_minIQp;
    uint32_t  m_maxIQp;
    uint32_t  m_initPQp;
    uint32_t  m_minPQp;
    uint32_t  m_maxPQp;
    uint32_t  m_GOPSize;
    int     m_lastSliceType;    // for output info
    uint32_t  m_lastFrameSize;    // for output info

#if SPECIAL_P_BA
    int     m_SP_frm_num;
    int     m_SP_index;
    int     m_SP_sign;
    int     m_GOP_SPnumleft;
#endif
#if SUPPORT_LT_SVC_RC
	int     m_LT_frm_num;
	int     m_LT_index;
	int     m_LT_sign;
	int     m_GOP_LTnumleft;
	int     m_deltaLT_QP;
#endif
	int     m_lastpQP;

#if RD_LAMBDA_UPDATE
	uint64_t frame_MSE;
	int      disable_clip;
	uint32_t MotionRatio;
	uint32_t ave_moRatio;
#endif

#if SUPPORT_SVC_RC
	int m_HP_sign;
	int m_HP_index;
	int m_P2_frm_num;
	int m_P3_frm_num;
	int m_deltaP2_QP;
	int m_deltaP3_QP;
	int m_HP_period;
	int m_GOP_P2numleft;
	int m_GOP_P3numleft;

#if SUPPORT_SVC_FIXED_WEIGHT_BA
	uint32_t m_svcBAMode;
	int m_svcP2Weight;
	int m_svcP3Weight;
#endif

	//JineHE
#if !USE_HP_DELTAQP
	int m_HPSize;
	int m_lastLevel;
	int m_targetBpp;	
#endif
#endif

#if USE_IP_DELTAQP
    int     m_rc_deltaQPip;
    int     m_rc_deltaQPkp;
#elif WEIGHT_ADJUST
    int     m_rc_wfip;
	int     m_rc_wfkp;
#endif

#if NVT_OPTIMIZED
    int     m_iPrevPicBitsBias[4];
    int     ave_ROWQP;
#endif
    int     m_inumpic;
    int     m_activeQP;
    int     m_currQP;
    int     m_currlambda;
    int     m_slicetype;

#if SUPPORT_SVC_RC
    #if SUPPORT_LT_SVC_RC
    int     m_lastLevelLambda[RC_MAX_FRAME_LEVEL];
    int     m_lastLevelQP[RC_MAX_FRAME_LEVEL];
    #else
	int     m_lastLevelLambda[5];
	int     m_lastLevelQP[5];
    #endif
#else
    int     m_lastLevelLambda[3];
    int     m_lastLevelQP[3];
#endif
    int     m_lastPicLambda;
    int     m_lastValidLambda;
    int     m_lastPicQP;
    int     m_lastValidQP;

    int64_t m_GOPbitrate;
    int64_t m_GOPMinBitrate;
    int     m_rowRCEnable;
    uint32_t  m_smoothWindowSize;
    //uint32_t  m_ctuSize;

    uint32_t  m_rcMode;
    uint32_t  m_fixIQp;
    uint32_t  m_fixPQp;
    uint32_t  m_frameRateBase;
    uint32_t  m_frameRateIncr;
    uint32_t  m_staticTime;
    uint32_t  m_minGOPQP;
    uint32_t  m_maxGOPQP;
#if BITRATE_OVERFLOW_CHECK
    TRCBitrateRecord m_encRCBr;
#endif
    uint32_t  m_vbrChangePos;
#if VBR_PRELIMIT_BITRATE
    int64_t m_vbrPrelimitBRThd;
#endif
    uint32_t  m_keyPPeriod;
#if HANDLE_BITSTREAM_OVERFLOW
    int     m_overflowIncQP;
#endif
    int64_t m_prevTargetBits;
    int     m_motionAQStrength;
#if SUPPORT_EVBR
    TEncRCEVBR m_encRCEvbr;
    //H26XEncRowRc m_evbrRowRc;
    uint32_t  m_motionRatio;
	#if EVBR_MEASURE_BR_METHOD
	int     m_lastIsize;
	int     m_lastKeyPsize;
	#endif

	#if EVBR_MODE_SWITCH_NEW_METHOD
	int     m_still2motion_I;
	int     m_still2motion_KP;
	int     m_record_init_weight[RC_MAX_FRAME_LEVEL];
	int     m_record_lastQP;
	int     m_record_lastLambda;
	#endif
	#if HANDLE_DIFF_LTR_KEYP_PERIOD
    uint32_t  m_ltrFrame;
    uint32_t  m_ltrPeriod;
	#endif
#endif
#if SUPPORT_VBR2
	TEncRCVBR2 m_encRCVbr2;
#endif
#if SUPPORT_SVC_RC
    int     m_svcLayer;
#endif
#if MAX_SIZE_BUFFER
	int    m_max_frm_bit;
	int    m_max_frm_bool;
#endif
    TRCLogInfo rc_log_info;
	uint32_t m_aslog2;
	uint32_t m_glb_max_frm_size;	// byte
	uint32_t reserved[9];
}H26XEncRC;

typedef struct _H26XEncRC   TEncRateCtrl;

/* extern function */
extern int h26xEnc_RcInit(H26XEncRC *pRc, H26XEncRCParam *pRCParam);
extern int h26xEnc_RcPreparePicture(H26XEncRC *pRc, H26XEncRCPreparePic *pPic);
extern int h26xEnc_RcUpdatePicture(H26XEncRC *pRc, H26XEncRCUpdatePic *pUPic);
extern int h26xEnc_RcGetLog(H26XEncRC *pRc, unsigned int *log_addr);

#endif
