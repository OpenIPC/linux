/**
    JPEG header information.

    @file       jpg_header.h

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

*/
#ifndef _JPEHEADER_H
#define _JPEHEADER_H

#if defined (__UITRON)
#include "SysKer.h"
#include "JpgHeader.h"
#include "Type.h"

#elif defined __KERNEL__

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#define debug_msg        printk

#else  //__FREERTOS
#include "kwrap/type.h"
#include "kwrap/platform.h"
#endif



/**
     @name JPEG header size.
*/
//@{
#define JPG_HEADER_SIZE    (sizeof(JPG_HEADER) - 6) ///< Refer to #_JPG_HEADER.
//@}

/**
    Huffman table DC
*/
typedef _PACKED_BEGIN struct _HUFF_TBL_DC {
	UINT8    index;          ///< table index
	UINT8    symbol_nums[16];   ///< symbol numbers
	UINT8    symbol_data[12];   ///< symbol data
} _PACKED_END HUFF_TBL_DC;

/**
    Huffman table AC
*/
typedef _PACKED_BEGIN struct _HUFF_TBL_AC {
	UINT8    index;          ///< table index
	UINT8    symbol_nums[16];   ///< symbol numbers
	UINT8    symbol_data[162];  ///< symbol data
} _PACKED_END HUFF_TBL_AC;

/**
    quantization table
*/
typedef _PACKED_BEGIN struct _Q_TABLE {
	UINT8    index;          ///< table index
	UINT8    q_data[64];        ///< quantization data
} _PACKED_END Q_TABLE;

/**
    component info of SOF
*/
typedef _PACKED_BEGIN struct _SOF_COMPINFO {
	UINT8    id;        ///< component ID
	UINT8    sample_hv_rate;       ///< Horizontal & Vertical rate
	UINT8    q_tbl_index; ///< Q-table index
} _PACKED_END SOF_COMPINFO;

/**
    component info of SOS
*/
typedef _PACKED_BEGIN struct _SOS_COMPINFO {
	UINT8    id;             ///< component ID
	UINT8    huf_tbl_index;    ///< AC & DC Huffman table index
} _PACKED_END SOS_COMPINFO;

/**
    DHT tag
*/
typedef _PACKED_BEGIN struct _TAG_DHT {
	UINT8           tag_id[2];         ///< tag ID
	UINT8           tag_length[2];     ///< tag length
	HUFF_TBL_DC    huff_dc_0th;        ///< 00->0th DC Huffman table (Y)
	HUFF_TBL_AC    huff_ac_0th;        ///< 10->0th AC Huffman table (Y)
	HUFF_TBL_DC    huff_dc_1th;        ///< 01->1th DC Huffman table (UV)
	HUFF_TBL_AC    huff_ac_1th;        ///< 11->1th AC Huffman table (UV)
} _PACKED_END TAG_DHT;    // Huffman tables

/**
    DQT tag
*/
typedef _PACKED_BEGIN struct _TAG_DQT {
	UINT8           tag_id[2];       ///< tag ID
	UINT8           tag_length[2];   ///< tag length
	Q_TABLE        q_tbl_0th;        ///< 00: 0th Q-table
	Q_TABLE        q_tbl_1th;        ///< 01: 1th Q-table
} _PACKED_END TAG_DQT;    // Quantization tables

/**
    SOF tag
*/
typedef _PACKED_BEGIN struct _TAG_SOF {
	UINT8           tag_id[2];       ///< tag ID
	UINT8           tag_length[2];   ///< tag length
	UINT8           bit_per_pixel;   ///< bits per pixel per color component
	UINT8           img_height[2];   ///< image height
	UINT8           img_width[2];    ///< image width
	UINT8           component_nums;  ///< number of color components
	SOF_COMPINFO    component1;     ///< component 1
	SOF_COMPINFO    component2;     ///< component 2
	SOF_COMPINFO    component3;     ///< component 3
} _PACKED_END TAG_SOF;    // Start of Frame

/**
    SOS tag
*/
typedef _PACKED_BEGIN struct _TAG_SOS {
	UINT8           tag_id[2];       ///< tag ID
	UINT8           tag_length[2];   ///< tag length
	UINT8           component_nums;  ///< number of color components
	SOS_COMPINFO    component1;     ///< component 1
	SOS_COMPINFO    component2;     ///< component 2
	SOS_COMPINFO    component3;     ///< component 3
	UINT8           other_data[3];
} _PACKED_END TAG_SOS;    // Start of Scan

typedef _PACKED_BEGIN struct _TAG_DRI {
	UINT8           tag_id[2];       ///< tag ID
	UINT8           tag_length[2];   ///< tag length
	UINT16          interval;        ///< restart interval
} _PACKED_END TAG_DRI;    // Quantization tables

/**
    JPEG header
*/
typedef _PACKED_BEGIN struct _JPG_HEADER {
	UINT16        soi;              ///< SOI marker
	TAG_DQT       mark_dqt;          ///< DQT marker
	TAG_SOF       mark_sof;          ///< SOF marker
	TAG_DHT       mark_dht;          ///< DHT marker
	TAG_SOS       mark_sos;          ///< SOS marker
	TAG_DRI       mark_dri;          ///< DRI marker
} _PACKED_END JPG_HEADER, *PJPG_HEADER;

/**
    YUV format
*/
typedef enum _JPG_YUV_FORMAT {
	JPG_FMT_YUV211  = 0,   ///< YCbCr 2h11
	JPG_FMT_YUV422  = 0,   ///< YCbCr 2h11
	JPG_FMT_YUV420  = 1,   ///< YCbCr 411
	JPG_FMT_YUV211V = 3,   ///< YCbCr 2v11
	JPG_FMT_YUV100  = 4,   ///< YCbCr 100 (Y plane only)
	ENUM_DUMMY4WORD(JPG_YUV_FORMAT)
} JPG_YUV_FORMAT;


/**
    Decode JPEG type.

    One jpeg file contains one primary image, one 160x120 thumbnail image,
    and one screennail image.
    For MJPEG AVI file, the frame inside also a JPEG file.
*/
typedef enum _JPG_DEC_TYPE {
	DEC_PRIMARY,        ///< decode primary image
	DEC_THUMBNAIL,      ///< decode thumbnail, or decode primary image
	DEC_HIDDEN,         ///< decode hidden thumb, or decode primary image
	DEC_THUMB_ONLY,     ///< decode thumbnail only
} JPG_DEC_TYPE;

/**
   Error Code.

   Error code for parsing JPG header.
*/
typedef enum _JPG_HEADER_ER {
	JPG_HEADER_ER_OK        =   0x0000,      ///< JPEG decode OK
	JPG_HEADER_ER_MARKER    =   0xFFFF,      ///< invalid or reserved marker
	JPG_HEADER_ER_SOI_NF    =   0xD8FF,      ///< SOI marker not found
	JPG_HEADER_ER_DQT_LEN   =   0xDB00,      ///< invalid DQT segment length
	JPG_HEADER_ER_DQT_TYPE  =   0xDB01,      ///< unknown quant table type
	JPG_HEADER_ER_SOF_NBL   =   0xC1CF,      ///< encoding mode is not baseline
	JPG_HEADER_ER_SOF_P     =   0xC004,      ///< sample precision != 8
	JPG_HEADER_ER_SOF_NC    =   0xC005,      ///< num components/frame != 3
	JPG_HEADER_ER_SOF_SFY   =   0xC005,      ///< MCU format != YUV420/422
	JPG_HEADER_ER_DHT_LEN   =   0xC400,      ///< invalid DHT segment length
	JPG_HEADER_ER_DHT_TYPE  =   0xC401,      ///< unknown huffman table type
	JPG_HEADER_ER_SOS_NF    =   0xDAFF,      ///< SOS marker not found
	JPG_HEADER_ER_RST_NF    =   0xD0FF,      ///< RST marker not found
	JPG_HEADER_ER_EOI_NF    =   0xD9FF,      ///< EOI marker not found
	JPG_HEADER_ER_APP1      =   0xE1FF,      ///< EOI marker not found
	JPG_HEADER_ER_AVI_NF    =   0x5249,      ///< AVI marker not found
	JPG_HEADER_ER_APP2      =   0xE2FF,      ///< APP2 error
	ENUM_DUMMY4WORD(JPG_HEADER_ER)
} JPG_HEADER_ER;

/**
    The prototype of callback function.
*/
typedef JPG_HEADER_ER (*PARSE_EXIF_CB)(UINT32 buf, UINT32 *p_thumboffset, UINT32 *p_thumbsize);

/**
    JPEG decode configuration.
*/
typedef struct _JPGHEAD_DEC_CFG {
	UINT32  headerlen;          ///< [out]length of header
	UINT8   *inbuf;             ///< [in]input buffer start address
	UINT8   *p_q_tbl_y;            ///< [out]start addr of 0th-Quantization table
	UINT8   *p_q_tbl_uv;           ///< [out]start addr of 1st-Quantization table
	UINT8   *p_q_tbl_uv2;          ///< [out]start addr of 2nd-Quantization table
	UINT8   *p_huff_dc0th;        ///< [out]start addr of 0th-Huff-DC table
	UINT8   *p_huff_dc1th;        ///< [out]start addr of 1st-Huff-DC table
	UINT8   *p_huff_ac0th;        ///< [out]start addr of 0th-Huff-AC table
	UINT8   *p_huff_ac1th;        ///< [out]start addr of 1st-Huff-AC table
	UINT8   *p_tag_sof;           ///< [out]start addr of SOF (image-width, image-height, image-format)
	UINT16  imageheight;        ///< [out]image height
	UINT16  imagewidth;         ///< [out]image width
	UINT16  numcomp;            ///< [out]number of components
	UINT16  scanfreq[3][2];     ///< [out]h and v freqs for each of 3 components
	UINT16  qtype[3];           ///< [out]quantization table type for each component
	JPG_YUV_FORMAT  fileformat; ///< [out]file format, refer to JPG_YUV_FORMAT
	UINT16  rstinterval;        ///< [out]restart interval (initially set to 0)
	UINT16  lineoffset_y;        ///< [out]Y  line offset after scaling
	UINT16  lineoffset_uv;       ///< [out]UV line offset after scaling
	UINT16  ori_imageheight;    ///< [out]ori-image height before scaling
	UINT16  ori_imagewidth;     ///< [out]ori-image width  before scaling
	BOOL    speedup_sn;       ///< [in]speed up Screennail
} JPGHEAD_DEC_CFG, *PJPGHEAD_DEC_CFG;

/**
    Parse JPEG header and get information from JPEG header.

    @param[in,out] jdcfg_p JPEG decoding info: #_JPGHEAD_DEC_CFG.
    @param[in] dec_type JPEG decoding type: #_JPG_DEC_TYPE.
    @return JPEG decoding error code: #_JPG_HEADER_ER.
*/
extern JPG_HEADER_ER jpg_parse_header(JPGHEAD_DEC_CFG *jdcfg_p, JPG_DEC_TYPE dec_type, PARSE_EXIF_CB parse_exif_cb);

/**
    Check if the image format supported.

    @param[in] comp_num - Component number
    @param[in] scan_freq[3][2] - Component format
    @param[out] p_file_format - Supported format
    @return result
        - @b FALSE - Format not support
        - @b TRUE - Format support
*/
extern BOOL jpg_supported_format(UINT16 comp_num, UINT16 scan_freq[3][2], JPG_YUV_FORMAT *p_file_format);

#endif//_JPEHEADER_H
