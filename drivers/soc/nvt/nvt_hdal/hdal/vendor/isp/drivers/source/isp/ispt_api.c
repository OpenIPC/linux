#if defined(__FREERTOS)
#include <string.h>
#else
#include <mach/nvt-io.h>
#endif
#include "kwrap/type.h"

#include "isp_api.h"
#include "isp_dbg.h"
#include "kflow_videocapture/ctl_sen.h"
#include "kflow_videocapture/ctl_sie_isp.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "kflow_videoenc/isf_vdoenc_int.h"

#if defined(__FREERTOS)
#include "isp_dev.h"
#else
#include "isp_main.h"
#endif
#include "isp_mtr_int.h"
#include "isp_version.h"
#include "ispt_api_int.h"

typedef void (*ispt_fp)(UINT32 addr);

//=============================================================================
// function declaration
//=============================================================================
static void ispt_api_get_version(UINT32 addr);
static void ispt_api_get_size_tab(UINT32 addr);
static void ispt_api_get_func(UINT32 addr);
static void ispt_api_get_yuv(UINT32 addr);
static void ispt_api_get_raw(UINT32 addr);
static void ispt_api_get_frame(UINT32 addr);
static void ispt_api_get_memory(UINT32 addr);
static void ispt_api_get_sensor_info(UINT32 addr);
static void ispt_api_get_sensor_reg(UINT32 addr);
static void ispt_api_get_sensor_mode_info(UINT32 addr);
static void ispt_api_get_3dnr_sta(UINT32 addr);
static void ispt_api_get_md_sta(UINT32 addr);
static void ispt_api_get_emc_new_buf(UINT32 addr);
static void ispt_api_get_emu_run(UINT32 addr);
static void ispt_api_get_chip_info(UINT32 addr);
static void ispt_api_get_ca_data(UINT32 addr);
static void ispt_api_get_la_data(UINT32 addr);
static void ispt_api_get_va_data(UINT32 addr);
static void ispt_api_get_va_indep_data(UINT32 addr);
static void ispt_api_get_wait_vd(UINT32 addr);
static void ispt_api_get_sensor_expt(UINT32 addr);
static void ispt_api_get_sensor_gain(UINT32 addr);
static void ispt_api_get_d_gain(UINT32 addr);
static void ispt_api_get_c_gain(UINT32 addr);
static void ispt_api_get_total_gain(UINT32 addr);
static void ispt_api_get_lv(UINT32 addr);
static void ispt_api_get_ct(UINT32 addr);
static void ispt_api_get_motor_iris(UINT32 addr);
static void ispt_api_get_motor_focus(UINT32 addr);
static void ispt_api_get_motor_zoom(UINT32 addr);
static void ispt_api_get_motor_misc(UINT32 addr);
static void ispt_api_get_sensor_direction(UINT32 addr);
static void ispt_api_get_histo_data(UINT32 addr);
static void ispt_api_get_ir_info(UINT32 addr);
static void ispt_api_get_md_data(UINT32 addr);
static void ispt_api_set_yuv(UINT32 addr);
static void ispt_api_set_raw(UINT32 addr);
static void ispt_api_set_frame(UINT32 addr);
static void ispt_api_set_sensor_reg(UINT32 addr);
static void ispt_api_set_sensor_expt(UINT32 addr);
static void ispt_api_set_sensor_gain(UINT32 addr);
static void ispt_api_set_d_gain(UINT32 addr);
static void ispt_api_set_c_gain(UINT32 addr);
static void ispt_api_set_total_gain(UINT32 addr);
static void ispt_api_set_lv(UINT32 addr);
static void ispt_api_set_ct(UINT32 addr);
static void ispt_api_set_motor_iris(UINT32 addr);
static void ispt_api_set_motor_focus(UINT32 addr);
static void ispt_api_set_motor_zoom(UINT32 addr);
static void ispt_api_set_motor_misc(UINT32 addr);
static void ispt_api_set_sensor_direction(UINT32 addr);
static void ispt_api_reserve(UINT32 addr);
static void ispt_api_set_sensor_sleep(UINT32 addr);
static void ispt_api_set_sensor_wakeup(UINT32 addr);

#define RESERVE_SIZE 0

static ISPT_INFO ispt_info = { {
	//id                             size
	{ISPT_ITEM_VERSION,              sizeof(UINT32)                   },
	{ISPT_ITEM_SIZE_TAB,             sizeof(ISPT_INFO)                },
	{ISPT_ITEM_FUNC,                 sizeof(ISPT_FUNC)                },
	{ISPT_ITEM_YUV,                  sizeof(ISPT_YUV_INFO)            },
	{ISPT_ITEM_RAW,                  sizeof(ISPT_RAW_INFO)            },
	{ISPT_ITEM_FRAME,                sizeof(ISPT_MEMORY_INFO)         },
	{ISPT_ITEM_MEMORY,               sizeof(ISPT_MEMORY_INFO)         },
	{ISPT_ITEM_SENSOR_INFO,          sizeof(ISPT_SENSOR_INFO)         },
	{ISPT_ITEM_SENSOR_REG,           sizeof(ISPT_SENSOR_REG)          },
	{ISPT_ITEM_SENSOR_MODE_INFO,     sizeof(ISPT_SENSOR_MODE_INFO),   },
	{ISPT_ITEM_3DNR_STA,             sizeof(ISPT_3DNR_STA_INFO)       },
	{ISPT_ITEM_EMU_NEW_BUF,          sizeof(ISPT_EMU_NEW_BUF)         },
	{ISPT_ITEM_EMU_RUN,              sizeof(ISPT_EMU_RUN)             },
	{ISPT_ITEM_CHIP_INFO,            sizeof(ISPT_CHIP_INFO)           },
	{ISPT_ITEM_RESERVE_14,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_15,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_16,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_17,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_18,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_19,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_20,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_21,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_22,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_23,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_24,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_25,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_26,           RESERVE_SIZE                     },
	{ISPT_ITEM_RESERVE_27,           RESERVE_SIZE                     },
	{ISPT_ITEM_CA_DATA,              sizeof(ISPT_CA_DATA)             },
	{ISPT_ITEM_LA_DATA,              sizeof(ISPT_LA_DATA)             },
	{ISPT_ITEM_VA_DATA,              sizeof(ISPT_VA_DATA)             },
	{ISPT_ITEM_VA_INDEP_DATA,        sizeof(ISPT_VA_INDEP_DATA)       },
	{ISPT_ITEM_WAIT_VD,              sizeof(ISPT_WAIT_VD)             },
	{ISPT_ITEM_SENSOR_EXPT,          sizeof(ISPT_SENSOR_EXPT)         },
	{ISPT_ITEM_SENSOR_GAIN,          sizeof(ISPT_SENSOR_GAIN)         },
	{ISPT_ITEM_D_GAIN,               sizeof(ISPT_D_GAIN)              },
	{ISPT_ITEM_C_GAIN,               sizeof(ISPT_C_GAIN)              },
	{ISPT_ITEM_TOTAL_GAIN,           sizeof(ISPT_TOTAL_GAIN)          },
	{ISPT_ITEM_LV,                   sizeof(ISPT_LV),                 },
	{ISPT_ITEM_CT,                   sizeof(ISPT_CT),                 },
	{ISPT_ITEM_MOTOR_IRIS,           sizeof(ISPT_MOTOR_IRIS)          },
	{ISPT_ITEM_MOTOR_FOCUS,          sizeof(ISPT_MOTOR_FOCUS)         },
	{ISPT_ITEM_MOTOR_ZOOM,           sizeof(ISPT_MOTOR_ZOOM)          },
	{ISPT_ITEM_MOTOR_MISC,           sizeof(ISPT_MOTOR_MISC)          },
	{ISPT_ITEM_SENSOR_DIRECTION,     sizeof(ISPT_SENSOR_DIRECTION)    },
	{ISPT_ITEM_SENSOR_SLEEP,         sizeof(UINT32),                  },
	{ISPT_ITEM_SENSOR_WAKEUP,        sizeof(UINT32),                  },
	{ISPT_ITEM_HISTO_DATA,           sizeof(ISPT_HISTO_DATA)          },
	{ISPT_ITEM_IR_INFO,              sizeof(ISPT_IR_INFO)             },
	{ISPT_ITEM_MD_DATA,              sizeof(ISPT_MD_DATA)             },
	{ISPT_ITEM_MD_STA,               sizeof(ISPT_MD_STA_INFO)         }
} };

static ispt_fp ispt_get_tab[ISPT_ITEM_MAX] = {
	ispt_api_get_version,
	ispt_api_get_size_tab,
	ispt_api_get_func,
	ispt_api_get_yuv,
	ispt_api_get_raw,
	ispt_api_get_frame,             // 5
	ispt_api_get_memory,
	ispt_api_get_sensor_info,
	ispt_api_get_sensor_reg,
	ispt_api_get_sensor_mode_info,
	ispt_api_get_3dnr_sta,          // 10
	ispt_api_get_emc_new_buf,
	ispt_api_get_emu_run,
	ispt_api_get_chip_info,
	ispt_api_reserve,
	ispt_api_reserve,               // 15
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 20
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 25
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_get_ca_data,
	ispt_api_get_la_data,
	ispt_api_get_va_data,           // 30
	ispt_api_get_va_indep_data,
	ispt_api_get_wait_vd,
	ispt_api_get_sensor_expt,
	ispt_api_get_sensor_gain,
	ispt_api_get_d_gain,            // 35
	ispt_api_get_c_gain,
	ispt_api_get_total_gain,
	ispt_api_get_lv,
	ispt_api_get_ct,
	ispt_api_get_motor_iris,        // 40
	ispt_api_get_motor_focus,
	ispt_api_get_motor_zoom,
	ispt_api_get_motor_misc,
	ispt_api_get_sensor_direction,
	ispt_api_reserve,               // 45
	ispt_api_reserve,
	ispt_api_get_histo_data,
	ispt_api_get_ir_info,
	ispt_api_get_md_data,
	ispt_api_get_md_sta,            // 50
};

static ispt_fp ispt_set_tab[ISPT_ITEM_MAX] = {
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_set_yuv,
	ispt_api_set_raw,
	ispt_api_set_frame,             // 5
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_set_sensor_reg,
	ispt_api_reserve,
	ispt_api_reserve,               // 10
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 15
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 20
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 25
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 30
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_set_sensor_expt,
	ispt_api_set_sensor_gain,
	ispt_api_set_d_gain,            // 35
	ispt_api_set_c_gain,
	ispt_api_set_total_gain,
	ispt_api_set_lv,
	ispt_api_set_ct,
	ispt_api_set_motor_iris,        // 40
	ispt_api_set_motor_focus,
	ispt_api_set_motor_zoom,
	ispt_api_set_motor_misc,
	ispt_api_set_sensor_direction,
	ispt_api_set_sensor_sleep,      // 45
	ispt_api_set_sensor_wakeup,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,
	ispt_api_reserve,               // 50
};

static void ispt_api_get_version(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	*data = isp_get_version();
}

static void ispt_api_get_size_tab(UINT32 addr)
{
	memcpy((ISPT_INFO *)addr, &ispt_info, sizeof(ISPT_INFO));
}

static void ispt_api_get_func(UINT32 addr)
{
	ISPT_FUNC *data = (ISPT_FUNC *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_func(data->id, &data->func_info);
}

static void ispt_api_get_yuv(UINT32 addr)
{
	ISPT_YUV_INFO *data = (ISPT_YUV_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_yuv(data->id, &data->yuv_info);
}

static void ispt_api_get_raw(UINT32 addr)
{
	ISPT_RAW_INFO *data = (ISPT_RAW_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_raw(data->id, &data->raw_info);
}

static void ispt_api_get_frame(UINT32 addr)
{
	ISPT_MEMORY_INFO *data = (ISPT_MEMORY_INFO *)addr;

	memcpy((UINT32 *)&data->buf, (UINT32 *)data->addr, data->size);
}

static void ispt_api_get_memory(UINT32 addr)
{
	ISPT_MEMORY_INFO *data = (ISPT_MEMORY_INFO *)addr;
	UINT32 base_addr;
	UINT32 i, read_value;

	#if defined(__FREERTOS)
	base_addr = data->addr;

	for (i = 0; i < data->size; i+=4) {
		read_value = *(UINT32 volatile *)(base_addr+i);
		memcpy((UINT32 *)&data->buf[i], &read_value, sizeof(UINT32));
	}

	#else
	base_addr = (UINT32)ioremap_nocache(data->addr, data->size);

	for (i = 0; i < data->size; i+=4) {
		read_value = ioread32((void *)(base_addr+i));
		memcpy((UINT32 *)&data->buf[i], &read_value, sizeof(UINT32));
	}
	#endif
}

static void ispt_api_get_sensor_info(UINT32 addr)
{
	ISPT_SENSOR_INFO *data = (ISPT_SENSOR_INFO *)addr;

	isp_api_get_sensor_info(&data->sensor_info);
}

static void ispt_api_get_sensor_reg(UINT32 addr)
{
	ISPT_SENSOR_REG *data = (ISPT_SENSOR_REG *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_sensor_reg(data->id, &data->addr, &data->data);
}

static void ispt_api_get_sensor_mode_info(UINT32 addr)
{
	ISPT_SENSOR_MODE_INFO *data = (ISPT_SENSOR_MODE_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_sensor_mode_info(data->id, &data->info);
}

static void ispt_api_get_3dnr_sta(UINT32 addr)
{
	ISPT_3DNR_STA_INFO *data = (ISPT_3DNR_STA_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_3dnr_sta(data->id, &data->_3dnr_sta_info);
}

static void ispt_api_get_md_sta(UINT32 addr)
{
	ISPT_MD_STA_INFO *data = (ISPT_MD_STA_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_md_sta(data->id, &data->md_sta_info);
}

static void ispt_api_get_emc_new_buf(UINT32 addr)
{
	ISPT_EMU_NEW_BUF *data = (ISPT_EMU_NEW_BUF *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	data->rt = isp_api_set_emu_new_buf(data->id, &data->buf_new);
}

static void ispt_api_get_emu_run(UINT32 addr)
{
	ISPT_EMU_RUN *data = (ISPT_EMU_RUN *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	data->rt = isp_api_set_emu_run(data->id, &data->buf_push);
}

static void ispt_api_get_chip_info(UINT32 addr)
{
	ISPT_CHIP_INFO *data = (ISPT_CHIP_INFO *)addr;

	isp_get_chip_info(&data->info);

}

static void ispt_api_get_ca_data(UINT32 addr)
{
	ISPT_CA_DATA *data = (ISPT_CA_DATA *)addr;
	ISP_CA_RSLT *ca_rslt = isp_dev_get_ca(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	if (ca_rslt != NULL) {
		memcpy(&data->ca_rslt, ca_rslt, sizeof(ISP_CA_RSLT));
	}
}

static void ispt_api_get_la_data(UINT32 addr)
{
	ISPT_LA_DATA *data = (ISPT_LA_DATA *)addr;
	ISP_LA_RSLT *la_rslt = isp_dev_get_la(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	if (la_rslt != NULL) {
		memcpy(&data->la_rslt, la_rslt, sizeof(ISP_LA_RSLT));
	}
}

static void ispt_api_get_va_data(UINT32 addr)
{
	ISPT_VA_DATA *data = (ISPT_VA_DATA *)addr;
	ISP_VA_RSLT *va_rslt = isp_dev_get_va(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	if (va_rslt != NULL) {
		memcpy(&data->va_rslt, va_rslt, sizeof(ISP_VA_RSLT));
	}
}

static void ispt_api_get_va_indep_data(UINT32 addr)
{
	ISPT_VA_INDEP_DATA *data = (ISPT_VA_INDEP_DATA *)addr;
	ISP_VA_INDEP_RSLT *va_rslt = isp_dev_get_va_indep(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	if (va_rslt != NULL) {
		memcpy(&data->va_indep_rslt, va_rslt, sizeof(ISP_VA_INDEP_RSLT));
	}
}

static void ispt_api_get_wait_vd(UINT32 addr)
{
	ISPT_WAIT_VD *data = (ISPT_WAIT_VD *)addr;

	isp_dev_wait_vd(data->id, data->timeout);
}

static void ispt_api_get_sensor_expt(UINT32 addr)
{
	ISPT_SENSOR_EXPT *data = (ISPT_SENSOR_EXPT *)addr;
	ISP_SENSOR_CTRL sensor_ctrl = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_expt(data->id, &sensor_ctrl);
	memcpy(&data->time, &sensor_ctrl.exp_time, sizeof(UINT32)*ISP_SEN_MFRAME_MAX_NUM);
}

static void ispt_api_get_sensor_gain(UINT32 addr)
{
	ISPT_SENSOR_GAIN *data = (ISPT_SENSOR_GAIN *)addr;
	ISP_SENSOR_CTRL sensor_ctrl = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_gain(data->id, &sensor_ctrl);
	memcpy(&data->ratio, &sensor_ctrl.gain_ratio, sizeof(UINT32)*ISP_SEN_MFRAME_MAX_NUM);
}

static void ispt_api_get_d_gain(UINT32 addr)
{
	ISPT_D_GAIN *data = (ISPT_D_GAIN *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_IPP, ISP_SYNC_AE_DGAIN, &data->gain);
}

static void ispt_api_get_c_gain(UINT32 addr)
{
	ISPT_C_GAIN *data = (ISPT_C_GAIN *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_IPP, ISP_SYNC_AWB_CGAIN, &data->gain);
}

static void ispt_api_get_total_gain(UINT32 addr)
{
	ISPT_TOTAL_GAIN *data = (ISPT_TOTAL_GAIN *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_IPP, ISP_SYNC_AE_TOTAL_GAIN, &data->gain);
}

static void ispt_api_get_lv(UINT32 addr)
{
	ISPT_LV *data = (ISPT_LV *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_IPP, ISP_SYNC_AE_LV, &data->lv);
}

static void ispt_api_get_ct(UINT32 addr)
{
	ISPT_CT *data = (ISPT_CT *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_IPP, ISP_SYNC_AWB_CT, &data->ct);
}

static void ispt_api_get_motor_iris(UINT32 addr)
{
	ISPT_MOTOR_IRIS *data = (ISPT_MOTOR_IRIS *)addr;

	isp_api_get_iris(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_get_motor_focus(UINT32 addr)
{
	ISPT_MOTOR_FOCUS *data = (ISPT_MOTOR_FOCUS *)addr;

	isp_api_get_focus(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_get_motor_zoom(UINT32 addr)
{
	ISPT_MOTOR_ZOOM *data = (ISPT_MOTOR_ZOOM *)addr;

	isp_api_get_zoom(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_get_motor_misc(UINT32 addr)
{
	ISPT_MOTOR_MISC *data = (ISPT_MOTOR_MISC *)addr;

	isp_api_get_misc(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_get_sensor_direction(UINT32 addr)
{
	ISPT_SENSOR_DIRECTION *data = (ISPT_SENSOR_DIRECTION *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}

	isp_api_get_direction(data->id, &data->direction);
}

static void ispt_api_get_histo_data(UINT32 addr)
{
	ISPT_HISTO_DATA *data = (ISPT_HISTO_DATA *)addr;
	ISP_HISTO_RSLT *histo_rslt = isp_dev_get_histo(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	if (histo_rslt != NULL) {
		memcpy(&data->histo_rslt, histo_rslt, sizeof(ISP_HISTO_RSLT));
	}
}

static void ispt_api_get_ir_info(UINT32 addr)
{
	ISPT_IR_INFO *data = (ISPT_IR_INFO *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_get_ir_info(data->id, &data->info);
}

static void ispt_api_get_md_data(UINT32 addr)
{
	ISPT_MD_DATA *data = (ISPT_MD_DATA *)addr;
	ISP_MD_RSLT *md_rslt = isp_dev_get_md(data->id);

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}

	if (md_rslt != NULL) {
		memcpy(&data->md_rslt, md_rslt, sizeof(ISP_MD_RSLT));
	}
}

UINT32 ispt_api_get_item_size(ISPT_ITEM item)
{
	return ispt_info.size_tab[item][1];
}

ER ispt_api_get_cmd(ISPT_ITEM item, UINT32 addr)
{
	if (ispt_get_tab[item] == NULL) {
		DBG_ERR("ispt_get_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if (item >= ISPT_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	ispt_get_tab[item](addr);
	return E_OK;
}

static void ispt_api_set_yuv(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	isp_api_set_yuv(*data);
}

static void ispt_api_set_raw(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	isp_api_set_raw(*data);
}

static void ispt_api_set_frame(UINT32 addr)
{
	ISPT_MEMORY_INFO *data = (ISPT_MEMORY_INFO *)addr;

	memcpy((UINT32 *)data->addr, (UINT32 *)&data->buf, data->size);
}

static void ispt_api_set_sensor_reg(UINT32 addr)
{
	ISPT_SENSOR_REG *data = (ISPT_SENSOR_REG *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_api_set_sensor_reg(data->id, &data->addr, &data->data);
}

static void ispt_api_set_sensor_expt(UINT32 addr)
{
	ISPT_SENSOR_EXPT *data = (ISPT_SENSOR_EXPT *)addr;
	ISP_SENSOR_CTRL sensor_ctrl = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	memcpy(&sensor_ctrl.exp_time, &data->time, sizeof(UINT32)*ISP_SEN_MFRAME_MAX_NUM);
	isp_api_set_expt(data->id, &sensor_ctrl);
}

static void ispt_api_set_sensor_gain(UINT32 addr)
{
	ISPT_SENSOR_GAIN *data = (ISPT_SENSOR_GAIN *)addr;
	ISP_SENSOR_CTRL sensor_ctrl = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	memcpy(&sensor_ctrl.gain_ratio, &data->ratio, sizeof(UINT32)*ISP_SEN_MFRAME_MAX_NUM);
	isp_api_set_gain(data->id, &sensor_ctrl);
}

static void ispt_api_set_d_gain(UINT32 addr)
{
	ISPT_D_GAIN *data = (ISPT_D_GAIN *)addr;
	ISP_AE_INIT_INFO ae_init_info = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_SIE, ISP_SYNC_AE_DGAIN, &data->gain);

	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
	ae_init_info.d_gain= data->gain;
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
}

static void ispt_api_set_c_gain(UINT32 addr)
{
	ISPT_C_GAIN *data = (ISPT_C_GAIN *)addr;
	ISP_AWB_INIT_INFO awb_init_info = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_SIE, ISP_SYNC_AWB_CGAIN, &data->gain);

	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
	awb_init_info.r_gain = data->gain[0];
	awb_init_info.g_gain = data->gain[1];
	awb_init_info.b_gain = data->gain[2];
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
}

static void ispt_api_set_total_gain(UINT32 addr)
{
	ISPT_TOTAL_GAIN *data = (ISPT_TOTAL_GAIN *)addr;
	ISP_AE_INIT_INFO ae_init_info = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_SIE, ISP_SYNC_AE_TOTAL_GAIN, &data->gain);

	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
	ae_init_info.total_gain= data->gain;
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
}

static void ispt_api_set_lv(UINT32 addr)
{
	ISPT_LV *data = (ISPT_LV *)addr;
	ISP_AE_INIT_INFO ae_init_info = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_SIE, ISP_SYNC_AE_LV, &data->lv);

	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
	ae_init_info.lv= data->lv;
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AE_INIT, &ae_init_info);
}

static void ispt_api_set_ct(UINT32 addr)
{
	ISPT_CT *data = (ISPT_CT *)addr;
	ISP_AWB_INIT_INFO awb_init_info = {0};

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_SIE, ISP_SYNC_AWB_CT, &data->ct);

	isp_dev_get_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
	awb_init_info.ct = data->ct;
	isp_dev_set_sync_item(data->id, ISP_SYNC_SEL_CURRENT, ISP_SYNC_AWB_INIT, &awb_init_info);
}

static void ispt_api_set_motor_iris(UINT32 addr)
{
	ISPT_MOTOR_IRIS *data = (ISPT_MOTOR_IRIS *)addr;

	isp_api_set_iris(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_set_motor_focus(UINT32 addr)
{
	ISPT_MOTOR_FOCUS *data = (ISPT_MOTOR_FOCUS *)addr;

	isp_api_set_focus(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_set_motor_zoom(UINT32 addr)
{
	ISPT_MOTOR_ZOOM *data = (ISPT_MOTOR_ZOOM *)addr;

	isp_api_set_zoom(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_set_motor_misc(UINT32 addr)
{
	ISPT_MOTOR_MISC *data = (ISPT_MOTOR_MISC *)addr;

	isp_api_set_misc(data->cmd_type, &data->ctl_cmd);
}

static void ispt_api_set_sensor_direction(UINT32 addr)
{
	ISPT_SENSOR_DIRECTION *data = (ISPT_SENSOR_DIRECTION *)addr;

	if (data->id >= ISP_ID_MAX_NUM) {
		PRINT_ISP_WRN(isp_dbg_get_dbg_mode(data->id) & ISP_DBG_WRN_MSG, "isp id out of range (%d) \r\n", data->id);
		return;
	}

	isp_api_set_direction(data->id, &data->direction);
}

static void ispt_api_set_sensor_sleep(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	isp_api_set_sensor_sleep(*data);
}

static void ispt_api_set_sensor_wakeup(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	isp_api_set_sensor_wakeup(*data);
}

static void ispt_api_reserve(UINT32 addr)
{
	return;
}

ER ispt_api_set_cmd(ISPT_ITEM item, UINT32 addr)
{
	if (ispt_set_tab[item] == NULL) {
		DBG_ERR("ispt_set_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if (item >= ISPT_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	ispt_set_tab[item](addr);
	return E_OK;
}

