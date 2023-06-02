#ifndef _AKPCM_CAPTURE_H_
#define _AKPCM_CAPTURE_H_

#include <linux/irq.h>

static inline int is_capture_opened(struct akpcm *pcm)
{
	return test_bit(STATUS_BIT_OPENED, &(pcm->capture_sm_flag));
}

static inline int is_capture_ready(struct akpcm *pcm)
{
	return test_bit(STATUS_BIT_PREPARED, &(pcm->capture_sm_flag));
}

static inline int has_capture_stream(struct akpcm *pcm)
{
	return test_bit(IO_BIT_STREAM, &(pcm->inflag));
}

static inline int is_capture_working(struct akpcm *pcm)
{
	return test_bit(IO_BIT_DMA, &(pcm->inflag));
}

void LininDet_wq_work(struct work_struct *work);
irqreturn_t linindet_interrupt(int irq, void *dev_id);

/**
 * @brief  resume operation for capture
 * @author panqihe 2014-06-12
 * @modify
 * @return void
 */
void capture_start(struct akpcm *pcm);

void capture_pause(struct akpcm *pcm);
long capture_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
ssize_t capture_read(struct file *filp, char __user *buf, size_t count,
                    loff_t*f_pos);
int capture_open(struct inode *inode, struct file *filp);
int capture_close(struct inode *inode, struct file *filp);

#endif
