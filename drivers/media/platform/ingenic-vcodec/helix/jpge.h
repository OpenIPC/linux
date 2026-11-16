#ifndef __JPGE_H__
#define __JPGE_H__

#include "api/helix_jpeg_enc.h"

struct jpge_params {
	int compr_quality;

	int width;
	int height;
	int format;	/*input format?*/
};

struct jpge_ctx {
	int vdma_chain_len;

	unsigned int *desc;
	dma_addr_t desc_pa;

	struct video_frame_buffer *frame;	/* raw frame.*/
	struct ingenic_vcodec_mem *bs;		/* output bs.*/

	unsigned int header_size;		/*jpg header*/
	unsigned int bslen;			/*encoder output bslen*/

	struct jpge_params p;
	_JPEGE_SliceInfo *s;
	void *priv;
};


extern int jpeg_encoder_encode(struct jpge_ctx *ctx, struct video_frame_buffer *frame, struct ingenic_vcodec_mem *bs);

extern int jpeg_encoder_set_fmt(struct jpge_ctx *ctx, int width, int height, int format);

extern int jpeg_encoder_alloc_workbuf(struct jpge_ctx *ctx);

extern int jpeg_encoder_free_workbuf(struct jpge_ctx *ctx);

extern int jpeg_encoder_init(struct jpge_ctx *ctx);

extern int jpeg_encoder_deinit(struct jpge_ctx *ctx);

extern void jpeg_encoder_set_priv(struct jpge_ctx *ctx, void *data);

#endif
