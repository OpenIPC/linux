#ifndef _KDRV_AI_INT_H_
#define _KDRV_AI_INT_H_

#include "kdrv_ai.h"
#include "cnn_lib.h"
#include "nue_lib.h"
#include "nue2_lib.h"

ER kdrv_ai_tran_cnn_parm(KDRV_AI_NEURAL_PARM *p_ai_parm, CNN_PARM *p_eng_parm, UINT32 func_en);
ER kdrv_ai_tran_roipool_parm(KDRV_AI_ROIPOOL_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_svm_parm(KDRV_AI_SVM_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_nue_fc_parm(KDRV_AI_FC_PARM *p_ai_parm, NUE_PARM *p_eng_parm, UINT32 func_en);
ER kdrv_ai_tran_permute_parm(KDRV_AI_PERMUTE_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_reorg_parm(KDRV_AI_REORG_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_anchor_parm(KDRV_AI_ANCHOR_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_softmax_parm(KDRV_AI_SOFTMAX_PARM *p_ai_parm, NUE_PARM *p_eng_parm);
ER kdrv_ai_tran_nue2_preproc_parm(KDRV_AI_PREPROC_PARM *p_ai_parm, NUE2_PARM *p_eng_parm, UINT32 func_en);
#endif