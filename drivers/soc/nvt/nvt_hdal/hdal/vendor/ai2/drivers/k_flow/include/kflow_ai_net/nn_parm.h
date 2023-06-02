/**
    @brief Header file of NN engine definition of vendor net flow sample.

    @file nn_parm.h

    @ingroup net_flow_sample

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NN_PARM_H_
#define _NN_PARM_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#include "kwrap/type.h"
#include "nn_net.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#if AI_V4
#define NN_NET_PARM_VERSION             "1.02.001"
#elif USE_NEON
#define NN_NET_PARM_VERSION             "1.01.000"
#else
#define NN_NET_PARM_VERSION             "1.00.002"
#endif

#define NN_PRIORBOX_SIZE_NUM            4
#define NN_PRIORBOX_ASPECT_RATIO_NUM    4
#define NN_PRIORBOX_VAR_NUM             4

#define NN_LSTM_PARM_NUM                4

#define NN_AXIS_NUM                     4

/********************************************************************
    TYPE DEFINITION
********************************************************************/
/**
    Input/Output type.
*/
//@{
typedef enum {
	NN_AI_IO_INT8 = 0,
	NN_AI_IO_UINT8 = 1,
	NN_AI_IO_INT16 = 2,
	NN_AI_IO_UINT16 = 3,
	ENUM_DUMMY4WORD(NN_AI_IO_TYPE)
} NN_AI_IO_TYPE;
//@}

/**
	select pooling mode.
*/
//@{
typedef enum {
	NN_AI_POOL_LOCAL_MAX = 0,       ///< max local pooling
	NN_AI_POOL_LOCAL_AVG = 1,       ///< average local pooling
	NN_AI_POOL_GLOBAL_MAX = 2,      ///< max global pooling
	NN_AI_POOL_GLOBAL_AVG = 3,      ///< average global pooling
	ENUM_DUMMY4WORD(NN_AI_POOL_MODE)
} NN_AI_POOL_MODE;
//@}

/**
    select pooling calculate type.
*/
//@{
typedef enum {
	NN_AI_CEIL         = 0,            ///< ceil output calculation
	NN_AI_FLOOR        = 1,            ///< floor output calculation
	ENUM_DUMMY4WORD(NN_AI_POOL_CAL_TYPE)
} NN_AI_POOL_CAL_TYPE;
//@}

/**
    select avarage pooling divide type.
*/
//@{
typedef enum {
	NN_AI_NON_BOUNDARY         = 0,           ///< not consider non-valid boundary point, div_num = ker_w * ker_h
	NN_AI_CONSIDER_BOUNDARY    = 1,           ///< consider non-valid boundary point, div_num = (ker_w-isEndx) * (ker_h- isEndy)
	ENUM_DUMMY4WORD(NN_AI_POOL_AVE_DIV_TYPE)
} NN_AI_POOL_AVE_DIV_TYPE;
//@}

/**
	image padding kernel parameters configuration
*/
//@{
typedef struct _NN_AI_PAD_KERPARM {
	UINT8 top_pad_num;              ///< number of padding top pixels
	UINT8 bot_pad_num;              ///< number of padding bottom pixels
	UINT8 left_pad_num;             ///< number of padding left pixels
	UINT8 right_pad_num;            ///< number of padding right pixels
	INT32 pad_val;                  ///< padding value based on KDRV_AI_IO_TYPE
} NN_AI_PAD_KERPARM;
//@}


/**
    scale-shift parameters configuration
*/
//@{
typedef struct _NN_AI_SCLSFT_KERPARM {
	INT8 in_shift;
	INT8 out_shift;
	UINT32 in_scale;
	UINT32 out_scale;
} NN_AI_SCLSFT_KERPARM;
//@}

/**
	local pooling kernel parameters configuration
*/
//@{
typedef struct _NN_AI_LOCAL_POOL_KERPARM {
	UINT8 ker_w;                    ///< pooling kernel width
	UINT8 ker_h;                    ///< pooling kernel height
	UINT8 ker_stridex;              ///< pooling kernel stride x
	UINT8 ker_stridey;              ///< pooling kernel stride y
	NN_AI_PAD_KERPARM pad;        ///< padding parameters
	NN_AI_POOL_CAL_TYPE pool_cal_type;                ///< pooling calculate type
	NN_AI_POOL_AVE_DIV_TYPE pool_div_type;            ///< pooling divide type
} NN_AI_LOCAL_POOL_KERPARM;
//@}

/**
    Global pooling kernel parameters.
*/
//@{
typedef struct _NN_AI_GLOBAL_POOL_KERPARM {
	INT32 avg_mul;                  ///< multiplier of average pooling results
	INT8  avg_shf;                  ///< shift of average pooling results; right shift(>0), left shift(<0)
} NN_AI_GLOBAL_POOL_KERPARM;
//@}

/**
	pooling kernel parameters configuration
*/
//@{
typedef struct _NN_AI_POOL_KERPARM {
	NN_AI_POOL_MODE mode;             ///< select pooling mode
	NN_AI_LOCAL_POOL_KERPARM local;   ///< local pooling parameters
	NN_AI_GLOBAL_POOL_KERPARM global; ///< global pooling parameters
	INT8  pool_shf;                     ///< shift after pooling; right shift(>0), left shift(<0)
	NN_AI_SCLSFT_KERPARM sclshf;              ///< scale-shift
} NN_AI_POOL_KERPARM;
//@}

/**
    Size parameters.
*/
//@{
typedef struct _NN_AI_SIZE {
	UINT16 width;                           ///< width
	UINT16 height;                          ///< height
	UINT16 channel;                         ///< number of channels
} NN_AI_SIZE;
//@}

/**
    Shape parameters.
*/
//@{
typedef struct _NN_SHAPE {
	UINT16 width;                           ///< width
	UINT16 height;                          ///< height
	UINT16 channel;                         ///< number of channels
	UINT16 batch_num;                       ///< batch number
} NN_SHAPE;
//@}

/**
    Offset parameters.
*/
//@{
typedef struct _NN_AI_OFS {
	UINT32 line_ofs;                        ///< line offset for multi-stripe mode or image mode
	UINT32 channel_ofs;                     ///< channel offst for multi-stripe mode
	UINT32 batch_ofs;                       ///< batch offset for batch mode
} NN_AI_OFS;
//@}

/**
    Scale factor parameters.
*/
//@{
typedef struct _NN_SF {
	INT32 mul;                              ///< multiply
	INT8 shf;                               ///< shift; right shift(>0), left shift(<0)
} NN_SF;
//@}

typedef NN_AI_SIZE  NN_SIZE;
typedef NN_AI_OFS   NN_OFS;

/**
    CPU layer parameters.
*/
//@{
typedef struct _NN_CPU_PARM {
	UINT32 addr_in;
	UINT32 addr_out;
	UINT32 size_in;
	UINT32 size_out;
	UINT32 width;
	UINT32 height;
	UINT32 channel;
	UINT32 batch;
	UINT32 in_lineofs;
	UINT32 in_channelofs;
	UINT32 in_batchofs;
	UINT32 out_lineofs;
	UINT32 out_channelofs;
	UINT32 out_batchofs;
} NN_CPU_PARM;
//@}

/**
    DSP layer parameters.
*/
//@{
typedef struct _NN_DSP_PARM {
	UINT32 addr_in;
	UINT32 addr_out;
	UINT32 size_in;
	UINT32 size_out;
} NN_DSP_PARM;
//@}

/**
    FullyConnected layer post-processing parameters.
*/
//@{
typedef struct _NN_FC_POST_PARM {
#if AI_V4
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address
	UINT32 bias_addr;                       ///< bias address
	NN_SHAPE shape;                         ///< input shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
	INT32 leaky_value;
	INT8 leaky_shift;
#else
	UINT8 in_bitdepth;
	UINT8 in_sign;
	UINT8 out_bitdepth;
	UINT8 out_sign;
	UINT32 addr_in;
	UINT32 addr_out;
	UINT32 bias_addr;
	UINT32 width;
	UINT32 height;
	UINT32 in_lofs;
	INT32 norm_scale;
	INT8 norm_shift;
	INT32 leaky_value;
	INT8 leaky_shift;
#endif
} NN_FC_POST_PARM;

//@}

/**
    Pooling layer parameters.
*/
//@{
typedef struct _NN_POOL_PARM {
#if AI_V4
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address
	NN_SHAPE shape;                         ///< input shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
	NN_AI_POOL_KERPARM pool;                ///< pooling parameters
#else
	NN_AI_IO_TYPE in_type;                  ///< input type
	NN_AI_IO_TYPE out_type;                 ///< output type
	NN_AI_SIZE size;                        ///< input size
	UINT16 batch_num;                       ///< batch number
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	NN_AI_OFS in_ofs;                       ///< input offset
	NN_AI_OFS out_ofs;                      ///< output offset
	NN_AI_POOL_KERPARM pool;                ///< pooling parameters
	INT32 norm_scale;                       ///< osf multiply
	INT8 norm_shift;                        ///< osf shift; right shift(>0), left shift(<0)
#endif
} NN_POOL_PARM;
//@}

/**
    BatchNormalization/Scale layer parameters.
*/
//@{
typedef struct _NN_BNSCALE_PARM {
	UINT8 in_bitdepth;
	UINT8 in_sign;
	UINT8 out_bitdepth;
	UINT8 out_sign;
	UINT32 in_addr;
	UINT32 out_addr;
	UINT32 mean_addr;
	UINT32 alpha_addr;
	UINT32 beta_addr;
	UINT32 width;
	UINT32 height;
	UINT32 channel;
	INT8 bn_shf_m;
	INT8 bn_shf_b;
	INT32 norm_scl;
	INT8 norm_shf;
	INT32 leaky_value;
	INT8 leaky_shift;
	INT8 relu_shift;
} NN_BNSCALE_PARM;
//@}

/**
    Softmax layer parameters.
*/
//@{
typedef struct _NN_SOFTMAX_PARM {
#if AI_V4
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address
	NN_SHAPE shape;                         ///< input/output shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
	INT8 axis;                              ///< axis to perform softmax
#else
	UINT32 in_addr;
	UINT32 out_addr;
	UINT32 width;
	UINT32 height;
	UINT32 channel;
	UINT32 batch_num;
	NN_FMT in_bit_fmt;
	NN_FMT out_bit_fmt;
#if ((!CNN_25_MATLAB) || USE_NEON)
	INT8 axis;
	UINT32 in_trans_addr;
	UINT32 out_trans_addr;
#endif
#endif
} NN_SOFTMAX_PARM;
//@}

/**
    Post-processing parameters.
*/
//@{
typedef struct _NN_POSTPROC_PARM {
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address
#if AI_V4
	NN_SHAPE shape;                         ///< input/output shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
#else
#if ((!CNN_25_MATLAB) || USE_NEON)
	UINT32 width;
	UINT32 height;
#endif
	UINT32 channel;
	UINT32 batch_num;
#endif
	UINT32 top_n;
} NN_POSTPROC_PARM;
//@}

/**
    PReLU layer parameters.
*/
//@{
typedef struct _NN_PRELU_PARM {
	UINT32 in_addr;
	UINT32 out_addr;
	UINT32 slope_addr;
	UINT32 width;
	UINT32 height;
	UINT32 channel;
	UINT32 batch_num;
	NN_FMT in_bit_fmt;
	NN_FMT out_bit_fmt;
	NN_FMT slope_bit_fmt;
	UINT8 channel_shared;
	INT32 out_scale;                    ///< output scale
	INT8 out_shift;                     ///< output shift; right shift(>0), left shift(<0)
} NN_PRELU_PARM;
//@}

/**
    LRN layer parameters.
*/
//@{
typedef struct _NN_LRN_PARM {
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
#if AI_V4
	UINT32 tmp_addr;                        ///< temporary buffer address
	NN_SHAPE shape;                         ///< input/output shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
#else
	UINT32 scale_addr;
	UINT32 padded_square_addr;
	UINT32 width;
	UINT32 height;
	UINT32 channels;
	UINT32 num;
#endif
	UINT32 pre_pad;
	UINT32 size;
	UINT32 k;
	FLOAT  alpha;
	FLOAT  beta;
} NN_LRN_PARM;
//@}

/**
    PriorBox layer parameters.
*/
//@{
typedef struct _NN_PRIORBOX_PARM {
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
#if AI_V4
	UINT32 tmp_addr;                        ///< temporary buffer address
#endif
	UINT32 in_width;
	UINT32 in_height;
	UINT32 img_width;
	UINT32 img_height;
	UINT32 out_channel_ofs;
#if AI_V4
	NN_FMT out_fmt;                         ///< output format
	NN_SF osf;                              ///< output scale factor
#else
	NN_FMT out_bit_fmt;
#endif
	FLOAT min_sizes[NN_PRIORBOX_SIZE_NUM];
	FLOAT max_sizes[NN_PRIORBOX_SIZE_NUM];
#if AI_V4
	UINT32 min_size_num;                    ///< number of max sizes
	UINT32 max_size_num;                    ///< number of min sizes
#else
	UINT32 size_num;
#endif
	FLOAT aspect_ratios[NN_PRIORBOX_ASPECT_RATIO_NUM];
	UINT32 aspect_ratio_num;
	UINT8 flip;
	UINT8 clip;
	FLOAT variances[NN_PRIORBOX_VAR_NUM];
	UINT32 variance_num;
	FLOAT offset;
#if !AI_V4
	UINT32 in_trans_addr;
	UINT32 out_trans_addr;
#endif
} NN_PRIORBOX_PARM;
//@}

// Detection Output ------------------------------------------------------------
/**
	PriorBox layer: code type.
*/
typedef enum {
	NN_PRIORBOX_CODE_CENTER = 0,        ///< caffe.PriorBoxParameter.CENTER_SIZE
	NN_PRIORBOX_CODE_CORNER = 1,        ///< caffe.PriorBoxParameter.CORNER
	NN_PRIORBOX_CODE_CORNER_SIZE = 2,	///< caffe.PriorBoxParameter.CORNER_SIZE
	ENUM_DUMMY4WORD(NN_PRIROBOX_CODE_TYPE)
} NN_PRIORBOX_CODE_TYPE;

/**
	DetectionOutput layer: input type.
*/
//@{
typedef enum {
	NN_DETOUT_IN_LOC = 0,               ///< location
	NN_DETOUT_IN_CONF = 1,              ///< confidence
	NN_DETOUT_IN_PRIOR = 2,             ///< prior box

	NN_DETOUT_IN_NUM,                   ///< number of input
	ENUM_DUMMY4WORD(NN_DETOUT_IN_TYPE)
} NN_DETOUT_IN_TYPE;
//@}

/**
    DetectionOutput layer parameters.
*/
//@{
typedef struct _NN_DETOUT_PARM {
	UINT32 in_addr[NN_DETOUT_IN_NUM];       ///< input addresses
	UINT32 out_addr;                        ///< output address
#if AI_V4
	UINT32 tmp_addr;                        ///< temporary buffer address
	UINT16 batch_num;                       ///< batch number
	NN_OFS in_ofs[NN_DETOUT_IN_NUM];        ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt[NN_DETOUT_IN_NUM];        ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf[NN_DETOUT_IN_NUM];            ///< input scale factor
	NN_SF osf;                              ///< output scale factor
#else
	UINT32 batch_num;                       ///< batch number
	UINT32 img_width;                       ///< network input width
	UINT32 img_height;                      ///< network input height
	NN_FMT in_bit_fmt[NN_DETOUT_IN_NUM];    ///< input bit format
	NN_FMT out_bit_fmt;                     ///< output bit format
#endif
	UINT32 num_classes;                     ///< number of classes
	UINT32 num_priors;                      ///< number of prior boxes
	BOOL share_loc;                         ///< share location
	UINT32 bg_lbl_id;                       ///< background label id
	NN_PRIORBOX_CODE_TYPE code_type;        ///< code type
	BOOL var_enc_in_target;                 ///< variance encoded in_target
	UINT32 keep_top_k;                      ///< keep top k
	FLOAT conf_thresh;                      ///< confidence threshold
	FLOAT nms_thresh;                       ///< NMS threshold
	UINT32 top_k;                           ///< top k
	FLOAT eta;                              ///< eta
#if !AI_V4
	UINT32 in_trans_addr;                   ///< input bit-transform buffer address
	UINT32 out_trans_addr;                  ///< output bit-transform buffer address
	UINT32 tmp_addr;                        ///< temporary buffer address
#endif
} NN_DETOUT_PARM;
//@}
// Detection Output end --------------------------------------------------------

/**
    LSTM layer parameters.
*/
//@{
typedef struct _NN_LSTM_PARM {
	UINT32 in_addr0;                            ///< input-0 address
	UINT32 in_addr1;                            ///< input-1 address
	UINT32 out_addr;                            ///< output address
	UINT32 tmp_addr;                            ///< temporary buffer address
	UINT32 indicator_parm_addr;                 ///< indicator buffer address
	UINT32 feat_parm_addr[NN_LSTM_PARM_NUM];    ///< feature parameter address
	UINT32 static_parm_addr[NN_LSTM_PARM_NUM];  ///< static parameter address
	UINT32 hidden_parm_addr[NN_LSTM_PARM_NUM];  ///< hidden parameter address
	UINT32 bias_parm_addr[NN_LSTM_PARM_NUM];    ///< bias parameter address
	NN_SHAPE in_shape0;                         ///< input-0 shape
	NN_SHAPE in_shape1;                         ///< input-1 shape
	NN_OFS in_ofs0;                             ///< input-0 offset
	NN_OFS in_ofs1;                             ///< input-1 offset
	NN_OFS out_ofs;                             ///< output offset
	NN_FMT in_fmt0;                             ///< input-0 format
	NN_FMT in_fmt1;                             ///< input-1 format
	NN_FMT out_fmt;                             ///< output format
	NN_SF isf0;                                 ///< input-0 scale factor
	NN_SF isf1;                                 ///< input-1 scale factor
	NN_SF osf;                                  ///< output scale factor
	BOOL has_static_input;                      ///< Has static input, if enable, has 2 input
	UINT32 num_output;                          ///< feat number
}NN_LSTM_PARM;
//@}

/**
    Permute layer parameters.
*/
//@{
typedef struct _NN_PERMUTE_PARM {
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address (unused)
	NN_SHAPE shape;                         ///< input/output shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
	INT8 order[NN_AXIS_NUM];                ///< permute order
} NN_PERMUTE_PARM;
//@}

/**
    Reverse layer parameters.
*/
//@{
typedef struct _NN_REVERSE_PARM {
	UINT32 in_addr;                         ///< input address
	UINT32 out_addr;                        ///< output address
	UINT32 tmp_addr;                        ///< temporary buffer address (unused)
	NN_SHAPE shape;                         ///< input/output shape
	NN_OFS in_ofs;                          ///< input offset
	NN_OFS out_ofs;                         ///< output offset
	NN_FMT in_fmt;                          ///< input format
	NN_FMT out_fmt;                         ///< output format
	NN_SF isf;                              ///< input scale factor
	NN_SF osf;                              ///< output scale factor
	INT8 axis;                              ///< axis to perform the operation
} NN_REVERSE_PARM;
//@}

// Custom ----------------------------------------------------------------------
/**
    Custom layer parameters.
*/
//@{
#if CUST_SUPPORT_MULTI_IO
typedef struct _NN_CUSTOM_PARM {
	UINT32 input_num;
	UINT32 output_num;
	UINT32 model_num;
	UINT32 temp_buf_addr;
	UINT32 temp_buf_size;
	UINT32 parm_size;
	/*
	NN_DATA* input;    // size = sizeof(NN_DATA)*input_num
	NN_DATA* output; // size = sizeof(NN_DATA)*output_num
	NN_DATA* model;  // size = sizeof(NN_DATA)*model_num
	NN_CUSTOM_DIM* input_dim;  // size = sizeof(NN_CUSTOM_DIM)*input_num
	NN_CUSTOM_DIM* output_dim; // size = sizeof(NN_CUSTOM_DIM)*output_num
	...
	custom parameters
	*/
} NN_CUSTOM_PARM;
#else
typedef struct _NN_CUSTOM_PARM {
	NN_DATA input;
	NN_DATA output;
	NN_DATA model;
	UINT32 parm_size;
	/*
	    ...
	    custom parameters
	*/
} NN_CUSTOM_PARM;
#endif
//@}

#endif  /* _NN_PARM_H_ */