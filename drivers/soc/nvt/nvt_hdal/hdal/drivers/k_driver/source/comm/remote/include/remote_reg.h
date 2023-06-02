/*
    Internal register header file for remote controller module.

    This file is the internal register header file that define the API and data type for remote
    controller module.

    @file       remote.h
    @ingroup    mIREMOTE
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _REMOTE_REG_H_
#define _REMOTE_REG_H_

#include "remote_int.h"

// Remote Control Register
#define REMOTE_CONTROL_REG_OFS              0x00
REGDEF_BEGIN(REMOTE_CONTROL_REG)
REGDEF_BIT(rm_en, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(th_sel, 1)
REGDEF_BIT(rm_inv, 1)
REGDEF_BIT(rm_msb, 1)
REGDEF_BIT(rm_rep_en, 1)
REGDEF_BIT(rm_hedf_chkmethod, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(bi_phase_en, 1)
REGDEF_BIT(bi_phase_detect_header_th, 1)
#if defined (_BSP_NA51089_)
REGDEF_BIT(bi_phase_hw_detect_header, 2)
#else
REGDEF_BIT(, 2)
#endif
REGDEF_BIT(rm_length, 7)
REGDEF_BIT(, 1)
REGDEF_BIT(match_length, 7)
REGDEF_BIT(, 1)
REGDEF_END(REMOTE_CONTROL_REG)

// Remote Status Register
#define REMOTE_INTEN_REG_OFS               0x08
REGDEF_BEGIN(REMOTE_INTEN_REG)
REGDEF_BIT(rm_rd, 1)
REGDEF_BIT(rm_err, 1)
REGDEF_BIT(match, 1)
REGDEF_BIT(repeat, 1)
REGDEF_BIT(overrun, 1)
REGDEF_BIT(, 27)
REGDEF_END(REMOTE_INTEN_REG)

// Remote Status Register
#define REMOTE_STATUS_REG_OFS               0x0C
REGDEF_BEGIN(REMOTE_STATUS_REG)
REGDEF_BIT(rm_rd, 1)
REGDEF_BIT(rm_err, 1)
REGDEF_BIT(match, 1)
REGDEF_BIT(repeat, 1)
REGDEF_BIT(overrun, 1)
REGDEF_BIT(, 27)
REGDEF_END(REMOTE_STATUS_REG)


// Remote Low Data Register
#define REMOTE_LOW_DATA_REG_OFS             0x10
REGDEF_BEGIN(REMOTE_LOW_DATA_REG)
REGDEF_BIT(rm_rdr_l, 32)
REGDEF_END(REMOTE_LOW_DATA_REG)

// Remote High Data Register
#define REMOTE_HIGH_DATA_REG_OFS            0x14
REGDEF_BEGIN(REMOTE_HIGH_DATA_REG)
REGDEF_BIT(rm_rdr_h, 32)
REGDEF_END(REMOTE_HIGH_DATA_REG)

// Remote MATCH Low Data Register
#define REMOTE_MATCH_LOW_DATA_REG_OFS       0x18
REGDEF_BEGIN(REMOTE_MATCH_LOW_DATA_REG)
REGDEF_BIT(match_data_l, 32)
REGDEF_END(REMOTE_MATCH_LOW_DATA_REG)

// Remote MATCH High Data Register
#define REMOTE_MATCH_HIGH_DATA_REG_OFS      0x1C
REGDEF_BEGIN(REMOTE_MATCH_HIGH_DATA_REG)
REGDEF_BIT(match_data_h, 32)
REGDEF_END(REMOTE_MATCH_HIGH_DATA_REG)

// Remote Threshold Register 0
#define REMOTE_TH0_REG_OFS                  0x20
REGDEF_BEGIN(REMOTE_TH0_REG)
REGDEF_BIT(header_th, 16)
REGDEF_BIT(rep_th, 16)
REGDEF_END(REMOTE_TH0_REG)

// Remote Threshold Register 1
#define REMOTE_TH1_REG_OFS                  0x24
REGDEF_BEGIN(REMOTE_TH1_REG)
#if defined (_BSP_NA51089_)
REGDEF_BIT(logic_th, 12)
REGDEF_BIT(, 4)
#else
REGDEF_BIT(logic_th, 11)
REGDEF_BIT(, 5)
#endif
REGDEF_BIT(rm_gsr, 8)
REGDEF_BIT(, 8)
REGDEF_END(REMOTE_TH1_REG)

// Remote Threshold Register 2
#define REMOTE_TH2_REG_OFS                  0x28
REGDEF_BEGIN(REMOTE_TH2_REG)
REGDEF_BIT(error_th, 16)
REGDEF_BIT(, 16)
REGDEF_END(REMOTE_TH2_REG)

// Remote RAW Low Data Register
#define REMOTE_RAW_LOW_DATA_REG_OFS         0x2C
REGDEF_BEGIN(REMOTE_RAW_LOW_DATA_REG)
REGDEF_BIT(rm_raw_rdr_l, 32)
REGDEF_END(REMOTE_RAW_LOW_DATA_REG)

// Remote RAW High Data Register
#define REMOTE_RAW_HIGH_DATA_REG_OFS        0x30
REGDEF_BEGIN(REMOTE_RAW_HIGH_DATA_REG)
REGDEF_BIT(rm_raw_rdr_h, 32)
REGDEF_END(REMOTE_RAW_HIGH_DATA_REG)

#endif
