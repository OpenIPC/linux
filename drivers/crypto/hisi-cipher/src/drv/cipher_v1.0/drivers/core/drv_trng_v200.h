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

#ifndef _DRV_TRNG_V200_H_
#define _DRV_TRNG_V200_H_

#include "drv_osal_lib.h"

/*************************** Internal Structure Definition ****************************/
/** \addtogroup      trng drivers*/
/** @{*/  /** <!-- [trng]*/

/*! \Define the offset of reg */
#define  HISEC_COM_TRNG_CTRL            (0x00)
#define  HISEC_COM_TRNG_FIFO_DATA       (0x04)
#define  HISEC_COM_TRNG_DATA_ST         (0x08)

/* Define the union hisec_com_trng_ctrl */
typedef union {
    /* Define the struct bits */
    struct {
        hi_u32   osc_sel            :  2;   /* [1..0]  */
        hi_u32   cleardata          :  1;   /* [2]  */
        hi_u32   drbg_enable        :  1;   /* [3]  */
        hi_u32   pre_process_enable :  1;   /* [4]  */
        hi_u32   drop_enable        :  1;   /* [5]  */
        hi_u32   mix_enable         :  1;   /* [6]  */
        hi_u32   post_process_enable:  1;   /* [7]  */
        hi_u32   post_process_depth :  8;   /* [15..8]  */
        hi_u32   reserved0          :  1;   /* [16]  */
        hi_u32   trng_sel           :  2;   /* [18..17]  */
        hi_u32   pos_self_test_en   :  1;   /* [19]  */
        hi_u32   pre_self_test_en     :  1;   /* [20]  */
        hi_u32   reserved1          :  11;  /* [31..21]  */
    } bits;

    /* Define an unsigned member */
    hi_u32    u32;

} hisec_com_trng_ctrl;

/* Define the union hisec_com_trng_data_st */
typedef union {
    /* Define the struct bits */
    struct {
        hi_u32    low_osc_st0        :    1; /* [0]  */
        hi_u32    low_osc_st1        :    1; /* [1]  */
        hi_u32    low_ro_st0         :    1; /* [2]  */
        hi_u32    low_ro_st1         :    1; /* [3]  */
        hi_u32    otp_trng_sel       :    1; /* [4]  */
        hi_u32    reserved0          :    3; /* [7..5]  */
        hi_u32    trng_fifo_data_cnt :    8; /* [15..8]  */
        hi_u32    sic_trng_alarm     :    6; /* [21..16]  */
        hi_u32    sic_trng_bist_alarm:    1; /* [22]  */
        hi_u32    reserved1          :    9; /* [31..23]  */
    } bits;

    /* Define an unsigned member */
    hi_u32    u32;

} hisec_com_trng_data_st;

/** @}*/  /** <!-- ==== Structure Definition end ====*/
#endif
