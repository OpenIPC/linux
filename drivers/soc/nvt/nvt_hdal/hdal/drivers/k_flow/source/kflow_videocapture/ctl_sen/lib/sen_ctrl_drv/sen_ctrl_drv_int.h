#ifndef _SENSOR_CTRL_DRV_INT_H_
#define _SENSOR_CTRL_DRV_INT_H_

#include "sen_int.h"

typedef struct {
	/* operation for sensor driver */
	INT32(*pwr_ctrl)(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag); ///< sensor power sequence

	INT32(*open)(CTL_SEN_ID id);                                    ///< initial sensor flow
	INT32(*close)(CTL_SEN_ID id);                                   ///< un-initial sensor flow
	INT32(*sleep)(CTL_SEN_ID id);                                    ///< enter sensor sleep mode
	INT32(*wakeup)(CTL_SEN_ID id);                                   ///< exit sensor sleep mode
	INT32(*write)(CTL_SEN_ID id, CTL_SEN_CMD *cmd);                   ///< write command
	INT32(*read)(CTL_SEN_ID id, CTL_SEN_CMD *cmd);                    ///< read command
	INT32(*chgmode)(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj, CTL_SEN_MODE *chg_sen_mode_rst);   ///< change mode flow
	INT32(*chgfps)(CTL_SEN_ID id, CTL_SEN_CHGMODE_OBJ chgmode_obj);   ///< change mode flow
	INT32(*set_cfg)(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value);  ///< set sensor information
	INT32(*get_cfg)(CTL_SEN_ID id, CTL_SEN_CFGID cfg_id, void *value);  ///< get sensor information

	ER(*sendrv_set)(CTL_SEN_ID id, CTL_SENDRV_CFGID sendrv_cfg_id, void *data);  ///< get sendrv information
} CTL_SEN_CTRL_DRV;

extern CTL_SEN_CTRL_DRV *sen_ctrl_drv_get_obj(void);

#endif //_SENSOR_CTRL_DRV_INT_H_
