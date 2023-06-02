/*
 * arch/arm/plat-ambarella/include/plat/audio.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_AUDIO_H__
#define __PLAT_AMBARELLA_AUDIO_H__

/* ==========================================================================*/
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define I2S_OFFSET			0x1A000
#else
#define I2S_OFFSET			0xA000
#endif
#define I2S_BASE			(AHB_BASE + I2S_OFFSET)
#define I2S_BASE_PHYS 			(AHB_PHYS_BASE + I2S_OFFSET)
#define I2S_REG(x)			(I2S_BASE + (x))
#define I2S_REG_PHYS(x)  		(I2S_BASE_PHYS + (x))

/* ==========================================================================*/
#define I2S_MODE_OFFSET				0x00
#define I2S_RX_CTRL_OFFSET			0x04
#define I2S_TX_CTRL_OFFSET			0x08
#define I2S_WLEN_OFFSET				0x0c
#define I2S_WPOS_OFFSET				0x10
#define I2S_SLOT_OFFSET				0x14
#define I2S_TX_FIFO_LTH_OFFSET			0x18
#define I2S_RX_FIFO_GTH_OFFSET			0x1c
#define I2S_CLOCK_OFFSET			0x20
#define I2S_INIT_OFFSET				0x24
#define I2S_TX_STATUS_OFFSET			0x28
#define I2S_TX_LEFT_DATA_OFFSET			0x2c
#define I2S_TX_RIGHT_DATA_OFFSET		0x30
#define I2S_RX_STATUS_OFFSET			0x34
#define I2S_RX_DATA_OFFSET			0x38
#define I2S_TX_FIFO_CNTR_OFFSET			0x3c
#define I2S_RX_FIFO_CNTR_OFFSET			0x40
#define I2S_TX_INT_ENABLE_OFFSET		0x44
#define I2S_RX_INT_ENABLE_OFFSET		0x48
#define I2S_RX_ECHO_OFFSET			0x4c
#define I2S_24BITMUX_MODE_OFFSET		0x50
#define I2S_GATEOFF_OFFSET			0x54
#define I2S_CHANNEL_SELECT_OFFSET		0x58
#define I2S_RX_DATA_DMA_OFFSET			0x80
#define I2S_TX_LEFT_DATA_DMA_OFFSET		0xc0

#define I2S_MODE_REG				I2S_REG(0x00)
#define I2S_RX_CTRL_REG				I2S_REG(0x04)
#define I2S_TX_CTRL_REG				I2S_REG(0x08)
#define I2S_WLEN_REG				I2S_REG(0x0c)
#define I2S_WPOS_REG				I2S_REG(0x10)
#define I2S_SLOT_REG				I2S_REG(0x14)
#define I2S_TX_FIFO_LTH_REG			I2S_REG(0x18)
#define I2S_RX_FIFO_GTH_REG			I2S_REG(0x1c)
#define I2S_CLOCK_REG				I2S_REG(0x20)
#define I2S_INIT_REG				I2S_REG(0x24)
#define I2S_TX_STATUS_REG			I2S_REG(0x28)
#define I2S_TX_LEFT_DATA_REG			I2S_REG(0x2c)
#define I2S_TX_RIGHT_DATA_REG			I2S_REG(0x30)
#define I2S_RX_STATUS_REG			I2S_REG(0x34)
#define I2S_RX_DATA_REG				I2S_REG(0x38)
#define I2S_TX_FIFO_CNTR_REG			I2S_REG(0x3c)
#define I2S_RX_FIFO_CNTR_REG			I2S_REG(0x40)
#define I2S_TX_INT_ENABLE_REG			I2S_REG(0x44)
#define I2S_RX_INT_ENABLE_REG			I2S_REG(0x48)
#define I2S_RX_ECHO_REG				I2S_REG(0x4c)
#define I2S_24BITMUX_MODE_REG			I2S_REG(0x50)
#define I2S_GATEOFF_REG				I2S_REG(0x54)
#define I2S_CHANNEL_SELECT_REG			I2S_REG(0x58)
#define I2S_RX_DATA_DMA_REG			I2S_REG_PHYS(0x80)
#define I2S_TX_LEFT_DATA_DMA_REG		I2S_REG_PHYS(0xc0)

#define I2S_LEFT_JUSTIFIED_MODE			0x0
#define I2S_RIGHT_JUSTIFIED_MODE		0x1
#define I2S_MSB_EXTEND_MODE			0x2
#define I2S_I2S_MODE				0x4
#define I2S_DSP_MODE				0x6

#define I2S_TX_FIFO_RESET_BIT			(1 << 4)
#define I2S_RX_FIFO_RESET_BIT			(1 << 3)
#define I2S_TX_ENABLE_BIT			(1 << 2)
#define I2S_RX_ENABLE_BIT			(1 << 1)
#define I2S_FIFO_RESET_BIT			(1 << 0)

#define I2S_RX_LOOPBACK_BIT			(1 << 3)
#define I2S_RX_ORDER_BIT			(1 << 2)
#define I2S_RX_WS_MST_BIT			(1 << 1)
#define I2S_RX_WS_INV_BIT			(1 << 0)

#define I2S_TX_LOOPBACK_BIT			(1 << 7)
#define I2S_TX_ORDER_BIT			(1 << 6)
#define I2S_TX_WS_MST_BIT			(1 << 5)
#define I2S_TX_WS_INV_BIT			(1 << 4)
#define I2S_TX_UNISON_BIT			(1 << 3)
#define I2S_TX_MUTE_BIT				(1 << 2)
#define I2S_TX_MONO_MASK			0xfffffffc
#define I2S_TX_MONO_RIGHT			(1 << 1)
#define I2S_TX_MONO_LEFT			(1 << 0)

#define I2S_CLK_WS_OUT_EN			(1 << 9)
#define I2S_CLK_BCLK_OUT_EN			(1 << 8)
#define I2S_CLK_BCLK_OUTPUT			(1 << 7)
#define I2S_CLK_MASTER_MODE			(I2S_CLK_WS_OUT_EN	|	\
						 I2S_CLK_BCLK_OUT_EN	|	\
						 I2S_CLK_BCLK_OUTPUT)
#define I2S_CLK_TX_PO_FALL			(1 << 6)
#define I2S_CLK_RX_PO_FALL			(1 << 5)
#define I2S_CLK_DIV_MASK			0xffffffe0

#define I2S_RX_SHIFT_ENB			(1 << 1)
#define I2S_TX_SHIFT_ENB			(1 << 0)

#define I2S_2CHANNELS_ENB			0x00
#define I2S_4CHANNELS_ENB			0x01
#define I2S_6CHANNELS_ENB			0x02

#define I2S_FIFO_THRESHOLD_INTRPT		0x08
#define I2S_FIFO_FULL_INTRPT			0x02
#define I2S_FIFO_EMPTY_INTRPT			0x01

/* I2S_24BITMUX_MODE_REG */
#define I2S_24BITMUX_MODE_ENABLE		0x1
#define I2S_24BITMUX_MODE_FDMA_BURST_DIS	0x2
#define I2S_24BITMUX_MODE_RST_CHAN0		0x4
#define I2S_24BITMUX_MODE_DMA_BOOTSEL		0x8

/* ==========================================================================*/
#define DAI_CLOCK_MASK				0x0000001f

#define MAX_MCLK_IDX_NUM			15

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct notifier_block;

struct ambarella_i2s_interface {
	u32 state;
	u32 mode;
	u32 clksrc;
	u32 mclk;
	u32 div;
	u32 sfreq;
	u32 channels;
	u32 word_len;
	u32 word_pos;
	u32 slots;
	u32 rx_ctrl;
	u32 tx_ctrl;
	u32 rx_fifo_len;
	u32 tx_fifo_len;
	u32 multi24;
};

enum Audio_Notify_Type
{
	AUDIO_NOTIFY_UNKNOWN,
	AUDIO_NOTIFY_INIT,
	AUDIO_NOTIFY_SETHWPARAMS,
	AUDIO_NOTIFY_REMOVE
};

#define AMBARELLA_CLKSRC_ONCHIP			0x0
#define AMBARELLA_CLKSRC_SP_CLK			0x1
#define AMBARELLA_CLKSRC_CLK_SI			0x2
#define AMBARELLA_CLKSRC_EXTERNAL		0x3
#define AMBARELLA_CLKSRC_LVDS_IDSP_SCLK		0x4

#define AMBARELLA_CLKDIV_LRCLK			0

/* ==========================================================================*/
extern int ambarella_init_audio(void);

extern void ambarella_audio_notify_transition(
	struct ambarella_i2s_interface *data, unsigned int type);
extern int ambarella_audio_register_notifier(struct notifier_block *nb);
extern int ambarella_audio_unregister_notifier(struct notifier_block *nb);

extern struct ambarella_i2s_interface get_audio_i2s_interface(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_AUDIO_H__ */

