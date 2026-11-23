// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon JPEG decoder (JPGD): JPEG header parsing and conversion of the
 * quantization and Huffman tables into the format of the table memories.
 *
 * The hardware parses the entropy-coded segment itself; software has to hand
 * it the tables and the frame geometry. Everything that does not fit the
 * hardware (progressive, 12 bit, arithmetic coding, unusual sampling) is
 * refused with -EINVAL instead of being guessed.
 */

#include <linux/bitfield.h>
#include <linux/minmax.h>
#include <linux/string.h>

#include <media/v4l2-jpeg.h>

#include "hisi-jpgd.h"

#define JPEG_SOF0	0xc0

#define HUFF_DC_MAX_SYMBOLS	12
#define HUFF_AC_MAX_SYMBOLS	162

struct huff {
	const u8 *bits;		/* 16 code counts */
	const u8 *val;
	unsigned int nsym;
};

static int huff_load(const struct v4l2_jpeg_reference *ref, unsigned int max_sym,
		     struct huff *h)
{
	unsigned int i, n = 0;

	if (!ref->start || ref->length < 16)
		return -EINVAL;

	for (i = 0; i < 16; i++)
		n += ref->start[i];
	/*
	 * A table is loaded only when a component of the scan selects it, and
	 * every block of the scan needs a DC and an AC code: a table without
	 * any code cannot decode the file.
	 */
	if (!n || n > max_sym || ref->length != 16 + n)
		return -EINVAL;

	h->bits = ref->start;
	h->val = ref->start + 16;
	h->nsym = n;

	return 0;
}

/* Canonical code space check of T.81 Annex C: no length may overflow */
static int huff_check(const struct huff *h)
{
	unsigned int len, code = 0;

	for (len = 1; len <= 16; len++) {
		code += h->bits[len - 1];
		if (code > (1U << len))
			return -EINVAL;
		code <<= 1;
	}

	return 0;
}

/*
 * DC table: one word per symbol (category 0..11): code (low 8 bits) and code
 * length for the luma table in [11:0], for the chroma table in [23:12].
 */
static int fill_dc(u32 *words, const struct huff *h, unsigned int shift)
{
	unsigned int len, k = 0, code = 0, i;

	for (len = 1; len <= 16; len++) {
		for (i = 0; i < h->bits[len - 1]; i++, k++) {
			u8 sym = h->val[k];

			/* the length field has four bits */
			if (sym >= HUFF_DC_MAX_SYMBOLS || len > 15)
				return -EINVAL;
			words[sym] |= (((len & 0xf) << 8) | ((code + i) & 0xff))
				      << shift;
		}
		code = (code + h->bits[len - 1]) << 1;
	}

	return 0;
}

/*
 * AC tables: per code length l = 1..16 the first canonical code ("min") and
 * the symbol index offset ("base"), both reduced to their low 8 bits; the
 * lengths above the longest code get min = 0xff. Packed four bytes to a word
 * (luma l, l + 1, chroma l, l + 1), symbols (luma | chroma << 8) per index.
 */
static void build_ac(const struct huff *h, u8 *min, u8 *base, u32 *sym,
		     unsigned int shift)
{
	unsigned int len, longest = 0, code = 0, before = 0, k;

	for (len = 1; len <= 16; len++)
		if (h->bits[len - 1])
			longest = len;

	for (len = 1; len <= 16; len++) {
		unsigned int cnt = h->bits[len - 1];

		if (len > longest) {
			min[len - 1] = 0xff;
			base[len - 1] = 0;
		} else {
			min[len - 1] = code & 0xff;
			base[len - 1] = cnt ? (before - code) & 0xff : 0;
		}
		before += cnt;
		code = (code + cnt) << 1;
	}

	for (k = 0; k < h->nsym; k++)
		sym[k] |= (u32)h->val[k] << shift;
}

static void pack_ac(u32 *words, const u8 *lu, const u8 *ch)
{
	unsigned int i;

	for (i = 0; i < JPGD_HAC_WORDS; i++)
		words[i] = lu[2 * i] | lu[2 * i + 1] << 8 |
			   ch[2 * i] << 16 | (u32)ch[2 * i + 1] << 24;
}

static int fill_huffman(struct hisi_jpgd_tables *tbl, struct v4l2_jpeg_header *hdr,
			const struct v4l2_jpeg_reference *tabs,
			const struct v4l2_jpeg_reference *ref_tabs,
			unsigned int dc_l, unsigned int ac_l,
			unsigned int dc_c, unsigned int ac_c)
{
	const struct v4l2_jpeg_reference *src = hdr->num_dht ? tabs : ref_tabs;
	struct huff dcl, dcc, acl, acc;
	u8 minl[16], basel[16], minc[16], basec[16];
	int ret;

	/* DC0, DC1, AC0, AC1 */
	if (dc_l > 1 || dc_c > 1 || ac_l > 1 || ac_c > 1)
		return -EINVAL;

	ret = huff_load(&src[dc_l], HUFF_DC_MAX_SYMBOLS, &dcl);
	if (!ret)
		ret = huff_load(&src[dc_c], HUFF_DC_MAX_SYMBOLS, &dcc);
	if (!ret)
		ret = huff_load(&src[2 + ac_l], HUFF_AC_MAX_SYMBOLS, &acl);
	if (!ret)
		ret = huff_load(&src[2 + ac_c], HUFF_AC_MAX_SYMBOLS, &acc);
	if (ret)
		return ret;

	if (huff_check(&dcl) || huff_check(&dcc) || huff_check(&acl) ||
	    huff_check(&acc))
		return -EINVAL;

	memset(tbl->hdc, 0, sizeof(tbl->hdc));
	memset(tbl->hacsym, 0, sizeof(tbl->hacsym));

	ret = fill_dc(tbl->hdc, &dcl, 0);
	if (!ret)
		ret = fill_dc(tbl->hdc, &dcc, 12);
	if (ret)
		return ret;

	build_ac(&acl, minl, basel, tbl->hacsym, 0);
	build_ac(&acc, minc, basec, tbl->hacsym, 8);
	pack_ac(tbl->hacmin, minl, minc);
	pack_ac(tbl->hacbase, basel, basec);

	return 0;
}

static int classify(const struct v4l2_jpeg_frame_header *f,
		    enum hisi_jpgd_sampling *s)
{
	const struct v4l2_jpeg_frame_component_spec *y = &f->component[0];
	const struct v4l2_jpeg_frame_component_spec *u = &f->component[1];
	const struct v4l2_jpeg_frame_component_spec *v = &f->component[2];

	if (f->num_components == 1) {
		*s = HISI_JPGD_S400;
		return 0;
	}
	if (f->num_components != 3)	/* CMYK, YCCK */
		return -EINVAL;

	/* Both chroma components 1x1, luma 1 or 2 in each direction */
	if (u->horizontal_sampling_factor != 1 || u->vertical_sampling_factor != 1 ||
	    v->horizontal_sampling_factor != 1 || v->vertical_sampling_factor != 1)
		return -EINVAL;
	if (y->horizontal_sampling_factor > 2 || y->vertical_sampling_factor > 2)
		return -EINVAL;

	/* 4:4:0 (PICTYPE 5) is not offered */
	if (y->horizontal_sampling_factor == 1 && y->vertical_sampling_factor == 2)
		return -EINVAL;

	if (y->horizontal_sampling_factor == 2 && y->vertical_sampling_factor == 2)
		*s = HISI_JPGD_S420;
	else if (y->horizontal_sampling_factor == 2)
		*s = HISI_JPGD_S422H;
	else
		*s = HISI_JPGD_S444;

	return 0;
}

/**
 * hisi_jpgd_parse - parse a baseline JPEG and convert its tables
 * @var: SoC variant (size limits)
 * @buf: the JPEG file
 * @len: its size
 * @info: receives the geometry
 * @tbl: receives the table memory contents
 *
 * Return: 0, or -EINVAL when the hardware cannot decode the file.
 */
int hisi_jpgd_parse(const struct hisi_jpgd_variant *var, void *buf, size_t len,
		    struct hisi_jpgd_info *info, struct hisi_jpgd_tables *tbl)
{
	struct v4l2_jpeg_reference quant_ref[V4L2_JPEG_MAX_TABLES] = {};
	struct v4l2_jpeg_reference huff_ref[V4L2_JPEG_MAX_TABLES] = {};
	struct v4l2_jpeg_reference ref_huff[V4L2_JPEG_MAX_TABLES] = {};
	struct v4l2_jpeg_scan_header scan = {};
	struct v4l2_jpeg_header hdr = {};
	struct v4l2_jpeg_frame_header *f = &hdr.frame;
	unsigned int i, nc, hf, vf;
	u8 qnat[3][64];
	int ret;

	hdr.scan = &scan;
	hdr.quantization_tables = quant_ref;
	hdr.huffman_tables = huff_ref;

	ret = v4l2_jpeg_parse_header(buf, len, &hdr);
	if (ret < 0)
		return -EINVAL;

	/* SOF0 only: the parser also lets SOF1 (extended sequential) through */
	if (!hdr.sof.start || ((u8 *)hdr.sof.start)[-1] != JPEG_SOF0 ||
	    f->precision != 8 || !hdr.ecs_offset || hdr.ecs_offset >= len)
		return -EINVAL;

	if (f->width < var->min_dim || f->height < var->min_dim ||
	    f->width > var->max_dim || f->height > var->max_dim)
		return -EINVAL;

	ret = classify(f, &info->sampling);
	if (ret)
		return ret;

	/*
	 * Three components with an Adobe APP14 transform of 0 are R, G and B,
	 * not Y, Cb and Cr (T.872 6.5.3). The block has no color conversion and
	 * writes the components as Y/Cr/Cb, so the colors would be wrong
	 * without any error: refuse the file.
	 */
	if (f->num_components == 3 &&
	    hdr.app14_tf == V4L2_JPEG_APP14_TF_CMYK_RGB)
		return -EINVAL;

	nc = f->num_components == 1 ? 1 : 3;

	/* One interleaved scan with the components in frame order */
	if (scan.num_components != nc)
		return -EINVAL;
	for (i = 0; i < nc; i++)
		if (scan.component[i].component_selector !=
		    f->component[i].component_identifier)
			return -EINVAL;
	if (nc == 3 &&
	    (scan.component[1].dc_entropy_coding_table_selector !=
	     scan.component[2].dc_entropy_coding_table_selector ||
	     scan.component[1].ac_entropy_coding_table_selector !=
	     scan.component[2].ac_entropy_coding_table_selector))
		return -EINVAL;

	/* Quantization tables: 8 bit, natural order, Y | U << 8 | V << 16 */
	memset(qnat, 0, sizeof(qnat));
	for (i = 0; i < nc; i++) {
		/* the frame header's selector is a raw byte: only 0..3 exist */
		unsigned int tq = f->component[i].quantization_table_selector;
		const struct v4l2_jpeg_reference *q;
		unsigned int k;

		if (tq >= V4L2_JPEG_MAX_TABLES)
			return -EINVAL;
		q = &quant_ref[tq];
		if (!q->start || q->length != V4L2_JPEG_PIXELS_IN_BLOCK)
			return -EINVAL;
		for (k = 0; k < V4L2_JPEG_PIXELS_IN_BLOCK; k++)
			qnat[i][v4l2_jpeg_zigzag_scan_index[k]] = q->start[k];
	}
	for (i = 0; i < JPGD_QUANT_WORDS; i++)
		tbl->quant[i] = qnat[0][i] | qnat[1][i] << 8 | (u32)qnat[2][i] << 16;

	/*
	 * Huffman tables: the luma slot comes from the Y selectors, the chroma
	 * slot from the U/V selectors; a one component file fills both with
	 * the luma tables. A file without any DHT (Motion-JPEG) uses the
	 * T.81 Annex K tables.
	 */
	ref_huff[0].start = (u8 *)v4l2_jpeg_ref_table_luma_dc_ht;
	ref_huff[0].length = V4L2_JPEG_REF_HT_DC_LEN;
	ref_huff[1].start = (u8 *)v4l2_jpeg_ref_table_chroma_dc_ht;
	ref_huff[1].length = V4L2_JPEG_REF_HT_DC_LEN;
	ref_huff[2].start = (u8 *)v4l2_jpeg_ref_table_luma_ac_ht;
	ref_huff[2].length = V4L2_JPEG_REF_HT_AC_LEN;
	ref_huff[3].start = (u8 *)v4l2_jpeg_ref_table_chroma_ac_ht;
	ref_huff[3].length = V4L2_JPEG_REF_HT_AC_LEN;

	ret = fill_huffman(tbl, &hdr, huff_ref, ref_huff,
			   scan.component[0].dc_entropy_coding_table_selector,
			   scan.component[0].ac_entropy_coding_table_selector,
			   scan.component[nc - 1].dc_entropy_coding_table_selector,
			   scan.component[nc - 1].ac_entropy_coding_table_selector);
	if (ret)
		return ret;

	/* PICSIZE units: 8 pixels, or 16 when the luma is sampled twice */
	info->width = f->width;
	info->height = f->height;
	hf = info->sampling == HISI_JPGD_S400 ? 1 :
	     f->component[0].horizontal_sampling_factor;
	vf = info->sampling == HISI_JPGD_S400 ? 1 :
	     f->component[0].vertical_sampling_factor;
	info->picsize = FIELD_PREP(GENMASK(15, 0), DIV_ROUND_UP(f->width, 8 * hf)) |
			FIELD_PREP(GENMASK(31, 16), DIV_ROUND_UP(f->height, 8 * vf));
	if (info->picsize & var->picsize_bad)
		return -EINVAL;

	info->sampling_reg = (hf << 4 | vf) << 16;
	if (nc == 3)
		info->sampling_reg |= (1 << 4 | 1) << 8 | (1 << 4 | 1);
	info->restart = hdr.restart_interval;
	info->ecs_offset = hdr.ecs_offset;

	return 0;
}
