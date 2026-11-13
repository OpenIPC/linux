/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Kernel-side extension of <uapi/linux/spi/spidev.h> for the SigmaStar
 * in-kernel MSPI client API (drivers/spi/spidev.c, drivers/sstar/mspi).
 *
 * The vendor SDK put these declarations into the uapi header itself, so
 * headers_install exported struct mutex, spinlock_t and the kernel-only
 * u8/u16/u32 into every toolchain sysroot and no userspace program could
 * include <linux/spi/spidev.h> (OpenIPC/firmware#2836). Kernel code finds
 * this file before the uapi one, so its includes are unchanged.
 */
#ifndef _LINUX_SPI_SPIDEV_H
#define _LINUX_SPI_SPIDEV_H

#include <uapi/linux/spi/spidev.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/mutex.h>

struct spi_device;

struct spidev_cfg {
	u32		max_speed_hz;
	u8		bits_per_word;
	u16		mode;
};

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
};

struct mspi_ioc_transfer {
	u8		*tx_buf;
	u8		*rx_buf;

	u32		len;
	u32		speed_hz;

	u16		delay_usecs;
	u8		bits_per_word;
	u8		cs_change;
	u8		tx_nbits;
	u8		rx_nbits;
	u16		pad;
};

struct spidev_data *mspidev_open(int mspi_nr);
int mspidev_release(struct spidev_data *mspidev);
int mspidev_set(struct spidev_data *mspidev, struct spidev_cfg *mspidev_cfg);
ssize_t mspidev_write(struct spidev_data *mspidev, const char *buf, size_t count);
ssize_t mspidev_read(struct spidev_data *mspidev, char *buf, size_t count);
int mspidev_transfer(struct spidev_data *mspidev, struct mspi_ioc_transfer *m_xfers, unsigned n_xfers);

#endif /* _LINUX_SPI_SPIDEV_H */
