/*
 * sound/soc/ambarella_pcm.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2008/08/07 - [Cao Rongrong] Fix the buffer bug,eg: size and allocation
 *	2008/11/14 - [Cao Rongrong] Support pause and resume
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2009/03/05 - [Cao Rongrong] Update from 2.6.22.10
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2009/06/30 - [Cao Rongrong] Fix last_desc bug
 *	2010/10/25 - [Cao Rongrong] Port to 2.6.36+
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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

#include <linux/module.h>
#include <linux/init.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <plat/dma.h>


#define AMBA_MAX_DESC_NUM		128
#define AMBA_MIN_DESC_NUM		2
#define AMBA_PERIOD_BYTES_MAX		(128 * 1024)
#define AMBA_PERIOD_BYTES_MIN		32
#define AMBA_BUFFER_BYTES_MAX		(256 * 1024)

/*
 * Note that we specify SNDRV_PCM_INFO_JOINT_DUPLEX here, but only because a
 * limitation in the I2S driver requires the sample rates for playback and
 * capture to be the same.
 */
static const struct snd_pcm_hardware ambarella_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME |
				  SNDRV_PCM_INFO_BATCH |
				  SNDRV_PCM_INFO_JOINT_DUPLEX,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,
	.rates			= SNDRV_PCM_RATE_8000_48000,
	.rate_min		= 8000,
	.rate_max		= 48000,
	.period_bytes_min	= AMBA_PERIOD_BYTES_MIN,
	.period_bytes_max	= AMBA_PERIOD_BYTES_MAX,
	.periods_min		= AMBA_MIN_DESC_NUM,
	.periods_max		= AMBA_MAX_DESC_NUM,
	.buffer_bytes_max	= AMBA_BUFFER_BYTES_MAX,
};


static const struct snd_dmaengine_pcm_config ambarella_dmaengine_pcm_config = {
	.pcm_hardware = &ambarella_pcm_hardware,
	.prepare_slave_config = snd_dmaengine_pcm_prepare_slave_config,
	.prealloc_buffer_size = AMBA_BUFFER_BYTES_MAX,
};

int ambarella_pcm_platform_register(struct device *dev)
{
	return snd_dmaengine_pcm_register(dev, &ambarella_dmaengine_pcm_config,
			SND_DMAENGINE_PCM_FLAG_NO_RESIDUE |
			SND_DMAENGINE_PCM_FLAG_COMPAT);
}
EXPORT_SYMBOL_GPL(ambarella_pcm_platform_register);

void ambarella_pcm_platform_unregister(struct device *dev)
{
	snd_dmaengine_pcm_unregister(dev);
}
EXPORT_SYMBOL_GPL(ambarella_pcm_platform_unregister);


MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc PCM DMA module");
MODULE_LICENSE("GPL");

