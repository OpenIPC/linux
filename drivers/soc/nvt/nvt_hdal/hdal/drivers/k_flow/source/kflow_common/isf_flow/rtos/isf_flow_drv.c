#include "isf_flow_drv.h"
#include "isf_flow_ioctl.h"
#include "../isf_flow_api.h"
#include "comm/hwclock.h"
#include "isf_flow_dbg.h"
#include "../isf_flow_int.h"
#include <libfdt.h>
#include <compiler.h>
#include <plat/rtosfdt.h>

#define EFAULT 14

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int isf_flow_open (char* file, int flag)
{
	isf_flow_install_id();
	isf_init(0, 0, 0, 0);
	return 1;
}

int isf_flow_close (int fd)
{
	isf_exit(0, 0, 0, 0);
	isf_flow_uninstall_id();
	return 0;
}

static int isf_flow_drv_parse_maxpath_from_dts_p(ISF_FLOW_IOCTL_GET_MAX_PATH *p_max_path)
{
	unsigned char *p_fdt = (unsigned char *)fdt_get_base();
	int len, i;
	const void *nodep;  /* property node pointer */
	unsigned int *p_data;

	if (p_fdt== NULL) {
		printf("p_fdt is NULL. \n");
		return -1;
	}

	// read /hdal-memory/media
	int nodeoffset = fdt_path_offset(p_fdt, "/hdal-maxpath-cfg");
	if (nodeoffset < 0) {
		printf("failed to offset for  /hdal-maxpath-cfg = %d \n", nodeoffset);
	} else {
		printf("offset for  /hdal-maxpath-cfg = %d \n", nodeoffset);
	}
	nodep = fdt_getprop(p_fdt, nodeoffset, "vdocap_active_list", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdocap_active_list\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	for (i = 0; i < 8; i++) {
		p_max_path->vdocap_active_list[i] = be32_to_cpu(p_data[i]);
	}

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoprc_maxdevice", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoprc_maxdevice\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoprc_maxdevice = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoenc_maxpath", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoenc_maxpath\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoenc_maxpath = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdodec_maxpath", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdodec_maxpath\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdodec_maxpath = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoout_maxdevice", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoout_maxdevice\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoout_maxdevice = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "adocap_maxdevice", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read adocap_maxdevice\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->adocap_maxdevice = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "adoout_maxdevice", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read adoout_maxdevice\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->adoout_maxdevice = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "adoenc_maxpath", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read adoenc_maxpath\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->adoenc_maxpath = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "adodec_maxpath", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read adodec_maxpath\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->adodec_maxpath = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "gfx_maxjob", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read gfx_maxjob\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->gfx_maxjob = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "stamp_maximg", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read stamp_maximg\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->stamp_maximg = be32_to_cpu(p_data[0]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoprc_maxstamp", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoprc_maxstamp\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoprc_maxstamp[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoprc_maxstamp[1] = be32_to_cpu(p_data[1]);

		nodep = fdt_getprop(p_fdt, nodeoffset, "vdoprc_maxmask", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoprc_maxmask\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoprc_maxmask[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoprc_maxmask[1] = be32_to_cpu(p_data[1]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoenc_maxstamp", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoenc_maxstamp\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoenc_maxstamp[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoenc_maxstamp[1] = be32_to_cpu(p_data[1]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoenc_maxmask", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoenc_maxmask\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoenc_maxmask[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoenc_maxmask[1] = be32_to_cpu(p_data[1]);

		nodep = fdt_getprop(p_fdt, nodeoffset, "vdoout_maxstamp", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoout_maxstamp\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoout_maxstamp[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoout_maxstamp[1] = be32_to_cpu(p_data[1]);

	nodep = fdt_getprop(p_fdt, nodeoffset, "vdoout_maxmask", &len);
	if (len == 0 || nodep == NULL) {
		DBG_ERR("Failed to read vdoout_maxmask\r\n");
		return -1;
	}
	p_data = (unsigned int *)nodep;
	p_max_path->vdoout_maxmask[0] = be32_to_cpu(p_data[0]);
	p_max_path->vdoout_maxmask[1] = be32_to_cpu(p_data[1]);

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

/*
static MODULE_INFO module_info = {0};
static PMODULE_INFO pmodule_info = &module_info;
*/

int isf_flow_ioctl (int fd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {

	case ISF_FLOW_CMD_DO_CLOSE: {
			isf_exit(0, 0, 0, 0);
		}
		break;

	case ISF_FLOW_CMD_OUT_WAI: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_OUT_LOG *)arg;
			DBG_IND("ioctl OUT_WAI: open\r\n");

			p_cmd->uid = debug_log_wait();
		}
		break;

	case ISF_FLOW_CMD_OUT_SIG: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_OUT_LOG *)arg;
			DBG_IND("ioctl OUT_LOG: close\r\n");

			debug_log_sig(p_cmd->uid);
		}
		break;

	case ISF_FLOW_CMD_OUT_STR: {
			ISF_FLOW_IOCTL_OUT_LOG *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_OUT_LOG *)arg;

			DBG_IND("ioctl OUT_LOG: %s",
				p_cmd->str);

			debug_log_output(p_cmd->str);
		}
		break;

	case ISF_FLOW_CMD_SET_BIND: {
			ISF_FLOW_IOCTL_BIND_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_BIND_ITEM *)arg;

			DBG_IND("ioctl SET_BIND(src = %08x, dest = %08x)\r\n",
				p_cmd->src, p_cmd->dest);

			p_cmd->rv = isf_unit_set_bind(p_cmd->src, p_cmd->dest); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
		}
		break;

	case ISF_FLOW_CMD_GET_BIND: {
			ISF_FLOW_IOCTL_BIND_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_BIND_ITEM *)arg;

			DBG_IND("ioctl GET_BIND(src = %08x)\r\n",
				p_cmd->src);

			p_cmd->rv = isf_unit_get_bind(p_cmd->src, (UINT32*)&(p_cmd->dest)); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(dest = %08x)\r\n", p_cmd->rv, p_cmd->dest);
		}
		break;

	case ISF_FLOW_CMD_SET_STATE: {
			ISF_FLOW_IOCTL_STATE_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_STATE_ITEM *)arg;

			DBG_IND("ioctl SET_STATE(src = %08x, state = %08x)\r\n",
				p_cmd->src, p_cmd->state);

			p_cmd->rv = isf_unit_set_state(p_cmd->src, p_cmd->state); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
		}
		break;

	case ISF_FLOW_CMD_GET_STATE: {
			ISF_FLOW_IOCTL_STATE_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_STATE_ITEM *)arg;

			DBG_IND("ioctl GET_STATE(src = %08x)\r\n",
				p_cmd->src);

			p_cmd->rv = isf_unit_get_state(p_cmd->src, (UINT32*)&(p_cmd->state)); //NOTE: p_cmd->sid is not used

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(state = %08x)\r\n", p_cmd->rv, p_cmd->state);
		}
		break;

	case ISF_FLOW_CMD_SET_PARAM: {
			ISF_FLOW_IOCTL_PARAM_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_PARAM_ITEM *)arg;

			DBG_IND("ioctl SET_PARAM(dest = %08x, param = %08x, value = %08x, size = %08X)\r\n",
				p_cmd->dest, p_cmd->param, p_cmd->value, p_cmd->size);

			if(p_cmd->size > 0) {
				p_cmd->rv = isf_unit_set_struct(p_cmd->dest, p_cmd->param, (UINT32*)(p_cmd->value), p_cmd->size);
			} else {
				p_cmd->rv = isf_unit_set_param(p_cmd->dest, p_cmd->param, p_cmd->value);
			}

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
		}
		break;

	case ISF_FLOW_CMD_GET_PARAM: {
			ISF_FLOW_IOCTL_PARAM_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_PARAM_ITEM *)arg;

			DBG_IND("ioctl GET_PARAM(dest = %08x, param = %08x, size = %08X)\r\n",
				p_cmd->dest, p_cmd->param, p_cmd->size);

			{
				if(p_cmd->size > 0) {
					p_cmd->rv = isf_unit_get_struct(p_cmd->dest, p_cmd->param, (UINT32*)(p_cmd->value), p_cmd->size);
				} else {
					p_cmd->rv = isf_unit_get_param(p_cmd->dest, p_cmd->param, (UINT32*)&(p_cmd->value));
				}
			}

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)(value = %08x)\r\n", p_cmd->rv, p_cmd->value);
		}
		break;

	case ISF_FLOW_CMD_RELEASE_DATA: {
			ISF_DATA* p_data = 0;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_DATA_ITEM *)arg;
			p_data  = (ISF_DATA*)p_cmd->p_data;

			DBG_IND("ioctl RELEASE_DATA(dest = %08x, p_data = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data);

			p_cmd->rv = isf_unit_release_data(p_cmd->dest, p_data);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
		}
		break;

	case ISF_FLOW_CMD_PUSH_DATA: {
			ISF_DATA* p_data = 0;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_DATA_ITEM *)arg;
			p_data  = (ISF_DATA*)p_cmd->p_data;

			DBG_IND("ioctl PUSH_DATA(dest = %08x, p_data = %08x, async = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data, p_cmd->async);

			p_cmd->rv = isf_unit_push_data(p_cmd->dest, p_data, p_cmd->async);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

		}
		break;

	case ISF_FLOW_CMD_PULL_DATA: {
			ISF_DATA* p_data = 0;
			ISF_FLOW_IOCTL_DATA_ITEM *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_DATA_ITEM *)arg;
			p_data  = (ISF_DATA*)p_cmd->p_data;

			DBG_IND("ioctl PULL_DATA(dest = %08x, p_data = %08x, async = %08x)\r\n",
				p_cmd->dest, (UINT32)p_cmd->p_data, p_cmd->async);

			p_cmd->rv = isf_unit_pull_data(p_cmd->dest, p_data, p_cmd->async);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);
		}
		break;

	case ISF_FLOW_CMD_GET_TIMESTAMP: {
			ISF_FLOW_IOCTL_GET_TIMESTAMP *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_GET_TIMESTAMP *)arg;

			p_cmd->timestamp = hwclock_get_longcounter();
		}
		break;

	case ISF_FLOW_CMD_CMD: {
			ISF_FLOW_IOCTL_CMD *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_CMD *)arg;

			DBG_IND("ioctl DO_COMMAND(cmd = %08x, p0 = %08x, p1 = %08x, p2 = %08x)\r\n",
				p_cmd->cmd, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			p_cmd->rv = isf_cmd(0, p_cmd->cmd, p_cmd->p0, p_cmd->p1, p_cmd->p2);

			if(p_cmd->rv != ISF_OK) {
				ret = -EFAULT;
			}
			DBG_IND("- (rv = %08x)\r\n", p_cmd->rv);

		}
		break;
	case ISF_FLOW_CMD_GET_MAX_PATH: {
			ISF_FLOW_IOCTL_GET_MAX_PATH *p_cmd  = 0;
			p_cmd  = (ISF_FLOW_IOCTL_GET_MAX_PATH *)arg;

			return isf_flow_drv_parse_maxpath_from_dts_p(p_cmd);
		}
		break;

	}

	return ret;
}
