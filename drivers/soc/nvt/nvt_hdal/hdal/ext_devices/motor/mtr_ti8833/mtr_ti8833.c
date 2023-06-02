
#ifdef __KERNEL__
#include <linux/kthread.h>
#else
#include <stdio.h>
#endif
#include "kwrap/verinfo.h"
#include "isp_dev.h"
#include "mtr_cfg.h"
#include "mtr_drv_ti8833.h"


//=============================================================================
// module parameter : set module parameters when insert the module
//=============================================================================
#ifdef __KERNEL__
char *mtr_cfg_path = "null";
module_param_named(mtr_cfg_path, mtr_cfg_path, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mtr_cfg_path, "Path of cfg file");

#ifdef DEBUG
unsigned int mtr_debug_level = THIS_DBGLVL;
module_param_named(mtr_debug_level, mtr_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mtr_debug_level, "Debug message level");
#endif
#else
char *mtr_cfg_path = "null";
unsigned int mtr_debug_level = THIS_DBGLVL;
#endif


//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_mtr_ti8833, 1, 03, 000, 00);


//=============================================================================
// extern functions
//=============================================================================
extern CFG_FILE_FMT *mtr_common_open_cfg(INT8 *pfile_name);
extern void mtr_common_close_cfg(CFG_FILE_FMT *pcfg_file);
extern void mtr_common_load_cfg(CFG_FILE_FMT *pcfg_file, void *param);

extern ER mtr_common_load_dtsi_file(UINT8 *node_path, UINT8 *file_path, UINT8 *buf_addr, void *param);
#ifdef __KERNEL__
extern ER mtr_common_load_dtsi(struct device_node *node, void *param);
#endif


//=============================================================================
// function declaration
//=============================================================================
static ER mtr_open_ti8833(UINT32 id);
static ER mtr_close_ti8833(UINT32 id);
static ER mtr_get_focus_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_set_focus_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_get_zoom_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_set_zoom_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_get_ircut_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_set_ircut_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_get_aperture_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_set_aperture_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_get_shutter_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_set_shutter_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd);
static ER mtr_get_misc_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_LCMD *pcmd);
static ER mtr_set_misc_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_LCMD *pcmd);


//=============================================================================
// global variable
//=============================================================================
#ifdef __KERNEL__
static MTR_MOTION_CMD mtr_mot_cmd = { .mtr_id = MTR_ID_0, .mtr_mot_item = MTR_ITEM_MOTION_NONE, .mtr_mot_arg.argu = { 0, 0 } };
#endif
static CTL_MTR_DRV_TAB mtr_drv_tab_ti8833 = {
	mtr_open_ti8833,
	mtr_close_ti8833,
	mtr_get_focus_ti8833,
	mtr_set_focus_ti8833,
	mtr_get_zoom_ti8833,
	mtr_set_zoom_ti8833,
	mtr_get_ircut_ti8833,
	mtr_set_ircut_ti8833,
	mtr_get_aperture_ti8833,
	mtr_set_aperture_ti8833,
	mtr_get_shutter_ti8833,
	mtr_set_shutter_ti8833,
	mtr_get_misc_ti8833,
	mtr_set_misc_ti8833,
};

CTL_MTR_DRV_TAB *mtr_get_drv_tab(void)
{
	return &mtr_drv_tab_ti8833;
}

static ER mtr_open_ti8833(UINT32 id)
{
	ER rt = E_OK;

	id = MTR_MIN(id, MTR_ID_MAX-1);

	ti8833_open(id);

	return rt;
}

static ER mtr_close_ti8833(UINT32 id)
{
	ER rt = E_OK;

	id = MTR_MIN(id, MTR_ID_MAX-1);

	ti8833_close(id);

	return rt;
}

static ER mtr_get_focus_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	UINT32 tmp;
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_FOCUS_SPEED:
		pcmd->data[0] = ti8833_get_focus_speed(id);
		break;

	case MTR_GET_FOCUS_FOCAL_LENGTH:
		pcmd->data[0] = ti8833_get_focus_focal_length(id, pcmd->argu[0]);
		break;

	case MTR_GET_FOCUS_RANGE:
		tmp = ti8833_get_focus_range(id);
		pcmd->data[0] = tmp & 0xFFFF;
		pcmd->data[1] = (tmp >> 16) & 0xFFFF;
		break;

	case MTR_GET_FOCUS_POSITION:
		pcmd->data[0] = ti8833_get_focus_position(id);
		break;

	case MTR_GET_FOCUS_BUSY_STATUS:
		pcmd->data[0] = (ti8833_get_busy_status(id) >> 0) & 0x01;
		break;

	case MTR_GET_MOTION_QUEUE_CMD_STATUS:
#ifdef __KERNEL__
		pcmd->data[0] = mtr_mot_cmd.mtr_mot_item;
		pcmd->data[1] = mtr_mot_cmd.mtr_mot_arg.data[1];
#else
		pcmd->data[0] = pcmd->data[1] = 0;
#endif
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_focus_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;
#ifdef __KERNEL__
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(0);
#endif

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_FOCUS_INIT:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_FOCUS_INIT;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_FOCUS_INIT;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		ti8833_init_focus(id);
#endif
		break;

	case MTR_SET_FOCUS_SPEED:
		ti8833_set_focus_speed(id, pcmd->argu[0]);
		break;

	case MTR_SET_FOCUS_ASSIGN_POSITION:
		ti8833_assign_focus_position(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	case MTR_SET_FOCUS_FOCAL_LENGTH:
		ti8833_set_focus_focal_length(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	case MTR_SET_FOCUS_POSITION:
		pcmd->data[0] = ti8833_set_focus_position(id, pcmd->argu[0], TRUE);
		break;

	case MTR_SET_FOCUS_POSITION_IN_QUEUE:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_FOCUS_POSITION;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_FOCUS_POSITION;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		pcmd->data[0] = ti8833_set_focus_position(id, pcmd->argu[0], TRUE);
#endif
		break;

	case MTR_SET_FOCUS_PRESS:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_FOCUS_PRESS;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_FOCUS_PRESS;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		pcmd->data[0] = ti8833_press_focus_move(id, (BOOL)pcmd->argu[0]);
#endif
		break;

	case MTR_SET_FOCUS_RELEASE:
		ti8833_release_focus_move(id);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_get_zoom_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_ZOOM_SPEED:
		pcmd->data[0] = ti8833_get_zoom_speed(id);
		break;

	case MTR_GET_ZOOM_SECTION_POS:
		pcmd->data[0] = ti8833_get_zoom_section_pos(id);
		break;

	case MTR_GET_ZOOM_MAX_SECTION_POS:
		pcmd->data[0] = ti8833_get_zoom_max_section_pos(id);
		break;

	case MTR_GET_ZOOM_POSITION:
		pcmd->data[0] = ti8833_get_zoom_position(id);
		break;

	case MTR_GET_ZOOM_BUSY_STATUS:
		pcmd->data[0] = (ti8833_get_busy_status(id) >> 1) & 0x01;
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_zoom_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;
#ifdef __KERNEL__
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(0);
#endif

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_ZOOM_INIT:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_ZOOM_INIT;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_ZOOM_INIT;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		ti8833_init_zoom(id);
#endif
		break;

	case MTR_SET_ZOOM_SPEED:
		ti8833_set_zoom_speed(id, pcmd->argu[0]);
		break;

	case MTR_SET_ZOOM_ASSIGN_POSITION:
		ti8833_assign_zoom_position(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	case MTR_SET_ZOOM_SECTION_POS:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_ZOOM_SECTION_POS;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_ZOOM_SECTION_POS;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		pcmd->data[0] = ti8833_set_zoom_section_pos(id, pcmd->argu[0]);
#endif
		break;

	case MTR_SET_ZOOM_POSITION:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_ZOOM_POSITION;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_ZOOM_POSITION;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		pcmd->data[0] = ti8833_set_zoom_position(id, pcmd->argu[0]);
#endif
		break;

	case MTR_SET_ZOOM_PRESS:
#ifdef __KERNEL__
		if (mtr_mot_cmd.mtr_mot_item == MTR_ITEM_MOTION_NONE) {
			mtr_mot_cmd.mtr_id = id;
			mtr_mot_cmd.mtr_mot_item = MTR_ITEM_ZOOM_PRESS;
			memcpy(&mtr_mot_cmd.mtr_mot_arg, pcmd, sizeof(MTR_CTL_CMD));
			pmotor_info->mtr_event_flag |= MTR_ITEM_ZOOM_PRESS;
			wake_up(&pmotor_info->mtr_event_queue);
		} else {
			rt = E_QOVR;
			DBG_WRN("mtr cmd ongoing!\r\n");
		}
#else
		pcmd->data[0] = ti8833_press_zoom_move(id, (BOOL)pcmd->argu[0]);
#endif
		break;

	case MTR_SET_ZOOM_RELEASE:
		ti8833_release_zoom_move(id);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_get_ircut_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_IRCUT_POSITION:
		pcmd->data[0] = ti8833_get_ircut_state(id);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_ircut_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_IRCUT_INIT:
		ti8833_set_ircut_state(id, IRCUT_OPEN, 100);
		break;

	case MTR_SET_IRCUT_POSITION:
		ti8833_set_ircut_state(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_get_aperture_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_APERTURE_INFO:
		sprintf((CHAR *)pcmd->data, "NON");
		pcmd->data[1] = 0;    // min(bit0~15) / max(bit16~31)
		break;

	case MTR_GET_APERTURE_FNO:
		pcmd->data[0] = ti8833_get_aperture_fno(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	case MTR_GET_APERTURE_SECTION_POS:
		pcmd->data[0] = ti8833_get_aperture_section_pos(id);
		break;

	case MTR_GET_APERTURE_POSITION:
		pcmd->data[0] = ti8833_get_aperture_position(id);
		break;

	case MTR_GET_APERTURE_FLUX_RATIO:
		DBG_WRN("not support!\r\n");
		break;

	case MTR_GET_APERTURE_BUSY_STATUS:
		pcmd->data[0] = (ti8833_get_busy_status(id) >> 2) & 0x01;
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_aperture_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_APERTURE_INIT:
		ti8833_init_aperture(id);
		break;

	case MTR_SET_APERTURE_ASSIGN_POSITION:
		DBG_WRN("not support!\r\n");
		break;

	case MTR_SET_APERTURE_SECTION_POS:
		ti8833_set_aperture_section_pos(id, pcmd->argu[0]);
		break;

	case MTR_SET_APERTURE_POSITION:
		ti8833_set_aperture_position(id, pcmd->argu[0]);
		break;

	case MTR_SET_APERTURE_FLUX_RATIO:
		DBG_WRN("not support!\r\n");
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_get_shutter_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_SHUTTER_STATE:
		pcmd->data[0] = ti8833_get_shutter_state(id);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_shutter_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_CMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_SHUTTER_INIT:
		ti8833_set_shutter_state(id, SHUTTER_OPEN, 50);
		break;

	case MTR_SET_SHUTTER_STATE:
		ti8833_set_shutter_state(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_get_misc_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_LCMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_GET_MISC_NAME_STR:
		sprintf((CHAR *)pcmd->data, "TI-DRV8833");
		break;

	case MTR_GET_MISC_CAPABILITY:
		pcmd->data[0] = ti8833_get_capability_info(id);
		break;

	case MTR_GET_MISC_ZF_RANGE_TABLE:
		pcmd->data[0] = ti8833_get_zf_range_table(id, pcmd->argu[0], (LENS_ZOOM_FOCUS_TAB *)&pcmd->data[1]);
		break;

	case MTR_GET_MISC_ZF_CURVE_TABLE:
		pcmd->data[0] = ti8833_get_zf_curve_table(id, pcmd->argu[0], (LENS_FOCUS_DISTANCE_TAB *)&pcmd->data[1]);
		break;

	case MTR_GET_MISC_LED_STATE:
		pcmd->data[0] = ti8833_get_signal_state(id, pcmd->argu[0]);
		break;

	case MTR_GET_MISC_FZ_BACKLASH:
		ti8833_get_fz_backlash_value(id, (INT32 *)&pcmd->data[0], (INT32 *)&pcmd->data[1]);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

static ER mtr_set_misc_ti8833(UINT32 id, UINT32 cmd_type, MTR_CTL_LCMD *pcmd)
{
	ER rt = E_OK;

	if (pcmd == NULL) {
		return E_SYS;
	}

	id = MTR_MIN(id, MTR_ID_MAX-1);

	switch (cmd_type) {
	case MTR_SET_MISC_ZF_RANGE_TABLE:
		pcmd->data[0] = ti8833_set_zf_range_table(id, pcmd->argu[0], (LENS_ZOOM_FOCUS_TAB *)&pcmd->argu[1]);
		break;

	case MTR_SET_MISC_ZF_CURVE_TABLE:
		pcmd->data[0] = ti8833_set_zf_curve_table(id, pcmd->argu[0], (LENS_FOCUS_DISTANCE_TAB *)&pcmd->argu[1]);
		break;

	case MTR_SET_MISC_LED_STATE:
		ti8833_set_signal_state(id, pcmd->argu[0], pcmd->argu[1]);
		break;

	case MTR_SET_MISC_FZ_BACKLASH:
		ti8833_set_fz_backlash_value(id, (INT32)pcmd->argu[0], (INT32)pcmd->argu[1]);
		break;

	default:
		rt = E_NOSPT;
		break;
	}

	return rt;
}

#ifdef __KERNEL__
static void do_motion_work(void)
{
	//DBG_DUMP("mtr_cmd: %u,%u,%u\r\n", mtr_mot_cmd.mtr_id, mtr_mot_cmd.mtr_mot_item, mtr_mot_cmd.mtr_mot_arg.argu[0]);

	switch (mtr_mot_cmd.mtr_mot_item) {
	case MTR_ITEM_FOCUS_INIT:
		ti8833_init_focus(mtr_mot_cmd.mtr_id);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_FOCUS_INIT;
		break;

	case MTR_ITEM_FOCUS_POSITION:
		mtr_mot_cmd.mtr_mot_arg.data[0] = ti8833_set_focus_position(mtr_mot_cmd.mtr_id, mtr_mot_cmd.mtr_mot_arg.argu[0], TRUE);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_FOCUS_POSITION;
		break;

	case MTR_ITEM_FOCUS_PRESS:
		mtr_mot_cmd.mtr_mot_arg.data[0] = ti8833_press_focus_move(mtr_mot_cmd.mtr_id, (BOOL)mtr_mot_cmd.mtr_mot_arg.argu[0]);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_FOCUS_PRESS;
		break;

	case MTR_ITEM_ZOOM_INIT:
		ti8833_init_zoom(mtr_mot_cmd.mtr_id);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_ZOOM_INIT;
		break;

	case MTR_ITEM_ZOOM_SECTION_POS:
		mtr_mot_cmd.mtr_mot_arg.data[0] = ti8833_set_zoom_section_pos(mtr_mot_cmd.mtr_id, mtr_mot_cmd.mtr_mot_arg.argu[0]);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_ZOOM_SECTION_POS;
		break;

	case MTR_ITEM_ZOOM_POSITION:
		mtr_mot_cmd.mtr_mot_arg.data[0] = ti8833_set_zoom_position(mtr_mot_cmd.mtr_id, mtr_mot_cmd.mtr_mot_arg.argu[0]);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_ZOOM_POSITION;
		break;

	case MTR_ITEM_ZOOM_PRESS:
		mtr_mot_cmd.mtr_mot_arg.data[0] = ti8833_press_zoom_move(mtr_mot_cmd.mtr_id, (BOOL)mtr_mot_cmd.mtr_mot_arg.argu[0]);
		mtr_mot_cmd.mtr_mot_arg.data[1] = MTR_ITEM_ZOOM_PRESS;
		break;

	case MTR_ITEM_MOTION_NONE:
	default:
		mtr_mot_cmd.mtr_mot_arg.data[0] = mtr_mot_cmd.mtr_mot_arg.data[1] = 0;
		break;
	}
}

static INT32 do_motion_thread(void *private)
{
	unsigned long flags;
	MOTOR_DRV_INFO *pmotor_info = (MOTOR_DRV_INFO *)private;

	do {
		wait_event(pmotor_info->mtr_event_queue, pmotor_info->mtr_event_flag & MTR_ITEM_MOTION_ALL);

		if (pmotor_info->mtr_event_flag & MTR_ITEM_MOTION_STOP) {
			continue;
		}

		do_motion_work();

		vk_spin_lock_irqsave(&pmotor_info->mtr_lock, flags);
		mtr_mot_cmd.mtr_mot_item = MTR_ITEM_MOTION_NONE;
		pmotor_info->mtr_event_flag = 0;    // clear event flag
		vk_spin_unlock_irqrestore(&pmotor_info->mtr_lock, flags);
	} while (!kthread_should_stop());

	return 0;
}
#endif

#ifdef __KERNEL__
static int __init mtr_init_ti8833(void)
#else
int mtr_init_ti8833(void)
#endif
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	UINT32 i;
	CFG_FILE_FMT *pcfg_file;
#if MTR_DTSI_FROM_FILE
	MTR_DTSI_INFO mtr_dtsi = { 0 };
#endif
	ER rt = E_OK;
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(0);

	vk_spin_lock_init(&pmotor_info->mtr_lock);

#ifdef __KERNEL__
	pmotor_info->mtr_event_flag = 0;
	init_waitqueue_head(&pmotor_info->mtr_event_queue);

	pmotor_info->motion_thread = kthread_create(do_motion_thread, pmotor_info, "mtr_motion");
	if (!IS_ERR(pmotor_info->motion_thread)) {
		wake_up_process(pmotor_info->motion_thread);
	} else {
		DBG_ERR("fail to create motor thread!\n");
		pmotor_info->motion_thread = NULL;
		rt = -EFAULT;
	}
#endif

	// parsing cfg or dtsi file if exist
	if ((strstr(mtr_cfg_path, "null")) || (strstr(mtr_cfg_path, "NULL"))) {
		DBG_WRN("mtr cfg file no exist!\r\n");
		cfg_path[0] = '\0';
#if MTR_DTSI_FROM_FILE
	} else if ((strstr(mtr_cfg_path, "dtsi")) || (strstr(mtr_cfg_path, "TDSI"))) {
		mtr_dtsi.buf_addr = NULL;

		strncpy(mtr_dtsi.node_path, "/motor/mtr_cfg/mtr_ti8833", MTR_DTSI_NAME_LENGTH);
		mtr_dtsi.node_path[MTR_DTSI_NAME_LENGTH-1] = '\0';

		strncpy(mtr_dtsi.file_path, "/mnt/app/motor/motor.dtb", MTR_DTSI_NAME_LENGTH);
		mtr_dtsi.file_path[MTR_DTSI_NAME_LENGTH-1] = '\0';

		if (mtr_common_load_dtsi_file((UINT8 *)mtr_dtsi.node_path, (UINT8 *)mtr_dtsi.file_path, mtr_dtsi.buf_addr, (void *)pmotor_info) == E_OK) {
			DBG_MSG("mtr load dtsi file success\r\n");
		} else {
			DBG_ERR("mtr load dtsi file fail!\r\n");
		}
#endif
	} else {
		if ((mtr_cfg_path != NULL) && (strlen(mtr_cfg_path) <= MAX_PATH_NAME_LENGTH)) {
			strncpy((char *)cfg_path, mtr_cfg_path, MAX_PATH_NAME_LENGTH);
		}

		if ((pcfg_file = mtr_common_open_cfg(cfg_path)) != NULL) {
			mtr_common_load_cfg(pcfg_file, (void *)pmotor_info);
			mtr_common_close_cfg(pcfg_file);
			DBG_MSG("mtr load %s success\r\n", mtr_cfg_path);
		} else {
			DBG_ERR("mtr load cfg fail!\r\n");
		}
	}

	for (i = 0; i < MTR_ID_MAX; i++) {
		mtr_open_ti8833(i);
	}

	isp_dev_reg_motor_driver(&mtr_drv_tab_ti8833);

	return rt;
}

#ifdef __KERNEL__
static void __exit mtr_exit_ti8833(void)
#else
void mtr_exit_ti8833(void)
#endif
{
	UINT32 i;
#ifdef __KERNEL__
	MOTOR_DRV_INFO *pmotor_info = mdrv_get_motor_info(0);
#endif

	for (i = 0; i < MTR_ID_MAX; i++) {
		mtr_close_ti8833(i);
	}

#ifdef __KERNEL__
	pmotor_info->mtr_event_flag = MTR_ITEM_MOTION_STOP;
	wake_up(&pmotor_info->mtr_event_queue);

	if (pmotor_info->motion_thread) {
		kthread_stop(pmotor_info->motion_thread);
	}
#endif
}

#ifdef __KERNEL__
module_init(mtr_init_ti8833);
module_exit(mtr_exit_ti8833);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("Motor Driver TI8833");
MODULE_LICENSE("GPL");
#endif

