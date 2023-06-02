/*!
*****************************************************************************
** \file        arch/arm/mach-gk/switch.c
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
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
#include <mach/audio_codec.h>


#define GK_USB_SWITCH_NAME 	        "usb_switch"
#define GK_AUDIO_SWITCH_NAME 	    "audio_switch"
#define GK_CVBS_SWITCH_NAME 	    "cvbs_switch"
#define GK_PHY_SWITCH_NAME 		    "phy_switch"
#define GK_ADC_SWITCH_NAME 	        "adc_switch"
#define BUFER_COUNT                 50
#define CHECK_COUNT                 {if(count>=(BUFER_COUNT-1))count=(BUFER_COUNT-1);}

static u8 usb_status = 1;
static int usb_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	int len = 0;

	if (off)
	{
	    *eof = 1;
		return ret;
	}

    len = sprintf(page, "%s\n", usb_status==0?"off":"on");

	return len;
}

static int usb_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[BUFER_COUNT];

    CHECK_COUNT;
	if (copy_from_user(buf, buffer, count)) {
		retval = -EFAULT;
	}

    switch(buf[0])
    {
    case '0':
        usb_status = 0;
        gk_usb_setbitsl(GK_VA_USB_PHY, 0x0000FFC4);
        break;
    case '1':
        usb_status = 1;
        gk_usb_clrbitsl(GK_VA_USB_PHY, 0x0000FFC4);
        break;
    }
	return count;
}

static int create_usb_proc_node(void)
{
	int err_code = 0;
	struct proc_dir_entry *usb_entry;

	usb_entry = create_proc_entry(GK_USB_SWITCH_NAME, S_IRUGO | S_IWUGO, get_gk_proc_dir());

	if (!usb_entry)
    {
		err_code = -EINVAL;
	}
    else
	{
		usb_entry->read_proc  = usb_read_proc;
		usb_entry->write_proc = usb_write_proc;
	}

	return err_code;
}

static u8 audio_status = 1;
static int audio_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	int len = 0;

	if (off)
	{
	    *eof = 1;
		return ret;
	}

    audio_status = gk_aud_readl(AUDC_ANALOG_CTRL00_REG);
    len = sprintf(page, "%s\n", audio_status==0?"off":"on");

	return len;
}

static int audio_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[BUFER_COUNT];
    u32 addr = 0;

    CHECK_COUNT;
	if (copy_from_user(buf, buffer, count)) {
		retval = -EFAULT;
	}

    switch(buf[0])
    {
    case '0':
        // 0x90022474 set to 0x000003FF
        // Audio
        //0x90021C00~0x90021C34   address space clear (0)
        //0x90020C00~0x90020D6C   address space clear (0)
        for(addr = GK_VA_AUDIO_CODEC_ANALOG; addr <= (GK_VA_AUDIO_CODEC_ANALOG + 0x34); addr+=4)
        {
            //printk("[%s] 0x%08x=0x%08x\n", __func__, addr, gk_readl(addr));
            gk_aud_writel( addr, 0x00000000);
        }
        for(addr = GK_VA_AUDIO_CODEC_DIGITAL; addr <= (GK_VA_AUDIO_CODEC_DIGITAL + 0x16C); addr+=4)
        {
            //printk("[%s] 0x%08x=0x%08x\n", __func__, addr, gk_readl(addr));
            gk_aud_writel( addr, 0x00000000);
        }
        audio_status = 0;
        break;
    case '1':
        // 0x90022474 set to 0x000003FF
        gk_aud_writel(AUDC_ANALOG_CTRL00_REG, 0x8040);
        gk_aud_writel(AUDC_ANALOG_CTRL03_REG, 0x4004);
        gk_aud_writel(AUDC_ANALOG_CTRL04_REG, 0x2000);
        gk_aud_writel(AUDC_ANALOG_CTRL06_REG, 0x4);
        gk_aud_writel(AUDC_ANALOG_CTRL11_REG, 0x0);
        gk_aud_writel(AUDC_ANALOG_CTRL13_REG, 0x6000);
        gk_aud_writel(AUDC_ANALOG_CTRL00_REG, 0x8040);
        gk_aud_writel(AUDC_ANALOG_CTRL03_REG, 0x401c);
        gk_aud_writel(AUDC_ANALOG_CTRL04_REG, 0x233a);
        gk_aud_writel(AUDC_ANALOG_CTRL05_REG, 0x280c);
        gk_aud_writel(AUDC_ANALOG_CTRL06_REG, 0x8407);
        gk_aud_writel(AUDC_ANALOG_CTRL07_REG, 0x3802);
        gk_aud_writel(AUDC_ANALOG_CTRL08_REG, 0xf8);
        gk_aud_writel(AUDC_ANALOG_CTRL09_REG, 0x0);
        gk_aud_writel(AUDC_ANALOG_CTRL10_REG, 0xc0c0);
        gk_aud_writel(AUDC_ANALOG_CTRL11_REG, 0x9080);
        gk_aud_writel(AUDC_ANALOG_CTRL12_REG, 0x0);
        gk_aud_writel(AUDC_ANALOG_CTRL13_REG, 0x6000);
        gk_aud_writel(AUDC_DIGITAL_MMP1_DPGA_CFG1_REG, 0x5);
        gk_aud_writel(AUDC_DIGITAL_MMP2_DPGA_CFG1_REG, 0x5);
        gk_aud_writel(AUDC_DIGITAL_SYS_RST_CTRL0_REG, 0xe000);
        gk_aud_writel(AUDC_DIGITAL_SYS_RST_CTRL0_REG, 0x0);
        gk_aud_writel(AUDC_DIGITAL_AUDIOBAND_CTRL0_REG, 0x2);
        gk_aud_writel(AUDC_DIGITAL_CKG_CTRL0_REG, 0x3);
        gk_aud_writel(AUDC_DIGITAL_TIMING_CTRL0_REG, 0x33f);
        gk_aud_writel(AUDC_DIGITAL_TIMING_CTRL1_REG, 0xf3e);
        gk_aud_writel(AUDC_DIGITAL_AUDIOBAND_CTRL2_REG, 0xfaaf);
        gk_aud_writel(AUDC_DIGITAL_SDM_CTRL0_REG, 0x15);
        gk_aud_writel(AUDC_DIGITAL_MMP1_DPGA_CFG1_REG, 0x0);
        gk_aud_writel(AUDC_DIGITAL_MMP2_DPGA_CFG1_REG, 0x0);
        gk_aud_writel(AUDC_DIGITAL_TIMING_CTRL0_REG, 0x23f);
        gk_aud_writel(AUDC_DIGITAL_MIX_CTRL0_REG, 0x2500);
        gk_aud_writel(AUDC_DIGITAL_TIMING_CTRL1_REG, 0x23e);
        gk_aud_writel(AUDC_DIGITAL_AUDIOBAND_CTRL1_REG, 0xea82);
        gk_aud_writel(AUDC_DIGITAL_AUDIOBAND_CTRL2_REG, 0xaaaf);
        gk_aud_writel(AUDC_ANALOG_CTRL02_REG, 0x8);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_1_NF_H_REG, 0x000000BB);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_1_NF_L_REG, 0x00008000);
        mdelay(1);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_1_NF_H_REG, 0x0000C0BB);
        mdelay(1);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_1_NF_H_REG, 0x000000BB);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_2_NF_H_REG, 0x000000c0);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_2_NF_L_REG, 0x00000000);
        mdelay(1);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_2_NF_H_REG, 0x0000c0c0);
        mdelay(1);
        gk_aud_writel(AUDC_DIGITAL_NF_SYNTH_2_NF_H_REG, 0x0000c0);
        gk_aud_writel(AUDC_DIGITAL_FIFO_TH_CTRL0_REG, 0x402);
        gk_aud_writel(AUDC_DIGITAL_FIFO_CTRL_REG, 0x8808);
        gk_aud_writel(AUDC_DIGITAL_FIFO_CTRL_REG, 0x8888);
        gk_greg_writel(AHB_GENNERNAL1_REG, 0x00000001);
        audio_status = 1;
        break;
    }
	return count;
}

static int create_audio_proc_node(void)
{
	int err_code = 0;
	struct proc_dir_entry *audio_entry;

	audio_entry = create_proc_entry(GK_AUDIO_SWITCH_NAME, S_IRUGO | S_IWUGO, get_gk_proc_dir());

	if (!audio_entry)
    {
		err_code = -EINVAL;
	}
    else
	{
		audio_entry->read_proc  = audio_read_proc;
		audio_entry->write_proc = audio_write_proc;
	}

	return err_code;
}

static int cvbs_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	int len = 0;
    u32 cvbs_status;

	if (off)
	{
	    *eof = 1;
		return ret;
	}
    cvbs_status = gk_vout_readl(GK_VA_VEDIO_DAC);
    cvbs_status &= 0x00000001;
    len = sprintf(page, "%s\n", cvbs_status==0?"off":"on");

	return len;
}

static int cvbs_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[BUFER_COUNT];
    CHECK_COUNT;

	if (copy_from_user(buf, buffer, count)) {
		retval = -EFAULT;
	}

    switch(buf[0])
    {
    case '0':
        gk_vout_clrbitsl(GK_VA_VEDIO_DAC, 0x00000001);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x04, 0x00);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x08, 0x00);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x10, 0x00);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x14, 0x00);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x18, 0x00);
        break;
    case '1':
        gk_vout_setbitsl(GK_VA_VEDIO_DAC, 0x00000001);
        //config video DAC for CVBS
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x00, 0x01);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x04, 0x01);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x08, 0x00);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x10, 0x01);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x14, 0x01);
        gk_vout_writel( GK_VA_VEDIO_DAC + 0x18, 0x3F);
        break;
    }
	return count;
}

static int create_cvbs_proc_node(void)
{
	int err_code = 0;
	struct proc_dir_entry *cvbs_entry;

	cvbs_entry = create_proc_entry(GK_CVBS_SWITCH_NAME, S_IRUGO | S_IWUGO, get_gk_proc_dir());

	if (!cvbs_entry)
    {
		err_code = -EINVAL;
	}
    else
	{
		cvbs_entry->read_proc  = cvbs_read_proc;
		cvbs_entry->write_proc = cvbs_write_proc;
	}

	return err_code;
}

static int phy_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	int len = 0;
    u32 phy_status;
	if (off)
	{
	    *eof = 1;
		return ret;
	}
    phy_status = gk_eth_readl(GK_VA_ETH_PHY + 0x474);
    len = sprintf(page, "%s\n", phy_status==0x000003FF?"off":"on");

	return len;
}

static int phy_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[BUFER_COUNT];
    CHECK_COUNT;

	if (copy_from_user(buf, buffer, count)) {
		retval = -EFAULT;
	}

    switch(buf[0])
    {
    case '0':
        // 0x90022474 set to 0x000003FF
        gk_eth_writel(GK_VA_ETH_PHY + 0x474, 0x000003FF);
        break;
    case '1':
        // 0x90022474 set to 0x000003FF
        gk_eth_writel(GK_VA_ETH_PHY + 0x474, 0x00000000);
        break;
    }
	return count;
}

static int create_phy_proc_node(void)
{
	int err_code = 0;
	struct proc_dir_entry *phy_entry;

	phy_entry = create_proc_entry(GK_PHY_SWITCH_NAME, S_IRUGO | S_IWUGO, get_gk_proc_dir());

	if (!phy_entry)
    {
		err_code = -EINVAL;
	}
    else
	{
		phy_entry->read_proc  = phy_read_proc;
		phy_entry->write_proc = phy_write_proc;
	}

	return err_code;
}

static int adc_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int ret = 0;
	int len = 0;
    u32 adc_status;
	if (off)
	{
	    *eof = 1;
		return ret;
	}
    adc_status = gk_adc_readl(GK_VA_ADC  + 0x00);
    len = sprintf(page, "%s\n", adc_status&0x00000001?"off":"on");

	return len;
}

static int adc_write_proc(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int retval = 0;
	char buf[BUFER_COUNT];
    CHECK_COUNT;

	if (copy_from_user(buf, buffer, count)) {
		retval = -EFAULT;
	}

    switch(buf[0])
    {
    case '0':
        gk_vout_setbitsl(GK_VA_ADC  + 0x00, 0x00000001);
        gk_vout_clrbitsl(GK_VA_ADC2 + 0x18, 0x00000001);
        break;
    case '1':
        gk_vout_clrbitsl(GK_VA_ADC  + 0x00, 0x00000001);
        gk_vout_setbitsl(GK_VA_ADC2 + 0x18, 0x00000001);
        break;
    }
	return count;
}

static int create_adc_proc_node(void)
{
	int err_code = 0;
	struct proc_dir_entry *adc_entry;

	adc_entry = create_proc_entry(GK_ADC_SWITCH_NAME, S_IRUGO | S_IWUGO, get_gk_proc_dir());

	if (!adc_entry)
    {
		err_code = -EINVAL;
	}
    else
	{
		adc_entry->read_proc  = adc_read_proc;
		adc_entry->write_proc = adc_write_proc;
	}

	return err_code;
}

static int __init switch_init(void)
{
	create_usb_proc_node();
	create_audio_proc_node();
	create_cvbs_proc_node();
	create_phy_proc_node();
	create_adc_proc_node();
	return 0;
}

static void __exit switch_exit(void)
{
}

module_init(switch_init);
module_exit(switch_exit);
MODULE_DESCRIPTION("gk swith driver");
MODULE_LICENSE("Proprietary");

