////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sigmastar Technology Corp.
// All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef IR_CONFIG_H
#define IR_CONFIG_H

#define IR_TYPE_OLD                 0
#define IR_TYPE_NEW                 1
#define IR_TYPE_MSTAR_DTV           2
#define IR_TYPE_MSTAR_RAW		    3
#define IR_TYPE_RC_V16              4
#define IR_TYPE_CUS03_DTV           5
#define IR_TYPE_MSTAR_FANTASY       6
#define IR_TYPE_MSTAR_SZ1           7
#define IR_TYPE_CUS08_RC5           8
#define IR_TYPE_CUS21SH             9
#define IR_TYPE_RCMM               10

#define IR_TYPE_SEL  IR_TYPE_MSTAR_DTV

#if   (IR_TYPE_SEL == IR_TYPE_MSTAR_DTV)
#include "IR_MSTAR_DTV.h"
#elif (IR_TYPE_SEL == IR_TYPE_RCMM)
#include "IR_RCMM.h"
#else
#include "IR_MSTAR_DTV.h"
#endif

#endif
