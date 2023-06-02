#ifdef __KERNEL__
#include <linux/i2c.h>
#include <linux/slab.h>
#include "aud_emu.h"
#else
#include "aud_emu.h"
#endif

ER aud_i2c_init_driver(UINT32 i2c_id)
{
	return E_OK;
}

void aud_i2c_remove_driver(UINT32 id)
{
}

INT32 aud_i2c_transfer(struct i2c_msg *msgs, INT32 num)
{
	return 0;
}
