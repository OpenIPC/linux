// $Change: 616729 $
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////


#define PQTABLE_NAME    MAIN                // table config parameter
#if defined(MSOS_TYPE_LINUX_KERNEL)
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif

#include "MsTypes.h"
#include "color_reg.h"
#include "drvPQ_Define.h"
#include "drvPQ_Declare.h"
#include "drvPQ.h"
#include "hwreg_utility2.h"

#define PQTBL_REGTYPE       PQTBL_NORMAL    // table config parameter

#if(PQ_ONLY_SUPPORT_BIN == 0)

#include "Infinity3e_Main.c"                  // table config parameter
#include "Infinity3e_Main_1920.c"             // table config parameter
#include "Infinity3e_Main_HSPRule.c"          // table config parameter
#include "Infinity3e_Main_VSPRule.c"          // table config parameter
#include "Infinity3e_Main_GRule.c"            // table config parameter
#include "Infinity3e_Main_1920_GRule.c"       // table config parameter
#endif

#include "QualityMode.h"
#include "drvPQ_Datatypes.h"
#include "mhal_pq.h"
#include "drvPQ_Template.h"
