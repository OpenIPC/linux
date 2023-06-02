#ifdef __KERNEL__
#include <linux/i2c.h>
#include <linux/slab.h>
#include "aud_ac108.h"

#define MALLOC(x) kzalloc((x), GFP_KERNEL)
#define FREE(x) kfree((x))
#else
#include "aud_ac108.h"
#include <stdlib.h>
#include <string.h>

#include <libfdt.h>
#include <compiler.h>
#include <plat/rtosfdt.h>

#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

typedef struct {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} AUD_I2C_INFO;

static AUD_I2C_INFO *aud_i2c_info;

static const struct i2c_device_id aud_i2c_id[] = {
	{ I2C_NAME, 0 },
	{ }
};

static AUD_I2C aud_i2c = {
	0, 0
};

#if defined(__FREERTOS)
static struct i2c_board_info aud_i2c_device = {
	.type = I2C_NAME,
	.addr = 0,
};
#else
static const struct of_device_id aud_ac108_i2c_of_match[] = {
	{ .compatible = "nvt,aud_ac108" },
	{ },
};
#endif

#if defined(__FREERTOS)
BOOL aud_ac108_check_compatible(CHAR *compatible)
{
	int offset = 0;
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();

	if (fdt_node_offset_by_compatible(pfdt_addr, offset, compatible) > 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int aud_ac108_dts_parser(CHAR *compatible)
{
	unsigned char *pfdt_addr = (unsigned char *)fdt_get_base();
	INT32 offset = 0;
	CHAR node_path[32] = {0};
	INT32 data_len;
	UINT32 *pdata = NULL;
	const void *pfdt_node;
	INT32 node_ofst = 0;
	CHAR sub_node_name[64] = {0};
	CHAR keystr[32] = {0};

	while ((offset = fdt_node_offset_by_compatible(pfdt_addr, offset, compatible)) > 0) {
		if (fdt_get_path(pfdt_addr, offset, node_path, sizeof(node_path)) < 0) {
			DBG_WRN("failed to get path from compatible: %s\n", compatible);
			continue;
		}

		sprintf(sub_node_name, "%s/I2C", node_path);
		node_ofst = fdt_path_offset(pfdt_addr, sub_node_name);
		if (node_ofst >= 0) {
			sprintf(keystr, "i2c_id");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				//vk_printk("%s = %d \r\n", keystr, be32_to_cpu(*pdata));
				aud_i2c.id = be32_to_cpu(*pdata);
			}

			sprintf(keystr, "i2c_addr");
			pfdt_node = fdt_getprop(pfdt_addr, node_ofst, keystr, (int *)&data_len);
			if ((pfdt_node != NULL) && (data_len != 0)) {
				pdata = (UINT32 *)pfdt_node;
				//vk_printk("%s = 0x%X \r\n", keystr, be32_to_cpu(*pdata));
				aud_i2c.addr = be32_to_cpu(*pdata);
			}
		} else {
			DBG_ERR("%s not exist \n", sub_node_name);
		}
	}

	return 0;
}
#else
int aud_ac108_dts_parser(struct device *dev)
{
	struct device_node *of_node = dev->of_node;
	struct device_node *child;
	CHAR keystr[32];

	for_each_child_of_node(of_node, child) {
		sprintf((char *)keystr, "i2c_id");
		if (of_property_read_u32(child, keystr, &aud_i2c.id) == 0) {
			//vk_printk("%s = %d \r\n", keystr, aud_i2c.id);
		} else {
			DBG_ERR("parse id failed\r\n");
		}

		sprintf((char *)keystr, "i2c_addr");
		if (of_property_read_u32(child, keystr, &aud_i2c.addr) == 0) {
			//vk_printk("%s = %d \r\n", keystr, aud_i2c.addr);
		} else {
			DBG_ERR("parse addr failed\r\n");
		}
	}

	return 0;
}
#endif

static int aud_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	aud_i2c_info = NULL;
	aud_i2c_info = MALLOC(sizeof(AUD_I2C_INFO));
	if (aud_i2c_info == NULL) {
		DBG_ERR("%s fail: MALLOC not OK.\n", __FUNCTION__);
		return E_SYS;
	}

	aud_i2c_info->iic_client  = client;
	aud_i2c_info->iic_adapter = client->adapter;

#if defined(__KERNEL__)
	if (client->dev.of_node == NULL) {
		return -1;
	}
	aud_ac108_dts_parser(&client->dev);
#endif

	i2c_set_clientdata(client, aud_i2c_info);

	return 0;
}

static int aud_i2c_remove(struct i2c_client *client)
{
	FREE(aud_i2c_info);
	aud_i2c_info = NULL;
	return 0;
}

static struct i2c_driver aud_i2c_driver = {
	.driver = {
		.name  = "aud_ac108_i2c",
		.owner = THIS_MODULE,
#if defined(__KERNEL__)
		.of_match_table = of_match_ptr(aud_ac108_i2c_of_match),
#endif
	},
	.probe    = aud_i2c_probe,
	.remove   = aud_i2c_remove,
	.id_table = aud_i2c_id
};


ER aud_i2c_init_driver(void)
{
	ER ret = E_OK;

#if defined(__FREERTOS)
	CHAR compatible[64];

	sprintf(compatible, "nvt,aud_ac108");

	if (aud_ac108_check_compatible(compatible)) {
		 aud_ac108_dts_parser(compatible);
	} else {
		DBG_ERR("check_compatible failed\r\n");
		ret = E_SYS;

		return ret;
	}

	aud_i2c_device.addr = aud_i2c.addr;

	if (i2c_new_device(i2c_get_adapter(aud_i2c.id), &aud_i2c_device) == NULL) {
		DBG_ERR("%s fail: i2c_new_device not OK.\n", __FUNCTION__);
		ret = E_SYS;

		return ret;
	}
#endif

	if (i2c_add_driver(&aud_i2c_driver) != 0) {
		DBG_ERR("%s fail: i2c_add_driver not OK.\n", __FUNCTION__);
		ret = E_SYS;

		return ret;
	}

	if (aud_i2c.id == 0 && aud_i2c.addr == 0) {
		DBG_ERR("aud_i2c.id = %d, aud_i2c.addr = %x\r\n", aud_i2c.id, aud_i2c.addr);
		return E_SYS;
	}

	return ret;
}

void aud_i2c_remove_driver(UINT32 id)
{
	i2c_unregister_device(aud_i2c_info->iic_client);
	i2c_del_driver(&aud_i2c_driver);
}

static INT32 aud_i2c_transfer(struct i2c_msg *msgs, INT32 num)
{
	if (unlikely(aud_i2c_info->iic_adapter == NULL)) {
		DBG_ERR("%s fail: aud_i2c_info->ii2c_adapter not OK\n", __FUNCTION__);
		return -1;
	}

	if (unlikely(i2c_transfer(aud_i2c_info->iic_adapter, msgs, num) != num)) {
		DBG_ERR("%s fail: i2c_transfer not OK \n", __FUNCTION__);
		return -1;
	}

	return 0;
}

void aud_i2c_write(UINT32 addr, UINT32 value)
{
	struct i2c_msg  msgs;

	unsigned char	buf[2];

	buf[0]     = addr  & 0xFF;
	buf[1]     = value & 0xFF;
	msgs.addr  = aud_i2c.addr;
	msgs.flags = 0;//w
	msgs.len   = 2;
	msgs.buf   = buf;

	aud_i2c_transfer(&msgs, 1);
}

UINT32 aud_i2c_read(UINT32 addr)
{
	struct i2c_msg  msgs[2];
	unsigned char   buf[1], buf2[1];

	buf[0]        = addr & 0xFF;
	msgs[0].addr  = aud_i2c.addr;
	msgs[0].flags = 0;//w
	msgs[0].len   = 1;
	msgs[0].buf   = buf;

	buf2[0]       = 0;
	msgs[1].addr  = aud_i2c.addr;
	msgs[1].flags = 1;//r
	msgs[1].len   = 1;
	msgs[1].buf   = buf2;

	aud_i2c_transfer(msgs, 2);

	return (UINT32)buf2[0];
}

void aud_i2c_update(UINT32 addr, UINT32 mask, UINT32 value)
{
	UINT32 temp;

	temp = aud_i2c_read(addr);
	temp &= (~mask);
	aud_i2c_write(addr, temp|(value&mask));
}