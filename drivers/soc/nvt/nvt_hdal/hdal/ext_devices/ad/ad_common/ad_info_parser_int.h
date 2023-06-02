/**
    ad info parser

    @file       ad_info_parser_int.h
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _AD_INFO_PARSER_INT_H
#define _AD_INFO_PARSER_INT_H

#include "kwrap/nvt_type.h"
#include "ad_util_int.h"
#if defined(__FREERTOS)
#include "kdrv_i2c.h"
#else
#include <linux/i2c.h>
#endif

#define AD_CFG_INVALID_VALUE 0xffffffff
#define AD_IO_CTL_DES_MAX 20
typedef struct {
	UINT32 pin;
	UINT32 lvl;
	UINT32 hold_time;
} AD_IO_CTL_DES;

typedef struct {
	UINT32 mclk_sel;
	AD_IO_CTL_DES io_des[AD_IO_CTL_DES_MAX];
} AD_INFO_PWR;

typedef struct {
	UINT32 ch;
	UINT32 slv_addr;
} AD_INFO_I2C;

typedef struct {
	UINT32 type; // CTL_SEN_AD_SIGNAL
} AD_INFO_SIG;

typedef struct {
	UINT32 in;		//bit0~31(inport0~31), 1: enable 0:disable
	UINT32 in_num;	//0:means no inport
	AD_INFO_SIG sig;
} AD_INFO_OUT;


#define AD_CFG_STS_OK 0x0000001
typedef struct {
	AD_INFO_PWR pwr;
	AD_INFO_I2C i2c;
	UINT32 out_max_num;
	AD_INFO_OUT *out;
	UINT32 status;
} AD_CFG_INFO;


#if defined(__FREERTOS)
INT32 AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(UINT8 *pfdt, struct i2c_device_id *dev_tab, AD_CFG_INFO *ad_info, UINT32 ad_max_num);
void AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(UINT8 *pfdt, struct i2c_device_id *dev_tab);
#else
INT32 AD_UTIL_DECLARE_FUNC(ad_info_parse_data)(struct i2c_client *client, AD_CFG_INFO *ad_info);
void AD_UTIL_DECLARE_FUNC(ad_info_dump_data)(struct i2c_client *client);
#endif

#endif //_AD_INFO_PARSER_INT_H

