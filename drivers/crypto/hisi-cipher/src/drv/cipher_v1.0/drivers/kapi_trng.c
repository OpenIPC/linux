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

#include "drv_osal_lib.h"
#include "cryp_trng.h"

/** @}*/  /** <!-- ==== Structure Definition end ====*/


/******************************* API Code *****************************/
/** \addtogroup      trng */
/** @{*/  /** <!-- [kapi]*/

hi_s32 kapi_trng_get_random(hi_u32 *randnum, hi_u32 timeout)
{
    hi_s32 ret = HI_FAILURE;

    HI_LOG_FUNC_ENTER();

    HI_LOG_CHECK_PARAM(randnum == HI_NULL);

    ret = cryp_trng_get_random(randnum, timeout);
    if (ret != HI_SUCCESS) {
        return ret;
    }

    HI_LOG_FUNC_EXIT();
    return HI_SUCCESS;
}

/** @}*/  /** <!-- ==== Structure Definition end ====*/
