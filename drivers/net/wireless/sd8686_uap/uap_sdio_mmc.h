/** @file uap_sdio_mmc.h
 *  @brief This file contains SDIO IF (interface) module
 *  related macros, enum, and structure.
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
	10/10/07: initial version
****************************************************/

#ifndef	_UAP_SDIO_MMC_H
#define	_UAP_SDIO_MMC_H

#include	<linux/mmc/sdio.h>
#include	<linux/mmc/sdio_ids.h>
#include	<linux/mmc/sdio_func.h>
#include	<linux/mmc/card.h>

#include	"uap_headers.h"

/** The number of times to try when waiting for downloaded firmware to
     become active. (polling the scratch register). */
#define MAX_FIRMWARE_POLL_TRIES		100

/** Firmware ready */
#define FIRMWARE_READY			0xfedc

/** Number of firmware blocks to transfer */
#define FIRMWARE_TRANSFER_NBLOCK	2

/* Host Control Registers */
/** Host Control Registers : I/O port 0 */
#define IO_PORT_0_REG			0x00
/** Host Control Registers : I/O port 1 */
#define IO_PORT_1_REG			0x01
/** Host Control Registers : I/O port 2 */
#define IO_PORT_2_REG			0x02

/** Host Control Registers : Configuration */
#define CONFIGURATION_REG		0x03
/** Host Control Registers : Host without Command 53 finish host */
#define HOST_WO_CMD53_FINISH_HOST	(0x1U << 2)
/** Host Control Registers : Host power up */
#define HOST_POWER_UP			(0x1U << 1)
/** Host Control Registers : Host power down */
#define HOST_POWER_DOWN			(0x1U << 0)

/** Host Control Registers : Host interrupt mask */
#define HOST_INT_MASK_REG		0x04
/** Host Control Registers : Upload host interrupt mask */
#define UP_LD_HOST_INT_MASK		(0x1U)
/** Host Control Registers : Download host interrupt mask */
#define DN_LD_HOST_INT_MASK		(0x2U)
/** Enable Host interrupt mask */
#define HIM_ENABLE			(UP_LD_HOST_INT_MASK | DN_LD_HOST_INT_MASK)
/** Disable Host interrupt mask */
#define	HIM_DISABLE			0xff

/** Host Control Registers : Host interrupt status */
#define HOST_INTSTATUS_REG		0x05
/** Host Control Registers : Upload host interrupt status */
#define UP_LD_HOST_INT_STATUS		(0x1U)
/** Host Control Registers : Download host interrupt status */
#define DN_LD_HOST_INT_STATUS		(0x2U)

/** Host F1 read base 0 */
#define HOST_F1_RD_BASE_0		0x10
/** Host F1 read base 1 */
#define HOST_F1_RD_BASE_1		0x11

/** Card Control Registers : Card status register */
#define CARD_STATUS_REG              	0x20
/** Card Control Registers : Card I/O ready */
#define CARD_IO_READY              	(0x1U << 3)
/** Card Control Registers : CIS card ready */
#define CIS_CARD_RDY                 	(0x1U << 2)
/** Card Control Registers : Upload card ready */
#define UP_LD_CARD_RDY               	(0x1U << 1)
/** Card Control Registers : Download card ready */
#define DN_LD_CARD_RDY               	(0x1U << 0)

/** Card Control Registers : Card OCR 0 register */
#define CARD_OCR_0_REG               	0x34
/** Card Control Registers : Card OCR 1 register */
#define CARD_OCR_1_REG               	0x35

/** Firmware status 0 register */
#define CARD_FW_STATUS0_REG		0x34//0x40
/** Firmware status 1 register */
#define CARD_FW_STATUS1_REG		0x35//0x41
/** Rx length register */
#define CARD_RX_LEN_REG			0x80fd//0x42
/** Rx unit register */
#define CARD_RX_UNIT_REG		0x80fc//0x43

/** Chip Id Register 0 */
#define CARD_CHIP_ID_0_REG		0x801c
/** Chip Id Register 1 */
#define CARD_CHIP_ID_1_REG		0x801d

#ifdef PXA3XX_DMA_ALIGN
/** DMA alignment value for PXA3XX platforms */
#define PXA3XX_DMA_ALIGNMENT    8
/** Macros for Data Alignment : size */
#define ALIGN_SZ(p, a)  \
    (((p) + ((a) - 1)) & ~((a) - 1))

/** Macros for Data Alignment : address */
#define ALIGN_ADDR(p, a)    \
    ((((u32)(p)) + (((u32)(a)) - 1)) & ~(((u32)(a)) - 1))
#endif /* PXA3XX_DMA_ALIGN */

struct sdio_mmc_card
{
        /** sdio_func structure pointer */
    struct sdio_func *func;
        /** uap_private structure pointer */
    uap_private *priv;
};

#endif /* _UAP_SDIO_MMC_H */
