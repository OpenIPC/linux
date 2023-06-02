/**
	@brief Header file of internal definition of vendor net flow sample.

	@file net_flow_sample_int.h

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _NET_FLOW_SAMPLE_INT_H_
#define _NET_FLOW_SAMPLE_INT_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/nn_net.h"

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
ER nvt_ai_chk_vers(VENDOR_AIS_FLOW_VERS *p_vers_info);

//init/uninit/reset
ER nvt_ai_reset_net(void);
ER nvt_ai_init_net(VENDOR_AIS_FLOW_ID *id_info);
ER nvt_ai_uninit_net(void);

//lock/unlock for state open/start
ER nvt_ai_lock_net(UINT32 net_id);
ER nvt_ai_unlock_net(UINT32 net_id);

ER nvt_ai_open_net(VENDOR_AIS_FLOW_MAP_MEM_PARM  *p_mem, UINT32 net_id);
ER nvt_ai_close_net(UINT32 net_id);
ER nvt_ai_copy_net_from_user(UINT32 net_id);

//UINT32 vendor_ais_load_net(UINT32 addr);
ER nvt_ai_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id);
//VOID vendor_ais_proc_net(INT32 in_img, UINT32 start_addr, UINT32 end_addr);
ER nvt_ai_proc_net(VENDOR_AIS_FLOW_PROC_PARM *p_parm, UINT32 net_id);

ER nvt_ai_update_layer(UINT32 layer, UINT32 net_id);
ER nvt_ai_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
ER nvt_ai_set_ll_base(VENDOR_AIS_FLOW_LL_BASE *p_ll_base);
#endif
ER nvt_ai_set_mem_ofs(VENDOR_AIS_FLOW_MEM_OFS *p_mem_ofs);

#if !CNN_25_MATLAB
ER nvt_ai_unpars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id);
ER nvt_ai_set_input(UINT32 net_addr, NN_DATA *p_imem, UINT32 imem_cnt, UINT32 net_id);
#if CNN_MULTI_INPUT
ER nvt_ai_set_input2(UINT32 net_addr, NN_DATA *p_imem, UINT32 imem_cnt, UINT32 proc_idx, UINT32 net_id);
#endif
#else
ER nvt_ai_set_input(UINT32 net_addr, NN_IOMEM *p_in_io_mem, UINT32 net_id);
#endif
ER nvt_ai_clr_input(UINT32 net_addr, UINT32 net_id);
#if CNN_MULTI_INPUT
ER nvt_ai_clr_input2(UINT32 net_addr, UINT32 proc_idx, UINT32 net_id);
ER nvt_ai_clr_all_input2(UINT32 net_addr, UINT32 proc_idx, UINT32 net_id);
#endif

#endif  /* _NET_FLOW_SAMPLE_INT_H_ */
