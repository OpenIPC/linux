/*
 * fh_sadc.h
 *
 *  Created on: Mar 13, 2015
 *      Author: duobao
 */

#ifndef FH_SADC_H_
#define FH_SADC_H_

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

/****************************************************************************
 * #define section
 *	add constant #define here if any
 ***************************************************************************/
#define FH_SADC_PROC_FILE    "driver/sadc"
#define MAX_CHANNEL_NO		(8)
#define LOOP_MODE			(0x55)
#define ISR_MODE			(0xAA)


#define SADC_TIMEOUT		0x55
/****************************************************************************
 * ADT section
 *	add Abstract Data Type definition here
 ***************************************************************************/

struct wrap_sadc_reg {
	u32 sadc_model;
	u32 sadc_cmd;
	u32 sadc_control;
	u32 sadc_scan_en;
	u32 sadc_chn_cfg;
	u32 sadc_chn_cfg1;
	u32 sadc_glitch_cfg;
	u32 sadc_continuous_time;
	u32 sadc_glitch_time;
	u32 sadc_ier;
	u32 sadc_int_status;
	u32 sadc_dout0;
	u32 sadc_dout1;
	u32 sadc_dout2;
	u32 sadc_dout3;
	u32 sadc_dout0_all;
	u32 sadc_dout1_all;
	u32 sadc_dout2_all;
	u32 sadc_dout3_all;
	u32 sadc_debuge0;
	u32 sadc_status;
	u32 sadc_cnt;
	u32 sadc_timeout;
	u32 sadc_status2;
#ifdef CONFIG_FH_SADC_V21
	u32 sadc_hit_value0;
	u32 sadc_hit_value1;
	u32 sadc_hit_value2;
	u32 sadc_hit_value3;
	u32 sadc_hit_cfg;
	u32 sadc_hit_data0;
	u32 sadc_hit_data1;
	u32 sadc_hit_data2;
	u32 sadc_hit_data3;
#endif
};


struct wrap_sadc_obj {
	void *regs;
	u32 irq_no;
	u32 active_channel_no;
	u32 active_channel_status;
	uint16_t channel_data[MAX_CHANNEL_NO];
	u32 error_rec;
	u32 en_isr;
	u32 sample_mode;
	spinlock_t lock;
	struct mutex		sadc_lock;
	struct completion done;
	struct proc_dir_entry *proc_file;
	wait_queue_head_t readqueue;
	u32 activebit;
	u32 refvol;
	struct clk *clk;
	char isr_name[10];
};

long fh_sadc_get_value(int channel);


#endif /* fh_SADC_H_ */
