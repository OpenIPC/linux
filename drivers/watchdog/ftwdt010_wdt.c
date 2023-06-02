/*
 *	Watchdog driver for the FTWDT010 Watch Dog Driver
 *
 *      (c) Copyright 2004 GM Technology Corp. (www.grain-media.com)
 *	    Based on sa1100_wdt.c by Oleg Drokin <green@crimea.edu>
 *          Based on SoftDog driver by Alan Cox <alan@redhat.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *      27/11/2004 Initial release
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/device.h>
#include <linux/synclink.h>
#include <mach/ftpmu010.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/fmem.h>
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define SYSCLK  30000000
#else
#define SYSCLK  12000000
#endif

#define MAX_RESET_TIME (UINT_MAX / SYSCLK)
#define DEFAULT_RESET_SECS	5

#define WdCounter   0x0
#define WdLoad		0x4
#define WdRestart	0x8
#define WdCR		0xC

static int reset_secs = DEFAULT_RESET_SECS;
static int nowayout = WATCHDOG_NOWAYOUT;
static int watchdog_enable = 1;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout,
	"Watchdog cannot be stopped once started (default="
		__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static ssize_t ftwdt010dog_write(struct file *file, const char *data,
				 size_t len, loff_t * ppos);
static long ftwdt010dog_ioctl(struct file *file,
			     unsigned int cmd, unsigned long arg);
static int ftwdt010dog_open(struct inode *inode, struct file *file);
static int ftwdt010dog_release(struct inode *inode, struct file *file);

static struct file_operations ftwdt010dog_fops = {
    owner:          THIS_MODULE,
    write:          ftwdt010dog_write,
    unlocked_ioctl: ftwdt010dog_ioctl,
    open:           ftwdt010dog_open,
    release:        ftwdt010dog_release,
};

static struct miscdevice ftwdt010dog_miscdev = {
	WATCHDOG_MINOR,
	"watchdog",
	&ftwdt010dog_fops
};

static void reset_wdt(void __iomem * vbase, int alive_sec)
{
    u32 wdt_counter = 0;

    if (!alive_sec || alive_sec > MAX_RESET_TIME) {
        printk(KERN_WARNING "Warning! illegal watchdog reset time: %d!\n", alive_sec);
        printk(KERN_WARNING "We force to set reset time to %d sec\n", DEFAULT_RESET_SECS);
        alive_sec = DEFAULT_RESET_SECS; /* a buffer time for hw to count WDT timer */
    }
    wdt_counter = alive_sec * SYSCLK;

    /*
     * Watchdog program sequence (FTWDT010_DS_v1.5.pdf, by eason)
     * 1. Disable the WatchDog timer
     * 2. Set the WdLoad register
     * 3. Write 0x5AB9 to the WdRestart register
     * 4. Set the WdCR[Clock] bit (default: PCLK)
     * 5. Enable the WatchDog timer
     */
	outb(inl(vbase + WdCR) & ~BIT0, vbase + WdCR);
	outl(wdt_counter, vbase + WdLoad);
	outl(0x5ab9, vbase + WdRestart);	/* Magic number */
	outb(inl(vbase + WdCR) | BIT4, vbase + WdCR);
	outb(inl(vbase + WdCR) | BIT0 | BIT1, vbase + WdCR);
}

/*
 *	Allow only one person to hold it open
 */

static int ftwdt010dog_open(struct inode *inode, struct file *file)
{
	void __iomem *wdt_va_base = NULL;
	int ret = 0;
	/* Activate FTWDT010 Watchdog timer */
	wdt_va_base = ioremap(WDT_FTWDT010_PA_BASE, WDT_FTWDT010_PA_SIZE);
	printk(KERN_INFO "WDT base virtual address = %p\n", wdt_va_base);
	if (!wdt_va_base) {
		printk(KERN_INFO "Remap is failed\n");
		return -EIO;
	}
	ret = dev_set_drvdata(ftwdt010dog_miscdev.this_device, (void *)wdt_va_base);
	if (ret < 0)
		printk(KERN_ERR"%s: Set wdt_va_base to drvdata error!\n", __func__);
	reset_wdt(wdt_va_base, reset_secs);
	return 0;
}

static int ftwdt010dog_release(struct inode *inode, struct file *file)
{
	void __iomem *wdt_va_base = dev_get_drvdata(ftwdt010dog_miscdev.this_device);
	/*
	 *      Shut off the timer.
	 *      Lock it in if it's a module and we defined ...NOWAYOUT
	 */
	outl(0x5ab9, wdt_va_base + WdRestart);

    if (nowayout != WATCHDOG_NOWAYOUT)
        outb(0, wdt_va_base + WdCR);
	return 0;
}

static ssize_t ftwdt010dog_write(struct file *file, const char *data,
				 size_t len, loff_t * ppos)
{
	void __iomem *wdt_va_base = dev_get_drvdata(ftwdt010dog_miscdev.this_device);

	if (len) {
		outl(0x5ab9, wdt_va_base + WdRestart);
		return 1;
	}
	return 0;
}

static long ftwdt010dog_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int __user *p = argp;
	unsigned long count;
    static struct watchdog_info ident = {
identity:	"FTWDT010 Watchdog",
    };
	void __iomem *wdt_va_base = dev_get_drvdata(ftwdt010dog_miscdev.this_device);

    switch (cmd) {
        default:
            return -ENOIOCTLCMD;

        case WDIOC_GETSUPPORT:
            return copy_to_user((struct watchdog_info *)arg, &ident,
                    sizeof(ident));

        case WDIOC_GETSTATUS:
        case WDIOC_GETBOOTSTATUS:
            return put_user(0, (int *)arg);

        case WDIOC_SETOPTIONS:
    	{
    		int options;

    		if (get_user(options, p))
    			return -EFAULT;
    		if (options & WDIOS_DISABLECARD) {
    			outb(inl(wdt_va_base + WdCR) & ~BIT0, wdt_va_base + WdCR);
                watchdog_enable = 0;
                printk("%s, watchdog is disabled! \n", __func__);
    			return 0;
    		}
    		if (options & WDIOS_ENABLECARD) {
    			outb(inl(wdt_va_base + WdCR) | BIT0 | BIT1, wdt_va_base + WdCR);
                watchdog_enable = 1;
                reset_wdt(wdt_va_base + WdCounter, reset_secs);
                printk("%s, watchdog is enabled! timeout: %ds\n", __func__, reset_secs);
    			return 0;
    		}
    		printk("%s, invalid option value: 0x%x! \n", __func__, options);
    		return -EINVAL;
    	}

        case WDIOC_SETTIMEOUT:
            count =
                copy_from_user(&reset_secs, (int *)arg, sizeof(int));
            reset_wdt(wdt_va_base + WdCounter, reset_secs);
            return 0;

        case WDIOC_GETTIMEOUT:
            return put_user(reset_secs, (int *)arg);

        case WDIOC_KEEPALIVE:
            if (watchdog_enable)
                reset_wdt(wdt_va_base + WdCounter, reset_secs);
            return 0;
    }
}

/* PMU register data */
static int wdt_fd = -1;
static pmuReg_t regWDTArray[] = {
/* reg_off  bit_masks  lock_bits     init_val    init_mask */
#if defined(CONFIG_PLATFORM_GM8181)
	{0x3C, BIT18, BIT18, 0, BIT18},
#elif defined (CONFIG_PLATFORM_GM8126)
	{0x3C, BIT15, BIT15, 0, BIT15},
#elif defined (CONFIG_PLATFORM_GM8210) || defined (CONFIG_PLATFORM_GM8287)
	{0xB8, BIT13, 0, 0, BIT13},
#elif defined (CONFIG_PLATFORM_GM8139) || defined (CONFIG_PLATFORM_GM8136)
	{0xB8, BIT10, 0, 0, BIT10},
#endif
};

static pmuRegInfo_t wdt_clk_info = {
	"wdt_clk",
	ARRAY_SIZE(regWDTArray),
	ATTR_TYPE_NONE,		/* no clock source */
	regWDTArray
};

static int __init ftwdt010dog_init(void)
{
	int ret;
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id != FMEM_CPU_FA726 || pci_id != FMEM_PCI_HOST)
        return 0;
#endif

	ret = misc_register(&ftwdt010dog_miscdev);

	if (ret)
		return ret;

	wdt_fd = ftpmu010_register_reg(&wdt_clk_info);
#if defined(CONFIG_PLATFORM_GM8181)
	if (ftpmu010_del_lockbits(wdt_fd, 0x3C, BIT18) < 0) {
		printk(KERN_ERR "%s: Unlock PMU offset 0x3C BIT18 failed\n",
		       __func__);
	}
#elif defined (CONFIG_PLATFORM_GM8126)
	if (ftpmu010_del_lockbits(wdt_fd, 0x3C, BIT15) < 0) {
		printk(KERN_ERR "%s: Unlock PMU offset 0x3C BIT15 failed\n",
		       __func__);
	}
#endif
	if (unlikely(wdt_fd < 0)) {
		printk(KERN_ERR "WDT registers to PMU fail! \n");
		return wdt_fd;
	}

	return 0;
}

static void __exit ftwdt010dog_exit(void)
{
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
    if (cpu_id != FMEM_CPU_FA726 || pci_id != FMEM_PCI_HOST)
        return;
#endif
	/* disable WDT clk */
#if defined(CONFIG_PLATFORM_GM8181)
	if (ftpmu010_add_lockbits(wdt_fd, 0x3C, BIT18) < 0) {
		printk(KERN_ERR
		       "Add PMU register 0x3C lock_bit BIT18 Failed\n");
	}
	if (ftpmu010_write_reg(wdt_fd, 0x3C, BIT18, BIT18) < 0) {
		printk(KERN_ERR "Write PMU register 0x3C Failed\n");
	}
#elif defined (CONFIG_PLATFORM_GM8126)
	if (ftpmu010_add_lockbits(wdt_fd, 0x3C, BIT15) < 0) {
		printk(KERN_ERR
		       "Add PMU register 0x3C lock_bit BIT15 Failed\n");
	}
	if (ftpmu010_write_reg(wdt_fd, 0x3C, BIT15, BIT15) < 0) {
		printk(KERN_ERR "Write PMU register 0x3C Failed\n");
	}
#endif
	if (ftpmu010_deregister_reg(wdt_fd) < 0)
		printk(KERN_ERR "Unregister WDT from PMU Failed\n");

	misc_deregister(&ftwdt010dog_miscdev);
}

module_init(ftwdt010dog_init);
module_exit(ftwdt010dog_exit);

MODULE_AUTHOR("GM Corp.");
MODULE_LICENSE("GPL");
