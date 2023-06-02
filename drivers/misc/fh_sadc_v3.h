/*
 * fh_sadc.h
 *
 *  Created on: Mar 13, 2015
 *      Author: duobao
 */

#ifndef FH_SADC_V3_H_
#define FH_SADC_V3_H_

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

#define SADC_CTRL0			0x0
#define SADC_CTRL1			0x4
#define SADC_CTRL2			0x8
#define SADC_CONFIG0		0x10
#define SADC_CONFIG1		0x14
#define SADC_CONFIG2		0x18
#define SADC_CONFIG3		0x1c
#define SADC_CONFIG4		0x20
#define SADC_CONFIG5		0x24
#define SADC_CONFIG6		0x28
#define SADC_CONFIG7		0x2c
#define SADC_INT_EN			0x40
#define SADC_INT_STA		0x44
#define SADC_DOUT0			0x50
#define SADC_DOUT1			0x54
#define SADC_DOUT2			0x58
#define SADC_DOUT3			0x5c
#define SADC_BUTTON_DOUT0	0x60
#define SADC_BUTTON_DOUT1	0x64
#define SADC_BUTTON_DOUT2	0x68
#define SADC_BUTTON_DOUT3	0x6c
#define SADC_DEBUG0			0x100
#define SADC_DEBUG1			0x104
#define SADC_DEBUG2			0x108
#define SADC_DEBUG3			0x10c
#define SADC_DEBUG4			0x110
#define SADC_DEBUG5			0x114
#define SADC_DEBUG6			0x118
#define SADC_ECO			0x120

struct wrap_sadc_obj {
	void *regs;
	u32 irq_no;
	u32 frequency;
	u32 active_channel_no;
	uint16_t channel_data[MAX_CHANNEL_NO];
	spinlock_t lock;
	struct mutex		sadc_lock;
	struct completion done;
	struct proc_dir_entry *proc_file;
	u32 activebit;
	u32 refvol;
	struct clk  *clk;
	char isr_name[10];
};

long fh_sadc_get_value(int channel);


#endif /* fh_SADC_V3_H_ */
