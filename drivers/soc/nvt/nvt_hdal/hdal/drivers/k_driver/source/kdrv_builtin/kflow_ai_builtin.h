/**
	@brief Header file of definition of kflow ai builtin.

	@file kflow_ai_builtin.h

	@ingroup kflow_ai_builtin

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _KFLOW_AI_BUILTIN_H_
#define _KFLOW_AI_BUILTIN_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#ifdef __KERNEL__
#include <linux/ioctl.h>
#endif
#include "kwrap/ioctl.h"
#include "kdrv_ai/kdrv_ai.h"
#include "kdrv_ipp_builtin.h"
#include "pdcnn_api.h"
#include "kflow_ai_builtin_api.h"

#define USE_NEON                        1
#define CNN_528_PSW                     1  ////////// _TODO : remove this later
#define CNN_25_MATLAB                   0 //0: new interface, 1: matlab interface
#define CNN_CGEN_NEW_TMP_BUF            1 //0: original mctrl, 1: mctrl with new tmp buffer info (only available if CNN_25_MATLAB=0) (only available if CNN_FMT_V4=0)
#define CNN_FMT_V4                      1 //0: v3 format, 1: v4 format (only available if CNN_25_MATLAB=0)
#define AI_V4                           CNN_FMT_V4
#define CNN_INNER_POSTPROC              1 //0: user call postproc, 1: ai2 call postproc

#define NVT_FAST_AI_FLOW                1 //1: enable ai fastboot flow

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define NN_SUPPORT_NET_MAX		        1
#define NN_IMEM_NUM                     8
#define KFLOW_AI_BUILTIN_YUVQ_MAX       2
#define KFLOW_AI_BUILTIN_RSLTQ_MAX      30
#define PD_RSLT_SIZE                    0x980  // (PDCNN_MAX_OUTNUM * sizeof(PDCNN_RESULT) + 32)

// pd postproc
#define VDO_SIZE_W          960
#define VDO_SIZE_H          540
#define SENSOR_MAIN_SIZE_W  1920
#define SENSOR_MAIN_SIZE_H  1080

#define OUT_BUF_ATTR_GET(p_mctrl, attr_parm) (((p_mctrl)->out_buf_attr & attr_parm) / (attr_parm & ~(attr_parm-1)))

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef KDRV_IPP_BUILTIN_FMD_CB_INFO KFLOW_AI_BUILTIN_FMD_INFO;

/**
    linked list configuration
*/
typedef KDRV_AI_LL_HEAD VENDOR_AI_LL_HEAD;

typedef enum _HD_VIDEO_PXLFMT {
	HD_VIDEO_PXLFMT_RGB888_PLANAR =     0x23180888, ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)
	HD_VIDEO_PXLFMT_YUV420 =            0x520c0420, ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format with U1V1
	HD_VIDEO_PXLFMT_YUV422_ONE =        0x51100422, ///< 1 plane, pixel=UYVY(w,h), packed format with Y2U1V1
	HD_VIDEO_PXLFMT_Y8 =                0x51080400, ///< 1 plane, pixel=Y(w,h)
	ENUM_DUMMY4WORD(HD_VIDEO_PXLFMT)
} HD_VIDEO_PXLFMT;

typedef enum _NN_OUT_BUF_ATTR {
	NN_OUT_BUF_ATTR_PRESERVE         = 0x80000000,
	NN_OUT_BUF_ATTR_PREPROC_OUT_FMT  = 0x0000000f,
	ENUM_DUMMY4WORD(NN_OUT_BUF_ATTR)
} NN_OUT_BUF_ATTR;

//========== for first layer linked list mode ==========
typedef struct {
	unsigned : 32;
	unsigned : 32;
} ENG_NULL_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned addr : 32;
	} bit;
} ENG_ADDR_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned ofs : 29;
	} bit;
} ENG_OFS_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned width : 10;
		unsigned : 2;
		unsigned height : 10;
	} bit;
} CNN_SIZE0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned channel : 12;
		unsigned batch_num : 7;
	} bit;
} CNN_SIZE1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned area : 14;
		unsigned : 2;
		unsigned channel : 12;
	} bit;
} NUE_SIZE1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned obj_num : 7;
		unsigned : 1;
		unsigned roi_num : 7;
	} bit;
} NUE_SIZE2_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned width : 11;
		unsigned : 5;
		unsigned height : 11;
	} bit;
} NUE2_SIZE0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned ofs : 17;
	} bit;
} NUE2_LOFS_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_rate : 7;
		unsigned : 1;
		unsigned v_rate : 7;
	} bit;
} NUE2_SCL0_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_sfact : 16;
		unsigned v_sfact : 16;
	} bit;
} NUE2_SCL1_REG;

typedef union {
	UINT64 reg;
	struct {
		unsigned h_scl_size : 11;
		unsigned : 5;
		unsigned v_scl_size : 11;
	} bit;
} NUE2_SCL_SIZE_REG;

typedef struct _CNN_LL_PARM {
	ENG_ADDR_REG input;
	ENG_ADDR_REG interm_in;
	ENG_ADDR_REG output[2];
	ENG_ADDR_REG weight;
	ENG_ADDR_REG kmean;
	ENG_ADDR_REG bias_bnscal;
	CNN_SIZE0_REG size0;
	CNN_SIZE1_REG size1;
} CNN_LL_PARM;

typedef struct _NUE_LL_PARM {
	ENG_ADDR_REG input;
	ENG_ADDR_REG elt_in;
	ENG_ADDR_REG roi_in;
	ENG_ADDR_REG output;
	ENG_ADDR_REG sv;
	ENG_ADDR_REG alpha;
	ENG_ADDR_REG rho;
	ENG_ADDR_REG kmean;
	ENG_NULL_REG reserved[2];
	NUE_SIZE1_REG size1;
	NUE_SIZE2_REG size2;
} NUE_LL_PARM;

typedef struct _NUE2_LL_PARM {
	ENG_ADDR_REG input[3];
	ENG_ADDR_REG output[3];
	NUE2_SIZE0_REG size0;
	NUE2_LOFS_REG ilofs[3];
	NUE2_LOFS_REG olofs[3];
	NUE2_SCL0_REG scale0;
	NUE2_SCL1_REG scale1;
	NUE2_SCL_SIZE_REG scale_size;
} NUE2_LL_PARM;

typedef enum {
	BUF_IN_IDX = 0,
	BUF_PARM_IDX0 = 1,
	BUF_INTERM_IDX,
	BUF_PARM_IDX1,
	BUF_PARM_IDX2,

	BUF_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(BUF_INMEM_IDX_TYPE)
} BUF_INMEM_IDX_TYPE;

typedef enum {
	NUE2_IN_IDX0 = BUF_IN_IDX,
	NUE2_IN_IDX1 = 1,
	NUE2_IN_IDX2 = 2,

	NUE2_TOTAL_IDX_INMEM,
	ENUM_DUMMY4WORD(NUE2_INMEM_IDX_TYPE)
} NUE2_INMEM_IDX_TYPE;

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

typedef struct _VENDOR_AIS_IMG_PARM {
	UINT32 pa;
    UINT32 va;
	UINT32 fmt;                         ///< image format, refer to HD_VIDEO_PXLFMT.
	UINT16 width;                       ///< image width
	UINT16 height;                      ///< image height
	UINT16 channel;                     ///< image channels
	UINT16 batch_num;                   ///< batch processing number
	UINT32 line_ofs;                    ///< line offset
	UINT32 channel_ofs;                 ///< channel offset
	UINT32 batch_ofs;                   ///< batch offset
#if AI_SUPPORT_MULTI_FMT
	UINT32 fmt_type;                    ///< format type
#endif
	UINT32 timestamp;                   ///< timestamp of yuv
	UINT32 release_flag;                ///< release flag from ipp
} VENDOR_AIS_IMG_PARM;

typedef enum {
	KFLOW_AI_BUILTIN_DT_PARAM_KERL_PARM,
	KFLOW_AI_BUILTIN_DT_PARAM_USER_MODEL,
	KFLOW_AI_BUILTIN_DT_PARAM_PD_BUFF,
	KFLOW_AI_BUILTIN_DT_PARAM_IO_BUFF,
} KFLOW_AI_BUILTIN_DT_PARAM;

typedef struct {
	UINT32 va;
	UINT32 width;
	UINT32 height;
	UINT32 line_ofs;
	UINT32 timestamp;
	BOOL   release_flag;
} KFLOW_AI_BUILTIN_YUV_INFO;

typedef struct {
	UINT32                      front;                  ///< front pointer
	UINT32                      rear;                   ///< rear pointer
	UINT32                      b_full;                 ///< full flag
	KFLOW_AI_BUILTIN_YUV_INFO   queue[KFLOW_AI_BUILTIN_YUVQ_MAX];
} KFLOW_AI_BUILTIN_YUVQ, *PKFLOW_AI_BUILTIN_YUVQ;

typedef struct {
	UINT32                      front;                  ///< front pointer
	UINT32                      rear;                   ///< rear pointer
	UINT32                      b_full;                 ///< full flag
	KFLOW_AI_BUILTIN_RSLT_INFO  *queue;
} KFLOW_AI_BUILTIN_RSLTQ, *PKFLOW_AI_BUILTIN_RSLTQ;

/**
	Starting Memory information of normal dump stage
*/
typedef struct _KFLOW_AI_BUILTIN_KERL_START_MEM {
	UINT32 pa;
	UINT32 va;
} KFLOW_AI_BUILTIN_KERL_START_MEM;

typedef struct _KFLOW_AI_BUILTIN_MEM {
	UINT32 addr;
	UINT32 size;
} KFLOW_AI_BUILTIN_MEM;

typedef struct _KFLOW_AI_BUILTIN_OBJ {
	KFLOW_AI_BUILTIN_YUVQ        yuv_queue;
	KFLOW_AI_BUILTIN_RSLTQ       rslt_queue;
	KFLOW_AI_BUILTIN_MAP_MEM_PARM mem_parm;
	UINT32 trig_cnt;
	BOOL stop_trig;
	int yuv_unlock_cnt;
} KFLOW_AI_BUILTIN_OBJ;

#endif  /* _KFLOW_AI_BUILTIN_H_ */
