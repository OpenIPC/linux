// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the JD9853 LCD Controller
 */
#define DEBUG
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <video/mipi_display.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#include "fbtft.h"

#define DRVNAME "fb_jd9853"
#define JD9853_IPS_GAMMA ""
#define TIMER_INV_MS 5000

static int debug_on = 0;
module_param(debug_on, int, 0644);
MODULE_PARM_DESC(debug_on, "Debug mode 0=off other=on (default=off)");

static inline bool dbgon(void)
{
	return !(debug_on == 0);
}

struct jd9853_priv_data {
	struct device *dev;
	struct fbtft_par *par;
	struct mutex io_lock;

	int gpio_te;
	int irq_te;
	bool te_enabled;
	atomic_t _pending_frame;

	struct workqueue_struct *te_wq;
    struct work_struct te_work;
	struct work_struct rst_work;
	struct timer_list te_timer;

	u8* frame_buf;
	u8* tx_buf;
    size_t frame_buf_size;
    spinlock_t frame_lock;

	atomic_t te_irq_count;
	int prev_te_irq_count;
};

static void te_work_handler(struct work_struct *work);
static irqreturn_t jd9853_te_irq_handler(int irq, void *dev_id);
static void lcd_reset_handler(struct work_struct *work);
static void te_timer_handler(struct timer_list *t);
static int set_var(struct fbtft_par *par);
static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye);
static int jd9853_fbtft_write_buf16_bus8(struct fbtft_par *par, const u16 *src, size_t len);
static void memcpy_reverse32(u8 *dst, const u8 *src, size_t len);
static int write_vmem_te(struct fbtft_par *par, size_t offset, size_t len);
static void init_lcd(struct jd9853_priv_data* panel);

static inline bool panel_pending_frame(struct jd9853_priv_data *panel)
{
    return atomic_read(&panel->_pending_frame) == 1;
}

static int init_display(struct fbtft_par *par)
{
	struct device *spidev = &par->spi->dev;
    struct device_node *np = spidev->of_node;
	struct jd9853_priv_data* panel;
	int ret = -1;

	dev_info(spidev, "debug mode: %s", dbgon() ? "on" : "off");

	panel = kzalloc(sizeof(*panel), GFP_KERNEL);
	if (!panel) {
		dev_err(spidev, "Failed to kzalloc(struct jd9853_priv_data)");
		return -ENOMEM;
	}

	panel->dev = &par->spi->dev;
	panel->par = par;
	panel->gpio_te = -1;
	panel->irq_te = -1;
	panel->te_enabled = false;
	atomic_set(&panel->_pending_frame, 0);
	atomic_set(&panel->te_irq_count, 0);

	mutex_init(&panel->io_lock);

	init_lcd(panel);
	
	if (np) {
        panel->gpio_te = of_get_named_gpio(np, "te-gpios", 0);
        if (panel->gpio_te < 0) {
            dev_warn(spidev, "No te-gpios defined in DT, skipping TE setup\n");
            panel->gpio_te = -1;
        } else {
			if (dbgon()) {
				dev_info(spidev, "Found TE gpio: %d\n", panel->gpio_te);
			}
        }
    }

	if (gpio_is_valid(panel->gpio_te)) {
		panel->irq_te = gpio_to_irq(panel->gpio_te);
		if (panel->irq_te < 0) {
			dev_err(spidev, "Failed to get IRQ from GPIO %d\n", panel->gpio_te);
			ret = panel->irq_te;
            goto err_free_panel;
		}

		panel->te_wq = alloc_workqueue("jd9853_te_wq", WQ_HIGHPRI | WQ_UNBOUND, 1);
        if (!panel->te_wq) {
            dev_err(spidev, "Failed to create workqueue\n");
            ret = -ENOMEM;
            goto err_free_panel;
        }

        INIT_WORK(&panel->te_work, te_work_handler);
		INIT_WORK(&panel->rst_work, lcd_reset_handler);
		timer_setup(&panel->te_timer, te_timer_handler, 0);
		mod_timer(&panel->te_timer, jiffies + msecs_to_jiffies(TIMER_INV_MS));

		panel->frame_buf_size = par->info->fix.smem_len;
		panel->frame_buf = kzalloc(panel->frame_buf_size, GFP_KERNEL);
		if (!panel->frame_buf) {
			dev_err(spidev, "Failed to allocate frame buffer\n");
			ret = -ENOMEM;
            goto err_destroy_wq;
		}
		if (dbgon()) {
			dev_info(spidev, "1: Alloc %ld bytes mem\n", panel->frame_buf_size);
		}
		panel->tx_buf = kzalloc(panel->frame_buf_size, GFP_KERNEL);
		if (!panel->tx_buf) {
			dev_err(spidev, "Failed to allocate frame buffer\n");
			ret = -ENOMEM;
            goto err_free_frame_buf;
		}
		if (dbgon()) {
			dev_info(spidev, "2: Alloc %ld bytes mem\n", panel->frame_buf_size);
		}
		spin_lock_init(&panel->frame_lock);

        ret = request_irq(panel->irq_te, jd9853_te_irq_handler,
                          IRQF_TRIGGER_FALLING, "jd9853_te", par);
		
		if (ret) {
			dev_err(spidev, "Failed to request TE IRQ: %d\n", ret);
			goto err_free_tx_buf;
		}
	
		panel->te_enabled = true;
		if (dbgon()) {
			dev_info(spidev, "TE IRQ registered on GPIO %d (IRQ %d)\n",
						panel->gpio_te, panel->irq_te);
		}
	}
	dev_set_drvdata(spidev, panel);

	return 0;

err_free_tx_buf:
    kfree(panel->tx_buf);
err_free_frame_buf:	
    kfree(panel->frame_buf);
err_destroy_wq:
    destroy_workqueue(panel->te_wq);
err_free_panel:
    dev_set_drvdata(spidev, NULL);
    kfree(panel);
    return ret;
}

static void jd9853_unregister(struct fbtft_par *par)
{
    struct jd9853_priv_data *panel = dev_get_drvdata(&par->spi->dev);
    if (!panel)
        return;

    dev_set_drvdata(&par->spi->dev, NULL);
    cancel_work_sync(&panel->te_work);
    cancel_work_sync(&panel->rst_work);

    if (panel->te_enabled) {
        del_timer_sync(&panel->te_timer);
        free_irq(panel->irq_te, par);

        if (panel->te_wq) {
            flush_workqueue(panel->te_wq);
            destroy_workqueue(panel->te_wq);
        }
    }

    kfree(panel->frame_buf);
    kfree(panel->tx_buf);
    kfree(panel);
}

static int set_var(struct fbtft_par *par)
{
#define MADCTL_MH	BIT(2)
#define MADCTL_BGR	BIT(3)
#define MADCTL_ML	BIT(4)
#define MADCTL_MV	BIT(5)
#define MADCTL_MX	BIT(6)
#define MADCTL_MY	BIT(7)

	struct device *dev = par->info->device;
	struct jd9853_priv_data *panel = dev_get_drvdata(&par->spi->dev);
	u8 madctl_par = 0;

	if (!panel) {
		dev_err(&par->spi->dev, "set_var(): panel is NULL\n");
		return -EINVAL;
	}

	if (par->bgr) {
		if (dbgon()) {
			dev_info(dev, "use BGR");
		}
		madctl_par |= MADCTL_BGR;
	} else {
		if (dbgon()) {
			dev_info(dev, "use RGB");
		}
	}
		
	switch (par->info->var.rotate) {
	case 0:
		break;
	case 90:
		madctl_par |= (MADCTL_MV | MADCTL_MY);
		break;
	case 180:
		madctl_par |= (MADCTL_MX | MADCTL_MY);
		break;
	case 270:
		madctl_par |= (MADCTL_MV | MADCTL_MX);
		break;
	default:
		return -EINVAL;
	}
	mutex_lock(&panel->io_lock);
	write_reg(par, 0x36, madctl_par);
	mutex_unlock(&panel->io_lock);

	if (dbgon()) {
		dev_dbg(dev, "madctl: 0x%02x", madctl_par);
	}
	return 0;
}

static int set_gamma(struct fbtft_par *par, u32 *curves)
{
	(void)par;
	(void)curves;
	return 0;
}

static int blank(struct fbtft_par *par, bool on)
{
	struct jd9853_priv_data *panel = dev_get_drvdata(&par->spi->dev);
	if (!panel) {
		dev_err(&par->spi->dev, "blank(): panel is NULL\n");
		return -EINVAL;
	}
	
	mutex_lock(&panel->io_lock);
	write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
	mutex_unlock(&panel->io_lock);
	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	struct jd9853_priv_data *panel = dev_get_drvdata(&par->spi->dev);
	if (!panel) {
		dev_err(&par->spi->dev, "set_addr_win(): panel is NULL\n");
		return;
	}

	mutex_lock(&panel->io_lock);
    switch (par->info->var.rotate) {
	case 0:
        write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 90:
		write_reg(par, 0x2b, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2a, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 180:
		write_reg(par, 0x2a, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2b, 0x00, 0x00, 0x01, 0x3f);
		break;
	case 270:
		write_reg(par, 0x2b, 0x00, 0x22, 0x00, 0xcd);
        write_reg(par, 0x2a, 0x00, 0x00, 0x01, 0x3f);
		break;
	default:
		return;
	}

    write_reg(par, 0x35, 0x00);
    write_reg(par, 0x2c);
	mutex_unlock(&panel->io_lock);
}

static int jd9853_fbtft_write_buf16_bus8(struct fbtft_par *par, const u16 *src, size_t len)
{
	__be16 *txbuf16 = par->txbuf.buf;
	size_t remain;
	size_t to_copy;
	size_t tx_array_size;
	int i;
	int ret = 0;
	size_t startbyte_size = 0;

	remain = len / 2;

	if (par->gpio.dc != -1)
		gpio_set_value(par->gpio.dc, 1);

	if (!par->txbuf.buf)
		return par->fbtftops.write(par, src, len);

	tx_array_size = par->txbuf.len / 2;

	if (par->startbyte) {
		txbuf16 = par->txbuf.buf + 1;
		tx_array_size -= 2;
		*(u8 *)(par->txbuf.buf) = par->startbyte | 0x2;
		startbyte_size = 1;
	}

	while (remain) {
		to_copy = min(tx_array_size, remain);
		for (i = 0; i < to_copy; i++)
			txbuf16[i] = cpu_to_be16(src[i]);

		src += to_copy;
		ret = par->fbtftops.write(par, par->txbuf.buf,
					 startbyte_size + to_copy * 2);
		if (ret < 0)
			return ret;
		remain -= to_copy;
	}

	return ret;
}

static void memcpy_reverse32(u8 *dst, const u8 *src, size_t len)
{
    size_t i;

    for (i = 0; i + 3 < len; i += 4) {
        dst[i]     = src[i + 3];
        dst[i + 1] = src[i + 2];
        dst[i + 2] = src[i + 1];
        dst[i + 3] = src[i];
    }
}

static int write_vmem_te(struct fbtft_par *par, size_t offset, size_t len)
{
    struct jd9853_priv_data *panel = dev_get_drvdata(&par->spi->dev);

	if (!panel || !panel->te_enabled)
        return fbtft_write_vmem16_bus8(par, offset, len);

	if (dbgon()) {
		dev_info(&par->spi->dev, "TE IRQ count = %d\n", atomic_read(&panel->te_irq_count));
	}

    spin_lock(&panel->frame_lock);
    memcpy_reverse32(panel->frame_buf, par->info->screen_base, panel->frame_buf_size);
    spin_unlock(&panel->frame_lock);

	atomic_set(&panel->_pending_frame, 1);

    return 0;
}

static void init_lcd(struct jd9853_priv_data* panel)
{
	struct fbtft_par *par;
	int i = 0;

	if (panel == NULL) {
		return;
	}
	par = panel->par;

	mutex_lock(&panel->io_lock);
	for (i = 0;i < 2; ++i) {
		mdelay(100);

		par->fbtftops.reset(par);
		mdelay(50);

		write_reg(par, 0xDF, 0x98, 0x53);
		write_reg(par, 0xB2, 0x23);
		write_reg(par, 0xB7, 0x00, 0x47, 0x00, 0x6F);
		write_reg(par, 0xBB, 0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0);
		write_reg(par, 0xC0, 0x44, 0xA4);

		write_reg(par, 0xC1, 0x12);
		write_reg(par, 0xC3, 0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77);
		write_reg(par, 0xC4, 0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82);
		write_reg(par, 0xC8, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00);
		write_reg(par, 0xD0, 0x04, 0x06, 0x6B, 0x0F, 0x00);

		write_reg(par, 0xD7, 0x00, 0x30);
		write_reg(par, 0xE6, 0x14);
		write_reg(par, 0xDE, 0x01);
		write_reg(par, 0xB7, 0x03, 0x13, 0xEF, 0x35, 0x35);
		write_reg(par, 0xC1, 0x14, 0x15, 0xC0);

		write_reg(par, 0xC2, 0x06, 0x3A);
		write_reg(par, 0xC4, 0x72, 0x12);
		write_reg(par, 0xBE, 0x00);
		write_reg(par, 0xDE, 0x02);
		write_reg(par, 0xE5, 0x00, 0x02, 0x00);

		write_reg(par, 0xE5, 0x01, 0x02, 0x00);
		write_reg(par, 0xDE, 0x00);
		write_reg(par, 0x34);
		write_reg(par, 0x44, 0x00, 0x00);
		write_reg(par, 0x35, 0x00);

		write_reg(par, 0x44, 0x00, 0x00);
		
		write_reg(par, 0x3A, 0x55);	// 16bit 565

		mutex_unlock(&panel->io_lock);
		set_var(par);
		set_addr_win(par,0,0,0,0);
		mutex_lock(&panel->io_lock);
		write_reg(par, 0x11);

		mdelay(120);

		write_reg(par, 0xDE, 0x02);
		write_reg(par, 0xE5, 0x00, 0x02, 0x00);
		write_reg(par, 0xDE, 0x00);
		write_reg(par, 0x29);

		mdelay(10);
	}
	mutex_unlock(&panel->io_lock);
}

static irqreturn_t jd9853_te_irq_handler(int irq, void *dev_id)
{
	struct fbtft_par *par = dev_id;
    struct jd9853_priv_data *panel;

    if (!par) {
		return IRQ_NONE;
	}
        
    panel = dev_get_drvdata(&par->spi->dev);
    if (!panel) {
		return IRQ_NONE;
	}

	if (!panel->te_enabled) {
		return IRQ_NONE;
	}

    atomic_inc(&panel->te_irq_count);

    if (panel_pending_frame(panel) && !work_pending(&panel->te_work)) {
        queue_work(panel->te_wq, &panel->te_work);
    }

    return IRQ_HANDLED;
}

static void te_work_handler(struct work_struct *work)
{
    struct jd9853_priv_data *panel = container_of(work, struct jd9853_priv_data, te_work);
    struct fbtft_par *par = panel->par;

    if (!panel || !par) {
		return;
	}
        
    if (!panel_pending_frame(panel)) {
		dev_warn(panel->dev, "(!panel->_pending_frame) == true\n");
		return;
	}

    spin_lock(&panel->frame_lock);
    memcpy(panel->tx_buf, panel->frame_buf, panel->frame_buf_size);
    spin_unlock(&panel->frame_lock);
	atomic_set(&panel->_pending_frame, 0);

	mutex_lock(&panel->io_lock);
    jd9853_fbtft_write_buf16_bus8(par, (u16*)panel->tx_buf, panel->frame_buf_size);
	mutex_unlock(&panel->io_lock);

	if (dbgon()) {
		dev_info(panel->dev, "TE triggered frame write done\n");
	}
}

static void lcd_reset_handler(struct work_struct *work)
{
    struct jd9853_priv_data *panel = container_of(work, struct jd9853_priv_data, rst_work);
    struct fbtft_par *par = panel->par;

    if (!panel || !par){
		return;
	}

	init_lcd(panel);

	if (dbgon()) {
		dev_info(panel->dev, "LCD reset finish\n");
	}
}

static void te_timer_handler(struct timer_list *t)
{
	struct jd9853_priv_data *panel = container_of(t, struct jd9853_priv_data, te_timer);
	int cnt;

	if (!panel)
        return;

	cnt = atomic_read(&panel->te_irq_count);
	if (cnt == panel->prev_te_irq_count) {
        if (!work_pending(&panel->rst_work)) {
			queue_work(panel->te_wq, &panel->rst_work);
		}   
    } else {
        panel->prev_te_irq_count = cnt;
    }

    mod_timer(&panel->te_timer, jiffies + msecs_to_jiffies(TIMER_INV_MS));
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = 172,
	.height = 320,
	.gamma_num = 0,
	.gamma_len = 0,
	.gamma = JD9853_IPS_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.set_var = set_var,
		.set_gamma = set_gamma,
		.blank = blank,
        .set_addr_win = set_addr_win,
		.write_vmem = write_vmem_te,
		.unregister_backlight = jd9853_unregister,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "jadard,jd9853", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:jd9853");
MODULE_ALIAS("platform:jd9853");

MODULE_DESCRIPTION("FB driver for the JD9853 LCD Controller");
MODULE_AUTHOR("iawak9lkm");
MODULE_LICENSE("GPL");
