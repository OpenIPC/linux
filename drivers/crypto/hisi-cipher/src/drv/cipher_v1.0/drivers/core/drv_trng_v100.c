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

#include "drv_trng_v100.h"
#include "drv_trng.h"

#ifdef CHIP_TRNG_VER_V100

/*************************** Internal Structure Definition ****************************/
/** \addtogroup      cipher drivers*/
/** @{*/  /** <!-- [cipher]*/

/*! Define the post process depth */
#define TRNG_POST_PROCESS_DEPTH     0x10

/*! Define the osc sel */
#define TRNG_OSC_SEL                0x02

/** @} */  /** <!-- ==== Structure Definition end ==== */

/******************************* API Declaration *****************************/
/** \addtogroup      hdcp */
/** @{ */  /** <!--[hdcp]*/

hi_s32 drv_trng_randnum(hi_u32 *randnum, hi_u32 timeout)
{
    rng_stat stat;
    rng_ctrl ctrl;
    hi_u32 times = 0;
    static hi_u32 last = 0x1082;

    HI_LOG_FUNC_ENTER();

    HI_LOG_CHECK_PARAM(randnum == HI_NULL);

    if (ctrl.u32 != last) {
        module_enable(CRYPTO_MODULE_ID_TRNG);

        ctrl.u32 = TRNG_READ(RNG_CTRL);
        ctrl.bits.filter_enable = 0x00;
        ctrl.bits.mix_en = 0x00;
        ctrl.bits.drop_enable = 0x00;
        ctrl.bits.post_process_enable = 0x01;
        ctrl.bits.post_process_depth = TRNG_POST_PROCESS_DEPTH;
        ctrl.bits.osc_sel = TRNG_OSC_SEL;
        TRNG_WRITE(RNG_CTRL, ctrl.u32);
    }

#if defined(HI_PLATFORM_TYPE_LINUX)
    if (timeout == 0) { /* unblock */
        /* trng number is valid ? */
        stat.u32 = TRNG_READ(RNG_STAT);
        if (stat.bits.rng_data_count == 0x00) {
            return HI_ERR_CIPHER_NO_AVAILABLE_RNG;
        }
    } else /* block */
#endif
    {
        while (times++ < TRNG_TIMEOUT) {
            /* trng number is valid ? */
            stat.u32 = TRNG_READ(RNG_STAT);
            if (stat.bits.rng_data_count > 0x00) {
                break;
            }
        }

        /* time out */
        if (times >= TRNG_TIMEOUT) {
            return HI_ERR_CIPHER_NO_AVAILABLE_RNG;
        }
    }

    /* read valid randnum */
    *randnum = TRNG_READ(RNG_FIFO_DATA);

    HI_LOG_INFO("randnum: 0x%x\n", *randnum);

    HI_LOG_FUNC_EXIT();
    return HI_SUCCESS;
}

void drv_trng_get_capacity(trng_capacity *capacity)
{
    crypto_memset(capacity, sizeof(trng_capacity), 0,  sizeof(trng_capacity));

    capacity->trng = CRYPTO_CAPACITY_SUPPORT;

    return;
}

/** @} */  /** <!-- ==== API declaration end ==== */

#endif //End of CHIP_TRNG_VER_V100
