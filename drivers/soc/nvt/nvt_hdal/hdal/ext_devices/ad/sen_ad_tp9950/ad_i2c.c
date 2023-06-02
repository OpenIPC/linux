#ifdef __KERNEL__
#include <linux/i2c.h>
#else
#include <stdio.h>
#define i2c_put_adapter(adapt)
#endif

#include "kflow_videocapture/ctl_sen.h"
#include "ad_i2c_int.h"
#include "ad_dbg_int.h"

#if defined(__FREERTOS)
static struct i2c_board_info i2c_device_tab[CTL_SEN_ID_MAX] = {
};
#endif

static struct i2c_client *i2c_client_tab[CTL_SEN_ID_MAX] = {
	[0 ... (CTL_SEN_ID_MAX-1)] = NULL
};

#if defined(__FREERTOS)
static struct i2c_board_info* ad_get_i2c_device(UINT32 id)
{
	if (id >= CTL_SEN_ID_MAX) {
		DBG_ERR("id(%d) overflow\r\n", id);
		return NULL;
	}
	return &i2c_device_tab[id];
}
#endif

static struct i2c_client* ad_get_i2c_client(UINT32 id)
{
	if (id >= CTL_SEN_ID_MAX) {
		DBG_ERR("id(%d) overflow\r\n", id);
		return NULL;
	}
	return i2c_client_tab[id];
}

static void ad_set_i2c_client(UINT32 id, struct i2c_client *client)
{
	if (id >= CTL_SEN_ID_MAX) {
		DBG_ERR("id(%d) overflow\r\n", id);
		return;
	}
	i2c_client_tab[id] = client;
}

#if defined(__FREERTOS)
ER AD_UTIL_DECLARE_FUNC(ad_i2c_init_driver)(char *name, UINT32 id, AD_I2C_ID i2c_id, UINT32 slave_addr)
{
	ER rt = E_SYS;

	struct i2c_client *client;
	struct i2c_adapter *adapt;
	struct i2c_board_info *board;

	client = ad_get_i2c_client(id);
	if (client != NULL) {
		DBG_ERR("id(%d) client is not null\r\n", id);
		return rt;
	}

	adapt = i2c_get_adapter(i2c_id);
	if (adapt == NULL) {
		DBG_ERR("id(%d) i2c_get_adapter(%d) fail\r\n", id, i2c_id);
		return rt;
	}

	board = ad_get_i2c_device(id);
	if (board == NULL) {
		goto err;
	}

	//set board name
	snprintf(board->type, sizeof(board->type), "%s_%d", name, (int)id);
	board->addr = slave_addr;
	client = i2c_new_device(adapt, board);
	if (client == NULL) {
		DBG_ERR("id(%d) i2c_new_device fail.\r\n", id);
		goto err;
	}
	ad_set_i2c_client(id, client);
	rt = E_OK;

err:
	i2c_put_adapter(adapt);
	return rt;
}

ER AD_UTIL_DECLARE_FUNC(ad_i2c_uninit_driver)(UINT32 id)
{
    struct i2c_client *org_client;

    org_client = ad_get_i2c_client(id);
    if (org_client != NULL)
    {
        i2c_unregister_device(org_client);
    }
    ad_set_i2c_client(id, NULL);
    return E_OK;
}
#else
ER AD_UTIL_DECLARE_FUNC(ad_i2c_init_driver)(UINT32 id, struct i2c_client *client)
{
	ER rt = E_SYS;
	struct i2c_client *org_client;

	org_client = ad_get_i2c_client(id);
	if (org_client != NULL) {
		DBG_ERR("id(%d) client is not null\r\n", id);
		return rt;
	}

	if (client == NULL) {
		DBG_ERR("input client is null\r\n");
		return rt;
	}
	rt = E_OK;
	ad_set_i2c_client(id, client);
	return rt;
}

ER AD_UTIL_DECLARE_FUNC(ad_i2c_uninit_driver)(UINT32 id)
{
    ad_set_i2c_client(id, NULL);
    return E_OK;
}
#endif

INT32 AD_UTIL_DECLARE_FUNC(ad_i2c_transfer)(UINT32 id, struct i2c_msg *msgs, INT32 num)
{
	INT i2c_num;
	struct i2c_client *client;

	client = ad_get_i2c_client(id);
	if (client == NULL) {
		DBG_ERR("id(%d) client is null\r\n", id);
		return -1;
	}

	i2c_num = i2c_transfer(client->adapter, msgs, num);
	if (i2c_num != num) {
		DBG_ERR("id(%d) i2c_transfer fail. \r\n", id);
		return -1;
	}
	return 0;
}
