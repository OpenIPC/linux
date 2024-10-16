/*
 * The citation should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.No warranty is attached;
 *
 * */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#include "esp_rb.h"

int esp_rb_init(esp_rb_t *rb, size_t sz)
{
	init_waitqueue_head(&(rb->wq));

	rb->buf = kmalloc(sz, GFP_KERNEL);
	if (!rb->buf) {
		printk(KERN_ERR "%s, Failed to allocate memory for rb\n", __func__);
		return -ENOMEM;
	}

	rb->end = rb->buf + sz;
	rb->rp = rb->wp = rb->buf;
	rb->size = sz;

	sema_init(&(rb->sem), 1);
	printk(KERN_INFO "ESP RB %px rp %px wp %px init buf %px size %d end %px\n",
			rb, rb->rp, rb->wp, rb->buf, sz, rb->end);
	return 0;
}

ssize_t esp_rb_read_by_user(esp_rb_t *rb, const char __user *buf, size_t sz, int block)
{
	size_t read_len = 0, temp_len = 0;

	if (down_interruptible(&rb->sem)) {
		return -ERESTARTSYS; /* Signal interruption */
	}

	printk(KERN_INFO "ESP RB %px Read: STRT rp %px wp %px free_space: %d\n",
			rb, rb->rp, rb->wp, get_free_space(rb));
	while (rb->rp == rb->wp) {
		up(&rb->sem);
		if (block == 0) {
			return -EAGAIN;
		}
		if (wait_event_interruptible(rb->wq, (rb->rp != rb->wp))) {
			return -ERESTARTSYS; /* Signal interruption */
		}
		if (down_interruptible(&rb->sem)) {
			return -ERESTARTSYS;
		}
	}

	if (rb->wp > rb->rp) {
		read_len = min(sz, (size_t)(rb->wp - rb->rp));
	} else {
		read_len = min(sz, (size_t)(rb->end - rb->rp));
	}

	if (copy_to_user((void *)buf, rb->rp, read_len)) {
		printk(KERN_WARNING "%s, Incomplete/Failed read\n", __func__);
		up(&rb->sem);
		return -EFAULT;
	}

	rb->rp += read_len;
	if (rb->rp == rb->end) {
		rb->rp = rb->buf;
	}

	if (read_len < sz) {
		temp_len = min(sz-read_len, (size_t)(rb->wp - rb->rp));
		printk(KERN_INFO "%s, wrap around read condition \n", __func__);
		if (copy_to_user((void *)buf+read_len, rb->rp, temp_len)) {
			printk(KERN_WARNING "%s, Incomplete/Failed read\n", __func__);
			up(&rb->sem);
			return -EFAULT;
		}
	}

	rb->rp += temp_len;
	read_len += temp_len;

	printk(KERN_INFO "ESP RB %px Read: END  rp %px wp %px free_space: %d\n",
		rb, rb->rp, rb->wp, get_free_space(rb));
	up(&rb->sem);
	return read_len;
}

size_t get_free_space(esp_rb_t *rb)
{
	if (rb->rp == rb->wp) {
		return rb->size - 1;
	} else {
		printk(KERN_INFO "rp %px size %d wp %px calculation %d\n", rb->rp, rb->size, rb->wp, (rb->rp + rb->size - rb->wp));
		return ((rb->rp + rb->size - rb->wp) % rb->size) - 1;
	}
}

ssize_t esp_rb_write_by_kernel(esp_rb_t *rb, const char *buf, size_t sz)
{
	size_t write_len = 0, temp_len = 0;

	if (down_interruptible(&rb->sem)) {
		return -ERESTARTSYS;
	}
	printk(KERN_INFO "%s, Asked write %d on RB\n", __func__, sz);
	printk(KERN_INFO "ESP RB %px Writ: STRT rp %px wp %px free_space: %d\n",
		rb, rb->rp, rb->wp, get_free_space(rb));

	if (get_free_space(rb) == 0) {
		printk(KERN_INFO "ESP RB %px Writ: ERR  rp %px wp %px free_space: %d\n",
			rb, rb->rp, rb->wp, get_free_space(rb));
		printk(KERN_ERR "%s, Ringbuffer full, no space to write\n", __func__);
		up(&rb->sem);
		return 0;
	}

	sz = min(sz, get_free_space(rb));
	if (rb->wp >= rb->rp) {
		write_len = min(sz, (size_t)(rb->end - rb->wp));
		printk(KERN_INFO "Write_len %d size %d end %px wp %px\n", write_len, sz, rb->end, rb->wp);
	} else {
		printk(KERN_INFO "Write_len2 %d size %d end %px wp %px\n", write_len, sz, rb->end, rb->wp);
		write_len = min(sz, (size_t)(rb->rp - rb->wp - 1));
	}

	memcpy(rb->wp, buf, write_len);
	rb->wp += write_len;
	if (rb->wp == rb->end) {
		printk(KERN_INFO "WRAP wp: %px Write_len %d size %d end %px \n", rb->wp, write_len, sz, rb->end);
		rb->wp = rb->buf;
	}

	if (write_len < sz) {
		temp_len = (size_t)min(sz-write_len, (size_t)(rb->rp - rb->wp - 1));
		printk(KERN_INFO "%s, after reaching end write from start", __func__);
		memcpy(rb->wp, buf+write_len, temp_len);
	}

	rb->wp += temp_len;
	write_len += temp_len;

	printk(KERN_INFO "ESP RB %px Writ: END  rp %px wp %px free_space: %d temp_len %d\n",
		rb, rb->rp, rb->wp, get_free_space(rb), temp_len);
	up(&rb->sem);

	wake_up_interruptible(&rb->wq);

	printk(KERN_INFO "%s, write %d on RB\n", __func__, write_len);
	return write_len;
}

void esp_rb_del(esp_rb_t *rb)
{
	printk(KERN_INFO "ESP RB %px del\n", rb);
	kfree(rb->buf);
	rb->buf = rb->end = rb->rp = rb->wp = NULL;
	rb->size = 0;
	return;
}

void esp_rb_cleanup(esp_rb_t *rb)
{
	printk(KERN_INFO "ESP RB %px cleanup\n", rb);
	if (down_interruptible(&rb->sem)) {
		return;
	}

	rb->rp = rb->wp;

	up(&rb->sem);
	return;
}
