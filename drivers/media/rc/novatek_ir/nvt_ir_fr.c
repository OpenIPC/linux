/*
 * Novatek IR remote receiver - FR platform hardware operation
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
#include <linux/delay.h>
#include <linux/clk-provider.h>

#include <media/rc-core.h>

#include "nvt_ir.h"
#include "nvt_ir_fr.h"

#include <plat/top.h>

static u64 ir_cycle_time;
static bool ir_input_inverse = true;
static bool ir_first = false;

static struct clk *clk_sel0, *clk_sel1;

/*******************
 *      Debug      *
 *******************/
static int __ir_isr_debug = 0;
module_param_named(ir_isr_debug, __ir_isr_debug, int, 0600);

static int __ir_debug = 0;
module_param_named(ir_debug, __ir_debug, int, 0600);

static int __ir_flow_debug = 0;
module_param_named(ir_flow_debug, __ir_flow_debug, int, 0600);

static int __ir_repeat_is_data = 0;
module_param_named(ir_repeat_is_data, __ir_repeat_is_data, int, 0600);

static int __ir_data_order = IR_DATA_MSB;
module_param_named(ir_data_order, __ir_data_order, int, 0600);

static int __ir_use_raw = 0;
module_param_named(ir_use_raw, __ir_use_raw, int, 0600);

static int __ir_first_delay = 10;
module_param_named(ir_first_delay, __ir_first_delay, int, 0600);

static int __ir_repeat_delay = 10;
module_param_named(ir_repeat_delay, __ir_repeat_delay, int, 0600);

#define ir_isr_dbg(fmt, ...) do { \
	if (__ir_isr_debug) { \
		pr_info("nvt_ir_fr: "fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_dbg(fmt, ...) do { \
	if (__ir_debug) { \
		pr_info("nvt_ir_fr: "fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_flow_dbg(fmt, ...) do { \
	if (__ir_flow_debug) { \
		pr_info("nvt_ir_fr: %s\n"fmt, __func__, ##__VA_ARGS__); \
	} \
} while (0)

#define ir_err(fmt, ...) do { \
	pr_err("nvt_ir_fr: "fmt, ##__VA_ARGS__); \
} while (0)

/**********************
 *      Internal      *
 **********************/
static void nvt_ir_interrupt_enable(struct nvt_ir *ir, IR_INTERRUPT int_id)
{
	ir_flow_dbg();

	nvt_ir_masked_write(ir, IR_INT_REG, RD_INT_BIT | ERR_INT_BIT | MATCH_INT_BIT | REP_INT_BIT, int_id);
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
	case IR_CONFIG_ID_CLK_SRC_SEL: {
			if (IS_ERR(clk_sel0)) {
				ir_err("clk_sel0 not initialized\n");
				return;
			} else if (IS_ERR(clk_sel1)) {
				ir_err("clk_sel1 not initialized\n");
				return;
			}

			// Set the clock source
			if (nvt_get_chip_id() == CHIP_NA51084) {
				switch (cfg_value) {
				case IR_CLK_SRC_RTC:
					if (__clk_is_enabled(clk_sel0))
						clk_disable_unprepare(clk_sel0);

					if (__clk_is_enabled(clk_sel1))
						clk_disable_unprepare(clk_sel1);

					ir_cycle_time = 32768;
					clk_set_rate(ir->clk, ir_cycle_time);
					break;

				case IR_CLK_SRC_OSC:
					if (!__clk_is_enabled(clk_sel0))
						clk_prepare_enable(clk_sel0);

					if (__clk_is_enabled(clk_sel1))
						clk_disable_unprepare(clk_sel1);

					ir_cycle_time = 32000;
					clk_set_rate(ir->clk, ir_cycle_time);
					break;

				case IR_CLK_SRC_EXT:
					if (__clk_is_enabled(clk_sel0))
						clk_disable_unprepare(clk_sel0);

					if (!__clk_is_enabled(clk_sel1))
						clk_prepare_enable(clk_sel1);
					break;

				case IR_CLK_SRC_3M:
					if (!__clk_is_enabled(clk_sel0))
						clk_prepare_enable(clk_sel0);

					if (!__clk_is_enabled(clk_sel1))
						clk_prepare_enable(clk_sel1);

					ir_cycle_time = 3000000;
					clk_set_rate(ir->clk, ir_cycle_time);
					break;
				}
			} else {
				switch (cfg_value) {
				case IR_CLK_SRC_RTC:
					if (clk_set_parent(ir->clk, clk_sel0)) {
						ir_err("clk_sel0 set parent failed\n");
					} else {
						ir_cycle_time = 32768;
						clk_set_rate(ir->clk, ir_cycle_time);
					}
					break;

				case IR_CLK_SRC_OSC:
					if (clk_set_parent(ir->clk, clk_sel1)) {
						ir_err("clk_sel1 set parent failed\n");
					} else {
						ir_cycle_time = 32000;
						clk_set_rate(ir->clk, ir_cycle_time);
					}
					break;
				}
			}
		}
		break;

	case IR_CONFIG_ID_LOGIC_TH: {
			if (count > 2047) {
				count = 2047;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			nvt_ir_masked_write(ir, IR_TH1_REG, LOGIC_TH_MASK, count);
		}
		break;

	case IR_CONFIG_ID_GSR_TH: {
			if (count > 255) {
				count = 255;
			}

			// Set Glitch Suppress Threshold Value as clock cycles.
			// The input signal change below than the threshold period would be treated as glitch
			// And the glitch would be ignored.
			nvt_ir_masked_write(ir, IR_TH1_REG, GSR_MASK, count << _GSR_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_REPEAT_TH: {
			if (count > 65535) {
				count = 65535;
			}

			// This is designed to distinguish the repeat code or the header code.
			// In the NEC protocol, the normal command after header is about 4.2ms and the repeat pulse after
			// the header is about 2.2ms. So this paramter must be set to the range from 2.2 ~ 4.2 ms.
			// To help the controller differentiate the normal command or the repeat code.
			nvt_ir_masked_write(ir, IR_TH0_REG, REP_TH_MASK, count << _REP_TH_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_HEADER_TH: {
			if (count > 65535) {
				count = 65535;
			}

			// The pulse width larger than this header threshold value would be treated as HEADER.
			nvt_ir_masked_write(ir, IR_TH0_REG, HEADER_TH_MASK, count);
		}
		break;

	case IR_CONFIG_ID_ERROR_TH: {
			if (count > 65535) {
				count = 65535;
			}
			// If the un-complete command stopping over the error threshold value.
			// It would trigger the controller error interrupt event.
			nvt_ir_masked_write(ir, IR_TH2_REG, ERR_TH_MASK, count);
		}
		break;

	case IR_CONFIG_ID_LOGIC_TH_HW: {
			if (cfg_value > 2047) {
				cfg_value = 2047;
			}

			// Set Logic0/1 Threshold Value as clock cycles
			// The distance length larger than the threshold would be regarded as 1, otherwise 0.
			nvt_ir_masked_write(ir, IR_TH1_REG, LOGIC_TH_MASK, cfg_value);
		}
		break;

	case IR_CONFIG_ID_GSR_TH_HW: {
			if (cfg_value > 255) {
				cfg_value = 255;
			}

			// Set Glitch Suppress Threshold Value as clock cycles.
			// The input signal change below than the threshold period would be treated as glitch
			// And the glitch would be ignored.
			nvt_ir_masked_write(ir, IR_TH1_REG, GSR_MASK, cfg_value << _GSR_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_ERROR_TH_HW: {
			if (cfg_value > 65535) {
				cfg_value = 65535;
			}
			// If the un-complete command stopping over the error threshold value.
			// It would trigger the controller error interrupt event.
			nvt_ir_masked_write(ir, IR_TH2_REG, ERR_TH_MASK, cfg_value);
		}
		break;

	case IR_CONFIG_ID_THRESHOLD_SEL: {
			// Set the signal logic 0/1 is judged by the distance of pulse or space.
			nvt_ir_masked_write(ir, IR_CTRL_REG, TH_SEL_BIT, cfg_value << _TH_SEL_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_DATA_LENGTH: {
			// Set the command length is 1~64 bits.
			nvt_ir_masked_write(ir, IR_CTRL_REG, LENGTH_MASK, cfg_value << _LENGTH_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_DATA_ORDER: {
			// Select the input command signal is LSb or MSb first.
			nvt_ir_masked_write(ir, IR_CTRL_REG, MSB_BIT, cfg_value << _MSB_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_HEADER_DETECT: {
			// Select the force header detection enable/disable.
			nvt_ir_masked_write(ir, IR_CTRL_REG, HEADER_METHOD_MASK, cfg_value << _HEADER_METHOD_MASK_SHIFT);
		}
		break;

	case IR_CONFIG_ID_INPUT_INVERSE: {
			// Select input inverse enable/disable
			nvt_ir_masked_write(ir, IR_CTRL_REG, INV_BIT, cfg_value << _INV_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_REPEAT_DET_EN: {
			// Select repeat code detection enable/disable
			nvt_ir_masked_write(ir, IR_CTRL_REG, REP_EN_BIT, cfg_value << _REP_EN_BIT_SHIFT);
		}
		break;

	case IR_CONFIG_ID_MATCH_LOW: {
			// Configure the MATCH command LSb 32 bits.
			// If the received command is matched with the MATCH command configuration,
			// it would trigger the MATCH interrupt status.
			// This function is designed to wakeup the CPU from sleeo mode
			// through the external remote control's power key.
			nvt_ir_masked_write(ir, IR_MATCH_DATA_LO_REG, MATCH_DATA_LO_MASK, cfg_value);
		}
		break;

	case IR_CONFIG_ID_MATCH_HIGH: {
			// Configure the MATCH command MSb 32 bits.
			// If the received command is matched with the MATCH command configuration,
			// it would trigger the MATCH interrupt status.
			// This function is designed to wakeup the CPU from sleeo mode
			// through the external remote control's power key.
			nvt_ir_masked_write(ir, IR_MATCH_DATA_HI_REG, MATCH_DATA_HI_MASK, cfg_value);
		}
		break;

	case IR_CONFIG_ID_WAKEUP_ENABLE: {
			// Select the wakeup CPU source interrupt enable
			//guiRemoteWakeupParam[REMOTE_WAKEUP_SRC] |= (uiCfgValue & REMOTE_INT_ALL);
		}
		break;

	case IR_CONFIG_ID_WAKEUP_DISABLE: {
			// Select the wakeup CPU source interrupt disable
			//guiRemoteWakeupParam[REMOTE_WAKEUP_SRC] &= ~(uiCfgValue & REMOTE_INT_ALL);
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_EN: {
			if (nvt_get_chip_id() == CHIP_NA51084) {
				// Select bi-phase mode enable/disable
				nvt_ir_masked_write(ir, IR_CTRL_REG, BI_PHASE_EN_BIT, cfg_value << _BI_PHASE_EN_BIT_SHIFT);
			} else {
				ir_err("the platform does not support bi-phase mode\n");
			}
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH: {
			// Set bi-phase mode to detect the long header pulse, such as RC6 protocol.
			if (nvt_ir_read(ir, IR_CTRL_REG) & BI_PHASE_EN_BIT) {
				nvt_ir_masked_write(ir, IR_CTRL_REG, BI_PHASE_DETECT_HEADER_TH_BIT, cfg_value << _BI_PHASE_DETECT_HEADER_TH_BIT_SHIFT);
			} else {
				ir_dbg("bi-phase mode is disabled, do nothing\n");
			}
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_HEADER_LENGTH: {
			// Set bi-phase header length, the unit is half cycle
			// EX: A 2T header with the following pattern should be described with match_length = 4
			//       |------|      |------|
			//       |      |      |      |
			// ------|      |------|      |
			//  1/2T   1/2T   1/2T   1/2T
			if (nvt_ir_read(ir, IR_CTRL_REG) & BI_PHASE_EN_BIT) {
				nvt_ir_masked_write(ir, IR_CTRL_REG, MATCH_LENGTH_MASK, cfg_value << _MATCH_LENGTH_MASK_SHIFT);
			} else {
				ir_dbg("bi-phase mode is disabled, do nothing\n");
			}
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW: {
			// Set bi-phase header match data low, each bit represents a level of half cycle, and the order is LSb first
			// EX: A 2T header with the following pattern should be described with match_data_l = 0xA (1010)
			//       |------|      |------|
			//       |      |      |      |
			// ------|      |------|      |
			//  1/2T   1/2T   1/2T   1/2T
			if (nvt_ir_read(ir, IR_CTRL_REG) & BI_PHASE_EN_BIT) {
				nvt_ir_masked_write(ir, IR_MATCH_DATA_LO_REG, MATCH_DATA_LO_MASK, cfg_value);
			} else {
				ir_dbg("bi-phase mode is disabled, do nothing\n");
			}
		}
		break;

	case IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH: {
			// Set bi-phase header match data high, each bit represents a level of half cycle, and the order is LSb first
			if (nvt_ir_read(ir, IR_CTRL_REG) & BI_PHASE_EN_BIT) {
				nvt_ir_masked_write(ir, IR_MATCH_DATA_HI_REG, MATCH_DATA_HI_MASK, cfg_value);
			} else {
				ir_dbg("bi-phase mode is disabled, do nothing\n");
			}
		}
		break;

	default:
		break;
	}
}

static void nvt_ir_decoder(struct work_struct *work)
{
	struct nvt_ir *ir = container_of(work, struct nvt_ir, decoder_work.work);
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

			ir_dbg("decoding NEC protocol: protocol(%d) raw(0x%x) scancode(0x%x), report %s\n", protocol, data, scancode, __ir_use_raw ? "raw" : "scancode");

			if (__ir_use_raw) {
				rc_keydown(ir->rc, protocol, data, 0);
			} else {
				rc_keydown(ir->rc, protocol, scancode, 0);
			}
		}
		break;

	case RC_PROTO_BIT_JVC: {
			u32 data = ir->data_lo;

			scancode = (bitrev8((data >> 8) & 0xff) << 8) |
				   (bitrev8((data >> 0) & 0xff) << 0);
			protocol = RC_PROTO_JVC;

			ir_dbg("decoding JVC protocol: protocol(%d) raw(0x%x) scancode(0x%x), report %s\n", protocol, data, scancode, __ir_use_raw ? "raw" : "scancode");

			if (__ir_use_raw) {
				rc_keydown(ir->rc, protocol, data, 0);
			} else {
				rc_keydown(ir->rc, protocol, scancode, 0);
			}
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

			ir_dbg("decoding Sharp protocol: protocol(%d) raw(0x%x) scancode(0x%x), report %s\n", protocol, data, scancode, __ir_use_raw ? "raw" : "scancode");

			if (__ir_use_raw) {
				rc_keydown(ir->rc, protocol, data, 0);
			} else {
				rc_keydown(ir->rc, protocol, scancode, 0);
			}
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

			ir_dbg("decoding RC5 protocol: protocol(%d) raw(0x%x) scancode(0x%x), report %s\n", protocol, data, scancode, __ir_use_raw ? "raw" : "scancode");

			if (__ir_use_raw) {
				rc_keydown(ir->rc, protocol, data, 0);
			} else {
				rc_keydown(ir->rc, protocol, scancode, 0);
			}
		}
		break;

	case RC_PROTO_BIT_OTHER: {
			u64 data = (u64)ir->data_lo + ((u64)ir->data_hi << 32);
			u64 digit = 0;  /* D00 to D39 */
			u64 sn = 0;     /* D08 to D39 */
			int k = 39;     /* index for digit */
			int i = 0;      /* index for data */

			while (k >= 0) {
				if ((i + 1) % 5 == 0) {
					/* skip parity bit */
					i++;
					continue;
				}

				if (data & ((u64)1 << i)) {
					digit = digit + ((u64)1 << k);  /* reverse data */
				}

				k--;
				i++;
			}

			sn = digit & (u64)0xFFFFFFFF;  /* skip D00 to D07 */

			scancode = (u32)sn;

			ir_dbg("decoding EM4095 protocol: protocol(%d) raw(0x%llx) sn(%010lld) scancode(0x%x), report %s\n", protocol, data, sn, scancode, __ir_use_raw ? "raw" : "scancode");

			if (__ir_use_raw) {
				rc_keydown(ir->rc, protocol, data, 0);
			} else {
				rc_keydown(ir->rc, protocol, scancode, 0);
			}
		}
		break;

	default:
		ir_err("decoding as unhandled protocol, rc_type(0x%llx)\n", ir->rc_type);

		break;
	}

	if (ir_first) {
		ir_first = false;

		mdelay(__ir_first_delay);
	}

	nvt_ir_enable(ir);
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

		if (nvt_ir_read(ir, IR_CTRL_REG) & BI_PHASE_EN_BIT) {
			ir_isr_dbg("RD interrupt, data_lo(0x%x) data_hi(0x%x) raw_lo(0x%x) raw_hi(0x%x)\n", ir->data_lo, ir->data_hi,
				nvt_ir_read(ir, IR_RAW_DATA_LO_REG), nvt_ir_read(ir, IR_RAW_DATA_HI_REG));
		} else {
			ir_isr_dbg("RD interrupt, data_lo(0x%x) data_hi(0x%x)\n", ir->data_lo, ir->data_hi);
		}

		ir_first = true;

		nvt_ir_disable(ir);

		/* Decode raw data to scancode */
		schedule_delayed_work(&ir->decoder_work, msecs_to_jiffies(0));
	} else if (sts_r & REP_STS_BIT) {
		if (!__ir_repeat_is_data) {
			ir_isr_dbg("REP interrupt\n");
		} else {
			/* Get the raw data */
			ir->data_lo = nvt_ir_read(ir, IR_DATA_LO_REG);
			ir->data_hi = nvt_ir_read(ir, IR_DATA_HI_REG);

			ir_isr_dbg("REP interrupt, data_lo(0x%x) data_hi(0x%x)\n", ir->data_lo, ir->data_hi);

			nvt_ir_disable(ir);

			/* Decode raw data to scancode */
			schedule_delayed_work(&ir->decoder_work, msecs_to_jiffies(__ir_repeat_delay));
		}
	}

	if (sts_r & ERR_STS_BIT) {
		ir_isr_dbg("TOUT interrupt\n");
	}

	if (sts_r & MATCH_STS_BIT) {
		ir_isr_dbg("MATCH interrupt\n");
	}

	if (sts_r & OVERRUN_STS_BIT) {
		ir_isr_dbg("OVERRUN interrupt\n");
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
		if (pinmux_config->config) {
			ir_dbg("pinmux(0x%x)\n", pinmux_config->config);
		} else {
			ir_err("pinmux is not set\n");
		}
	}

	switch (ir->rc_type) {
	case (RC_PROTO_BIT_NEC + RC_PROTO_BIT_NECX + RC_PROTO_BIT_NEC32):
		ir_dbg("configuring IR receiver for NEC protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_SRC_SEL, IR_CLK_SRC_RTC);
		nvt_ir_config(ir, IR_CONFIG_ID_THRESHOLD_SEL, IR_SPACE_DET);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_FORCE_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_REPEAT_DET_EN, true);
		nvt_ir_config(ir, IR_CONFIG_ID_LOGIC_TH, 1100);
		nvt_ir_config(ir, IR_CONFIG_ID_ERROR_TH, 20000);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_TH, 6000);
		nvt_ir_config(ir, IR_CONFIG_ID_GSR_TH, 62);
		nvt_ir_config(ir, IR_CONFIG_ID_REPEAT_TH, 3000);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 32);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, __ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_EN, false);

		nvt_ir_interrupt_enable(ir, IR_INT_RD | IR_INT_ERR | IR_INT_REP | IR_INT_OVERRUN);
		nvt_ir_enable(ir);

		break;

	case RC_PROTO_BIT_JVC:
		ir_dbg("configuring IR receiver for JVC protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_SRC_SEL, IR_CLK_SRC_RTC);
		nvt_ir_config(ir, IR_CONFIG_ID_THRESHOLD_SEL, IR_SPACE_DET);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_FORCE_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_REPEAT_DET_EN, false);
		nvt_ir_config(ir, IR_CONFIG_ID_LOGIC_TH, 1100);
		nvt_ir_config(ir, IR_CONFIG_ID_ERROR_TH, 15000);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_TH, 6000);
		nvt_ir_config(ir, IR_CONFIG_ID_GSR_TH, 62);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 16);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, __ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_EN, false);

		nvt_ir_interrupt_enable(ir, IR_INT_RD | IR_INT_ERR | IR_INT_REP | IR_INT_OVERRUN);
		nvt_ir_enable(ir);

		break;

	case RC_PROTO_BIT_SHARP:
		ir_dbg("configuring IR receiver for Sharp protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_SRC_SEL, IR_CLK_SRC_RTC);
		nvt_ir_config(ir, IR_CONFIG_ID_THRESHOLD_SEL, IR_SPACE_DET);
		nvt_ir_config(ir, IR_CONFIG_ID_HEADER_DETECT, IR_NO_DETECT_HEADER);
		nvt_ir_config(ir, IR_CONFIG_ID_REPEAT_DET_EN, false);
		nvt_ir_config(ir, IR_CONFIG_ID_LOGIC_TH, 1100);
		nvt_ir_config(ir, IR_CONFIG_ID_ERROR_TH, 10000);
		nvt_ir_config(ir, IR_CONFIG_ID_GSR_TH, 62);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 13);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, __ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, ir_input_inverse);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_EN, false);

		nvt_ir_interrupt_enable(ir, IR_INT_RD | IR_INT_ERR | IR_INT_REP | IR_INT_OVERRUN);
		nvt_ir_enable(ir);

		break;

	case (RC_PROTO_BIT_RC5 + RC_PROTO_BIT_RC5X_20):
		ir_dbg("configuring IR receiver for RC5 protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_SRC_SEL, IR_CLK_SRC_RTC);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 11);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, __ir_data_order);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_EN, true);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, 0);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH, false);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_LENGTH, 6); /* including 2 start and 1 toggle */
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW, 0x1A); /* bit0-5 010110 then bit5-0 011010 = 0x1A */
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH, 0);
		nvt_ir_config(ir, IR_CONFIG_ID_LOGIC_TH_HW, 23); /* accepted range is 19 to 28, 23*30.52=701.96us */
		nvt_ir_config(ir, IR_CONFIG_ID_ERROR_TH, 20000);
		nvt_ir_config(ir, IR_CONFIG_ID_GSR_TH, 62);

		nvt_ir_interrupt_enable(ir, IR_INT_RD | IR_INT_ERR | IR_INT_REP | IR_INT_OVERRUN);
		nvt_ir_enable(ir);

		break;

	case RC_PROTO_BIT_OTHER:
		ir_dbg("configuring IR receiver for EM4095 protocol, rc_type(0x%llx)\n", ir->rc_type);

		nvt_ir_interrupt_disable(ir, IR_INT_ALL);
		nvt_ir_disable(ir);

		nvt_ir_config(ir, IR_CONFIG_ID_CLK_SRC_SEL, IR_CLK_SRC_3M);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_LENGTH, 55);
		nvt_ir_config(ir, IR_CONFIG_ID_DATA_ORDER, IR_DATA_LSB);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_EN, true);
		nvt_ir_config(ir, IR_CONFIG_ID_INPUT_INVERSE, 1);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH, false);
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_LENGTH, 16); /* including 8 logic1 */
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW, 0x5555); /* bit0-15 1010101010101010 then bit15-0 0101010101010101 = 0x5555 */
		nvt_ir_config(ir, IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH, 0);
		nvt_ir_config(ir, IR_CONFIG_ID_LOGIC_TH_HW, 598); /* accepted range is 545 to 650, 598*0.333=199.33us */
		nvt_ir_config(ir, IR_CONFIG_ID_ERROR_TH_HW, 24000); /* 24000*0.333=8000us */
		nvt_ir_config(ir, IR_CONFIG_ID_GSR_TH_HW, 50); /* 50*0.333=16.66us */

		nvt_ir_interrupt_enable(ir, IR_INT_RD | IR_INT_ERR | IR_INT_REP | IR_INT_OVERRUN);
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

	INIT_DELAYED_WORK(&ir->decoder_work, nvt_ir_decoder);

	if (nvt_get_chip_id() == CHIP_NA51084) {
		clk_sel0 = clk_get(NULL, "remote_clk_sel0");
		if (IS_ERR(clk_sel0)) {
			ir_err("clk remote_clk_sel0 not found\n");
		}

		clk_sel1 = clk_get(NULL, "remote_clk_sel1");
		if (IS_ERR(clk_sel1)) {
			ir_err("clk remote_clk_sel1 not found\n");
		}
	} else {
		clk_sel0 = clk_get(NULL, "fix32.768k");
		if (IS_ERR(clk_sel0)) {
			ir_err("clk fix32.768k not found\n");
		}

		clk_sel1 = clk_get(NULL, "fix32k");
		if (IS_ERR(clk_sel1)) {
			ir_err("clk fix32k not found\n");
		}
	}
}
