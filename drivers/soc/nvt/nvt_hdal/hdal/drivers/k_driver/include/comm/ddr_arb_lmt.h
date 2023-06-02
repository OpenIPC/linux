/**
 * @file ddr_arb_lmt.h
 * @brief parameter limitation of DDR ARB.
 * @author ESW
 * @date in the year 2019
 */

#ifndef __DDR_ARB_LIMIT_H__
#define __DDR_ARB_LIMIT_H__

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

// protect limitation
#define PROTECT_ADDR_ALIGN			DRV_LIMIT_ALIGN_4WORD
#define PROTECT_SIZE_ALIGN			DRV_LIMIT_ALIGN_4WORD

#endif
