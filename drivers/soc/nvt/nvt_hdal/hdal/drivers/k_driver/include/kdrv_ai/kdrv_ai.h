#ifndef _KDRV_AI_H_
#define _KDRV_AI_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/platform.h"
#include "kdrv_type.h"
#include "kdrv_ai_builtin.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/


#define NEW_AI_FLOW	1
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#define AI_SUPPORT_NET_MAX		1
#else
#define AI_SUPPORT_NET_MAX		128
#endif


#define KDRV_AI_FCD_VLC_EN              0x00000001      ///< compression variable length encoding enable
#define KDRV_AI_FCD_QUANTI_EN           0x00000002      ///< compression quantization enable
#define KDRV_AI_FCD_SPARSE_EN           0x00000004      ///< compression sparse encoding enable
#define KDRV_AI_FCD_QUANTI_KMEANS_EN    0x00000008      ///< compression quantization kmeans enable
#define KDRV_AI_FCD_ALL_EN              0x0000000f      ///< compression all function enable

#define KDRV_AI_NEURAL_DECONV_EN        0x00000001      ///< neural network deconvolution enable
#define KDRV_AI_NEURAL_CONV_EN          0x00000002      ///< neural network convolution enable
#define KDRV_AI_NEURAL_NORM_EN          0x00000004      ///< neural network normalized enable
#define KDRV_AI_NEURAL_ELT_EN           0x00000008      ///< neural network eltwise enable
#define KDRV_AI_NEURAL_ACT_EN           0x00000010      ///< neural network activation enable
#define KDRV_AI_NEURAL_POOL_EN          0x00000020      ///< neural network pooling enable
#define KDRV_AI_NEURAL_PREACT_EN        0x00000040      ///< neural network pre-act enable
//#define KDRV_AI_NEURAL_ALL_EN           0x0000003f      ///< neural network all function enable
#define KDRV_AI_NEURAL_FUNC_CNT         0x7             ///< number of network function

#define KDRV_AI_FC_FULLY_EN             0x00000001      ///< fc fully connected enable
#define KDRV_AI_FC_ACT_EN               0x00000002      ///< fc activation enable
//#define KDRV_AI_FC_ALL_EN               0x00000003      ///< fc all function enable
#define KDRV_AI_FC_FUNC_CNT             0x2             ///< number of fc function


#define KDRV_AI_PREPROC_YUV2RGB_EN      0x00000001      ///< preprocessing from YUV to RGB enable
#define KDRV_AI_PREPROC_SUB_EN          0x00000002      ///< preprocessing mean subtraction enable
#define KDRV_AI_PREPROC_PAD_EN          0x00000004      ///< preprocessing padding enable
#define KDRV_AI_PREPROC_ROT_EN          0x00000008      ///< preprocessing rotate enable
#define KDRV_AI_PREPROC_FUNC_CNT        0x4

#define LL_SUPPORT_FIRST_LAYER          1
#define LL_SUPPORT_ROI                  0
#define APPLY_OPEN_FIRST                0
#define LL_BUF_FROM_USR                 1

#define AI_MAX_MODIFY_NUM       1280

#define SUPPORT_SYNC_BUF_ALIGN          1
#if SUPPORT_SYNC_BUF_ALIGN
#define SYNC_ALIGN_SZ                   32
#if (SYNC_ALIGN_SZ == 32)
#define AI_SYNC_ALIGN_CEIL(a) ALIGN_CEIL_32(a)
#define AI_SYNC_ALIGN_FLOOR(a) ALIGN_FLOOR_32(a)
#endif
#endif

#define AI_SUPPORT_MULTI_FMT            1
#define AI_IOREMAP_IN_KERNEL            1

#define AI_V4_STRIPE_STRUCT             1
#define AI_V4_STRIPE_FUNC               1
/********************************************************************
	TYPE DEFINITION
********************************************************************/

typedef INT32(*KDRV_AI_ISRCB)(UINT32, UINT32, UINT32, void *);

/**
    KDRV AI structure - trigger parameter
*/
typedef struct _KDRV_AI_TRIGGER_PARAM {
	BOOL    is_nonblock;
	UINT32	time_out_ms;	///< set 0 for disable time out
	BOOL    is_isr_trigger;
} KDRV_AI_TRIGGER_PARAM;

/*
    AI trigger mode
*/
typedef enum {
	AI_TRIG_MODE_APP     = 0,
	AI_TRIG_MODE_LL      = 1,
	AI_TRIG_MODE_FC      = 2,
	AI_TRIG_MODE_PREC    = 3,
	ENUM_DUMMY4WORD(KDRV_AI_TRIG_MODE)
} KDRV_AI_TRIG_MODE;

/**
    AI engine
*/
typedef enum {
	AI_ENG_UNKNOWN          = -1,
	AI_ENG_CNN             	= 0,
	AI_ENG_NUE            	= 1,
	AI_ENG_NUE2            	= 2,
	AI_ENG_CNN2             = 3,
	AI_ENG_TOTAL,
	ENUM_DUMMY4WORD(KDRV_AI_ENG)
} KDRV_AI_ENG;

/**
    AI processing function
*/
//@{
typedef enum {
	AI_MODE_NULL            = 0,
	AI_MODE_NEURAL          = 1,        ///< use KDRV_AI_NEURAL_PARM parameters
	AI_MODE_ROIPOOL,                    ///< use KDRV_AI_ROIPOOL_PARM parameters
	AI_MODE_SVM,                        ///< use KDRV_AI_SVM_PARM parameters
	AI_MODE_FC,                         ///< use KDRV_AI_FC_PARM parameters
	AI_MODE_PERMUTE,                    ///< use KDRV_AI_PERMUTE_PARM parameters
	AI_MODE_REORG,                      ///< use KDRV_AI_REORG_PARM parameters
	AI_MODE_ANCHOR,	                    ///< use KDRV_AI_ANCHOR_PARM parameters
	AI_MODE_SOFTMAX,		            ///< use KDRV_AI_SOFTMAX_PARM parameters
	AI_MODE_PREPROC,                    ///< use KDRV_AI_PREPROC_PARM parameters
	ENUM_DUMMY4WORD(KDRV_AI_MODE)
} KDRV_AI_MODE;
//@}

/**
    select input/output type
*/
//@{
typedef enum {
	AI_IO_INT8          = 0,
	AI_IO_UINT8         = 1,
	AI_IO_INT16         = 2,
	AI_IO_UINT16        = 3,
	AI_IO_INT12,                    ///< (for NT96680)
	AI_IO_UINT12,                   ///< (for NT96680)
	ENUM_DUMMY4WORD(KDRV_AI_IO_TYPE)
} KDRV_AI_IO_TYPE;
//@}

/**
    source format for neural network processing
*/
//@{
typedef enum {
	AI_NEURAL_SRC_IMG      = 0,     ///< source image format
	AI_NEURAL_SRC_FEAT     = 1,     ///< source feature format
	ENUM_DUMMY4WORD(KDRV_AI_NEURAL_SRC_FMT)
} KDRV_AI_NEURAL_SRC_FMT;
//@}

/**
    source format for preprocessing
*/
//@{   
typedef enum {
	AI_PREPROC_SRC_YUV420      = 0,     ///< source YUV420 format
	AI_PREPROC_SRC_YONLY       = 1,     ///< source YONLY format
	AI_PREPROC_SRC_UVPACK      = 2,     ///< source UVPACK format
	AI_PREPROC_SRC_RGB         = 3,     ///< source RGB format
#if AI_SUPPORT_MULTI_FMT
	AI_PREPROC_SRC_YUV420_NV21 = 4,     ///< source YUV420_NV21 format
#endif
	ENUM_DUMMY4WORD(KDRV_AI_PREPROC_SRC_FMT)
} KDRV_AI_PREPROC_SRC_FMT;
//@}

/**
    deconvolution pad mode
*/
//@{
typedef enum {
	AI_DECONV_SEL           = 0,        ///< Normal deconvolution
	AI_UPSAMPLE_SEL         = 1,        ///< Upsample
	ENUM_DUMMY4WORD(KDRV_AI_DECONV_MODE)
} KDRV_AI_DECONV_MODE;
//@}

/**
    convolution normalization mode
*/
//@{
typedef enum {
	AI_NORM_BN_SCL      = 0,        ///< bn scale mode
	AI_NORM_LRN         = 1,        ///< LRN mode (for NT96680)
	ENUM_DUMMY4WORD(KDRV_AI_NORM_MODE)
} KDRV_AI_NORM_MODE;
//@}

/**
    select eltwise mode
*/
//@{
typedef enum {
	AI_ELT_ADD       = 0,           ///< (in0*coef0 + in1*coef1)
	AI_ELT_MUL       = 1,           ///< (in0*coef0 * in1*coef1)
	AI_ELT_MAX       = 2,           ///< MAX(in0*coef0,in1*coef1)
	ENUM_DUMMY4WORD(KDRV_AI_ELT_MODE)
} KDRV_AI_ELT_MODE;
//@}

/**
    select activation mode.
*/
//@{
typedef enum {
	AI_ACT_RELU     = 0,            ///< relu activation
	AI_ACT_TANH     = 1,            ///< tanh activation
	ENUM_DUMMY4WORD(KDRV_AI_ACT_MODE)
} KDRV_AI_ACT_MODE;
//@}

/**
    select pooling mode.
*/
//@{
typedef enum {
	AI_POOL_LOCAL_MAX       = 0,    ///< max local pooling
	AI_POOL_LOCAL_AVG       = 1,    ///< average local pooling
	AI_POOL_GLOBAL_MAX      = 2,    ///< max global pooling
	AI_POOL_GLOBAL_AVG      = 3,    ///< average global pooling
	ENUM_DUMMY4WORD(KDRV_AI_POOL_MODE)
} KDRV_AI_POOL_MODE;
//@}

/**
    select pooling calculate type.
*/
//@{  
typedef enum {
    AI_CEIL         = 0,            ///< ceil output calculation
	AI_FLOOR        = 1,            ///< floor output calculation
	ENUM_DUMMY4WORD(KDRV_AI_POOL_CAL_TYPE)
} KDRV_AI_POOL_CAL_TYPE;
//@}

/**
    select avarage pooling divide type.
*/
//@{   
typedef enum {
    AI_NON_BOUNDARY         = 0,           ///< not consider non-valid boundary point, div_num = ker_w * ker_h
	AI_CONSIDER_BOUNDARY    = 1,           ///< consider non-valid boundary point, div_num = (ker_w-isEndx) * (ker_h- isEndy)
	ENUM_DUMMY4WORD(KDRV_AI_POOL_AVE_DIV_TYPE)
} KDRV_AI_POOL_AVE_DIV_TYPE;
//@}

/**
    select roi pooling mode
*/
//@{    
typedef enum {
	AI_ROIPOOL_ORIGINAL     = 0,                ///< original roi pooling
	AI_PS_ROIPOOL           = 1,                ///< PS roi pooling
	ENUM_DUMMY4WORD(KDRV_AI_ROIPOOL_MODE)
} KDRV_AI_ROIPOOL_MODE;
//@}

/**
    select svm kernel mode
*/
//@{
typedef enum {
	AI_SVMKER_LINEAR   = 0,         ///< linear kernel function
	AI_SVMKER_POLY     = 1,         ///< polynomial kernel function
	AI_SVMKER_RBF      = 2,         ///< RBF kernel function
	AI_SVMKER_SIGMOID  = 3,         ///< sigmoid kernel function
	AI_SVMKER_INTER    = 4,         ///< inter kernel function
	ENUM_DUMMY4WORD(KDRV_AI_SVMKER_MODE)
} KDRV_AI_SVMKER_MODE;
//@}

/**
    select fully connect source format
*/
//@{
typedef enum {
	AI_FC_SRC_FEAT               = 0,   ///< source feature format
	AI_FC_SRC_BATCH_INTERLACE    = 1,   ///< source batch interlace format
	ENUM_DUMMY4WORD(KDRV_AI_FC_SRC_FMT)
} KDRV_AI_FC_SRC_FMT;
//@}

/**
    select permute ordering mode
*/
//@{
typedef enum {
	AI_PERMUTE_CH_W_H    = 0,                   ///< output ordering: channel, width, height
	AI_PERMUTE_H_W_CH    = 1,                   ///< output ordering: height, width, channel
	ENUM_DUMMY4WORD(KDRV_AI_PERMUTE_MODE)
} KDRV_AI_PERMUTE_MODE;
//@}

/**
    select mean subtraction mode
*/
//@{    
typedef enum {
	AI_SUB_DC       = 0,            ///< mean subtraction DC-mode
	AI_SUB_PLANAR   = 1,            ///< mean subtraction Planner-mode
	ENUM_DUMMY4WORD(KDRV_AI_SUB_MODE)
} KDRV_AI_SUB_MODE;
//@}

/**
    select mean subtraction duplicate rate
*/
//@{   
typedef enum {
	AI_SUB_DUP_1x = 0,              ///< mean subtraction input 1x duplicate
	AI_SUB_DUP_2x = 1,              ///< mean subtraction input 2x duplicate
	AI_SUB_DUP_4x = 2,              ///< mean subtraction input 4x duplicate
	AI_SUB_DUP_8x = 3,              ///< mean subtraction input 8x duplicate
	ENUM_DUMMY4WORD(KDRV_AI_SUB_DUP_RATE)
} KDRV_AI_SUB_DUP_RATE;
//@}

/**
    select rotate degree
*/
//@{    
typedef enum {
	AI_ROT_90   = 0,              ///< rotate 90 degree
	AI_ROT_270  = 1,              ///< rotate 270 degree
	AI_ROT_180  = 2,              ///< rotate 180 degree
	ENUM_DUMMY4WORD(KDRV_AI_ROT_MODE)
} KDRV_AI_ROT_MODE;
//@}

/**
    app configuration
*/
//@{
typedef struct _KDRV_AI_APP_HEAD {
	KDRV_AI_MODE mode;              ///< select ai mode (ex: mode=KDRV_AI_MODE_NEURAL)
	UINT32 parm_addr;               ///< model parameter starting address (pa)
	UINT32 parm_size;               ///< model parameter size
	UINT32 stripe_head_addr;        ///< next head of stripe engine
} KDRV_AI_APP_HEAD;
//@}

/**
    linked list configuration
*/
//@{
typedef struct _KDRV_AI_LL_HEAD {
	KDRV_AI_MODE mode;              ///< select ai mode for first engine triggered (ex: mode=KDRV_AI_MODE_NEURAL)
	UINT32 parm_addr;               ///< linked list parameter starting address (pa)
	UINT32 parm_size;               ///< linked list parameter size
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
	KDRV_AI_ENG eng;                ///< engine type
//========== by CCC 191004 ==========
#endif
} KDRV_AI_LL_HEAD;
//@}

/**
    size configuration
*/
//@{
typedef struct _KDRV_AI_SIZE {
	UINT16 width;
	UINT16 height;
	UINT16 channel;
} KDRV_AI_SIZE;
//@}

/**
    offset configuration
*/
//@{
typedef struct _KDRV_AI_OFS {
	UINT32 line_ofs;                ///< line offset for multi-stripe mode or image mode
	UINT32 channel_ofs;             ///< channel offst for multi-stripe mode
	UINT32 batch_ofs;               ///< batch offset for batch mode
#if AI_V4_STRIPE_STRUCT
	UINT8 h_stripe_en;
	UINT8 v_stripe_en;
	UINT8 ch_stripe_en;
#endif
} KDRV_AI_OFS;
//@}

/**
    image padding kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_PAD_KERPARM {
	UINT8 top_pad_num;              ///< number of padding top pixels
	UINT8 bot_pad_num;              ///< number of padding bottom pixels
	UINT8 left_pad_num;             ///< number of padding left pixels
	UINT8 right_pad_num;            ///< number of padding right pixels
	INT32 pad_val;                  ///< padding value based on KDRV_AI_IO_TYPE
} KDRV_AI_PAD_KERPARM;
//@}

/**
    compression kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_FCD_KERPARM {
	UINT32 func_en;                 ///< default: KDRV_AI_FCD_ALL_EN
	UINT32 quanti_kmeans_addr;      ///< input quanti kmeans address
	UINT32 enc_bit_length;          ///< encode bit length
	UINT32 vlc_code_size;           ///< vlc table code size
	UINT32 vlc_valid_size;          ///< vlc table valid size
	UINT32 vlc_ofs_size;            ///< vlc table offset size
	UINT32 *p_vlc_code;             ///< vlc table code
	UINT32 *p_vlc_valid;            ///< vlc table valid
	UINT32 *p_vlc_ofs;              ///< vlc table offset
} KDRV_AI_FCD_KERPARM;
//@}

/**
    scale-shift parameters configuration
*/
//@{    
typedef struct _KDRV_AI_SCLSFT_KERPARM {
	INT8 in_shift;
    INT8 out_shift;
	UINT32 in_scale;
	UINT32 out_scale;
} KDRV_AI_SCLSFT_KERPARM;
//@}

/**
    deconvolution kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_DECONV_KERPARM {
	UINT8 ker_stridex;                  		///< deconvolution kernel stride x
	UINT8 ker_stridey;                  		///< deconvolution kernel stride y
	KDRV_AI_PAD_KERPARM pad;            		///< padding parameters
	KDRV_AI_DECONV_MODE func_sel;               ///< deconv pad or Upsample selection
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_DECONV_KERPARM;
//@}

/**
    convolution kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_CONV_KERPARM {
	UINT32 weight_addr;                         ///< input weight address
	UINT32 bias_addr;                           ///< input bias address
	KDRV_AI_FCD_KERPARM fcd;                    ///< compression parameters
	KDRV_AI_PAD_KERPARM pad;                    ///< padding parameters
	UINT8 ker_w;                                ///< convolution kernel width
	UINT8 ker_h;                                ///< convolution kernel height
	UINT8 ker_stridex;                          ///< convolution kernel stride x
	UINT8 ker_stridey;                          ///< convolution kernel stride y
	UINT16 ker_set_num;                         ///< number of convolution kernel set
	INT8 acc_shf;                               ///< shift after convolution accumulation to avoid overflow; right shift(>0), left shift(<0)
	INT32 norm_scl;                             ///< normalized scale after accumulation
	INT8 norm_shf;                              ///< normalized shift after accumulation; right shift(>0), left shift(<0)
	INT8 bias_para_shf;                         ///< shift of bias before addition; right shift(>0), left shift(<0)
	INT8 conv_shf;                              ///< shift after convolution; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_CONV_KERPARM;
//@}

/**
    bn scale kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_BN_SCALE_KERPARM {
	UINT32 bn_scale_addr;                       ///< packed with conv bias_addr
	INT8 mean_para_shf;                         ///< shift of batchnorm mean; right shift(>0), left shift(<0)
	INT8 bias_para_shf;                         ///< shift of batchnorm bias; right shift(>0), left shift(<0)
	INT8 alpha_para_shf;                        ///< shift of batchnorm alpha; right shift(>0), left shift(<0)
	INT32 norm_scl;                             ///< normalized scale after scaling
	INT8 norm_shf;                              ///< normalized shift after scaling; right shift(>0), left shift(<0)
	INT8 bn_scale_shf;                          ///< shift after bn sacle; right shift(>0), left shift(<0)
} KDRV_AI_BN_SCALE_KERPARM;
//@}

/**
    (for NT96680)
    LRN kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_LRN_KERPARM {
	INT32 alpha;                    ///< input alpha value
	INT32 beta;                     ///< input beta value
	INT16 accr_ch;                  ///< input accross channel number
	INT8 lrn_shf;                   ///< shift after lrn; right shift(>0), left shift(<0)
} KDRV_AI_LRN_KERPARM;
//@}

/**
    convolution normalization kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_NORM_KERPARM {
	KDRV_AI_NORM_MODE mode;             ///< convolution noramalization mode
	KDRV_AI_BN_SCALE_KERPARM bn_scl;    ///< bn scale parameters
	KDRV_AI_LRN_KERPARM lrn;            ///< LRN parameters (for NT96680)
} KDRV_AI_NORM_KERPARM;
//@}

/**
    eltwise kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_ELT_KERPARM {
	KDRV_AI_ELT_MODE mode;                      ///< eltwise mode (add, multiply, max)
	KDRV_AI_IO_TYPE type;                       ///< eltwse input type
	UINT32 addr;                                ///< eltwise input address
	KDRV_AI_OFS in_ofs;                         ///< eltwise input offset (line-offset, channel-offset, batch-offset)
	INT32 coef_scl0;                            ///< multiplier of coefficient0
	INT8  coef_shf0;                            ///< shift of coefficient0; right shift(>0), left shift(<0)
	INT32 coef_scl1;                            ///< multiplier of coefficient1
	INT8  coef_shf1;                            ///< shift of coefficient1; right shift(>0), left shift(<0)
	INT8  norm_shf;                             ///< normalized shift of output results; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_ELT_KERPARM;
//@}

/**
    relu kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_RELU_KERPARM {
	INT32 leaky_val;                ///< multiplier of leaky ratio
	INT8  leaky_shf;                ///< shift of leaky ratio; right shift(>0), left shift(<0)
} KDRV_AI_RELU_KERPARM;
//@}

/**
    activation kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_ACT_KERPARM {
	KDRV_AI_ACT_MODE mode;                      ///< activation mode
	KDRV_AI_RELU_KERPARM relu;                  ///< relu parameters
	BOOL neg_en;                                ///< negation enable
	INT8 act_shf0;                              ///< shift output0 after activation; right shift(>0), left shift(<0)
	INT8 act_shf1;                              ///< shift output1 after activation; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_ACT_KERPARM;
//@}

/**
    global pooling kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_GLOBAL_POOL_KERPARM {
	INT32 avg_mul;                  ///< multiplier of average pooling results
	INT8  avg_shf;                  ///< shift of average pooling results; right shift(>0), left shift(<0)
} KDRV_AI_GLOBAL_POOL_KERPARM;
//@}

/**
    local pooling kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_LOCAL_POOL_KERPARM {
	UINT8 ker_w;                                        ///< pooling kernel width
	UINT8 ker_h;                                        ///< pooling kernel height
	UINT8 ker_stridex;                                  ///< pooling kernel stride x
	UINT8 ker_stridey;                                  ///< pooling kernel stride y
	KDRV_AI_PAD_KERPARM pad;                            ///< padding parameters
	KDRV_AI_POOL_CAL_TYPE pool_cal_type;                ///< pooling calculate type
    KDRV_AI_POOL_AVE_DIV_TYPE pool_div_type;            ///< pooling divide type
} KDRV_AI_LOCAL_POOL_KERPARM;
//@}

/**
    pooling kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_POOL_KERPARM {
	KDRV_AI_POOL_MODE mode;                     ///< select pooling mode
	KDRV_AI_LOCAL_POOL_KERPARM local;           ///< local pooling parameters
	KDRV_AI_GLOBAL_POOL_KERPARM global;         ///< global pooling parameters
	INT8  pool_shf;                             ///< shift after pooling; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_POOL_KERPARM;
//@}

/**
    roi pooling kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_ROIPOOL_KERPARM {
	KDRV_AI_ROIPOOL_MODE mode;                  ///< roi pooling mode
	UINT8 pool_w;                               ///< output roi width
	UINT8 pool_h;                               ///< output roi height
	UINT32 roi_addr;                            ///< input roi address (top-left_x0, top-left_y0, bottom-right_x0, bottom-right_y0, ...)
	UINT16 roi_num;                             ///< number of roi
	KDRV_AI_OFS out_ofs;                        ///< output offset (only use batch-offset)
	INT32 ratio_mul;                            ///< multiplier of scale ratio
	INT8  ratio_shf;                            ///< shift of scale ratio; right shift(>0), left shift(<0)
	INT8  roipool_shf;                          ///< shift after roi pooling; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_ROIPOOL_KERPARM;
//@}

/**
    svm kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_SVM_KERPARM {
	KDRV_AI_SVMKER_MODE ker_mode;               ///< svm mode (linear, poly, inter, rbf, sigmoid)
	UINT16 sv_width;                            ///< sv width
	UINT16 sv_height;                           ///< sv height
	UINT32 sv_addr;                             ///< input sv address
	UINT32 alpha_addr;                          ///< input alpha address
	INT8  ft_shf;                               ///< feature shift; right shift(>0), left shift(<0)
	INT32 gamma;                                ///< input gamma value
	INT8  gamma_shf;                            ///< gamma shift; right shift(>0), left shift(<0)
	INT32 coef;                                 ///< input coefficient value
	INT8  degree;                               ///< input polynomial degree
	INT8  alpha_shf;                            ///< alpha shift; right shift(>0), left shift(<0)
	INT32 rho;                                  ///< input rho value
	INT8  rho_fmt;                              ///< shift before rho substraction; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_SVM_KERPARM;
//@}

/**
    fully connect kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_FC_KERPARM {
	UINT32 weight_addr;                         ///< input weighting address
	UINT32 bias_addr;                           ///< input bias address
	UINT32 weight_w;                            ///< weighting width
	UINT32 weight_h;                            ///< weighting height
	INT8 weight_shf;                            ///< shift after weighting; right shift(>0), left shift(<0)
	INT8 acc_shf;                               ///< shift after accumulate; right shift(>0), left shift(<0)
	INT32 norm_scl;                             ///< normalized scale of output results
	INT8 norm_shf;                              ///< normalized shift of output results; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_FC_KERPARM;
//@}

/**
    permute kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_PERMUTE_KERPARM {
	KDRV_AI_PERMUTE_MODE mode;                  ///< permute mode
	INT8 permute_shf;                           ///< shift after permute; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_PERMUTE_KERPARM;
//@}

/**
    reorganize kernel parameters configuration
*/
//@{
typedef struct _KDRV_AI_REORG_KERPARM {
	INT8 reorg_shf;                             ///< shift after reorganize; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_REORG_KERPARM;
//@}

/**
    anchor transform kernel parameters configuration
*/
//@{	
typedef struct _KDRV_AI_ANCHOR_KERPARM {
	INT8 anchor_w_shf;                          ///< shift before anchor transform; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} KDRV_AI_ANCHOR_KERPARM;
//@}

/**
    softmax kernel parameters configuration
*/
//@{	
typedef struct _KDRV_AI_SOFTMAX_KERPARM {
	INT8 softmax_in_shf;                        ///< shift before softmax; right shift(>0), left shift(<0)
	INT8 softmax_out_shf;                       ///< shift after softmax; right shift(>0), left shift(<0)
	KDRV_AI_SCLSFT_KERPARM sclshf;       	    ///< scale-shift-0
} KDRV_AI_SOFTMAX_KERPARM;
//@}

/**
    scaling parameters configuration
*/
//@{	
typedef struct _KDRV_AI_SCALE_KERPARM {
	KDRV_AI_SIZE scl_out_size;                          ///< output size
    BOOL fact_update_en;                                ///< enable/disable updating down rate and scaling factor
} KDRV_AI_SCALE_KERPARM;
//@}

/**
    mean subtraction parameters configuration
*/
//@{	
typedef struct _KDRV_AI_SUB_KERPARM {
	KDRV_AI_SUB_MODE sub_mode;                  ///< mean subtraction mode selection
	KDRV_AI_SUB_DUP_RATE dup_rate;              ///< mean subtraction input duplicate rate
	UINT32 sub_in_w;                            ///< mean subtraction input width
	UINT32 sub_in_h;                            ///< mean subtraction input height
    UINT8 sub_dc_coef[3];                       ///< mean subtraction DC mode coefficient
    INT8 sub_shf;                               ///< shift after mean subtraction; right shift(>0), left shift(<0)
} KDRV_AI_SUB_KERPARM;
//@}

/**
    crop and padding parameters configuration
*/
//@{	
typedef struct _KDRV_AI_CROPPAD_KERPARM {
	UINT32 crop_x;                           ///< starting X of cropping
	UINT32 crop_y;                           ///< starting Y of cropping
	UINT32 crop_w;                           ///< width of cropping
    UINT32 crop_h;                           ///< height of cropping
    UINT32 pad_out_x;                        ///< starting X of padding after crop
    UINT32 pad_out_y;                        ///< starting Y of padding after crop
    UINT32 pad_out_w;                        ///< width of output
    UINT32 pad_out_h;                        ///< height of output
    UINT32 pad_val[3];                       ///< padding value
} KDRV_AI_CROPPAD_KERPARM;
//@}

/**
    rotate parameters configuration
*/
//@{	
typedef struct _KDRV_AI_ROTATE_KERPARM {
	KDRV_AI_ROT_MODE rot_mode;               ///< rotate degree selection
} KDRV_AI_ROTATE_KERPARM;
//@}

/**
    Neural Network Processing parameters
    de-convolution -> convolution -> normalization -> eltwise -> activation -> pooling
*/
//@{
typedef struct _KDRV_AI_NEURAL_PARM {
	UINT8 func_list[KDRV_AI_NEURAL_FUNC_CNT];
	KDRV_AI_NEURAL_SRC_FMT src_fmt;             ///< input source format
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out0_type;                  ///< output0 type
	KDRV_AI_IO_TYPE out1_type;                  ///< output1 type
	KDRV_AI_SIZE size;                          ///< input size
	UINT16 batch_num;                           ///< batch mode if batch>1
	UINT32 in_addr;                             ///< input address
	UINT32 out0_addr;                           ///< output0 address
	UINT32 out1_addr;                           ///< output1 address
	UINT32 in_interm_addr;                      ///< input intermediate address from convolution
	UINT32 out_interm_addr;                     ///< output intermediate address from convolution
	UINT32 tmp_buf_addr;                        ///< temporarily buffer address for intermediate output
	INT8 out0_dma_en;                           ///< enable/disable ouput to dram0
	INT8 out1_dma_en;                           ///< enable/disable ouput to dram1
	INT8 in_interm_dma_en;                      ///< enable/disable input intermediate to dram
	INT8 out_interm_dma_en;                     ///< enable/disable ouput intermediate to dram
	KDRV_AI_OFS in_ofs;                         ///< input offset (line-offset, channel-offset, batch-offset)
	KDRV_AI_OFS out0_ofs;                       ///< output0 offset (line-offset, channel-offset, batch-offset)
	KDRV_AI_OFS out1_ofs;                       ///< output1 offset (line-offset, channel-offset, batch-offset)
	UINT16 out0_cropy;                          ///< crop output0 results
	KDRV_AI_DECONV_KERPARM deconv;              ///< deconvolution parameters
	KDRV_AI_CONV_KERPARM conv;                  ///< convolution parameters
	KDRV_AI_NORM_KERPARM norm;                  ///< convolution normalized parameters
	KDRV_AI_ELT_KERPARM elt;                    ///< eltwise parameters
	KDRV_AI_ACT_KERPARM preact;                 ///< pre-activation parameters
	KDRV_AI_ACT_KERPARM act;                    ///< activation-0 parameters
	KDRV_AI_ACT_KERPARM act1;                   ///< activation-1 parameters
	KDRV_AI_POOL_KERPARM pool;                  ///< pooling parameters
} KDRV_AI_NEURAL_PARM;
//@}

/**
    roi pooling parameters configuration
*/
//@{
typedef struct _KDRV_AI_ROIPOOL_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_ROIPOOL_KERPARM roi_ker;            ///< roi pooling kernel parameters
} KDRV_AI_ROIPOOL_PARM;
//@}

/**
    svm parameters configuration
*/
//@{
typedef struct _KDRV_AI_SVM_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	UINT16 ft_size;                             ///< input feature size
	UINT16 obj_num;                             ///< input object number
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_SVM_KERPARM svm_ker;                ///< svm kernel parameters
	KDRV_AI_FCD_KERPARM fcd;                    ///< compression parameters
} KDRV_AI_SVM_PARM;
//@}

/**
    fully connect parameters configuration
*/
//@{
typedef struct _KDRV_AI_FC_PARM {
	UINT8 func_list[KDRV_AI_FC_FUNC_CNT];
	KDRV_AI_FC_SRC_FMT src_fmt;                 ///< input source format
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT16 batch_num;                           ///< number of batch
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	UINT32 in_interm_addr;                      ///< input intermediate address from inner product
	UINT32 out_interm_addr;                     ///< output intermediate address from inner product
	INT8 in_interm_dma_en;                      ///< enable/disable input intermediate to dram
	INT8 out_interm_dma_en;                     ///< enable/disable ouput intermediate to dram
	KDRV_AI_OFS in_interm_ofs;                  ///< input intermediate offset (line-offset, channel-offset, batch-offset)
	KDRV_AI_FC_KERPARM fc_ker;                  ///< fully connect kernel parameters
	KDRV_AI_ACT_KERPARM act;                    ///< activation parameters
	KDRV_AI_FCD_KERPARM fcd;                    ///< compression parameters
} KDRV_AI_FC_PARM;
//@}

/**
    permute parameters configuration
*/
//@{
typedef struct _KDRV_AI_PERMUTE_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_PERMUTE_KERPARM perm_ker;           ///< permute kernel parameters
} KDRV_AI_PERMUTE_PARM;
//@}

/**
    reorganize parameters configuration
*/
//@{
typedef struct _KDRV_AI_REORG_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_REORG_KERPARM reorg_ker;            ///< reorganize kernel parameters
} KDRV_AI_REORG_PARM;
//@}

/**
    anchor transform parameters configuration
*/
//@{   
typedef struct _KDRV_AI_ANCHOR_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT32 in_addr;                             ///< input address
    UINT32 w_addr;                              ///< weight address
    UINT32 b_addr;                              ///< bias address
    UINT32 tbl_addr;                            ///< exp table address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_ANCHOR_KERPARM anchor_ker;          ///< anchor transform kernel parameters
} KDRV_AI_ANCHOR_PARM;
//@}

/**
    softmax parameters configuration
*/
//@{   
typedef struct _KDRV_AI_SOFTMAX_PARM {
	KDRV_AI_IO_TYPE in_type;                    ///< input type
	KDRV_AI_IO_TYPE out_type;                   ///< output type
	KDRV_AI_SIZE size;                          ///< input size
	UINT32 in_addr;                             ///< input address
	UINT32 out_addr;                            ///< output address
	KDRV_AI_SOFTMAX_KERPARM softmax_ker;        ///< softmax kernel parameters
} KDRV_AI_SOFTMAX_PARM;
//@}

/**
    preprocessing parameters configuration
*/
//@{   
typedef struct _KDRV_AI_PREPROC_PARM {
    UINT8 func_list[KDRV_AI_PREPROC_FUNC_CNT];
    KDRV_AI_PREPROC_SRC_FMT src_fmt;                ///< input source format
    KDRV_AI_PREPROC_SRC_FMT rst_fmt;                ///< output source format
	KDRV_AI_IO_TYPE in_type;                        ///< input type
	KDRV_AI_IO_TYPE out_type;                       ///< output type
	KDRV_AI_SIZE in_size;                           ///< input size
	KDRV_AI_OFS in_ofs[3];                          ///< input offset (line-offset, channel-offset, batch-offset)
	KDRV_AI_OFS out_ofs[3];                         ///< output offset (line-offset, channel-offset, batch-offset)
	UINT32 in_addr[3];                              ///< input channel address
	UINT32 out_addr[3];                             ///< output channel address
	KDRV_AI_SCALE_KERPARM scale_ker;                ///< scaling parameters
	KDRV_AI_SUB_KERPARM sub_ker;                    ///< mean subtraction parameters
	KDRV_AI_CROPPAD_KERPARM pad_ker;                ///< crop and padding parameters
	KDRV_AI_ROTATE_KERPARM rotate_ker;          	///< rotate parameters
} KDRV_AI_PREPROC_PARM;
//@}

/**
    KDRV AI structure - ai open object
*/
typedef struct _KDRV_AI_OPENCFG {
	UINT32          clock_sel;                      ///< Engine clock selection
} KDRV_AI_OPENCFG, *pKDRV_AI_OPENCFG;

/**
    KDRV AI structure
    AI app structure
*/
typedef struct _KDRV_AI_APP_INFO {
	KDRV_AI_APP_HEAD *p_head;
	UINT32 head_cnt;
} KDRV_AI_APP_INFO, *pKDRV_AI_APP_INFO;

/**
    KDRV AI structure
    AI linked list structure
*/
typedef struct _KDRV_AI_LL_INFO {
	KDRV_AI_LL_HEAD *p_head;
	UINT32 head_cnt;
} KDRV_AI_LL_INFO, *pKDRV_AI_LL_INFO;

/**
    parameter data for kdrv_ai_set, kdrv_ai_get
*/
typedef enum {
	KDRV_AI_PARAM_OPENCFG = 0,                  ///< set engine open info,  data_type: KDRV_AI_OPENCFG

	KDRV_AI_PARAM_MODE_INFO,                    ///< set mode, data_type: KDRV_AI_TRIG_MODE
	KDRV_AI_PARAM_APP_INFO,                     ///< set app parameters, data_type: KDRV_AI_APP_INFO
	KDRV_AI_PARAM_LL_INFO,                      ///< set ll parameters, data_type: KDRV_AI_LL_INFO
	KDRV_AI_PARAM_ISR_CB,						///< set isr callback, data_type: KDRV_AI_ISRCB

	KDRV_AI_PARAM_MAX,
	KDRV_AI_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_AI_PARAM_ID)
} KDRV_AI_PARAM_ID;
//STATIC_ASSERT((KDRV_AI_PARAM_MAX &KDRV_AI_PARAM_REV) == 0);

#define AI_MAX_USR_LAYER_NUM 3
#define AI_MAX_ROI_NUM   80
#define AI_MAX_IN_NUM    3
#define AI_MAX_OUT_NUM   3
typedef struct _AI_USR_LAYER_INFO {
	UINT32 in_layer_idx[AI_MAX_USR_LAYER_NUM];                    // layer index of user define input address layer
	UINT32 out_layer_idx[AI_MAX_USR_LAYER_NUM];                   // layer index of user define output address layer
	UINT32 in_layer_num;                      				      // layer num of user define input address layer
	UINT32 out_layer_num;                                         // layer num of user define output address layer
	UINT32 roi_num;                                               // roi number
	UINT32 in_layer_addr[AI_MAX_USR_LAYER_NUM][AI_MAX_ROI_NUM][AI_MAX_IN_NUM];   // physical address of user define input address layer
	UINT32 out_layer_addr[AI_MAX_USR_LAYER_NUM][AI_MAX_ROI_NUM][AI_MAX_OUT_NUM];  // physical address of user define output address layer
	UINT32 total_layer_num;
} AI_USR_LAYER_INFO;
#define KDRV_AI_IGN_CHK     KDRV_AI_PARAM_REV	//only support set/get function


#if 0
#ifdef __KERNEL__
extern void kdrv_ai_install_id(void);
extern void kdrv_ai_uninstall_id(void);
#else
extern void kdrv_ai_install_id(void) _SECTION(".kercfg_text");
extern void kdrv_ai_uninstall_id(void) _SECTION(".kercfg_text");
#endif
#endif

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
/*!
 * @fn INT32 kdrv_ai_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ai_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_ai_set(UINT32 id, KDRV_AI_PARAM_ID param_id, void* p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_set(UINT32 id, KDRV_AI_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_ai_get(UINT32 id, KDRV_AI_PARAM_ID param_id, void* p_param)
 * @brief get parameters from hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_get(UINT32 id, KDRV_AI_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_ai_trigger(UINT32 id,  KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id                the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_cb_func         the callback function
 * @param p_user_data       the private user data
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_trigger(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data);

/*!
 * @fn INT32 kdrv_ai_wait_done(UINT32 id,  KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data)
 * @brief wait hardware engine done
 * @param id                the id of hardware
 * @param p_param           the parameter for trigger
 * @param p_cb_func         the callback function
 * @param p_user_data       the private user data
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_waitdone(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data);

/*!
 * @fn INT32 kdrv_ai_reset(UINT32 id)
 * @brief reset hardware engine
 * @param id                the id of hardware
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_ai_reset(UINT32 id, KDRV_AI_TRIGGER_PARAM *p_param);

extern INT32 kdrv_ai_set_ll_base_addr(KDRV_AI_ENG eng, UINT32 addr);

extern UINT32 kdrv_ai_get_eng_caps(KDRV_AI_ENG eng);

extern UINT8 * kdrv_ai_vmalloc(UINT32 v_size);

extern VOID kdrv_ai_vfree(UINT8 *v_buff);

extern VOID kdrv_ai_reset_status(VOID);

extern void kdrv_ai_config_flow(UINT32 flow);

extern INT32 kdrv_ai_dma_abort(UINT32 chip, UINT32 engine);

extern INT32 nvt_ai_is_do_ai_init(VOID);

#endif
