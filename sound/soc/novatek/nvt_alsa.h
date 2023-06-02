#ifndef _NVT_ALSA_H
#define _NVT_ALSA_H


struct audio_substream_data {
	struct page *pg;
	unsigned int order;
	u16 num_of_pages;
	u16 direction;
	uint64_t size;
	void __iomem *dai_mmio;
};

#endif
