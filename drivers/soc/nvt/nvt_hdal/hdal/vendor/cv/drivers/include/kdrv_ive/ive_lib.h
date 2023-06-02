/*
    Public header file for IVE module.

    This file is the header file that define the API and data type for IVE
    module.

    @file       ive_lib.h
    @ingroup    mIDrvIPPIVE
    @note

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _IVE_LIB_H
#define _IVE_LIB_H

/*
 *#ifdef __cplusplus
 *extern "C" {
 *#endif
 */

#include "kdrv_type.h"

//#include    "Type.h"//a header for basic variable type

/**
    @addtogroup mIDrvIPPIVE
*/
//@{

/**
    Define IVE interrput enable.
*/
//@{
#define IVE_INTE_FRM_END                    0x00000001  ///< Enable interrupt of frame end
#define IVE_INTE_LLEND                      0x00000100  ///< Status of interrupt of frame end(LL)
#define IVE_INTE_LLERROR                    0x00000200  ///< Status of interrupt of frame end(LL)
#define IVE_INTE_LLJOBEND                   0x00000400  ///< Status of interrupt of frame job_end(LL)
#define IVE_INTE_ALL                        0x00000301  ///< Enable all interrupts

//@}

/**
    Define IVE interrput status.
*/
//@{
#define IVE_INT_FRM_END                     0x00000001  ///< Status of interrupt of frame end
#define IVE_INT_LLEND                       0x00000100  ///< Status of interrupt of frame end(LL)
#define IVE_INT_LLERROR                     0x00000200  ///< Status of interrupt of frame end(LL)
#define IVE_INT_LLJOBEND                    0x00000400  ///< Status of interrupt of frame job_end(LL)
#define IVE_INT_ALL                         0x00000701  ///< Status of all interrupts
//@}

#define GEN_COEFF_NUM       10
#define EDGE_COEFF_NUM      9
#define EDGE_TH_LUT_TAP     15
#define MORPH_NEIGH_NUM     24

/*----------- enumeration ------------*/
/**
    Operations of engine action
*/
//@{
typedef enum {
	IVE_OP_OPEN          = 0,    ///< Open engine
	IVE_OP_CLOSE         = 1,    ///< Close engine
	IVE_OP_SETPARAM      = 2,    ///< Set parameter
	IVE_OP_START         = 3,    ///< Start engine
	IVE_OP_PAUSE         = 4,    ///< Pause engine

	ENUM_DUMMY4WORD(IVE_ACTION_OP)
} IVE_ACTION_OP;
//@}

typedef enum {
	IVE_DBG_PORT1 = 0,
	IVE_DBG_PORT2 = 1,
	IVE_DBG_PORT3 = 2,
	IVE_DBG_PORT4 = 3,

	ENUM_DUMMY4WORD(IVE_DBG_PORT_SEL)
} IVE_DBG_PORT_SEL;

/*
    Operations of engine action
*/
typedef enum {
	IVE_START_LOAD  = 0,    ///< Load parameter at engine start(enable 0->1)
	IVE_FRMEND_LOAD = 1,    ///< Load parameter at engine frame end(frame end interrupt occur)

	ENUM_DUMMY4WORD(IVE_LOAD_TYPE)
} IVE_LOAD_TYPE;

typedef enum {
	BURST_64W = 0,
	BURST_48W = 1,
	BURST_32W = 2,
	BURST_16W = 3,

	ENUM_DUMMY4WORD(BURST_LEN_SEL)
} BURST_LEN_SEL;

/**
    State of State Machine
*/
//@{
typedef enum {
	IVE_ENGINE_IDLE  = 0,        ///< Idel state
	IVE_ENGINE_READY = 1,        ///< Ready state
	IVE_ENGINE_PAUSE = 2,        ///< Pause state
	IVE_ENGINE_RUN   = 3,        ///< Run state

	ENUM_DUMMY4WORD(IVE_STATE_MACHINE)
} IVE_STATE_MACHINE;
//@}

typedef enum {
	MEDIAN = 0,
	MAX = 1,
	MIN = 2,


	ENUM_DUMMY4WORD(MEDN_MODE)
} MEDN_MODE;

typedef enum {
	BI_DIR = 0,
	NO_DIR = 1,

	ENUM_DUMMY4WORD(EDGE_MODE)
} EDGE_MODE;

typedef enum {
	EDGE_1B = 0,
	EDGE_4B = 1,
	IMG_MEDIAN_8B = 2,
	IMG_NONMAX_8B = 3,
	EDGE_MAG_THETA_12B = 4,//reserved because of alg error
	EDGE_X_Y_16B = 5,
	INTEGRAL_32B = 6,

	ENUM_DUMMY4WORD(OUT_DATA_TYPE)
} OUT_DATA_TYPE;

typedef enum {
	DILATE = 0,
	ERODE = 1,

	ENUM_DUMMY4WORD(MORPH_OP)
} MORPH_OP;

typedef enum {
	INVAL_0 = 0,
	INVAL_63 = 1,

	ENUM_DUMMY4WORD(IRV_INVAL_VAL)
} IRV_INVAL_VAL;

typedef enum {
	TH_LUT_IN = 0,
	DRAM_4BIT = 1,
	DRAM_1BIT = 2,

	ENUM_DUMMY4WORD(MORPH_IN_SEL)
} MORPH_IN_SEL;

typedef enum {
	HIST_7X7 = 0,
	HIST_5X5 = 1,

	ENUM_DUMMY4WORD(HIST_MODE)
} HIST_MODE;


typedef enum {
	INTEGRAL_FMT_Y = 0,
	INTEGRAL_FMT_UV = 1,
	INTEGRAL_FMT_HSV = 2,
	ENUM_DUMMY4WORD(INTEGRAL_FMT_SEL)
} INTEGRAL_FMT_SEL;

extern UINT32 g_uiIveFrmEndIntCnt;

/*----------- engine driver structure -------------*/
/*
    IVE Open Object - Open Object is used for opening IVE module.
*/
typedef struct _IVE_OPENOBJ {
	VOID (*FP_IVEISR_CB)(UINT32 uiIntStatus, VOID *ive_trig_hdl); ///< isr callback function
	UINT32 IVE_CLOCKSEL;
} IVE_OPENOBJ;

/**
    IVE output setting

    Structure of settings of output function
*/
//@{
typedef struct _IVE_FUNC_EN_STRUCT {
	BOOL bGenFiltEn;    ///< Enable general filter
	BOOL bMednFiltEn;   ///< Enable median filter
	BOOL bEdgeFiltEn;   ///< Enable edge filter
	BOOL bNonMaxEn;     ///< Enable non-maximal suppression
	BOOL bThresLutEn;   ///< Enable threshold LUT
	BOOL bMorphFiltEn;  ///< Enable morphological filter
	BOOL bIntegralEn;   ///< Enable integral image
	BOOL bIrvEn;        ///< Enable IRV

} IVE_FUNC_EN;
//@}

/**
    IVE DMA in info

    Structure of DMA input parameters
*/
//@{
typedef struct _IVE_DMA_IN_INFO_STRUCT {
	UINT32 uiInSaddr;        ///< Start address of input data
	UINT32 uiInLofst;        ///< Line offset of input data
} IVE_DMA_IN_INFO;
//@}

typedef struct _IVE_DMA_LL_INFO_STRUCT {
	UINT32 uiLLAddr;        ///< Start address of input data
} IVE_DMA_LL_INFO;

/**
    IVE DMA out info

    Structure of DMA output parameters
*/
//@{
typedef struct _IVE_DMA_OUT_INFO_STRUCT {
	UINT32 uiOutSaddr;       ///< Start address of output data
	UINT32 uiOutLofst;       ///< line offset of output data
} IVE_DMA_OUT_INFO;
//@}

/**
    IVE image size info

    Structure of input image size
*/
//@{
typedef struct _IVE_IMG_SIZE_STRUCT {
	UINT32 uiWidth;              ///< input image width
	UINT32 uiHeight;             ///< input image height
} IVE_IMG_SIZE;
//@}

/**
    IVE general filter parameters

    Structure of general filter parameters
*/
//@{
typedef struct _IVE_GEN_FILT_PARAM_STRUCT {
	UINT32 *puiGenCoeff;     ///< Pointer of general filter coefficient

} IVE_GEN_FILT_PARAM;
//@}

/**
    IVE edge filter parameters

    Structure of edge filter parameters
*/
//@{
typedef struct _IVE_EDGE_FILT_PARAM_STRUCT {
	EDGE_MODE uiEdgeMode;        ///< selection of edge type
	INT32 *pEdgeCoeff1;      ///< Pointer of edge filter coefficient
	INT32 *pEdgeCoeff2;      ///< Pointer of edge filter coefficient
	UINT32 uiEdgeShiftBit;    ///< shift bit to prevent overflow
	UINT32 uiAngSlpFact;      ///< angle slope factor of edge calculation
} IVE_EDGE_FILT_PARAM;
//@}

/**
    IVE morphological filter parameters

    Structure of morphological filter parameters
*/
//@{
typedef struct _IVE_MORPH_FILT_PARAM_STRUCT {
	MORPH_OP uiMorphOp;       ///< operation of morphological filter
	BOOL *pbMorphNeighEn;    ///< Pointer of neighbor enable array
} IVE_MORPH_FILT_PARAM;
//@}

/**
    IVE IRV parameters

    Structure of IRV parameters
*/
//@{
typedef struct _IVE_IRV_PARAM_STRUCT {
	UINT8 en;
	HIST_MODE uiHistMode;        ///< selection of IRV type
	IRV_INVAL_VAL uiInvalidVal;      ///< shift bit to prevent overflow
	UINT32 uiThrS;            ///< threshold of histogram sum ratio
	UINT32 uiThrH;            ///< threshold of histogram frequency ratio
	UINT32 uiMednInvalTh;     ///< invalid threshold of median filter
} IVE_IRV_PARAM;
//@}

/**
    IVE morphological filter parameters

    Structure of morphological filter parameters
*/
//@{
typedef struct _IVE_THRES_LUT_PARAM_STRUCT {
	UINT32 *puiThresLut;            ///< pointer of threshold LUT values
} IVE_THRES_LUT_PARAM;
//@}

/**
    IVE flow control

    Structure of flow control
*/
//@{
typedef struct _IVE_FLOW_CT_PARAM_STRUCT {
	UINT32 dma_do_not_sync;            ///< do not cache flush
} IVE_FLOW_CT_PARAM;
//@}


/**
    IVE engine parameter for library

    Structure of all avaliable engine parameter
*/
//@{
typedef struct _IVE_PARAM_STRUCT {
	IVE_FUNC_EN FuncEn;             ///< Enable function of IVE
	MORPH_IN_SEL uiMorphInSel;      ///< selection of input data flow
	OUT_DATA_TYPE uiOutDataSel;     ///< selection of output data flow
	INTEGRAL_FMT_SEL uiIntegralFmt_Sel;///< selection of integral format
	IVE_DMA_IN_INFO DmaIn;          ///< Info of DMA frame mode input
	IVE_DMA_OUT_INFO DmaOut;        ///< Info of DMA frame mode output
	IVE_IMG_SIZE InSize;            ///< Input image size
	IVE_GEN_FILT_PARAM GenF;        ///< general filter parameters
	MEDN_MODE uiMednMode;           ///< median filter mode
	IVE_EDGE_FILT_PARAM EdgeF;      ///< edge filter parameters
	UINT32 uiEdgeMagTh;             ///< edge threshold for non-maximal suppression
	IVE_THRES_LUT_PARAM Thres;		///< threshold LUT
	IVE_MORPH_FILT_PARAM MorphF;    ///< morphological filter parameters
    IVE_DMA_LL_INFO LLDmaIn;        ///< Info of DMA frame mode input
	BURST_LEN_SEL InBurstSel;       ///< Input burst length selection
	BURST_LEN_SEL OutBurstSel;      ///< Output burst length selection
	IVE_DBG_PORT_SEL DbgPortSel;    ///< Debug port selection
	IVE_IRV_PARAM Irv;              ///< IRV parameter
	IVE_FLOW_CT_PARAM flowCT;       ///< flow control: Dma do not sync     
} IVE_PARAM;
//@}


//external function prototype

//engine control
extern ER ive_stateMachine(IVE_ACTION_OP RdeOp, BOOL bUpdate);
extern VOID ive_setSWReset(VOID);
extern ER ive_open(IVE_OPENOBJ *pIveObjCB);//Available clock rate(MHz): 240, 480, PLL6, PLL13
extern ER ive_start(VOID);
extern ER ive_setMode(IVE_PARAM *pIveParam);
extern ER ive_pause(VOID);
extern ER ive_close(VOID);
extern ER ive_waitFrameEnd(VOID);
extern ER ive_clearFrameEndFlag(VOID);
extern VOID ive_isr(VOID);
extern VOID ive_waitFrameEnd_LL(BOOL is_clr_flag);
extern VOID ive_clearFrameEndFlag_LL(VOID);
extern ER ive_ll_start(VOID);
extern ER ive_ll_setmode(UINT32 ll_addr);
extern ER ive_ll_pause(VOID);
//get settings
extern UINT32 ive_getClockRate(VOID);
extern UINT32 ive_getDmaInAddr(VOID);
extern UINT32 ive_getDmaOutAddr(VOID);

extern VOID ive_create_resource(VOID *parm, UINT32 clk_freq);
extern VOID ive_release_resource(VOID *parm);
extern VOID ive_set_base_addr(UINT32 addr);

extern VOID ive_setTriHdl(VOID *ive_trig_hdl);
extern VOID * ive_getTriHdl(VOID);
extern UINT32 ive_getTriHdl_flagId(VOID);

extern VOID ive_abort(VOID);
extern BOOL ive_wait_idle(VOID);
extern UINT32 ive_get_clock_rate(VOID);

extern ER ive_waitFrameEnd_timeout(UINT32 timeout_ms);
/*
 *#ifdef __cplusplus
 *}
 *#endif
 */

#endif

//@}
