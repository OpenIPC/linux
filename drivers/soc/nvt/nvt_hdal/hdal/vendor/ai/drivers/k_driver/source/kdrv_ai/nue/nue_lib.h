/**
    Public header file for NUE module.

    @file       nue_lib.h
    @ingroup    mIIPPNUE

    @brief

    How to start:\n
    1. nue_open\n
    2. nue_setmode\n
    3. nue_start\n


    How to stop:\n
    1. nue_wait_frameend\n
    2. nue_pause\n
    3. nue_close\n

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _NUE_LIB_H_
#define _NUE_LIB_H_

#include "kdrv_ai.h"
#include "kwrap/type.h"
//#include "kwrap/error_no.h"
/**
    @addtogroup mIIPPNUE
*/
//@{

/**
    Define NUE interrput enable.
*/
//@{
#define NUE_INTE_ROUTEND                0x00000001
#define NUE_INTE_DMAIN0END              0x00000002
#define NUE_INTE_DMAIN1END              0x00000004
#define NUE_INTE_DMAIN2END              0x00000008
#define NUE_INTE_LLEND                  0x00000100
#define NUE_INTE_LLERR                  0x00000200
#define NUE_INTE_FCD_DECODE_DONE        0x00010000
#define NUE_INTE_FCD_VLC_DEC_ERR        0x00020000
#define NUE_INTE_FCD_BS_SIZE_ERR        0x00040000
#define NUE_INTE_FCD_SPARSE_DATA_ERR    0x00080000
#define NUE_INTE_FCD_SPARSE_INDEX_ERR   0x00100000
//#define NUE_INTE_AXI0_ERR               0x01000000
//#define NUE_INTE_AXI1_ERR               0x02000000
//#define NUE_INTE_AXI2_ERR               0x04000000

#define NUE_INTE_CNN_OCCUPY_APP_TO_APP_ERR 0x00001000
#define NUE_INTE_CNN_OCCUPY_APP_TO_LL_ERR  0x00002000
#define NUE_INTE_CNN_OCCUPY_LL_TO_APP_ERR  0x00004000
#define NUE_INTE_CNN_OCCUPY_LL_TO_LL_ERR   0x00008000

#define NUE_INTE_CHECKSUM_MISMATCH      0x02000000

#define NUE_INTE_ALL                    0x021ff311
#define NUE_INTE_RUNLL                  0x021ef300
//@}

/**
    Define NUE interrput status.
*/
//@{
#define NUE_INT_ROUTEND                0x00000001
#define NUE_INT_DMAIN0END              0x00000002
#define NUE_INT_DMAIN1END              0x00000004
#define NUE_INT_DMAIN2END              0x00000008
#define NUE_INT_LLEND                  0x00000100
#define NUE_INT_LLERR                  0x00000200
#define NUE_INT_FCD_DECODE_DONE        0x00010000
#define NUE_INT_FCD_VLC_DEC_ERR        0x00020000
#define NUE_INT_FCD_BS_SIZE_ERR        0x00040000
#define NUE_INT_FCD_SPARSE_DATA_ERR    0x00080000
#define NUE_INT_FCD_SPARSE_INDEX_ERR   0x00100000
//#define NUE_INT_AXI0_ERR               0x01000000
//#define NUE_INT_AXI1_ERR               0x02000000
//#define NUE_INT_AXI2_ERR               0x04000000

#define NUE_INT_CNN_OCCUPY_APP_TO_APP_ERR 0x00001000
#define NUE_INT_CNN_OCCUPY_APP_TO_LL_ERR  0x00002000
#define NUE_INT_CNN_OCCUPY_LL_TO_APP_ERR  0x00004000
#define NUE_INT_CNN_OCCUPY_LL_TO_LL_ERR   0x00008000

#define NUE_INT_CHECKSUM_MISMATCH      0x02000000
#define NUE_INT_ALL                    0x021ff311
//@}

/**
    @name Define NUE svm function enable.
*/
//@{
#define NUE_SVM_KER2_EN             0x00010000  ///< Kernel2 function enable
#define NUE_SVM_CAL_EN              0x00020000  ///< Svmcal function enable
#define NUE_SVM_INTERLACE_EN        0x00040000  ///< svm interlace enable
#define NUE_FCD_VLC_EN              0x00100000  ///< compression variable length encoding enable
#define NUE_FCD_QUANTI_EN           0x00200000  ///< compression quantization enable
#define NUE_FCD_SPARSE_EN           0x00400000  ///< compression sparse encoding enable
#define NUE_FCD_QUANTI_KMEANS_EN    0x00800000  ///< compression quantization kmeans enable
#define NUE_RELU_EN                 0x02000000  ///< relu function enable
#define NUE_SVM_INTERMEDIATE_IN_EN  0x04000000  ///< svm intermediate input enable
#define NUE_SVM_SCI_NOTATION_EN     0x08000000  ///< svm scientific notation enable
#define NUE_ALL_EN                  0x0ef70000  ///< all function enable
//@}

#define NUE_CYCLE_TEST ENABLE
#define NUE_CYCLE_LAYER DISABLE
#define NUE_MAX_IO_NUM 8
/**
    NUE mode selection

    Select NUE mode
*/
typedef enum {
	NUE_OPMODE_SVM              = 0,    ///< output SVM results
	NUE_OPMODE_M_LINEAR_SVM     = 1,    ///< output multi-linear svm results
	NUE_OPMODE_DOT,                     ///< output Dot product results
	NUE_OPMODE_FULLYCONNECT,            ///< output fully connect results
	NUE_OPMODE_RELU,                    ///< output RELU results
	NUE_OPMODE_ROIPOOLING,              ///< output roi-pooling results
	NUE_OPMODE_PERMUTE,                 ///< output permute results
	NUE_OPMODE_REORG,                   ///< output reorganize results
	NUE_OPMODE_ANCHOR,                  ///< output anchor results
	NUE_OPMODE_SOFTMAX,                 ///< output softmax results
	NUE_OPMODE_USERDEFINE,              ///< all manual mode
	NUE_OPMODE_USERDEFINE_NO_INT,       ///< all manual mode without interrupt

	ENUM_DUMMY4WORD(NUE_OPMODE)
} NUE_OPMODE;

/**
    NUE In/Out Type
*/
//@{
typedef enum {
	NUE_INT8        = 0,
	NUE_UINT8       = 1,
	NUE_INT16       = 2,
	NUE_UINT16      = 3,
	ENUM_DUMMY4WORD(NUE_IO_TYPE)
} NUE_IO_TYPE;
//@}

/**
    NUE Kernel Type
*/
//@{
typedef enum {
	NUE_SVMKER_LINEAR   = 0,
	NUE_SVMKER_POLY     = 1,
	NUE_SVMKER_RBF      = 2,
	NUE_SVMKER_SIGMOID  = 3,
	NUE_SVMKER_INTER    = 4,
	ENUM_DUMMY4WORD(NUE_SVMKER_TYPE)
} NUE_SVMKER_TYPE;
//@}

/**
    NUE RELU Type
*/
//@{
typedef enum {
	NUE_RELU_LEAKY      = 0,
	NUE_RELU_ABS        = 1,
	NUE_RELU_ZERO,
	ENUM_DUMMY4WORD(NUE_RELU_TYPE)
} NUE_RELU_TYPE;
//@}

/**
    NUE Input Buffer ID
    NUE Input buffer ID.
*/
//@{
typedef enum {
	NUE_IN0_BUF     = 0,
	NUE_IN1_BUF     = 1,
	NUE_SV_BUF,
	NUE_ALPHA_BUF,
	NUE_RHO_BUF,
	NUE_ROI_BUF,
	NUE_KQ_BUF,
	ENUM_DUMMY4WORD(NUE_IN_BUFID)
} NUE_IN_BUFID;
//@}

/**
    NUE Open Object.

    Open Object is used for opening NUE module.
    \n Used for NUE_open()

*/
//@{
typedef struct _NUE_OPENOBJ {
	UINT32 clk_sel;                             ///< support 220/192/160/120/80/60/48 Mhz
	VOID (*fp_isr_cb)(UINT32 int_status);       ///< isr callback function
} NUE_OPENOBJ;
//@}

/**
    NUE Size.

    Structure of size.
*/
//@{
typedef struct _NUE_SIZE {
	UINT32 width;
	UINT32 height;
	UINT32 channel;
} NUE_SIZE;
//@}

/**
    NUE Input Size for svm
*/
//@{
typedef struct _NUE_SVM_IN_SIZE {
	UINT32 insize;
	UINT32 channel;
	UINT32 objnum;
	UINT32 sv_w;
	UINT32 sv_h;
} NUE_SVM_IN_SIZE;
//@}


/**
    NUE Kernel Parameters.

    Structure of information in NUE kernel parameters.
*/
//@{
typedef struct _NUE_SVMKERL_PARM {
	UINT32 ft_shf;
	UINT32 gv;                      ///< input gamma value
	UINT32 gv_shf;                  ///< gamma shift;
	INT32  coef;                    ///< input coefficient value
	UINT32 degree;                  ///< input polynomial degree
	INT32  rho;                     ///< input rho value
	UINT32 rho_fmt;                 ///< rho shift; [3+N]
	UINT32 alpha_shf;               ///< alpha shift;
} NUE_SVMKERL_PARM;
//@}

/**
    NUE DRAM input and output addresses.

    Structure of information in DRAM addresses.
*/
//@{
typedef struct _NUE_DMAIO_ADDR {
	UINT32 addrin0;                 ///< input starting address of input data 0
	UINT32 addrin1;                 ///< input starting address of input data 1
	UINT32 addrin_sv;               ///< input starting address of SV
	UINT32 addrin_alpha;            ///< input starting address of alpha
	UINT32 addrin_rho;              ///< input starting address of rho
	UINT32 addrin_roi;              ///< input starting address of roi
	//UINT32 addrin_ll;             ///< input starting address of linked list
	//UINT32 addrin_ll_base;
	UINT32 addrin_kq;               ///< input starting address of kmeans
	UINT32 addr_out;                ///< output starting address
	UINT8  drv_dma_not_sync[NUE_MAX_IO_NUM];         ///< 1: don't need to sync, 0:force to sync
} NUE_DMAIO_ADDR;
//@}

/**
    NUE LL struct

    Structure of LL information
*/
//@{
typedef struct _NUE_LL_PRM {
	UINT32 addrin_ll;               ///< input starting address of linked list
	UINT32 addrin_ll_base;
} NUE_LL_PRM;
//@}


/**
    NUE Mode type
*/
//@{
typedef enum {
	NUE_SVM             = 0,
	NUE_ROIPOOLING      = 1,
	NUE_RESERVED        = 2,
	NUE_REORGANIZATION  = 3,
	NUE_PERMUTE         = 4,
	NUE_ANCHOR          = 5,
	NUE_SOFTMAX         = 6,
	ENUM_DUMMY4WORD(NUE_MODE_TYPE)
} NUE_MODE_TYPE;
//@}

/**
    NUE Kernel1 Type
*/
//@{
typedef enum {
	NUE_SVMKER1_DOT    = 0,
	NUE_SVMKER1_RBF    = 1,
	NUE_SVMKER1_INTER  = 2,
	ENUM_DUMMY4WORD(NUE_SVMKER1_TYPE)
} NUE_SVMKER1_TYPE;
//@}

/**
    NUE Kernel2 Type
*/
//@{
typedef enum {
	NUE_SVMKER2_POLY       = 0,
	NUE_SVMKER2_SIGMOID    = 1,
	ENUM_DUMMY4WORD(NUE_SVMKER2_TYPE)
} NUE_SVMKER2_TYPE;
//@}

/**
    NUE Result Type
*/
//@{
typedef enum {
	NUE_SVMRST_FULL          = 0,
	NUE_SVMRST_SUB_RHO       = 1,
	NUE_SVMRST_ACCRST_SUBRHO = 2,
	ENUM_DUMMY4WORD(NUE_SVMRST_TYPE)
} NUE_SVMRST_TYPE;
//@}

/**
    NUE DRAM Output Path
*/
//@{
typedef enum {
	NUE_OUT_RST          = 0,
	NUE_OUT_INTERMEDIATE = 1,
	NUE_OUT_KERL2        = 2,
	NUE_OUT_KERL1        = 3,
	ENUM_DUMMY4WORD(NUE_DMAO_PATH_TYPE)
} NUE_DMAO_PATH_TYPE;
//@}


/**
    NUE Roi-pooling ker size
*/
//@{
typedef enum {
	NUE_ROI_POOL_SIZE_6     = 0,
	NUE_ROI_POOL_SIZE_7     = 1,
	NUE_ROI_POOL_SIZE_14    = 2,
	NUE_PSROI_POOL_SIZE_1   = 3,
	NUE_PSROI_POOL_SIZE_2   = 4,
	NUE_PSROI_POOL_SIZE_3   = 5,
	NUE_PSROI_POOL_SIZE_4   = 6,
	NUE_PSROI_POOL_SIZE_5   = 7,
	NUE_PSROI_POOL_SIZE_6   = 8,
	NUE_PSROI_POOL_SIZE_7   = 9,
	ENUM_DUMMY4WORD(NUE_ROI_POOL_SIZE)
} NUE_ROI_POOL_SIZE;
//@}
/**
    NUE Roi-pooling mode
*/
typedef enum {
	NUE_NORMAL_ROI    = 0,
	NUE_PS_ROI        = 1,
	ENUM_DUMMY4WORD(NUE_ROI_MODE)
} NUE_ROI_MODE;
//@}

/**

    NUE permute mode
*/
typedef enum {
	NUE_PERMUTE_CH_PRIOR      = 0,
	NUE_PERMUTE_HEIGHT_PRIOR  = 1,
	ENUM_DUMMY4WORD(NUE_PERMUTE_MODE)
} NUE_PERMUTE_MODE;
//@}
/**
    NUE SVM Results.

    Structure of information in svm results.
*/
//@{
typedef struct _NUE_SVMRSTS {
	INT32 rslts[32];                    ///< output results
} NUE_SVMRSTS;
//@}

/**
    NUE SVM User Define Parameters
*/
//@{
typedef struct _NUE_SVMUSER_DEF {
	NUE_SVMKER1_TYPE kerl1type;        ///< kernel1 type
	NUE_SVMKER2_TYPE kerl2type;        ///< kernel2 type
	NUE_SVMRST_TYPE rslttype;          ///< Result type
	BOOL dmao_en;                      ///< DMA output enable
	NUE_DMAO_PATH_TYPE dmao_path;      ///< DMA output path
} NUE_SVMUSER_DEF;
//@}

/**
    NUE FCD parameters
*/
//@{
typedef struct _NUE_FCD_PARM {
	BOOL fcd_vlc_en;
	BOOL fcd_quanti_en;
	BOOL fcd_sparse_en;
	BOOL fcd_quanti_kmean_en;
	BOOL fcd_kq_tbl_update_disable;
	UINT32 fcd_enc_bit_length;
	UINT32 fcd_vlc_code[23];
	UINT32 fcd_vlc_valid[23];
	UINT32 fcd_vlc_ofs[23];
} NUE_FCD_PARM;
//@}

/**
    NUE SVM Parameters
*/
//@{
typedef struct _NUE_SVM_PARA {
	NUE_SVMKER_TYPE svmker_type;        ///< input NUE kernel type (not work in used define mode)
	BOOL in_rfh;                        ///< input refresh
	NUE_SVMKERL_PARM svmker_parms;      ///< input NUE kernel parameters
	NUE_FCD_PARM fcd_parm;              ///< input FCD parameters
	UINT32 ilofs;                       ///< input lineoffset
} NUE_SVM_PARA;
//@}

/**
    NUE ReLU Parameters.

    Structure of information in NUE ReLU parameters.
*/
//@{
typedef struct _NUE_RELU_PARM {
	NUE_RELU_TYPE relu_type;             ///< input RELU type (not work in used define mode)
	INT32 leaky_val;
	UINT32 leaky_shf;
	INT32 out_shf;
} NUE_RELU_PARM;
//@}

/**
    NUE ROI-Pooling patameters
*/
//@{
typedef struct _NUE_ROI_POOL_PARM {
	UINT32 roi_num;
	UINT32 olofs;
	NUE_ROI_POOL_SIZE size;
	UINT32 ratio_mul;
	UINT32 ratio_shf;
	INT32 out_shf;
	NUE_ROI_MODE mode;
} NUE_ROI_POOL_PARM;
//@}

/**
    NUE Reorgnize parameters
*/
//@{
typedef struct _NUE_REORG_PARM {
	INT32 out_shf;
} NUE_REORG_PARM;
//@}

/**
    NUE Permute parameters
*/
//@{
typedef struct _NUE_PERMUTE_PARM {
	NUE_PERMUTE_MODE mode;
	INT32 out_shf;
	UINT32 olofs;
} NUE_PERMUTE_PARM;
//@}

/**
    NUE anchor transform parameters
*/
//@{
typedef struct _NUE_ANCHOR_PARM {
	BOOL at_table_update;
	UINT32 at_w_shf;
} NUE_ANCHOR_PARM;
//@}

/**
    NUE softmax parameters
*/
//@{
typedef struct _NUE_SOFTMAX_PARM {
	INT32 in_shf;
	INT32 out_shf;
	UINT32 group_num;
	UINT32 set_num;
} NUE_SOFTMAX_PARM;
//@}

/**
    NUE User Define
*/
//@{
typedef struct _NUE_USER_DEF {
	UINT32 func_en;
	NUE_MODE_TYPE eng_mode;
	NUE_SVMUSER_DEF svm_userdef;
	BOOL permute_stripe_en;
} NUE_USER_DEF;
//@}

/**
    NUE Input scale shift parameters
*/
//@{
typedef struct _NUE_IN_PARM {
	INT32 in_shf;
	UINT32 in_scale;
} NUE_IN_PARM;
//@}

/**
    NUE chksum parameters
*/
//@{
typedef struct _NUE_CHKSUM_PARM {
	UINT32 out_chksum;
} NUE_CHKSUM_PARM;
//@}

/**
    NUE Entire Configuration

    Structure of NUE module parameters when one wants to configure this module.
*/
//@{
typedef struct _NUE_PARM_STRUCT {
	NUE_IO_TYPE intype;
	NUE_IO_TYPE outtype;
	NUE_IN_PARM in_parm;
	NUE_CHKSUM_PARM chksum_parm;
	NUE_SIZE insize;                    ///< input size
	NUE_SVM_IN_SIZE insvm_size;
	NUE_DMAIO_ADDR dmaio_addr;          ///< DRAM input and output addresses
	NUE_SVM_PARA svm_parm;
	NUE_RELU_PARM relu_parm;
	NUE_ROI_POOL_PARM roipool_parm;
	NUE_REORG_PARM reorg_parm;
	NUE_PERMUTE_PARM permute_parm;
	NUE_ANCHOR_PARM anchor_parm;
	NUE_SOFTMAX_PARM softmax_parm;
	NUE_USER_DEF userdef;
} NUE_PARM;
//@}

//ISR
extern VOID nue_isr(VOID);

//interrupt
extern ER       nue_change_interrupt(UINT32 int_en);
extern UINT32   nue_get_int_enable(VOID);
extern VOID     nue_clr_intr_status(UINT32 status);
extern UINT32   nue_get_intr_status(VOID);
//basic function
extern ER       nue_open(NUE_OPENOBJ *p_obj_cb);
extern BOOL     nue_is_opened(VOID);
extern ER       nue_close(VOID);
extern VOID     nue_wait_frameend(BOOL is_clr_flag);
extern BOOL     nue_isenable(VOID);
extern VOID     nue_set_dmain_lladdr(UINT32 addr0);
extern UINT32   nue_get_dmain_lladdr(VOID);
extern VOID     nue_reset(VOID);
extern ER       nue_ll_pause(VOID);
extern ER       nue_ll_start(VOID);
extern VOID     nue_wait_ll_frameend(BOOL is_clr_flag);
extern ER       nue_ll_setmode(NUE_LL_PRM *ll_prm);

extern ER       nue_ll_isr_start(VOID);

VOID nue_create_resource(VOID *parm, UINT32 clk_freq);
VOID nue_release_resource(VOID);
VOID nue_set_base_addr(UINT32 addr);
UINT32 nue_get_base_addr(VOID);
//Engine
//get mode
extern NUE_MODE_TYPE nue_get_engmode(VOID);
extern NUE_IO_TYPE nue_get_intype(VOID);
extern NUE_IO_TYPE nue_get_outtype(VOID);
extern BOOL nue_get_dmao_en(VOID);
extern NUE_SVMKER_TYPE nue_get_ker(VOID);
extern UINT32 nue_get_kerl_en(VOID);
//set info
extern VOID nue_set_in_rfh(BOOL enable);
//get info
extern NUE_SVM_IN_SIZE nue_get_svmsize(VOID);
extern NUE_SIZE nue_get_insize(VOID);
extern VOID nue_loop_frameend(VOID);
//SVM
extern BOOL nue_get_in_rfh(VOID);
extern ER nue_get_rsts(NUE_SVMRSTS *p_rsts);
extern NUE_SVMKERL_PARM nue_get_parm(VOID);
//ReLU
extern ER nue_get_relu_leaky(INT32 *p_val, UINT32 *p_shf);
extern INT32 nue_get_relu_shf(VOID);
//ROI Pooling
extern UINT32 nue_get_roinum(VOID);
extern NUE_ROI_POOL_SIZE nue_get_roipool_standard_kersize(VOID);
extern NUE_ROI_POOL_SIZE nue_get_roipool_psroi_kersize(VOID);
extern ER nue_get_roipool_ratio(UINT32 *p_mul, UINT32 *p_shf);
extern INT32 nue_get_roipool_shf(VOID);
//Reorg
extern INT32 nue_get_reorgshf(VOID);
//input/output
extern NUE_DMAIO_ADDR nue_get_dmaio_addr(VOID);
//check
extern BOOL nue_chk_ker1_overflow(VOID);
extern UINT32 nue_init(VOID);
extern UINT32 nue_uninit(VOID);
#if NUE_CYCLE_TEST
extern UINT64 nue_dump_total_cycle(UINT32 debug_layer);
#endif
extern VOID nue_reset_status(VOID);
extern ER	nue_pause(VOID);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
extern ER	nue_isr_start(VOID);
extern ER	nue_start(VOID);
extern ER	nue_setmode(NUE_OPMODE mode, NUE_PARM *p_parm);
#endif
extern VOID nue_dma_abort(VOID);

//@}
#endif

