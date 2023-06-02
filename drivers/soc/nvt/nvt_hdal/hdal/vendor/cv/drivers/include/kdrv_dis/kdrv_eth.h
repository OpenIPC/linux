/**
	Public header file for dis

	This file is the header file that define the API and data type for kdrv_dis.

	@file	kdrv_dis.h
	@ingroup	mIDrvIPP_DIS
	@note	Nothing (or anything need to be mentioned).

	Copyright	Novatek Microelectronics Corp. 2018.	All rights reserved.
*/
#ifndef _KDRV_ETH_H_
#define _KDRV_ETH_H_

#include "dis_platform.h"

#ifndef KDRV_DIS_HANDLE_MAX_NUM
#define KDRV_DIS_HANDLE_MAX_NUM (8)
#endif


/**
	ETH input parameter structure
*/
typedef struct _KDRV_ETH_IN_PARAM {
	BOOL enable;		///< eth enable
	BOOL out_bit_sel;	///< 0 --> output 2 bit per pixel, 1 --> output 8 bit per pixel
	BOOL out_sel;		///< 0 --> output all pixel, 1 --> output pixel select by b_h_out_sel/b_v_out_sel
	BOOL h_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	BOOL v_out_sel; 	///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	
	UINT16 th_low;		///< edge threshold
	UINT16 th_mid;
	UINT16 th_high;

} KDRV_ETH_IN_PARAM;

/**
	ETH input buffer structure
*/
typedef struct _KDRV_ETH_IN_BUFFER_INFO {
	UINT32 ui_inadd;       ///< output buffer address
	UINT32 buf_size;	   ///< output buffer size [Set SIE]kflow will force disable eth when buffer size < eth out size, [Get] none
	UINT32 frame_cnt;
} KDRV_ETH_IN_BUFFER_INFO;

/**
	ETH Output structure
*/
typedef struct _KDRV_ETH_OUT_PARAM {
	USIZE  out_size;		///< eth output size;
	UINT32 out_lofs;        ///< eth output lineoffset;
} KDRV_ETH_OUT_PARAM;

//----------------------------------------------------------------------

/**
    parameter data for kdrv_dis_set, kdrv_dis_get
*/
typedef enum {
	KDRV_ETH_PARAM_IN_INFO,			            /// [Set/Get], use KDRV_ETH_IN_PARAM structure
	KDRV_ETH_BUFFER_IN_INFO,		            /// [Set/Get], use KDRV_ETH_IN_BUFFER_INFO structure, set the dram input address	
	KDRV_ETH_PARAM_INT_EN,		                /// [Set/Get]
	KDRV_ETH_PARAM_OUT_INFO,                     /// [Set/Get], use KDRV_DIS_OUT_DMA_INFO structure, set the dram output address

	KDRV_ETH_PARAM_MAX,
	KDRV_ETH_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_ETH_PARAM_ID)
} KDRV_ETH_PARAM_ID;
STATIC_ASSERT((KDRV_ETH_PARAM_MAX &KDRV_ETH_PARAM_REV) == 0);


typedef struct _KDRV_DIS_ETH_PRAM {
	KDRV_ETH_IN_PARAM			    in_param_info;
	KDRV_ETH_IN_BUFFER_INFO			in_dma_info;
	KDRV_ETH_OUT_PARAM              out_param_info;
} KDRV_DIS_ETH_PRAM, *pKDRV_DIS_ETH_PRAM;

ER kdrv_dis_eth_set_in_param_info(UINT32 id, void* p_data);
ER kdrv_dis_eth_set_dma_out(UINT32 id, void* p_data);
ER kdrv_dis_eth_get_in_param_info(UINT32 id, void* p_data);
ER kdrv_dis_eth_get_dma_out(UINT32 id, void* p_data);
ER kdrv_dis_eth_get_out_info(UINT32 id, void* p_data);


#endif //_KDRV_ETH_H_
