/*
 * drivers/input/misc/ambarella_ir_sony.c
 *
 * History:
 *      2007/03/28 - [Dragon Chiang] created file
 *	2009/03/10 - [Anthony Ginger] Port to 2.6.28
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

#include <linux/platform_device.h>
#include <linux/interrupt.h>

/**
 * Space-Coded Signals (REC-80) vary the length of the spaces between pulses
 * to code the information. In this case if the space width is short
 * (approximately 550us) it corresponds to a logical zero or a low. If the
 * space width is long (approximately 1650us) it corresponds to a logical one
 * or a high.
 *
 *      ---+  +--+  +--+    +--+    +---
 *         |  |  |  |  |    |  |    |
 *         +--+  +--+  +----+  +----+
 *          0     0      1       1
 */

/* Sony 12-bit, 15-bit and 20-bit versions of the protocol exist (12-bit processed here) */
#define SONY_DEFAULT_FREQUENCY		36000	/* 36KHz */
#define SONY_DEFAULT_SMALLER_TIME	600	/* T, 600 microseconds. */

/** bit 0 [1200us]
 *      ---+    +----+
 *         |    |    |
 *         +----+    +---
 *           -T   +T
 */

/** bit 1 [1800us]
 *      ---+    +--------+
 *         |    |        |
 *         +----+        +---
 *           -T    +2T
 */

/** start [1800us]
 *         +------------------------------+
 *         |                              |
 *   ------+                              +-----------
 *                   -3T(1800us)
 */

/**
 * If you hold the remote button pressed, the whole transmited frame
 * repeats every 25ms.
 */

#define SONY_LEADER_LOW_UPBOUND		33	/* default 1800us */
#define SONY_LEADER_LOW_LOWBOUND	28

#define SONY_DATA_LOW_UPBOUND		9	/* default 560us  */
#define SONY_DATA_LOW_LOWBOUND		4
#define SONY_DATA_0_HIGH_UPBOUND	9	/* default 560us  */
#define SONY_DATA_0_HIGH_LOWBOUND	4
#define SONY_DATA_1_HIGH_UPBOUND	18	/* default 1680us */
#define SONY_DATA_1_HIGH_LOWBOUND	12

/**
 * Check the waveform data is leader code or not.
 */
static int ambarella_ir_sony_space_leader_code(struct ambarella_ir_info *pinfo)
{
	u16 val = ambarella_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < SONY_LEADER_LOW_UPBOUND) &&
	    (val > SONY_LEADER_LOW_LOWBOUND) )
		return 1;	/* leader code */
	else
		return 0;
}

static int ambarella_ir_sony_find_head(struct ambarella_ir_info *pinfo)
{
	int i, val = 0;

	i = ambarella_ir_get_tick_size(pinfo) - pinfo->frame_info.frame_head_size + 1;

	while(i--) {
		if(ambarella_ir_sony_space_leader_code(pinfo)) {
			dev_dbg(&pinfo->input->dev, "find leader code, i [%d]\n", i);
			val = 1;
			break;
		} else {
			dev_dbg(&pinfo->input->dev, "didn't  find leader code, i [%d]\n", i);
			ambarella_ir_move_read_ptr(pinfo, 1);
		}
	}

	return val ;
}

/**
 * Check the waveform data is 0 bit or not.
 */
static int ambarella_ir_sony_space_code_0(struct ambarella_ir_info *pinfo)
{
	/* 500us of Silence + 700us of IR for bits ZERO, */
	u16 val = ambarella_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < SONY_DATA_LOW_UPBOUND) &&
	    (val > SONY_DATA_LOW_LOWBOUND)) {
	} else {
		return 0;
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = ambarella_ir_read_data(pinfo, 0);
		if ((val < SONY_DATA_0_HIGH_UPBOUND) &&
		    (val > SONY_DATA_0_HIGH_LOWBOUND) )
			return 1;	/* code 0 */
		else
			return 0;
	} else {
		val = ambarella_ir_read_data(pinfo, pinfo->ir_pread + 1);
		if ((val < SONY_DATA_0_HIGH_UPBOUND) &&
		    (val > SONY_DATA_0_HIGH_LOWBOUND) )
			return 1;	/* code 0 */
		else
			return 0;
	}
}

/**
 * Check the waveform data is 1 bit or not.
 */
static int ambarella_ir_sony_space_code_1(struct ambarella_ir_info *pinfo)
{
	/* 500us of Silence + 1300us of IR for bits ONE. */
	u16 val = ambarella_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < SONY_DATA_LOW_UPBOUND) &&
	    (val > SONY_DATA_LOW_LOWBOUND)) {
	} else {
		return 0;
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = ambarella_ir_read_data(pinfo, 0);
		if ((val < SONY_DATA_1_HIGH_UPBOUND) &&
		    (val > SONY_DATA_1_HIGH_LOWBOUND) )
			return 1;	/* code 1 */
		else
			return 0;
	} else {
		val = ambarella_ir_read_data(pinfo, pinfo->ir_pread + 1);
		if ((val < SONY_DATA_1_HIGH_UPBOUND) &&
		    (val > SONY_DATA_1_HIGH_LOWBOUND) )
			return 1;	/* code 1 */
		else
			return 0;
	}
}

/**
 * Translate waveform data to useful message.
 */
static int ambarella_ir_sony_space_decode(struct ambarella_ir_info *pinfo, u32 *uid)
{
	/* Following the header you will find straight 12 bits.
	   The first immediate bit after the START is the LSB of the 12 bits.
	   Lets name this first bit as B0, the Last will be B12.
	   B0 to B6 form the 7 bits for the Command Code.
	   B8 to B11 form the 5 bits for the Device Address.
	*/

	int i;

	u8 addr = 0, data = 0;

	/* command - 7 bits*/
	for (i = 0; i < 7; i++) {
		if (ambarella_ir_sony_space_code_0(pinfo)) {
		}
		else if (ambarella_ir_sony_space_code_1(pinfo)) {
			data |= 1 << i;
		}
		else {
			dev_dbg(&pinfo->input->dev, "%d ERROR, the waveform can't match", i);
		}
		ambarella_ir_move_read_ptr(pinfo, 2);
	}

	/* device address - 5 bits */
	for (i = 0; i < 5; i++) {
		if (ambarella_ir_sony_space_code_0(pinfo)) {
		}
		else if (ambarella_ir_sony_space_code_1(pinfo)) {
			addr |= 1 << i;
		}
		else {
			dev_dbg(&pinfo->input->dev, "%d ERROR, the waveform can't match", i);
		}
		ambarella_ir_move_read_ptr(pinfo, 2);
	}

	*uid = (addr << 16) | data;

	return 0;
}

int ambarella_ir_sony_parse(struct ambarella_ir_info *pinfo, u32 *uid)
{
	int				rval;
	int				cur_ptr = pinfo->ir_pread;

	if (ambarella_ir_sony_find_head(pinfo)
		&& ambarella_ir_get_tick_size(pinfo) >= pinfo->frame_info.frame_data_size
		+ pinfo->frame_info.frame_head_size) {

		dev_dbg(&pinfo->input->dev, "go to decode statge\n");
		ambarella_ir_move_read_ptr(pinfo, pinfo->frame_info.frame_head_size);//move ptr to data
		rval = ambarella_ir_sony_space_decode(pinfo, uid);
	} else {
		return -1;
	}

	if (rval >= 0) {
		dev_dbg(&pinfo->input->dev, "buffer[%d]-->mornal key\n", cur_ptr);
		return 0;
	}

	return (-1);
}

void ambarella_ir_get_sony_info(struct ambarella_ir_frame_info *pframe_info)
{
	pframe_info->frame_head_size 	= 1;
	pframe_info->frame_data_size 	= 24;
	pframe_info->frame_end_size	= 1;
	pframe_info->frame_repeat_head_size	= 0;
}

