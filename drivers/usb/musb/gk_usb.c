/*
 * drivers/usb/gk_usb.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2014-2015, goke, Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/prefetch.h>
#include <linux/usb.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <plat/gk_gpio.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE
#include <mach/hardware.h>

#include "musb_core.h"
#include "gk_usb.h"


static struct timer_list musb_idle_timer;
static struct musb *the_musb;


struct gk_glue {
    struct device           *dev;
    struct platform_device  *musb;
};

#define USB_PHY_BASE		(GK_VA_USB_PHY)
#define USB_PHY_REG(x)		(USB_PHY_BASE + (x))

#define REG_USB_PHY_UTMI0                                   USB_PHY_REG(0x000) /* read/write */
#define REG_USB_PHY_UTMI_REG_01                             USB_PHY_REG(0x004) /* read/write */
#define REG_USB_PHY_UTMI_REG_02                             USB_PHY_REG(0x008) /* read/write */
#define REG_USB_PHY_UTMI_REG_03                             USB_PHY_REG(0x00C) /* read/write */
#define REG_USB_PHY_UTMI_REG_04                             USB_PHY_REG(0x010) /* read/write */
#define REG_USB_PHY_UTMI_REG_05                             USB_PHY_REG(0x014) /* read/write */
#define REG_USB_PHY_UTMI_REG_06                             USB_PHY_REG(0x018) /* read/write */
#define REG_USB_PHY_UTMI_STATUS                             USB_PHY_REG(0x01C) /* read */
#define REG_USB_PHY_UTMI_REG_08                             USB_PHY_REG(0x020) /* read/write */
#define REG_USB_PHY_UTMI_REG_09                             USB_PHY_REG(0x024) /* read/write */
#define REG_USB_PHY_UTMI_REG_0A                             USB_PHY_REG(0x028) /* read/write */
#define REG_USB_PHY_UTMI_REG_0B                             USB_PHY_REG(0x02C) /* read/write */
#define REG_USB_PHY_UTMI_REG_0C                             USB_PHY_REG(0x030) /* read/write */
#define REG_USB_PHY_UTMI_REG_10                             USB_PHY_REG(0x040) /* read/write */
#define REG_USB_PHY_UTMI_REG_11                             USB_PHY_REG(0x044) /* read/write */
#define REG_USB_PHY_UTMI_REG_12                             USB_PHY_REG(0x048) /* read/write */
#define REG_USB_PHY_UTMI_REG_13                             USB_PHY_REG(0x04C) /* read/write */
#define REG_USB_PHY_UTMI_REG_14                             USB_PHY_REG(0x050) /* read/write */
#define REG_USB_PHY_UTMI_REG_15                             USB_PHY_REG(0x054) /* read/write */
#define REG_USB_PHY_UTMI_REG_16                             USB_PHY_REG(0x058) /* read/write */
#define REG_USB_PHY_UTMI_REG_17                             USB_PHY_REG(0x05C) /* read/write */
#define REG_USB_PHY_UTMI_REG_18                             USB_PHY_REG(0x060) /* read/write */
#define REG_USB_PHY_UTMI_REG_19                             USB_PHY_REG(0x064) /* read/write */
#define REG_USB_PHY_UTMI_REG_1A                             USB_PHY_REG(0x068) /* read/write */
#define REG_USB_PHY_UTMI_REG_1B                             USB_PHY_REG(0x06C) /* read/write */
#define REG_USB_PHY_UTMI_REG_1C                             USB_PHY_REG(0x070) /* read/write */
#define REG_USB_PHY_UTMI_REG_1D                             USB_PHY_REG(0x074) /* read/write */
#define REG_USB_PHY_UTMI_REG_1E                             USB_PHY_REG(0x078) /* read/write */

#define REG_UTMI_BASE   GK_VA_USB_PHY
#define REG_PLL_POWER_CONTROL      (GK_VA_RCT + 0x0050) /* read/write */

//#define USB_AHB_W(addr, value)  *(volatile unsigned long*)(addr) = (value)
//#define USB_AHB_R(addr, value)  value = *(volatile unsigned long*)(addr)
#ifdef CONFIG_GK_USB_OTG_MODE
static int usb_ctrl_init(void);
#endif

void GH_PLL_set_POWER_CONTROL_usb(u8 data)
{
    GH_PLL_POWER_CONTROL_S d;
	d.all = gk_usb_readl(REG_PLL_POWER_CONTROL);
    //d.all = *(volatile u32 *)REG_PLL_POWER_CONTROL;
    d.bitc.usb = data;
	gk_usb_writel(REG_PLL_POWER_CONTROL, d.all);
    //*(volatile u32 *)REG_PLL_POWER_CONTROL = d.all;
}

void GH_USB_PHY_init(void)
{
	gk_usb_writew(REG_USB_PHY_UTMI0, ((u16)0x0000ff05));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_01, ((u16)0x00009080));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_02, ((u16)0x00003000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_03, ((u16)0x00003020));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_04, ((u16)0x00000483));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_05, ((u16)0x00008044));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_06, ((u16)0x000000c0));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_08, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_09, ((u16)0x000000ff));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_0A, ((u16)0x0000000b));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_0B, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_0C, ((u16)0x000023cc));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_10, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_11, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_12, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_13, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_14, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_15, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_16, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_17, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_18, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_19, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_1A, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_1B, ((u16)0x00000001));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_1C, ((u16)0x00002801));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_1D, ((u16)0x00000000));
	gk_usb_writew(REG_USB_PHY_UTMI_REG_1E, ((u16)0x00000000));
}

int gk_usb_phy_init(void)
{
	u32 value = 0;
    struct gk_gpio_io_info  usb_gpio_conf;
    GH_USB_PHY_init();
    printk("musb phy Begin initial sequence ...\n");

    usb_gpio_conf.gpio_id = gk_all_gpio_cfg.usb_host;
#if defined(CONFIG_GK_USB_HOST_MODE)
    usb_gpio_conf.active_level = GPIO_HIGH;
#elif defined(CONFIG_GK_USB_SLAVE_MODE)
    usb_gpio_conf.active_level = GPIO_LOW;
#elif defined(CONFIG_GK_USB_OTG_MODE)
    usb_gpio_conf.active_level = GPIO_HIGH;
#endif
    usb_gpio_conf.active_delay = 1;

    gk_set_gpio_output(&usb_gpio_conf, 1);
    mdelay(20);//100ms

#if defined(CONFIG_GK_MUSB_CON_V1_10)
    value = gk_usb_readl(GK_VA_AHB_GREG + 0x8);
    printk(KERN_DEBUG "val=0x%x\n", value);
    value &= ~0x00000008;                          //08->disbale handle
    value |= 0x00000003;                           //00->burst8 01->burst16 02->burst32 03->burst64
    gk_usb_writel((GK_VA_AHB_GREG + 0x8), value);
    value = gk_usb_readl(GK_VA_AHB_GREG + 0x8);
#endif
    //USB_AHB_R(0x70170050, value);
    //USB_AHB_W(0x70170050, value | 0x02);
    //PHY reset
    GH_PLL_set_POWER_CONTROL_usb(1);
    GH_PLL_set_POWER_CONTROL_usb(0);
    mdelay(20);
    GH_PLL_set_POWER_CONTROL_usb(1);
    mdelay(10);
    // init phy
    //init UTMI
	gk_usb_writel((REG_UTMI_BASE+(0x04<<2)), 0x0000040f);
	gk_usb_writel((REG_UTMI_BASE+(0x10<<2)), 0x00008051);
	gk_usb_writel((REG_UTMI_BASE+(0x11<<2)), 0x00005080);
	gk_usb_writel((REG_UTMI_BASE+(0x11<<2)), 0x00005090);
	gk_usb_writel((REG_UTMI_BASE+(0x00<<2)), 0x00006bc3);
	gk_usb_writel((REG_UTMI_BASE+(0x0A<<2)), 0x00003403);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x00<<2)), 0x000069c3);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x11<<2)), 0x00005080);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x00<<2)), 0x00000001);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x1e<<2)), 0x00000001);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x1e<<2)), 0x00000000);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x03<<2)), 0x00000023);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x03<<2)), 0x00000020);
	gk_usb_writel((REG_UTMI_BASE+(0x0a<<2)), 0x00000003);
#if defined(CONFIG_GK_USB_HOST_MODE)
    gk_usb_writel((REG_UTMI_BASE+(0x0a<<2)), 0x00000103);
#elif defined(CONFIG_GK_USB_OTG_MODE)
    if (!gk_gpio_get(CONFIG_GK_USB_OTG_GPIO))
    {
        gk_usb_writel((REG_UTMI_BASE+(0x0a<<2)), 0x00000103);
    }
    else
    {
        gk_usb_writel((REG_UTMI_BASE+(0x0a<<2)), 0x0000010B);
    }
#elif defined(CONFIG_GK_USB_SLAVE_MODE)
    gk_usb_writel((REG_UTMI_BASE+(0x0a<<2)), 0x0000010B);
#endif
	mdelay(10);

	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x03<<2)), 0x00000023);
	mdelay(20);
	gk_usb_writel((REG_UTMI_BASE+(0x03<<2)), 0x00000020);
    mdelay(20);

    //reset USB controller
    gk_rct_writel(GK_VA_RCT + 0x0088, 0x6CC36011);
    mdelay(20);
    gk_rct_writel(GK_VA_RCT + 0x0088, 0x4CC36011);
    mdelay(20);

    return 0;
}


/*************************************************************************/

static void gk_musb_enable(struct musb *musb)
{

}

static void gk_musb_disable(struct musb *musb)
{
    void __iomem    *pBase = musb->ctrl_base;

    gk_musb_writew((unsigned int)pBase, 0x06, 0x00);
    gk_musb_writew((unsigned int)pBase, 0x08, 0x00);
    gk_musb_writeb((unsigned int)pBase, 0x0B, 0x00);
}

static int gk_musb_set_mode(struct musb *musb, u8 musb_mode)
{
    return 0;
}

static void gk_musb_try_idle(struct musb *musb, unsigned long timeout)
{

}

/*
 * Updates cable VBUS status. Caller must take care of locking.
 */
static int gk_musb_vbus_status(struct musb *musb)
{
    int        ret = 0;


    return ret;
}

static void gk_musb_set_vbus(struct musb *musb, int is_on)
{

}

static void musb_do_idle(unsigned long _musb)
{

}


static int gk_musb_init(struct musb *musb)
{
    struct platform_device  *pdev;

    usb_nop_xceiv_register();
    musb->xceiv = usb_get_transceiver();
    //printk("musb xceiv = 0x%x \n", (uint32_t)musb->xceiv);
    if (!musb->xceiv)
        return -ENODEV;


    //musb->isr = gk_musb_interrupt;

    pdev = to_platform_device(musb->controller);

    setup_timer(&musb_idle_timer, musb_do_idle, (unsigned long) musb);

    printk("gk musb init end...\n");

    return 0;
}

static int gk_musb_exit(struct musb *musb)
{
    del_timer_sync(&musb_idle_timer);
    the_musb = NULL;

    if (musb->board_set_power)
        musb->board_set_power(0);

    usb_put_transceiver(musb->xceiv);
    usb_nop_xceiv_unregister();
    return 0;
}

static const struct musb_platform_ops gk_ops = {
    .init       = gk_musb_init,
    .exit       = gk_musb_exit,

    .enable     = gk_musb_enable,
    .disable    = gk_musb_disable,

    .set_mode   = gk_musb_set_mode,
    .try_idle   = gk_musb_try_idle,

    .vbus_status= gk_musb_vbus_status,
    .set_vbus   = gk_musb_set_vbus,
};

static int gk_probe(struct platform_device *pdev)
{
    int ret = -ENOMEM;
    struct musb_hdrc_platform_data  *pdata = pdev->dev.platform_data;
    struct platform_device          *musb;
    struct gk_glue              *glue;

    gk_usb_phy_init();
    glue = kzalloc(sizeof(*glue), GFP_KERNEL);
    if (!glue) {
        dev_err(&pdev->dev, "failed to allocate glue context\n");
        goto err0;
    }

    musb = platform_device_alloc("musb-hdrc", -1);
    if (!musb) {
        dev_err(&pdev->dev, "failed to allocate musb device\n");
        goto err1;
    }

    musb->dev.parent            = &pdev->dev;
    musb->dev.dma_mask          = &gk_dmamask;
    musb->dev.coherent_dma_mask = gk_dmamask;

    glue->dev                   = &pdev->dev;
    glue->musb                  = musb;

    pdata->platform_ops         = &gk_ops;

    platform_set_drvdata(pdev, glue);

    ret = platform_device_add_resources(musb, pdev->resource,
            pdev->num_resources);
    if (ret) {
        dev_err(&pdev->dev, "failed to add resources\n");
        goto err2;
    }

    ret = platform_device_add_data(musb, pdata, sizeof(*pdata));
    if (ret) {
        dev_err(&pdev->dev, "failed to add platform_data\n");
        goto err2;
    }

    ret = platform_device_add(musb);
    if (ret) {
        dev_err(&pdev->dev, "failed to register musb device\n");
        goto err1;
    }
    printk("platform add gk musb...\n");


    return 0;

err2:
    platform_device_put(musb);

err1:
    kfree(glue);

err0:
    return ret;
}

static int gk_remove(struct platform_device *pdev)
{
    struct gk_glue    *glue = platform_get_drvdata(pdev);

    platform_device_del(glue->musb);
    platform_device_put(glue->musb);
    pdev->resource[0].parent = NULL;
    pdev->resource[0].sibling = NULL;
    pdev->resource[0].child = NULL;
    pdev->resource[1].parent = NULL;
    pdev->resource[1].sibling = NULL;
    pdev->resource[1].child = NULL;
    kfree(glue);

    return 0;
}

static struct platform_driver gk_driver = {
    .probe      = gk_probe,
    .remove     = gk_remove,
    .driver     = {
        .name   = "musb-gk",
    },
};

static int __init gk_init(void)
{
#ifdef CONFIG_GK_USB_OTG_MODE
    usb_ctrl_init();
#endif
    return platform_driver_register(&gk_driver);
}
module_init(gk_init);

static void __exit gk_exit(void)
{
    platform_driver_unregister(&gk_driver);
}
module_exit(gk_exit);

#ifdef CONFIG_GK_USB_OTG_MODE
static struct semaphore sema_usb_ctrl;
static u32 usb_ctrl_mode;
extern void set_usb_unregister(void);
extern void set_usb_master(void);
extern void set_usb_slave(void);

void set_usb_phy_unregister(void)
{
    platform_driver_unregister(&gk_driver);
}
EXPORT_SYMBOL(set_usb_phy_unregister);

void set_usb_phy_master(void)
{
    struct gk_gpio_io_info  usb_gpio_conf;
    usb_gpio_conf.gpio_id = gk_all_gpio_cfg.usb_host;
    usb_gpio_conf.active_level = GPIO_HIGH;
    usb_gpio_conf.active_delay = 1;

    gk_set_gpio_output(&usb_gpio_conf, 1);

    mdelay(10);

    platform_driver_register(&gk_driver);

}
EXPORT_SYMBOL(set_usb_phy_master);

void set_usb_phy_slave(void)
{
    struct gk_gpio_io_info  usb_gpio_conf;
    usb_gpio_conf.gpio_id = gk_all_gpio_cfg.usb_host;
    usb_gpio_conf.active_level = GPIO_LOW;
    usb_gpio_conf.active_delay = 1;

    gk_set_gpio_output(&usb_gpio_conf, 1);

    mdelay(10);

    platform_driver_register(&gk_driver);
}

EXPORT_SYMBOL(set_usb_phy_slave);

static irqreturn_t usb_ctrl_irq_handle(int irq, void *dummy)
{
    printk("usb_ctrl_irq_handle\n");
    printk("irq : %d\n", gk_gpio_get(CONFIG_GK_USB_OTG_GPIO));

    if (gk_gpio_get(CONFIG_GK_USB_OTG_GPIO))
    {
        usb_ctrl_mode = 1;
    }
    else
    {
        usb_ctrl_mode = 0;
    }

    mdelay(1);

    up(&sema_usb_ctrl);
}

static void usb_ctrl_task(struct task_struct *p)
{
    while(1)
    {
        down(&sema_usb_ctrl);
        if (usb_ctrl_mode == 1)
        {
            printk("usb entry slave mode\n");
            set_usb_unregister();
            set_usb_phy_unregister();

            set_usb_phy_slave();
            set_usb_slave();
        }
        else
        {
            printk("usb entry master mode\n");
            set_usb_unregister();
            set_usb_phy_unregister();

            set_usb_phy_master();
            set_usb_master();
        }
    }
}

static int usb_ctrl_init(void)
{
    static struct gk_gpio_irq_info usb_ctrl_irq;
#if 0
    static struct gk_gpio_io_info  usb_ctrl_io;
    usb_ctrl_io.gpio_id = CONFIG_GK_USB_OTG_GPIO;
    usb_ctrl_io.active_level = 1;
    usb_ctrl_io.active_delay = 10;

    printk("GPIO%d : %d\n", CONFIG_GK_USB_OTG_GPIO, gk_get_gpio_input(&usb_ctrl_io));
#endif

    usb_ctrl_irq.pin        = CONFIG_GK_USB_OTG_GPIO;
    usb_ctrl_irq.type       = IRQ_TYPE_EDGE_BOTH;
    usb_ctrl_irq.handler    = usb_ctrl_irq_handle;

    sema_init(&sema_usb_ctrl, 0);
    if (gk_gpio_get(CONFIG_GK_USB_OTG_GPIO))
    {
        usb_ctrl_mode = 1;
    }
    else
    {
        usb_ctrl_mode = 0;
    }

    kernel_thread(usb_ctrl_task, NULL, 0);
    gk_gpio_request_irq(&usb_ctrl_irq);
}
#endif

MODULE_DESCRIPTION("GOKE MUSB driver");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_LICENSE("GPL v2");

