#include "kwrap/error_no.h"
#include "kwrap/type.h"

#include "isp_dev.h"

//=============================================================================
// external functions
//=============================================================================

ER isp_api_get_iris(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->get_aperture(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_get_focus(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->get_focus(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_get_zoom(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->get_zoom(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_get_misc(UINT32 cmd_type, MTR_CTL_LCMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->get_misc(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_set_iris(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->set_aperture(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_set_focus(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->set_focus(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_set_zoom(UINT32 cmd_type, MTR_CTL_CMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->set_zoom(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

ER isp_api_set_misc(UINT32 cmd_type, MTR_CTL_LCMD *ctl_cmd)
{
	CTL_MTR_DRV_TAB *mrt_drv_tab = isp_dev_get_mtr_drv_tab();
	ER rt = E_OK;

	if (mrt_drv_tab == NULL) {
		return E_SYS;
	}

	rt = mrt_drv_tab->set_misc(MTR_ID_0, cmd_type, ctl_cmd);

	return rt;
}

