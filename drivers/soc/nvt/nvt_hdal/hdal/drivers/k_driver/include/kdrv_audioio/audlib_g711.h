/**
    @file       G711.h
    @ingroup    mIAVG711

    @brief      Header file of G.711 u-law, A-law Encode/Decode API Library.

    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2015.  All rights reserved.
*/

/**
    @addtogroup mIAVG711
*/
//@{

#ifndef _AUDLIB_G711_H
#define _AUDLIB_G711_H

#ifdef __KERNEL__
#include "kdrv_type.h"
#else
#include "kdrv_type.h"
#endif
/**
    G.711 u-Law encode function

    G.711 u-Law encode function.

    @param[in]  p_data_in         16-bit audio data input
    @param[out] p_data_out        8-bit audio data output
    @param[in]  sample_count   audio sample count
    @param[in]  input_swap         input data swap or not

    @return error code
*/
extern ER g711_ulaw_encode(INT16 *p_data_in, UINT8 *p_data_out, UINT32 sample_count, BOOL input_swap);

/**
    G.711 u-Law decode function

    G.711 u-Law decode function.

    @param[in]  p_data_in         8-bit audio data input
    @param[out] p_data_out        16-bit audio data output
    @param[in]  sample_count   audio sample count
    @param[in]  duplicate_channel          duplicate channel or not
    @param[in]  output_swap        output data swap or not

    @return error code
*/
extern ER g711_ulaw_decode(UINT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, BOOL duplicate_channel, BOOL output_swap);

/**
    G.711 A-Law encode function

    G.711 A-Law encode function.

    @param[in]  p_data_in         16-bit audio data input
    @param[out] p_data_out        8-bit audio data output
    @param[in]  sample_count   audio sample count
    @param[in]  input_swap         input data swap or not

    @return error code
*/
extern ER g711_alaw_encode(INT16 *p_data_in, UINT8 *p_data_out, UINT32 sample_count, BOOL input_swap);

/**
    G.711 A-Law decode function

    G.711 A-Law decode function.

    @param[in]  p_data_in         8-bit audio data input
    @param[out] p_data_out        16-bit audio data output
    @param[in]  sample_count   audio sample count
    @param[in]  duplicate_channel          duplicate channel or not
    @param[in]  output_swap        output data swap or not

    @return error code
*/
extern ER g711_alaw_decode(UINT8 *p_data_in, INT16 *p_data_out, UINT32 sample_count, BOOL duplicate_channel, BOOL output_swap);

//@}
int kdrv_audlib_g711_init(void);

#endif
