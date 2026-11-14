/*
 * u_uac1.c -- ALSA audio utilities for Gadget stack
 *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/random.h>
#include <linux/syscalls.h>

#include "u_ak_uac1.h"

#define UAC_DEV_MAX 4 // two capture dev and two playback dev
#define UAC_CLASS   "uac"

static int uac_major = 0;
static struct class *uac_class;

static int uac_cdev_setup(struct gaudio *card);
static void uac_cdev_cleanup(struct gaudio *card);

struct f_audio_buf *f_audio_buffer_alloc(int buf_size)
{
	struct f_audio_buf *copy_buf;

	copy_buf = kzalloc(sizeof *copy_buf, GFP_ATOMIC);
	if (!copy_buf)
		return ERR_PTR(-ENOMEM);

    INIT_LIST_HEAD(&copy_buf->list);

	copy_buf->buf = kzalloc(buf_size, GFP_ATOMIC);
	if (!copy_buf->buf) {
		kfree(copy_buf);
		return ERR_PTR(-ENOMEM);
	}

	return copy_buf;
}

void f_audio_buffer_free(struct f_audio_buf *audio_buf)
{
    if (audio_buf->buf)
        kfree(audio_buf->buf);
    if (audio_buf)
        kfree(audio_buf);
}

#define GET_BYTES_NOLOCK(chnnel) \
int get_##chnnel##_bytes_nolock(struct gaudio_snd_dev *snd) \
{ \
    struct f_audio_buf *buf; \
    int ret = 0; \
    if (!list_empty(&snd->chnnel##_queue)) { \
        list_for_each_entry(buf, &snd->chnnel##_queue, list) { \
            ret += buf->actual; \
        } \
    } \
    if (snd->chnnel##_buf) { \
        ret += snd->chnnel##_buf->actual; \
    } \
    return ret; \
}

GET_BYTES_NOLOCK(data);
GET_BYTES_NOLOCK(control);

#define GET_BUFFER_COUNT_NOLOCK(chnnel) \
int get_##chnnel##_buffer_count_nolock(struct gaudio_snd_dev *snd) \
{ \
    struct f_audio_buf *buf; \
    int ret = 0; \
    if (!list_empty(&snd->chnnel##_queue)) { \
        list_for_each_entry(buf, &snd->chnnel##_queue, list) \
            ret += 1; \
    } \
    if (snd->chnnel##_buf) \
        ret += 1; \
    return ret; \
}

GET_BUFFER_COUNT_NOLOCK(data);
GET_BUFFER_COUNT_NOLOCK(control);

#define DEVICE_OPEN(func, channel) \
static int func##_##channel##_open(struct inode *inode, struct file *filp) \
{ \
    struct gaudio_snd_dev *snd = container_of(inode->i_cdev, \
            struct gaudio_snd_dev, channel##_cdev); \
    filp->private_data = snd; \
    spin_lock(&snd->channel##_lock); \
    if (snd->channel##_opened) { \
        spin_unlock(&snd->channel##_lock); \
        ERROR(snd->card, #func " " #channel " file already opened!"); \
        return -EPERM; \
    } \
    snd->channel##_opened = 1; \
    spin_unlock(&snd->channel##_lock); \
    return 0; \
}

#define DEVICE_CLOSE(func, channel) \
static int func##_##channel##_close(struct inode *inode, struct file *filp) \
{ \
    struct gaudio_snd_dev *snd = filp->private_data; \
    spin_lock(&snd->channel##_lock); \
    if (!snd->channel##_opened) { \
        spin_unlock(&snd->channel##_lock); \
        ERROR(snd->card, #func " " #channel " file not opened!"); \
        return -EPERM; \
    } \
    snd->channel##_opened = 0; \
    spin_unlock(&snd->channel##_lock); \
    wake_up_interruptible(&snd->channel##_wq); \
    return 0; \
}

#define DEVICE_READ(func, channel) \
static ssize_t func##_##channel##_read(struct file *filp, char __user *buf, \
    size_t count, loff_t *f_pos) \
{ \
    struct gaudio_snd_dev *snd = filp->private_data; \
    struct f_audio_buf *audio_buf, *tmp_audio_buf; \
    int bytes = 0; \
    size_t n = 0; \
    unsigned long flags; \
    if (count == 0) \
        return 0; \
    spin_lock_irqsave(&snd->channel##_lock, flags); \
    if (!snd->channel##_opened) { \
        spin_unlock_irqrestore(&snd->channel##_lock, flags); \
        ERROR(snd->card, #func " " #channel " file not opened!"); \
        return -EPERM; \
    } \
    if (get_##channel##_bytes_nolock(snd) <= snd->channel##_read_threshold) { \
        if(filp->f_flags & O_NONBLOCK) { \
            spin_unlock_irqrestore(&snd->channel##_lock, flags); \
            return -EAGAIN; \
        } \
    } \
    do { \
        spin_unlock_irqrestore(&snd->channel##_lock, flags); \
        if (wait_event_interruptible(snd->channel##_wq, \
                    (get_##channel##_bytes_nolock(snd) > snd->channel##_read_threshold || !snd->channel##_opened)) < 0) \
            return -ERESTARTSYS; \
        spin_lock_irqsave(&snd->channel##_lock, flags); \
        if (get_##channel##_bytes_nolock(snd) > snd->channel##_read_threshold || !snd->channel##_opened) \
            break; \
    } while (1); \
    if (!snd->channel##_opened) { \
        spin_unlock_irqrestore(&snd->channel##_lock, flags); \
        return -EPERM; \
    } \
    if (!list_empty(&snd->channel##_queue) && (count > 0)) { \
        list_for_each_entry_safe(audio_buf, tmp_audio_buf, &snd->channel##_queue, list) { \
            bytes = (count > audio_buf->actual)? audio_buf->actual: count; \
            spin_unlock_irqrestore(&snd->channel##_lock, flags); \
            if (copy_to_user(buf, audio_buf->buf, bytes)) \
                return -EFAULT; \
            spin_lock_irqsave(&snd->channel##_lock, flags); \
            audio_buf->actual -= bytes; \
            if (audio_buf->actual) { \
                memcpy(audio_buf->buf, audio_buf->buf + bytes, audio_buf->actual); \
            } else { \
                list_del_init(&audio_buf->list); \
                f_audio_buffer_free(audio_buf); \
            } \
            count -= bytes; \
            n += bytes; \
            if (count == 0) \
                break; \
        } \
    } \
    if (snd->channel##_buf && (count > 0)) { \
        audio_buf = snd->channel##_buf; \
        bytes = (count > audio_buf->actual)? audio_buf->actual: count; \
        spin_unlock_irqrestore(&snd->channel##_lock, flags); \
        if (copy_to_user(buf, audio_buf->buf, bytes)) \
            return -EFAULT; \
        spin_lock_irqsave(&snd->channel##_lock, flags); \
        audio_buf->actual -= bytes; \
        if (audio_buf->actual) { \
            memcpy(audio_buf->buf, audio_buf->buf + bytes, audio_buf->actual); \
        } else { \
            if (snd->channel##_buf != audio_buf) { /*maybe buffer is added to queue*/ \
                list_del_init(&audio_buf->list); \
                f_audio_buffer_free(audio_buf); \
            } \
        } \
        count -= bytes; \
        n += bytes; \
    } \
    spin_unlock_irqrestore(&snd->channel##_lock, flags); \
    return n; \
}

#define DEVICE_READ_POLL(func, channel) \
static unsigned int func##_##channel##_poll(struct file *filp, struct poll_table_struct *wait) \
{ \
    unsigned int mask = 0; \
    unsigned long flags; \
    struct gaudio_snd_dev *snd = filp->private_data; \
    spin_lock_irqsave(&snd->channel##_lock, flags); \
    poll_wait(filp, &snd->channel##_wq, wait); \
    if (get_##channel##_bytes_nolock(snd) > snd->channel##_read_threshold) { \
        mask |= POLLIN | POLLRDNORM; \
    } \
    spin_unlock_irqrestore(&snd->channel##_lock, flags); \
    return mask; \
}

DEVICE_READ_POLL(capture, control);
DEVICE_READ_POLL(playback, control);
DEVICE_READ_POLL(playback, data);

#define DEVICE_WRITE_POLL(f, channel) \
static unsigned int f##_##channel##_poll(struct file *filp, struct poll_table_struct *wait) \
{ \
    unsigned int mask = 0; \
    unsigned long flags; \
    struct gaudio_snd_dev *snd = filp->private_data; \
    struct f_uac1_opts *opts = container_of(snd->card->func.fi, struct f_uac1_opts, func_inst); \
    spin_lock_irqsave(&snd->channel##_lock, flags); \
    poll_wait(filp, &snd->channel##_wq, wait); \
    if (snd->channel##_running && snd->channel##_writable \
            && (snd->capture_req_count <= opts->capture_req_max_count)) { \
        mask |= POLLOUT | POLLWRNORM; \
    } \
    spin_unlock_irqrestore(&snd->channel##_lock, flags); \
    return mask; \
}

DEVICE_WRITE_POLL(capture, data);

#define COPY_TO_BUFFER_NOLOCK(channel) \
int copy_to_##channel##_buffer_nolock(struct gaudio_snd_dev *snd, void *data, unsigned int n) \
{ \
    struct f_uac1_opts *opts = container_of(snd->card->func.fi, struct f_uac1_opts, func_inst); \
    int audio_buf_size = opts->audio_buf_size; \
    struct f_audio_buf *buf; \
    buf = snd->channel##_buf; \
    if (!buf) { \
        ERROR(snd->card, #channel " buf is NULL!\n"); \
        return -EINVAL; \
    } \
    if (n > audio_buf_size) { \
        ERROR(snd->card, "copy too much data to audio buf!\n"); \
        return -EINVAL; \
    } \
    /* Copy buffer is full, add it to the play_queue */ \
    if (audio_buf_size - buf->actual < n) { \
        list_add_tail(&buf->list, &snd->channel##_queue); \
        buf = f_audio_buffer_alloc(audio_buf_size); \
        if (IS_ERR(buf)) { \
            return -ENOMEM; \
        } \
    } \
    memcpy(buf->buf + buf->actual, data, n); \
    buf->actual += n; \
    snd->channel##_buf = buf; \
    return 0; \
}

COPY_TO_BUFFER_NOLOCK(control);

DEVICE_OPEN(capture, data);
DEVICE_CLOSE(capture, data);

static ssize_t capture_data_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *f_pos)
{
    struct gaudio_snd_dev *snd = filp->private_data;
    struct f_audio *audio = container_of(snd->card, struct f_audio, card);
    struct f_uac1_opts *opts = container_of(snd->card->func.fi, struct f_uac1_opts, func_inst);
    unsigned long flags;
    int err = 0;
    struct usb_request *req;
    //int audio_buf_size = opts->audio_buf_size;
    //struct f_audio_buf *data_buf;

    spin_lock_irqsave(&snd->data_lock, flags);
    if (!snd->data_opened) {
        spin_unlock_irqrestore(&snd->data_lock, flags);
        ERROR(snd->card, "capture data file not opened!");
        return -EPERM;
    }
    if (!snd->data_writable || !snd->data_running) {
        if (filp->f_flags & O_NONBLOCK) {
            spin_unlock_irqrestore(&snd->data_lock, flags);
            return -EAGAIN;
        }
    }
    if (snd->data_writable && snd->data_running
            && (snd->capture_req_count > opts->capture_req_max_count)) {
        DBG(snd->card, "capture request count %d is too large!\n", snd->capture_req_count);

        if (filp->f_flags & O_NONBLOCK) {
            spin_unlock_irqrestore(&snd->data_lock, flags);
            return -EAGAIN;
        }
    }

    if (!(filp->f_flags & O_NONBLOCK)) {
        do {
            spin_unlock_irqrestore(&snd->data_lock, flags);
            if (wait_event_interruptible(snd->data_wq,
                    ((snd->data_writable && snd->data_running
                      && (snd->capture_req_count <= opts->capture_req_max_count))
                     || !snd->data_opened)) < 0)
                return -ERESTARTSYS;
            spin_lock_irqsave(&snd->data_lock, flags);
            if ((snd->data_writable && snd->data_running
                    && (snd->capture_req_count <= opts->capture_req_max_count))
                    || !snd->data_opened)
                break;
        } while (1);
    }

    if (!snd->data_opened) {
        spin_unlock_irqrestore(&snd->data_lock, flags);
        return -EPERM;
    }

    spin_unlock_irqrestore(&snd->data_lock, flags);

    req = usb_ep_alloc_request(snd->card->in_ep, GFP_ATOMIC);
    if (req) {
        req->buf = kzalloc(count, GFP_ATOMIC);
        if (req->buf) {
            req->length = count;
            req->context = audio;
            req->complete = f_audio_complete;
            if (copy_from_user(req->buf, buf, count)) {
                kfree(req->buf);
                usb_ep_free_request(snd->card->in_ep, req);
                return -EFAULT;
            }
            err = usb_ep_queue(snd->card->in_ep, req, GFP_ATOMIC);
            if (err) {
                ERROR(snd->card, "%s queue req failed, return %d\n",
                        snd->card->in_ep->name, err);
                usb_ep_free_request(snd->card->in_ep, req);
            } else {
                snd->capture_req_count++;
            }
        } else {
            usb_ep_free_request(snd->card->in_ep, req);
            err = -ENOMEM;
        }
    } else
        err = -ENOMEM;

    return (err == 0)? count: err;
}

DEVICE_OPEN(capture, control);
DEVICE_CLOSE(capture, control);
DEVICE_READ(capture, control);

DEVICE_OPEN(playback, data);
DEVICE_CLOSE(playback, data);
DEVICE_READ(playback, data);

DEVICE_OPEN(playback, control);
DEVICE_CLOSE(playback, control);
DEVICE_READ(playback, control);

static const struct file_operations capture_data_fops = {
    .owner   = THIS_MODULE,
    .open    = capture_data_open,
    .release = capture_data_close,
    .write   = capture_data_write,
    .poll    = capture_data_poll,
};

static const struct file_operations capture_control_fops = {
    .owner   = THIS_MODULE,
    .open    = capture_control_open,
    .release = capture_control_close,
    .read    = capture_control_read,
    .poll    = capture_control_poll,
};

static const struct file_operations playback_data_fops = {
    .owner   = THIS_MODULE,
    .open    = playback_data_open,
    .release = playback_data_close,
    .read    = playback_data_read,
    .poll    = playback_data_poll,
};

static const struct file_operations playback_control_fops = {
    .owner   = THIS_MODULE,
    .open    = playback_control_open,
    .release = playback_control_close,
    .read    = playback_control_read,
    .poll    = playback_control_poll,
};

static int gaudio_open_snd_dev(struct gaudio *card)
{
	struct f_uac1_opts *opts;
    struct gaudio_snd_dev *snd;
    int err;

	opts = container_of(card->func.fi, struct f_uac1_opts, func_inst);

	card->playback.card = card;
	card->capture.card = card;

    err = uac_cdev_setup(card);
    if (!err) {
        snd = &card->playback;
        snd->control_buf = f_audio_buffer_alloc(opts->audio_buf_size);
        if (IS_ERR(snd->control_buf)) {
            ERROR(card, "alloc control buffer failed!\n");
            uac_cdev_cleanup(card);
            return -ENOMEM;
        }

        INIT_LIST_HEAD(&snd->data_queue);
        INIT_LIST_HEAD(&snd->control_queue);
        init_waitqueue_head(&snd->data_wq);
        init_waitqueue_head(&snd->control_wq);
        snd->control_running = 1;
        snd->data_running = 0;
        if (opts->playback_min_read_bytes > 0)
            snd->data_read_threshold = opts->playback_min_read_bytes - 1;

        snd = &card->capture;
        snd->control_buf = f_audio_buffer_alloc(opts->audio_buf_size);
        if (IS_ERR(snd->control_buf)) {
            ERROR(card, "alloc control buffer failed!\n");
            uac_cdev_cleanup(card);
            return -ENOMEM;
        }

        INIT_LIST_HEAD(&snd->data_queue);
        INIT_LIST_HEAD(&snd->control_queue);
        init_waitqueue_head(&snd->data_wq);
        init_waitqueue_head(&snd->control_wq);
        snd->control_running = 1;
        snd->data_running = 0;
    }

    return err;
}

static void gaudio_cleanup_snd_dev(struct gaudio_snd_dev *snd)
{
    struct f_audio_buf *buf, *tmp_buf;
    if (!list_empty(&snd->data_queue)) {
        list_for_each_entry_safe(buf, tmp_buf, &snd->data_queue, list) {
            list_del_init(&buf->list);
            f_audio_buffer_free(buf);
        }
    }
    if (snd->data_buf) {
        f_audio_buffer_free(snd->data_buf);
    }
    if (!list_empty(&snd->control_queue)) {
        list_for_each_entry_safe(buf, tmp_buf, &snd->control_queue, list) {
            list_del_init(&buf->list);
            f_audio_buffer_free(buf);
        }
    }
    if (snd->control_buf) {
        f_audio_buffer_free(snd->control_buf);
    }
}

static void gaudio_close_snd_dev(struct gaudio *card)
{
    gaudio_cleanup_snd_dev(&card->playback);
    gaudio_cleanup_snd_dev(&card->capture);
    uac_cdev_cleanup(card);
}

static int uac_class_setup(struct gaudio *card)
{
    int err = 0;
    dev_t devno;

    uac_class = class_create(THIS_MODULE, UAC_CLASS);
    if(IS_ERR(uac_class)) {
        err = PTR_ERR(uac_class);
        ERROR(card, "can't register %s class\n", UAC_CLASS);
        goto exit_class_setup;
    }

    if (uac_major) {
        devno = MKDEV(uac_major, 0);
        err = register_chrdev_region(devno, UAC_DEV_MAX, "uac");
        if (err < 0) {
            ERROR(card, "cannot register uac chrdev region, major %d\n", uac_major);
            goto exit_class_setup;
        }
    } else {
        err = alloc_chrdev_region(&devno, 0, UAC_DEV_MAX, "uac");
        if (err < 0) {
            ERROR(card, "cannot alloc uac chrdev region\n");
            goto exit_class_setup;
        }
        uac_major = MAJOR(devno);
    }

    DBG(card, "uac_major %d\n", uac_major);

    return 0;

exit_class_setup:
    class_destroy(uac_class);

    return err;
}

static void uac_class_cleanup(struct gaudio *card)
{
    unregister_chrdev_region(uac_major, UAC_DEV_MAX);
    if (uac_class) {
        class_destroy(uac_class);
        uac_class = NULL;
    }
}

static int uac_cdev_setup(struct gaudio *card)
{
    int err = 0;
    dev_t devno;

    cdev_init(&card->capture.control_cdev, &capture_control_fops);
    cdev_init(&card->capture.data_cdev, &capture_data_fops);
    cdev_init(&card->playback.control_cdev, &playback_control_fops);
    cdev_init(&card->playback.data_cdev, &playback_data_fops);

    devno = MKDEV(uac_major, 0);
    err = cdev_add(&card->capture.control_cdev, devno, 1);
    if (err)
        return err;
    card->capture.control_dev = device_create(uac_class, NULL, devno, &card->capture, "uacc%d", 0);
    if (IS_ERR(card->capture.control_dev)) {
        err = PTR_ERR(card->capture.control_dev);
        ERROR(card, "can't create uacc%d", 0);
        cdev_del(&card->capture.control_cdev);
        return err;
    }

    devno = MKDEV(uac_major, 1);
    err = cdev_add(&card->capture.data_cdev, devno, 1);
    if (err) {
        goto err_capture_data;
    }
    card->capture.data_dev = device_create(uac_class, NULL, devno, &card->capture, "uacc%d", 1);
    if (IS_ERR(card->capture.data_dev)) {
        err = PTR_ERR(card->capture.data_dev);
        ERROR(card, "can't create uacc%d", 1);
        cdev_del(&card->capture.data_cdev);
        goto err_capture_data;
    }

    devno = MKDEV(uac_major, 2);
    err = cdev_add(&card->playback.control_cdev, devno, 1);
    if (err) {
        goto err_playback_control;
    }
    card->playback.control_dev = device_create(uac_class, NULL, devno, &card->playback, "uacp%d", 0);
    if (IS_ERR(card->playback.control_dev)) {
        err = PTR_ERR(card->playback.control_dev);
        ERROR(card, "can't create uacp%d", 0);
        cdev_del(&card->playback.control_cdev);
        goto err_playback_control;
    }

    devno = MKDEV(uac_major, 3);
    err = cdev_add(&card->playback.data_cdev, devno, 1);
    if (err) {
        goto err_playback_data;
    }
    card->playback.data_dev = device_create(uac_class, NULL, devno, &card->playback, "uacp%d", 1);
    if (IS_ERR(card->playback.data_dev)) {
        err = PTR_ERR(card->playback.data_dev);
        ERROR(card, "can't create uacp%d", 1);
        cdev_del(&card->playback.data_cdev);
        goto err_playback_data;
    }

    DBG(card, "create uac device files ok\n");

    return 0;

err_capture_data:
    cdev_del(&card->playback.control_cdev);
    device_destroy(uac_class, 2);
err_playback_control:
    cdev_del(&card->capture.data_cdev);
    device_destroy(uac_class, 1);
err_playback_data:
    cdev_del(&card->capture.control_cdev);
    device_destroy(uac_class, 0);

    return err;
}

static void uac_cdev_cleanup(struct gaudio *card)
{
    cdev_del(&card->playback.data_cdev);
    device_destroy(uac_class, 3);
    device_del(card->playback.data_dev);

    cdev_del(&card->playback.control_cdev);
    device_destroy(uac_class, 2);
    device_del(card->playback.control_dev);

    cdev_del(&card->capture.data_cdev);
    device_destroy(uac_class, 1);
    device_del(card->capture.data_dev);

    cdev_del(&card->capture.control_cdev);
    device_destroy(uac_class, 0);
    device_del(card->capture.control_dev);
}

/**
 * gaudio_setup - setup ALSA interface and preparing for USB transfer
 *
 * This sets up PCM, mixer or MIDI ALSA devices fore USB gadget using.
 *
 * Returns negative errno, or zero on success
 */
int gaudio_setup(struct gaudio *card)
{
	int	ret;

    ret = uac_class_setup(card);
    if (ret)
        return ret;

	ret = gaudio_open_snd_dev(card);
	if (ret) {
		ERROR(card, "we need at least one control device\n");
        uac_class_cleanup(card);
        return -ENODEV;
    }

    return 0;
}

/**
 * gaudio_cleanup - remove ALSA device interface
 *
 * This is called to free all resources allocated by @gaudio_setup().
 */
void gaudio_cleanup(struct gaudio *card)
{
    DBG(card, "%s\n", __func__);

	if (card)
		gaudio_close_snd_dev(card);

    uac_class_cleanup(card);
}

