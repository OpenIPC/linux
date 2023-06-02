/*
    KDRV of Serial Sensor Interface

    Exported KDRV of Serial Sensor Interface

    @file       kdrv_ssenif.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "ssenif_int.h"
#include "kdrv_videocapture/kdrv_ssenif.h"

#ifndef __KERNEL__
#define __MODULE__    rtos_ssenif
#include <kwrap/debug.h>
extern unsigned int rtos_ssenif_debug_level;

#include "pll.h"
#include "pll_protected.h"
#else
#include <linux/clk.h>
#include <linux/clk-provider.h>
#endif

INT32 kdrv_ssenif_open(UINT32 chip, UINT32 engine)
{
	PDAL_SSENIFOBJ object;

	if ((engine == KDRV_SSENIF_ENGINE_CSI0)||(engine == KDRV_SSENIF_ENGINE_CSI1)) {
	} else if ((engine == KDRV_SSENIF_ENGINE_LVDS0)||(engine == KDRV_SSENIF_ENGINE_LVDS1)) {
#if SSENIF_CSI_EN
		engine = KDRV_SSENIF_ENGINE_CSI2 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#else
		engine = KDRV_SSENIF_ENGINE_CSI0 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#endif
	} else {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	object = dal_ssenif_get_object((DAL_SSENIF_ID)(engine & 0xFF));

	object->init();

	if ((object->serial_senif_id != DAL_SSENIF_ID_VX1) && (object->serial_senif_id != DAL_SSENIF_ID_VX1_2)) {
		if (!object->is_opened()) {
			object->open();
		}
	}

	return 0;
}

INT32 kdrv_ssenif_close(UINT32 chip, UINT32 engine)
{
	PDAL_SSENIFOBJ object;

	if ((engine == KDRV_SSENIF_ENGINE_CSI0)||(engine == KDRV_SSENIF_ENGINE_CSI1)) {
	} else if ((engine == KDRV_SSENIF_ENGINE_LVDS0)||(engine == KDRV_SSENIF_ENGINE_LVDS1)) {
#if SSENIF_CSI_EN
		engine = KDRV_SSENIF_ENGINE_CSI2 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#else
		engine = KDRV_SSENIF_ENGINE_CSI0 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#endif
	} else {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	object = dal_ssenif_get_object((DAL_SSENIF_ID)(engine & 0xFF));

	if (object->is_opened()) {
		object->close();
	}

	return 0;
}

INT32 kdrv_ssenif_trigger(UINT32 id, BOOL enable)
{
	PDAL_SSENIFOBJ  object;
	UINT32          engine;

	engine = KDRV_DEV_ID_ENGINE(id);
	if ((engine == KDRV_SSENIF_ENGINE_CSI0)||(engine == KDRV_SSENIF_ENGINE_CSI1)) {
	} else if ((engine == KDRV_SSENIF_ENGINE_LVDS0)||(engine == KDRV_SSENIF_ENGINE_LVDS1)) {
#if SSENIF_CSI_EN
		engine = KDRV_SSENIF_ENGINE_CSI2 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#else
		engine = KDRV_SSENIF_ENGINE_CSI0 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#endif
	} else {
		DBG_ERR("Err engine\r\n");
		return -1;
	}

	object = dal_ssenif_get_object((DAL_SSENIF_ID)(engine & 0xFF));

	if (object) {

		if (!object->is_opened()) {
			DBG_ERR("no open 0x%08X\r\n", (UINT)id);
			return -1;
		}

		if (enable) {
			object->start();
		} else {
			object->stop();
		}
	} else {
		DBG_ERR("Err id\r\n");
		return -1;
	}

	return 0;
}

INT32 __kdrv_ssenif_set(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param)
{
	PDAL_SSENIFOBJ  object;
	UINT32          group;
	UINT32          engine;
	#ifdef __KERNEL__
	static UINT32   siecommonclk;
	#endif

	group = param_id & 0xFF000000;

	if (group != KDRV_SSENIF_GLOBAL_BASE) {

		engine = KDRV_DEV_ID_ENGINE(id);
		if ((engine == KDRV_SSENIF_ENGINE_CSI0)||(engine == KDRV_SSENIF_ENGINE_CSI1)) {
		} else if ((engine == KDRV_SSENIF_ENGINE_LVDS0)||(engine == KDRV_SSENIF_ENGINE_LVDS1)) {
#if SSENIF_CSI_EN
			engine = KDRV_SSENIF_ENGINE_CSI2 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#else
			engine = KDRV_SSENIF_ENGINE_CSI0 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#endif
		} else {
			DBG_ERR("Err engine\r\n");
			return -1;
		}

		object = dal_ssenif_get_object((DAL_SSENIF_ID)(engine & 0xFF));

		if (((object->serial_senif_id == DAL_SSENIF_ID_VX1) || (object->serial_senif_id == DAL_SSENIF_ID_VX1_2)) && (param_id == KDRV_SSENIF_VX1_TXTYPE) && (!object->is_opened())) {
			object->set_vx1_config(DAL_SSENIFVX1_CFGID_TXTYPE, (UINT32)p_param);
			object->open();
		} else {

			if (!object->is_opened()) {
				DBG_ERR("no open 0x%08X\r\n", (UINT)id);
				return -1;
			}
		}
	}

	if (group == KDRV_SSENIF_CSI_BASE) {
		/*
		    CSI GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_CSI_WAIT_INTERRUPT: {
				object->wait_interrupt((DAL_SSENIF_INTERRUPT)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_SENSORTYPE: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_SENSORTYPE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_SENSOR_TARGET_MCLK: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_SENSOR_REAL_MCLK: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_DLANE_NUMBER: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_DLANE_NUMBER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE2: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE4: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE5: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE5, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_CLANE_SWITCH: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_CLANE_SWITCH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_TIMEOUT_PERIOD: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_STOP_METHOD: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_STOP_METHOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE2: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE4: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE5: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE0_PIN: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE0_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE1_PIN: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE1_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE2_PIN: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE2_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE3_PIN: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE3_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL2: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL3: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL3, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL_FORMAT: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL_FORMAT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL2_FORMAT: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL2_FORMAT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL3_FORMAT: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL3_FORMAT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL_DATA_ID: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL_DATA_ID, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL2_DATA_ID: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL2_DATA_ID, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL3_DATA_ID: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_MANUAL3_DATA_ID, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_HSCLK, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_CSI_IADJ: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_IADJ, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_CSI_CLANE_CMETHOD: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_CLANE_FORCE_HS, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_CSI_HSDATAOUT_DLY: {
				object->set_csi_config(DAL_SSENIFCSI_CFGID_HSDATAO_DLY, (UINT32)p_param);
			}
			break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}

	} else if (group == KDRV_SSENIF_LVDS_BASE) {
		/*
		    LVDS GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_LVDS_WAIT_INTERRUPT: {
				object->wait_interrupt((DAL_SSENIF_INTERRUPT)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_SENSORTYPE: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_SENSORTYPE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_NUMBER: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_DLANE_NUMBER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_VALID_WIDTH: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_WIDTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_VALID_HEIGHT: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_HEIGHT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_PIXEL_DEPTH: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_CLANE_SWITCH: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_CLANE_SWITCH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_TIMEOUT_PERIOD: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_STOP_METHOD: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_STOP_METHOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE2: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE4: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE5: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE5, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_0: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_0, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_1: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_1, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_2: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_3: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_3, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_4: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_5: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_5, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_6: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_6, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_7: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_7, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;
		case KDRV_SSENIF_LVDS_PIXEL_INORDER: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_INORDER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_FSET_BIT: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_FSET_BIT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT0: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT0, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT1: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT1, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT2: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT3: {
				object->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT3, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_CW_HD: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_CTRLWORD_HD, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_CW_VD: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_CW_VD2: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD2, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_CW_VD3: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD3, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_CW_VD4: {
				object->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_CTRLWORD_VD4, (DAL_SSENIF_LANESEL)(((UINT32)p_param >> 20) & 0xFF), ((UINT32)p_param) & 0xFFFF);
			}
			break;


		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}
	} else if (group == KDRV_SSENIF_VX1_BASE) {

		/*
		    VX1 GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_VX1_WAIT_INTERRUPT: {
				object->wait_interrupt((DAL_SSENIF_INTERRUPT)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_TXTYPE: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_TXTYPE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENSORTYPE: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSORTYPE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_DATAMUX_SEL: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_DATAMUX_SEL, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_TIMEOUT_PERIOD: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_TIMEOUT_PERIOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_LOCKCHG_CALLBACK: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_LOCKCHG_CALLBACK, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_I2C_SPEED: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_I2C_SPEED, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_I2CNACK_CHECK: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_I2CNACK_CHECK, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_VX1_SENSOR_SLAVEADDR: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_SLAVEADDR, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENREG_ADDR_BC: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_ADDR_BC, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENREG_DATA_BC: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_DATA_BC, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_I2C_WRITE: {
				PKDRV_SSENIFVX1_I2CCMD i2c_cmd;

				i2c_cmd = (PKDRV_SSENIFVX1_I2CCMD)p_param;
				object->sensor_i2c_sequential_write(i2c_cmd->reg_start_addr, i2c_cmd->data_size, (UINT32) i2c_cmd->data_buffer);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_0: {
				object->set_vx1_gpio(DAL_SSENIFVX1_GPIO_0, ((UINT32)p_param) > 0);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_1: {
				object->set_vx1_gpio(DAL_SSENIFVX1_GPIO_1, ((UINT32)p_param) > 0);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_2: {
				object->set_vx1_gpio(DAL_SSENIFVX1_GPIO_2, ((UINT32)p_param) > 0);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_3: {
				object->set_vx1_gpio(DAL_SSENIFVX1_GPIO_3, ((UINT32)p_param) > 0);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_4: {
				object->set_vx1_gpio(DAL_SSENIFVX1_GPIO_4, ((UINT32)p_param) > 0);
			}
			break;


		case KDRV_SSENIF_VX1_VALID_WIDTH: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_VALID_WIDTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_PIXEL_DEPTH: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_PIXEL_DEPTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_VX1LANE_NUMBER: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_VX1LANE_NUMBER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_MIPILANE_NUMBER: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_MIPILANE_NUMBER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_HDR_ENABLE: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_HDR_ENABLE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE2: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE3: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE3, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE4: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_241_INPUT_CLK_FREQ: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_241_INPUT_CLK_FREQ, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_TARGET_MCLK: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_TARGET_MCLK, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_REAL_MCLK: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_REAL_MCLK, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_CKSPEED_BPS: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_CKSPEED_BPS, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL0: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL0, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL1: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL1, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL2: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL2, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL3: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL3, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL4: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL4, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL5: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL5, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL6: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL6, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL7: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL7, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_CKO_OUTPUT: {
				object->set_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_CKO_OUTOUT, (UINT32)p_param);
			}
			break;

		//case KDRV_SSENIF_VX1_I2C_READ:
		//case KDRV_SSENIF_VX1_GET_PLUG:
		//case KDRV_SSENIF_VX1_GET_LOCK:
		//case KDRV_SSENIF_VX1_GET_SPEED:
		//case KDRV_SSENIF_VX1_GET_START:
		default :
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}

	} else if (group == KDRV_SSENIF_SLVSEC_BASE) {

		/*
		    SLVSEC GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_SLVSEC_WAIT_INTERRUPT: {
				object->wait_interrupt((DAL_SSENIF_INTERRUPT)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_SENSORTYPE: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SENSORTYPE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_PIXEL_DEPTH: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_PIXEL_DEPTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DLANE_NUMBER: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DLANE_NUMBER, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_VALID_WIDTH: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_VALID_WIDTH, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_VALID_HEIGHT: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_VALID_HEIGHT, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_SPEED: {
				if (((UINT32)p_param) == 2304) {
					object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SPEED, DAL_SSENIFSLVSEC_SPEED_2304);
				} else if (((UINT32)p_param) == 1152) {
					object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SPEED, DAL_SSENIFSLVSEC_SPEED_1152);
				} else {
					DBG_WRN("unknown speed %d\r\n", ((UINT)p_param));
				}
			}
			break;
		case KDRV_SSENIF_SLVSEC_POST_INIT: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_POST_INIT, 0);
			}
			break;
		case KDRV_SSENIF_SLVSEC_TIMEOUT_PERIOD: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_TIMEOUT_PERIOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_STOP_METHOD: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_STOP_METHOD, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_IMGID_TO_SIE: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_IMGID_TO_SIE2: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE2, (UINT32)p_param);
			}
			break;

		case KDRV_SSENIF_SLVSEC_DATALANE0_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE0_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE1_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE1_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE2_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE2_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE3_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE3_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE4_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE4_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE5_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE5_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE6_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE6_PIN, (UINT32)p_param);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE7_PIN: {
				object->set_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE7_PIN, (UINT32)p_param);
			}
			break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}

	} else if (group == KDRV_SSENIF_GLOBAL_BASE) {

		switch (param_id) {
		case KDRV_SSENIF_SIEMCLK_ENABLE: {
#ifndef __KERNEL__

				if (((UINT32)p_param) > 0)
					pll_enable_clock(SIE_MCLK);
				else
					pll_disable_clock(SIE_MCLK);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemck");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get siemck clk fail\r\n", __func__);
				}

				if (((UINT32)p_param) > 0) {
					clk_prepare(src_clk);
					clk_enable(src_clk);
				} else {
					clk_unprepare(src_clk);
					clk_disable(src_clk);
				}

				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK2_ENABLE: {
#ifndef __KERNEL__

				if (((UINT32)p_param) > 0)
					pll_enable_clock(SIE_MCLK2);
				else
					pll_disable_clock(SIE_MCLK2);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk2");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get siemk2 clk fail\r\n", __func__);
				}

				if (((UINT32)p_param) > 0) {
					clk_prepare(src_clk);
					clk_enable(src_clk);
				} else {
					clk_unprepare(src_clk);
					clk_disable(src_clk);
				}

				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK3_ENABLE: {
#ifndef __KERNEL__

				if (((UINT32)p_param) > 0)
					pll_enable_clock(SIE_MCLK3);
				else
					pll_disable_clock(SIE_MCLK3);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk3");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get siemck3 clk fail\r\n", __func__);
				}

				if (((UINT32)p_param) > 0) {
					clk_prepare(src_clk);
					clk_enable(src_clk);
				} else {
					clk_unprepare(src_clk);
					clk_disable(src_clk);
				}

				clk_put(src_clk);
#endif
			}
			break;

		case KDRV_SSENIF_SIEMCLK_SOURCE: {
#ifndef __KERNEL__
				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_480);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL5);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL10);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLKSRC, PLL_CLKSEL_SIE_MCLKSRC_PLL12);
#else
				struct clk *parent_clk = NULL, *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemck");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

				siecommonclk = ((UINT32)p_param);

				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					parent_clk = clk_get(NULL, "fix480m");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					parent_clk = clk_get(NULL, "pll5");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					parent_clk = clk_get(NULL, "pll10");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					parent_clk = clk_get(NULL, "pll12");

				if (!IS_ERR(parent_clk)) {
					clk_set_parent(src_clk, parent_clk);
				} else
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

				clk_put(parent_clk);
				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK2_SOURCE: {
#ifndef __KERNEL__
				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_480);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL5);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL10);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK2SRC, PLL_CLKSEL_SIE_MCLK2SRC_PLL12);
#else
				struct clk *parent_clk = NULL, *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk2");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

				siecommonclk = ((UINT32)p_param);

				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					parent_clk = clk_get(NULL, "fix480m");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					parent_clk = clk_get(NULL, "pll5");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					parent_clk = clk_get(NULL, "pll10");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					parent_clk = clk_get(NULL, "pll12");

				if (!IS_ERR(parent_clk)) {
					clk_set_parent(src_clk, parent_clk);
				} else
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

				clk_put(parent_clk);
				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK3_SOURCE: {
#ifndef __KERNEL__
				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_480);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL5);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL10);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL12);
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL18)
					pll_set_clock_rate(PLL_CLKSEL_SIE_MCLK3SRC, PLL_CLKSEL_SIE_MCLK3SRC_PLL18);
#else
				struct clk *parent_clk = NULL, *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk3");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

				if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					parent_clk = clk_get(NULL, "fix480m");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					parent_clk = clk_get(NULL, "pll5");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					parent_clk = clk_get(NULL, "pll10");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					parent_clk = clk_get(NULL, "pll12");
				else if (((UINT32)p_param) == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL18)
					parent_clk = clk_get(NULL, "pll18");

				if (!IS_ERR(parent_clk))
					clk_set_parent(src_clk, parent_clk);
				else
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

				clk_put(parent_clk);
				clk_put(src_clk);
#endif
			}
			break;

		case KDRV_SSENIF_SIEMCLK_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_clock_freq(SIEMCLK_FREQ, (UINT32) p_param);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemck");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

            	clk_set_rate(src_clk, (unsigned long)p_param);

				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK2_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_clock_freq(SIEMCLK2_FREQ, (UINT32) p_param);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk2");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

            	clk_set_rate(src_clk, (unsigned long)p_param);

				clk_put(src_clk);
#endif
			}
			break;
		case KDRV_SSENIF_SIEMCLK3_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_clock_freq(SIEMCLK3_FREQ, (UINT32) p_param);
#else
				struct clk *src_clk = NULL;

				src_clk = clk_get(NULL, "f0c00000.siemk3");
				if (IS_ERR(src_clk)) {
					DBG_ERR("%s: get src clk fail\r\n", __func__);
				}

            	clk_set_rate(src_clk, (unsigned long)p_param);

				clk_put(src_clk);
#endif
			}
			break;

		case KDRV_SSENIF_PLL05_ENABLE: {
#ifndef __KERNEL__
				pll_set_pll_enable(PLL_ID_5, ((UINT32) p_param)>0);
#endif
			}
			break;
		case KDRV_SSENIF_PLL12_ENABLE: {
#ifndef __KERNEL__
				pll_set_pll_enable(PLL_ID_12, ((UINT32) p_param)>0);
#endif
			}
			break;
		case KDRV_SSENIF_PLL18_ENABLE: {
#ifndef __KERNEL__
				pll_set_pll_enable(PLL_ID_18, ((UINT32) p_param)>0);
#endif
			}
			break;

		case KDRV_SSENIF_PLL05_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_pll_freq(PLL_ID_5, (UINT32) p_param);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll5");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	clk_set_rate(parent_clk, (unsigned long)p_param);
				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL12_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_pll_freq(PLL_ID_12, (UINT32) p_param);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll12");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	clk_set_rate(parent_clk, (unsigned long)p_param);
				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL18_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_pll_freq(PLL_ID_18, (UINT32) p_param);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll18");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	clk_set_rate(parent_clk, (unsigned long)p_param);
				clk_put(parent_clk);
#endif
			}
			break;


		case KDRV_SSENIF_SIEMCLK_12SYNC_FREQUENCY: {
#ifndef __KERNEL__
				pll_set_clock_freq(SIEMCLK_12SYNC_FREQ, (UINT32) p_param);
#else
				struct clk *parent_clk = NULL, *src_clk2 = NULL;

				src_clk2 = clk_get(NULL, "siemck_common");
				if (IS_ERR(src_clk2)) {
					DBG_ERR("%s: get src2 clk fail\r\n", __func__);
				}

				if (siecommonclk == KDRV_SSENIFGLO_SIEMCLK_SOURCE_480)
					parent_clk = clk_get(NULL, "fix480m");
				else if (siecommonclk == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL5)
					parent_clk = clk_get(NULL, "pll5");
				else if (siecommonclk == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL10)
					parent_clk = clk_get(NULL, "pll10");
				else if (siecommonclk == KDRV_SSENIFGLO_SIEMCLK_SOURCE_PLL12)
					parent_clk = clk_get(NULL, "pll12");

				if (IS_ERR(parent_clk)) {
					DBG_ERR("%s: get parent_clk fail\r\n", __func__);
				}

				clk_set_parent(src_clk2, parent_clk);
				clk_put(parent_clk);
				clk_put(src_clk2);


				parent_clk = clk_get(NULL, "siemck_common");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

				if ((unsigned long)p_param > 50000) {
					clk_set_rate(parent_clk, (unsigned long)((unsigned long)p_param - 50000));
				}  else {
					clk_set_rate(parent_clk, (unsigned long)(50000));
				}

            	clk_set_rate(parent_clk, (unsigned long)p_param);
				clk_put(parent_clk);
#endif
			}
			break;



		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}


	} else {
		DBG_ERR("Err param_id 0x%08X\r\n", param_id);
		return -1;
	}






	return 0;
}

INT32 __kdrv_ssenif_get(UINT32 id, KDRV_SSENIF_PARAM_ID param_id, VOID *p_param)
{
	PDAL_SSENIFOBJ  object;
	UINT32          group, *ret, tmp;
	UINT32          engine;

	group = param_id & 0xFF000000;

	if (group != KDRV_SSENIF_GLOBAL_BASE) {

		engine = KDRV_DEV_ID_ENGINE(id);
		if ((engine == KDRV_SSENIF_ENGINE_CSI0)||(engine == KDRV_SSENIF_ENGINE_CSI1)) {
		} else if ((engine == KDRV_SSENIF_ENGINE_LVDS0)||(engine == KDRV_SSENIF_ENGINE_LVDS1)) {
#if SSENIF_CSI_EN
			engine = KDRV_SSENIF_ENGINE_CSI2 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#else
			engine = KDRV_SSENIF_ENGINE_CSI0 + engine - KDRV_SSENIF_ENGINE_LVDS0;
#endif
		} else {
			DBG_ERR("Err engine\r\n");
			return -1;
		}

		object = dal_ssenif_get_object((DAL_SSENIF_ID)(engine & 0xFF));

		if (!object->is_opened()) {
			DBG_ERR("no open 0x%08X\r\n", (UINT)id);
			return -1;
		}

		if (p_param == NULL) {
			DBG_ERR("null p_param\r\n");
			return -1;
		}
	}

	ret = (UINT32 *)p_param;
	tmp = *ret;
	*ret = 0;

	if (group == KDRV_SSENIF_CSI_BASE) {
		/*
		    CSI GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_CSI_WAIT_INTERRUPT: {
				*ret = object->wait_interrupt((DAL_SSENIF_INTERRUPT)tmp);
			}
			break;
		case KDRV_SSENIF_CSI_SENSORTYPE: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_SENSORTYPE);
			}
			break;
		case KDRV_SSENIF_CSI_SENSOR_TARGET_MCLK: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK);
			}
			break;
		case KDRV_SSENIF_CSI_SENSOR_REAL_MCLK: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK);
			}
			break;
		case KDRV_SSENIF_CSI_DLANE_NUMBER: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_DLANE_NUMBER);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE2: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE2);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE4: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE4);
			}
			break;
		case KDRV_SSENIF_CSI_VALID_HEIGHT_SIE5: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT_SIE5);
			}
			break;
		case KDRV_SSENIF_CSI_CLANE_SWITCH: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_CLANE_SWITCH);
			}
			break;
		case KDRV_SSENIF_CSI_TIMEOUT_PERIOD: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD);
			}
			break;
		case KDRV_SSENIF_CSI_STOP_METHOD: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_STOP_METHOD);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE2: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE4: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE4);
			}
			break;
		case KDRV_SSENIF_CSI_IMGID_TO_SIE5: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE5);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE0_PIN: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_DATALANE0_PIN);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE1_PIN: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_DATALANE1_PIN);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE2_PIN: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_DATALANE2_PIN);
			}
			break;
		case KDRV_SSENIF_CSI_DATALANE3_PIN: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_DATALANE3_PIN);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL2: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL2);
			}
			break;
		case KDRV_SSENIF_CSI_LPKT_MANUAL3: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_LPKT_MANUAL3);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL_FORMAT: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL_FORMAT);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL2_FORMAT: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL2_FORMAT);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL3_FORMAT: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL3_FORMAT);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL_DATA_ID: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL_DATA_ID);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL2_DATA_ID: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL2_DATA_ID);
			}
			break;
		case KDRV_SSENIF_CSI_MANUAL3_DATA_ID: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_MANUAL3_DATA_ID);
			}
			break;

		case KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_HSCLK);
			}
			break;
		case KDRV_SSENIF_CSI_SENSOR_FS_CNT: {
				*ret = object->get_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_FS_CNT);
			}
			break;
		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}

	} else if (group == KDRV_SSENIF_LVDS_BASE) {
		/*
		    LVDS GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_LVDS_WAIT_INTERRUPT: {
				*ret = object->wait_interrupt((DAL_SSENIF_INTERRUPT)tmp);
			}
			break;
		case KDRV_SSENIF_LVDS_SENSORTYPE: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_SENSORTYPE);
			}
			break;
		case KDRV_SSENIF_LVDS_DLANE_NUMBER: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_DLANE_NUMBER);
			}
			break;
		case KDRV_SSENIF_LVDS_VALID_WIDTH: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_WIDTH);
			}
			break;
		case KDRV_SSENIF_LVDS_VALID_HEIGHT: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_HEIGHT);
			}
			break;
		case KDRV_SSENIF_LVDS_PIXEL_DEPTH: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH);
			}
			break;
		case KDRV_SSENIF_LVDS_CLANE_SWITCH: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_CLANE_SWITCH);
			}
			break;
		case KDRV_SSENIF_LVDS_TIMEOUT_PERIOD: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_TIMEOUT_PERIOD);
			}
			break;
		case KDRV_SSENIF_LVDS_STOP_METHOD: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_STOP_METHOD);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE2: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE3: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE3);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_TO_SIE4: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE4);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_0: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_0);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_1: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_1);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_2: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_2);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_3: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_3);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_4: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_4);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_5: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_5);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_6: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_6);
			}
			break;
		case KDRV_SSENIF_LVDS_OUTORDER_7: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_7);
			}
			break;

		case KDRV_SSENIF_LVDS_PIXEL_INORDER: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_INORDER);
			}
			break;
		case KDRV_SSENIF_LVDS_FSET_BIT: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_FSET_BIT);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT0: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT0);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT1: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT1);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT2: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT2);
			}
			break;
		case KDRV_SSENIF_LVDS_IMGID_BIT3: {
				*ret = object->get_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_BIT3);
			}
			break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}
	} else if (group == KDRV_SSENIF_VX1_BASE) {

		/*
		    VX1 GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_VX1_WAIT_INTERRUPT: {
				*ret = object->wait_interrupt((DAL_SSENIF_INTERRUPT)tmp);
			}
			break;
		case KDRV_SSENIF_VX1_TXTYPE: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_TXTYPE);
			}
			break;
		case KDRV_SSENIF_VX1_SENSORTYPE: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENSORTYPE);
			}
			break;
		case KDRV_SSENIF_VX1_DATAMUX_SEL: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_DATAMUX_SEL);
			}
			break;
		case KDRV_SSENIF_VX1_TIMEOUT_PERIOD: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_TIMEOUT_PERIOD);
			}
			break;

		case KDRV_SSENIF_VX1_I2C_SPEED: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_I2C_SPEED);
			}
			break;
		case KDRV_SSENIF_VX1_I2CNACK_CHECK: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_I2CNACK_CHECK);
			}
			break;

		case KDRV_SSENIF_VX1_SENSOR_SLAVEADDR: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_SLAVEADDR);
			}
			break;
		case KDRV_SSENIF_VX1_SENREG_ADDR_BC: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_ADDR_BC);
			}
			break;
		case KDRV_SSENIF_VX1_SENREG_DATA_BC: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_DATA_BC);
			}
			break;
		case KDRV_SSENIF_VX1_I2C_READ: {
				PKDRV_SSENIFVX1_I2CCMD i2c_cmd;
				UINT32 addr, sz, dbc;
				UINT8 *buf;

				i2c_cmd = (PKDRV_SSENIFVX1_I2CCMD)p_param;

				addr = tmp;//i2c_cmd->reg_start_addr;
				sz   = i2c_cmd->data_size;
				buf  = (UINT8 *)i2c_cmd->data_buffer;
				dbc  = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_DATA_BC);

				while (sz >= dbc) {
					object->sensor_i2c_read(addr, (UINT32 *)buf);
					addr += dbc;
					buf  += dbc;
					sz   -= dbc;
				}
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_0: {
				*ret = object->get_vx1_gpio(DAL_SSENIFVX1_GPIO_0);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_1: {
				*ret = object->get_vx1_gpio(DAL_SSENIFVX1_GPIO_1);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_2: {
				*ret = object->get_vx1_gpio(DAL_SSENIFVX1_GPIO_2);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_3: {
				*ret = object->get_vx1_gpio(DAL_SSENIFVX1_GPIO_3);
			}
			break;
		case KDRV_SSENIF_VX1_GPIO_4: {
				*ret = object->get_vx1_gpio(DAL_SSENIFVX1_GPIO_4);
			}
			break;


		case KDRV_SSENIF_VX1_VALID_WIDTH: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_VALID_WIDTH);
			}
			break;
		case KDRV_SSENIF_VX1_PIXEL_DEPTH: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_PIXEL_DEPTH);
			}
			break;
		case KDRV_SSENIF_VX1_VX1LANE_NUMBER: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_VX1LANE_NUMBER);
			}
			break;
		case KDRV_SSENIF_VX1_MIPILANE_NUMBER: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_MIPILANE_NUMBER);
			}
			break;
		case KDRV_SSENIF_VX1_HDR_ENABLE: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_HDR_ENABLE);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE2: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE2);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE3: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE3);
			}
			break;
		case KDRV_SSENIF_VX1_IMGID_TO_SIE4: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_IMGID_TO_SIE4);
			}
			break;
		case KDRV_SSENIF_VX1_241_INPUT_CLK_FREQ: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_241_INPUT_CLK_FREQ);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_TARGET_MCLK: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_TARGET_MCLK);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_REAL_MCLK: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_REAL_MCLK);
			}
			break;
		case KDRV_SSENIF_VX1_SENSOR_CKSPEED_BPS: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_CKSPEED_BPS);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL0: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL0);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL1: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL1);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL2: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL2);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL3: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL3);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL4: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL4);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL5: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL5);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL6: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL6);
			}
			break;
		case KDRV_SSENIF_VX1_THCV241_PLL7: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_THCV241_PLL7);
			}
			break;

		case KDRV_SSENIF_VX1_GET_PLUG: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_PLUG);
			}
			break;
		case KDRV_SSENIF_VX1_GET_LOCK: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_LOCK);
			}
			break;
		case KDRV_SSENIF_VX1_GET_SPEED: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_SPEED);
			}
			break;
		case KDRV_SSENIF_VX1_GET_START: {
				*ret = object->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_START);
			}
			break;

		//case KDRV_SSENIF_VX1_LOCKCHG_CALLBACK
		//case KDRV_SSENIF_VX1_I2C_WRITE:
		default :
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}

	} else if (group == KDRV_SSENIF_SLVSEC_BASE) {

		/*
		    SLVSEC GROUP
		*/
		switch (param_id) {
		case KDRV_SSENIF_SLVSEC_WAIT_INTERRUPT: {
				*ret = object->wait_interrupt((DAL_SSENIF_INTERRUPT)tmp);
			}
			break;
		case KDRV_SSENIF_SLVSEC_SENSORTYPE: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SENSORTYPE);
			}
			break;
		case KDRV_SSENIF_SLVSEC_PIXEL_DEPTH: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_PIXEL_DEPTH);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DLANE_NUMBER: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DLANE_NUMBER);
			}
			break;
		case KDRV_SSENIF_SLVSEC_VALID_WIDTH: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_VALID_WIDTH);
			}
			break;
		case KDRV_SSENIF_SLVSEC_VALID_HEIGHT: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_VALID_HEIGHT);
			}
			break;
		case KDRV_SSENIF_SLVSEC_SPEED: {
				if (object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_SPEED) == DAL_SSENIFSLVSEC_SPEED_1152) {
					*ret = 1152;
				} else {
					*ret = 2304;
				}
			}
			break;
		case KDRV_SSENIF_SLVSEC_TIMEOUT_PERIOD: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_TIMEOUT_PERIOD);
			}
			break;
		case KDRV_SSENIF_SLVSEC_STOP_METHOD: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_STOP_METHOD);
			}
			break;
		case KDRV_SSENIF_SLVSEC_IMGID_TO_SIE: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE);
			}
			break;
		case KDRV_SSENIF_SLVSEC_IMGID_TO_SIE2: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_IMGID_TO_SIE2);
			}
			break;

		case KDRV_SSENIF_SLVSEC_DATALANE0_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE0_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE1_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE1_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE2_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE2_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE3_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE3_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE4_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE4_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE5_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE5_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE6_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE6_PIN);
			}
			break;
		case KDRV_SSENIF_SLVSEC_DATALANE7_PIN: {
				*ret = object->get_slvsec_config(DAL_SSENIFSLVSEC_CFGID_DATALANE7_PIN);
			}
			break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}
	} else if (group == KDRV_SSENIF_GLOBAL_BASE) {

		switch (param_id) {
		case KDRV_SSENIF_PLL05_FREQUENCY: {
#ifndef __KERNEL__
				*ret = pll_get_pll_freq(PLL_ID_5);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll5");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret = clk_get_rate(parent_clk);
				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL12_FREQUENCY: {
#ifndef __KERNEL__
				*ret = pll_get_pll_freq(PLL_ID_12);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll12");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret = clk_get_rate(parent_clk);
				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL18_FREQUENCY: {
#ifndef __KERNEL__
				*ret = pll_get_pll_freq(PLL_ID_18);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll18");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret = clk_get_rate(parent_clk);
				clk_put(parent_clk);
#endif
			}
			break;

		case KDRV_SSENIF_PLL05_ENABLE: {
#ifndef __KERNEL__
				*ret = pll_get_pll_enable(PLL_ID_5);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll5");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret= __clk_is_enabled(parent_clk);

				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL12_ENABLE: {
#ifndef __KERNEL__
				*ret = pll_get_pll_enable(PLL_ID_12);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll12");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret= __clk_is_enabled(parent_clk);

				clk_put(parent_clk);
#endif
			}
			break;
		case KDRV_SSENIF_PLL18_ENABLE: {
#ifndef __KERNEL__
				*ret = pll_get_pll_enable(PLL_ID_18);
#else
				struct clk *parent_clk = NULL;

				parent_clk = clk_get(NULL, "pll18");
				if (IS_ERR(parent_clk))
					DBG_ERR("%s: get parent clk fail\r\n", __func__);

            	*ret= __clk_is_enabled(parent_clk);

				clk_put(parent_clk);
#endif
			}
			break;

		default:
			DBG_ERR("Err param_id 0x%08X\r\n", param_id);
			return -1;
		}
	} else {
		DBG_ERR("Err param_id 0x%08X\r\n", param_id);
		return -1;
	}


	return 0;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(kdrv_ssenif_open);
EXPORT_SYMBOL(kdrv_ssenif_close);
EXPORT_SYMBOL(kdrv_ssenif_trigger);
EXPORT_SYMBOL(__kdrv_ssenif_set);
EXPORT_SYMBOL(__kdrv_ssenif_get);
#endif

