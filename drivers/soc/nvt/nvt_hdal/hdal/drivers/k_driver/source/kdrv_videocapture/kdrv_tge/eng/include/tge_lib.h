/*
    Public header file for RDE module.

    This file is the header file that define the API and data type for TGE
    module.

    @file       tge_lib.h
    @ingroup    mIDrvIPPTGE
    @note

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _TGE_LIB_H
#define _TGE_LIB_H

#include "kwrap/type.h"
#include "kdrv_videocapture/kdrv_tge.h"

/**
    @addtogroup mIIPPTGE
*/
//@{

/**
    @name Define TGE function.
*/
//@{
//@}



/**
    @name Define TGE interrput.
*/
//@{
/*
#define TGE_INT_VD                  0x00000001 ///< Enable interrupt
#define TGE_INT_BP1                 0x00000002 ///< Enable interrupt
#define TGE_INT_BP2                 0x00000004 ///< Enable interrupt
#define TGE_INT_FLSH_TRG            0x00000008 ///< Enable interrupt
#define TGE_INT_MSHA_CLOSE_TRG      0x00000010 ///< Enable interrupt
#define TGE_INT_MSHA_OPEN_TRG       0x00000020 ///< Enable interrupt
#define TGE_INT_MSHB_CLOSE_TRG      0x00000040 ///< Enable interrupt
#define TGE_INT_MSHB_OPEN_TRG       0x00000080 ///< Enable interrupt
#define TGE_INT_FLSH_END            0x00000100 ///< Enable interrupt
#define TGE_INT_MSHA_CLOSE_END      0x00000200 ///< Enable interrupt
#define TGE_INT_MSHA_OPEN_END       0x00000400 ///< Enable interrupt
#define TGE_INT_MSHB_CLOSE_END      0x00000800 ///< Enable interrupt
#define TGE_INT_MSHB_OPEN_END       0x00001000 ///< Enable interrupt
#define TGE_INT_ALL                 0xffffffff ///< Enable interrupt
*/
#define TGE_INT_VD                  0x00000001 ///< Enable interrupt
#define TGE_INT_VD2                 0x00000002 ///< Enable interrupt
#define TGE_INT_VD3                 0x00000004 ///< Enable interrupt
#define TGE_INT_VD4                 0x00000008 ///< Enable interrupt
#define TGE_INT_VD5                 0x00000010 ///< Enable interrupt
#define TGE_INT_VD6                 0x00000020 ///< Enable interrupt
#define TGE_INT_VD7                 0x00000040 ///< Enable interrupt
#define TGE_INT_VD8                 0x00000080 ///< Enable interrupt
#define TGE_INT_VD_BP               0x00000100 ///< Enable interrupt
#define TGE_INT_VD2_BP              0x00000200 ///< Enable interrupt
#define TGE_INT_VD3_BP              0x00000400 ///< Enable interrupt
#define TGE_INT_VD4_BP              0x00000800 ///< Enable interrupt
#define TGE_INT_VD5_BP              0x00001000 ///< Enable interrupt
#define TGE_INT_VD6_BP              0x00002000 ///< Enable interrupt
#define TGE_INT_VD7_BP              0x00004000 ///< Enable interrupt
#define TGE_INT_VD8_BP              0x00008000 ///< Enable interrupt
#define TGE_INT_FLSH_TRG            0x00010000 ///< Enable interrupt
#define TGE_INT_MSHA_CLOSE_TRG      0x00020000 ///< Enable interrupt
#define TGE_INT_MSHA_OPEN_TRG       0x00040000 ///< Enable interrupt
#define TGE_INT_MSHB_CLOSE_TRG      0x00080000 ///< Enable interrupt
#define TGE_INT_MSHB_OPEN_TRG       0x00100000 ///< Enable interrupt
#define TGE_INT_FLSH_END            0x00200000 ///< Enable interrupt
#define TGE_INT_MSHA_CLOSE_END      0x00400000 ///< Enable interrupt
#define TGE_INT_MSHA_OPEN_END       0x00800000 ///< Enable interrupt
#define TGE_INT_MSHB_CLOSE_END      0x01000000 ///< Enable interrupt
#define TGE_INT_MSHB_OPEN_END       0x02000000 ///< Enable interrupt
#define TGE_INT_ALL                 0xffffffff ///< Enable interrupt
//@}



/**
TGE Enumeration

@name   TGE_Enumeration
*/
//@{

typedef enum {
	MODE_MASTER         = 0,   ///<
	MODE_SLAVE_TO_PAD,         ///<
	MODE_SLAVE_TO_CSI,         ///<
	MODE_SLAVE_TO_SLVSEC,      ///<
	ENUM_DUMMY4WORD(TGE_MODE_SEL)
} TGE_MODE_SEL;

typedef enum {
	FLSH_MSH_FROM_VD   = 0,
	FLSH_MSH_FROM_VD2,
	FLSH_MSH_FROM_VD3,
	FLSH_MSH_FROM_VD4,
	ENUM_DUMMY4WORD(TGE_FLSH_MSH_VD_IN_SEL)
} TGE_FLSH_MSH_VD_IN_SEL;

typedef enum {
	TGE_PHASE_RISING  = 0,  ///< rising edge latch/trigger
	TGE_PHASE_FALLING,      ///< falling edge latch/trigger
	ENUM_DUMMY4WORD(TGE_DATA_PHASE_SEL)
} TGE_DATA_PHASE_SEL;

typedef enum {
	WAVEFORM_PULSE  = 0,    ///<
	WAVEFORM_LEVEL,         ///<
	ENUM_DUMMY4WORD(TGE_MSH_WAVEFORM_SEL)
} TGE_MSH_WAVEFORM_SEL;

typedef enum {
	TRG_IMD          = 0,   ///<
	TRG_WAIT_VD,            ///<
	TRG_WAIT_EXT_TRG,       ///<
	ENUM_DUMMY4WORD(TGE_CTRL_PIN_TRG_SEL)
} TGE_CTRL_PIN_TRG_SEL;


typedef enum {
	TGE_VDTOSIE_SRC_VD  = 0,
	TGE_VDTOSIE_SRC_VD3,
	ENUM_DUMMY4WORD(TGE_VDTOSIE_SEL)
} TGE_VDTOSIE_SEL;

typedef enum {
	TGE_VDTOSIE3_SRC_VD5 = 0,
	TGE_VDTOSIE3_SRC_VD7,
	ENUM_DUMMY4WORD(TGE_VDTOSIE3_SEL)
} TGE_VDTOSIE3_SEL;

typedef enum {
	TGE_CHGRST_VD_RST           = 0x00000001,
	TGE_CHGRST_VD2_RST          = 0x00000002,
	TGE_CHGRST_VD3_RST          = 0x00000004,
	TGE_CHGRST_VD4_RST          = 0x00000008,
	TGE_CHGRST_VD5_RST          = 0x00000010,
	TGE_CHGRST_VD6_RST          = 0x00000020,
	TGE_CHGRST_VD7_RST          = 0x00000040,
	TGE_CHGRST_VD8_RST          = 0x00000080,
} TGE_CHANGE_RST_SEL;

typedef enum {
	TGE_CHG_VDINFO           = 0x00000001,
	TGE_CHG_VD2INFO          = 0x00000002,
	TGE_CHG_VD3INFO          = 0x00000004,
	TGE_CHG_VD4INFO          = 0x00000008,
	TGE_CHG_VD5INFO          = 0x00000010,
	TGE_CHG_VD6INFO          = 0x00000020,
	TGE_CHG_VD7INFO          = 0x00000040,
	TGE_CHG_VD8INFO          = 0x00000080,
} TGE_CHANGE_VDHD_SEL;

typedef enum {
	TGE_CHG_VD_RST          =  0,  ///< VD   reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD2_RST,               ///< VD 2 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD3_RST,               ///< VD 3 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD4_RST,               ///< VD 4 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD5_RST,               ///< VD 5 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD6_RST,               ///< VD 6 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD7_RST,               ///< VD 7 reset                  , please refer to "TGE_VD_RST_INFO"
	TGE_CHG_VD8_RST,               ///< VD 8 reset                  , please refer to "TGE_VD_RST_INFO"

	TGE_CHG_VD_PHASE,              ///< VD   Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD2_PHASE,             ///< VD 2 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD3_PHASE,             ///< VD 3 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD4_PHASE,             ///< VD 4 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD5_PHASE,             ///< VD 5 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD6_PHASE,             ///< VD 6 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD7_PHASE,             ///< VD 7 Phase                  , please refer to "TGE_VD_PHASE"
	TGE_CHG_VD8_PHASE,             ///< VD 8 Phase                  , please refer to "TGE_VD_PHASE"


	TGE_CHG_VD_INV,                ///< VD   Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD2_INV,               ///< VD 2 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD3_INV,               ///< VD 3 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD4_INV,               ///< VD 4 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD5_INV,               ///< VD 5 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD6_INV,               ///< VD 6 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD7_INV,               ///< VD 7 Inv                    , please refer to "TGE_VD_INV"
	TGE_CHG_VD8_INV,               ///< VD 8 Inv                    , please refer to "TGE_VD_INV"


	TGE_CHG_VD_MODE,               ///< VD   Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD2_MODE,              ///< VD 2 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD3_MODE,              ///< VD 3 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD4_MODE,              ///< VD 4 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD5_MODE,              ///< VD 5 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD6_MODE,              ///< VD 6 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD7_MODE,              ///< VD 7 Model Sel              , please refer to "TGE_MODE_SEL"
	TGE_CHG_VD8_MODE,              ///< VD 8 Model Sel              , please refer to "TGE_MODE_SEL"

	TGE_CHG_VD2SIE1,               ///< TGE VD to SIE1              , please refer to "TGE_VDTOSIE_SEL"
	TGE_CHG_VD2SIE3,               ///< TGE VD to SIE3              , please refer to "TGE_VDTOSIE3_SEL"
	TGE_CHG_FLSHMSHVDSEL,          ///< VD source sel for Flash/Msh , please refer to "TGE_FLSH_MSH_VD_IN_SEL"
	TGE_CHG_OUTSWAP,               ///< VD output swap              , please refer to "TGE_OUT_SWAP_INFO"

	TGE_CHG_VDHD,                  ///< change VD/HD                , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD2HD2,                ///< change VD2/HD2              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD3HD3,                ///< change VD3/HD3              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD4HD4,                ///< change VD4/HD4              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD5HD5,                ///< change VD5/HD5              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD6HD6,                ///< change VD6/HD6              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD7HD7,                ///< change VD7/HD7              , please refer to "TGE_VDHD_INFO"
	TGE_CHG_VD8HD8,                ///< change VD8/HD8              , please refer to "TGE_VDHD_INFO"

	TGE_CHG_TIMING_VD_PAUSE,       ///< pause timing(VD/HD)         , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD2_PAUSE,      ///< pause timing(VD2/HD2)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD3_PAUSE,      ///< pause timing(VD3/HD3)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD4_PAUSE,      ///< pause timing(VD4/HD4)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD5_PAUSE,      ///< pause timing(VD5/HD5)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD6_PAUSE,      ///< pause timing(VD6/HD6)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD7_PAUSE,      ///< pause timing(VD7/HD7)       , please refer to "TGE_TIMING_PAUSE_INFO"
	TGE_CHG_TIMING_VD8_PAUSE,      ///< pause timing(VD8/HD8)       , please refer to "TGE_TIMING_PAUSE_INFO"

	TGE_CHG_VD_BP,                 ///< CHANGE VD BP                , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD2_BP,                ///< CHANGE VD2 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD3_BP,                ///< CHANGE VD3 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD4_BP,                ///< CHANGE VD4 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD5_BP,                ///< CHANGE VD5 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD6_BP,                ///< CHANGE VD6 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD7_BP,                ///< CHANGE VD7 BP               , please refer to "TGE_BREAKPOINT_INFO"
	TGE_CHG_VD8_BP,                ///< CHANGE VD8 BP               , please refer to "TGE_BREAKPOINT_INFO"

	TGE_CHG_FLSH_CTRL_INV,         ///< Flash inv
	TGE_CHG_FLSH_EXT_INV,          ///< Flash external-trig inv
	TGE_CHG_MSHA_CTRL_INV,         ///< MshA inv
	TGE_CHG_MSHB_CTRL_INV,         ///< MshB inv
	TGE_CHG_MSH_EXT_INV,           ///< Msh external-trig inv
	TGE_CHG_MSHA_SWAP,             ///< MshA two wires swap
	TGE_CHG_MSHB_SWAP,             ///< MshB two wires swap
	TGE_CHG_MSHA_CTRL_MODE,        ///< MshA control mode
	TGE_CHG_MSHB_CTRL_MODE,        ///< MshB control mode

	TGE_CHG_FLSH_TRIG,             ///< trigger Flash                       , please refer to "TGE_CTRL_PIN_TRG_SEL"
	TGE_CHG_MSHA_CLOSE_TRIG,       ///< trigger M-shtr-A close pin          , please refer to "TGE_CTRL_PIN_TRG_SEL"
	TGE_CHG_MSHA_OPEN_TRIG,        ///< trigger M-shtr-A open  pin          , please refer to "TGE_CTRL_PIN_TRG_SEL"
	TGE_CHG_MSHB_CLOSE_TRIG,       ///< trigger M-shtr-B close pin          , please refer to "TGE_CTRL_PIN_TRG_SEL"
	TGE_CHG_MSHB_OPEN_TRIG,        ///< trigger M-shtr-B open  pin          , please refer to "TGE_CTRL_PIN_TRG_SEL"

	TGE_CHG_FLSH_CTRL,             ///< change  Flash setting               , please refer to "TGE_CTRL_PIN_INFO"
	TGE_CHG_MSHA_CLOSE_CTRL,       ///< change  M-shtr-A close pin setting  , please refer to "TGE_CTRL_PIN_INFO"
	TGE_CHG_MSHA_OPEN_CTRL,        ///< change  M-shtr-A open  pin setting  , please refer to "TGE_CTRL_PIN_INFO"
	TGE_CHG_MSHB_CLOSE_CTRL,       ///< change  M-shtr-B close pin setting  , please refer to "TGE_CTRL_PIN_INFO"
	TGE_CHG_MSHB_OPEN_CTRL,        ///< change  M-shtr-B open  pin setting  , please refer to "TGE_CTRL_PIN_INFO"

    TGE_CHG_CLK1,                  ///< change TgeClkSrc1
    TGE_CHG_CLK2,                  ///< change TgeClkSrc2

	TGE_CHG_IDX_NUM,               ///<

	ENUM_DUMMY4WORD(TGE_CHANGE_FUN_PARAM_SEL)
} TGE_CHANGE_FUN_PARAM_SEL;

typedef enum {
	CLKSRC_PXCLK = 0, ///< TGE clock source as
	CLKSRC_MCLK,      ///< TGE clock source as, this is used only for VD1/HD1
	CLKSRC_MCLK2,     ///< TGE clock source as, this is used only for VD2/HD2
} TGE_CLKSRC_SEL;

typedef enum {
	TGE_CLK_1    = 0,
	TGE_CLK_2,
	ENUM_DUMMY4WORD(TGE_CLK_ID)
} TGE_CLK_ID;

#if 0 // move to kdrv_tge.h
typedef enum {
	TGE_WAIT_VD         =  0,  ///<
	TGE_WAIT_VD2,
	TGE_WAIT_VD3,
	TGE_WAIT_VD4,
	TGE_WAIT_VD5,
	TGE_WAIT_VD6,
	TGE_WAIT_VD7,
	TGE_WAIT_VD8,
	TGE_WAIT_VD_BP1,           ///<
	TGE_WAIT_VD2_BP1,          ///<
	TGE_WAIT_VD3_BP1,          ///<
	TGE_WAIT_VD4_BP1,          ///<
	TGE_WAIT_VD5_BP1,          ///<
	TGE_WAIT_VD6_BP1,          ///<
	TGE_WAIT_VD7_BP1,          ///<
	TGE_WAIT_VD8_BP1,          ///<
	ENUM_DUMMY4WORD(TGE_WAIT_EVENT_SEL)
} TGE_WAIT_EVENT_SEL;
#endif

typedef enum {
	TGE_VD_DISABLE       = 0,
	TGE_VD_ENABLE,
	ENUM_DUMMY4WORD(TGE_VD_RST_SEL)
} TGE_VD_RST_SEL;

typedef enum {
	TGE_OUT_NO_SWAP     = 0,
	TGE_OUT_SWAP,
	ENUM_DUMMY4WORD(TGE_OUT_SWAP_SEL)
} TGE_OUT_SWAP_SEL;

typedef struct {
	TGE_MODE_SEL            ModeSel;          ///<
	TGE_MODE_SEL            Mode2Sel;         ///<
	TGE_MODE_SEL            Mode3Sel;         ///<
	TGE_MODE_SEL            Mode4Sel;         ///<
	TGE_MODE_SEL            Mode5Sel;         ///< master mode only
	TGE_MODE_SEL            Mode6Sel;         ///< master mode only
	TGE_MODE_SEL            Mode7Sel;         ///< master mode only
	TGE_MODE_SEL            Mode8Sel;         ///< master mode only
	TGE_FLSH_MSH_VD_IN_SEL  FlshMshVdHdInSel; ///< VD 1~4 only
} TGE_MODE_SEL_INFO;



typedef struct {
	TGE_DATA_PHASE_SEL  VdPhase;     ///<
	TGE_DATA_PHASE_SEL  HdPhase;     ///<
} TGE_VD_PHASE;

typedef struct {
	BOOL                bVdInv;      ///<
	BOOL                bHdInv;      ///<
} TGE_VD_INV;

typedef struct {
	TGE_VD_PHASE        VdPhaseData;      ///<
	TGE_VD_PHASE        Vd2PhaseData;     ///<
	TGE_VD_PHASE        Vd3PhaseData;     ///<
	TGE_VD_PHASE        Vd4PhaseData;     ///<
	TGE_VD_PHASE        Vd5PhaseData;     ///<
	TGE_VD_PHASE        Vd6PhaseData;     ///<
	TGE_VD_PHASE        Vd7PhaseData;     ///<
	TGE_VD_PHASE        Vd8PhaseData;     ///<
} TGE_VD_PHASE_INFO;

typedef struct {
	TGE_VD_INV          VdInvData;      ///<
	TGE_VD_INV          Vd2InvData;      ///<
	TGE_VD_INV          Vd3InvData;      ///<
	TGE_VD_INV          Vd4InvData;      ///<
	TGE_VD_INV          Vd5InvData;      ///<
	TGE_VD_INV          Vd6InvData;      ///<
	TGE_VD_INV          Vd7InvData;      ///<
	TGE_VD_INV          Vd8InvData;      ///<
} TGE_VD_INV_INFO;

typedef struct {
	TGE_VD_PHASE_INFO   VdPhaseData;
	TGE_VD_INV_INFO     VdInvData;
	BOOL                bFlshExtTrgInv;     ///<
	BOOL                bMshExtTrgInv;     ///<
	BOOL                bFlshCtrlInv;     ///<
	BOOL                bMshACtrlInv;     ///<
	BOOL                bMshBCtrlInv;     ///<
	BOOL                bMshASwap;     ///<
	BOOL                bMshBSwap;     ///<
	TGE_MSH_WAVEFORM_SEL    MshAWvFrm;
	TGE_MSH_WAVEFORM_SEL    MshBWvFrm;
	TGE_VDTOSIE_SEL     Vd2SieSrc;
	TGE_VDTOSIE3_SEL    Vd2Sie3Src;
} TGE_BASIC_SET_INFO;

typedef struct {
	TGE_VD_RST_SEL        VdRst;
	TGE_VD_RST_SEL        Vd2Rst;
	TGE_VD_RST_SEL        Vd3Rst;
	TGE_VD_RST_SEL        Vd4Rst;
	TGE_VD_RST_SEL        Vd5Rst;
	TGE_VD_RST_SEL        Vd6Rst;
	TGE_VD_RST_SEL        Vd7Rst;
	TGE_VD_RST_SEL        Vd8Rst;
} TGE_VD_RST_INFO;

typedef struct {
	UINT32      uiVdPeriod;
	UINT32      uiVdAssert;
	UINT32      uiVdFrontBlnk;
	UINT32      uiHdPeriod;
	UINT32      uiHdAssert;
	UINT32      uiHdCnt;
} TGE_VDHD_INFO;

typedef struct {
	TGE_VDHD_INFO  VdInfo;
	TGE_VDHD_INFO  Vd2Info;
	TGE_VDHD_INFO  Vd3Info;
	TGE_VDHD_INFO  Vd4Info;
	TGE_VDHD_INFO  Vd5Info;
	TGE_VDHD_INFO  Vd6Info;
	TGE_VDHD_INFO  Vd7Info;
	TGE_VDHD_INFO  Vd8Info;
} TGE_VDHD_SET;

typedef struct {
	TGE_OUT_SWAP_SEL  SnXvsOutSwap;
	TGE_OUT_SWAP_SEL  Sn2XvsOutSwap;
	TGE_OUT_SWAP_SEL  Sn3XvsOutSwap;
	TGE_OUT_SWAP_SEL  Sn4XvsOutSwap;
} TGE_OUT_SWAP_INFO;

typedef struct {
	UINT32 uiVdBp;             ///< VD break point
	UINT32 uiVd2Bp;             ///< VD2 break point
	UINT32 uiVd3Bp;             ///< VD3 break point
	UINT32 uiVd4Bp;             ///< VD4 break point
	UINT32 uiVd5Bp;             ///< VD5 break point
	UINT32 uiVd6Bp;             ///< VD6 break point
	UINT32 uiVd7Bp;             ///< VD7 break point
	UINT32 uiVd8Bp;             ///< VD8 break point
} TGE_BREAKPOINT_INFO;



typedef struct {
	UINT8       uiClkCnt;
	BOOL        bVD;
	BOOL        bHD;
	BOOL        bFlshExtTrg;
	BOOL        bMshExtTrg;
	BOOL        bFlshCtrl;
	BOOL        bMshCtrlA0;
	BOOL        bMshCtrlA1;
	BOOL        bMshCtrlB0;
	BOOL        bMshCtrlB1;
} TGE_ENGINE_STATUS_INFO;


typedef struct {
	UINT32      uiDelay;
	UINT32      uiAssert;
	//TGE_CTRL_PIN_TRG_SEL TrigSel;
} TGE_CTRL_PIN_INFO;


typedef struct {
	BOOL    bVdPause;
	BOOL    bHdPause;
} TGE_TIMING_PAUSE_INFO;



/**
    TGE Entire Configuration.

    Structure of entire TGE configuration.
*/
//@{
typedef struct {
	// No run-time change
	UINT32  uiIntrpEn;
	TGE_BASIC_SET_INFO          BasicSetInfo;
	TGE_MODE_SEL_INFO           ModeSelInfo;
	TGE_VD_RST_INFO             VdRstInfo;
	TGE_VDHD_SET                VdHdInfo;
	TGE_OUT_SWAP_INFO           OutSwapInfo;
	TGE_BREAKPOINT_INFO         BreakPointInfo;

} TGE_MODE_PARAM;
//@}
//@}
typedef struct _TGE_OPENOBJ {
	void (*pfTgeIsrCb)(UINT32 uiIntpStatus); ///< isr callback function
	TGE_CLKSRC_SEL TgeClkSel;   ///< TGE clock source selection
	TGE_CLKSRC_SEL TgeClkSel2;  ///< TGE clock2 source selection
} TGE_OPENOBJ;

/**
TGE API

@name   TGE_API
*/
//@{

// tge_lib.c
extern void tge_create_resource(void);
extern void tge_release_resource(void);
extern void tge_isr(void);
extern ER tge_open(TGE_OPENOBJ *pObjCB);
extern BOOL tge_isOpened(void);
extern ER tge_close(void);
extern ER tge_pause(void);
extern ER tge_start(void);
extern ER tge_setMode(TGE_MODE_PARAM *pTgeParam);
extern ER tge_chgParam(void *pParam, TGE_CHANGE_FUN_PARAM_SEL FunSel);
extern ER tge_chgRst(TGE_VD_RST_INFO *pParam, TGE_CHANGE_RST_SEL FunSel);
extern ER tge_chgVdHd(TGE_VDHD_SET *pParam, TGE_CHANGE_VDHD_SEL FunSel);
extern ER tge_waitEvent(TGE_WAIT_EVENT_SEL WaitEvent, BOOL bClrFlag);
extern UINT32 tge_getClockRate(void);
extern INT32 tge_chk_limitation(UINT32 flag);
// tge_int.c
extern void tge_getVdRstInfo(TGE_VD_RST_INFO *pParam);

extern ER tge_setClock(TGE_CLKSRC_SEL ClkSrc);
extern ER tge_setClock2(TGE_CLKSRC_SEL ClkSrc);

#endif
//@}

//@}


