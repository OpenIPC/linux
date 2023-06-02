/***************************************************************************
* Copyright  Novatek Microelectronics Corp. 2019.  All rights reserved.    *
*--------------------------------------------------------------------------*
* Name: MD Library Module                                                  *
* Description:                                                             *
* Author: Ming Yang                                                         *
****************************************************************************/

/**
    MD lib

    Sample module detailed description.

    @file       libmd.h
    @ingroup    mhdal
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _LIB_MD_H_
#define _LIB_MD_H_

#include "hd_type.h"

#define CV_MDBC_IMPL_VERSION      "01.01.2105170" //implementation version

/********************************************************************
MACRO CONSTANT DEFINITIONS
********************************************************************/
#define LIB_MD_MAX_CH_NUM								 32
#define LIB_MD_MAX_OBJ_NUM								 128
#define LIB_MD_MAX_SUB_REGION_NUM						 4

/********************************************************************
TYPE DEFINITION
********************************************************************/
typedef enum _LIB_MD_PARAM_ID {
	LIB_MD_MOTION_DETECT_INFO,								   ///<  support get/set, LIB_MD_MDT_INFO for capture channel motion detection info
	LIB_MD_AP_ENABLE_PARAM,									   ///<  support get/set, LIB_MD_AP_ENABLE for capture channel motion detection md application
	LIB_MD_AP_GLOBAL_MOTION_ALARM_PARAM,					   ///<  support get/set, LIB_MD_AP_GLOBAL_MOTION_ALARM for capture channel motion detection globel motion alarm application
	LIB_MD_AP_SUBREGION_MOTION_ALARM_PARAM,					   ///<  support get/set, LIB_MD_AP_SUBREGION_MOTION_ALARM for capture channel motion detection sub-region motion alarm application
	LIB_MD_AP_OBJ_PARAM,
	LIB_MD_AP_SCENE_CHANGE_ALARM_PARAM,						   ///<  support get/set, LIB_MD_AP_SCENE_CHANGE_ALARM_PARAM for capture channel scene change alarm application
	LIB_MD_RESULT_INFO,										   ///<  support get, LIB_MD_MDT_RESULT_INFO for capture channel application result
	LIB_MD_MAX,
	ENUM_DUMMY4WORD(LIB_MD_PARAM_ID)
} LIB_MD_PARAM_ID;

/* capture channel motion detection info */
typedef struct _LIB_MD_EVT_INFO {
	UINT8		  *p_md_bitmap;									///< motion detection bitmap, size is md_bitmap_sz
	UINT32        md_bitmap_sz;								    ///< mb_x_num*mb_y_num MB
	//UINT32		  timestamp;
} LIB_MD_PHY_RST;

/* capture channel motion detection info */
typedef struct _LIB_MD_MDT_INFO {
	UINT32         libmd_enabled;								///< motion detection 0:disable 1:enable
	UINT32         phy_md_x_num;								///< capture image width /phy_mb_x_size (physical MB)
	UINT32         phy_md_y_num;								///< capture image height/phy_mb_y_size (physical MB)
	LIB_MD_PHY_RST phy_md_rst;								    ///< size phy_mb_x_num*phy_mb_y_num MB
} LIB_MD_MDT_INFO;

/* capture channel motion detection application enable type */
typedef struct _LIB_MD_AP_ENABLE {
	UINT32        globel_md_alarm_detect_en;					///< 0: no motion alarm 1:global motion alarm
	UINT32        subregion_md_alarm_detect_en;					///< 0: no motion alarm 1:sub-region motion alarm
	UINT32        scene_change_alarm_detect_en;					///< 0: no scene_change alarm 1:scene_change alarm
	UINT32        md_obj_detect_en;
} LIB_MD_AP_ENABLE;

typedef struct _LIB_MD_AP_GLOBAL_MOTION_ALARM {
	UINT8		  motion_alarm_th;							   ///< global motion alarm sensitive threshold
	//UINT32		  ref_cell_en;								   ///< whether reference cell_map or not
	//UINT8		  *p_cell_map;								   ///< decide which MB is motion detection active !Athe map size is cell_map_sz
	//UINT32		  cell_map_sz;								   ///< vp_mb_x_num*vp_mb_y_num MB
} LIB_MD_AP_GLOBAL_MOTION_ALARM;

/* capture channel motion detection sub-region rect info */
typedef struct _LIB_MD_AP_SUBREGION {
	UINT32		  enabled;									   ///< decide sub-region enable when subregion_md_alarm_detect_en = 1
	UINT32		  x_start;									   ///< start x mb position (virtual MB)
	UINT32		  y_start;									   ///< start y mb position (virtual MB)
	UINT32		  x_end;									   ///< end x mb position (virtual MB)
	UINT32		  y_end;									   ///< end x mb position (virtual MB)
	UINT32		  alarm_th;									   ///< sub-region motion alarm sensitive threshold
} LIB_MD_AP_SUBREGION;

/* capture channel motion detection application sub-region motion alarm */
typedef struct _LIB_MD_AP_SUBREGION_MOTION_ALARM {
	UINT32		  sub_region_num;							   ///< total sub_region_num (max is 4)
	LIB_MD_AP_SUBREGION sub_region[LIB_MD_MAX_SUB_REGION_NUM]; ///sub-region rect info
} LIB_MD_AP_SUBREGION_MOTION_ALARM;

/* capture channel motion detection application obj param */
typedef struct _LIB_MD_AP_OBJ {
	UINT32		  obj_size_th;								   ///detect minimum obj size threshold
} LIB_MD_AP_OBJ;

/* capture channel motion detection obj info */
typedef struct _LIB_MD_AP_OBJ_INFO {
	UINT32        start_x;									   ///< obj rectangle left upper x position (virtual pixel)  
	UINT32        start_y;									   ///< obj rectangle left upper y position (virtual pixel)  
	UINT32        end_x;									   ///< obj rectangle right down x position (virtual pixel)  
	UINT32        end_y;									   ///< obj rectangle right down y position (virtual pixel)  
	UINT32        label;									   ///< obj label
} LIB_MD_AP_OBJ_INFO;

/* capture channel motion detection application scene change alarm */
typedef struct _LIB_MD_AP_SCENE_CHANGE_ALARM {
	UINT8		  scene_change_alarm_th;							   ///< scene change alarm sensitive threshold
} LIB_MD_AP_SCENE_CHANGE_ALARM;

/* capture channel motion detection result */
typedef struct _LIB_MD_MDT_RESULT_INFO {
	UINT8         global_motion_alarm;						   ///< global motion alarm
	//UINT32		  global_motion_alarm_num;					   ///< global motion alarm sensitive threshold
	UINT32        sub_motion_alarm[LIB_MD_MAX_SUB_REGION_NUM]; ///< sub-region motion alarm
	UINT8         scene_change_alarm;						   ///< scene change alarm
	UINT32        obj_num;								       ///< detect object number (max=128)
	LIB_MD_AP_OBJ_INFO	obj[LIB_MD_MAX_OBJ_NUM];
} LIB_MD_MDT_RESULT_INFO;

typedef struct _LIB_MD_MDT_LIB_INFO {
	LIB_MD_MDT_INFO mdt_info;
	LIB_MD_AP_ENABLE mdt_enable;
	LIB_MD_AP_GLOBAL_MOTION_ALARM mdt_global_param;
	LIB_MD_AP_SUBREGION_MOTION_ALARM mdt_subregion_param;
	LIB_MD_AP_OBJ mdt_obj;
	LIB_MD_AP_SCENE_CHANGE_ALARM mdt_scene_change_param;
}LIB_MD_MDT_LIB_INFO;

/********************************************************************
EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT lib_md_get(INT idx, LIB_MD_PARAM_ID id, void *p_param);
HD_RESULT lib_md_set(INT idx, LIB_MD_PARAM_ID id, void *p_param);
HD_RESULT lib_md_init(INT idx);
HD_RESULT lib_md_uninit(INT idx);
CHAR *lib_md_get_version(VOID);


#endif  /* _LIB_MD_H_ */