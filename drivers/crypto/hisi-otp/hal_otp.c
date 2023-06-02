/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "hi_types.h"
#include "drv_osal_lib.h"
#include "hal_otp.h"

#ifdef OTP_SUPPORT

hi_u8 *efuse_otp_reg_base = HI_NULL;

/* OTP init */
hi_s32 hal_efuse_otp_init(hi_void)
{
    hi_u32 crg_value = 0;
    hi_u32 *sys_addr = HI_NULL;

    sys_addr = ioremap_nocache(REG_SYS_OTP_CLK_ADDR_PHY, 0x100);
    if (sys_addr == HI_NULL) {
        HI_LOG_ERROR("ERROR: sys_addr ioremap with nocache failed!!\n");
        return HI_FAILURE;
    }

    HAL_CIPHER_READ_REG(sys_addr, &crg_value);
#if defined(CHIP_TYPE_hi3559av100)
    crg_value |= OTP_CRG_RESET_BIT;   /* reset */
    crg_value |= OTP_CRG_CLOCK_BIT;   /* set the bit 0, clock opened */
    HAL_CIPHER_WRITE_REG(sys_addr, crg_value);

    /* clock select and cancel reset 0x30100*/
    crg_value &= (~OTP_CRG_RESET_BIT); /* cancel reset */
#endif
    crg_value |= OTP_CRG_CLOCK_BIT;   /* set the bit 0, clock opened */
    HAL_CIPHER_WRITE_REG(sys_addr, crg_value);

    iounmap(sys_addr);
    sys_addr = HI_NULL;

    efuse_otp_reg_base = ioremap_nocache(OTP_REG_BASE_ADDR_PHY, 0x100);
    if (efuse_otp_reg_base == HI_NULL) {
        HI_LOG_ERROR("ERROR: osal_ioremap_nocache for OTP failed!!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}
EXPORT_SYMBOL(hal_efuse_otp_init);

/* CRC16 */
static unsigned short calculate_crc16 (const unsigned char* data_array_ptr, unsigned long data_array_length)
{
    unsigned short crc_value = 0xffff;    /* init value 0xffff */
    unsigned short polynomial = 0x8005;   /* polynomial 0x8005 */
    unsigned short data_index = 0;
    int l = 0;
    unsigned char stored_buf[data_array_length];
    hi_u32 *iter_ptr;

    if (data_array_ptr == 0) {
        return crc_value;
    }
    if (data_array_length == 0) {
        return crc_value;
    }

	memset(stored_buf, 0, data_array_length);
    iter_ptr = (hi_u32 *)stored_buf;
	for(data_index = 0; data_index < data_array_length/4; data_index++) {
        *iter_ptr++ = ntohl(*((unsigned int *)data_array_ptr + data_index));
	}

    /* data_array_length=17 */
    for (data_index = 0; data_index < data_array_length; data_index++) {
        unsigned char byte0 = stored_buf[data_index];
        crc_value ^= byte0 * 256;
        for (l=0;l<8;l++) {
            HI_BOOL flag = ((crc_value & 0x8000)==32768);
            crc_value = (crc_value & 0x7FFF)*2;
            if (flag == HI_TRUE) {
                crc_value ^= polynomial;
            }
        }
    }
    return crc_value;
};

static hi_s32 hal_otp_wait_free(hi_void)
{
    hi_u32 time_out_cnt = 0;
    hi_u32 reg_value = 0;

    while(1) {
        HAL_CIPHER_READ_REG(OTP_USER_CTRL_STA, &reg_value);

        /* bit0:otp_op_busy 0:idle, 1:busy */
        if((reg_value&0x1)==0) {
            return HI_SUCCESS;
        }

        time_out_cnt++;
        if(time_out_cnt >= 10000) {
            HI_LOG_ERROR("Otp wait free time out!\n");
            break;
        }
    }
    return HI_FAILURE;
}

static hi_s32 hal_otp_set_mode(otp_user_work_mode otp_mode)
{
    hi_u32 reg_value = otp_mode;

    if(otp_mode >= OTP_UNKOOWN_MODE) {
        HI_LOG_ERROR("Mode Unknown!\n");
        return  HI_FAILURE;
    }

    (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_WORK_MODE, reg_value);
    return HI_SUCCESS;
}

static hi_void hal_otp_op_start(hi_void)
{
    hi_u32 reg_value = 0x1acce551;
    (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_OP_START, reg_value);
}

static hi_s32 hal_otp_wait_op_done(hi_void)
{
    hi_u32 time_out_cnt = 0;
    hi_u32 reg_value = 0;

    while(1) {
        HAL_CIPHER_READ_REG(OTP_USER_CTRL_STA, &reg_value);
        if(reg_value & 0x2) {
            return HI_SUCCESS;
        }

        time_out_cnt++;
        if(time_out_cnt >= 10000) {
            HI_LOG_ERROR("OTP_Wait_OP_done TimeOut!\n");
            break;
        }
    }
    return HI_FAILURE;
}

static hi_u32 hal_get_otp_lock_status_by_type(otp_lock_sta_type lock_status_type)
{
    hi_u32 reg_addr = 0;
    hi_u32 reg_value = 0;

    if (lock_status_type == OTP_USER_LOCK_STA0_TYPE) {
        reg_addr = (hi_u32)(hi_uintptr_t)OTP_USER_LOCK_STA0;
    } else {
        reg_addr = (hi_u32)(hi_uintptr_t)OTP_USER_LOCK_STA1;
    }

    HAL_CIPHER_READ_REG(reg_addr, &reg_value);

    return reg_value;
}

static hi_void hal_choose_otp_key(otp_user_key_index which_key)
{
    hi_u32 reg_value = 0;

    reg_value = which_key;
    (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_INDEX, reg_value);
}

/* write data to otp */
static hi_s32 hal_write_key(otp_user_key_length key_length,
    hi_u32 *key_data_buf, hi_u32 use_crc)
{
    hi_u32 crc16 = 0;

    switch(key_length) {
        case OTP_KEY_LENGTH_64BIT: {
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA0, key_data_buf[0]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA1, key_data_buf[1]);
            if(use_crc==HI_TRUE) {
                /* computer user key crc16. */
                crc16 = calculate_crc16((hi_u8 *)key_data_buf,8);
                crc16&=0xffff;
                (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA2, crc16);
            }
            break;
        }
        case OTP_KEY_LENGTH_128BIT: {
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA0, key_data_buf[0]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA1, key_data_buf[1]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA2, key_data_buf[2]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA3, key_data_buf[3]);
            if(use_crc == HI_TRUE) {
                /* computer user key crc16. */
                crc16 = calculate_crc16((hi_u8*)key_data_buf,16);
                crc16&=0xffff;
                (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA4, crc16);
            }
            break;
        }
        case OTP_KEY_LENGTH_256BIT: {
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA0, key_data_buf[0]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA1, key_data_buf[1]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA2, key_data_buf[2]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA3, key_data_buf[3]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA4, key_data_buf[4]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA5, key_data_buf[5]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA6, key_data_buf[6]);
            (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA7, key_data_buf[7]);
            if(use_crc == HI_TRUE) {
                crc16 = calculate_crc16((hi_u8*)key_data_buf, 32);
                crc16&=0xffff;
                HI_LOG_ERROR("crc: 0x%x\n",crc16);
                (hi_void)HAL_CIPHER_WRITE_REG(OTP_USER_KEY_DATA8, crc16);
            }
            break;
        }
        default:
            break;
    }
    return HI_SUCCESS;
}

static hi_s32 hal_get_otp_lock_status(otp_lock_sta_type lock_status_type,
    hi_u32 *status)
{
    hi_s32 ret = HI_FAILURE;

    if(lock_status_type >= OTP_USER_LOCK_UNKNOWN_STA) {
        return HI_FAILURE;
    }

    ret = hal_otp_wait_free();
    if(ret != HI_SUCCESS) {
        HI_LOG_ERROR("hal otp wait free failed.\n");
        return ret;
    }

    ret = hal_otp_set_mode(OTP_READ_LOCK_STA_MODE);
    if(ret != HI_SUCCESS) {
        HI_LOG_ERROR("hal otp set mode failed.\n");
        return ret;
    }

    hal_otp_op_start();

    ret = hal_otp_wait_op_done();
    if(ret != HI_SUCCESS) {
        HI_LOG_ERROR("hal otp wait op done failed.\n");
        return ret;
    }

    *status = hal_get_otp_lock_status_by_type(lock_status_type);
    return HI_SUCCESS;
}

static hi_s32 is_locked(otp_user_key_index which_key, hi_u32 lock_sta)
{
    hi_u32 status = lock_sta;

    if(which_key>OTP_USER_KEY3) {
        HI_LOG_ERROR("Unsupport key!\n");
        return HI_FAILURE;
    }

    if((status >> (which_key+5))&0x1) {
        HI_LOG_ERROR("Key%d was locked!\n", which_key);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

/* set user key to otp */
hi_s32 hal_write_key_to_otp(otp_user_key_index which_key,
    hi_u32 *key_data_buf, hi_u32 use_crc)
{
    hi_u32 lock_sta0 = 0;

    if(which_key > OTP_USER_KEY3) {
        return HI_FAILURE;
    }
    hal_get_otp_lock_status(OTP_USER_LOCK_STA0_TYPE, &lock_sta0);

    if(is_locked(which_key,lock_sta0)) {
        return HI_FAILURE;
    }

    if(HI_FAILURE == hal_otp_wait_free()) {
        return HI_FAILURE;
    }

    hal_choose_otp_key(which_key);

    hal_write_key(OTP_KEY_LENGTH_128BIT, key_data_buf, HI_TRUE);

    if(hal_otp_set_mode(OTP_WRITE_KEY_ID_OR_PASSWD_MODE)) {
        return HI_FAILURE;
    }
    hal_otp_op_start();

    if(HI_FAILURE == hal_otp_wait_op_done()) {
        return HI_FAILURE;
    }
    return  HI_SUCCESS;
}
EXPORT_SYMBOL(hal_write_key_to_otp);

/* check otp key crc */
static hi_s32 hal_is_crc_check_ok(hi_void)
{
    hi_u32 reg_value = 0;

    HAL_CIPHER_READ_REG(OTP_USER_CTRL_STA, &reg_value);
    if(reg_value&0x10) {
        return HI_SUCCESS;
    }
    return HI_FAILURE;
}

hi_s32 hal_key_crc_check_out(otp_user_key_index which_key,hi_u32 *result)
{

    if(which_key > OTP_USER_KEY3) {
        return HI_FAILURE;
    }

    if(HI_FAILURE == hal_otp_wait_free()) {
        return HI_FAILURE;
    }

    hal_choose_otp_key(which_key);

    if(hal_otp_set_mode(OTP_KEY_ID_OR_PASSWD_CRC_MODE)) {
        return HI_FAILURE;
    }
    hal_otp_op_start();

    if(HI_FAILURE == hal_otp_wait_op_done()) {
        return HI_FAILURE;
    }

    *result = hal_is_crc_check_ok();
    if((*result) != HI_SUCCESS) {
        HI_LOG_ERROR("CRC Error!!!!!!!!!!!!\n");
        return  HI_FAILURE;
    }

    return  HI_SUCCESS;
}
EXPORT_SYMBOL(hal_key_crc_check_out);

/* set otp key to klad */
hi_s32 hal_efuse_otp_load_cipher_key(hi_u32 chn_id, hi_u32 opt_id)
{
    if(opt_id > OTP_USER_KEY3) {
        opt_id = OTP_USER_KEY0;
    }

    if(HI_FAILURE == hal_otp_wait_free()) {
        return HI_FAILURE;
    }
    hal_choose_otp_key(opt_id);

    if(hal_otp_set_mode(OTP_LOCK_CIPHER_KEY_MODE)) {
        return HI_FAILURE;
    }

    hal_otp_op_start();

    if(HI_FAILURE == hal_otp_wait_op_done()) {
        return HI_FAILURE;
    }

    return  HI_SUCCESS;
}
#endif

