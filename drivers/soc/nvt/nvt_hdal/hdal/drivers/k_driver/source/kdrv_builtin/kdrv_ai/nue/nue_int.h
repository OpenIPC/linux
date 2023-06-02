/*
    NUE module driver

    NT98313 NUE registers header file.

    @file       nue_int.h
    @ingroup    mIIPPNUE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NUE_INT_H
#define _NUE_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include    "nue_lmt.h"
#include    "nue_lib.h"

extern UINT32 nue_reg_base_addr;

#define NUE_DMA_CACHE_HANDLE   (1)

#define NUE_IOADDR_REG_BASE     nue_reg_base_addr
#define NUE_REG_ADDR(ofs)       (nue_reg_base_addr+(ofs))


/*
    NUE State machine
*/
typedef enum {
	NUE_ENGINE_IDLE    = 0,
	NUE_ENGINE_READY   = 1,
	NUE_ENGINE_RUN     = 2,
	NUE_ENGINE_PAUSE   = 3,
	ENUM_DUMMY4WORD(NUE_ENGINE_STATUS)
} NUE_ENGINE_STATUS;

/*
    NUE Operation
*/
typedef enum {
	NUE_OP_OPEN             = 0,
	NUE_OP_CLOSE            = 1,
	NUE_OP_SET2READY        = 2,   ///< set module to ready state
	NUE_OP_SET2PAUSE        = 3,   ///< set module to pause
	NUE_OP_SET2RUN          = 4,   ///< set module to run
	NUE_OP_ATTACH           = 5,
	NUE_OP_DETACH           = 6,
	NUE_OP_READLUT          = 7,
	NUE_OP_CHGINT           = 8,
	ENUM_DUMMY4WORD(NUE_OPERATION)
} NUE_OPERATION;


VOID nue_cycle_en(BOOL enable);
UINT32 nue_getcycle(VOID);
NUE_OPMODE nue_get_drvmode(VOID);
VOID nue_set_drvmode(NUE_OPMODE mode);
VOID nue_enable_int(BOOL enable, UINT32 intr);
UINT32 nue_get_int_enable(VOID);
VOID nue_clr_intr_status(UINT32 status);
UINT32 nue_get_intr_status(VOID);
VOID nue_clr(BOOL reset);
VOID nue_enable(BOOL start);
VOID nue_ll_enable(BOOL start);
VOID nue_ll_terminate(BOOL terminate);
VOID nue_set_dmain_lladdr(UINT32 addr0);
UINT32 nue_get_dmain_lladdr(VOID);
VOID nue_set_dmain_lladdr_base(UINT32 addr0);
UINT32 nue_get_dmain_lladdr_base(VOID);
ER nue_set_svmsize(NUE_SVM_IN_SIZE *p_size);
VOID nue_set_in_shf(INT32 shift);
INT32 nue_get_in_shf(VOID);
VOID nue_set_in_scale(UINT32 scale);
UINT32 nue_get_in_scale(VOID);
VOID nue_set_clamp_th0(UINT32 threshold);
UINT32 nue_get_clamp_th0(VOID);
VOID nue_set_clamp_th1(UINT32 threshold);
UINT32 nue_get_clamp_th1(VOID);
ER nue_set_anchor_shf(UINT32 shift);
UINT32 nue_get_anchor_shf(VOID);
VOID nue_set_anchor_table_update(BOOL update);
BOOL nue_get_anchor_table_update(VOID);
ER nue_set_softmax_in_shf(INT32 shift);
INT32 nue_get_softmax_in_shf(VOID);
ER nue_set_softmax_out_shf(INT32 shift);
INT32 nue_get_softmax_out_shf(VOID);
ER nue_set_softmax_group_num(UINT32 group_num);
UINT32 nue_get_softmax_group_num(VOID);
VOID nue_set_softmax_setnum(UINT32 num);
UINT32 nue_get_softmax_setnum(VOID);

VOID nue_set_permute_stripe(BOOL stripe_en);
BOOL nue_get_permute_stripe(VOID);

BOOL nue_is_linklist_terminated(VOID);
//VOID nue_set_axidis(BOOL is_dis);
//BOOL nue_get_axidis(VOID);
//BOOL nue_get_axiidle(VOID);
BOOL nue_get_engine_idle(VOID);
BOOL nue_get_dma_disable(VOID);
VOID nue_set_dma_disable(BOOL disable);

//Change data format
UINT32 nue_tran_intval(INT32 value, UINT32 bits);
INT32 nue_tran_bitval(UINT32 value, UINT32 bits);
//For FPGA verify only

//checksum
VOID nue_set_chksum(UINT32 chksum);
UINT32 nue_get_chksum(VOID);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue_set_dmain_lofs(UINT32 ilofs0);
UINT32 nue_get_dmain_lofs(VOID);
VOID nue_set_dmaout_lofs(UINT32 olofs0);
UINT32 nue_get_dmaout_lofs(VOID);
VOID nue_set_dmaout_addr(UINT32 addr0);
UINT32 nue_get_dmaout_addr(VOID);
VOID nue_set_dmain_kqaddr(UINT32 addr0);
UINT32 nue_get_dmain_addr(NUE_IN_BUFID bufid);
VOID nue_set_dmain_roiaddr(UINT32 addr0);
VOID nue_set_dmain_addr(UINT32 addr0, UINT32 addr1);
VOID nue_set_dmain_svmaddr(UINT32 addr0, UINT32 addr1, UINT32 addr2);
ER nue_set_engmode(NUE_MODE_TYPE mode);
NUE_MODE_TYPE nue_get_engmode(VOID);
VOID nue_set_intype(NUE_IO_TYPE in_type);
VOID nue_set_outtype(NUE_IO_TYPE out_type);
NUE_IO_TYPE nue_get_intype(VOID);
NUE_IO_TYPE nue_get_outtype(VOID);
VOID nue_set_kerl_en(BOOL enable, UINT32 kerl);
UINT32 nue_get_kerl_en(VOID);
ER nue_set_ker(NUE_SVMKER_TYPE ker_type);
NUE_SVMKER_TYPE nue_get_ker(VOID);
ER nue_set_kerl1mode(NUE_SVMKER1_TYPE type);
NUE_SVMKER1_TYPE nue_get_kerl1mode(VOID);
ER nue_set_kerl2mode(NUE_SVMKER2_TYPE type);
NUE_SVMKER2_TYPE nue_get_kerl2mode(VOID);
ER nue_set_rstmode(NUE_SVMRST_TYPE type);
NUE_SVMRST_TYPE nue_get_rstmode(VOID);
ER nue_set_permute_mode(NUE_PERMUTE_MODE mode);
NUE_PERMUTE_MODE nue_get_permute_mode(VOID);
VOID nue_set_permute_shf(INT32 shift);
INT32 nue_get_permute_shf(VOID);
ER nue_set_roipool_mode(NUE_ROI_MODE mode);
NUE_ROI_MODE nue_get_roipool_mode(VOID);
VOID nue_set_dmao_en(BOOL enable);
BOOL nue_get_dmao_en(VOID);
ER nue_set_dmao_path(NUE_DMAO_PATH_TYPE path);
NUE_DMAO_PATH_TYPE nue_get_dmao_path(VOID);
NUE_SVM_IN_SIZE nue_get_svmsize(VOID);
ER nue_set_insize(NUE_SIZE *p_size);
NUE_SIZE nue_get_insize(VOID);
ER nue_set_roinum(UINT32 roi_num);
UINT32 nue_get_roinum(VOID);
VOID nue_set_in_rfh(BOOL enable);
BOOL nue_get_in_rfh(VOID);
ER nue_set_dmaio_addr(NUE_DMAIO_ADDR dma_addr);
NUE_DMAIO_ADDR nue_get_dmaio_addr(VOID);
BOOL nue_isenable(VOID);
ER nue_set_parm(NUE_SVMKERL_PARM *p_parm);
NUE_SVMKERL_PARM nue_get_parm(VOID);
ER nue_set_fcd_parm(NUE_FCD_PARM *p_parm);
NUE_FCD_PARM nue_get_fcd_parm(VOID);
ER nue_get_rsts(NUE_SVMRSTS *p_rsts);
ER nue_set_relu_leaky(INT32 value, UINT32 shf);
ER nue_get_relu_leaky(INT32 *p_value, UINT32 *p_shf);
ER nue_set_relu_shf(INT32 shift);
INT32 nue_get_relu_shf(VOID);
VOID nue_set_roipool_kersize(NUE_ROI_POOL_SIZE size);
NUE_ROI_POOL_SIZE nue_get_roipool_standard_kersize(VOID);
NUE_ROI_POOL_SIZE nue_get_roipool_psroi_kersize(VOID);
ER nue_set_roipool_ratio(UINT32 mul, UINT32 shf);
ER nue_get_roipool_ratio(UINT32 *p_mul, UINT32 *p_shf);
ER nue_set_roipool_shf(INT32 shift);
INT32 nue_get_roipool_shf(VOID);
ER nue_set_reorgshf(INT32 shift);
INT32 nue_get_reorgshf(VOID);
#endif//#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

#ifdef __cplusplus
}
#endif

#endif
