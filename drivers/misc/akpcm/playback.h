#ifndef _AKPCM_PLAYBACK_H_
#define _AKPCM_PLAYBACK_H_

static inline int is_playback_opened(struct akpcm *pcm)
{
	return test_bit(STATUS_BIT_OPENED, &(pcm->playback_sm_flag));
}

static inline int is_playback_ready(struct akpcm *pcm)
{
	return test_bit(STATUS_BIT_PREPARED, &(pcm->playback_sm_flag));
}

static inline int has_playback_stream(struct akpcm *pcm)
{
	return test_bit(IO_BIT_STREAM, &(pcm->outflag));
}

static inline int is_playback_working(struct akpcm *pcm)
{
	return test_bit(IO_BIT_DMA, &(pcm->outflag));
}

/**
 * @brief  start to playback
 * @author panqihe 2014-06-26
 * @modify
 * @return void
 */
void playback_start_dma(struct akpcm *pcm);

/**
 * @brief  stop output process for timer_stop_output
 * @author Cheng Mingjuan
 * @modify panqihe 2014-06-11
 * @return void
 */
void playback_stop_hw(unsigned long data);

int playback_start_force(struct akpcm *pcm);

/**
 * @brief  pause operation for playback
 * @author panqihe 2014-06-12
 * @modify
 * @return void
 */
int playback_pause(struct akpcm *pcm);

long playback_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
ssize_t playback_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos);
int playback_open(struct inode *inode, struct file *filp);
int playback_close(struct inode *inode, struct file *filp);
int check_playback_aec_ptr(void);

#endif
