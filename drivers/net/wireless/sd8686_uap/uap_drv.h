/** @file uap_drv.h
  * @brief This file contains Linux OS related definitions and
  * declarations, uAP driver
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

#ifndef _UAP_DRV_H
#define _UAP_DRV_H

/** Driver release version */
#define DRIVER_VERSION		"26146"

/** True */
#ifndef	TRUE
#define TRUE			1
#endif
/** False */
#ifndef	FALSE
#define	FALSE			0
#endif

/** Bit definitions */
#ifndef BIT
#define BIT(x)	(1UL << (x))
#endif

/** Dma addresses are 32-bits wide.  */
#ifndef __ATTRIB_ALIGN__
#define __ATTRIB_ALIGN__ __attribute__((aligned(4)))
#endif

/**  attribute pack */
#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__ __attribute__ ((packed))
#endif

/** Debug Macro definition*/
#ifdef	DEBUG_LEVEL1

extern u32 drvdbg;

/** Debug message control bit definition for drvdbg */
/** Debug message */
#define	DBG_MSG		BIT(0)
/** Debug fatal message */
#define DBG_FATAL	BIT(1)
/** Debug error message */
#define DBG_ERROR	BIT(2)
/** Debug data message */
#define DBG_DATA	BIT(3)
/** Debug command message */
#define DBG_CMND	BIT(4)

/** Debug data */
#define DBG_DAT_D	BIT(16)
/** Debug command */
#define DBG_CMD_D	BIT(17)

/** Debug entry */
#define DBG_ENTRY	BIT(28)
/** Debug warning */
#define DBG_WARN	BIT(29)
/** Debug info */
#define DBG_INFO	BIT(30)

/** Print info */
#define	PRINTM_INFO(msg...)  {if (drvdbg & DBG_INFO) printk(KERN_DEBUG msg);}
/** Print warn message */
#define	PRINTM_WARN(msg...)  {if (drvdbg & DBG_WARN) printk(KERN_DEBUG msg);}
/** Print entry */
#define	PRINTM_ENTRY(msg...) {if (drvdbg & DBG_ENTRY) printk(KERN_DEBUG msg);}
/** Print cmd_d */
#define	PRINTM_CMD_D(msg...) {if (drvdbg & DBG_CMD_D) printk(KERN_DEBUG msg);}
/** Print data_d */
#define	PRINTM_DAT_D(msg...) {if (drvdbg & DBG_DAT_D) printk(KERN_DEBUG msg);}
/** Print command */
#define	PRINTM_CMND(msg...)  {if (drvdbg & DBG_CMND) printk(KERN_DEBUG msg);}
/** Print data */
#define	PRINTM_DATA(msg...)  {if (drvdbg & DBG_DATA) printk(KERN_DEBUG msg);}
/** Print error message */
#define	PRINTM_ERROR(msg...) {if (drvdbg & DBG_ERROR) printk(KERN_DEBUG msg);}
/** Print fatal message */
#define	PRINTM_FATAL(msg...) {if (drvdbg & DBG_FATAL) printk(KERN_DEBUG msg);}
/** Print message */
#define	PRINTM_MSG(msg...)   {if (drvdbg & DBG_MSG) printk(KERN_ALERT msg);}
/** Print level */
#define	PRINTM(level,msg...) PRINTM_##level(msg)

#else

#define	PRINTM(level,msg...) do {} while (0)

#endif /* DEBUG_LEVEL1 */

/** Wait until a condition becomes true */
#define ASSERT(cond)						\
do {								\
	if (!(cond))						\
		PRINTM(INFO, "ASSERT: %s, %s:%i\n",		\
		       __FUNCTION__, __FILE__, __LINE__);	\
} while(0)

/** Log enrty point for debugging */
#define ENTER()                 PRINTM(ENTRY, "Enter: %s, %s:%i\n", __FUNCTION__, \
                                                        __FILE__, __LINE__)
/** Log exit point for debugging */
#define LEAVE()                 PRINTM(ENTRY, "Leave: %s, %s:%i\n", __FUNCTION__, \
                                                        __FILE__, __LINE__)

#ifdef	DEBUG_LEVEL1
/** Dump buffer length */
#define DBG_DUMP_BUF_LEN    64
/** Maximum dump per line */
#define MAX_DUMP_PER_LINE   16
/** Data dump length */
#define DATA_DUMP_LEN       32

static inline void
hexdump(char *prompt, u8 * buf, int len)
{
    int i;
    char dbgdumpbuf[DBG_DUMP_BUF_LEN];
    char *ptr = dbgdumpbuf;

    printk(KERN_DEBUG "%s:\n", prompt);
    for (i = 1; i <= len; i++) {
        ptr += sprintf(ptr, "%02x ", *buf);
        buf++;
        if (i % MAX_DUMP_PER_LINE == 0) {
            *ptr = 0;
            printk(KERN_DEBUG "%s\n", dbgdumpbuf);
            ptr = dbgdumpbuf;
        }
    }
    if (len % MAX_DUMP_PER_LINE) {
        *ptr = 0;
        printk(KERN_DEBUG "%s\n", dbgdumpbuf);
    }
}

/** Debug command */
#define DBG_HEXDUMP_CMD_D(x,y,z)    {if (drvdbg & DBG_CMD_D) hexdump(x,y,z);}
/** Debug data */
#define DBG_HEXDUMP_DAT_D(x,y,z)    {if (drvdbg & DBG_DAT_D) hexdump(x,y,z);}
/** Debug hexdump */
#define	DBG_HEXDUMP(level,x,y,z)    DBG_HEXDUMP_##level(x,y,z)
/** hexdump */
#define HEXDUMP(x,y,z)              {if (drvdbg & DBG_INFO) hexdump(x,y,z);}
#else
/** Do nothing since debugging is not turned on */
#define DBG_HEXDUMP(level,x,y,z)    do {} while (0)
/** Do nothing since debugging is not turned on */
#define HEXDUMP(x,y,z)              do {} while (0)
#endif

/**
 * Typedefs
 */
/** Unsigned char */
typedef u8 BOOLEAN;

/*
 * OS macro definitions
 */
/** OS macro to get time */
#define os_time_get()	jiffies

/** OS macro to update transfer start time */
#define UpdateTransStart(dev) { \
	dev->trans_start = jiffies; \
}

/** Try to get a reference to the module */
#define MODULE_GET	try_module_get(THIS_MODULE)
/** Decrease module reference count */
#define MODULE_PUT	module_put(THIS_MODULE)

/** OS macro to initialize semaphore */
#define OS_INIT_SEMAPHORE(x)	sema_init(x,1)
/** OS macro to acquire blocking semaphore */
#define OS_ACQ_SEMAPHORE_BLOCK(x)	down_interruptible(x)
/** OS macro to acquire non-blocking semaphore */
#define OS_ACQ_SEMAPHORE_NOBLOCK(x)	down_trylock(x)
/** OS macro to release semaphore */
#define OS_REL_SEMAPHORE(x)		up(x)

static inline void
os_sched_timeout(u32 millisec)
{
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout((millisec * HZ) / 1000);
}

/** Maximum size of ethernet packet */
#define MRVDRV_MAXIMUM_ETH_PACKET_SIZE	1514

/** Maximum size of multicast list */
#define MRVDRV_MAX_MULTICAST_LIST_SIZE	32

/** Find minimum */
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

/** Find maximum */
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

/** Find number of elements */
#ifndef NELEMENTS
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#endif

/** Buffer Constants */

/** Size of command buffer */
#define MRVDRV_SIZE_OF_CMD_BUFFER       (2 * 1024)

/** Length of device length */
#define DEV_NAME_LEN			32

/** Length of ethernet address */
#ifndef	ETH_ALEN
#define ETH_ALEN			6
#endif

/** Default watchdog timeout */
#define MRVDRV_DEFAULT_WATCHDOG_TIMEOUT (2 * HZ)

/** Success */
#define UAP_STATUS_SUCCESS         (0)
/** Failure */
#define UAP_STATUS_FAILURE         (-1)
/** Not accepted */
#define UAP_STATUS_NOT_ACCEPTED    (-2)

/** Max loop count (* 100ms) for waiting device ready at init time */
#define MAX_WAIT_DEVICE_READY_COUNT	50

/** Tx high watermark. Stop Tx queue after this is crossed */
#define TX_HIGH_WATERMARK   4
/** Tx low watermark. Restart Tx queue after this is crossed */
#define TX_LOW_WATERMARK    2

/** Netlink protocol number */
#define NETLINK_MARVELL     (MAX_LINKS - 1)
/** Netlink maximum payload size */
#define NL_MAX_PAYLOAD      1024
/** Netlink multicast group number */
#define NL_MULTICAST_GROUP  1

/** 20 seconds */
#define MRVDRV_TIMER_20S		20000

/** Host Command option for wait till Send */
#define HostCmd_OPTION_WAITFORSEND            0x0001
/** Host Command option for wait for RSP */
#define HostCmd_OPTION_WAITFORRSP             0x0002
/** Host Command option for wait for RSP or Timeout */
#define HostCmd_OPTION_WAITFORRSP_TIMEOUT     0x0003
/** Host Command option for wait for RSP of sleep confirm */
#define HostCmd_OPTION_WAITFORRSP_SLEEPCONFIRM   0x0004

/** Sleep until a condition gets true or a timeout elapses */
#define os_wait_interruptible_timeout(waitq, cond, timeout) \
	wait_event_interruptible_timeout(waitq, cond, ((timeout) * HZ / 1000))

/** Private command ID to Host command */
#define	UAPHOSTCMD			(SIOCDEVPRIVATE + 1)

/** Private command ID to Power Mode */
#define	UAP_POWER_MODE			(SIOCDEVPRIVATE + 3)
/** sleep_param */
typedef struct _ps_sleep_param
{
    /** control bitmap */
    u32 ctrl_bitmap;
    /** minimum sleep period (micro second) */
    u32 min_sleep;
    /** maximum sleep period (micro second) */
    u32 max_sleep;
} ps_sleep_param;

/** inactivity sleep_param */
typedef struct _inact_sleep_param
{
    /** inactivity timeout (micro second) */
    u32 inactivity_to;
    /** miniumu awake period (micro second) */
    u32 min_awake;
    /** maximum awake period (micro second) */
    u32 max_awake;
} inact_sleep_param;

/** flag for ps mode */
#define PS_FLAG_PS_MODE                 1
/** flag for sleep param */
#define PS_FLAG_SLEEP_PARAM             2
/** flag for inactivity sleep param */
#define PS_FLAG_INACT_SLEEP_PARAM       4

/** Disable power mode */
#define PS_MODE_DISABLE                      0
/** Enable periodic dtim ps */
#define PS_MODE_PERIODIC_DTIM                1
/** Enable inactivity ps */
#define PS_MODE_INACTIVITY                   2

/** sleep parameter */
#define SLEEP_PARAMETER                     1
/** inactivity sleep parameter */
#define INACTIVITY_SLEEP_PARAMETER          2
/** ps_mgmt */
typedef struct _ps_mgmt
{
    /** flags for valid field */
    u16 flags;
    /** power mode */
    u16 ps_mode;
    /** sleep param */
    ps_sleep_param sleep_param;
    /** inactivity sleep param */
    inact_sleep_param inact_param;
} ps_mgmt;

/** Semaphore structure */
typedef struct semaphore SEMAPHORE;

/** Global Varibale Declaration */
/** Private data structure of the device */
typedef struct _uap_private uap_private;
/** Adapter data structure of the device */
typedef struct _uap_adapter uap_adapter;
/** private structure */
extern uap_private *uappriv;

/** ENUM definition*/

/** Hardware status codes */
typedef enum _HARDWARE_STATUS
{
    HWReady,
    HWInitializing,
    HWReset,
    HWClosing,
    HWNotReady
} HARDWARE_STATUS;

/** info for debug purpose */
typedef struct _uap_dbg
{
        /** Number of host to card command failures */
    u32 num_cmd_host_to_card_failure;
        /** Number of host to card Tx failures */
    u32 num_tx_host_to_card_failure;
} uap_dbg;

/** Set thread state */
#define OS_SET_THREAD_STATE(x)		set_current_state(x)

typedef struct
{
    /** Task */
    struct task_struct *task;
    /** Queue */
    wait_queue_head_t waitQ;
    /** PID */
    pid_t pid;
    /** Private structure */
    void *priv;
} uap_thread;

static inline void
uap_activate_thread(uap_thread * thr)
{
        /** Record the thread pid */
    thr->pid = current->pid;

        /** Initialize the wait queue */
    init_waitqueue_head(&thr->waitQ);
}

static inline void
uap_deactivate_thread(uap_thread * thr)
{
    thr->pid = 0;
    return;
}

static inline void
uap_create_thread(int (*uapfunc) (void *), uap_thread * thr, char *name)
{
    thr->task = kthread_run(uapfunc, thr, "%s", name);
}

static inline int
uap_terminate_thread(uap_thread * thr)
{
    /* Check if the thread is active or not */
    if (!thr->pid)
        return -1;
    kthread_stop(thr->task);
    return 0;
}

/** Data structure for the Marvell uAP device */
typedef struct _uap_dev
{
        /** device name */
    char name[DEV_NAME_LEN];
        /** card pointer */
    void *card;
        /** IO port */
    u32 ioport;
        /** Rx unit */
    u8 rx_unit;
        /** Data sent:
	    TRUE - Data is sent to fw, no Tx Done received
	    FALSE - Tx done received for previous Tx */
    BOOLEAN data_sent;
        /** CMD sent:
	    TRUE - CMD is sent to fw, no CMD Done received
	    FALSE - CMD done received for previous CMD */
    BOOLEAN cmd_sent;
        /** netdev pointer */
    struct net_device *netdev;
} uap_dev_t, *puap_dev_t;

/** Private structure for the MV device */
struct _uap_private
{
        /** Device open */
    int open;

        /** Device adapter structure */
    uap_adapter *adapter;
        /** Device structure */
    uap_dev_t uap_dev;

        /** Net device statistics structure */
    struct net_device_stats stats;

        /** Number of Tx timeouts */
    u32 num_tx_timeout;

        /** Media connection status */
    BOOLEAN MediaConnected;

#ifdef CONFIG_PROC_FS
    struct proc_dir_entry *proc_uap;
    struct proc_dir_entry *proc_entry;
#endif                          /* CONFIG_PROC_FS */

        /** Firmware helper */
    const struct firmware *fw_helper;
        /** Firmware */
    const struct firmware *firmware;
        /** Hotplug device */
    struct device *hotplug_device;
        /** thread to service interrupts */
    uap_thread MainThread;
        /** Driver lock */
    spinlock_t driver_lock;
        /** Driver lock flags */
    ulong driver_flags;

};

/** PS_CMD_ConfirmSleep */
typedef struct _PS_CMD_ConfirmSleep
{
        /** SDIO Length */
    u16 SDLen;
    /** SDIO Type */
    u16 SDType;
        /** Command */
    u16 Command;
        /** Size */
    u16 Size;
        /** Sequence number */
    u16 SeqNum;
        /** Result */
    u16 Result;
} __ATTRIB_PACK__ PS_CMD_ConfirmSleep, *PPS_CMD_ConfirmSleep;

/** Wlan Adapter data structure*/
struct _uap_adapter
{
        /** Power save confirm sleep command */
    PS_CMD_ConfirmSleep PSConfirmSleep;
        /** Device status */
    HARDWARE_STATUS HardwareStatus;
        /** Interrupt counter */
    u32 IntCounter;
        /** Tx packet queue */
    struct sk_buff_head tx_queue;
        /** Cmd packet queue */
    struct sk_buff_head cmd_queue;
        /** Command sequence number */
    u16 SeqNum;
        /** Command buffer */
    u8 *CmdBuf;
        /** cmd pending flag */
    u8 cmd_pending;
        /** cmd wait option */
    u8 cmd_wait_option;
        /** Command buffer length */
    u32 CmdSize;
        /** Command wait queue */
    wait_queue_head_t cmdwait_q __ATTRIB_ALIGN__;
        /** Command wait queue state flag */
    u8 CmdWaitQWoken;
        /** PnP support */
    BOOLEAN SurpriseRemoved;
        /** Debug */
    uap_dbg dbg;
        /** Netlink kernel socket */
    struct sock *nl_sk;
        /** Semaphore for CMD */
    SEMAPHORE CmdSem;
         /** Power Save mode */
    u8 psmode;
        /** Power Save state */
    u8 ps_state;
        /** Number of wakeup tries */
    u32 WakeupTries;
};

static inline int
os_upload_rx_packet(uap_private * priv, struct sk_buff *skb)
{
    skb->dev = priv->uap_dev.netdev;
    skb->protocol = eth_type_trans(skb, priv->uap_dev.netdev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    if (in_interrupt())
        netif_rx(skb);
    else
        netif_rx_ni(skb);
    return 0;
}

/*
 *  netif carrier_on/off and start(wake)/stop_queue handling
 */
static inline void
os_carrier_on(uap_private * priv)
{
    if (!netif_carrier_ok(priv->uap_dev.netdev) &&
        (priv->MediaConnected == TRUE)) {
        netif_carrier_on(priv->uap_dev.netdev);
    }
}

static inline void
os_carrier_off(uap_private * priv)
{
    if (netif_carrier_ok(priv->uap_dev.netdev)) {
        netif_carrier_off(priv->uap_dev.netdev);
    }
}

static inline void
os_start_queue(uap_private * priv)
{
    if (netif_queue_stopped(priv->uap_dev.netdev) &&
        (priv->MediaConnected == TRUE)) {
        netif_wake_queue(priv->uap_dev.netdev);
    }
}

static inline void
os_stop_queue(uap_private * priv)
{
    if (!netif_queue_stopped(priv->uap_dev.netdev)) {
        netif_stop_queue(priv->uap_dev.netdev);
    }
}

/** Interface specific header */
#define INTF_HEADER_LEN         4

/** headroom alignment for tx packet */
#define HEADER_ALIGNMENT	8

/** The number of times to try when polling for status bits */
#define MAX_POLL_TRIES			100

/** Length of SNAP header */
#define MRVDRV_SNAP_HEADER_LEN          8

/** Extra length of Tx packet buffer */
#define EXTRA_LEN	36

/** Buffer size for ethernet Tx packets */
#define MRVDRV_ETH_TX_PACKET_BUFFER_SIZE \
	(ETH_FRAME_LEN + sizeof(TxPD) + EXTRA_LEN)

/** Buffer size for ethernet Rx packets */
#define MRVDRV_ETH_RX_PACKET_BUFFER_SIZE \
	(ETH_FRAME_LEN + sizeof(RxPD) \
	 + MRVDRV_SNAP_HEADER_LEN + EXTRA_LEN)

/** Packet type: data, command & event */
typedef enum _mv_type
{
    MV_TYPE_DAT = 0,
    MV_TYPE_CMD = 1,
    MV_TYPE_EVENT = 3
} mv_type;

/** Disable interrupt */
#define OS_INT_DISABLE	spin_lock_irqsave(&priv->driver_lock, priv->driver_flags)
/** Enable interrupt */
#define	OS_INT_RESTORE	spin_unlock_irqrestore(&priv->driver_lock, priv->driver_flags)

int uap_process_rx_packet(uap_private * priv, struct sk_buff *skb);
void uap_interrupt(uap_private * priv);
uap_private *uap_add_card(void *card);
int uap_remove_card(void *card);
int uap_process_event(uap_private * priv, u8 * payload, uint len);
int uap_soft_reset(uap_private * priv);
int uap_process_sleep_confirm_resp(uap_private * priv, u8 * resp, int resp_len);

#ifdef CONFIG_PROC_FS
/** The proc fs interface */
void uap_proc_entry(uap_private * priv, struct net_device *dev);
void uap_proc_remove(uap_private * priv);
int string_to_number(char *s);
void uap_debug_entry(uap_private * priv, struct net_device *dev);
void uap_debug_remove(uap_private * priv);
#endif /* CONFIG_PROC_FS */

int sbi_register(void);
void sbi_unregister(void);
int sbi_register_dev(uap_private * priv);
int sbi_unregister_dev(uap_private * priv);
int sbi_prog_fw_w_helper(uap_private *);

int sbi_host_to_card(uap_private * priv, u8 * payload, u16 nb);
int sbi_claim_irq(uap_private * priv);
int sbi_enable_host_int(uap_private * priv);
int sbi_disable_host_int(uap_private * priv);

int sbi_get_int_status(uap_private * priv, u8 * ireg);
/** Check firmware status */
int sbi_check_fw_status(uap_private *, int);
int sbi_prog_helper(uap_private *);

int sbi_wakeup_firmware(uap_private * priv);

#endif /* _UAP_DRV_H */
