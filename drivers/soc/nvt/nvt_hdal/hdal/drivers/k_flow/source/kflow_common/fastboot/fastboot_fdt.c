#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <kwrap/file.h>
#include <comm/libfdt.h>
#include "fastboot_fdt.h"

#include <kwrap/debug.h>
#include <kwrap/cpu.h>
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "kdrv_builtin/vdoenc_builtin.h"
//============================================================================
// Define
//============================================================================
//============================================================================
// Global variable
//============================================================================
static char path[256] = { 0 };

//============================================================================
// Function define
//============================================================================
static int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name)
{
	int offset;
	offset = fdt_subnode_offset(fdt, parentoffset, name);
	if (offset == -FDT_ERR_NOTFOUND) {
		offset = fdt_add_subnode(fdt, parentoffset, name);
	}
	if (offset < 0) {
		DBG_ERR("%s: %s: %s\n", __func__, name, fdt_strerror(offset));
	}
	return offset;
}

static int fdt_find_or_add_node(void *fdt, const char *node_name)
{
	int i, j, nodeoffset = 0;

	if (node_name[0] != '/') {
		DBG_ERR("node name must start at '/': %s\n", node_name);
		return -1;
	}
	if (strlen(node_name) + 1 > sizeof(path)) {
		DBG_ERR("path is too long: %s\n", node_name);
		return -1;
	}
	memset(path, 0, sizeof(path));
	// for '>=' includes '/0' char
	for (i = 1, j = 0; i <= strlen(node_name); i++) {
		if (node_name[i] == '/') {
			path[j] = 0;
			j = 0;
			if ((nodeoffset = fdt_find_or_add_subnode(fdt, nodeoffset, path)) <= 0) {
				return nodeoffset;
			}
		} else {
			path[j++] = node_name[i];
		}
	}
	return fdt_find_or_add_subnode(fdt, nodeoffset, path);
}

static int update_property_u32array(void *fdt, const char *node_name, const char *property_name, unsigned int *p_u32, unsigned int u32_cnt)
{
	int i, len, er;
	unsigned int *u32_array = NULL;
	int nodeoffset = fdt_find_or_add_node(fdt, node_name);

	if (nodeoffset <= 0) {
		return nodeoffset;
	}

	len = sizeof(unsigned int) * u32_cnt;
	u32_array = (unsigned int *)kzalloc(len, GFP_KERNEL);
	if (u32_array == NULL) {
		DBG_ERR("failed to alloc u32_array\n");
		return -1;
	}
	for (i = 0; i < u32_cnt; i++) {
		u32_array[i] = cpu_to_be32(p_u32[i]);
	}

	if ((er = fdt_setprop(fdt, nodeoffset, property_name, u32_array, len)) != 0) {
		DBG_ERR("failed to fdt_setprop %s, er = %d\n", property_name, er);
		kfree(u32_array);
		return er;
	}

	kfree(u32_array);
	return 0;
}

static int update_property_u32(void *fdt, const char *node_name, const char *property_name, unsigned int val)
{
	int len, er;
	unsigned int *u32_array = NULL;
	int nodeoffset = fdt_find_or_add_node(fdt, node_name);

	if (nodeoffset <= 0) {
		return nodeoffset;
	}

	len = sizeof(unsigned int);
	val = cpu_to_be32(val);

	if ((er = fdt_setprop(fdt, nodeoffset, property_name, &val, len)) != 0) {
		DBG_ERR("failed to fdt_setprop %s, er = %d\n", property_name, er);
		kfree(u32_array);
		return er;
	}

	return 0;
}

int fastboot_fdt_update(unsigned char *fdt)
{
	unsigned int u32_array[3] = {0xA, 0xB, 0xC};

	// example for write a u32
	if (update_property_u32(fdt, "/fastboot/test", "param1", 5) != 0) {
		DBG_ERR("failed to update_property.");
	}
	// example for write u32 array
	if (update_property_u32array(fdt, "/fastboot/test", "param2", u32_array, 3) != 0) {
		DBG_ERR("failed to update_property.");
	}

	// ipp fastboot register config
	{
		char *dtsi_reg_cfg_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
			"ife_reg_cfg",
			"dce_reg_cfg",
			"ipe_reg_cfg",
			"ime_reg_cfg",
		};
		char dtsi_node[64];
		unsigned int hdl_list[8] = {0};
		unsigned int *dtsi_buffer = NULL;
		unsigned int *reg_buffer[KDRV_IPP_BUILTIN_ENG_MAX] = {NULL};
		int len;
		int arr_num;
		int eng_idx;
		int i;

		/* dtsi buffer alloc */
		len = 0;
		for (eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
			len += kdrv_ipp_builtin_get_reg_num(eng_idx);
		}
		len = len * 2 * 4;
		dtsi_buffer = (unsigned int *)kzalloc(len, GFP_KERNEL);
		if (dtsi_buffer) {
			for (eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
				if (eng_idx == 0) {
					reg_buffer[eng_idx] = dtsi_buffer;
				} else {
					reg_buffer[eng_idx] = reg_buffer[eng_idx - 1] + (kdrv_ipp_builtin_get_reg_num(eng_idx - 1) * 2);
				}
			}

			/* get register for each ipp handle */
			kdrv_ipp_builtin_get_hdal_hdl_list(hdl_list, 8);
			for (i = 0; i < 8; i++) {
				if (hdl_list[i] == 0) {
					break;
				}

				snprintf(dtsi_node, 63, "/fastboot/ipp/config-%d/register", i);
				kdrv_ipp_builtin_get_hdal_reg_dtsi(hdl_list[i], i, reg_buffer);
				for (eng_idx = KDRV_IPP_BUILTIN_IFE; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
					arr_num = kdrv_ipp_builtin_get_reg_num(eng_idx) * 2;
					if (arr_num == 0) {
						continue;
					}

					//DBG_DUMP("fastboot fdt ipp reg eng %d, array 0x%.8x(0x%.8x), arr_num %d, max_arr_byte %d\r\n",
					//			eng_idx, (unsigned int)reg_array, (unsigned int)vos_cpu_get_phy_addr((VOS_ADDR)reg_array), arr_num, len);
					if (update_property_u32array(fdt, dtsi_node, dtsi_reg_cfg_tag[eng_idx], reg_buffer[eng_idx], arr_num) != 0) {
						DBG_ERR("failed to update_property.");
					}
				}
			}

			kfree(dtsi_buffer);
		} else {
			DBG_ERR("alloc dtsi buffer(size 0x%.8x) failed\r\n", len);
		}
	}

	// venc fastboot codec type update
	{
		UINT32 dynamic_code = 0;

		VdoEnc_Builtin_GetParam(0, BUILTIN_VDOENC_INIT_PARAM_DYNAMIC_CODEC, &dynamic_code);
		if (update_property_u32(fdt, "/fastboot/venc", "codectype", dynamic_code) != 0) {
			DBG_ERR("failed to update_property.");
		}
	}

	return 0;
}
