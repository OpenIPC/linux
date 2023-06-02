////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2012 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (MStar Confidential Information) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MS_TYPES_H__
#define __MS_TYPES_H__

#include <linux/types.h>

#ifndef FALSE
#define FALSE 	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef U8
#define U8      u8
#endif

#ifndef U16
#define U16     u16
#endif

#ifndef U32
#define U32     u32
#endif

#if defined(__cplusplus) && defined(__GNUC__) && (__GNUC__>=4) && (__GNUC_MINOR__>=6)
#define opaque_enum(e) enum e : int
#else
#define opaque_enum(e) enum e
#endif
#define typedef_opaque_enum(e1, e2) \
    opaque_enum(e1); \
    typedef enum e1 e2


#endif

