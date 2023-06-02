/*
 * Novatek IR remote receiver - BK platform hardware operation
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/bitrev.h>

#include <media/rc-core.h>

#include "nvt_ir.h"
#include "nvt_ir_bk.h"

#include <plat/top.h>

#define IR_CLK_FREQ  12000000

static u32 ir_clk_div = IR_CLK_DIV_120;
static u64 ir_cycle_time;
static bool ir_input_inverse = true;
static IR_DATA_ORDER ir_data_order = IR_DATA_MSB;
static IR_INPUT_CH_SEL ir_input_ch;

/*******************
 *      Debug      *
 *******************/
static int __ir_isr_debug = 0;
module_param_named(ir_isr_debug, __ir_isr_debug, int, 0600);

static int __ir_debug = 0;
module_param_named(ir_debug, __ir_debug, int, 0600);

static int __ir_flow_debug = 0;
module_param_named(ir_flow_debug, __ir_flow_debug, int, 0600);

#define ir_isr_dbg(fmt, ...) do { \
	if (__ir_isr_debug) { \
		pr_info("nvt_ir_bk: "fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_dbg(fmt, ...) do { \
	if (__ir_debug) { \
		pr_info("nvt_ir_bk: "fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_flow_dbg(fmt, ...) do { \
	if (__ir_flow_debug) { \
		pr_info("nvt_ir_bk: %s\n"fmt, __func__, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_err(fmt, ...) do { \
	pr_err("nvt_ir_bk: "fmt, ##__VA_ARGS__); \
} while (0)

/**********************
 *      Internal      *
 **********************/
static void nvt_ir_interrupt_enable(struct nvt_ir *ir, IR_INTERRUPT int_id)
{
	ir_flow_dbg();

	nvt_ir_masked_write(ir, IR_INT_REG, RD_INT_BIT | REP_INT_BIT | TOUT_INT_BIT, int_id);
}

static void nvt_ir_interrupt_disable(struct nvt_ir *ir, IR_INTERRUPT int_id)
{
	ir_flow_dbg();

	nvt_ir_masked_write(ir, IR_INT_REG, int_id, 0);
}

static void nvt_ir_config(struct nvt_ir *ir, IR_CONFIG_ID cfg_id, u32 cfg_value)
{
	u64 count;

	ir_flow_dbg();

	// Re-Map the setting period from US to the controller clock cycles according to the module clock design.
	count = cfg_value * ir_cycle_time;
	do_div(count, 1000000);

	ir_dbg("cfg_id(%d), cfg_value(%u) count(%llu)\n", cfg_id, cfg_value, count);

	switch (cfg_id) {
	case IR_CONFIG_ID_CLK_DIV: {
			// Set the clock divider, the (clk_div) we want, the (clk_div - 2) we set
			nvt_ir_masked_write(ir, IR_PARAM2_REG, CLK_DIV_MASK, (cfg_value - 2));

			ir_clk_div = cfg_value;
			ir_cycle_time = IR_CLK_FREQ / ir_clk_div;
		}
		break;

	case IR_CONFIG_ID_HIGH_TH: {
			if (count > 32767) {
				count = 32767;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			nvt_ir_masked_write(ir, IR_PARAM0_REG, HIGH_TH_MASK, count);
		}
		break;

	case IR_CONFIG_ID_LOW_TH0: {
			if (count > 65535) {
				count = 65535;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			nvt_ir_masked_write(ir, IR_PARAM1_REG, LOW_TH0_MASK, count);
		}
		break;

	case IR_CONFIG_ID_LOW_TH1: {
			if (count > 65535) {
				count = 65535;
			}

			// This is designed to distinguish the repeat code or the header code.
			// In the NEC protocol, the normal command after header is about 4.2ms and the repeat pulse after
			// the header is about 2.2ms. So this paramter must be set to the range from 2.2 ~ 4.2 ms.
			// To help the controller differentiate the normal command or the repeat code.
			nvt_ir_masked_write(ir, IR_PARAM1_REG, LOW_TH1_MASK, count << _LOW_TH1_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_TIMEOUT_TH: {
			if (count > 65535) {
				count = 65535;
			}

			// If the un-complete command stopping over the error threshold value.
			// It would trigger the controller error interrupt event.
			nvt_ir_masked_write(ir, IR_PARAM0_REG, TOUT_TH_MASK, count << _TOUT_TH_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_HEADER_DETECT: {
			// Select the force header detection enable/disable.
			nvt_ir_masked_write(ir, IR_CTRL_REG, NSCD_BIT, cfg_value << _NSCD_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_INPUT_INVERSE: {
			// Select input inverse enable/disable
			nvt_ir_masked_write(ir, IR_CTRL_REG, IIL_BIT, cfg_value << _IIL_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_SEL: {
			// Select input inverse enable/disable
			nvt_ir_masked_write(ir, IR_CTRL_REG, BI_PHASE_BIT, cfg_value << _BI_PHASE_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_DATA_ORDER: {
			// Select the input command signal is LSb or MSb first.
			nvt_ir_masked_write(ir, IR_CTRL_REG, MSB_BIT, cfg_value << _MSB_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_DATA_LENGTH: {
			// Set the data length, the (fifo_size) we want, the (fifo_size - 1) we set
			nvt_ir_masked_write(ir, IR_PARAM2_REG, FIFO_SIZE_MASK, (cfg_value - 1) << _FIFO_SIZE_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_INPUT_CH_SEL: {
			// Select the input channel
			nvt_ir_masked_write(ir, IR_PARAM2_REG, CH_SEL_MASK, cfg_value << _CH_SEL_MASK_SHIFT);
		}
		break;

	default:
		break;
	}
}

static void nvt_ir_decoder(struct work_struct *work)
{
	struct nvt_ir *ir = container_of(work, struct nvt_ir, decoder_work);
	u32 scancode;
	enum rc_proto protocol;
	u8 address, not_address, command, not_command;

	switch (ir->rc_type) {
	case (RC_PROTO_BIT_NEC + RC_PROTO_BIT_NECX + RC_PROTO_BIT_NEC32): {
			u32 data = ir->data_lo;

			address     = bitrev8((data >> 24) & 0xff);
			not_address = bitrev8((data >> 16) & 0xff);
			command	    = bitrev8((data >>  8) & 0xff);
			not_command = bitrev8((data >>  0) & 0xff);

			scancode = ir_nec_bytes_to_scancode(address, not_address, command, not_command, &protocol);

			ir_dbg("decoding NEC protocol: protocol(%d) raw(0x%x) scancode(0x%x)\n", protocol, data, scancode);

			rc_keydown(ir->rc, protocol, scancode, 0);
		}
		break;

	case RC_PROTO_BIT_JVC: {
			u32 data = ir->data_lo;

			scancode = (bitrev8((data >> 8) & 0xff) << 8) |
				   (bitrev8((data >> 0) & 0xff) << 0);
			protocol = RC_PROTO_JVC;

			ir_dbg("decoding JVC protocol: protocol(%d) raw(0x%x) scancode(0x%x)\n", protocol, data, scancode);

			rc_keydown(ir->rc, protocol, scancode, 0);
		}
		break;

	case RC_PROTO_BIT_SHARP: {
			u32 data = ir->data_lo;
			u32 msg, echo;

			/* Validate - command, ext, chk should be inverted in 2nd */
			msg = (data >> 15) & 0x7fff;
			echo = data & 0x7fff;
			if ((msg ^ echo) != 0x3ff) {
				ir_err("Sharp checksum error: received msg(0x%04x) echo(0x%04x)\n", msg, echo);
			}

			address = bitrev8((msg >> 7) & 0xf8);
			command = bitrev8((msg >> 2) & 0xff);

			scancode = address << 8 | command;
			protocol = RC_PROTO_SHARP;

			ir_dbg("decoding Sharp protocol: protocol(%d) raw(0x%x) scancode(0x%x)\n", protocol, data, scancode);

			rc_keydown(ir->rc, protocol, scancode, 0);
		}
		break;

	case (RC_PROTO_BIT_RC5 + RC_PROTO_BIT_RC5X_20): {
			u32 data = ir->data_lo;
			u8 command, system, toggle;

			command  = (data & 0x0003F) >> 0;
			system   = (data & 0x007C0) >> 6;
			toggle   = (data & 0x00800) ? 1 : 0;
			command += (data & 0x01000) ? 0 : 0x40;
			scancode = system << 8 | command;
			protocol = RC_PROTO_RC5;

			ir_dbg("decoding RC5 protocol: protocol(%d) raw(0x%x) scancode(0x%x)\n", protocol, data, scancode);

			rc_keydown(ir->rc, protocol, scancode, toggle);
		}
		break;

	default:
		ir_err("decoding as unhandled protocol, rc_type(0x%llx)\n", ir->rc_type);

		break;
	}
}

/***********************************
 *      External to interface      *
 ***********************************/
void nvt_ir_irq_cb(struct nvt_ir *ir)
{
	u32 sts_r;

	ir_flow_dbg();

	sts_r = nvt_ir_read(ir, IR_STS_REG);
	sts_r &= nvt_ir_read(ir, IR_INT_REG);

	nvt_ir_write(ir, IR_STS_REG, sts_r);  /* clear interrupt status */

	if (sts_r & RD_STS_BIT) {
		/* Get the raw data */
		ir->data_lo = nvt_ir_read(ir, IR_DATA_LO_REG);
		ir->data_hi = nvt_ir_read(ir, IR_DATA_HI_REG);

		nvt_ir_masked_write(ir, IR_CTRL_REG, EN_BIT, EN_BIT);  /* re-enable because it is cleared by hardware */

		ir_isr_dbg("RD interrupt, data_lo(0x%x) data_hi(0x%x)\n", ir->data_lo, ir->data_hi);

		/* Decode raw data to scancode */
		schedule_work(&ir->decoder_work);
	}

	if (sts_r & REP_STS_BIT) {
		ir_isr_dbg("REP interrupt\n");
	}

	if (sts_r & TOUT_STS_BIT) {
		ir_isr_dbg("TOUT interrupt\n");
	}
}

void nvt_ir_enable(struct nvt_ir *ir)
{
	ir_flow_dbg();

	nvt_ir_masked_write(ir, IR_CTRL_REG, EN_BIT, EN_BIT);
}

void nvt_ir_disable(struct nvt_ir *ir)
{
	ir_flow_dbg();

	nvt_ir_masked_write(ir, IR_CTRL_REG, EN_BIT, 0);
}

void nvt_ir_protocol_config(struct nvt_ir *ir)
{
	PIN_GROUP_CONFIG pinmux_config[1];
	int ret;

	ir_flow_dbg();

	pinmux_config->pin_function = PIN_FUNC_REMOTE;

	ret = nvt_pinmux_capture(pinmux_config, 1);
	if (ret) {
		ir_err("get pinmux error\n");
	} else {
		for (ir_input_ch = IR_INPUT_CH_0; ir_input_ch < IR_INPUT_CH_NUM; ir_input_ch++) {
			if ((0x1 << ir_input_ch) & pinmux_config->config) {
				ir_dbg("pinmux(0x%x), ir_input_ch(%d)\n", pinmux_config->config, ir_input_ch);
				break;
			}
		}

		if (ir_input_ch == IR_INPUT_CH_NUM) {
			ir_err("pinmux is not set\n");
		}
	}

	switch (ir->rc_type) {
	case (RC_PROTO_BIT_NEC + RC_PROTO_BIT_NECX + RC_PROTO_BIT_NEC32):
		ir_dbg("configuring IR receiver for NEC protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_DIV, IR_CLK_DIV_120);
		nvt_ir_config(ir, IR_CONFIG_ID_HIGH_TH, 500);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH0, 4000);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH1, 1120);
		nvt_ir_config(ir, IR_CONFIG_ID_TIMEOUT_TH, 10240);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_FORCE_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_SEL, false);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 32);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_CH_SEL, ir_input_ch);

		nvt_ir_interrupt_enable(ir, IR_INT_ALL);
		nvt_ir_enable(ir);

		break;

	case RC_PROTO_BIT_JVC:
		ir_dbg("configuring IR receiver for JVC protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_DIV, IR_CLK_DIV_120);
		nvt_ir_config(ir, IR_CONFIG_ID_HIGH_TH, 500);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH0, 4000);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH1, 1120);
		nvt_ir_config(ir, IR_CONFIG_ID_TIMEOUT_TH, 10240);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_FORCE_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_SEL, false);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 16);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_CH_SEL, ir_input_ch);

		nvt_ir_interrupt_enable(ir, IR_INT_ALL);
		nvt_ir_enable(ir);

		break;

	case RC_PROTO_BIT_SHARP:
		ir_dbg("configuring IR receiver for Sharp protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_DIV, IR_CLK_DIV_120);
		nvt_ir_config(ir, IR_CONFIG_ID_HIGH_TH, 280);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH0, 4000);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH1, 1180);
		nvt_ir_config(ir, IR_CONFIG_ID_TIMEOUT_TH, 40320);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_NO_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_SEL, false);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 13);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_CH_SEL, ir_input_ch);

		nvt_ir_interrupt_enable(ir, IR_INT_ALL);
		nvt_ir_enable(ir);

		break;

	case (RC_PROTO_BIT_RC5 + RC_PROTO_BIT_RC5X_20):
		ir_dbg("configuring IR receiver for RC5 protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		if (ir->clk) {
			clk_disable_unprepare(ir->clk);
			ret = clk_prepare_enable(ir->clk);
			if (ret) {
				ir_err("clk_prepare_enable failed, unable to toggle IR reset\n");
			}
		}

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_DIV, IR_CLK_DIV_120);
		nvt_ir_config(ir, IR_CONFIG_ID_HIGH_TH, 0);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH0, 0);
		nvt_ir_config(ir, IR_CONFIG_ID_LOW_TH1, 1350);
		nvt_ir_config(ir, IR_CONFIG_ID_TIMEOUT_TH, 2720);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_NO_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_SEL, true);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 14);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_CH_SEL, ir_input_ch);

		nvt_ir_interrupt_enable(ir, IR_INT_ALL);
		nvt_ir_enable(ir);

		break;

	default:
		ir_err("configuring as unhandled protocol, rc_type(0x%llx)\n", ir->rc_type);

		break;
	}
}

void nvt_ir_platform_init(struct nvt_ir *ir)
{
	ir_flow_dbg();

	INIT_WORK(&ir->decoder_work, nvt_ir_decoder);
}
