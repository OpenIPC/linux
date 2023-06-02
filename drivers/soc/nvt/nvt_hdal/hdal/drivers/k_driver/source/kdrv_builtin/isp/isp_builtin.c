#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>

#include "mach/rcw_macro.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include "kwrap/type.h"
#include "bridge.h"

#include "sie_init.h"
#include "sie_lib.h"
#include "kdrv_ipp_builtin.h"
#include "vdoenc_builtin.h"
#include "ae_param.h"
#include "ae_builtin.h"
#include "awb_param.h"
#include "awb_builtin.h"
#include "iq_builtin.h"
#include "isp_builtin.h"
#include "sensor_builtin.h"
#if (_2A_BUILTIN_NVT)
#include "ae_builtin_nvt.h"
#include "awb_builtin_nvt.h"
#endif

//=============================================================================
// default dtsi
//=============================================================================
#define IQ_NODE_PATH_1                  "/fastboot/iq"
#define IQ_DPC_NODE_PATH_1              "/fastboot/iq_dpc"
#define IQ_SHADING_NODE_PATH_1          "/fastboot/iq_shading"
#define IQ_LDC_NODE_PATH_1              "/fastboot/iq_ldc"
#define IQ_NODE_PATH_2                  "/fastboot/iq_1"
#define IQ_DPC_NODE_PATH_2              "/fastboot/iq_dpc_1"
#define IQ_SHADING_NODE_PATH_2          "/fastboot/iq_shading_1"
#define IQ_LDC_NODE_PATH_2              "/fastboot/iq_ldc_1"

#define IQ_SHDR_NODE_PATH_1             "/fastboot/iq_shdr"
#define IQ_SHDR_DPC_NODE_PATH_1         "/fastboot/iq_dpc_shdr"
#define IQ_SHDR_SHADING_NODE_PATH_1     "/fastboot/iq_shading_shdr"
#define IQ_SHDR_LDC_NODE_PATH_1         "/fastboot/iq_ldc_shdr"
#define IQ_SHDR_NODE_PATH_2             "/fastboot/iq_shdr_2"
#define IQ_SHDR_DPC_NODE_PATH_2         "/fastboot/iq_dpc_shdr_2"
#define IQ_SHDR_SHADING_NODE_PATH_2     "/fastboot/iq_shading_shdr_2"
#define IQ_SHDR_LDC_NODE_PATH_2         "/fastboot/iq_ldc_shdr_2"

#define IQ_IR_NODE_PATH_1               "/fastboot/iq_ir"
#define IQ_IR_DPC_NODE_PATH_1           "/fastboot/iq_dpc_ir"
#define IQ_IR_SHADING_NODE_PATH_1       "/fastboot/iq_shading_ir"
#define IQ_IR_LDC_NODE_PATH_1           "/fastboot/iq_ldc_ir"
#define IQ_IR_NODE_PATH_2               "/fastboot/iq_ir_2"
#define IQ_IR_DPC_NODE_PATH_2           "/fastboot/iq_dpc_ir_2"
#define IQ_IR_SHADING_NODE_PATH_2       "/fastboot/iq_shading_ir_2"
#define IQ_IR_LDC_NODE_PATH_2           "/fastboot/iq_ldc_ir_2"

#define AE_NODE_PATH_1                  "/fastboot/ae"
#define AE_NODE_PATH_2                  "/fastboot/ae_1"

#define AE_SHDR_NODE_PATH_1             "/fastboot/ae_shdr"
#define AE_SHDR_NODE_PATH_2             "/fastboot/ae_shdr_1"

#define AE_IR_NODE_PATH_1               "/fastboot/ae_ir"
#define AE_IR_NODE_PATH_2               "/fastboot/ae_ir_2"

#define AWB_NODE_PATH_1                 "/fastboot/awb"
#define AWB_NODE_PATH_2                 "/fastboot/awb_1"

#define AWB_SHDR_NODE_PATH_1            "/fastboot/awb_shdr"
#define AWB_SHDR_NODE_PATH_2            "/fastboot/awb_shdr_1"

#define AWB_IR_NODE_PATH_1              "/fastboot/awb_ir"
#define AWB_IR_NODE_PATH_2              "/fastboot/awb_ir_2"

//=============================================================================
// global
//=============================================================================
void *isp_builtin_ca_rslt[ISP_BUILTIN_ID_MAX_NUM] = {NULL, NULL, NULL, NULL, NULL};
void *isp_builtin_la_rslt[ISP_BUILTIN_ID_MAX_NUM] = {NULL, NULL, NULL, NULL, NULL};
void *isp_builtin_histo_rslt[ISP_BUILTIN_ID_MAX_NUM] = {NULL, NULL, NULL, NULL, NULL};
static UINT32 ct[ISP_BUILTIN_ID_MAX_NUM] = {5500, 5500, 5500, 5500, 5500};
static UINT32 lv[ISP_BUILTIN_ID_MAX_NUM] = {8000000, 8000000, 8000000, 8000000, 8000000};
static UINT32 overexposure_offset[ISP_BUILTIN_ID_MAX_NUM] = {0};
static UINT32 total_gain[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static ISP_BUILTIN_AE_STATUS ae_status[ISP_BUILTIN_ID_MAX_NUM] = {ISP_BUILTIN_AE_STATUS_STABLE, ISP_BUILTIN_AE_STATUS_STABLE, ISP_BUILTIN_AE_STATUS_STABLE, ISP_BUILTIN_AE_STATUS_STABLE, ISP_BUILTIN_AE_STATUS_STABLE};
static ISP_BUILTIN_CGAIN cgain[ISP_BUILTIN_ID_MAX_NUM] = {{256, 256, 256}, {256, 256, 256}, {256, 256, 256}, {256, 256, 256}, {256, 256, 256}};
static ISP_BUILTIN_SENSOR_CTRL sensor_ctrl[ISP_BUILTIN_ID_MAX_NUM] = {{{10000, 625}, {1000, 1000}}, {{10000, 625}, {1000, 1000}}, {{10000, 625}, {1000, 1000}}, {{10000, 625}, {1000, 1000}}, {{10000, 625}, {1000, 1000}}};
static UINT32 dgain[ISP_BUILTIN_ID_MAX_NUM] = {128, 128, 128, 128, 128};
#if (NVT_FAST_ISP_FLOW)
static SIE_CA_WIN_INFO sie_ca_win_info[ISP_BUILTIN_ID_MAX_NUM] = {{31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}};
static SIE_LA_WIN_INFO sie_la_win_info[ISP_BUILTIN_ID_MAX_NUM] = {{31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}, {31, 31, 10, 8}};
static BOOL isp_builtin_task_open = FALSE;
static BOOL isp_builtin_direct_mode = TRUE;
static UINT32 isp_sensor_path_2 = 1;
static UINT32 shdr_map_path[ISP_BUILTIN_ID_MAX_NUM] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
#endif
static UINT32 scene_chg_w[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 sensor_name[ISP_BUILTIN_ID_MAX_NUM] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};  // -1: not available, 0: imx290, 1: f37, 2: os02k10, 3: os05a10, 4: f35, 5: gc4653
static UINT32 sensor_chgmode_fps[ISP_BUILTIN_ID_MAX_NUM] = {3000, 3000, 3000, 3000, 3000};
static UINT32 sensor_expt_max[ISP_BUILTIN_ID_MAX_NUM] = {30000, 30000, 3000, 3000, 3000};
static UINT32 sensor_i2c_id[ISP_BUILTIN_ID_MAX_NUM] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static UINT32 sensor_i2c_addr[ISP_BUILTIN_ID_MAX_NUM];
static BOOL sensor_valid[ISP_BUILTIN_ID_MAX_NUM];
static BOOL shdr_enable[ISP_BUILTIN_ID_MAX_NUM];
static UINT32 shdr_id_mask[ISP_BUILTIN_ID_MAX_NUM];
static UINT32 nr_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 _3dnr_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 sharpness_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 saturation_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 contrast_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 brightness_lv[ISP_BUILTIN_ID_MAX_NUM] = {100, 100, 100, 100, 100};
static UINT32 night_mode[ISP_BUILTIN_ID_MAX_NUM];
static ISP_BUILTIN_DTSI isp_builtin_dtsi[ISP_BUILTIN_DTSI_MAX] = {
	{ IQ_NODE_PATH_1, IQ_DPC_NODE_PATH_1, IQ_SHADING_NODE_PATH_1, IQ_LDC_NODE_PATH_1, AE_NODE_PATH_1, AWB_NODE_PATH_1 },
	{ IQ_NODE_PATH_1, IQ_DPC_NODE_PATH_1, IQ_SHADING_NODE_PATH_1, IQ_LDC_NODE_PATH_1, AE_NODE_PATH_1, AWB_NODE_PATH_1 }
};

THREAD_HANDLE isp_builtin_task_id;
ID isp_builtin_flag_id;

//=============================================================================
// function declaration
//=============================================================================
extern void nvt_bootts_add_ts(char *name);
#if (NVT_FAST_ISP_FLOW)
static UINT32 isp_builtin_get_src_id(UINT32 id);
#endif

//=============================================================================
// internal functions
//=============================================================================
#if (NVT_FAST_ISP_FLOW)
static void isp_builtin_get_win_info(UINT32 id)
{
	iq_builtin_get_win_info(id, &sie_ca_win_info[id], &sie_la_win_info[id]);
}

#if ((ISP_BUILTIN_AWB_TRIG_MODE == 0) || (ISP_BUILTIN_AWB_TRIG_MODE == 1))
static void isp_builtin_set_ca_crop(UINT32 id, BOOL enable)
{
	iq_builtin_set_ca_crop(id, enable);
}
#endif

static void isp_builtin_cb_sie(UINT32 id, UINT32 status)
{
	static UINT32 shdr_frame_cnt_dram_end[ISP_BUILTIN_ID_MAX_NUM];
	static UINT32 shdr_frame_cnt_vd[ISP_BUILTIN_ID_MAX_NUM];
	static UINT32 src_id_mask_dram_end[ISP_BUILTIN_ID_MAX_NUM] = {0};
	static UINT32 src_id_mask_vd[ISP_BUILTIN_ID_MAX_NUM] = {0};
	static UINT32 src_id;

	#if (ISP_BUILTIN_TRIG_2A_IQ == 0)
	return;
	#endif

	src_id = isp_builtin_get_src_id(id);

	if (status == SIE_INT_ALL) { 
		isp_builtin_direct_mode = iq_builtin_get_direct_mode();

		#if (ISP_BUILTIN_PRINT_VD_CNT)
		printk("vd  rst id = %d, src id = %d, direct mode = %d \r\n", id, src_id, isp_builtin_direct_mode);
		#endif

		if (src_id == 0) {  // 1st sensor
			set_flg(isp_builtin_flag_id, FLGPTN_SIE_RESET);
		} else if (sensor_valid[src_id]) {  // 2nd sensor
			set_flg(isp_builtin_flag_id, FLGPTN_SIE_RESET_2);
		}
		return;
	}

	if (status & SIE_INT_DRAM_OUT1_END) {
		if (shdr_enable[src_id]) {
			shdr_frame_cnt_dram_end[src_id]++;
			src_id_mask_dram_end[src_id] |= 1 << id;

			if (shdr_frame_cnt_dram_end[src_id] == ISP_SEN_MFRAME_MAX_NUM) {
				if (src_id_mask_dram_end[src_id] == shdr_id_mask[src_id]) {
					if (src_id == 0) {
						set_flg(isp_builtin_flag_id, FLGPTN_PROC_DRAMEND);
					} else if (sensor_valid[src_id]) {
						set_flg(isp_builtin_flag_id, FLGPTN_PROC_DRAMEND_2);
					}
				} else {
					printk("warning: dram end trig, src id = %d, mask %d == %d ? \r\n", src_id, src_id_mask_dram_end[src_id], shdr_id_mask[src_id]);
				}
				shdr_frame_cnt_dram_end[src_id] = 0;
				src_id_mask_dram_end[src_id] = 0;
			} 
		} else {
			if (src_id == 0) {
				set_flg(isp_builtin_flag_id, FLGPTN_PROC_DRAMEND);
			} else if (sensor_valid[src_id]) {
				set_flg(isp_builtin_flag_id, FLGPTN_PROC_DRAMEND_2);
			}
		}
	}

	if (status & SIE_INT_VD) {
		#if (ISP_BUILTIN_PRINT_VD_CNT)
		{
			static UINT32 print_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};

			print_cnt[id] ++;
			if (print_cnt[id] < ISP_BUILTIN_PRINT_VD_CNT) {
				printk(" vd cnt = %d, id = %d, src id = %d \r\n", print_cnt[id], id, src_id);
			}
		}
		#endif

		if (shdr_enable[src_id]) {
			shdr_frame_cnt_vd[src_id] ++;
			src_id_mask_vd[src_id] |= 1 << id;

			if (shdr_frame_cnt_vd[src_id] == ISP_SEN_MFRAME_MAX_NUM) {
				if (src_id_mask_vd[src_id] == shdr_id_mask[src_id]) {
					if (src_id == 0) {
						set_flg(isp_builtin_flag_id, FLGPTN_PROC_VD);
					} else if (sensor_valid[src_id]) {
						set_flg(isp_builtin_flag_id, FLGPTN_PROC_VD_2);
					}
				} else {
					printk("warning: vd trig, src id = %d, mask %d == %d ? \r\n", src_id, src_id_mask_vd[src_id], shdr_id_mask[src_id]);
				}
				shdr_frame_cnt_vd[src_id] = 0;
				src_id_mask_vd[src_id] = 0;
			}
		} else {
			if (src_id == 0) {
				set_flg(isp_builtin_flag_id, FLGPTN_PROC_VD);
			} else if (sensor_valid[src_id]) {
				set_flg(isp_builtin_flag_id, FLGPTN_PROC_VD_2);
			}
		}
	}
}

static void isp_builtin_cb_ipp(UINT32 id, UINT32 status)
{
	#if (ISP_BUILTIN_TRIG_2A_IQ == 0)
	return;
	#endif

	if (status == KDRV_IPP_BUILTIN_ISP_EVENT_RESET) {
		isp_builtin_direct_mode = iq_builtin_get_direct_mode();

		#if (ISP_BUILTIN_PRINT_IPP_CNT)
		printk("ipp rst id = %d, direct mode = %d \r\n", id, isp_builtin_direct_mode);
		#endif
		iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_IPP_RESET);
	}

	if ((status == KDRV_IPP_BUILTIN_ISP_EVENT_TRIG) && (!isp_builtin_direct_mode)) {
		#if (ISP_BUILTIN_PRINT_IPP_CNT)
		{
			static UINT32 print_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};

			print_cnt[id] ++;
			if (print_cnt[id] < ISP_BUILTIN_PRINT_IPP_CNT) {
				printk("ipp cnt = %d, id = %d \r\n", print_cnt[id], id);
			}

		}
		#endif
		iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_CGAIN);
		iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_IPP);
	}
}

static void isp_builtin_cb_enc(UINT32 id, UINT32 status)
{
	#if (ISP_BUILTIN_TRIG_2A_IQ == 0)
	return;
	#endif

	if (status == BUILTIN_ISP_EVENT_ENC_SHARPEN) {
		iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_ENC);
	}
}

THREAD_DECLARE(isp_builtin_tsk, arglist)
{
	static UINT32 trig_dram_num = 0, trig_vd_num = 0, trig_dram_num2 = 0, trig_vd_num2 = 0;
	static BOOL tag_2a = FALSE;
	UINT32 id;
	FLGPTN flag = FLGPTN_IDLE;

	THREAD_ENTRY();

	while (1) {
		set_flg(isp_builtin_flag_id, FLGPTN_IDLE);
		wai_flg(&flag, isp_builtin_flag_id, FLGPTN_SIE_RESET | FLGPTN_PROC_DRAMEND | FLGPTN_PROC_VD | 
											FLGPTN_SIE_RESET_2 | FLGPTN_PROC_DRAMEND_2 | FLGPTN_PROC_VD_2 | 
											FLGPTN_STOP, TWF_ORW | TWF_CLR);

		// NOTE: path 1
		if (flag & FLGPTN_SIE_RESET) {
			id = 0;
			#if (_2A_BUILTIN_NVT)
			ae_builtin_trig_nvt(id, ISP_BUILTIN_AE_TRIG_RESET);
			awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_RESET);
			#else
			ae_builtin_trig(id, ISP_BUILTIN_AE_TRIG_RESET);
			awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_RESET);
			#endif
			iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_SIE_RESET);
			isp_builtin_get_win_info(id);
			#if ((ISP_BUILTIN_AWB_TRIG_MODE == 0) || (ISP_BUILTIN_AWB_TRIG_MODE == 1))
			isp_builtin_set_ca_crop(id, ENABLE);
			#endif
		}

		if (flag & FLGPTN_PROC_DRAMEND) {
			id = 0;
			trig_dram_num ++;
			#if ((ISP_BUILTIN_AWB_TRIG_MODE == 0) || (ISP_BUILTIN_AWB_TRIG_MODE == 1))
			#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
			if (trig_dram_num == 1) { // first dramend
			#endif
				#if (_2A_BUILTIN_NVT)
				awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_PROC);
				#else
				awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_PROC);
				#endif
				if (isp_builtin_direct_mode) {
					iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_CGAIN);
				}
			#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				isp_builtin_set_ca_crop(id, DISABLE);
			}
			#endif
			if ((trig_dram_num == 2) && !tag_2a) {
				nvt_bootts_add_ts("2A"); //end
				tag_2a = TRUE;
			}
			#endif

			if ((trig_dram_num == 1) && isp_builtin_direct_mode) {  // 1st ipp trigger move to dram end.
				iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_IPP);
			}
		}

		if (flag & FLGPTN_PROC_VD) {
			id = 0;
			trig_vd_num ++;
			if (trig_vd_num > 1) {
				#if ((ISP_BUILTIN_AWB_TRIG_MODE == 1) ||(ISP_BUILTIN_AWB_TRIG_MODE == 2))
				#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				if (trig_vd_num > 2) {
				#endif
					#if (_2A_BUILTIN_NVT)
					awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_PROC);
					#else
					awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_PROC);
					#endif
					if (isp_builtin_direct_mode) {
						iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_CGAIN);
					}
				#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				}
				#endif
				#endif
				#if (_2A_BUILTIN_NVT)
				ae_builtin_trig_nvt(id, ISP_BUILTIN_AE_TRIG_PROC);
				#else
				ae_builtin_trig(id, ISP_BUILTIN_AE_TRIG_PROC);
				#endif
			}

			iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_SIE);
			if ((trig_vd_num > 1) && isp_builtin_direct_mode) {  // 1st ipp trigger move to dram end.
				iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_IPP);
			}

			#if (ISP_BUILTIN_AWB_TRIG_MODE == 2)
			if ((trig_vd_num == 3) && !tag_2a) {
				nvt_bootts_add_ts("2A"); //end
				tag_2a = TRUE;
			}
			#endif
		}

		// NOTE: path 2
		if (flag & FLGPTN_SIE_RESET_2) {
			id = isp_sensor_path_2;
			#if (_2A_BUILTIN_NVT)
			ae_builtin_trig_nvt(id, ISP_BUILTIN_AE_TRIG_RESET);
			awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_RESET);
			#else
			ae_builtin_trig(id, ISP_BUILTIN_AE_TRIG_RESET);
			awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_RESET);
			#endif
			iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_SIE_RESET);
			isp_builtin_get_win_info(id);
			#if ((ISP_BUILTIN_AWB_TRIG_MODE == 0) || (ISP_BUILTIN_AWB_TRIG_MODE == 1))
			isp_builtin_set_ca_crop(id, ENABLE);
			#endif
		}

		if (flag & FLGPTN_PROC_DRAMEND_2) {
			id = isp_sensor_path_2;
			trig_dram_num2 ++;
			#if ((ISP_BUILTIN_AWB_TRIG_MODE == 0) || (ISP_BUILTIN_AWB_TRIG_MODE == 1))
			#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
			if (trig_dram_num2 == 1) { // first dramend
			#endif
			#if (_2A_BUILTIN_NVT)
				awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_PROC);
			#else
				awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_PROC);
			#endif
				if (isp_builtin_direct_mode) {
					iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_CGAIN);
				}
			#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				isp_builtin_set_ca_crop(id, DISABLE);
			}
			#endif
			#endif
		}

		if (flag & FLGPTN_PROC_VD_2) {
			id = isp_sensor_path_2;
			trig_vd_num2 ++;
			if (trig_vd_num2 > 1) {
				#if ((ISP_BUILTIN_AWB_TRIG_MODE == 1) ||(ISP_BUILTIN_AWB_TRIG_MODE == 2))
				#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				if (trig_vd_num2 > 2) {
				#endif
					#if (_2A_BUILTIN_NVT)
					awb_builtin_trig_nvt(id, ISP_BUILTIN_AWB_TRIG_PROC);
					#else
					awb_builtin_trig(id, ISP_BUILTIN_AWB_TRIG_PROC);
					#endif
					if (isp_builtin_direct_mode) {
						iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_CGAIN);
					}
				#if (ISP_BUILTIN_AWB_TRIG_MODE == 1)
				}
				#endif
				#endif
				#if (_2A_BUILTIN_NVT)
				ae_builtin_trig_nvt(id, ISP_BUILTIN_AE_TRIG_PROC);
				#else
				ae_builtin_trig(id, ISP_BUILTIN_AE_TRIG_PROC);
				#endif
			}

			iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_SIE);
			if (isp_builtin_direct_mode) {
				iq_builtin_trig(id, ISP_BUILTIN_IQ_TRIG_IPP);
			}
		}

		if (flag & FLGPTN_STOP) {
			printk("ISP_BUILTIN_STOP \n");
			break;
		}
	}

	return 0;
}

static UINT32 isp_builtin_get_src_id(UINT32 id)
{
	UINT32 src_id = 0;

	if ((id == 0) || (id == shdr_map_path[0])) {
		src_id = 0;
	} else if ((id == isp_sensor_path_2) || (id == shdr_map_path[isp_sensor_path_2])) {
		src_id = isp_sensor_path_2;
	}

	return src_id;
}

#endif

//=============================================================================
// external functions
//=============================================================================
ISP_BUILTIN_CA_RSLT *isp_builtin_get_ca(UINT32 id)
{
	#if (NVT_FAST_ISP_FLOW)
	UINT32 addr1,addr2,addr3;
	ISP_BUILTIN_CA_RSLT *ca_rslt = (ISP_BUILTIN_CA_RSLT *)isp_builtin_ca_rslt[id];
	SIE_STCS_CA_RSLT_INFO ca_rst_info;

	if (ca_rslt == NULL) {
		return NULL;
	}

	ca_rst_info.puiBufR = ca_rslt->r;
	ca_rst_info.puiBufG = ca_rslt->g;
	ca_rst_info.puiBufB = ca_rslt->b;
	ca_rst_info.puiBufIR = ca_rslt->ir;
	ca_rst_info.puiAccCnt = ca_rslt->acc_cnt;
	// NOTE: addr2 is CA
	sie_fb_get_rdy_addr(id, &addr1, &addr2, &addr3);
	sie_get_ca_rslt(id, &ca_rst_info, &sie_ca_win_info[id], addr2);

	return ca_rslt;
	#else
	return NULL;
	#endif
}

ISP_BUILTIN_LA_RSLT *isp_builtin_get_la(UINT32 id)
{
	#if (NVT_FAST_ISP_FLOW)
	UINT32 addr1, addr2, addr3;
	ISP_BUILTIN_LA_RSLT *la_rslt = (ISP_BUILTIN_LA_RSLT *)isp_builtin_la_rslt[id];

	if (la_rslt == NULL) {
		return NULL;
	}

	// NOTE: addr3 is LA
	sie_fb_get_rdy_addr(id, &addr1, &addr2, &addr3);
	sie_get_la_rslt(id, la_rslt->lum_1, la_rslt->lum_2, &sie_la_win_info[id], addr3);

	return la_rslt;
	#else
	return NULL;
	#endif
}

ISP_BUILTIN_HISTO_RSLT *isp_builtin_get_histo(UINT32 id)
{
	#if (NVT_FAST_ISP_FLOW)
	ISP_BUILTIN_HISTO_RSLT *histo_rslt = (ISP_BUILTIN_HISTO_RSLT *)isp_builtin_histo_rslt[id];

	iq_builtin_get_histo(id, histo_rslt);
	
	return histo_rslt;
	#else
	return NULL;
	#endif
}

UINT32 isp_builtin_get_i2c_id(UINT32 id)
{
	return sensor_i2c_id[id];
}

UINT32 isp_builtin_get_i2c_addr(UINT32 id)
{
	return sensor_i2c_addr[id];
}

BOOL isp_builtin_get_shdr_enable(UINT32 id)
{
	return shdr_enable[id];
}

UINT32 isp_builtin_get_shdr_id_mask(UINT32 id)
{
	return shdr_id_mask[id];
}

UINT32 isp_builtin_get_nr_lv(UINT32 id)
{
	return nr_lv[id];
}

UINT32 isp_builtin_get_3dnr_lv(UINT32 id)
{
	return _3dnr_lv[id];
}

UINT32 isp_builtin_get_sharpness_lv(UINT32 id)
{
	return sharpness_lv[id];
}

UINT32 isp_builtin_get_saturation_lv(UINT32 id)
{
	return saturation_lv[id];
}

UINT32 isp_builtin_get_contrast_lv(UINT32 id)
{
	return contrast_lv[id];
}

UINT32 isp_builtin_get_brightness_lv(UINT32 id)
{
	return brightness_lv[id];
}

UINT32 isp_builtin_get_night_mode(UINT32 id)
{
	return night_mode[id];
}

UINT32 isp_builtin_get_sensor_name(UINT32 id)
{
	// 0: imx290, 1: f37, 2: os02k10, 3: os05a10, 4: f35, 5: gc4653
	return sensor_name[id];
}

UINT32 isp_builtin_get_chgmode_fps(UINT32 id)
{
	return sensor_chgmode_fps[id];
}

UINT32 isp_builtin_get_sensor_expt_max(UINT32 id)
{
	return sensor_expt_max[id];
}

UINT32 isp_builtin_get_total_gain(UINT32 id)
{
	return total_gain[id];
}

UINT32 isp_builtin_get_ct(UINT32 id)
{
	return ct[id];
}

UINT32 isp_builtin_get_lv(UINT32 id)
{
	return lv[id];
}

UINT32 isp_builtin_get_scene_chg_w(UINT32 id)
{
	return scene_chg_w[id];
}

UINT32 isp_builtin_get_ae_status(UINT32 id)
{
	return ae_status[id];
}

ISP_BUILTIN_CGAIN *isp_builtin_get_cgain(UINT32 id)
{
	return &cgain[id];
}

UINT32 isp_builtin_get_dgain(UINT32 id)
{
	return dgain[id];
}

ISP_BUILTIN_SENSOR_CTRL *isp_builtin_get_sensor_gain(UINT32 id)
{
	return &sensor_ctrl[id];
}

ISP_BUILTIN_SENSOR_CTRL *isp_builtin_get_sensor_expt(UINT32 id)
{
	return &sensor_ctrl[id];
}

BOOL isp_builtin_get_sensor_valid(UINT32 id)
{
	return sensor_valid[id];
}

UINT32 isp_builtin_get_sensor_row_time(UINT32 id)
{
	#if (NVT_FAST_ISP_FLOW)
	return sensor_builtin_get_row_time(id);
	#else
	return 0;
	#endif
}

UINT32 isp_builtin_get_overexposure_offset(UINT32 id)
{
	return overexposure_offset[id];
}

UINT32 isp_builtin_get_isp_version(void) {

	return ISP_BUILTIN_VERSION;
}

UINT32 isp_builtin_get_ae_version(void) {

	return AE_BUILTIN_VERSION;
}

UINT32 isp_builtin_get_awb_version(void) {

	return AWB_BUILTIN_VERSION;
}

UINT32 isp_builtin_get_iq_version(void) {

	return IQ_BUILTIN_VERSION;
}

UINT32 isp_builtin_get_sensor_version(void) {

	return SENSOR_BUILTIN_VERSION;
}

UINT32 isp_builtin_get_ae_msg_cnt(void) {

	return ISP_BUILTIN_PRINT_AE_MSG_CNT;
}

UINT32 isp_builtin_get_awb_msg_cnt(void) {

	return ISP_BUILTIN_PRINT_AWB_MSG_CNT;
}

void isp_builtin_get_ae_param(UINT32 id, UINT32 *param_addr)
{
	#if (NVT_FAST_ISP_FLOW)
	ae_builtin_get_param(id, param_addr);
	#else
	*param_addr = 0;
	#endif
}

void isp_builtin_get_awb_param(UINT32 id, UINT32 *param_addr)
{
	#if (NVT_FAST_ISP_FLOW)
	awb_builtin_get_param(id, param_addr);
	#else
	*param_addr = 0;
	#endif
}

void isp_builtin_get_iq_param(UINT32 id, UINT32 *param_addr)
{
	#if (NVT_FAST_ISP_FLOW)
	iq_builtin_get_param(id, param_addr);
	#else
	*param_addr = 0;
	#endif
}

ISP_BUILTIN_DTSI *isp_builtin_get_dtsi(UINT32 id)
{
	if ((id == 0) || (id >= ISP_BUILTIN_ID_MAX_NUM)) {
		return &isp_builtin_dtsi[0];
	} else {
		return &isp_builtin_dtsi[1];
	}
}

void isp_builtin_uninit_i2c(UINT32 id)
{
	sensor_i2c_id[id] = 0xFFFFFFFF;
	sensor_i2c_addr[id] = 0x0;

	#if (NVT_FAST_ISP_FLOW)
	sensor_builtin_uninit_i2c(id);
	#endif
}

INT32 isp_builtin_set_transfer_i2c(UINT32 id, void *msgs, INT32 num)
{
	#if (NVT_FAST_ISP_FLOW)
	return sensor_builtin_set_transfer_i2c(id, msgs, num);
	#else
	return -1;
	#endif
}

void isp_builtin_set_total_gain(UINT32 id, UINT32 value)
{
	total_gain[id] = value;
}

void isp_builtin_set_ct(UINT32 id, UINT32 value)
{
	ct[id] = value;
}

void isp_builtin_set_lv(UINT32 id, UINT32 value)
{
	lv[id] = value;
}

void isp_builtin_set_scene_chg_w(UINT32 id, UINT32 value)
{
	scene_chg_w[id] = value;
}

void isp_builtin_set_ae_status(UINT32 id, ISP_BUILTIN_AE_STATUS value)
{
	ae_status[id] = value;
}

void isp_builtin_set_cgain(UINT32 id, ISP_BUILTIN_CGAIN *value)
{
	ISP_BUILTIN_CGAIN *cgain_temp = (ISP_BUILTIN_CGAIN *) value;
	cgain[id].r = cgain_temp->r;
	cgain[id].g = cgain_temp->g;
	cgain[id].b = cgain_temp->b;
}

void isp_builtin_set_dgain(UINT32 id, UINT32 value)
{
	dgain[id] = value;
}

void isp_builtin_set_sensor_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value)
{
	#if (NVT_FAST_ISP_FLOW)
	ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl_temp = (ISP_BUILTIN_SENSOR_CTRL *) value;

	sensor_ctrl[id].gain_ratio[0] = sensor_ctrl_temp->gain_ratio[0];
	sensor_ctrl[id].gain_ratio[1] = sensor_ctrl_temp->gain_ratio[1];

	#if (ISP_BUILTIN_PRINT_GAIN_CNT)
	{
		static UINT32 print_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};
		UINT32 src_id = isp_builtin_get_src_id(id);

		print_cnt[src_id] ++;
		if (print_cnt[src_id] < ISP_BUILTIN_PRINT_GAIN_CNT) {
			printk("cnt = %d, id = %d, src_id  = %d, gain 0 = %d, gain 1 = %d, d gain = %d \r\n", print_cnt[src_id], id, src_id, sensor_ctrl[src_id].gain_ratio[0], sensor_ctrl[src_id].gain_ratio[1], dgain[src_id]);
		}
	}
	#endif

	sensor_builtin_set_gain(id, &sensor_ctrl[id]);
	#endif
}

void isp_builtin_set_sensor_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value)
{
	#if (NVT_FAST_ISP_FLOW)
	ISP_BUILTIN_SENSOR_CTRL *sensor_ctrl_temp = (ISP_BUILTIN_SENSOR_CTRL *) value;

	sensor_ctrl[id].exp_time[0] = sensor_ctrl_temp->exp_time[0];
	sensor_ctrl[id].exp_time[1] = sensor_ctrl_temp->exp_time[1];

	#if (ISP_BUILTIN_PRINT_EXPT_CNT)
	{
		static UINT32 print_cnt[ISP_BUILTIN_ID_MAX_NUM] = {0};
		UINT32 src_id = isp_builtin_get_src_id(id);

		print_cnt[src_id] ++;
		if (print_cnt[src_id] < ISP_BUILTIN_PRINT_EXPT_CNT) {
			printk("cnt = %d, id = %d, src_id  = %d, expt 0 = %d, expt 1 = %d \r\n", print_cnt[src_id], id, src_id, sensor_ctrl[src_id].exp_time[0], sensor_ctrl[src_id].exp_time[1]);
		}
	}
	#endif

	sensor_builtin_set_expt(id, &sensor_ctrl[id]);
	#endif
}

void isp_builtin_set_overexposure_offset(UINT32 id, UINT32 value)
{
	overexposure_offset[id] = value;
}

#if (NVT_FAST_ISP_FLOW)
ER isp_builtin_init(void)
{
	ISP_BUILTIN_CGAIN cgain_bridge = {0, 0, 0};
	UINT32 id, sensor_name_1, sensor_name_2;

	// get sensor expt/gain from rtos
	kdrv_bridge_get_tag(SENSOR_PRESET_NAME, &sensor_name_1);
	if (sensor_name_1 != 0xFFFFFFFF) {
		id = 0;

		sensor_name[id] = sensor_name_1;
		sensor_valid[id] = TRUE;
		kdrv_bridge_get_tag(SENSOR_CHGMODE_FPS, &sensor_chgmode_fps[id]);
		kdrv_bridge_get_tag(SENSOR_PRESET_EXPT, &sensor_ctrl[id].exp_time[0]);
		kdrv_bridge_get_tag(SENSOR_PRESET_GAIN, &sensor_ctrl[id].gain_ratio[0]);
		kdrv_bridge_get_tag(SENSOR_EXPT_MAX, &sensor_expt_max[id]);
		kdrv_bridge_get_tag(SENSOR_I2C_ID, &sensor_i2c_id[id]);
		kdrv_bridge_get_tag(SENSOR_I2C_ADDR, &sensor_i2c_addr[id]);
		kdrv_bridge_get_tag(ISP_D_GAIN, &dgain[id]);
		kdrv_bridge_get_tag(ISP_R_GAIN, &cgain_bridge.r);
		kdrv_bridge_get_tag(ISP_G_GAIN, &cgain_bridge.g);
		kdrv_bridge_get_tag(ISP_B_GAIN, &cgain_bridge.b);
		kdrv_bridge_get_tag(ISP_SHDR_ENABLE, &shdr_enable[id]);
		kdrv_bridge_get_tag(ISP_SHDR_PATH, &shdr_map_path[id]);
		kdrv_bridge_get_tag(ISP_SHDR_MASK, &shdr_id_mask[id]);
		kdrv_bridge_get_tag(ISP_NR_LV, &nr_lv[id]);
		kdrv_bridge_get_tag(ISP_3DNR_LV, &_3dnr_lv[id]);
		kdrv_bridge_get_tag(ISP_SHARPNESS_LV, &sharpness_lv[id]);
		kdrv_bridge_get_tag(ISP_SATURATION_LV, &saturation_lv[id]);
		kdrv_bridge_get_tag(ISP_CONTRAST_LV, &contrast_lv[id]);
		kdrv_bridge_get_tag(ISP_BRIGHTNESS_LV, &brightness_lv[id]);
		kdrv_bridge_get_tag(ISP_NIGHT_MODE, &night_mode[id]);
		printk("sensor(%d), name: 0x%X, i2c: 0x%X, 0x%X, expt: %d, gain: %d, shdr: 0x%X, 0x%X, 0x%X \r\n", id, sensor_name[id], sensor_i2c_id[id], sensor_i2c_addr[id], sensor_ctrl[id].exp_time[0], sensor_ctrl[id].gain_ratio[0], shdr_enable[id], shdr_map_path[id], shdr_id_mask[id]);
	} else {
		printk("sensor1 not available, name = 0x%X \r\n", sensor_name_1);
	}

	kdrv_bridge_get_tag(SENSOR_PRESET_NAME_2, &sensor_name_2);
	if (sensor_name_2 != 0xFFFFFFFF) {
		kdrv_bridge_get_tag(ISP_PATH_2, &isp_sensor_path_2);
		id = isp_sensor_path_2;

		sensor_name[id] = sensor_name_2;
		sensor_valid[id] = TRUE;
		kdrv_bridge_get_tag(SENSOR_CHGMODE_FPS_2, &sensor_chgmode_fps[id]);
		kdrv_bridge_get_tag(SENSOR_PRESET_EXPT_2, &sensor_ctrl[id].exp_time[0]);
		kdrv_bridge_get_tag(SENSOR_PRESET_GAIN_2, &sensor_ctrl[id].gain_ratio[0]);
		kdrv_bridge_get_tag(SENSOR_EXPT_MAX_2, &sensor_expt_max[id]);
		kdrv_bridge_get_tag(SENSOR_I2C_ID_2, &sensor_i2c_id[id]);
		kdrv_bridge_get_tag(SENSOR_I2C_ADDR_2, &sensor_i2c_addr[id]);
		kdrv_bridge_get_tag(ISP_D_GAIN_2, &dgain[id]);
		kdrv_bridge_get_tag(ISP_R_GAIN_2, &cgain_bridge.r);
		kdrv_bridge_get_tag(ISP_G_GAIN_2, &cgain_bridge.g);
		kdrv_bridge_get_tag(ISP_B_GAIN_2, &cgain_bridge.b);
		kdrv_bridge_get_tag(ISP_SHDR_ENABLE_2, &shdr_enable[id]);
		kdrv_bridge_get_tag(ISP_SHDR_PATH_2, &shdr_map_path[id]);
		kdrv_bridge_get_tag(ISP_SHDR_MASK_2, &shdr_id_mask[id]);
		kdrv_bridge_get_tag(ISP_NR_LV_2, &nr_lv[id]);
		kdrv_bridge_get_tag(ISP_3DNR_LV_2, &_3dnr_lv[id]);
		kdrv_bridge_get_tag(ISP_SHARPNESS_LV_2, &sharpness_lv[id]);
		kdrv_bridge_get_tag(ISP_SATURATION_LV_2, &saturation_lv[id]);
		kdrv_bridge_get_tag(ISP_CONTRAST_LV_2, &contrast_lv[id]);
		kdrv_bridge_get_tag(ISP_BRIGHTNESS_LV_2, &brightness_lv[id]);
		kdrv_bridge_get_tag(ISP_NIGHT_MODE_2, &night_mode[id]);
		printk("sensor(%d), name: 0x%X, i2c: 0x%X, 0x%X, expt: %d, gain: %d, shdr: 0x%X, 0x%X, 0x%X \r\n", id, sensor_name[id], sensor_i2c_id[id], sensor_i2c_addr[id], sensor_ctrl[id].exp_time[0], sensor_ctrl[id].gain_ratio[0], shdr_enable[id], shdr_map_path[id], shdr_id_mask[id]);
	} else {
		printk("sensor2 not available, name = 0x%X \r\n", sensor_name_2);
	}

	if (((sensor_i2c_id[0] == 0xFFFFFFFF) || (sensor_i2c_addr[0] == 0))  && ((sensor_i2c_id[1] == 0xFFFFFFFF) || (sensor_i2c_addr[1] == 0))) {
		printk("get sensor i2c bridge parameters fail, id = %d, addr = 0x%X, id2 = %d, addr2 = 0x%X \n", sensor_i2c_id[0], sensor_i2c_addr[0], sensor_i2c_id[1], sensor_i2c_addr[1]);
		return -E_SYS;
	}

	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++) {
		if (((id == 0) && (sensor_valid[0] || shdr_enable[isp_builtin_get_src_id(0)])) || ((id == isp_sensor_path_2) && (sensor_valid[isp_sensor_path_2] || shdr_enable[isp_builtin_get_src_id(isp_sensor_path_2)]))) {
			printk("id %d, alloc ca & la buffer \r\n", id);
			isp_builtin_ca_rslt[id] = kzalloc(sizeof(ISP_BUILTIN_CA_RSLT)*ISP_SEN_MFRAME_MAX_NUM, GFP_KERNEL);
			if (isp_builtin_ca_rslt[id] == NULL) {
				printk("allocate isp_builtin_ca_rslt fail!\n");
				return -E_SYS;
			}
			isp_builtin_la_rslt[id] = kzalloc(sizeof(ISP_BUILTIN_LA_RSLT)*ISP_SEN_MFRAME_MAX_NUM, GFP_KERNEL);
			if (isp_builtin_la_rslt[id] == NULL) {
				printk("allocate isp_builtin_la_rslt fail!\n");
				return -E_SYS;
			}
			isp_builtin_histo_rslt[id] = kzalloc(sizeof(ISP_BUILTIN_HISTO_RSLT), GFP_KERNEL);
			if (isp_builtin_histo_rslt[id] == NULL) {
				printk("allocate isp_builtin_histo_rslt fail!\n");
				return -E_SYS;
			}
		}
	}

	if (!isp_builtin_task_open) {
		// init task
		OS_CONFIG_FLAG(isp_builtin_flag_id);
		clr_flg(isp_builtin_flag_id, FLGPTN_BIT_ALL);
		THREAD_CREATE(isp_builtin_task_id, isp_builtin_tsk, NULL, "isp_builtin_tsk");
		if (isp_builtin_task_id == 0) {
			printk("ISP THREAD_CREATE fail (%d) \r\n", (int)isp_builtin_task_id);
			return -E_SYS;
		}

		THREAD_RESUME(isp_builtin_task_id);

		isp_builtin_task_open = TRUE;
	}

	// clear all flag
	clr_flg(isp_builtin_flag_id, FLGPTN_IDLE);

	// init i2c
	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++) {
		if (sensor_valid[id]) {
			sensor_builtin_init_i2c(id);
		}
	}

	// modify dtsi if is in night/shdr mode
	for (id = 0; id < ISP_BUILTIN_DTSI_MAX; id++) {
		if (id == 0) {
			if (night_mode[id] == 1) {
				sprintf(isp_builtin_dtsi[id].iq_node_path, IQ_IR_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_dpc_node_path, IQ_IR_DPC_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_shading_node_path, IQ_IR_SHADING_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_ldc_node_path, IQ_IR_LDC_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].ae_node_path, AE_IR_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].awb_node_path, AWB_IR_NODE_PATH_1);
			} else if (shdr_enable[id] == 1) {
				sprintf(isp_builtin_dtsi[id].iq_node_path, IQ_SHDR_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_dpc_node_path, IQ_SHDR_DPC_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_shading_node_path, IQ_SHDR_SHADING_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].iq_ldc_node_path, IQ_SHDR_LDC_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].ae_node_path, AE_SHDR_NODE_PATH_1);
				sprintf(isp_builtin_dtsi[id].awb_node_path, AWB_SHDR_NODE_PATH_1);
			}
		} else if (id == 1) {
			if (night_mode[isp_sensor_path_2] == 1) {
				sprintf(isp_builtin_dtsi[id].iq_node_path, IQ_IR_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_dpc_node_path, IQ_IR_DPC_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_shading_node_path, IQ_IR_SHADING_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_ldc_node_path, IQ_IR_LDC_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].ae_node_path, AE_IR_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].awb_node_path, AWB_IR_NODE_PATH_2);
			} else if (shdr_enable[isp_sensor_path_2] == 1) {
				sprintf(isp_builtin_dtsi[id].iq_node_path, IQ_SHDR_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_dpc_node_path, IQ_SHDR_DPC_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_shading_node_path, IQ_SHDR_SHADING_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].iq_ldc_node_path, IQ_SHDR_LDC_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].ae_node_path, AE_SHDR_NODE_PATH_2);
				sprintf(isp_builtin_dtsi[id].awb_node_path, AWB_SHDR_NODE_PATH_2);
			}
		}
	}

	// init 2A/IQ
	nvt_bootts_add_ts("2A"); //begin
	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++) {
		if (sensor_valid[id]) {
			// update c gain if not zero value.
			if ((cgain_bridge.r != 0) && (cgain_bridge.g != 0) && (cgain_bridge.b != 0)) {
				isp_builtin_set_cgain(id, &cgain_bridge);
			}

			#if (_2A_BUILTIN_NVT)
			ae_builtin_init_nvt(id);
			awb_builtin_init_nvt(id);
			#else
			ae_builtin_init(id);
			awb_builtin_init(id);
			#endif
			iq_builtin_init(id);
		}
	}

	// regist call-back fucntion
	sie_fb_reg_isr_Cb(&isp_builtin_cb_sie);
	kdrv_ipp_builtin_reg_isp_cb(&isp_builtin_cb_ipp);
	vdoenc_builtin_evt_fp_reg(NULL, &isp_builtin_cb_enc);

	return E_OK;
}

ER isp_builtin_uninit(void)
{
	UINT32 id;

	if (isp_builtin_task_open) {
		// un-init task
		rel_flg(isp_builtin_flag_id);

		isp_builtin_task_open = FALSE;
	}

	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++) {
		if (sensor_valid[id]) {
			sensor_builtin_init_i2c(id);
		}
	}

	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++) {
		if (sensor_valid[id]) {
			sensor_builtin_uninit_i2c(id);
			
			if (isp_builtin_ca_rslt[id]) {
				kfree(isp_builtin_ca_rslt[id]);
				isp_builtin_ca_rslt[id] = NULL;
			}
			if (isp_builtin_la_rslt[id]) {
				kfree(isp_builtin_la_rslt[id]);
				isp_builtin_la_rslt[id] = NULL;
			}
			if (isp_builtin_histo_rslt[id]) {
				kfree(isp_builtin_histo_rslt[id]);
				isp_builtin_histo_rslt[id] = NULL;
			}
		}
		#if (_2A_BUILTIN_NVT)
		awb_builtin_uninit(id);
		ae_builtin_uninit(id);
		#endif

	}

	return E_OK;
}
#endif

EXPORT_SYMBOL(isp_builtin_get_i2c_id);
EXPORT_SYMBOL(isp_builtin_get_i2c_addr);
EXPORT_SYMBOL(isp_builtin_get_shdr_enable);
EXPORT_SYMBOL(isp_builtin_get_shdr_id_mask);
EXPORT_SYMBOL(isp_builtin_get_nr_lv);
EXPORT_SYMBOL(isp_builtin_get_3dnr_lv);
EXPORT_SYMBOL(isp_builtin_get_sharpness_lv);
EXPORT_SYMBOL(isp_builtin_get_saturation_lv);
EXPORT_SYMBOL(isp_builtin_get_contrast_lv);
EXPORT_SYMBOL(isp_builtin_get_brightness_lv);
EXPORT_SYMBOL(isp_builtin_get_night_mode);
EXPORT_SYMBOL(isp_builtin_get_chgmode_fps);
EXPORT_SYMBOL(isp_builtin_get_sensor_expt_max);
EXPORT_SYMBOL(isp_builtin_get_total_gain);
EXPORT_SYMBOL(isp_builtin_get_ct);
EXPORT_SYMBOL(isp_builtin_get_lv);
EXPORT_SYMBOL(isp_builtin_get_cgain);
EXPORT_SYMBOL(isp_builtin_get_dgain);
EXPORT_SYMBOL(isp_builtin_get_sensor_gain);
EXPORT_SYMBOL(isp_builtin_get_sensor_expt);
EXPORT_SYMBOL(isp_builtin_get_sensor_valid);
EXPORT_SYMBOL(isp_builtin_get_isp_version);
EXPORT_SYMBOL(isp_builtin_get_ae_version);
EXPORT_SYMBOL(isp_builtin_get_awb_version);
EXPORT_SYMBOL(isp_builtin_get_sensor_version);
EXPORT_SYMBOL(isp_builtin_get_iq_version);
EXPORT_SYMBOL(isp_builtin_get_ae_param);
EXPORT_SYMBOL(isp_builtin_get_awb_param);
EXPORT_SYMBOL(isp_builtin_get_iq_param);
EXPORT_SYMBOL(isp_builtin_uninit_i2c);
EXPORT_SYMBOL(isp_builtin_set_transfer_i2c);
EXPORT_SYMBOL(isp_builtin_get_overexposure_offset);

#endif
