
#if defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#include <compiler.h>
#include "plat/gpio.h"
#else
#include <linux/slab.h>
#endif
#include <kwrap/file.h>
#include <libfdt.h>
#include "kflow_videocapture/ctl_sen.h"
#include "ad_info_parser_int.h"
#include "ad_dbg_int.h"

#define AD_PARSE_TAB_END 0xffffffff
#define AD_CFG_PATH_LEN 80
#define MAX_VAL_NUM 10

//root path
#define AD_PATH_ROOT "/sensor/sen_cfg/"
#define AD_PATH_IDX "/idx@"

typedef enum {
	AD_LIST_TYPE_NONE, //don't care
	AD_LIST_TYPE_NECESSARY, // not option (necessary parameters)
	ENUM_DUMMY4WORD(AD_LIST_TYPE)
} AD_LIST_TYPE;

typedef INT32(*AD_PARSE_FP)(void *, UINT32, UINT32 *, UINT32 *);
typedef INT32(*AD_CONV_FP)(UINT32 *, UINT32, UINT32, void *);

typedef struct {
	UINT32 node_idx; //node path
	UINT32 key_idx;  //key idx
	CHAR *key; // property key
	AD_PARSE_FP parse_fp;
	AD_CONV_FP conv_fp;
	AD_LIST_TYPE type;
} AD_CFG_PARSE_ITEM;

typedef INT32(*AD_PROC_FP)(UINT8 *pfdt, CHAR *path, const AD_CFG_PARSE_ITEM *parser, AD_CFG_INFO *ad_info);

typedef struct {
	CHAR *path;
	AD_PROC_FP fp;
} AD_NODE_ITEAM;

typedef struct{
	char *str;
	unsigned int val;
} AD_STR_VAL_ITEM;

#define CLK_MAP_MAX 3
static const AD_STR_VAL_ITEM clk_map[CLK_MAP_MAX] = {
	{"MCLK", CTL_SEN_CLK_SEL_SIEMCLK },
	{"MCLK2", CTL_SEN_CLK_SEL_SIEMCLK2 },
	{"MCLK3", CTL_SEN_CLK_SEL_SIEMCLK3 },
};
static INT32 ad_list_parse_mclk_sel(void *in, UINT32 in_len, UINT32 *out, UINT32 *out_num)
{
	INT32 rt = CTL_SEN_E_OK;
	UINT32 i;

	for (i = 0; i < CLK_MAP_MAX; i++) {

		if (strncmp(clk_map[i].str, (CHAR *)in, (in_len - 1)) == 0) {
			*out = i;
			*out_num = 1;
			return rt;
		}
	}

	*out_num = 0;
	rt = CTL_SEN_E_IN_PARAM;
	DBG_ERR("input error (%s)\r\n", (CHAR *)in);
	return rt;
}
static INT32 ad_list_conv_mclk_sel(UINT32 *in, UINT32 in_num, UINT32 key_idx, void *out_info)
{
	INT32 rt = 0;
	AD_CFG_INFO *ad_info = (AD_CFG_INFO *)out_info;

	if (in[0] >= CLK_MAP_MAX) {
		DBG_ERR("ad_info->pwr.mclk_sel %d out of range %d\r\n", in[0], CLK_MAP_MAX);
		rt = CTL_SEN_E_IN_PARAM;
	}
	ad_info->pwr.mclk_sel = clk_map[in[0]].val;
	return rt;
}


static INT32 ad_list_parse_uint32(void *in, UINT32 in_len, UINT32 *out, UINT32 *out_num)
{
	INT32 rt = CTL_SEN_E_OK;
	unsigned int *p_tmp = (unsigned int *)in;
	int i, n = in_len / sizeof(unsigned int);

	if (n > MAX_VAL_NUM) {
		n = MAX_VAL_NUM;
		DBG_ERR("val overflow (%d > %d)\r\n", n, MAX_VAL_NUM);
	}

	for (i = 0; i < n; i++) {
		*(out + i) = be32_to_cpu(p_tmp[i]);
	}

	*out_num = n;

	return rt;
}

static INT32 ad_list_conv_io_ctl_des(UINT32 *in, UINT32 in_num, UINT32 key_idx, void *out_info)
{
	AD_CFG_INFO *ad_info = (AD_CFG_INFO *)out_info;

	if (key_idx >= AD_IO_CTL_DES_MAX) {
		DBG_ERR("key_idx overflow (%d > %d)\r\n", key_idx, AD_IO_CTL_DES_MAX);
		return CTL_SEN_E_IN_PARAM;
	}

	ad_info->pwr.io_des[key_idx].pin = in[0];
	ad_info->pwr.io_des[key_idx].lvl = in[1];
	ad_info->pwr.io_des[key_idx].hold_time = in[2];
	return CTL_SEN_E_OK;
}

static INT32 ad_list_conv_i2c_slv_addr(UINT32 *in, UINT32 in_num,UINT32 key_idx, void *out_info)
{
	AD_CFG_INFO *ad_info = (AD_CFG_INFO *)out_info;

	ad_info->i2c.slv_addr = in[0];

	return CTL_SEN_E_OK;
}

static INT32 ad_list_conv_port(UINT32 *in, UINT32 in_num,UINT32 key_idx, void *out_info)
{
	UINT32 val, i;
	AD_INFO_OUT *out = (AD_INFO_OUT *)out_info;

	val = 0;
	for (i = 0; i < in_num; i++) {
		val |= (1 << in[i]);
	}

	out->in_num = in_num;
	out->in = val;
	return CTL_SEN_E_OK;
}
static INT32 ad_list_conv_port_sig_type(UINT32 *in, UINT32 in_num, UINT32 key_idx, void *out_info)
{
	AD_INFO_OUT *out = (AD_INFO_OUT *)out_info;

	out->sig.type = in[0];
	return CTL_SEN_E_OK;
}

#if 0
#endif

static void ad_parse_dump_key_info(CHAR *path, CHAR *key, UINT32 *val, UINT32 val_num)
{
	UINT32 i;

	DBG_IND("%s[%s]: \r\n", path, key);
	for (i = 0; i < val_num; i++) {
		DBG_IND("%d \r\n", (unsigned int)val[i]);
	}
}
static INT32 ad_parse_info(UINT8 *pfdt, CHAR *node_path, const AD_CFG_PARSE_ITEM *parser, AD_CFG_INFO *ad_info)
{
	INT32 rt;
	const void *pfdt_node;  // property node pointer
	INT32 node_ofs;
	UINT32 data_len, val[MAX_VAL_NUM], val_num;

	rt = CTL_SEN_E_OK;
	node_ofs = fdt_path_offset(pfdt, node_path);
	if (node_ofs > 0) {
		/* get data */
		pfdt_node = fdt_getprop(pfdt, node_ofs, parser->key, (int *)&data_len);

		if ((pfdt_node != 0) && (data_len > 0) && (parser->parse_fp != 0)) {

			rt = parser->parse_fp((void *)pfdt_node, data_len, &val[0], &val_num);
			if (rt == CTL_SEN_E_OK) {
				rt = parser->conv_fp(&val[0], val_num, parser->key_idx, ad_info);
			}
			ad_parse_dump_key_info(node_path, parser->key, val, val_num);
		} else {
			if (parser->type == AD_LIST_TYPE_NECESSARY) {
				rt = CTL_SEN_E_IN_PARAM;
				DBG_ERR("%s[%s]: fail\r\n", node_path, parser->key);
			}
			else {
				DBG_IND("%s[%s]: null\r\n", node_path, parser->key);
			}
		}
	}
	else {
		if (parser->type == AD_LIST_TYPE_NECESSARY) {
			rt = CTL_SEN_E_IN_PARAM;
			DBG_ERR("%s[%s]: fail\r\n", node_path, parser->key);
		}
	}
	return rt;
}
static INT32 ad_parse_outport(UINT8 *pfdt, CHAR *node_path, const AD_CFG_PARSE_ITEM *parser, AD_CFG_INFO *ad_info)
{
	INT32 rt, node_ofs, err_cnt;
	UINT32 data_len, val[MAX_VAL_NUM], val_num, port_idx, port_cnt;
	CHAR path[AD_CFG_PATH_LEN];
	const void *pfdt_node;  // property node pointer

	port_cnt = 0;
	port_idx = 0;
	err_cnt = 0;
	while (port_idx < ad_info->out_max_num) {

		//get valid port idx
		snprintf(&path[0], AD_CFG_PATH_LEN, "%s%d", node_path, (unsigned int)port_idx);
		node_ofs = fdt_path_offset(pfdt, path);
		if (node_ofs > 0) {

			/* get data */
			pfdt_node = fdt_getprop(pfdt, node_ofs, parser->key, (int *)&data_len);

			if ((pfdt_node != NULL) && (data_len > 0) && (parser->parse_fp != NULL)) {

				rt = parser->parse_fp((void *)pfdt_node, data_len, &val[0], &val_num);
				if (rt == CTL_SEN_E_OK) {
					parser->conv_fp(&val[0], val_num, parser->key_idx, &ad_info->out[port_idx]);
				}
				port_cnt += 1;
				ad_parse_dump_key_info(path, parser->key, val, val_num);
			}
			else {

				if (parser->type == AD_LIST_TYPE_NECESSARY) {
					err_cnt += 1;
					DBG_ERR("%s[%s]: fail\r\n", path, parser->key);
				}
				else {
					DBG_IND("%s[%s]: null\r\n", path, parser->key);
				}
			}
		}
		port_idx += 1;
	}

	if ((port_cnt == 0) || err_cnt) {
		return CTL_SEN_E_IN_PARAM;
	}
	return CTL_SEN_E_OK;
}

//node path
#define AD_PARSE_NODE_MAX 3
static const AD_NODE_ITEAM node_tab[AD_PARSE_NODE_MAX] = {
	{"/power_cfg", ad_parse_info},
	{"", ad_parse_info},
	{"/outport@", ad_parse_outport}
};

static const AD_CFG_PARSE_ITEM parse_tab[] = {
	{ 0,  0, "mclk_sel", ad_list_parse_mclk_sel,    ad_list_conv_mclk_sel,         AD_LIST_TYPE_NONE },
	{ 0,  0, "io_ctl0",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  1, "io_ctl1",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  2, "io_ctl2",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  3, "io_ctl3",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  4, "io_ctl4",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  5, "io_ctl5",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  6, "io_ctl6",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  7, "io_ctl7",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  8, "io_ctl8",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0,  9, "io_ctl9",  ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 10, "io_ctl10", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 11, "io_ctl11", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 12, "io_ctl12", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 13, "io_ctl13", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 14, "io_ctl14", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 15, "io_ctl15", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 16, "io_ctl16", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 17, "io_ctl17", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 18, "io_ctl18", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 0, 19, "io_ctl19", ad_list_parse_uint32,      ad_list_conv_io_ctl_des,	   AD_LIST_TYPE_NONE },
	{ 1,  0, "reg",      ad_list_parse_uint32,      ad_list_conv_i2c_slv_addr,     AD_LIST_TYPE_NECESSARY },
	{ 2,  0, "inport",   ad_list_parse_uint32,      ad_list_conv_port,      	   AD_LIST_TYPE_NECESSARY },
	{ 2,  0, "signal",   ad_list_parse_uint32,      ad_list_conv_port_sig_type,    AD_LIST_TYPE_NECESSARY },
	{ AD_PARSE_TAB_END, 0, "", 0, 0, AD_LIST_TYPE_NONE },
};

static const AD_NODE_ITEAM* ad_parse_get_node_item(UINT32 node_idx)
{
	if (node_idx >= AD_PARSE_NODE_MAX) {
		DBG_ERR("error node_idx(%d < %d) overflow\r\n", node_idx, AD_PARSE_NODE_MAX);
		return 0;
	}
	return &node_tab[node_idx];
}

static INT32 ad_parse_proc(UINT8 *pfdt, CHAR *ad_name, AD_CFG_INFO *ad_info)
{
	INT32 rt, err_cnt;
	const AD_CFG_PARSE_ITEM *parse_item;
	CHAR path[AD_CFG_PATH_LEN];
	const AD_NODE_ITEAM *node_item;

	err_cnt = 0;
	parse_item = &parse_tab[0];

	while (parse_item->node_idx != AD_PARSE_TAB_END) {

		node_item = ad_parse_get_node_item(parse_item->node_idx);
		if (node_item == 0) {
			continue;
		}

		snprintf(&path[0], AD_CFG_PATH_LEN, "%s%s", ad_name, node_item->path);

		rt = CTL_SEN_E_IN_PARAM;
		if (node_item->fp) {
			rt = node_item->fp(pfdt, path, parse_item, ad_info);
		}

		if (rt != CTL_SEN_E_OK) {
			err_cnt += 1;
		}

		parse_item += 1;
	}

	rt = CTL_SEN_E_OK;
	if (err_cnt) {
		rt = CTL_SEN_E_IN_PARAM;
	}
	return rt;
}

#if 0
#endif

static void ad_dtsi_set_cfg2def(AD_CFG_INFO *ad_info)
{
	UINT32 i;

	memset(&ad_info->i2c, AD_CFG_INVALID_VALUE, sizeof(AD_INFO_I2C));
	memset(&ad_info->pwr, AD_CFG_INVALID_VALUE, sizeof(AD_INFO_PWR));

	for (i = 0; i < ad_info->out_max_num; i++) {
		ad_info->out[i].in = 0;
		ad_info->out[i].in_num = 0;
		ad_info->out[i].sig.type = AD_CFG_INVALID_VALUE;
	}

	ad_info->status = 0;
}
#if 0
#endif
#define CONFIG_CMD_FDT_MAX_DUMP 64
static CHAR isprint_fake(unsigned char c)
{
	if (c >= 0x20 && c <= 0x7e)
		return 1;
	return 0;
}

static inline int isspace_fake(unsigned char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\12');
}

static int is_printable_string(const void *data, int len)
{
	int isp, iss;
	const CHAR *s = (const CHAR *)data;

	/* zero length is not */
	if (len == 0)
		return 0;

	/* must terminate with zero or '\n' */
	if (s[len - 1] != '\0' && s[len - 1] != '\n')
		return 0;

	/* printable or a null byte (concatenated strings) */
	isp = isprint_fake(*s);
	iss = isspace_fake(*s);
	while (((*s == '\0') || isp || iss) && (len > 0)) {
		/*
		* If we see a null, there are three possibilities:
		* 1) If len == 1, it is the end of the string, printable
		* 2) Next character also a null, not printable.
		* 3) Next character not a null, continue to check.
		*/
		if (s[0] == '\0') {
			if (len == 1)
				return 1;
			if (s[1] == '\0')
				return 0;
		}
		s++;
		len--;
	}

	/* Not the null termination, or not done yet: not printable */
	if (*s != '\0' || (len != 0))
		return 0;

	return 1;
}

static void print_prop(const CHAR *pathp, int level, const void *data, int len)
{
	int i, cnt;
	unsigned int *p;
	unsigned char *s;
	CHAR buf[AD_CFG_PATH_LEN];

	if (pathp == NULL) {
		DBG_ERR("err pathp == NULL\r\n");
		return;
	}

	/* no data, don't print */
	if ((len == 0) || (data == NULL)) {
		DBG_DUMP("%*s%s\n", level, "", pathp);
		return;
	}

	/*
	* It is a string, but it may have multiple strings (embedded '\0's).
	*/
	if (is_printable_string(data, len)) {
		DBG_DUMP("%*s%s = \"%s\"\n", level, "", pathp, (CHAR *)data);
		return;
	}

	if ((len % 4) == 0) {
		if (len > CONFIG_CMD_FDT_MAX_DUMP) {
			DBG_DUMP("%*s%s = * 0x%p [0x%08x]\n", level, "", pathp, data, len);
		} else {
			p = (unsigned int *)data;
			cnt = 0;
			cnt += snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), "%*s%s = <", level, "", pathp);
			for (i = 0; i < len / 4; i++) {
				cnt += snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), "0x%.8x ", be32_to_cpu(p[i]));
			}
			if (cnt) {
				cnt -= 1;
			}
			snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), ">");
			DBG_DUMP("%s\n", buf);
		}
	}
	else { /* anything else... hexdump */
		if (len > CONFIG_CMD_FDT_MAX_DUMP) {
			DBG_DUMP("%*s%s = * 0x%p [0x%08x]\n", level, "", pathp, data, len);
		} else {
			s = (unsigned char *)data;
			cnt = 0;
			cnt += snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), "%*s%s = [ ", level, "", pathp);
			for (i = 0; i < len; i++) {
				cnt += snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), "%.2x ", s[i]);
			}
			snprintf(&buf[cnt], (AD_CFG_PATH_LEN - cnt), " ]");
			DBG_DUMP("%s\n", buf);
		}
	}
}

static INT32 ad_dtsi_get_compat_dev_node(UINT8 *pfdt, CHAR* compat_name, char *compat_path, UINT32 compat_path_max, UINT32 *i2c_id)
{
	int offset = 0;
	const struct fdt_property *i2c_prop;
	int len;

	while ((offset = fdt_node_offset_by_compatible(pfdt, offset, compat_name)) > 0) {

		int parent = fdt_parent_offset(pfdt, offset);
		if (parent < 0) {
			DBG_ERR("failed to get parent from compatible: %s\n", compat_name);
			return -1;
		}
		if (fdt_get_path(pfdt, offset, compat_path, compat_path_max) < 0) {
			DBG_ERR("failed to get path from compatible: %s\n", compat_name);
			return -1;
		}
		DBG_IND("path: %s\n", compat_path);

		i2c_prop = fdt_get_property(pfdt, parent, "id", &len);
		if (i2c_prop == NULL || len == 0) {
			DBG_ERR("failed to get i2c id: %s\n", compat_name);
			return -1;
		}

		*i2c_id = cpu_to_be32(*(int *)i2c_prop->data);
		return 1;
	}
	return -1;
}

static void ad_dtsi_dump(UINT8 *pfdt, struct i2c_device_id *dev)
{
	INT32 rt;
	INT32 node_ofs;
	CHAR buf[AD_CFG_PATH_LEN];
	UINT32 i2c_id;
	const void *nodep;	/* property node pointer */
	const CHAR *pathp;
	int pty_ofs, base_space, level, len;
	const struct fdt_property *fdt_prop;

	i2c_id = 0;
	memset(buf, 0, AD_CFG_PATH_LEN);
	rt = ad_dtsi_get_compat_dev_node(pfdt, dev->name, buf, AD_CFG_PATH_LEN, &i2c_id);

	if (rt > 0) {

		DBG_DUMP("%s\r\n", buf);

		node_ofs = fdt_path_offset(pfdt, buf);
		if (node_ofs < 0) {
			DBG_ERR("get %s fail\r\n", buf);
			return;
		}

		//dump current property
		len = 0;
		base_space = 0;
		pty_ofs = fdt_first_property_offset(pfdt, node_ofs);
		while (pty_ofs > 0) {
			len= 0;
			fdt_prop = fdt_get_property_by_offset(pfdt, pty_ofs, &len);
			if ((fdt_prop != NULL) && (len >= 0)) {
				pathp = fdt_string(pfdt, be32_to_cpu(fdt_prop->nameoff));
				nodep = fdt_prop->data;
				//coverity[tainted_data]
				print_prop(pathp, (base_space + 2), nodep, len);
			}
			pty_ofs = fdt_next_property_offset(pfdt, pty_ofs);
		}

		//dump information
		level = 0;
		while (level >= 0) {
			node_ofs = fdt_next_node(pfdt, node_ofs, &level);
			if ((node_ofs != 0) && (level >= 0)) {
				pathp = fdt_get_name(pfdt, node_ofs, NULL);
				base_space = ((level - 1) * 4);
				DBG_DUMP("%*s%s:\n", base_space, "", pathp);

				pty_ofs = fdt_first_property_offset(pfdt, node_ofs);
				while (pty_ofs > 0) {
					len= 0;
					fdt_prop = fdt_get_property_by_offset(pfdt, pty_ofs, &len);
					if ((fdt_prop != NULL) && (len >= 0)) {
						pathp = fdt_string(pfdt, be32_to_cpu(fdt_prop->nameoff));
						nodep = fdt_prop->data;
						//coverity[tainted_data]
						print_prop(pathp, (base_space + 2), nodep, len);
					}
					pty_ofs = fdt_next_property_offset(pfdt, pty_ofs);
				}
			}
		}
	} else {
		DBG_DUMP("get compat(%s) fail\r\n", dev->name);
	}
}
#if 0
#endif
static INT32 ad_dtsi_parser(UINT8 *pfdt, struct i2c_device_id *dev_tab, AD_CFG_INFO *ad_info, UINT32 ad_max_num)
{
	INT32 rt;
	UINT32 i, err_cnt;
	UINT32 i2c_id = 0;
	CHAR buf[AD_CFG_PATH_LEN];

	err_cnt = 0;
	for (i  = 0; i < ad_max_num; i ++) {

		memset(buf, 0, AD_CFG_PATH_LEN);
		rt = ad_dtsi_get_compat_dev_node(pfdt, dev_tab[i].name, buf, AD_CFG_PATH_LEN, &i2c_id);

		if (rt > 0) {

			//set to default value
			ad_dtsi_set_cfg2def(&ad_info[dev_tab[i].rev]);

			//set i2c ch id
			ad_info[dev_tab[i].rev].i2c.ch = i2c_id;

			//scan chip information
			rt = ad_parse_proc(pfdt, buf, &ad_info[dev_tab[i].rev]);
			if (rt != CTL_SEN_E_OK) {
				err_cnt += 1;
			}

			if (err_cnt == 0) {
				ad_info[dev_tab[i].rev].status |= AD_CFG_STS_OK;
			}
		}
	}
	return 0;
}


INT32 AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(UINT8 *pfdt, struct i2c_device_id *dev_tab, AD_CFG_INFO *ad_info, UINT32 ad_max_num)
{
	if (ad_info == NULL) {
		DBG_ERR("ad_info is NULL\r\n");
		return CTL_SEN_E_IN_PARAM;
	}

	if (pfdt == NULL) {
		DBG_ERR("pfdt is NULL\r\n");
		return CTL_SEN_E_IN_PARAM;
	}

	if (dev_tab == NULL) {
		DBG_ERR("dev_tab is NULL\r\n");
		return CTL_SEN_E_IN_PARAM;
	}

	return ad_dtsi_parser(pfdt, dev_tab, ad_info, ad_max_num);
}

void AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(UINT8 *pfdt, struct i2c_device_id *dev)
{
	if (pfdt == NULL) {
		DBG_ERR("pfdt is NULL\r\n");
		return;
	}

	if (dev == NULL) {
		DBG_ERR("dev is NULL\r\n");
		return;
	}
	//parse dtb file
	ad_dtsi_dump(pfdt, dev);
}
