/**
    @brief Header file of NN engine definition of vendor net flow sample.

    @file nn_net.h

    @ingroup net_flow_sample

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NN_NET_H_
#define _NN_NET_H_

/********************************************************************
    INCLUDE FILES
********************************************************************/
#ifdef __KERNEL__
#include <linux/kernel.h>
#endif
//#include "kdrv_ai.h"
#include "nn_verinfo.h"

/********************************************************************
    MACRO CONSTANT DEFINITIONS
********************************************************************/
#define USE_NEON                        1
#define NN_USE_DSP              		FALSE

#if AI_V4
#define NN_NET_MODULE_VERSION           "1.02.001"
#elif USE_NEON
#define NN_NET_MODULE_VERSION           "1.01.000"
#else
#define NN_NET_MODULE_VERSION           "1.00.002"
#endif
#define NN_GEN_ADDR_TYPE_MASK           0xf0000000
#define NN_GEN_ADDR_MASK                0x0fffffff
#define NN_GEN_NULL_ADDR_TYPE           0x00000000
#define NN_GEN_NO_ADDR_UPD_TYPE         0x10000000
#define NN_GEN_MODEL_ADDR_TYPE          0x20000000
#define NN_GEN_BUF_ADDR_TYPE            0x40000000

#if CNN_25_MATLAB
#define NN_IMEM_NUM                     8
#define NN_OMEM_NUM                     3
#endif

#define NN_LL_ADDR_MASK                 0x00000000ffffffff
#define NN_LL_CMD_MASK                  0xffffffff00000000

#define NN_MAX_DIM                      5


#define NN_LAYER_LIB_CUSTNN     		0x00000000
#define NN_LAYER_LIB_NVTNN      		0x10000000
#define NN_LAYER_TYPE_ID_MASK   		0x0fffffff


/********************************************************************
    TYPE DEFINITION
********************************************************************/
/*
    "Hardware" CNN struct : CNN_LIB_PARA, NN_SVM_CLAS_PARA
*/
typedef enum {
	NN_CONV 				= 0,
	NN_DECONV,
	NN_SVM,
	NN_ROIPOOLING,
	NN_ELTWISE,
	NN_REORGANIZATION,
	NN_RESHAPE,
	NN_PROPOSAL,
	NN_POSTPROC,
	NN_SOFTMAX,
	NN_FC 					= 10,
	NN_PREPROC,
	NN_FC_POST,
	NN_POOL,
	NN_BNSCALE,
	NN_CUSTOMER,
	NN_ANCHOR,
	NN_UPSAMPLE,
	//NN_CUSTOMER,
	NN_SCALEUP,
#if (USE_NEON || (!CNN_25_MATLAB))
	NN_PRELU,
	NN_SIGMOID,
	NN_PRIORBOX,
	NN_DETOUT,
#endif
#if AI_V4
	NN_DEPTHWISE,
#endif
    NN_FP2FIX,
	NN_LSTM,
	NN_REVERSE,
	ENUM_DUMMY4WORD(NN_MODE)
} NN_MODE;


/*
    Data format
*/
typedef struct _NN_FMT {
	INT8 frac_bits;
	INT8 int_bits;
	INT8 sign_bits;
	INT8 reserved;
} NN_FMT;

/*
    "Memory related" struct : NN_DATA, NN_BUFFER, NN_IOMEM
*/
typedef struct _NN_DATA {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	NN_FMT fmt;         // sign_bit[16]; int_bit[15..8]; frac_bit[7..0];
} NN_DATA;

typedef struct _NN_CUSTOM_DIM {
	UINT32 dim[5];
	UINT32 ofs[4];
} NN_CUSTOM_DIM;
typedef struct _NN_IOMEM {
#if CNN_25_MATLAB
	/*
	Define SAI[8]:
	[0] --> CNN : SAI0_ADDR(channel 0),         NUE : SAI0_ADDR,        VPE : input             NUE2 : SAI0_ADDR 
	[1] --> CNN : SAI1_ADDR(channel 1),         NUE : SAI1_ADDR,        VPE : mean image		NUE2 : SAI1_ADDR
	[2] --> CNN : SAI2_ADDR(channel 2),         NUE : SAISV_ADDR,       VPE : temporary buffer  NUE2 : SAI2_ADDR
	[3] --> CNN : SAI3_ADDR(weighting),         NUE : SAIALPHA_ADDR,    VPE : reserved			
	[4] --> CNN : SAI4_ADDR(k means),           NUE : SAIRHO_ADDR,      VPE : reserved
	[5] --> CNN : SAI5_ADDR(bias & bn scale),   NUE : SAIROI_ADDR,      VPE : reserved
	[6] --> CNN : SAI6_ADDR(intermedoate),      NUE : SAILL_ADDR,       VPE : reserved
	[7] --> CNN : SAI7_ADDR(link list),         NUE : SAIKQ_ADDR,       VPE : reserved
	*/
	NN_DATA SAI[NN_IMEM_NUM];
	/*
	Define SAO[2]:
	[0] --> CNN : SAO0_ADDR,                    NUE : SAOR_ADDR,        VPE : output			NUE2 : SAO0
	[1] --> CNN : SAO1_ADDR,                    NUE : reserved,         VPE : reserved			NUE2 : SAO1
	[2] --> CNN : researved,					NUE : reserved,									NUE2 : SAO2	
	*/
	NN_DATA SAO[NN_OMEM_NUM];
#else
	UINT32 imem_addr;
	UINT32 imem_cnt;                    ///< size = imem_cnt * sizeof(NN_DATA)
	UINT32 omem_addr;
	UINT32 omem_cnt;                    ///< size = omem_cnt * sizeof(NN_DATA)
#endif	
} NN_IOMEM;

typedef enum {
#if CNN_25_MATLAB
	NN_GEN_ENG_VPE = 0,
	NN_GEN_ENG_CNN,
	NN_GEN_ENG_CNN2,
	NN_GEN_ENG_NUE,
	NN_GEN_ENG_NUE2,
	NN_GEN_ENG_CPU,
	NN_GEN_ENG_DSP,
	NN_GEN_ENG_UNKNOWN,
	ENUM_DUMMY4WORD(NN_GEN_ENG_TYPE)
#else
	NN_GEN_ENG_UNKNOWN = -1,
	NN_GEN_ENG_VPE = 0,
	NN_GEN_ENG_CNN,
	NN_GEN_ENG_NUE,
	NN_GEN_ENG_CPU,
	NN_GEN_ENG_DSP,
	NN_GEN_ENG_CNN2,
	NN_GEN_ENG_NUE2,
	ENUM_DUMMY4WORD(NN_GEN_ENG_TYPE)
#endif
} NN_GEN_ENG_TYPE;

typedef enum {
	NN_GEN_TRIG_APP_AI_DRV  = 0,        ///< use KDRV_AI_MODE      (for DLA layer)
	NN_GEN_TRIG_LL_AI_DRV   = 1,        ///< use KDRV_AI_LL_MODE   (for DLA layer)
	NN_GEN_TRIG_COMMON      = 2,        ///<                       (for non-DLA layer)
	NN_GEN_TRIG_USER_AI_DRV = 3,        ///< use KDRV_AI_USER_MODE (for DLA single OP)
	ENUM_DUMMY4WORD(NN_GEN_TRIG_SRC)
} NN_GEN_TRIG_SRC;

#if CNN_FMT_V4
typedef struct _NN_BUF_INFO_IN {
	INT32  buf_index;
	UINT32 buf_offset;
} NN_BUF_INFO_IN;

typedef struct _NN_BUF_INFO_OUT {
	INT32  buf_index;
	UINT32 buf_offset;                 ///< bit 0x8000000 indicate if this is TMP buffer
	INT32  buf_size;
} NN_BUF_INFO_OUT;
#endif//CNN_FMT_V4

typedef enum _NN_IN_BUF_ATTR {
	NN_IN_BUF_ATTR_ELTWISE_IN_SRC = 0x00000020,
	NN_IN_BUF_ATTR_CONV_IN_ISIMG  = 0x00000010,
	NN_IN_BUF_ATTR_PREPROC_IN_FMT = 0X0000000f,
	ENUM_DUMMY4WORD(NN_IN_BUF_ATTR)
} NN_IN_BUF_ATTR;

typedef enum _NN_OUT_BUF_ATTR {
	NN_OUT_BUF_ATTR_PRESERVE         = 0x80000000,
	NN_OUT_BUF_ATTR_PREPROC_OUT_FMT  = 0x0000000f,
	ENUM_DUMMY4WORD(NN_OUT_BUF_ATTR)
} NN_OUT_BUF_ATTR;

#if CNN_FMT_V4
#define IN_BUF_INDEX(p_mctrl, buf_idx)    (((NN_BUF_INFO_IN*)((p_mctrl)->in_bufinfo_addr))[buf_idx].buf_index)
#define IN_BUF_OFFSET(p_mctrl, buf_idx)   (((NN_BUF_INFO_IN*)((p_mctrl)->in_bufinfo_addr))[buf_idx].buf_offset)
#define IN_BUF_NUM(p_mctrl)               (((p_mctrl)->mode==NN_DEPTHWISE)? (((p_mctrl)->in_bufinfo_cnt&0xFFFF0000)>>16):((p_mctrl)->in_bufinfo_cnt))
#define IN_BUF_NUM_REAL(p_mctrl)          (((p_mctrl)->mode==NN_DEPTHWISE)? (((p_mctrl)->in_bufinfo_cnt&0x0000FFFF)):((p_mctrl)->in_bufinfo_cnt))
#define OUT_BUF_INDEX(p_mctrl, buf_idx)   (((NN_BUF_INFO_OUT*)((p_mctrl)->out_bufinfo_addr))[buf_idx].buf_index)
#define OUT_BUF_OFFSET(p_mctrl, buf_idx)  (((NN_BUF_INFO_OUT*)((p_mctrl)->out_bufinfo_addr))[buf_idx].buf_offset & 0x7fffffff)
#define OUT_BUF_NUM(p_mctrl)              (((p_mctrl)->mode==NN_DEPTHWISE)? (((p_mctrl)->out_bufinfo_cnt&0xFFFF0000)>>16):((p_mctrl)->out_bufinfo_cnt))
#define OUT_BUF_NUM_REAL(p_mctrl)         (((p_mctrl)->mode==NN_DEPTHWISE)? (((p_mctrl)->out_bufinfo_cnt&0x0000FFFF)):((p_mctrl)->out_bufinfo_cnt))
#define OUT_BUF_SIZE(p_mctrl, buf_idx)    (((NN_BUF_INFO_OUT*)((p_mctrl)->out_bufinfo_addr))[buf_idx].buf_size)
#define OUT_BUF_IS_TMP(p_mctrl, buf_idx) ((((NN_BUF_INFO_OUT*)((p_mctrl)->out_bufinfo_addr))[buf_idx].buf_offset & 0x80000000) >> 31)

#define IN_BUF_ATTR_GET(p_mctrl, attr_parm) (((p_mctrl)->in_buf_attr & attr_parm) / (attr_parm & ~(attr_parm-1)))
#define IN_BUF_ATTR_SET(p_mctrl, attr_parm, value)  do{(p_mctrl)->in_buf_attr = ((p_mctrl)->in_buf_attr & ~(attr_parm)) | ((value * (attr_parm & ~(attr_parm-1))) & attr_parm);}while(0)
#define OUT_BUF_ATTR_GET(p_mctrl, attr_parm) (((p_mctrl)->out_buf_attr & attr_parm) / (attr_parm & ~(attr_parm-1)))
#define OUT_BUF_ATTR_SET(p_mctrl, attr_parm, value)  do{(p_mctrl)->out_buf_attr = ((p_mctrl)->out_buf_attr & ~(attr_parm)) | ((value * (attr_parm & ~(attr_parm-1))) & attr_parm);}while(0)

#else
#define IN_BUF_INDEX(p_mctrl, buf_idx)   ((p_mctrl)->in_buf_index[buf_idx])
#define IN_BUF_OFFSET(p_mctrl, buf_idx)  ((p_mctrl)->stripe_inaddr[buf_idx])
#define IN_BUF_NUM(p_mctrl)              (sizeof((p_mctrl)->in_buf_index) / sizeof((p_mctrl)->in_buf_index[0]))
#define OUT_BUF_INDEX(p_mctrl, buf_idx)  ((p_mctrl)->out_buf_index[buf_idx])
#define OUT_BUF_OFFSET(p_mctrl, buf_idx) ((p_mctrl)->stripe_outaddr[buf_idx])
#define OUT_BUF_NUM(p_mctrl)             (sizeof((p_mctrl)->out_buf_index) / sizeof((p_mctrl)->out_buf_index[0]))
#define OUT_BUF_SIZE(p_mctrl, buf_idx)   ((p_mctrl)->output_buffsize[buf_idx])
#define OUT_BUF_IS_TMP(p_mctrl, buf_idx) ((buf_idx >= 4)? TRUE:FALSE)

#define IN_BUF_ATTR_GET(p_mctrl, attr_parm)  ((attr_parm == NN_IN_BUF_ATTR_ELTWISE_IN_SRC)?(p_mctrl)->eltwise_in1_src: \
                                              (attr_parm == NN_IN_BUF_ATTR_CONV_IN_ISIMG)?(p_mctrl)->cnn_in_isimg: \
                                              (attr_parm == NN_IN_BUF_ATTR_PREPROC_IN_FMT)?(p_mctrl)->preproc_in_fmt:0)

#define IN_BUF_ATTR_SET(p_mctrl, attr_parm, value)  do{ \
                                                        switch(attr_parm) { \
                                                        case NN_IN_BUF_ATTR_ELTWISE_IN_SRC: (p_mctrl)->eltwise_in1_src = value; break; \
                                                        case NN_IN_BUF_ATTR_CONV_IN_ISIMG:  (p_mctrl)->cnn_in_isimg = value; break; \
                                                        case NN_IN_BUF_ATTR_PREPROC_IN_FMT: (p_mctrl)->preproc_in_fmt = value; break; \
                                                        default: break;} \
                                                    }while(0)

#define OUT_BUF_ATTR_GET(p_mctrl, attr_parm) ((attr_parm == NN_OUT_BUF_ATTR_PRESERVE)?(p_mctrl)->is_preserve: \
                                              (attr_parm == NN_OUT_BUF_ATTR_PREPROC_OUT_FMT)?(p_mctrl)->preproc_out_fmt:0)

#define OUT_BUF_ATTR_SET(p_mctrl, attr_parm, value)  do{ \
                                                        switch(attr_parm) { \
                                                        case NN_OUT_BUF_ATTR_PRESERVE:        (p_mctrl)->is_preserve = value; break; \
                                                        case NN_OUT_BUF_ATTR_PREPROC_OUT_FMT: (p_mctrl)->preproc_out_fmt = value; break; \
                                                        default: break;} \
                                                    }while(0)

#endif //CNN_FMT_V4

typedef struct _NN_GEN_CHIP_CONFIG {
    UINT32 id;
    UINT32 gentool_vers;
} NN_GEN_CHIP_CONFIG;

typedef struct _NN_GEN_MODE_CTRL {
	NN_GEN_TRIG_SRC trig_src;
	NN_GEN_ENG_TYPE eng;
	UINT32 mode;                        ///< based on trig_src
	UINT32 layer_index;
	UINT32 nn_layer_index;				///< based on caffe model
	UINT32 addr;
	UINT32 size;
#if !CNN_25_MATLAB
	NN_IOMEM iomem;
#endif
	UINT32 tot_trig_eng_times;          ///< indicate total engine trigger times (include pipe and linked)
#if CNN_528_PSW
#if CNN_FMT_V4
	UINT32 in_bufinfo_addr;
	UINT32 in_bufinfo_cnt;              ///< size = in_bufinfo_cnt  * sizeof(NN_BUF_INFO_IN)
	UINT32 out_bufinfo_addr;
	UINT32 out_bufinfo_cnt;             ///< size = out_bufinfo_cnt * sizeof(NN_BUF_INFO_OUT)
#else
#if CNN_CGEN_NEW_TMP_BUF
	INT32 in_buf_index[4];
	INT32 out_buf_index[8];
	UINT32 stripe_inaddr[4];
	UINT32 stripe_outaddr[8];
	INT32 output_buffsize[8];
#else
	INT32 in_buf_index[3];
	INT32 out_buf_index[2];
	UINT32 stripe_inaddr[3];
	UINT32 stripe_outaddr[2];
	INT32 output_buffsize[2];
#endif
#endif
	UINT32 prev_num;                    ///< previous num
	UINT32 prev_layer_idx_addr;         ///< start address (byte)
	UINT32 next_num;                    ///< next layer num
	UINT32 next_layer_idx_addr;         ///< start address (byte)
	UINT32 idea_cycle;                  ///< idea cycle info
	//-----
#if CNN_FMT_V4
	UINT32 in_buf_attr;                 ///< input  buffer attribute (bit-wise value), using NN_IN_BUF_ATTR enum
	UINT32 out_buf_attr;                ///< output buffer attribute (bit-wise value), using NN_OUT_BUF_ATTR enum
#else
	UINT32 eltwise_in1_src;
#if CNN_25_MATLAB
	UINT32 preproc_out_fmt;
	UINT32 preproc_in_fmt;
	UINT32 cnn_in_isimg;
#else
	UINT8  preproc_out_fmt;
	UINT8  preproc_in_fmt;
	UINT8  cnn_in_isimg;
	UINT8  is_preserve;
#endif
#endif//CNN_FMT_V4
#endif//CNN_528_PSW
} NN_GEN_MODE_CTRL;

typedef struct _NN_GEN_MODEL_HEAD {
	NN_GEN_CHIP_CONFIG chip;            ///< configuration of chip
	UINT32 mode_ctrl_num;           // number of mode control
	UINT32 layer_num;               // total layers
	UINT32 model_size;              // model size
	UINT32 parm_size;               // parameters size
	UINT32 io_buff_size;            // IO buffer size
#if CNN_528_PSW
#if !CNN_25_MATLAB
	UINT32 iomem_size;              // io memory information size (sai/sao)
#endif
	UINT32 layer_id_list_size;      // pre/last layer id list size
#if !CNN_25_MATLAB
	UINT32 external_size;           // output buffer dim/lofs info
#endif
#if CNN_FMT_V4
	UINT32 bufinfo_size;            // bufinfo total size (buf id/offset/size)
#endif
#endif
} NN_GEN_MODEL_HEAD;

typedef struct _NN_GEN_NET_INFO {
    NN_GEN_MODEL_HEAD *p_head;
    NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
#if CNN_528_PSW
    UINT32 *p_id_list;
#endif
} NN_GEN_NET_INFO;

#if CNN_FMT_V4
typedef enum {
	NN_RGB888,
	NN_BGR888,
	NN_YUV420,
	NN_Y_ONLY,
	NN_UV_PAC,
	NN_FEATURE,
} NN_IN_OUT_BUF_FMT;

typedef enum {
	NN_BITDEPTH_8,
	NN_BITDEPTH_16,
	NN_BITDEPTH_32,
} NN_BITDEPTH_FMT;

typedef struct _NN_IN_OUT_BUF_INFO {
	char    layer_name[192];
	char    layer_type[96];
	UINT16  caffe_layer_index;
	UINT16  fusion_layer_index;
	UINT16  width;
	UINT16  height;
	UINT16  channel;
	UINT16  batch;
	UINT16  time;
	UINT8   out_bitdepth;
	UINT8   out_sign_bit_num;
	INT8    out_int_bit_num;
	INT8    out_frac_bit_num;
	float   out_scale_ratio;
	UINT16 	out_lofs;
	UINT16 	out_ch_ofs;
	UINT32 	out_batch_ofs;
	UINT32 	out_time_ofs;
	UINT32  ext_id;
	char    data_order[8];
}NN_IN_OUT_BUF_INFO;
#endif
#if !CNN_25_MATLAB
typedef struct _NN_LAYER_OUTPUT_INFO
{
	char layer_name[100];
	UINT16 caffe_layer_index;
	UINT16 fusion_layer_index;
	UINT16 out_width;
	UINT16 out_height;
	UINT16 out_channel;
	UINT16 out_batch;
	UINT16 out_time;
	UINT16 reserve;
	UINT8 out_bitdepth;
	UINT8 out_sign_bit_num;
	UINT8 out_int_bit_num;
	UINT8 out_frac_bit_num;
	float out_scale_ratio;
	UINT16 out_lofs;
	UINT16 out_ch_ofs;
	UINT32 out_batch_ofs;
	UINT32 out_time_ofs;
#if CNN_FMT_V4
	char   layer_type[96];
	char   data_order[8];
	UINT32 reserved[8];
#endif
} NN_LAYER_OUTPUT_INFO;
#else
typedef struct _NN_LAYER_OUTPUT_INFO {
	char	layer_name[101];
	UINT16	caffe_layer_index;
	UINT16	fusion_layer_index;
	UINT16	out_width;
	UINT16	out_height;
	UINT16	out_channel;
	UINT8	out_batch;
	UINT8	out_bitdepth;
	UINT8	out_sign_bit_num;
	UINT8	out_int_bit_num;
	UINT8	out_frac_bit_num;
	float	out_scale_ratio;
} NN_LAYER_OUTPUT_INFO;
#endif
typedef struct _NN_IN_OUT_FMT {
	char   model_fmt[20];
	UINT16 model_width;
	UINT16 model_height;
	UINT16 model_channel;
	UINT16 model_batch;
	char   in_fmt[20];
	UINT16 in_channel;
	UINT16 reserve;
} NN_IN_OUT_FMT;

#endif  /* _NN_NET_H_ */


