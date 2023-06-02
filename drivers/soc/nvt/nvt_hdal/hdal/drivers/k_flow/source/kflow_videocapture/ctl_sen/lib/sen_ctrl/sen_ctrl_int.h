#ifndef _SENSOR_CTRL_INT_H_
#define _SENSOR_CTRL_INT_H_


// sensor
#include "sen_int.h"
#include "sen_dbg_infor_int.h"
#include "sen_ctrl_drv_int.h"
#include "sen_ctrl_if_int.h"
#include "sen_ctrl_cmdif_int.h"
#include "sen_utility_clk_int.h"

typedef struct {
	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*clk_prepare)(CTL_SEN_ID id, CTL_SEN_MODE mode);
	INT32(*clk_unprepare)(CTL_SEN_ID id, CTL_SEN_MODE mode);

} CTL_SEN_CTRL_CLK;

extern CTL_SEN_CTRL_CLK *sen_ctrl_clk_get_obj(void);

#endif //_SENSOR_CTRL_INT_H_
