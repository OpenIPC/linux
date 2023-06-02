#include "isf_vdoenc_drv.h"
#include "isf_vdoenc_ioctl.h"
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_core.h"
#include "kflow_videoenc/isf_vdoenc.h"
#include "kflow_videoenc/isf_vdoenc_platform.h"
#include "nmediarec_api.h"

#include <linux/of_device.h>

//#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "isf_vdoenc_dbg.h"

///#include "../isf_vdoenc_int.h"

void* ISF_VdoEnc_Unit_Addr = NULL;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int _isf_vdoenc_alloc_unit(void)
{
    char *path = "/hdal-maxpath-cfg";
    struct device_node *dt_node;
	UINT32 vdoenc_maxpath = 0, buff_addr = 0, buff_size = 0, i = 0;

    dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		DBG_ERR("Failed to find node by path: %s.\r\n", path);
		return -1;
    }
	if (of_property_read_u32(dt_node, "vdoenc_maxpath", &vdoenc_maxpath)) {
		DBG_ERR("Failed to read vdoenc_maxpath\r\n");
		return -1;
	}

	// port count
	isf_vdoenc.port_in_count = vdoenc_maxpath,
	isf_vdoenc.port_out_count = vdoenc_maxpath,
	isf_vdoenc.port_path_count = vdoenc_maxpath,

	buff_size = (sizeof(ISF_PORT_CAPS *) + sizeof(ISF_PORT_CAPS *) + sizeof(ISF_STATE) + sizeof(ISF_STATE *)
				+ sizeof(ISF_INFO) + sizeof(ISF_INFO *) + sizeof(ISF_INFO) + sizeof(ISF_INFO *)
				+ sizeof(ISF_PORT_PATH)) * vdoenc_maxpath;
	ISF_VdoEnc_Unit_Addr = vdoenc_alloc(buff_size);
	if (ISF_VdoEnc_Unit_Addr == 0) {
		DBG_ERR("ISF_VdoEnc_Unit_Addr get fail !!!\r\n");
		return -1;
	}

	buff_addr = (UINT32)ISF_VdoEnc_Unit_Addr;

	// ISF_VdoEnc_InputPortList_Caps
	ISF_VdoEnc_InputPortList_Caps = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		*(ISF_VdoEnc_InputPortList_Caps + i) = &ISF_VdoEnc_Input_Caps;
	}
	isf_vdoenc.port_incaps = ISF_VdoEnc_InputPortList_Caps;
	buff_addr += sizeof(ISF_PORT_CAPS *)*vdoenc_maxpath;


	// ISF_VdoEnc_OutputPortList_Caps
	ISF_VdoEnc_OutputPortList_Caps = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		*(ISF_VdoEnc_OutputPortList_Caps + i) = &ISF_VdoEnc_OutputPort_Caps;
	}
	isf_vdoenc.port_outcaps = ISF_VdoEnc_OutputPortList_Caps;
	buff_addr += sizeof(ISF_PORT_CAPS *)*vdoenc_maxpath;


	// ISF_VdoEnc_OutputPort_State
	ISF_VdoEnc_OutputPort_State = (void *)buff_addr;
	memset(ISF_VdoEnc_OutputPort_State, 0, sizeof(ISF_STATE)*vdoenc_maxpath);
	buff_addr += sizeof(ISF_STATE)*vdoenc_maxpath;
	ISF_VdoEnc_OutputPortList_State = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		*(ISF_VdoEnc_OutputPortList_State + i) = &ISF_VdoEnc_OutputPort_State[i];
	}
	isf_vdoenc.port_outstate = ISF_VdoEnc_OutputPortList_State;
	buff_addr += sizeof(ISF_STATE *)*vdoenc_maxpath;


	// isf_vdoenc_outputinfolist_in
	isf_vdoenc_outputinfo_in = (void *)buff_addr;
	memset(isf_vdoenc_outputinfo_in, 0, sizeof(ISF_INFO)*vdoenc_maxpath);
	buff_addr += sizeof(ISF_INFO)*vdoenc_maxpath;
	isf_vdoenc_outputinfolist_in = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		*(isf_vdoenc_outputinfolist_in + i) = &isf_vdoenc_outputinfo_in[i];
	}
	isf_vdoenc.port_ininfo = isf_vdoenc_outputinfolist_in;
	buff_addr += sizeof(ISF_INFO *)*vdoenc_maxpath;


	// isf_vdoenc_outputinfolist_out
	isf_vdoenc_outputinfo_out = (void *)buff_addr;
	memset(isf_vdoenc_outputinfo_out, 0, sizeof(ISF_INFO)*vdoenc_maxpath);
	buff_addr += sizeof(ISF_INFO)*vdoenc_maxpath;
	isf_vdoenc_outputinfolist_out = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		*(isf_vdoenc_outputinfolist_out + i) = &isf_vdoenc_outputinfo_out[i];//&ISF_VdoEnc_OutputPort_State[i];
	}
	isf_vdoenc.port_outinfo = isf_vdoenc_outputinfolist_out;
	buff_addr += sizeof(ISF_INFO *)*vdoenc_maxpath;


	// ISF_VdoEnc_PathList
	ISF_VdoEnc_PathList = (void *)buff_addr;
	for (i = 0; i < vdoenc_maxpath; i++) {
		ISF_PORT_PATH *pPortPath = &ISF_VdoEnc_PathList[i];
		pPortPath->p_unit = &isf_vdoenc;
		pPortPath->iport = ISF_IN(i);
		pPortPath->oport = ISF_OUT(i);
	}
	isf_vdoenc.port_path = ISF_VdoEnc_PathList;

	return 0;
}

int _isf_vdoenc_free_unit(void)
{
	if (ISF_VdoEnc_Unit_Addr) {
		vdoenc_free(ISF_VdoEnc_Unit_Addr);
	}

	return 0;
}

int isf_vdoenc_drv_open(ISF_VDOENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_vdoenc_drv_release(ISF_VDOENC_INFO *pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_vdoenc_drv_init(ISF_VDOENC_INFO *pmodule_info)
{
	int er = 0;
	/* device node path - check it from /proc/device-tree/ */

	er = _isf_vdoenc_alloc_unit();
	if (er != 0) {
		DBG_ERR("Alloc isf_vdoenc unit fail !!!\r\n");
		return er;
	}

	//isf_vdoenc_install_id();
	//nmr_vdoenc_install_id();
	nmr_vdocodec_install_id();

    isf_reg_unit(ISF_UNIT_ID(VDOENC,0), &isf_vdoenc);

	return er;
}

int isf_vdoenc_drv_remove(ISF_VDOENC_INFO *pmodule_info)
{
	int er = 0;

	er = _isf_vdoenc_free_unit();
	if (er != 0) {
		DBG_ERR("Alloc isf_vdoenc unit fail !!!\r\n");
		return er;
	}

	//isf_vdoenc_uninstall_id();
	//nmr_vdoenc_uninstall_id();
	nmr_vdocodec_uninstall_id();

	return 0;
}

int isf_vdoenc_drv_suspend(ISF_VDOENC_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_vdoenc_drv_resume(ISF_VDOENC_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

int isf_vdoenc_drv_ioctl(unsigned char minor, ISF_VDOENC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int er = 0;

	DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {
	case ISF_VDOENC_IOC_CMD: {
		}
		break;
	}

	return er;
}
