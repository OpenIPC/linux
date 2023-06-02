/**
    Header file for I2C/I2C2/I2C3 module

    This file is the header file that defines the API and data type for I2C/I2C2/I2C3 module.

    @file       kdrv_i2c.h
    @ingroup    mIDrvIO_I2C
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _KDRV_I2C_H
#define _KDRV_I2C_H

#include "kwrap/type.h"
#include <kwrap/task.h>
#include <kwrap/flag.h>
#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>
#include "kdrv_type.h"

/*
	i2c_msg flags
*/
#define I2C_W_FLG 0x0
#define I2C_R_FLG 0x1

#define NVT_I2C_DESC_SEND_W     0x0100    /* trigger descriptor send write */
#define NVT_I2C_DESC_SEND_D     0x0200    /* trigger descriptor send delay */
#define NVT_I2C_DESC_SEND_P     0x0400    /* trigger descriptor send poll */
#define NVT_I2C_DESC_SEND_S     0x0800    /* trigger descriptor send set */
#define NVT_I2C_VD_SEND         0x1000    /* trigger vd send */
#define NVT_I2C_VD_SRC          0x2000    /* set vd source */
#define NVT_I2C_BUSFREE_VAL     0x4000    /* set bus free interval */

#define NVT_I2C_CONFIG_MASK     (NVT_I2C_VD_SRC | NVT_I2C_BUSFREE_VAL)
#define NVT_I2C_DESC_MASK       (NVT_I2C_DESC_SEND_W | NVT_I2C_DESC_SEND_D | NVT_I2C_DESC_SEND_P | NVT_I2C_DESC_SEND_S)

/*
	desc mode buffer length
*/
#define NVT_I2C_DESC_ALIGN 4	// desc buffer need word alignment
#define NVT_I2C_DESC_W_BUF 8	// WRITE
#define NVT_I2C_DESC_D_BUF 4	// DELAY
#define NVT_I2C_DESC_P_BUF 12	// POLL
#define NVT_I2C_DESC_S_BUF 8	// SET
#define NVT_I2C_DESC_E_BUF 4    // END
#define NVT_I2C_DESC_BUF(w_num, d_num, p_num, s_num) (w_num * NVT_I2C_DESC_W_BUF + d_num * NVT_I2C_DESC_D_BUF + p_num * NVT_I2C_DESC_P_BUF + s_num * NVT_I2C_DESC_S_BUF + NVT_I2C_DESC_E_BUF) // byte



#define I2C_NAME_SIZE   20
#define THIS_MODULE 0xFFFF

struct device_driver {
	const char      *name;  /* Name of the device driver. */
	UINT32          owner;
};

struct device {
	struct device_driver *driver;   /* which driver has allocated this device */
};

struct i2c_adapter {
//	int timeout;         /* in jiffies */
//	int retries;
	struct device dev;      /* the adapter device */

	int nr; // I2C_ID
};

struct i2c_board_info {
	char        type[I2C_NAME_SIZE];
	unsigned short  addr;
};

struct i2c_client {
	struct i2c_adapter *adapter;
};

struct i2c_device_id {
	char name[I2C_NAME_SIZE];
	UINT32 rev;
};

struct i2c_driver {
	/* Standard driver model interfaces */
	int(*probe)(struct i2c_client *, const struct i2c_device_id *);
	int(*remove)(struct i2c_client *);
	struct device_driver driver;
	const struct i2c_device_id *id_table;
};

struct i2c_msg {
	UINT16 addr;    /* slave address            */
	UINT16 flags;
	UINT16 len;     /* msg length
                    write : max support (I2C_BYTE_CNT_8-1) byte, one byte for device addr
                    read and write : write max support (I2C_BYTE_CNT_8-2) byte, one byte for device addr, one byte for r_addr
                    read and write : read max support (I2C_BYTE_CNT_8) byte*/
	UINT8 *buf;     /* pointer to msg data          */
};

/*
	descriptor mode extern packed data APIs
*/
#define I2C_DESCDATA_WRITE_1B1B(slave_7b_adr, addr, data)		(((((UINT64)addr)>>0) & 0xFF) | (((((UINT64)data)>>0) & 0xFF)<<8)                                                                      | ((UINT64)0x02<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40))
#define I2C_DESCDATA_WRITE_1B2B(slave_7b_adr, addr, data)		(((((UINT64)addr)>>0) & 0xFF) | (((((UINT64)data)>>8) & 0xFF)<<8) | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0x03<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40))
#define I2C_DESCDATA_WRITE_2B1B(slave_7b_adr, addr, data)		(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0x03<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40))
#define I2C_DESCDATA_WRITE_2B2B(slave_7b_adr, addr, data)		(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>8) & 0xFF)<<16) | ((((UINT64)data) & 0xFF)<<24) | ((UINT64)0x04<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40))

#define I2C_DESCDATA_SET_1B1B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>0) & 0xFF)                                     | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0xE0<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>0) & 0xFF)<<48))
#define I2C_DESCDATA_SET_1B2B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>0) & 0xFF)                                     | (((((UINT64)data)>>8) & 0xFF)<<16) | ((((UINT64)data) & 0xFF)<<24) | ((UINT64)0xE1<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>8) & 0xFF)<<48) | ((((UINT64)mask) & 0xFF)<<56))
#define I2C_DESCDATA_SET_2B1B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0xE2<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>0) & 0xFF)<<48))
#define I2C_DESCDATA_SET_2B2B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>8) & 0xFF)<<16) | ((((UINT64)data) & 0xFF)<<24) | ((UINT64)0xE3<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>8) & 0xFF)<<48) | ((((UINT64)mask) & 0xFF)<<56))

#define I2C_DESCDATA_POLL_1B1B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>0) & 0xFF)                                     | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0xC0<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>0) & 0xFF)<<48))
#define I2C_DESCDATA_POLL_1B2B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>0) & 0xFF)                                     | (((((UINT64)data)>>8) & 0xFF)<<16) | ((((UINT64)data) & 0xFF)<<24) | ((UINT64)0xC1<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>8) & 0xFF)<<48) | ((((UINT64)mask) & 0xFF)<<56))
#define I2C_DESCDATA_POLL_2B1B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>0) & 0xFF)<<16)                                 | ((UINT64)0xC2<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>0) & 0xFF)<<48))
#define I2C_DESCDATA_POLL_2B2B(slave_7b_adr, addr, data, mask)	(((((UINT64)addr)>>8) & 0xFF) | (((((UINT64)addr)>>0) & 0xFF)<<8) | (((((UINT64)data)>>8) & 0xFF)<<16) | ((((UINT64)data) & 0xFF)<<24) | ((UINT64)0xC3<<32) | ((((UINT64)slave_7b_adr)&0x7F)<<40) | (((((UINT64)mask)>>8) & 0xFF)<<48) | ((((UINT64)mask) & 0xFF)<<56))

extern BOOL i2c_descpack_start(UINT32 *current_index);
extern BOOL i2c_descpack_write(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data);
extern BOOL i2c_descpack_delay_us(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT32 delay_us);
extern BOOL i2c_descpack_set(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data);
extern BOOL i2c_descpack_poll(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size, UINT64 data, UINT32 delay_us, UINT32 retry);
extern BOOL i2c_descpack_end(UINT32 desc_buf_addr, UINT32 *current_index, UINT32 max_buf_size);

/*
	I2C public APIs
*/
extern struct i2c_adapter *i2c_get_adapter(INT i2c_id);
extern struct i2c_client *i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info);
extern void i2c_unregister_device(struct i2c_client *client);
extern INT i2c_add_driver(struct i2c_driver *driver);
extern void i2c_del_driver(struct i2c_driver *driver);
extern void i2c_set_clientdata(struct i2c_client *dev, void *data);
extern INT32 i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, INT num);

#endif
