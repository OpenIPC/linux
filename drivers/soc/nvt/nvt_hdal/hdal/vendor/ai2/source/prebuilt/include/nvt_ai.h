#ifndef _NVT_AI_H_
#define _NVT_AI_H_

#include "hd_type.h"
#include "kwrap/type.h"

#ifdef __cplusplus
extern "C" {
#endif
ER nn_check_net_parm(UINT32 parm_addr);

//extern HD_RESULT vendor_ai_bnscale_process(NN_BNSCALE_PARM *p_parm);
//extern HD_RESULT vendor_ai_fc_post_process(NN_FC_POST_PARM *p_parm);
//extern HD_RESULT vendor_ai_pool_process   (NN_POOL_PARM    *p_parm);
#ifdef __cplusplus
}
#endif
#endif