/******************** (C) COPYRIGHT 2016 Silan *********************************
 * 
 * File Name          : sc7a20.c
 * Authors            : dongjianqing@silan.com.cn
 * Version            : V.1.0.0
 * Date               : 20/09/2016
 * Description        : SC7A20 accelerometer sensor API
 *
 *******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, SILSN SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *
 ******************************************************************************
 Revision 1.0.0 20/09/2016
 First Release


 ******************************************************************************/

#include	<linux/module.h>
#include	<linux/err.h>
#include	<linux/errno.h>
#include	<linux/delay.h>
#include	<linux/fs.h>
#include	<linux/i2c.h>

#include	<linux/input.h>
#include	<linux/input-polldev.h>
#include	<linux/miscdevice.h>
#include	<linux/uaccess.h>
#include    <linux/slab.h>

#include	<linux/workqueue.h>
#include	<linux/irq.h>
#include	<linux/gpio.h>
#include	<linux/interrupt.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include    <linux/earlysuspend.h>
#endif

#include   "sc7a20.h"
#include   <linux/of_device.h>
#include   <linux/of_address.h>
#include   <linux/of_gpio.h>

#define	INTERRUPT_MANAGEMENT		1

#define	G_MAX						16000	/** Maximum polled-device-reported g value */

/*
#define	SHIFT_ADJ_2G				4
#define	SHIFT_ADJ_4G				3
#define	SHIFT_ADJ_8G				2
#define	SHIFT_ADJ_16G				1
*/

#define SENSITIVITY_2G				1	/**	mg/LSB	*/
#define SENSITIVITY_4G				2	/**	mg/LSB	*/
#define SENSITIVITY_8G				4	/**	mg/LSB	*/
#define SENSITIVITY_16G				12	/**	mg/LSB	*/

#define	HIGH_RESOLUTION				0x08

//#define	AXISDATA_REG			0x28


/* Accelerometer Sensor BDU Mask */
#define SC7A20_ACC_BDU_MASK         0x80

/* Accelerometer Sensor BLE Mask */
#define SC7A20_ACC_BLE_MASK         0x40

/* Accelerometer Sensor HR Mask */
#define SC7A20_ACC_HR_MASK          0x08

#define WHOAMI_SC7A20_ACC	        0x11

/* OUTPUT REGISTERS */
#define XOUT_L						0x28
#define XOUT_H						0x29
#define YOUT_L						0x2a
#define YOUT_H						0x2b
#define ZOUT_L						0x2c
#define ZOUT_H						0x2d
#define WHO_AM_I					0x0F	/*      WhoAmI register         */

#define CTRL_REG1					0x20
#define CTRL_REG2					0x21
#define CTRL_REG3					0x22
#define CTRL_REG4					0x23
#define CTRL_REG5					0x24
#define CTRL_REG6					0x25
#define STATUS_REG					0x27


#define SC7A20_ENABLE_ALL_AXES	    0x07
#define SC7A20_AC_MASK              0xF0
/* Accelerometer Sensor odr set */
#define ODR1		                0x10	 /* 1Hz output data rate */
#define ODR10		                0x20	 /* 10Hz output data rate */
#define ODR25		                0x30	 /* 25Hz output data rate */
#define ODR50		                0x40	 /* 50Hz output data rate */
#define ODR100		                0x50	 /* 100Hz output data rate */
#define ODR200		                0x60	 /* 200Hz output data rate */
#define ODR400		                0x70	 /* 400Hz output data rate */
#define ODR1250		                0x90	 /* 1250Hz output data rate */

/* INPUT_ABS CONSTANTS */
#define FUZZ			            32
#define FLAT			            32
/* RESUME STATE INDICES */
#define RES_CTRL_REG1               0
#define RES_CTRL_REG2               1
#define RES_CTRL_REG3               2
#define RES_CTRL_REG4               3
#define RES_CTRL_REG5               4
#define RES_CTRL_REG6               5

#define	RESUME_ENTRIES		        6

#define DEVICE_INFO                 "SL, SC7A20"

#define DEVICE_INFO_LEN             32
/* end RESUME STATE INDICES */

#define GSENSOR_GINT1_GPI           0
#define GSENSOR_GINT2_GPI           1

#define GSENSOR_CALI_SUPPORT

#ifdef GSENSOR_CALI_SUPPORT
#define GSENSOR_CAL_FILE_PATH		"/productinfo/silan_cal"       //silan Calbration file path
#endif


#define	I2C_RETRY_DELAY		        5
#define	I2C_RETRIES		            5
#define	I2C_AUTO_INCREMENT	        0x80

struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} sc7a20_odr_table[] = {
	{1,    ODR1250},
	{3,    ODR400 }, 
	{5,    ODR200 }, 
	{10,   ODR100 }, 
	{20,   ODR50  }, 
	{40,   ODR25  }, 
	{100,  ODR10  }, 
	{1000, ODR1   },
};

enum {
	ACC_STANDBY = 0,
	ACC_ACTIVED = 1,
};

struct sc7a20_data {
	struct i2c_client *client;
	struct sc7a20_platform_data *pdata;

	struct mutex lock;
	struct delayed_work input_work;

	struct input_dev *input_dev;

	int hw_initialized;
	/* hw_working=-1 means not tested yet */
	int hw_working;
	atomic_t enabled;
	int on_before_suspend;

	int poll_interval;
	int min_interval;
	int active;
	u8 sensitivity;
	u8 shift;
	u8 ctrl_reg1;
	u8 g_range;
	u8 bdu;
	u8 ble;
	u8 hr;
	u8 resume_state[RESUME_ENTRIES];

#ifdef GSENSOR_CALI_SUPPORT
	int get_cali_flag;
#endif

	int irq1;
	struct work_struct irq1_work;
	struct workqueue_struct *irq1_work_queue;
	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

/* Addresses scanned */
static const unsigned short normal_i2c[] = { SC7A20_ACC_I2C_ADDR, I2C_CLIENT_END };

#ifdef GSENSOR_CALI_SUPPORT
static int accel_cali[3] = {0,0,0};
#endif

/*
 * Because misc devices can not carry a pointer from driver register to
 * open, we keep this global.  This limits the driver to a single instance.
 */
struct sc7a20_data *sc7a20_misc_data;
struct i2c_client *sc7a20_i2c_client;
int sc7a20_update_g_range(struct sc7a20_data *acc, u8 new_g_range);

int sc7a20_update_bdu(struct sc7a20_data *acc, u8 new_bdu);
int sc7a20_update_ble(struct sc7a20_data *acc, u8 new_ble);
int sc7a20_update_hr(struct sc7a20_data *acc, u8 new_hr);

int sc7a20_update_odr(struct sc7a20_data *acc, int poll_interval_ms);

// static int sc7a20_register_write(struct sc7a20_data *acc, u8 * buf,
// 		u8 reg_address, u8 new_value);
static int sc7a20_register_read(struct sc7a20_data *acc, u8 * buf,
		u8 reg_address);
static int sc7a20_i2c_write(struct sc7a20_data *acc, u8 * buf, int len);

static int sc7a20_register_write(struct sc7a20_data *acc,u8 reg_address, u8 new_value)
{
	u8 buf[2];
	
	buf[0] = reg_address;
	buf[1] = new_value;
	return sc7a20_i2c_write(acc, buf, 1);
}


static int sc7a20_i2c_read(struct sc7a20_data *acc, u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buf,},
		{
			.addr = acc->client->addr,
			.flags = (acc->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = buf,},
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int sc7a20_i2c_write(struct sc7a20_data *acc, u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg msgs[] = { {.addr = acc->client->addr,
		.flags = acc->client->flags & I2C_M_TEN,
		.len = len + 1,.buf = buf,},
	};
	do {
		err = i2c_transfer(acc->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int sc7a20_hw_init(struct sc7a20_data *acc)
{
	int err = -1;
	u8 buf[2];

	printk("%s: hw init start\n", SC7A20_ACC_DEV_NAME);

	memset(acc->resume_state, 0, ARRAY_SIZE(acc->resume_state));

	acc->resume_state[RES_CTRL_REG1] = SC7A20_ENABLE_ALL_AXES;
	acc->resume_state[RES_CTRL_REG2] = 0x00;
	acc->resume_state[RES_CTRL_REG3] = 0x00;
	acc->resume_state[RES_CTRL_REG4] = 0x00;
	acc->resume_state[RES_CTRL_REG5] = 0x00;
	acc->resume_state[RES_CTRL_REG6] = 0x00;


	acc->g_range     = SC7A20_ACC_G_2G;
	acc->sensitivity = SENSITIVITY_2G;
	err = sc7a20_update_g_range(acc, acc->g_range); //CTRL_REG4 , 0x23  //acc->pdata->g_range
	if (err < 0) {
		dev_err(&acc->client->dev, "update_g_range failed\n");
		goto error1;
	}

	/* BDU set 1*/
	acc->bdu = 0x80;//CTRL_REG4 , 0x23
	err = sc7a20_update_bdu(acc, acc->bdu);
	if (err < 0) {
		dev_err(&acc->client->dev, "update_bdu failed\n");
		goto error1;
	}

	/* BLE set 0*/
	acc->ble = 0x00; //CTRL_REG4 , 0x23
	err = sc7a20_update_ble(acc, acc->ble);
	if (err < 0) {
		dev_err(&acc->client->dev, "update_ble failed\n");
		goto error1;
	}

	/* HR set 1*/
	acc->hr = 0x08;//CTRL_REG4 , 0x23
	err = sc7a20_update_hr(acc, acc->hr);
	if (err < 0) {
		dev_err(&acc->client->dev, "update_hr failed\n");
		goto error1;
	}


	acc->min_interval  = 20;
	acc->poll_interval = 20;	//CTRL_REG1 0x020
	err = sc7a20_update_odr(acc, acc->poll_interval);  //acc->pdata->poll_interval
	if (err < 0) {
		dev_err(&acc->client->dev, "update_odr failed\n");
		goto error1;
	}

	//initialized ,power down
	/*
	   buf[0] = CTRL_REG1;
	   buf[1] = acc->ctrl_reg1 & ~SC7A20_AC_MASK;
	   err = sc7a20_i2c_write(acc, buf, 1);

	   if (err < 0) {
	//dev_err(&acc->client->dev, "initialized ,power down failed\n");
	goto error1;
	}
	acc->active = ACC_STANDBY;
	*/
	acc->hw_initialized = 1;

	printk("%s: hw init done\n", SC7A20_ACC_DEV_NAME);
	return 0;

error1:
	acc->hw_initialized = 0;
	printk("hw init error 0x%x,0x%x: %d\n", buf[0],buf[1], err);
	return err;
}

static void sc7a20_device_power_off(struct sc7a20_data *acc)
{
	int err;
	u8 buf[2];

	//acc->ctrl_reg1 &= ~SC7A20_AC_MASK;
	printk("sprd-gsensor: -- %s -- !\n",__func__);
	buf[0] = CTRL_REG1;
	buf[1] = acc->ctrl_reg1 & ~SC7A20_AC_MASK;;
	err = sc7a20_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power off failed\n");
	if (acc->pdata->power_off) {
		if (acc->irq1 != 0)
			disable_irq_nosync(acc->irq1);
		if (acc->irq2 != 0)
			disable_irq_nosync(acc->irq2);
		acc->pdata->power_off();
		acc->hw_initialized = 0;
	}
	if (acc->hw_initialized) {
		if (acc->irq1 != 0)
			disable_irq_nosync(acc->irq1);
		if (acc->irq2 != 0)
			disable_irq_nosync(acc->irq2);
		acc->hw_initialized = 0;
	}
	acc->active = ACC_STANDBY;
}

static int sc7a20_device_power_on(struct sc7a20_data *acc)
{
	int err = -1;
	printk("sprd-gsensor: -- %s -- !\n",__func__);
	if (acc->pdata->power_on) {
		err = acc->pdata->power_on();
		if (err < 0) {
			dev_err(&acc->client->dev,
					"power_on failed: %d\n", err);
			return err;
		}
	}

	if (!acc->hw_initialized) {
		err = sc7a20_hw_init(acc);
		if (acc->hw_working == 1 && err < 0) {
			sc7a20_device_power_off(acc);
			return err;
		}
	}
	acc->active = ACC_ACTIVED;
	return 0;
}

static irqreturn_t sc7a20_isr1(int irq, void *dev)
{
	struct sc7a20_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq1_work_queue, &acc->irq1_work);

	pr_debug("%s: isr1 queued\n", SC7A20_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static irqreturn_t sc7a20_isr2(int irq, void *dev)
{
	struct sc7a20_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq2_work_queue, &acc->irq2_work);

	pr_debug("%s: isr2 queued\n", SC7A20_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static void sc7a20_irq1_work_func(struct work_struct *work)
{

	/*struct sc7a20_data *acc =
	  container_of(work, struct sc7a20_data, irq1_work);
	  */
	/* TODO  add interrupt service procedure.
ie:sc7a20_get_int1_source(acc); */
	;
	/*  */
	pr_debug("%s: IRQ1 triggered\n", SC7A20_ACC_DEV_NAME);
}

static void sc7a20_irq2_work_func(struct work_struct *work)
{

	/*struct sc7a20_data *acc =
	  container_of(work, struct sc7a20_data, irq2_work);
	  */
	/* TODO  add interrupt service procedure.
ie:sc7a20_get_tap_source(acc); */
	;
	/*  */

	pr_debug("%s: IRQ2 triggered\n", SC7A20_ACC_DEV_NAME);
}

int sc7a20_update_g_range(struct sc7a20_data *acc, u8 new_g_range)
{
	int err;

	u8 sensitivity;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = SC7A20_ACC_FS_MASK;

	pr_debug("%s\n", __func__);

	switch (new_g_range) {
		case SC7A20_ACC_G_2G:
			sensitivity = 4;
			break;
		case SC7A20_ACC_G_4G:

			sensitivity = 3;
			break;
		case SC7A20_ACC_G_8G:

			sensitivity = 2;
			break;
		case SC7A20_ACC_G_16G:

			sensitivity = 1;
			break;		
		default:
			dev_err(&acc->client->dev, "invalid g range requested: %u\n",
					new_g_range);
			return -EINVAL;
	}

	if (atomic_read(&acc->enabled)) {
		/* Set configuration register 4, which contains g range setting
		 *  NOTE: this is a straight overwrite because this driver does
		 *  not use any of the other configuration bits in this
		 *  register.  Should this become untrue, we will have to read
		 *  out the value and only change the relevant bits --XX----
		 *  (marked by X) */

		err = sc7a20_register_read(acc, &init_val, CTRL_REG4);
		if (err < 0) {
			printk("%s, error read register CTRL_REG4\n", __func__);
			return -EAGAIN;
		}	

		new_val = new_g_range;
		updated_val = ((mask & new_val) | ((~mask) & init_val));

		buf[0] = CTRL_REG4;
		buf[1] = updated_val;
		err = sc7a20_i2c_write(acc, buf, 1);
		if (err < 0)
			dev_err(&acc->client->dev, "soft power off failed\n");

		acc->resume_state[RES_CTRL_REG4] = updated_val;	   
		acc->sensitivity = sensitivity;

		pr_debug("%s sensitivity %d g-range %d\n", __func__, sensitivity,new_g_range);
	}

	return 0;
}

int sc7a20_update_bdu(struct sc7a20_data *acc, u8 new_bdu)
{
	int err = -1;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = SC7A20_ACC_BDU_MASK;


	err = sc7a20_register_read(acc, &init_val, CTRL_REG4);
	if (err < 0) {
		printk("%s, error read register CTRL_REG4\n", __func__);
		return -EAGAIN;
	}	

	new_val = new_bdu;	
	updated_val = ((mask & new_val) | ((~mask) & init_val));

	buf[0] = CTRL_REG4;
	buf[1] = updated_val;
	err = sc7a20_i2c_write(acc, buf, 1);

	if (err < 0)
		goto error;
	acc->resume_state[RES_CTRL_REG4] = updated_val;

	return 0;

error:
	dev_err(&acc->client->dev, "update bdu failed 0x%x,0x%x: %d\n",
			buf[0], buf[1], err);
	return err;	
}

int sc7a20_update_ble(struct sc7a20_data *acc, u8 new_ble)
{
	int err = -1;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = SC7A20_ACC_BLE_MASK;



	err = sc7a20_register_read(acc, &init_val, CTRL_REG4);
	if (err < 0) {
		printk("%s, error read register CTRL_REG4\n", __func__);
		return -EAGAIN;
	}

	new_val = new_ble;	
	updated_val = ((mask & new_val) | ((~mask) & init_val));


	buf[0] = CTRL_REG4;
	buf[1] = updated_val;
	err = sc7a20_i2c_write(acc, buf, 1);

	if (err < 0)
		goto error;
	acc->resume_state[RES_CTRL_REG4] = updated_val;

	return 0;

error:
	dev_err(&acc->client->dev, "update ble failed 0x%x,0x%x: %d\n",
			buf[0], buf[1], err);
	return err;	
}


int sc7a20_update_hr(struct sc7a20_data *acc, u8 new_hr)
{
	int err = -1;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = SC7A20_ACC_HR_MASK;


	err = sc7a20_register_read(acc, &init_val, CTRL_REG4);
	if (err < 0) {
		printk("%s, error read register CTRL_REG4\n", __func__);
		return -EAGAIN;
	}

	new_val = new_hr;	
	updated_val = ((mask & new_val) | ((~mask) & init_val));

	buf[0] = CTRL_REG4;
	buf[1] = updated_val;
	err = sc7a20_i2c_write(acc, buf, 1);

	if (err < 0)
		goto error;
	acc->resume_state[RES_CTRL_REG4] = updated_val;

	return 0;

error:
	dev_err(&acc->client->dev, "update hr failed  0x%x,0x%x: %d\n",
			buf[0], buf[1], err);
	return err;	
}


int sc7a20_update_odr(struct sc7a20_data *acc, int poll_interval_ms)
{
	int err = -1;
	int i;
	u8 buf[2];
	u8 new_value;

	for (i = ARRAY_SIZE(sc7a20_odr_table) - 1; i >= 0; i--) {
		if (sc7a20_odr_table[i].cutoff_ms <= poll_interval_ms)
			break;
	}

	new_value = sc7a20_odr_table[i].mask;

	new_value |= SC7A20_ENABLE_ALL_AXES;

	buf[0] = CTRL_REG1;
	buf[1] = new_value;
	err = sc7a20_i2c_write(acc, buf, 1);
	if (err < 0)
		return err;

	acc->resume_state[RES_CTRL_REG1] = new_value;
	acc->ctrl_reg1 = new_value;


	return 0;

}

/* */

// static int sc7a20_register_write(struct sc7a20_data *acc, u8 * buf,
// 		u8 reg_address, u8 new_value)
// {
// 	int err = -1;

// 	if (atomic_read(&acc->enabled)) {
// 		/* Sets configuration register at reg_address
// 		 *  NOTE: this is a straight overwrite  */
// 		buf[0] = reg_address;
// 		buf[1] = new_value;
// 		err = sc7a20_i2c_write(acc, buf, 1);
// 		if (err < 0)
// 			return err;
// 	}
// 	return err;
// }

static int sc7a20_register_read(struct sc7a20_data *acc, u8 * buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = sc7a20_i2c_read(acc, buf, 1);
	return err;
}

// static int sc7a20_register_update(struct sc7a20_data *acc, u8 * buf,
// 		u8 reg_address, u8 mask,
// 		u8 new_bit_values)
// {
// 	int err = -1;
// 	u8 init_val;
// 	u8 updated_val;
// 	err = sc7a20_register_read(acc, buf, reg_address);
// 	if (!(err < 0)) {
// 		init_val = buf[0];   //djq,2016-09-21  old buf[1];  changed buf[0]
// 		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
// 		err = sc7a20_register_write(acc, buf, reg_address,
// 				updated_val);
// 	}
// 	return err;
// }

/* */
int xyzBuf[3];


static int sc7a20_get_acceleration_data(struct sc7a20_data *acc,
		int *xyz)
{
	int err = -1;
	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	u8  buf[2];
	u8  acc_data[6];
	s16 hw_d[3] = { 0 };
	//	acc_data
	/* x,y,z hardware data */
	//printk("sprd-gsensor: -- %s --  \n",__func__);

	///

	err = sc7a20_register_read(acc, buf, STATUS_REG);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
	{
		if ((buf[0] & 0x0f) != 0x0f)
		{
			xyz[0] = xyzBuf[0];
			xyz[1] = xyzBuf[1];
			xyz[2] = xyzBuf[2];
			printk("%s read not ready\n",__func__);
			return err;
		}
	}

	err = sc7a20_register_read(acc, buf, XOUT_L);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[0] = buf[0];

	err = sc7a20_register_read(acc, buf, XOUT_H);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[1] = buf[0];	

	err = sc7a20_register_read(acc, buf, YOUT_L);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[2] = buf[0];

	err = sc7a20_register_read(acc, buf, YOUT_H);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[3] = buf[0];			

	err = sc7a20_register_read(acc, buf, ZOUT_L);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[4] = buf[0];

	err = sc7a20_register_read(acc, buf, ZOUT_H);
	if (err < 0)
		dev_err(&acc->client->dev, "accelerometer data read failed\n");
	else
		acc_data[5] = buf[0];

	hw_d[0] =  ((acc_data[1] << 8) & 0xff00) | acc_data[0];
	hw_d[1] =  ((acc_data[3] << 8) & 0xff00) | acc_data[2];
	hw_d[2] =  ((acc_data[5] << 8) & 0xff00) | acc_data[4];

//	printk("%s read x_h=%d, y_h=%d, z_h=%d \n",
//			__func__, acc_data[1], acc_data[3], acc_data[5]);

	hw_d[0] = ((s16) hw_d[0] ) >> acc->sensitivity;///4
	hw_d[1] = ((s16) hw_d[1] ) >> acc->sensitivity;
	hw_d[2] = ((s16) hw_d[2] ) >> acc->sensitivity;

	xyz[0] = ((acc->pdata->negate_x) ? (hw_d[acc->pdata->axis_map_x]) : (-hw_d[acc->pdata->axis_map_x]));
	xyz[1] = ((acc->pdata->negate_y) ? (hw_d[acc->pdata->axis_map_y]) : (-hw_d[acc->pdata->axis_map_y]));
	xyz[2] = ((acc->pdata->negate_z) ? (hw_d[acc->pdata->axis_map_z]) : (-hw_d[acc->pdata->axis_map_z]));

//	printk("%s read x=%d, y=%d, z=%d err =%d\n",__func__, xyz[0], xyz[1], xyz[2],err);

	
#ifdef GSENSOR_CALI_SUPPORT
	xyz[0] -= accel_cali[0];
	xyz[1] -= accel_cali[1];
	xyz[2] -= accel_cali[2];
#endif

	xyzBuf[0] = xyz[0];
	xyzBuf[1] = xyz[1];
	xyzBuf[2] = xyz[2];

	return err;
}

static void sc7a20_report_values(struct sc7a20_data *acc, int *xyz)
{
	#if 0
	input_report_rel(acc->input_dev, ABS_X, xyz[0]);
	input_report_rel(acc->input_dev, ABS_Y, xyz[1]);
	input_report_rel(acc->input_dev, ABS_Z, xyz[2]);
	#else
	input_report_abs(acc->input_dev, ABS_X, xyz[0]);
	input_report_abs(acc->input_dev, ABS_Y, xyz[1]);
	input_report_abs(acc->input_dev, ABS_Z, xyz[2]);
	#endif
	input_sync(acc->input_dev);
}

#ifdef GSENSOR_CALI_SUPPORT
static int read_factory_calibration(int *buf);
#endif

static int sc7a20_enable(struct sc7a20_data *acc)
{
	int err;

	printk("sprd-gsensor: -- %s -- !\n",__func__);

	if (!atomic_cmpxchg(&acc->enabled, 0, 1))
	{
#ifdef GSENSOR_CALI_SUPPORT
		if (sc7a20_misc_data->get_cali_flag == 0)
		{
			read_factory_calibration(accel_cali);
			sc7a20_misc_data->get_cali_flag = 1;
		}
#endif
		//printk("techain_sprd-gsensor: -- %s -- !acc->hw_initialized =%d  state 1\n",__func__,acc->hw_initialized);
		err = sc7a20_device_power_on(acc);
		if (err < 0)
		{
			atomic_set(&acc->enabled, 0);
			return err;
		}
		//printk("techain_sprd-gsensor: -- %s -- !acc->hw_initialized =%d  state 2\n",__func__,acc->hw_initialized);
		if (acc->hw_initialized)
		{
			if (acc->irq1 != 0)
				enable_irq(acc->irq1);
			if (acc->irq2 != 0)
				enable_irq(acc->irq2);
			pr_debug("%s: power on: irq enabled\n",	SC7A20_ACC_DEV_NAME);
		}

		schedule_delayed_work(&acc->input_work,	msecs_to_jiffies(acc->pdata->poll_interval));
	}

	printk("sprd-gsensor: -- %s -- success!\n",__func__);
	return 0;
}

static int sc7a20_disable(struct sc7a20_data *acc)
{
	printk("sprd-gsensor: -- %s -- \n",__func__);
	if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
		cancel_delayed_work_sync(&acc->input_work);
		sc7a20_device_power_off(acc);
	}

	return 0;
}

static int sc7a20_misc_open(struct inode *inode, struct file *file)
{
	int err;
	err = nonseekable_open(inode, file);
	if (err < 0)
		return err;

	file->private_data = sc7a20_misc_data;

	return 0;
}

static long sc7a20_misc_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	u8 buf[4];
	//u8 mask;
	//u8 reg_address;
	u8 bit_values;
	int err;
	int interval;
	int xyz[3] = { 0 };
	struct sc7a20_data *acc = file->private_data;
	printk("sprd-gsensor: -- %s -- \n",__func__);

	switch (cmd) {
		case SC7A20_ACC_IOCTL_GET_DELAY:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_GET_DELAY \n",__func__);
			interval = acc->pdata->poll_interval;
			if (copy_to_user(argp, &interval, sizeof(interval)))
				return -EFAULT;
			break;

		case SC7A20_ACC_IOCTL_SET_DELAY:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_SET_DELAY \n",__func__);
			if (copy_from_user(&interval, argp, sizeof(interval)))
				return -EFAULT;
			if (interval < 0 || interval > 1000)
				return -EINVAL;

			acc->pdata->poll_interval = max(interval,
					acc->pdata->min_interval);
			err = sc7a20_update_odr(acc, acc->pdata->poll_interval);
			/* TODO: if update fails poll is still set */
			if (err < 0)
				return err;
			break;

		case SC7A20_ACC_IOCTL_SET_ENABLE:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_SET_ENABLE \n",__func__);
			if (copy_from_user(&interval, argp, sizeof(interval)))
				return -EFAULT;
			if (interval > 1)
				return -EINVAL;
			if (interval)
				err = sc7a20_enable(acc);
			else
				err = sc7a20_disable(acc);
			return err;
			break;

		case SC7A20_ACC_IOCTL_GET_ENABLE:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_GET_ENABLE \n",__func__);
			interval = atomic_read(&acc->enabled);
			if (copy_to_user(argp, &interval, sizeof(interval)))
				return -EINVAL;
			break;

		case SC7A20_ACC_IOCTL_SET_G_RANGE:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_SET_G_RANGE \n",__func__);
			if (copy_from_user(buf, argp, 1))
				return -EFAULT;
			bit_values = buf[0];
			err = sc7a20_update_g_range(acc, bit_values);
			if (err < 0)
				return err;
			break;

		case SC7A20_ACC_IOCTL_GET_COOR_XYZ:
			printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_GET_COOR_XYZ \n",__func__);
			err = sc7a20_get_acceleration_data(acc, xyz);
			if (err < 0)
				return err;

			if (copy_to_user(argp, xyz, sizeof(xyz))) {
				pr_err(" %s %d error in copy_to_user \n",
						__func__, __LINE__);
				return -EINVAL;
			}
			break;
		case SC7A20_ACC_IOCTL_GET_CHIP_ID:
			{

				u8 devid = 0;
				u8 devinfo[DEVICE_INFO_LEN] = {0};
				printk("sprd-gsensor: -- %s -- SC7A20_ACC_IOCTL_GET_CHIP_ID \n",__func__);
				err = sc7a20_register_read(acc, &devid, WHO_AM_I);
				if (err < 0) {
					printk("%s, error read register WHO_AM_I\n", __func__);
					return -EAGAIN;
				}
				sprintf(devinfo, "%s, %#x", DEVICE_INFO, devid);

				if (copy_to_user(argp, devinfo, sizeof(devinfo))) {
					printk("%s error in copy_to_user(IOCTL_GET_CHIP_ID)\n", __func__);
					return -EINVAL;
				}
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations sc7a20_misc_fops = {
	.owner = THIS_MODULE,
	.open = sc7a20_misc_open,
	.unlocked_ioctl = sc7a20_misc_ioctl,
};

static struct miscdevice sc7a20_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SC7A20_ACC_DEV_NAME,
	.fops = &sc7a20_misc_fops,
};

static ssize_t sc7a20_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	//struct input_dev *input_dev = dev_get_drvdata(dev);
	struct sc7a20_data *acc = sc7a20_misc_data;
	int err;	
	u8 val;
	int enable;

	mutex_lock(&acc->lock);
	err = sc7a20_register_read(acc, &val, CTRL_REG1);  
	if (err < 0) {
		printk("%s, error read register CTRL_REG1\n", __func__);
		return -EAGAIN;
	}
	if (val & SC7A20_AC_MASK)
		enable = 1;
	else
		enable = 0;
	mutex_unlock(&acc->lock);

	return sprintf(buf, "%d\n", enable);
}

static ssize_t sc7a20_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	//struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct sc7a20_data *acc = sc7a20_misc_data;

	int err;
	unsigned long enable;
	u8 buffer[2];

	enable = simple_strtoul(buf, NULL, 10); 	   
	mutex_lock(&acc->lock);
	enable = (enable > 0) ? 1 : 0;
	if (enable && acc->active == ACC_STANDBY)
	{  	   
		buffer[0] = CTRL_REG1;
		buffer[1] = acc->ctrl_reg1;
		err = sc7a20_i2c_write(acc, buffer, 1);  
		if (err < 0) {
			printk("%s, set enable failed!\n",__func__);
		}
		else
		{
			acc->active = ACC_ACTIVED;
			printk("sc7a20 enable setting valid \n");
		}
	}
	else if (enable == 0  && acc->active == ACC_ACTIVED)
	{
		buffer[0] = CTRL_REG1;
		buffer[1] = acc->ctrl_reg1 & ~SC7A20_AC_MASK;
		err = sc7a20_i2c_write(acc, buffer, 1);  
		if (err < 0) {
			printk("%s, set disable failed!\n",__func__);
		}
		else
		{
			acc->active = ACC_STANDBY;
			printk("sc7a20 disable setting valid \n");
		}
	}
	mutex_unlock(&acc->lock);

	return count;
}

static ssize_t sc7a20_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	//struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct sc7a20_data *acc = sc7a20_misc_data;

	int err;
	u8 value;
	u8 i;
	ssize_t count = 0;

	mutex_lock(&acc->lock);	
	for (i = 0x20; i < 0x58; i++) {
		err = sc7a20_register_read(acc, &value, i); 
		if (err < 0)	
		{
			printk("read register 0x%x error\n",i);
		}			
		count += sprintf(&buf[count], "0x%x: 0x%x\n", i, value);		
	}		
	mutex_unlock(&acc->lock);

	return count;          
}

/*----------------------------------------------------------------------------*/
static ssize_t sc7a20_reg_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	//struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct sc7a20_data *acc = sc7a20_misc_data;

	int err;
	u8 address, value;
	u8 buffer[2];

	sscanf(buf, "0x%hhx=0x%hhx", &address, &value);

	buffer[0] = address;
	buffer[1] = value;
	err = sc7a20_i2c_write(acc, buffer, 1);  
	if (err < 0) {
		printk("%s, set register failed!\n",__func__);
	}

	return count;            
}

static void sc7a20_input_work_func(struct work_struct *work)
{
	struct sc7a20_data *acc;

	int xyz[3] = { 0 };
	int err;

	acc = container_of((struct delayed_work *)work,
			struct sc7a20_data, input_work);

	mutex_lock(&acc->lock);
	err = sc7a20_get_acceleration_data(acc, xyz);
	if (err < 0)
		dev_err(&acc->client->dev, "get_acceleration_data failed\n");
	else
		sc7a20_report_values(acc, xyz);

	schedule_delayed_work(&acc->input_work,
			msecs_to_jiffies(acc->pdata->poll_interval));
	mutex_unlock(&acc->lock);
}

#ifdef SC7A20_OPEN_ENABLE
int sc7a20_input_open(struct input_dev *input)
{
	struct sc7a20_data *acc = input_get_drvdata(input);

	return sc7a20_enable(acc);
}

void sc7a20_input_close(struct input_dev *dev)
{
	struct sc7a20_data *acc = input_get_drvdata(dev);

	sc7a20_disable(acc);
}
#endif

static int sc7a20_validate_pdata(struct sc7a20_data *acc)
{
	acc->pdata->poll_interval = max(acc->pdata->poll_interval,
			acc->pdata->min_interval);

	acc->pdata->poll_interval = 100;

	if (acc->pdata->axis_map_x > 2 || acc->pdata->axis_map_y > 2
			|| acc->pdata->axis_map_z > 2) {
		dev_err(&acc->client->dev, "invalid axis_map value "
				"x:%u y:%u z%u\n", acc->pdata->axis_map_x,
				acc->pdata->axis_map_y, acc->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (acc->pdata->negate_x > 1 || acc->pdata->negate_y > 1
			|| acc->pdata->negate_z > 1) {
		dev_err(&acc->client->dev, "invalid negate value "
				"x:%u y:%u z:%u\n", acc->pdata->negate_x,
				acc->pdata->negate_y, acc->pdata->negate_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (acc->pdata->poll_interval < acc->pdata->min_interval) {
//		dev_err(&acc->client->dev, "minimum poll interval violated\n");
//		return -EINVAL;
	}

	return 0;
}

static int sc7a20_input_init(struct sc7a20_data *acc)
{
	int err;
	/* Polling rx data when the interrupt is not used.*/
	if (1 /*acc->irq1 == 0 && acc->irq1 == 0 */ ) {
		INIT_DELAYED_WORK(&acc->input_work, sc7a20_input_work_func);
	}

	acc->input_dev = input_allocate_device();
	if (!acc->input_dev) {
		err = -ENOMEM;
		dev_err(&acc->client->dev, "input device allocate failed\n");
		goto err0;
	}
#ifdef SC7A20_ACC_OPEN_ENABLE
	acc->input_dev->open = sc7a20_input_open;
	acc->input_dev->close = sc7a20_input_close;
#endif

	input_set_drvdata(acc->input_dev, acc);

//	set_bit(EV_REL, acc->input_dev->relbit);

	set_bit(EV_ABS, acc->input_dev->evbit);
	/*      next is used for interruptA sources data if the case */
	set_bit(ABS_MISC, acc->input_dev->absbit);
	/*      next is used for interruptB sources data if the case */
	set_bit(ABS_WHEEL, acc->input_dev->absbit);
	/*
	   input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, FUZZ, FLAT);
	   input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, FUZZ, FLAT);
	   input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, FUZZ, FLAT);
	   */	
	input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, 0, 0);
	input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, 0, 0);
	input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, 0, 0);


	/*      next is used for interruptA sources data if the case */
	input_set_abs_params(acc->input_dev, ABS_MISC, INT_MIN, INT_MAX, 0, 0);
	/*      next is used for interruptB sources data if the case */
	input_set_abs_params(acc->input_dev, ABS_WHEEL, INT_MIN, INT_MAX, 0, 0);

	acc->input_dev->name = "accelerometer";

	err = input_register_device(acc->input_dev);
	if (err) {
		dev_err(&acc->client->dev,
				"unable to register input polled device %s\n",
				acc->input_dev->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(acc->input_dev);
err0:
	return err;
}

static void sc7a20_input_cleanup(struct sc7a20_data *acc)
{
	input_unregister_device(acc->input_dev);
	input_free_device(acc->input_dev);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void sc7a20_early_suspend(struct early_suspend *es);
static void sc7a20_early_resume(struct early_suspend *es);
#endif

#ifdef CONFIG_OF
static struct sc7a20_platform_data *sc7a20_parse_dt(struct device *dev)
{
	struct sc7a20_platform_data *pdata;
	struct device_node *np = dev->of_node;
	int ret;
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		dev_err(dev, "Could not allocate struct sc7a20_platform_data");
		return NULL;
	}
	ret = of_property_read_u32(np, "poll_interval", &pdata->poll_interval);
	if (ret) {
		dev_err(dev, "fail to get poll_interval\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "min_interval", &pdata->min_interval);
	if (ret) {
		dev_err(dev, "fail to get min_interval\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "g_range", &pdata->g_range);
	if (ret) {
		dev_err(dev, "fail to get g_range\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "axis_map_x", &pdata->axis_map_x);
	if (ret) {
		dev_err(dev, "fail to get axis_map_x\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "axis_map_y", &pdata->axis_map_y);
	if (ret) {
		dev_err(dev, "fail to get axis_map_y\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "axis_map_z", &pdata->axis_map_z);
	if (ret) {
		dev_err(dev, "fail to get axis_map_z\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "negate_x", &pdata->negate_x);
	if (ret) {
		dev_err(dev, "fail to get negate_x\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "negate_y", &pdata->negate_y);
	if (ret) {
		dev_err(dev, "fail to get negate_y\n");
		goto fail;
	}
	ret = of_property_read_u32(np, "negate_z", &pdata->negate_z);
	if (ret) {
		dev_err(dev, "fail to get negate_z\n");
		goto fail;
	}
    printk("%s: %d,%d,%d[%d,%d,%d][%d,%d,%d]",__FUNCTION__,pdata->poll_interval,pdata->min_interval,
        pdata->g_range,pdata->axis_map_x,pdata->axis_map_y,pdata->axis_map_z,
        pdata->negate_x,pdata->negate_y,pdata->negate_z);
	return pdata;
fail:
	kfree(pdata);
	return NULL;
}
#endif

#ifdef GSENSOR_CALI_SUPPORT
static int write_factory_calibration(int *buf)
{
	struct file *fp_cal;

	mm_segment_t fs;
	loff_t pos;

	printk("[%s]: start: \n", __func__);

	pos = 0;

	fp_cal = filp_open(GSENSOR_CAL_FILE_PATH, O_CREAT|O_RDWR|O_TRUNC, 0660);
	if (IS_ERR(fp_cal))
	{
		printk(" %s create file %s error\n", __func__, GSENSOR_CAL_FILE_PATH);
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(fp_cal, (char*)buf, 12, &pos);

	filp_close(fp_cal, NULL);

	set_fs(fs);

	printk("[%s]: end-> %d,%d,%d,%d  \n", __func__,buf[0],buf[1],buf[2],sizeof(buf));
	return 0;
}


static int read_factory_calibration(int *buf)
{
	//struct i2c_client *client = epl_data->client;
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;

	printk("[%s]: start \n", __func__);

	fp = filp_open(GSENSOR_CAL_FILE_PATH, O_RDONLY, (S_IRUSR|S_IRGRP));

	if (IS_ERR(fp))
	{
		printk("open Gsensor calibration file fail!!(%d)\n", (int)IS_ERR(fp));
		return -EINVAL;
	}
	else
	{
		pos = 0;
		fs = get_fs();
		set_fs(KERNEL_DS);
		vfs_read(fp, (char *)buf, 12, &pos);
		filp_close(fp, NULL);
		set_fs(fs);

		printk("[%s]: x =%d, y=%d z=%d\n", __func__, *buf,*(buf+1),*(buf+2));
	}

	printk("[%s]: end \n", __func__);
	return 0;
}

static int gsensor_acc_cali_data(int sample_cnt, int *buf)
{
	int i=0;
	int sum_x=0,sum_y=0,sum_z=0;
	int xyz_data[3];

	if ((sc7a20_misc_data==NULL) || (buf==NULL) || (sample_cnt==0))
	{
		return -1;
	}

	accel_cali[0]=0;
	accel_cali[1]=0;
	accel_cali[2]=0;

	for (i=0; i<sample_cnt; i++) {
		if (sc7a20_get_acceleration_data(sc7a20_misc_data, &xyz_data[0]) < 0)
		{
			return -1;
		}
		sum_x += xyz_data[0]; //x raw data
		sum_y += xyz_data[1];
		sum_z += xyz_data[2];
		msleep(20);
	}

	*buf++ = sum_x/sample_cnt;
	*buf++ = sum_y/sample_cnt;
	*buf = sum_z/sample_cnt - ((sum_z<0)? (-1024):1024);

	return 0;
}

static ssize_t gsensor_cali_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	int *cali_ptr = accel_cali;

	//printk("%s\n",__func__);

	if (gsensor_acc_cali_data(5, cali_ptr) == 0)
	{
		//printk("%s: x=%d,y=%d,z=%d\n",__func__,*cali_ptr,*(accel_cali+1),*(accel_cali+2));

		if (write_factory_calibration(cali_ptr) < 0)
		{
			return -1;
		}
		else
		{
			sc7a20_misc_data->get_cali_flag = 1;
			return count;
		}
	}
	else
	{
		return -1;
	}
}

static ssize_t gsensor_cali_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	int *cali_ptr = accel_cali;

	if (read_factory_calibration(cali_ptr) < 0)
	{
		printk("%s failed\n", __func__);
		return 0;
	}

	//printk("%s: #### x=%d,y=%d,z=%d\n",__func__,*cali_ptr,*(cali_ptr+1),*(cali_ptr+2));

	return sprintf(buffer,"x=%d,y=%d,z=%d\n",*cali_ptr,*(cali_ptr+1),*(cali_ptr+2));
}

static int gsensor_cali_open(struct inode *inode, struct file *f)
{
	return 0;
}

static int gsensor_cali_release(struct inode *inode, struct file *f)
{
	return 0;
}

static const struct file_operations gsensor_cali_fops =
{
	.owner = THIS_MODULE,
	.open = gsensor_cali_open,
	.write = gsensor_cali_write,
	.read = gsensor_cali_read,
	.release = gsensor_cali_release
};

static struct miscdevice gsensor_cali_struct = {
	.name = "gsensor_cali",
	.fops = &gsensor_cali_fops,
	.minor = MISC_DYNAMIC_MINOR,
};
#endif

typedef enum
{
	GSensor_Reg_CHIPID			= 0x0F,
	GSensor_Reg_CTRL_REG1		= 0x20,
	GSensor_Reg_CTRL_REG2		= 0x21,
	GSensor_Reg_CTRL_REG3		= 0x22,
	GSensor_Reg_CTRL_REG4		= 0x23,//GSensor_Reg_CTRL4 			= 0x23,
	GSensor_Reg_CTRL_REG5		= 0x24,
	GSensor_Reg_CTRL_REG6		= 0x25,
	GSensor_Reg_OUT_X_L			= 0x28,
	GSensor_Reg_OUT_X_H			= 0x29,
	GSensor_Reg_OUT_Y_L			= 0x2A,
	GSensor_Reg_OUT_Y_H			= 0x2B,
	GSensor_Reg_OUT_Z_L			= 0x2C,
	GSensor_Reg_OUT_Z_H			= 0x2D,
	GSensor_Reg_CLICK_CFG		= 0x38,
	GSensor_Reg_CLICK_CFG1		= 0x39,
	GSensor_Reg_CLICK_CFG2		= 0x3A,
	GSensor_Reg_TIME_LIMIT		= 0x3B,
	GSensor_Reg_TIME_LATENC		= 0x3C,
	GSensor_Reg_INVALID
	
}GSENSOR_REG;



static void GsensorSc7a20_Open_interrupt(int sensitivity,int OnOff)
{
	//
	int err;
//	u8 uTmp = 0,buf[2];
//	u8 uIntSens[4] = {0x0c/*192mg*/, 0x18/*384mg*/, 0x30/*768mg*/,0x6c/*960mg*/};
	
	if (!OnOff)
	{		
		//Close Interrupt
	//	err = sc7a20_register_write(sc7a20_misc_data,CTRL_REG2, 0x00);
		err = sc7a20_register_write(sc7a20_misc_data,0x22, 0x00);
		printk("power off write:reg=0x22 data:0 ,err=0x%x \n", err);
		err = sc7a20_register_write(sc7a20_misc_data,GSensor_Reg_CLICK_CFG, 0x00);
		printk("power off write:reg=0x38 data:0err=0x%x \n", err);
	}
	else
	{		
		#if 1
		switch(OnOff)
	        {

				case 1://POWER_ON_GSNR_SENS_LEVEL0:
				case 2:	
					sc7a20_register_write(sc7a20_misc_data, 0x20, 0x27);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG6, 0x00);  //High Interrupt Triggler
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG2, 0x40); //0x30//Interrup thresold
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LIMIT, 0x7f);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LATENC, 0x8f); //Interrup Status Keep time
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG, 0x15);   //X//Y/Z Interrup Detect Enable
					sc7a20_register_write(sc7a20_misc_data, 0x30, 0x15); 
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG3, 0x80);   //Enable Interrupt
	        	case 5://POWER_ON_GSNR_SENS_LEVEL2:
	        	case 6:
					sc7a20_register_write(sc7a20_misc_data, 0x20, 0x27);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG6, 0x00);   //High Interrupt Triggler
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG2, 0x60); //0x50 //Interrup thresold
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LIMIT, 0x7f);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LATENC, 0x8f); //Interrup Status Keep time
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG, 0x15);   //X//Y/Z Interrup Detect Enable
					sc7a20_register_write(sc7a20_misc_data, 0x30, 0x15); 
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG3, 0x80);   //Enable Interrupt
	        	break;					
				case 3:
	        	case 4://POWER_ON_GSNR_SENS_LEVEL1:
	        	default :
					sc7a20_register_write(sc7a20_misc_data, 0x20, 0x27);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG6, 0x00);   //High Interrupt Triggler
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG2, 0x50);  //0x40 //0x3A,Interrup thresold
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LIMIT, 0x7f);
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_TIME_LATENC, 0x8f); //Interrup Status Keep time
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CLICK_CFG, 0x15);   //X//Y/Z Interrup Detect Enable
					sc7a20_register_write(sc7a20_misc_data, 0x30, 0x15); 
					
					sc7a20_register_write(sc7a20_misc_data, GSensor_Reg_CTRL_REG3, 0x80);   //Enable Interrupt
	        	break;
	        	
	        	break;
	    }
		#else
		err = sc7a20_register_write(sc7a20_misc_data,CTRL_REG6, 0x02);//0x02==>LOW Valid?��?0x00==>High Valid
		
		err = sc7a20_register_write(sc7a20_misc_data,CTRL_REG1, 0x47);//DR 0x37:25hz,0x47:50hz,

		err = sc7a20_register_read(sc7a20_misc_data,buf,0x26);
		uTmp = buf[0];

		err = sc7a20_register_write(sc7a20_misc_data,CTRL_REG2, 0x0D);//??������1?��

		err = sc7a20_register_write(sc7a20_misc_data,CTRL_REG4, 0x80);//��?3��+-2g

		switch(sensitivity)
		{
			case 1:
				sc7a20_register_write(sc7a20_misc_data,0x3A,  uIntSens[0]);//�䣤����?D?��,??????2?����??
				break;
			case 2:
				sc7a20_register_write(sc7a20_misc_data,0x3A,  uIntSens[1]);//�䣤����?D?��
				break;
			case 3:
				sc7a20_register_write(sc7a20_misc_data,0x3A,  uIntSens[2]);//�䣤����?D?��
				break;
			case 4:
				sc7a20_register_write(sc7a20_misc_data,0x3A,  uIntSens[3]);//�䣤����?D?��
				break;
			default:
				sc7a20_register_write(sc7a20_misc_data,0x3A,  uIntSens[1]);//�䣤����?D?��
				break;
		}

		sc7a20_register_write(sc7a20_misc_data,0x3B,  0x7F);
		sc7a20_register_write(sc7a20_misc_data,0x3C, 0x64);//
		sc7a20_register_write(sc7a20_misc_data,0x38,  0x15);
		
		sc7a20_register_write(sc7a20_misc_data,0x22,  0x80);//enable interrupt
		//clear latch and set latch mode
		sc7a20_register_read(sc7a20_misc_data,buf, 0x39);
		uTmp = buf[0];
		#endif
	}

	
}

static int int2_enable = 0;
static int int2_statu = 0;


static ssize_t Gsensor_int2_enable_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    int on  ;
    int num ;
   // MIR_HANDLE handle = mir_handle;

    num = 0;
    int2_enable = simple_strtoul(buf, NULL, 10);
    on  = int2_enable;
    printk("sc7a20_int2_enable_store num:%d on:%d \n", num, on);
	
	GsensorSc7a20_Open_interrupt(2,int2_enable);
    return count;
}

static ssize_t Gsensor_int2_enable_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int ret;
    ret = sprintf(buf, "%d\n", int2_enable);
    printk(" mir3da_int2_enable_show ret [ %d ]\n",ret);
    return ret;
}

static ssize_t mir3da_int2_start_statu_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    int2_statu = simple_strtoul(buf, NULL, 10);
    return count;
}

static ssize_t mir3da_int2_start_statu_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int ret;
    ret = sprintf(buf, "%d\n", int2_statu);
    printk(" mir3da_int2_enable_show ret [ %d ]\n",ret);
    return ret;
}



static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO,
		sc7a20_enable_show, sc7a20_enable_store);
static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO,
		sc7a20_reg_show, sc7a20_reg_store);

static DEVICE_ATTR(int2_enable, 0660,  Gsensor_int2_enable_show,     Gsensor_int2_enable_store);
static DEVICE_ATTR(int2_start_status,     0660,  mir3da_int2_start_statu_show,   mir3da_int2_start_statu_store);


static struct attribute *sc7a20_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_reg.attr,
	&dev_attr_int2_enable.attr,
	&dev_attr_int2_start_status.attr,
	NULL
};

static const struct attribute_group sc7a20_attr_group = {
	.attrs = sc7a20_attributes,
};


static int sc7a20_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	struct sc7a20_data *acc;
	struct sc7a20_platform_data *pdata = client->dev.platform_data;
	static int ok=-1;
	int err = -1;
	int tempvalue;
	struct device_node *np;

	printk("sprd-gsensor: -- %s -%x- start !\n", __func__, client->addr);
	printk("%s: probe start.\n", SC7A20_ACC_DEV_NAME);
	if (ok==1)
	{
		printk("sprd-gsensor: -- %s --already ok !\n", __func__);

		return 0;
	}
#ifdef CONFIG_OF
	np = client->dev.of_node;
	if (np && !pdata) {
		pdata = sc7a20_parse_dt(&client->dev);
		if (pdata) {
			client->dev.platform_data = pdata;
		}
		if (!pdata) {
			err = -ENOMEM;
			goto exit_alloc_platform_data_failed;
		}
	}
#endif
	/*
	   if (client->dev.platform_data == NULL) {
	   dev_err(&client->dev, "platform data is NULL. exiting.\n");
	   err = -ENODEV;
	   goto exit_check_functionality_failed;
	   }
	   */
	client->addr = SC7A20_ACC_I2C_ADDR;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE |
				I2C_FUNC_SMBUS_BYTE_DATA |
				I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
		dev_err(&client->dev, "client not smb-i2c capable:3\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}

	/*
	 * OK. From now, we presume we have a valid client. We now create the
	 * client structure, even though we cannot fill it completely yet.
	 */
	tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);
	//sc7a20_register_read(acc, (u8 *)buf, WHO_AM_I);
    printk("###tempvalue=0x%x,addr=0x%x\n",tempvalue,client->addr);
	if ((tempvalue & 0x00FF) == WHOAMI_SC7A20_ACC) {
		printk( "%s I2C driver registered!\n",
				SC7A20_ACC_DEV_NAME);
	} else {
		//	acc->client = NULL;
		printk("I2C driver not registered!"
				" Device unknown 0x%x\n", tempvalue);
		goto exit_check_functionality_failed;
	}


	acc = kzalloc(sizeof(struct sc7a20_data), GFP_KERNEL);
	if (acc == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
				"%d\n", err);
		goto exit_alloc_data_failed;
	}

	mutex_init(&acc->lock);
	mutex_lock(&acc->lock);

	acc->client = client;
	sc7a20_i2c_client = client;
	i2c_set_clientdata(client, acc);

	printk("%s: %s has set irq1 to irq: %d\n",
			SC7A20_ACC_DEV_NAME, __func__, acc->irq1);
	printk("%s: %s has set irq2 to irq: %d\n",
			SC7A20_ACC_DEV_NAME, __func__, acc->irq2);

	gpio_request(GSENSOR_GINT1_GPI, "GSENSOR_INT1");
	gpio_request(GSENSOR_GINT2_GPI, "GSENSOR_INT2");
	acc->irq1 = 0; /* gpio_to_irq(GSENSOR_GINT1_GPI); */
	acc->irq2 = 0; /* gpio_to_irq(GSENSOR_GINT2_GPI); */

	

	if (acc->irq1 != 0) {
		printk("%s request irq1\n", __func__);
		err =
			request_irq(acc->irq1, sc7a20_isr1, IRQF_TRIGGER_RISING,
					"sc7a20_acc_irq1", acc);
		if (err < 0) {
			dev_err(&client->dev, "request irq1 failed: %d\n", err);
			goto err_mutexunlockfreedata;
		}
		disable_irq_nosync(acc->irq1);

		INIT_WORK(&acc->irq1_work, sc7a20_irq1_work_func);
		acc->irq1_work_queue =
			create_singlethread_workqueue("sc7a20_wq1");
		if (!acc->irq1_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev, "cannot create work queue1: %d\n",
					err);
			goto err_free_irq1;
		}
	}

	if (acc->irq2 != 0) {
		err =
			request_irq(acc->irq2, sc7a20_isr2, IRQF_TRIGGER_RISING,
					"sc7a20_acc_irq2", acc);
		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err_destoyworkqueue1;
		}
		disable_irq_nosync(acc->irq2);

		/*		 Create workqueue for IRQ.*/

		INIT_WORK(&acc->irq2_work, sc7a20_irq2_work_func);
		acc->irq2_work_queue =
			create_singlethread_workqueue("sc7a20_wq2");
		if (!acc->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev, "cannot create work queue2: %d\n",
					err);
			goto err_free_irq2;
		}
	}

	acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
	if (acc->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n", err);
		goto err_destoyworkqueue2;
	}

	memcpy(acc->pdata, client->dev.platform_data, sizeof(*acc->pdata));

	err = sc7a20_validate_pdata(acc);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}

	i2c_set_clientdata(client, acc);

	if (acc->pdata->init) {
		err = acc->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err2;
		}
	}
	
	{
		u8 TempDate[2];
		err = sc7a20_register_read(acc,TempDate,GSensor_Reg_CLICK_CFG1);
		if (err >= 0)
		{
			if (TempDate[0] & 0xE0)
				int2_statu = 1;
			else
				int2_statu = 0;
		}
		printk("2 ParkMonitor powerOn status(0x39) is %d,Read data=0x%x,ret=%x ..\n", int2_statu,TempDate[0],err);

		//err = sc7a20_register_read(acc,TempDate,0X31);

	//if (err >= 0)
		//printk("ParkMonitor powerOn INT1(0x31) data=0x%x,err=%x ..\n",TempDate[0],err);
	}


	err = sc7a20_device_power_on(acc);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	memset(acc->resume_state, 0, ARRAY_SIZE(acc->resume_state));
	atomic_set(&acc->enabled, 1);

	err = sc7a20_input_init(acc);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}

	/*create device group in sysfs as user interface */
	err = sysfs_create_group(&acc->input_dev->dev.kobj, &sc7a20_attr_group);
	if (err) {
		dev_err(&client->dev, "create device file failed!\n");
		err = -EINVAL;
		goto err_create_sysfs;
	}

	sc7a20_misc_data = acc;

	err = misc_register(&sc7a20_misc_device);
	if (err < 0) {
		dev_err(&client->dev,
				"misc SC7A20_ACC_DEV_NAME register failed\n");
		goto err_input_cleanup;
	}

	sc7a20_device_power_off(acc);

	/* As default, do not report information */
	atomic_set(&acc->enabled, 0);
	/*
#ifdef CONFIG_HAS_EARLYSUSPEND
acc->early_suspend.suspend = sc7a20_early_suspend;
acc->early_suspend.resume = sc7a20_early_resume;
acc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
register_early_suspend(&acc->early_suspend);
#endif
*/

#ifdef GSENSOR_CALI_SUPPORT
	sc7a20_misc_data->get_cali_flag = 0;
	if (misc_register(&gsensor_cali_struct) < 0)
	{
		printk("%s: Creat gsensor_cali_struct device file error!!\n", __func__);
	}
#endif

	mutex_unlock(&acc->lock);
	dev_info(&client->dev, "###%s###\n", __func__);
	printk("sprd-gsensor: -- %s -- success !\n",__func__);
	ok = 1;

	sc7a20_enable(acc); // jeff test

	return 0;

err_input_cleanup:
	sc7a20_input_cleanup(acc);
err_create_sysfs:
	sysfs_remove_group(&client->dev.kobj, &sc7a20_attr_group);
err_power_off:
	sc7a20_device_power_off(acc);
err2:
	if (acc->pdata->exit)
		acc->pdata->exit();
exit_kfree_pdata:
	kfree(acc->pdata);
err_destoyworkqueue2:
	if (acc->irq2_work_queue)
		destroy_workqueue(acc->irq2_work_queue);
err_free_irq2:
	if (acc->irq2) {
		free_irq(acc->irq2, acc);
		gpio_free(GSENSOR_GINT2_GPI);
	}
err_destoyworkqueue1:
	if (acc->irq1_work_queue)
		destroy_workqueue(acc->irq1_work_queue);
err_free_irq1:
	if (acc->irq1) {
		free_irq(acc->irq1, acc);
		gpio_free(GSENSOR_GINT1_GPI);
	}
err_mutexunlockfreedata:
	i2c_set_clientdata(client, NULL);
	mutex_unlock(&acc->lock);
	kfree(acc);
	sc7a20_misc_data = NULL;
exit_alloc_data_failed:
exit_check_functionality_failed:
	pr_err("%s: Driver Init failed\n", SC7A20_ACC_DEV_NAME);
exit_alloc_platform_data_failed:
	//	sc7a20_exit();
	return err;
}

static int  sc7a20_remove(struct i2c_client *client)
{
	/* TODO: revisit ordering here once _probe order is finalized */
	struct sc7a20_data *acc = i2c_get_clientdata(client);
	if (acc != NULL) {
		sc7a20_disable(acc); // jeff test

		if (acc->irq1) {
			free_irq(acc->irq1, acc);
			gpio_free(GSENSOR_GINT1_GPI);
		}
		if (acc->irq2) {
			free_irq(acc->irq2, acc);
			gpio_free(GSENSOR_GINT2_GPI);
		}

		if (acc->irq1_work_queue)
			destroy_workqueue(acc->irq1_work_queue);
		if (acc->irq2_work_queue)
			destroy_workqueue(acc->irq2_work_queue);
		misc_deregister(&sc7a20_misc_device);
		sc7a20_input_cleanup(acc);
		sc7a20_device_power_off(acc);
		if (acc->pdata->exit)
			acc->pdata->exit();
		kfree(acc->pdata);
		kfree(acc);
	}

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static int sc7a20_resume(struct i2c_client *client)
{
	struct sc7a20_data *acc = i2c_get_clientdata(client);
	printk("techain_sprd-gsensor: -- %s -- !\n",__func__);
	dev_dbg(&client->dev, "###%s###\n", __func__);

	if (acc != NULL && acc->on_before_suspend)
		return sc7a20_enable(acc);

	return 0;
}

static int sc7a20_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct sc7a20_data *acc = i2c_get_clientdata(client);
	printk("techain_sprd-gsensor: -- %s -- !\n",__func__);
	dev_dbg(&client->dev, "###%s###\n", __func__);

	if (acc != NULL) {
		acc->on_before_suspend = atomic_read(&acc->enabled);
		return sc7a20_disable(acc);
	}
	return 0;
}

static void sc7a20_early_suspend(struct early_suspend *es)
{
	sc7a20_suspend(sc7a20_i2c_client, (pm_message_t) {
			.event = 0});
}

static void sc7a20_early_resume(struct early_suspend *es)
{
	sc7a20_resume(sc7a20_i2c_client);
}

#endif /* CONFIG_HAS_EARLYSUSPEND */

/* Return 0 if detection is successful, -ENODEV otherwise */
static int
sc7a20_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strlcpy(info->type, SC7A20_ACC_I2C_NAME, I2C_NAME_SIZE);
	return 0;
}

static const struct i2c_device_id sc7a20_id[]
= { {SC7A20_ACC_I2C_NAME, 0}, {}, };

MODULE_DEVICE_TABLE(i2c, sc7a20_id);

static const struct of_device_id sc7a20_of_match[] = {
	{ .compatible = "silan,sc7a20_acc", },
	{ }
};
MODULE_DEVICE_TABLE(of, sc7a20_of_match);
static struct i2c_driver sc7a20_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name = SC7A20_ACC_I2C_NAME,
        .owner = THIS_MODULE,
		.of_match_table = sc7a20_of_match,
	},
	.probe = sc7a20_probe,
	.remove = sc7a20_remove,
	// .resume = sc7a20_resume,
	// .suspend = sc7a20_suspend,
	.id_table = sc7a20_id,
	.detect	= sc7a20_detect,
	.address_list = normal_i2c,
};

module_i2c_driver(sc7a20_driver);

MODULE_DESCRIPTION("sc7a20 3-Axis Accelerometer driver");
MODULE_AUTHOR("Silan Microelectronics Co., LTD.");
MODULE_LICENSE("GPL");


