/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/log2.h>
#include <linux/uaccess.h>
#include "osal_ax.h"
#include "osal_kfifo.h"
#include "osal_dev_ax.h"


/*
 * internal helper to calculate the unused elements in a fifo
 */
static inline unsigned int ax_kfifo_unused(struct __ax_kfifo *fifo)
{
	return (fifo->mask + 1) - (fifo->in - fifo->out);
}

int __ax_kfifo_alloc(struct __ax_kfifo *fifo, unsigned int size,
		size_t esize, gfp_t gfp_mask)
{
	/*
	 * round up to the next power of 2, since our 'let the indices
	 * wrap' technique works only in this case.
	 */
	size = roundup_pow_of_two(size);

	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = esize;

	if (size < 2) {
		fifo->data = NULL;
		fifo->mask = 0;
		return -EINVAL;
	}

	fifo->data = kmalloc_array(esize, size, gfp_mask);

	if (!fifo->data) {
		fifo->mask = 0;
		return -ENOMEM;
	}
	fifo->mask = size - 1;

	return 0;
}

EXPORT_SYMBOL(__ax_kfifo_alloc);

void __ax_kfifo_free(struct __ax_kfifo *fifo)
{
	kfree(fifo->data);
	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = 0;
	fifo->data = NULL;
	fifo->mask = 0;
}

EXPORT_SYMBOL(__ax_kfifo_free);

int __ax_kfifo_init(struct __ax_kfifo *fifo, void *buffer,
		unsigned int size, size_t esize)
{
	size /= esize;

	if (!is_power_of_2(size))
		size = rounddown_pow_of_two(size);

	fifo->in = 0;
	fifo->out = 0;
	fifo->esize = esize;
	fifo->data = buffer;

	if (size < 2) {
		fifo->mask = 0;
		return -EINVAL;
	}
	fifo->mask = size - 1;

	return 0;
}

EXPORT_SYMBOL(__ax_kfifo_init);

static void ax_kfifo_copy_in(struct __ax_kfifo *fifo, const void *src,
		unsigned int len, unsigned int off)
{
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(fifo->data + off, src, l);
	memcpy(fifo->data, src + l, len - l);
	/*
	 * make sure that the data in the fifo is up to date before
	 * incrementing the fifo->in index counter
	 */
	smp_wmb();
}

unsigned int __ax_kfifo_in(struct __ax_kfifo *fifo,
		const void *buf, unsigned int len)
{
	unsigned int l;

	l = ax_kfifo_unused(fifo);
	if (len > l)
		len = l;

	ax_kfifo_copy_in(fifo, buf, len, fifo->in);
	fifo->in += len;
	return len;
}

EXPORT_SYMBOL(__ax_kfifo_in);

static void ax_kfifo_copy_out(struct __ax_kfifo *fifo, void *dst,
		unsigned int len, unsigned int off)
{
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;

	off &= fifo->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	memcpy(dst, fifo->data + off, l);
	memcpy(dst + l, fifo->data, len - l);
	/*
	 * make sure that the data is copied before
	 * incrementing the fifo->out index counter
	 */
	smp_wmb();
}

unsigned int __ax_kfifo_out_peek(struct __ax_kfifo *fifo,
		void *buf, unsigned int len)
{
	unsigned int l;

	l = fifo->in - fifo->out;
	if (len > l)
		len = l;

	ax_kfifo_copy_out(fifo, buf, len, fifo->out);
	return len;
}

EXPORT_SYMBOL(__ax_kfifo_out_peek);

unsigned int __ax_kfifo_out(struct __ax_kfifo *fifo,
		void *buf, unsigned int len)
{
	len = __ax_kfifo_out_peek(fifo, buf, len);
	fifo->out += len;
	return len;
}

EXPORT_SYMBOL(__ax_kfifo_out);

static unsigned long ax_kfifo_copy_from_user(struct __ax_kfifo *fifo,
	const void __user *from, unsigned int len, unsigned int off,
	unsigned int *copied)
{
	unsigned int size = fifo->mask + 1;
	unsigned int esize = fifo->esize;
	unsigned int l;
	unsigned long ret;

	off &= fifo->mask;
	if (esize != 1) {
		off *= esize;
		size *= esize;
		len *= esize;
	}
	l = min(len, size - off);

	ret = copy_from_user(fifo->data + off, from, l);
	if (unlikely(ret))
		ret = DIV_ROUND_UP(ret + len - l, esize);
	else {
		ret = copy_from_user(fifo->data, from + l, len - l);
		if (unlikely(ret))
			ret = DIV_ROUND_UP(ret, esize);
	}
	/*
	 * make sure that the data in the fifo is up to date before
	 * incrementing the fifo->in index counter
	 */
	smp_wmb();
	*copied = len - ret * esize;
	/* return the number of elements which are not copied */
	return ret;
}


int __ax_kfifo_from_user(struct __ax_kfifo *fifo, const void __user *from,
		unsigned long len, unsigned int *copied)
{
	unsigned int l;
	unsigned long ret;
	unsigned int esize = fifo->esize;
	int err;

	if (esize != 1)
		len /= esize;

	l = ax_kfifo_unused(fifo);
	if (len > l)
		len = l;

	ret = ax_kfifo_copy_from_user(fifo, from, len, fifo->in, copied);
	if (unlikely(ret)) {
		len -= ret;
		err = -EFAULT;
	} else
		err = 0;
	fifo->in += len;
	return err;
}

EXPORT_SYMBOL(__ax_kfifo_from_user);


unsigned int __ax_kfifo_max_r(unsigned int len, size_t recsize)
{
	unsigned int max = (1 << (recsize << 3)) - 1;

	if (len > max)
		return max;
	return len;
}
EXPORT_SYMBOL(__ax_kfifo_max_r);

#define	__ax_kfifo_PEEK(data, out, mask) \
	((data)[(out) & (mask)])
/*
 * __ax_kfifo_peek_n internal helper function for determinate the length of
 * the next record in the fifo
 */
static unsigned int __ax_kfifo_peek_n(struct __ax_kfifo *fifo, size_t recsize)
{
	unsigned int l;
	unsigned int mask = fifo->mask;
	unsigned char *data = fifo->data;

	l = __ax_kfifo_PEEK(data, fifo->out, mask);

	if (--recsize)
		l |= __ax_kfifo_PEEK(data, fifo->out + 1, mask) << 8;

	return l;
}

#define	__ax_kfifo_POKE(data, in, mask, val) \
	( \
	(data)[(in) & (mask)] = (unsigned char)(val) \
	)

/*
 * __ax_kfifo_poke_n internal helper function for storing the length of
 * the record into the fifo
 */
static void __ax_kfifo_poke_n(struct __ax_kfifo *fifo, unsigned int n, size_t recsize)
{
	unsigned int mask = fifo->mask;
	unsigned char *data = fifo->data;

	__ax_kfifo_POKE(data, fifo->in, mask, n);

	if (recsize > 1)
		__ax_kfifo_POKE(data, fifo->in + 1, mask, n >> 8);
}

unsigned int __ax_kfifo_len_r(struct __ax_kfifo *fifo, size_t recsize)
{
	return __ax_kfifo_peek_n(fifo, recsize);
}

EXPORT_SYMBOL(__ax_kfifo_len_r);

unsigned int __ax_kfifo_in_r(struct __ax_kfifo *fifo, const void *buf,
		unsigned int len, size_t recsize)
{
	if (len + recsize > ax_kfifo_unused(fifo))
		return 0;

	__ax_kfifo_poke_n(fifo, len, recsize);

	ax_kfifo_copy_in(fifo, buf, len, fifo->in + recsize);
	fifo->in += len + recsize;
	return len;
}

EXPORT_SYMBOL(__ax_kfifo_in_r);

static unsigned int ax_kfifo_out_copy_r(struct __ax_kfifo *fifo,
	void *buf, unsigned int len, size_t recsize, unsigned int *n)
{
	*n = __ax_kfifo_peek_n(fifo, recsize);

	if (len > *n)
		len = *n;

	ax_kfifo_copy_out(fifo, buf, len, fifo->out + recsize);
	return len;
}


unsigned int __ax_kfifo_out_peek_r(struct __ax_kfifo *fifo, void *buf,
		unsigned int len, size_t recsize)
{
	unsigned int n;

	if (fifo->in == fifo->out)
		return 0;

	return ax_kfifo_out_copy_r(fifo, buf, len, recsize, &n);
}

EXPORT_SYMBOL(__ax_kfifo_out_peek_r);

unsigned int __ax_kfifo_out_r(struct __ax_kfifo *fifo, void *buf,
		unsigned int len, size_t recsize)
{
	unsigned int n;

	if (fifo->in == fifo->out)
		return 0;

	len = ax_kfifo_out_copy_r(fifo, buf, len, recsize, &n);
	fifo->out += n + recsize;
	return len;
}

EXPORT_SYMBOL(__ax_kfifo_out_r);

void __ax_kfifo_skip_r(struct __ax_kfifo *fifo, size_t recsize)
{
	unsigned int n;

	n = __ax_kfifo_peek_n(fifo, recsize);
	fifo->out += n + recsize;
}

EXPORT_SYMBOL(__ax_kfifo_skip_r);
