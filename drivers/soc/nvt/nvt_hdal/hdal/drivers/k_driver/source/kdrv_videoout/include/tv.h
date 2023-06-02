/**
    Header file for TV module

    Header file for TV module.
    The user can reference this section for the detail description of the each driver API usage
    and also the parameter descriptions and its limitations.
    The overall combinational usage flow is introduced in the application note document,
    and the user must reference to the application note for the driver usage flow.

    @file       tv.h
    @ingroup    mIDrvDisp_TV
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

#ifndef __TV_H__
#define __TV_H__

#ifdef __KERNEL__
#include "kwrap/type.h"
#endif
#include "ide.h"

#if 0
/**
    TV src select.

    Select the TV src.
*/
//@{
typedef enum {
	IDE_TV_SRC_IDE = 0,     ///< TV source is IDE1
	IDE_TV_SRC_IDE2 = 1,     ///< TV source is IDE2

	ENUM_DUMMY4WORD(IDE_TV_SRC)
} IDE_TV_SRC;
//@}


/**
    TV mode select.

    Select the TV mode.
*/
//@{
typedef enum {
	IDE_TV_NTSC_M = 0,      ///< TV mode is NTSC(M)
	IDE_TV_PAL_BDGHI = 1,   ///< TV mode is PAL(BDGHI)
	IDE_TV_PAL_M = 2,       ///< TV mode is PAL(M)
	IDE_TV_PAL_NC = 3,       ///< TV mode is PAL(NC)

	ENUM_DUMMY4WORD(IDE_TV_MODE)
} IDE_TV_MODE;
//@}


/**
   Defines TV Test mode

   Defines TV Test mode
*/
//@{
typedef enum {
	IDETV_DAC_TESTMODE_SEL_COLORBAR = 0,    ///< TV test mode color bar
	IDETV_DAC_TESTMODE_SEL_RAMP,            ///< TV test mode ramp
	IDETV_DAC_TESTMODE_SEL_FIX,             ///< TV test mode fix value

	ENUM_DUMMY4WORD(IDETV_DAC_TESTMODE_SEL)
} IDETV_DAC_TESTMODE_SEL;
//@}
#endif

//-------------------------------------------------
// TV Encoder register programm APIs
//-------------------------------------------------
#ifdef __KERNEL__
extern void tv_create_resource(void);
extern void tv_release_resource(void);
extern void tv_set_base_addr(UINT32 addr);
#endif
ER   tv_open(void);
ER   tv_close(void);
BOOL tv_is_opened(void);
//void ide_set_tv_use_post_icst(BOOL b_sel);
//void ide_set_tv_use_post_gamma(BOOL b_sel);
//void ide_set_tv_out_offset(UINT32 ui_y, UINT32 ui_cb, UINT32 ui_cr);
//void ide_set_tv_out_limit(UINT8 ui_y_low, UINT8 ui_y_up, UINT8 ui_cb_low, UINT8 ui_cb_up, UINT8 ui_cr_low, UINT8 ui_cr_up);

void ide_set_tv_mode(IDE_TV_MODE ui_tv_mode);
void ide_set_tv_test_mode(BOOL b_en);
void ide_set_tv_test_mode_sel(UINT8 uisel);
void ide_set_tv_test_mode_fix_value(UINT32 uival);
void ide_set_tv_slave_mode(BOOL b_en);
//void ide_set_tv_sacle_swap(BOOL b_en);
//void ide_set_tv_data_swap(BOOL b_en);
//void ide_set_tv_sin_cos_swap(BOOL b_en);
void ide_set_tv_search_en(BOOL b_en);
void ide_set_tv_ckpn_pol(BOOL b_sel);
void ide_set_tv_burst(UINT8 ui_burst);
void ide_set_tv_bll(UINT8 ui_bll);
void ide_set_tv_brl(UINT8 ui_brl);
void ide_set_tv_ntsc_setup_level(UINT8 ui_ntsc_setup);
void ide_set_tv_src(IDE_TV_SRC src);
void ide_set_tv_scale(UINT8 ui_y, UINT8 ui_cb, UINT8 ui_cr);
void ide_set_tv_power_down(BOOL b_pd);
BOOL ide_get_tv_power_down(void);
void ide_set_tv_sela(UINT8 ui_sela);
void ide_set_tv_low_level(UINT32 ui_level);
void ide_set_tv_trim(UINT8 ui_trim);
void ide_get_tv_trim(UINT8 *ui_trim);

UINT8 ide_get_tv_bll(void);
UINT8 ide_get_tv_brl(void);
UINT8 ide_get_tv_ntsc_setup_level(void);
void ide_get_tv_scale(UINT8 *ui_y, UINT8 *ui_cb, UINT8 *ui_cr);

void ide_set_tv_bypass_lpf(BOOL b_en);
BOOL ide_get_tv_bypass_lpf(void);
//void ide_set_tv_bypass_notch(BOOL b_en);
//BOOL ide_get_tv_bypass_notch(void);
void ide_set_tv_chroma_filter(BOOL b_prag, UINT8 *ui_taps);
void ide_set_tv_y_filter(BOOL b_prag, UINT8 *ui_taps);

#endif

