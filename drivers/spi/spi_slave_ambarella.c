/*
 * linux/drivers/spi/spi_slave_ambarella.c
 *
 * History:
 *	2012/02/21 - [Zhenwu Xue]  created file
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <plat/spi.h>

#define MAX_SPI_SLAVES		8

#define SPI_RXFIS_MASK 		0x00000010
#define SPI_RXOIS_MASK 		0x00000008
#define SPI_RXUIS_MASK 		0x00000004

#define BUFFER_LEN		4096

#define DUMMY_DATA		0xffff

static int rx_ftlr = 8;
module_param(rx_ftlr, int, 0644);

struct ambarella_spi_slave {
	void __iomem				*regbase;

	int					major;
	char					dev_name[32];
	struct class				*psClass;
	struct device				*psDev;

	u16					mode;
	u16					bpw;

	int					count;
	struct mutex				um_mtx;

	u16					r_buf[BUFFER_LEN];
	u16					r_buf_start;
	u16					r_buf_end;
	u16					r_buf_empty;
	u16					r_buf_full;
	u16					r_buf_round;
	spinlock_t				r_buf_lock;

	u16					w_buf[BUFFER_LEN];
	u16					w_buf_start;
	u16					w_buf_end;
	u16					w_buf_empty;
	u16					w_buf_full;
	u16					w_buf_round;
	spinlock_t				w_buf_lock;
};

DEFINE_MUTEX(g_mtx);
static int amb_slave_id = 0;
static struct ambarella_spi_slave	*priv_array[MAX_SPI_SLAVES] =
	{NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static int ambarella_spi_slave_inithw(struct ambarella_spi_slave *priv)
{
	struct clk *clk;
	u32 ctrlr0;

	clk = clk_get(NULL, "gclk_ssi2");
	if (!IS_ERR_OR_NULL(clk))
		clk_set_rate(clk, 13500000);

	/* Initial Register Settings */
	ctrlr0 = (SPI_CFS << 12) | (SPI_WRITE_READ << 8) | (priv->mode << 6) |
				(SPI_FRF << 4) | (priv->bpw - 1);
	amba_writel(priv->regbase + SPI_CTRLR0_OFFSET, ctrlr0);

	amba_writel(priv->regbase + SPI_TXFTLR_OFFSET, 0);
	amba_writel(priv->regbase + SPI_RXFTLR_OFFSET, rx_ftlr - 1);
	amba_writel(priv->regbase + SPI_IMR_OFFSET, SPI_RXFIS_MASK);

	return 0;
}

static irqreturn_t ambarella_spi_slave_isr(int irq, void *dev_data)
{
	struct ambarella_spi_slave	*priv	= dev_data;
	u32				i, rxflr, txflr;

	if (amba_readl(priv->regbase + SPI_ISR_OFFSET)) {
		txflr = 16 - amba_readl(priv->regbase + SPI_TXFLR_OFFSET);
		spin_lock(&priv->w_buf_lock);
		if (priv->w_buf_empty) {
			for (i = 0; i < txflr; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, DUMMY_DATA);
			}
		} else {
			priv->w_buf_full = 0;
			if (priv->w_buf_round & 0x01) {
				if (BUFFER_LEN - priv->w_buf_start > txflr) {
					for (i = 0; i < txflr; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
					}
				} else if (BUFFER_LEN - priv->w_buf_start + priv->w_buf_end > txflr) {
					for (i = 0; i < BUFFER_LEN - priv->w_buf_start; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
					}
					for (i = 0; i < txflr + priv->w_buf_start - BUFFER_LEN; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[i]);
					}
					priv->w_buf_start = i + 1;
					priv->w_buf_round++;
				} else {
					for (i = 0; i < BUFFER_LEN - priv->w_buf_start; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
					}
					for (i = 0; i < priv->w_buf_end; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[i]);
					}
					for (i = 0; i < txflr + priv->w_buf_start - BUFFER_LEN - priv->w_buf_end; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, DUMMY_DATA);
					}
					priv->w_buf_empty = 1;
				}
			} else {
				if (priv->w_buf_end - priv->w_buf_start > txflr) {
					for (i = 0; i < txflr; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
					}
				} else {
					for (i = 0; i < priv->w_buf_end - priv->w_buf_start; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
					}
					for (i = 0; i < txflr - priv->w_buf_end + priv->w_buf_start; i++) {
						amba_writel(priv->regbase + SPI_DR_OFFSET, DUMMY_DATA);
					}
					priv->w_buf_empty = 1;
				}
			}
		}
		spin_unlock(&priv->w_buf_lock);

		rxflr = amba_readl(priv->regbase + SPI_RXFLR_OFFSET);
		spin_lock(&priv->r_buf_lock);
		if (priv->r_buf_empty) {
			priv->r_buf_start	= 0;
			priv->r_buf_end		= 0;
			priv->r_buf_empty	= 0;
		}

		for (i = 0; i < rxflr; i++) {
			if (priv->r_buf_full) {
				printk("%s: Rx buffer overflow!!\n", __FILE__);
				priv->r_buf_start++;
				if (priv->r_buf_start >= BUFFER_LEN) {
					priv->r_buf_start = 0;
					priv->r_buf_round++;
				}
			}

			priv->r_buf[priv->r_buf_end++] = amba_readl(priv->regbase + SPI_DR_OFFSET);
			if (priv->r_buf_end >= BUFFER_LEN) {
				priv->r_buf_end = 0;
				priv->r_buf_round++;
			}

			if (priv->r_buf_round & 0x01) {
				if (priv->r_buf_end >= priv->r_buf_start) {
					priv->r_buf_full = 1;
				}
			}
		}
		spin_unlock(&priv->r_buf_lock);

		amba_writel(priv->regbase + SPI_ISR_OFFSET, 0);
	}

	return IRQ_HANDLED;
}

static int spi_slave_open(struct inode *inode, struct file *filp)
{
	int				i, ret = 0;
	struct ambarella_spi_slave	*priv = NULL;

	mutex_lock(&g_mtx);

	for (i = 0; i < MAX_SPI_SLAVES; i++) {
		if (priv_array[i] && MKDEV(priv_array[i]->major, 0) == inode->i_rdev) {
			priv = priv_array[i];
			break;
		}
	}
	mutex_lock(&priv->um_mtx);
	if (!priv->count) {
		priv->count++;
		filp->private_data = priv;
	} else {
		ret = -ENXIO;
	}
	mutex_unlock(&priv->um_mtx);

	mutex_unlock(&g_mtx);

	return ret;
}

static ssize_t
spi_slave_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t				 ret = 0;
	struct ambarella_spi_slave	*priv;
	int				result;

	priv = (struct ambarella_spi_slave *)filp->private_data;

	spin_lock(&priv->r_buf_lock);

	if (!priv->r_buf_empty) {
		if (priv->r_buf_round & 0x01) {
			if ((BUFFER_LEN - priv->r_buf_start) * 2 >= count) {
				result = copy_to_user(buf, (void *)&priv->r_buf[priv->r_buf_start], count);
				priv->r_buf_start += count / 2;
				ret = count;
			} else if ((BUFFER_LEN - priv->r_buf_start + priv->r_buf_end) * 2 >= count) {
				result = copy_to_user(buf, (void *)&priv->r_buf[priv->r_buf_start], (BUFFER_LEN - priv->r_buf_start) * 2);
				result = copy_to_user(buf + (BUFFER_LEN - priv->r_buf_start) * 2, (void *)priv->r_buf, count - (BUFFER_LEN - priv->r_buf_start) * 2);
				priv->r_buf_start = BUFFER_LEN - priv->r_buf_start + priv->r_buf_end - count / 2;
				priv->r_buf_round++;
				ret = count;
			} else {
				result = copy_to_user(buf, (void *)&priv->r_buf[priv->r_buf_start], (BUFFER_LEN - priv->r_buf_start) * 2);
				result = copy_to_user(buf + (BUFFER_LEN - priv->r_buf_start) * 2, (void *)priv->r_buf, priv->r_buf_end * 2);
				priv->r_buf_start = priv->r_buf_end;
				priv->r_buf_round++;
				ret = (BUFFER_LEN - priv->r_buf_start + priv->r_buf_end) * 2;
			}
		} else {
			if ((priv->r_buf_end - priv->r_buf_start) * 2 > count) {
				ret = count;
			} else {
				ret = (priv->r_buf_end - priv->r_buf_start) * 2;
			}
			result = copy_to_user(buf, (void *)&priv->r_buf[priv->r_buf_start], ret);
			priv->r_buf_start += ret / 2;
		}

		if (priv->r_buf_start >= BUFFER_LEN) {
			priv->r_buf_start = 0;
			priv->r_buf_round++;
		}
	}

	spin_unlock(&priv->r_buf_lock);

	return ret;
}

static ssize_t
spi_slave_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t				 ret = 0;
	struct ambarella_spi_slave	*priv;
	int				txflr, i, result;

	priv = (struct ambarella_spi_slave *)filp->private_data;

	spin_lock(&priv->w_buf_lock);

	if (!priv->w_buf_full) {
		if (priv->w_buf_empty) {
			priv->w_buf_empty = 0;

			if (count < 2 * BUFFER_LEN) {
				ret = count;
				priv->w_buf_start = 0;
				priv->w_buf_end = count / 2;
				priv->w_buf_round = 0;
				result = copy_from_user(priv->w_buf, buf, ret);
			} else {
				ret = 2 * BUFFER_LEN;
				priv->w_buf_full = 1;
				priv->w_buf_start = 0;
				priv->w_buf_end = 0;
				priv->w_buf_round = 1;
				result = copy_from_user(priv->w_buf, buf, ret);
			}
		} else {
			if (priv->w_buf_round & 0x01) {
				if (count < (priv->w_buf_start - priv->w_buf_end) * 2) {
					ret = count;
					result = copy_from_user((void *)&priv->w_buf[priv->w_buf_end], buf, ret);
					priv->w_buf_end += ret / 2;
				} else {
					ret = (priv->w_buf_start - priv->w_buf_end) * 2;
					result = copy_from_user(&priv->w_buf[priv->w_buf_end], buf, ret);
					priv->w_buf_end = priv->w_buf_start;
					priv->w_buf_full = 1;
				}
			} else {
				if (count <= (BUFFER_LEN - priv->w_buf_end) * 2) {
					ret = count;
					result = copy_from_user((void *)&priv->w_buf[priv->w_buf_end], buf, ret);
					priv->w_buf_end += ret / 2;
				} else if (count < (BUFFER_LEN + priv->w_buf_start - priv->w_buf_end) * 2) {
					ret = count;
					result = copy_from_user((void *)&priv->w_buf[priv->w_buf_end], buf, (BUFFER_LEN - priv->w_buf_end) * 2);
					result = copy_from_user((void *)priv->w_buf, buf + (BUFFER_LEN - priv->w_buf_end) * 2, ret - (BUFFER_LEN - priv->w_buf_end) * 2);
					priv->w_buf_end = priv->w_buf_end + ret / 2 - BUFFER_LEN;
					priv->w_buf_round++;
				} else {
					ret = (BUFFER_LEN + priv->w_buf_start - priv->w_buf_end) * 2;
					result = copy_from_user((void *)&priv->w_buf[priv->w_buf_end], buf, (BUFFER_LEN - priv->w_buf_end) * 2);
					result = copy_from_user((void *)priv->w_buf, buf + (BUFFER_LEN - priv->w_buf_end) * 2, ret - (BUFFER_LEN - priv->w_buf_end) * 2);
					priv->w_buf_end = priv->w_buf_start;
					priv->w_buf_round++;
					priv->w_buf_full = 1;
				}
			}
		}

		if (priv->w_buf_end >= BUFFER_LEN) {
			priv->w_buf_end = 0;
			priv->w_buf_round++;
		}
	}

	txflr = 16 - amba_readl(priv->regbase + SPI_TXFLR_OFFSET);
	priv->w_buf_full = 0;
	if (priv->w_buf_round & 0x01) {
		if (BUFFER_LEN - priv->w_buf_start > txflr) {
			for (i = 0; i < txflr; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
			}
		} else if (BUFFER_LEN - priv->w_buf_start + priv->w_buf_end > txflr) {
			for (i = 0; i < BUFFER_LEN - priv->w_buf_start; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
			}
			for (i = 0; i < txflr + priv->w_buf_start - BUFFER_LEN; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[i]);
			}
			priv->w_buf_start = i + 1;
			priv->w_buf_round++;
		} else {
			for (i = 0; i < BUFFER_LEN - priv->w_buf_start; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
			}
			for (i = 0; i < priv->w_buf_end; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[i]);
			}
			for (i = 0; i < txflr + priv->w_buf_start - BUFFER_LEN - priv->w_buf_end; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, DUMMY_DATA);
			}
			priv->w_buf_empty = 1;
		}
	} else {
		if (priv->w_buf_end - priv->w_buf_start > txflr) {
			for (i = 0; i < txflr; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
			}
		} else {
			for (i = 0; i < priv->w_buf_end - priv->w_buf_start; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, priv->w_buf[priv->w_buf_start++]);
			}
			for (i = 0; i < txflr - priv->w_buf_end + priv->w_buf_start; i++) {
				amba_writel(priv->regbase + SPI_DR_OFFSET, DUMMY_DATA);
			}
			priv->w_buf_empty = 1;
		}
	}

	spin_unlock(&priv->w_buf_lock);

	return ret;
}

long spi_slave_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int			err = -ENOIOCTLCMD;

	switch(cmd)
	{
	default:
		break;
	}

	return err;
}

static int spi_slave_release(struct inode *inode, struct file *filp)
{
	int				ret = 0;
	struct ambarella_spi_slave	*priv;

	priv = (struct ambarella_spi_slave *)filp->private_data;

	mutex_lock(&priv->um_mtx);
	if (priv->count) {
		priv->count--;
	}
	mutex_unlock(&priv->um_mtx);

	return ret;
}

static struct file_operations spi_slave_fops = {
	.open		= spi_slave_open,
	.read		= spi_slave_read,
	.write		= spi_slave_write,
	.unlocked_ioctl	= spi_slave_ioctl,
	.release	= spi_slave_release,
};

static int ambarella_spi_slave_probe(struct platform_device *pdev)
{
	struct ambarella_spi_slave		*priv = NULL;
	struct resource 			*res;
	int					irq, i, rval = 0;
	int					major_id = 0;
	void __iomem				*reg;

	/* Get Base Address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "get mem resource failed\n");
		return -ENOENT;
	}

	reg = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!reg) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	/* Get IRQ NO. */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "get irq failed\n");
		return -ENOENT;
	}

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "no memory\n");
		return -ENOMEM;
	}

	priv->regbase = reg;

	priv->mode = 3;
	priv->bpw = 16;

	priv->r_buf_empty = 1;
	priv->r_buf_full = 0;
	priv->w_buf_empty = 1;
	priv->w_buf_full = 0;

	spin_lock_init(&priv->r_buf_lock);
	spin_lock_init(&priv->w_buf_lock);
	mutex_init(&priv->um_mtx);

	ambarella_spi_slave_inithw(priv);

	/* Request IRQ */
	rval = devm_request_irq(&pdev->dev, irq, ambarella_spi_slave_isr,
				IRQF_TRIGGER_HIGH, dev_name(&pdev->dev), priv);
	if (rval) {
		rval = -EIO;
		goto ambarella_spi_slave_probe_exit;
	}

	pdev->id = amb_slave_id++;
	sprintf(priv->dev_name, "slave_spi%d", pdev->id);
	major_id = register_chrdev(0, priv->dev_name, &spi_slave_fops);
	if (major_id <= 0) {
		rval = -EIO;
		printk("Error: %s: Unable to get major number\n", __FILE__);
		goto ambarella_spi_slave_probe_exit;
	}

	priv->major = major_id;

	priv->psClass = class_create(THIS_MODULE, priv->dev_name);
	if (IS_ERR(priv->psClass)) {
		rval = -EIO;
		printk("Error: %s: Unable to create class\n", __FILE__);
		goto ambarella_spi_slave_probe_exit;
	}

	priv->psDev = device_create(priv->psClass, NULL, MKDEV(major_id, 0),
					priv, priv->dev_name);
	if (IS_ERR(priv->psDev)) {
		rval = -EIO;
		printk("Error: %s: Unable to create device\n", __FILE__);
		goto ambarella_spi_slave_probe_exit;
	}

	platform_set_drvdata(pdev, priv);

	if (pdev->id < MAX_SPI_SLAVES) {
		mutex_lock(&g_mtx);
		priv_array[pdev->id] = priv;
		mutex_unlock(&g_mtx);
	} else {
		rval = -EIO;
		printk("Error: %s: platform device id %d is greater than %d\n",
			__FILE__, pdev->id, MAX_SPI_SLAVES - 1);
		goto ambarella_spi_slave_probe_exit;
	}

	dev_info(&pdev->dev, "Ambarella SPI Slave Controller %d created!\n", pdev->id);

	amba_writel(priv->regbase + SPI_SSIENR_OFFSET, 1);
	for (i = 0; i < 16; i++) {
		amba_writel(priv->regbase + SPI_DR_OFFSET, i);
	}

	return 0;

ambarella_spi_slave_probe_exit:
	if (rval < 0) {
		if (priv->psDev) {
			device_destroy(priv->psClass, MKDEV(major_id, 0));
		}
		if (priv->psClass) {
			class_destroy(priv->psClass);
		}
		if (major_id > 0) {
			unregister_chrdev(major_id, priv->dev_name);
		}
	}

	return rval;
}

static int ambarella_spi_slave_remove(struct platform_device *pdev)
{
	struct ambarella_spi_slave	*priv;

	priv = platform_get_drvdata(pdev);

	mutex_lock(&g_mtx);
	priv_array[pdev->id] = NULL;
	mutex_unlock(&g_mtx);

	mutex_lock(&priv->um_mtx);

	if (priv->psDev) {
		device_destroy(priv->psClass, MKDEV(priv->major, 0));
	}
	if (priv->psClass) {
		class_destroy(priv->psClass);
	}
	if (priv->major > 0) {
		unregister_chrdev(priv->major, priv->dev_name);
	}

	mutex_unlock(&priv->um_mtx);

	return 0;
}

#ifdef CONFIG_PM
static int ambarella_spi_slave_suspend_noirq(struct device *dev)
{
	return 0;
}

static int ambarella_spi_slave_resume_noirq(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops ambarella_spi_slave_dev_pm_ops = {
	.suspend_noirq = ambarella_spi_slave_suspend_noirq,
	.resume_noirq = ambarella_spi_slave_resume_noirq,
};
#endif

static const struct of_device_id ambarella_spi_slave_dt_ids[] = {
	{.compatible = "ambarella,spi-slave", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_spi_slave_dt_ids);

static struct platform_driver ambarella_spi_slave_driver = {
	.probe		= ambarella_spi_slave_probe,
	.remove		= ambarella_spi_slave_remove,
	.driver		= {
		.name	= "ambarella-spi-slave",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_spi_slave_dt_ids,
#ifdef CONFIG_PM
		.pm	= &ambarella_spi_slave_dev_pm_ops,
#endif
	},
};

static int __init ambarella_spi_slave_init(void)
{
	return platform_driver_register(&ambarella_spi_slave_driver);
}

static void __exit ambarella_spi_slave_exit(void)
{
	platform_driver_unregister(&ambarella_spi_slave_driver);
}

subsys_initcall(ambarella_spi_slave_init);
module_exit(ambarella_spi_slave_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SPI Slave Controller");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("GPL");
