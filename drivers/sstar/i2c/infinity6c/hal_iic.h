/*
 * hal_iic.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _HAL_IIC_H_
#define _HAL_IIC_H_

#include <cam_os_wrapper.h>

#define HAL_I2C_SRCCLK_72M (72000000)
#define HAL_I2C_SRCCLK_54M (54000000)
#define HAL_I2C_SRCCLK_12M (12000000)

#define HAL_I2C_CNT_DEC_12M (8)
#define HAL_I2C_CNT_DEC_54M (5)
#define HAL_I2C_CNT_DEC_72M (4)

#define HAL_I2C_SPEED_200KHZ  (200000)
#define HAL_I2C_SPEED_700KHZ  (700000)
#define HAL_I2C_SPEED_1500KHZ (1500000)

#define HAL_I2C_SET_WRITEBIT_INDATA (0xFE)
#define HAL_I2C_SET_READBIT_INDATA  (0x01)

/* if we open OEN enable,we set offset h0010 bit6,
 *  if we don`t set offset h0010 bit6,it SDA waveform is below
 *        _
 *       / |
 *  ____|  |___
 *  it we set offset h0010 bit6,it will take some time for waitting pull-up
 *  it will take some time between SDA push 1T between SDA low like below
 *       __
 *      |
 *  ____/('/' is the time)
 */
#define HAL_I2C_SUPPLY_PUSHPULL_12M (3)
#define HAL_I2C_SUPPLY_PUSHPULL_54M (8)
#define HAL_I2C_SUPPLY_PUSHPULL_72M (11)

typedef enum
{
    HAL_I2C_OK = 0,
    HAL_I2C_ERR,
    HAL_I2C_INIT,         // 2 INIT ERR
    HAL_I2C_MST_SETUP,    // 3 I2C MASTER SET UP ERR
    HAL_I2C_CNT_SETUP,    // 4 COUNT SET UP ERR
    HAL_I2C_DMA_SETUP,    // 5 DMA SET UP ERR
    HAL_I2C_SRCCLK_SETUP, // 6 SET SOURCE CLK ERR
    HAL_I2C_WRITE,        // 7 WRITE ERR
    HAL_I2C_READ,         // 8 READ ERR
    HAL_I2C_PARAMETER,    // 9 PARAMETER ERR
    HAL_I2C_TIMEOUT,      // 10 TIMEOUT ERR
    HAL_I2C_STOP_CMD,     // 11 STOP SIGNAL ERR
    HAL_I2C_RETRY,
} i2c_err_num;

enum i2c_addr_mode
{
    HAL_I2C_ADDRMODE_NORMAL = 0,
    HAL_I2C_ADDRMODE_10BIT,
    HAL_I2C_ADDRMODE_MAX,
};

enum i2c_miu_priority
{
    HAL_I2C_MIUPRI_LOW = 0,
    HAL_I2C_MIUPRI_HIGH,
    HAL_I2C_MIUPRI_MAX,
};

enum i2c_miu_channel
{
    HAL_I2C_MIU_CHANNEL0 = 0,
    HAL_I2C_MIU_CHANNEL1,
    HAL_I2C_MIU_MAX,
};

struct hal_i2c_clkcnt
{
    u16 cnt_scl_high;
    u16 cnt_scl_low;
    u16 cnt_sda_setup;
    u16 cnt_sda_latch;
    u16 cnt_timeout_delay;
    u16 cnt_start_setup;
    u16 cnt_start_hold;
    u16 cnt_stop_setup;
    u16 cnt_stop_hold;
};

struct hal_i2c_dma_addr
{
    u64 dma_phys_addr;
    u8 *dma_virt_addr;
    u64 dma_miu_addr;
};

struct hal_i2c_dma_ctrl
{
    enum i2c_addr_mode      dma_addr_mode;
    enum i2c_miu_priority   dma_miu_prioty;
    enum i2c_miu_channel    dma_miu_chnnel;
    struct hal_i2c_dma_addr dma_addr_msg;
    u8                      dma_intr_en;
};

enum i2c_speed_mode
{
    HAL_I2C_SPEED_NORMAL = 0,
    HAL_I2C_SPEED_HIGH,
    HAL_I2C_SPEED_ULTRA,
    HAL_I2C_SPEED_MAX,
};

struct hal_i2c_ctrl
{
    u64                     bank_addr;
    u32                     dma_en;
    u32                     speed;
    u32                     group;
    u32                     max_srcclk_rate;
    u8                      config;
    u8                      push_pull;
    u8                      oen_cnt;
    struct hal_i2c_clkcnt   clock_count;
    struct hal_i2c_dma_ctrl dma_ctrl;
    s32 (*calbak_dma_transfer)(void *);
};

extern s32 hal_i2c_cnt_reg_set(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_dma_reset(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_disenable);
extern s32 hal_i2c_reset(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_rst);
extern s32 hal_i2c_dma_intr_en(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_disenable);
extern u32 hal_i2c_dma_trans_cnt(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_dma_trigger(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_dma_done_clr(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_val);
extern s32 hal_i2c_dma_stop_set(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_stop);
extern s32 hal_i2c_release(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_wn_mode_clr(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_wn_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len,
                            u8 para_wnlen, u16 para_waitcnt);
extern s32 hal_i2c_dma_async_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len);
extern s32 hal_i2c_wn_async_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len,
                                  u8 para_wnlen, u16 para_waitcnt);
extern s32 hal_i2c_dma_async_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len);
extern s32 hal_i2c_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len);
extern s32 hal_i2c_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len);
extern s32 hal_i2c_speed_calc(struct hal_i2c_ctrl *para_hal_ctrl);
extern s32 hal_i2c_init(struct hal_i2c_ctrl *para_hal_ctrl);
#endif
