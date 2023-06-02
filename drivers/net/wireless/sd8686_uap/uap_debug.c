/** @file uap_debug.c
  * @brief This file contains functions for debug proc file.
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
#include  "uap_headers.h"

/********************************************************
		Local Variables
********************************************************/

#define item_size(n) (sizeof ((uap_adapter *)0)->n)
#define item_addr(n) ((u32) &((uap_adapter *)0)->n)

#define item_dbg_size(n) (sizeof (((uap_adapter *)0)->dbg.n))
#define item_dbg_addr(n) ((u32) &(((uap_adapter *)0)->dbg.n))

#define item_dev_size(n) (sizeof ((uap_dev_t *)0)->n)
#define item_dev_addr(n) ((u32) &((uap_dev_t *)0)->n)

/** MicroAp device offset */
#define OFFSET_UAP_DEV		0x01
/** Bluetooth adapter offset */
#define OFFSET_UAP_ADAPTER	0x02

struct debug_data
{
    /** Name */
    char name[32];
    /** Size */
    u32 size;
    /** Address */
    u32 addr;
    /** Offset */
    u32 offset;
    /** Flag */
    u32 flag;
};

/* To debug any member of uap_adapter, simply add one line here.
 */
static struct debug_data items[] = {
    {"cmd_sent", item_dev_size(cmd_sent), 0, item_dev_addr(cmd_sent),
     OFFSET_UAP_DEV},
    {"data_sent", item_dev_size(data_sent), 0, item_dev_addr(data_sent),
     OFFSET_UAP_DEV},
    {"IntCounter", item_size(IntCounter), 0, item_addr(IntCounter),
     OFFSET_UAP_ADAPTER},
    {"cmd_pending", item_size(cmd_pending), 0, item_addr(cmd_pending),
     OFFSET_UAP_ADAPTER},
    {"num_cmd_h2c_fail", item_dbg_size(num_cmd_host_to_card_failure), 0,
     item_dbg_addr(num_cmd_host_to_card_failure), OFFSET_UAP_ADAPTER},
    {"num_tx_h2c_fail", item_dbg_size(num_tx_host_to_card_failure), 0,
     item_dbg_addr(num_tx_host_to_card_failure), OFFSET_UAP_ADAPTER},
    {"psmode", item_size(psmode), 0, item_addr(psmode), OFFSET_UAP_ADAPTER},
    {"ps_state", item_size(ps_state), 0, item_addr(ps_state),
     OFFSET_UAP_ADAPTER},
#ifdef DEBUG_LEVEL1
    {"drvdbg", sizeof(drvdbg), (u32) & drvdbg, 0, 0}
#endif
};

static int num_of_items = sizeof(items) / sizeof(items[0]);

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
 *  @param s       read data starting position
 *  @param off     offset
 *  @param cnt     counter
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
uap_debug_read(char *page, char **s, off_t off, int cnt, int *eof, void *data)
{
    int val = 0;
    char *p = page;
    int i;

    struct debug_data *d = (struct debug_data *) data;

    if (MODULE_GET == 0)
        return UAP_STATUS_FAILURE;

    for (i = 0; i < num_of_items; i++) {
        if (d[i].size == 1)
            val = *((u8 *) d[i].addr);
        else if (d[i].size == 2)
            val = *((u16 *) d[i].addr);
        else if (d[i].size == 4)
            val = *((u32 *) d[i].addr);

        p += sprintf(p, "%s=%d\n", d[i].name, val);
    }
    MODULE_PUT;
    return p - page;
}

/**
 *  @brief proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
uap_debug_write(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    int r, i;
    char *pdata;
    char *p;
    char *p0;
    char *p1;
    char *p2;
    struct debug_data *d = (struct debug_data *) data;

    if (MODULE_GET == 0)
        return UAP_STATUS_FAILURE;

    pdata = (char *) kmalloc(cnt, GFP_KERNEL);
    if (pdata == NULL) {
        MODULE_PUT;
        return 0;
    }

    if (copy_from_user(pdata, buf, cnt)) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(pdata);
        MODULE_PUT;
        return 0;
    }

    p0 = pdata;
    for (i = 0; i < num_of_items; i++) {
        do {
            p = strstr(p0, d[i].name);
            if (p == NULL)
                break;
            p1 = strchr(p, '\n');
            if (p1 == NULL)
                break;
            p0 = p1++;
            p2 = strchr(p, '=');
            if (!p2)
                break;
            p2++;
            r = string_to_number(p2);
            if (d[i].size == 1)
                *((u8 *) d[i].addr) = (u8) r;
            else if (d[i].size == 2)
                *((u16 *) d[i].addr) = (u16) r;
            else if (d[i].size == 4)
                *((u32 *) d[i].addr) = (u32) r;
            break;
        } while (TRUE);
    }
    kfree(pdata);
#ifdef DEBUG_LEVEL1
    printk(KERN_ALERT "drvdbg = 0x%x\n", drvdbg);
    printk(KERN_ALERT "INFO  (%08lx) %s\n", DBG_INFO,
           (drvdbg & DBG_INFO) ? "X" : "");
    printk(KERN_ALERT "WARN  (%08lx) %s\n", DBG_WARN,
           (drvdbg & DBG_WARN) ? "X" : "");
    printk(KERN_ALERT "ENTRY (%08lx) %s\n", DBG_ENTRY,
           (drvdbg & DBG_ENTRY) ? "X" : "");
    printk(KERN_ALERT "CMD_D (%08lx) %s\n", DBG_CMD_D,
           (drvdbg & DBG_CMD_D) ? "X" : "");
    printk(KERN_ALERT "DAT_D (%08lx) %s\n", DBG_DAT_D,
           (drvdbg & DBG_DAT_D) ? "X" : "");
    printk(KERN_ALERT "CMND  (%08lx) %s\n", DBG_CMND,
           (drvdbg & DBG_CMND) ? "X" : "");
    printk(KERN_ALERT "DATA  (%08lx) %s\n", DBG_DATA,
           (drvdbg & DBG_DATA) ? "X" : "");
    printk(KERN_ALERT "ERROR (%08lx) %s\n", DBG_ERROR,
           (drvdbg & DBG_ERROR) ? "X" : "");
    printk(KERN_ALERT "FATAL (%08lx) %s\n", DBG_FATAL,
           (drvdbg & DBG_FATAL) ? "X" : "");
    printk(KERN_ALERT "MSG   (%08lx) %s\n", DBG_MSG,
           (drvdbg & DBG_MSG) ? "X" : "");
#endif
    MODULE_PUT;
    return cnt;
}

/********************************************************
		Global Functions
********************************************************/
/**
 *  @brief create debug proc file
 *
 *  @param priv	   pointer uap_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
void
uap_debug_entry(uap_private * priv, struct net_device *dev)
{
    int i;
    struct proc_dir_entry *r;

    if (priv->proc_entry == NULL)
        return;

    for (i = 0; i < num_of_items; i++) {
        if (items[i].flag & OFFSET_UAP_ADAPTER)
            items[i].addr = items[i].offset + (u32) priv->adapter;
        if (items[i].flag & OFFSET_UAP_DEV)
            items[i].addr = items[i].offset + (u32) & priv->uap_dev;
    }
    r = create_proc_entry("debug", 0644, priv->proc_entry);
    if (r == NULL)
        return;

    r->data = &items[0];
    r->read_proc = uap_debug_read;
    r->write_proc = uap_debug_write;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
    r->owner = THIS_MODULE;
#endif
}

/**
 *  @brief remove proc file
 *
 *  @param priv	   pointer uap_private
 *  @return 	   N/A
 */
void
uap_debug_remove(uap_private * priv)
{
    remove_proc_entry("debug", priv->proc_entry);
}

#endif
