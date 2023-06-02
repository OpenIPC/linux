#ifndef _MTR_DRV_TI8833_H_
#define _MTR_DRV_TI8833_H_

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#else
#include "plat/gpio.h"
#endif
#include "mtr_common.h"


//=============================================================================
// compile switch
//=============================================================================
#define P_IRIS_FUNC_ENABLE          0
#define ZOOM_FOCUS_SIM_MOVE         0

#define REVERSE_FOCUS_GOHOME_DIR    0    // *
#define REVERSE_ZOOM_GOHOME_DIR     0    // *
#define REVERSE_IRIS_GOHOME_DIR     0    // *


//=============================================================================
// constant define
//=============================================================================
#define MOTOR_IRIS_ENABLE_PIN    (0x20+16)    //* P_GPIO_16


//=============================================================================
// struct & definition
//=============================================================================


//=============================================================================
// extern functions
//=============================================================================
extern MOTOR_DRV_INFO* mdrv_get_motor_info(UINT32 id);

extern void   ti8833_open(UINT32 id);
extern void   ti8833_close(UINT32 id);

extern INT32  ti8833_init_focus(UINT32 id);
extern UINT32 ti8833_get_focus_speed(UINT32 id);
extern void   ti8833_set_focus_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   ti8833_assign_focus_position(UINT32 id, INT32 focus_pos, INT32 dir);
extern UINT32 ti8833_get_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos);
extern void   ti8833_set_focus_focal_length(UINT32 id, UINT32 zoom_sec_pos, UINT32 focal_length);
extern INT32  ti8833_get_focus_range(UINT32 id);
extern INT32  ti8833_get_focus_position(UINT32 id);
extern INT32  ti8833_set_focus_position(UINT32 id, INT32 focus_pos, BOOL limit_range);
extern INT32  ti8833_press_focus_move(UINT32 id, BOOL move_dir);
extern void   ti8833_release_focus_move(UINT32 id);

extern INT32  ti8833_init_zoom(UINT32 id);
extern UINT32 ti8833_get_zoom_speed(UINT32 id);
extern void   ti8833_set_zoom_speed(UINT32 id, MOTOR_SPEED_CATEGORY spd_category);
extern void   ti8833_assign_zoom_position(UINT32 id, INT32 zoom_pos, INT32 dir);
extern UINT32 ti8833_get_zoom_section_pos(UINT32 id);
extern UINT32 ti8833_get_zoom_max_section_pos(UINT32 id);
extern INT32  ti8833_set_zoom_section_pos(UINT32 id, UINT32 zoom_sec_pos);
extern INT32  ti8833_get_zoom_position(UINT32 id);
extern INT32  ti8833_set_zoom_position(UINT32 id, INT32 zoom_pos);
extern INT32  ti8833_press_zoom_move(UINT32 id, BOOL move_dir);
extern void   ti8833_release_zoom_move(UINT32 id);

extern UINT32 ti8833_get_ircut_state(UINT32 id);
extern void   ti8833_set_ircut_state(UINT32 id, IRCUT_ACT state, UINT32 param);

extern void   ti8833_init_aperture(UINT32 id);
extern UINT32 ti8833_get_aperture_fno(UINT32 id, UINT32 zoom_sec_pos, UINT32 iris_pos);
extern APERTURE_SEC_POS ti8833_get_aperture_section_pos(UINT32 id);
extern void   ti8833_set_aperture_section_pos(UINT32 id, APERTURE_SEC_POS iris_sec_pos);
extern UINT32 ti8833_get_aperture_position(UINT32 id);
extern void   ti8833_set_aperture_position(UINT32 id, UINT32 iris_pos);

extern SHUTTER_ACT ti8833_get_shutter_state(UINT32 id);
extern void   ti8833_set_shutter_state(UINT32 id, SHUTTER_ACT state, UINT32 param);

extern UINT32 ti8833_get_capability_info(UINT32 id);
extern UINT32 ti8833_get_busy_status(UINT32 id);
extern UINT32 ti8833_get_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 ti8833_set_zf_range_table(UINT32 id, UINT32 tab_idx, LENS_ZOOM_FOCUS_TAB *zf_rtab);
extern UINT32 ti8833_get_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 ti8833_set_zf_curve_table(UINT32 id, UINT32 tab_idx, LENS_FOCUS_DISTANCE_TAB *zf_ctab);
extern UINT32 ti8833_get_signal_state(UINT32 id, MOTOR_SIGNAL signal);
extern void   ti8833_set_signal_state(UINT32 id, MOTOR_SIGNAL signal, UINT32 state);
extern void   ti8833_get_fz_backlash_value(UINT32 id, INT32 *focus_backlash, INT32 *zoom_backlash);
extern void   ti8833_set_fz_backlash_value(UINT32 id, INT32 focus_backlash, INT32 zoom_backlash);


#endif

