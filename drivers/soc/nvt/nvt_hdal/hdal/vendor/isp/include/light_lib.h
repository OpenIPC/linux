
#ifndef __LIGHTLIB_H__
#define __LIGHTLIB_H__

#include "hd_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define E_OK                   0      //success
#define E_GET_DEV_FAIL        -1      //No ISP device
#define E_GET_EV_FAIL         -2      //Get EV value fail
#define E_NOT_READY           -3      //AE not nready for get EV value
#define E_SYS                 -4      //System fail
#define E_FAIL               -99      //Other fail
//============================================================================
// global
//============================================================================
/*
Function: sw_cds_init
Description: Init sw cds library and set parameters.
Parameter:
    1. night_ev : EV threshold to switch to night mode
    2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
    3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_init(UINT32 id, INT night_ev, INT day_ev, INT sensitive, INT open_device);

/*
Function: sw_cds_exit
Description: Exit sw cds library to release device file hanlde.
Parameter: none
*/
extern INT sw_cds_exit(void);

/*
Function: sw_cds_set_max_ir_ev
Description: Set the max ev value of IR light.
Parameter: ev value
*/
extern INT sw_cds_set_max_ir_ev(INT ev);

/*
Function: sw_cds_get_curr_ev
Description: Function to get curret environment luminance.
Parameter: none
*/
extern INT sw_cds_get_curr_ev(void);

/*
Function: sw_cds_night_pre_cb
Description: Call back function before setting to night mode.
Parameter: none
*/
extern void sw_cds_night_pre_cb(void);

/*
Function: sw_cds_night_post_cb
Description: Call back function after setting to night mode.
Parameter: none
*/
extern void sw_cds_night_post_cb(void);

/*
Function: sw_cds_check_env_night
Description: Call to sw cds library to check if switch to night mode.
Parameter: none
Return value:  -1: waiting AE converge timeout
				0: Not night scene
				1: Night Scene
*/
extern INT sw_cds_check_env_night(void);

/*
Function: sw_cds_check_env_day
Description: Call to sw cds library to check if switch to day mode.
Parameter: none
Return value:  -1: waiting AE converge timeout
				0: Not night scene
				1: Night Scene
*/
extern INT sw_cds_check_env_day(void);

/*
Function: sw_cds_get_parameter
Description: sw cds get parameters.
Parameter:      1. night_ev : EV threshold to switch to night mode
				2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
				3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_get_parameter(INT *night_ev, INT *day_ev, INT *sensitive);

/*
Function: sw_cds_set_parameter
Description: sw cds set parameters.
Parameter:
    1. night_ev : EV threshold to switch to night mode
    2. day_ev   : EV threshold to switch to day mode. **Must greater thad night_ev to avoid vibration**
    3. sensitive: Threshold to judge the light source type.
*/
extern INT sw_cds_set_parameter(INT night_ev, INT day_ev, INT sensitive);

/*
Function: sw_cds_set_dbg_out
Description: Function to set debug message output on/off.
Parameter: on : 0 -> disable ; 1 -> enable
*/
extern void sw_cds_set_dbg_out(INT on);

#ifdef __cplusplus
}
#endif

#endif // __LIGHTLIB_H__
