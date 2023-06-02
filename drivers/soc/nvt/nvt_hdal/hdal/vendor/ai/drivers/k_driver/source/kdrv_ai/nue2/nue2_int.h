/*
    NUE2 module driver

    NT98520 NUE2 registers header file.

    @file       nue2_int.h
    @ingroup    mIIPPNUE2
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/


#ifndef _NUE2_INT_H
#define _NUE2_INT_H

#include "nue2_lmt.h"
#include "nue2_lib.h"

#define NUE2_DMA_CACHE_HANDLE   (1)
#define NUE2_IOADDR_REG_BASE     nue2_reg_base_addr

extern UINT32 nue2_reg_base_addr;

/*
    NUE2 Flip mode for CHIP:528
*/
typedef enum {
    NUE2_NO_FLIP         = 0,
    NUE2_X_FLIP          = 1,
    NUE2_Y_FLIP          = 2,
    NUE2_X_FLIP_Y_FLIP   = 3,
    ENUM_DUMMY4WORD(NUE2_FLIP)
} NUE2_FLIP;

/*
    NUE2 State machine
*/
typedef enum {
	NUE2_ENGINE_IDLE    = 0,
	NUE2_ENGINE_READY   = 1,
	NUE2_ENGINE_RUN     = 2,
	NUE2_ENGINE_PAUSE   = 3,
	ENUM_DUMMY4WORD(NUE2_ENGINE_STATUS)
} NUE2_ENGINE_STATUS;

/*
    NUE2 Operation
*/
typedef enum {
	NUE2_OP_OPEN             = 0,
	NUE2_OP_CLOSE            = 1,
	NUE2_OP_SET2READY        = 2,   ///< set module to ready state
	NUE2_OP_SET2PAUSE        = 3,   ///< set module to pause
	NUE2_OP_SET2RUN          = 4,   ///< set module to run
	NUE2_OP_ATTACH           = 5,
	NUE2_OP_DETACH           = 6,
	NUE2_OP_READLUT          = 7,
	NUE2_OP_CHGINT           = 8,
	ENUM_DUMMY4WORD(NUE2_OPERATION)
} NUE2_OPERATION;

/*
    NUE2 Cycle Count
*/
typedef enum {
	NUE2_CYCLE_OFF    = 0,
	NUE2_CYCLE_APP    = 1,
	NUE2_CYCLE_LL     = 2,
	ENUM_DUMMY4WORD(NUE2_ENGINE_CYCLE_MODE)
} NUE2_ENGINE_CYCLE_MODE;


#if NUE2_CYCLE_TEST
UINT32 nue2_getcycle(VOID);
#endif
VOID nue2_enable_int(BOOL enable, UINT32 intr);
UINT32 nue2_get_int_enable(VOID);
VOID nue2_clr_intr_status(UINT32 status);
UINT32 nue2_get_intr_status(VOID);
VOID nue2_clr(BOOL reset);
VOID nue2_enable(BOOL start);
VOID nue2_ll_enable(BOOL start);
VOID nue2_ll_terminate(BOOL isterminate);
VOID nue2_set_dmain_addr(UINT32 addr0, UINT32 addr1, UINT32 addr2);
UINT32 nue2_get_dmain_addr(NUE2_IN_BUFID bufid);
VOID nue2_set_dmaout_addr(UINT32 addr0, UINT32 addr1, UINT32 addr2);
UINT32 nue2_get_dmaout_addr(NUE2_OUT_BUFID bufid);
VOID nue2_set_dmain_lladdr(UINT32 addr0);
UINT32 nue2_get_dmain_lladdr(VOID);
VOID nue2_set_ll_base_addr(UINT32 ll_base_addr);
UINT32 nue2_get_ll_base_addr(VOID);
ER nue2_set_burst(UINT8 in_burst_mode, UINT8 out_burst_mode);
VOID nue2_set_cycle_en(NUE2_ENGINE_CYCLE_MODE mode);

VOID nue2_set_dma_disable(UINT8 is_en);
BOOL nue2_get_dma_disable(VOID);
BOOL nue2_get_engine_idle(VOID);

#if defined(_BSP_NA51089_)
VOID nue2_set_chksum_en(BOOL is_en);
NUE2_CHKSUM_ERR_STS_PARM nue2_get_output_chksum_err_sts_parm(VOID);
VOID nue2_clr_output_chksum_err_sts_parm(VOID);
NUE2_CHKSUM_PARM nue2_get_output_chksum_parm(VOID);
VOID nue2_fill_output_chksum_parm(NUE2_CHKSUM_PARM chksum_parm);
#endif

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_set_drvmode(NUE2_OPMODE mode);
NUE2_OPMODE nue2_get_drvmode(VOID);
VOID nue2_setYuv2rgbEnReg(BOOL bEn);
BOOL nue2_getYuv2rgbEnReg(VOID);
VOID nue2_setSubEnReg(BOOL bEn);
BOOL nue2_getSubEnReg(VOID);
VOID nue2_setPadEnReg(BOOL bEn);
BOOL nue2_getPadEnReg(VOID);
VOID nue2_setHsvEnReg(BOOL bEn);
BOOL nue2_getHsvEnReg(VOID);
VOID nue2_setRotateEnReg(BOOL bEn);
BOOL nue2_getRotateEnReg(VOID);
VOID nue2_set_infmt(NUE2_IN_FMT in_fmt);
NUE2_IN_FMT nue2_get_infmt(VOID);
VOID nue2_set_outfmt(NUE2_OUT_FMT out_fmt);
NUE2_OUT_FMT nue2_get_outfmt(VOID);

ER nue2_set_insize(NUE2_IN_SIZE in_size);
NUE2_IN_SIZE nue2_get_insize(VOID);
ER nue2_set_func_en(NUE2_FUNC_EN func_en);
NUE2_FUNC_EN nue2_get_func_en(VOID);
ER nue2_set_scale_parm(NUE2_SCALE_PARM scale_parm);
NUE2_SCALE_PARM nue2_get_scale_parm(VOID);
ER nue2_set_sub_parm(NUE2_SUB_PARM sub_parm);
NUE2_SUB_PARM nue2_get_sub_parm(VOID);
ER nue2_set_pad_parm(NUE2_PAD_PARM pad_parm);
NUE2_PAD_PARM nue2_get_pad_parm(VOID);
ER nue2_set_dmaio_lofs(NUE2_DMAIO_LOFS dmaio_lofs);
NUE2_DMAIO_LOFS nue2_get_dmaio_lofs(VOID);
ER nue2_set_dmaio_addr(NUE2_DMAIO_ADDR dma_addr);
NUE2_DMAIO_ADDR nue2_get_dmaio_addr(VOID);
ER nue2_set_mean_shift_parm(NUE2_MEAN_SHIFT_PARM mean_shift_parm);
NUE2_MEAN_SHIFT_PARM nue2_get_mean_shift_parm(VOID);
ER nue2_set_flip_parm(NUE2_FLIP_PARM flip_parm);
NUE2_FLIP_PARM nue2_get_flip_parm(VOID);
ER nue2_set_hsv_parm(NUE2_HSV_PARM hsv_parm);
NUE2_HSV_PARM nue2_get_hsv_parm(VOID);
ER nue2_set_rotate_parm(NUE2_ROTATE_PARM rotate_parm);
NUE2_ROTATE_PARM nue2_get_rotate_parm(VOID);
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

#endif
