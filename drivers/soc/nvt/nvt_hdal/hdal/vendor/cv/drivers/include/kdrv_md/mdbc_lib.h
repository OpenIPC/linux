/**
    Public header file for MDBC module.

    @file       mdbc_lib.h
    @ingroup    mIDrvIPP_MDBC

    @brief

    How to start:\n
    [Movie]\n
    1. mdbc_open\n
    2. mdbc_setMode\n
    3. mdbc_start\n


    How to stop:\n
    1. mdbc_pause\n
    2. mdbc_close\n

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _MDBC_LIB_H
#define _MDBC_LIB_H

#ifdef __KERNEL__

#include "kwrap/type.h"

#else

#include <kwrap/nvt_type.h>

#endif
/**
    @addtogroup mIDrvIPP_MDBC
*/
//@{

/*
    Define MDBC interrput enable.
*/
//@{
#define MDBC_INTE_FRM                    0x00000001    ///< Issue once when a frame is done
#define MDBC_INTE_LLEND                  0x00000010
#define MDBC_INTE_LLERROR                0x00000020
#define MDBC_INTE_ALL                    0x00000031
//@}

/*
    Define MDBC interrput status.
*/
//@{
#define MDBC_INT_FRM                    0x00000001    ///< Issue once when a frame is done
#define MDBC_INT_LLEND                  0x00000010
#define MDBC_INT_LLERROR                0x00000020
#define MDBC_INT_ALL                    0x00000031
//@}

// Structure definition
/**
    MDBC Engine Operation Selection.

    Select MDBC module operation.
*/
//@{
typedef enum {
	INIT              = 0,   ///< init mode
	NORM              = 1,   ///< normal mode
	ENUM_DUMMY4WORD(MDBC_MODE)
} MDBC_MODE;
//@}

/**
    MDBC Morphological process Sel.

    Select MDBC Morphological process type.
*/
//@{
typedef enum {
	Median         = 0,   ///< Median
	Dilate         = 1,   ///< Dilate
	Erode          = 2,   ///< Erode
	Bypass         = 3,   ///< Bypass
	ENUM_DUMMY4WORD(MDBC_MOR_SEL)
} MDBC_MOR_SEL;
//@}

/**
    MDBC Open Object.

    Open Object is used for opening MDBC module.
    \n Used for mdbc_open()

*/
//@{
typedef struct _MDBC_OPENOBJ {
	void (*FP_MDBCISR_CB)(UINT32 uiIntStatus); ///< isr callback function
	UINT32 uiMdbcClockSel;                     ///< MDBC clock selection
} MDBC_OPENOBJ;
//@}

/**
    MDBC Input Info.
*/
//@{
typedef struct {
	UINT32      update_nei_en;  ///< update neighbor enable
	UINT32      deghost_en;     ///< deghost enable
	UINT32      roi_en0;        ///< roi0 enable
	UINT32      roi_en1;        ///< roi1 enable
	UINT32      roi_en2;        ///< roi2 enable
	UINT32      roi_en3;        ///< roi3 enable
	UINT32      roi_en4;        ///< roi4 enable
	UINT32      roi_en5;        ///< roi5 enable
	UINT32      roi_en6;        ///< roi6 enable
	UINT32      roi_en7;        ///< roi7 enable
	UINT32      chksum_en;      ///< check sum enable
	UINT32      bgmw_save_bw_en;///< bandwidth saving enable
	UINT32      bc_y_only_en;
} MDBC_CONTROL_EN;
//@}

/**
    MDBC Input Info.
*/
//@{
typedef struct {
	UINT32      uiInAddr0;   ///< Y address
	UINT32      uiInAddr1;   ///< UV address
	UINT32      uiInAddr2;   ///< last UV address
	UINT32      uiInAddr3;   ///< bgmodel address
	UINT32      uiInAddr4;   ///< var1 address
	UINT32      uiInAddr5;   ///< var2 address
	UINT32      uiLofs0;     ///< Y line-offset
	UINT32      uiLofs1;     ///< UV line-offset
} MDBC_INPUT_INFO;
//@}

/**
    MDBC Output Info.
*/
//@{
typedef struct {
	UINT32      uiOutAddr0;  ///< foreground address
	UINT32      uiOutAddr1;  ///< bgmodel address
	UINT32      uiOutAddr2;  ///< var1 address
	UINT32      uiOutAddr3;  ///< var2 address
} MDBC_OUTPUT_INFO;
//@}

/**
    MDBC Model Match Parameters.
*/
//@{
typedef struct _MDBC_MDMATCH_PARAM {
	UINT32 lbsp_th;                         ///< Lbsp threshold
	UINT32 d_colour;                        ///< D colour
	UINT32 r_colour;                        ///< R colour
	UINT32 d_lbsp;                          ///< D lbsp
	UINT32 r_lbsp;                          ///< R lbsp
	UINT32 model_num;                       ///< bg model number
	UINT32 t_alpha;                         ///< T_ALPHA
	UINT32 dw_shift;                        ///< DE_SHIFT
	UINT32 dlast_alpha;                     ///< D_LAST_ALPHA
	UINT32 min_match;                       ///< min match
	UINT32 dlt_alpha;                       ///< DLT_ALPHA
	UINT32 dst_alpha;                       ///< DST_ALPHA
	UINT32 uv_thres;                        ///< UV threshold
	UINT32 s_alpha;                         ///< S_ALPHA
	UINT32 dbg_lumDiff;                     ///< debug Lum Diff
	UINT32 dbg_lumDiff_en;                  ///< debug Lum Diff enable
} MDBC_MDMATCH_PARAM;
//@}

/**
    MDBC Morphological process Parameters.
*/
//@{
typedef struct _MDBC_MOR_PARAM {
	//UINT32 th_ero;                          ///< erode threshold
	//UINT32 th_dil;                          ///< dilate threshold
	UINT32 mor_th0;                         ///< mor0 threshold
	UINT32 mor_th1;                         ///< mor1 threshold
	UINT32 mor_th2;                         ///< mor2 threshold
	UINT32 mor_th3;                         ///< mor3 threshold
	UINT32 mor_th_dil;                      ///< mor dil threshold
    UINT32 mor_sel0;                        ///< Morphological type
    UINT32 mor_sel1;                        ///< Morphological type
    UINT32 mor_sel2;                        ///< Morphological type
    UINT32 mor_sel3;                        ///< Morphological type
} MDBC_MOR_PARAM;
//@}

/**
    MDBC Update Parameters.
*/
//@{
typedef struct _MDBC_UPD_PARAM {
	UINT32 minT;                      ///< Update min T
	UINT32 maxT;                      ///< Update max T
	UINT32 maxFgFrm;                  ///< Update T when Max foreground frame
	UINT32 deghost_dth;               ///< Deghost threshold for Dlast
	UINT32 deghost_sth;               ///< Deghost threshold for S1
	UINT32 stable_frm;                ///< stable frame
	UINT32 update_dyn;                ///< update dyn
	UINT32 va_distth;                 ///< Va distance threshold
	UINT32 t_distth;                  ///< T distance threshold
	UINT32 dbg_frmID;                 ///< debug frame ID
	UINT32 dbg_frmID_en;              ///< debug frame ID enable
	UINT32 dbg_rnd;                   ///< debug random
	UINT32 dbg_rnd_en;                ///< debug random enable
} MDBC_UPD_PARAM;
//@}

/**
    MDBC ROI Parameters.
*/
//@{
typedef struct _MDBC_ROI_PARAM {
	UINT32 roi_x;                     ///< roi x position
	UINT32 roi_y;                     ///< roi y position
	UINT32 roi_w;                     ///< roi width
	UINT32 roi_h;                     ///< roi height
	UINT32 roi_uv_thres;              ///< roi UV threshold
	UINT32 roi_lbsp_th;               ///< roi Lbsp threshold
	UINT32 roi_d_colour;              ///< roi D colour
	UINT32 roi_r_colour;              ///< roi R colour
	UINT32 roi_d_lbsp;                ///< roi D lbsp
	UINT32 roi_r_lbsp;                ///< roi R lbsp
	UINT32 roi_morph_en;              ///< roi morphological process enable
	UINT32 roi_minT;                  ///< Update roi min T
	UINT32 roi_maxT;                  ///< Update roi max T
} MDBC_ROI_PARAM;
//@}

/**
    MDBC structure - MDBC input size info.
*/
//@{
typedef struct {
	UINT32 uiMdbcWidth;              ///< MDBC input width
	UINT32 uiMdbcHeight;             ///< MDBC input Height
} MDBC_IN_SIZE;
//@}

/**
    MDBC All Parameters.

    This is used for MDBC setmode.

*/
//@{
typedef struct _MDBC_PARAM {
	MDBC_MODE           mode;          ///< MDBC operation mode
	MDBC_CONTROL_EN     controlEn;     ///< MDBC control enable
	UINT32              uiIntEn;       ///< MDBC Interrupt enable
    MDBC_INPUT_INFO     InInfo;        ///< MDBC input address & lofs
    MDBC_OUTPUT_INFO    OutInfo;       ///< MDBC output address
	UINT32      		uiLLAddr;      ///< LinkedList address
	MDBC_IN_SIZE        Size;          ///< MDBC size
    MDBC_MDMATCH_PARAM  MdmatchPara;   ///< MDBC Model Match
    MDBC_MOR_PARAM      MorPara;       ///< MDBC Morphological process
    MDBC_UPD_PARAM      UpdPara;       ///< MDBC Update
    MDBC_ROI_PARAM      ROIPara0;      ///< MDBC ROI0
    MDBC_ROI_PARAM      ROIPara1;      ///< MDBC ROI1
    MDBC_ROI_PARAM      ROIPara2;      ///< MDBC ROI2
    MDBC_ROI_PARAM      ROIPara3;      ///< MDBC ROI3
    MDBC_ROI_PARAM      ROIPara4;      ///< MDBC ROI4
    MDBC_ROI_PARAM      ROIPara5;      ///< MDBC ROI5
    MDBC_ROI_PARAM      ROIPara6;      ///< MDBC ROI6
    MDBC_ROI_PARAM      ROIPara7;      ///< MDBC ROI7
} MDBC_PARAM;
//@}




//
extern ER mdbc_open(MDBC_OPENOBJ *pObjCB);
extern BOOL mdbc_isOpened(void);
extern ER mdbc_close(void);
extern ER mdbc_pause(void);
extern ER mdbc_start(void);
extern void mdbc_clrFrameEnd(void);
extern void mdbc_waitFrameEnd(BOOL bClrFlag);
extern void mdbc_clrIntrStatus(UINT32 uiStatus);
extern ER mdbc_setMode(MDBC_PARAM *pMdbcPara);
extern UINT32 mdbc_getLumDiff(void);
extern UINT32 mdbc_getFrmID(void);
extern UINT32 mdbc_getRND(void);
extern VOID   mdbc_set_dma_disable(BOOL disable);
extern BOOL   mdbc_get_dma_disable(VOID);
extern BOOL   mdbc_get_engine_idle(VOID);
extern ER mdbc_ll_pause(void);
extern ER mdbc_ll_start(void);
extern void mdbc_clr_ll_frameend(void);
extern void mdbc_wait_ll_frameend(BOOL is_clr_flag);
extern ER mdbc_ll_setmode(UINT32 ll_addr);
extern UINT64 mdbc_ll_null_cmd(UINT8 tbl_idx);
extern UINT64 mdbc_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val);
extern UINT64 mdbc_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx);
extern UINT64 mdbc_ll_nextupd_cmd(UINT32 addr);
//
extern UINT32 mdbc_getIntEnable(void);
extern UINT32 mdbc_getIntrStatus(void);
extern UINT32 mdbc_getClockRate(void);
/*
#ifdef __KERNEL__
extern VOID mdbc_setBaseAddr(UINT32 uiAddr);
extern VOID mdbc_create_resource(VOID);
extern VOID mdbc_release_resource(VOID);
#endif
*/
//@}
#endif