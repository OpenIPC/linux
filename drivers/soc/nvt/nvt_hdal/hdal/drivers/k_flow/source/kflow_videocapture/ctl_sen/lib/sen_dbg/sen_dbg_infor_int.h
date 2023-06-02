#ifndef _SENSOR_DEBUG_INT_H_
#define _SENSOR_DEBUG_INT_H_

#include "sen_int.h"
#include "sen_ctrl_if_int.h"
#include "sen_ctrl_cmdif_int.h"
#include "sen_ctrl_drv_int.h"

#define CTL_SEN_DBG_DET DISABLE

#define SEN_CMD_VAR2STR(x)   	(#x)
#define SEN_CMD_DUMPD(x)      	DBG_DUMP("%s = %d\r\n", SEN_CMD_VAR2STR(x), x)
#define SEN_CMD_DUMPH(x)      	DBG_DUMP("%s = 0x%.8x\r\n", SEN_CMD_VAR2STR(x), x)
#define SEN_STR_CUR_MODE 		"===> CUR_MODE :"

typedef struct {
	void(*dump_info)(CTL_SEN_ID id);                                ///< dump get info (PCTL_SEN_OBJ()->get_cfg)
	void(*dump_ext_info)(CTL_SEN_ID id);                            ///< dump ext info (PCTL_SEN_OBJ()->init_cfg & PCTL_SEN_OBJ()->get_cfg(CTL_SEN_CFGID_INIT_XX, value))
	void(*dump_drv_info)(CTL_SEN_ID id);                            ///< dump sensor driver get info (CTL_SEN_DRV_TAB()->get_cfg(id, CTL_SENDRV_CFGID, value))
	void(*wait_inte)(CTL_SEN_ID id, CTL_SEN_INTE inte);             ///< wait sen_ctrl_if_get_obj inter. only support serial sensor (LVDS/CSI/SLVS-EC)
	void(*dump_map_tbl_info)(void);                                 ///< dump (ctl_sen_reg_sendrv/ctl_sen_unreg_sendrv & PCTL_SEN_OBJ()->init_cfg) mapping table
	void(*dump_proc_time)(void);                                    ///< dump ctl sen process time
	void(*dbg_msg)(CTL_SEN_ID id, BOOL en, CTL_SEN_MSG_TYPE type);  ///< enable debug msg
	void(*dump_ctl_info)(CTL_SEN_ID id);                            ///< dump ctl_sen inner info
	void(*dump_er)(CTL_SEN_ID id);                                  ///< dump error code, which set by ctl_sen_set_er
	void(*set_lv)(CTL_SEN_DBG_LV dbg_lv);   ///< set dbg msg level
} CTL_SEN_DBG;

CHAR *sen_get_clksrc_str(CTL_SEN_CLKSRC_SEL in);
CHAR *sen_get_clksel_str(CTL_SEN_CLK_SEL in);
CHAR *sen_get_opstate_str(CTL_SEN_STATE in);
CHAR *sen_get_cmdiftype_str(CTL_SEN_CMDIF_TYPE in);

extern CTL_SEN_MSG_TYPE ctl_sen_msg_type[CTL_SEN_ID_MAX_SUPPORT];
extern CTL_SEN_DBG *sen_dbg_get_obj(void);
/*
	for ctl_sen module operation error

	CTL_SEN_ER_ITEM_OUTPUT for ctl_sen module output, other for sen_ctrl.c updtae
*/
extern void ctl_sen_set_er(CTL_SEN_ID id, CTL_SEN_ER_OP op, CTL_SEN_ER_ITEM item, INT32 rt);
/*
	for sensor driver operation error

	item:
	CTL_SEN_ER_OP_SET: CTL_SENDRV_CFGID
	CTL_SEN_ER_OP_GET: CTL_SENDRV_CFGID
	OTHER: 0
*/
extern void ctl_sen_set_er_sendrv(CTL_SEN_ID id, CTL_SEN_ER_OP op, UINT32 item);

extern void ctl_sen_set_proc_time_adv(CHAR *func_name, UINT32 cfg, CTL_SEN_ID id, CTL_SEN_PROC_TIME_ITEM item, UINT32 tag);
extern void ctl_sen_set_op_io(CTL_SEN_ID id, CTL_SEN_ER_OP op, CTL_SEN_PROC_TIME_ITEM item);

#endif //_SENSOR_DEBUG_INT_H_
