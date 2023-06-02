#ifndef _ISP_MTR_INT_H_
#define _ISP_MTR_INT_H_

#include "isp_mtr.h"

//=============================================================================
// extern functions
//=============================================================================
extern CTL_MTR_DRV_TAB *isp_dev_get_mtr_drv_tab(void);
extern ER isp_api_get_iris(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_get_focus(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_get_zoom(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_get_misc(UINT32 cmd_type, MTR_CTL_LCMD *ctl_cmd);
extern ER isp_api_set_iris(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_set_focus(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_set_zoom(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd);
extern ER isp_api_set_misc(UINT32 cmd_type, MTR_CTL_LCMD *ctl_cmd);

#endif

