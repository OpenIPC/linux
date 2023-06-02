/** @file uap_proc.c
  * @brief This file contains functions for proc file.
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
#ifdef CONFIG_PROC_FS
#include "uap_headers.h"

/** /proc directory root */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#define PROC_DIR NULL
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#define PROC_DIR &proc_root
#else
#define PROC_DIR proc_net
#endif

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/**
 *  @brief proc read function
 *
 *  @param page	   pointer to buffer
 *  @param start   read data starting position
 *  @param offset  offset
 *  @param count   counter
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
uap_proc_read(char *page, char **start, off_t offset,
              int count, int *eof, void *data)
{
    int i;
    char *p = page;
    struct net_device *netdev = data;
    struct netdev_hw_addr *ha;
    char fmt[64];
    uap_private *priv = (uap_private *) netdev_priv(netdev);

    if (offset != 0) {
        *eof = 1;
        goto exit;
    }

    strcpy(fmt, DRIVER_VERSION);

    p += sprintf(p, "driver_name = " "\"uap\"\n");
    p += sprintf(p, "driver_version = %s-(FP%s)", fmt, FPNUM);
    p += sprintf(p, "\nInterfaceName=\"%s\"\n", netdev->name);
    p += sprintf(p, "State=\"%s\"\n",
                 ((priv->MediaConnected ==
                   FALSE) ? "Disconnected" : "Connected"));
    p += sprintf(p, "MACAddress=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                 netdev->dev_addr[0], netdev->dev_addr[1], netdev->dev_addr[2],
                 netdev->dev_addr[3], netdev->dev_addr[4], netdev->dev_addr[5]);
    i = 0;
    netdev_for_each_mc_addr(ha, netdev) {
        ++i;
    }
    p += sprintf(p, "MCCount=\"%d\"\n", i);

    /*
     * Put out the multicast list
     */
    i = 0;
    netdev_for_each_mc_addr(ha, netdev) {
        p += sprintf(p,
                     "MCAddr[%d]=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                     i++,
                     ha->addr[0], ha->addr[1],
                     ha->addr[2], ha->addr[3],
                     ha->addr[4], ha->addr[5]);
    }

    p += sprintf(p, "num_tx_bytes = %lu\n", priv->stats.tx_bytes);
    p += sprintf(p, "num_rx_bytes = %lu\n", priv->stats.rx_bytes);
    p += sprintf(p, "num_tx_pkts = %lu\n", priv->stats.tx_packets);
    p += sprintf(p, "num_rx_pkts = %lu\n", priv->stats.rx_packets);
    p += sprintf(p, "num_tx_pkts_dropped = %lu\n", priv->stats.tx_dropped);
    p += sprintf(p, "num_rx_pkts_dropped = %lu\n", priv->stats.rx_dropped);
    p += sprintf(p, "num_tx_pkts_err = %lu\n", priv->stats.tx_errors);
    p += sprintf(p, "num_rx_pkts_err = %lu\n", priv->stats.rx_errors);
    p += sprintf(p, "num_tx_timeout = %u\n", priv->num_tx_timeout);
    p += sprintf(p, "carrier %s\n",
                 ((netif_carrier_ok(priv->uap_dev.netdev)) ? "on" : "off"));
    p += sprintf(p, "tx queue %s\n",
                 ((netif_queue_stopped(priv->uap_dev.netdev)) ? "stopped" :
                  "started"));

  exit:
    return (p - page);
}

/**
 *  @brief hwstatus proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return	   number of data
 */
static int
uap_hwstatus_write(struct file *f, const char *buf, unsigned long cnt,
                   void *data)
{
    struct net_device *netdev = data;
    uap_private *priv = (uap_private *) netdev_priv(netdev);
    char databuf[10];
    int hwstatus;
    MODULE_GET;
    if (cnt > 10) {
        MODULE_PUT;
        return cnt;
    }
    if (copy_from_user(databuf, buf, cnt)) {
        MODULE_PUT;
        return 0;
    }
    hwstatus = string_to_number(databuf);
    switch (hwstatus) {
    case HWReset:
        PRINTM(MSG, "reset hw\n");
        uap_soft_reset(priv);
        priv->adapter->HardwareStatus = HWReset;
        break;
    default:
        break;
    }
    MODULE_PUT;
    return cnt;
}

/**
 *  @brief hwstatus proc read function
 *
 *  @param page	   pointer to buffer
 *  @param s       read data starting position
 *  @param off     offset
 *  @param cnt     counter
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return	   number of output data
 */
static int
uap_hwstatus_read(char *page, char **s, off_t off, int cnt, int *eof,
                  void *data)
{
    char *p = page;
    struct net_device *netdev = data;
    uap_private *priv = (uap_private *) netdev_priv(netdev);
    MODULE_GET;
    p += sprintf(p, "%d\n", priv->adapter->HardwareStatus);
    MODULE_PUT;
    return p - page;
}

/********************************************************
		Global Functions
********************************************************/
/**
 *  @brief create uap proc file
 *
 *  @param priv	   pointer uap_private
 *  @param dev     pointer net_device
 *  @return	   N/A
 */
void
uap_proc_entry(uap_private * priv, struct net_device *dev)
{
    struct proc_dir_entry *r = PROC_DIR;

    PRINTM(INFO, "Creating Proc Interface\n");
    /* Check if uap directory already exists */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
    for (r = r->subdir; r; r = r->next) {
        if (r->namelen && !strcmp("uap", r->name)) {
            /* Directory exists */
            PRINTM(WARN, "proc directory already exists!\n");
            priv->proc_uap = r;
            break;
        }
    }
#endif
    if (!priv->proc_uap) {
        priv->proc_uap = proc_mkdir("uap", PROC_DIR);
        if (!priv->proc_uap)
            return;
        else
            atomic_set(&priv->proc_uap->count, 1);
    } else {
        atomic_inc(&priv->proc_uap->count);
    }
    priv->proc_entry = proc_mkdir(dev->name, priv->proc_uap);

    if (priv->proc_entry) {
        r = create_proc_read_entry("info", 0, priv->proc_entry, uap_proc_read,
                                   dev);
        r = create_proc_entry("hwstatus", 0644, priv->proc_entry);
        if (r) {
            r->data = dev;
            r->read_proc = uap_hwstatus_read;
            r->write_proc = uap_hwstatus_write;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
            r->owner = THIS_MODULE;
#endif
        } else
            PRINTM(MSG, "Fail to create proc hwstatus\n");
    }
}

/**
 *  @brief remove proc file
 *
 *  @param priv	   pointer uap_private
 *  @return	   N/A
 */
void
uap_proc_remove(uap_private * priv)
{
    if (priv->proc_uap) {
        if (priv->proc_entry) {
            remove_proc_entry("info", priv->proc_entry);
            remove_proc_entry("hwstatus", priv->proc_entry);
        }
        remove_proc_entry(priv->uap_dev.netdev->name, priv->proc_uap);
        atomic_dec(&priv->proc_uap->count);
        if (atomic_read(&(priv->proc_uap->count)) == 0)
            remove_proc_entry("uap", PROC_DIR);
    }
}

/**
 *  @brief convert string to number
 *
 *  @param s	   pointer to numbered string
 *  @return	   converted number from string s
 */
int
string_to_number(char *s)
{
    int r = 0;
    int base = 0;
    int pn = 1;

    if (strncmp(s, "-", 1) == 0) {
        pn = -1;
        s++;
    }
    if ((strncmp(s, "0x", 2) == 0) || (strncmp(s, "0X", 2) == 0)) {
        base = 16;
        s += 2;
    } else
        base = 10;

    for (s = s; *s != 0; s++) {
        if ((*s >= '0') && (*s <= '9'))
            r = (r * base) + (*s - '0');
        else if ((*s >= 'A') && (*s <= 'F'))
            r = (r * base) + (*s - 'A' + 10);
        else if ((*s >= 'a') && (*s <= 'f'))
            r = (r * base) + (*s - 'a' + 10);
        else
            break;
    }

    return (r * pn);
}

#endif
