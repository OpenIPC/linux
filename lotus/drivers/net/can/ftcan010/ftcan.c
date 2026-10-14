// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/can/dev.h>
#include <linux/can/error.h>
#include <linux/can/led.h>
#include <linux/pm_runtime.h>

#ifndef CONFIG_FTCAN_RETRANSMISSION_TIMES
#define CONFIG_FTCAN_RETRANSMISSION_TIMES 2
#endif /* CONFIG_FTCAN_RETRANSMISSION_TIMES */

#ifndef CONFIG_FTCAN_TX_BUFFER_NUM
#define CONFIG_FTCAN_TX_BUFFER_NUM 1
#endif /* CONFIG_FTCAN_TX_BUFFER_NUM */

#ifndef CONFIG_FTCAN_RX_NAPI_SUPPORT
#define CONFIG_FTCAN_RX_NAPI_SUPPORT
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */

#include "ftcan.h"

/* When get timming, consider 99M as 100M to calculate */
u32 gcan0_clk = FTCAN_CLK_99M; // 100MHz
module_param(gcan0_clk, uint, 0444);
MODULE_PARM_DESC(gcan0_clk, "choose can0 clk: 24M or 100M");
u32 gcan1_clk = FTCAN_CLK_99M; // 100MHz
module_param(gcan1_clk, uint, 0444);
MODULE_PARM_DESC(gcan1_clk, "choose can1 clk: 24M or 100M");

/* CAN Arbitration Bittiming constants */
static const struct can_bittiming_const ftcan_nominal_bittiming_const = {
	.name = DRIVER_NAME,
	/* Time segment 1 = prop_seg + phase_seg1.
	 * prop_seg   min 1; max 64.
	 * phase_seg1 min 1; max 32.
	 */
	.tseg1_min = 2,
	.tseg1_max = 96,
	.tseg2_min = 2,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 32,
	.sjw_max = 32,		/* Synchronisation jump width */
	.brp_min = 2,		/* Bit-rate prescaler */
	.brp_max = 256,
	.brp_inc = 1,
};

/* CANFD 2.0 Data Bittiming constants */
static const struct can_bittiming_const ftcan_data_bittiming_const_canfd = {
	.name = DRIVER_NAME,
	/* Time segment 1 = prop_seg + phase_seg1.
	 * prop_seg   min 0; max 8.
	 * phase_seg1 min 1; max 8
	 */
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 2,		/* Time segment 2 = phase_seg2 */
	.tseg2_max = 8,
	.sjw_max = 4,		/* Synchronisation jump width */
	.brp_min = 2,		/* Bit-rate prescaler */
	.brp_max = 256,
	.brp_inc = 1,
};

static inline void ftcan_write_reg(void __iomem *addr, u32 val)
{
	iowrite32(val, addr);
}

static inline u32 ftcan_read_reg(void __iomem *addr)
{
	return ioread32(addr);
}

static inline void ftcan_clear_bits(void __iomem *addr, u32 mask)
{
	ftcan_write_reg(addr, ftcan_read_reg(addr) & ~mask);
}

static inline void ftcan_set_bits(void __iomem *addr, u32 mask)
{
	ftcan_write_reg(addr, ftcan_read_reg(addr) | mask);
}

static inline u32 ftcan_read_bits(void __iomem *addr, u32 mask)
{
	return ftcan_read_reg(addr) & mask;
}

static inline void ftcan_write_bits(void __iomem *addr, u32 value, u32 mask)
{
	u32 old = ftcan_read_reg(addr);

	ftcan_write_reg(addr, (old & ~mask) | (value & mask));
}

static inline void ftcan_clear_reg(void __iomem *addr, u32 c, size_t count)
{
	memset_io(addr, c, count);
}

static s32 ftcan_clk_disable(struct net_device *ndev)
{
	void __iomem *addr;

	addr = ioremap(CRG_ADDR_BASE, CRG_OFFESET_SIZE);
	if (unlikely(!addr))
		return -ENXIO;

	if (!memcmp(netdev_name(ndev), "can0", 4))
		/* disable can0 clk */
		ftcan_clear_bits(addr, 0x04000000);
	else
		/* disable can1 clk */
		ftcan_clear_bits(addr, 0x20000000);

	iounmap(addr);

	return 0;
}

static s32 ftcan_clk_enable(struct net_device *ndev)
{
	void __iomem *addr;

	addr = ioremap(CRG_ADDR_BASE, CRG_OFFESET_SIZE);
	if (unlikely(!addr))
		return -ENXIO;

	if (!memcmp(netdev_name(ndev), "can0", 4)) {
		if (gcan0_clk == FTCAN_CLK_24M) {
			/* select 24M clk and enable can0 clk */
			ftcan_clear_bits(addr, 0x10000000);
			ftcan_set_bits(addr, 0x04000000);

		} else
			/* select 99M clk and enable can0 clk */
			ftcan_set_bits(addr, 0x14000000);
	} else {
		if (gcan1_clk == FTCAN_CLK_24M) {
			/* select 24M clk and enable can1 clk */
			ftcan_clear_bits(addr, 0x80000000);
			ftcan_set_bits(addr, 0x20000000);
		} else
			/* select 99M clk and enable can1 clk */
			ftcan_set_bits(addr, 0xa0000000);
	}

	iounmap(addr);

	return 0;
}

static s32 ftcan_clk_init(void)
{
	void __iomem *addr;

	addr = ioremap(CRG_ADDR_BASE, CRG_OFFESET_SIZE);
	if (unlikely(!addr))
		return -ENXIO;

	/* can clk config, including:
	 * bit-30: can1_cksel, 0-24M 1-100M
	 * bit-29: can1_srst_req, 0-undo 1-do
	 * bit-28: can1_cken, 0-disable 1-enable
	 * bit-27: can0_cksel, 0-24M 1-100M
	 * bit-26: can0_srst_req, 0-undo 1-do
	 * bit-25: can0_cken, 0-disable 1-enable
	 */
	ftcan_set_bits(addr, 0x6c000000);

	udelay(2000);

	/* undo can0_srst_req and can1_srst_req */
	ftcan_clear_bits(addr, 0x48000000);

	iounmap(addr);

	return 0;
}

static s32 ftcan_clk_exit(void)
{
	void __iomem *addr;

	addr = ioremap(CRG_ADDR_BASE, CRG_OFFESET_SIZE);
	if (unlikely(!addr))
		return -ENXIO;

	/* can clk disable and undo reset, including:
	 * bit-30: can1_cksel, 0-24M 1-100M
	 * bit-29: can1_srst_req, 0-undo 1-do
	 * bit-28: can1_cken, 0-disable 1-enable
	 * bit-27: can0_cksel, 0-24M 1-100M
	 * bit-26: can0_srst_req, 0-undo 1-do
	 * bit-25: can0_cken, 0-disable 1-enable
	 */
	ftcan_clear_bits(addr, 0xfc000000);

	udelay(2000);

	iounmap(addr);

	return 0;
}

/*
 * ftcan_mode_change - Let the CAN enter the user-specific mode
 * @regs:	can reg base
 * @mode:	can mode
 *	CAN_MODE_CONFIG,
 *	CAN_MODE_NORMAL,
 *	CAN_MODE_SlEEP,
 *	CAN_MODE_LISTEN,
 *	CAN_MODE_LOOPBACK
 *
 * Change the mode by setting the OMR field in CE0 register.
 *
 * Return: 0 on success and failure value on error
 */

s32 ftcan_mode_change(struct can_regs *regs, u32 mode)
{
	u32 i;

	for (i = 0; i < LOTUS_CAN_SET_MODE_RETRIES; i++) {
		/* Set the mode in OMR field of CE1 register */
		ftcan_write_bits(&regs->can_cantrol.all, mode, CAN_CE1_OMR_W);

		udelay(100);
		if (ftcan_read_bits(&regs->can_cantrol.all, CAN_CE1_OMR_W) == mode)
			break;
	}

	if (ftcan_read_bits(&regs->can_cantrol.all, CAN_CE1_OMR_W) != mode)
		return -ETIMEDOUT;

	return 0;
}

/*
 * ftcan_get_ir_status - get the IR status.
 * @regs:	can reg base
 *
 * Return IR register 8-bit value
 */

u8 ftcan_get_ir_status(struct can_regs *regs)
{
	return (u8)ftcan_read_reg(&regs->can_interrupt.all);
}

/*
 * ftcan_get_curr_transmit_status - Get the current transmit status from
 *					TS register.
 * @regs:	can reg base
 *
 * Return last error buffer bit
 */

u8 ftcan_get_curr_transmit_status(struct can_regs *regs)
{
	u32 reg_value;

	reg_value = ftcan_read_bits(&regs->can_status.all, CAN_TS_MASK_W);

	return (u8)(reg_value >> CAN_TS_SHIFT);
}

/*
 * ftcan_get_laset_err_buffer - Get the last error buffer from TED register.
 * @regs:	can reg base
 *
 * Return last error buffer bit
 */

u8 ftcan_get_laset_err_buffer(struct can_regs *regs)
{
	u32 reg_value;

	reg_value = ftcan_read_bits(&regs->can_error_monitor.status.all,
				    CAN_TED_MSAK_W);

	return (u8)(reg_value >> CAN_TED_SHIFT);
}

/*
 * ftcan_get_laset_err_code - Get the last error code from ET register.
 * @regs:	can reg base
 *
 * Return last error code
 */

u8 ftcan_get_laset_err_code(struct can_regs *regs)
{
	u32 reg_value;

	reg_value = ftcan_read_bits(&regs->can_error_monitor.status.all,
				    CAN_ET_MASK_W);

	return (u8)reg_value;
}

/**
 * ftcan_clear_it - Clear a specific bit in IR reigster
 * @regs:		can reg base
 * @can_status_bit:	status bit value
 *
 * The attribute of this bit is RW1C. Can_status_bit is only one of
 * below parameter:
 *	CAN_WIR_STATUS_BIT
 *	CAN_OIR_STATUS_BIT
 *	CAN_EIR_STATUS_BIT
 *	CAN_TBI0_STATUS_BIT
 *	CAN_TBI1_STATUS_BIT
 *	CAN_TBI2_STATUS_BIT
 *	CAN_RBI0_STATUS_BIT
 *	CAN_RBI1_STATUS_BIT
 *
 * Use the can_status_bit bit[4:2] to get the target bit.
 * And write one to clear this stauts bit.
 */

void ftcan_clear_it(struct can_regs *regs, u8 can_status_bit)
{
	u8 reg_bit;

	reg_bit = (can_status_bit & 0x1C) >> 2;
	ftcan_write_reg(&regs->can_interrupt.all, (1 << reg_bit));
}

/**
 * ftcan_get_tec - Get the TEC value from TECH and TECL registers.
 * @ regs: can reg base
 * @ regs: tec pointer. return the current can tec value
 * @ regs: rec pointer. return the current can rec value
 */

void ftcan_get_tec_rec(struct can_regs *regs, u16 *tec, u16 *rec)
{
	u32 reg_value;

	reg_value = ftcan_read_reg(&regs->can_error_monitor.counter.all);
	*tec = (u16)(reg_value & 0x0000FFFF);
	*rec = (u16)((reg_value & 0xFFFF0000) >> 16);
}

/**
 * ftcan_get_stat_bit - Input the can_status_bit. This function can return
 *                    the stauts bit or field across three registers (TS, TRBS,
 *                    and BS). can_status_bit is contained the register, bit
 *                    number, and  field width about this status information.
 * @ regs: can reg base
 * @ can_status_bit is consist of 8-bit:
 *	 X X │ X X X │ X X │
 *	       │      │      └──┬ 0:TS register
 *		   │      │         │ 1:TRBS register
 *		   │      │         │ 2:BS register
 *		   │      │         └ 3:IR register
 *		   │      └─┬ 0: bit[0]
 *		   │        │ 1: bit[1]
 *		   │        │ ...
 *		   │        └ 7: bit[7]
 *		   └─┬ If this status is field type:
 *		     │ 0: 1-bit field, mask = 0x1
 *			 │ 1: 2-bit field, mask = 0x3
 *			 └ 2,3: Reserved
 * @ value: Status bit/field value
 *
 * Procedure:
 *	1. Parsing can_status_bit bit[1:0]to get this status from which register
 *	2. Parsing can_status_bit bit[4:2]to get this status from this resgister
 *	   which number bit.
 *	3. Parsing can_status_bit bit[6:5]to get this status how many bit fields
 *	4. return this status bit/field.
 *
 * Comment:
 *	This function only watch a status in a time, so can_status_bit cannot be
 *	'or' operated more can_status_bit.
 */

void ftcan_get_stat_bit(struct can_regs *regs, u8 can_status_bit, u8 *value)
{
	u8 reg, reg_bit, reg_field;
	/* 0表示field=1 bit, 1表示field=2 bits, 2表示field=3 bits... */
	u8 mask_table[4] = {0x1, 0x3, 0x7, 0xF};
	u32 reg_value;

	/* Get the status bit info from parameter can_status_bit */
	reg = can_status_bit & 0x3;
	reg_bit = (can_status_bit & 0x1C) >> 2;
	reg_field = ((can_status_bit & 0x60) >> 5);

	if (reg == 0) {
		reg_value = ftcan_read_reg(&regs->can_status.all);
		*value = (u8)((((reg_value >> CAN_TS_SHIFT) & 0xFF)
			 >> reg_bit) & mask_table[reg_field]);
	} else if (reg == 1) {
		reg_value = ftcan_read_reg(&regs->can_status.all);
		*value = (u8)((((reg_value >> CAN_TRBS_SHIFT) & 0xFF)
			 >> reg_bit) & mask_table[reg_field]);
	} else if (reg == 2) {
		reg_value = ftcan_read_reg(&regs->can_status.all);
		*value = (u8)(((reg_value & 0xFF)
			 >> reg_bit) & mask_table[reg_field]);
	} else if (reg == 3) {
		reg_value = ftcan_read_reg(&regs->can_interrupt.all);
		*value = (u8)(((reg_value & 0xFF)
			 >> reg_bit) & mask_table[reg_field]);
	}
}

/**
 * ftcan_reset - Resets the CAN device mode
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver reset mode routine.The driver
 * enters into configuration mode.
 *
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_reset(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	u32 i;

	/* clear isr */
	ftcan_clear_bits(&regs->can_cantrol.all, CAN_IR_MASK_ALL_W);

	/* Enable RR bit */
	ftcan_set_bits(&regs->can_cantrol.all, CAN_CE1_RR_W);

	for (i = 0; i < LOTUS_CAN_SET_MODE_RETRIES; i++) {
		if (!ftcan_read_bits(&regs->can_cantrol.all, CAN_CE1_RR_W))
			break;
		udelay(100);
	}

	if (ftcan_read_bits(&regs->can_cantrol.all, CAN_CE1_RR_W)) {
		netdev_warn(ndev, "timed out for config mode\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/**
 * ftcan_set_bittiming - CAN set bit timing routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver set bittiming  routine.
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_set_bittiming(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	struct can_bittiming *bt = &priv->can.bittiming;
	struct can_bittiming *dbt = &priv->can.data_bittiming;
	u32 reg_value;

	netdev_dbg(ndev, "nominal bit timing:\n");
	netdev_dbg(ndev, "   bitrate      = %u\n", bt->bitrate);
	netdev_dbg(ndev, "   sample_point = %u\n", bt->sample_point);
	netdev_dbg(ndev, "   tq           = %u\n", bt->tq);
	netdev_dbg(ndev, "   sjw          = %u\n", bt->sjw);
	netdev_dbg(ndev, "   brp          = %u\n", bt->brp);
	netdev_dbg(ndev, "   prop_seg     = %u\n", bt->prop_seg);
	netdev_dbg(ndev, "   phase_seg1   = %u\n", bt->phase_seg1);
	netdev_dbg(ndev, "   phase_seg2   = %u\n", bt->phase_seg2);
	netdev_dbg(ndev, "data    bit timing:\n");
	netdev_dbg(ndev, "   bitrate      = %u\n", dbt->bitrate);
	netdev_dbg(ndev, "   sample_point = %u\n", dbt->sample_point);
	netdev_dbg(ndev, "   tq           = %u\n", dbt->tq);
	netdev_dbg(ndev, "   sjw          = %u\n", dbt->sjw);
	netdev_dbg(ndev, "   brp          = %u\n", dbt->brp);
	netdev_dbg(ndev, "   prop_seg     = %u\n", dbt->prop_seg);
	netdev_dbg(ndev, "   phase_seg1   = %u\n", dbt->phase_seg1);
	netdev_dbg(ndev, "   phase_seg2   = %u\n", dbt->phase_seg2);

	/* reset nominal and data bit timing configuration*/
	ftcan_write_reg(&regs->can_bit_timing.nominal_l.all, 0);
	ftcan_write_reg(&regs->can_bit_timing.nominal_h.all, 0);
	ftcan_write_reg(&regs->can_bit_timing.data.all, 0);

	/* Set nominal bit timing */
	reg_value = (((bt->sjw  - 1) << CAN_NSJW_SHIFT) |
		    ((bt->brp  - 1) << CAN_NBRP_SHIFT) |
		    ((bt->prop_seg - 1) << CAN_NPROP_SHIFT) |
		    ((bt->phase_seg1  - 1) << CAN_NPS1_SHIFT));
	ftcan_write_reg(&regs->can_bit_timing.nominal_l.all, reg_value);


	reg_value = (bt->phase_seg2 - 1) << CAN_NPS2_SHIFT;
	ftcan_write_reg(&regs->can_bit_timing.nominal_h.all, reg_value);

	/* Set data bit timing if in canFD mode */
	if (priv->can.ctrlmode & CAN_CTRLMODE_FD) {
		/* Setting Baud Rate prescalar value in F_BRPR Register */
		reg_value = ((dbt->brp - 1) << CAN_DBRP_SHIFT |
			    ((dbt->sjw - 1) << CAN_DSJW_SHIFT) |
			    ((dbt->prop_seg << CAN_DPROP_SHIFT)) |
			    ((dbt->phase_seg1 - 1) << CAN_DPS1_SHIFT) |
			    ((dbt->phase_seg2 - 1) << CAN_DPS2_SHIFT));
		ftcan_write_reg(&regs->can_bit_timing.data.all, reg_value);
	} else
		ftcan_write_reg(&regs->can_bit_timing.data.all, 0);

	return 0;
}

int ftcan_set_data_bittiming(struct net_device *dev)
{
	/**
	 * In linux, this function must be registered in order to set data
	 * bittimg. We set nominal and data bittiming in function
	 * ftcan_chip_start when can interface is up.
	 */
	return 0;
}

/**
 * ftcan_chip_start - This the drivers start routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the drivers start routine.
 * Based on the State of the CAN device it puts
 * the CAN device into a proper mode.
 *
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_chip_start(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	s32 err;
	u32 retrans_times = priv->retransmit_times << 27;

	/* Check if it is in reset mode */
	err = ftcan_reset(ndev);
	if (err < 0)
		return -EPERM;

	err = ftcan_mode_change(regs, CAN_MODE_CONFIG);
	if (err < 0) {
		netdev_err(ndev, "could not enter configuration mode\n");
		return -EPERM;
	}

	ftcan_set_bits(&regs->can_cantrol.all, (CAN_CR_RRBA0_W | CAN_CR_RRBA1_W));
	ftcan_set_bits(&regs->can_cantrol.all, CAN_IR_MASK_ALL_W);
#ifdef CONFIG_FTCAN_SUPPORT_OVERLOAD_FRAME
	/* Set the overload feature */
	ftcan_set_bits(&regs->can_cantrol.all, CAN_CE1_TOE_W);
#endif /* CONFIG_FTCAN_SUPPORT_OVERLOAD_FRAME*/
	/* Set the debounced feature */
	ftcan_set_bits(&regs->can_cantrol.all, CAN_CE0_ENDB_W);

	/* Set the number of retransmission */
	ftcan_write_bits(&regs->can_cantrol.all, retrans_times, CAN_CE1_RT_W);

	err = ftcan_set_bittiming(ndev);
	if (err < 0)
		return err;

	/* Receive buffer0 acceptance filter group enable */
	ftcan_set_bits(&regs->can_filter.control.all, CAN_AFC0_RBAFG0E_W);

#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	/* Receive buffer1 acceptance filter group enable */
	ftcan_set_bits(&regs->can_filter.control.all, CAN_AFC0_RBAFG1E_W);
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */

	/* all messages can pass the filter */
	ftcan_set_bits(&regs->can_mask.control.all, CAN_FMC_FMA_W);

	/* Check whether it is loopback mode or normal mode  */
	if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK) {
		err = ftcan_mode_change(regs, CAN_MODE_LOOPBACK);
		if (err < 0) {
			netdev_err(ndev, "could not enter lookback mode\n");
			return -EPERM;
		}
	} else {
		err = ftcan_mode_change(regs, CAN_MODE_NORMAL);
		if (err < 0) {
			netdev_err(ndev, "could not enter normal mode\n");
			return -EPERM;
		}
	}

	priv->can.state = CAN_STATE_ERROR_ACTIVE;

	return 0;
}

/**
 * ftcan_chip_stop - Driver stop routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the drivers stop routine. It will disable the
 * interrupts and put the device into configuration mode.
 */

static void ftcan_chip_stop(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	s32 ret;

	/* Disable interrupts and leave the can in configuration mode */
	ret = ftcan_reset(ndev);
	if (ret < 0)
		netdev_warn(ndev, "ftcan_reset() Failed\n");


	ftcan_mode_change(priv->reg_base, CAN_MODE_CONFIG);
	ftcan_clear_bits(&regs->can_cantrol.all, CAN_CR_BTR_ALL_W);

	priv->can.state = CAN_STATE_STOPPED;
}

static void ftcan_clean(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	u32 i;

	for (i = 0; i < priv->tx_max; i++) {
		if (priv->tx_buffer[i]) {
			ndev->stats.tx_errors++;
			can_free_echo_skb(ndev, i);
			priv->tx_buffer[i] = NULL;
		}
	}
}

/**
 * ftcan_do_set_mode - This sets the mode of the driver
 * @ndev:	Pointer to net_device structure
 * @mode:	Tells the mode of the driver
 *
 * This check the drivers state and calls the corresponding modes to set.
 *
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_do_set_mode(struct net_device *ndev, enum can_mode mode)
{
	s32 ret;

	switch (mode) {
	case CAN_MODE_START:
		ftcan_clean(ndev);
		ret = ftcan_chip_start(ndev);
		if (ret < 0) {
			netdev_err(ndev, "ftcan_chip_start failed!\n");
			return ret;
		}
		netif_wake_queue(ndev);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

/**
 * ftcan_write_frame - Write a frame to HW
 * @ndev:		Pointer to net_device structure
 * @skb:		sk_buff pointer that contains data to be Txed
 * @tx_num:		tx buffer that write the frame to
 */

static void ftcan_write_frame(struct net_device *ndev, struct sk_buff *skb,
			      u8 tx_num)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	struct canfd_frame *cf = (struct canfd_frame *)skb->data;
	u32 id, dlc, i, rtr, j;
	u32 reg_tx_l = 0, reg_tx_h = 0;

	/**
	 * Clear the available Tx buffer register
	 * tx_l (TFD0, Offset: 0x08 ~ 0x0b)
	 * tx_h (0x0c ~0F)
	 */
	ftcan_write_reg(&regs->can_tr[tx_num].tx_l.all, 0);
	ftcan_write_reg(&regs->can_tr[tx_num].tx_h.all, 0);

	/* Clear the available Tx buffer data register */
	ftcan_clear_reg(&regs->can_td[tx_num].data[0].all, 0, 64);

	netdev_dbg(ndev, "TX can_id:%x len:%u flag:%x\n",
		   cf->can_id, cf->len, cf->flags);

	/* Watch carefully on the bit sequence */
	if (cf->can_id & CAN_EFF_FLAG) {
		/* Extended CAN ID format */
		id = cf->can_id & CAN_EFF_MASK;
		/* Standard id */
		reg_tx_h |= id >> (CAN_EFF_ID_BITS - CAN_SFF_ID_BITS) <<
			CAN_RBI_SHIFT;
		/* Extended id */
		reg_tx_h |= (id  & 0x3FC00) >> CAN_REIH_SHIFT;
		reg_tx_l |= (id & 0x3FF) << CAN_REIL_SHIFT;
		/* Use extended format for transmit request */
		reg_tx_l |= (u32)(0x1 << 14);
	} else {
		id = cf->can_id & CAN_SFF_MASK;
		reg_tx_h |= id << CAN_RBI_SHIFT;
	}

	/* Set the frame is the remote frame or not */
	rtr = cf->can_id & CAN_RTR_FLAG;
	if (rtr)
		reg_tx_l |= (u32)(0x1 << 15);

	if (can_is_canfd_skb(skb)) {
		/* If it is FD frame, then set the TEDL bit */
		reg_tx_l |= (u32)(0x1 << 7);
		/* If the data phase will change speed, set the TBRS bit */
		if (cf->flags & CANFD_BRS)
			reg_tx_l |= (u32)(0x1 << 6);
	}

	/* Set DLC field */
	dlc = can_len2dlc(cf->len);
	reg_tx_h |= (dlc & 0xF) << 8;

	ftcan_write_reg(&regs->can_tr[tx_num].tx_l.all, reg_tx_l);
	ftcan_write_reg(&regs->can_tr[tx_num].tx_h.all, reg_tx_h);

	/* put skb to echo[tx_num] */
	can_put_echo_skb(skb, ndev, tx_num);

	/* If the frame is not remote frame and data length > 0 bytes,
	 * set the message data value
	 */
	if (rtr != 1) {
		for (i = 0, j = 0; i < cf->len; i += sizeof(u32), j++) {
			ftcan_write_reg(&regs->can_td[tx_num].data[j].all,
					*(__be32 *)(cf->data + i));
		}
	}
}

/**
 * ftcan_get_available_tx_buff - get available tx buffer
 * @ndev:	Pointer to net_device structure
 *
 * Return: 0 ~ 2 is the tx buffer index, -1 if there is no space
 */
static s8 ftcan_get_available_tx_buff(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	u32 reg_value;

	reg_value = ftcan_read_bits(&regs->can_cantrol.all, CAN_CR_BTR_ALL_W);
	if (!(reg_value & CAN_CR_BTR0_W) && !priv->tx_buffer[0]) {
		return CAN_TXBUFFER_0;
	} else if (priv->tx_max >= 2 && !priv->tx_buffer[1] &&
		   !(reg_value & CAN_CR_BTR1_W)) {
		return CAN_TXBUFFER_1;
	} else if (priv->tx_max == 3 && !priv->tx_buffer[2] &&
		   !(reg_value & CAN_CR_BTR2_W)) {
		return CAN_TXBUFFER_2;
	}

	return -1;
}

/**
 * ftcan_start_xmit - Starts the transmission
 * @skb:	sk_buff pointer that contains data to be Txed
 * @ndev:	Pointer to net_device structure
 *
 * This function is invoked from upper layers to initiate transmission.
 *
 * Return: NETDEV_TX_OK on success and NETDEV_TX_BUSY when the tx queue is full
 */

static netdev_tx_t ftcan_start_xmit(struct sk_buff *skb,
				    struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	unsigned long flags;
	s8 tx_num;

	if (can_dropped_invalid_skb(ndev, skb))
		return NETDEV_TX_OK;

	spin_lock_irqsave(&priv->tx_lock, flags);

	tx_num = ftcan_get_available_tx_buff(ndev);
	if (tx_num < 0) {
		netif_stop_queue(ndev);
		spin_unlock_irqrestore(&priv->tx_lock, flags);
		netdev_err(ndev, "TX full when queue awake!\n");
		return NETDEV_TX_BUSY;
	}
	priv->tx_buffer[tx_num] = skb;

	if (ftcan_get_available_tx_buff(ndev) < 0)
		netif_stop_queue(ndev);

	ftcan_write_frame(ndev, skb, tx_num);

	/* Set the request bit of this Tx number */
	if (tx_num == CAN_TXBUFFER_0)
		ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_BTR0_W);
	else if (tx_num == CAN_TXBUFFER_1)
		ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_BTR1_W);
	else if (tx_num == CAN_TXBUFFER_2)
		ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_BTR2_W);

	spin_unlock_irqrestore(&priv->tx_lock, flags);

	netdev_dbg(ndev, "TX success! tx buffer:%u\n", tx_num);

	return NETDEV_TX_OK;
}

/**
 * ftcan_current_error_state - get can current error state
 * @ndev:	Pointer to net_device structure
 * @rx_state:	The CAN state of rx
 * @tx_state:	The CAN state of tx
 *
 * Get can current error status, include tx and rx state.
 */

static enum can_state ftcan_current_error_state(struct net_device *ndev,
						enum can_state *tx_state,
						enum can_state *rx_state)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	enum can_state new_state;
	u16 rec, tec;

	ftcan_get_tec_rec(regs, &tec, &rec);
	if (tec > 255)
		new_state = CAN_STATE_BUS_OFF;
	else if (rec >= 128 || tec >= 128)
		new_state = CAN_STATE_ERROR_PASSIVE;
	else if (rec >= 96 || tec >= 96)
		new_state = CAN_STATE_ERROR_WARNING;
	else
		new_state = CAN_STATE_ERROR_ACTIVE;

	*rx_state = rec >= tec ? new_state : 0;
	*tx_state = rec <= tec ? new_state : 0;

	return new_state;
}

/**
 * ftcan_update_error_state_after_rxtx - Update CAN error state after RX/TX
 * @ndev:	Pointer to net_device structure
 *
 * If the device is in a ERROR-WARNING or ERROR-PASSIVE state, check if
 * the performed RX/TX has caused it to drop to a lesser state and set
 * the interface state accordingly.
 */

static void ftcan_update_error_state_after_rxtx(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	enum can_state old_state = priv->can.state;
	struct can_regs *regs = priv->reg_base;
	enum can_state new_state, tx_state, rx_state;
	u16 rec, tec;

	/* changing error state due to successful frame RX/TX can only
	 * occur from these states
	 */
	if (old_state != CAN_STATE_ERROR_WARNING &&
	    old_state != CAN_STATE_ERROR_PASSIVE)
		return;

	new_state = ftcan_current_error_state(ndev, &tx_state, &rx_state);
	ftcan_get_tec_rec(regs, &tec, &rec);

	if (new_state != old_state) {
		struct can_frame *cf;
		struct sk_buff *skb;

		skb = alloc_can_err_skb(ndev, &cf);
		can_change_state(ndev, cf, tx_state, rx_state);

		if (cf) {
			cf->data[6] = tec;
			cf->data[7] = rec;
		}

		if (skb) {
			struct net_device_stats *stats = &ndev->stats;

			stats->rx_packets++;
			stats->rx_bytes += cf->can_dlc;
			netif_rx(skb);
		}
	}
}

static void ftcan_tx_failure_cleanup(struct net_device *ndev,
				     struct can_frame *cf)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	unsigned long flags;
	u32 reg_btr_all;
	u8 err_buffer, curr_transmit_stat;

	reg_btr_all = ftcan_read_bits(&regs->can_cantrol.all, CAN_CR_BTR_ALL_W);
	err_buffer = ftcan_get_laset_err_buffer(regs);
	curr_transmit_stat = ftcan_get_curr_transmit_status(regs);

	netdev_dbg(ndev, "err buffer    status %x\n", err_buffer);
	netdev_dbg(ndev, "curr transmit status %x\n", curr_transmit_stat);

	if ((curr_transmit_stat & (CAN_TS_BTA0 | CAN_TS_BTLA0)) &&
	    !(reg_btr_all & CAN_CR_BTR0_W) && priv->tx_buffer[0]) {
		netdev_warn(ndev, "TX fail! Free echo skb[0]\n");
		if (curr_transmit_stat & CAN_TS_BTLA0) {
			priv->can.can_stats.arbitration_lost++;
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
				cf->can_id |= CAN_ERR_LOSTARB;
				cf->data[0] = CAN_ERR_LOSTARB_UNSPEC;
			}
		}
		can_free_echo_skb(ndev, 0);
		spin_lock_irqsave(&priv->tx_lock, flags);
		priv->tx_buffer[0] = NULL;
		netif_wake_queue(ndev);
		spin_unlock_irqrestore(&priv->tx_lock, flags);
	}

	if (CONFIG_FTCAN_TX_BUFFER_NUM >= 2 &&
	    (curr_transmit_stat & (CAN_TS_BTA1 | CAN_TS_BTLA1)) &&
	    !(reg_btr_all & CAN_CR_BTR1_W) && priv->tx_buffer[1]) {
		netdev_warn(ndev, "TX fail! Free echo skb[1]\n");
		if (curr_transmit_stat & CAN_TS_BTLA1) {
			priv->can.can_stats.arbitration_lost++;
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
				cf->can_id |= CAN_ERR_LOSTARB;
				cf->data[0] = CAN_ERR_LOSTARB_UNSPEC;
			}
		}
		can_free_echo_skb(ndev, 1);
		spin_lock_irqsave(&priv->tx_lock, flags);
		priv->tx_buffer[1] = NULL;
		netif_wake_queue(ndev);
		spin_unlock_irqrestore(&priv->tx_lock, flags);
	}

	if (CONFIG_FTCAN_TX_BUFFER_NUM == 3 &&
	    (curr_transmit_stat & (CAN_TS_BTA2 | CAN_TS_BTLA2)) &&
	    !(reg_btr_all & CAN_CR_BTR1_W) && priv->tx_buffer[2]) {
		netdev_warn(ndev, "TX fail! Free echo skb[2]\n");
		if (curr_transmit_stat & CAN_TS_BTLA2) {
			priv->can.can_stats.arbitration_lost++;
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
				cf->can_id |= CAN_ERR_LOSTARB;
				cf->data[0] = CAN_ERR_LOSTARB_UNSPEC;
			}
		}
		can_free_echo_skb(ndev, 2);
		spin_lock_irqsave(&priv->tx_lock, flags);
		priv->tx_buffer[2] = NULL;
		netif_wake_queue(ndev);
		spin_unlock_irqrestore(&priv->tx_lock, flags);
	}
}

/**
 * ftcan_err_interrupt - error frame Isr
 * @ndev:	net_device pointer
 * @isr:	interrupt status register value
 * @cf:		can_frame pointer
 *
 * This is the CAN error interrupt and it will  check the type of check the type
 * of error and forward the error frame to upper layers.
 */

static void ftcan_err_interrupt(struct net_device *ndev, u32 isr,
				struct can_frame *cf)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_regs *regs = priv->reg_base;
	enum can_state new_state;
	bool berr_reporting = false;
	u32 bus_off_status, err_warn_status;
	u8 err_type;

	/* Get error type*/
	err_type = ftcan_get_laset_err_code(regs);

	netdev_dbg(ndev, "err type status %x\n", err_type);

	if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING) {
		berr_reporting = true;
		cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;
	}

	/* Check for Ack error*/
	if (err_type & CAN_ET_AE) {
		netdev_warn(ndev, "Ack error\n");
		stats->tx_errors++;
		if (berr_reporting) {
			cf->can_id |= CAN_ERR_ACK;
			cf->data[3] = CAN_ERR_PROT_LOC_ACK;
		}
	}

	/* Check for Bit error */
	if (err_type & CAN_ET_BE) {
		netdev_warn(ndev, "Bit error\n");
		stats->tx_errors++;
		if (berr_reporting) {
			cf->can_id |= CAN_ERR_PROT;
			cf->data[2] = CAN_ERR_PROT_BIT;
		}
	}

	/* Check for Stuff error */
	if (err_type & CAN_ET_SE) {
		netdev_warn(ndev, "Stuff error\n");
		stats->rx_errors++;
		if (berr_reporting) {
			cf->can_id |= CAN_ERR_PROT;
			cf->data[2] = CAN_ERR_PROT_STUFF;
		}
	}

	/* Check for Form error */
	if (err_type & CAN_ET_FE) {
		netdev_warn(ndev, "Form error\n");
		stats->rx_errors++;
		if (berr_reporting) {
			cf->can_id |= CAN_ERR_PROT;
			cf->data[2] = CAN_ERR_PROT_FORM;
		}
	}

	/* Check for CRC error */
	if (err_type & CAN_ET_CE) {
		netdev_warn(ndev, "CRC error\n");
		stats->rx_errors++;
		if (berr_reporting) {
			cf->can_id |= CAN_ERR_PROT;
			cf->data[3] = CAN_ERR_PROT_LOC_CRC_SEQ;
		}
	}

	/* On always retransmit mode, every transmission error will result in an
	 * error interrupt. So do not clean tx skb in linux.
	 */
	if (priv->retransmit_times != CAN_RT_ALWAYS)
		ftcan_tx_failure_cleanup(ndev, cf);

	priv->can.can_stats.bus_error++;

	/* bus off */
	bus_off_status = ftcan_read_bits(&regs->can_status.all, CAN_BS_BO_W);
	err_warn_status = ftcan_read_bits(&regs->can_status.all, CAN_BS_EW_W);

	netdev_dbg(ndev, "dump can status reg %08x\n",
		ftcan_read_reg(&regs->can_status.all));

	if (bus_off_status) {
		netdev_dbg(ndev, "bus off!\n");
		priv->can.state = CAN_STATE_BUS_OFF;
		priv->can.can_stats.bus_off++;
		ftcan_reset(ndev);
		ftcan_clear_bits(&regs->can_cantrol.all, CAN_CR_BTR_ALL_W);
		ftcan_clean(ndev);
		can_bus_off(ndev);
		cf->can_id |= CAN_ERR_BUSOFF;
	} else if (err_warn_status) {
		u16 rec, tec;
		enum can_state tx_state, rx_state;
		enum can_state old_state = priv->can.state;

		new_state = ftcan_current_error_state(ndev, &tx_state, &rx_state);
		ftcan_get_tec_rec(regs, &tec, &rec);

		if (new_state != old_state && new_state < CAN_STATE_BUS_OFF) {
			can_change_state(ndev, cf, tx_state, rx_state);
			if (cf) {
				cf->data[6] = tec;
				cf->data[7] = rec;
			}
		}
	}
}

/**
 * ftcan_get_rx_frame - get rx frame form can register
 * @ndev:	net_device pointer
 * @cf:		canfd_frame pointer
 * @fifo_num:	Rx FIFO number
 */

static void ftcan_get_rx_frame(struct net_device *ndev,
			       struct canfd_frame *cf, u8 fifo_num)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	u32 dlc, reg32_Rx_L, reg32_Rx_H, i, j;

	reg32_Rx_L = ftcan_read_reg(&regs->can_rr[fifo_num].rx_l.all);
	reg32_Rx_H = ftcan_read_reg(&regs->can_rr[fifo_num].rx_h.all);

	dlc = (reg32_Rx_H & CAN_RBILDL_RDL_W) >> CAN_RBILDL_RDL_SHIFT;

	if (reg32_Rx_L & CAN_RFD_REDL_W) {
		cf->len = can_dlc2len(dlc);
		/* The received bit rate switch */
		if (reg32_Rx_L & CAN_RFD_RBRS_W)
			cf->flags |= CANFD_BRS;
		if (reg32_Rx_L & CAN_RFD_RESI_W)
			cf->flags |= CANFD_ESI;
	} else {
		cf->len = get_can_dlc(dlc);
		/* Received remote transmission request */
		if (reg32_Rx_L & CAN_RREIE_RRTR_W)
			cf->can_id |= CAN_RTR_FLAG;
	}

	/* Change lotus CAN ID format to socketCAN ID format */
	if (reg32_Rx_L & CAN_RREIE_REIE_W) {
		/* The received frame is an Extended format frame */
		cf->can_id = ((reg32_Rx_H & CAN_RBI_W) >> CAN_RBI_SHIFT) <<
			(CAN_EFF_ID_BITS - CAN_SFF_ID_BITS);
		cf->can_id |= ((reg32_Rx_H & CAN_REIH_W) << CAN_REIH_SHIFT) |
			((reg32_Rx_L & CAN_REIL_W) >> CAN_REIL_SHIFT);
		cf->can_id |= CAN_EFF_FLAG;

		if (reg32_Rx_L & CAN_RFD_RBRS_W)
			cf->flags |= CANFD_BRS;
	} else {
		/* The received frame is a standard format frame */
		cf->can_id = ((reg32_Rx_H & CAN_RBI_W) >> CAN_RBI_SHIFT);
	}

	netdev_dbg(ndev, "RX can id:%x len:%u flag:%x\n",
		   cf->can_id, cf->len, cf->flags);

	for (i = 0, j = 0; i < cf->len; i += sizeof(u32), j++)
		*(__be32 *)(cf->data + i) =
			ftcan_read_reg(&regs->can_rd[fifo_num].data[j].all);
}

/**
 * ftcan_rx_process - Rx Done Isr
 * @ndev:	net_device pointer
 * @fifo_num:	Rx FIFO number
 *
 * This function is invoked from the CAN isr(poll) to process the Rx frames. It
 * does minimal processing and invokes "netif_receive_skb" to complete further
 * processing.
 *
 * Return: 0 on success and failure value on error
 */

static int ftcan_rx_process(struct net_device *ndev, u8 fifo_num)
{
	struct net_device_stats *stats = &ndev->stats;
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	struct canfd_frame *cf;
	struct sk_buff *skb;
	u32 reg32_Rx_L;

	reg32_Rx_L = ftcan_read_reg(&regs->can_rr[fifo_num].rx_l.all);

	if (reg32_Rx_L & CAN_RFD_REDL_W)
		skb = alloc_canfd_skb(ndev, &cf);
	else
		skb = alloc_can_skb(ndev, (struct can_frame **)&cf);

	if (unlikely(!skb)) {
		stats->rx_dropped++;
		return -ENOBUFS;
	}

	ftcan_get_rx_frame(ndev, cf, fifo_num);

	if (fifo_num == CAN_RXFIFO_0)
	/* Release a top buffer in this RxFIFO */
		ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_RRB0_W);
	else
		ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_RRB1_W);

	stats->rx_bytes += cf->len;
	stats->rx_packets++;
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	netif_receive_skb(skb);
#else
	netif_rx(skb);
#endif /* # CONFIG_FTCAN_RX_NAPI_SUPPORT */

	return 0;
}

#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
/**
 * ftcan_rx0_poll - Poll routine for rx packets (NAPI)
 * @napi:	napi structure pointer
 * @quota:	Max number of rx packets to be processed.
 *
 * This is the poll routine for rx part.
 * It will process the packets maximux quota value.
 *
 * Return: number of packets received
 */
static s32 ftcan_rx0_poll(struct napi_struct *napi, s32 quota)
{
	struct net_device *ndev = napi->dev;
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	s32 work_done = 0;
	u8 buffer0_num;
	u32 reg_value;

	reg_value = ftcan_read_bits(&regs->can_status.all, CAN_TRBS_BRS0_W);
	buffer0_num = (u8)(reg_value >> CAN_TRBS_BRS0_SHIFT);

	while (work_done < quota && buffer0_num) {
		ftcan_rx_process(ndev, CAN_RXFIFO_0);
		work_done++;
		reg_value = ftcan_read_bits(&regs->can_status.all,
					    CAN_TRBS_BRS0_W);
		buffer0_num = (u8)(reg_value >> CAN_TRBS_BRS0_SHIFT);
	}

	if (work_done < quota)
		napi_complete_done(napi, work_done);

	if (work_done)
		ftcan_update_error_state_after_rxtx(ndev);

	if (buffer0_num)
		napi_schedule(&priv->napi_rx0);

	return work_done;
}

#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
static s32 ftcan_rx1_poll(struct napi_struct *napi, s32 quota)
{
	struct net_device *ndev = napi->dev;
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	u32 reg_value;
	s32 work_done = 0;
	u8 buffer1_num;

	reg_value = ftcan_read_bits(&regs->can_status.all, CAN_TRBS_BRS1_W);
	buffer1_num = (u8)(reg_value >> CAN_TRBS_BRS1_SHIFT);

	while (work_done < quota && buffer1_num) {
		ftcan_rx_process(ndev, CAN_RXFIFO_1);
		work_done++;
		reg_value = ftcan_read_bits(&regs->can_status.all,
					    CAN_TRBS_BRS1_W);
		buffer1_num = (u8)(reg_value >> CAN_TRBS_BRS1_SHIFT);
	}

	if (work_done < quota)
		napi_complete_done(napi, work_done);

	if (work_done)
		ftcan_update_error_state_after_rxtx(ndev);

	if (buffer1_num)
		napi_schedule(&priv->napi_rx0);

	return work_done;
}

#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */


/**
 * ftcan_tx_interrupt - Tx Done Isr
 * @ndev:	net_device pointer
 * @isr:	Interrupt status register value
 */

static void ftcan_tx_interrupt(struct net_device *ndev, u8 isr)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct net_device_stats *stats = &ndev->stats;
	struct can_regs *regs = priv->reg_base;
	unsigned long flags;

	/**
	 * sometimes we tx once, but recevie two rx interrupt,
	 * so use tx_buffer to judge the rx interrupt is valid.
	 */
	if (isr & CAN_IR_TBI0) {
		netdev_dbg(ndev, "Tx buffer 0 is successful.\n");
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_TBI0_STATUS_BIT);
		if (priv->tx_buffer[0]) {
			stats->tx_bytes += can_get_echo_skb(ndev, 0);
			stats->tx_packets++;
			spin_lock_irqsave(&priv->tx_lock, flags);
			priv->tx_buffer[0] = NULL;
			netif_wake_queue(ndev);
			spin_unlock_irqrestore(&priv->tx_lock, flags);
		}
	}

	if (CONFIG_FTCAN_TX_BUFFER_NUM >= 2 && isr & CAN_IR_TBI1) {
		netdev_dbg(ndev, "Tx buffer 1 is successful.\n");
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_TBI1_STATUS_BIT);
		if (priv->tx_buffer[1]) {
			stats->tx_bytes += can_get_echo_skb(ndev, 1);
			stats->tx_packets++;
			spin_lock_irqsave(&priv->tx_lock, flags);
			priv->tx_buffer[1] = NULL;
			netif_wake_queue(ndev);
			spin_unlock_irqrestore(&priv->tx_lock, flags);
		}
	}

	if (CONFIG_FTCAN_TX_BUFFER_NUM == 3 && isr & CAN_IR_TBI2) {
		netdev_dbg(ndev, "Tx buffer 2 is successful.\n");
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_TBI2_STATUS_BIT);
		if (priv->tx_buffer[2]) {
			stats->tx_bytes += can_get_echo_skb(ndev, 2);
			stats->tx_packets++;
			spin_lock_irqsave(&priv->tx_lock, flags);
			priv->tx_buffer[2] = NULL;
			netif_wake_queue(ndev);
			spin_unlock_irqrestore(&priv->tx_lock, flags);
		}
	}

	ftcan_update_error_state_after_rxtx(ndev);
}

/**
 * ftcan_interrupt - CAN Isr
 * @irq:	irq number
 * @dev_id:	device id pointer
 *
 * This is the lotus CAN Isr. It checks for the type of interrupt
 * and invokes the corresponding ISR.
 *
 * Return:
 * IRQ_NONE - If CAN device is in sleep mode, IRQ_HANDLED otherwise
 */

static irqreturn_t ftcan_interrupt(s32 irq, void *dev_id)
{
	struct net_device *ndev = (struct net_device *)dev_id;
	struct net_device_stats *stats = &ndev->stats;
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;
	struct can_frame cf = {0};
	u8 isr;

	/* Get the IR register status bits */
	isr = ftcan_get_ir_status(regs);

	if (!isr)
		return IRQ_NONE;

	netdev_dbg(ndev, "interrupt status %x\n", isr);

	/* Check transmit bits TBIn */
	if (isr & CAN_IR_TB)
		ftcan_tx_interrupt(ndev, isr);

	/* Check receive bits RBIn */
	if (isr & CAN_IR_RBI0) {
		netdev_dbg(ndev, "Rx FIFO 0 is successful.\n");
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
		napi_schedule(&priv->napi_rx0);
#else
		if (ftcan_read_bits(&regs->can_status.all, CAN_TRBS_BRS0_W)) {
			ftcan_rx_process(ndev, CAN_RXFIFO_0);
			ftcan_update_error_state_after_rxtx(ndev);
		}
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_RBI0_STATUS_BIT);
	}

#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	if (isr & CAN_IR_RBI1) {
		netdev_dbg(ndev, "Rx FIFO 1 is successful.\n");
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
		napi_schedule(&priv->napi_rx1);
#else
		if (ftcan_read_bits(&regs->can_status.all, CAN_TRBS_BRS1_W)) {
			ftcan_rx_process(ndev, CAN_RXFIFO_1);
			ftcan_update_error_state_after_rxtx(ndev);
		}
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_RBI1_STATUS_BIT);
	}
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */

	if (isr & CAN_IR_OIR) {
		u8 rx_buf_stats = DEFAULT_STATUS;

		stats->rx_over_errors++;
		/* drop four rx framme onces*/
		stats->rx_errors += 4;
		cf.can_id |= CAN_ERR_CRTL;
		cf.data[1] |= CAN_ERR_CRTL_RX_OVERFLOW;

		ftcan_get_stat_bit(regs, CAN_DO_STATUS_BIT, &rx_buf_stats);

		if (rx_buf_stats == 0)
			netdev_warn(ndev, "No FIFO occurs overrun\n");
		else if (rx_buf_stats == 1) {
			ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_RRBA0_W);
			netdev_warn(ndev, "FIFO 0 occurs overrun\n");
		} else if (rx_buf_stats == 2) {
			ftcan_set_bits(&regs->can_cantrol.all, CAN_CR_RRBA1_W);
			netdev_warn(ndev, "FIFO 1 occurs overrun\n");
		} else if (rx_buf_stats == 3) {
			ftcan_set_bits(&regs->can_cantrol.all,
				       (CAN_CR_RRBA0_W | CAN_CR_RRBA1_W));
			netdev_warn(ndev, "Both FIFOs occur overrun\n");
		}

		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_OIR_STATUS_BIT);
	}

	if (isr & CAN_IR_EIR) {
		ftcan_err_interrupt(ndev, isr, &cf);
		/* RW1C this status bit */
		ftcan_clear_it(regs, CAN_EIR_STATUS_BIT);
	}

	if (cf.can_id) {
		struct can_frame *skb_cf;
		struct sk_buff *skb = alloc_can_err_skb(ndev, &skb_cf);

		if (skb) {
			skb_cf->can_id |= cf.can_id;
			memcpy(skb_cf->data, cf.data, CAN_ERR_DLC);
			stats->rx_packets++;
			stats->rx_bytes += CAN_ERR_DLC;
			netif_rx(skb);
		}
	}

	return IRQ_HANDLED;
}

/**
 * ftcan_open - Driver open routine
 * @ndev:	Pointer to net_device structure
 *
 * This is the driver open routine.
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_open(struct net_device *ndev)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	s32 ret, i;

	ret = ftcan_clk_enable(ndev);
	if (ret < 0) {
		netdev_err(ndev, "ftcan_clk_enable failed!\n");
		goto err;
	}

	ret = request_irq(ndev->irq, ftcan_interrupt, priv->irq_flags,
			  ndev->name, ndev);
	if (ret < 0) {
		netdev_err(ndev, "irq allocation for CAN failed\n");
		goto err;
	}

	/* Common open */
	ret = open_candev(ndev);
	if (ret)
		goto err_irq;

	for (i = 0; i < priv->tx_max; i++)
		priv->tx_buffer[i] = NULL;

	ret = ftcan_chip_start(ndev);
	if (ret < 0) {
		netdev_err(ndev, "ftcan_chip_start failed!\n");
		goto err_candev;
	}

#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	napi_enable(&priv->napi_rx0);
#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	napi_enable(&priv->napi_rx1);
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
	netif_start_queue(ndev);

	return 0;

err_candev:
	close_candev(ndev);
err_irq:
	free_irq(ndev->irq, ndev);
err:
	return ret;
}

/**
 * ftcan_close - Driver close routine
 * @ndev:	Pointer to net_device structure
 *
 * Return: 0 always
 */

static s32 ftcan_close(struct net_device *ndev)
{
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	struct ftcan_priv *priv = netdev_priv(ndev);
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
	s32 i;

	netif_stop_queue(ndev);
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	napi_disable(&priv->napi_rx0);
#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	napi_disable(&priv->napi_rx1);
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
	ftcan_chip_stop(ndev);
	free_irq(ndev->irq, ndev);
	close_candev(ndev);

	for (i = 0; i < priv->tx_max; i++)
		priv->tx_buffer[i] = NULL;

	return 0;
}

/**
 * ftcan_get_berr_counter - error counter routine
 * @ndev:	Pointer to net_device structure
 * @bec:	Pointer to can_berr_counter structure
 *
 * This is the driver error counter routine.
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_get_berr_counter(const struct net_device *ndev,
				  struct can_berr_counter *bec)
{
	struct ftcan_priv *priv = netdev_priv(ndev);
	struct can_regs *regs = priv->reg_base;

	ftcan_get_tec_rec(regs, &bec->txerr, &bec->rxerr);

	return 0;
}

static const struct net_device_ops ftcan_netdev_ops = {
	.ndo_open	= ftcan_open,
	.ndo_stop	= ftcan_close,
	.ndo_start_xmit	= ftcan_start_xmit,
	.ndo_change_mtu	= can_change_mtu,
};

/**
 * ftcan_suspend - Suspend method for the driver
 * @dev:	Address of the device structure
 *
 * Put the driver into low power mode.
 * Return: 0 on success and failure value on error
 */

static s32 __maybe_unused ftcan_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	s32 ret;

	if (netif_running(ndev)) {
		netif_stop_queue(ndev);
		netif_device_detach(ndev);
		ftcan_chip_stop(ndev);
	}

	ret = ftcan_clk_disable(ndev);
	if (ret < 0) {
		netdev_warn(ndev, "ftcan_clk_disable failed on suspend\n");
		goto err;
	}

	return 0;

err:
	return ret;

}

/**
 * ftcan_resume - Resume from suspend
 * @dev:	Address of the device structure
 *
 * Resume operation after suspend.
 * Return: 0 on success and failure value on error
 */

static s32 __maybe_unused ftcan_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	s32 ret;

	if (netif_running(ndev)) {
		ret = ftcan_chip_start(ndev);
		if (ret) {
			dev_err(dev, "ftcan_chip_start failed on resume\n");
			goto err;
		}

		netif_device_attach(ndev);
		netif_start_queue(ndev);
	}

	ret = ftcan_clk_enable(ndev);
	if (ret < 0) {
		netdev_err(ndev, "ftcan_clk_enable failed on resume\n");
		goto err;
	}

	return 0;

err:
	return ret;
}

static const struct dev_pm_ops ftcan_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ftcan_suspend, ftcan_resume)
};

static const struct ftcan_devtype_data ftcanfd_data = {
	.cantype = FT_CANFD,
	.bittiming_const = &ftcan_nominal_bittiming_const,
	.data_bittiming_const = &ftcan_data_bittiming_const_canfd,
};

/* Match table for OF platform binding */
static const struct of_device_id ftcan_of_match[] = {
	{ .compatible = "lotus,lotus-can", .data = &ftcanfd_data },
	{ }
};
MODULE_DEVICE_TABLE(of, ftcan_of_match);

/**
 * ftcan_probe - Platform registration call
 * @pdev:	Handle to the platform device structure
 *
 * This function does all the memory allocation and registration for the CAN
 * device.
 *
 * Return: 0 on success and failure value on error
 */

static s32 ftcan_probe(struct platform_device *pdev)
{
	struct net_device *ndev;
	struct ftcan_priv *priv;
	const struct of_device_id *of_id;
	const struct ftcan_devtype_data *devtype = &ftcanfd_data;
	void __iomem *addr;
	s32 ret, rx0_max, rx1_max, tx_max;

	if ((gcan0_clk != FTCAN_CLK_24M && gcan0_clk != FTCAN_CLK_99M) ||
	    (gcan1_clk != FTCAN_CLK_24M && gcan1_clk != FTCAN_CLK_99M)) {
		dev_err(&pdev->dev, "can device clock must be 24M or 100M\n");
		goto err;
	}

	/* FIXME: clk init must before read reg, otherwise system stuck */
	ret = ftcan_clk_init();
	if (ret < 0) {
		dev_err(&pdev->dev, "fail to init can device clock\n");
		goto err;
	}

	/* Get the virtual base address for the device */
	addr = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(addr)) {
		ret = PTR_ERR(addr);
		goto err_exit_clk;
	}

	of_id = of_match_device(ftcan_of_match, &pdev->dev);
	if (of_id && of_id->data)
		devtype = of_id->data;

	tx_max = CONFIG_FTCAN_TX_BUFFER_NUM;

	rx0_max = RX_BUFFER0_DEPTH;
	rx1_max = RX_BUFFER1_DEPTH;

	/* Create a CAN device instance */
	ndev = alloc_candev(sizeof(struct ftcan_priv), tx_max);
	if (!ndev) {
		ret = -ENOMEM;
		goto err_exit_clk;
	}

	priv = netdev_priv(ndev);
	priv->dev = &pdev->dev;
	priv->reg_base = addr;
	priv->can.bittiming_const = devtype->bittiming_const;
	priv->can.data_bittiming_const = devtype->data_bittiming_const;
	priv->can.do_set_data_bittiming = ftcan_set_data_bittiming;
	priv->can.do_set_mode = ftcan_do_set_mode;
	priv->can.do_get_berr_counter = ftcan_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
				       CAN_CTRLMODE_BERR_REPORTING;

	if (devtype->cantype == FT_CANFD)
		priv->can.ctrlmode_supported |= CAN_CTRLMODE_FD;

	priv->tx_max = tx_max;
	priv->devtype = *devtype;
	priv->retransmit_times = (u32)CONFIG_FTCAN_RETRANSMISSION_TIMES;
	spin_lock_init(&priv->tx_lock);

	/* Get IRQ for the device */
	ret = platform_get_irq(pdev, 0);
	if (ret < 0)
		goto err_free;

	ndev->irq = ret;

	ndev->flags |= IFF_ECHO; /* We support local echo */

	platform_set_drvdata(pdev, ndev);
	SET_NETDEV_DEV(ndev, &pdev->dev);
	ndev->netdev_ops = &ftcan_netdev_ops;

#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	netif_napi_add(ndev, &priv->napi_rx0, ftcan_rx0_poll, rx0_max);
#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	netif_napi_add(ndev, &priv->napi_rx1, ftcan_rx1_poll, rx1_max);
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */

	ret = register_candev(ndev);
	if (ret) {
		dev_err(&pdev->dev, "fail to register failed (err=%d)\n", ret);
		goto err_free;
	}

	if (!memcmp(netdev_name(ndev), "can0", 4))
		priv->can.clock.freq = gcan0_clk;
	else
		priv->can.clock.freq = gcan1_clk;

	netdev_dbg(ndev, "irq=%d clock=%d, tx buffers: using %d\n",
		   ndev->irq, priv->can.clock.freq,  priv->tx_max);

	return 0;

err_free:
	free_candev(ndev);
err_exit_clk:
	ftcan_clk_exit();
err:
	return ret;
}

/**
 * ftcan_remove - Unregister the device after releasing the resources
 * @pdev:	Handle to the platform device structure
 *
 * This function frees all the resources allocated to the device.
 * Return: 0 always
 */

static s32 ftcan_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	struct ftcan_priv *priv = netdev_priv(ndev);
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */

	unregister_candev(ndev);
#ifdef CONFIG_FTCAN_RX_NAPI_SUPPORT
	netif_napi_del(&priv->napi_rx0);
#ifdef CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT
	netif_napi_del(&priv->napi_rx1);
#endif /* CONFIG_FTCAN_SECOND_RX_BUFFER_SUPPORT */
#endif /* CONFIG_FTCAN_RX_NAPI_SUPPORT */
	ftcan_clk_disable(ndev);
	free_candev(ndev);

	return 0;
}

static struct platform_driver ftcan_driver = {
	.probe	= ftcan_probe,
	.remove	= ftcan_remove,
	.driver	= {
		.name = DRIVER_NAME,
		.pm = &ftcan_dev_pm_ops,
		.of_match_table	= ftcan_of_match,
	},
};

module_platform_driver(ftcan_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lotus Inc");
MODULE_DESCRIPTION("FTCAN interface");
