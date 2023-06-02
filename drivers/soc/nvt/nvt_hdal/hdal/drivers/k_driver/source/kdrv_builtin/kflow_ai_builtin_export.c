#if defined(__KERNEL__)
/*
    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.

    @file       kflow_ai_builtin_export.c
    @ingroup

    @note       Nothing.
*/

#include <linux/module.h>
#include "kflow_ai_builtin.h"

/*EXPORT_SYMBOL(VdoEnc_Builtin_GetBS);
EXPORT_SYMBOL(VdoEnc_Builtin_HowManyInBSQ);
EXPORT_SYMBOL(VdoEnc_Builtin_GetEncVar);
EXPORT_SYMBOL(VdoEnc_Builtin_SetParam);
EXPORT_SYMBOL(VdoEnc_Builtin_GetParam);
EXPORT_SYMBOL(VdoEnc_Builtin_CheckBuiltinStop);
EXPORT_SYMBOL(VdoEnc_Builtin_FreeQueMem);
EXPORT_SYMBOL(vdoenc_builtin_evt_fp_reg);
EXPORT_SYMBOL(vdoenc_builtin_evt_fp_unreg);
EXPORT_SYMBOL(vdoenc_builtin_set);*/

EXPORT_SYMBOL(kflow_ai_builtin_get_rslt);
EXPORT_SYMBOL(kflow_ai_builtin_how_many_in_rsltq);
EXPORT_SYMBOL(kflow_ai_builtin_free_rslt_que_mem);
EXPORT_SYMBOL(kflow_ai_builtin_exit);
EXPORT_SYMBOL(kflow_ai_builtin_get_builtin_mem);
#endif
