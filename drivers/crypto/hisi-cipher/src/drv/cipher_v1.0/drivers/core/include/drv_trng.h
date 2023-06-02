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

#ifndef __DRV_TRNG_H__
#define __DRV_TRNG_H__

/*************************** Structure Definition ****************************/
/** \addtogroup     rsa */
/** @{ */  /** <!-- [rsa] */

/*! \rsa capacity, 0-nonsupport, 1-support */
typedef struct {
    hi_u32 trng         : 1 ;    /*!<  Support TRNG */
} trng_capacity;

/** @} */  /** <!-- ==== Structure Definition end ==== */

/******************************* API Declaration *****************************/
/** \addtogroup      trng */
/** @{ */  /** <!--[trng]*/


/**
\brief get rand number.
\param[out]  randnum rand number.
\param[in]   timeout time out.
\retval     On success, HI_SUCCESS is returned.  On error, HI_FAILURE is returned.
*/
hi_s32 drv_trng_randnum(hi_u32 *randnum, hi_u32 timeout);

/**
\brief  get the trng capacity.
\param[out] capacity The hash capacity.
\retval     NA.
*/
void drv_trng_get_capacity(trng_capacity *capacity);

/** @} */  /** <!-- ==== API declaration end ==== */

#endif
