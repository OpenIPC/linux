/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Ingenic T23 Low-Power RISC-V Coprocessor Platform Driver Header
 *
 * Copyright (C) 2026 Difan Zhang
 */

#ifndef _T23_COPROC_H_
#define _T23_COPROC_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>

#define T23_PKT_MAGIC          0x504B5431 /* "PKT1" */
#define T23_MAX_PAYLOAD_LEN    48

/**
 * struct t23_ipc_packet - Standard 64-byte IPC Packet Descriptor
 */
struct t23_ipc_packet {
	u32 magic;
	u16 seq_id;
	u16 cmd;
	u32 status;
	u16 data_len;
	u16 checksum;
	u8  data[T23_MAX_PAYLOAD_LEN];
} __attribute__((packed));

/*
 * MMIO Apertures:
 * AUX Control Aperture:  0x132a0000, 4 KiB
 * TCSM SRAM Aperture:    0x132c0000, 16 KiB
 * 0x12000000 must not be touched.
 */
#define AUX_IOBASE			0x132a0000
#define AUX_IOSIZE			0x1000
#define TCSM_IOBASE			0x132c0000
#define TCSM_IOSIZE			0x4000

/*
 * Register offsets in AUX MMIO aperture.
 * Note: The coprocessor register map is documented as placeholder;
 * these auxiliary MMIO registers and bitfields remain undocumented.
 */
#define REG_COPROC_CTRL			0x00	/* placeholder; undocumented */
#define REG_COPROC_STATUS		0x04	/* placeholder; undocumented */
#define REG_HOST_DOORBELL		0x08	/* placeholder; undocumented */
#define REG_COPROC_DOORBELL		0x0c	/* placeholder; undocumented */
#define REG_HOST_IRQ_STATUS		0x10	/* placeholder; undocumented */
#define REG_HOST_IRQ_CLEAR		0x14	/* placeholder; undocumented */
#define REG_FAST_MSG_TX			0x20	/* placeholder; undocumented */
#define REG_FAST_MSG_RX			0x24	/* placeholder; undocumented */

/* Control and status bits (placeholder; undocumented) */
#define COPROC_CTRL_SW_RESET		BIT(0)
#define COPROC_CTRL_MSG_IRQ_EN		BIT(3)
#define COPROC_STATUS_RUNNING		BIT(0)
#define COPROC_DOORBELL_TRIGGER		BIT(0)
#define HOST_IRQ_RX_PENDING		BIT(2)

/* CPM reset register and bit */
#define CPM_SRBC_OFFSET			0xc4
#define SRBC_RISCV_SR			BIT(26)

/* TCSM shared memory partitioning */
#define T23_TCSM_MBOX_OFFSET		0x0000
#define T23_TCSM_TX_OFFSET		0x0200
#define T23_TCSM_RX_OFFSET		0x0900
#define T23_TCSM_FW_OFFSET		0x1000

#define T23_RING_NUM_SLOTS		28
#define T23_MBOX_MAGIC			0x5432334D /* "T23M" */
#define T23_MBOX_VERSION		0x0100

/**
 * struct t23_mbox_header - TCSM mailbox header at offset 0x0000 (64 bytes)
 */
struct t23_mbox_header {
	u32 magic;              /* offset 0x00: 0x5432334D ("T23M") */
	u16 version;            /* offset 0x04: 0x0100 */
	u16 flags;              /* offset 0x06 */
	u32 tx_buf_offset;      /* offset 0x08: 0x0200 */
	u32 tx_buf_size;        /* offset 0x0c: 1792 bytes */
	u32 rx_buf_offset;      /* offset 0x10: 0x0900 */
	u32 rx_buf_size;        /* offset 0x14: 1792 bytes */
	u32 config_offset;      /* offset 0x18 */
	u32 reserved0;          /* offset 0x1c: 32-bit alignment padding */
	u32 heartbeat_counter;  /* offset 0x20: updated by RISC-V loop */
	u32 tx_head;            /* offset 0x24: updated by Host MIPS */
	u32 tx_tail;            /* offset 0x28: updated by RISC-V */
	u32 rx_head;            /* offset 0x2c: updated by RISC-V */
	u32 rx_tail;            /* offset 0x30: updated by Host MIPS */
	u32 reserved[3];        /* offset 0x34..0x3f: padding to 64 bytes */
} __attribute__((packed));

/**
 * struct t23_coproc_priv - Driver private state context
 */
struct t23_coproc_priv {
	struct device *dev;
	struct rproc *rproc;
	struct mutex lock;
	void __iomem *aux_base;
	void __iomem *tcsm_base;
	struct regmap *cpm_map;
	struct clk *clk;
	int irq;
	bool running;
};

/* Remoteproc operations */
extern const struct rproc_ops t23_coproc_rproc_ops;

#endif /* _T23_COPROC_H_ */
