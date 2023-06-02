#ifndef _SENSOR_ID_MAP_INT_H_
#define _SENSOR_ID_MAP_INT_H_

#include "sen_int.h"
#include "sen_dbg_infor_int.h"

extern UINT32 sen_get_drvdev_csi(CTL_SEN_ID id);
extern UINT32 sen_get_drvdev_lvds(CTL_SEN_ID id);
extern UINT32 sen_get_drvdev_tge(CTL_SEN_ID id);
extern UINT32 sen_get_drvdev_tge_chsft(CTL_SEN_ID id);
extern UINT32 sen_get_drvdev_vx1(CTL_SEN_ID id);
extern UINT32 sen_get_drvdev_slvsec(CTL_SEN_ID id);

#endif //_SENSOR_ID_MAP_INT_H_
