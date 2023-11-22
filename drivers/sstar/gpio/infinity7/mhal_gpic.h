/*
 * mhal_gpic.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MHAL_GPIC_H_
#define _MHAL_GPIC_H_

#include <asm/types.h>
#include "mdrv_types.h"

struct hal_gpic_t
{
    U32 base;
    U8  pad;
    U8  dir;
    U8  edge;
    U16 init;
    U16 target;
    U8  enable;
};

extern int MHal_GPIC_Enable(struct hal_gpic_t* hal);
extern int MHal_GPIC_Disable(struct hal_gpic_t* hal);
extern int MHal_GPIC_GetCount(struct hal_gpic_t* hal, U16* count);

#endif // _MHAL_GPIC_H_
