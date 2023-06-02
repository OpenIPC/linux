#if defined(__FREERTOS)
#include <compiler.h>
#include <rtosfdt.h>
#endif

#include "kwrap/type.h"
#include "kflow_videocapture/ctl_sen.h"

#include "sen_cfg.h"
#include "sen_common.h"

#define SEN_COMMON_SHOW_MSG 0

UINT32 sen_common_calc_log_2(UINT32 devider, UINT32 devident)
{
	UINT32 ratio;
	UINT32 idx, ret;
	static UINT32 log_tbl[20] = {104, 107, 112, 115, 119, 123, 128, 131, 137, 141, 147, 152, 157, 162, 168, 174, 181, 187, 193, 200};

	if (devident == 0) {
		return 1;
	}

	ret = 0;
	ratio = (devider * 100) / devident;
	while (ratio >= 200) {
		ratio /= 2;
		ret += 100;
	}

	for (idx = 0; idx < 20; idx++) {
		if (ratio < log_tbl[idx]) {
			break;
		}
	}

	return ret + idx * 5;
}

CFG_FILE_FMT *sen_common_open_cfg(INT8 *pfile_name)
{
	CFG_FILE_FMT *pcfg_file;

	pcfg_file = sen_cfg_open(pfile_name, O_RDONLY);

	return pcfg_file;
}

void sen_common_close_cfg(CFG_FILE_FMT *pcfg_file)
{
	sen_cfg_close(pcfg_file);
}

#if defined(__FREERTOS)
BOOL sen_common_check_compatible(CHAR *compatible)
{
	int offset = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();

	if (fdt_node_offset_by_compatible(pfdt_addr, offset, compatible) > 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void sen_common_load_cfg_preset_compatible(CHAR *compatible, void *param)
{
	INT32 path = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();
	INT32 offset = 0;
	CHAR node_path[32] = {0};
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	SEN_PRESET *sen_preset = (SEN_PRESET *)param;

	while ((offset = fdt_node_offset_by_compatible(pfdt_addr, offset, compatible)) > 0) {
		if (fdt_get_path(pfdt_addr, offset, node_path, sizeof(node_path)) < 0) {
			DBG_WRN("failed to get path from compatible: %s\n", compatible);
			return;
		}
		node_ofst = fdt_path_offset(pfdt_addr, node_path);
		if (node_ofst >= 0) {
			sprintf(keystr, "path");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				path = be32_to_cpu(*pdata);
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("path = %d \r\n", path);
				#endif
			} else {
				vk_printk("%s\%s not exist \n", node_path,keystr);
			}
		} else {
			vk_printk("%s not exist \n", node_path);
		}

		sprintf(sub_node_name, "%s/PRESET", node_path);
		node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);
		if (node_ofst >= 0) {
			sprintf(keystr, "expt_time");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_preset[path].expt_time = be32_to_cpu(*pdata);
			}
	
			sprintf(keystr, "gain_ratio");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_preset[path].gain_ratio = be32_to_cpu(*pdata);
			}
		} else {
			vk_printk("%s not exist \n", sub_node_name);
		}
	}
}

#else
void sen_common_load_cfg_preset_compatible(struct device_node *of_node, void *param)
{
	UINT32 path = 0;
	CHAR keystr[32];
	struct device_node *child;
	SEN_PRESET *sen_preset = (SEN_PRESET *)param;

	sprintf((char *)keystr, "path");
	if (of_property_read_u32(of_node, keystr, &path) != 0) {
		DBG_ERR("cannot find %s \r\n", keystr);
		return;
	}
	#if (SEN_COMMON_SHOW_MSG == 1)
	vk_printk("%s = %d \r\n", keystr, path);
	#endif

	for_each_child_of_node(of_node, child) {
		sprintf((char *)keystr, "expt_time");
		if (of_property_read_u32(child, keystr, &sen_preset[path].expt_time) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_preset[path].expt_time);
			#endif
		}

		sprintf((char *)keystr, "gain_ratio");
		if (of_property_read_u32(child, keystr, &sen_preset[path].gain_ratio) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_preset[path].gain_ratio);
			#endif
		}
	}
}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_direction_compatible(CHAR *compatible, void *param)
{
	INT32 path = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();
	INT32 offset = 0;
	CHAR node_path[32] = {0};
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	SEN_DIRECTION *sen_direction = (SEN_DIRECTION *)param;

	while ((offset = fdt_node_offset_by_compatible(pfdt_addr, offset, compatible)) > 0) {
		if (fdt_get_path(pfdt_addr, offset, node_path, sizeof(node_path)) < 0) {
			DBG_WRN("failed to get path from compatible: %s\n", compatible);
			return;
		}
		node_ofst = fdt_path_offset(pfdt_addr, node_path);
		if (node_ofst >= 0) {
			sprintf(keystr, "path");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				path = be32_to_cpu(*pdata);
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("path = %d \r\n", path);
				#endif
			} else {
				vk_printk("%s\%s not exist \n", node_path,keystr);
			}
		} else {
			vk_printk("%s not exist \n", node_path);
		}

		sprintf(sub_node_name, "%s/DIRECTION", node_path);
		node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);
		if (node_ofst >= 0) {
			sprintf(keystr, "mirror");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_direction[path].mirror = be32_to_cpu(*pdata);
			}
	
			sprintf(keystr, "flip");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_direction[path].flip = be32_to_cpu(*pdata);
			}
		} else {
			vk_printk("%s not exist \n", sub_node_name);
		}
	}
}

#else
void sen_common_load_cfg_direction_compatible(struct device_node *of_node, void *param)
{
	UINT32 path = 0;
	CHAR keystr[32];
	struct device_node *child;
	SEN_DIRECTION *sen_direction = (SEN_DIRECTION *)param;

	sprintf((char *)keystr, "path");
	if (of_property_read_u32(of_node, keystr, &path) != 0) {
		DBG_ERR("cannot find %s \r\n", keystr);
		return;
	}
	#if (SEN_COMMON_SHOW_MSG == 1)
	vk_printk("%s = %d \r\n", keystr, path);
	#endif

	for_each_child_of_node(of_node, child) {
		sprintf((char *)keystr, "mirror");
		if (of_property_read_u32(child, keystr, &sen_direction[path].mirror) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_direction[path].mirror);
			#endif
		}

		sprintf((char *)keystr, "flip");
		if (of_property_read_u32(child, keystr, &sen_direction[path].flip) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_direction[path].flip);
			#endif
		}
	}
}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_power_compatible(CHAR *compatible, void *param)
{
	INT32 path = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();
	INT32 offset = 0;
	CHAR node_path[32] = {0};
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	SEN_POWER *sen_power = (SEN_POWER *)param;

	while ((offset = fdt_node_offset_by_compatible(pfdt_addr, offset, compatible)) > 0) {
		if (fdt_get_path(pfdt_addr, offset, node_path, sizeof(node_path)) < 0) {
			DBG_WRN("failed to get path from compatible: %s\n", compatible);
			return;
		}
		node_ofst = fdt_path_offset(pfdt_addr, node_path);
		if (node_ofst >= 0) {
			sprintf(keystr, "path");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				path = be32_to_cpu(*pdata);
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("path = %d \r\n", path);
				#endif
			} else {
				vk_printk("%s\%s not exist \n", node_path,keystr);
			}
		} else {
			vk_printk("%s not exist \n", node_path);
		}

		sprintf(sub_node_name, "%s/POWER", node_path);
		node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);
		if (node_ofst >= 0) {
			sprintf(keystr, "mclk");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[path].mclk = be32_to_cpu(*pdata);
			}
	
			sprintf(keystr, "pwdn_pin");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[path].pwdn_pin = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "rst_pin");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[path].rst_pin = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "rst_time");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[path].rst_time = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "stable_time");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[path].stable_time = be32_to_cpu(*pdata);
			}
		} else {
			vk_printk("%s not exist \n", sub_node_name);
		}
	}
}

#else
void sen_common_load_cfg_power_compatible(struct device_node *of_node, void *param)
{
	UINT32 path = 0;
	CHAR keystr[32];
	struct device_node *child;
	SEN_POWER *sen_direction = (SEN_POWER *)param;

	sprintf((char *)keystr, "path");
	if (of_property_read_u32(of_node, keystr, &path) != 0) {
		DBG_ERR("cannot find %s \r\n", keystr);
		return;
	}
	#if (SEN_COMMON_SHOW_MSG == 1)
	vk_printk("%s = %d \r\n", keystr, path);
	#endif

	for_each_child_of_node(of_node, child) {
		sprintf((char *)keystr, "mclk");
		if (of_property_read_u32(child, keystr, &sen_direction[path].mclk) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_direction[path].mclk);
			#endif
		}

		sprintf((char *)keystr, "pwdn_pin");
		if (of_property_read_u32(child, keystr, &sen_direction[path].pwdn_pin) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = 0x%X \r\n", keystr, sen_direction[path].pwdn_pin);
			#endif
		}

		sprintf((char *)keystr, "rst_pin");
		if (of_property_read_u32(child, keystr, &sen_direction[path].rst_pin) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = 0x%X \r\n", keystr, sen_direction[path].rst_pin);
			#endif
		}

		sprintf((char *)keystr, "rst_time");
		if (of_property_read_u32(child, keystr, &sen_direction[path].rst_time) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_direction[path].rst_time);
			#endif
		}

		sprintf((char *)keystr, "stable_time");
		if (of_property_read_u32(child, keystr, &sen_direction[path].stable_time) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_direction[path].stable_time);
			#endif
		}
	}
}

#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_i2c_compatible(CHAR *compatible, void *param)
{
	INT32 path = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();
	INT32 offset = 0;
	CHAR node_path[32] = {0};
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	SEN_I2C *sen_i2c = (SEN_I2C *)param;

	while ((offset = fdt_node_offset_by_compatible(pfdt_addr, offset, compatible)) > 0) {
		if (fdt_get_path(pfdt_addr, offset, node_path, sizeof(node_path)) < 0) {
			DBG_WRN("failed to get path from compatible: %s\n", compatible);
			return;
		}
		node_ofst = fdt_path_offset(pfdt_addr, node_path);
		if (node_ofst >= 0) {
			sprintf(keystr, "path");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				path = be32_to_cpu(*pdata);
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("path = %d \r\n", path);
				#endif
			} else {
				vk_printk("%s\%s not exist \n", node_path,keystr);
			}
		} else {
			vk_printk("%s not exist \n", node_path);
		}

		sprintf(sub_node_name, "%s/I2C", node_path);
		node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);
		if (node_ofst >= 0) {
			sprintf(keystr, "i2c_id");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_i2c[path].id = be32_to_cpu(*pdata);
			}
	
			sprintf(keystr, "i2c_addr");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_i2c[path].addr = be32_to_cpu(*pdata);
			}
		} else {
			vk_printk("%s not exist \n", sub_node_name);
		}
	}
}

#else
void sen_common_load_cfg_i2c_compatible(struct device_node *of_node, void *param)
{
	UINT32 path = 0;
	CHAR keystr[32];
	struct device_node *child;
	SEN_I2C *sen_i2c = (SEN_I2C *)param;

	sprintf((char *)keystr, "path");
	if (of_property_read_u32(of_node, keystr, &path) != 0) {
		DBG_ERR("cannot find %s \r\n", keystr);
		return;
	}
	#if (SEN_COMMON_SHOW_MSG == 1)
	vk_printk("%s = %d \r\n", keystr, path);
	#endif

	for_each_child_of_node(of_node, child) {
		sprintf((char *)keystr, "i2c_id");
		if (of_property_read_u32(child, keystr, &sen_i2c[path].id) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = %d \r\n", keystr, sen_i2c[path].id);
			#endif
		}

		sprintf((char *)keystr, "i2c_addr");
		if (of_property_read_u32(child, keystr, &sen_i2c[path].addr) != 0) {
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("%s = 0x%X \r\n", keystr, sen_i2c[path].addr);
			#endif
		}
	}
}

#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_map(UINT8 *pfdt_addr, CHAR *node_path, void *param)
{
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	UINT32 i;
	UINT32 *sen_map = (UINT32 *)param;

	sprintf(sub_node_name, "%s/MAP", node_path);
	node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);

	if (node_ofst >= 0) {
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			sprintf(keystr, "path_%u", (unsigned int)(i+1));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				if (be32_to_cpu(*pdata) == 1) {
					*sen_map |= (1<<i);
				}
			}
		}
	} else {
		vk_printk("%s not exist \n", sub_node_name);
	}
}

#else
void sen_common_load_cfg_map(CFG_FILE_FMT *pcfg_file, void *param)
{
	INT8 cfg_buf[LINE_LEN];
	INT8 keystr[16];
	UINT32 i;
	UINT32 *sen_map = (UINT32 *)param;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		sprintf((char *)keystr, "path_%u", (unsigned int)(i+1));
		if (sen_cfg_get_field_str("MAP", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if (CFG_STR2UINT((char *)cfg_buf) == 1) {
				*sen_map |= (1<<i);
				#if (SEN_COMMON_SHOW_MSG == 1) 
				vk_printk("path_%u = %u \n", (unsigned int)i, (unsigned int)CFG_STR2UINT((char *)cfg_buf));
				#endif
			}
		} else {
			vk_printk("path_%u not exist \n", (unsigned int)i);
		}
	}

}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_preset(UINT8 *pfdt_addr, CHAR *node_path, void *param)
{
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	UINT32 i;
	SEN_PRESET *sen_preset = (SEN_PRESET *)param;

	sprintf(sub_node_name, "%s/PRESET", node_path);
	node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);

	if (node_ofst >= 0) {
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			sprintf(keystr, "path_%u", (unsigned int)(i+1));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				if (be32_to_cpu(*pdata) == 0) {
					continue;
				}
			}

			sprintf(keystr, "id_%u_expt_time", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_preset[i].expt_time = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_gain_ratio", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_preset[i].gain_ratio = be32_to_cpu(*pdata);
			}
		}
	} else {
		vk_printk("%s not exist \n", sub_node_name);
	}
}

#else
void sen_common_load_cfg_preset(CFG_FILE_FMT *pcfg_file, void *param)
{
	INT8 cfg_buf[LINE_LEN];
	INT8 keystr[16];
	UINT32 i;
	SEN_PRESET *sen_preset = (SEN_PRESET *)param;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		// Check if exist
		sprintf((char *)keystr, "path_%u", (unsigned int)(i+1));
		if (sen_cfg_get_field_str("MAP", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if (CFG_STR2UINT((char *)cfg_buf) == 0) {
				continue;
			}
		}

		sprintf((char *)keystr, "id_%u_expt_time", (unsigned int)i);
		if (sen_cfg_get_field_str("PRESET", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_preset[i].expt_time = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_expt_time = %u \n", (unsigned int)i, (unsigned int)sen_preset[i].expt_time);
			#endif
		} else {
			vk_printk("id_%u_expt_time not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_gain_ratio", (unsigned int)i);
		if (sen_cfg_get_field_str("PRESET", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_preset[i].gain_ratio = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_gain_ratio = %u \n", (unsigned int)i, (unsigned int)sen_preset[i].gain_ratio);
			#endif
		} else {
			vk_printk("id_%u_gain_ratio not exist \n", (unsigned int)i);
		}
	}
}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_direction(UINT8 *pfdt_addr, CHAR *node_path, void *param)
{
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	UINT32 i;
	SEN_DIRECTION *sen_direction = (SEN_DIRECTION *)param;

	sprintf(sub_node_name, "%s/DIRECTION", node_path);
	node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);

	if (node_ofst >= 0) {
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			sprintf(keystr, "path_%u", (unsigned int)(i+1));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				if (be32_to_cpu(*pdata) == 0) {
					continue;
				}
			}

			sprintf(keystr, "id_%u_mirror", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_direction[i].mirror = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_flip", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_direction[i].flip = be32_to_cpu(*pdata);
			}
		}
	} else {
		vk_printk("%s not exist \n", sub_node_name);
	}
}

#else
void sen_common_load_cfg_direction(CFG_FILE_FMT *pcfg_file, void *param)
{
	INT8 cfg_buf[LINE_LEN];
	INT8 keystr[16];
	UINT32 i;
	SEN_DIRECTION *sen_direction = (SEN_DIRECTION *)param;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		// Check if exist
		sprintf((char *)keystr, "path_%u", (unsigned int)(i+1));
		if (sen_cfg_get_field_str("MAP", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if (CFG_STR2UINT((char *)cfg_buf) == 0) {
				continue;
			}
		}

		sprintf((char *)keystr, "id_%u_mirror", (unsigned int)i);
		if (sen_cfg_get_field_str("DIRECTION", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_direction[i].mirror = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_mirror = %u \n", (unsigned int)i, (unsigned int)sen_direction[i].mirror);
			#endif
		} else {
			vk_printk("id_%u_mirror not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_flip", (unsigned int)i);
		if (sen_cfg_get_field_str("DIRECTION", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_direction[i].flip = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_flip = %u \n", (unsigned int)i, (unsigned int)sen_direction[i].flip);
			#endif
		} else {
			vk_printk("id_%u_flip not exist \n", (unsigned int)i);
		}
	}
}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_power(UINT8 *pfdt_addr, CHAR *node_path, void *param)
{
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	UINT32 i;
	SEN_POWER *sen_power = (SEN_POWER *)param;

	sprintf(sub_node_name, "%s/POWER", node_path);
	node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);

	if (node_ofst >= 0) {
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			sprintf(keystr, "path_%u", (unsigned int)(i+1));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				if (be32_to_cpu(*pdata) == 0) {
					continue;
				}
			}

			sprintf(keystr, "id_%u_mclk", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[i].mclk = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_pwdn_pin", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[i].pwdn_pin = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_rst_pin", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[i].rst_pin = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_rst_time", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[i].rst_time = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_stable_time", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_power[i].stable_time = be32_to_cpu(*pdata);
			}
		}
	} else {
		vk_printk("%s not exist \n", sub_node_name);
	}
}

#else
void sen_common_load_cfg_power(CFG_FILE_FMT *pcfg_file, void *param)
{
	INT8 cfg_buf[LINE_LEN];
	INT8 keystr[32];
	UINT32 i;
	SEN_POWER *sen_power = (SEN_POWER *)param;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		// Check if exist
		sprintf((char *)keystr, "path_%u", (unsigned int)(i+1));
		if (sen_cfg_get_field_str("MAP", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if (CFG_STR2UINT((char *)cfg_buf) == 0) {
				continue;
			}
		}

		sprintf((char *)keystr, "id_%u_mclk", (unsigned int)i);
		if (sen_cfg_get_field_str("POWER", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_power[i].mclk = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_mclk = %u \n", (unsigned int)i, (unsigned int)sen_power[i].mclk);
			#endif
		} else {
			vk_printk("id_%u_mclk not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_pwdn_pin", (unsigned int)i);
		if (sen_cfg_get_field_str("POWER", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_power[i].pwdn_pin = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_pwdn_pin = 0x%X \n", (unsigned int)i, (unsigned int)sen_power[i].pwdn_pin);
			#endif
		} else {
			vk_printk("id_%u_pwdn_pin not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_rst_pin", (unsigned int)i);
		if (sen_cfg_get_field_str("POWER", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_power[i].rst_pin = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_rst_pin = 0x%X \n", (unsigned int)i, (unsigned int)sen_power[i].rst_pin);
			#endif
		} else {
			vk_printk("id_%u_rst_pin not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_rst_time", (unsigned int)i);
		if (sen_cfg_get_field_str("POWER", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_power[i].rst_time = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_rst_time = %u \n", (unsigned int)i, (unsigned int)sen_power[i].rst_time);
			#endif
		} else {
			vk_printk("id_%u_rst_time not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_stable_time", (unsigned int)i);
		if (sen_cfg_get_field_str("POWER", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_power[i].stable_time = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_stable_time = %u \n", (unsigned int)i, (unsigned int)sen_power[i].stable_time);
			#endif
		} else {
			vk_printk("id_%u_stable_time not exist \n", (unsigned int)i);
		}
	}
}
#endif

#if defined(__FREERTOS)
void sen_common_load_cfg_i2c(UINT8 *pfdt_addr, CHAR *node_path, void *param)
{
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64];
	CHAR keystr[32];
	UINT32 i;
	SEN_I2C *sen_i2c = (SEN_I2C *)param;

	sprintf(sub_node_name, "%s/I2C", node_path);
	node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);

	if (node_ofst >= 0) {
		for (i = 0; i < CTL_SEN_ID_MAX; i++) {
			sprintf(keystr, "path_%u", (unsigned int)(i+1));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				if (be32_to_cpu(*pdata) == 0) {
					continue;
				}
			}

			sprintf(keystr, "id_%u_i2c_id", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_i2c[i].id = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "id_%u_i2c_addr", (unsigned int)(i));
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				#if (SEN_COMMON_SHOW_MSG == 1)
				vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				#endif
				sen_i2c[i].addr = be32_to_cpu(*pdata);
			}
		}
	} else {
		vk_printk("%s not exist \n", sub_node_name);
	}
}

#else
void sen_common_load_cfg_i2c(CFG_FILE_FMT *pcfg_file, void *param)
{
	INT8 cfg_buf[LINE_LEN];
	INT8 keystr[16];
	UINT32 i;
	SEN_I2C *sen_i2c = (SEN_I2C *)param;

	for (i = 0; i < CTL_SEN_ID_MAX; i++) {
		// Check if exist
		sprintf((char *)keystr, "path_%u", (unsigned int)(i+1));
		if (sen_cfg_get_field_str("MAP", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			if (CFG_STR2UINT((char *)cfg_buf) == 0) {
				continue;
			}
		}

		sprintf((char *)keystr, "id_%u_i2c_id", (unsigned int)i);
		if (sen_cfg_get_field_str("I2C", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_i2c[i].id = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_i2c_id = %u \n", (unsigned int)i, (unsigned int)sen_i2c[i].id);
			#endif
		} else {
			vk_printk("id_%u_i2c_id not exist \n", (unsigned int)i);
		}

		sprintf((char *)keystr, "id_%u_i2c_addr", (unsigned int)i);
		if (sen_cfg_get_field_str("I2C", keystr, cfg_buf, LINE_LEN-1, pcfg_file) > 0) {
			sen_i2c[i].addr = CFG_STR2UINT((char *)cfg_buf);
			#if (SEN_COMMON_SHOW_MSG == 1)
			vk_printk("id_%u_i2c_addr = 0x%X \n", (unsigned int)i, (unsigned int)sen_i2c[i].addr);
			#endif
		} else {
			vk_printk("id_%u_i2c_addr not exist \n", (unsigned int)i);
		}
	}
}

#endif

