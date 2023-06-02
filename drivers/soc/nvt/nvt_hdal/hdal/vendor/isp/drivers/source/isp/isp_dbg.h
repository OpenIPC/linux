#ifndef _ISP_DBG_H_
#define _ISP_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          isp
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"
#include "kflow_common/isp_if.h"

#define ISP_DBG_NONE                   0x0000000
#define ISP_DBG_SYNC_AE_STATUS         0x0000001
#define ISP_DBG_SYNC_TOTAL_GAIN        0x0000002
#define ISP_DBG_SYNC_D_GAIN            0x0000004
#define ISP_DBG_SYNC_LV                0x0000010
#define ISP_DBG_SYNC_LV_BASE           0x0000020
#define ISP_DBG_SYNC_SIE_CA            0x0000080
#define ISP_DBG_SYNC_C_GAIN            0x0000100
#define ISP_DBG_SYNC_FINAL_C_GAIN      0x0000200
#define ISP_DBG_SYNC_CT                0x0000400
#define ISP_DBG_SYNC_CAPTURE           0x0000800
#define ISP_DBG_SENSOR_EXPT            0x0001000
#define ISP_DBG_SENSOR_GAIN            0x0002000
#define ISP_DBG_SENSOR_REG             0x0004000
#define ISP_DBG_SIE_CB                 0x0010000
#define ISP_DBG_IPP_CB                 0x0020000
#define ISP_DBG_ENC_CB                 0x0040000
#define ISP_DBG_SIE_SET                0x0100000
#define ISP_DBG_IPP_SET                0x0200000
#define ISP_DBG_ENC_SET                0x0400000
#define ISP_DBG_ERR_MSG                0x1000000
#define ISP_DBG_WRN_MSG                0x2000000

#define ISP_BYPASS_NONE_PARAM          0x0000000
#define ISP_BYPASS_SIE_ROI             0x0000001
#define ISP_BYPASS_SIE_PARAM           0x0000002
#define ISP_BYPASS_IFE_PARAM           0x0000004
#define ISP_BYPASS_DCE_PARAM           0x0000008
#define ISP_BYPASS_IPE_PARAM           0x0000010
#define ISP_BYPASS_IFE2_PARAM          0x0000020
#define ISP_BYPASS_IME_PARAM           0x0000040
#define ISP_BYPASS_IFE_VIG_CENT        0x0000080
#define ISP_BYPASS_DCE_DC_CENT         0x0000100
#define ISP_BYPASS_IFE2_FILT_TIME      0x0000200
#define ISP_BYPASS_IME_LCA_SIZE        0x0000400
#define ISP_BYPASS_IPE_VA_WIN_SIZE     0x0000800
#define ISP_BYPASS_ENC_3DNR_PARAM      0x0001000
#define ISP_BYPASS_ENC_SHARPEN_PARAM   0x0002000
#define ISP_BYPASS_SEN_REG             0x0004000
#define ISP_BYPASS_SEN_EXPT            0x0008000
#define ISP_BYPASS_SEN_GAIN            0x0010000
#define ISP_BYPASS_SEN_DIR             0x0020000
#define ISP_BYPASS_SEN_SLEEP           0x0040000
#define ISP_BYPASS_SEN_WAKEUP          0x0080000

#define PRINT_ISP(type, fmt, args...) {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_ISP_ERR(type, fmt, args...) {if (type) DBG_ERR(fmt, ## args); if (isp_dbg_check_err_msg(type)) DBG_ERR(fmt, ## args); }
#define PRINT_ISP_WRN(type, fmt, args...) {if (type) DBG_WRN(fmt, ## args); if (isp_dbg_check_wrn_msg(type)) DBG_WRN(fmt, ## args); }

extern UINT32 isp_dbg_get_dbg_mode(ISP_ID id);
extern void isp_dbg_set_dbg_mode(ISP_ID id, UINT32 cmd);
extern UINT32 isp_dbg_get_bypass_eng(ISP_ID id);
extern void isp_dbg_set_bypass_eng(ISP_ID id, UINT32 cmd);
extern BOOL isp_dbg_check_err_msg(BOOL show_dbg_msg);
extern void isp_dbg_clr_err_msg(void);
extern UINT32 isp_dbg_get_err_msg(void);
extern BOOL isp_dbg_check_wrn_msg(BOOL show_dbg_msg);
extern void isp_dbg_clr_wrn_msg(void);
extern UINT32 isp_dbg_get_wrn_msg(void);

#endif
