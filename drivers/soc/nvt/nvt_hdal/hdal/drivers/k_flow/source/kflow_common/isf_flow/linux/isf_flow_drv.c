#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include "isf_flow_drv.h"
#include "isf_flow_ioctl.h"
#include "../isf_flow_api.h"
#include "comm/hwclock.h"
#include "isf_flow_dbg.h"
#include "../isf_flow_int.h"

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_flow_drv_open(PMODULE_INFO pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int isf_flow_drv_close(void)
{
	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_flow_drv_release(PMODULE_INFO pmodule_info, unsigned char minor)
{
	DBG_IND("%d\n", minor);

	isf_abort(minor);
	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int isf_flow_drv_init(PMODULE_INFO pmodule_info)
{
	int er = 0;
#if 0
	ISF_FLOW_INIT init = {0};

	isf_flow_install_id();

	init.api_ver = ISF_STREAM_API_VERSION;
	if (!isf_flow_init(&init)) {
		DBG_ERR("stream_init() Failed!\r\n");
		return -ENODEV;
	}
	isf_flow_regbi_adj();
#endif
	isf_flow_install_id();

	return er;

#if 0
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return er;
#endif
}

int isf_flow_drv_remove(PMODULE_INFO pmodule_info)
{
#if 0
	isf_flow_exit();
#endif
	isf_flow_uninstall_id();
	return 0;
}

int isf_flow_drv_suspend(PMODULE_INFO pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int isf_flow_drv_resume(PMODULE_INFO pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

static int isf_flow_drv_parse_maxpath_from_dts_p(ISF_FLOW_IOCTL_GET_MAX_PATH *p_max_path)
{
    /* device node path - check it from /proc/device-tree/ */
    char *path = "/hdal-maxpath-cfg";
    struct device_node *dt_node;

    dt_node = of_find_node_by_path(path);
	if (!dt_node) {
		DBG_ERR("Failed to find node by path: %s.\r\n", path);
		return -1;
    }
	DBG_IND("Found the node for %s.\r\n", path);
	if (of_property_read_u32_array(dt_node, "vdocap_active_list", p_max_path->vdocap_active_list, 8)) {
		DBG_ERR("Failed to read vdocap_active_list\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "vdoprc_maxdevice", &p_max_path->vdoprc_maxdevice)) {
		DBG_ERR("Failed to read vdoprc_maxdevice\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "vdoenc_maxpath", &p_max_path->vdoenc_maxpath)) {
		DBG_ERR("Failed to read vdoenc_maxpath\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "vdodec_maxpath", &p_max_path->vdodec_maxpath)) {
		DBG_ERR("Failed to read vdodec_maxpath\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "vdoout_maxdevice", &p_max_path->vdoout_maxdevice)) {
		DBG_ERR("Failed to read vdoout_maxdevice\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "adocap_maxdevice", &p_max_path->adocap_maxdevice)) {
		DBG_ERR("Failed to read adocap_maxdevice\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "adoout_maxdevice", &p_max_path->adoout_maxdevice)) {
		DBG_ERR("Failed to read adoout_maxdevice\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "adoenc_maxpath", &p_max_path->adoenc_maxpath)) {
		DBG_ERR("Failed to read adoenc_maxpath\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "adodec_maxpath", &p_max_path->adodec_maxpath)) {
		DBG_ERR("Failed to read adodec_maxpath\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "gfx_maxjob", &p_max_path->gfx_maxjob)) {
		DBG_ERR("Failed to read gfx_maxjob\r\n");
		return -1;
	}
	if (of_property_read_u32(dt_node, "stamp_maximg", &p_max_path->stamp_maximg)) {
		DBG_ERR("Failed to read stamp_maximg\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoprc_maxstamp", p_max_path->vdoprc_maxstamp, 2)) {
		DBG_ERR("Failed to read vdoprc_maxstamp\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoprc_maxmask", p_max_path->vdoprc_maxmask, 2)) {
		DBG_ERR("Failed to read vdoprc_maxmask\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoenc_maxstamp", p_max_path->vdoenc_maxstamp, 2)) {
		DBG_ERR("Failed to read vdoenc_maxstamp\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoenc_maxmask", p_max_path->vdoenc_maxmask, 2)) {
		DBG_ERR("Failed to read vdoenc_maxmask\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoout_maxstamp", p_max_path->vdoout_maxstamp, 2)) {
		DBG_ERR("Failed to read vdoout_maxstamp\r\n");
		return -1;
	}
	if (of_property_read_u32_array(dt_node, "vdoout_maxmask", p_max_path->vdoout_maxmask, 2)) {
		DBG_ERR("Failed to read vdoout_maxmask\r\n");
		return -1;
	}
	#if 0
	DBG_DUMP("vdocap_active_list = %d %d %d %d %d %d %d %d\r\n", p_max_path->vdocap_active_list[0], p_max_path->vdocap_active_list[1],
		      p_max_path->vdocap_active_list[2], p_max_path->vdocap_active_list[3], p_max_path->vdocap_active_list[4],
		      p_max_path->vdocap_active_list[5], p_max_path->vdocap_active_list[6], p_max_path->vdocap_active_list[7]);
	DBG_DUMP("vdoprc_maxdevice = %d\r\n", p_max_path->vdoprc_maxdevice);
	DBG_DUMP("vdoenc_maxpath = %d\r\n", p_max_path->vdoenc_maxpath);
	DBG_DUMP("vdodec_maxpath = %d\r\n", p_max_path->vdodec_maxpath);
	DBG_DUMP("vdoout_maxdevice = %d\r\n", p_max_path->vdoout_maxdevice);
	DBG_DUMP("adocap_maxdevice = %d\r\n", p_max_path->adocap_maxdevice);
	DBG_DUMP("adoout_maxdevice = %d\r\n", p_max_path->adoout_maxdevice);
	DBG_DUMP("adoenc_maxpath = %d\r\n", p_max_path->adoenc_maxpath);
	DBG_DUMP("adodec_maxpath = %d\r\n", p_max_path->adodec_maxpath);
	DBG_DUMP("gfx_maxjob = %d\r\n", p_max_path->gfx_maxjob);
	DBG_DUMP("stamp_maximg = %d\r\n", p_max_path->stamp_maximg);
	DBG_DUMP("vdoprc_maxstamp = %d %d\r\n", p_max_path->vdoprc_maxstamp[0], p_max_path->vdoprc_maxstamp[1]);
	DBG_DUMP("vdoprc_maxmask = %d %d\r\n", p_max_path->vdoprc_maxmask[0], p_max_path->vdoprc_maxmask[1]);
	DBG_DUMP("vdoenc_maxstamp = %d %d\r\n", p_max_path->vdoenc_maxstamp[0], p_max_path->vdoenc_maxstamp[1]);
	DBG_DUMP("vdoenc_maxmask = %d %d\r\n", p_max_path->vdoenc_maxmask[0], p_max_path->vdoenc_maxmask[1]);
	DBG_DUMP("vdoout_maxstamp = %d %d\r\n", p_max_path->vdoout_maxstamp[0], p_max_path->vdoout_maxstamp[1]);
	DBG_DUMP("vdoout_maxmask = %d %d\r\n", p_max_path->vdoout_maxmask[0], p_max_path->vdoout_maxmask[1]);
	#endif
	return 0;
}


int isf_flow_drv_ioctl(unsigned char minor, PMODULE_INFO pmodule_info, unsigned int cmd_id, unsigned long arg)
{
	int ret = 0;
	int __user *argp = (int __user *)arg;

	//DBG_IND("IF-%d cmd:%x\n", minor, cmd_id);

	switch (cmd_id) {

	case ISF_FLOW_CMD_INIT: {
			ISF_FLOW_IOCTL_CMD ctrl_cmd;
			ISF_FLOW_IOCTL_CMD *p_cmd  = 0;
			p_cmd  = &ctrl_cmd;

			DBG_IND("ioctl INIT\r\n");

			p_cmd->rv = isf_init(minor, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_CMD)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_CMD));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		}
		break;

	case ISF_FLOW_CMD_UNINIT: {
			ISF_FLOW_IOCTL_CMD ctrl_cmd;
			ISF_FLOW_IOCTL_CMD *p_cmd  = 0;
			p_cmd  = &ctrl_cmd;

			DBG_IND("ioctl UNINIT\r\n");

			p_cmd->rv = isf_exit(minor, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_CMD)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_CMD));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		}
		break;

	case ISF_FLOW_CMD_CMD: {
			ISF_FLOW_IOCTL_CMD ctrl_cmd;
			ISF_FLOW_IOCTL_CMD *p_cmd  = 0;
			p_cmd  = &ctrl_cmd;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_CMD)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_CMD));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl DO_COMMAND(cmd = %08x, p0 = %08x, p1 = %08x, p2 = %08x)\r\n",
				p_cmd->cmd, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			p_cmd->rv = isf_cmd(minor, p_cmd->cmd, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_CMD)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_CMD));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		}
		break;

	case ISF_FLOW_CMD_OUT_WAI: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = &pmodule_info->u_log_cmd;
			DBG_IND("ioctl OUT_WAI: open\r\n");

			p_cmd->uid = debug_log_wait();
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_OUT_LOG)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				return -EFAULT;
			}
		}
		break;

	case ISF_FLOW_CMD_OUT_SIG: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = &pmodule_info->u_log_cmd;
			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_OUT_LOG)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_OUT_LOG));
				return -EFAULT;
			}
			DBG_IND("ioctl OUT_LOG: close\r\n");

			debug_log_sig(p_cmd->uid);

		}
		break;

	case ISF_FLOW_CMD_OUT_STR: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = &pmodule_info->u_log_cmd;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_OUT_LOG)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_OUT_LOG));
				return -EFAULT;
			}

			DBG_IND("ioctl OUT_LOG: %s",
				p_cmd->str);

			debug_log_output(p_cmd->str);
			/*
			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}*/

		}
		break;

	case ISF_FLOW_CMD_SET_BIND: {
			ISF_FLOW_IOCTL_BIND_ITEM ctrl_bind;
			ISF_FLOW_IOCTL_BIND_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_bind;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_BIND_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl SET_BIND(src = %08x, dest = %08x)\r\n",
				p_cmd->src, p_cmd->dest);

			p_cmd->rv = isf_unit_set_bind(p_cmd->src, p_cmd->dest); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_GET_BIND: {
			ISF_FLOW_IOCTL_BIND_ITEM ctrl_bind;
			ISF_FLOW_IOCTL_BIND_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_bind;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_BIND_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl GET_BIND(src = %08x)\r\n",
				p_cmd->src);

			p_cmd->rv = isf_unit_get_bind(p_cmd->src, &(p_cmd->dest)); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(dest = %08x)\r\n", p_cmd->rv, p_cmd->dest);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_BIND_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_SET_STATE: {
			ISF_FLOW_IOCTL_STATE_ITEM ctrl_state;
			ISF_FLOW_IOCTL_STATE_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_state;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_STATE_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_STATE_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl SET_STATE(src = %08x, state = %08x)\r\n",
				p_cmd->src, p_cmd->state);

			p_cmd->rv = isf_unit_set_state(p_cmd->src, p_cmd->state); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_STATE_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_STATE_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_GET_STATE: {
			ISF_FLOW_IOCTL_STATE_ITEM ctrl_state;
			ISF_FLOW_IOCTL_STATE_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_state;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_STATE_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_STATE_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl GET_STATE(src = %08x)\r\n",
				p_cmd->src);

			p_cmd->rv = isf_unit_get_state(p_cmd->src, &(p_cmd->state)); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(state = %08x)\r\n", p_cmd->rv, p_cmd->state);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_STATE_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_STATE_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_SET_PARAM: {
			ISF_FLOW_IOCTL_PARAM_ITEM ctrl_param;
			ISF_FLOW_IOCTL_PARAM_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_param;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl SET_PARAM(dest = %08x, param = %08x, value = %08x, size = %08X)\r\n",
				p_cmd->dest, p_cmd->param, p_cmd->value, p_cmd->size);

			if(p_cmd->size > 0) {
				void *p_struct;
				if(p_cmd->size > 4*1024) {
					DBG_ERR("p_cmd size > 4K!\r\n");
					return -EFAULT;
				}
				p_struct = (void*)kmalloc(p_cmd->size, GFP_KERNEL);
				if(p_struct == 0) {
					DBG_ERR("kmalloc fail\r\n");
					return -EFAULT;
				}
				if (unlikely(copy_from_user(p_struct, ((int __user *)p_cmd->value), p_cmd->size))) {
					DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_struct, p_cmd->value, p_cmd->size);
					kfree(p_struct);
					//p_struct = NULL;
					return -EFAULT;
				}
				p_cmd->rv = isf_unit_set_struct(p_cmd->dest, p_cmd->param, (UINT32*)p_struct, p_cmd->size);
				kfree(p_struct);
				//p_struct = NULL;
			} else {
				p_cmd->rv = isf_unit_set_param(p_cmd->dest, p_cmd->param, p_cmd->value);
			}

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_GET_PARAM: {
			ISF_FLOW_IOCTL_PARAM_ITEM ctrl_param;
			ISF_FLOW_IOCTL_PARAM_ITEM *p_cmd  = 0;
			p_cmd  = &ctrl_param;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl GET_PARAM(dest = %08x, param = %08x, size = %08X)\r\n",
				p_cmd->dest, p_cmd->param, p_cmd->size);

			{
				if(p_cmd->size > 0) {
					void *p_struct;
					if(p_cmd->size > 4*1024) {
						DBG_ERR("p_cmd size > 4K!\r\n");
						return -EFAULT;
					}
					p_struct = (void*)kmalloc(p_cmd->size, GFP_KERNEL);
					if(p_struct == 0) {
						DBG_ERR("kmalloc fail\r\n");
						return -EFAULT;
					}
					p_cmd->rv = isf_unit_get_struct(p_cmd->dest, p_cmd->param, p_struct, p_cmd->size);
					if (unlikely(copy_to_user(((int __user *)p_cmd->value), p_struct, p_cmd->size))) {
						DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", p_cmd->value, (UINT32)p_struct, p_cmd->size);
						kfree(p_struct);
						//p_struct = NULL;
						return -EFAULT;
					}
					kfree(p_struct);
					//p_struct = NULL;
				} else {
					p_cmd->rv = isf_unit_get_param(p_cmd->dest, p_cmd->param, &p_cmd->value);
				}
			}

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(value = %08x)\r\n", p_cmd->rv, p_cmd->value);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_PARAM_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_NEW_DATA: {
		#if 0
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM new_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &new_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl NEW_DATA(size = %08x, p_data = %08x)\r\n",
				p_cmd->size, (UINT32)p_cmd->p_data);

			p_cmd->rv = isf_unit_new_data(p_cmd->size, &this_data);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
			if (unlikely(copy_to_user((ISF_DATA*)p_cmd->p_data, &this_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd->p_data, (UINT32)&this_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		#endif

		}
		break;

	case ISF_FLOW_CMD_ADD_DATA: {
		#if 0
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM add_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &add_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}
			if (unlikely(copy_from_user(&this_data, (ISF_DATA*)p_cmd->p_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)&this_data, (UINT32)p_cmd->p_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl ADD_DATA(p_data = %08x)\r\n",
				(UINT32)p_cmd->p_data);

			p_cmd->rv = isf_unit_add_data(&this_data);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
			if (unlikely(copy_to_user((ISF_DATA*)p_cmd->p_data, &this_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd->p_data, (UINT32)&this_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		#endif
		}
		break;

	case ISF_FLOW_CMD_RELEASE_DATA: {
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM rel_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &rel_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}
			if (unlikely(copy_from_user(&this_data, (ISF_DATA*)p_cmd->p_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)&this_data, (UINT32)p_cmd->p_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl RELEASE_DATA(dest = %08x, p_data = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data);

			p_cmd->rv = isf_unit_release_data(p_cmd->dest, &this_data);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_PUSH_DATA: {
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM push_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &push_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}
			if (unlikely(copy_from_user(&this_data, (ISF_DATA*)p_cmd->p_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)&this_data, (UINT32)p_cmd->p_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl PUSH_DATA(dest = %08x, p_data = %08x, async = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data, p_cmd->async);

			p_cmd->rv = isf_unit_push_data(p_cmd->dest, &this_data, p_cmd->async);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_PULL_DATA: {
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM pull_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &pull_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl PULL_DATA(dest = %08x, p_data = %08x, async = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data, p_cmd->async);
			
			p_cmd->rv = isf_unit_pull_data(p_cmd->dest, &this_data, p_cmd->async);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
			if (unlikely(copy_to_user((ISF_DATA*)p_cmd->p_data, &this_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd->p_data, (UINT32)&this_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}

		}
		break;

	case ISF_FLOW_CMD_NOTIFY_DATA: {
		#if 0
			ISF_DATA this_data = {0};
			ISF_FLOW_IOCTL_DATA_ITEM nty_data;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = &nty_data;

			if (unlikely(copy_from_user(p_cmd, argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)p_cmd, (UINT32)argp, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}
			if (unlikely(copy_from_user(&this_data, (ISF_DATA*)p_cmd->p_data, sizeof(ISF_DATA)))) {
				DBG_ERR("copy_from_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)&this_data, (UINT32)p_cmd->p_data, sizeof(ISF_DATA));
				p_cmd->rv = ISF_ERR_COPY_FROM_USER;
				return -EFAULT;
			}

			DBG_IND("ioctl NOTIFY_DATA(dest = %08x, p_data = %08x, async = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data, p_cmd->async);

			p_cmd->rv = isf_unit_notify_data(p_cmd->dest, &this_data, p_cmd->async);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

			//ret = -EAGAIN;
			//ret = -1;
			if (unlikely(copy_to_user(argp, p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM)))) {
				DBG_ERR("copy_to_user(0x%X, 0x%X, 0x%X)\r\n", (UINT32)argp, (UINT32)p_cmd, sizeof(ISF_FLOW_IOCTL_DATA_ITEM));
				p_cmd->rv = ISF_ERR_COPY_TO_USER;
				return -EFAULT;
			}
		#endif
		}
		break;
	case ISF_FLOW_CMD_GET_TIMESTAMP: {
			ISF_FLOW_IOCTL_GET_TIMESTAMP    msg = {0};

			msg.timestamp = hwclock_get_longcounter();
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				DBG_ERR("copy_to_user\r\n");
				return -EFAULT;
			}
		}
		break;

	case ISF_FLOW_CMD_GET_MAX_PATH: {
			ISF_FLOW_IOCTL_GET_MAX_PATH msg = {0};

			isf_flow_drv_parse_maxpath_from_dts_p(&msg);
			if (unlikely(copy_to_user(argp, &msg, sizeof(msg)))) {
				DBG_ERR("copy_to_user\r\n");
				return -EFAULT;
			}
		}
		break;

	}

	return ret;
}
