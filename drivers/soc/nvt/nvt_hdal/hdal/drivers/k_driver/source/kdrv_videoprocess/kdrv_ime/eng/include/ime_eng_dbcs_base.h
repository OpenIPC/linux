/*
    IME module driver

    NT98520 IME internal header file.

    @file       ime_dbcs_base.h
    @ingroup    mIIPPIME
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/


#ifndef _IME_ENG_DBCS_BASE_H_
#define _IME_ENG_DBCS_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kwrap/type.h"
#include "ime_eng_int_comm.h"

//-------------------------------------------------------------------------------
// DBCS
extern VOID ime_eng_set_dbcs_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_dbcs_center_buf_reg(UINT32 cent_u, UINT32 cent_v);
extern VOID ime_eng_set_dbcs_mode_buf_reg(UINT32 set_mode);
extern VOID ime_eng_set_dbcs_step_buf_reg(UINT32 step_y, UINT32 step_uv);
extern VOID ime_eng_set_dbcs_weight_y_buf_reg(UINT8 *p_wts);
extern VOID ime_eng_set_dbcs_weight_uv_buf_reg(UINT8 *p_wts);

#if (IME_GET_API_EN == 1)
extern UINT32 ime_eng_get_dbcs_enable_buf_reg(VOID);
#endif

#ifdef __cplusplus
}
#endif


#endif // _IME_DBCS_REG_
