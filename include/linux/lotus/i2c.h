#ifndef __I2C_H_
#define __I2C_H_

#include <linux/i2c.h>


int lotus_i2c_master_send(const struct i2c_client *client, const char *buf, int count);

int lotus_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);


#endif/* End of #ifndef __I2C_H */
