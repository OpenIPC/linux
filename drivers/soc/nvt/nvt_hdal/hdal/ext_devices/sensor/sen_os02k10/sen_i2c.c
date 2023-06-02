#ifdef __KERNEL__
#include <linux/i2c.h>
#include <linux/slab.h>
#include "sen_dbg.h"

#define MALLOC(x) kmalloc((x), GFP_KERNEL)
#define FREE(x) kfree((x))

//extern void sen_test_imx290(void);
typedef void (*SEN_CB_FP)(struct device_node *of_node); 
static SEN_CB_FP sen_cb_fp = NULL;

#else
#include <stdlib.h>
#include <string.h>
#include "rtos_na51089/kdrv_i2c.h"
#include "sen_dbg.h"

#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

#define CTL_SEN_ID_MAX 8

#ifndef SEN_I2C_NAME_1
#define SEN_I2C_NAME_1 "sensor_1"
#endif

#ifndef SEN_I2C_NAME_2
#define SEN_I2C_NAME_2 "sensor_2"
#endif

#ifndef SEN_I2C_NAME_3
#define SEN_I2C_NAME_3 "sensor_3"
#endif

#ifndef SEN_I2C_NAME_4
#define SEN_I2C_NAME_4 "sensor_4"
#endif

#ifndef SEN_I2C_NAME_5
#define SEN_I2C_NAME_5 "sensor_5"
#endif

#ifndef SEN_I2C_NAME_6
#define SEN_I2C_NAME_6 "sensor_6"
#endif

#ifndef SEN_I2C_NAME_7
#define SEN_I2C_NAME_7 "sensor_7"
#endif

#ifndef SEN_I2C_NAME_8
#define SEN_I2C_NAME_8 "sensor_8"
#endif

#ifndef SEN_I2C_ADDR
#define SEN_I2C_ADDR 0
#endif

typedef struct _SEN_I2C_INFO {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} SEN_I2C_INFO;

static SEN_I2C_INFO *sen_i2c_info[CTL_SEN_ID_MAX];

#if defined(__KERNEL__)
void sen_i2c_reg_cb(SEN_CB_FP cb_fp)
{
	sen_cb_fp  = cb_fp;
}

static const struct of_device_id sen_i2c_of_match[][CTL_SEN_ID_MAX] = {
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	},
	{
		{ .compatible = SEN_I2C_COMPATIBLE},
		{ },
	}
};
#endif

static const struct i2c_device_id sen_i2c_id[][CTL_SEN_ID_MAX] = {
	{
		{ SEN_I2C_NAME_1, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_2, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_3, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_4, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_5, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_6, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_7, 0 },
		{ }
	},
	{
		{ SEN_I2C_NAME_8, 0 },
		{ }
	}
};

static INT sen_i2c_probe_1(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[0] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[0]->iic_client  = client;
	sen_i2c_info[0]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[0]);

	return 0;
}

static INT sen_i2c_probe_2(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[1] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[1]->iic_client  = client;
	sen_i2c_info[1]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[1]);

	return 0;
}

static INT sen_i2c_probe_3(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[2] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[2]->iic_client  = client;
	sen_i2c_info[2]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[2]);

	return 0;
}

static INT sen_i2c_probe_4(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[3] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[3]->iic_client  = client;
	sen_i2c_info[3]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[3]);

	return 0;
}

static INT sen_i2c_probe_5(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[4] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[4]->iic_client  = client;
	sen_i2c_info[4]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[4]);

	return 0;
}

static INT sen_i2c_probe_6(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[5] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[5]->iic_client  = client;
	sen_i2c_info[5]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[5]);

	return 0;
}

static INT sen_i2c_probe_7(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[6] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[6]->iic_client  = client;
	sen_i2c_info[6]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[6]);

	return 0;
}

static INT sen_i2c_probe_8(struct i2c_client *client, const struct i2c_device_id *id)
{
	#if defined(__KERNEL__)
	if (client->dev.of_node && (sen_cb_fp != NULL)) {
		sen_cb_fp(client->dev.of_node);
	}
	#endif

	if (!(sen_i2c_info[7] = MALLOC(sizeof(SEN_I2C_INFO)))) {
		DBG_ERR(" malloc fail. \r\n");
		return E_SYS;
	}

	sen_i2c_info[7]->iic_client  = client;
	sen_i2c_info[7]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, sen_i2c_info[7]);

	return 0;
}

static INT sen_i2c_remove_1(struct i2c_client *client)
{
	FREE(sen_i2c_info[0]);
	sen_i2c_info[0] = NULL;
	return 0;
}

static INT sen_i2c_remove_2(struct i2c_client *client)
{
	FREE(sen_i2c_info[1]);
	sen_i2c_info[1] = NULL;
	return 0;
}

static INT sen_i2c_remove_3(struct i2c_client *client)
{
	FREE(sen_i2c_info[2]);
	sen_i2c_info[2] = NULL;
	return 0;
}

static INT sen_i2c_remove_4(struct i2c_client *client)
{
	FREE(sen_i2c_info[3]);
	sen_i2c_info[3] = NULL;
	return 0;
}

static INT sen_i2c_remove_5(struct i2c_client *client)
{
	FREE(sen_i2c_info[4]);
	sen_i2c_info[4] = NULL;
	return 0;
}

static INT sen_i2c_remove_6(struct i2c_client *client)
{
	FREE(sen_i2c_info[5]);
	sen_i2c_info[5] = NULL;
	return 0;
}

static INT sen_i2c_remove_7(struct i2c_client *client)
{
	FREE(sen_i2c_info[6]);
	sen_i2c_info[6] = NULL;
	return 0;
}

static INT sen_i2c_remove_8(struct i2c_client *client)
{
	FREE(sen_i2c_info[7]);
	sen_i2c_info[7] = NULL;
	return 0;
}

static struct i2c_driver sen_i2c_driver[CTL_SEN_ID_MAX] = {
	{
		.driver = {
			.name  = SEN_I2C_NAME_1,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[0],
			#endif
		},
		.probe    = sen_i2c_probe_1,
		.remove   = sen_i2c_remove_1,
		.id_table = sen_i2c_id[0]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_2,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[1],
			#endif
		},
		.probe    = sen_i2c_probe_2,
		.remove   = sen_i2c_remove_2,
		.id_table = sen_i2c_id[1]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_3,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[2],
			#endif
		},
		.probe    = sen_i2c_probe_3,
		.remove   = sen_i2c_remove_3,
		.id_table = sen_i2c_id[2]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_4,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[3],
			#endif
		},
		.probe    = sen_i2c_probe_4,
		.remove   = sen_i2c_remove_4,
		.id_table = sen_i2c_id[3]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_5,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[4],
			#endif
		},
		.probe    = sen_i2c_probe_5,
		.remove   = sen_i2c_remove_5,
		.id_table = sen_i2c_id[4]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_6,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[5],
			#endif
		},
		.probe    = sen_i2c_probe_6,
		.remove   = sen_i2c_remove_6,
		.id_table = sen_i2c_id[5]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_7,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[6],
			#endif
		},
		.probe    = sen_i2c_probe_7,
		.remove   = sen_i2c_remove_7,
		.id_table = sen_i2c_id[6]
	},
	{
		.driver = {
			.name  = SEN_I2C_NAME_8,
			.owner = THIS_MODULE,
			#if defined(__KERNEL__)
			.of_match_table = sen_i2c_of_match[7],
			#endif
		},
		.probe    = sen_i2c_probe_8,
		.remove   = sen_i2c_remove_8,
		.id_table = sen_i2c_id[7]
	}
};

static struct i2c_board_info sen_i2c_device[CTL_SEN_ID_MAX] = {
	{
		.type = SEN_I2C_NAME_1,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_2,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_3,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_4,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_5,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_6,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_7,
		.addr = SEN_I2C_ADDR
	},
	{
		.type = SEN_I2C_NAME_8,
		.addr = SEN_I2C_ADDR
	}
};

static ER sen_i2c_init_driver(UINT32 id, SEN_I2C *sen_i2c)
{
	ER ret = E_OK;

	// add i2c_device to i2c bus
	sen_i2c_device[id].addr = sen_i2c->addr;
	if (i2c_new_device(i2c_get_adapter(sen_i2c->id), &sen_i2c_device[id]) == NULL) {
		DBG_ERR(" i2c_new_device fail. \r\n");
		return E_SYS;
	}

	// bind i2c client driver to i2c bus
	if ((ret = i2c_add_driver(&sen_i2c_driver[id])) != 0) {
		DBG_ERR(" i2c_add_driver fail. \r\n");
	}

	return ret;
}

static void sen_i2c_remove_driver(UINT32 id)
{
	i2c_unregister_device(sen_i2c_info[id]->iic_client);
	i2c_del_driver(&sen_i2c_driver[id]);
}

static INT32 sen_i2c_transfer(UINT32 id, struct i2c_msg *msgs, INT32 num)
{
	if (unlikely(sen_i2c_info[id]->iic_adapter == NULL)) {
		DBG_ERR(" sensen_i2c_info->ii2c_adapter fail. \r\n");
		return -1;
	}

	if (unlikely(i2c_transfer(sen_i2c_info[id]->iic_adapter, msgs, num) != num)) {
		DBG_ERR(" i2c_transfer fail. \r\n");
		return -1;
	}

	return 0;
}

