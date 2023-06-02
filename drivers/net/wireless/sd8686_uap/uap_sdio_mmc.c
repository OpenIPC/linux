/** @file uap_sdio_mmc.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
 *
 * Copyright (C) 2007-2009, Marvell International Ltd.
 *
 * This software file (the "File") is distributed by Marvell International
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991
 * (the "License").  You may use, redistribute and/or modify this File in
 * accordance with the terms and conditions of the License, a copy of which
 * is available along with the File in the gpl.txt file or by writing to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
 * this warranty disclaimer.
 *
 */
/****************************************************
Change log:
****************************************************/

#include	"uap_sdio_mmc.h"

#include <linux/firmware.h>
#include <linux/mmc/host.h>

/** define SDIO block size */
/* We support up to 480-byte block size due to FW buffer limitation. */
#define SD_BLOCK_SIZE		256

/** define allocated buffer size */
#define ALLOC_BUF_SIZE		(((MAX(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE, \
					MRVDRV_SIZE_OF_CMD_BUFFER) + INTF_HEADER_LEN \
					+ SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE)

/** Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY	2

/********************************************************
		Local Variables
********************************************************/

/** SDIO Rx unit */
static u8 sdio_rx_unit = 0;

/**Interrupt status */
static u8 sd_ireg = 0;
/********************************************************
		Global Variables
********************************************************/
extern u8 *helper_name;
extern u8 *fw_name;
/** Default helper name */
#define DEFAULT_HELPER_NAME "mrvl/helper_sd.bin"
/** Default firmware name */
#define DEFAULT_FW_NAME "mrvl/sd8686_ap.bin"

/********************************************************
		Local Functions
********************************************************/
/**
 *  @brief This function reads the IO register.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param reg	   register to be read
 *  @param dat	   A pointer to variable that keeps returned value
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sbi_read_ioreg(uap_private * priv, u32 reg, u8 * dat)
{
    struct sdio_mmc_card *card;
    int ret = UAP_STATUS_FAILURE;

    card = priv->uap_dev.card;
    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_read_ioreg(): card or function is NULL!\n");
        goto done;
    }

    *dat = sdio_readb(card->func, reg, &ret);
    if (ret) {
        PRINTM(ERROR, "sbi_read_ioreg(): sdio_readb failed! ret=%d\n", ret);
        goto done;
    }

    PRINTM(INFO, "sbi_read_ioreg() priv=%p func=%d reg=%#x dat=%#x\n", priv,
           card->func->num, reg, *dat);

  done:
    return ret;
}

/**
 *  @brief This function writes the IO register.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param reg	   register to be written
 *  @param dat	   the value to be written
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sbi_write_ioreg(uap_private * priv, u32 reg, u8 dat)
{
    struct sdio_mmc_card *card;
    int ret = UAP_STATUS_FAILURE;

    ENTER();

    card = priv->uap_dev.card;
    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_write_ioreg(): card or function is NULL!\n");
        goto done;
    }

    PRINTM(INFO, "sbi_write_ioreg() priv=%p func=%d reg=%#x dat=%#x\n", priv,
           card->func->num, reg, dat);

    sdio_writeb(card->func, dat, reg, &ret);
    if (ret) {
        PRINTM(ERROR, "sbi_write_ioreg(): sdio_readb failed! ret=%d\n", ret);
        goto done;
    }

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function get rx_unit value
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sd_get_rx_unit(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u8 reg;

    ENTER();

    ret = sbi_read_ioreg(priv, CARD_RX_UNIT_REG, &reg);
    if (ret == UAP_STATUS_SUCCESS)
        {
        sdio_rx_unit = reg;
        }

    LEAVE();
    return ret;
}

static int
sd_read_scratch(uap_private * priv, u16 * dat)
{
    int ret;
    u8 scr0;
    u8 scr1;

    ENTER();

    ret =
        sbi_read_ioreg(priv, CARD_FW_STATUS0_REG, &scr0);
    if (ret < 0)
        return UAP_STATUS_FAILURE;

    ret =
        sbi_read_ioreg(priv, CARD_FW_STATUS1_REG, &scr1);
    PRINTM(INFO, "CARD_OCR_0_REG = 0x%x, CARD_OCR_1_REG = 0x%x\n", scr0, scr1);
    if (ret < 0)
        return UAP_STATUS_FAILURE;

    *dat = (((u16) scr1) << 8) | scr0;

    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function reads rx length
 *
 *  @param priv    A pointer to uap_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sd_read_rx_len(uap_private * priv, u16 * dat)
{
    int ret = UAP_STATUS_SUCCESS;
    u8 reg;

    ENTER();

    // For sd8686, you read scratch reg instead of len reg.
    ret = sbi_read_ioreg(priv, CARD_RX_LEN_REG, &reg);

    if (ret == UAP_STATUS_SUCCESS)
        *dat = (u16) reg << sdio_rx_unit;

    LEAVE();
    return ret;
}


/**
 *  @brief This function reads fw status registers
 *
 *  @param priv    A pointer to uap_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sd_read_firmware_status(uap_private * priv, u16 * dat)
{
    int ret = UAP_STATUS_SUCCESS;
    u16 x;

    ret = sd_read_scratch(priv, &x); 
    if (ret < 0) {
        LEAVE();
        return UAP_STATUS_FAILURE;
    }

    *dat = x;

    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function polls the card status register.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @param bits    	the bit mask
 *  @return 	   	UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
mv_sdio_poll_card_status(uap_private * priv, u8 bits)
{
    int tries;
    u8 cs;

    ENTER();

    for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
        if (sbi_read_ioreg(priv, CARD_STATUS_REG, &cs) < 0)
            break;
        else if ((cs & bits) == bits) {
            LEAVE();
            return UAP_STATUS_SUCCESS;
        }
        udelay(10);
    }

    PRINTM(WARN, "mv_sdio_poll_card_status failed, tries = %d\n", tries);

    LEAVE();
    return UAP_STATUS_FAILURE;
}

/**
 *  @brief This function set the sdio bus width.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @param mode    	1--1 bit mode, 4--4 bit mode
 *  @return 	   	UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
#if 0
static int
sdio_set_bus_width(uap_private * priv, u8 mode)
{
    ENTER();
    LEAVE();
    return UAP_STATUS_SUCCESS;
}
#endif

/**
 *  @brief This function reads data from the card.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @return 	   	UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
sd_card_to_host(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u16 buf_len = 0;
    u16 chunk = 0;
    int buf_block_len;
    int blksz;
    struct sk_buff *skb = NULL;
    u16 type;
    u8 *payload = NULL;
    struct sdio_mmc_card *card = priv->uap_dev.card;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        ret = UAP_STATUS_FAILURE;
        goto exit;
    }

    ret = sd_read_rx_len(priv, &buf_len);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read scratch reg failed\n");
        ret = UAP_STATUS_FAILURE;
        goto exit;
    }

    /* Allocate buffer */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (buf_len + blksz - 1) / blksz;
    if (buf_len <= INTF_HEADER_LEN || (buf_block_len * blksz) > ALLOC_BUF_SIZE) {
        PRINTM(ERROR, "card_to_host, invalid packet length: %d\n", buf_len);
        ret = UAP_STATUS_FAILURE;
        goto exit;
    }

    skb = dev_alloc_skb(buf_block_len * blksz);

    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        goto exit;
    }

    payload = skb->tail;
    ret = sdio_readsb(card->func, payload, priv->uap_dev.ioport,
                      buf_block_len * blksz);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read iomem failed: %d\n", ret);
        ret = UAP_STATUS_FAILURE;
        goto exit;
    }
    HEXDUMP("SDIO Blk Rd", payload, blksz * buf_block_len);
    /*
     * This is SDIO specific header
     *  u16 length,
     *  u16 type (MV_TYPE_DAT = 0, MV_TYPE_CMD = 1, MV_TYPE_EVENT = 3)
     */
    chunk = uap_le16_to_cpu(*(u16 *) & payload[0]);
    type = uap_le16_to_cpu(*(u16 *) & payload[2]);
    if (chunk > buf_len) {
	printk("Packet fragment\n");
        ret = -EINVAL;
	goto exit;
    }
    buf_len = chunk;
        
    switch (type) {
    case MV_TYPE_EVENT:
        skb_put(skb, buf_len);
        skb_pull(skb, INTF_HEADER_LEN);
        uap_process_event(priv, skb->data, skb->len);
        kfree_skb(skb);
        skb = NULL;
        break;
    case MV_TYPE_CMD:
        skb_put(skb, buf_len);
        skb_pull(skb, INTF_HEADER_LEN);
        priv->adapter->cmd_pending = FALSE;
        if (priv->adapter->cmd_wait_option ==
            HostCmd_OPTION_WAITFORRSP_SLEEPCONFIRM) {
            priv->adapter->cmd_wait_option = FALSE;
            uap_process_sleep_confirm_resp(priv, skb->data, skb->len);
        } else if (priv->adapter->cmd_wait_option) {
            memcpy(priv->adapter->CmdBuf, skb->data, skb->len);
            priv->adapter->CmdSize = skb->len;
            priv->adapter->cmd_wait_option = FALSE;
            priv->adapter->CmdWaitQWoken = TRUE;
            wake_up_interruptible(&priv->adapter->cmdwait_q);
        }
        kfree_skb(skb);
        skb = NULL;
        break;
    case MV_TYPE_DAT:
        skb_put(skb, buf_len);
        skb_pull(skb, INTF_HEADER_LEN);
        uap_process_rx_packet(priv, skb);
        break;
    default:
        priv->stats.rx_errors++;
        priv->stats.rx_dropped++;
        /* Driver specified event and command resp should be handle here */
        PRINTM(INFO, "Unknown PKT type:%d\n", type);
        kfree_skb(skb);
        skb = NULL;
        break;
    }
  exit:
    if (ret) {
        if (skb)
            kfree_skb(skb);
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function enables the host interrupts mask
 *
 *  @param priv    A pointer to uap_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   UAP_STATUS_SUCCESS
 */
static int
enable_host_int_mask(uap_private * priv, u8 mask)
{
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    /* Simply write the mask to the register */
    ret = sbi_write_ioreg(priv, HOST_INT_MASK_REG, mask);

    if (ret) {
        PRINTM(WARN, "Unable to enable the host interrupt!\n");
        ret = UAP_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**  @brief This function disables the host interrupts mask.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
disable_host_int_mask(uap_private * priv, u8 mask)
{
    int ret = UAP_STATUS_SUCCESS;
    u8 host_int_mask;

    ENTER();

    /* Read back the host_int_mask register */
    ret = sbi_read_ioreg(priv, HOST_INT_MASK_REG, &host_int_mask);
    if (ret) {
        ret = UAP_STATUS_FAILURE;
        goto done;
    }

    /* Update with the mask and write back to the register */
    host_int_mask &= ~mask;
    ret = sbi_write_ioreg(priv, HOST_INT_MASK_REG, host_int_mask);
    if (ret < 0) {
        PRINTM(WARN, "Unable to diable the host interrupt!\n");
        ret = UAP_STATUS_FAILURE;
        goto done;
    }

  done:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

/**
 *  @brief This function handles the interrupt.
 *
 *  @param func	   A pointer to sdio_func structure.
 *  @return 	   n/a
 */
static void
sbi_interrupt(struct sdio_func *func)
{
    struct sdio_mmc_card *card;
    uap_private *priv;
    u8 ireg = 0;
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    card = sdio_get_drvdata(func);
    if (!card || !card->priv) {
        PRINTM(MSG, "%s: sbi_interrupt(%p) card or priv is NULL, card=%p\n",
               __FUNCTION__, func, card);
        LEAVE();
        return;
    }
    priv = card->priv;
#ifdef FW_WAKEUP_TIME
    if ((priv->adapter->wt_pwrup_sending != 0L) &&
        (priv->adapter->wt_int == 0L))
        priv->adapter->wt_int = get_utimeofday();
#endif

    ireg = sdio_readb(card->func, HOST_INTSTATUS_REG, &ret);
    if (ret) {
        PRINTM(WARN, "sdio_read_ioreg: read int status register failed\n");
        goto done;
    }
    if (ireg != 0) {
        /*
         * DN_LD_HOST_INT_STATUS and/or UP_LD_HOST_INT_STATUS
         * Clear the interrupt status register and re-enable the interrupt
         */
        PRINTM(INFO, "sdio_ireg = 0x%x\n", ireg);
        sdio_writeb(card->func,
                    ~(ireg) & (DN_LD_HOST_INT_STATUS | UP_LD_HOST_INT_STATUS),
                    HOST_INTSTATUS_REG, &ret);
        if (ret) {
            PRINTM(WARN,
                   "sdio_write_ioreg: clear int status register failed\n");
            goto done;
        }
    }
    OS_INT_DISABLE;
    sd_ireg |= ireg;
    OS_INT_RESTORE;

    uap_interrupt(priv);
  done:
    LEAVE();
}

/**
 *  @brief This function probe the card
 *
 *  @param func    A pointer to sdio_func structure
 *  @param id	   A pointer to structure sd_device_id
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    int ret = UAP_STATUS_FAILURE;
    struct sdio_mmc_card *card = NULL;

    ENTER();

    PRINTM(MSG, "%s: vendor=0x%4.04X device=0x%4.04X class=%d function=%d\n",
           __FUNCTION__, func->vendor, func->device, func->class, func->num);

    card = kzalloc(sizeof(struct sdio_mmc_card), GFP_KERNEL);
    if (!card) {
        ret = -ENOMEM;
        goto done;
    }

    card->func = func;

    if (!uap_add_card(card)) {
        PRINTM(ERROR, "%s: uap_add_callback failed\n", __FUNCTION__);
        kfree(card);
        ret = UAP_STATUS_FAILURE;
        goto done;
    }

    ret = UAP_STATUS_SUCCESS;

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function removes the card
 *
 *  @param func    A pointer to sdio_func structure
 *  @return        N/A
 */
static void
uap_remove(struct sdio_func *func)
{
    struct sdio_mmc_card *card;

    ENTER();

    if (func) {
        card = sdio_get_drvdata(func);
        if (card) {
            uap_remove_card(card);
            kfree(card);
        }
    }

    LEAVE();
}

#ifdef CONFIG_PM
/**
 *  @brief This function handles client driver suspend
 *
 *  @param func    A pointer to sdio_func structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
uap_suspend(struct sdio_func *func)
{
    ENTER();
    LEAVE();
    return 0;
}

/**
 *  @brief This function handles client driver resume
 *
 *  @param func    A pointer to sdio_func structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
uap_resume(struct sdio_func *func)
{
    ENTER();
    LEAVE();
    return 0;
}
#endif

/** Device ID for SD8688 */
#define  SD_DEVICE_ID_8688_UAP 0x9103
/** UAP IDs */
static const struct sdio_device_id uap_ids[] = {
    {SDIO_DEVICE(SDIO_VENDOR_ID_MARVELL, SD_DEVICE_ID_8688_UAP)},
    {},
};

MODULE_DEVICE_TABLE(sdio, uap_ids);

static struct sdio_driver uap_sdio = {
    .name = "uap_sdio",
    .id_table = uap_ids,
    .probe = uap_probe,
    .remove = uap_remove,
#ifdef CONFIG_PM
/*    .suspend	= uap_suspend, */
/*    .resume	= uap_resume, */
#endif

};

/**
 *  @brief This function registers the IF module in bus driver.
 *
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int __init
sbi_register()
{
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    /* SDIO Driver Registration */
    if (sdio_register_driver(&uap_sdio) != 0) {
        PRINTM(FATAL, "SDIO Driver Registration Failed \n");
        ret = UAP_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function de-registers the IF module in bus driver.
 *
 *  @return 	   n/a
 */
void __exit
sbi_unregister(void)
{
    ENTER();

    /* SDIO Driver Unregistration */
    sdio_unregister_driver(&uap_sdio);

    LEAVE();
}

/**
 *  @brief This function checks the interrupt status and handle it accordingly.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param ireg    A pointer to variable that keeps returned value
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_get_int_status(uap_private * priv, u8 * ireg)
{
    int ret = UAP_STATUS_SUCCESS;
    u8 sdio_ireg = 0;
    struct sdio_mmc_card *card = priv->uap_dev.card;

    ENTER();

    *ireg = 0;
    OS_INT_DISABLE;
    sdio_ireg = sd_ireg;
    sd_ireg = 0;
    OS_INT_RESTORE;

    sdio_claim_host(card->func);

    if (sdio_ireg & DN_LD_HOST_INT_STATUS) {    /* tx_done INT */
        if (!priv->uap_dev.cmd_sent) {  /* tx_done already received */
            PRINTM(INFO,
                   "warning: tx_done already received: tx_dnld_rdy=0x%x int status=0x%x\n",
                   priv->uap_dev.cmd_sent, sdio_ireg);
        } else {
            priv->uap_dev.cmd_sent = FALSE;
            priv->uap_dev.data_sent = FALSE;
            if ( (priv->uap_dev.netdev->reg_state == NETREG_REGISTERED) && (skb_queue_len(&priv->adapter->tx_queue) < TX_LOW_WATERMARK)) {
                os_start_queue(priv);
	    }
        }
    }
    if (sdio_ireg & UP_LD_HOST_INT_STATUS) {
        sd_card_to_host(priv);
    }

    *ireg = sdio_ireg;
    ret = UAP_STATUS_SUCCESS;
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/**
 *  @brief This function disables the host interrupts.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_disable_host_int(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    int ret;

    ENTER();

    sdio_claim_host(card->func);
    ret = disable_host_int_mask(priv, HIM_DISABLE);
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

int sbi_claim_irq(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    struct sdio_func *func;
    int ret;

    ENTER();
    func = card->func;

    sdio_claim_host(func);

    ret = sdio_claim_irq(func, sbi_interrupt);
    if (ret) {
        PRINTM(FATAL, "sdio_claim_irq failed: ret=%d\n", ret);
    }

    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/**
 *  @brief This function enables the host interrupts.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS
 */
int
sbi_enable_host_int(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    int ret;

    ENTER();

    sdio_claim_host(card->func);
    ret = enable_host_int_mask(priv, HIM_ENABLE);
    sdio_release_host(card->func);

    if(card->func->card->host->caps & MMC_CAP_SDIO_IRQ)
           card->func->card->host->ops->enable_sdio_irq
               (card->func->card->host, 1);

    LEAVE();
    return ret;
}

/**
 *  @brief This function de-registers the device.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS
 */
int
sbi_unregister_dev(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "Error: card or function is NULL!\n");
        goto done;
    }

    sdio_claim_host(card->func);
    sdio_release_irq(card->func);
    sdio_disable_func(card->func);
    sdio_release_host(card->func);

    sdio_set_drvdata(card->func, NULL);

  done:
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function registers the device.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_register_dev(uap_private * priv)
{
    int ret = UAP_STATUS_FAILURE;
    u8 reg;
    struct sdio_mmc_card *card = priv->uap_dev.card;
    struct sdio_func *func;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "Error: card or function is NULL!\n");
        goto done;
    }

    func = card->func;

    /* Initialize the private structure */
    priv->uap_dev.ioport = 0;

    sdio_claim_host(func);

    ret = sdio_enable_func(func);
    if (ret) {
        PRINTM(FATAL, "sdio_enable_func() failed: ret=%d\n", ret);
        goto release_host;
    }

    /* Read the IO port */
    ret = sbi_read_ioreg(priv, IO_PORT_0_REG, &reg);
    if (ret)
        goto disable_func;
    else
        priv->uap_dev.ioport |= reg;

    ret = sbi_read_ioreg(priv, IO_PORT_1_REG, &reg);
    if (ret)
        goto disable_func;
    else
        priv->uap_dev.ioport |= (reg << 8);

    ret = sbi_read_ioreg(priv, IO_PORT_2_REG, &reg);
    if (ret)
        goto disable_func;
    else
        priv->uap_dev.ioport |= (reg << 16);

    PRINTM(INFO, "SDIO FUNC #%d IO port: 0x%x\n", func->num,
           priv->uap_dev.ioport);

    ret = sdio_set_block_size(card->func, SD_BLOCK_SIZE);
    if (ret) {
        PRINTM(ERROR, "%s: cannot set SDIO block size\n", __FUNCTION__);
        ret = UAP_STATUS_FAILURE;
        goto disable_func;
    }
    priv->hotplug_device = &func->dev;

    if (helper_name == NULL) {
        helper_name = DEFAULT_HELPER_NAME;
    }
    if (fw_name == NULL) {
        fw_name = DEFAULT_FW_NAME;
    }
    sdio_release_host(func);

    sdio_set_drvdata(func, card);

    ret = UAP_STATUS_SUCCESS;
    goto done;

  disable_func:
    sdio_disable_func(func);
  release_host:
    sdio_release_host(func);

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function sends data to the card.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param payload A pointer to the data/cmd buffer
 *  @param nb	   the length of data/cmd
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_host_to_card(uap_private * priv, u8 * payload, u16 nb)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    int ret = UAP_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    int i = 0;
    u8 *buf = NULL;
#ifdef PXA3XX_DMA_ALIGN
    void *tmpbuf = NULL;
    int tmpbufsz;
#endif

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }
    buf = payload;
#ifdef PXA3XX_DMA_ALIGN
    if ((u32) payload & (PXA3XX_DMA_ALIGNMENT - 1)) {
        tmpbufsz = ALIGN_SZ(nb, PXA3XX_DMA_ALIGNMENT);
        tmpbuf = kmalloc(tmpbufsz, GFP_KERNEL);
        memset(tmpbuf, 0, tmpbufsz);
        /* Ensure 8-byte aligned CMD buffer */
        buf = (u8 *) ALIGN_ADDR(tmpbuf, PXA3XX_DMA_ALIGNMENT);
        memcpy(buf, payload, nb);
    }
#endif
    /* Allocate buffer and copy payload */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (nb + blksz - 1) / blksz;
    sdio_claim_host(card->func);
#define MAX_WRITE_IOMEM_RETRY	2
    priv->uap_dev.cmd_sent = TRUE;
    priv->uap_dev.data_sent = TRUE;
    do {
        /* Transfer data to card */
        ret = sdio_writesb(card->func, priv->uap_dev.ioport, buf,
                           buf_block_len * blksz);
        if (ret < 0) {
            i++;
            PRINTM(ERROR, "host_to_card, write iomem (%d) failed: %d\n", i,
                   ret);
            ret = UAP_STATUS_FAILURE;
            if (i > MAX_WRITE_IOMEM_RETRY)
                goto exit;
        } else {
            HEXDUMP("SDIO Blk Wr", payload, nb);
        }
    } while (ret == UAP_STATUS_FAILURE);
  exit:
    sdio_release_host(card->func);
#ifdef PXA3XX_DMA_ALIGN
    if (tmpbuf)
        kfree(tmpbuf);
#endif
    if (ret == UAP_STATUS_FAILURE) {
        priv->uap_dev.cmd_sent = FALSE;
        priv->uap_dev.data_sent = FALSE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function reads CIS information.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param cisinfo A pointer to CIS information output buffer
 *  @param cislen  A pointer to length of CIS info output buffer
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
#if 0
static int
sbi_get_cis_info(uap_private * priv, void *cisinfo, int *cislen)
{
#define CIS_PTR (0x8000)
    struct sdio_mmc_card *card = priv->uap_dev.card;
    unsigned int i, cis_ptr = CIS_PTR;
    int ret = UAP_STATUS_FAILURE;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_get_cis_info(): card or function is NULL!\n");
        goto exit;
    }
#define MAX_SDIO_CIS_INFO_LEN (256)
    if (!cisinfo || (*cislen < MAX_SDIO_CIS_INFO_LEN)) {
        PRINTM(WARN, "ERROR! get_cis_info: insufficient buffer passed\n");
        goto exit;
    }

    *cislen = MAX_SDIO_CIS_INFO_LEN;

    sdio_claim_host(card->func);

    PRINTM(INFO, "cis_ptr=%#x\n", cis_ptr);

    /* Read the Tuple Data */
    for (i = 0; i < *cislen; i++) {
        ((unsigned char *) cisinfo)[i] =
            sdio_readb(card->func, cis_ptr + i, &ret);
        if (ret) {
            PRINTM(WARN, "get_cis_info error: ret=%d\n", ret);
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        PRINTM(INFO, "cisinfo[%d]=%#x\n", i, ((unsigned char *) cisinfo)[i]);
    }

  done:
    sdio_release_host(card->func);
  exit:
    LEAVE();
    return ret;
}
#endif
/**
 *  @brief This function downloads helper image to the card.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @return 	   	UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_prog_helper(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    u8 *helper = NULL;
    int helperlen;
    int ret = UAP_STATUS_SUCCESS;
    void *tmphlprbuf = NULL;
    int tmphlprbufsz;
    u8 *hlprbuf;
    int hlprblknow;
    u32 tx_len;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_prog_helper(): card or function is NULL!\n");
        goto done;
    }

    if (priv->fw_helper) {
        helper = (u8 *) priv->fw_helper->data;
        helperlen = priv->fw_helper->size;
    } else {
        PRINTM(MSG, "No helper image found! Terminating download.\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }

    PRINTM(INFO, "Downloading helper image (%d bytes), block size %d bytes\n",
           helperlen, SD_BLOCK_SIZE);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif

#ifdef PXA3XX_DMA_ALIGN
    tmphlprbufsz = ALIGN_SZ(UAP_UPLD_SIZE, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
    tmphlprbufsz = UAP_UPLD_SIZE;
#endif /* !PXA3XX_DMA_ALIGN */
    tmphlprbuf = kmalloc(tmphlprbufsz, GFP_KERNEL);
    if (!tmphlprbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for helper. Terminating download\n");
        ret = UAP_STATUS_FAILURE;
        goto done;
    }
    memset(tmphlprbuf, 0, tmphlprbufsz);
#ifdef PXA3XX_DMA_ALIGN
    hlprbuf = (u8 *) ALIGN_ADDR(tmphlprbuf, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
    hlprbuf = (u8 *) tmphlprbuf;
#endif /* !PXA3XX_DMA_ALIGN */

    sdio_claim_host(card->func);

    /* Perform helper data transfer */
    tx_len = (FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE) - INTF_HEADER_LEN;
    hlprblknow = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "Helper download poll status timeout @ %d\n",
                   hlprblknow);
            goto done;
        }

        /* More data? */
        if (hlprblknow >= helperlen)
            break;

        /* Set blocksize to transfer - checking for last block */
        if (helperlen - hlprblknow < tx_len)
            tx_len = helperlen - hlprblknow;

        /* Set length to the 4-byte header */
        *(u32 *) hlprbuf = uap_cpu_to_le32(tx_len);

        /* Copy payload to buffer */
        memcpy(&hlprbuf[INTF_HEADER_LEN], &helper[hlprblknow], tx_len);

        PRINTM(INFO, ".");

        /* Send data */
        ret = sdio_writesb(card->func, priv->uap_dev.ioport,
                           hlprbuf, FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE);

        if (ret < 0) {
            PRINTM(FATAL, "IO error during helper download @ %d\n", hlprblknow);
            goto done;
        }

        hlprblknow += tx_len;
    } while (TRUE);

#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "helper: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000, (tv2 % 1000000) / 1000,
           tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000, (tv2 % 1000000) / 1000,
           tv2 % 1000);
#endif

    /* Write last EOF data */
    PRINTM(INFO, "\nTransferring helper image EOF block\n");
    memset(hlprbuf, 0x0, SD_BLOCK_SIZE);
    ret = sdio_writesb(card->func, priv->uap_dev.ioport,
                       hlprbuf, SD_BLOCK_SIZE);

    if (ret < 0) {
        PRINTM(FATAL, "IO error in writing helper image EOF block\n");
        goto done;
    }

    ret = UAP_STATUS_SUCCESS;

  done:
    sdio_release_host(card->func);
    if (tmphlprbuf)
        kfree(tmphlprbuf);

    LEAVE();
    return ret;
}

/**
 *  @brief This function downloads firmware image to the card.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @return 	   	UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_prog_fw_w_helper(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    u8 *firmware = NULL;
    int firmwarelen;
    u8 base0;
    u8 base1;
    int ret = UAP_STATUS_SUCCESS;
    int offset;
    void *tmpfwbuf = NULL;
    int tmpfwbufsz;
    u8 *fwbuf;
    u16 len;
    int txlen = 0;
    int tx_blocks = 0;
    int i = 0;
    int tries = 0;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_prog_fw_w_helper(): card or function is NULL!\n");
        goto done;
    }

    if (priv->firmware) {
        firmware = (u8 *) priv->firmware->data;
        firmwarelen = priv->firmware->size;
    } else {
        PRINTM(MSG, "No firmware image found! Terminating download.\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }

    PRINTM(INFO, "Downloading FW image (%d bytes)\n", firmwarelen);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif

#ifdef PXA3XX_DMA_ALIGN
    tmpfwbufsz = ALIGN_SZ(UAP_UPLD_SIZE, PXA3XX_DMA_ALIGNMENT);
#else /* PXA3XX_DMA_ALIGN */
    tmpfwbufsz = UAP_UPLD_SIZE;
#endif /* PXA3XX_DMA_ALIGN */
    tmpfwbuf = kmalloc(tmpfwbufsz, GFP_KERNEL);
    if (!tmpfwbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for firmware. Terminating download.\n");
        ret = UAP_STATUS_FAILURE;
        goto done;
    }
    memset(tmpfwbuf, 0, tmpfwbufsz);
#ifdef PXA3XX_DMA_ALIGN
    /* Ensure 8-byte aligned firmware buffer */
    fwbuf = (u8 *) ALIGN_ADDR(tmpfwbuf, PXA3XX_DMA_ALIGNMENT);
#else /* PXA3XX_DMA_ALIGN */
    fwbuf = (u8 *) tmpfwbuf;
#endif /* PXA3XX_DMA_ALIGN */

    sdio_claim_host(card->func);

    /* Perform firmware data transfer */
    offset = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download with helper poll status timeout @ %d\n",
                   offset);
            goto done;
        }

        /* More data? */
        if (offset >= firmwarelen)
            break;

        for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
            if ((ret = sbi_read_ioreg(priv, HOST_F1_RD_BASE_0, &base0)) < 0) {
                PRINTM(WARN, "Dev BASE0 register read failed:"
                       " base0=0x%04X(%d). Terminating download.\n", base0,
                       base0);
                ret = UAP_STATUS_FAILURE;
                goto done;
            }
            if ((ret = sbi_read_ioreg(priv, HOST_F1_RD_BASE_1, &base1)) < 0) {
                PRINTM(WARN, "Dev BASE1 register read failed:"
                       " base1=0x%04X(%d). Terminating download.\n", base1,
                       base1);
                ret = UAP_STATUS_FAILURE;
                goto done;
            }
            len = (((u16) base1) << 8) | base0;

            /* For SD8688 wait until the length is not 0, 1 or 2 before
               downloading the first FW block, since BOOT code writes the
               register to indicate the helper/FW download winner, the value
               could be 1 or 2 (Func1 or Func2). */
            if ((len && offset) || (len > 2))
                break;
            udelay(10);
        }

        if (len == 0)
            break;
        else if (len > UAP_UPLD_SIZE) {
            PRINTM(FATAL, "FW download failure @ %d, invalid length %d\n",
                   offset, len);
            ret = UAP_STATUS_FAILURE;
            goto done;
        }

        txlen = len;

        if (len & BIT(0)) {
            i++;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                PRINTM(FATAL,
                       "FW download failure @ %d, over max retry count\n",
                       offset);
                ret = UAP_STATUS_FAILURE;
                goto done;
            }
            PRINTM(ERROR, "FW CRC error indicated by the helper:"
                   " len = 0x%04X, txlen = %d\n", len, txlen);
            len &= ~BIT(0);
            /* Setting this to 0 to resend from same offset */
            txlen = 0;
        } else {
            i = 0;

            /* Set blocksize to transfer - checking for last block */
            if (firmwarelen - offset < txlen) {
                txlen = firmwarelen - offset;
            }
            PRINTM(INFO, ".");

            tx_blocks = (txlen + SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE;

            /* Copy payload to buffer */
            memcpy(fwbuf, &firmware[offset], txlen);
        }

        /* Send data */
        ret = sdio_writesb(card->func, priv->uap_dev.ioport,
                           fwbuf, tx_blocks * SD_BLOCK_SIZE);

        if (ret < 0) {
            PRINTM(ERROR, "FW download, write iomem (%d) failed @ %d\n", i,
                   offset);
            if (sbi_write_ioreg(priv, CONFIGURATION_REG, 0x04) < 0) {
                PRINTM(ERROR, "write ioreg failed (CFG)\n");
            }
        }

        offset += txlen;
    } while (TRUE);

    PRINTM(INFO, "\nFW download over, size %d bytes\n", offset);

    ret = UAP_STATUS_SUCCESS;
  done:
#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "FW: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
#endif
    sdio_release_host(card->func);
    if (tmpfwbuf)
        kfree(tmpfwbuf);

    LEAVE();
    return ret;
}

/**
 *  @brief This function checks if the firmware is ready to accept
 *  command or not.
 *
 *  @param priv    A pointer to uap_private structure
 *  @param pollnum Poll number
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_check_fw_status(uap_private * priv, int pollnum)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    int ret = UAP_STATUS_SUCCESS;
    u16 firmwarestat;
    int tries;

    ENTER();

    sdio_claim_host(card->func);

    /* Wait for firmware initialization event */
    for (tries = 0; tries < pollnum; tries++) {
        if ((ret = sd_read_firmware_status(priv, &firmwarestat)) < 0)
            continue;

        if (firmwarestat == FIRMWARE_READY) {
            ret = UAP_STATUS_SUCCESS;
            break;
        } else {
            mdelay(10);
            ret = UAP_STATUS_FAILURE;
        }
    }

    if (ret < 0)
        goto done;

    ret = UAP_STATUS_SUCCESS;

    sd_get_rx_unit(priv);

  done:
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/**
 *  @brief This function set bus clock on/off
 *
 *  @param priv    A pointer to uap_private structure
 *  @param option    TRUE--on , FALSE--off
 *  @return 	   UAP_STATUS_SUCCESS
 */
#if 0
static int
sbi_set_bus_clock(uap_private * priv, u8 option)
{
    ENTER();
    LEAVE();
    return UAP_STATUS_SUCCESS;
}
#endif

/**
 *  @brief This function wakeup firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
sbi_wakeup_firmware(uap_private * priv)
{
    struct sdio_mmc_card *card = priv->uap_dev.card;
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    sdio_writeb(card->func, HOST_POWER_UP, CONFIGURATION_REG, &ret);
    sdio_release_host(card->func);
    LEAVE();
    return ret;
}
