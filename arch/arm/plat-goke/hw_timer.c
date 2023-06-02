/*
 * arch/arm/mach-gk/hw_timer.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
 *
 * * This program is free software; you can redistribute it and/or modify
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
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/i2c.h>

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>


#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/localtimer.h>
#include <asm/sched_clock.h>

#include <linux/clk.h>
#include <asm/dma.h>
#include <asm/uaccess.h>

#include <mach/hardware.h>
#include <mach/timer.h>
#include <mach/io.h>
#include <mach/uart.h>
#include <mach/rct.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE


#define GK_HWTIMER_NAME 				"hwtimer"

#define GK_HWTIMER_STATUS_REG		TIMER1_STATUS_REG
#define GK_HWTIMER_RELOAD_REG		TIMER1_RELOAD_REG
#define GK_HWTIMER_MATCH1_REG		TIMER1_MATCH1_REG
#define GK_HWTIMER_MATCH2_REG		TIMER1_MATCH2_REG
#define GK_HWTIMER_CTR_EN			TIMER_CTR_EN1
#define GK_HWTIMER_CTR_OF			TIMER_CTR_OF1
#define GK_HWTIMER_CTR_CSL			TIMER_CTR_CSL1
#define GK_HWTIMER_CTR_MASK			0x0000000F

#define GK_HWTIMER_IRQ				TIMER1_IRQ

#define HWTIMER_INPUT_FREQ					(get_apb_bus_freq_hz())
#define HWTIMER_OUTPUT_FREQ					90000

typedef struct hw_timer_state_s
{
	u32		enable_flag;
	u32		reg_value;
	u64		overflow_count;
	u64		overflow_value;
	u64		output_ticks;
}hw_timer_state_t;

static u64 hwtimer_overflow_count = 0;
static u64 hwtimer_init_output_value = 0;
static u32 hwtimer_init_overflow_value = 0;
static u32 hwtimer_input_freq_unchanged = 1;

static hw_timer_state_t timer_state;

static spinlock_t timer_isr_lock;

static int hwtimer_reset(void);

static inline void gk_hwtimer_disable(void)
{
	gk_timer_clrbitsl(TIMER_CTR_REG, GK_HWTIMER_CTR_EN);
}

static inline void gk_hwtimer_enable(void)
{
	gk_timer_setbitsl(TIMER_CTR_REG, GK_HWTIMER_CTR_EN);
}

static inline void gk_hwtimer_misc(void)
{
	gk_timer_setbitsl(TIMER_CTR_REG, GK_HWTIMER_CTR_OF);
	gk_timer_clrbitsl(TIMER_CTR_REG, GK_HWTIMER_CTR_CSL);
}

static inline void gk_hwtimer_config(void)
{
	if (hwtimer_init_overflow_value) {
		gk_timer_writel(GK_HWTIMER_STATUS_REG, hwtimer_init_overflow_value);
		gk_timer_writel(GK_HWTIMER_RELOAD_REG, hwtimer_init_overflow_value);
	} else {
		hwtimer_init_overflow_value = HWTIMER_INPUT_FREQ;
		gk_timer_writel(GK_HWTIMER_STATUS_REG, hwtimer_init_overflow_value);
		gk_timer_writel(GK_HWTIMER_RELOAD_REG, hwtimer_init_overflow_value);
	}

	gk_timer_writel(GK_HWTIMER_MATCH1_REG, 0x0);
	gk_timer_writel(GK_HWTIMER_MATCH2_REG, 0x0);
	gk_hwtimer_misc();
}

static inline void gk_hwtimer_init(void)
{
	gk_hwtimer_disable();
	gk_hwtimer_config();
	gk_hwtimer_enable();
	timer_state.enable_flag = 1;
}

static u32 gk_hwtimer_read(void)
{
	return (u32)gk_timer_readl(GK_HWTIMER_STATUS_REG);
}

static inline int get_gcd(int a, int b)
{
	if ((a == 0) || (b == 0)) {
		printk("wrong input for gcd \n");
		return 1;
	}
	while ((a != 0) && (b != 0)) {
		if (a > b) {
			a = a % b;
		} else {
			b = b % a;
		}
	}
	return (a == 0) ? b : a;
}

static int calc_output_ticks(void)
{
	u64 total_ticks;
	u64 overflow_ticks;
	u64 hwtimer_ticks;
	u32 curr_reg_value;
	int output_freq = HWTIMER_OUTPUT_FREQ;

	int gcd;

	if (hwtimer_input_freq_unchanged) {
		curr_reg_value = gk_hwtimer_read();

		overflow_ticks = hwtimer_overflow_count * hwtimer_init_overflow_value;
		hwtimer_ticks = hwtimer_init_overflow_value - curr_reg_value;
		total_ticks = overflow_ticks + hwtimer_ticks;

		// change frequency from apb clock to 90K
		gcd = get_gcd(output_freq, (int)hwtimer_init_overflow_value);
		total_ticks = total_ticks * (output_freq / gcd) +
					hwtimer_init_overflow_value / (gcd * 2);
		if (hwtimer_init_overflow_value)
			do_div(total_ticks, hwtimer_init_overflow_value / gcd);
		else {
			printk("HWTimer: calculate output ticks failed! Can not divide zero!\n");
			return -EINVAL;
		}

		//update timer state
		timer_state.output_ticks = total_ticks;
		timer_state.overflow_count = hwtimer_overflow_count;
		timer_state.reg_value = curr_reg_value;
	} else {
		curr_reg_value = gk_hwtimer_read();
		if (curr_reg_value > timer_state.reg_value) {
			overflow_ticks = (hwtimer_overflow_count - timer_state.overflow_count)
						* hwtimer_init_overflow_value;
			hwtimer_ticks = curr_reg_value - timer_state.reg_value;
			total_ticks = overflow_ticks + hwtimer_ticks;
		} else {
			overflow_ticks = (hwtimer_overflow_count - timer_state.overflow_count)
						* hwtimer_init_overflow_value;
			hwtimer_ticks = timer_state.reg_value - curr_reg_value;
			total_ticks = overflow_ticks - hwtimer_ticks;
		}

		// change frequency from apb clock to 90K
		total_ticks = total_ticks * HWTIMER_OUTPUT_FREQ;
		if (timer_state.overflow_value)
			do_div(total_ticks, timer_state.overflow_value);
		 else {
			printk("HWTimer: calculate output ticks failed! Can not divide zero!\n");
			return -EINVAL;
		}
		total_ticks += timer_state.output_ticks;

		//update timer state structure
		timer_state.output_ticks = total_ticks;
		timer_state.overflow_count = hwtimer_overflow_count;
		timer_state.reg_value = curr_reg_value;
	}

	return 0;
}

/*static void hw_timer_check_input_freq(int isr_flag)
{
	u32 curr_input_freq = HWTIMER_INPUT_FREQ;

	if (timer_state.overflow_value != curr_input_freq) {
		//printk("HWTimer: input frequency is changed from %lld to %u!\n",
		//	timer_state.overflow_value, curr_input_freq);
		if (hwtimer_input_freq_unchanged) {
			hwtimer_input_freq_unchanged = 0;
		}
		if (isr_flag)
			calc_output_ticks();
		timer_state.overflow_value = curr_input_freq;
	}
}*/

static int hwtimer_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[50];

	if (count > 50) {
		printk("HWTimer: %s: count %d out of size!\n", __func__, (u32)count);
		retval = -ENOSPC;
		goto hwtimer_write_proc_exit;
	}
	if (count > 1) {
		if (copy_from_user(buf, buffer, count - 1)) {
			printk("HWTimer: %s: copy_from_user fail!\n", __func__);
			retval = -EFAULT;
			goto hwtimer_write_proc_exit;
		}
		buf[count] = '\0';
		hwtimer_init_output_value = simple_strtoull(buf, NULL, 10);
	}

	hwtimer_reset();

	retval = count;

hwtimer_write_proc_exit:
	return retval;
}

int get_hwtimer_output_ticks(u64 *out_tick)
{
	int ret = 0;
	if (timer_state.enable_flag) {
		ret = calc_output_ticks();
		if (!ret) {
			//hw_timer_check_input_freq(0);
			*out_tick = timer_state.output_ticks + hwtimer_init_output_value;
		} else {
			*out_tick = 0;
		}
	} else {
		*out_tick = 0;
	}

	return ret;
}
EXPORT_SYMBOL(get_hwtimer_output_ticks);

static int hwtimer_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	u64 final_ticks = 0;

	if (off)
		return ret;

	*start = page + off;
	*eof = 1;

	if (timer_state.enable_flag) {
		ret = calc_output_ticks();
		if (!ret) {
			//hw_timer_check_input_freq(0);
			final_ticks = timer_state.output_ticks + hwtimer_init_output_value;
			ret = sprintf(*start, "%lld\n", final_ticks);
		} else {
			ret = sprintf(*start, "%d\n", 0);
		}
	} else {
		ret = sprintf(*start, "%d\n", 0);
	}

	return ret;
}

/* buffer size is one page but our output routines use some slack for overruns */
#define PROC_BLOCK_SIZE	(PAGE_SIZE - 1024)
static ssize_t hwtimer_proc_file_read_func(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct inode * inode = file->f_path.dentry->d_inode;
	char 	*page;
	ssize_t	retval=0;
	int	eof=0;
	ssize_t	n, count;
	char	*start;
	struct proc_dir_entry * dp;
	unsigned long long pos;

	pos = *ppos;
	if (pos > MAX_NON_LFS)
		return 0;
	if (nbytes > MAX_NON_LFS - pos)
		nbytes = MAX_NON_LFS - pos;

	dp = PDE(inode);
	if (!(page = (char*) __get_free_page(GFP_TEMPORARY)))
		return -ENOMEM;

	while ((nbytes > 0) && !eof) {
		count = min_t(size_t, PROC_BLOCK_SIZE, nbytes);

		start = NULL;
		if (dp->read_proc) {
			n = dp->read_proc(page, &start, *ppos,
					  count, &eof, dp->data);
		} else
			break;

		if (n == 0)   /* end of file */
			break;
		if (n < 0) {  /* error */
			if (retval == 0)
				retval = n;
			break;
		}

		if (start == NULL) {
			if (n > PAGE_SIZE) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE;
			}
			n -= *ppos;
			if (n <= 0)
				break;
			if (n > count)
				n = count;
			start = page + *ppos;
		} else if (start < page) {
			if (n > PAGE_SIZE) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE;
			}
			if (n > count) {
				/*
				 * Don't reduce n because doing so might
				 * cut off part of a data block.
				 */
				printk(KERN_WARNING
				       "proc_file_read: Read count exceeded\n");
			}
		} else /* start >= page */ {
			unsigned long startoff = (unsigned long)(start - page);
			if (n > (PAGE_SIZE - startoff)) {
				printk(KERN_ERR
				       "proc_file_read: Apparent buffer overflow!\n");
				n = PAGE_SIZE - startoff;
			}
			if (n > count)
				n = count;
		}

		n -= copy_to_user(buf, start < page ? page : start, n);
		if (n == 0) {
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		// disable proc file offset update to support read without sync
		//*ppos += start < page ? (unsigned long)start : n;
		nbytes -= n;
		buf += n;
		retval += n;
	}
	free_page((unsigned long) page);
	return retval;
}

static void __hwtimer_pde_users_dec(struct proc_dir_entry *pde)
{
	pde->pde_users--;
	if (pde->pde_unload_completion && pde->pde_users == 0)
		complete(pde->pde_unload_completion);
}

static void hw_timer_pde_users_dec(struct proc_dir_entry *pde)
{
	spin_lock(&pde->pde_unload_lock);
	__hwtimer_pde_users_dec(pde);
	spin_unlock(&pde->pde_unload_lock);
}

static ssize_t hwtimer_proc_file_read(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct proc_dir_entry *pde = PDE(file->f_path.dentry->d_inode);
	ssize_t rv = -EIO;

	spin_lock(&pde->pde_unload_lock);
	if (!pde->proc_fops) {
		spin_unlock(&pde->pde_unload_lock);
		return rv;
	}
	pde->pde_users++;
	spin_unlock(&pde->pde_unload_lock);

	rv = hwtimer_proc_file_read_func(file, buf, nbytes, ppos);

	hw_timer_pde_users_dec(pde);
	return rv;
}

static ssize_t hwtimer_proc_file_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	struct proc_dir_entry *pde = PDE(file->f_path.dentry->d_inode);
	ssize_t rv = -EIO;

	if (pde->write_proc) {
		spin_lock(&pde->pde_unload_lock);
		if (!pde->proc_fops) {
			spin_unlock(&pde->pde_unload_lock);
			return rv;
		}
		pde->pde_users++;
		spin_unlock(&pde->pde_unload_lock);

		/* FIXME: does this routine need ppos?  probably... */
		rv = pde->write_proc(file, buffer, count, pde->data);
		hw_timer_pde_users_dec(pde);
	}
	return rv;
}

static const struct file_operations hwtimer_fops = {
	.owner = THIS_MODULE,
	.read = hwtimer_proc_file_read,
	.write = hwtimer_proc_file_write,
};

static int hwtimer_create_proc(void)
{
	int err_code = 0;
	struct proc_dir_entry *hwtimer_entry;

	hwtimer_entry = proc_create_data(GK_HWTIMER_NAME,
		S_IRUGO | S_IWUGO, get_gk_proc_dir(), &hwtimer_fops, NULL);
	//hwtimer_entry = create_proc_entry(GK_HWTIMER_NAME,
	//						S_IRUGO, get_gk_proc_dir());

	if (!hwtimer_entry) {
		err_code = -EINVAL;
	} else {
		hwtimer_entry->read_proc = hwtimer_read_proc;
		hwtimer_entry->write_proc = hwtimer_write_proc;
	}

	return err_code;
}

static irqreturn_t hwtimer_isr(int irq, void *dev_id)
{
	spin_lock(&timer_isr_lock);
	hwtimer_overflow_count++;
	spin_unlock(&timer_isr_lock);

	return IRQ_HANDLED;
}

static void init_timer_state(void)
{
	timer_state.enable_flag = 0;
	timer_state.output_ticks = 0;
	timer_state.overflow_count = 0;
	timer_state.reg_value = hwtimer_init_overflow_value;
	timer_state.overflow_value = hwtimer_init_overflow_value;
}

static void hwtimer_remove_proc(void)
{
	remove_proc_entry(GK_HWTIMER_NAME, get_gk_proc_dir());
}

int hwtimer_reset()
{
	hwtimer_overflow_count = 0;
	hwtimer_init_overflow_value = HWTIMER_INPUT_FREQ;
	init_timer_state();
	gk_hwtimer_init();

	return 0;
}

static int __init hwtimer_init(void)
{
	int err_code = 0;
	err_code = hwtimer_create_proc();
	if (err_code) {
		printk("HWTimer: create proc file for hw timer failed!\n");
		goto hwtimer_init_err_create_proc;
	}

	hwtimer_overflow_count = 0;
	err_code = request_irq(GK_HWTIMER_IRQ, hwtimer_isr,
					IRQF_TRIGGER_RISING, GK_HWTIMER_NAME, NULL);
	if (err_code) {
		printk("HWTimer: request irq for hw timer failed!\n");
		goto hwtimer_init_err_request_irq;
	}

	hwtimer_init_overflow_value = HWTIMER_INPUT_FREQ;
	init_timer_state();
	gk_hwtimer_init();

	goto hwtimer_init_err_na;

hwtimer_init_err_request_irq:
	free_irq(GK_HWTIMER_IRQ, NULL);

hwtimer_init_err_create_proc:
	hwtimer_remove_proc();

hwtimer_init_err_na:
	return err_code;
}

static void __exit hwtimer_exit(void)
{
	if (timer_state.enable_flag) {
		gk_hwtimer_disable();
		timer_state.enable_flag = 0;
	}
	free_irq(GK_HWTIMER_IRQ, NULL);
	hwtimer_remove_proc();
}

module_init(hwtimer_init);
module_exit(hwtimer_exit);

MODULE_DESCRIPTION("gk hardware timer driver");
MODULE_LICENSE("Proprietary");

