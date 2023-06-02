/*
 * Faraday FTIIC010 I2C Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/ftpmu010.h>
#include "ftiic010.h"

#define MAX_RETRY	1000
#define SCL_SPEED	(100 * 1000)
#define TIMEOUT     (HZ/10)    /* 100 ms */

/* Default APB bus is APB0 for I2C, we don't use APB_CLK_IN anymore */
#define PCLK	APB0_CLK_IN

#define GSR	    0xF
#define TSR	    0x27

#define CONFIG_I2C_AUTO_RELEASE_HANGING
#ifdef CONFIG_I2C_AUTO_RELEASE_HANGING

#ifdef CONFIG_PLATFORM_GM8210
#include <mach/fmem.h>
#endif

#include <linux/kthread.h>

#define I2C_HANGS_DETECT_THRESHOLD 20 ///< 20 (times) * 100ms (thread delay) = 2 seconds

#endif ///< end of CONFIG_I2C_AUTO_RELEASE_HANGING

struct ftiic010 {
	struct resource *res;
	void __iomem    *base;
	int     irq;
	struct i2c_adapter adapter;
	int     ack;
	wait_queue_head_t waitq;
};

#ifdef CONFIG_I2C_AUTO_RELEASE_HANGING
static int i2c_hangs_detect_count = 0;

static void ftiic010_hw_init(struct ftiic010 *);

static int i2c_hangs_detect_handler(void *data)
{
    struct ftiic010 *i2c = (struct ftiic010 *)data;
    u32 sda, cr;
    int i;

    while(true) {
        sda = ioread32(i2c->base + FTIIC010_OFFSET_BMR) & FTIIC010_BMR_SDA_IN;
        if(sda == 1)
            i2c_hangs_detect_count = 0;
        else
            i2c_hangs_detect_count++;

        if(i2c_hangs_detect_count >= I2C_HANGS_DETECT_THRESHOLD) { ///< I2C bus is busy over 2s
                i2c_hangs_detect_count = 0;
                cr = ioread32(i2c->base + FTIIC010_OFFSET_CR);
                for(i = 0 ; i < 10; i++) { ///< Force I2C host output 9 SCL clock pulse
	                iowrite32(cr | FTIIC010_CR_SCL_LOW, i2c->base + FTIIC010_OFFSET_CR);
                    udelay(10);
	                iowrite32((cr & ~(FTIIC010_CR_SCL_LOW)), i2c->base + FTIIC010_OFFSET_CR);
                    udelay(10);
                }
                ftiic010_hw_init(i2c);
                if(printk_ratelimit())
                    dev_err(&i2c->adapter.dev, "I2C hangs detected! Release the I2C bus via I2C CR!\n");
        }
        msleep(100);
    }
    return 0;
}
static struct task_struct *i2c_hangs_detect_thread = NULL;

#endif ///< end of CONFIG_I2C_AUTO_RELEASE_HANGING

/******************************************************************************
 * internal functions
 *****************************************************************************/
static void ftiic010_reset(struct ftiic010 *ftiic010)
{
	int cr = FTIIC010_CR_I2C_RST;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);

	/* wait until reset bit cleared by hw */
	while (ioread32(ftiic010->base + FTIIC010_OFFSET_CR) & FTIIC010_CR_I2C_RST);
}

void ftiic010_set_clock_speed(struct ftiic010 *ftiic010, int hz)
{
	int cdr;
        
    if(unlikely(hz < 50*1000)){
	    dev_err(&ftiic010->adapter.dev, "Speed smaller than 50 KHz, set %d hz fail\n",hz);
        return ;
    }


    if(unlikely(hz > 400*1000)){
	    dev_err(&ftiic010->adapter.dev, "Speed greater than 400 KHz, set %d hz fail\n",hz);
        return;
    }

	cdr = (PCLK / hz - GSR) / 2 - 2;
	cdr &= FTIIC010_CDR_MASK;

	dev_dbg(&ftiic010->adapter.dev, "  [CDR] = %08x\n", cdr);
	iowrite32(cdr, ftiic010->base + FTIIC010_OFFSET_CDR);
}

static void ftiic010_set_tgsr(struct ftiic010 *ftiic010, int tsr, int gsr)
{
	int tgsr;

	tgsr = FTIIC010_TGSR_TSR(tsr);
	tgsr |= FTIIC010_TGSR_GSR(gsr);

	dev_dbg(&ftiic010->adapter.dev, "  [TGSR] = %08x\n", tgsr);
	iowrite32(tgsr, ftiic010->base + FTIIC010_OFFSET_TGSR);
}

static void ftiic010_hw_init(struct ftiic010 *ftiic010)
{
#ifdef CONFIG_I2C_AUTO_RELEASE_HANGING
#ifdef CONFIG_PLATFORM_GM8210
    fmem_pci_id_t pci_id;
    fmem_cpu_id_t cpu_id;
    fmem_get_identifier(&pci_id, &cpu_id);
#endif
#endif ///< End of CONFIG_I2C_AUTO_RELEASE_HANGING
    ftiic010_reset(ftiic010);
    ftiic010_set_tgsr(ftiic010, TSR, GSR);
    ftiic010_set_clock_speed(ftiic010, SCL_SPEED);
#ifdef CONFIG_I2C_AUTO_RELEASE_HANGING
#ifdef CONFIG_PLATFORM_GM8210
    if((!i2c_hangs_detect_thread) && (cpu_id == FMEM_CPU_FA626)) {
#else
    if(!i2c_hangs_detect_thread) {
#endif
        i2c_hangs_detect_thread = kthread_run(i2c_hangs_detect_handler, (void *)ftiic010, "i2c_hangs_detect_thread");
        if (IS_ERR(i2c_hangs_detect_thread))
            panic("%s: unable to create kernel thread: %ld\n", __func__, PTR_ERR(i2c_hangs_detect_thread));
        else
            printk("I2C hangs detection thread started!\n");
    }
#endif ///< End of CONFIG_I2C_AUTO_RELEASE_HANGING
}

static inline void ftiic010_set_cr(struct ftiic010 *ftiic010, int start,
		int stop, int nak)
{
	unsigned int cr;

	cr = FTIIC010_CR_I2C_EN
	   | FTIIC010_CR_SCL_EN
	   | FTIIC010_CR_TB_EN
#ifdef CONFIG_I2C_INTERRUPT_MODE	   
	   | FTIIC010_CR_DTI_EN
	   | FTIIC010_CR_DRI_EN
#endif	   
	   | FTIIC010_CR_BERRI_EN
	   | FTIIC010_CR_ALI_EN;

	if (start)
		cr |= FTIIC010_CR_START;

	if (stop)
		cr |= FTIIC010_CR_STOP;

	if (nak)
		cr |= FTIIC010_CR_NAK;
            
	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);
}

static int ftiic010_tx_byte(struct ftiic010 *ftiic010, __u8 data, int start,
		int stop)
{   		
	iowrite32(data, ftiic010->base + FTIIC010_OFFSET_DR);

	ftiic010->ack = 0;
	ftiic010_set_cr(ftiic010, start, stop, 0);	

#ifdef CONFIG_I2C_INTERRUPT_MODE
	wait_event_timeout(ftiic010->waitq, ftiic010->ack, TIMEOUT);
		
	if (unlikely(!ftiic010->ack)) 
	{
	    dev_err(&ftiic010->adapter.dev, "I2C TX data 0x%x timeout!\n", data);
	    
	    ftiic010_hw_init(ftiic010);
	    return -EIO;
	}
	
	return 0;
#else
    {
        int i = 0;

        for (i = 0; i < MAX_RETRY; i++) 
        {
            unsigned int status;

            status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
            if (status & FTIIC010_SR_DT)
                return 0;

            udelay(1);
        }	
        
        ftiic010_hw_init(ftiic010);
    }
	return -EIO;
#endif
}

static int ftiic010_rx_byte(struct ftiic010 *ftiic010, int stop, int nak)
{       
	ftiic010->ack = 0;
	ftiic010_set_cr(ftiic010, 0, stop, nak);

#ifdef CONFIG_I2C_INTERRUPT_MODE	
	wait_event_timeout(ftiic010->waitq, ftiic010->ack, TIMEOUT);
		
	if (unlikely(!ftiic010->ack)) 
	{
	    
	    ftiic010_hw_init(ftiic010);
	    dev_err(&ftiic010->adapter.dev, "I2C RX timeout!\n");
	    return -EIO;
	}
	
	return ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;
#else
    {
        int i = 0;
        for (i = 0; i < MAX_RETRY; i++) 
        {
            unsigned int status;

            status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
            if (status & FTIIC010_SR_DR) 		
                return ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;

            udelay(1);
        }

        dev_err(&ftiic010->adapter.dev, "I2C: Failed to receive!\n");
        
        ftiic010_hw_init(ftiic010);
    }
	return -EIO;
#endif	
}

static int ftiic010_tx_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	__u8 data;	
	int i, ret;

	data = (msg->addr & 0x7f) << 1 | 0; /* write */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if(unlikely(ret < 0)){
		return ret;
	}

	for (i = 0; i < msg->len; i++) {
		int stop = 0;

		if (last && (i + 1 == msg->len))
			stop = 1;
        
		ret = ftiic010_tx_byte(ftiic010, msg->buf[i], 0, stop);
		if(unlikely(ret < 0)){
			return ret;
		}
	}
	
	return 0;
}

static int ftiic010_rx_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	__u8 data;
	int i, ret;

	data = (msg->addr & 0x7f) << 1 | 1; /* read */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if (unlikely(ret < 0)){
		return ret;
	}
	    
	for (i = 0; i < msg->len; i++) {
		int nak = 0;
		int stop = 0;

		if (i + 1 == msg->len){
			if(last){
				stop = 1;
			}
			nak = 1;
		}

		ret = ftiic010_rx_byte(ftiic010, stop, nak);
		if (unlikely(ret < 0)){
			return ret;
		}
	    
		msg->buf[i] = ret;
	}
	
	return 0;
}

static int ftiic010_do_msg(struct ftiic010 *ftiic010,
		struct i2c_msg *msg, int last)
{
	if (msg->flags & I2C_M_RD)
		return ftiic010_rx_msg(ftiic010, msg, last);
	else
		return ftiic010_tx_msg(ftiic010, msg, last);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftiic010_interrupt(int irq, void *dev_id)
{
	struct ftiic010 *ftiic010 = dev_id;
	struct i2c_adapter *adapter = &ftiic010->adapter;
	int sr;

	sr = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);

	if (sr & FTIIC010_SR_DT) {
		dev_dbg(&adapter->dev, "data transmitted\n");		
		ftiic010->ack = 1;
		wake_up(&ftiic010->waitq);
	}

	if (sr & FTIIC010_SR_DR) {
		dev_dbg(&adapter->dev, "data received\n");		
		ftiic010->ack = 1;
		wake_up(&ftiic010->waitq);
	}

	if (sr & FTIIC010_SR_BERR) {
		dev_err(&adapter->dev, "NAK!\n");
		ftiic010_hw_init(ftiic010);
	}

	if (sr & FTIIC010_SR_AL) {
		dev_err(&adapter->dev, "arbitration lost!\n");
		ftiic010_hw_init(ftiic010);
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct i2c_algorithm functions
 *****************************************************************************/
static int ftiic010_master_xfer(struct i2c_adapter *adapter,
		struct i2c_msg *msgs, int num)
{
	struct ftiic010 *ftiic010 = i2c_get_adapdata(adapter);
	int i;
	
	for (i = 0; i < num; i++) {
		int ret, last = 0;

		if (i == num - 1){
			last = 1;
        	}

		ret = ftiic010_do_msg(ftiic010, &msgs[i], last);
		if (unlikely(ret < 0)){
			ftiic010_hw_init(ftiic010);
	        	return ret;
		}
	}
	return num;
}

static u32 ftiic010_functionality(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm ftiic010_algorithm = {
	.master_xfer	= ftiic010_master_xfer,
	.functionality	= ftiic010_functionality,
};

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftiic010_probe(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010;
	struct resource *res;
	int irq;
	int ret;
    
    /* This function will be called several times and pass different pdev structure
     */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

	if ((irq = platform_get_irq(pdev, 0)) < 0)
		return irq;

	ftiic010 = kzalloc(sizeof(*ftiic010), GFP_KERNEL);
	if (!ftiic010) {
		dev_err(&pdev->dev, "Could not allocate private data\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	init_waitqueue_head(&ftiic010->waitq);
    
    /* mark the region is occupied */
	ftiic010->res = request_mem_region(res->start,
			res->end - res->start, dev_name(&pdev->dev));
	if (ftiic010->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		ret = -ENOMEM;
		goto err_req_mem;
	}

	ftiic010->base = ioremap(res->start, res->end - res->start);
	if (ftiic010->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	ret = request_irq(irq, ftiic010_interrupt, IRQF_SHARED, pdev->name, ftiic010);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}

	ftiic010->irq = irq;

	/*
	 * initialize i2c adapter
	 */
	ftiic010->adapter.owner 	= THIS_MODULE;
	ftiic010->adapter.algo		= &ftiic010_algorithm;
	ftiic010->adapter.timeout	= 1;
	ftiic010->adapter.dev.parent	= &pdev->dev;
	strcpy(ftiic010->adapter.name, "ftiic010 adapter");

	i2c_set_adapdata(&ftiic010->adapter, ftiic010);

//#define I2C_DYNAMIC_BUS_NUM
#ifndef I2C_DYNAMIC_BUS_NUM
        ftiic010->adapter.nr = pdev->id; // "pdev->id" was defined in platform.c
        ret = i2c_add_numbered_adapter(&ftiic010->adapter);
#else
	ret = i2c_add_adapter(&ftiic010->adapter);
#endif
	if (ret) {
		dev_err(&pdev->dev, "Failed to add i2c adapter\n");
		goto err_add_adapter;
	}

	platform_set_drvdata(pdev, ftiic010);

	dev_info(&pdev->dev, "irq %d, mapped at %p\n", irq, ftiic010->base);

	ftiic010_hw_init(ftiic010);

	return 0;

err_add_adapter:
	free_irq(ftiic010->irq, ftiic010);
err_req_irq:
	iounmap(ftiic010->base);
err_ioremap:
	release_resource(ftiic010->res);
err_req_mem:
	kfree(ftiic010);
err_alloc:
	return ret;
};

static int __devexit ftiic010_remove(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010 = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&ftiic010->adapter);
	free_irq(ftiic010->irq, ftiic010);
	iounmap(ftiic010->base);
	release_resource(ftiic010->res);
	kfree(ftiic010);
	return 0;
};


static struct platform_driver ftiic010_driver = {
	.probe		= ftiic010_probe,
	.remove		= __devexit_p(ftiic010_remove),
	.driver		= {
		.name	= "ftiic010",
		.owner	= THIS_MODULE,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftiic010_init(void)
{        
	return platform_driver_register(&ftiic010_driver);
}

static void __exit ftiic010_exit(void)
{
	platform_driver_unregister(&ftiic010_driver);
}

EXPORT_SYMBOL(ftiic010_set_clock_speed);

module_init(ftiic010_init);
module_exit(ftiic010_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTIIC010 I2C bus adapter");
MODULE_LICENSE("GPL");
