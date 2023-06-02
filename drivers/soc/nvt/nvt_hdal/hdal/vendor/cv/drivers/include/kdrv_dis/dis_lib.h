/**
    Public header file for DIS module.

    @file       dis_lib.h
    @ingroup    mIDrvIPP_DIS

    @brief

    How to start:\n
    [Movie]\n
    1. dis_open\n
    2. dis_setMode\n
    3. dis_start\n


    How to stop:\n
    1. dis_pause\n
    2. dis_close\n

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/

#ifndef _DIS_LIB_H
#define _DIS_LIB_H

//#include "Type.h"
/**
    @addtogroup mIDrvIPP_DIS
*/
//@{

/*
    Define DIS interrput enable.
*/
//@{
#define DIS_INT_FRM                    0x00000001    ///< Issue once when a frame is done
#define DIS_INT_MDS                    0x00000002    ///< Issue once when a MDS is done
#define DIS_INT_BLK                    0x00000004    ///< Issue once when a block is done
#define DIS_INT_MDS_HBLK               0x00000008
#define DIS_INT_ALL                    0x0000000F   //0x00000007
//@}

#define BLKHMAX                     4
#define BLKVMAX                     64
#define MDSNUMMAX                   32
#define MVNUMMAX                    (BLKHMAX*BLKVMAX*MDSNUMMAX)

#define BINNUM                      64  ///< MV statistic histogram bin number
#define TBUFNUM                     8   ///< temporal filter buffer number
#define MVCNT_MIN                   10  ///< %
#define MVCNT_MAX                   70  ///< %

/**
    DIS load type selection

    Select dis load type
*/
typedef enum {
	DIS_FRMEND_LOAD = 0,     ///< DIS frame end load
	DIS_START_LOAD = 1,      ///< DIS start load
	ENUM_DUMMY4WORD(DIS_LOAD_TYPE)
} DIS_LOAD_TYPE;

/**
    DIS Process Event Selection.

    Select DIS process event. Event control is carried out by PhotoDisTsk
*/
//@{
/*typedef enum {
	DIS_PROC_RESTART = 0,   ///< process restart
	DIS_PROC_UPDATE  = 1,   ///< process update display information
	DIS_PROC_PAUSE   = 2,   ///< process pause
	DIS_PROC_NOOP    = 3,   ///< no operation
	ENUM_DUMMY4WORD(DIS_PROC_EVENT)
} DIS_PROC_EVENT;*/
//@}

/**
    DIS Score Level of Screening Motion Vectors.

    Select the level of DIS motion vectors.
*/
//@{
/*typedef enum {
	DIS_MVSCORE_HIGH    = 0, ///< high score level
	DIS_MVSCORE_NORMAL  = 1, ///< normal score level
	DIS_MVSCORE_LOW     = 2, ///< low score level
	DIS_MVSCORE_LOW2    = 3, ///< low score level 2
	ENUM_DUMMY4WORD(DIS_MVSCORE_LEVEL)
} DIS_MVSCORE_LEVEL;*/
//@}

/**
    DIS Stickiness Level of Compensation.

    Select the level of how sticky the compensation is.
*/
//@{
/*typedef enum {
	DIS_STICKY_TOP      = 0,  ///< top level of stickiness
	DIS_STICKY_HIGH     = 1,  ///< high level of stickiness
	DIS_STICKY_NORMAL   = 2,  ///< normal level of stickiness
	DIS_STICKY_LOW      = 3,  ///< low level of stickiness
	DIS_STICKY_LOW2     = 4,  ///< low level 2 of stickiness
	ENUM_DUMMY4WORD(DIS_STICKY_LEVEL)
} DIS_STICKY_LEVEL;*/
//@}

typedef enum _DIS_ACCESS_TYPE_ {
	DIS_ACCESS_SET = 0,
	DIS_ACCESS_GET = 1,
	DIS_ACCESS_MAX = 0xFF,
	ENUM_DUMMY4WORD(DIS_ACCESS_TYPE)
} DIS_ACCESS_TYPE;

/*
typedef enum _DIS_IMGSRC_PATH_
{
    DIS_IMGSRC_PATH_1 = 0,
    DIS_IMGSRC_PATH_2 = 1,
    DIS_IMGSRC_PATH_MAX
}DIS_IMGSRC_PATH;
*/

/**
    DIS Input Ping-Pong Buffer ID

    DIS Input ping-pong buffer ID.
*/
//@{
typedef enum {
	DIS_EDGE_BUF0 = 0, ///< input buffer 0
	DIS_EDGE_BUF1 = 1, ///< input buffer 1
	DIS_EDGE_BUF2 = 2, ///< input buffer 2
	ENUM_DUMMY4WORD(DIS_EDGE_BUFID)
} DIS_EDGE_BUFID;
//@}

/**
    DIS Output Ping-Pong Buffer ID

    DIS output ping-pong buffer ID.
*/
//@{
typedef enum {
	DIS_MOTION_BUF0 = 0, ///< output buffer 0
	DIS_MOTION_BUF1 = 1, ///< output buffer 1
	ENUM_DUMMY4WORD(DIS_MOTION_BUFID)
} DIS_MOTION_BUFID;
//@}

/**
    DIS Search range Selection.

    Select DIS search range in motion estimation.
*/
//@{
typedef enum {
	DIS_SR_32 = 0,    ///< search range from -32 to 32
	DIS_SR_16 = 1,    ///< search range from -16 to 16
	ENUM_DUMMY4WORD(DIS_SR)
} DIS_SR;
//@}
/**
   DIS Block Size Selection
*/
typedef enum {
	DIS_BLKSZ_64x48 = 0,  ///< positive LUT -> NOTE: not support in 98560
	DIS_BLKSZ_32x32 = 1,  ///< linear LUT
	ENUM_DUMMY4WORD(DIS_BLKSZ)
} DIS_BLKSZ;

/**
   DIS LUT Selection
*/
typedef enum {
	DIS_LUT_POS = 0,  ///< positive LUT
	DIS_LUT_LIN = 1,  ///< linear LUT
	ENUM_DUMMY4WORD(DIS_LUT)
} DIS_LUT;

/// jsliu@201014
/**
   DIS DMA disable
*/
typedef enum {
	DIS_DMA_DISABLE = 0,
	DIS_DMA_ENABLE = 1,
	ENUM_DUMMY4WORD(DIS_ABORT)
} DIS_ABORT;

#if 0
/**
    DIS Sub-in Selection.

    Select DIS subsample input.
*/
//@{
typedef enum {
	DIS_SUBIN_1X = 0,    ///< no input subsample
	DIS_SUBIN_2X = 1,    ///< input subsample 2x
	DIS_SUBIN_4X = 2,    ///< input subsample 4x
	DIS_SUBIN_8X = 3,    ///< input subsample 8x
	ENUM_DUMMY4WORD(DIS_SUBIN)
} DIS_SUBIN;
//@}
#endif

/**
    DIS Open Object.

    Open Object is used for opening DIS module.
    \n Used for dis_open()

*/
//@{
typedef struct _DIS_OPENOBJ {
	void (*FP_DISISR_CB)(UINT32 uiIntStatus); ///< isr callback function
	UINT32 uiDisClockSel; ///< support 220/192/160/120/80/60/48 Mhz
} DIS_OPENOBJ;
//@}

/**
    DIS Motion Information.

    Structure of information in a motion vector.
*/
//@{
typedef struct _MOTION_INFOR {
	INT32   iX;         ///< x component
	INT32   iY;         ///< y component
	UINT32  uiSad;      ///< sum of absolute difference
	UINT32  uiCnt;      ///< number of edge pixels (src block)
	UINT32  uiIdx;      ///< block index
	BOOL    bValid;     ///< reliability of MV
} MOTION_INFOR;
//@}
/**
   Motion Estimation Thresholds
*/
typedef struct _DIS_EST_THRHS {
	INT32 uiScrOffs;     ///< score offset
	INT32 uiScrThrh;     ///< score threshold
	INT32 uiCenScrThrh;  ///< center score threshold
	INT32 uiFeaCntThrh;  ///< feature# threshold
} DIS_EST_THRHS;

/**
   Motion Estimation Credits
*/
typedef struct _DIS_EST_CREDITS {
	INT32 uiCredit0;  ///<
	INT32 uiCredit1;  ///<
	INT32 uiCredit2;  ///<
} DIS_EST_CREDITS;

#if 0
/**
    Motion in tracking.

    Structure of information of a moving object.
*/
//@{
typedef struct _TRACK_MOTION_INFOR {
	INT32   iTrkX;         ///< x component
	INT32   iTrkY;         ///< y component
	UINT32  uiPosX;        ///< left
	UINT32  uiPosY;        ///< top
	BOOL    bMovObj;       ///< if it's a moving object
} TRACK_MOTION_INFOR;
//@}
#endif

/**
    DIS Compensation Information

    Structure of compensation parameters.
*/
//@{
/*typedef struct _COMPENSATION_INFO {
	UINT32 uiBoundH;    ///< horizontal boundary
	UINT32 uiBoundV;    ///< vertical boundary
	UINT8  uiDZoomRate; ///< dzoom rate
} COMPENSATION_INFO;*/
//@}

/**
    DIS Entire Configuration

    Structure of DIS module parameters when one wants to configure this module.
*/
//@{
typedef struct _DIS_PARAM_STRUCT {
	//DIS_ENGINE_OPERATION DisOperation;  ///< DIS engine operation
	UINT32 uiWidth;                     ///< input horizontal size
	UINT32 uiHeight;                    ///< input vertical size
	UINT32 uiInOfs;                     ///< input line offset
	UINT32 uiInAdd0;                    ///< input starting address 0
	UINT32 uiInAdd1;                    ///< input starting address 1
	UINT32 uiInAdd2;                    ///< input starting address 2
	UINT32 uiOutAdd0;                   ///< output starting address 0
	UINT32 uiOutAdd1;                   ///< output starting address 1
	UINT32 uiIntEn;                     ///< interrupt enable switch
} DIS_PARAM;
//@}

/**
    Struct DIS MDS dimension.

    Structure of dimension elements in a motion detecting section.
*/
//@{
typedef struct _MDS_DIM {
	UINT8 uiBlkNumH;    ///< horizontal block number in use
	UINT8 uiBlkNumV;    ///< vertical block number in use
	UINT8 uiMdsNum;     ///< total MDS number in use
} MDS_DIM;
//@}

/**
    Struct DIS Ping-Pong Buffers

    Structure of DIS ping-pong buffer parameters.
*/
//@{
typedef struct DIS_PPB_STRUCT {
	BOOL  bPpbiEn;      ///< enable input ppbs
	UINT8 uiPpbiIni;    ///< intial buffer ID of input ppbs
	BOOL  bPpboEn;      ///< enable output ppbs
	UINT8 uiPpboIni;    ///< intial buffer ID of output ppbs
} DIS_PPB;
//@}

/**
    Struct DIS Motion Estimation.

    Structure of motion estimation parameters.
*/
//@{
typedef struct _MOTION_EST {
	DIS_SR          SearchRagne;   ///< search range
	DIS_BLKSZ       BlockSize;     ///< MDS block size
	DIS_LUT         Lut;           ///< estimation LUT
	DIS_EST_THRHS   Thrshs;        ///<
	DIS_EST_CREDITS Credits;       ///<
///    UINT8   uiSrc2RefX;     ///< x shift between source and reference blocks
///    UINT8   uiSrc2RefY;     ///< y shift between source and reference blocks
} MOTION_EST;
//@}

/**
    DIS Motion Vector.

    Structure of directional motion vector elements.
*/
//@{
typedef struct _MOTION_VECTOR {
	INT32 iX; ///< x component
	INT32 iY; ///< y component
} MOTION_VECTOR;
//@}

/**
    Struct DIS Register Setting

    Structure of all DIS register settings.
*/
//@{
typedef struct STR_DIS_REG {
	DIS_PPB         PpbInfo;    ///< input/output ppbs
	MOTION_EST      MotSet;     ///< motion estimation
	MDS_DIM         MdsDim;     ///< MDS dimension
	MOTION_VECTOR   MdsAddr[MDSNUMMAX];///< MDS starting addresses
	MOTION_VECTOR   MdsOfs[MDSNUMMAX]; ///< MDS Offset
} STR_DIS_REG;
//@}

typedef struct _MOTION_VECTOR_QUEUE {
	UINT32 uiTotalXCnt;
	UINT32 uiZeroXCnt;
} MOTION_VECTOR_QUEUE;

typedef struct DIS_SGM_STRUCT {
	MOTION_INFOR *pCurMv; ///< current motion vector
	MOTION_INFOR *pTmpMv; ///< temporal motion vector
	MOTION_INFOR *pBlkMv; ///< block motion vectors
	MOTION_INFOR *pPreMv; ///< previous motion vectors
	MOTION_VECTOR_QUEUE *pLastMotVectQue;
} DIS_SGM;


// basic function
extern ER       dis_open(DIS_OPENOBJ *pObjCB);
extern BOOL     dis_isOpened(VOID);
extern ER       dis_close(VOID);
extern ER       dis_start(VOID);
extern ER       dis_pause(VOID);
extern ER       dis_setMode(DIS_PARAM *pDisAddrSize);
extern ER       dis_changeSize(DIS_PARAM *pDisAddrSize);
extern BOOL     dis_isEnabled(VOID);
extern ER       dis_clrFrameEnd(VOID);
extern VOID     dis_waitFrameEnd(BOOL bClrFlag);
// interrupt
extern ER       dis_changeInterrupt(UINT32 uiIntEn);
extern UINT32   dis_getIntEnable(VOID);
extern UINT32   dis_getIntrStatus(VOID);
extern VOID     dis_clrIntrStatus(UINT32 uiStatus);
// motion information
extern VOID     dis_setFrameCnt(UINT16 uiCnt);
extern VOID     dis_getMotionVectors(MOTION_INFOR *pMotResult);
// get status
extern MDS_DIM  dis_getMDSDim(VOID);
extern UINT16   dis_getFrameCnt(VOID);
extern UINT32   dis_getClockRate(VOID);
extern UINT32   dis_getDmaOutAddr(DIS_MOTION_BUFID BufId);
extern UINT32   dis_getDmaInAddr(DIS_EDGE_BUFID BufId);
// engine suspending
extern VOID     dis_clr(BOOL bReset);
extern VOID     dis_set_dma_disable(BOOL disable);
extern BOOL     dis_get_dma_disable(VOID);
extern BOOL     dis_get_engine_idle(VOID);
//
extern VOID dis_create_resource(VOID);
extern VOID dis_release_resource(VOID);
extern VOID dis_setBaseAddr(UINT32 uiAddr);

//@}
#endif
