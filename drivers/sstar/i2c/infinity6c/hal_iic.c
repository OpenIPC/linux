/*
 * hal_iic.c- Sigmastar
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

#include <iic_os.h>
#include <hal_iic.h>
#include <hal_iic_reg.h>

#define HAL_I2C_READ_REG(reg)       HAL_I2C_READ_WORD(para_hal_ctrl->bank_addr + ((reg) << 2))
#define HAL_I2C_WRITE_REG(reg, val) HAL_I2C_WRITE_WORD(para_hal_ctrl->bank_addr + ((reg) << 2), (val))
#define HAL_I2C_WRITE_REG_MASK(reg, val, mask) \
    HAL_I2C_WRITE_WORD_MASK(para_hal_ctrl->bank_addr + ((reg) << 2), val, mask)

#define HAL_I2C_WRITE_BYTE_REG(reg, val) HAL_I2C_WRITE_BYTE(para_hal_ctrl->bank_addr + (reg), (val))
#define HAL_I2C_READ_BYTE_REG(reg)       HAL_I2C_READ_BYTE(para_hal_ctrl->bank_addr + (reg))

#define HAL_I2C_TRANSFER_RETRY (50000)
#define HAL_I2C_POLL_RETRY     (10000000)

static s32 hal_i2c_intr_flag(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_FLAG);
    ret = ret & HAL_I2C_MASK_INT_FLAG;

    return ret;
}

static s32 hal_i2c_intr_clr(struct hal_i2c_ctrl *para_hal_ctrl)
{
    u16 retry = 0;

    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_INT_FLAG, 1, HAL_I2C_MASK_INT_FLAG);

    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++)
    {
        if (!hal_i2c_intr_flag(para_hal_ctrl))
        {
            return HAL_I2C_OK;
        }
    }

    return -HAL_I2C_ERR;
}

static inline s32 hal_i2c_dma_done_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_DMA_TRANSFER_DONE);
    ret &= HAL_I2C_MASK_DMA_TRSFER_DONE;

    return ret;
}

static s32 hal_i2c_dma_miu_reset(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_disenable)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_CONFIG, para_disenable << HAL_I2C_BIT_SHIFT_3, HAL_I2C_MASK_DMACFG_MIURST);

    return HAL_I2C_OK;
}

static s32 hal_i2c_slv_addr_length(struct hal_i2c_ctrl *para_hal_ctrl, const enum i2c_addr_mode para_adr_mode)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_10BITMODE, para_adr_mode << HAL_I2C_BIT_SHIFT_10, HAL_I2C_MASK_DMABITMODE);

    return HAL_I2C_OK;
}

static s32 hal_i2c_miu_priority_set(struct hal_i2c_ctrl *para_hal_ctrl, const enum i2c_miu_priority para_miu_pri)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_CONFIG, para_miu_pri << HAL_I2C_BIT_SHIFT_4, HAL_I2C_MASK_DMACFG_MIUPRI);

    return HAL_I2C_OK;
}

static s32 hal_i2c_miu_channel_sel(struct hal_i2c_ctrl *para_hal_ctrl, const enum i2c_miu_channel para_miu_chanl)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_MIUCHANL_SEL, para_miu_chanl << HAL_I2C_BIT_SHIFT_7, HAL_I2C_MASK_MIUCHANL_SEL);

    return HAL_I2C_OK;
}

static s32 hal_i2c_miu_addr_set(struct hal_i2c_ctrl *para_hal_ctrl, const u64 para_miu_addr)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_MIUADDR_LOW, (u16)(para_miu_addr & 0x0000FFFF));
    HAL_I2C_WRITE_REG(HAL_I2C_REG_MIUADDR_HIGH, ((u16)((para_miu_addr & 0xFFFF0000) >> 16)));
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_MIUADDR_MSB, ((u16)((para_miu_addr & 0xF00000000) >> 32)),
                           HAL_I2C_MASK_MIUADDR_MSB);

    return HAL_I2C_OK;
}
static s32 hal_i2c_dma_transfer(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret        = HAL_I2C_OK;
    u32 poll_count = HAL_I2C_POLL_RETRY;

    if (para_hal_ctrl->calbak_dma_transfer)
    {
        ret = para_hal_ctrl->calbak_dma_transfer(para_hal_ctrl);
        if (ret)
        {
            return -HAL_I2C_ERR;
        }
    }
    else
    {
        while (poll_count--)
        {
            ret = hal_i2c_dma_done_chk(para_hal_ctrl);
            if (ret)
            {
                hal_i2c_dma_done_clr(para_hal_ctrl, 1);
                break;
            }
            if (1 == poll_count)
            {
                dmsg_i2c_halerr("i2c-%d dma read polling timeout!\n", para_hal_ctrl->group);
                return -HAL_I2C_TIMEOUT;
            }
        }
    }

    return HAL_I2C_OK;
}

static s32 hal_i2c_stop_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_FOR_STOP, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_high_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_HIGH_PERIOD, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_low_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_LOW_PERIOD, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_sda_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_BTWN_FALEDGE, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_start_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_FOR_START, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_data_latch_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_DATA_LATCHTIM, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_timeout_cnt_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_cnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_TIMEOUT, para_cnt);

    return HAL_I2C_OK;
}

static s32 hal_i2c_cmd_data_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_data)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_DMA_CMD_DATA_25H, para_data);

    return HAL_I2C_OK;
}

static s32 hal_i2c_cmd_len_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_len)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_CMD_LEN_29H, para_len, HAL_I2C_MASK_DMA_CMDLEN);

    return HAL_I2C_OK;
}

static s32 hal_i2c_dma_len_set(struct hal_i2c_ctrl *para_hal_ctrl, u32 para_len)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_TRANS_DATA_LEN_LOW, (u16)(para_len & 0x0000FFFF));
    dmsg_i2c_halwarn("i2c-%d set data len : %d\n", para_hal_ctrl->group, para_len);
    HAL_I2C_WRITE_REG(HAL_I2C_REG_TRANS_DATA_LEN_HIGH, (u16)((para_len & 0xFFFF0000) >> 16));

    return HAL_I2C_OK;
}

static s32 hal_i2c_dma_rdwr_set(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_boolwr)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_RDWR_FMT, para_boolwr << HAL_I2C_BIT_SHIFT_6, HAL_I2C_MASK_RDWR_FMT);

    return HAL_I2C_OK;
}

static s32 hal_i2c_dma_slvadr_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr)
{
    if (HAL_I2C_ADDRMODE_10BIT == para_hal_ctrl->dma_ctrl.dma_addr_mode)
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_SLAVEADDR, para_slvadr & HAL_I2C_MASK_DMASLVADDR_10BIT,
                               HAL_I2C_MASK_DMASLVADDR_10BIT);
    }
    else
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_SLAVEADDR, para_slvadr & HAL_I2C_MASK_DMASLVADDR_NORM,
                               HAL_I2C_MASK_DMASLVADDR_NORM);
    }

    return HAL_I2C_OK;
}

static s32 hal_i2c_start_bit_send(struct hal_i2c_ctrl *para_hal_ctrl)
{
    HAL_I2C_WRITE_BYTE_REG((HAL_I2C_REG_STAR_STOP << 2), HAL_I2C_MASK_CMD_START);

    return HAL_I2C_OK;
}

static s32 hal_i2c_stop_bit_send(struct hal_i2c_ctrl *para_hal_ctrl)
{
    HAL_I2C_WRITE_BYTE_REG(((HAL_I2C_REG_STAR_STOP << 2) + 1), HAL_I2C_MASK_CMD_STOP);

    return HAL_I2C_OK;
}

static inline s32 hal_i2c_start_status_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_STATUS) & HAL_I2C_MASK_INTSTU_START;
    ret = ret >> HAL_I2C_BIT_SHIFT_8;

    return ret;
}

static inline s32 hal_i2c_stop_status_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_STATUS) & HAL_I2C_MASK_INTSTU_STOP;
    ret = ret >> HAL_I2C_BIT_SHIFT_9;

    return ret;
}

static inline s32 hal_i2c_idle_status_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_STATUS) & HAL_I2C_MASK_INTSTU_STATE;

    return ret;
}

static inline s32 hal_i2c_rx_status_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_STATUS) & HAL_I2C_MASK_INTSTU_RXDONE;
    ret = ret >> HAL_I2C_BIT_SHIFT_10;

    return ret;
}

static inline s32 hal_i2c_tx_status_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret;

    ret = HAL_I2C_READ_REG(HAL_I2C_REG_INT_STATUS) & HAL_I2C_MASK_INTSTU_TXDONE;
    ret = ret >> HAL_I2C_BIT_SHIFT_11;

    return ret;
}

static inline u16 hal_i2c_ack_chk(struct hal_i2c_ctrl *para_hal_ctrl)
{
    return (HAL_I2C_READ_BYTE_REG((HAL_I2C_REG_WRITE_DATA << 2) + 1) & 0x01);
}

static s32 hal_i2c_read_trigger(struct hal_i2c_ctrl *para_hal_ctrl, const u8 ack)
{
    /*
     * miic offset h0003 bit8(reg_rdata_en) will take effect by the action write register
     * rather than value 1 in this bit
     * so wo move reg_ack_bit here or it`ll trigger twice
     * and the seconed write trigger will cover the last read action whenever the last read done or not
     * then cause some unexpected clk bit
     */
    HAL_I2C_WRITE_BYTE_REG(((HAL_I2C_REG_READ_DATA << 2) + 1), (1 | (ack << HAL_I2C_BIT_SHIFT_1)));

    return 0;
}

static s32 hal_i2c_send_byte(struct hal_i2c_ctrl *para_hal_ctrl, const u8 para_val)
{
    s32 ret = 0;
    u16 retry;

    HAL_I2C_WRITE_BYTE_REG((HAL_I2C_REG_WRITE_DATA << 2), para_val);
    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++)
    {
        ret |= (hal_i2c_intr_flag(para_hal_ctrl) && hal_i2c_tx_status_chk(para_hal_ctrl));
        if (ret)
        {
            ret |= hal_i2c_intr_clr(para_hal_ctrl);
            retry = 0;
            break;
        }
    }
    if (retry >= HAL_I2C_TRANSFER_RETRY)
    {
        dmsg_i2c_halerr("i2c-%d err tx done timeout\n", para_hal_ctrl->group);
        return -HAL_I2C_RETRY;
    }
    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++) // ask for ack
    {
        if (!hal_i2c_ack_chk(para_hal_ctrl))
        {
            hal_i2c_intr_clr(para_hal_ctrl);
            dmsg_i2c_halwarn("i2c-%d get an ack\n", para_hal_ctrl->group);
            return 0;
        }
    }
    hal_i2c_intr_clr(para_hal_ctrl);
    dmsg_i2c_halerr("i2c-%d no ack!\n", para_hal_ctrl->group);
    return -HAL_I2C_RETRY;
}

static s32 hal_i2c_send_buf(struct hal_i2c_ctrl *para_hal_ctrl, u8 *para_pdata, u32 para_length)
{
    s32 ret = 0;
    u16 i;
    for (i = 0; i < para_length; i++)
    {
        ret |= hal_i2c_send_byte(para_hal_ctrl, *(para_pdata + i));
        if (ret < 0) // send byte
        {
            return ret;
        }
    }

    return 0;
}

static int hal_i2c_recv_byte(struct hal_i2c_ctrl *para_hal_ctrl, u8 *para_buf, const u8 ack)
{
    u16 retry;
    s32 ret = 0;

    ret |= hal_i2c_read_trigger(para_hal_ctrl, ack);

    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++)
    {
        ret |= (hal_i2c_intr_flag(para_hal_ctrl) && hal_i2c_rx_status_chk(para_hal_ctrl));
        if (ret)
        {
            hal_i2c_intr_clr(para_hal_ctrl);
            retry = 0;
            break;
        }
    }
    if (retry >= HAL_I2C_TRANSFER_RETRY)
    {
        dmsg_i2c_halerr("i2c-%d recv byte timeout err\n", para_hal_ctrl->group);
        ret = -HAL_I2C_RETRY;
        goto OUT;
    }
    *para_buf = (u8)(HAL_I2C_READ_BYTE_REG(HAL_I2C_REG_READ_DATA << 2));
    ret       = 0;
OUT:
    hal_i2c_intr_clr(para_hal_ctrl);
    return ret;
}

static s32 hal_i2c_recv_buf(struct hal_i2c_ctrl *para_hal_ctrl, u8 *para_pdata, u32 para_length)
{
    u16 i;
    u8  ack;
    for (i = 0; i < para_length; i++)
    {
        if (i == (para_length - 1))
        {
            dmsg_i2c_halwarn("i2c-%d return nack\n", para_hal_ctrl->group);
            ack = 0x01;
        }
        else
        {
            ack = 0x00;
            dmsg_i2c_halwarn("i2c-%d return ack\n", para_hal_ctrl->group);
        }
        if (hal_i2c_recv_byte(para_hal_ctrl, (para_pdata + i), ack))
        {
            dmsg_i2c_halerr("i2c-%d receive buffer error\n", para_hal_ctrl->group);
            break;
        }
    }
    return i;
}

static s32 hal_i2c_dma_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = 0;
    u8 *dma_virt_addr;
    u32 dma_transfer_cnt;

    // set read/write cmd
    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 0); // set write
    // set slave addr
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x0);
    // set miu addr
    /*done in hal_i2c_dma_init*/
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    // reset and reset miu
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0);
    // copy data to miu addr
    dma_virt_addr = para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_virt_addr;
    memcpy(dma_virt_addr, para_pdata, para_len);
    hal_i2c_cache_flush(dma_virt_addr, para_len);
    //  set enable dma interrupt
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    // set clear done
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);

    // set trigger
    ret |= hal_i2c_dma_transfer(para_hal_ctrl);
    if (ret)
    {
        dmsg_i2c_halerr("i2c-%d, trigger dma write failed\n", para_hal_ctrl->group);
        return -HAL_I2C_WRITE;
    }

    dma_transfer_cnt = hal_i2c_dma_trans_cnt(para_hal_ctrl);

    if (dma_transfer_cnt != para_len)
    {
        dmsg_i2c_halerr("i2c-%d err dma write transform len:%d != para_len:%d\n", para_hal_ctrl->group,
                        dma_transfer_cnt, para_len);
        return -HAL_I2C_WRITE;
    }
    dmsg_i2c_halwarn("DMA WRITE SUCCESS DONE\n");
    return 0;
} // end hal_i2c_dma_write()

static s32 hal_i2c_wn_grplen_set(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_grplen)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_WN_GROUP_LEN, para_grplen, HAL_I2C_MASK_WN_GROUP_LEN);
    return 0;
}

static s32 hal_i2c_wn_mode_set(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_en)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_WN_MODE, para_en, HAL_I2C_MASK_WN_MODE_EN);
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_WN_MODE, para_en << 1, HAL_I2C_MASK_WN_RS_P);
    return 0;
}
static s32 hal_i2c_wn_wait_set(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_waitcnt)
{
    HAL_I2C_WRITE_REG(HAL_I2C_REG_WN_WAIT_CNT, para_waitcnt);
    return 0;
}
static s32 hal_i2c_riu_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret   = HAL_I2C_OK;
    u16 retry = 0;
    // reset iic
    // 1.send start
    hal_i2c_intr_clr(para_hal_ctrl);
    ret |= hal_i2c_start_bit_send(para_hal_ctrl); // do we need clear start bit??
    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++)
    {
        if (hal_i2c_intr_flag(para_hal_ctrl))
        {
            hal_i2c_intr_clr(para_hal_ctrl);
            retry = 0;
            break;
        }
    }
    if (retry >= HAL_I2C_TRANSFER_RETRY)
    {
        ret = -HAL_I2C_RETRY;
        dmsg_i2c_halerr("i2c-%d write start err!\n", para_hal_ctrl->group);
        goto err_rtn;
    }
    // 2.send slave addr << 1 | w_bit
    ret |= hal_i2c_send_byte(para_hal_ctrl, para_slvadr);
    if (ret < 0)
    {
        dmsg_i2c_halerr("i2c-%d send slave addr|write = 0x%04x nack err %d\n", para_hal_ctrl->group, para_slvadr, ret);
        goto err_rtn;
    }

    // 3.send data
    ret |= hal_i2c_send_buf(para_hal_ctrl, para_pdata, para_len);
    if (ret < 0)
    {
        dmsg_i2c_halerr("i2c-%d write send data err %d\n", para_hal_ctrl->group, ret);
        goto err_rtn;
    }
    dmsg_i2c_halwarn("FIFO MODE WRITE SUCCESS DONE\n");
    return HAL_I2C_OK;
err_rtn:
    return ret;
}

static s32 hal_i2c_dma_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = HAL_I2C_OK;
    u8 *dma_virt_addr;
    u32 dma_transfer_cnt;

    dma_virt_addr = para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_virt_addr;

    // set read/write cmd
    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 1); // set read
    // set slave addr
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x00);
    // set miu addr
    /*done in hal_i2c_dma_init*/
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    // reset and reset miu

    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0);
    // set enable dma interrupt
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    // set clear done
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);

    // set trigger
    ret |= hal_i2c_dma_transfer(para_hal_ctrl);
    if (ret)
    {
        dmsg_i2c_halerr("i2c-%d, trigger dma read failed\n", para_hal_ctrl->group);
        return -HAL_I2C_READ;
    }

    dma_transfer_cnt = hal_i2c_dma_trans_cnt(para_hal_ctrl);

    if (dma_transfer_cnt != para_len)
    {
        dmsg_i2c_halerr("i2c-%d dma read transform len:%d != para_len:%d err\n", para_hal_ctrl->group, dma_transfer_cnt,
                        para_len);
        return -HAL_I2C_READ;
    }
    hal_i2c_cache_invalidate(dma_virt_addr, para_len);
    memcpy(para_pdata, dma_virt_addr, para_len);
    dmsg_i2c_halwarn("DMA READ SUCCESS DONE\n");
    return ret;
}

static s32 hal_i2c_riu_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret   = HAL_I2C_OK;
    u16 retry = 0;

    // reset iic
    hal_i2c_intr_clr(para_hal_ctrl);
    I2C_DELAY_N_US(1);
    // 1.send start
    ret |= hal_i2c_start_bit_send(para_hal_ctrl);
    for (retry = 0; retry < HAL_I2C_TRANSFER_RETRY; retry++)
    {
        if (hal_i2c_intr_flag(para_hal_ctrl))
        {
            hal_i2c_intr_clr(para_hal_ctrl);
            retry = 0;
            break;
        }
    }
    if (retry >= HAL_I2C_TRANSFER_RETRY)
    {
        ret = -HAL_I2C_RETRY;
        dmsg_i2c_halerr("i2c-%d start err %d\n", para_hal_ctrl->group, ret);
        goto err_rtn;
    }

    // 2.send slave addr <<  1| r_bit
    ret |= hal_i2c_send_byte(para_hal_ctrl, para_slvadr);
    if (ret)
    {
        dmsg_i2c_halerr("i2c-%d send slave addr|read = 0x%04x nack from slave err %d\n", para_hal_ctrl->group,
                        para_slvadr, ret);
        goto err_rtn;
    }

    // 3.recv data
    ret |= hal_i2c_recv_buf(para_hal_ctrl, para_pdata, para_len);
    if (para_len != ret)
    {
        dmsg_i2c_halerr("i2c-%d read data err %d\n", para_hal_ctrl->group, ret);
        goto err_rtn;
    }
    dmsg_i2c_halwarn("NORMAL READ SUCCESS DONE\n");
    return HAL_I2C_OK;
err_rtn:
    return ret;
}

static s32 hal_i2c_thd_tsu_set(struct hal_i2c_ctrl *para_hal_ctrl)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_CNT_TSUTHD_OPEN, HAL_I2C_MASK_TSUTDH_OPEN, HAL_I2C_MASK_TSUTDH_OPEN);
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_TSU_SET, para_hal_ctrl->clock_count.cnt_start_setup);
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CNT_THD_SET, para_hal_ctrl->clock_count.cnt_stop_hold);
    return 0;
}

static s32 hal_i2c_dma_init(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret = HAL_I2C_OK;
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);
    ret |= hal_i2c_slv_addr_length(para_hal_ctrl, para_hal_ctrl->dma_ctrl.dma_addr_mode);

    ret |= hal_i2c_miu_priority_set(para_hal_ctrl, para_hal_ctrl->dma_ctrl.dma_miu_prioty);

    ret |= hal_i2c_miu_channel_sel(para_hal_ctrl, para_hal_ctrl->dma_ctrl.dma_miu_chnnel);

    ret |= hal_i2c_miu_addr_set(para_hal_ctrl, para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_miu_addr);
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_CONFIG, 0, HAL_I2C_MASK_DMACFG_SOFTRST);
    // set interrupt occure after miu move data done completely
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_RESERVED, BIT4 | BIT5 | BIT7, HAL_I2C_MASK_LASTDONE);

    return ret;
}

static s32 hal_i2c_config_set(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret = HAL_I2C_OK;

    // 1.clear interrupt
    ret |= hal_i2c_intr_clr(para_hal_ctrl);
    // 2.reset master
    ret |= hal_i2c_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_reset(para_hal_ctrl, 0);
    // 3.config set
    HAL_I2C_WRITE_REG(HAL_I2C_REG_CONFIG, para_hal_ctrl->config);
    if ((HAL_I2C_CONFIG_OEN_PUSH_EN & para_hal_ctrl->config) && para_hal_ctrl->oen_cnt)
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_CNT_TSUTHD_OPEN, BIT6, BIT6);
    }

    ret |= hal_i2c_dma_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_reset(para_hal_ctrl, 0);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0);
    return ret;
}

static s32 hal_i2c_count_calc(struct hal_i2c_ctrl *para_hal_ctrl, u32 para_src_clk, enum i2c_speed_mode para_speed_mode)
{
    u16 cnt_scl_low, cnt_scl_high, cnt_perid;
    u32 cnt_start_hd = 0;
    u32 cnt_stop_su  = 0;
    u8  cnt_supply   = 0;

    cnt_perid = (para_src_clk / para_hal_ctrl->speed);
    if (para_src_clk % para_hal_ctrl->speed)
    {
        cnt_perid += 1;
    }
    switch (para_speed_mode)
    {
        case HAL_I2C_SPEED_NORMAL:
            cnt_perid -= HAL_I2C_CNT_DEC_12M;
            cnt_scl_high = (cnt_perid / 2);
            cnt_scl_low  = cnt_perid - cnt_scl_high;
            cnt_supply   = HAL_I2C_SUPPLY_PUSHPULL_12M;
            break;
        case HAL_I2C_SPEED_HIGH:
            cnt_perid -= HAL_I2C_CNT_DEC_54M;
            cnt_scl_high = (cnt_perid / 2);
            cnt_scl_low  = cnt_perid - cnt_scl_high;
            cnt_supply   = HAL_I2C_SUPPLY_PUSHPULL_54M;
            break;
        case HAL_I2C_SPEED_ULTRA:
            cnt_perid -= HAL_I2C_CNT_DEC_72M;
            cnt_scl_high = (cnt_perid / 2);
            cnt_scl_low  = cnt_perid - cnt_scl_high;
            cnt_supply   = HAL_I2C_SUPPLY_PUSHPULL_72M;
            break;
        default:
            cnt_scl_low  = 0;
            cnt_scl_high = 0;
            dmsg_i2c_halerr("i2c-%d mode err\n", para_hal_ctrl->group);
            break;
    } // end of switch(para_speed_mode)

    // why we need this calculate,see in hal_iic.h for OEN_WAIT_PUSHPULL_12M
    if ((HAL_I2C_CONFIG_OEN_PUSH_EN & para_hal_ctrl->config) && para_hal_ctrl->oen_cnt)
    {
        cnt_scl_low -= cnt_supply;
        cnt_scl_high -= cnt_supply;
    }
    para_hal_ctrl->clock_count.cnt_scl_low   = cnt_scl_low;
    para_hal_ctrl->clock_count.cnt_scl_high  = cnt_scl_high;
    para_hal_ctrl->clock_count.cnt_sda_setup = cnt_scl_low / 2;
    para_hal_ctrl->clock_count.cnt_sda_latch = cnt_scl_high / 2;

    if (para_hal_ctrl->speed >= 250000)
    {
        cnt_start_hd = cnt_perid * (para_hal_ctrl->speed / 250000) + 1;
        cnt_stop_su  = cnt_perid * (para_hal_ctrl->speed / 250000) + 1;
    }
    else
    {
        cnt_start_hd = cnt_perid / (250000 / para_hal_ctrl->speed);
        cnt_stop_su  = cnt_perid / (250000 / para_hal_ctrl->speed);
    }
    if (0 == para_hal_ctrl->clock_count.cnt_start_hold)
    {
        para_hal_ctrl->clock_count.cnt_start_hold = (cnt_start_hd >= (u32)0xFFFF) ? 0xFFFF : (u16)cnt_start_hd;
    }
    if (0 == para_hal_ctrl->clock_count.cnt_stop_setup)
    {
        para_hal_ctrl->clock_count.cnt_stop_setup = (cnt_stop_su >= (u32)0xFFFF) ? 0xFFFF : (u16)cnt_stop_su;
    }

    if (0 == para_hal_ctrl->clock_count.cnt_stop_hold)
        para_hal_ctrl->clock_count.cnt_stop_hold = para_hal_ctrl->clock_count.cnt_scl_high;

    if (0 == para_hal_ctrl->clock_count.cnt_start_setup)
        para_hal_ctrl->clock_count.cnt_start_setup = para_hal_ctrl->clock_count.cnt_scl_high;

    return 0;
}

s32 hal_i2c_cnt_reg_set(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret = HAL_I2C_OK;

    ret |= hal_i2c_stop_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_stop_setup);
    ret |= hal_i2c_high_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_scl_high);
    ret |= hal_i2c_low_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_scl_low);
    ret |= hal_i2c_sda_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_sda_setup);
    ret |= hal_i2c_start_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_start_hold);
    ret |= hal_i2c_data_latch_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_sda_latch);
    ret |= hal_i2c_timeout_cnt_set(para_hal_ctrl, para_hal_ctrl->clock_count.cnt_timeout_delay);
    if (para_hal_ctrl->clock_count.cnt_start_setup | para_hal_ctrl->clock_count.cnt_stop_hold)
    {
        ret |= hal_i2c_thd_tsu_set(para_hal_ctrl);
    }

    return ret;
}

s32 hal_i2c_dma_reset(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_disenable)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_RESERVED, para_disenable, HAL_I2C_MASK_DMA_RESET);
    return HAL_I2C_OK;
}

s32 hal_i2c_reset(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_rst)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_CONFIG, para_rst, HAL_I2C_MASK_CFG_RST);

    return HAL_I2C_OK;
}

s32 hal_i2c_dma_intr_en(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_disenable)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_CONFIG, para_disenable << HAL_I2C_BIT_SHIFT_2, HAL_I2C_MASK_DMACFG_INTFLAG);

    return HAL_I2C_OK;
}

u32 hal_i2c_dma_trans_cnt(struct hal_i2c_ctrl *para_hal_ctrl)
{
    u32 ret;
    ret = HAL_I2C_READ_REG(HAL_I2C_REG_DMA_TC_2CH);
    ret |= (HAL_I2C_READ_REG(HAL_I2C_REG_DMA_TC_2DH) << 16);
    return ret;
}

s32 hal_i2c_dma_trigger(struct hal_i2c_ctrl *para_hal_ctrl)
{
    HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_TRIGGER, 1, HAL_I2C_MASK_DMATRIG);
    return HAL_I2C_OK;
}

s32 hal_i2c_dma_done_clr(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_val)
{
    s32 ret;
    ret = hal_i2c_dma_done_chk(para_hal_ctrl);
    if (ret)
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_DMA_TRANSFER_DONE, para_val, HAL_I2C_MASK_DMA_TRSFER_DONE);
        return -HAL_I2C_ERR;
    }
    return HAL_I2C_OK;
}

s32 hal_i2c_dma_stop_set(struct hal_i2c_ctrl *para_hal_ctrl, u8 para_stop)
{
    if (para_stop)
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_STOP_FMT_DIS, 0 << 5, HAL_I2C_MASK_STOP_FMT_DIS);
    }
    else
    {
        HAL_I2C_WRITE_REG_MASK(HAL_I2C_REG_STOP_FMT_DIS, 1 << 5, HAL_I2C_MASK_STOP_FMT_DIS);
    }
    return HAL_I2C_OK;
}

s32 hal_i2c_release(struct hal_i2c_ctrl *para_hal_ctrl)
{
    u64 count    = 0;
    u64 retry    = 0;
    u64 time_out = 0;
    s32 ret      = HAL_I2C_OK;

    if (0 == para_hal_ctrl->dma_en)
    {
        hal_i2c_intr_clr(para_hal_ctrl);
        ret |= hal_i2c_stop_bit_send(para_hal_ctrl);

        count = (HAL_I2C_READ_REG(HAL_I2C_REG_CNT_HIGH_PERIOD) * 4) + (HAL_I2C_READ_REG(HAL_I2C_REG_CNT_LOW_PERIOD) * 4)
                + HAL_I2C_READ_REG(HAL_I2C_REG_CNT_FOR_STOP) + HAL_I2C_READ_REG(HAL_I2C_REG_CNT_THD_SET);
        time_out = (count * 1000000) / para_hal_ctrl->max_srcclk_rate;
        time_out++;

        for (retry = 0; retry < time_out; retry++)
        {
            if (0 == hal_i2c_intr_flag(para_hal_ctrl))
                I2C_DELAY_N_US(1);
            else
                break;
        }

        hal_i2c_intr_clr(para_hal_ctrl);

        if (retry >= time_out)
        {
            ret = -HAL_I2C_STOP_CMD;
            dmsg_i2c_halerr("i2c-%d stop signal err!\n", para_hal_ctrl->group);
            return ret;
        }
    }

    return ret;
}

s32 hal_i2c_wn_mode_clr(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret = 0;
    ret |= hal_i2c_wn_grplen_set(para_hal_ctrl, 0);
    ret |= hal_i2c_wn_mode_set(para_hal_ctrl, 0);

    return ret;
}

s32 hal_i2c_wn_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len, u8 para_wnlen,
                     u16 para_waitcnt)
{
    s32 ret = HAL_I2C_OK;
    u8 *dma_virt_addr;
    u32 dma_transfer_cnt;

    ret |= hal_i2c_dma_reset(para_hal_ctrl, 1);     // dma reset
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1); // miu reset
    ret |= hal_i2c_dma_reset(para_hal_ctrl, 0);     // dma not reset
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0); // miu not reset

    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 0); // set write
    ret |= hal_i2c_wn_grplen_set(para_hal_ctrl, para_wnlen);
    ret |= hal_i2c_wn_mode_set(para_hal_ctrl, 1);              // enable wn mode
    ret |= hal_i2c_wn_wait_set(para_hal_ctrl, para_waitcnt);   // weather set wait counter
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x0);
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    dma_virt_addr = para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_virt_addr;
    memcpy(dma_virt_addr, para_pdata, para_len);
    hal_i2c_cache_flush(dma_virt_addr, para_len);

    ret |= hal_i2c_dma_transfer(para_hal_ctrl);

    dma_transfer_cnt = hal_i2c_dma_trans_cnt(para_hal_ctrl);
    if (dma_transfer_cnt != para_len)
    {
        dmsg_i2c_halerr("i2c-%d err dma write transform len:%d != para_len:%d\n", para_hal_ctrl->group,
                        dma_transfer_cnt, para_len);
        return -HAL_I2C_WRITE;
    }

    return ret;
}
s32 hal_i2c_dma_async_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = HAL_I2C_OK;

    // set read/write cmd
    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 1); // set read
    // set slave addr
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x00);
    // set miu addr
    /*done in hal_i2c_dma_init*/
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    // reset and reset miu

    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0);
    // set enable dma interrupt
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    // set clear done
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);

    // set trigger
    ret |= hal_i2c_dma_transfer(para_hal_ctrl);

    return ret;
}

s32 hal_i2c_wn_async_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len,
                           u8 para_wnlen, u16 para_waitcnt)
{
    s32 ret = HAL_I2C_OK;
    u8 *dma_virt_addr;

    ret |= hal_i2c_dma_reset(para_hal_ctrl, 1);     // dma reset
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1); // miu reset
    ret |= hal_i2c_dma_reset(para_hal_ctrl, 0);     // dma not reset
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0); // miu not reset

    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 0); // set write
    ret |= hal_i2c_wn_grplen_set(para_hal_ctrl, para_wnlen);
    ret |= hal_i2c_wn_mode_set(para_hal_ctrl, 1);              // enable wn mode
    ret |= hal_i2c_wn_wait_set(para_hal_ctrl, para_waitcnt);   // weather set wait counter
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x0);
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    dma_virt_addr = para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_virt_addr;
    memcpy(dma_virt_addr, para_pdata, para_len);
    hal_i2c_cache_flush(dma_virt_addr, para_len);

    ret |= hal_i2c_dma_transfer(para_hal_ctrl);

    return ret;
}

s32 hal_i2c_dma_async_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = 0;
    u8 *dma_virt_addr;

    // set read/write cmd
    ret |= hal_i2c_dma_rdwr_set(para_hal_ctrl, 0); // set write
    // set slave addr
    ret |= hal_i2c_dma_slvadr_set(para_hal_ctrl, para_slvadr); // set dma slave address
    // set cmd data
    ret |= hal_i2c_cmd_data_set(para_hal_ctrl, 0x0000);
    // set cmd len
    ret |= hal_i2c_cmd_len_set(para_hal_ctrl, 0x0);
    // set miu addr
    // done in hal_i2c_dma_init
    // set data length
    ret |= hal_i2c_dma_len_set(para_hal_ctrl, para_len);
    // reset and reset miu
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 1);
    ret |= hal_i2c_dma_miu_reset(para_hal_ctrl, 0);
    // copy data to miu addr
    dma_virt_addr = para_hal_ctrl->dma_ctrl.dma_addr_msg.dma_virt_addr;
    memcpy(dma_virt_addr, para_pdata, para_len);
    hal_i2c_cache_flush(dma_virt_addr, para_len);
    // set enable dma interrupt
    ret |= hal_i2c_dma_intr_en(para_hal_ctrl, 1);
    // set clear done
    ret |= hal_i2c_dma_done_clr(para_hal_ctrl, 1);

    // set trigger
    ret |= hal_i2c_dma_transfer(para_hal_ctrl);

    return 0;
} // end hal_i2c_dma_write()

s32 hal_i2c_write(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = 0;
    u8  slave_addr;

    slave_addr = (u8)((para_slvadr & 0x00FF) << 1);
    slave_addr &= HAL_I2C_SET_WRITEBIT_INDATA;
    if (para_hal_ctrl->dma_en)
    {
        ret |= hal_i2c_dma_write(para_hal_ctrl, para_slvadr, para_pdata, para_len);
    }
    else
    {
        ret |= hal_i2c_riu_write(para_hal_ctrl, slave_addr, para_pdata, para_len);
    }

    if (ret)
    {
        ret = -HAL_I2C_WRITE;
        dmsg_i2c_halwarn("i2c-%d write err\n", para_hal_ctrl->group);
    }

    return ret;
}

s32 hal_i2c_read(struct hal_i2c_ctrl *para_hal_ctrl, u16 para_slvadr, u8 *para_pdata, u32 para_len)
{
    s32 ret = 0;
    u8  slave_addr;

    slave_addr = (u8)((para_slvadr & 0x00FF) << 1);
    slave_addr |= HAL_I2C_SET_READBIT_INDATA;

    if (para_hal_ctrl->dma_en)
    {
        ret |= hal_i2c_dma_read(para_hal_ctrl, para_slvadr, para_pdata, para_len);
    }
    else
    {
        ret |= hal_i2c_riu_read(para_hal_ctrl, slave_addr, para_pdata, para_len);
    }
    if (ret)
    {
        ret = -HAL_I2C_READ;
        dmsg_i2c_halwarn("i2c-%d read err\n", para_hal_ctrl->group);
    }
    return ret;
}

s32 hal_i2c_speed_calc(struct hal_i2c_ctrl *para_hal_ctrl)
{
    u32 speed;
    s32 ret = 0;

    speed = para_hal_ctrl->speed;
    speed = (speed / 1000) * 1000;

    // select source clock
    if (speed > HAL_I2C_SPEED_1500KHZ)
    {
        printk("i2c-%d limit 1500KHz\n", para_hal_ctrl->group);
        speed = HAL_I2C_SPEED_1500KHZ;
    }

    para_hal_ctrl->speed = speed;

    hal_i2c_count_calc(para_hal_ctrl, para_hal_ctrl->max_srcclk_rate, HAL_I2C_SPEED_ULTRA);

    return ret;
}

s32 hal_i2c_init(struct hal_i2c_ctrl *para_hal_ctrl)
{
    s32 ret = 0;

    // reg miic config
    para_hal_ctrl->config &= 0x00;
    para_hal_ctrl->config |= (HAL_I2C_CONFIG_CLKSTRE_EN | HAL_I2C_CONFIG_FILTER_EN);
    if (para_hal_ctrl->push_pull)
    {
        para_hal_ctrl->config |= HAL_I2C_CONFIG_OEN_PUSH_EN;
    }
    if (para_hal_ctrl->dma_en)
    {
        para_hal_ctrl->config |= HAL_I2C_CONFIG_DMA_EN | HAL_I2C_CONFIG_INT_EN | HAL_I2C_CONFIG_TIMOUT_INT_EN;
    }
    // reg cnt default val
    para_hal_ctrl->clock_count.cnt_sda_setup     = 5;
    para_hal_ctrl->clock_count.cnt_sda_latch     = 5;
    para_hal_ctrl->clock_count.cnt_scl_high      = 25;
    para_hal_ctrl->clock_count.cnt_scl_low       = 27;
    para_hal_ctrl->clock_count.cnt_timeout_delay = 0xff;
    // dma confing
    para_hal_ctrl->dma_ctrl.dma_addr_mode  = HAL_I2C_ADDRMODE_NORMAL;
    para_hal_ctrl->dma_ctrl.dma_miu_chnnel = HAL_I2C_MIU_CHANNEL0;
    para_hal_ctrl->dma_ctrl.dma_miu_prioty = HAL_I2C_MIUPRI_LOW;
    para_hal_ctrl->dma_ctrl.dma_intr_en    = 1;
    // selcet source clk
    ret = hal_i2c_speed_calc(para_hal_ctrl);
    ret |= hal_i2c_cnt_reg_set(para_hal_ctrl);

    ret |= hal_i2c_config_set(para_hal_ctrl);
    if (para_hal_ctrl->dma_en)
        ret |= hal_i2c_dma_init(para_hal_ctrl);

    return ret;
}
