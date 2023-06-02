#include <linux/slab.h>
#include <kwrap/file.h>
#include <libfdt.h>
#include "kflow_videocapture/ctl_sen.h"
#include "ad_info_parser_int.h"
#include "ad_dbg_int.h"

#include <linux/gpio.h>
#include <linux/delay.h>
#include <plat/nvt-gpio.h>

#define AD_PARSE_TAB_END 0xffffffff
#define AD_CFG_PATH_LEN 80
#define MAX_VAL_NUM 10

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

typedef INT32(*AD_PROC_FP)(struct device_node *node, UINT32 node_idx, char *node_name, const AD_CFG_PARSE_ITEM *parse_tab, AD_CFG_INFO *ad_info);

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
static void ad_parse_dump_key_info(const char *path, CHAR *key, UINT32 *val, UINT32 val_num)
{
	UINT32 i, idx, total_size;
	CHAR msg[AD_CFG_PATH_LEN];

	idx = 0;
	total_size = AD_CFG_PATH_LEN;
	idx += snprintf(&msg[idx], (total_size - idx), "%s[%s]: ", path, key);
	for (i = 0; i < val_num; i++) {
		idx += snprintf(&msg[idx], (total_size - idx), "%d ", (unsigned int)val[i]);
	}
	DBG_IND("%s\r\n", msg);
}


static INT32 ad_parse_info(struct device_node *node, UINT32 node_idx, char *node_name, const AD_CFG_PARSE_ITEM *parse_tab, AD_CFG_INFO *ad_info)
{
	INT32 rt = 0;
	const AD_CFG_PARSE_ITEM *parser;
	const void *data;
	UINT32 data_len, val[MAX_VAL_NUM], val_num;

	parser = parse_tab;
	while (parser->node_idx != AD_PARSE_TAB_END) {

		if (parser->node_idx == node_idx) {

			data = of_get_property(node, parser->key, &data_len);
			if ((data != 0) && (data_len > 0) && (parser->parse_fp != 0)) {

				rt = parser->parse_fp((void *)data, data_len, &val[0], &val_num);
				if (rt == CTL_SEN_E_OK) {
					parser->conv_fp(&val[0], val_num, parser->key_idx, ad_info);
				}
				ad_parse_dump_key_info(node->full_name, parser->key, val, val_num);
			} else {
				if (parser->type == AD_LIST_TYPE_NECESSARY) {
					DBG_ERR("%s[%s]: fail\r\n", node->full_name, parser->key);
					return CTL_SEN_E_IN_PARAM;
				}
				else {
					DBG_IND("%s[%s]: null\r\n", node->full_name, parser->key);
				}
			}
		}
		parser += 1;
	}
	return CTL_SEN_E_OK;
}

static INT32 ad_parse_outport(struct device_node *node, UINT32 node_idx, char *node_name, const AD_CFG_PARSE_ITEM *parse_tab, AD_CFG_INFO *ad_info)
{
	INT32 rt, err_cnt;
	UINT32 data_len, val[MAX_VAL_NUM], val_num, port_idx;
	const void *data;
	const AD_CFG_PARSE_ITEM *parser;

	rt = sscanf(node->full_name, "outport@%d", &port_idx);
	if (rt == -1) {
		DBG_ERR("get port_idx idx fail (%s)\r\n", node->full_name);
		return CTL_SEN_E_IN_PARAM;
	}

	if (port_idx >= ad_info->out_max_num) {
		DBG_ERR("port_idx(%d) overflow (%d)\r\n", port_idx, ad_info->out_max_num);
		return CTL_SEN_E_IN_PARAM;
	}
	DBG_IND("port_idx = %d\r\n", port_idx);

	err_cnt = 0;
	parser = parse_tab;
	while (parser->node_idx != AD_PARSE_TAB_END) {
		if (parser->node_idx == node_idx) {

			/* get data */
			data = of_get_property(node, parser->key, &data_len);
			if ((data != 0) && (data_len > 0) && (parser->parse_fp != NULL)) {

				rt = parser->parse_fp((void *)data, data_len, &val[0], &val_num);
				if (rt == CTL_SEN_E_OK) {
					parser->conv_fp(&val[0], val_num, parser->key_idx, &ad_info->out[port_idx]);
				}
				ad_parse_dump_key_info(node->full_name, parser->key, val, val_num);
			} else {

				if (parser->type == AD_LIST_TYPE_NECESSARY) {
					err_cnt += 1;
					DBG_ERR("%s[%s]: fail\r\n", node->full_name, parser->key);
				}
				else {
					DBG_IND("%s[%s]: null\r\n", node->full_name, parser->key);
				}
			}
		}
		parser += 1;
	}
	if (err_cnt) {
		return CTL_SEN_E_IN_PARAM;
	}
	return CTL_SEN_E_OK;
}

//node path
#define AD_PARSE_NODE_MAX 2
static const AD_NODE_ITEAM node_tab[AD_PARSE_NODE_MAX] = {
	{"power_cfg", ad_parse_info},
	{"outport", ad_parse_outport}
};

static const AD_CFG_PARSE_ITEM parse_tab[] = {
	{ 0,  0, "mclk_sel",  ad_list_parse_mclk_sel,    ad_list_conv_mclk_sel,        AD_LIST_TYPE_NONE },
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
	{ 1,  0, "inport",	  ad_list_parse_uint32,      ad_list_conv_port,      	   AD_LIST_TYPE_NECESSARY },
	{ 1,  0, "signal",    ad_list_parse_uint32,      ad_list_conv_port_sig_type,   AD_LIST_TYPE_NECESSARY },
	{ AD_PARSE_TAB_END, 0, "", 0, 0, AD_LIST_TYPE_NONE },
};

static const AD_NODE_ITEAM* ad_devnode_get_node_by_name(const char *node_name, UINT32 *node_idx)
{
	UINT32 i, size;

	for (i = 0; i < AD_PARSE_NODE_MAX; i ++) {
		size = strlen(node_tab[i].path);
		if (strncmp(node_tab[i].path, node_name, size) == 0) {
			*node_idx = i;
			return &node_tab[i];
		}

	}
	DBG_ERR("can't find node(%s)\r\n", node_name);
	return 0;
}

static INT32 ad_devnode_proc(struct device_node *node, AD_CFG_INFO *ad_info)
{
	INT32 rt;
	UINT32 err_cnt, idx;
	struct device_node *child;
	const AD_NODE_ITEAM *node_item;

	DBG_IND("******** %s ********\r\n", node->full_name);

	idx = 0;
	err_cnt = 0;
	for_each_child_of_node(node, child) {
		node_item = ad_devnode_get_node_by_name(child->name, &idx);
		if (node_item) {
			if (node_item->fp) {
				rt = node_item->fp(child, idx, node_item->path, &parse_tab[0], ad_info);

				if (rt < 0) {
					err_cnt += 1;
				}
			}
		}
	}

	if (err_cnt > 0) {
		return CTL_SEN_E_IN_PARAM;
	}

	return CTL_SEN_E_OK;
}

static void ad_devnode_set_cfg2def(AD_CFG_INFO *ad_info)
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

static void ad_devnode_dump(struct i2c_client *client)
{
#define SPACE_NUM_PER_LVL 4
	int level;
	struct device_node *child;
	struct property *pty;
	struct device_node *parent;

	parent = client->dev.of_node;

	DBG_DUMP("%s\r\n", parent->full_name);
	level = 1;
	DBG_DUMP("%*sslave addr = 0x%x\r\n", (level * SPACE_NUM_PER_LVL), "", client->addr);
	for_each_child_of_node(parent, child) {
		DBG_DUMP("%*s%s\r\n", (level * SPACE_NUM_PER_LVL), "", child->full_name);
		level += 1;
		for_each_property_of_node(child, pty) {
			if ((strncmp(pty->name, "name", 4) == 0) && (strncmp(pty->value, child->name, strlen(child->name)) == 0)) {
			} else {
				print_prop(pty->name, (level * SPACE_NUM_PER_LVL), pty->value, pty->length);
			}
		}
		level -= 1;
	}
}

static INT32 ad_devnode_parser(struct i2c_client *client, AD_CFG_INFO *ad_info)
{
	INT32 rt;
	struct device_node *node;
	struct i2c_adapter *adap = to_i2c_adapter(client->dev.parent); // my parent master

	//set to default value
	ad_devnode_set_cfg2def(ad_info);

	node = client->dev.of_node;
	ad_info->i2c.ch = adap->nr;
	ad_info->i2c.slv_addr = client->addr; // 7-bit

	rt = ad_devnode_proc(node, ad_info);
	if (rt < 0) {
		DBG_ERR("fail\r\n");
		return rt;
	}
	ad_info->status |= AD_CFG_STS_OK;
	return rt;
}

INT32 AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(struct i2c_client *client, AD_CFG_INFO *ad_info)
{
	return ad_devnode_parser(client, ad_info);
}

void AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(struct i2c_client *client)
{
	ad_devnode_dump(client);
}
