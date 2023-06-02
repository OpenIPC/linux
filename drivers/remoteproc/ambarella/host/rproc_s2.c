/*
 * Author: Tzu-Jung Lee <tjlee@ambarella.com>
 *
 * Copyright (C) 2012-2012, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/platform_device.h>

#include <linux/slab.h>
#include <linux/virtio_ids.h>
#include <linux/rpmsg.h>
#include <linux/dma-mapping.h>

#include <plat/remoteproc.h>
#include <plat/remoteproc_cfg.h>

/*
 * The gen_rsc_table_* functions below fabricate the "resource table", which is
 * used to initialize the vring configurations by the RPROC subsystem.
 *
 * The tables were originally meant to be loaded from ELF files on filesystem.
 * It's a flexible and generic design, but requires extra maintenance out side
 * the kernel space. So we currently code it plainly and directly here.
 *
 * The following illustrate the layout and structure of the table we use.
 * Watch out the "sizeof()", since most of the structures here are embedded with
 * variable-length array.
 *
 *  0x0000  &table
 *
 *          < sizeof(*table) >
 *          < sizeof(u32) * table->num >
 *
 *  0x0014  &hdr (&table->offset)
 *
 *          < sizeof(*hdr) >
 *
 *  0x0018  &vdev (&hdr->data)
 *
 *          < sizeof(*vdev) >
 *
 *  0x0030  &vdev->ring[0] (&hdr->data)
 *
 *          < sizeof(*vring) >
 *
 *  0x0044  &vdev->ring[1];
 *
 *         < sizeof(*vring) >
 *  0x0058
 */

static struct resource_table *gen_rsc_table_cortex(int *tablesz)
{
	struct resource_table		*table;
	struct fw_rsc_hdr		    *hdr;
	struct fw_rsc_vdev		    *vdev;
	struct fw_rsc_vdev_vring	*vring;

	*tablesz		= sizeof(*table) + sizeof(u32) * 1
				    + sizeof(*hdr) + sizeof(*vdev)
				    + sizeof(*vring) * 2;

	table			    = kzalloc((*tablesz), GFP_KERNEL);
	table->ver		    = 1;
	table->num		    = 1;
	table->offset[0]    = sizeof(*table) + sizeof(u32) * table->num;

	hdr			        = (void*)table + table->offset[0];
	hdr->type		    = RSC_VDEV;
	vdev			    = (void*)&hdr->data;
	vdev->id		    = VIRTIO_ID_RPMSG;
	vdev->notifyid	    = 124;
	vdev->dfeatures	    = (1 << VIRTIO_RPMSG_F_NS);
	vdev->config_len	= 0;
	vdev->num_of_vrings	= 2;

	vring			    = (void*)&vdev->vring[0];
	vring->da		    = VRING_C0_TO_C1;
	vring->align		= PAGE_SIZE;
	vring->num		    = RPMSG_NUM_BUFS >> 1;
	vring->notifyid		= 111;

	vring			    = (void*)&vdev->vring[1];
	vring->da		    = VRING_C1_TO_C0;
	vring->align		= PAGE_SIZE;
	vring->num		    = RPMSG_NUM_BUFS >> 1;
	vring->notifyid		= 112;

	return table;
}

static struct ambarella_rproc_pdata pdata_cortex = {
	.name			= "c0_and_c1",
	.firmware		= "dummy",
	.svq_tx_irq		= VRING_IRQ_C1_TO_C0_KICK,  //108
	.svq_rx_irq		= VRING_IRQ_C0_TO_C1_ACK,   //107
	.rvq_tx_irq		= VRING_IRQ_C1_TO_C0_ACK,   //109
	.rvq_rx_irq		= VRING_IRQ_C0_TO_C1_KICK,  //106
	.ops			= NULL,
	.buf_addr_pa	= VRING_C0_AND_C1_BUF,
	.gen_rsc_table	= gen_rsc_table_cortex,
};

struct platform_device ambarella_rproc_cortex_dev = {
	.name			= "c0_and_c1",
	.id			    = VIRTIO_ID_RPMSG,
	.resource		= NULL,
	.num_resources	= 0,
	.dev			= {
		.platform_data		= &pdata_cortex,
		.dma_mask		    = &ambarella_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
};
