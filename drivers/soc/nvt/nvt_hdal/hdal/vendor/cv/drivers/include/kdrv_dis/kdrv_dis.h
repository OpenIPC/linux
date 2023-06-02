/**
	Public header file for dis

	This file is the header file that define the API and data type for kdrv_dis.

	@file	kdrv_dis.h
	@ingroup	mIDrvIPP_DIS
	@note	Nothing (or anything need to be mentioned).

	Copyright	Novatek Microelectronics Corp. 2018.	All rights reserved.
*/
#ifndef _KDRV_DIS_H_
#define _KDRV_DIS_H_

#include "dis_platform.h"

//structure data type
typedef struct _KDRV_DIS_TRIGGER_PARAM {
	BOOL    wait_end;
	UINT32	time_out_ms;	          ///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_DIS_TRIGGER_PARAM;

//----------------------------------------------------------------------

/**
	DIS structure - dis kdrv open object
*/
typedef struct _KDRV_DIS_OPENCFG {
	UINT32 dis_clock_sel;	          ///Engine clock selection
} KDRV_DIS_OPENCFG, *pKDRV_DIS_OPENCFG;
//----------------------------------------------------------------------
/**
	DIS input image structure
*/
typedef struct _KDRV_DIS_IN_IMG_INFO {
	UINT32 ui_width;		         ///image width
	UINT32 ui_height;		         ///image height
	UINT32 ui_inofs;	             ///DRAM line offset of input channel
	BOOL   changesize_en;            ///enable for only change size
} KDRV_DIS_IN_IMG_INFO;
//----------------------------------------------------------------------

/**
	DIS KDRV Address info
*/
typedef struct _KDRV_DIS_IN_DMA_INFO {
	UINT32 ui_inadd0;					///< input starting DMA address 0
	UINT32 ui_inadd1;					///< input starting DMA address 1
	UINT32 ui_inadd2;					///< input starting DMA address 2
} KDRV_DIS_IN_DMA_INFO;

typedef struct _KDRV_DIS_OUT_DMA_INFO {	
	UINT32 ui_outadd0;                   ///< output starting DMA address 0
	UINT32 ui_outadd1;                   ///< output starting DMA address 1	
} KDRV_DIS_OUT_DMA_INFO;

//----------------------------------------------------------------------

/**
	DIS KDRV MV output info
*/
typedef struct _KDRV_MOTION_INFOR {
	INT32   ix;          ///< x component
	INT32   iy;          ///< y component
	UINT32  ui_sad;      ///< sum of absolute difference
	UINT32  ui_cnt;      ///< number of edge pixels (src block)
	UINT32  ui_idx;      ///< block index
	BOOL    bvalid;      ///< reliability of MV
} KDRV_MOTION_INFOR;


typedef struct _KDRV_MV_OUT_DMA_INFO {
	KDRV_MOTION_INFOR* p_mvaddr;
} KDRV_MV_OUT_DMA_INFO;

typedef struct _KDRV_MDS_DIM {
	UINT8 ui_blknum_h;    ///< horizontal block number in use
	UINT8 ui_blknum_v;    ///< vertical block number in use
	UINT8 ui_mdsnum;      ///< total MDS number in use
} KDRV_MDS_DIM;

typedef enum {
	KDRV_DIS_BLKSZ_64x48 = 0,  ///< 64*48
	KDRV_DIS_BLKSZ_32x32 = 1,  ///< 32*32
	
	KDRV_DIS_BLKSZ_MAX,
	ENUM_DUMMY4WORD(KDRV_BLOCKS_DIM)
} KDRV_BLOCKS_DIM;


//----------------------------------------------------------------------

/**
    parameter data for kdrv_dis_set, kdrv_dis_get
*/
typedef enum {
	KDRV_DIS_PARAM_OPENCFG = 0,                 ///< set engine open info,  data_type: KDRV_DIS_OPENCFG
	KDRV_DIS_PARAM_IN_IMG,			            /// [Set/Get], use KDRV_DIS_IN_IMG_INFO structure, set input image size info, line offset
	KDRV_DIS_PARAM_DMA_IN,		                /// [Set/Get], use KDRV_DIS_IN_DMA_INFO structure, set the dram input address	
	KDRV_DIS_PARAM_INT_EN,		                /// [Set/Get]
	KDRV_DIS_PARAM_DMA_OUT,                     /// [Set/Get], use KDRV_DIS_OUT_DMA_INFO structure, set the dram output address
	KDRV_DIS_PARAM_MV_OUT,                      /// [Set/Get], use KDRV_MV_OUT_DMA_INFO  structure, set the MV output address
	KDRV_DIS_PARAM_MV_DIM,                      /// [Get], use KDRV_MDS_DIM structure, get the MV dim
	KDRV_DIS_PARAM_BLOCK_DIM,                   /// [Get], use KDRV_BLOCKS_DIM enum, get the block dim
	KDRV_DIS_ETH_PARAM_IN,
	KDRV_DIS_ETH_BUFFER_IN,
	KDRV_DIS_ETH_PARAM_OUT,
	KDRV_DIS_PARAM_ISR_CB,						///< (for NT96680)
	KDRV_DIS_DMA_ABORT,						///< (for UVC DMA disable), jsliu@201014

	KDRV_DIS_PARAM_MAX,
	KDRV_DIS_PARAM_REV = 0x80000000,
	ENUM_DUMMY4WORD(KDRV_DIS_PARAM_ID)
} KDRV_DIS_PARAM_ID;
STATIC_ASSERT((KDRV_DIS_PARAM_MAX &KDRV_DIS_PARAM_REV) == 0);

#define KDRV_DIS_IGN_CHK KDRV_DIS_PARAM_REV	//only support set/get function

#if defined(__LINUX)
extern void kdrv_dis_install_id(void) _SECTION(".kercfg_text");
extern void kdrv_dis_uninstall_id(void) _SECTION(".kercfg_text");
#else
extern void kdrv_dis_install_id(void);
extern void kdrv_dis_uninstall_id(void);
#endif

/*!
 * @fn INT32 kdrv_dis_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_dis_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_dis_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_dis_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_dis_set(UINT32 id, KDRV_DIS_PARAM_ID param_id, void* p_param)
 * @brief set parameters to hardware engine
 * @param id	    the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_dis_set(UINT32 id, KDRV_DIS_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_dis_get(UINT32 id, KDRV_DIS_PARAM_ID param_id, void* p_param)
 * @brief get parameters from hardware engine
 * @param id	    the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_dis_get(UINT32 id, KDRV_DIS_PARAM_ID param_id, void *p_param);

/*!
 * @fn INT32 kdrv_dis_trigger(UINT32 id, KDRV_DIS_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data)
 * @brief trigger hardware engine
 * @param id	        the id of hardware
 * @param p_param	    the parameter for trigger
 * @param p_cb_func	    the callback function
 * @param p_user_data	the private user data
 * @return return 0 on success, others on error
 */
extern INT32 kdrv_dis_trigger(UINT32 id, KDRV_DIS_TRIGGER_PARAM *p_param, VOID *p_cb_func, VOID *p_user_data);

#endif //_KDRV_DIS_H_
