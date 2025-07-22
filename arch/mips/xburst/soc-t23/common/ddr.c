/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/vmalloc.h>
#include <linux/suspend.h>
#include <linux/seq_file.h>
#include <linux/cpufreq.h>

#include <soc/ddr.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_proc.h>

/*
 *  unit: kHz
 */
static struct timer_list timer;
static void timercount1(unsigned long data);
static void timercount2(unsigned long data);
static void timercount3(unsigned long data);
static int  timer_flag = 0;

static int dump_out_ddr(struct seq_file *m, void *v)
{
	int len = 0;
	int i = 0;
	len += seq_printf(m ,"--------------------dump the DDRC1---------------\n");

	len += seq_printf(m ,"DDRC_STATUS\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_STATUS),DDRC_BASE + DDRC_STATUS);
	len += seq_printf(m ,"DDRC_CFG\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_CFG),DDRC_BASE + DDRC_CFG);
	len += seq_printf(m ,"DDRC_CTRL\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_CTRL),DDRC_BASE + DDRC_CTRL);
	len += seq_printf(m ,"DDRC_LMR\t:0x%08x\taddress\t:0x%08x\n",ddr_readl(DDRC_LMR),DDRC_BASE + DDRC_LMR);
	len += seq_printf(m ,"DDRC_TIMING1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(1)),DDRC_BASE + DDRC_TIMING(1));
	len += seq_printf(m ,"DDRC_TIMING2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(2)),DDRC_BASE + DDRC_TIMING(2));
	len += seq_printf(m ,"DDRC_TIMING3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(3)),DDRC_BASE + DDRC_TIMING(3));
	len += seq_printf(m ,"DDRC_TIMING4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(4)),DDRC_BASE + DDRC_TIMING(4));
	len += seq_printf(m ,"DDRC_TIMING5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(5)),DDRC_BASE + DDRC_TIMING(5));
	len += seq_printf(m ,"DDRC_TIMING6\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_TIMING(6)),DDRC_BASE + DDRC_TIMING(6));
	len += seq_printf(m ,"DDRC_REFCNT\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REFCNT),DDRC_BASE + DDRC_REFCNT);
	len += seq_printf(m ,"DDRC_MMAP0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_MMAP0),DDRC_BASE + DDRC_MMAP0);
	len += seq_printf(m ,"DDRC_MMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_MMAP1),DDRC_BASE + DDRC_MMAP1);
	len += seq_printf(m ,"DDRC_REMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(1)),DDRC_BASE + DDRC_REMAP(1));
	len += seq_printf(m ,"DDRC_REMAP2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(2)),DDRC_BASE + DDRC_REMAP(2));
	len += seq_printf(m ,"DDRC_REMAP3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(3)),DDRC_BASE + DDRC_REMAP(3));
	len += seq_printf(m ,"DDRC_REMAP4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(4)),DDRC_BASE + DDRC_REMAP(4));
	len += seq_printf(m ,"DDRC_REMAP5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(5)),DDRC_BASE + DDRC_REMAP(5));
	len += seq_printf(m ,"DDRC_AUTOSR_EN\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_AUTOSR_EN),DDRC_BASE + DDRC_AUTOSR_EN);

	len += seq_printf(m ,"--------------------dump the DDRP---------------\n");

	len += seq_printf(m ,"DDRP_PIR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PIR),DDRC_BASE + DDRP_PIR);
	len += seq_printf(m ,"DDRP_PGCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PGCR),DDRC_BASE + DDRP_PGCR);
	len += seq_printf(m ,"DDRP_PGSR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PGSR),DDRC_BASE + DDRP_PGSR);
	len += seq_printf(m ,"DDRP_PTR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR0),DDRC_BASE + DDRP_PTR0);
	len += seq_printf(m ,"DDRP_PTR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR1),DDRC_BASE + DDRP_PTR1);
	len += seq_printf(m ,"DDRP_PTR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_PTR2),DDRC_BASE + DDRP_PTR2);
	len += seq_printf(m ,"DDRP_DSGCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DSGCR),DDRC_BASE + DDRP_DSGCR);
	len += seq_printf(m ,"DDRP_DCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DCR),DDRC_BASE + DDRP_DCR);
	len += seq_printf(m ,"DDRP_DTPR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR0),DDRC_BASE + DDRP_DTPR0);
	len += seq_printf(m ,"DDRP_DTPR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR1),DDRC_BASE + DDRP_DTPR1);
	len += seq_printf(m ,"DDRP_DTPR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_DTPR2),DDRC_BASE + DDRP_DTPR2);
	len += seq_printf(m ,"DDRP_MR0\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR0),DDRC_BASE + DDRP_MR0);
	len += seq_printf(m ,"DDRP_MR1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR1),DDRC_BASE + DDRP_MR1);
	len += seq_printf(m ,"DDRP_MR2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR2),DDRC_BASE + DDRP_MR2);
	len += seq_printf(m ,"DDRP_MR3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_MR3),DDRC_BASE + DDRP_MR3);
	len += seq_printf(m ,"DDRP_ODTCR\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRP_ODTCR),DDRC_BASE + DDRP_ODTCR);
	for(i=0;i<4;i++) {
		len += seq_printf(m ,"DX%dGSR0 \t:0x%08x\taddress\t:0x%08x\n", i, ddr_readl(DDRP_DXGSR0(i)),DDRC_BASE + DDRP_DXGSR0(i));
		len += seq_printf(m ,"@pas:DXDQSTR(%d)\t:0x%08x\taddress\t:0x%08x\n", i,ddr_readl(DDRP_DXDQSTR(i)),DDRC_BASE + DDRP_DXDQSTR(i));
	}
	return len;
}

static int ddr_mon1_proc_read(struct seq_file *m, void *v)
{
	printk("\nSimple MODE to collect DDR rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor1\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor1\n");
	printk("********************************************\n");
	return 0;
}

static int ddr_mon1_proc_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int i;
	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor1 CLOSED *************\n");
		del_timer_sync(&timer);
		/* write 0 to disable */
		*((volatile unsigned int *)0xb34f030c) = 0;
		msleep(1000);
	}
	if(i == 1){
		printk("***********DDR Monitor1 START **************\n");
		/* write enable && total_cycles to register(0xb34f030c)*/
		*((volatile unsigned int *)0xb34f030c) = (0x10000000 | 0x3ff);
		msleep(1000);

		setup_timer(&timer, timercount1, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}
	return count;
}

static void timercount1(unsigned long data)
{
	unsigned int i = 0;

	/* Read valid cycle of ddr from register(0xb34f0310)*/
	i = *((volatile unsigned int *)0xb34f0310);
	printk(KERN_DEBUG "total_cycles = %d,valid_cycles = %d\n", 1023, i);
	printk(KERN_DEBUG "rate = %%%d\n",i * 100 / 1023);
	mod_timer(&timer,jiffies + msecs_to_jiffies(1000));
}

static int ddr_mon2_proc_read(struct seq_file *m, void *v)
{
	printk("\n\nMODE2 to collect DDR's  [ Using && IDLE ] rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor2\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor2\n");
	printk("********************************************\n");

	return 0;
}

static int ddr_mon2_proc_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int i;
	/*DDR Clk Gate Register(0xb00000d0) */
	/*In this mode, bit6 should be set in this Register*/
	i = *((volatile unsigned int *)0xb00000d0);
	i |= (1<<6);
	*((volatile unsigned int *)0xb00000d0) = i;

	/* T15 dont support this */
	/* *((volatile unsigned int *)0xb00000d0) = 0x7d; */
	printk("ddr_drcg = 0x%x\n",*((volatile unsigned int *)0xb00000d0));

	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor2 CLOSED *************\n");
		del_timer_sync(&timer);
	}
	if(i == 1){
		printk("***********DDR Monitor2 START **************\n");
		setup_timer(&timer, timercount2, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}

	return count;
}

static void timercount2(unsigned long data)
{
	unsigned int i,j,k;
	i = 0;
	j = 0;
	k = 0;
#define SAMPLE_TIME_MS 100
#define SAMPLE_INTERVAL_MS 210
#if (SAMPLE_TIME_MS >= 10)
	if(!timer_flag) {
		/* write 0 to clear counter */
		*((volatile unsigned int *)0xb34f00d4) = 0;
		*((volatile unsigned int *)0xb34f00d8) = 0;
		*((volatile unsigned int *)0xb34f00dc) = 0;
		/* start counter */
		*((volatile unsigned int *)0xb34f00e4) = 3;
		mod_timer(&timer,jiffies + msecs_to_jiffies(SAMPLE_TIME_MS));
		timer_flag = 1;
	} else {
		/* stop counter */
		*((volatile unsigned int *)0xb34f00e4) = 2;
		/* read counter */
		i = *((volatile unsigned int *)0xb34f00d4);
		j = *((volatile unsigned int *)0xb34f00d8);
		k = *((volatile unsigned int *)0xb34f00dc);

		printk(KERN_DEBUG "total_cycle = %d,valid_cycle = %d\n",i,j);
		printk(KERN_DEBUG "idle rate = %%%d\n\n",k * 100 / i);
		printk(KERN_DEBUG "actual rate = %%%d\n",j * 100 / i);
		printk(KERN_DEBUG "valid rate = %%%d\n",j * 100 / (i - k));
		mod_timer(&timer,jiffies + msecs_to_jiffies(SAMPLE_INTERVAL_MS));
		timer_flag = 0;
	}
#else
	/* write 0 to clear counter */
	*((volatile unsigned int *)0xb34f00d4) = 0;
	*((volatile unsigned int *)0xb34f00d8) = 0;
	*((volatile unsigned int *)0xb34f00dc) = 0;
	/* start counter */
	*((volatile unsigned int *)0xb34f00e4) = 3;
	mdelay(SAMPLE_TIME_MS);
	/* stop counter */
	*((volatile unsigned int *)0xb34f00e4) = 2;
	/* read counter */
	i = *((volatile unsigned int *)0xb34f00d4);
	j = *((volatile unsigned int *)0xb34f00d8);
	k = *((volatile unsigned int *)0xb34f00dc);

	printk(KERN_DEBUG "total_cycle = %d,valid_cycle = %d\n",i,j);
	printk(KERN_DEBUG "idle rate = %%%d\n\n",k * 100 / i);
	printk(KERN_DEBUG "actual rate = %%%d\n",j * 100 / i);
	printk(KERN_DEBUG "valid rate = %%%d\n",j * 100 / (i - k));
	mod_timer(&timer,jiffies + msecs_to_jiffies(SAMPLE_INTERVAL_MS));
#endif
}

static int ddr_mon3_proc_read(struct seq_file *m, void *v)
{
	printk("\n\nMODE3 to collect DDR's [ Change_bank && Change_rw && Miss_page ] rate \n\n");
	printk("Usage: For Example\n");
	printk("********************************************\n");
	printk("Start: echo 1 > /proc/jz/ddr/ddr_monitor3\n");
	printk("Stop : echo 0 > /proc/jz/ddr/ddr_monitor3\n");
	printk("********************************************\n");

	return 0;
}

static int ddr_mon3_proc_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int i;

	/*DDR Clk Gate Register(0xb00000d0) */
	/*In this mode, bit6 should be set in this Register*/
	i = *((volatile unsigned int *)0xb00000d0);
	i |= (1<<6);
	*((volatile unsigned int *)0xb00000d0) = i;
	printk("ddr_drcg = 0x%x\n",*((volatile unsigned int *)0xb00000d0));

	i = simple_strtoul(buffer,NULL,0);

	if(i == 0){
		printk("***********DDR Monitor3 CLOSED *************\n");
		del_timer_sync(&timer);
	}
	if(i == 1){
		printk("***********DDR Monitor3 START **************\n");
		setup_timer(&timer, timercount3, (unsigned long)NULL);
		mod_timer(&timer, jiffies+10);
	}

	return count;
}

static void timercount3(unsigned long data)
{
	unsigned int i,j,k,l;
	i = 0;
	j = 0;
	k = 0;
	l = 0;
	if(!timer_flag) {
		/* write 0 to clear counter */
		*((volatile unsigned int *)0xb34f00d4) = 0;
		*((volatile unsigned int *)0xb34f00d8) = 0;
		*((volatile unsigned int *)0xb34f00dc) = 0;
		*((volatile unsigned int *)0xb34f00e0) = 0;
		/* start counter */
		*((volatile unsigned int *)0xb34f00e4) = 1;
		mod_timer(&timer,jiffies + msecs_to_jiffies(1));
		timer_flag = 1;
	} else {
		/* stop counter */
		*((volatile unsigned int *)0xb34f00e4) = 0;
		/* read counter */
		i = *((volatile unsigned int *)0xb34f00d4);
		j = *((volatile unsigned int *)0xb34f00d8);
		k = *((volatile unsigned int *)0xb34f00dc);
		l = *((volatile unsigned int *)0xb34f00e0);
		printk(KERN_DEBUG "change_bank = %%%d\n",j * 100 / i);
		printk(KERN_DEBUG "change_rw   = %%%d\n",k * 100 / i);
		printk(KERN_DEBUG "miss page   = %%%d\n\n",l * 100 / i);

		mod_timer(&timer,jiffies + msecs_to_jiffies(999));
		timer_flag = 0;
	}
}

static int ddr_register_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_out_ddr, PDE_DATA(inode));
}

static int ddr_mon1_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_mon1_proc_read, PDE_DATA(inode));
}

static int ddr_mon2_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_mon2_proc_read, PDE_DATA(inode));
}

static int ddr_mon3_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ddr_mon3_proc_read, PDE_DATA(inode));
}

static const struct file_operations ddr_register_proc_fops ={
	.read = seq_read,
	.open = ddr_register_proc_open,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations ddr_mon1_proc_fops ={
	.read = seq_read,
	.open = ddr_mon1_proc_open,
	.write = ddr_mon1_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations ddr_mon2_proc_fops ={
	.read = seq_read,
	.open = ddr_mon2_proc_open,
	.write = ddr_mon2_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations ddr_mon3_proc_fops ={
	.read = seq_read,
	.open = ddr_mon3_proc_open,
	.write = ddr_mon3_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init init_ddr_proc(void)
{
	struct proc_dir_entry *p;

#ifdef CONFIG_DDR_DEBUG
	register int bypassmode = 0;
	register int AutoSR_en = 0;

	bypassmode = ddr_readl(DDRP_PIR) & DDRP_PIR_DLLBYP;
	if(bypassmode) {
		printk("the ddr is in bypass mode\n");
	}else{
		printk("the ddr it not in bypass mode\n");
	}

	AutoSR_en = ddr_readl(DDRC_AUTOSR_EN) & DDRC_AUTOSR_ENABLE;
	if(AutoSR_en) {
		printk("the ddr self_refresh is enable\n");
	}else{
		printk("the ddr self_refrese is not enable\n");
	}

	printk("the ddr remap register is\n");
	printk("DDRC_REMAP1\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(1)),DDRC_BASE + DDRC_REMAP(1));
	printk("DDRC_REMAP2\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(2)),DDRC_BASE + DDRC_REMAP(2));
	printk("DDRC_REMAP3\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(3)),DDRC_BASE + DDRC_REMAP(3));
	printk("DDRC_REMAP4\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(4)),DDRC_BASE + DDRC_REMAP(4));
	printk("DDRC_REMAP5\t:0x%08x\taddress\t:0x%08x\n", ddr_readl(DDRC_REMAP(5)),DDRC_BASE + DDRC_REMAP(5));
#endif
	p = jz_proc_mkdir("ddr");
	if (!p) {
		pr_warning("create_proc_entry for common ddr failed.\n");
		return -ENODEV;
	}
	proc_create_data("ddr_Register", 0444, p, &ddr_register_proc_fops, 0);

	proc_create_data("ddr_monitor1", 0444, p, &ddr_mon1_proc_fops, 0);

	proc_create_data("ddr_monitor2", 0444, p, &ddr_mon2_proc_fops, 0);

	proc_create_data("ddr_monitor3", 0444, p, &ddr_mon3_proc_fops, 0);

	return 0;
}

module_init(init_ddr_proc);
