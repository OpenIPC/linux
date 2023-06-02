#ifndef _AD_I2C_H_
#define _AD_I2C_H_

#include "kwrap/nvt_type.h"
#include "ad_util_int.h"
#if defined(__FREERTOS)
#include "plat/kdrv_i2c.h"
#else
#include <linux/i2c.h>
#endif

typedef enum _AD_I2C_ID {
	AD_I2C_ID_1 = 0,
	AD_I2C_ID_2 = 1,
	AD_I2C_ID_3 = 2,
	AD_I2C_ID_4 = 3,
	AD_I2C_ID_5 = 4,
	ENUM_DUMMY4WORD(AD_I2C_ID)
} AD_I2C_ID;

#if defined(__FREERTOS)
ER AD_UTIL_DECLARE_FUNC(ad_i2c_init_driver)(char *name, UINT32 id, AD_I2C_ID i2c_id, UINT32 slave_addr);
#else
ER AD_UTIL_DECLARE_FUNC(ad_i2c_init_driver)(UINT32 id, struct i2c_client *client);
#endif
ER AD_UTIL_DECLARE_FUNC(ad_i2c_uninit_driver)(UINT32 id);
INT32 AD_UTIL_DECLARE_FUNC(ad_i2c_transfer)(UINT32 id, struct i2c_msg *msgs, INT32 num);


#endif //_AD_I2C_H_
