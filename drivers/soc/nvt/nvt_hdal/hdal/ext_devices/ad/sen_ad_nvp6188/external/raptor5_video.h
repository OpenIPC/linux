
#ifndef _RAPTOR5_VIDEO_
#define _RAPTOR5_VIDEO_

#include "raptor5_common.h"


/********************************************************************
 *  1. Video Option Function
 ********************************************************************/
void nc_drv_video_setting_info_get( void *pParam );

/********************************************************************
 *  2. Video Input Function
 ********************************************************************/
void nc_drv_video_input_initialize_set(NC_U8 dev);


void nc_drv_video_input_vfc_status_get(void *pParam);
void nc_drv_video_input_eq_stage_get(void *pParam);
void nc_drv_video_input_set(void *pParam);
void nc_drv_video_input_eq_stage_set(void *pParam);
void nc_drv_video_input_manual_set(void * pParam);

void nc_drv_video_auto_manual_mode_set(void *pParam);
/********************************************************************
 *  3. Video Output Function
 ********************************************************************/
void nc_drv_video_output_port_set(void *pParam);

void nc_drv_video_output_port_manual_set(void *pParam);
void nc_drv_video_output_color_pattern_set(void *pParam);

/********************************************************************
 *  4. Video Color Tune Function
 ********************************************************************/
void nc_drv_video_color_tune_get(void *pParam);
void nc_drv_video_color_tune_set(void *pParam);

#endif
/********************************************************************
 *  End of file
 ********************************************************************/
