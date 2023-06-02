/*
CNN module driver

NT98520 CNN registers header file.

@file       cnn_int.h
@ingroup    mIIPPCNN
@note       Nothing

Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CNN_INT_H
#define _CNN_INT_H

#ifdef __cplusplus
extern "C" {
#endif
#include    "cnn_lmt.h"
#include    "cnn_lib.h"

	extern UINT32 cnn_reg_base_addr1;
	extern UINT32 cnn_reg_base_addr2;

#define CNN_DMA_CACHE_HANDLE    (1)

#define CNN_IOADDR_REG_BASE1    cnn_reg_base_addr1
#define CNN_IOADDR_REG_BASE2    cnn_reg_base_addr2


#if defined(__FREERTOS)
#define CNN_SETDATA(ofs, value, addr)   OUTW((addr + ofs), value)
#define CNN_GETDATA(ofs, addr)          INW(addr + ofs)
#else
#define CNN_SETDATA(ofs, value, addr)   iowrite32(value, (VOID*)(addr + ofs))
#define CNN_GETDATA(ofs, addr)          ioread32((VOID*)(addr + ofs))
#endif

	/*
	CNN State machine
	*/
	typedef enum {
		CNN_ENGINE_IDLE    = 0,
		CNN_ENGINE_READY   = 1,
		CNN_ENGINE_RUN     = 2,
		CNN_ENGINE_PAUSE   = 3,
		ENUM_DUMMY4WORD(CNN_ENGINE_STATUS)
	} CNN_ENGINE_STATUS;

	/*
	CNN Engine Control
	*/
	typedef enum {
		CNN_CTRL_RESET     = 0, ///< contrl engine reset
		CNN_CTRL_START     = 1, ///< contrl engine start
		CNN_CTRL_LOAD      = 2, ///< contrl engine load
		CNN_CTRL_INTPEN    = 8, ///< contrl engine interrupt enable
		CNN_CTRL_INTP      = 9, ///< contrl engine interrupt status
		CNN_CTRL_OUTDRAM   = 10, ///< contrl engine output dram when direct mode
		ENUM_DUMMY4WORD(CNN_ENGINE_CTRL_SEL)
	} CNN_ENGINE_CTRL_SEL;
	//------------------------------------------------------------------

	/*
	CNN Operation
	*/
	typedef enum {
		CNN_OP_OPEN             = 0,
		CNN_OP_CLOSE            = 1,
		CNN_OP_SET2READY        = 2,   ///< set module to ready state
		CNN_OP_SET2PAUSE        = 3,   ///< set module to pause
		CNN_OP_SET2RUN          = 4,   ///< set module to run
		CNN_OP_ATTACH           = 5,
		CNN_OP_DETACH           = 6,
		CNN_OP_READLUT          = 7,
		CNN_OP_CHGINT           = 8,
		ENUM_DUMMY4WORD(CNN_OPERATION)
	} CNN_OPERATION;


	UINT32 cnn_getcycle(BOOL cnn_id);
	//
	VOID cnn_enable_int(BOOL cnn_id, BOOL enable, UINT32 intr);
	UINT32 cnn_get_int_enable(BOOL cnn_id);
	VOID cnn_clr_intr_status(BOOL cnn_id, UINT32 status);
	UINT32 cnn_get_intr_status(BOOL cnn_id);
	VOID cnn_clr(BOOL cnn_id, BOOL reset);
	VOID cnn_ll_enable(BOOL cnn_id, BOOL start);
	VOID cnn_ll_terminate(BOOL cnn_id, BOOL isterminate);
	//
	ER cnn_set_out0mode(BOOL cnn_id, CNN_OUT0_TYPE mode);
	//
	CNN_OUT0_TYPE cnn_get_out0mode(BOOL cnn_id);
	//
	VOID cnn_set_dmain_lladdr(BOOL cnn_id, UINT32 addr0);
	UINT32 cnn_get_dmain_lladdr(BOOL cnn_id);
	VOID cnn_set_dmain_lladdr_base(BOOL cnn_id, UINT32 addr0);
	UINT32 cnn_get_dmain_lladdr_base(BOOL cnn_id);
	ER cnn_set_out_scale(BOOL cnn_id, CNN_OUT_SCALE_PARM *p_size);
	CNN_OUT_SCALE_PARM cnn_get_out_scale(BOOL cnn_id);
	ER cnn_set_convparm(BOOL cnn_id, CNN_CONVKERL_PARM *p_parm);
	CNN_CONVKERL_PARM cnn_get_convparm(BOOL cnn_id);
	//ER cnn_set_clamp(BOOL cnn_id, CNN_CLAMP_PARM *p_parm);
	//ER cnn_get_clamp(BOOL cnn_id, CNN_CLAMP_PARM *p_parm);
	//ER cnn_get_clamp_rst(BOOL cnn_id, CNN_OUT_CLAMP_RST *p_parm);
	//
	ER cnn_set_bnscale_parm(BOOL cnn_id, CNN_BNSCALE_PARM *p_parm);
	CNN_BNSCALE_PARM cnn_get_bnscale_parm(BOOL cnn_id);

	ER cnn_set_prerelu_parm(BOOL cnn_id, CNN_RELU_IN *p_parm);
	CNN_RELU_IN cnn_get_prerelu_parm(BOOL cnn_id);

	ER cnn_set_relu0_parm(BOOL cnn_id, CNN_RELU_IN *p_parm);
	CNN_RELU_IN cnn_get_relu0_parm(BOOL cnn_id);

	ER cnn_set_relu1_parm(BOOL cnn_id, CNN_RELU_IN *p_parm);
	CNN_RELU_IN cnn_get_relu1_parm(BOOL cnn_id);

	ER cnn_set_deconvparm(BOOL cnn_id, CNN_DECONV_PARM *p_parm);
	CNN_DECONV_PARM cnn_get_deconvparm(BOOL cnn_id);
	ER cnn_set_scaleupparm(BOOL cnn_id, CNN_SCALEUP_PARM *p_parm);
	CNN_SCALEUP_PARM cnn_get_scaleupparm(BOOL cnn_id);

	//ER cnn_set_lrnparm(BOOL cnn_id, CNN_LRN_PARM *p_parm);
	//CNN_LRN_PARM cnn_get_lrnparm(BOOL cnn_id);

	ER cnn_set_poolkertype(BOOL cnn_id, CNN_POOL_KER type);
	ER cnn_set_localpool_stride(BOOL cnn_id, CNN_POOL_KER_STRIDE stride);
	CNN_POOL_KER_STRIDE cnn_get_localpool_stride(BOOL cnn_id);
	ER cnn_set_localpool_kersize(BOOL cnn_id, CNN_POOL_KER_SIZE kersize);
	CNN_POOL_KER_SIZE cnn_get_localpool_kersize(BOOL cnn_id);
	VOID cnn_set_poolavedivtype(BOOL cnn_id, CNN_POOL_AVE_DIV_TYPE ave_div_type);
	CNN_POOL_AVE_DIV_TYPE cnn_get_poolavedivtype(BOOL cnn_id);
	VOID cnn_set_localpool_out_cal_type(BOOL cnn_id, CNN_POOL_OUT_CAL_TYPE out_cal_type);
	CNN_POOL_OUT_CAL_TYPE cnn_get_localpool_out_cal_type(BOOL cnn_id);
	VOID cnn_set_localpool_pad(BOOL cnn_id, BOOL padtop, BOOL padleft, BOOL padbot, BOOL padright);
	ER cnn_get_localpool_pad(BOOL cnn_id, BOOL *p_padtop, BOOL *p_padleft, BOOL *p_padbot, BOOL *p_padright);
	ER cnn_set_localpool_parm(BOOL cnn_id, CNN_LOCAL_POOL_PARM *p_parm);
	CNN_LOCAL_POOL_PARM cnn_get_localpool_parm(BOOL cnn_id);
	ER cnn_set_globalpool_avg(BOOL cnn_id, UINT32 mul, UINT32 shf);
	ER cnn_get_globalpool_avg(BOOL cnn_id, UINT32 *p_mul, UINT32 *p_shf);
	ER cnn_set_globalpool_parm(BOOL cnn_id, CNN_GLOBAL_POOL_PARM *p_parm);
	CNN_GLOBAL_POOL_PARM cnn_get_globalpool_parm(BOOL cnn_id);
	ER cnn_set_poolshift(BOOL cnn_id, BOOL pool_shf_dir, UINT32 pool_shf);
	ER cnn_get_poolshift(BOOL cnn_id, BOOL *p_pool_shf_dir, UINT32 *p_pool_shf);
	ER cnn_set_elt_parm(BOOL cnn_id, CNN_ELTWISE_PARM *p_parm);
	CNN_ELTWISE_PARM cnn_get_elt_parm(BOOL cnn_id);
	
	BOOL cnn_get_engine_idle(BOOL cnn_id);
	BOOL cnn_get_dma_disable(BOOL cnn_id);
	VOID cnn_set_dma_disable(BOOL cnn_id, BOOL disable);

	//Change data format
	UINT32 cnn_tran_intval(INT32 value, UINT32 bits);
	INT32 cnn_tran_bitval(UINT32 value, UINT32 bits);
	//For FPGA verify onlyA

	//checksum
	VOID cnn_set_act0_chksum(BOOL cnn_id, CNN_CHKSUM_PARM *p_parm);
	VOID cnn_set_act1_chksum(BOOL cnn_id, CNN_CHKSUM_PARM *p_parm);
	UINT32 cnn_get_act0_chksum(BOOL cnn_id);
	UINT32 cnn_get_act1_chksum(BOOL cnn_id);
	VOID cnn_set_pool_chksum(BOOL cnn_id, CNN_CHKSUM_PARM *p_parm);
	UINT32 cnn_get_pool_chksum(BOOL cnn_id);
	UINT32 cnn_get_chksum_mismatch_idx(BOOL cnn_id);
	

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
	CNN_POOLING_TYPE cnn_get_poolmode(BOOL cnn_id);
	CNN_LUT_MODE_TYPE cnn_get_lutmode(BOOL cnn_id);
	ER cnn_set_poolmode(BOOL cnn_id, CNN_POOLING_TYPE mode);
	ER cnn_set_lutmode(BOOL cnn_id, CNN_LUT_MODE_TYPE mode);

	ER cnn_set_actmode(BOOL cnn_id, CNN_ACT_MODE_TYPE mode);
	CNN_ACT_MODE_TYPE cnn_get_actmode(BOOL cnn_id);
	ER cnn_set_eltmode(BOOL cnn_id, CNN_ELT_MODE_TYPE mode);
	CNN_ELT_MODE_TYPE cnn_get_eltmode(BOOL cnn_id);
	VOID cnn_enable(BOOL cnn_id, BOOL start);
	ER cnn_set_engmode(BOOL cnn_id, CNN_MODE_TYPE mode);
	CNN_MODE_TYPE cnn_get_engmode(BOOL cnn_id);
	VOID cnn_set_kerl_en(BOOL cnn_id, BOOL enable, UINT32 kerl);
	UINT32 cnn_get_kerl_en(BOOL cnn_id);
	ER cnn_set_intype(BOOL cnn_id, CNN_IO_TYPE in_type);
	VOID cnn_set_eltwise_intype(BOOL cnn_id, CNN_IO_TYPE elttype);
	ER cnn_set_out1type(BOOL cnn_id, CNN_IO_TYPE out_type);
	ER cnn_set_out0type(BOOL cnn_id, CNN_IO_TYPE out_type);
	CNN_IO_TYPE cnn_get_intype(BOOL cnn_id);
	CNN_IO_TYPE cnn_get_elttype(BOOL cnn_id);
	CNN_IO_TYPE cnn_get_out0type(BOOL cnn_id);
	CNN_IO_TYPE cnn_get_out1type(BOOL cnn_id);
	ER cnn_set_out0ofs(BOOL cnn_id, CNN_OUT_SIZE *p_parm);
	CNN_OUT_SIZE cnn_get_out0ofs(BOOL cnn_id);
	ER cnn_set_dmaio_addr(BOOL cnn_id, CNN_DMAIO_ADDR dma_addr);
	ER cnn_set_dmain_addr(BOOL cnn_id, UINT32 addr0, UINT32 addr1, UINT32 addr2, UINT32 addr3, UINT32 addr4);
	UINT32 cnn_get_dmain_addr(BOOL cnn_id, CNN_IN_BUFID buf_id);
	ER cnn_set_dma_lofs(BOOL cnn_id, CNN_DMAIO_LOFS *p_lofs);
	CNN_DMAIO_LOFS cnn_get_dmain_lofs(BOOL cnn_id);
	ER cnn_set_dmaout0_addr(BOOL cnn_id, UINT32 addr0);
	UINT32 cnn_get_dmaout0_addr(BOOL cnn_id);
	ER cnn_set_insize(BOOL cnn_id, CNN_IN_SIZE *p_size);
	CNN_IN_SIZE cnn_get_insize(BOOL cnn_id);
	ER cnn_set_fcd_parm(BOOL cnn_id, CNN_FCD_PARM *p_parm);
	CNN_FCD_PARM cnn_get_fcd_parm(BOOL cnn_id);
	ER cnn_set_fcd_en(BOOL cnn_id, CNN_FCD_PARM *p_parm);
	ER cnn_get_fcd_en(BOOL cnn_id, CNN_FCD_PARM *p_parm);
	ER cnn_set_dmaout1_addr(BOOL cnn_id, UINT32 addr0);
	UINT32 cnn_get_dmaout1_addr(BOOL cnn_id);
	VOID cnn_set_io_enable(BOOL cnn_id, BOOL enable, UINT32 io);
	UINT32 cnn_get_io_enable(BOOL cnn_id);
#endif


#ifdef __cplusplus
}
#endif

#endif
