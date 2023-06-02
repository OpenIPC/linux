#if defined(__KERNEL__)
/*
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    @file       vdoenc_builtin_export.c
    @ingroup

    @brief

    @note       Nothing.

    @version    V0.00.001
    @author     YanYu Chen
    @date       2020/06/03
*/

#include <linux/module.h>
#include "vdoenc_builtin.h"

EXPORT_SYMBOL(VdoEnc_Builtin_GetBS);
EXPORT_SYMBOL(VdoEnc_Builtin_HowManyInBSQ);
EXPORT_SYMBOL(VdoEnc_Builtin_GetEncVar);
EXPORT_SYMBOL(VdoEnc_Builtin_SetParam);
EXPORT_SYMBOL(VdoEnc_Builtin_GetParam);
EXPORT_SYMBOL(VdoEnc_Builtin_CheckBuiltinStop);
EXPORT_SYMBOL(VdoEnc_Builtin_FreeQueMem);
#endif
