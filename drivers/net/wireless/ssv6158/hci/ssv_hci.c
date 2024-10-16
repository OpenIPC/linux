/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/firmware.h>
#include <linux/completion.h>
#include <ssv6200.h>

#include <smac/dev.h>
#include <hal.h>
#include "hctrl.h"

//include firmware binary header
#include "ssv6x5x-sw.h"


MODULE_AUTHOR("iComm-semi, Ltd");
MODULE_DESCRIPTION("HCI driver for SSV6xxx 802.11n wireless LAN cards.");
MODULE_SUPPORTED_DEVICE("SSV6xxx WLAN cards");
MODULE_LICENSE("Dual BSD/GPL");

#define MAX_HW_RESOURCE_FULL_CNT  100


static int ssv6xxx_hci_write_word(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 addr, u32 regval);
static int ssv6xxx_hci_check_tx_resource(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int *max_count);
static int ssv6xxx_hci_aggr_xmit(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int max_count);
static int ssv6xxx_hci_xmit(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int max_count);
static int ssv6xxx_hci_enqueue(struct ssv6xxx_hci_ctrl *hci_ctrl, struct sk_buff *skb, int txqid,
            bool force_trigger, u32 tx_flags);

static void ssv6xxx_hci_trigger_tx(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    /* if polling, wake up thread to send frame */
    wake_up_interruptible(&hci_ctrl->tx_wait_q);
}

static int ssv6xxx_hci_irq_enable(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    /* enable interrupt */
    HCI_IRQ_ENABLE(hci_ctrl);
    HCI_IRQ_SET_MASK(hci_ctrl, ~(hci_ctrl->int_mask));
    return 0;
}

static int ssv6xxx_hci_irq_disable(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    /* disable interrupt */
    HCI_IRQ_SET_MASK(hci_ctrl, 0xffffffff);
    HCI_IRQ_DISABLE(hci_ctrl);
    return 0;
}

static int ssv6xxx_hci_hwif_cleanup(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_HWIF_CLEANUP(hci_ctrl);
    return 0;
}

static int ssv6xxx_reset_skb(struct sk_buff *skb)
{
    struct skb_shared_info *s = skb_shinfo(skb);
    memset(s, 0, offsetof(struct skb_shared_info, dataref));
    atomic_set(&s->dataref, 1);
    memset(skb, 0, offsetof(struct sk_buff, tail));
    skb_reset_tail_pointer(skb);
    return 0;
}

#ifdef CONFIG_PM
static int ssv6xxx_hci_irq_suspend(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    /* disable interrupt except RX INT for host wakeup */
    HCI_IRQ_SET_MASK(hci_ctrl, 0xffffffff);
    HCI_IRQ_DISABLE(hci_ctrl);
    //HCI_IRQ_SET_MASK(hci_ctrl, 0xfffffffe);
    //HCI_IRQ_ENABLE(hci_ctrl);
    return 0;
}
#endif

static void *ssv6xxx_hci_open_firmware(char *user_mainfw)
{
    struct file *fp;

    fp = filp_open(user_mainfw, O_RDONLY, 0);
    if (IS_ERR(fp))
        fp = NULL;

    return fp;
}

static int ssv6xxx_hci_read_fw_block(char *buf, int len, void *image)
{
    struct file *fp = (struct file *)image;
    int rdlen;

    if (!image)
        return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    rdlen = kernel_read(fp, buf, len, &fp->f_pos);
#else
    rdlen = kernel_read(fp, fp->f_pos, buf, len);
    if (rdlen > 0)
        fp->f_pos += rdlen;
#endif

    return rdlen;
}

static void ssv6xxx_hci_close_firmware(void *image)
{
    if (image)
        filp_close((struct file *)image, NULL);
}

/* ####### firmware info #######
 * ------- block count        -------    4 bytes
 * ------- block0 sram addr   -------    4 bytes
 * ------- block0 sram len    -------    4 bytes
 * ------- block1 sram addr   -------    4 bytes
 * ------- block1 sram len    -------    4 bytes
 * ------- block2 sram addr   -------    4 bytes
 * ------- block2 sram len    -------    4 bytes
 * */
#define MAX_FIRMWARE_BLOCKS_CNT     3
#define MAC_FIRMWARE_BLOCKS_LEN     8
static int ssv6xxx_hci_get_firmware_info(struct ssv6xxx_hci_ctrl *hci_ctrl, void *fw_fp, u8 openfile)
{
    int offset = 0;
    int i = 0, len = 0;
    u8 buf[MAX_FIRMWARE_BLOCKS_CNT * MAC_FIRMWARE_BLOCKS_LEN];
    u32 fw_block_cnt = 0;
    u32 fw_block_info_len = 0;
    u32 start_addr = 0, fw_block_len = 0;

    memset(buf, 0, MAX_FIRMWARE_BLOCKS_CNT * MAC_FIRMWARE_BLOCKS_LEN);
    if (openfile) {
        // reset file pos
        ((struct file *)fw_fp)->f_pos = 0;

        // fw info len, 4 byte
        if (!(len = ssv6xxx_hci_read_fw_block((char *)&fw_block_cnt, sizeof(u32), fw_fp))) {
            offset = -1;
            goto out;
        }

        // fw block info
        fw_block_info_len = fw_block_cnt * MAC_FIRMWARE_BLOCKS_LEN;
        if (!(len = ssv6xxx_hci_read_fw_block((u8 *)buf, fw_block_info_len, fw_fp))) {
            offset = -1;
            goto out;
        }

    } else {
        memcpy(&fw_block_cnt, (u8 *)fw_fp, (sizeof(u32)));
        fw_block_info_len = fw_block_cnt * MAC_FIRMWARE_BLOCKS_LEN;
        memcpy((u8 *)buf, (u8 *)fw_fp+sizeof(u32), fw_block_info_len);
    }

    // parse fw block info
    for (i = 0; i < fw_block_cnt; i++) {
        start_addr =   (buf[i*8+0] << 0) | (buf[i*8+1] << 8) | (buf[i*8+2] << 16) | (buf[i*8+3] << 24);
        fw_block_len = (buf[i*8+4] << 0) | (buf[i*8+5] << 8) | (buf[i*8+6] << 16) | (buf[i*8+7] << 24);
        hci_ctrl->fw_info[i].block_len = fw_block_len;
        hci_ctrl->fw_info[i].block_start_addr = start_addr;
    }
    offset = 4 + fw_block_info_len;

out:
    return offset;
}

static u32 ssv6xxx_hci_write_firmware_to_sram(struct ssv6xxx_hci_ctrl *hci_ctrl, void *fw_buffer,
        void *fw_fp, int fw_len, u32 sram_start_addr, u8 openfile, bool need_checksum, bool *write_sram_err)
{
    int ret = 0, i = 0;
    u32 len = 0, total_len = 0;
    u32 sram_addr = sram_start_addr;
    u32 *fw_data32 = NULL;
    u32 fw_res_size = fw_len;
    u32 checksum = 0;
    u8 *pt=NULL;
    HCI_DBG_PRINT(hci_ctrl, "Writing firmware to SSV6XXX...\n");
    while (1) { // load fw block size
        if (fw_res_size < FW_BLOCK_SIZE)
            break;

        memset(fw_buffer, 0xA5, FW_BLOCK_SIZE);
        if (openfile) {
            //((struct file *)fw_fp)->f_pos = fw_offset;
            if (!(len = ssv6xxx_hci_read_fw_block((char*)fw_buffer, FW_BLOCK_SIZE, fw_fp))) {
                break;
            }
        } else {
            memcpy((void *)fw_buffer, (const void *)fw_fp, FW_BLOCK_SIZE);
            pt=(u8 *)(fw_fp);
            fw_fp = (void *)(pt+FW_BLOCK_SIZE);
        }

        fw_res_size -= FW_BLOCK_SIZE;
        total_len += FW_BLOCK_SIZE;

        if ((ret = HCI_LOAD_FW(hci_ctrl, sram_addr, (u8 *)fw_buffer, FW_BLOCK_SIZE)) != 0) {
            *write_sram_err = true;
            goto out;
        }

        // checksum
        if (need_checksum) {
            fw_data32 = (u32 *)fw_buffer;
            for (i = 0; i < (FW_BLOCK_SIZE / sizeof(u32)); i++)
                checksum += fw_data32[i];
        }

        sram_addr += FW_BLOCK_SIZE;
    }

    if (fw_res_size) { // load fw res size
        u32 cks_blk_cnt, cks_blk_res;

        cks_blk_cnt = fw_res_size / CHECKSUM_BLOCK_SIZE;
        cks_blk_res = fw_res_size % CHECKSUM_BLOCK_SIZE;
        memset(fw_buffer, 0xA5, FW_BLOCK_SIZE);

        if (0 != cks_blk_res) {
            cks_blk_cnt = cks_blk_cnt + 1;
        }

        if (openfile) {
            len = ssv6xxx_hci_read_fw_block((char*)fw_buffer, fw_res_size, fw_fp);
        } else {
            memcpy((void *)fw_buffer, (const void *)fw_fp, fw_res_size);
            pt=(u8 *)(fw_fp);
            fw_fp = (void *)(pt+fw_res_size);
        }

        fw_res_size = 0;
        total_len += fw_res_size;

        if ((ret = HCI_LOAD_FW(hci_ctrl, sram_addr, (u8 *)fw_buffer, cks_blk_cnt*CHECKSUM_BLOCK_SIZE)) != 0) {
            *write_sram_err = true;
            goto out;
        }

        // checksum
        if (need_checksum) {
            fw_data32 = (u32 *)fw_buffer;
            for (i = 0; i < (cks_blk_cnt * CHECKSUM_BLOCK_SIZE / sizeof(u32)); i++)
                checksum += fw_data32[i];
        }

        sram_addr += fw_res_size;
    }

out:
    return checksum;
}

static int _ssv6xxx_hci_load_firmware(struct ssv6xxx_hci_ctrl *hci_ctrl, u8 *firmware_name, u8 openfile)
{
    int ret = 0;
    u8   *fw_buffer = NULL;
    u32   block_count = 0, res_size = 0;
    u32   ilm_block_count = 0, bus_block_count = 0;
    void *fw_fp = NULL;
    //u8    interface = HCI_DEVICE_TYPE(hci_ctrl);
    u32   checksum = FW_CHECKSUM_INIT;
    u32   fw_checksum, fw_clkcnt;
    u32   retry_count = 1;
    int fw_start_offset = 0, fw_pos = 0;
    int fw_block_idx = 0;
    bool write_sram_err = false;
    u8 *pt=NULL;

    //if (hci_ctrl->redownload == 1) {
    /* force mcu jump to rom for always*/
    // Redownload firmware
    HCI_DBG_PRINT(hci_ctrl, "Re-download FW\n");
    HCI_JUMP_TO_ROM(hci_ctrl);
    //}

    // Enable all SRAM
    HAL_ENABLE_SRAM(hci_ctrl->shi->sh);

    // Load firmware
    if(openfile)
    {
        fw_fp = ssv6xxx_hci_open_firmware(firmware_name);
        if (!fw_fp) {
            HCI_DBG_PRINT(hci_ctrl, "failed to find firmware (%s)\n", firmware_name);
            ret = -1;
            goto out;
        }
    }
    else
    {
        fw_fp = (void *)ssv6x5x_sw_bin;
    }

    fw_start_offset = ssv6xxx_hci_get_firmware_info(hci_ctrl, fw_fp, openfile);
    if (fw_start_offset < 0) {
        HCI_DBG_PRINT(hci_ctrl, "Failed to get firmware information\n");
        ret = -1;
        goto out;
    }

    // Allocate buffer firmware aligned with FW_BLOCK_SIZE and padding with 0xA5 in empty space.
    fw_buffer = (u8 *)kzalloc(FW_BLOCK_SIZE, GFP_KERNEL);
    if (fw_buffer == NULL) {
        HCI_DBG_PRINT(hci_ctrl, "Failed to allocate buffer for firmware.\n");
        ret = -1;
        goto out;
    }

    do {
        // initial value
        checksum = FW_CHECKSUM_INIT;
        res_size = 0;

        // Disable MCU (USB ROM code must be alive for downloading FW, so USB doesn't do it)
        //if (!(interface == SSV_HWIF_INTERFACE_USB)) {
            ret = SSV_LOAD_FW_DISABLE_MCU(hci_ctrl);
            if (ret == -1)
                goto out;
        //}

        for (fw_block_idx = 0; fw_block_idx < MAX_FIRMWARE_BLOCKS_CNT; fw_block_idx++) {
            // calculate the fw block position
            if (fw_block_idx == 0)
                fw_pos += fw_start_offset;
            else
                fw_pos += hci_ctrl->fw_info[fw_block_idx-1].block_len;

            if (openfile)
                ((struct file *)fw_fp)->f_pos = fw_pos;
            else {
                fw_fp = (void *)ssv6x5x_sw_bin;
                pt=(u8 *)(fw_fp);
                fw_fp = (void *)(pt+fw_pos);
            }

            // write firmware to sram and calculate checksum(ILM, BUS)
            // fw_block_idx == 1, DLM
            checksum += ssv6xxx_hci_write_firmware_to_sram(hci_ctrl, (void *)fw_buffer, (void *)fw_fp,
                    hci_ctrl->fw_info[fw_block_idx].block_len, hci_ctrl->fw_info[fw_block_idx].block_start_addr,
                    openfile, ((fw_block_idx == 1) ? false : true), &write_sram_err);

            if (write_sram_err) {
                printk("Firmware \"%s\" fail to write sram.\n", firmware_name);
                ret = -1;
                goto out;
            }
        }

        // Calculate the final checksum.
        checksum = ((checksum >> 24) + (checksum >> 16) + (checksum >> 8) + checksum) & 0x0FF;
        checksum <<= 16;
#if 0
        // Reset CPU for USB switching ROM to firmware
        if (interface == SSV_HWIF_INTERFACE_USB) {
            if (-1 == (ret = SSV_RESET_CPU(hci_ctrl)))
                goto out;
        }
#endif
        // calculate ILM block count
        ilm_block_count = hci_ctrl->fw_info[0].block_len / CHECKSUM_BLOCK_SIZE;
        res_size = hci_ctrl->fw_info[0].block_len % CHECKSUM_BLOCK_SIZE;
        if (res_size)
            ilm_block_count++;

        // calculate BUS block count
        bus_block_count = hci_ctrl->fw_info[2].block_len / CHECKSUM_BLOCK_SIZE;
        res_size = hci_ctrl->fw_info[2].block_len % CHECKSUM_BLOCK_SIZE;
        if (res_size)
            bus_block_count++;

        block_count = ((ilm_block_count<<16) | (bus_block_count<<0));
        // Inform FW that how many blocks is downloaded such that FW can calculate the checksum.
        SSV_LOAD_FW_SET_STATUS(hci_ctrl, block_count);
        SSV_LOAD_FW_GET_STATUS(hci_ctrl, &fw_clkcnt);
        HCI_DBG_PRINT(hci_ctrl, "block_count = 0x%08x, reg = 0x%08x\n", block_count, fw_clkcnt);
        printk("block_count = 0x%08x, reg = 0x%08x\n", block_count, fw_clkcnt);
        // Release reset to let CPU run.
        SSV_LOAD_FW_ENABLE_MCU(hci_ctrl);
        HCI_DBG_PRINT(hci_ctrl, "Firmware \"%s\" loaded\n", firmware_name);
        // Wait FW to calculate checksum.
        msleep(50);
        // Check checksum result and set to complement value if checksum is OK.
        SSV_LOAD_FW_GET_STATUS(hci_ctrl, &fw_checksum);
        fw_checksum = fw_checksum & FW_STATUS_MASK;
        if (fw_checksum == checksum) {
            SSV_LOAD_FW_SET_STATUS(hci_ctrl, (~checksum & FW_STATUS_MASK));
            HCI_DBG_PRINT(hci_ctrl, "Firmware check OK.%04x = %04x\n", fw_checksum, checksum);
            ret = 0;
        } else {
            HCI_DBG_PRINT(hci_ctrl, "FW checksum error: %04x != %04x\n", fw_checksum, checksum);
            ret = -1;
        }

        if (0 == ret) // firmware check ok
	        break;

    } while (--retry_count);

out:
    if (fw_buffer != NULL) {
        kfree(fw_buffer);
    }

    if (openfile) {
        if (fw_fp) {
            ssv6xxx_hci_close_firmware(fw_fp);
        }
    }

    return ret;
}

static int ssv6xxx_hci_load_firmware_openfile(struct ssv6xxx_hci_ctrl *hci_ctrl, u8 *firmware_name)
{
    return _ssv6xxx_hci_load_firmware(hci_ctrl, firmware_name, 1);
}

static int ssv6xxx_hci_load_firmware_fromheader(struct ssv6xxx_hci_ctrl *hci_ctrl, u8 *firmware_name)
{
    return _ssv6xxx_hci_load_firmware(hci_ctrl, firmware_name, 0);
}

// Due to USB Rx task always keeps running in background
// It must start/stop USB acc of Rx endpoint(Ep4) while HCI start/stop
// Don't stop Ep 1,2 here for accessing register and don't stop Ep 3 here for flush buffered TX data in HCI.
static int ssv6xxx_hci_start_acc(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_START_USB_ACC(hci_ctrl, 4);
    return 0;
}

static int ssv6xxx_hci_stop_acc(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_STOP_USB_ACC(hci_ctrl, 4);
    return 0;
}

static int ssv6xxx_hci_start(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    int i = 0;

    // for all wifi station sw txq
    for (i = 0; i <= SSV_SW_TXQ_ID_MNG; i++)
        hci_ctrl->sw_txq[i].paused = false;

    return 0;
}

static int ssv6xxx_hci_stop(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    int i = 0;

    // for all wifi sw txq
    for (i = 0; i <= SSV_SW_TXQ_ID_MNG; i++)
        hci_ctrl->sw_txq[i].paused = true;

    return 0;
}

static int ssv6xxx_hci_hcmd_start(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    // fow cmd sw txq
    hci_ctrl->sw_txq[SSV_SW_TXQ_ID_WIFI_CMD].paused = false;
    return 0;
}

static int ssv6xxx_hci_ble_start(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    // fow ble sw txq
    hci_ctrl->sw_txq[SSV_SW_TXQ_ID_BLE_PDU].paused = false;
    return 0;
}

static int ssv6xxx_hci_ble_stop(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    // fow ble sw txq
    hci_ctrl->sw_txq[SSV_SW_TXQ_ID_BLE_PDU].paused = true;
    return 0;
}

#ifdef CONFIG_PM
static int ssv6xxx_hci_suspend(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    ssv6xxx_hci_stop(hci_ctrl);
    ssv6xxx_hci_irq_suspend(hci_ctrl);
    ssv6xxx_hci_stop_acc(hci_ctrl);
    return 0;
}

static int ssv6xxx_hci_resume(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    ssv6xxx_hci_irq_enable(hci_ctrl);
    ssv6xxx_hci_start_acc(hci_ctrl);
    ssv6xxx_hci_start(hci_ctrl);
    HCI_IRQ_TRIGGER(hci_ctrl);
    return 0;
}
#endif

static void ssv6xxx_hci_write_hw_config(struct ssv6xxx_hci_ctrl *hci_ctrl, int val)
{
    hci_ctrl->write_hw_config = val;
}

static int ssv6xxx_hci_read_word(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 addr, u32 *regval)
{
    int ret = HCI_REG_READ(hci_ctrl, addr, regval);
    return ret;
}

static int ssv6xxx_hci_write_word(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 addr, u32 regval)
{
    if (hci_ctrl->write_hw_config && (hci_ctrl->shi->write_hw_config_cb != NULL))
        hci_ctrl->shi->write_hw_config_cb((void *)SSV_SC(hci_ctrl), addr, regval);

    return HCI_REG_WRITE(hci_ctrl, addr, regval);
}

static int ssv6xxx_hci_burst_read_word(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 *addr, u32 *regval, u8 reg_amount)
{
    int ret = HCI_BURST_REG_READ(hci_ctrl, addr, regval, reg_amount);
    return ret;
}

static int ssv6xxx_hci_burst_write_word(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 *addr, u32 *regval, u8 reg_amount)
{
    return HCI_BURST_REG_WRITE(hci_ctrl, addr, regval, reg_amount);
}

static int ssv6xxx_hci_load_fw(struct ssv6xxx_hci_ctrl *hci_ctrl, u8 *firmware_name, u8 openfile)
{
    int ret = 0;

    SSV_LOAD_FW_PRE_CONFIG_DEVICE(hci_ctrl);

    if (openfile)
        ret = ssv6xxx_hci_load_firmware_openfile(hci_ctrl, firmware_name);
    else
        ret = ssv6xxx_hci_load_firmware_fromheader(hci_ctrl, firmware_name);
#if 0 //Keep it, but not used.
    ret = ssv6xxx_hci_load_firmware_request(hci_ctrl, firmware_name);
#endif

    // Sleep to let SSV6XXX get ready.
    msleep(50);

    if (ret == 0)
        SSV_LOAD_FW_POST_CONFIG_DEVICE(hci_ctrl);

    return ret;
}

static int ssv6xxx_hci_write_sram(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 addr, u8 *data, u32 size)
{
    return HCI_SRAM_WRITE(hci_ctrl, addr, data, size);
}

static int ssv6xxx_hci_pmu_wakeup(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_PMU_WAKEUP(hci_ctrl);
    return 0;
}

static int ssv6xxx_hci_interface_reset(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_IFC_RESET(hci_ctrl);
    return 0;
}

static int ssv6xxx_hci_sysplf_reset(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 addr, u32 value)
{
    HCI_SYSPLF_RESET(hci_ctrl, addr, value);
    return 0;
}

#ifdef CONFIG_PM
static int ssv6xxx_hci_hwif_suspend(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_HWIF_SUSPEND(hci_ctrl);
    return 0;
}

static int ssv6xxx_hci_hwif_resume(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    HCI_HWIF_RESUME(hci_ctrl);
    return 0;
}
#endif

static int ssv6xxx_hci_set_cap(struct ssv6xxx_hci_ctrl *hci_ctrl, enum ssv_hci_cap cap, bool enable)
{
    if (enable) {
        hci_ctrl->hci_cap |= BIT(cap);
    } else {
        hci_ctrl->hci_cap &= (~BIT(cap));
    }

    return 0;
}

static int ssv6xxx_hci_set_trigger_conf(struct ssv6xxx_hci_ctrl *hci_ctrl, bool en, u32 qlen, u32 pkt_size, u32 timeout)
{
    hci_ctrl->tx_trigger_en = en;
    hci_ctrl->tx_trigger_qlen = qlen;
    hci_ctrl->tx_trigger_pkt_size = pkt_size;
    hci_ctrl->task_timeout = timeout;
    return 0;
}

static void ssv6xxx_hci_cmd_done(struct ssv6xxx_hci_ctrl *hci_ctrl, struct sk_buff *skb)
{
    struct cfg_host_event *h_evt = NULL;

    if (hci_ctrl->ignore_cmd)
        return;

    h_evt = (struct cfg_host_event *)skb->data;
    if ((h_evt->c_type == HOST_EVENT) &&
            (hci_ctrl->blocking_seq_no == h_evt->blocking_seq_no)) {
        hci_ctrl->blocking_seq_no = 0;
        complete(&hci_ctrl->cmd_done);
    }
}

static void ssv6xxx_hci_ignore_cmd(struct ssv6xxx_hci_ctrl *hci_ctrl, bool val)
{
    hci_ctrl->ignore_cmd = val;
    if (hci_ctrl->blocking_seq_no != 0) {
        hci_ctrl->blocking_seq_no = 0;
        complete(&hci_ctrl->cmd_done);
    }
}

static int ssv6xxx_hci_send_cmd(struct ssv6xxx_hci_ctrl *hci_ctrl, struct sk_buff *skb)
{
#define MAX_BLOCKING_CMD_WAIT_PERIOD 5000
    int ret = -1, retval = 0;
    unsigned long expire = msecs_to_jiffies(MAX_BLOCKING_CMD_WAIT_PERIOD);
    u32 blocking_seq_no = 0;
    struct cfg_host_cmd *host_cmd = NULL;
    static u8 host_cmd_seq_no = 1;

    if (hci_ctrl->ignore_cmd) {
        return 0;
    }

    mutex_lock(&hci_ctrl->cmd_mutex);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    blocking_seq_no = host_cmd->blocking_seq_no;
    if (blocking_seq_no) {
        // update host cmd blocking seqno, blocking seqno must be unique
        host_cmd->blocking_seq_no |= (host_cmd_seq_no << 24);
        blocking_seq_no = host_cmd->blocking_seq_no;
        hci_ctrl->blocking_seq_no = host_cmd->blocking_seq_no;
        host_cmd_seq_no++;
        if (host_cmd_seq_no == 128)
            host_cmd_seq_no = 1;
    }

    ret = ssv6xxx_hci_enqueue(hci_ctrl, (void *)skb, SSV_SW_TXQ_ID_WIFI_CMD, false, 0);
    if (-1 == ret) {
        HCI_DBG_PRINT(hci_ctrl, "ssv6xxx_hci_send_cmd fail......\n");
        retval = -1;
        goto out;
    }

    if (blocking_seq_no) {
        if (!wait_for_completion_interruptible_timeout(&hci_ctrl->cmd_done, expire)) {
            printk("Cannot finish the command, host_cmd->h_cmd = 0x%08x, blocking_seq_no = 0x%08x\n",
                    host_cmd->h_cmd, host_cmd->blocking_seq_no);
        }
    }
out:
    mutex_unlock(&hci_ctrl->cmd_mutex);

    return retval;

}

static int ssv6xxx_hci_enqueue(struct ssv6xxx_hci_ctrl *hci_ctrl, struct sk_buff *skb, int txqid,
            bool force_trigger, u32 tx_flags)
{
    struct ssv_sw_txq *sw_txq;
    //unsigned long flags;
    int qlen = 0;
    int pkt_size = skb->len;

    BUG_ON(txqid >= SSV_SW_TXQ_NUM || txqid < 0);
    if (txqid >= SSV_SW_TXQ_NUM || txqid < 0)
        return -1;

    sw_txq = &hci_ctrl->sw_txq[txqid];

    sw_txq->tx_flags = tx_flags;
    if (tx_flags & HCI_FLAGS_ENQUEUE_HEAD)
        skb_queue_head(&sw_txq->qhead, skb);
    else
        skb_queue_tail(&sw_txq->qhead, skb);

    atomic_inc(&hci_ctrl->sw_txq_cnt);
    qlen = (int)skb_queue_len(&sw_txq->qhead);

    if (0 != hci_ctrl->tx_trigger_en) {
        // check data txq len to trigger task
        if ((txqid < (SSV_SW_TXQ_NUM-1)) &&
            (false == force_trigger) &&
            (qlen < hci_ctrl->tx_trigger_qlen) &&
            (hci_ctrl->tx_trigger_pkt_size <= pkt_size)) {

            return qlen;
        }
    }

    ssv6xxx_hci_trigger_tx(hci_ctrl);

    return qlen;
}

static int ssv6xxx_hci_txq_len(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    int tx_count = 0;

    tx_count = atomic_read(&hci_ctrl->sw_txq_cnt);
    return tx_count;
}

static bool ssv6xxx_hci_is_txq_empty(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    struct ssv_sw_txq *sw_txq;
    BUG_ON(txqid >= SSV_SW_TXQ_NUM);
    if (txqid >= SSV_SW_TXQ_NUM)
        return false;

    sw_txq = &hci_ctrl->sw_txq[txqid];
    if (skb_queue_len(&sw_txq->qhead) <= 0)
        return true;
    return false;
}

static int ssv6xxx_hci_txq_flush(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    struct ssv_sw_txq *sw_txq;
    struct sk_buff *skb = NULL;
    int txqid;

    // flush wifi sw_txq
    for (txqid = 0; txqid <= SSV_SW_TXQ_ID_MNG; txqid++) {
        sw_txq = &hci_ctrl->sw_txq[txqid];
        while((skb = skb_dequeue(&sw_txq->qhead))) {
            hci_ctrl->shi->hci_tx_buf_free_cb (skb, (void *)SSV_SC(hci_ctrl));
            atomic_dec(&hci_ctrl->sw_txq_cnt);
        }
    }

    /* free tx frame, it should update flowctl status */
    hci_ctrl->shi->hci_update_flowctl_cb((void *)(SSV_SC(hci_ctrl)));
    return 0;
}

static int ssv6xxx_hci_ble_txq_flush(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    struct ssv_sw_txq *sw_txq;
    struct sk_buff *skb = NULL;

    // flush ble sw_txq
    sw_txq = &hci_ctrl->sw_txq[SSV_SW_TXQ_ID_BLE_PDU];
    while((skb = skb_dequeue(&sw_txq->qhead))) {
        hci_ctrl->shi->hci_tx_buf_free_cb (skb, (void *)SSV_SC(hci_ctrl));
        atomic_dec(&hci_ctrl->sw_txq_cnt);
    }

    /* free tx frame, it should update flowctl status */
    hci_ctrl->shi->hci_update_flowctl_cb((void *)(SSV_SC(hci_ctrl)));
    return 0;
}

static int ssv6xxx_hci_hcmd_txq_flush(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    struct ssv_sw_txq *sw_txq;
    struct sk_buff *skb = NULL;

    // flush hostcmd sw_txq
    sw_txq = &hci_ctrl->sw_txq[SSV_SW_TXQ_ID_WIFI_CMD];
    while((skb = skb_dequeue(&sw_txq->qhead))) {
        hci_ctrl->shi->hci_tx_buf_free_cb (skb, (void *)SSV_SC(hci_ctrl));
        atomic_dec(&hci_ctrl->sw_txq_cnt);
    }

    /* free tx frame, it should update flowctl status */
    hci_ctrl->shi->hci_update_flowctl_cb((void *)(SSV_SC(hci_ctrl)));
    return 0;
}

static int ssv6xxx_hci_txq_flush_by_sta(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    struct ssv_sw_txq *sw_txq;
    struct sk_buff *skb = NULL;

    if (false == ssv6xxx_hci_is_sw_sta_txq(txqid))
        return -1;

    sw_txq = &hci_ctrl->sw_txq[txqid];
    while((skb = skb_dequeue(&sw_txq->qhead))) {
        hci_ctrl->shi->hci_tx_buf_free_cb (skb, (void *)SSV_SC(hci_ctrl));
        atomic_dec(&hci_ctrl->sw_txq_cnt);
    }

    /* free tx frame, it should update flowctl status */
    hci_ctrl->shi->hci_update_flowctl_cb((void *)(SSV_SC(hci_ctrl)));
    return 0;
}

static void ssv6xxx_hci_txq_lock_by_sta(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    if (false == ssv6xxx_hci_is_sw_sta_txq(txqid))
        return;

    mutex_lock(&hci_ctrl->sw_txq[txqid].txq_lock);
}

static void ssv6xxx_hci_txq_unlock_by_sta(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    if (false == ssv6xxx_hci_is_sw_sta_txq(txqid))
        return;

    mutex_unlock(&hci_ctrl->sw_txq[txqid].txq_lock);
}

static int ssv6xxx_hci_txq_pause(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 hw_txq_mask)
{
    struct ssv_sw_txq *sw_txq;
    int txqid;

    if (SSV_SC(hci_ctrl) == NULL){
        HCI_DBG_PRINT(hci_ctrl, "%s: can't pause due to software structure not initialized !!\n", __func__);
        return 1;
    }

    mutex_lock(&hci_ctrl->hw_txq_mask_lock);
    hci_ctrl->hw_txq_mask |= (hw_txq_mask & 0x1F);

    /* Pause all STA SWTXQ */
    for(txqid=0; txqid<=SSV_SW_TXQ_ID_STAMAX; txqid++) {
        sw_txq = &hci_ctrl->sw_txq[txqid];
        sw_txq->paused = true;
    }
    /* Pause MNG SWTXQ */
    sw_txq = &hci_ctrl->sw_txq[SSV_SW_TXQ_ID_MNG];
    sw_txq->paused = true;

    /* halt hardware tx queue */
    HAL_UPDATE_TXQ_MASK(SSV_SH(hci_ctrl), hci_ctrl->hw_txq_mask);
    mutex_unlock(&hci_ctrl->hw_txq_mask_lock);


    //HCI_DBG_PRINT(hci_ctrl, "%s(): hci_ctrl->txq_mas=0x%x\n", __FUNCTION__, hci_ctrl->hw_txq_mask);
    return 0;
}

static int ssv6xxx_hci_txq_resume(struct ssv6xxx_hci_ctrl *hci_ctrl, u32 hw_txq_mask)
{
    struct ssv_sw_txq *sw_txq;
    int txqid;

    if (SSV_SC(hci_ctrl) == NULL){
        HCI_DBG_PRINT(hci_ctrl, "%s: can't resume due to software structure not initialized !!\n", __func__);
        return 1;
    }

    /* resume hardware tx queue */
    mutex_lock(&hci_ctrl->hw_txq_mask_lock);
    hci_ctrl->hw_txq_mask &= ~(hw_txq_mask & 0x1F);
    HAL_UPDATE_TXQ_MASK(SSV_SH(hci_ctrl), hci_ctrl->hw_txq_mask);

    /* Resume all STA SWTXQ */
    for(txqid=0; txqid<=SSV_SW_TXQ_ID_STAMAX; txqid++) {
        sw_txq = &hci_ctrl->sw_txq[txqid];
        sw_txq->paused = false;
    }
    /* Resume MNG SWTXQ */
    sw_txq = &hci_ctrl->sw_txq[SSV_SW_TXQ_ID_MNG];
    sw_txq->paused = false;

    mutex_unlock(&hci_ctrl->hw_txq_mask_lock);
    ssv6xxx_hci_trigger_tx(hci_ctrl);

    //HCI_DBG_PRINT(hci_ctrl, "%s(): hci_ctrl->txq_mas=0x%x\n", __FUNCTION__, hci_ctrl->txq_mask);
    return 0;
}

static void ssv6xxx_hci_txq_pause_by_sta(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    struct ssv_sw_txq *sw_txq;

    if (false == ssv6xxx_hci_is_sw_sta_txq(txqid))
        return;

    sw_txq = &hci_ctrl->sw_txq[txqid];
    sw_txq->paused = true;
}

static void ssv6xxx_hci_txq_resume_by_sta(struct ssv6xxx_hci_ctrl *hci_ctrl, int txqid)
{
    struct ssv_sw_txq *sw_txq;

    if (false == ssv6xxx_hci_is_sw_sta_txq(txqid))
        return;

    sw_txq = &hci_ctrl->sw_txq[txqid];
    sw_txq->paused = false;
}
#if 0
static int _do_force_tx (struct ssv6xxx_hci_ctrl *hci_ctrl, int *err)
{
    int                q_num;
    int                tx_count = 0;
    struct ssv_sw_txq *sw_txq;
    int max_count = 0;
    int (*handler)(struct ssv6xxx_hci_ctrl *, struct ssv_sw_txq *, int);
    int ret = 0;

    if (hci_ctrl->hci_cap & BIT(HCI_CAP_TX_AGGR)) {
        handler = ssv6xxx_hci_aggr_xmit;
    } else {
        handler = ssv6xxx_hci_xmit;
    }
    for (q_num = (SSV_SW_TXQ_NUM - 1); q_num >= 0; q_num--)
    {
        sw_txq = &hci_ctrl->sw_txq[q_num];
        max_count = skb_queue_len(&sw_txq->qhead);
        if ((0 == max_count) || (true == sw_txq->paused)) {
            continue;
        }

        *err = ssv6xxx_hci_check_tx_resource(hci_ctrl, sw_txq, &max_count);
        if (*err == 0) {
            ret = handler(hci_ctrl, sw_txq, max_count);
            if (ret < 0) {
                *err = ret;
            } else {
                tx_count += ret;
            }
        } else if (*err < 0) {
            break;
        }
    }

    return tx_count;
}
#endif

/**
 * int ssv6xxx_hci_aggr_xmit() - send the specified number of frames for a specified
 *                          tx queue to SDIO with hci_tx_aggr enabled.
 *
 * @ struct ssv_sw_txq *sw_txq: the output queue to send.
 * @ int max_count: the maximal number of frames to send.
 */
static int ssv6xxx_hci_aggr_xmit(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int max_count)
{
    struct sk_buff_head tx_cb_list;
    struct sk_buff *skb = NULL;
    int tx_count = 0, ret = 0;
    int reason = -1;
    // start of variable for hci tx aggr
    struct sk_buff *p_aggr_skb = hci_ctrl->p_tx_aggr_skb;
    static u32 left_len = 0;
    u32 cpy_len = 0;
    static u8 *p_cpy_dest = NULL;
    static bool aggr_done = false;
    static bool new_round  = 1;
    // end of variable for hci tx aggr

    skb_queue_head_init(&tx_cb_list);

    for(tx_count=0; tx_count < max_count; tx_count++) {

        if (sw_txq->paused) {
            // Is it possible to stop/paused in middle of for loop?
            HCI_DBG_PRINT(hci_ctrl, "ssv6xxx_hci_aggr_xmit - sw_txq->pause = false\n");
            aggr_done = true;
            goto xmit_out;
        }

        mutex_lock(&sw_txq->txq_lock);
        skb = skb_dequeue(&sw_txq->qhead);
        if (!skb){
            HCI_DBG_PRINT(hci_ctrl, "ssv6xxx_hci_xmit - queue empty\n");
            mutex_unlock(&sw_txq->txq_lock);
            goto xmit_out;
        }

        //(STA0....STAn) (MNG) (BLE_DATA) (BLE_CMD) (WIFI_CMD)
        // for wifi packet, it must update txdesc
        if (sw_txq->txq_no <= SSV_SW_TXQ_ID_MNG) {
            if (hci_ctrl->shi->hci_txtput_reason_cb)
                reason = hci_ctrl->shi->hci_txtput_reason_cb(skb, (void *)(SSV_SC(hci_ctrl)));

            if ((reason != ID_TRAP_SW_TXTPUT) && hci_ctrl->shi->hci_update_wep_cb)
                hci_ctrl->shi->hci_update_wep_cb(skb, (void *)(SSV_SC(hci_ctrl)));
        }
        mutex_unlock(&sw_txq->txq_lock);
        //ampdu1.3
        if(1 == new_round) {
            p_cpy_dest = p_aggr_skb->data;
            left_len = HCI_TX_AGGR_SKB_LEN;
            new_round = 0;
        }

        if ((skb->len) & 0x3) {
            cpy_len = (skb->len) + (4 - ((skb->len) & 0x3));
        } else {
            cpy_len = skb->len;
        }

        if (left_len > (cpy_len + 8)) {
            *((u32 *)(p_cpy_dest)) = ((cpy_len + 8) | ((cpy_len + 8) << 16));
            *((u32 *)(p_cpy_dest+4)) = 0;
            p_cpy_dest += 8;
            memcpy(p_cpy_dest, skb->data, cpy_len);
            p_cpy_dest +=  cpy_len;

            left_len -= (8 + cpy_len);

            skb_put(p_aggr_skb, 8 + cpy_len);

            /*
            * Tx frame should enqueue tx_cb_list after IF_SEND()
            * hci_post_tx_cb() will handle tx frame. ex: drop, send to mac80211 layer
            */
            skb_queue_tail(&tx_cb_list, skb);
            sw_txq->tx_pkt++;

        } else {
            // packet will be add again in next round
            skb_queue_head(&sw_txq->qhead, skb);
            aggr_done = true;
            break;
        }

    } //end of for(tx_count=0; tx_count<max_count; tx_count++)
    if (tx_count == max_count) {
        aggr_done = true;
    }
xmit_out:
    if (aggr_done) {
        if ((p_aggr_skb) && (new_round==0)) {
            *((u32 *)(p_cpy_dest)) = 0;
            p_cpy_dest += 4;
            skb_put(p_aggr_skb, 4);

            //printk(KERN_ERR "hci tx aggr len=%d\n", p_aggr_skb->len);
            ret = IF_SEND(hci_ctrl, (void *)p_aggr_skb, p_aggr_skb->len, sw_txq->txq_no);
            if (ret < 0) {
                BUG_ON(1);
                //printk("ssv6xxx_hci_aggr_xmit fail, err %d\n", ret);
                //ret = -1;
            }
            aggr_done = false;
            ssv6xxx_reset_skb(p_aggr_skb);
            new_round = 1;
        }
    }

    if (hci_ctrl->shi->hci_post_tx_cb) {
        hci_ctrl->shi->hci_post_tx_cb (&tx_cb_list, (void *)(SSV_SC(hci_ctrl)));
    }

    return (ret == 0) ? tx_count : ret;
}
/**
 * int ssv6xxx_hci_xmit() - send the specified number of frames for a specified
 *                          tx queue to SDIO.
 *
 * @ struct ssv_sw_txq *sw_txq: the output queue to send.
 * @ int max_count: the maximal number of frames to send.
 */
static int ssv6xxx_hci_xmit(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int max_count)
{
    struct sk_buff_head tx_cb_list;
    struct sk_buff *skb = NULL;
    int tx_count = 0, ret = 0;
    int reason = -1;

    skb_queue_head_init(&tx_cb_list);

    for(tx_count=0; tx_count<max_count; tx_count++) {

        if (sw_txq->paused) {
            HCI_DBG_PRINT(hci_ctrl, "ssv6xxx_hci_xmit - sw_txq->pause = false\n");
            goto xmit_out;
        }

        mutex_lock(&sw_txq->txq_lock);
        skb = skb_dequeue(&sw_txq->qhead);
        if (!skb){
            HCI_DBG_PRINT(hci_ctrl, "ssv6xxx_hci_xmit - queue empty\n");
            mutex_unlock(&sw_txq->txq_lock);
            goto xmit_out;
        }

        //(STA0....STAn) (MNG) (BLE_DATA) (BLE_CMD) (WIFI_CMD)
        // for wifi packet, it must update txdesc
        if (sw_txq->txq_no <= SSV_SW_TXQ_ID_MNG) {
            if (hci_ctrl->shi->hci_txtput_reason_cb)
                reason = hci_ctrl->shi->hci_txtput_reason_cb(skb, (void *)(SSV_SC(hci_ctrl)));

            if ((reason != ID_TRAP_SW_TXTPUT) && hci_ctrl->shi->hci_update_wep_cb)
                hci_ctrl->shi->hci_update_wep_cb(skb, (void *)(SSV_SC(hci_ctrl)));
        }

        mutex_unlock(&sw_txq->txq_lock);

        //ampdu1.3
        ret = IF_SEND(hci_ctrl, (void *)skb, skb->len, sw_txq->txq_no);

        if (ret < 0) {
            BUG_ON(1);
            //printk("ssv6xxx_hci_xmit fail, err %d\n", ret);
            // remove tx desc, which will be add again in next round
            //skb_pull(skb, TXPB_OFFSET);
            //skb_queue_head(&sw_txq->qhead, skb);
            //ret = -1;
            break;
        }

        /*
         * Tx frame should enqueue tx_cb_list after IF_SEND()
         * hci_post_tx_cb() will handle tx frame. ex: drop, send to mac80211 layer
         */
        skb_queue_tail(&tx_cb_list, skb);
        sw_txq->tx_pkt++;
    } //end of for(tx_count=0; tx_count<max_count; tx_count++)
xmit_out:

    if (hci_ctrl->shi->hci_post_tx_cb) {
        hci_ctrl->shi->hci_post_tx_cb (&tx_cb_list, (void *)(SSV_SC(hci_ctrl)));
    }

    return (ret == 0) ? tx_count : ret;
}


/* return value
 * -EIO, hw error or no operation struct
 * 0, success
 * 1, no hw resource
 */
#define HCI_TX_NO_RESOURCE      1
static int ssv6xxx_hci_check_tx_resource(struct ssv6xxx_hci_ctrl *hci_ctrl, struct ssv_sw_txq *sw_txq, int *p_max_count)
{
    struct ssv6xxx_hci_txq_info txq_info;
    int hci_used_id = -1;
    int ret = 0;
    int tx_count = 0;
    struct sk_buff *skb;
    int page_count = 0;
    unsigned long flags;
#if defined(CONFIG_PREEMPT_NONE) && defined(CONFIG_SDIO_FAVOR_RX)
    struct ssv6xxx_hwif_ops *ifops = hci_ctrl->shi->if_ops;
    int sdio_intr_status;
#endif
    int tx_req_cnt=0;
    struct ssv6xxx_hw_resource hw_resource;

    if (SSV_SC(hci_ctrl) != NULL) {
        ret = HAL_READRG_TXQ_INFO(hci_ctrl->shi->sh ,(u32 *)&txq_info, &hci_used_id);
    } else {
        printk(KERN_ERR"%s: can't read txq_info due to software structure not initialized !!\n",  __func__);
        return -EIO;
    }
    if (ret < 0) {
        printk(KERN_ERR"%s: can't read txq_info\n",  __func__);
        return -EIO;
    }

    if (SSV6XXX_PAGE_TX_THRESHOLD(hci_ctrl) < txq_info.tx_use_page) {
        BUG_ON(1);
    }
    if (SSV6XXX_ID_TX_THRESHOLD(hci_ctrl) < txq_info.tx_use_id) {
        BUG_ON(1);
    }
    HCI_HWIF_GET_TX_REQ_CNT(hci_ctrl,&tx_req_cnt);

    if(sw_txq->txq_no==SSV_SW_TXQ_ID_WIFI_CMD)
    {
        hw_resource.free_tx_page =
            (int) SSV6XXX_PAGE_TX_THRESHOLD(hci_ctrl) - (int)txq_info.tx_use_page - tx_req_cnt*7;
    }
    else
    {
        hw_resource.free_tx_page =
            (int) SSV6XXX_PAGE_TX_THRESHOLD(hci_ctrl)-6 - (int)txq_info.tx_use_page - tx_req_cnt*7; //6: Reserved for WIFI CMD
    }
    hw_resource.free_tx_id = (int) SSV6XXX_ID_TX_THRESHOLD(hci_ctrl) - (int) txq_info.tx_use_id-tx_req_cnt;
    if ((hw_resource.free_tx_page <= 0) || (hw_resource.free_tx_id <= 0)) {
        hci_ctrl->hw_tx_resource_full_cnt++;
        if (hci_ctrl->hw_tx_resource_full_cnt > MAX_HW_RESOURCE_FULL_CNT) {
            hci_ctrl->hw_tx_resource_full = true;
            hci_ctrl->hw_tx_resource_full_cnt = 0;
        }
        (*p_max_count) = 0;
        return HCI_TX_NO_RESOURCE;
    }

    spin_lock_irqsave(&(sw_txq->qhead.lock), flags);
    for(tx_count = 0; tx_count < (*p_max_count); tx_count++) {
        if (0 == tx_count) {
            skb = skb_peek(&sw_txq->qhead);
        } else {
            skb = ssv6xxx_hci_skb_peek_next(skb, &sw_txq->qhead);
        }
        if (!skb){
            break;
        }
        page_count = (skb->len + SSV6200_PKT_HEADROOM_RSVD + SSV6200_TX_PKT_RSVD);
        if (page_count & HW_MMU_PAGE_MASK)
            page_count = (page_count >> HW_MMU_PAGE_SHIFT) + 1;
        else
            page_count = page_count >> HW_MMU_PAGE_SHIFT;

        if ((hw_resource.free_tx_page < page_count) || (hw_resource.free_tx_id <= 0)) {
            // If it cannot get tx resource more than 100 consecutive times,
            // it will set resource full flag and wait timeout.
            if (0 == tx_count) {
                hci_ctrl->hw_tx_resource_full_cnt++;
                if (hci_ctrl->hw_tx_resource_full_cnt > MAX_HW_RESOURCE_FULL_CNT) {
                    hci_ctrl->hw_tx_resource_full = true;
                    hci_ctrl->hw_tx_resource_full_cnt = 0;
                }
            }
            break;
        } else {
            hci_ctrl->hw_tx_resource_full_cnt = 0;
        }

        hw_resource.free_tx_page -= page_count;
        hw_resource.free_tx_id--;
    }
    spin_unlock_irqrestore(&(sw_txq->qhead.lock), flags);
    (*p_max_count) = tx_count;


#if defined(CONFIG_PREEMPT_NONE) && defined(CONFIG_SDIO_FAVOR_RX)
    if (SSV_HWIF_INTERFACE_SDIO == HCI_DEVICE_TYPE(hci_ctrl)) {
        //HCI_REG_READ(hci_ctrl, 0xc0000808, &sdio_intr_status);
        ifops->irq_getstatus(hci_ctrl->shi->dev, &sdio_intr_status);
        if (sdio_intr_status & SSV6XXX_INT_RX) {
            hci_ctrl->hw_sdio_rx_available = true;
            schedule();
        }
    } else {
        hci_ctrl->hw_sdio_rx_available = false;
    }
#endif

    return ((tx_count == 0) ? HCI_TX_NO_RESOURCE : 0);
}


static bool ssv6xxx_hci_is_frame_send(struct ssv6xxx_hci_ctrl *hci_ctrl)
{
    int q_num;
    struct ssv_sw_txq *sw_txq;

    if (hci_ctrl->hw_tx_resource_full)
        return false;

#if defined(CONFIG_PREEMPT_NONE) && defined(CONFIG_SDIO_FAVOR_RX)
    if (hci_ctrl->hw_sdio_rx_available) {
        hci_ctrl->hw_sdio_rx_available = false;
        return false;
    }
#endif
    for (q_num = (SSV_SW_TXQ_NUM - 1); q_num >= 0; q_num--) {
        sw_txq = &hci_ctrl->sw_txq[q_num];

        if (!sw_txq->paused && !ssv6xxx_hci_is_txq_empty(hci_ctrl, q_num))
            return true;
    }

    return false;
}

static void ssv6xxx_hci_isr_reset(struct work_struct *work)
{
    struct ssv6xxx_hci_ctrl *hci_ctrl;

    hci_ctrl = container_of(work, struct ssv6xxx_hci_ctrl, isr_reset_work);

    HCI_DBG_PRINT(hci_ctrl, "ISR Reset!!!");
    ssv6xxx_hci_irq_disable(hci_ctrl);
    ssv6xxx_hci_irq_enable(hci_ctrl);
}

static int ssv6xxx_hci_tx_task (void *data)
{
#define MAX_HCI_TX_TASK_SEND_FAIL       1000
    struct ssv6xxx_hci_ctrl *hci_ctrl = (struct ssv6xxx_hci_ctrl *)data;
    unsigned long     wait_period = 0;
    int err = 0;
    u32 timeout = (-1);
#if 1
    int q_num;
    struct ssv_sw_txq *sw_txq;
    int max_count = 0;
#endif
    printk(KERN_ERR "SSV6XXX HCI TX Task started.\n");

    while (!kthread_should_stop())
    {
        wait_period = msecs_to_jiffies(hci_ctrl->task_timeout);
        timeout = wait_event_interruptible_timeout(hci_ctrl->tx_wait_q,
                (  kthread_should_stop()
                   || ssv6xxx_hci_is_frame_send(hci_ctrl)),
                wait_period);

        if (kthread_should_stop())
        {
            hci_ctrl->hci_tx_task = NULL;
            printk(KERN_ERR "Quit HCI TX task loop...\n");
            break;
        }

        if (timeout == 0) {
            hci_ctrl->hw_tx_resource_full = false;
        }

        if (0 != timeout)
        {
            err = 0;
#if 1
            for (q_num = (SSV_SW_TXQ_NUM - 1); q_num >= 0; q_num--)
            {
                sw_txq = &hci_ctrl->sw_txq[q_num];
                max_count = skb_queue_len(&sw_txq->qhead);
                if ((0 == max_count) || (true == sw_txq->paused)) {
                    continue;
                }
                err = ssv6xxx_hci_check_tx_resource(hci_ctrl, sw_txq, &max_count);
                /* err
                 * -EIO, hw error or no operation struct
                 * 0, success
                 * 1, no hw resource
                  */
                if (err == 0) {
                    if (hci_ctrl->hci_cap & BIT(HCI_CAP_TX_AGGR)) {
                        ssv6xxx_hci_aggr_xmit(hci_ctrl, sw_txq, max_count);
                    } else {
                        ssv6xxx_hci_xmit(hci_ctrl, sw_txq, max_count);
                    }
                } else if (err < 0) {
                   break;
                }
            }
#else
            _do_force_tx(hci_ctrl, &err);
#endif
        }
    }

    hci_ctrl->hci_tx_task = NULL;

    printk(KERN_ERR "SSV6XXX HCI TX Task end.\n");
    return 0;
}

static struct ssv6xxx_hci_ops hci_ops =
{
    .hci_start            = ssv6xxx_hci_start,
    .hci_stop             = ssv6xxx_hci_stop,
    .hci_hcmd_start       = ssv6xxx_hci_hcmd_start,
    .hci_ble_start        = ssv6xxx_hci_ble_start,
    .hci_ble_stop         = ssv6xxx_hci_ble_stop,
#ifdef CONFIG_PM
    .hci_suspend          = ssv6xxx_hci_suspend,
    .hci_resume           = ssv6xxx_hci_resume,
#endif
    .hci_write_hw_config  = ssv6xxx_hci_write_hw_config,
    .hci_read_word        = ssv6xxx_hci_read_word,
    .hci_write_word       = ssv6xxx_hci_write_word,
    .hci_burst_read_word  = ssv6xxx_hci_burst_read_word,
    .hci_burst_write_word = ssv6xxx_hci_burst_write_word,
    .hci_tx               = ssv6xxx_hci_enqueue,
    .hci_tx_pause         = ssv6xxx_hci_txq_pause,
    .hci_tx_resume        = ssv6xxx_hci_txq_resume,
    .hci_tx_pause_by_sta  = ssv6xxx_hci_txq_pause_by_sta,
    .hci_tx_resume_by_sta = ssv6xxx_hci_txq_resume_by_sta,
    .hci_txq_flush        = ssv6xxx_hci_txq_flush,
    .hci_ble_txq_flush    = ssv6xxx_hci_ble_txq_flush,
    .hci_hcmd_txq_flush   = ssv6xxx_hci_hcmd_txq_flush,
    .hci_txq_flush_by_sta = ssv6xxx_hci_txq_flush_by_sta,
    .hci_txq_lock_by_sta  = ssv6xxx_hci_txq_lock_by_sta,
    .hci_txq_unlock_by_sta= ssv6xxx_hci_txq_unlock_by_sta,
    .hci_txq_len          = ssv6xxx_hci_txq_len,
    .hci_txq_empty        = ssv6xxx_hci_is_txq_empty,
    .hci_load_fw          = ssv6xxx_hci_load_fw,
    .hci_pmu_wakeup       = ssv6xxx_hci_pmu_wakeup,
    .hci_cmd_done         = ssv6xxx_hci_cmd_done,
    .hci_send_cmd         = ssv6xxx_hci_send_cmd,
    .hci_ignore_cmd       = ssv6xxx_hci_ignore_cmd,
    .hci_write_sram       = ssv6xxx_hci_write_sram,
    .hci_interface_reset  = ssv6xxx_hci_interface_reset,
    .hci_sysplf_reset     = ssv6xxx_hci_sysplf_reset,
#ifdef CONFIG_PM
    .hci_hwif_suspend     = ssv6xxx_hci_hwif_suspend,
    .hci_hwif_resume      = ssv6xxx_hci_hwif_resume,
#endif
    .hci_set_cap          = ssv6xxx_hci_set_cap,
    .hci_set_trigger_conf = ssv6xxx_hci_set_trigger_conf,
};


int ssv6xxx_hci_deregister(struct ssv6xxx_hci_info *shi)
{
    struct ssv6xxx_hci_ctrl *hci_ctrl;

    /**
     * Wait here until there is no frame on the hardware. Before
     * call this function, the RF shall be turned off to make sure
     * no more incoming frames. This function also disable interrupt
     * once no more frames.
     */
    printk(KERN_ERR "%s(): \n", __FUNCTION__);


    if (shi->hci_ctrl == NULL)
        return -1;

    hci_ctrl = shi->hci_ctrl;
    hci_ctrl->ignore_cmd = true;
    // flush txq packet
    ssv6xxx_hci_txq_flush(hci_ctrl);
    ssv6xxx_hci_ble_txq_flush(hci_ctrl);
    ssv6xxx_hci_hcmd_txq_flush(hci_ctrl);
    ssv6xxx_hci_irq_disable(hci_ctrl);
    ssv6xxx_hci_stop_acc(hci_ctrl);
    ssv6xxx_hci_hwif_cleanup(hci_ctrl);
    if (hci_ctrl->hci_tx_task != NULL)
    {
        printk(KERN_ERR "Stopping HCI TX task...\n");
        kthread_stop(hci_ctrl->hci_tx_task);
        while(hci_ctrl->hci_tx_task != NULL) {
            msleep(1);
        }
        printk(KERN_ERR "Stopped HCI TX task.\n");
    }

    if (hci_ctrl->p_tx_aggr_skb) {
        // free hci tx aggr buffer
        shi->skb_free((void *)(SSV_SC(hci_ctrl)), hci_ctrl->p_tx_aggr_skb);
    }

    shi->hci_ctrl = NULL;
    kfree(hci_ctrl);

    return 0;
}
EXPORT_SYMBOL(ssv6xxx_hci_deregister);


int ssv6xxx_hci_register(struct ssv6xxx_hci_info *shi, bool hci_tx_aggr)
{
    int i;
    struct ssv6xxx_hci_ctrl *hci_ctrl;

    if (shi == NULL/* || hci_ctrl->shi*/) {
        printk(KERN_ERR "NULL sh when register HCI.\n");
        return -1;
    }

    hci_ctrl = kzalloc(sizeof(*hci_ctrl), GFP_KERNEL);
    if (hci_ctrl == NULL)
        return -ENOMEM;

    memset((void *)hci_ctrl, 0, sizeof(*hci_ctrl));

    /* HCI & hw/sw mac interface binding */
    shi->hci_ctrl = hci_ctrl;
    shi->hci_ops = &hci_ops;
    hci_ctrl->shi = shi;

    hci_ctrl->task_timeout = 3; //default timeout 3ms
    hci_ctrl->hw_txq_mask = 0;
    mutex_init(&hci_ctrl->hw_txq_mask_lock);
    mutex_init(&hci_ctrl->hci_mutex);
    mutex_init(&hci_ctrl->cmd_mutex);
    init_completion(&hci_ctrl->cmd_done);

    /* TX queue initialization */
    for (i=0; i < SSV_SW_TXQ_NUM; i++) {
        memset(&hci_ctrl->sw_txq[i], 0, sizeof(struct ssv_sw_txq));
        skb_queue_head_init(&hci_ctrl->sw_txq[i].qhead);
        mutex_init(&hci_ctrl->sw_txq[i].txq_lock);
        hci_ctrl->sw_txq[i].txq_no = (u32)i;
        hci_ctrl->sw_txq[i].paused = true;
    }

    INIT_WORK(&hci_ctrl->isr_reset_work, ssv6xxx_hci_isr_reset);
    /*
     * This layer holds a tx task to send frames.
     */
    init_waitqueue_head(&hci_ctrl->tx_wait_q);
    hci_ctrl->hci_tx_task = kthread_run(ssv6xxx_hci_tx_task, hci_ctrl, "ssv6xxx_hci_tx_task");
    /*
     * Under layer holds ISR to receive frames.
     */
    hci_ctrl->int_mask = SSV6XXX_INT_RX;
    //ssv6xxx_hci_irq_enable(hci_ctrl);
    //HCI_IRQ_TRIGGER(hci_ctrl);
    ssv6xxx_hci_start_acc(hci_ctrl);
    ssv6xxx_hci_hcmd_start(hci_ctrl);
    hci_ctrl->ignore_cmd = false;

    /* pre-alloc tx aggr skb */
    if (true == hci_tx_aggr) {
        hci_ctrl->p_tx_aggr_skb = shi->skb_alloc((void *)(SSV_SC(hci_ctrl)), HCI_TX_AGGR_SKB_LEN);
        if (NULL == hci_ctrl->p_tx_aggr_skb) {
            printk("Cannot alloc tx aggr skb size %d\n", HCI_TX_AGGR_SKB_LEN);
        }
    }

    return 0;
}
EXPORT_SYMBOL(ssv6xxx_hci_register);


#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
int ssv6xxx_hci_init(void)
#else
static int __init ssv6xxx_hci_init(void)
#endif
{
    printk("%s() start\n", __FUNCTION__);
    return 0;
}

#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
void ssv6xxx_hci_exit(void)
#else
static void __exit ssv6xxx_hci_exit(void)
#endif
{
    printk("%s() start\n", __FUNCTION__);
}


#if (defined(CONFIG_SSV_SUPPORT_ANDROID)||defined(CONFIG_SSV_BUILD_AS_ONE_KO))
EXPORT_SYMBOL(ssv6xxx_hci_init);
EXPORT_SYMBOL(ssv6xxx_hci_exit);
#else
module_init(ssv6xxx_hci_init);
module_exit(ssv6xxx_hci_exit);
#endif
