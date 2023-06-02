#ifndef _SENSOR_CTRL_IF_INT_H_
#define _SENSOR_CTRL_IF_INT_H_

#include "sen_int.h"
#include "sen_id_map_int.h"

/* LVDS info */
#define SEN_LVDS_CTL_NUM 8 // lvds controller number
#define SEN_LVDS_DL_NUM 8 // lvds max support datalan
/* CSI info */
#define SEN_CSI_CTL_NUM 8 // csi controller number
#define SEN_CSI_DL_NUM 4 // csi max support datalan
/* SLVS-EC info */
#define SEN_SLVSEC_CTL_NUM 8 // slvs-ec controller number
#define SEN_SLVSEC_SIE1_DL_NUM 4 // slvs-ec + sie1 max support datalan
#define SEN_SLVSEC_SIE2_DL_NUM 2 // slvs-ec + sie2 max support datalan

typedef struct {

	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
} CTL_SEN_CTRL_IF_CSI;

typedef struct {

	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
} CTL_SEN_CTRL_IF_LVDS;

typedef struct {

	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
} CTL_SEN_CTRL_IF_SLVSEC;

typedef struct {

	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id);
	INT32(*stop)(CTL_SEN_ID id);
	INT32(*reset)(CTL_SEN_ID id, CTL_SEN_OUTPUT_DEST output_dest, BOOL en);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, void *value); // pls used KDRV_TGE_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, void *value); // pls used KDRV_TGE_PARAM_ID as cfg_id input
} CTL_SEN_CTRL_IF_TGE;

typedef struct {
	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input

} CTL_SEN_CTRL_IF_GLB;

typedef struct {
	INT32(*open)(CTL_SEN_ID id);
	INT32(*close)(CTL_SEN_ID id);
	BOOL(*is_opend)(CTL_SEN_ID id);

	INT32(*start)(CTL_SEN_ID id, BOOL b_sen_chgmode);
	INT32(*stop)(CTL_SEN_ID id);

	CTL_SEN_INTE(*wait_interrupt)(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);

	INT32(*set_mode_cfg)(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest);

	INT32(*set_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input
	INT32(*get_cfg)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used KDRV_SSENIF_PARAM_ID as cfg_id input

	INT32(*set_cfg_tge)(CTL_SEN_ID id, UINT32 cfg_id, void *value); // pls used KDRV_TGE_PARAM_ID as cfg_id input
	INT32(*get_cfg_tge)(CTL_SEN_ID id, UINT32 cfg_id, void *value); // pls used KDRV_TGE_PARAM_ID as cfg_id input

	INT32(*set_cfg_kflow)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value); // pls used CTL_SEN_CFGID as cfg_id input
	INT32(*get_cfg_kflow)(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value); // pls used CTL_SEN_CFGID as cfg_id input

} CTL_SEN_CTRL_IF;

extern UINT32   g_ctl_sen_if_kdrv_hdl[CTL_SEN_ID_MAX_SUPPORT];
extern CTL_SEN_CTRL_IF *sen_ctrl_if_get_obj(void);
extern CTL_SEN_CTRL_IF_CSI *sen_ctrl_if_csi_get_obj(void);
extern CTL_SEN_CTRL_IF_LVDS *sen_ctrl_if_lvds_get_obj(void);
extern CTL_SEN_CTRL_IF_SLVSEC *sen_ctrl_if_slvsec_get_obj(void);
extern CTL_SEN_CTRL_IF_TGE *sen_ctrl_if_tge_get_obj(void);
extern CTL_SEN_CTRL_IF_GLB *sen_ctrl_if_glb_get_obj(void);

extern BOOL sen_csi_get_force_dis(CTL_SEN_ID id);
extern UINT8 sen_ctrl_if_csi_get_last_stop_idx(CTL_SEN_ID id);

#endif //_SENSOR_CTRL_IF_INT_H_
