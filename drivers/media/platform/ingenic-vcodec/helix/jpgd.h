#ifndef __JPGD_H__
#define __JPGD_H__


#include "api/helix_jpeg_dec.h"

struct jpgd_params {
	int hea[64];
	int heb[336];
	int huffmin[64];
	int qt[4][64];

	int huffenc[384];
	int min[64];
	unsigned int pxc[4];	/* component x config */

	int width;
	int height;
	int format;	/*input format?*/
};

struct jpgd_ctx {
	int vdma_chain_len;

	unsigned int *desc;
	dma_addr_t desc_pa;

	struct video_frame_buffer *frame;	/* raw frame.*/
	struct ingenic_vcodec_mem *bs;		/* output bs.*/

	unsigned int header_size;		/*jpg header*/
	unsigned int bslen;			/*decoder output bslen*/

	struct jpgd_params p;
	_JPEGD_SliceInfo *s;
	void *priv;
};


extern int jpeg_decoder_decode(struct jpgd_ctx *ctx, struct ingenic_vcodec_mem *bs, struct video_frame_buffer *frame);

extern int jpeg_decoder_set_fmt(struct jpgd_ctx *ctx, int width, int height, int format);

extern int jpeg_decoder_alloc_workbuf(struct jpgd_ctx *ctx);

extern int jpeg_decoder_free_workbuf(struct jpgd_ctx *ctx);

extern int jpeg_decoder_init(struct jpgd_ctx *ctx);

extern int jpeg_decoder_deinit(struct jpgd_ctx *ctx);

extern void jpeg_decoder_set_priv(struct jpgd_ctx *ctx, void *data);

#endif
