/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CRYP_TRNG_H_
#define __CRYP_TRNG_H_

#include "drv_osal_lib.h"
#include "drv_trng.h"

/******************************* API Declaration *****************************/
/** \addtogroup      trng */
/** @{ */  /** <!--[trng]*/

/** @}*/  /** <!-- ==== Structure Definition end ====*/

/******************************* API Code *****************************/
/** \addtogroup      trng drivers*/
/** @{*/  /** <!-- [trng]*/

/**
\brief get rand number.
\param[out]  randnum rand number.
\param[in]   timeout time out.
\retval     On success, HI_SUCCESS is returned.  On error, HI_FAILURE is returned.
*/
hi_s32 cryp_trng_get_random( hi_u32 *randnum, hi_u32 timeout );

/** @} */  /** <!-- ==== API declaration end ==== */
#endif
