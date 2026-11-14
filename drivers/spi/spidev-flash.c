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

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/uaccess.h>

#define SPI_FLASH

#define OPCODE_WREN             0x06    /* Write Enable */

#define OPCODE_RDSR1            0x05    /* Read Status Register1 */
#define OPCODE_RDSR2            0x35    /* Read Status Register2 */

#define OPCODE_WRSR1            0x01    /* Write Status Register */
#define OPCODE_WRSR2            0x31    /* Write Status2 Register eg:gd25q128c*/

#define OPCODE_NORM_READ        0x03    /* Read Data Bytes */
#define OPCODE_FAST_READ        0x0b    /* Read Data Bytes at Higher Speed */

#define OPCODE_PP               0x02    /* Page Program */

#define OPCODE_SE               0x20    /* Sector (4K) Erase */
#define OPCODE_RDID             0x9f    /* Read JEDEC ID */

#define CMD_SIZE 1
#define ADDR_SIZE 3
#define FLASH_PAGESIZE 256
#define FLASH_BUF_SIZE (32 * 1024)
#define ADDR 0x400000

#define MAX_READY_WAIT_JIFFIES  (40 * HZ)   /* 40s max chip erase */

#define FILL_CMD(c, val) do{c[0] = (val);}while(0)
#define FILL_ADDR(c, val, addr) do{ \
        if(4 == addr){  \
            c[CMD_SIZE] = (val) >> 24;  \
            c[CMD_SIZE + 1] = (val) >> 16;  \
            c[CMD_SIZE + 2] = (val) >> 8;   \
            c[CMD_SIZE + 3] = (val);    \
        }else{  \
        c[CMD_SIZE] = (val) >> 16;  \
        c[CMD_SIZE+1] = (val) >> 8; \
        c[CMD_SIZE+2] = (val);      \
        }   \
        }while(0)

#define FILL_DUMMY_DATA(c, val, addr) do{   \
        c[addr + CMD_SIZE] = val >> 16; \
        c[addr + CMD_SIZE + 1] = 0; \
        c[addr + CMD_SIZE + 2] = 0; \
        c[addr + CMD_SIZE + 3] = 0; \
        }while(0)

#define ALIGN_DOWN(a, b)  (((a) / (b)) * (b))

/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);


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
	struct list_head	device_entry;

	/* TX/RX buffers are NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*tx_buffer;
	u8			*rx_buffer;
	u32			speed_hz;
	u8 command[5];
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

static u32 ak_normal_read_sr(struct spidev_data *spidev)
{
    ssize_t retval;
    u8 code;
    u32 status;
    u8 st_tmp= 0;

    /*
    * OPCODE_RDSR1:the first read status cmd
    */
    code = OPCODE_RDSR1;
    if((retval = spi_write_then_read(spidev->spi, &code, 1, &st_tmp, 1))<0){
        return retval;
    }
    status = st_tmp;

    code = OPCODE_RDSR2;
    if((retval = spi_write_then_read(spidev->spi, &code, 1, &st_tmp, 1))<0){
        pr_err("%s,line:%d\n", __func__, __LINE__);
        return retval;
    }
    status = (status | (st_tmp << 8));

    return status;
}

static int wait_till_ready(struct spidev_data *spidev)
{
    unsigned long deadline;
    u32 sr;

    deadline = jiffies + MAX_READY_WAIT_JIFFIES;

    do {
        if ((sr = ak_normal_read_sr(spidev)) < 0)
            break;
        else if (!(sr & (1<<0)))
            return 0;

        cond_resched();

    } while (!time_after_eq(jiffies, deadline));

    return 1;
}

static inline int write_enable(struct spidev_data *spidev)
{
    u8 code = OPCODE_WREN;

    return spi_write_then_read(spidev->spi, &code, 1, NULL, 0);
}

static int spiflash_read(struct spidev_data *spidev, loff_t from, size_t len,
        size_t *retlen, u_char *buf)
{
    struct spi_transfer t[3];
    struct spi_message m;
    void *bounce_buf;
    int addr_size = 3;

    spi_message_init(&m);
    memset(t, 0, (sizeof t));

//    mutex_lock(&spidev->buf_lock);

    bounce_buf =  buf;

    t[0].tx_buf = spidev->command;
    t[0].len = CMD_SIZE;
    spi_message_add_tail(&t[0], &m);

    t[1].tx_buf = &spidev->command[CMD_SIZE];
    t[1].len = addr_size + 1;
    spi_message_add_tail(&t[1], &m);

    t[2].rx_buf = bounce_buf;
    t[2].len = len;
    t[2].cs_change = 0;
    t[2].rx_nbits = 1;
    spi_message_add_tail(&t[2], &m);

    /* Byte count starts at zero. */
    if (retlen)
        *retlen = 0;

    /* Wait till previous write/erase is done. */
    if (wait_till_ready(spidev)) {
        /* REVISIT status return?? */
//        mutex_unlock(&spidev->buf_lock);
        return -EBUSY;
    }

    /* Set up the write data buffer. */
    FILL_CMD(spidev->command, OPCODE_FAST_READ);
    FILL_ADDR(spidev->command, from, addr_size);
    FILL_DUMMY_DATA(spidev->command, 0x00, addr_size);

    spi_sync(spidev->spi, &m);
    *retlen = m.actual_length - (CMD_SIZE + addr_size) - 1;

//    mutex_unlock(&spidev->buf_lock);

    return 0;
}


static int ak_spiflash_read(struct spidev_data *spidev, loff_t from, size_t len,
        size_t *retlen, u_char *buf)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    u32 count = len;
    while(count > 0) {
        xfer_len = (count > FLASH_BUF_SIZE) ? FLASH_BUF_SIZE : count;

        if(xfer_len > FLASH_PAGESIZE)
            xfer_len = ALIGN_DOWN(xfer_len, FLASH_PAGESIZE);

        ret = spiflash_read(spidev, from + offset, xfer_len, &rlen, buf + offset);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }

        *retlen += rlen;
        count -= rlen;
        offset += rlen;
    }
out:
    return ret;
}

static int ak_spiflash_erase(struct spidev_data *spidev, u32 addr)
{
    int addr_size = 3;
    /* Wait until finished previous write command. */
    if (wait_till_ready(spidev))
        return -EBUSY;

    /* Send write enable, then erase commands. */
    write_enable(spidev);

    /* Set up command buffer. */
    spidev->command[0] = OPCODE_SE;
    spidev->command[1] = addr >> 16;
    spidev->command[2] = addr >> 8;
    spidev->command[3] = addr;

    spi_write(spidev->spi, spidev->command, (1 + addr_size));

    return 0;
}

static int ak_spiflash_write(struct spidev_data *spidev, loff_t to, size_t len,
    size_t *retlen, const u_char *buf)
{
    u32 page_offset;
    u32 page_size;
    struct spi_transfer t[3];
    struct spi_message m;
    void *bounce_buf;
    int addr_size = 3;
    pr_debug( "%s: %s %s 0x%08x, len %zd\n",
            dev_name(&spidev->spi->dev), __func__, "to",
            (u32)to, len);

    if (retlen)
        *retlen = 0;

    /* sanity checks */
    if (!len)
        return(0);

    spi_message_init(&m);
    memset(t, 0, (sizeof t));

//    mutex_lock(&spidev->buf_lock);
    bounce_buf = (void *)buf;

    t[0].tx_buf = spidev->command;
    t[0].len = CMD_SIZE;
    spi_message_add_tail(&t[0], &m);

    t[1].tx_buf = &spidev->command[CMD_SIZE];
    t[1].len = addr_size;
    spi_message_add_tail(&t[1], &m);

    t[2].tx_buf = bounce_buf;
    t[2].cs_change = 0;
    t[2].tx_nbits = 1;
    spi_message_add_tail(&t[2], &m);

    /* Wait until finished previous write command. */
    if (wait_till_ready(spidev)) {
//        mutex_unlock(&spidev->buf_lock);
        return 1;
    }

    write_enable(spidev);

    /* Set up the opcode in the write buffer. */
    FILL_CMD(spidev->command, OPCODE_PP);
    FILL_ADDR(spidev->command, to, addr_size);

    /* what page do we start with? */
    page_offset = to % FLASH_PAGESIZE;

    /* do all the bytes fit onto one page? */
    if (page_offset + len <= FLASH_PAGESIZE) {
        t[2].len = len;
        spi_sync(spidev->spi, &m);

        *retlen = m.actual_length - (CMD_SIZE + addr_size);
    } else {
        u32 i;
        /* the size of data remaining on the first page */
        page_size = FLASH_PAGESIZE - page_offset;

        t[2].len = page_size;
        spi_sync(spidev->spi, &m);

        *retlen = m.actual_length - (CMD_SIZE + addr_size);

        /* write everything in PAGESIZE chunks */
        for (i = page_size; i < len; i += page_size) {
            page_size = len - i;
            if (page_size > FLASH_PAGESIZE)
                page_size = FLASH_PAGESIZE;

            /* write the next page to flash */
            FILL_ADDR(spidev->command, to+i, addr_size);

            t[2].tx_buf = buf + i;
            t[2].len = page_size;

            wait_till_ready(spidev);

            write_enable(spidev);

            spi_sync(spidev->spi, &m);

            if (retlen)
                *retlen += m.actual_length - (CMD_SIZE + addr_size);
        }
    }

    pr_debug("ak_spiflash_write: retlen=%d\n", *retlen);

//    mutex_unlock(&spidev->buf_lock);

    return 0;
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
    ak_spiflash_read(spidev, ADDR, count, &status, spidev->rx_buffer);
	if (status > 0) {
		unsigned long	missing;
		missing = copy_to_user(buf, spidev->rx_buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;
	unsigned long		missing;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->tx_buffer, buf, count);
	if (missing == 0){
        ak_spiflash_erase(spidev, ADDR);
        ak_spiflash_write(spidev, ADDR, count, &status, spidev->tx_buffer);
	} else
		status = -EFAULT;
	mutex_unlock(&spidev->buf_lock);

	return status;
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MODE32:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u32 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		retval = __put_user(spidev->speed_hz, (__u32 __user *)arg);
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
	case SPI_IOC_WR_MODE32:
		if (cmd == SPI_IOC_WR_MODE)
			retval = __get_user(tmp, (u8 __user *)arg);
		else
			retval = __get_user(tmp, (u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u16)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u32	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval >= 0)
				spidev->speed_hz = tmp;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
			spi->max_speed_hz = save;
		}
		break;

	default:
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status) {
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));
		goto err_find_dev;
	}

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

	spidev->users++;
	filp->private_data = spidev;
	nonseekable_open(inode, filp);

	mutex_unlock(&device_list_lock);
	return 0;

err_alloc_rx_buf:
	kfree(spidev->tx_buffer);
	spidev->tx_buffer = NULL;
err_find_dev:
	mutex_unlock(&device_list_lock);
	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			dofree;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;

	spin_lock_irq(&spidev->spi_lock);
	/* ... after we unbound from the underlying device? */
	dofree = (spidev->spi == NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* last close? */
	spidev->users--;
	if (!spidev->users) {

		kfree(spidev->tx_buffer);
		spidev->tx_buffer = NULL;

		kfree(spidev->rx_buffer);
		spidev->rx_buffer = NULL;

		if (dofree)
			kfree(spidev);
		else
			spidev->speed_hz = spidev->spi->max_speed_hz;
	}
#ifdef CONFIG_SPI_SLAVE
	if (!dofree)
		spi_slave_abort(spidev->spi);
#endif
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

#ifdef CONFIG_OF
static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{ .compatible = "lineartechnology,ltc2488" },
	{},
};
MODULE_DEVICE_TABLE(of, spidev_dt_ids);
#endif

/*-------------------------------------------------------------------------*/

static int spidev_probe(struct spi_device *spi)
{
	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;

	/*
	 * spidev should never be referenced in DT without a specific
	 * compatible string, it is a Linux implementation thing
	 * rather than a description of the hardware.
	 */
	WARN(spi->dev.of_node &&
	     of_device_is_compatible(spi->dev.of_node, "spidev"),
	     "%pOF: buggy DT: spidev listed directly in DT\n", spi->dev.of_node);

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_ERR_OR_ZERO(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	spidev->speed_hz = spi->max_speed_hz = 50000000;

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);

	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spin_unlock_irq(&spidev->spi_lock);

	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.of_match_table = of_match_ptr(spidev_dt_ids),
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

static int __init spidev_flash_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}
	return status;
}
module_init(spidev_flash_init);

static void __exit spidev_flash_exit(void)
{
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
}
module_exit(spidev_flash_exit);

MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("User mode SPI FLASH device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev-flash");
