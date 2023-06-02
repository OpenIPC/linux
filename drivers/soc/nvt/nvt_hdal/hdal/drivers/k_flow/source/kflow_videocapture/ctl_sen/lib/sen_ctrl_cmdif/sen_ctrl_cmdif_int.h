#ifndef _SENSOR_CTRL_CMDIF_INT_H_
#define _SENSOR_CTRL_CMDIF_INT_H_

// sensor
#include "sen_int.h"
#include "sen_id_map_int.h"

/*
	vx1 object
*/
typedef struct {
	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input


} CTL_SEN_CTRL_CMDIF_VX1;

typedef struct {
	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);
	BOOL(*is_opend)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input

} CTL_SEN_CTRL_CMDIF;

/*
	extern API
*/
extern UINT32   g_ctl_sen_cmdif_kdrv_hdl[CTL_SEN_ID_MAX_SUPPORT];
extern CTL_SEN_CTRL_CMDIF *sen_ctrl_cmdif_get_obj(void);
extern CTL_SEN_CTRL_CMDIF_VX1 *sen_ctrl_cmdif_vx1_get_obj(void);

#endif //_SENSOR_CTRL_CMDIF_INT_H_
