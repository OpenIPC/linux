/**
    Public header file for dis

    This file is the header file that define the API and data type for vendor_dis.

    @file       vendor_dis.h
    @ingroup    mIDrvIPP_DIS
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2018.    All rights reserved.
*/
#ifndef _VENDOR_DIS_H_
#define _VENDOR_DIS_H_

#include "hd_type.h"

//----------------------------------------------------------------------
typedef struct _VENDOR_DIS_TRIGGER_PARAM {
	BOOL    wait_end;
	UINT32	time_out_ms;	///< force time out when wait_end == TRUE, set 0 for disable time out
} VENDOR_DIS_TRIGGER_PARAM;

//----------------------------------------------------------------------

/**
	DIS structure - dis open object
*/
typedef struct _VENDOR_DIS_OPENCFG {
	UINT32 dis_clock_sel;	//Engine clock selection
} VENDOR_DIS_OPENCFG;

//----------------------------------------------------------------------
/**
	DIS input image structure
*/
typedef struct _VENDOR_DIS_IN_IMG_INFO {
	UINT32 ui_width;		         ///image width
	UINT32 ui_height;		         ///image height
	UINT32 ui_inofs;	             ///DRAM line offset of input channel
	BOOL   changesize_en;            ///enable for only change size
} VENDOR_DIS_IN_IMG_INFO;

//----------------------------------------------------------------------

/**
    DIS VENDOR Address info
*/
typedef struct _VENDOR_DIS_IMG_IN_DMA_INFO {
	UINT32 ui_inadd0;					///< input starting DMA address 0
	UINT32 ui_inadd1;					///< input starting DMA address 1
	UINT32 ui_inadd2;					///< input starting DMA address 2
} VENDOR_DIS_IMG_IN_DMA_INFO;

typedef struct _VENDOR_DIS_IMG_OUT_DMA_INFO {
	UINT32 ui_outadd0;                   ///< output starting DMA address 0
	UINT32 ui_outadd1;                   ///< output starting DMA address 1	
} VENDOR_DIS_IMG_OUT_DMA_INFO;
//----------------------------------------------------------------------

/**
	DIS KDRV MV output info
*/
typedef struct _VENDOR_DIS_MOTION_INFOR {
	INT32   ix;          ///< x component
	INT32   iy;          ///< y component
	UINT32  ui_sad;      ///< sum of absolute difference
	UINT32  ui_cnt;      ///< number of edge pixels (src block)
	UINT32  ui_idx;      ///< block index
	BOOL    bvalid;      ///< reliability of MV
} VENDOR_DIS_MOTION_INFOR;


typedef struct _VENDOR_DIS_MV_IMG_OUT_DMA_INFO {
	VENDOR_DIS_MOTION_INFOR* p_mvaddr;
} VENDOR_DIS_MV_IMG_OUT_DMA_INFO;

//----------------------------------------------------------------------
typedef struct _VENDOR_DIS_MDS_DIM {
	UINT8 ui_blknum_h;    ///< horizontal block number in use
	UINT8 ui_blknum_v;    ///< vertical block number in use
	UINT8 ui_mdsnum;      ///< total MDS number in use
} VENDOR_DIS_MDS_DIM;

typedef enum {
	VENDOR_DIS_BLKSZ_64x48 = 0,  ///< 64*48
	VENDOR_DIS_BLKSZ_32x32 = 1,  ///< 32*32
	ENUM_DUMMY4WORD(VENDOR_DIS_BLOCKS_DIM)
} VENDOR_DIS_BLOCKS_DIM;

/// jsliu@201014
typedef enum {
	VENDOR_DIS_ABORT_ENABLE = 0,
	VENDOR_DIS_ABORT_DISABLE = 1,
	ENUM_DUMMY4WORD(VENDOR_DIS_ABORT)
} VENDOR_DIS_ABORT;
//----------------------------------------------------------------------
/**
	ETH input parameter structure
*/
typedef struct _VENDOR_ETH_IN_PARAM {
	BOOL enable;		///< eth enable
	BOOL out_bit_sel;	///< 0 --> output 2 bit per pixel, 1 --> output 8 bit per pixel
	BOOL out_sel;		///< 0 --> output all pixel, 1 --> output pixel select by b_h_out_sel/b_v_out_sel
	BOOL h_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	BOOL v_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	
	UINT16 th_low;		///< edge threshold
	UINT16 th_mid;
	UINT16 th_high;

} VENDOR_DIS_ETH_IN_PARAM;

/**
	ETH input buffer structure
*/
typedef struct _VENDOR_ETH_IN_BUFFER_INFO {
	UINT32 ui_inadd;        ///< output buffer address
	UINT32 buf_size;	    ///< output buffer size [Set SIE]kflow will force disable eth when buffer size < eth out size, [Get] none
	UINT32 frame_cnt;
} VENDOR_DIS_ETH_IN_BUFFER_INFO;

/**
	ETH Output structure
*/
typedef struct {
	UINT32	w;		///< Rectangle width
	UINT32	h;		///< Rectangle height
} VENDOR_DIS_USIZE, *PVENDOR_DIS_USIZE;

typedef struct _VENDOR_DIS_ETH_OUT_PARAM {
	VENDOR_DIS_USIZE out_size;		///< eth output size;
	UINT32           out_lofs;      ///< eth output lineoffset;
} VENDOR_DIS_ETH_OUT_PARAM;
//----------------------------------------------------------------------

typedef enum {
	VENDOR_DIS_INPUT_INFO,			            
	VENDOR_DIS_INPUT_ADDR,		               	
	VENDOR_DIS_INT_EN,		                
	VENDOR_DIS_OUTPUT_ADDR, 
	VENDOR_DIS_MV_OUT,
	VENDOR_DIS_MDS_DIM_INFO,
	VENDOR_DIS_BLOCKS_DIM_INFO,
	VENDOR_DIS_ETH_PARAM_IN,
	VENDOR_DIS_ETH_BUFFER_IN,
	VENDOR_DIS_ETH_PARAM_OUT,
	VENDOR_DIS_DMA_ABORT,			/// jsliu@201014
	ENUM_DUMMY4WORD(VENDOR_DIS_FUNC)
} VENDOR_DIS_FUNC;
//----------------------------------------------------------------------

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern INT32 vendor_dis_init(UINT32 func);
extern INT32 vendor_dis_uninit(UINT32 func);
extern INT32 vendor_dis_set_param(VENDOR_DIS_FUNC param_id, void *p_param);
extern INT32 vendor_dis_get_param(VENDOR_DIS_FUNC param_id, void *p_param);
extern INT32 vendor_dis_trigger(VENDOR_DIS_TRIGGER_PARAM *p_param);

#endif //_VENDOR_DIS_H_
