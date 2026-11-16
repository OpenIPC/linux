#ifndef __ASOC_DMA_H__
#define __ASOC_DMA_H__

struct ingenic_pcm_runtime_data {
	struct snd_pcm_substream *substream;
	struct dma_chan *dma_chan;
	dma_cookie_t cookie;
	unsigned int pos;

	/* some pdma can not be force stop, when a descriptor is transfering */
	struct delayed_work stopdma_delayed_work;
	unsigned long stopdma_delayed_jiffies;
	atomic_t wait_stopdma;

	/* debug interface just use in debug*/
	void *copy_start;
	unsigned int copy_length;
	struct file *file;
	loff_t file_offset;
	mm_segment_t old_fs;
	char* file_name;
	struct work_struct debug_work;
};

static inline struct ingenic_pcm_runtime_data *substream_to_prtd(
	const struct snd_pcm_substream *substream)
{
	return substream->runtime->private_data;
}

#endif
