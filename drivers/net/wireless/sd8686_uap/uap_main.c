/** @file uap_main.c
  * @brief This file contains the major functions in uAP
  * driver. It includes init, exit etc..
  * This file also contains the initialization for SW,
  * FW and HW
  *
  * Copyright (C) 2008-2009, Marvell International Ltd.
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
/**
  * @mainpage uAP Linux Driver
  *
  * @section overview_sec Overview
  *
  * This is Linux reference driver for Marvell uAP.
  *
  * @section copyright_sec Copyright
  *
  * Copyright (C) 2008, Marvell International Ltd.
  *
  */

#include	"uap_headers.h"

/**
 * the global variable of a pointer to uap_private
 * structure variable
 */
uap_private *uappriv = NULL;
#ifdef DEBUG_LEVEL1
#define DEFAULT_DEBUG_MASK	(DBG_MSG | DBG_FATAL | DBG_ERROR)
u32 drvdbg = DEFAULT_DEBUG_MASK;
#endif
/** Helper name */
char *helper_name = NULL;
/** Firmware name */
char *fw_name = NULL;

/** Semaphore for add/remove card */
SEMAPHORE AddRemoveCardSem;

/********************************************************
		Local Functions
********************************************************/
/**
 *  @brief This function send sleep confirm command to firmware
 *
 *  @param priv    	A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS for success otherwise UAP_STATUS_FAILURE
 */
static int
uap_dnld_sleep_confirm_cmd(uap_private * priv)
{
    uap_adapter *Adapter = priv->adapter;
    int ret = UAP_STATUS_SUCCESS;
    ENTER();
    PRINTM(CMND, "Sleep confirm\n");
    Adapter->cmd_pending = TRUE;
    Adapter->cmd_wait_option = HostCmd_OPTION_WAITFORRSP_SLEEPCONFIRM;
    ret =
        sbi_host_to_card(priv, (u8 *) & Adapter->PSConfirmSleep,
                         sizeof(PS_CMD_ConfirmSleep));
    if (ret != UAP_STATUS_SUCCESS) {
        Adapter->ps_state = PS_STATE_AWAKE;
        Adapter->cmd_pending = FALSE;
        Adapter->cmd_wait_option = FALSE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief This function process sleep confirm resp from firmware
 *
 *  @param priv    	A pointer to uap_private structure
 *  @param resp 	A pointer to resp buf
 *  @param resp_len 	resp buf len
 *  @return 	   UAP_STATUS_SUCCESS for success otherwise UAP_STATUS_FAILURE
 */
int
uap_process_sleep_confirm_resp(uap_private * priv, u8 * resp, int resp_len)
{
    int ret = UAP_STATUS_SUCCESS;
    HostCmd_DS_COMMAND *cmd;
    uap_adapter *Adapter = priv->adapter;
    ENTER();
    PRINTM(CMND, "Sleep confirm resp\n");
    if (!resp_len) {
        PRINTM(ERROR, "Cmd Size is 0\n");
        ret = -EFAULT;
        goto done;
    }
    cmd = (HostCmd_DS_COMMAND *) resp;
    cmd->Result = uap_le16_to_cpu(cmd->Result);
    if (cmd->Result != UAP_STATUS_SUCCESS) {
        PRINTM(ERROR, "HOST_CMD_APCMD_PS_SLEEP_CONFIRM fail=%x\n", cmd->Result);
        ret = -EFAULT;
    }
  done:
    if (ret == UAP_STATUS_SUCCESS)
        Adapter->ps_state = PS_STATE_SLEEP;
    else
        Adapter->ps_state = PS_STATE_AWAKE;
    LEAVE();
    return ret;
}

/**
 *  @brief This function checks condition and prepares to
 *  send sleep confirm command to firmware if OK.
 *
 *  @param priv    	A pointer to uap_private structure
 *  @return 	   	n/a
 */
static void
uap_ps_cond_check(uap_private * priv)
{
    uap_adapter *Adapter = priv->adapter;

    ENTER();
    if (!priv->uap_dev.cmd_sent &&
        !Adapter->cmd_pending && !Adapter->IntCounter) {
        uap_dnld_sleep_confirm_cmd(priv);
    } else {
        PRINTM(INFO, "Delay Sleep Confirm (%s%s%s)\n",
               (priv->uap_dev.cmd_sent) ? "D" : "",
               (Adapter->cmd_pending) ? "C" : "",
               (Adapter->IntCounter) ? "I" : "");
    }
    LEAVE();
}

/**
 *  @brief This function add cmd to cmdQ and waiting for response
 *
 *  @param priv    A pointer to uap_private structure
 *  @param skb 	   A pointer to the skb for process
 *  @param wait_option Wait option
 *  @return 	   UAP_STATUS_SUCCESS for success otherwise UAP_STATUS_FAILURE
 */
static int
uap_process_cmd(uap_private * priv, struct sk_buff *skb, u8 wait_option)
{
    uap_adapter *Adapter = priv->adapter;
    int ret = UAP_STATUS_SUCCESS;
    HostCmd_DS_COMMAND *cmd;
    u8 *headptr;
    ENTER();

    if (Adapter->HardwareStatus != HWReady) {
        PRINTM(ERROR, "Hw not ready, uap_process_cmd\n");
        kfree(skb);
        LEAVE();
        return -EFAULT;
    }
    skb->cb[0] = wait_option;
    headptr = skb->data;
    *(u16 *) & headptr[0] = uap_cpu_to_le16(skb->len);
    *(u16 *) & headptr[2] = uap_cpu_to_le16(MV_TYPE_CMD);
    cmd = (HostCmd_DS_COMMAND *) (skb->data + INTF_HEADER_LEN);
    Adapter->SeqNum++;
    cmd->SeqNum = uap_cpu_to_le16(Adapter->SeqNum);
    DBG_HEXDUMP(CMD_D, "process_cmd", (u8 *) cmd, cmd->Size);
    if (!wait_option) {
        skb_queue_tail(&priv->adapter->cmd_queue, skb);
        wake_up_interruptible(&priv->MainThread.waitQ);
        LEAVE();
        return ret;
    }
    if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->CmdSem)) {
        PRINTM(ERROR, "Acquire semaphore error, uap_prepare_cmd\n");
        kfree(skb);
        LEAVE();
        return -EBUSY;
    }
    skb_queue_tail(&priv->adapter->cmd_queue, skb);
    Adapter->CmdWaitQWoken = FALSE;
    wake_up_interruptible(&priv->MainThread.waitQ);
    /* Sleep until response is generated by FW */
    if (wait_option == HostCmd_OPTION_WAITFORRSP_TIMEOUT) {
        if (!os_wait_interruptible_timeout
            (Adapter->cmdwait_q, Adapter->CmdWaitQWoken, MRVDRV_TIMER_20S)) {
            PRINTM(ERROR, "Cmd timeout\n");
            Adapter->cmd_pending = FALSE;
            ret = -EFAULT;
        }
    } else
        wait_event_interruptible(Adapter->cmdwait_q, Adapter->CmdWaitQWoken);
    OS_REL_SEMAPHORE(&Adapter->CmdSem);
    LEAVE();
    return ret;
}

/**
 *  @brief Inspect the response buffer for pointers to expected TLVs
 *
 *
 *  @param pTlv        Pointer to the start of the TLV buffer to parse
 *  @param tlvBufSize  Size of the TLV buffer
 *  @param reqTlvType  request tlv's tlvtype
 *  @param ppTlv       Output parameter: Pointer to the request TLV if found
 *
 *  @return            void
 */
static void
uap_get_tlv_ptrs(MrvlIEtypes_Data_t * pTlv, int tlvBufSize,
                 u16 reqTlvType, MrvlIEtypes_Data_t ** ppTlv)
{
    MrvlIEtypes_Data_t *pCurrentTlv;
    int tlvBufLeft;
    u16 tlvType;
    u16 tlvLen;

    ENTER();
    pCurrentTlv = pTlv;
    tlvBufLeft = tlvBufSize;
    *ppTlv = NULL;
    PRINTM(INFO, "uap_get_tlv: tlvBufSize = %d, reqTlvType=%x\n", tlvBufSize,
           reqTlvType);
    while (tlvBufLeft >= sizeof(MrvlIEtypesHeader_t)) {
        tlvType = uap_le16_to_cpu(pCurrentTlv->Header.Type);
        tlvLen = uap_le16_to_cpu(pCurrentTlv->Header.Len);
        if (reqTlvType == tlvType)
            *ppTlv = (MrvlIEtypes_Data_t *) pCurrentTlv;
        if (*ppTlv) {
            HEXDUMP("TLV Buf", (u8 *) * ppTlv, tlvLen);
            break;
        }
        tlvBufLeft -= (sizeof(pTlv->Header) + tlvLen);
        pCurrentTlv = (MrvlIEtypes_Data_t *) (pCurrentTlv->Data + tlvLen);
    }                           /* while */
    LEAVE();
}

/**
 *  @brief This function get mac
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS on success, otherwise failure code
 */
static int
uap_get_mac_address(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u32 CmdSize;
    HostCmd_DS_COMMAND *cmd;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb;
    MrvlIEtypes_MacAddr_t *pMacAddrTlv;
    MrvlIEtypes_Data_t *pTlv;
    u16 tlvBufSize;
    ENTER();
    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        ret = -ENOMEM;
        goto done;
    }
    CmdSize =
        S_DS_GEN + sizeof(HostCmd_SYS_CONFIG) + sizeof(MrvlIEtypes_MacAddr_t);
    cmd = (HostCmd_DS_COMMAND *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HOST_CMD_APCMD_SYS_CONFIGURE);
    cmd->Size = uap_cpu_to_le16(CmdSize);
    cmd->params.sys_config.Action = uap_cpu_to_le16(ACTION_GET);
    pMacAddrTlv =
        (MrvlIEtypes_MacAddr_t *) (skb->data + INTF_HEADER_LEN + S_DS_GEN +
                                   sizeof(HostCmd_SYS_CONFIG));
    pMacAddrTlv->Header.Type = uap_cpu_to_le16(MRVL_AP_MAC_ADDRESS_TLV_ID);
    pMacAddrTlv->Header.Len = uap_cpu_to_le16(ETH_ALEN);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP_TIMEOUT)) {
        PRINTM(ERROR, "Fail to process cmd SYS_CONFIGURE Query\n");
        ret = -EFAULT;
        goto done;
    }
    if (!Adapter->CmdSize) {
        PRINTM(ERROR, "Cmd Size is 0\n");
        ret = -EFAULT;
        goto done;
    }
    cmd = (HostCmd_DS_COMMAND *) Adapter->CmdBuf;
    cmd->Result = uap_le16_to_cpu(cmd->Result);
    if (cmd->Result != UAP_STATUS_SUCCESS) {
        PRINTM(ERROR, "uap_get_mac_address fail=%x\n", cmd->Result);
        ret = -EFAULT;
        goto done;
    }
    pTlv =
        (MrvlIEtypes_Data_t *) (Adapter->CmdBuf + S_DS_GEN +
                                sizeof(HostCmd_SYS_CONFIG));
    tlvBufSize = Adapter->CmdSize - S_DS_GEN - sizeof(HostCmd_SYS_CONFIG);
    uap_get_tlv_ptrs(pTlv, tlvBufSize, MRVL_AP_MAC_ADDRESS_TLV_ID,
                     (MrvlIEtypes_Data_t **) & pMacAddrTlv);
    if (pMacAddrTlv) {
        memcpy(priv->uap_dev.netdev->dev_addr, pMacAddrTlv->ApMacAddr,
               ETH_ALEN);
        HEXDUMP("Original MAC addr", priv->uap_dev.netdev->dev_addr, ETH_ALEN);
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function checks the conditions and sends packet to device
 *
 *  @param priv    A pointer to uap_private structure
 *  @param skb 	   A pointer to the skb for process
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_process_tx(uap_private * priv, struct sk_buff *skb)
{
    uap_adapter *Adapter = priv->adapter;
    int ret = UAP_STATUS_SUCCESS;
    TxPD *pLocalTxPD;
    u8 *headptr;
    struct sk_buff *newskb;
    int newheadlen;
    ENTER();
    ASSERT(skb);
    if (!skb) {
        LEAVE();
        return UAP_STATUS_FAILURE;
    }
    if (skb_headroom(skb) < (sizeof(TxPD) + INTF_HEADER_LEN + HEADER_ALIGNMENT)) {
        newheadlen = sizeof(TxPD) + INTF_HEADER_LEN + HEADER_ALIGNMENT;
        PRINTM(WARN, "Tx: Insufficient skb headroom %d\n", skb_headroom(skb));
        /* Insufficient skb headroom - allocate a new skb */
        newskb = skb_realloc_headroom(skb, newheadlen);
        if (unlikely(newskb == NULL)) {
            PRINTM(ERROR, "Tx: Cannot allocate skb\n");
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        kfree_skb(skb);
        skb = newskb;
        PRINTM(INFO, "new skb headroom %d\n", skb_headroom(skb));
    }
    /* headptr should be aligned */
    headptr = skb->data - sizeof(TxPD) - INTF_HEADER_LEN;
    headptr = (u8 *) ((u32) headptr & ~((u32) (HEADER_ALIGNMENT - 1)));

    pLocalTxPD = (TxPD *) (headptr + INTF_HEADER_LEN);
    memset(pLocalTxPD, 0, sizeof(TxPD));
    pLocalTxPD->BssType = PKT_TYPE_MICROAP;
    pLocalTxPD->TxPktLength = skb->len;
    /* offset of actual data */
    pLocalTxPD->TxPktOffset = (long) skb->data - (long) pLocalTxPD;
    endian_convert_TxPD(pLocalTxPD);
    *(u16 *) & headptr[0] =
        uap_cpu_to_le16(skb->len + ((long) skb->data - (long) headptr));
    *(u16 *) & headptr[2] = uap_cpu_to_le16(MV_TYPE_DAT);
    ret =
        sbi_host_to_card(priv, headptr,
                         skb->len + ((long) skb->data - (long) headptr));
    if (ret) {
        PRINTM(ERROR, "uap_process_tx Error: sbi_host_to_card failed: 0x%X\n",
               ret);
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }
    PRINTM(DATA, "Data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", headptr,
                MIN(skb->len + sizeof(TxPD), DATA_DUMP_LEN));
  done:
    /* Freed skb */
    kfree_skb(skb);
    LEAVE();
    return ret;
}

/**
 *  @brief This function initializes the adapter structure
 *  and set default value to the member of adapter.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_init_sw(uap_private * priv)
{
    uap_adapter *Adapter = priv->adapter;

    ENTER();

    if (!(Adapter->CmdBuf = kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "Failed to allocate command buffer!\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }

    Adapter->cmd_pending = FALSE;
    Adapter->CmdWaitQWoken = FALSE;
    Adapter->ps_state = PS_STATE_AWAKE;
    Adapter->WakeupTries = 0;

    memset(&Adapter->PSConfirmSleep, 0, sizeof(PS_CMD_ConfirmSleep));
        /** SDIO header */
    Adapter->PSConfirmSleep.SDLen =
        uap_cpu_to_le16(sizeof(PS_CMD_ConfirmSleep));
    Adapter->PSConfirmSleep.SDType = uap_cpu_to_le16(MV_TYPE_CMD);
    Adapter->PSConfirmSleep.SeqNum = 0;
    Adapter->PSConfirmSleep.Command = uap_cpu_to_le16(HOST_CMD_SLEEP_CONFIRM);
    Adapter->PSConfirmSleep.Size = uap_cpu_to_le16(sizeof(HostCmd_DS_GEN));
    Adapter->PSConfirmSleep.Result = 0;

    init_waitqueue_head(&Adapter->cmdwait_q);
    OS_INIT_SEMAPHORE(&Adapter->CmdSem);

    skb_queue_head_init(&Adapter->tx_queue);
    skb_queue_head_init(&Adapter->cmd_queue);

    /* Status variable */
    Adapter->HardwareStatus = HWInitializing;

    /* PnP support */
    Adapter->SurpriseRemoved = FALSE;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22)
    Adapter->nl_sk = netlink_kernel_create(NETLINK_MARVELL,
                                           NL_MULTICAST_GROUP, NULL,
                                           THIS_MODULE);
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    Adapter->nl_sk = netlink_kernel_create(NETLINK_MARVELL,
                                           NL_MULTICAST_GROUP, NULL, NULL,
                                           THIS_MODULE);
#else
//    Adapter->nl_sk = netlink_kernel_create(&init_net, NETLINK_MARVELL,
//                                           NL_MULTICAST_GROUP, NULL, NULL,
//                                           THIS_MODULE);


struct netlink_kernel_cfg cfg = {
    .input = NULL,
};

    Adapter->nl_sk = netlink_kernel_create(&init_net, NETLINK_MARVELL, &cfg);

#endif
#endif
    if (!Adapter->nl_sk) {
        PRINTM(ERROR,
               "Could not initialize netlink event passing mechanism!\n");
    }
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function sends FUNC_SHUTDOWN command to firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS on success, otherwise failure code
 */
static int __exit
uap_func_shutdown(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u32 CmdSize;
    HostCmd_DS_GEN *cmd;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb;
    ENTER();
    if (Adapter->HardwareStatus != HWReady) {
        PRINTM(ERROR, "uap_func_shutdown:Hardware is not ready!\n");
        ret = -EFAULT;
        goto done;
    }
    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        ret = -ENOMEM;
        goto done;
    }
    CmdSize = sizeof(HostCmd_DS_GEN);
    cmd = (HostCmd_DS_GEN *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HostCmd_CMD_FUNC_SHUTDOWN);
    cmd->Size = uap_cpu_to_le16(CmdSize);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    PRINTM(CMND, "HostCmd_CMD_FUNC_SHUTDOWN\n");
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP_TIMEOUT)) {
        PRINTM(ERROR, "Fail to process cmd HostCmd_CMD_FUNC_SHUTDOWN\n");
        ret = -EFAULT;
        goto done;
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function initializes firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_init_fw(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    ENTER();

    sbi_disable_host_int(priv);
    /* Check if firmware is already running */
    if (sbi_check_fw_status(priv, 1) == UAP_STATUS_SUCCESS) {
        PRINTM(MSG, "UAP FW already running! Skip FW download\n");
    } else {
        if ((ret = request_firmware(&priv->fw_helper, helper_name,
                                    priv->hotplug_device)) < 0) {
            PRINTM(FATAL,
                   "request_firmware() failed (helper), error code = %#x\n",
                   ret);
            goto done;
        }

        /* Download the helper */
        ret = sbi_prog_helper(priv);

        if (ret) {
            PRINTM(FATAL,
                   "Bootloader in invalid state! Helper download failed!\n");
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        if ((ret = request_firmware(&priv->firmware, fw_name,
                                    priv->hotplug_device)) < 0) {
            PRINTM(FATAL, "request_firmware() failed, error code = %#x\n", ret);
            goto done;
        }

        /* Download the main firmware via the helper firmware */
        if (sbi_prog_fw_w_helper(priv)) {
            PRINTM(FATAL, "UAP FW download failed!\n");
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        /* Check if the firmware is downloaded successfully or not */
        if (sbi_check_fw_status(priv, MAX_FIRMWARE_POLL_TRIES) ==
            UAP_STATUS_FAILURE) {
            PRINTM(FATAL, "FW failed to be active in time!\n");
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        PRINTM(MSG, "UAP FW is active\n");
    }

    sbi_claim_irq(priv);
    sbi_enable_host_int(priv);
    priv->adapter->HardwareStatus = HWReady;

#if 0
    if (uap_func_init(priv) != UAP_STATUS_SUCCESS) {
        ret = UAP_STATUS_FAILURE;
        goto done;
    }
#endif

  done:
    if (priv->fw_helper)
        release_firmware(priv->fw_helper);
    if (priv->firmware)
        release_firmware(priv->firmware);
    LEAVE();
    return ret;

}

/**
 *  @brief This function frees the structure of adapter
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   n/a
 */
static void
uap_free_adapter(uap_private * priv)
{
    uap_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter) {
        if ((Adapter->nl_sk) && ((Adapter->nl_sk)->sk_socket)) {
            sock_release((Adapter->nl_sk)->sk_socket);
            Adapter->nl_sk = NULL;
        }
        if (Adapter->CmdBuf)
            kfree(Adapter->CmdBuf);
        skb_queue_purge(&priv->adapter->tx_queue);
        skb_queue_purge(&priv->adapter->cmd_queue);
        /* Free the adapter object itself */
        kfree(Adapter);
        priv->adapter = NULL;
    }

    LEAVE();
}

/**
 *  @brief This function handles the major job in uap driver.
 *  it handles the event generated by firmware, rx data received
 *  from firmware and tx data sent from kernel.
 *
 *  @param data    A pointer to uap_thread structure
 *  @return        BT_STATUS_SUCCESS
 */
static int
uap_service_main_thread(void *data)
{
    uap_thread *thread = data;
    uap_private *priv = thread->priv;
    uap_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    u8 ireg = 0;
    struct sk_buff *skb;
    ENTER();
    uap_activate_thread(thread);
    init_waitqueue_entry(&wait, current);
    current->flags |= PF_NOFREEZE;

    for (;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);
        if ((Adapter->WakeupTries) ||
            (!Adapter->IntCounter && Adapter->ps_state == PS_STATE_PRE_SLEEP) ||
            (!priv->adapter->IntCounter
             && (priv->uap_dev.data_sent ||
                 skb_queue_empty(&priv->adapter->tx_queue))
             && (priv->uap_dev.cmd_sent || Adapter->cmd_pending ||
                 skb_queue_empty(&priv->adapter->cmd_queue))
            )) {
            PRINTM(INFO, "Main: Thread sleeping...\n");
            schedule();
        }
        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);
        if (kthread_should_stop() || Adapter->SurpriseRemoved) {
            PRINTM(INFO, "main-thread: break from main thread: "
                   "SurpriseRemoved=0x%x\n", Adapter->SurpriseRemoved);
            /* Cancel pending command */
            if (Adapter->cmd_pending == TRUE) {
                /* Wake up cmd Q */
                Adapter->CmdWaitQWoken = TRUE;
                wake_up_interruptible(&Adapter->cmdwait_q);
            }
            break;
        }

        PRINTM(INFO, "Main: Thread waking up...\n");
        if (priv->adapter->IntCounter) {
            OS_INT_DISABLE;
            Adapter->IntCounter = 0;
            OS_INT_RESTORE;
            sbi_get_int_status(priv, &ireg);
        } else if ((priv->adapter->ps_state == PS_STATE_SLEEP) &&
                   (!skb_queue_empty(&priv->adapter->cmd_queue) ||
                    !skb_queue_empty(&priv->adapter->tx_queue))) {
            priv->adapter->WakeupTries++;
            PRINTM(CMND, "%lu : Wakeup device...\n", os_time_get());
            sbi_wakeup_firmware(priv);
            continue;
        }
        if (Adapter->ps_state == PS_STATE_PRE_SLEEP)
            uap_ps_cond_check(priv);

        /* The PS state is changed during processing of Sleep Request event
           above */
        if ((Adapter->ps_state == PS_STATE_SLEEP) ||
            (Adapter->ps_state == PS_STATE_PRE_SLEEP))
            continue;
        /* Execute the next command */
        if (!priv->uap_dev.cmd_sent && !Adapter->cmd_pending &&
            (Adapter->HardwareStatus == HWReady)) {
            if (!skb_queue_empty(&priv->adapter->cmd_queue)) {
                skb = skb_dequeue(&priv->adapter->cmd_queue);
                if (skb) {
                    Adapter->CmdSize = 0;
                    Adapter->cmd_pending = TRUE;
                    Adapter->cmd_wait_option = skb->cb[0];
                    if (sbi_host_to_card(priv, skb->data, skb->len)) {
                        PRINTM(ERROR, "Cmd:sbi_host_to_card failed!\n");
                        Adapter->cmd_pending = FALSE;
                        Adapter->dbg.num_cmd_host_to_card_failure++;
                        /* Wake up cmd Q */
                        Adapter->CmdWaitQWoken = TRUE;
                        wake_up_interruptible(&Adapter->cmdwait_q);
                    } else {
                        if (Adapter->cmd_wait_option ==
                            HostCmd_OPTION_WAITFORSEND) {
                            /* Wake up cmd Q */
                            Adapter->CmdWaitQWoken = TRUE;
                            wake_up_interruptible(&Adapter->cmdwait_q);
                            Adapter->cmd_wait_option = FALSE;
                        }
                    }
                    kfree_skb(skb);
                }
            }
        }
        if (!priv->uap_dev.data_sent && (Adapter->HardwareStatus == HWReady)) {
            if (!skb_queue_empty(&priv->adapter->tx_queue)) {
                skb = skb_dequeue(&priv->adapter->tx_queue);
                if (skb) {
                    if (uap_process_tx(priv, skb)) {
                        priv->stats.tx_dropped++;
                        priv->stats.tx_errors++;
                        os_start_queue(priv);
                    } else {
                        priv->stats.tx_packets++;
                        priv->stats.tx_bytes += skb->len;
                    }

                }
            }
        }
    }
    uap_deactivate_thread(thread);
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief uap hostcmd ioctl handler
 *
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         UAP_STATUS_SUCCESS --success, otherwise fail
 */
/*********  format of ifr_data *************/
/*    buf_len + Hostcmd_body 		   */
/*    buf_len: 4 bytes                     */
/*             the length of the buf which */
/*             can be used to return data  */
/*             to application		   */
/*    Hostcmd_body       	           */
/*******************************************/
static int
uap_hostcmd_ioctl(struct net_device *dev, struct ifreq *req)
{
    u32 buf_len;
    HostCmd_HEADER head;
    uap_private *priv = (uap_private *) netdev_priv(dev);
    uap_adapter *Adapter = priv->adapter;
    int ret = UAP_STATUS_SUCCESS;
    struct sk_buff *skb;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(ERROR, "uap_hostcmd_ioctl() corrupt data\n");
        LEAVE();
        return -EFAULT;
    }
    if (copy_from_user(&buf_len, req->ifr_data, sizeof(buf_len))) {
        PRINTM(ERROR, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }
    memset(&head, 0, sizeof(HostCmd_HEADER));
    /* Get the command size from user space */
    if (copy_from_user
        (&head, req->ifr_data + sizeof(buf_len), sizeof(HostCmd_HEADER))) {
        PRINTM(ERROR, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }
    head.Size = uap_le16_to_cpu(head.Size);
    if (head.Size > MRVDRV_SIZE_OF_CMD_BUFFER) {
        PRINTM(ERROR, "CmdSize too big=%d\n", head.Size);
        LEAVE();
        return -EFAULT;
    }
    PRINTM(CMND, "ioctl: hostcmd=%x, size=%d,buf_len=%d\n", head.Command,
           head.Size, buf_len);
    skb = dev_alloc_skb(head.Size + INTF_HEADER_LEN);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        LEAVE();
        return -ENOMEM;
    }

    /* Get the command from user space */
    if (copy_from_user
        (skb->data + INTF_HEADER_LEN, req->ifr_data + sizeof(buf_len),
         head.Size)) {
        PRINTM(ERROR, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }
    skb_put(skb, head.Size + INTF_HEADER_LEN);
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP)) {
        PRINTM(ERROR, "Fail to process cmd\n");
        LEAVE();
        return -EFAULT;
    }
    if (!Adapter->CmdSize) {
        PRINTM(ERROR, "Cmd Size is 0\n");
        LEAVE();
        return -EFAULT;
    }
    if (Adapter->CmdSize > buf_len) {
        PRINTM(ERROR, "buf_len is too small\n");
        LEAVE();
        return -EFAULT;
    }
    /* Copy to user */
    if (copy_to_user
        (req->ifr_data + sizeof(buf_len), Adapter->CmdBuf, Adapter->CmdSize)) {
        PRINTM(ERROR, "Copy to user failed!\n");
        LEAVE();
        return -EFAULT;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief uap power mode ioctl handler
 *
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         UAP_STATUS_SUCCESS --success, otherwise fail
 */
static int
uap_power_mode_ioctl(struct net_device *dev, struct ifreq *req)
{
    ps_mgmt pm_cfg;
    int ret = UAP_STATUS_SUCCESS;
    uap_private *priv = (uap_private *) netdev_priv(dev);
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb = NULL;
    HostCmd_DS_COMMAND *cmd;
    u32 CmdSize;
    u8 *tlv = NULL;
    MrvlIEtypes_sleep_param_t *sleep_tlv = NULL;
    MrvlIEtypes_inact_sleep_param_t *inact_tlv = NULL;
    u16 tlv_buf_left = 0;
    MrvlIEtypesHeader_t *tlvbuf = NULL;
    u16 tlv_type = 0;
    u16 tlv_len = 0;

    ENTER();

    /* Sanity check */
    if (req->ifr_data == NULL) {
        PRINTM(ERROR, "uap_power_mode_ioctl() corrupt data\n");
        LEAVE();
        return -EFAULT;
    }

    memset(&pm_cfg, 0, sizeof(ps_mgmt));
    if (copy_from_user(&pm_cfg, req->ifr_data, sizeof(ps_mgmt))) {
        PRINTM(ERROR, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }
    PRINTM(CMND,
           "ioctl power: flag=0x%x ps_mode=%d ctrl_bitmap=%d min_sleep=%d max_sleep=%d "
           "inact_to=%d min_awake=%d max_awake=%d\n", pm_cfg.flags,
           (int) pm_cfg.ps_mode, (int) pm_cfg.sleep_param.ctrl_bitmap,
           (int) pm_cfg.sleep_param.min_sleep,
           (int) pm_cfg.sleep_param.max_sleep,
           (int) pm_cfg.inact_param.inactivity_to,
           (int) pm_cfg.inact_param.min_awake,
           (int) pm_cfg.inact_param.max_awake);

    if (pm_cfg.
        flags & ~(PS_FLAG_PS_MODE | PS_FLAG_SLEEP_PARAM |
                  PS_FLAG_INACT_SLEEP_PARAM)) {
        PRINTM(ERROR, "Invalid parameter: flags = 0x%x\n", pm_cfg.flags);
        ret = -EINVAL;
        goto done;
    }
    if (pm_cfg.ps_mode > PS_MODE_INACTIVITY) {
        PRINTM(ERROR, "Invalid parameter: ps_mode = %d\n", (int) pm_cfg.flags);
        ret = -EINVAL;
        goto done;
    }

    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(INFO, "No free skb\n");
        ret = -ENOMEM;
        goto done;
    }

    CmdSize = S_DS_GEN + sizeof(HostCmd_DS_POWER_MGMT_EXT);

    cmd = (HostCmd_DS_COMMAND *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HOST_CMD_POWER_MGMT_EXT);
    if (!pm_cfg.flags) {
        cmd->params.pm_cfg.action = uap_cpu_to_le16(ACTION_GET);
    } else {
        cmd->params.pm_cfg.action = uap_cpu_to_le16(ACTION_SET);
        cmd->params.pm_cfg.power_mode = uap_cpu_to_le16(pm_cfg.ps_mode);
        tlv = (u8 *) & cmd->params.pm_cfg + sizeof(HostCmd_DS_POWER_MGMT_EXT);

        if ((pm_cfg.ps_mode) && (pm_cfg.flags & PS_FLAG_SLEEP_PARAM)) {
            sleep_tlv = (MrvlIEtypes_sleep_param_t *) tlv;
            sleep_tlv->header.Type = uap_cpu_to_le16(TLV_TYPE_AP_SLEEP_PARAM);
            sleep_tlv->header.Len =
                uap_cpu_to_le16(sizeof(MrvlIEtypes_sleep_param_t) -
                                sizeof(MrvlIEtypesHeader_t));
            sleep_tlv->ctrl_bitmap =
                uap_cpu_to_le32(pm_cfg.sleep_param.ctrl_bitmap);
            sleep_tlv->min_sleep =
                uap_cpu_to_le32(pm_cfg.sleep_param.min_sleep);
            sleep_tlv->max_sleep =
                uap_cpu_to_le32(pm_cfg.sleep_param.max_sleep);
            CmdSize += sizeof(MrvlIEtypes_sleep_param_t);
            tlv += sizeof(MrvlIEtypes_sleep_param_t);
        }
        if ((pm_cfg.ps_mode == PS_MODE_INACTIVITY) &&
            (pm_cfg.flags & PS_FLAG_INACT_SLEEP_PARAM)) {
            inact_tlv = (MrvlIEtypes_inact_sleep_param_t *) tlv;
            inact_tlv->header.Type =
                uap_cpu_to_le16(TLV_TYPE_AP_INACT_SLEEP_PARAM);
            inact_tlv->header.Len =
                uap_cpu_to_le16(sizeof(MrvlIEtypes_inact_sleep_param_t) -
                                sizeof(MrvlIEtypesHeader_t));
            inact_tlv->inactivity_to =
                uap_cpu_to_le32(pm_cfg.inact_param.inactivity_to);
            inact_tlv->min_awake =
                uap_cpu_to_le32(pm_cfg.inact_param.min_awake);
            inact_tlv->max_awake =
                uap_cpu_to_le32(pm_cfg.inact_param.max_awake);
            CmdSize += sizeof(MrvlIEtypes_inact_sleep_param_t);
            tlv += sizeof(MrvlIEtypes_inact_sleep_param_t);
        }
    }
    cmd->Size = uap_cpu_to_le16(CmdSize);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP)) {
        PRINTM(ERROR, "Fail to process cmd POWER_MODE\n");
        ret = -EFAULT;
        goto done;
    }
    if (!Adapter->CmdSize) {
        PRINTM(ERROR, "Cmd Size is 0\n");
        ret = -EFAULT;
        goto done;
    }
    cmd = (HostCmd_DS_COMMAND *) Adapter->CmdBuf;
    cmd->Result = uap_le16_to_cpu(cmd->Result);
    if (cmd->Result != UAP_STATUS_SUCCESS) {
        PRINTM(ERROR, "HOST_CMD_APCMD_POWER_MODE fail=%x\n", cmd->Result);
        ret = -EFAULT;
        goto done;
    }
    if (pm_cfg.flags) {
        Adapter->psmode = uap_le16_to_cpu(cmd->params.pm_cfg.power_mode);
    } else {
        pm_cfg.flags = PS_FLAG_PS_MODE;
        pm_cfg.ps_mode = uap_le16_to_cpu(cmd->params.pm_cfg.power_mode);
        tlv_buf_left =
            cmd->Size - (sizeof(HostCmd_DS_POWER_MGMT_EXT) + S_DS_GEN);
        tlvbuf =
            (MrvlIEtypesHeader_t *) ((u8 *) & cmd->params.pm_cfg +
                                     sizeof(HostCmd_DS_POWER_MGMT_EXT));
        while (tlv_buf_left >= sizeof(MrvlIEtypesHeader_t)) {
            tlv_type = uap_le16_to_cpu(tlvbuf->Type);
            tlv_len = uap_le16_to_cpu(tlvbuf->Len);
            switch (tlv_type) {
            case TLV_TYPE_AP_SLEEP_PARAM:
                sleep_tlv = (MrvlIEtypes_sleep_param_t *) tlvbuf;
                pm_cfg.flags |= PS_FLAG_SLEEP_PARAM;
                pm_cfg.sleep_param.ctrl_bitmap =
                    uap_le32_to_cpu(sleep_tlv->ctrl_bitmap);
                pm_cfg.sleep_param.min_sleep =
                    uap_le32_to_cpu(sleep_tlv->min_sleep);
                pm_cfg.sleep_param.max_sleep =
                    uap_le32_to_cpu(sleep_tlv->max_sleep);
                break;
            case TLV_TYPE_AP_INACT_SLEEP_PARAM:
                inact_tlv = (MrvlIEtypes_inact_sleep_param_t *) tlvbuf;
                pm_cfg.flags |= PS_FLAG_INACT_SLEEP_PARAM;
                pm_cfg.inact_param.inactivity_to =
                    uap_le32_to_cpu(inact_tlv->inactivity_to);
                pm_cfg.inact_param.min_awake =
                    uap_le32_to_cpu(inact_tlv->min_awake);
                pm_cfg.inact_param.max_awake =
                    uap_le32_to_cpu(inact_tlv->max_awake);
                break;
            }
            tlv_buf_left -= tlv_len + sizeof(MrvlIEtypesHeader_t);
            tlvbuf =
                (MrvlIEtypesHeader_t *) ((u8 *) tlvbuf + tlv_len +
                                         sizeof(MrvlIEtypesHeader_t));
        }
        /* Copy to user */
        if (copy_to_user(req->ifr_data, &pm_cfg, sizeof(ps_mgmt))) {
            PRINTM(ERROR, "Copy to user failed!\n");
            LEAVE();
            return -EFAULT;
        }
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function send bss_stop command to firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS on success, otherwise failure code
 */
static int
uap_bss_stop(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u32 CmdSize;
    HostCmd_DS_GEN *cmd;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb;
    ENTER();
    if (Adapter->HardwareStatus != HWReady) {
        PRINTM(ERROR, "uap_bss_stop:Hardware is not ready!\n");
        ret = -EFAULT;
        goto done;
    }
    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        ret = -ENOMEM;
        goto done;
    }
    CmdSize = sizeof(HostCmd_DS_GEN);
    cmd = (HostCmd_DS_GEN *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HOST_CMD_APCMD_BSS_STOP);
    cmd->Size = uap_cpu_to_le16(CmdSize);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    PRINTM(CMND, "APCMD_BSS_STOP\n");
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP_TIMEOUT)) {
        PRINTM(ERROR, "Fail to process cmd BSS_STOP\n");
        ret = -EFAULT;
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
 *  @brief This function send soft_reset command to firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @return 	   UAP_STATUS_SUCCESS on success, otherwise failure code
 */
int
uap_soft_reset(uap_private * priv)
{
    int ret = UAP_STATUS_SUCCESS;
    u32 CmdSize;
    HostCmd_DS_GEN *cmd;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb;
    ENTER();

    ret = uap_bss_stop(priv);
    if (ret != UAP_STATUS_SUCCESS)
        goto done;
    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        ret = -ENOMEM;
        goto done;
    }
    CmdSize = sizeof(HostCmd_DS_GEN);
    cmd = (HostCmd_DS_GEN *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HOST_CMD_APCMD_SOFT_RESET);
    cmd->Size = uap_cpu_to_le16(CmdSize);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    PRINTM(CMND, "APCMD_SOFT_RESET\n");
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORSEND)) {
        PRINTM(ERROR, "Fail to process cmd SOFT_RESET\n");
        ret = -EFAULT;
        goto done;
    }
    Adapter->SurpriseRemoved = TRUE;
    /* delay to allow hardware complete reset */
    os_sched_timeout(5);
    if (priv->MediaConnected == TRUE) {
        os_stop_queue(priv);
        os_carrier_off(priv);
        priv->MediaConnected = FALSE;
    }
    Adapter->CmdSize = 0;
    Adapter->CmdWaitQWoken = TRUE;
    wake_up_interruptible(&Adapter->cmdwait_q);
    skb_queue_purge(&priv->adapter->tx_queue);
    skb_queue_purge(&priv->adapter->cmd_queue);
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function processes received packet and forwards it
 *  to kernel/upper layer
 *
 *  @param priv    A pointer to uap_private
 *  @param skb     A pointer to skb which includes the received packet
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
uap_process_rx_packet(uap_private * priv, struct sk_buff *skb)
{
    int ret = UAP_STATUS_SUCCESS;
    RxPD *pRxPD;
    ENTER();
    priv->adapter->ps_state = PS_STATE_AWAKE;
    pRxPD = (RxPD *) skb->data;
    endian_convert_RxPD(pRxPD);
    DBG_HEXDUMP(DAT_D, "Rx", skb->data, MIN(skb->len, DATA_DUMP_LEN));
    skb_pull(skb, pRxPD->RxPktOffset);
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += skb->len;
    os_upload_rx_packet(priv, skb);
    LEAVE();
    return ret;
}

/**
 *  @brief This function opens the network device
 *
 *  @param dev     A pointer to net_device structure
 *  @return        UAP_STATUS_SUCCESS
 */
static int
uap_open(struct net_device *dev)
{
    uap_private *priv = (uap_private *) (uap_private *) netdev_priv(dev);
    uap_adapter *Adapter = priv->adapter;
    int i = 0;

    ENTER();

    /* On some systems the device open handler will be called before HW ready. */
    /* Use the following flag check and wait function to work around the issue. */
    while ((Adapter->HardwareStatus != HWReady) &&
           (i < MAX_WAIT_DEVICE_READY_COUNT)) {
        i++;
        os_sched_timeout(100);
    }
    if (i >= MAX_WAIT_DEVICE_READY_COUNT) {
        PRINTM(FATAL, "HW not ready, uap_open() return failure\n");
        LEAVE();
        return UAP_STATUS_FAILURE;
    }

    if (MODULE_GET == 0)
        return UAP_STATUS_FAILURE;

    priv->open = TRUE;
    if (priv->MediaConnected == TRUE) {
        os_carrier_on(priv);
        os_start_queue(priv);
    } else {
        os_stop_queue(priv);
        os_carrier_off(priv);
    }
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function closes the network device
 *
 *  @param dev     A pointer to net_device structure
 *  @return        UAP_STATUS_SUCCESS
 */
static int
uap_close(struct net_device *dev)
{
    uap_private *priv = (uap_private *) netdev_priv(dev);

    ENTER();
    skb_queue_purge(&priv->adapter->tx_queue);
    os_stop_queue(priv);
    os_carrier_off(priv);

    MODULE_PUT;
    priv->open = FALSE;
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function returns the network statistics
 *
 *  @param dev     A pointer to uap_private structure
 *  @return        A pointer to net_device_stats structure
 */
static struct net_device_stats *
uap_get_stats(struct net_device *dev)
{
    uap_private *priv = (uap_private *) netdev_priv(dev);

    return &priv->stats;
}

/**
 *  @brief This function sets the MAC address to firmware.
 *
 *  @param dev     A pointer to uap_private structure
 *  @param addr    MAC address to set
 *  @return        UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_set_mac_address(struct net_device *dev, void *addr)
{
    int ret = UAP_STATUS_SUCCESS;
    uap_private *priv = (uap_private *) netdev_priv(dev);
    struct sockaddr *pHwAddr = (struct sockaddr *) addr;
    u32 CmdSize;
    HostCmd_DS_COMMAND *cmd;
    MrvlIEtypes_MacAddr_t *pMacAddrTlv;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb;

    ENTER();

    /* Dump MAC address */
    DBG_HEXDUMP(CMD_D, "Original MAC addr", dev->dev_addr, ETH_ALEN);
    DBG_HEXDUMP(CMD_D, "New MAC addr", pHwAddr->sa_data, ETH_ALEN);
    if (priv->open && (priv->MediaConnected == TRUE)) {
        os_carrier_on(priv);
        os_start_queue(priv);
    }
    skb = dev_alloc_skb(MRVDRV_SIZE_OF_CMD_BUFFER);
    if (!skb) {
        PRINTM(ERROR, "No free skb\n");
        LEAVE();
        return -ENOMEM;
    }
    CmdSize =
        S_DS_GEN + sizeof(HostCmd_SYS_CONFIG) + sizeof(MrvlIEtypes_MacAddr_t);
    cmd = (HostCmd_DS_COMMAND *) (skb->data + INTF_HEADER_LEN);
    cmd->Command = uap_cpu_to_le16(HOST_CMD_APCMD_SYS_CONFIGURE);
    cmd->Size = uap_cpu_to_le16(CmdSize);
    cmd->params.sys_config.Action = uap_cpu_to_le16(ACTION_SET);
    pMacAddrTlv =
        (MrvlIEtypes_MacAddr_t *) ((u8 *) cmd + S_DS_GEN +
                                   sizeof(HostCmd_SYS_CONFIG));
    pMacAddrTlv->Header.Type = uap_cpu_to_le16(MRVL_AP_MAC_ADDRESS_TLV_ID);
    pMacAddrTlv->Header.Len = uap_cpu_to_le16(ETH_ALEN);
    memcpy(pMacAddrTlv->ApMacAddr, pHwAddr->sa_data, ETH_ALEN);
    skb_put(skb, CmdSize + INTF_HEADER_LEN);
    PRINTM(CMND, "set_mac_address\n");
    if (UAP_STATUS_SUCCESS !=
        uap_process_cmd(priv, skb, HostCmd_OPTION_WAITFORRSP_TIMEOUT)) {
        PRINTM(ERROR, "Fail to set mac address\n");
        LEAVE();
        return -EFAULT;
    }
    if (!Adapter->CmdSize) {
        PRINTM(ERROR, "Cmd Size is 0\n");
        LEAVE();
        return -EFAULT;
    }
    cmd = (HostCmd_DS_COMMAND *) Adapter->CmdBuf;
    cmd->Result = uap_cpu_to_le16(cmd->Result);
    if (cmd->Result != UAP_STATUS_SUCCESS) {
        PRINTM(ERROR, "set mac addrress fail,cmd result=%x\n", cmd->Result);
        ret = -EFAULT;
    } else
        memcpy(dev->dev_addr, pHwAddr->sa_data, ETH_ALEN);
    LEAVE();
    return ret;
}

/**
 *  @brief This function sets multicast addresses to firmware
 *
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
uap_set_multicast_list(struct net_device *dev)
{
    ENTER();
#warning uap_set_multicast_list not implemented. Expect problems with IPv6.
    LEAVE();
}

/**
 *  @brief This function handles the timeout of packet
 *  transmission
 *
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
uap_tx_timeout(struct net_device *dev)
{
    uap_private *priv = (uap_private *) netdev_priv(dev);

    ENTER();
    PRINTM(DATA, "Tx timeout\n");
    UpdateTransStart(dev);
    priv->num_tx_timeout++;
    priv->adapter->IntCounter++;
    wake_up_interruptible(&priv->MainThread.waitQ);

    LEAVE();
}

/**
 *  @brief This function handles packet transmission
 *
 *  @param skb     A pointer to sk_buff structure
 *  @param dev     A pointer to net_device structure
 *  @return        UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int
uap_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    uap_private *priv = (uap_private *) netdev_priv(dev);
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    PRINTM(DATA, "Data <= kernel\n");
    DBG_HEXDUMP(DAT_D, "Tx", skb->data, MIN(skb->len, DATA_DUMP_LEN));
    /* skb sanity check */
    if (!skb->len || (skb->len > MRVDRV_MAXIMUM_ETH_PACKET_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n", skb->len,
               MRVDRV_MAXIMUM_ETH_PACKET_SIZE);
        priv->stats.tx_dropped++;
        kfree(skb);
        goto done;
    }
    skb_queue_tail(&priv->adapter->tx_queue, skb);
    wake_up_interruptible(&priv->MainThread.waitQ);
    if (skb_queue_len(&priv->adapter->tx_queue) > TX_HIGH_WATERMARK) {
        UpdateTransStart(dev);
        os_stop_queue(priv);
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief ioctl function - entry point
 *
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @param cmd      command
 *  @return         UAP_STATUS_SUCCESS--success, otherwise fail
 */
static int
uap_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    int ret = UAP_STATUS_SUCCESS;

    ENTER();

    PRINTM(CMND, "uap_do_ioctl: ioctl cmd = 0x%x\n", cmd);

    switch (cmd) {
    case UAPHOSTCMD:
        ret = uap_hostcmd_ioctl(dev, req);
        break;
    case UAP_POWER_MODE:
        ret = uap_power_mode_ioctl(dev, req);
        break;
    default:
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function handles events generated by firmware
 *
 *  @param priv    A pointer to uap_private structure
 *  @param payload A pointer to payload buffer
 *  @param len	   Length of the payload
 *  @return 	   UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
int
uap_process_event(uap_private * priv, u8 * payload, uint len)
{
    int ret = UAP_STATUS_SUCCESS;
    uap_adapter *Adapter = priv->adapter;
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nlh = NULL;
    struct sock *sk = Adapter->nl_sk;
    AP_Event *pEvent;

    ENTER();

    Adapter->ps_state = PS_STATE_AWAKE;
    if (len > NL_MAX_PAYLOAD) {
        PRINTM(ERROR, "event size is too big!!! len=%d\n", len);
        ret = UAP_STATUS_FAILURE;
        goto done;
    }
    pEvent = (AP_Event *) payload;
    switch (pEvent->EventId) {
    case MICRO_AP_EV_ID_BSS_START:
        memcpy(priv->uap_dev.netdev->dev_addr, pEvent->MacAddr, ETH_ALEN);
        DBG_HEXDUMP(CMD_D, "BSS MAC addr", priv->uap_dev.netdev->dev_addr,
                    ETH_ALEN);
        break;
    case MICRO_AP_EV_BSS_ACTIVE:
        // carrier on
        priv->MediaConnected = TRUE;
        os_carrier_on(priv);
        os_start_queue(priv);
        break;
    case MICRO_AP_EV_BSS_IDLE:
        os_stop_queue(priv);
        os_carrier_off(priv);
        priv->MediaConnected = FALSE;
        break;
    case EVENT_PS_AWAKE:
        PRINTM(CMND, "UAP: PS_AWAKE\n");
        Adapter->ps_state = PS_STATE_AWAKE;
        Adapter->WakeupTries = 0;
        break;
    case EVENT_PS_SLEEP:
        PRINTM(CMND, "UAP: PS_SLEEP\n");
        Adapter->ps_state = PS_STATE_PRE_SLEEP;
        break;
    default:
        break;
    }
    if ((pEvent->EventId == EVENT_PS_AWAKE) ||
        (pEvent->EventId == EVENT_PS_SLEEP))
        goto done;
    if (sk) {
        /* Allocate skb */
        if (!(skb = alloc_skb(NLMSG_SPACE(NL_MAX_PAYLOAD), GFP_ATOMIC))) {
            PRINTM(ERROR, "Could not allocate skb for netlink.\n");
            ret = UAP_STATUS_FAILURE;
            goto done;
        }
        nlh = (struct nlmsghdr *) skb->data;
        nlh->nlmsg_len = NLMSG_SPACE(len);

        /* From kernel */
        nlh->nlmsg_pid = 0;
        nlh->nlmsg_flags = 0;

        /* Data */
        skb_put(skb, nlh->nlmsg_len);
        memcpy(NLMSG_DATA(nlh), payload, len);

        /* From Kernel */
        NETLINK_CB(skb).portid = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        /* Multicast message */
        NETLINK_CB(skb).dst_pid = 0;
#endif

        /* Multicast group number */
        NETLINK_CB(skb).dst_group = NL_MULTICAST_GROUP;

        /* Send message */
        netlink_broadcast(sk, skb, 0, NL_MULTICAST_GROUP, GFP_KERNEL);

        ret = UAP_STATUS_SUCCESS;
    } else {
        PRINTM(ERROR, "Could not send event through NETLINK. Link down.\n");
        ret = UAP_STATUS_FAILURE;
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the interrupt. it will change PS
 *  state if applicable. it will wake up main_thread to handle
 *  the interrupt event as well.
 *
 *  @param priv    A pointer to uap_private structure
 *  @return        n/a
 */
void
uap_interrupt(uap_private * priv)
{
    ENTER();
    priv->adapter->IntCounter++;
    priv->adapter->WakeupTries = 0;
    wake_up_interruptible(&priv->MainThread.waitQ);

    LEAVE();

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
/** Network device handlers */
static const struct net_device_ops uap_netdev_ops = {
    .ndo_open = uap_open,
    .ndo_start_xmit = uap_hard_start_xmit,
    .ndo_stop = uap_close,
    .ndo_do_ioctl = uap_do_ioctl,
    .ndo_set_mac_address = uap_set_mac_address,
    .ndo_tx_timeout = uap_tx_timeout,
    .ndo_get_stats = uap_get_stats,
};
#endif

/**
 * @brief This function adds the card. it will probe the
 * card, allocate the uap_priv and initialize the device.
 *
 *  @param card    A pointer to card
 *  @return        A pointer to uap_private structure
 */
uap_private *
uap_add_card(void *card)
{
    struct net_device *dev = NULL;
    uap_private *priv = NULL;

    ENTER();

    if (OS_ACQ_SEMAPHORE_BLOCK(&AddRemoveCardSem))
        goto exit_sem_err;

    /* Allocate an Ethernet device */
    if (!(dev = alloc_etherdev(sizeof(uap_private)))) {
        PRINTM(FATAL, "Init ethernet device failed!\n");
        goto error;
    }
    priv = (uap_private *) netdev_priv(dev);

    /* Allocate name */
    if (dev_alloc_name(dev, "uap%d") < 0) {
        PRINTM(ERROR, "Could not allocate device name!\n");
        goto error;
    }

    /* Allocate buffer for uap_adapter */
    if (!(priv->adapter = kmalloc(sizeof(uap_adapter), GFP_KERNEL))) {
        PRINTM(FATAL, "Allocate buffer for uap_adapter failed!\n");
        goto error;
    }
    memset(priv->adapter, 0, sizeof(uap_adapter));

    priv->uap_dev.netdev = dev;
    priv->uap_dev.card = card;
    priv->MediaConnected = FALSE;
    uappriv = priv;
    ((struct sdio_mmc_card *) card)->priv = priv;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    SET_MODULE_OWNER(dev);
#endif

    /* Setup the OS Interface to our functions */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
    dev->open = uap_open;
    dev->stop = uap_close;
    dev->hard_start_xmit = uap_hard_start_xmit;
    dev->tx_timeout = uap_tx_timeout;
    dev->get_stats = uap_get_stats;
    dev->do_ioctl = uap_do_ioctl;
    dev->set_mac_address = uap_set_mac_address;
    dev->set_multicast_list = uap_set_multicast_list;
#else
    dev->netdev_ops = &uap_netdev_ops;
#endif
    dev->watchdog_timeo = MRVDRV_DEFAULT_WATCHDOG_TIMEOUT;
    dev->hard_header_len += sizeof(TxPD) + INTF_HEADER_LEN;
    dev->hard_header_len += HEADER_ALIGNMENT;
#define NETIF_F_DYNALLOC 16
    dev->features |= NETIF_F_DYNALLOC;
    dev->flags |= IFF_BROADCAST | IFF_MULTICAST;

    /* Init SW */
    if (uap_init_sw(priv)) {
        PRINTM(FATAL, "Software Init Failed\n");
        goto error;
    }

    PRINTM(INFO, "Starting kthread...\n");
    priv->MainThread.priv = priv;
    spin_lock_init(&priv->driver_lock);
    uap_create_thread(uap_service_main_thread, &priv->MainThread,
                      "uap_main_service");
    while (priv->MainThread.pid == 0) {
        os_sched_timeout(2);
    }

    /* Register the device */
    if (sbi_register_dev(priv) < 0) {
        PRINTM(FATAL, "Failed to register uap device!\n");
        goto err_registerdev;
    }
#ifdef FW_DNLD_NEEDED
    SET_NETDEV_DEV(dev, priv->hotplug_device);
#endif

    /* Init FW and HW */
    if (uap_init_fw(priv)) {
        PRINTM(FATAL, "Firmware Init Failed\n");
        goto err_init_fw;
    }

    priv->uap_dev.cmd_sent = FALSE;
    priv->uap_dev.data_sent = FALSE;

    /* Get mac address from firmware */
    if (uap_get_mac_address(priv)) {
        PRINTM(FATAL, "Fail to get mac address\n");
        goto err_init_fw;
    }
    /* Register network device */
    if (register_netdev(dev)) {
        printk(KERN_ERR "Cannot register network device!\n");
        goto err_init_fw;
    }
#ifdef CONFIG_PROC_FS
    uap_proc_entry(priv, dev);
    uap_debug_entry(priv, dev);
#endif /* CPNFIG_PROC_FS */
    OS_REL_SEMAPHORE(&AddRemoveCardSem);

    LEAVE();
    return priv;
  err_init_fw:
    sbi_unregister_dev(priv);
  err_registerdev:
    ((struct sdio_mmc_card *) card)->priv = NULL;
    /* Stop the thread servicing the interrupts */
    priv->adapter->SurpriseRemoved = TRUE;
    wake_up_interruptible(&priv->MainThread.waitQ);
    while (priv->MainThread.pid) {
        os_sched_timeout(1);
    }
  error:
    if (dev) {
        if (dev->reg_state == NETREG_REGISTERED)
            unregister_netdev(dev);
        if (priv->adapter)
            uap_free_adapter(priv);
        free_netdev(dev);
        uappriv = NULL;
    }
    OS_REL_SEMAPHORE(&AddRemoveCardSem);
  exit_sem_err:
    LEAVE();
    return NULL;
}

/**
 *  @brief This function removes the card.
 *
 *  @param card    A pointer to card
 *  @return        UAP_STATUS_SUCCESS
 */
int
uap_remove_card(void *card)
{
    uap_private *priv = uappriv;
    uap_adapter *Adapter;
    struct net_device *dev;

    ENTER();

    if (OS_ACQ_SEMAPHORE_BLOCK(&AddRemoveCardSem))
        goto exit_sem_err;

    if (!priv || !(Adapter = priv->adapter)) {
        goto exit_remove;
    }
    Adapter->SurpriseRemoved = TRUE;
    if (Adapter->cmd_pending == TRUE) {
        /* Wake up cmd Q */
        Adapter->CmdWaitQWoken = TRUE;
        wake_up_interruptible(&Adapter->cmdwait_q);
    }
    dev = priv->uap_dev.netdev;
    if (priv->MediaConnected == TRUE) {
        os_stop_queue(priv);
        os_carrier_off(priv);
        priv->MediaConnected = FALSE;
    }
    Adapter->CmdSize = 0;
    Adapter->CmdWaitQWoken = TRUE;
    wake_up_interruptible(&Adapter->cmdwait_q);
    skb_queue_purge(&priv->adapter->tx_queue);
    skb_queue_purge(&priv->adapter->cmd_queue);

    /* Disable interrupts on the card */
    sbi_disable_host_int(priv);
    PRINTM(INFO, "netdev_finish_unregister: %s%s.\n", dev->name,
           (dev->features & NETIF_F_DYNALLOC) ? "" : ", old style");
    unregister_netdev(dev);
    PRINTM(INFO, "Unregister finish\n");
    wake_up_interruptible(&priv->MainThread.waitQ);
    while (priv->MainThread.pid) {
        os_sched_timeout(1);
    }

    if ((Adapter->nl_sk) && ((Adapter->nl_sk)->sk_socket)) {
        sock_release((Adapter->nl_sk)->sk_socket);
        Adapter->nl_sk = NULL;
    }
#ifdef CONFIG_PROC_FS
    uap_debug_remove(priv);
    uap_proc_remove(priv);
#endif
    sbi_unregister_dev(priv);
    PRINTM(INFO, "Free Adapter\n");
    uap_free_adapter(priv);
    priv->uap_dev.netdev = NULL;
    free_netdev(dev);
    uappriv = NULL;

  exit_remove:
    OS_REL_SEMAPHORE(&AddRemoveCardSem);
  exit_sem_err:
    LEAVE();
    return UAP_STATUS_SUCCESS;
}

/**
 *  @brief This function initializes module.
 *
 *  @return    UAP_STATUS_SUCCESS or UAP_STATUS_FAILURE
 */
static int __init
uap_init_module(void)
{
    int ret = UAP_STATUS_SUCCESS;
    ENTER();

    OS_INIT_SEMAPHORE(&AddRemoveCardSem);
    ret = sbi_register();
    LEAVE();
    return ret;
}

/**
 *  @brief This function cleans module
 *
 *  @return        n/a
 */
static void __exit
uap_cleanup_module(void)
{
    ENTER();

    if (OS_ACQ_SEMAPHORE_BLOCK(&AddRemoveCardSem))
        goto exit_sem_err;

    if ((uappriv) && (uappriv->adapter)) {
        uap_func_shutdown(uappriv);
    }
    OS_REL_SEMAPHORE(&AddRemoveCardSem);
  exit_sem_err:
    sbi_unregister();
    LEAVE();
}

module_init(uap_init_module);
module_exit(uap_cleanup_module);
module_param(helper_name, charp, 0);
MODULE_PARM_DESC(helper_name, "Helper name");
module_param(fw_name, charp, 0);
MODULE_PARM_DESC(fw_name, "Firmware name");

MODULE_DESCRIPTION("M-UAP Driver");
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
