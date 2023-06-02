/**
	@brief Source file of Custom NN pooling layer operating using DSP.

	@file custnn_dsp_pool.c

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#if NN_USE_DSP
#include "vendor_ai_dsp_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_dsp_custnn_sample.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define POOL_PRINT_PARM     FALSE

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ai_dsp_cust_pool(NN_POOL_PARM *p_parm, UINT32 in_pa, UINT32 out_pa)
{
	UINT32 in_size = p_parm->in_ofs.batch_ofs * p_parm->batch_num;
	UINT32 out_size = p_parm->out_ofs.batch_ofs * p_parm->batch_num;
	CUSTNN_STA sta;

#if POOL_PRINT_PARM
	DBGD(p_parm->in_type);
	DBGD(p_parm->out_type);
	DBGD(p_parm->size.width);
	DBGD(p_parm->size.height);
	DBGD(p_parm->size.channel);
	DBGD(p_parm->batch_num);
	DBGD(p_parm->in_ofs.line_ofs);
	DBGD(p_parm->in_ofs.channel_ofs);
	DBGD(p_parm->in_ofs.batch_ofs);
	DBGD(p_parm->out_ofs.line_ofs);
	DBGD(p_parm->out_ofs.channel_ofs);
	DBGD(p_parm->out_ofs.batch_ofs);
	DBGD(p_parm->pool.mode);
	DBGD(p_parm->pool.local.ker_w);
	DBGD(p_parm->pool.local.ker_h);
	DBGD(p_parm->pool.local.ker_stridex);
	DBGD(p_parm->pool.local.ker_stridey);
	DBGD(p_parm->pool.local.pad.top_pad_num);
	DBGD(p_parm->pool.local.pad.bot_pad_num);
	DBGD(p_parm->pool.local.pad.left_pad_num);
	DBGD(p_parm->pool.local.pad.right_pad_num);
	DBGD(p_parm->pool.local.pad.pad_val);
	DBGD(p_parm->pool.global.avg_mul);
	DBGD(p_parm->pool.global.avg_shf);
	DBGD(p_parm->pool.pool_shf);
	DBGD(p_parm->norm_scale);
	DBGD(p_parm->norm_shift);

	DBGH(p_parm->in_addr);
	DBGH(p_parm->out_addr);
	DBGH(in_pa);
	DBGH(out_pa);
#endif

	//hd_common_mem_flush_cache((void *)p_parm->in_addr, in_size); //removed due to duplicated
	//hd_common_mem_flush_cache((void *)p_parm->out_addr, out_size); //removed due to duplicated
	p_parm->in_addr = in_pa;
	p_parm->out_addr = out_pa;

	sta = custnn_dsp_pool_process(p_parm, CUSTNN_SENDTO_DSP1);
	if (sta != CUSTNN_STA_OK) {
		printf("status = %d\r\n", sta);
		return HD_ERR_FAIL;
	}

	return HD_OK;
}
#endif // NN_USE_DSP
