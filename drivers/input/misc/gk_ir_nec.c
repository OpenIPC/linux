/*
 * drivers/input/misc/gk_ir_nec.c
 *
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include <plat/input.h>

/**
 * Pulse-Width-Coded Signals vary the length of pulses to code the information.
 * In this case if the pulse width is short (approximately 550us) it
 * corresponds to a logical zero or a low. If the pulse width is long
 * (approximately 2200us) it corresponds to a logical one or a high.
 *
 *         +--+  +--+  +----+  +----+
 *         |  |  |  |  |    |  |    |
 *      ---+  +--+  +--+    +--+    +---
 *          0     0      1       1
 */

/* NEC - APEX - HITACHI - PIONEER */
#define NEC_DEFAULT_FREQUENCY		36000	/* 36KHz */
#define NEC_DEFAULT_SMALLER_TIME	560	/* T, 560 microseconds. */

/** bit 0 [1120us]
 *      ---+    +----+
 *         |    |    |
 *         +----+    +---
 *           -T   +T
 */

/** bit 1 [2240us]
 *      ---+    +------------+
 *         |    |            |
 *         +----+            +---
 *           -T      +3T
 */

/** start [13.3ms]
 *      ---+                                +---------------+
 *         |                                |               |
 *         +--------------------------------+               +---
 *                     -16T(9ms)              +7.5T(4.2ms)
 */

/** Subsequent Frame [11.3ms]
 *      ---+                                +--------+  +---
 *         |                                |        |  |
 *         +--------------------------------+        +--+
 *                     -16T(9ms)            +4T(2.2ms)
 */

#define NEC_LEADER_LOW_UPBOUND		123	/* default 9ms   */
#define NEC_LEADER_LOW_LOWBOUND		113
#define NEC_LEADER_HIGH_UPBOUND		63	/* default 4.2ms */
#define NEC_LEADER_HIGH_LOWBOUND	52

#define SAM_LEADER_LOW_UPBOUND		64	/* default 4.5ms */
#define SAM_LEADER_LOW_LOWBOUND		50
#define SAM_LEADER_HIGH_UPBOUND		64	/* default 4.5ms */
#define SAM_LEADER_HIGH_LOWBOUND	50

#define NEC_REPEAT_LOW_UPBOUND		123	/* default 9ms   */
#define NEC_REPEAT_LOW_LOWBOUND		113
#define NEC_REPEAT_HIGH_UPBOUND		33	/* default 2.2ms */
#define NEC_REPEAT_HIGH_LOWBOUND	23

#define NEC_DATA_LOW_UPBOUND		12	/* default 560us  */
#define NEC_DATA_LOW_LOWBOUND		1
#define NEC_DATA_0_HIGH_UPBOUND		12	/* default 560us  */
#define NEC_DATA_0_HIGH_LOWBOUND	1
#define NEC_DATA_1_HIGH_UPBOUND		26	/* default 1680us */
#define NEC_DATA_1_HIGH_LOWBOUND	15

/**
 * Check the waveform data is leader code or not.
 */
static int gk_ir_nec_pulse_leader_code(struct gk_ir_info *pinfo)
{
	int				check_sam = 0;

	u16 val = gk_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < NEC_LEADER_LOW_UPBOUND)  &&
	    (val > NEC_LEADER_LOW_LOWBOUND)) {
	} else {
		if ((val < SAM_LEADER_LOW_UPBOUND)  &&
		    (val > SAM_LEADER_LOW_LOWBOUND)) {
		    check_sam = 1;
		} else {
			return 0;
		}
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = gk_ir_read_data(pinfo, 0);
	} else {
		val = gk_ir_read_data(pinfo, pinfo->ir_pread + 1);
	}

	if (check_sam) {
		if ((val < SAM_LEADER_HIGH_UPBOUND) &&
		    (val > SAM_LEADER_HIGH_LOWBOUND) )
			return 1;
		else
			return 0;
	} else {
		if ((val < NEC_LEADER_HIGH_UPBOUND) &&
		    (val > NEC_LEADER_HIGH_LOWBOUND) )
			return 1;
		else
			return 0;
	}
}

static int gk_ir_nec_find_head(struct gk_ir_info *pinfo)
{
	int i, val = 0;

	i = gk_ir_get_tick_size(pinfo) - pinfo->frame_info.frame_head_size + 1;

	while(i--) {
		if(gk_ir_nec_pulse_leader_code(pinfo)) {
			dev_dbg(&pinfo->pinput_dev->dev, "find leader code, i [%d]\n", i);
			val = 1;
			break;
		} else {
			dev_dbg(&pinfo->pinput_dev->dev, "didn't  find leader code, i [%d]\n", i);
			gk_ir_move_read_ptr(pinfo, 1);
		}
	}

	return val ;
}
/**
 * Check the waveform data is subsequent code or not.
 */
static int gk_ir_nec_pulse_subsequent_code(struct gk_ir_info *pinfo)
{
	u16 val = gk_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < NEC_REPEAT_LOW_UPBOUND)  &&
	    (val > NEC_REPEAT_LOW_LOWBOUND)) {
	} else {
		return 0;
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = gk_ir_read_data(pinfo, 0);
	} else {
		val = gk_ir_read_data(pinfo, pinfo->ir_pread + 1);
	}

	if ((val < NEC_REPEAT_HIGH_UPBOUND) &&
	    (val > NEC_REPEAT_HIGH_LOWBOUND) )
		return 1;
	else
		return 0;
}

static int gk_ir_nec_find_subsequent(struct gk_ir_info *pinfo)
{
	int i, val = 0;

	i = gk_ir_get_tick_size(pinfo) - pinfo->frame_info.frame_head_size + 1;

	while(i--) {
		if(gk_ir_nec_pulse_subsequent_code(pinfo)) {
			dev_dbg(&pinfo->pinput_dev->dev, "find leader code, i [%d]\n", i);
			val = 1;
			break;
		} else {
			dev_dbg(&pinfo->pinput_dev->dev, "didn't  find leader code, i [%d]\n", i);
			gk_ir_move_read_ptr(pinfo, 1);
		}
	}

	return val ;
}

/**
 * Check the waveform data is 0 bit or not.
 */
static int gk_ir_nec_pulse_code_0(struct gk_ir_info *pinfo)
{
	u16 val = gk_ir_read_data(pinfo, pinfo->ir_pread);
	if ((val < NEC_DATA_LOW_UPBOUND)  &&
	    (val > NEC_DATA_LOW_LOWBOUND)) {
	} else {
		return 0;
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = gk_ir_read_data(pinfo, 0);
	} else {
		val = gk_ir_read_data(pinfo, pinfo->ir_pread + 1);
	}

	if ((val < NEC_DATA_0_HIGH_UPBOUND) &&
	    (val > NEC_DATA_0_HIGH_LOWBOUND) )
		return 1;
	else
		return 0;
}

/**
 * Check the waveform data is 1 bit or not.
 */
static int gk_ir_nec_pulse_code_1(struct gk_ir_info *pinfo)
{
	u16 val = gk_ir_read_data(pinfo, pinfo->ir_pread);

	if ((val < NEC_DATA_LOW_UPBOUND)  &&
	    (val > NEC_DATA_LOW_LOWBOUND)) {
	} else {
		return 0;
	}

	if ((pinfo->ir_pread + 1) >= MAX_IR_BUFFER) {
		val = gk_ir_read_data(pinfo, 0);
	} else {
		val = gk_ir_read_data(pinfo, pinfo->ir_pread + 1);
	}

	if ((val < NEC_DATA_1_HIGH_UPBOUND) &&
	    (val > NEC_DATA_1_HIGH_LOWBOUND) )
		return 1;
	else
		return 0;
}

static int gk_ir_nec_pulse_data_translate(struct gk_ir_info *pinfo, u8 * data)
{
	int i;
	*data = 0;

	for (i = 7; i >= 0; i--) {
		if (gk_ir_nec_pulse_code_0(pinfo)) {

		} else if (gk_ir_nec_pulse_code_1(pinfo)) {
			*data |= 1 << i;
		} else {
			dev_dbg(&pinfo->pinput_dev->dev, "%d ERROR, the waveform can't match\n", i);
			return -1;
		}
		gk_ir_move_read_ptr(pinfo, 2);
	}

	return 0;
}

static int gk_ir_nec_pulse_decode(struct gk_ir_info *pinfo, u32 *uid)
{
	u8 addr = 0, inv_addr = 0, data = 0, inv_data = 0;
	int rval;

	/* Then follows 32 bits of data, broken down in 4 bytes of 8 bits. */

	/* The first 8 bits is the Address. */
	rval = gk_ir_nec_pulse_data_translate(pinfo, &addr);
	if (rval < 0)
		return rval;

	/* The second 8 bits is the Address Complement. */
	rval = gk_ir_nec_pulse_data_translate(pinfo, &inv_addr);
	if (rval < 0)
		return rval;

	/* The third 8 bits is the data. */
	rval = gk_ir_nec_pulse_data_translate(pinfo, &data);
	if (rval < 0)
		return rval;

	/* The fourth 8 bits is the data Complement. */
	rval = gk_ir_nec_pulse_data_translate(pinfo, &inv_data);
	if (rval < 0)
		return rval;

	dev_dbg(&pinfo->pinput_dev->dev, "addr\tinv_addr\tdata\tinv_data\n");
	dev_dbg(&pinfo->pinput_dev->dev, "0x%x\t0x%x\t\t0x%x\t0x%x\n", addr, inv_addr, data, inv_data);

	*uid = (addr << 24) | (inv_addr << 16) | (data << 8) | inv_data;

	return 0;
}

int gk_ir_nec_parse(struct gk_ir_info *pinfo, u32 *uid)
{
	int				rval;
	int				cur_ptr = pinfo->ir_pread;

	if ((gk_ir_nec_find_head(pinfo) || (gk_ir_nec_find_subsequent(pinfo)))
		&& gk_ir_get_tick_size(pinfo) >= pinfo->frame_info.frame_data_size
		+ pinfo->frame_info.frame_head_size) 
    {
		dev_dbg(&pinfo->pinput_dev->dev, "go to decode statge\n");
		gk_ir_move_read_ptr(pinfo, pinfo->frame_info.frame_head_size);//move ptr to data
		rval = gk_ir_nec_pulse_decode(pinfo, uid);
	} else {
		return -1;
	}

	if (rval >= 0) {
		dev_dbg(&pinfo->pinput_dev->dev, "buffer[%d]-->mornal key\n", cur_ptr);
		return 0;
	}

	return (-1);
}

void gk_ir_get_nec_info(struct gk_ir_frame_info *pframe_info)
{
	pframe_info->frame_head_size = 2;
	pframe_info->frame_data_size = 64;
	pframe_info->frame_end_size = 1;
	pframe_info->frame_repeat_head_size = 4;
}

