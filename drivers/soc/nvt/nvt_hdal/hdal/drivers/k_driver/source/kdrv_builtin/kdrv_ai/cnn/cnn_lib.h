/**
Public header file for CNN module.

@file       cnn_lib.h
@ingroup    mIIPPCNN

@brief

How to start:\n
1. cnn_open\n
2. cnn_setmode\n
3. cnn_start\n


How to stop:\n
1. cnn_wait_frameend
2. cnn_pause\n
3. cnn_close\n

Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _CNN_LIB_H_
#define _CNN_LIB_H_

#include "kdrv_ai.h"
#include "kwrap/type.h"

/**
@addtogroup mIIPPCNN
*/
//@{
#define CNN_CYCLE_TEST      ENABLE
#define CNN_CYCLE_LAYER     DISABLE
/**
Define CNN interrput enable.
*/
//@{
#define CNN_INTE_FRM_DONE					0x00000001
#define CNN_INTE_SRC_ILLEGALSIZE			0x00000004
#define CNN_INTE_CONVOUT_ILLEGALSIZE		0x00000008
#define CNN_INTE_LLEND						0x00000100
#define CNN_INTE_LLERROR					0x00000200
#define CNN_INTE_NUE_OCCUPY_APP_TO_APP_ERR  0x00001000
#define CNN_INTE_NUE_OCCUPY_APP_TO_LL_ERR	0x00002000
#define CNN_INTE_NUE_OCCUPY_LL_TO_APP_ERR	0x00004000
#define CNN_INTE_NUE_OCCUPY_LL_TO_LL_ERR	0x00008000
#define CNN_INTE_FCD_DECODE_DONE			0x00010000
#define CNN_INTE_FCD_VLC_DEC_ERR			0x00020000
#define CNN_INTE_FCD_BS_SIZE_ERR			0x00040000
#define CNN_INTE_FCD_SPARSE_DATA_ERR		0x00080000
#define CNN_INTE_FCD_SPARSE_INDEX_ERR		0x00100000

#define CNN_INTE_CHECKSUM_MISMATCH0			0x01000000
#define CNN_INTE_CHECKSUM_MISMATCH1			0x02000000
#define CNN_INTE_CHECKSUM_MISMATCH2			0x04000000

#define CNN_INTE_ALL                    0x071ff30d
#define CNN_INTE_APP_ALL                0x001ef20d
#define CNN_INTE_LL_ALL                 0x001ef30c
//@}

/**
Define CNN interrput status.
*/
//@{
#define CNN_INT_FRM_DONE					0x00000001
#define CNN_INT_SRC_ILLEGALSIZE				0x00000004
#define CNN_INT_CONVOUT_ILLEGALSIZE			0x00000008
#define CNN_INT_LLEND						0x00000100
#define CNN_INT_LLERROR						0x00000200
#define CNN_INT_NUE_OCCUPY_APP_TO_APP_ERR	0x00001000
#define CNN_INT_NUE_OCCUPY_APP_TO_LL_ERR	0x00002000
#define CNN_INT_NUE_OCCUPY_LL_TO_APP_ERR	0x00004000
#define CNN_INT_NUE_OCCUPY_LL_TO_LL_ERR		0x00008000
#define CNN_INT_FCD_DECODE_DONE				0x00010000
#define CNN_INT_FCD_VLC_DEC_ERR				0x00020000
#define CNN_INT_FCD_BS_SIZE_ERR				0x00040000
#define CNN_INT_FCD_SPARSE_DATA_ERR			0x00080000
#define CNN_INT_FCD_SPARSE_INDEX_ERR		0x00100000
#define CNN_INT_CHECKSUM_MISMATCH0			0x01000000
#define CNN_INT_CHECKSUM_MISMATCH1			0x02000000
#define CNN_INT_CHECKSUM_MISMATCH2			0x04000000
#define CNN_INT_ALL							0x071ff30d
//@}

/**
@name Define CNN function enable.
*/
//@{
#define CNN_BNSCALE_KERL_EN         0x00000100  ///< batchnorm+scale function enable
#define CNN_ACT_KERL_EN				0x00000200  ///< act function enable
#define CNN_POOLING_KERL_EN         0x00000400  ///< pooling function enable
#define CNN_ELTWISE_KERL_EN         0x00000800  ///< eltwise function enable
#define CNN_CONV_KERL_EN			0x00001000  ///< conv function enable
#define CNN_PREACT_KERL_EN			0x00002000  ///< preact function enable

#define CNN_FCD_VLC_EN              0x00010000  ///< compression variable length encoding enable
#define CNN_FCD_QUANTI_EN           0x00020000  ///< compression quantization enable
#define CNN_FCD_SPARSE_EN           0x00040000  ///< compression sparse encoding enable
#define CNN_FCD_QUANTI_KMEANS_EN    0x00080000  ///< compression quantization kmeans enable

#define CNN_ALL_KERL_EN             0x000f3f00  ///< all function enable

//@}

/**
@name Define CNN Input/Output enable.
*/
//@{
#define CNN_IN_ISIMAGE_EN           0x00000001  ///< input image enable
#define CNN_IN_ISHSTRIPE_EN			0x00000002  ///< input h-stripe enable
#define CNN_IN_ISVSTRIPE_EN			0x00000004  ///< input v-stripe enable
#define CNN_IN_ISCHANNELSTRIPE_EN   0x00000008  ///< input channel-stripe enable

#define CNN_OUT0_EN                 0x00000100  ///< output 0 enable
#define CNN_OUT1_EN                 0x00002000  ///< output 1 enable
#define CNN_IN_REMAINSRC_EN         0x00100000  ///< remain src enable
#define CNN_IN_FLIP_EN				0x00200000  ///< flip enable
#define CNN_IN_INTERLACE_EN         0x00400000  ///< interlace enable
#define CNN_ALL_IO_EN               0x0070210f  ///< all IO enable
//@}

/**
CNN mode selection

Select CNN mode
*/
//@{
typedef enum {
	CNN_OPMODE_CONV         = 0,        ///< convolution + bnscale + relu + pool
	CNN_OPMODE_DECONV       = 1,        ///< deconvolution
	CNN_OPMODE_SACLEUP      = 2,        ///< scaleup
	CNN_OPMODE_USERDEFINE,              ///< all manual mode
	ENUM_DUMMY4WORD(CNN_OPMODE)
} CNN_OPMODE;
//@}

/**
CNN In/Out Type
*/
//@{
typedef enum {
	CNN_INT8        = 0,
	CNN_UINT8       = 1,
	CNN_INT16       = 2,
	CNN_UINT16      = 3,
	ENUM_DUMMY4WORD(CNN_IO_TYPE)
} CNN_IO_TYPE;
//@}

/*
CNN RELU Type<=???
*/
//@{
typedef enum {
	CNN_RELU_LEAKY      = 0,
	CNN_RELU_ABS        = 1,
	CNN_RELU_ZERO,
	ENUM_DUMMY4WORD(CNN_RELU_TYPE)
} CNN_RELU_TYPE;
//@}

/**
CNN Input Buffer ID
*/
//@{
typedef enum {
	CNN_IN0_BUF     = 0,
	CNN_IN4_BUF,
	CNN_IN5_BUF,
	CNN_IN6_BUF,
	CNN_IN7_BUF,
	CNN_IN8_BUF,
	ENUM_DUMMY4WORD(CNN_IN_BUFID)
} CNN_IN_BUFID;
//@}

/**
CNN Mode type
*/
//@{
typedef enum {
	CNN_CONV            = 0,
	CNN_DECONV          = 1,
	CNN_SCALEUP         = 2,
	ENUM_DUMMY4WORD(CNN_MODE_TYPE)
} CNN_MODE_TYPE;
//@}

/**
CNN Eltwise Mode type
*/
//@{
typedef enum {
	CNN_ELT_ADD         = 0,
	CNN_ELT_MAX         = 2,
	ENUM_DUMMY4WORD(CNN_ELT_MODE_TYPE)
} CNN_ELT_MODE_TYPE;
//@}

/**
CNN Act Mode type
*/
//@{
typedef enum {
	CNN_ACT_RELU        = 0,
	CNN_ACT_TANH        = 1,
	CNN_ACT_LUT         = 2,
	ENUM_DUMMY4WORD(CNN_ACT_MODE_TYPE)
} CNN_ACT_MODE_TYPE;
//@}


/**
CNN Pooling type
*/
//@{
typedef enum {
	CNN_POOLING_LOCAL   = 0,
	CNN_POOLING_GLOBAL  = 1,
	ENUM_DUMMY4WORD(CNN_POOLING_TYPE)
} CNN_POOLING_TYPE;
//@}

/**
CNN LUT type
*/
//@{
typedef enum {
	CNN_LUT_TANH     = 0,
	CNN_LUT_SIGMOID  = 1,
	ENUM_DUMMY4WORD(CNN_LUT_MODE_TYPE)
} CNN_LUT_MODE_TYPE;
//@}

/**
CNN Conv Kernel size
*/
//@{
typedef enum {
	CNN_CONV_KERSZ_11_11    = 0,
	CNN_CONV_KERSZ_7_7		= 1,
	CNN_CONV_KERSZ_5_5		= 2,
	CNN_CONV_KERSZ_3_3		= 3,
	CNN_CONV_KERSZ_1_1		= 4,
	CNN_CONV_KERSZ_7_1		= 5,
	CNN_CONV_KERSZ_1_7		= 6,
	CNN_CONV_KERSZ_5_1		= 7,
	CNN_CONV_KERSZ_1_5		= 8,
	CNN_CONV_KERSZ_3_1		= 9,
	CNN_CONV_KERSZ_1_3		= 10,
	CNN_CONV_KERSZ_9_9		= 11,
	ENUM_DUMMY4WORD(CNN_CONV_KER_SIZE)
} CNN_CONV_KER_SIZE;
//@}

/**
CNN Conv Kernel Stride
*/
//@{
typedef enum {
	CNN_CONV_KER_STRIDE_1   = 0,
	CNN_CONV_KER_STRIDE_2   = 1,
	CNN_CONV_KER_STRIDE_4   = 2,
	ENUM_DUMMY4WORD(CNN_CONV_KER_STRIDE)
} CNN_CONV_KER_STRIDE;
//@}

/**
CNN Output 0 mode
*/
//@{
typedef enum {
	CNN_OUT0_RELU0        = 0,
	CNN_OUT0_INTERMEDIATE = 2,
	ENUM_DUMMY4WORD(CNN_OUT0_TYPE)
} CNN_OUT0_TYPE;
//@}

/**
CNN Pooling Kernel Size
*/
//@{
typedef enum {
	CNN_POOL_KERSZ_2    = 0,
	CNN_POOL_KERSZ_3    = 1,
	CNN_POOL_KERSZ_4    = 2,
	CNN_POOL_KERSZ_5    = 3,
	ENUM_DUMMY4WORD(CNN_POOL_KER_SIZE)
} CNN_POOL_KER_SIZE;
//@}

/**
CNN Pooling Kernel Stride
*/
//@{
typedef enum {
	CNN_POOL_KER_STRIDE_1   = 0,
	CNN_POOL_KER_STRIDE_2   = 1,
	ENUM_DUMMY4WORD(CNN_POOL_KER_STRIDE)
} CNN_POOL_KER_STRIDE;
//@}

/**
CNN Pooling kernel type.
*/
//@{
typedef enum {
	CNN_POOL_KER_MAX         = 0,
	CNN_POOL_KER_AVG         = 1,
	ENUM_DUMMY4WORD(CNN_POOL_KER)
} CNN_POOL_KER;
//@}

/**
CNN Pooling output cal type.
*/
//@{
typedef enum {
	CNN_POOL_OUT_CAL_CEIL         = 0,
	CNN_POOL_OUT_CAL_FLOOR        = 1,
	ENUM_DUMMY4WORD(CNN_POOL_OUT_CAL_TYPE)
} CNN_POOL_OUT_CAL_TYPE;
//@}

/**
CNN Pooling ave div type.
*/
//@{
typedef enum {
	CNN_POOL_NO_BOUNDARY     = 0,
	CNN_POOL_BOUNDARY        = 1,
	ENUM_DUMMY4WORD(CNN_POOL_AVE_DIV_TYPE)
} CNN_POOL_AVE_DIV_TYPE;
//@}

/**
CNN Deconvolution Kernel Stride
*/
//@{
typedef enum {
	CNN_DECONV_KER_STRIDE_1   = 0,
	CNN_DECONV_KER_STRIDE_2   = 1,
	CNN_DECONV_KER_STRIDE_4   = 2,
	CNN_DECONV_KER_STRIDE_8   = 3,
	ENUM_DUMMY4WORD(CNN_DECONV_KER_STRIDE)
} CNN_DECONV_KER_STRIDE;
//@}

/**
CNN ScaleUp Rate
*/
//@{
typedef enum {
	CNN_SCALEUP_1   = 0,
	CNN_SCALEUP_2   = 1,
	CNN_SCALEUP_4   = 2,
	CNN_SCALEUP_8   = 3,
	ENUM_DUMMY4WORD(CNN_SCALEUP_RATE)
} CNN_SCALEUP_RATE;
//@}

/**
CNN Open Object.

Open Object is used for opening CNN module.
\n Used for CNN_open()

*/
//@{
typedef struct _CNN_OPENOBJ {
	UINT32 clk_sel;                             ///< support 220/192/160/120/80/60/48 Mhz
	VOID (*fp_isr_cb)(UINT32 int_status);       ///< isr callback function
} CNN_OPENOBJ;
//@}

/**
CNN Input Size.
*/
//@{
typedef struct _CNN_IN_SIZE {
	UINT32 width;
	UINT32 height;
	UINT32 channel;
	UINT32 batch;
} CNN_IN_SIZE;
//@}

/**
CNN Output0 Size.
*/
//@{
typedef struct _CNN_OUT_SIZE {
	UINT32 out0_ofs;
} CNN_OUT_SIZE;
//@}


/**
CNN Conv Kernel Parameters.

Structure of information in CNN kernel parameters.
*/
//@{
typedef struct _CNN_CONVKERL_PARM {
	CNN_CONV_KER_SIZE conv_kersize;    ///< convolution kernel size 0:11x11, 1:7x7, 2:5x5, 3:3x3, 4:1x1, 5:7x1, 6:1x7, 7:5x1, 8:1x5, 9:3x1, 10:1x3, 11:9x9
	CNN_CONV_KER_STRIDE conv_stride;   ///< convolution kernel stride 0:1, 1:2, 2:4
	UINT32 conv_setnum;                     ///< convolution set number [1, 2048]
	UINT32 conv_shf_acc;                    ///< shift of accumulative results of conovlution [0, 4]
	BOOL is_top_pad;                        ///< input with top padding
	BOOL is_bot_pad;                        ///< input with bottom padding
	BOOL is_left_pad;                       ///< input with left padding
	BOOL is_right_pad;                      ///< input with right padding
	UINT32 conv_shf_bias;                   ///< convolution bias shift [0, 20]
} CNN_CONVKERL_PARM;
//@}

/**
CNN DRAM input and output addresses.

Structure of information in DRAM addresses.
*/
//@{
typedef struct _CNN_DMAIO_ADDR {
	UINT32 inaddr0;                 ///< input starting address of input data 0
	UINT32 inaddr4;                 ///< input starting address of conv weight
	UINT32 inaddr5;                 ///< input starting address of kmeans quantizes params
	UINT32 inaddr6;                 ///< input starting address of conv bias and bn params
	UINT32 inaddr7;                 ///< input starting address of intermediate input/eltwise input

	UINT32 outaddr0;                ///< output 0 starting address
	UINT32 outaddr1;                ///< output 1 starting address

	BOOL drv_dma_not_sync;          
} CNN_DMAIO_ADDR;
//@}


/**
CNN LL struct

Structure of LL information
*/
//@{
typedef struct _CNN_LL_PRM {
	UINT32 addrin_ll;               ///< input starting address of linked list
	UINT32 addrin_ll_base;
} CNN_LL_PRM;
//@}

/**
CNN DRAM input and output lineoffset.

Structure of information in DRAM lineoffset.
*/
//@{
typedef struct _CNN_DMAIO_LOFS {
	UINT32 in0_lofs;                 ///< input lineoffset/batchoffset of input data 0
	UINT32 in1_lofs;                 ///< input channel offset of input data 0
	UINT32 in2_lofs;                 ///< input lineoffset of eltwise input data
	UINT32 in3_lofs;                 ///< input channel lineoffset of eltwise input data
	UINT32 out0_lofs;               ///< output lineoffset/batchoffset of output data 0
	UINT32 out1_lofs;               ///< output channel offset of output data 0
	UINT32 out2_lofs;               ///< output lineoffset/batchoffset of output data 1
	UINT32 out3_lofs;               ///< output channel offset of output data 1
} CNN_DMAIO_LOFS;
//@}

/**
CNN FCD Parameters

Structure of information in CNN FCD parameters.
*/
//@{
typedef struct _CNN_FCD_PARM {
	BOOL fcd_vlc_en;
	BOOL fcd_quanti_en;
	BOOL fcd_sparse_en;
	BOOL fcd_quanti_kmean_en;
	BOOL fcd_kq_tbl_update_disable;
	UINT32 fcd_enc_bit_length;
	UINT32 fcd_vlc_code[23];
	UINT32 fcd_vlc_valid[23];
	UINT32 fcd_vlc_ofs[23];
} CNN_FCD_PARM;
//@}

/**
CNN Conv Parameters.

Structure of information in CNN Conv parameters.
*/
//@{
typedef struct _CNN_CONV_PARM {
	CNN_CONVKERL_PARM convkerl_parms;       ///< input CNN conv kernel parameters
	CNN_FCD_PARM fcd_parm;                  ///< input CNN conv fcd parameters
} CNN_CONV_PARM;
//@}

/**
CNN BnScale Parameters.

Structure of information in CNN BnScale parameters.
*/
//@{
typedef struct _CNN_BNSCALE_PARM {
	UINT32 bn_shf_mean;             ///< left shift of batchnorm mean
	UINT32 scale_shf_bias;          ///< left shift of batchnorm bias
	UINT32 scale_shf_alpha;         ///< right shift after alpha multiply
} CNN_BNSCALE_PARM;
//@}

/**
CNN Eltwise Parameters.

Structure of information in CNN Eltwise parameters.
*/
//@{
typedef struct _CNN_ELTWISE_PARM {
	BOOL elt_in_shift_dir; 
	UINT32 elt_in_shift;
	UINT32 elt_in_scale;
	UINT32 elt_shf0;             
	UINT32 elt_shf1;          
	UINT32 elt_outshf;              
	INT32 elt_coef0;               
	INT32 elt_coef1;         
} CNN_ELTWISE_PARM;
//@}

/**
CNN ReLU Parameters.

Structure of information in CNN ReLU parameters.
*/
//@{
typedef struct _CNN_RELU_IN {            
	CNN_RELU_TYPE relu_type;             ///< input RELU type (not work in user define mode)
	INT32  leaky_val;
	UINT32 leaky_shf;
	INT32 negation;
} CNN_RELU_IN;
//@}

/**
CNN ReLU Parameters.

Structure of information in CNN ReLU parameters.
*/
//@{
typedef struct _CNN_RELU_PARM {
	CNN_RELU_IN pre_relu;
	CNN_RELU_IN relu0;
	CNN_RELU_IN relu1;
} CNN_RELU_PARM;
//@}

/**
CNN Global Pooling Parameters

Structure of information in CNN global pooling parameters.
*/
//@{
typedef struct _CNN_GLOBAL_POOL_PARM {
	CNN_POOL_KER ker_type;	
	CNN_POOL_AVE_DIV_TYPE ave_div_type;
	UINT32 avg_mul;
	UINT32 avg_shf;
	BOOL pool_shf_dir;
	UINT32 pool_shf;
	BOOL pool_signedness;
} CNN_GLOBAL_POOL_PARM;
//@}

/**
CNN Local Pooling Parameters

Structure of information in CNN local pooling parameters.
*/
//@{
typedef struct _CNN_LOCAL_POOL_PARM {
	CNN_POOL_KER ker_type;
	CNN_POOL_OUT_CAL_TYPE out_cal_type;
	CNN_POOL_KER_STRIDE stride;
	CNN_POOL_KER_SIZE ker_size;	
	CNN_POOL_AVE_DIV_TYPE ave_div_type;
	BOOL is_top_pad;
	BOOL is_bot_pad;
	BOOL is_left_pad;
	BOOL is_right_pad;
	BOOL pool_shf_dir;
	UINT32 pool_shf;
	BOOL pool_signedness;
} CNN_LOCAL_POOL_PARM;
//@}

/**
CNN Deconv Parameters.

Structure of information in CNN Deconv parameters.
*/
//@{
typedef struct _CNN_DECONV_PARM {
	UINT32 is_top_pad;                            ///< input with top padding [0, 3]
	UINT32 is_bot_pad;                            ///< input with bottom padding [0, 3]
	UINT32 is_left_pad;                           ///< input with left padding [0, 3]
	UINT32 is_right_pad;                          ///< input with right padding [0, 3]
	INT32 deconv_padval;                       ///< pad value of input edge [-2^15, 2^15-1]/[0, 2^16-1]
	CNN_DECONV_KER_STRIDE deconv_stride;   ///< deconvolution stride 0:1, 1:2, 2:4, 3:8
} CNN_DECONV_PARM;
//@}

/**
CNN ScaleUp Parameters.

Structure of information in CNN ScaleUp parameters.
*/
//@{
typedef struct _CNN_SCALEUP_PARM {
	UINT32 is_top_pad;                            ///< input with top padding [0, 3]
	UINT32 is_bot_pad;                            ///< input with bottom padding [0, 3]
	UINT32 is_left_pad;                           ///< input with left padding [0, 3]
	UINT32 is_right_pad;                          ///< input with right padding [0, 3]
	INT32 scaleup_padval;                         ///< pad value of input edge [-2^15, 2^15-1]/[0, 2^16-1]
	CNN_SCALEUP_RATE scaleup_rate;   ///< scaleup_rate 0:1, 1:2, 2:4, 3:8
} CNN_SCALEUP_PARM;
//@}


/**
CNN output scale shift Parameters.

Structure of information in CNN output scale shift parameters.
*/
//@{
typedef struct _CNN_OUT_SCALE_PARM {
	BOOL conv_shf_dir;
	UINT32 conv_shf;
	UINT32 conv_scale;
	BOOL elt_shf_dir;
	UINT32 elt_shf;
	UINT32 elt_scale;
	BOOL pool_shf_dir;
	UINT32 pool_shf;
	UINT32 pool_scale;
	BOOL out0_shf_dir;
	UINT32 out0_shf;
	UINT32 out0_scale;
	BOOL out1_shf_dir;
	UINT32 out1_shf;
	UINT32 out1_scale;

} CNN_OUT_SCALE_PARM;
//@}

/**
CNN chksum Parameters.

Structure of information in CNN chksum parameters.
*/
//@{
typedef struct _CNN_CHKSUM_PARM {
	UINT32 act0chksum;
	UINT32 act1chksum;
	UINT32 pool_chksum;
} CNN_CHKSUM_PARM;
//@}
/**
User Definition
*/
//@{
typedef struct _CNN_USER_DEF {
	CNN_MODE_TYPE eng_mode;
	CNN_ELT_MODE_TYPE elt_mode;
	CNN_ACT_MODE_TYPE act_mode;
	CNN_POOLING_TYPE pool_mode;
	CNN_LUT_MODE_TYPE lut_mode;
	UINT32 func_en;
	UINT32 io_enable;
	CNN_OUT0_TYPE out0_mode;
} CNN_USER_DEF;
//@}

/**
CNN Entire Configuration

Structure of CNN module parameters when one wants to configure this module.
*/
//@{
typedef struct _CNN_PARAM {
	CNN_IO_TYPE intype;
	CNN_IO_TYPE elttype;
	CNN_IO_TYPE out0type;
	CNN_IO_TYPE out1type;
	CNN_IN_SIZE insize;
	CNN_OUT_SIZE out_ofs;
	CNN_DMAIO_ADDR dmaio_addr;
	CNN_DMAIO_LOFS dmaio_lofs;
	CNN_CONV_PARM conv_parm;
	CNN_BNSCALE_PARM bnscale_parm;
	CNN_ELTWISE_PARM elt_parm;
	CNN_RELU_PARM relu_parm;
	CNN_GLOBAL_POOL_PARM global_pool;
	CNN_LOCAL_POOL_PARM local_pool;
	CNN_DECONV_PARM deconv_parm;
	CNN_SCALEUP_PARM scaleup_parm;
	CNN_OUT_SCALE_PARM out_scale_parm;
	CNN_CHKSUM_PARM chk_sum_parm;
	CNN_USER_DEF userdef;
} CNN_PARM;
//@}

//ISR
extern VOID cnn_isr(BOOL cnn_id);

//interrupt
extern UINT32   cnn_get_int_enable(BOOL cnn_id);
extern VOID     cnn_clr_intr_status(BOOL cnn_id, UINT32 status);
extern UINT32   cnn_get_intr_status(BOOL cnn_id);
extern ER       cnn_change_interrupt(BOOL cnn_id, UINT32 int_en);
//basic function
extern ER cnn_set_clock_rate(BOOL cnn_id, UINT32 clock);
extern ER       cnn_open(BOOL cnn_id, CNN_OPENOBJ *p_obj_cb);
extern BOOL     cnn_is_opened(BOOL cnn_id);
extern ER       cnn_close(BOOL cnn_id);

extern VOID     cnn_wait_frameend(BOOL cnn_id, BOOL is_clr_flag);
extern VOID		cnn_clr_frameend(BOOL cnn_id);
extern BOOL     cnn_isenable(BOOL cnn_id);
extern VOID     cnn_set_dmain_lladdr(BOOL cnn_id, UINT32 addr0);
extern UINT32   cnn_get_dmain_lladdr(BOOL cnn_id);
extern VOID     cnn_reset(BOOL cnn_id);
extern ER       cnn_ll_start(BOOL cnn_id);
extern ER       cnn_ll_pause(BOOL cnn_id);
extern VOID     cnn_wait_ll_frameend(BOOL cnn_id, BOOL is_clr_flag);
extern ER       cnn_ll_setmode(BOOL cnn_id, CNN_LL_PRM *ll_prm);
extern ER       cnn_ll_isr_start(BOOL cnn_id);

#if defined(__FREERTOS)
VOID cnn_create_resource(BOOL cnn_id, UINT32 clk_freq);
#else
VOID cnn_create_resource(BOOL cnn_id, VOID *parm, UINT32 clk_freq);
#endif

VOID cnn_release_resource(BOOL cnn_id);
VOID cnn_set_base_addr(BOOL cnn_id, UINT32 addr);
UINT32 cnn_get_base_addr(BOOL cnn_id);

//Engine
//get mode
extern CNN_MODE_TYPE cnn_get_engmode(BOOL cnn_id);
extern CNN_IO_TYPE cnn_get_intype(BOOL cnn_id);
extern CNN_IO_TYPE cnn_get_out0type(BOOL cnn_id);
extern CNN_IO_TYPE cnn_get_out1type(BOOL cnn_id);
extern UINT32 cnn_get_kerl_en(BOOL cnn_id);
//set info
//get info
extern CNN_IN_SIZE cnn_get_insize(BOOL cnn_id);
//ReLU
extern ER cnn_get_relu_leaky(BOOL cnn_id, INT32 *p_value, UINT32 *p_shf);
//Pooling
extern CNN_POOL_KER cnn_get_poolkertype(BOOL cnn_id);
extern CNN_LOCAL_POOL_PARM cnn_get_localpool_parm(BOOL cnn_id);
extern CNN_GLOBAL_POOL_PARM cnn_get_globalpool_parm(BOOL cnn_id);
//input/output
extern CNN_DMAIO_ADDR cnn_get_dmaio_addr(BOOL cnn_id);
extern VOID cnn_enable_int(BOOL cnn_id, BOOL enable, UINT32 intr);
extern UINT32 cnn_init(BOOL cnn_id);
extern UINT32 cnn_uninit(BOOL cnn_id);
#if CNN_CYCLE_TEST
extern UINT64 cnn_dump_total_cycle(UINT32 debug_layer);
#endif
extern BOOL cnn_eng_valid(UINT32 cnn_id);
extern VOID cnn_reset_status(VOID);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
extern ER   cnn_pause(BOOL cnn_id);
extern ER   cnn_isr_start(BOOL cnn_id);
extern ER   cnn_start(BOOL cnn_id);
extern ER	cnn_setmode(BOOL cnn_id, CNN_OPMODE mode, CNN_PARM *p_parms);
#endif
extern VOID cnn_dma_abort(UINT32 cnn_id);
//@}

#endif
