/**
    NVT OTP
    This file will operate otp
    @file       nvt-otp.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/crypto.h>
#include <plat/efuse_protected.h>
#include <plat/dma_protected.h>
#include <linux/mutex.h>
#include <mach/rcw_macro.h>


#ifndef CHKPNT
#define CHKPNT    printk("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)
#endif

#ifndef DBGD
#define DBGD(x)   printk("\033[0;35m%s=%d\033[0m\r\n", #x, x)
#endif

#ifndef DBGH
#define DBGH(x)   printk("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#endif

#ifndef DBG_DUMP
#define DBG_DUMP(fmtstr, args...) printk(fmtstr, ##args)
#endif

#ifndef DBG_ERR
#define DBG_ERR(fmtstr, args...)  printk("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#endif

#ifndef DBG_WRN
#define DBG_WRN(fmtstr, args...)  printk("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#endif

#if 0
#define DBG_IND(fmtstr, args...) printk("%s(): " fmtstr, __func__, ##args)
#else
#ifndef DBG_IND
#define DBG_IND(fmtstr, args...)
#endif
#endif


struct mutex otp_lock;


#define otp_loc() {mutex_lock(&otp_lock); }
#define otp_unl() {mutex_unlock(&otp_lock); }
extern INT32    efuse_read_addr(UINT32 efuse_addr, UINT32 address);


static UINT32 OTP_READ[] = {
	0xE92D41F0,
	0xE1A04000,
	0xE1A06001,
	0xE30F2D66,
	0xE3E00020,
	0xE2867094,
	0xE3A0800D,
	0xE3A0501F,
	0xE0045005,
	0xE1550008,
	0xC1A0F007,
	0xE1A00802,
	0xE286703C,
	0xE3A01001,
	0xE5801020,
	0xE5901030,
	0xE7E01051,
	0xE3510001,
	0x11A0F007,
	0xE3A01000,
	0xE0811004,
	0xE3A05000,
	0xE3A05000,
	0xE2867070,
	0xE5801004,
	0xE5902020,
	0xE3822002,
	0xE5802020,
	0xE5902020,
	0xE7E020D2,
	0xE3520001,
	0x01A0F007,
	0xE5902030,
	0xE5802030,
	0xE3A02000,
	0xE5802020,
	0xE5900008,
	0xE8BD81F0,
	0xE320F000,
	0xEAFFFFFE,
	0xE320F000,
};

static UINT32 OTP_OP[] = {
	0xE92D41F0,
	0xE1A04000,
	0xE1A06001,
	0xE30F2D66,
	0xE1A00802,
	0xE30F2D62,
	0xE1A03802,
	0xE2867028,
	0xE3A01001,
	0xE5801020,
	0xE5901030,
	0xE7E01051,
	0xE3510001,
	0x11A0F007,
	0xE3A01010,
	0xE0811104,
	0xE3A05001,
	0xE5835020,
	0xE286705C,
	0xE5801004,
	0xE5902020,
	0xE3822002,
	0xE5802020,
	0xE5902020,
	0xE7E020D2,
	0xE3520001,
	0x01A0F007,
	0xE5902030,
	0xE5802030,
	0xE2811001,
	0xE3A05002,
	0xE5835020,
	0xE2867094,
	0xE5801004,
	0xE5902020,
	0xE3822002,
	0xE5802020,
	0xE5902020,
	0xE7E020D2,
	0xE3520001,
	0x01A0F007,
	0xE5902030,
	0xE5802030,
	0xE2811001,
	0xE3A05004,
	0xE5835020,
	0xE28670CC,
	0xE5801004,
	0xE5902020,
	0xE3822002,
	0xE5802020,
	0xE5902020,
	0xE7E020D2,
	0xE3520001,
	0x01A0F007,
	0xE5902030,
	0xE5802030,
	0xE2811001,
	0xE3A05008,
	0xE5835020,
	0xE2867F41,
	0xE5801004,
	0xE5902020,
	0xE3822002,
	0xE5802020,
	0xE5902020,
	0xE7E020D2,
	0xE3520001,
	0x01A0F007,
	0xE5902030,
	0xE5802030,
	0xE3A02000,
	0xE5802020,
	0xE8BD81F0,
	0xE320F000,
	0xEAFFFFFE,
	0xE320F000,
};


static UINT32 DMA_USAGE_DATA[] = {
	0xE92D41F0,
	0xE1A04000,
	0xE1A06001,
	0xE2868020,
	0xE30F2D00,
	0xE1A02802,
	0xE1A03A04,
	0xE3A07A09,
	0xE1822003,
	0xE1827007,
	0xE5921054,
	0xE5970000,
	0xE1A05001,
	0xE1A06000,
	0xE3500000,
	0x01A0F008,
	0xE3A03064,
	0xE7F91051,
	0xE7F90050,
	0xE0030193,
	0xE730F013,
	0xE8BD81F0,
	0xE320F000,
	0xEAFFFFFE,
	0xE320F000,
};

static INT32 efuse_check_read_data(UINT16 data)
{
	INT32 i_ret;

	if ((data & EFUSE_STATUS_VALID) != EFUSE_STATUS_VALID) {
		i_ret = EFUSE_INACTIVE_ERR;
	} else {
		if ((data & EFUSE_STATUS_FORCE_INVALID) == EFUSE_STATUS_FORCE_INVALID) {
			i_ret = EFUSE_INVALIDATE_ERR;
		} else {
			i_ret = EFUSE_SUCCESS;
		}
	}
	return i_ret;
}

static INT32 efuse_read_data(UINT32 efuse_addr, UINT32 address)
{
	INT32  result;

	otp_loc();
	__asm__("MOV r0, %1\n\t"
			"MOV r1, %2\n\t"
			"MOV lr, pc\n\t"
			"MOV pc, %2\n\t"
			"MOV %0, r0\n\t"
			: "=r"(result)
			: "r"(efuse_addr), "r"(address)
			: "lr", "r0", "r1");
	otp_unl();
	return result;
}

static INT32 efuse_set_key(EFUSE_OTP_KEY_SET_FIELD key_set_index, UINT32 address)
{
	otp_loc();
	__asm__("MOV r0, %0\n\t"
			"MOV r1, %1\n\t"
			"MOV lr, pc\n\t"
			"MOV pc, %1\n\t"
			:
			: "r"(key_set_index), "r"(address)
			: "lr", "r0", "r1");
	otp_unl();
	return E_OK;
}

static UINT32 dma_get_utilization_data(UINT32 id, UINT32 address)
{
	UINT32  result;
	__asm__("MOV r0, %1\n\t"
			"MOV r1, %2\n\t"
			"MOV lr, pc\n\t"
			"MOV pc, %2\n\t"
			"MOV %0, r0\n\t"
			: "=r"(result)
			:"r"(id), "r"(address)
			: "lr", "r0", "r1");
	return result;
}



#define EFUSE_READDATA(addr)    efuse_read_data(addr, (UINT32)OTP_READ)

/*
     efuse_read_param_ops

     efuse read system parameter

     @note for EFUSE_PARAM_DATA

     @param[in]     param   efuse system internal data field
     @param[in]      data   data ready to programmed

     @return
    -@b EFUSE_SUCCESS       success
    -@b EFUSE_UNKNOW_PARAM_ERR  unknow system internal data field
    -@b EFUSE_INVALIDATE_ERR    system internal data field invalidate
    -@b EFUSE_OPS_ERR       efuse operation error
*/
INT32 efuse_read_param_ops(EFUSE_PARAM_DATA param, UINT16 *data)
{
	INT32   i_ret = EFUSE_SUCCESS;
	UINT16  tmp_data;
	UINT32  paramdatacnt;
	UINT32  pri_param_id_no;

	switch (param) {
	case EFUSE_DDR2P_TRIM_DATA:
	case EFUSE_USBC2_TRIM_DATA:
	case EFUSE_HDMI_TRIM_DATA:
	case EFUSE_TVDC_TRIM_DATA:
	case EFUSE_USBC_TRIM_DATA:
	case EFUSE_DDRP_TRIM_DATA:
	case EFUSE_SLVSEC_TRIM_DATA:
	case EFUSE_VX1_TRIM_DATA:
		if (EFUSE_HDMI_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_HDMI_PRI;
		} else if (EFUSE_USBC_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_USB_PRI;
		} else if (EFUSE_DDRP_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_DDR_DRIVING_PRI;
		} else if (EFUSE_TVDC_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_TVDEC_PRI;
		} else if (EFUSE_VX1_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_VX1_PRI;
		} else if (EFUSE_SLVSEC_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_SLVSEC_PRI;
		} else if (EFUSE_USBC2_TRIM_DATA == param) {
			pri_param_id_no = EFUSE_TRIM_DATA_USB2_PRI;
		} else {
			pri_param_id_no = EFUSE_TRIM_DATA_DDR2_DRIVING_PRI;
		}
		for (paramdatacnt = 0; paramdatacnt < EFUSE_PARAM_OP_CNT; paramdatacnt++) {
			if (paramdatacnt == 0) {
				tmp_data = EFUSE_READDATA(pri_param_id_no) & 0xFFFF;
			} else {
				tmp_data = (EFUSE_READDATA(pri_param_id_no) >> 16) & 0xFFFF;
			}
			if (tmp_data < 0) {
				i_ret = EFUSE_OPS_ERR;
				DBG_DUMP("^RERR:PKG UID[%d] op error\r\n", paramdatacnt);
				continue;
			} else {
				*data = (tmp_data & 0x3FFF);

				i_ret = efuse_check_read_data(tmp_data);

				if (EFUSE_UNKNOW_PARAM_ERR == i_ret) {
					DBG_DUMP("^RERR:unknow param error\r\n");
				} else if (EFUSE_INVALIDATE_ERR == i_ret) {
					if (!paramdatacnt) {
						DBG_IND("^YWRN:[PRI] param invalidate\r\n");
					} else {
						DBG_IND("^YWRN:[SEC] param invalidate\r\n");
					}
				} else if (EFUSE_INACTIVE_ERR == i_ret) {
					if (!paramdatacnt) {
						DBG_IND("^YWRN:[PRI] param inactive\r\n");
					} else {
						DBG_IND("^YWRN:[SEC] param inactive\r\n");
					}
				} else {
					DBG_IND("[%d] success = 0\r\n", paramdatacnt);
					break;
				}
			}
		}
		break;

	default:
		i_ret = EFUSE_UNKNOW_PARAM_ERR;
		break;
	}
	return i_ret;
}

INT32 efuse_otp_set_key(EFUSE_OTP_KEY_SET_FIELD key_set_index)
{
	efuse_set_key(key_set_index, (UINT32)OTP_OP);
	return E_OK;
}

UINT32 dma_get_utilization(DMA_ID id)
{
	return dma_get_utilization_data(id, (UINT32)&DMA_USAGE_DATA[0]);
}


static int __init nvt_init_otp_mutex(void)
{
	mutex_init(&otp_lock);
	return 0;
}

core_initcall(nvt_init_otp_mutex);
EXPORT_SYMBOL(efuse_read_param_ops);
EXPORT_SYMBOL(efuse_otp_set_key);
EXPORT_SYMBOL(dma_get_utilization);
