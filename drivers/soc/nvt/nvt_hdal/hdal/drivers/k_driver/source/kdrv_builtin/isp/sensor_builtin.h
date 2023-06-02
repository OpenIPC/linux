#ifndef _SENSOR_BUILTIN_H_
#define _SENSOR_BUILTIN_H_

#include <linux/slab.h>

#include "kwrap/type.h"

//=============================================================================
// version
//=============================================================================
#define SENSOR_BUILTIN_VERSION 0x01000000

//=============================================================================
// define
//=============================================================================
#define MALLOC(x) kmalloc((x), GFP_KERNEL)
#define FREE(x) kfree((x))

#define hdr_expline_clamp(expl, l, u)    ((expl > u) ? u : ((expl < l) ? l : expl))

//=============================================================================
// struct & enum definition
//=============================================================================
typedef struct _SENSOR_BUILTIN_I2C_INFO {
	struct i2c_client  *iic_client;
	struct i2c_adapter *iic_adapter;
} SENSOR_BUILTIN_I2C_INFO;

typedef struct _SENSOR_BUILTIN_I2C_CMD {
	UINT32 addr;                      ///< address
	UINT32 data_len;                  ///< data length(bytes)
	UINT32 data[2];                   ///< data idx1(LSB) -> idx2(MSB)
} SENSOR_BUILTIN_I2C_CMD;

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 sensor_builtin_get_row_time(UINT32 id);
extern void sensor_builtin_set_gain(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value);
extern void sensor_builtin_set_expt(UINT32 id, ISP_BUILTIN_SENSOR_CTRL *value);
extern INT32 sensor_builtin_set_transfer_i2c(UINT32 id, void *msgs, INT32 num);
extern void sensor_builtin_init_i2c(UINT32 id);
extern void sensor_builtin_uninit_i2c(UINT32 id);

#endif

