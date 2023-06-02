/**
    Header file for DAL ISE module

    This file is the header file that define the API for DAL ISE.

    @file       kdrv_ise.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef _KDRV_ISE_H_
#define _KDRV_ISE_H_

#include "kdrv_type.h"

/******************************************************************************/
/* obsolete prototype for 520 backward compatible */

#define KDRV_ISE_PARAM_OPENCFG	(0xffff0000)
#define KDRV_ISE_PARAM_MODE		(0xffff0001)
#define KDRV_ISE_LL_PARAM_MODE	(0xffff0002)
#define KDRV_ISE_IS_OLD_PARAM_ID(id) (((id & 0xffff0000) == 0xffff0000) ? TRUE : FALSE)

/**
    KDRV ISE structure - ise open object
*/
typedef struct _KDRV_ISE_OPENOBJ {
	UINT32          ise_clock_sel;                            ///< Engine clock selection
	void            (*isr_cb)(UINT32 status, void *userdata); ///< ISR callback function
	void            *userdata;                                ///< the userdata that want to pass back from ISR callback fu
} KDRV_ISE_OPENOBJ, *pKDRV_ISE_OPENOBJ;

/**
    ISE structure - Engine flow setting structure
*/
typedef struct _KDRV_ISE_MODE {
	UINT32        io_pack_fmt;           ///< input and output packing format
	UINT32        argb_out_mode;         ///< select output mode, this parameter is valid when io_pack_fmt set ISE_ARGB8888
	UINT32        scl_method;            ///< scaling method selection
	UINT32        scl_filter;            ///< scaling filter mode selection

	UINT32        in_width;              ///< input image width
	UINT32        in_height;             ///< input image height
	UINT32        in_lofs;               ///< input lineoffset
	UINT32        in_addr;               ///< input buffer address
	UINT32        in_buf_flush;          ///< input buffer flush selection

	UINT32        out_width;             ///< output image width
	UINT32        out_height;            ///< output image height
	UINT32        out_lofs;              ///< output lineoffset
	UINT32        out_addr;              ///< output buffer address
	UINT32        out_buf_flush;         ///< output buffer flush selection

	UINT32        ll_job_nums;              ///< linked-list job numbers
	UINT32        ll_in_addr;               ///< linked-list command DMA start address
} KDRV_ISE_MODE, *pKDRV_ISE_MODE;

/**
    KDRV ISE structure - trigger type
*/
typedef struct _KDRV_ISE_TRIGGER_PARAM {
	BOOL    wait_end;                                   ///< wait trigger command end
	UINT32  time_out_ms;                                ///< force time out when wait_end == TRUE, set 0 for disable time out
} KDRV_ISE_TRIGGER_PARAM;


/*!
 * @fn INT32 kdrv_ise_linked_list_trigger(UINT32 id)
 * @brief trigger hardware engine using linked-list
 * @param id                the id of hardware
 * @return return 0 on success, -1 on error
 */
extern INT32 kdrv_ise_linked_list_trigger(UINT32 id);

/*!
 * @fn UINT32 kdrv_ise_get_linked_list_buf_size(UINT32 id, UINT32 proc_job_nums)
 * @brief get buffer size of linked-list commands
 * @param id                the id of hardware
 * @param proc_job_nums     the linked-list job numbers
 * @return return buffer size, unit: byte
 */
extern UINT32 kdrv_ise_get_linked_list_buf_size(UINT32 id, UINT32 proc_job_nums);
/******************************************************************************/

typedef enum {
	KDRV_ISE_PARAM_GEN_NODE = 0,	/* Generate config node.             SET-Only,   data_type: NULL */
	KDRV_ISE_PARAM_IO_CFG,			/* set ise input/output config.      SET-Only,   data_type: KDRV_ISE_IO_CFG */
	KDRV_ISE_PARAM_FLUSH_JOB,		/* flush current job.				 SET-Only,   data_type: NULL */
	KDRV_ISE_PARAM_FLUSH_CFG,		/* flush current cfg.				 SET-Only,   data_type: NULL */
	KDRV_ISE_PARAM_ID_MAX,
} KDRV_ISE_PARAM_ID;

typedef enum {
	KDRV_ISE_PROC_MODE_LINKLIST = 0,	/* linlist mode */
	KDRV_ISE_PROC_MODE_CPU,				/* cpu write register, only for debug */
	KDRV_ISE_PROC_MODE_MAX,
} KDRV_ISE_PROC_MODE;

typedef struct {
	UINT32 reserved;
} KDRV_ISE_CTX_BUF_CFG;


/**
    Input and output packing format selection
*/
typedef enum _KDRV_ISE_IO_PACK_SEL {
	KDRV_ISE_Y8          = 0,                   ///< Y-8bit packing
	KDRV_ISE_Y4          = 1,                   ///< Y-4bit packing
	KDRV_ISE_Y1          = 2,                   ///< Y-1bit packing
	KDRV_ISE_UVP         = 3,                   ///< UV-Packed
	KDRV_ISE_RGB565      = 4,                   ///< RGB565 packing
	KDRV_ISE_ARGB8888    = 5,                   ///< ARGB8888
	KDRV_ISE_ARGB1555    = 6,                   ///< ARGB1555
	KDRV_ISE_ARGB4444    = 7,                   ///< ARGB4444
	KDRV_ISE_YUVP        = 8,                   ///< YUV-Packed
	KDRV_ISE_IO_PACK_SEL_MAX,
	ENUM_DUMMY4WORD(KDRV_ISE_IO_PACK_SEL)
} KDRV_ISE_IO_PACK_SEL;

/**
    ARGB8888 output mode selection
*/
typedef enum _KDRV_ISE_ARGB_OUTMODE_SEL {
	KDRV_ISE_OUTMODE_ARGB8888 = 0,      ///< Output ARGB8888
	KDRV_ISE_OUTMODE_RGB888   = 1,      ///< Output RGB888, output stream is XRGB; where X is ignored data.
	KDRV_ISE_OUTMODE_A8       = 2,      ///< Output A8, output stream is AXXX; where X is ignored data.
	KDRV_ISE_ARGB_OUTMODE_MAX,
	ENUM_DUMMY4WORD(KDRV_ISE_ARGB_OUTMODE_SEL)
} KDRV_ISE_ARGB_OUTMODE_SEL;

/**
    Image scaling method selection
*/
typedef enum _KDRV_ISE_SCALE_METHOD {
	KDRV_ISE_BILINEAR    = 1,               ///< bilinear interpolation
	KDRV_ISE_NEAREST     = 2,               ///< nearest interpolation
	KDRV_ISE_INTEGRATION = 3,               ///< integration
	KDRV_ISE_SCALE_METHOD_MAX,
	ENUM_DUMMY4WORD(KDRV_ISE_SCALE_METHOD)
} KDRV_ISE_SCALE_METHOD;

/**
    Flush buffer selection
*/
typedef enum _KDRV_ISE_BUF_FLUSH_SEL {
	KDRV_ISE_DO_BUF_FLUSH       = 0,    ///< to do flush
	KDRV_ISE_NOTDO_BUF_FLUSH    = 1,    ///< not to do flush
	ENUM_DUMMY4WORD(KDRV_ISE_BUF_FLUSH_SEL)
} KDRV_ISE_BUF_FLUSH_SEL;

typedef struct {
	KDRV_ISE_IO_PACK_SEL        io_pack_fmt;           ///< input and output packing format
	KDRV_ISE_ARGB_OUTMODE_SEL   argb_out_mode;         ///< select output mode, this parameter is valid when io_pack_fmt set ISE_ARGB8888
	KDRV_ISE_SCALE_METHOD       scl_method;            ///< scaling method selection

	UINT32                      in_width;              ///< input image width
	UINT32                      in_height;             ///< input image height
	UINT32                      in_lofs;               ///< input lineoffset
	UINT32                      in_addr;               ///< input buffer address
	KDRV_ISE_BUF_FLUSH_SEL      in_buf_flush;          ///< input buffer flush selection

	UINT32                      out_width;             ///< output image width
	UINT32                      out_height;            ///< output image height
	UINT32                      out_lofs;              ///< output lineoffset
	UINT32                      out_addr;              ///< output buffer address
	KDRV_ISE_BUF_FLUSH_SEL      out_buf_flush;         ///< output buffer flush selection
} KDRV_ISE_IO_CFG;

typedef struct {
	KDRV_ISE_PROC_MODE mode;
} KDRV_ISE_TRIG_PARAM;

typedef struct {
	UINT32 job_id;
	UINT32 job_num;
	UINT32 err_num;
	UINT32 done_num;
} KDRV_ISE_CALLBACK_INFO;

UINT32 kdrv_ise_buf_query(KDRV_ISE_CTX_BUF_CFG cfg);
INT32 kdrv_ise_init(KDRV_ISE_CTX_BUF_CFG ctx_buf_cfg, UINT32 buf_addr, UINT32 buf_size);
INT32 kdrv_ise_uninit(void);
INT32 kdrv_ise_open(UINT32 chip, UINT32 engine);
INT32 kdrv_ise_close(UINT32 chip, UINT32 engine);
INT32 kdrv_ise_set(UINT32 dev_id, UINT32 param_id, void *p_data);
INT32 kdrv_ise_get(UINT32 dev_id, UINT32 param_id, void *p_data);
INT32 kdrv_ise_trigger(UINT32 id,  void *p_param, KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
INT32 kdrv_ise_dma_abort(UINT32 id, void *p_data);

#if defined (__LINUX)
#elif defined (__FREERTOS)
/* rtos drv_init api */
void kdrv_ise_drv_init(void);
#endif

#endif
