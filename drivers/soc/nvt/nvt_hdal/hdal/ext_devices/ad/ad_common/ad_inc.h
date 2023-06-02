#ifndef _AD_INC_H_
#define _AD_INC_H_

#include "kflow_videocapture/ctl_sen_ext.h"

#if defined(__FREERTOS)
extern int sen_init_ad_tc358743(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_tc358840(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_nvp6124b(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_nvp6188(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_tp2854(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_tp2855(SENSOR_DTSI_INFO *info);
extern int sen_init_ad_tp9950(SENSOR_DTSI_INFO *info);
#endif
#endif //_AD_INC_H_
