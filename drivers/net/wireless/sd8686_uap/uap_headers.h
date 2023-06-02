/** @file uap_headers.h
 *
 * @brief This file contains all the necessary include file.
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
#ifndef _UAP_HEADERS_H
#define _UAP_HEADERS_H

/* Linux header files */
#include    <linux/kernel.h>
#include    <linux/module.h>
#include    <linux/init.h>
#include    <linux/version.h>
#include    <linux/param.h>
#include    <linux/types.h>
#include    <linux/interrupt.h>
#include    <linux/proc_fs.h>
#include    <linux/kthread.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include    <linux/semaphore.h>
#else
#include    <asm/semaphore.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#include    <linux/config.h>
#endif

/* Net header files */
#include    <linux/netdevice.h>
#include    <linux/net.h>
#include    <linux/skbuff.h>
#include    <linux/if_ether.h>
#include    <linux/etherdevice.h>
#include    <net/sock.h>
#include    <linux/netlink.h>
#include    <linux/firmware.h>
#include    <linux/delay.h>

#include    "uap_drv.h"
#include    "uap_fw.h"

#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include "uap_sdio_mmc.h"

#endif /* _UAP_HEADERS_H */
