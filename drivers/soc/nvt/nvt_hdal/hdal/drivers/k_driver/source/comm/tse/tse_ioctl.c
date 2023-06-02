#include <linux/uaccess.h>
#include "comm/tse_ioctl.h"
#include "tse_platform_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
static TSE_BUF_INFO src0_info = {0};
static TSE_BUF_INFO dst0_info = {0};
static TSE_BUF_INFO dst1_info = {0};
static TSE_BUF_INFO dst2_info = {0};

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_tse_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
	int __user *argp = (int __user*)argc;
	int erReturn;

	switch (cmd) {
		case TSE_IOC_OPEN: {
			erReturn = tse_open();
			if (erReturn != E_OK) {
				return erReturn;
			}
			break;
		}
		case TSE_IOC_CLOSE: {
			erReturn = tse_close();
			if (erReturn != E_OK) {
	            return erReturn;
	        }
			break;
		}
		case TSE_IOC_MUX_W_CFG: {
			TSE_IOC_MUX_OBJ mux_obj;
			if(copy_from_user(&mux_obj, argp, sizeof(mux_obj)))
	            return -EFAULT;
			tse_setConfig(TSMUX_CFG_ID_PAYLOADSIZE, mux_obj.cfg.payload_size);
			src0_info.addr = mux_obj.cfg.src_info.addr;
			src0_info.size = mux_obj.cfg.src_info.size;
			src0_info.pnext = NULL;
			tse_setConfig(TSMUX_CFG_ID_SRC_INFO, (UINT32)&src0_info);
			dst0_info.addr = mux_obj.cfg.dst_info.addr;
			dst0_info.size = mux_obj.cfg.dst_info.size;
			dst0_info.pnext = NULL;
			tse_setConfig(TSMUX_CFG_ID_DST_INFO, (UINT32)&dst0_info);
			tse_setConfig(TSMUX_CFG_ID_SYNC_BYTE, mux_obj.cfg.sync_byte);
			tse_setConfig(TSMUX_CFG_ID_CONTINUITY_CNT, mux_obj.cfg.continuity_cnt);
			tse_setConfig(TSMUX_CFG_ID_PID, mux_obj.cfg.pid);
			tse_setConfig(TSMUX_CFG_ID_TEI, mux_obj.cfg.tei);
			tse_setConfig(TSMUX_CFG_ID_TP, mux_obj.cfg.tp);
			tse_setConfig(TSMUX_CFG_ID_SCRAMBLECTRL, mux_obj.cfg.scramblectrl);
			tse_setConfig(TSMUX_CFG_ID_START_INDICTOR, mux_obj.cfg.start_indictor);
			tse_setConfig(TSMUX_CFG_ID_STUFF_VAL, mux_obj.cfg.stuff_val);
			tse_setConfig(TSMUX_CFG_ID_ADAPT_FLAGS, mux_obj.cfg.adapt_flags);
			erReturn = tse_start(mux_obj.obj.bWait, TSE_MODE_TSMUX);
			if (erReturn != E_OK) {
	            return erReturn;
	        }
			mux_obj.cfg.muxing_len = tse_getConfig(TSMUX_CFG_ID_MUXING_LEN);
			mux_obj.cfg.con_curr_cnt = tse_getConfig(TSMUX_CFG_ID_CON_CURR_CNT);
			mux_obj.cfg.last_data_mux_mode = tse_getConfig(TSMUX_CFG_ID_LAST_DATA_MUX_MODE);
			if(copy_to_user(argp, &mux_obj, sizeof(mux_obj)))
	            return -EFAULT;
			break;
		}
		case TSE_IOC_DEMUX_W_CFG: {
			TSE_IOC_DEMUX_OBJ demux_obj;
			if(copy_from_user(&demux_obj, argp, sizeof(demux_obj)))
	            return -EFAULT;
			tse_setConfig(TSDEMUX_CFG_ID_SYNC_BYTE, demux_obj.cfg.sync_byte);
			tse_setConfig(TSDEMUX_CFG_ID_ADAPTATION_FLAG, demux_obj.cfg.adaptation_flag);
			src0_info.addr = demux_obj.cfg.in_info.addr;
			src0_info.size = demux_obj.cfg.in_info.size;
			src0_info.pnext = NULL;
			tse_setConfig(TSDEMUX_CFG_ID_IN_INFO, (UINT32)&src0_info);
			tse_setConfig(TSDEMUX_CFG_ID_PID0_ENABLE, demux_obj.cfg.pid_enable[0]);
			if (demux_obj.cfg.pid_enable[0]) {
				tse_setConfig(TSDEMUX_CFG_ID_PID0_VALUE, demux_obj.cfg.pid_value[0]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_MODE, demux_obj.cfg.continuity_mode[0]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY0_VALUE, demux_obj.cfg.continuity_value[0]);
				dst0_info.addr = demux_obj.cfg.out_info[0].addr;
				dst0_info.size = demux_obj.cfg.out_info[0].size;
				dst0_info.pnext = NULL;
				tse_setConfig(TSDEMUX_CFG_ID_OUT0_INFO, (UINT32)&dst0_info);
			}
			if (demux_obj.cfg.pid_enable[1]) {
				tse_setConfig(TSDEMUX_CFG_ID_PID1_VALUE, demux_obj.cfg.pid_value[1]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY1_MODE, demux_obj.cfg.continuity_mode[1]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY1_VALUE, demux_obj.cfg.continuity_value[1]);
				dst1_info.addr = demux_obj.cfg.out_info[1].addr;
				dst1_info.size = demux_obj.cfg.out_info[1].size;
				dst1_info.pnext = NULL;
				tse_setConfig(TSDEMUX_CFG_ID_OUT1_INFO, (UINT32)&dst1_info);
			}
			if (demux_obj.cfg.pid_enable[2]) {
				tse_setConfig(TSDEMUX_CFG_ID_PID2_VALUE, demux_obj.cfg.pid_value[2]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY2_MODE, demux_obj.cfg.continuity_mode[2]);
				tse_setConfig(TSDEMUX_CFG_ID_CONTINUITY2_VALUE, demux_obj.cfg.continuity_value[2]);
				dst2_info.addr = demux_obj.cfg.out_info[2].addr;
				dst2_info.size = demux_obj.cfg.out_info[2].size;
				dst2_info.pnext = NULL;
				tse_setConfig(TSDEMUX_CFG_ID_OUT2_INFO, (UINT32)&dst2_info);
			}
			erReturn = tse_start(demux_obj.obj.bWait, TSE_MODE_TSDEMUX);
			if (erReturn != E_OK) {
	            return erReturn;
	        }
			if (demux_obj.cfg.pid_enable[0]) {
				demux_obj.cfg.out_total_len[0] = tse_getConfig(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN);
				demux_obj.cfg.continuity_last[0] = tse_getConfig(TSDEMUX_CFG_ID_CONTINUITY0_LAST);
			}
			if (demux_obj.cfg.pid_enable[1]) {
				demux_obj.cfg.out_total_len[1] = tse_getConfig(TSDEMUX_CFG_ID_OUT1_TOTAL_LEN);
				demux_obj.cfg.continuity_last[1] = tse_getConfig(TSDEMUX_CFG_ID_CONTINUITY1_LAST);
			}
			if (demux_obj.cfg.pid_enable[2]) {
				demux_obj.cfg.out_total_len[2] = tse_getConfig(TSDEMUX_CFG_ID_OUT2_TOTAL_LEN);
				demux_obj.cfg.continuity_last[2] = tse_getConfig(TSDEMUX_CFG_ID_CONTINUITY2_LAST);
			}
			if(copy_to_user(argp, &demux_obj, sizeof(demux_obj)))
	            return -EFAULT;
			break;
		}
		case TSE_IOC_HWCPY_W_CFG: {
			TSE_IOC_HWCPY_OBJ hwcpy_obj;
			if(copy_from_user(&hwcpy_obj, argp, sizeof(hwcpy_obj)))
	            return -EFAULT;
			tse_setConfig(HWCOPY_CFG_ID_CMD, hwcpy_obj.cfg.id_cmd);
			if (hwcpy_obj.cfg.id_cmd == HWCOPY_LINEAR_SET) {
				tse_setConfig(HWCOPY_CFG_ID_CTEX, hwcpy_obj.cfg.id_ctex);
			}
			if (hwcpy_obj.cfg.id_cmd == HWCOPY_LINEAR_COPY) {
				tse_setConfig(HWCOPY_CFG_ID_SRC_ADDR, (UINT32)hwcpy_obj.cfg.src_addr);
			}
			tse_setConfig(HWCOPY_CFG_ID_DST_ADDR, (UINT32)hwcpy_obj.cfg.dst_addr);
			tse_setConfig(HWCOPY_CFG_ID_SRC_LEN, hwcpy_obj.cfg.src_len);
			erReturn = tse_start(hwcpy_obj.obj.bWait, TSE_MODE_HWCOPY);
			if (erReturn != E_OK) {
	            return erReturn;
	        }
			hwcpy_obj.cfg.total_len = tse_getConfig(HWCOPY_CFG_ID_TOTAL_LEN);
			if(copy_to_user(argp, &hwcpy_obj, sizeof(hwcpy_obj)))
	            return -EFAULT;
			break;
		}
		default: {
			return E_NOSPT;
		}
	}

	return E_OK;
}

