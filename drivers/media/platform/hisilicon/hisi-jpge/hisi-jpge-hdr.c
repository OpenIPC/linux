// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon JPEG encoder (JPGE): JFIF header and quantization tables
 *
 * The JPGE only produces the entropy-coded scan data (with fixed, standard
 * Huffman tables built into the hardware) plus a 64 byte packet descriptor.
 * Everything in front of the scan data is generated here. The Huffman tables
 * and the base quantization tables are the example tables of ITU-T T.81
 * Annex K, shared through v4l2-jpeg.
 */

#include <linux/align.h>
#include <linux/minmax.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/unaligned.h>

#include <media/v4l2-jpeg.h>

#include "hisi-jpge.h"

/**
 * hisi_jpge_scale_quant() - derive both quantization tables from a quality
 * @quality: 1..100 (IJG scale)
 * @luma: output, 64 entries, natural order
 * @chroma: output, 64 entries, natural order
 */
void hisi_jpge_scale_quant(unsigned int quality, u8 *luma, u8 *chroma)
{
	unsigned int scale, i;

	quality = clamp(quality, 1U, 100U);
	scale = quality < 50 ? 5000 / quality : 200 - 2 * quality;

	for (i = 0; i < 64; i++) {
		luma[i] = clamp((v4l2_jpeg_ref_table_luma_qt[i] * scale + 50) / 100, 1U, 255U);
		chroma[i] = clamp((v4l2_jpeg_ref_table_chroma_qt[i] * scale + 50) / 100, 1U, 255U);
	}
}

/**
 * hisi_jpge_qpt_words() - pack a quantization table for the JPGE_QPT window
 * @quant: 64 entries in natural order
 * @words: output, 16 register values
 *
 * Word n holds the entries s, s + 8, s + 16 and s + 24 (low byte first) with
 * s = (n & 1) * 32 + n / 2, i.e. columns of the two table halves. This
 * follows the HiSTB SDK driver for the same register family; the Hi3520D V100
 * data sheet has no register tables for the JPGE.
 *
 * Board facts (2026-11-10/11): the window reads back 0 after a write, so it is
 * write-only (or its read port is not wired). The packing is confirmed with a
 * textured picture: re-encoding the source with "divide by the header table,
 * entry for entry" reproduces the hardware scan byte for byte, which is only
 * true if every table entry sits in the position the packing above gives it.
 * With PICFG roundEn clear the hardware quantizer truncates toward zero where
 * libjpeg rounds, so at equal quality the output is smaller and 2 to 3 dB
 * lower in PSNR; that is the block, not the table. The driver sets roundEn,
 * which makes the block round like libjpeg.
 */
void hisi_jpge_qpt_words(const u8 *quant, u32 *words)
{
	unsigned int n, s;

	for (n = 0; n < 16; n++) {
		s = (n & 1) * 32 + (n >> 1);
		words[n] = quant[s] | quant[s + 8] << 8 |
			   quant[s + 16] << 16 | (u32)quant[s + 24] << 24;
	}
}

/*
 * Header sizes. The fixed part is SOI, DQT x2, SOF0 and SOS; APP0, the COM
 * comment and the four DHT segments are optional. The whole header is padded
 * with a COM segment to a multiple of 64 bytes.
 */
#define SOI_LEN		2
#define APP0_LEN	18
#define DQT_LEN		(2 + 2 + 1 + 64)
#define SOF_LEN		(2 + 17)
#define SOS_LEN		(2 + 12)
#define DRI_LEN		(2 + 4)
#define DHT_LEN		(4 * (2 + 2 + 1) + 2 * V4L2_JPEG_REF_HT_DC_LEN + \
			 2 * V4L2_JPEG_REF_HT_AC_LEN)
#define COM_TEXT	"HiSilicon JPGE"
#define COM_LEN		(4 + sizeof(COM_TEXT))

static unsigned int hdr_raw_len(u32 markers, unsigned int restart)
{
	unsigned int len = SOI_LEN + 2 * DQT_LEN + SOF_LEN + SOS_LEN;

	if (markers & V4L2_JPEG_ACTIVE_MARKER_APP0)
		len += APP0_LEN;
	if (markers & V4L2_JPEG_ACTIVE_MARKER_DHT)
		len += DHT_LEN;
	if (markers & V4L2_JPEG_ACTIVE_MARKER_COM)
		len += COM_LEN;
	if (restart)
		len += DRI_LEN;

	return len;
}

/**
 * hisi_jpge_header_len() - size of the header for a set of options
 * @markers: V4L2_JPEG_ACTIVE_MARKER_* bits
 * @restart: MCUs per restart interval, 0 for none
 *
 * Return: header size in bytes, a multiple of HISI_JPGE_HDR_ALIGN. A padding
 * COM segment needs at least 4 bytes, so a gap of 1..3 bytes grows by one more
 * alignment unit.
 */
unsigned int hisi_jpge_header_len(u32 markers, unsigned int restart)
{
	unsigned int raw = hdr_raw_len(markers, restart);
	unsigned int len = ALIGN(raw, HISI_JPGE_HDR_ALIGN);

	if (len - raw && len - raw < 4)
		len += HISI_JPGE_HDR_ALIGN;

	return len;
}

static u8 *put_marker(u8 *p, u8 marker, unsigned int len)
{
	*p++ = 0xff;
	*p++ = marker;
	put_unaligned_be16(len, p);

	return p + 2;
}

static u8 *put_dqt(u8 *p, u8 id, const u8 *quant)
{
	unsigned int k;

	p = put_marker(p, 0xdb, 2 + 1 + 64);
	*p++ = id;		/* 8 bit precision */
	for (k = 0; k < 64; k++)
		*p++ = quant[v4l2_jpeg_zigzag_scan_index[k]];

	return p;
}

/* @ht is a v4l2-jpeg reference table: 16 code length counts, then the values */
static u8 *put_dht(u8 *p, u8 class_id, const u8 *ht, unsigned int len)
{
	p = put_marker(p, 0xc4, 2 + 1 + len);
	*p++ = class_id;
	memcpy(p, ht, len);

	return p + len;
}

/**
 * hisi_jpge_build_header() - write the JFIF header in front of the scan data
 * @buf: buffer of at least hisi_jpge_header_len() bytes
 * @hdr: picture size, sampling, quantization tables and options
 *
 * Layout: SOI, APP0 (JFIF, optional), COM (optional), DQT x2 (Y = 0, Cb and
 * Cr = 1), SOF0, DHT x4 (optional, the fixed Huffman tables of the hardware:
 * T.81 Annex K), DRI (only with restart intervals), padding COM, SOS.
 * The picture size in SOF0 is the real size, not rounded up to whole MCUs: the
 * hardware pads the last MCU row and column itself, a decoder crops it.
 *
 * Return: the header size, always equal to hisi_jpge_header_len() for the same
 * options.
 */
unsigned int hisi_jpge_build_header(u8 *buf, const struct hisi_jpge_hdr *hdr)
{
	unsigned int len = hisi_jpge_header_len(hdr->markers, hdr->restart);
	unsigned int raw = hdr_raw_len(hdr->markers, hdr->restart);
	u8 *p = buf, *end = buf + len;
	u8 samp;

	switch (hdr->sampling) {
	case 444:
		samp = 0x11;
		break;
	case 422:
		samp = 0x21;
		break;
	default:
		samp = 0x22;
		break;
	}

	/* SOI */
	*p++ = 0xff;
	*p++ = 0xd8;

	if (hdr->markers & V4L2_JPEG_ACTIVE_MARKER_APP0) {
		/* APP0 JFIF 1.01, no units, 1:1 aspect, no thumbnail */
		p = put_marker(p, 0xe0, 16);
		memcpy(p, "JFIF\0", 5);
		p += 5;
		*p++ = 1;
		*p++ = 1;
		*p++ = 0;
		put_unaligned_be16(1, p);
		put_unaligned_be16(1, p + 2);
		p += 4;
		*p++ = 0;
		*p++ = 0;
	}

	if (hdr->markers & V4L2_JPEG_ACTIVE_MARKER_COM) {
		p = put_marker(p, 0xfe, COM_LEN - 2);
		memcpy(p, COM_TEXT, sizeof(COM_TEXT));
		p += sizeof(COM_TEXT);
	}

	p = put_dqt(p, 0, hdr->luma);
	p = put_dqt(p, 1, hdr->chroma);

	/* SOF0: baseline, 8 bit, Y/Cb/Cr */
	p = put_marker(p, 0xc0, 17);
	*p++ = 8;
	put_unaligned_be16(hdr->height, p);
	put_unaligned_be16(hdr->width, p + 2);
	p += 4;
	*p++ = 3;
	*p++ = 1;
	*p++ = samp;
	*p++ = 0;
	*p++ = 2;
	*p++ = 0x11;
	*p++ = 1;
	*p++ = 3;
	*p++ = 0x11;
	*p++ = 1;

	if (hdr->markers & V4L2_JPEG_ACTIVE_MARKER_DHT) {
		p = put_dht(p, V4L2_JPEG_LUM_HT | V4L2_JPEG_DC_HT,
			    v4l2_jpeg_ref_table_luma_dc_ht,
			    V4L2_JPEG_REF_HT_DC_LEN);
		p = put_dht(p, V4L2_JPEG_LUM_HT | V4L2_JPEG_AC_HT,
			    v4l2_jpeg_ref_table_luma_ac_ht,
			    V4L2_JPEG_REF_HT_AC_LEN);
		p = put_dht(p, V4L2_JPEG_CHR_HT | V4L2_JPEG_DC_HT,
			    v4l2_jpeg_ref_table_chroma_dc_ht,
			    V4L2_JPEG_REF_HT_DC_LEN);
		p = put_dht(p, V4L2_JPEG_CHR_HT | V4L2_JPEG_AC_HT,
			    v4l2_jpeg_ref_table_chroma_ac_ht,
			    V4L2_JPEG_REF_HT_AC_LEN);
	}

	if (hdr->restart) {
		p = put_marker(p, 0xdd, 4);
		put_unaligned_be16(hdr->restart, p);
		p += 2;
	}

	/* Padding COM segment (4 bytes minimum) up to the aligned size */
	if (len > raw) {
		unsigned int pad = len - raw;

		p = put_marker(p, 0xfe, pad - 2);
		memset(p, 0, pad - 4);
		p += pad - 4;
	}

	/* SOS: three components, DC/AC tables 0/0 for Y and 1/1 for Cb, Cr */
	p = put_marker(p, 0xda, 12);
	*p++ = 3;
	*p++ = 1;
	*p++ = 0x00;
	*p++ = 2;
	*p++ = 0x11;
	*p++ = 3;
	*p++ = 0x11;
	*p++ = 0;
	*p++ = 63;
	*p++ = 0;

	WARN_ON_ONCE(p != end);

	return len;
}
