/******************************************************************************
* File:             tag_param.c
*
* Author:           Lynn
* Created:          12/08/22
* Description:      PDM Tag lib and param tag parsing
*****************************************************************************/

#define pr_fmt(fmt) "Param Tag: " fmt

#include <asm/setup.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/export.h>

#define TAG_DATA_BUF_SIZE  4
struct tags_data_t {
	int version;
	unsigned int item_num;
	unsigned int buflen;
	char buf[TAG_DATA_BUF_SIZE];
};

#define TAG_ITEM_NAME_SIZE 8
struct tags_item_t {
	char name[TAG_ITEM_NAME_SIZE];
	unsigned short offset;
	unsigned short length;
};

#define BOOT_PARAMS_SIZE   2048

static char g_tags_data_buf[BOOT_PARAMS_SIZE];
static struct tags_data_t *g_tags_data = NULL;

/**************************************************************************************/
static int __init parse_param(const struct tag *tag, void *fdt)
{
	int length;

	/* bad tag format. */
	if (tag->hdr.size <= 2) {
		pr_err("%s: bad tag format.\n", __func__);
		goto fail;
	}

	length = ((tag->hdr.size - 2) << 2);
	if (length > BOOT_PARAMS_SIZE) {
		pr_err("%s: not enough parameter buffer.\n", __func__);
		goto fail;
	}

	memcpy(g_tags_data_buf, &tag->u, length);

	g_tags_data = (struct tags_data_t *)g_tags_data_buf;

	if ((g_tags_data->buflen + offsetof(struct tags_data_t, buf)) > length) {
		pr_err("%s: bad tag format. buflen too larger\n", __func__);
		goto fail;
	}

	if (g_tags_data->item_num * sizeof(struct tags_item_t)
	    > g_tags_data->buflen) {
		pr_err("%s: bad tag format. too many item\n", __func__);
		goto fail;
	}

	return 0;
fail:
	g_tags_data = NULL;
	return 0;
}

struct tagtable_lhs g_tag_param __initdata = {ATAG_PARAM, parse_param};

/*
 * name          param name
 * data          param buffer pointer;
 *
 * return       -1:    not found this param;
 *              -2:    input parameter bad;
 *              other: parameter real length;
 */
int find_param_tag(const char *name, char **data)
{
	char *data_buf = NULL;
	unsigned int item_num;
	struct tags_item_t *item = NULL;

	if ((name == NULL) || !*name) {
		pr_err("%s: bad parameter.\n", __func__);
		return -2; /* -2: input parameter bad; */
	}

	/* not found this tags */
	if (g_tags_data == NULL) {
		return -1;
	}

	item_num = g_tags_data->item_num;
	item = (struct tags_item_t *)g_tags_data->buf;

	while (item_num--) {
		if (!strncmp(item->name, name, 8)) { /* 8 for buffer name size */
			goto find;
		}
		item++;
	}
	return -1;

find:
	data_buf = g_tags_data->buf
		+ (g_tags_data->item_num * sizeof(struct tags_item_t));

	if ((data_buf + item->offset + item->length)
	     > (g_tags_data->buf + g_tags_data->buflen)) {
		pr_err("%s: tag format error.\n", __func__);
		return -1;
	}

	if (data != NULL)
		*data = (char *)(data_buf + item->offset);

	return (int)item->length;
}

/*
 * name         param name
 * buf          param buffer pointer;
 * buflen       param length;
 *
 * return       -1:    not found this param;
 *              -2:    input parameter bad;
 *              other: parameter real length;
 */
int get_param_data(const char *name, char *buf, unsigned int buflen)
{
        char *data = NULL;
        int len;

        if (buf == NULL) {
                pr_err("%s: bad parameter.\n", __FILE__);
                return -2; /* -2 - return value */
        }

        len = find_param_tag(name, &data);
        if (len < 0)
                return len;

        if (len > buflen) {
                pr_err("%s: buffer too smaller\n", __FILE__);
                return -2; /* -2 - return value */
        }

        memcpy(buf, data, len);

        return len;
}
EXPORT_SYMBOL(get_param_data);

/*
 * name         param name
 * buf          param buffer pointer;
 * buflen       param length;
 *
 * return       -1:    not found this param;
 *              -2:    input parameter bad;
 *              other: parameter real length;
 */
int set_param_data(const char *name, char *buf, unsigned int buflen)
{
        char *data = NULL;
        int len;

        if (buf == NULL) {
                pr_err("%s: bad parameter.\n", __FILE__);
                return -2; /* -2 - return value */
        }

        len = find_param_tag(name, &data);
        if (len < 0)
                return len;

        len = min_t(int, len, buflen);
        memcpy(data, buf, len);

        return len;
}
EXPORT_SYMBOL(set_param_data);

