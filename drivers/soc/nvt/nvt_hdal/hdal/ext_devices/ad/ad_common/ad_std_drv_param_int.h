/**
    std sensor parameter .


    @file       ad_std_drv_param_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _AD_STD_DRV_PARAM_INT_H_
#define _AD_STD_DRV_PARAM_INT_H_

#include "kflow_videocapture/ctl_sen.h"
#include "ad_util_int.h"

/**
     Sensor mode
     plz map to enum: SENSOR_MODE sequenitally and continuously
*/

#define MODE_MAX    1               //total senMode supported by this sensor

// get sensor information
extern CTL_SEN_DRV_TAB *AD_UTIL_DECLARE_FUNC(sensor_get_drv_tab_ad_std_drv)(void);

extern CTL_SENDRV_I2C *AD_UTIL_DECLARE_FUNC(get_sensor_i2c_info_ad_std_drv)(void);
extern void AD_UTIL_DECLARE_FUNC(set_sensor_i2c_info_tab)(UINT32 idx, UINT32 val);
#endif
