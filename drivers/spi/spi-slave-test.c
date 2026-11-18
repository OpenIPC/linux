/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
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
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/acpi.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY | SPI_TX_DUAL \
				| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD)

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
};

static unsigned bufsiz = 4096;
static char buf[4096];
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*-------------------------------------------------------------------------*/

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	int status;
	struct spi_device *spi;

	spin_lock_irq(&spidev->spi_lock);
	spi = spidev->spi;
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_sync(spi, message);

	if (status == 0)
		status = message->actual_length;
	else {
		pr_err("actual_length = %d\n", message->actual_length);
	}

	return status;
}

static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->rx_buffer,
			.len		= len,
			.speed_hz	= spidev->speed_hz,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct spidev_data	*slave)
{
	struct spidev_data	*spidev = slave;
	ssize_t			status = 0;

	int count = 32;
	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	mutex_lock(&spidev->buf_lock);
	status = spidev_sync_read(spidev, count);
#if 1
	if (status > 0) {
		unsigned long	missing;
		printk("%s:status:%d, spidev->rx_buffer:0x%x\n", __func__, status, spidev->rx_buffer[0]);
		missing = copy_to_user(buf, spidev->rx_buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
#endif
	mutex_unlock(&spidev->buf_lock);

	return status;
}

static int spidev_open(struct spidev_data *slave)
{
	struct spidev_data	*spidev = slave;
	int			status = -ENXIO;

	if (!spidev->tx_buffer) {
		spidev->tx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!spidev->tx_buffer) {
			dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_find_dev;
		}
	}

	if (!spidev->rx_buffer) {
		spidev->rx_buffer = kmalloc(bufsiz, GFP_KERNEL);
		if (!spidev->rx_buffer) {
			dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
			status = -ENOMEM;
			goto err_alloc_rx_buf;
		}
	}

	return 0;

err_alloc_rx_buf:
	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;
err_find_dev:
	return status;
}

static int spidev_release(struct spidev_data *slave)
{
	struct spidev_data	*spidev = slave;
	int		dofree;

	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;

	kfree(spidev->rx_buffer);
	spidev->rx_buffer = NULL;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi)
		spidev->speed_hz = spidev->spi->max_speed_hz;

	/* ... after we unbound from the underlying device? */
	dofree = (spidev->spi == NULL);
	spin_unlock_irq(&spidev->spi_lock);

	if (dofree)
		kfree(spidev);
#ifdef CONFIG_SPI_SLAVE
	spi_slave_abort(spidev->spi);
#endif

	return 0;
}

/*-------------------------------------------------------------------------*/

static int spi_slave_test(void *u)
{
	struct spidev_data *spidev = (struct spidev_data *)u;
	int			retval = 0;
	struct spi_device	*spi;
	u32			tmp, save;
	int i = 0;

	pr_err("Running....\n");
	spidev_open(spidev);

	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL) {
		pr_err("spi == NULL\n");
		return -ESHUTDOWN;
	}

	pr_err("Start set spi mode\n");
	tmp = 0;
	pr_err("spi mode: %x\n", tmp);
	save = spi->mode;
	if (tmp & ~SPI_MODE_MASK) {
		retval = -EINVAL;
		goto done;
	}

	tmp |= spi->mode & ~SPI_MODE_MASK;
	spi->mode = (u16)tmp;
	retval = spi_setup(spi);
	if (retval < 0)
		spi->mode = save;
	else
		dev_dbg(&spi->dev, "spi mode %x\n", tmp);

	pr_err("End set spi mode\n");

	pr_err("Start set spi bits per word\n");
	tmp = 8;
	pr_err("bits per word: %d\n", tmp);
	save = spi->bits_per_word;

	spi->bits_per_word = tmp;
	retval = spi_setup(spi);
	if (retval < 0)
		spi->bits_per_word = save;
	else
		dev_dbg(&spi->dev, "%d bits per word\n", tmp);

	pr_err("End set spi bits per word\n");


	pr_err("Start set spi max speed\n");
	tmp = 2; //
	pr_err("max speed: %dHz (%d KHz)\n", tmp, tmp/1000);
	save = spi->max_speed_hz;

	spi->max_speed_hz = tmp;
	retval = spi_setup(spi);
	if (retval >= 0)
		spidev->speed_hz = tmp;
	else
		dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
	spi->max_speed_hz = save;

	pr_err("End set spi max speed\n");

	while(1) {
		i++;
		pr_err("start read %d, %d\n", i, retval);
		retval = spidev_read(spidev);
		pr_err("read %d, %d\n", i, retval);
		if (retval != 32) {
			pr_err("read error %d, %d\n", i, retval);
			goto done;
		}

		if (i%1000 == 0) {
			pr_err("%d\n", i);
		}

	}

done:
	pr_err("Game Over!\n");
	spidev_release(spidev);
	return retval;
}

static int spidev_probe(struct spi_device *spi)
{
	struct spidev_data	*spidev;
	int			status = 0;

	struct task_struct *bgt_thread;

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	spidev->speed_hz = spi->max_speed_hz;

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);


	bgt_thread = kthread_run(spi_slave_test, spidev, "spi_slave_test");
	if (IS_ERR(bgt_thread)) {
		int err = PTR_ERR(bgt_thread);
		pr_err("can't create spi_slave_test thread, error = %d\n", err);
		status = err;
	}

	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spin_unlock_irq(&spidev->spi_lock);

	return 0;
}

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spi-slave",
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

/*-------------------------------------------------------------------------*/

static int __init spi_slave_test_init(void)
{
	return spi_register_driver(&spidev_spi_driver);
}
module_init(spi_slave_test_init);

static void __exit spi_slave_test_exit(void)
{
	spi_unregister_driver(&spidev_spi_driver);
}
module_exit(spi_slave_test_exit);

MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("spi slave test");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spi-slave");
